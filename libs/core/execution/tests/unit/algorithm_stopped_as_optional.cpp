//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/execution/algorithms/stopped_as_optional.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

void test_value_passes_through()
{
    auto result = hpx::this_thread::experimental::sync_wait(
        ex::stopped_as_optional(ex::just(42)));

    HPX_TEST(result.has_value());
    auto opt = hpx::get<0>(*result);
    HPX_TEST(opt.has_value());
    HPX_TEST_EQ(*opt, 42);
}

void test_value_string()
{
    auto result = hpx::this_thread::experimental::sync_wait(
        ex::stopped_as_optional(ex::just(std::string("hello"))));

    HPX_TEST(result.has_value());
    auto opt = hpx::get<0>(*result);
    HPX_TEST(opt.has_value());
    HPX_TEST_EQ(*opt, std::string("hello"));
}

void test_error_propagates()
{
    bool caught = false;
    try
    {
        auto s = ex::then(ex::just(),
            []() -> int { throw std::runtime_error("test error"); });
        hpx::this_thread::experimental::sync_wait(
            ex::stopped_as_optional(std::move(s)));
    }
    catch (std::runtime_error const&)
    {
        caught = true;
    }
    HPX_TEST(caught);
}

void test_pipe_operator()
{
    auto result = hpx::this_thread::experimental::sync_wait(
        ex::just(99) | ex::stopped_as_optional());

    HPX_TEST(result.has_value());
    auto opt = hpx::get<0>(*result);
    HPX_TEST(opt.has_value());
    HPX_TEST_EQ(*opt, 99);
}

int hpx_main()
{
    test_value_passes_through();
    test_value_string();
    test_error_propagates();
    test_pipe_operator();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
