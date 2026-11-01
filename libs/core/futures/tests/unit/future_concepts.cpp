//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Compile-time verification that the companion concepts for the future
// family of type traits agree with is_*_v. Nothing here executes at
// runtime; if this file compiles, every static_assert has already been
// checked by the compiler.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/tuple.hpp>

#include <functional>
#include <vector>

///////////////////////////////////////////////////////////////////////////
static_assert(hpx::future_type<hpx::future<int>>);
static_assert(hpx::future_type<hpx::shared_future<int>>);
static_assert(!hpx::future_type<int>);
static_assert(hpx::future_type<hpx::future<int>> ==
    hpx::traits::is_future_v<hpx::future<int>>);
static_assert(hpx::future_type<int> == hpx::traits::is_future_v<int>);

static_assert(hpx::unique_future<hpx::future<int>>);
static_assert(!hpx::unique_future<hpx::shared_future<int>>);
static_assert(hpx::unique_future<hpx::future<int>> ==
    hpx::traits::is_unique_future_v<hpx::future<int>>);

static_assert(
    hpx::ref_wrapped_future<std::reference_wrapper<hpx::future<int>>>);
static_assert(!hpx::ref_wrapped_future<hpx::future<int>>);
static_assert(
    hpx::ref_wrapped_future<std::reference_wrapper<hpx::future<int>>> ==
    hpx::traits::is_ref_wrapped_future_v<
        std::reference_wrapper<hpx::future<int>>>);

static_assert(hpx::future_range<std::vector<hpx::future<int>>>);
static_assert(!hpx::future_range<std::vector<int>>);
static_assert(hpx::future_range<std::vector<hpx::future<int>>> ==
    hpx::traits::is_future_range_v<std::vector<hpx::future<int>>>);

static_assert(hpx::ref_wrapped_future_range<
    std::reference_wrapper<std::vector<hpx::future<int>>>>);
static_assert(!hpx::ref_wrapped_future_range<std::vector<hpx::future<int>>>);

static_assert(
    hpx::future_tuple<hpx::tuple<hpx::future<int>, hpx::future<double>>>);
static_assert(!hpx::future_tuple<hpx::tuple<int, double>>);
static_assert(
    hpx::future_tuple<hpx::tuple<hpx::future<int>, hpx::future<double>>> ==
    hpx::traits::is_future_tuple_v<
        hpx::tuple<hpx::future<int>, hpx::future<double>>>);

///////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
