//  Copyright (c) 2017-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <type_traits>
#include <utility>

struct test_async_executor
{
    using execution_category = hpx::execution::parallel_execution_tag;

    template <typename F, typename... Ts>
    decltype(auto) async_execute(F&& f, Ts&&... ts) const
    {
        return hpx::dataflow(
            hpx::launch::async, std::forward<F>(f), std::forward<Ts>(ts)...);
    }
};

namespace hpx::execution::experimental {

    template <>
    struct is_two_way_executor<test_async_executor> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

int hpx_main()
{
    test_async_executor exec;
    hpx::future<void> f = hpx::make_ready_future();
    f.then(exec, [](hpx::future<void>&& f) { f.get(); });

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
