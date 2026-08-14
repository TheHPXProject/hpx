//  Copyright (c) 2020-2024 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////
// The purpose of this example is to show how to write an executor disables
// thread stealing for the duration of the execution of a parallel algorithm
// it is used with.

#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

namespace executor_example {

    template <typename BaseExecutor>
    class disable_thread_stealing_executor : public BaseExecutor
    {
    public:
        using execution_category = typename BaseExecutor::execution_category;
        using executor_parameters_type =
            typename BaseExecutor::executor_parameters_type;

        template <typename Executor>
            requires(!std::is_same_v<std::decay_t<Executor>,
                disable_thread_stealing_executor>)
        explicit disable_thread_stealing_executor(Executor&& exec)
          : BaseExecutor(std::forward<Executor>(exec))
        {
        }

        disable_thread_stealing_executor const& context() const noexcept
        {
            return *this;
        }

        // Add two executor API functions that will be called before the
        // parallel algorithm starts executing and after it has finished
        // executing.
        template <typename Executor>
        void mark_begin_execution(Executor&&) const
        {
            auto const pu_mask =
                hpx::execution::experimental::get_processing_units_mask(*this);
            hpx::threads::remove_scheduler_mode(
                hpx::threads::policies::scheduler_mode::enable_stealing,
                pu_mask);
        }

        template <typename Executor>
        void mark_end_execution(Executor&&) const
        {
            auto const pu_mask =
                hpx::execution::experimental::get_processing_units_mask(*this);
            hpx::threads::add_scheduler_mode(
                hpx::threads::policies::scheduler_mode::enable_stealing,
                pu_mask);
        }

        // support scheduling properties via query() for new CPO dispatch
        template <typename Tag, typename... Args>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag>)
        auto query(Tag tag, Args&&... args) const -> decltype(tag(
            std::declval<BaseExecutor const&>(), HPX_FORWARD(Args, args)...))
        {
            return tag(static_cast<BaseExecutor const&>(*this),
                HPX_FORWARD(Args, args)...);
        }
    };

    template <typename BaseExecutor>
    auto make_disable_thread_stealing_executor(BaseExecutor&& exec)
    {
        return disable_thread_stealing_executor<std::decay_t<BaseExecutor>>(
            std::forward<BaseExecutor>(exec));
    }
}    // namespace executor_example

///////////////////////////////////////////////////////////////////////////////
// simple forwarding implementations of executor traits
namespace hpx::execution::experimental {

    template <typename BaseExecutor>
    struct is_one_way_executor<
        executor_example::disable_thread_stealing_executor<BaseExecutor>>
      : is_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_never_blocking_one_way_executor<
        executor_example::disable_thread_stealing_executor<BaseExecutor>>
      : is_never_blocking_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_two_way_executor<
        executor_example::disable_thread_stealing_executor<BaseExecutor>>
      : is_two_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_bulk_one_way_executor<
        executor_example::disable_thread_stealing_executor<BaseExecutor>>
      : is_bulk_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_bulk_two_way_executor<
        executor_example::disable_thread_stealing_executor<BaseExecutor>>
      : is_bulk_two_way_executor<std::decay_t<BaseExecutor>>
    {
    };
}    // namespace hpx::execution::experimental

int hpx_main()
{
    std::vector<double> v(1000);
    std::iota(v.begin(), v.end(), 0.0);

    // The following for_loop will be executed while thread stealing is disabled
    auto exec = executor_example::make_disable_thread_stealing_executor(
        hpx::execution::to_hierarchical_spawning(
            hpx::execution::par.executor()));

    // This may lead to deadlock situations if the main thread executes some of
    // the chunks synchronously.
    auto hint = hpx::execution::experimental::get_hint(exec);
    hint.sharing_mode(hpx::threads::thread_sharing_hint::do_not_share_function |
        hpx::threads::thread_sharing_hint::do_not_combine_tasks);
    auto no_sharing_exec = hpx::execution::experimental::with_hint(exec, hint);

    hpx::experimental::for_loop(hpx::execution::par.on(no_sharing_exec), 0,
        v.size(), [](std::size_t) {});

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
