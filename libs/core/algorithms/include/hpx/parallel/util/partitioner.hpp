//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/modules/async_local.hpp>
#endif
#include <hpx/algorithms/traits/is_pair.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/chunk_size.hpp>
#include <hpx/parallel/util/detail/handle_local_exceptions.hpp>
#include <hpx/parallel/util/detail/partitioner_iteration.hpp>
#include <hpx/parallel/util/detail/scoped_executor_parameters.hpp>
#include <hpx/parallel/util/detail/select_partitioner.hpp>

#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::util::detail {

    HPX_CXX_CORE_EXPORT template <typename Result, bool Optimize = true,
        typename ExPolicy, typename IterOrR, typename F>
    auto partition(ExPolicy policy, IterOrR it_or_r, std::size_t count, F&& f)
    {
        // estimate a chunk size based on number of cores used
        using parameters_type =
            hpx::execution::experimental::extract_executor_parameters_t<
                std::decay_t<ExPolicy>>;
        constexpr bool has_variable_chunk_size =
            hpx::execution::experimental::extract_has_variable_chunk_size_v<
                parameters_type>;
        constexpr bool invokes_testing_function =
            hpx::execution::experimental::extract_invokes_testing_function_v<
                parameters_type>;

        std::size_t cores = 1;
        if constexpr (has_variable_chunk_size)
        {
            static_assert(!invokes_testing_function,
                "parameters object should not expose both, "
                "has_variable_chunk_size and invokes_testing_function");

            auto&& shape = detail::get_bulk_iteration_shape_variable(
                policy, it_or_r, count, cores);

            return execution::bulk_async_execute(policy.executor(),
                partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                HPX_MOVE(shape));
        }
        else if constexpr (!invokes_testing_function)
        {
            auto&& shape =
                detail::get_bulk_iteration_shape(policy, it_or_r, count, cores);

            using executor_type = decltype(policy.executor());

            // We attempt to perform some optimizations in case of non-task
            // execution.
            if constexpr (Optimize &&
                !hpx::is_async_execution_policy_v<ExPolicy>)
            {
                // Switch to sequential execution for one-core, one-chunk case
                // if the executor supports it.
                if constexpr (std::is_void_v<Result> &&
                    hpx::traits::is_one_way_executor_v<executor_type>)
                {
                    if (cores == 1 && std::size(shape) == 1)
                    {
                        return execution::sync_execute(policy.executor(),
                            partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                            *std::begin(HPX_MOVE(shape)));
                    }
                }

                if constexpr (hpx::traits::is_bulk_one_way_executor_v<
                                  executor_type>)
                {
                    return execution::bulk_sync_execute(policy.executor(),
                        partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                        HPX_MOVE(shape));
                }
                else
                {
                    // Fall back if given executor doesn't support any of the
                    // above optimizations.
                    auto&& items =
                        execution::bulk_async_execute(policy.executor(),
                            partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                            HPX_MOVE(shape));
                    if (hpx::wait_all_nothrow(items))
                    {
                        using handle_local_exceptions =
                            detail::handle_local_exceptions<ExPolicy>;
                        handle_local_exceptions::call(items);
                    }
                    return hpx::unwrap(HPX_MOVE(items));
                }
            }
            else
            {
                return execution::bulk_async_execute(policy.executor(),
                    partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                    HPX_MOVE(shape));
            }
        }
        else
        {
            std::vector<hpx::future<Result>> inititems;
            auto&& shape = detail::get_bulk_iteration_shape(
                policy, inititems, f, it_or_r, count, cores);

            auto&& workitems = execution::bulk_async_execute(policy.executor(),
                partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                HPX_MOVE(shape));

            return std::make_pair(HPX_MOVE(inititems), HPX_MOVE(workitems));
        }
    }

    HPX_CXX_CORE_EXPORT template <typename Result, typename ExPolicy,
        typename FwdIter, typename Stride, typename F>
    auto partition_with_index(
        ExPolicy policy, FwdIter first, std::size_t count, Stride stride, F&& f)
    {
        // estimate a chunk size based on number of cores used
        using parameters_type =
            hpx::execution::experimental::extract_executor_parameters_t<
                std::decay_t<ExPolicy>>;
        constexpr bool has_variable_chunk_size =
            hpx::execution::experimental::extract_has_variable_chunk_size_v<
                parameters_type>;
        constexpr bool invokes_testing_function =
            hpx::execution::experimental::extract_invokes_testing_function_v<
                parameters_type>;

        std::size_t cores = 1;
        if constexpr (has_variable_chunk_size)
        {
            static_assert(!invokes_testing_function,
                "parameters object should not expose both, "
                "has_variable_chunk_size and invokes_testing_function");

            auto&& shape = detail::get_bulk_iteration_shape_idx_variable(
                policy, first, count, cores, stride);

            return execution::bulk_async_execute(policy.executor(),
                partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                HPX_MOVE(shape));
        }
        else if constexpr (!invokes_testing_function)
        {
            auto&& shape = detail::get_bulk_iteration_shape_idx(
                policy, first, count, cores, stride);

            using executor_type = decltype(policy.executor());

            // We attempt to perform some optimizations in case of non-task
            // execution.
            if constexpr (!hpx::is_async_execution_policy_v<ExPolicy> &&
                !hpx::execution_policy_has_scheduler_executor_v<ExPolicy>)
            {
                // Switch to sequential execution for one-core, one-chunk case
                // if the executor supports it.
                if constexpr (std::is_void_v<Result> &&
                    hpx::traits::is_one_way_executor_v<executor_type>)
                {
                    if (cores == 1 && std::size(shape) == 1)
                    {
                        return execution::sync_execute(policy.executor(),
                            partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                            *std::begin(HPX_MOVE(shape)));
                    }
                }

                if constexpr (hpx::traits::is_bulk_one_way_executor_v<
                                  executor_type>)
                {
                    return execution::bulk_sync_execute(policy.executor(),
                        partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                        HPX_MOVE(shape));
                }
                else
                {
                    // Fall back if given executor doesn't support any of the
                    // above optimizations.
                    auto&& items =
                        execution::bulk_async_execute(policy.executor(),
                            partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                            HPX_MOVE(shape));
                    if (hpx::wait_all_nothrow(items))
                    {
                        using handle_local_exceptions =
                            detail::handle_local_exceptions<ExPolicy>;
                        handle_local_exceptions::call(items);
                    }
                    return hpx::unwrap(HPX_MOVE(items));
                }
            }
            else
            {
                return execution::bulk_async_execute(policy.executor(),
                    partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                    HPX_MOVE(shape));
            }
        }
        else
        {
            std::vector<hpx::future<Result>> inititems;
            auto&& shape = detail::get_bulk_iteration_shape_idx(
                policy, inititems, f, first, count, cores, stride);

            auto&& workitems = execution::bulk_async_execute(policy.executor(),
                partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                HPX_MOVE(shape));

            return std::make_pair(HPX_MOVE(inititems), HPX_MOVE(workitems));
        }
    }

    HPX_CXX_CORE_EXPORT template <typename Result, typename ExPolicy,
        typename FwdIter, typename Data, typename F>
    // requires is_container<Data>
    std::vector<hpx::future<Result>> partition_with_data(ExPolicy&& policy,
        FwdIter first, std::size_t count,
        std::vector<std::size_t> const& chunk_sizes, Data&& data, F&& f)
    {
        HPX_ASSERT(hpx::util::size(data) >= hpx::util::size(chunk_sizes));

        auto data_it = hpx::util::begin(data);
        auto chunk_size_it = hpx::util::begin(chunk_sizes);

        using data_type = std::decay_t<Data>;
        using tuple_type =
            hpx::tuple<typename data_type::value_type, FwdIter, std::size_t>;

        // schedule every chunk on a separate thread
        std::vector<tuple_type> shape;
        shape.reserve(chunk_sizes.size());

        while (count != 0)
        {
            std::size_t chunk = (std::min) (count, *chunk_size_it);
            HPX_ASSERT(chunk != 0);

            shape.emplace_back(*data_it, first, chunk);

            count -= chunk;
            std::advance(first, chunk);

            ++data_it;
            ++chunk_size_it;
        }
        HPX_ASSERT(chunk_size_it == chunk_sizes.end());

        return execution::bulk_async_execute(policy.executor(),
            partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
            HPX_MOVE(shape));
    }

    ///////////////////////////////////////////////////////////////////////
    // The static partitioner simply spawns one chunk of iterations for
    // each available core.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename R,
        typename Result>
    struct static_partitioner
    {
        using parameters_type = typename ExPolicy::executor_parameters_type;
        using executor_type = typename ExPolicy::executor_type;

        using handle_local_exceptions =
            detail::handle_local_exceptions<ExPolicy>;

        template <typename ExPolicy_, typename FwdIter, typename F1,
            typename F2>
        static decltype(auto) call(ExPolicy_&& policy, FwdIter first,
            std::size_t count, F1&& f1, F2&& f2)
        {
            // inform parameter traits
            using scoped_executor_parameters =
                detail::scoped_executor_parameters_ref<parameters_type,
                    typename std::decay_t<ExPolicy_>::executor_type>;

            scoped_executor_parameters scoped_params(
                policy.parameters(), policy.executor());

            try
            {
                if constexpr (std::is_void_v<decltype(detail::partition<Result>(
                                  policy, first, count, f1))>)
                {
                    detail::partition<Result>(HPX_FORWARD(ExPolicy_, policy),
                        first, count, HPX_FORWARD(F1, f1));

                    scoped_params.mark_end_of_scheduling();

                    return HPX_INVOKE(HPX_FORWARD(F2, f2));
                }
                else
                {
                    auto&& items = detail::partition<Result>(
                        HPX_FORWARD(ExPolicy_, policy), first, count,
                        HPX_FORWARD(F1, f1));

                    scoped_params.mark_end_of_scheduling();

                    return reduce(HPX_MOVE(items), HPX_FORWARD(F2, f2));
                }
            }
            catch (...)
            {
                handle_local_exceptions::call(std::current_exception());
            }
        }

        template <typename ExPolicy_, typename FwdIter, typename Stride,
            typename F1, typename F2>
        static decltype(auto) call_with_index(ExPolicy_&& policy, FwdIter first,
            std::size_t count, Stride stride, F1&& f1, F2&& f2)
        {
            // inform parameter traits
            using scoped_executor_parameters =
                detail::scoped_executor_parameters_ref<parameters_type,
                    typename std::decay_t<ExPolicy_>::executor_type>;

            scoped_executor_parameters scoped_params(
                policy.parameters(), policy.executor());

            try
            {
                if constexpr (std::is_void_v<
                                  decltype(detail::partition_with_index<Result>(
                                      policy, first, count, stride, f1))>)
                {
                    detail::partition_with_index<Result>(
                        HPX_FORWARD(ExPolicy_, policy), first, count, stride,
                        HPX_FORWARD(F1, f1));

                    scoped_params.mark_end_of_scheduling();

                    return HPX_INVOKE(HPX_FORWARD(F2, f2));
                }
                else
                {
                    auto&& items = detail::partition_with_index<Result>(
                        HPX_FORWARD(ExPolicy_, policy), first, count, stride,
                        HPX_FORWARD(F1, f1));

                    scoped_params.mark_end_of_scheduling();

                    return reduce(HPX_MOVE(items), HPX_FORWARD(F2, f2));
                }
            }
            catch (...)
            {
                handle_local_exceptions::call(std::current_exception());
            }
        }

        template <typename ExPolicy_, typename FwdIter, typename F1,
            typename F2, typename Data>
        // requires is_container<Data>
        static decltype(auto) call_with_data(ExPolicy_&& policy, FwdIter first,
            std::size_t count, F1&& f1, F2&& f2,
            std::vector<std::size_t> const& chunk_sizes, Data&& data)
        {
            // inform parameter traits
            using scoped_executor_parameters =
                detail::scoped_executor_parameters_ref<parameters_type,
                    typename std::decay_t<ExPolicy_>::executor_type>;

            scoped_executor_parameters scoped_params(
                policy.parameters(), policy.executor());

            try
            {
                auto&& items = detail::partition_with_data<Result>(
                    HPX_FORWARD(ExPolicy_, policy), first, count, chunk_sizes,
                    HPX_FORWARD(Data, data), HPX_FORWARD(F1, f1));

                scoped_params.mark_end_of_scheduling();

                if constexpr (hpx::execution_policy_has_scheduler_executor_v<
                                  ExPolicy_>)
                {
                    namespace ex = hpx::execution::experimental;
                    namespace tt = hpx::this_thread::experimental;
                    auto sender =
                        ex::then(HPX_MOVE(items), HPX_FORWARD(F2, f2));
                    auto result = tt::sync_wait(HPX_MOVE(sender));
                    if constexpr (hpx::tuple_size_v<
                                      std::decay_t<decltype(*result)>> == 0)
                    {
                        return;
                    }
                    else
                    {
                        auto value = hpx::get<0>(HPX_MOVE(*result));
                        return value;
                    }
                }
                else
                {
                    return reduce(HPX_MOVE(items), HPX_FORWARD(F2, f2));
                }
            }
            catch (...)
            {
                handle_local_exceptions::call(std::current_exception());
            }
        }

    private:
        template <typename Items, typename F,
            typename Enable =
                std::enable_if_t<!hpx::traits::is_pair_v<std::decay_t<Items>>>>
        static auto reduce(Items&& items, F&& f)
        {
            using decayed_items = std::decay_t<Items>;
            constexpr bool is_future =
                hpx::traits::is_future_v<decayed_items> ||
                hpx::traits::is_future_range_v<decayed_items>;

            namespace ex = hpx::execution::experimental;
            if constexpr (ex::is_sender_v<decayed_items> && !is_future)
            {
                // the predecessor sender could be exposing zero or more value
                // types
                return ex::then(HPX_FORWARD(Items, items),
                    [f = HPX_FORWARD(F, f)](auto&&... results) mutable {
                        return HPX_INVOKE(
                            f, HPX_FORWARD(decltype(results), results)...);
                    });
            }
            else
            {
                if constexpr (is_future)
                {
                    // wait for all tasks to finish
                    if (hpx::wait_all_nothrow(items))
                    {
                        // always rethrow if items has at least one exceptional
                        // future
                        handle_local_exceptions::call(items);
                    }
                }
                return HPX_INVOKE(f, HPX_FORWARD(Items, items));
            }
        }

        template <typename Items,
            typename Enable =
                std::enable_if_t<!hpx::traits::is_pair_v<std::decay_t<Items>>>>
        static auto reduce(Items&& items, hpx::util::empty_function)
        {
            using decayed_items = std::decay_t<Items>;
            constexpr bool is_future =
                hpx::traits::is_future_v<decayed_items> ||
                hpx::traits::is_future_range_v<decayed_items>;

            namespace ex = hpx::execution::experimental;
            if constexpr (ex::is_sender_v<decayed_items> && !is_future)
            {
                return HPX_FORWARD(Items, items);
            }
            else
            {
                if constexpr (is_future)
                {
                    // wait for all tasks to finish
                    if (hpx::wait_all_nothrow(items))
                    {
                        // always rethrow if items has at least one exceptional
                        // future
                        handle_local_exceptions::call(items);
                    }
                }
                return hpx::util::unused;
            }
        }

        template <typename Items1, typename Items2, typename F>
        static auto reduce(std::pair<Items1, Items2>&& items, F&& f)
        {
            if (items.first.empty())
            {
                return reduce(HPX_MOVE(items.second), HPX_FORWARD(F, f));
            }

            if constexpr (hpx::traits::is_future_v<Items2>)
            {
                items.first.emplace_back(HPX_MOVE(items.second));
            }
            else
            {
                items.first.insert(items.first.end(),
                    std::make_move_iterator(items.second.begin()),
                    std::make_move_iterator(items.second.end()));
            }

            return reduce(HPX_MOVE(items.first), HPX_FORWARD(F, f));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename R,
        typename Result>
    struct task_static_partitioner
    {
        using parameters_type = typename ExPolicy::executor_parameters_type;
        using executor_type = typename ExPolicy::executor_type;

        using scoped_executor_parameters =
            detail::scoped_executor_parameters<parameters_type, executor_type>;

        using handle_local_exceptions =
            detail::handle_local_exceptions<ExPolicy>;

        template <typename ExPolicy_, typename FwdIter, typename F1,
            typename F2>
        static hpx::future<R> call(ExPolicy_&& policy, FwdIter first,
            std::size_t count, F1&& f1, F2&& f2)
        {
            // inform parameter traits
            std::shared_ptr<scoped_executor_parameters> scoped_params =
                std::make_shared<scoped_executor_parameters>(
                    policy.parameters(), policy.executor());

            try
            {
                auto&& items =
                    detail::partition<Result>(HPX_FORWARD(ExPolicy_, policy),
                        first, count, HPX_FORWARD(F1, f1));

                scoped_params->mark_end_of_scheduling();

                return reduce(HPX_MOVE(scoped_params), HPX_MOVE(items),
                    HPX_FORWARD(F2, f2));
            }
            catch (...)
            {
                return hpx::make_exceptional_future<R>(
                    std::current_exception());
            }
        }

        template <typename ExPolicy_, typename FwdIter, typename Stride,
            typename F1, typename F2>
        static hpx::future<R> call_with_index(ExPolicy_&& policy, FwdIter first,
            std::size_t count, Stride stride, F1&& f1, F2&& f2)
        {
            // inform parameter traits
            std::shared_ptr<scoped_executor_parameters> scoped_params =
                std::make_shared<scoped_executor_parameters>(
                    policy.parameters(), policy.executor());

            try
            {
                auto&& items = detail::partition_with_index<Result>(
                    HPX_FORWARD(ExPolicy_, policy), first, count, stride,
                    HPX_FORWARD(F1, f1));

                scoped_params->mark_end_of_scheduling();

                return reduce(HPX_MOVE(scoped_params), HPX_MOVE(items),
                    HPX_FORWARD(F2, f2));
            }
            catch (...)
            {
                return hpx::make_exceptional_future<R>(
                    std::current_exception());
            }
        }

        template <typename ExPolicy_, typename FwdIter, typename F1,
            typename F2, typename Data>
        // requires is_container<Data>
        static hpx::future<R> call_with_data(ExPolicy_&& policy, FwdIter first,
            std::size_t count, F1&& f1, F2&& f2,
            std::vector<std::size_t> const& chunk_sizes, Data&& data)
        {
            // inform parameter traits
            std::shared_ptr<scoped_executor_parameters> scoped_params =
                std::make_shared<scoped_executor_parameters>(
                    policy.parameters(), policy.executor());

            try
            {
                auto&& items = detail::partition_with_data<Result>(
                    HPX_FORWARD(ExPolicy_, policy), first, count, chunk_sizes,
                    HPX_FORWARD(Data, data), HPX_FORWARD(F1, f1));

                scoped_params->mark_end_of_scheduling();

                return reduce(HPX_MOVE(scoped_params), HPX_MOVE(items),
                    HPX_FORWARD(F2, f2));
            }
            catch (...)
            {
                return hpx::make_exceptional_future<R>(
                    std::current_exception());
            }
        }

    private:
        template <typename Items1, typename Items2, typename F>
        static hpx::future<R> reduce(
            std::shared_ptr<scoped_executor_parameters>&& scoped_params,
            std::pair<Items1, Items2>&& items, F&& f)
        {
            if (items.first.empty())
            {
                return reduce(HPX_MOVE(scoped_params), HPX_MOVE(items.second),
                    HPX_FORWARD(F, f));
            }

            if constexpr (hpx::traits::is_future_v<Items2>)
            {
                items.first.emplace_back(HPX_MOVE(items.second));
            }
            else
            {
                items.first.insert(items.first.end(),
                    std::make_move_iterator(items.second.begin()),
                    std::make_move_iterator(items.second.end()));
            }

            return reduce(HPX_MOVE(scoped_params), HPX_MOVE(items.first),
                HPX_FORWARD(F, f));
        }

        template <typename Items, typename F,
            typename Enable =
                std::enable_if_t<!hpx::traits::is_pair_v<std::decay_t<Items>>>>
        static hpx::future<R> reduce(
            [[maybe_unused]] std::shared_ptr<scoped_executor_parameters>&&
                scoped_params,
            [[maybe_unused]] Items&& workitems, [[maybe_unused]] F&& f)
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            return hpx::future<R>();
#else
            // wait for all tasks to finish
            return hpx::dataflow(
                hpx::launch::sync,
                [scoped_params = HPX_MOVE(scoped_params),
                    f = HPX_FORWARD(F, f)](auto&& r) mutable -> R {
                    HPX_UNUSED(scoped_params);

                    handle_local_exceptions::call(r);

                    if constexpr (hpx::traits::is_future_v<decltype((r))> &&
                        !std::is_void_v<
                            hpx::traits::future_traits_t<decltype((r))>>)
                    {
                        return hpx::util::void_guard<R>(), f(r.get());
                    }
                    else
                    {
                        return hpx::util::void_guard<R>(),
                               f(HPX_FORWARD(decltype(r), r));
                    }
                },
                HPX_FORWARD(Items, workitems));
#endif
        }
    };
}    // namespace hpx::parallel::util::detail

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    // ExPolicy: execution policy
    // R:        overall result type
    // Result:   intermediate result type of first step
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename R = void,
        typename Result = R>
    struct partitioner
      : detail::select_partitioner<std::decay_t<ExPolicy>,
            detail::static_partitioner,
            detail::task_static_partitioner>::template apply<R, Result>
    {
    };

    // Helper to call partitioner and wrap the result with
    // algorithm_result::get(). Handles both void and non-void return types.
    template <typename ExPolicy, typename... Args>
    decltype(auto) call_with_algorithm_result(ExPolicy&& policy, Args&&... args)
    {
        if constexpr (std::is_void_v<decltype(partitioner<ExPolicy>::call(
                          HPX_FORWARD(ExPolicy, policy),
                          HPX_FORWARD(Args, args)...))>)
        {
            partitioner<ExPolicy>::call(
                HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Args, args)...);
            return detail::algorithm_result<ExPolicy>::get();
        }
        else
        {
            return detail::algorithm_result<ExPolicy>::get(
                partitioner<ExPolicy>::call(
                    HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Args, args)...));
        }
    }
}    // namespace hpx::parallel::util
