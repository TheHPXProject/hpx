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
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/parallel/util/detail/chunk_size.hpp>
#include <hpx/parallel/util/detail/handle_local_exceptions.hpp>
#include <hpx/parallel/util/detail/partitioner_iteration.hpp>
#include <hpx/parallel/util/detail/scoped_executor_parameters.hpp>
#include <hpx/parallel/util/detail/select_partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::util::detail {

    HPX_CXX_CORE_EXPORT template <typename Result, typename ExPolicy,
        typename FwdIter, typename F, typename ReShape>
    auto foreach_partition(ExPolicy policy, FwdIter first, std::size_t count,
        F&& f, ReShape&& reshape)
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
                policy, first, count, cores);

            return execution::bulk_async_execute(policy.executor(),
                partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                reshape(HPX_MOVE(shape)));
        }
        else if constexpr (!invokes_testing_function)
        {
            auto&& shape = detail::get_bulk_iteration_shape_idx(
                policy, first, count, cores);

            using executor_type = decltype(policy.executor());
            auto&& reshaped = reshape(HPX_MOVE(shape), cores);

            // We attempt to perform some optimizations in case of non-task
            // execution.
            if constexpr (!hpx::is_async_execution_policy_v<ExPolicy>)
            {
                // Switch to sequential execution for one-core, one-chunk case
                // if the executor supports it.
                if constexpr (std::is_void_v<Result> &&
                    hpx::traits::is_one_way_executor_v<executor_type>)
                {
                    if (cores == 1 && std::size(reshaped) == 1)
                    {
                        return execution::sync_execute(policy.executor(),
                            partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                            *std::begin(HPX_MOVE(reshaped)));
                    }
                }

                if constexpr (hpx::traits::is_bulk_one_way_executor_v<
                                  executor_type>)
                {
                    return execution::bulk_sync_execute(policy.executor(),
                        partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                        HPX_MOVE(reshaped));
                }
                else
                {
                    // Fall back if given executor doesn't support any of the
                    // above optimizations.
                    auto&& items =
                        execution::bulk_async_execute(policy.executor(),
                            partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                            HPX_MOVE(reshaped));
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
                    HPX_MOVE(reshaped));
            }
        }
        else
        {
            std::vector<hpx::future<Result>> inititems;
            auto&& shape = detail::get_bulk_iteration_shape_idx(
                policy, inititems, f, first, count, cores);

            auto&& workitems = execution::bulk_async_execute(policy.executor(),
                partitioner_iteration<Result, F>{HPX_FORWARD(F, f)},
                reshape(HPX_MOVE(shape)));

            return std::make_pair(HPX_MOVE(inititems), HPX_MOVE(workitems));
        }
    }

    ///////////////////////////////////////////////////////////////////////
    // The static partitioner simply spawns one chunk of iterations for
    // each available core.
    HPX_CXX_CORE_EXPORT struct default_reshape
    {
        using is_transparent = std::true_type;

        template <typename T, typename... Ts>
        HPX_HOST_DEVICE constexpr T&& operator()(T&& t, Ts&&...) const noexcept
        {
            return HPX_FORWARD(T, t);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Result>
    struct foreach_static_partitioner
    {
        using parameters_type = typename ExPolicy::executor_parameters_type;
        using executor_type = typename ExPolicy::executor_type;

        using handle_local_exceptions =
            detail::handle_local_exceptions<ExPolicy>;

        template <typename ExPolicy_, typename FwdIter, typename F1,
            typename F2, typename ReShape = default_reshape>
        static auto call(ExPolicy_&& policy, FwdIter first, std::size_t count,
            F1&& f1, F2&& f2, ReShape&& reshape = ReShape{})
        {
            // inform parameter traits
            using scoped_executor_parameters =
                detail::scoped_executor_parameters_ref<parameters_type,
                    typename std::decay_t<ExPolicy_>::executor_type>;

            scoped_executor_parameters scoped_params(
                policy.parameters(), policy.executor());

            FwdIter last = parallel::detail::next(first, count);
            try
            {
                if constexpr (std::is_void_v<decltype(foreach_partition<Result>(
                                  HPX_FORWARD(ExPolicy_, policy), first, count,
                                  HPX_FORWARD(F1, f1),
                                  HPX_FORWARD(ReShape, reshape)))>)
                {
                    detail::foreach_partition<Result>(
                        HPX_FORWARD(ExPolicy_, policy), first, count,
                        HPX_FORWARD(F1, f1), HPX_FORWARD(ReShape, reshape));

                    scoped_params.mark_end_of_scheduling();

                    return HPX_INVOKE(f2, HPX_MOVE(last));
                }
                else
                {
                    auto&& items = foreach_partition<Result>(
                        HPX_FORWARD(ExPolicy_, policy), first, count,
                        HPX_FORWARD(F1, f1), HPX_FORWARD(ReShape, reshape));

                    scoped_params.mark_end_of_scheduling();

                    return reduce(
                        HPX_MOVE(items), HPX_FORWARD(F2, f2), HPX_MOVE(last));
                }
            }
            catch (...)
            {
                handle_local_exceptions::call(std::current_exception());
            }
        }

    private:
        template <typename F, typename Items1, typename Items2,
            typename FwdIter>
        static auto reduce(
            std::pair<Items1, Items2>&& items, F&& f, FwdIter last)
        {
            // wait for all tasks to finish
            if (hpx::wait_all_nothrow(hpx::get<0>(items), hpx::get<1>(items)))
            {
                // always rethrow if inititems/workitems have at least one
                // exceptional future
                handle_local_exceptions::call(hpx::get<0>(items));
                handle_local_exceptions::call(hpx::get<1>(items));
            }

            // gcc 9 complains here if HPX_INVOKE is used.
            return hpx::invoke(f, HPX_MOVE(last));
        }

        template <typename F, typename Items, typename FwdIter,
            typename Enable =
                std::enable_if_t<!hpx::traits::is_pair_v<std::decay_t<Items>>>>
        static auto reduce(Items&& items, F&& f, FwdIter last)
        {
            using decayed_items = std::decay_t<Items>;
            constexpr bool is_future =
                hpx::traits::is_future_v<decayed_items> ||
                hpx::traits::is_future_range_v<decayed_items>;

            namespace ex = hpx::execution::experimental;
            if constexpr (ex::is_sender_v<decayed_items> && !is_future)
            {
                // the predecessor sender could be exposing zero or more
                // value types
                return ex::then(HPX_FORWARD(Items, items),
                    [f = HPX_FORWARD(F, f), last = HPX_MOVE(last)]() mutable {
                        return HPX_INVOKE(f, HPX_MOVE(last));
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
                return HPX_INVOKE(f, HPX_MOVE(last));
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Result>
    struct foreach_task_static_partitioner
    {
        using parameters_type = typename ExPolicy::executor_parameters_type;
        using executor_type = typename ExPolicy::executor_type;

        using scoped_executor_parameters =
            detail::scoped_executor_parameters<parameters_type, executor_type>;

        using handle_local_exceptions =
            detail::handle_local_exceptions<ExPolicy>;

        template <typename ExPolicy_, typename FwdIter, typename F1,
            typename F2, typename ReShape = default_reshape>
        static decltype(auto) call(ExPolicy_&& policy, FwdIter first,
            std::size_t count, F1&& f1, F2&& f2, ReShape&& reshape = ReShape{})
        {
            // inform parameter traits
            std::shared_ptr<scoped_executor_parameters> scoped_params =
                std::make_shared<scoped_executor_parameters>(
                    policy.parameters(), policy.executor());

            FwdIter last = parallel::detail::next(first, count);
            try
            {
                auto&& items = detail::foreach_partition<Result>(
                    HPX_FORWARD(ExPolicy_, policy), first, count,
                    HPX_FORWARD(F1, f1), HPX_FORWARD(ReShape, reshape));

                scoped_params->mark_end_of_scheduling();

                return reduce(HPX_MOVE(scoped_params), HPX_MOVE(items),
                    HPX_FORWARD(F2, f2), HPX_MOVE(last));
            }
            catch (...)
            {
                try
                {
                    handle_local_exceptions::call(std::current_exception());
                }
                catch (...)
                {
                    using return_type =
                        std::decay_t<std::invoke_result_t<F2&&, FwdIter&&>>;
                    return hpx::make_exceptional_future<return_type>(
                        std::current_exception());
                }
            }
        }

    private:
        template <typename F, typename Items1, typename Items2,
            typename FwdIter>
        static decltype(auto) reduce(
            [[maybe_unused]] std::shared_ptr<scoped_executor_parameters>&&
                scoped_params,
            [[maybe_unused]] std::pair<Items1, Items2>&& items,
            [[maybe_unused]] F&& f, [[maybe_unused]] FwdIter last)
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            using return_type =
                std::decay_t<std::invoke_result_t<F&&, FwdIter&&>>;
            return hpx::future<return_type>();
#else
            // wait for all tasks to finish
            // Note: the lambda takes the vectors by value (dataflow
            //       moves those into the lambda) to ensure that they
            //       will be destroyed before the lambda exists.
            //       Otherwise, the vectors stay alive in the dataflow's
            //       shared state and may reference data that has gone
            //       out of scope.
            return hpx::dataflow(
                hpx::launch::sync,
                [last, scoped_params = HPX_MOVE(scoped_params),
                    f = HPX_FORWARD(F, f)](auto&& r1, auto&& r2) mutable {
                    HPX_UNUSED(scoped_params);

                    handle_local_exceptions::call(
                        HPX_FORWARD(decltype(r1), r1));
                    handle_local_exceptions::call(
                        HPX_FORWARD(decltype(r2), r2));

                    return f(HPX_MOVE(last));
                },
                HPX_MOVE(hpx::get<0>(items)), HPX_MOVE(hpx::get<1>(items)));
#endif
        }

        template <typename F, typename Items, typename FwdIter,
            typename Enable =
                std::enable_if_t<!hpx::traits::is_pair_v<std::decay_t<Items>>>>
        static decltype(auto) reduce(
            [[maybe_unused]] std::shared_ptr<scoped_executor_parameters>&&
                scoped_params,
            [[maybe_unused]] Items&& items, [[maybe_unused]] F&& f,
            [[maybe_unused]] FwdIter last)
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            using return_type =
                std::decay_t<std::invoke_result_t<F&&, FwdIter&&>>;
            return hpx::future<return_type>();
#else
            // wait for all tasks to finish
            return hpx::dataflow(
                hpx::launch::sync,
                [last, scoped_params = HPX_MOVE(scoped_params),
                    f = HPX_FORWARD(F, f)](auto&& r) mutable {
                    HPX_UNUSED(scoped_params);

                    handle_local_exceptions::call(HPX_FORWARD(decltype(r), r));

                    return f(HPX_MOVE(last));
                },
                HPX_FORWARD(Items, items));
#endif
        }

        template <typename F, typename FwdIter>
        static decltype(auto) reduce(
            [[maybe_unused]] std::shared_ptr<scoped_executor_parameters>&&
                scoped_params,
            [[maybe_unused]] hpx::future<void>&& item, [[maybe_unused]] F&& f,
            [[maybe_unused]] FwdIter last)
        {
#if defined(HPX_COMPUTE_DEVICE_CODE)
            HPX_ASSERT(false);
            using return_type =
                std::decay_t<std::invoke_result_t<F&&, FwdIter&&>>;
            return hpx::future<return_type>();
#else
            // wait for the task to finish, invoke the given function before
            // returning
            return item.then(hpx::launch::sync,
                [last, scoped_params = HPX_MOVE(scoped_params),
                    f = HPX_FORWARD(F, f)](auto&& r) mutable {
                    HPX_UNUSED(scoped_params);

                    handle_local_exceptions::call(HPX_FORWARD(decltype(r), r));

                    return f(HPX_MOVE(last));
                });
#endif
        }
    };
}    // namespace hpx::parallel::util::detail

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    // ExPolicy: execution policy
    // Result:   intermediate result type of first step (default: void)
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Result = void>
    struct foreach_partitioner
      : detail::select_partitioner<std::decay_t<ExPolicy>,
            detail::foreach_static_partitioner,
            detail::foreach_task_static_partitioner>::template apply<Result>
    {
    };
}    // namespace hpx::parallel::util
