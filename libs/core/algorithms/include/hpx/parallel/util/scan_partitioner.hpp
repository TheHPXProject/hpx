//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2015 Daniel Bourgeois
//  Copyright (c)      2017 Taeguk Kwon
//  Copyright (c)      2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/parallel/util/detail/chunk_size.hpp>
#include <hpx/parallel/util/detail/handle_local_exceptions.hpp>
#include <hpx/parallel/util/detail/scoped_executor_parameters.hpp>
#include <hpx/parallel/util/detail/select_partitioner.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/modules/async_local.hpp>
#endif

#include <algorithm>
#include <cstddef>
#include <exception>
#include <list>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // The static partitioner simply spawns one chunk of iterations for
        // each available core.
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename R,
            typename Result1, typename Result2>
        struct scan_static_partitioner
        {
            using parameters_type = typename ExPolicy::executor_parameters_type;
            using executor_type = typename ExPolicy::executor_type;

            using scoped_executor_parameters =
                detail::scoped_executor_parameters_ref<parameters_type,
                    executor_type>;

            using handle_local_exceptions =
                detail::handle_local_exceptions<ExPolicy>;

            template <typename ExPolicy_, typename FwdIter, typename T,
                typename F1, typename F2, typename F3, typename F4>
            static R call([[maybe_unused]] ExPolicy_ policy,
                [[maybe_unused]] FwdIter first,
                [[maybe_unused]] std::size_t count, [[maybe_unused]] T&& init,
                [[maybe_unused]] F1&& f1, [[maybe_unused]] F2&& f2,
                [[maybe_unused]] F3&& f3, [[maybe_unused]] F4&& f4)
            {
#if defined(HPX_COMPUTE_DEVICE_CODE)
                HPX_ASSERT(false);
                return R();
#else
                // inform parameter traits
                scoped_executor_parameters scoped_params(
                    policy.parameters(), policy.executor());

                std::vector<hpx::shared_future<Result1>> workitems;
                std::vector<hpx::future<Result2>> finalitems;
                std::vector<Result1> f2results;
                std::list<std::exception_ptr> errors;
                try
                {
                    // pre-initialize first intermediate result
                    workitems.push_back(
                        make_ready_future(HPX_FORWARD(T, init)));

                    HPX_ASSERT(count > 0);
                    FwdIter first_ = first;
                    std::size_t const count_ = count;

                    // estimate a chunk size based on number of cores used
                    using has_variable_chunk_size = typename hpx::execution::
                        experimental::extract_has_variable_chunk_size<
                            parameters_type>::type;

                    std::size_t cores = 1;
                    auto shape = detail::get_bulk_iteration_shape(
                        has_variable_chunk_size(), policy, workitems, f1, first,
                        count, cores, 1);

                    // schedule every chunk on a separate thread
                    std::size_t size = hpx::util::size(shape);

                    // If the size of count was enough to warrant testing for a
                    // chunk, pre-initialize second intermediate result and
                    // start f3.
                    if (workitems.size() == 2)
                    {
                        HPX_ASSERT(count_ > count);

                        workitems.reserve(size + 2);
                        finalitems.reserve(size + 1);

                        finalitems.push_back(
                            execution::async_execute(policy.executor(), f3,
                                first_, count_ - count, workitems[0].get()));

                        workitems[1] = make_ready_future(HPX_INVOKE(
                            f2, workitems[0].get(), workitems[1].get()));
                    }
                    else
                    {
                        workitems.reserve(size + 1);
                        finalitems.reserve(size);
                    }

                    // Schedule first step of scan algorithm, step 2 is
                    // performed when all f1 tasks are done
                    for (auto const& elem : shape)
                    {
                        auto curr = execution::async_execute(policy.executor(),
                            f1, hpx::get<0>(elem), hpx::get<1>(elem))
                                        .share();

                        workitems.push_back(HPX_MOVE(curr));
                    }

                    // Wait for all f1 tasks to finish
                    if (hpx::wait_all_nothrow(workitems))
                    {
                        handle_local_exceptions::call(workitems, errors);
                    }

                    // perform f2 sequentially in one go
                    f2results.resize(workitems.size());
                    auto result = workitems[0].get();
                    f2results[0] = result;
                    for (std::size_t i = 1; i < workitems.size(); i++)
                    {
                        result = HPX_INVOKE(f2, result, workitems[i].get());
                        f2results[i] = result;
                    }

                    // start all f3 tasks
                    std::size_t i = 0;
                    for (auto const& elem : shape)
                    {
                        finalitems.push_back(execution::async_execute(
                            policy.executor(), f3, hpx::get<0>(elem),
                            hpx::get<1>(elem), f2results[i]));
                        i++;
                    }

                    scoped_params.mark_end_of_scheduling();
                }
                catch (...)
                {
                    handle_local_exceptions::call(
                        std::current_exception(), errors);
                }
                return reduce(HPX_MOVE(f2results), HPX_MOVE(finalitems),
                    HPX_MOVE(errors), HPX_FORWARD(F4, f4));
#endif
            }

        private:
            template <typename F>
            static R reduce([[maybe_unused]] std::vector<Result1>&& workitems,
                [[maybe_unused]] std::vector<hpx::future<Result2>>&& finalitems,
                [[maybe_unused]] std::list<std::exception_ptr>&& errors,
                [[maybe_unused]] F&& f)
            {
#if defined(HPX_COMPUTE_DEVICE_CODE)
                HPX_ASSERT(false);
                return R();
#else
                // wait for all tasks to finish
                if (hpx::wait_all_nothrow(finalitems) || !errors.empty())
                {
                    // always rethrow if 'errors' is not empty or 'finalitems'
                    // have an exceptional future
                    handle_local_exceptions::call(finalitems, errors);
                }

                try
                {
                    return f(HPX_MOVE(workitems), HPX_MOVE(finalitems));
                }
                catch (...)
                {
                    // rethrow either bad_alloc or exception_list
                    handle_local_exceptions::call(std::current_exception());
                }

                HPX_UNREACHABLE;    //-V779
#endif
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename R,
            typename Result1, typename Result2>
        struct scan_task_static_partitioner
        {
            template <typename ExPolicy_, typename FwdIter, typename T,
                typename F1, typename F2, typename F3, typename F4>
            static hpx::future<R> call(ExPolicy_&& policy, FwdIter first,
                std::size_t count, T&& init, F1&& f1, F2&& f2, F3&& f3, F4&& f4)
            {
                return execution::async_execute(policy.executor(),
                    [first, count, policy, init = HPX_FORWARD(T, init),
                        f1 = HPX_FORWARD(F1, f1), f2 = HPX_FORWARD(F2, f2),
                        f3 = HPX_FORWARD(F3, f3),
                        f4 = HPX_FORWARD(F4, f4)]() mutable -> R {
                        using partitioner_type =
                            scan_static_partitioner<ExPolicy, R, Result1,
                                Result2>;
                        return partitioner_type::call(policy, first, count,
                            HPX_MOVE(init), f1, f2, f3, f4);
                    });
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // Scheduler executors return senders instead of futures. Run both
        // parallel scan phases as bulk sender operations and calculate the
        // inter-partition prefixes between them.
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename R,
            typename Result1, typename Result2>
        struct scan_scheduler_partitioner
        {
            using handle_local_exceptions =
                detail::handle_local_exceptions<ExPolicy>;

            static std::exception_ptr transform_exception(
                std::exception_ptr error) noexcept
            {
                try
                {
                    handle_local_exceptions::call(error);
                }
                catch (...)
                {
                    return std::current_exception();
                }

                HPX_UNREACHABLE;
            }

            template <typename FwdIter>
            using chunk_type = hpx::tuple<std::size_t, FwdIter, std::size_t>;

            template <typename FwdIter>
            using final_chunk_type = hpx::tuple<FwdIter, std::size_t, Result1>;

            template <typename ExPolicy_, typename FwdIter, typename T,
                typename F1, typename F2, typename F3, typename F4>
            static decltype(auto) call(ExPolicy_&& policy, FwdIter first,
                std::size_t count, T&& init, F1&& f1, F2&& f2, F3&& f3, F4&& f4)
            {
                using parameters_type =
                    typename std::decay_t<ExPolicy_>::executor_parameters_type;
                using executor_type =
                    typename std::decay_t<ExPolicy_>::executor_type;
                using scoped_executor_parameters =
                    detail::scoped_executor_parameters_ref<parameters_type,
                        executor_type>;

                scoped_executor_parameters scoped_params(
                    policy.parameters(), policy.executor());

                std::size_t cores = 1;
                auto shape = [&]() {
                    if constexpr (hpx::execution::experimental::
                                      extract_has_variable_chunk_size_v<
                                          parameters_type>)
                    {
                        return detail::get_bulk_iteration_shape_variable(
                            policy, first, count, cores);
                    }
                    else
                    {
                        return detail::get_bulk_iteration_shape(
                            policy, first, count, cores);
                    }
                }();

                std::vector<chunk_type<FwdIter>> chunks;
                chunks.reserve(hpx::util::size(shape));
                for (auto const& chunk : shape)
                {
                    chunks.emplace_back(
                        chunks.size(), hpx::get<0>(chunk), hpx::get<1>(chunk));
                }

                namespace ex = hpx::execution::experimental;

                auto partial_results =
                    std::make_shared<std::vector<std::optional<Result1>>>(
                        chunks.size());
                auto exec = policy.executor();
                auto first_step = execution::bulk_async_execute(
                    exec,
                    [f1 = HPX_FORWARD(F1, f1), partial_results](
                        chunk_type<FwdIter> const& chunk) mutable {
                        auto f1_copy = f1;
                        (*partial_results)[hpx::get<0>(chunk)].emplace(
                            HPX_INVOKE(f1_copy, hpx::get<1>(chunk),
                                hpx::get<2>(chunk)));
                    },
                    chunks);
                auto first_step_with_errors = ex::let_error(
                    HPX_MOVE(first_step), [](std::exception_ptr error) {
                        return ex::just_error(transform_exception(error));
                    });

                scoped_params.mark_end_of_scheduling();

                return ex::let_value(HPX_MOVE(first_step_with_errors),
                    [exec = HPX_MOVE(exec), chunks = HPX_MOVE(chunks),
                        partial_results = HPX_MOVE(partial_results),
                        init = HPX_FORWARD(T, init), f2 = HPX_FORWARD(F2, f2),
                        f3 = HPX_FORWARD(F3, f3),
                        f4 = HPX_FORWARD(F4, f4)]() mutable {
                        try
                        {
                            std::vector<Result1> scan_results;
                            scan_results.reserve(partial_results->size() + 1);

                            Result1 result = HPX_MOVE(init);
                            scan_results.push_back(result);
                            for (auto& partial_result : *partial_results)
                            {
                                HPX_ASSERT(partial_result.has_value());
                                result =
                                    HPX_INVOKE(f2, result, *partial_result);
                                scan_results.push_back(result);
                            }

                            std::vector<final_chunk_type<FwdIter>> final_chunks;
                            final_chunks.reserve(chunks.size());
                            for (std::size_t i = 0; i != chunks.size(); ++i)
                            {
                                final_chunks.emplace_back(
                                    hpx::get<1>(chunks[i]),
                                    hpx::get<2>(chunks[i]), scan_results[i]);
                            }

                            auto final_step = execution::bulk_async_execute(
                                exec,
                                [f3 = HPX_MOVE(f3)](
                                    final_chunk_type<FwdIter> const&
                                        chunk) mutable {
                                    auto f3_copy = f3;
                                    HPX_INVOKE(f3_copy, hpx::get<0>(chunk),
                                        hpx::get<1>(chunk), hpx::get<2>(chunk));
                                },
                                final_chunks);
                            auto final_step_with_errors =
                                ex::let_error(HPX_MOVE(final_step),
                                    [](std::exception_ptr error) {
                                        return ex::just_error(
                                            transform_exception(error));
                                    });

                            return ex::then(HPX_MOVE(final_step_with_errors),
                                [f4 = HPX_MOVE(f4),
                                    scan_results =
                                        HPX_MOVE(scan_results)]() mutable -> R {
                                    try
                                    {
                                        std::vector<hpx::future<Result2>> data;
                                        if constexpr (std::is_void_v<R>)
                                        {
                                            HPX_INVOKE(f4,
                                                HPX_MOVE(scan_results),
                                                HPX_MOVE(data));
                                            return;
                                        }
                                        else
                                        {
                                            return HPX_INVOKE(f4,
                                                HPX_MOVE(scan_results),
                                                HPX_MOVE(data));
                                        }
                                    }
                                    catch (...)
                                    {
                                        handle_local_exceptions::call(
                                            std::current_exception());
                                    }
                                    HPX_UNREACHABLE;
                                });
                        }
                        catch (...)
                        {
                            handle_local_exceptions::call(
                                std::current_exception());
                        }
                        HPX_UNREACHABLE;
                    });
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // ExPolicy:    execution policy
    // R:           overall result type
    // Result1:     intermediate result type of first and second step
    // Result2:     intermediate result of the third step
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename R = void,
        typename Result1 = R, typename Result2 = void>
    struct scan_partitioner
      : std::conditional_t<
            hpx::execution_policy_has_scheduler_executor_v<ExPolicy>,
            detail::scan_scheduler_partitioner<std::decay_t<ExPolicy>, R,
                Result1, Result2>,
            typename detail::select_partitioner<std::decay_t<ExPolicy>,
                detail::scan_static_partitioner,
                detail::scan_task_static_partitioner>::template apply<R,
                Result1, Result2>>
    {
    };
}    // namespace hpx::parallel::util
