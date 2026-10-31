//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #7945: hpx::unwrap fails to compile for a container of containers without
// futures, which breaks hpx::reduce of std::string

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>
#include <hpx/unwrap.hpp>

#include <functional>
#include <string>
#include <vector>

void test_unwrap_nested_containers()
{
    // Container of containers without futures
    {
        auto res = hpx::unwrap(std::vector<std::vector<int>>{{1, 2}, {3, 4}});
        HPX_TEST_EQ(res.size(), 2u);
        HPX_TEST_EQ(res[0][0], 1);
        HPX_TEST_EQ(res[0][1], 2);
        HPX_TEST_EQ(res[1][0], 3);
        HPX_TEST_EQ(res[1][1], 4);
    }

    // Container of strings (where string is also a container of char)
    {
        auto res = hpx::unwrap(std::vector<std::string>{"a", "b"});
        HPX_TEST_EQ(res.size(), 2u);
        HPX_TEST_EQ(res[0], std::string("a"));
        HPX_TEST_EQ(res[1], std::string("b"));
    }

    {
        auto unwrapper = hpx::unwrapping(
            [](std::vector<std::string> values) { return values; });
        auto res = unwrapper(std::vector<std::string>{"a", "b"});
        HPX_TEST_EQ(res.size(), 2u);
        HPX_TEST_EQ(res[0], std::string("a"));
        HPX_TEST_EQ(res[1], std::string("b"));
    }

    // Parallel reduce of strings
    {
        std::vector<std::string> s(16, "a");
        auto r = hpx::reduce(hpx::execution::par, s.begin(), s.end(),
            std::string(), std::plus<>());
        HPX_TEST_EQ(r, std::string(16, 'a'));
    }

    {
        std::vector<std::string> s(16, "a");
        auto r = hpx::reduce(hpx::execution::par(hpx::execution::task),
            s.begin(), s.end(), std::string(), std::plus<>());
        HPX_TEST_EQ(r.get(), std::string(16, 'a'));
    }
}

int hpx_main()
{
    test_unwrap_nested_containers();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
