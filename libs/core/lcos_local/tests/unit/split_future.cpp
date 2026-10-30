//  Copyright (c) 2016-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
hpx::tuple<> make_tuple0_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return hpx::make_tuple();
}

void test_split_future0()
{
    hpx::lcos::local::futures_factory<hpx::tuple<>()> pt(make_tuple0_slowly);
    pt.post();

    hpx::tuple<hpx::future<void>> result = hpx::split_future(pt.get_future());

    hpx::get<0>(result).get();
}

///////////////////////////////////////////////////////////////////////////////
hpx::tuple<int> make_tuple1_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return hpx::make_tuple(42);
}

void test_split_future1()
{
    hpx::lcos::local::futures_factory<hpx::tuple<int>()> pt(make_tuple1_slowly);
    pt.post();

    hpx::tuple<hpx::future<int>> result = hpx::split_future(pt.get_future());

    HPX_TEST_EQ(hpx::get<0>(result).get(), 42);
}

///////////////////////////////////////////////////////////////////////////////
hpx::tuple<int, int> make_tuple2_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return hpx::make_tuple(42, 43);
}

void test_split_future2()
{
    hpx::lcos::local::futures_factory<hpx::tuple<int, int>()> pt(
        make_tuple2_slowly);
    pt.post();

    hpx::tuple<hpx::future<int>, hpx::future<int>> result =
        hpx::split_future(pt.get_future());

    HPX_TEST_EQ(hpx::get<0>(result).get(), 42);
    HPX_TEST_EQ(hpx::get<1>(result).get(), 43);
}

///////////////////////////////////////////////////////////////////////////////
hpx::tuple<int, int, int> make_tuple3_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return hpx::make_tuple(42, 43, 44);
}

void test_split_future3()
{
    hpx::lcos::local::futures_factory<hpx::tuple<int, int, int>()> pt(
        make_tuple3_slowly);
    pt.post();

    hpx::tuple<hpx::future<int>, hpx::future<int>, hpx::future<int>> result =
        hpx::split_future(pt.get_future());

    HPX_TEST_EQ(hpx::get<0>(result).get(), 42);
    HPX_TEST_EQ(hpx::get<1>(result).get(), 43);
    HPX_TEST_EQ(hpx::get<2>(result).get(), 44);
}

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_DATASTRUCTURES_HAVE_ADAPT_STD_TUPLE)
std::tuple<int, int, int> make_std_tuple_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return std::make_tuple(42, 43, 44);
}

void test_split_std_future()
{
    hpx::lcos::local::futures_factory<std::tuple<int, int, int>()> pt(
        make_std_tuple_slowly);
    pt.post();

    std::tuple<hpx::future<int>, hpx::future<int>, hpx::future<int>> result =
        hpx::split_future(pt.get_future());

    HPX_TEST_EQ(std::get<0>(result).get(), 42);
    HPX_TEST_EQ(std::get<1>(result).get(), 43);
    HPX_TEST_EQ(std::get<2>(result).get(), 44);
}
#endif

///////////////////////////////////////////////////////////////////////////////
std::pair<int, int> make_pair_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return std::make_pair(42, 43);
}

void test_split_future_pair()
{
    hpx::lcos::local::futures_factory<std::pair<int, int>()> pt(
        make_pair_slowly);
    pt.post();

    std::pair<hpx::future<int>, hpx::future<int>> result =
        hpx::split_future(pt.get_future());

    HPX_TEST_EQ(result.first.get(), 42);
    HPX_TEST_EQ(result.second.get(), 43);
}

///////////////////////////////////////////////////////////////////////////////
std::array<int, 0> make_array0_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return std::array<int, 0>();
}

void test_split_future_array0()
{
    hpx::lcos::local::futures_factory<std::array<int, 0>()> pt(
        make_array0_slowly);
    pt.post();

    std::array<hpx::future<void>, 1> result =
        hpx::split_future(pt.get_future());

    result[0].get();
}

///////////////////////////////////////////////////////////////////////////////
std::array<int, 3> make_array_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return std::array<int, 3>{{42, 43, 44}};
}

void test_split_future_array()
{
    hpx::lcos::local::futures_factory<std::array<int, 3>()> pt(
        make_array_slowly);
    pt.post();

    std::array<hpx::future<int>, 3> result = hpx::split_future(pt.get_future());

    HPX_TEST_EQ(result[0].get(), 42);
    HPX_TEST_EQ(result[1].get(), 43);
    HPX_TEST_EQ(result[2].get(), 44);
}

///////////////////////////////////////////////////////////////////////////////
std::vector<int> make_vector_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return std::vector<int>{{42, 43, 44}};
}

void test_split_future_vector()
{
    hpx::lcos::local::futures_factory<std::vector<int>()> pt(
        make_vector_slowly);
    pt.post();

    std::vector<hpx::future<int>> result =
        hpx::split_future(pt.get_future(), 3);

    HPX_TEST_EQ(result[0].get(), 42);
    HPX_TEST_EQ(result[1].get(), 43);
    HPX_TEST_EQ(result[2].get(), 44);
}

void test_split_invalid_future()
{
    auto check_no_state = [](auto& f) {
        try
        {
            f.get();
            HPX_TEST(false);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::no_state);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
    };

    // Test split_nth_continuation
    {
        hpx::future<hpx::tuple<int, int>> f_tuple;
        HPX_TEST(!f_tuple.valid());

        auto result_tuple = hpx::split_future(HPX_MOVE(f_tuple));

        check_no_state(hpx::get<0>(result_tuple));
        check_no_state(hpx::get<1>(result_tuple));
    }

#if defined(HPX_DATASTRUCTURES_HAVE_ADAPT_STD_TUPLE)
    {
        hpx::future<std::tuple<int, int>> f_tuple;
        HPX_TEST(!f_tuple.valid());

        auto result_tuple = hpx::split_future(HPX_MOVE(f_tuple));

        check_no_state(std::get<0>(result_tuple));
        check_no_state(std::get<1>(result_tuple));
    }
#endif

    // Test std::pair
    {
        hpx::future<std::pair<int, int>> f_pair;
        HPX_TEST(!f_pair.valid());

        auto result_pair = hpx::split_future(HPX_MOVE(f_pair));

        check_no_state(result_pair.first);
        check_no_state(result_pair.second);
    }

    // Test std::array
    {
        hpx::future<std::array<int, 2>> f_array;
        HPX_TEST(!f_array.valid());

        auto result_array = hpx::split_future(HPX_MOVE(f_array));

        check_no_state(result_array[0]);
        check_no_state(result_array[1]);
    }

    // Test split_continuation
    {
        hpx::future<std::vector<int>> f_vector;
        HPX_TEST(!f_vector.valid());

        auto result_vector = hpx::split_future(HPX_MOVE(f_vector), 2);
        HPX_TEST_EQ(result_vector.size(), std::size_t(2));

        for (auto& f : result_vector)
        {
            check_no_state(f);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_split_future0();
    test_split_future1();
    test_split_future2();
    test_split_future3();

#if defined(HPX_DATASTRUCTURES_HAVE_ADAPT_STD_TUPLE)
    test_split_std_future();
#endif

    test_split_future_pair();

    test_split_future_array0();
    test_split_future_array();

    test_split_future_vector();

    test_split_invalid_future();

    hpx::local::finalize();
    return hpx::util::report_errors();
}

int main(int argc, char* argv[])
{
    // We force this test to use several threads by default.
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
