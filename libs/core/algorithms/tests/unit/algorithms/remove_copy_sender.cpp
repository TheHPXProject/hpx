//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"
#include "test_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template <bool RemoveIf, typename Policy>
void test_sender(Policy policy)
{
    for (std::size_t n : {0, 1, 6, 10007})
    {
        auto input = sender_test::values(n);
        std::vector<int> output(n, -1), expected(n, -1);
        auto pred = [](int value) { return value % 2 == 0; };
        auto expected_end = [&] {
            if constexpr (RemoveIf)
            {
                return std::remove_copy_if(
                    input.begin(), input.end(), expected.begin(), pred);
            }
            else
            {
                return std::remove_copy(
                    input.begin(), input.end(), expected.begin(), 2);
            }
        }();
        auto sender = [&] {
            if constexpr (RemoveIf)
            {
                return sender_test::ex::just(
                           input.begin(), input.end(), output.begin(), pred) |
                    hpx::remove_copy_if(policy);
            }
            else
            {
                return sender_test::ex::just(
                           input.begin(), input.end(), output.begin(), 2) |
                    hpx::remove_copy(policy);
            }
        }();
        HPX_TEST(std::all_of(output.begin(), output.end(),
            [](int value) { return value == -1; }));
        auto result = sender_test::wait(std::move(sender));
        HPX_TEST(hpx::get<0>(*result) - output.begin() ==
            expected_end - expected.begin());
        HPX_TEST(output == expected);
    }
}

template <bool RemoveIf, typename Exception>
void test_error()
{
    using base_iterator = std::vector<int>::iterator;
    using iterator = test::decorated_iterator<base_iterator,
        std::random_access_iterator_tag>;

    auto input = sender_test::values(10007);
    std::vector<int> output(input.size());
    auto exec = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = hpx::execution::par(hpx::execution::task).on(exec);
    auto first = iterator(input.begin(), [] {
        if constexpr (std::is_same_v<Exception, std::bad_alloc>)
        {
            throw std::bad_alloc();
        }
        else
        {
            throw std::runtime_error("remove copy failed");
        }
    });
    auto sender = [&] {
        if constexpr (RemoveIf)
        {
            return hpx::remove_copy_if(policy, first, iterator(input.end()),
                output.begin(), [](int value) { return value % 2 == 0; });
        }
        else
        {
            return hpx::remove_copy(
                policy, first, iterator(input.end()), output.begin(), 2);
        }
    }();
    try
    {
        (void) sender_test::tt::sync_wait(std::move(sender));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& errors)
    {
        if constexpr (std::is_same_v<Exception, std::runtime_error>)
        {
            HPX_TEST(errors.size() != 0);
            for (auto const& error : errors)
            {
                try
                {
                    std::rethrow_exception(error);
                }
                catch (std::runtime_error const&)
                {
                    continue;
                }
                catch (...)
                {
                    HPX_TEST(false);
                }
                HPX_TEST(false);
            }
        }
        else
        {
            HPX_TEST(false);
        }
    }
    catch (std::bad_alloc const&)
    {
        HPX_TEST((std::is_same_v<Exception, std::bad_alloc>) );
    }
}

template <bool RemoveIf>
void test_synchronous_policy()
{
    auto input = sender_test::values(17);
    input[3] = 2;
    input[9] = 2;
    std::vector<int> output(input.size(), -1), expected(input.size(), -1);
    auto pred = [](int value) { return value % 2 == 0; };
    auto expected_end = [&] {
        if constexpr (RemoveIf)
        {
            return std::remove_copy_if(
                input.begin(), input.end(), expected.begin(), pred);
        }
        else
        {
            return std::remove_copy(
                input.begin(), input.end(), expected.begin(), 2);
        }
    }();
    auto check_result = [&](auto policy) {
        std::fill(output.begin(), output.end(), -1);
        auto result = [&] {
            if constexpr (RemoveIf)
            {
                return hpx::remove_copy_if(
                    policy, input.begin(), input.end(), output.begin(), pred);
            }
            else
            {
                return hpx::remove_copy(
                    policy, input.begin(), input.end(), output.begin(), 2);
            }
        }();
        HPX_TEST(result - output.begin() == expected_end - expected.begin());
        HPX_TEST(output == expected);
    };
    auto sync = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::sync));
    auto async = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::async));
    check_result(hpx::execution::seq.on(sync));
    check_result(hpx::execution::par.on(async));
}

template <bool RemoveIf>
void test_parallel()
{
    std::vector<int> input(65536, 1), output(input.size(), -1);
    auto exec = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = hpx::execution::par(hpx::execution::task).on(exec);
    auto sender = [&] {
        if constexpr (RemoveIf)
        {
            return hpx::remove_copy_if(policy, input.begin(), input.end(),
                output.begin(), [](int) { return false; });
        }
        else
        {
            return hpx::remove_copy(
                policy, input.begin(), input.end(), output.begin(), 2);
        }
    }();
    auto result = sender_test::wait(std::move(sender));
    HPX_TEST(hpx::get<0>(*result) == output.end());
    HPX_TEST(output == input);
}

int hpx_main()
{
#if defined(_MSC_VER)
    // Limit scan sender instantiations to keep MSVC compiler memory bounded.
    auto exec = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = hpx::execution::par(hpx::execution::task).on(HPX_MOVE(exec));
    test_sender<false>(policy);
    test_sender<true>(policy);
#else
    sender_test::policies([](auto policy) {
        test_sender<false>(policy);
        test_sender<true>(policy);
    });
    test_synchronous_policy<false>();
    test_synchronous_policy<true>();
    test_error<false, std::runtime_error>();
    test_error<false, std::bad_alloc>();
    test_error<true, std::runtime_error>();
    test_error<true, std::bad_alloc>();
    test_parallel<false>();
    test_parallel<true>();
#endif
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
