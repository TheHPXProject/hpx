//  Copyright (c) 2020-2024 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

///////////////////////////////////////////////////////////////////////////////
// Verify that a wrapping executor does not go out of scope prematurely when
// used with a seq(task) execution policy.

#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace test {

    template <typename BaseExecutor>
    class wrapping_executor
    {
    public:
        using execution_category =
            hpx::traits::executor_execution_category_t<BaseExecutor>;
        using executor_parameters_type =
            hpx::traits::executor_parameters_type_t<BaseExecutor>;

        template <typename Executor,
            typename Enable = std::enable_if_t<
                !std::is_same_v<wrapping_executor, std::decay_t<Executor>>>>
        explicit wrapping_executor(Executor&& exec)
          : exec_(std::forward<Executor>(exec))
        {
        }

        bool operator==(wrapping_executor const& rhs) const noexcept
        {
            return exec_ == rhs.exec_;
        }

        bool operator!=(wrapping_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        wrapping_executor const& context() const noexcept
        {
            return *this;
        }

    public:
        // OneWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) sync_execute(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::sync_execute(
                exec_, std::forward<F>(f), std::forward<Ts>(ts)...);
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::async_execute(
                exec_, std::forward<F>(f), std::forward<Ts>(ts)...);
        }

        template <typename F, typename Future, typename... Ts>
        decltype(auto) then_execute(
            F&& f, Future&& predecessor, Ts&&... ts) const
        {
            return hpx::parallel::execution::then_execute(exec_,
                std::forward<F>(f), std::forward<Future>(predecessor),
                std::forward<Ts>(ts)...);
        }

        // NonBlockingOneWayExecutor (adapted) interface
        template <typename F, typename... Ts>
        decltype(auto) post(F&& f, Ts&&... ts) const
        {
            hpx::parallel::execution::post(
                exec_, std::forward<F>(f), std::forward<Ts>(ts)...);
        }

        // BulkOneWayExecutor interface
        template <typename F, typename S, typename... Ts>
        decltype(auto) bulk_sync_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return hpx::parallel::execution::bulk_sync_execute(
                exec_, std::forward<F>(f), shape, std::forward<Ts>(ts)...);
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return hpx::parallel::execution::bulk_async_execute(
                exec_, std::forward<F>(f), shape, std::forward<Ts>(ts)...);
        }

        template <typename F, typename S, typename Future, typename... Ts>
        decltype(auto) bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts) const
        {
            return hpx::parallel::execution::bulk_then_execute(exec_,
                std::forward<F>(f), shape, std::forward<Future>(predecessor),
                std::forward<Ts>(ts)...);
        }

    private:
        BaseExecutor exec_;
    };

    // clang-format off
    template <typename BaseExecutor>
    wrapping_executor(BaseExecutor&& exec)
        -> wrapping_executor<std::decay_t<BaseExecutor>>;
    // clang-format on
}    // namespace test

///////////////////////////////////////////////////////////////////////////////
// simple forwarding implementations of executor traits
namespace hpx::execution::experimental {

    template <typename BaseExecutor>
    struct is_one_way_executor<test::wrapping_executor<BaseExecutor>>
      : is_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_never_blocking_one_way_executor<
        test::wrapping_executor<BaseExecutor>>
      : is_never_blocking_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_two_way_executor<test::wrapping_executor<BaseExecutor>>
      : is_two_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_bulk_one_way_executor<test::wrapping_executor<BaseExecutor>>
      : is_bulk_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_bulk_two_way_executor<test::wrapping_executor<BaseExecutor>>
      : is_bulk_two_way_executor<std::decay_t<BaseExecutor>>
    {
    };
}    // namespace hpx::execution::experimental

int hpx_main()
{
    std::vector<double> v(1000);
    std::iota(v.begin(), v.end(), 0.0);

    auto exec = test::wrapping_executor(hpx::execution::seq.executor());
    auto policy = hpx::execution::seq(hpx::execution::task).on(exec);

    auto f =
        hpx::experimental::for_loop(policy, 0, v.size(), [](std::size_t) {});
    f.get();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
