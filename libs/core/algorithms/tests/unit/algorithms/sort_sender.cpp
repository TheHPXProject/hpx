//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <functional>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 1007, 131073})
    {
        auto data = sender_test::values(n);
        auto original = data;
        auto expected = data;
        std::sort(expected.begin(), expected.end());
        auto sender = hpx::sort(policy, data.begin(), data.end());
        HPX_TEST(data == original);
        sender_test::wait(std::move(sender) | sender_test::ex::then([] {}));
        HPX_TEST(data == expected);
        auto ranged = sender_test::wait(hpx::ranges::sort(policy, data));
        HPX_TEST(hpx::get<0>(*ranged) == data.end());
        HPX_TEST(data == expected);

        sender_test::wait(sender_test::ex::just(
                              data.begin(), data.end(), std::greater<int>{}) |
            hpx::sort(policy) | sender_test::ex::then([] {}));
        std::reverse(expected.begin(), expected.end());
        HPX_TEST(data == expected);
    }
}

void test_exceptions()
{
    auto throws_runtime = [](int, int) -> bool {
        throw std::runtime_error("sort comparator failed");
    };
    auto throws_allocation = [](int, int) -> bool { throw std::bad_alloc(); };
    auto data = sender_test::values(131073);
    try
    {
        hpx::sort(
            hpx::execution::par, data.begin(), data.end(), throws_runtime);
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& errors)
    {
        HPX_TEST(errors.size() != 0);
        for (auto const& error : errors)
        {
            try
            {
                std::rethrow_exception(error);
            }
            catch (std::runtime_error const& e)
            {
                HPX_TEST_EQ(std::string(e.what()), "sort comparator failed");
            }
            catch (...)
            {
                HPX_TEST(false);
            }
        }
    }
    bool caught_bad_alloc = false;
    try
    {
        hpx::sort(
            hpx::execution::par, data.begin(), data.end(), throws_allocation);
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    HPX_TEST(caught_bad_alloc);
}

int hpx_main()
{
    sender_test::policies([](auto policy) { test(policy); });
    test_exceptions();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
