//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <atomic>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace {
    template <typename T>
    auto value(T&& result)
    {
        if constexpr (hpx::traits::is_future_v<std::decay_t<T>>)
            return result.get();
        else
            return HPX_FORWARD(T, result);
    }

    template <typename Policy>
    void test(Policy policy)
    {
        using namespace hpx::ranges;
        std::allocator<std::string> allocator;
        auto storage = allocator.allocate(2);
        auto output = std::ranges::subrange(storage, storage + 2);
        std::vector<std::string> input{"one", "two", "three"};
        auto copied = value(uninitialized_copy(policy, input, output));
        HPX_TEST(copied.in == input.begin() + 2);
        HPX_TEST(copied.out == output.end());
        HPX_TEST_EQ(storage[0], "one");
        HPX_TEST_EQ(storage[1], "two");
        HPX_TEST(value(destroy(policy, output)) == output.end());
        copied = value(uninitialized_copy_n(
            policy, input.begin(), -1, storage, storage + 2));
        HPX_TEST(copied.in == input.begin());
        HPX_TEST(copied.out == storage);
        copied = value(uninitialized_copy_n(
            policy, input.begin(), 3, storage, storage + 2));
        HPX_TEST(copied.in == input.begin() + 2);
        HPX_TEST(value(destroy_n(policy, storage, 2)) == storage + 2);
        HPX_TEST(
            value(uninitialized_fill(policy, output, {"x"})) == output.end());
        HPX_TEST_EQ(storage[0], "x");
        value(destroy(policy, output));
        HPX_TEST(value(uninitialized_fill_n(policy, storage, 2, {"y"})) ==
            storage + 2);
        HPX_TEST_EQ(storage[1], "y");
        value(destroy(policy, output));
        HPX_TEST(value(uninitialized_default_construct(policy, output)) ==
            output.end());
        HPX_TEST(storage[0].empty());
        value(destroy(policy, output));
        HPX_TEST(value(uninitialized_default_construct_n(policy, storage, 2)) ==
            storage + 2);
        value(destroy(policy, output));
        HPX_TEST(value(uninitialized_value_construct(policy, output)) ==
            output.end());
        value(destroy(policy, output));
        HPX_TEST(value(uninitialized_value_construct_n(policy, storage, -1)) ==
            storage);
        HPX_TEST(value(uninitialized_value_construct_n(policy, storage, 2)) ==
            storage + 2);
        value(destroy(policy, output));
        auto moved = value(uninitialized_move(policy, input, output));
        HPX_TEST(moved.in == input.begin() + 2);
        HPX_TEST(moved.out == output.end());
        HPX_TEST_EQ(storage[0], "one");
        HPX_TEST_EQ(storage[1], "two");
        value(destroy(policy, output));
        input = {"four", "five", "six"};
        moved = value(uninitialized_move_n(
            policy, input.begin(), 3, storage, storage + 2));
        HPX_TEST(moved.in == input.begin() + 2);
        HPX_TEST_EQ(storage[1], "five");
        value(destroy(policy, output));
        allocator.deallocate(storage, 2);

        using algorithm = decltype(uninitialized_value_construct);
        static_assert(
            !std::is_invocable_v<algorithm, Policy, std::vector<bool>&>);
        static_assert(
            std::is_invocable_v<algorithm, Policy, std::vector<int>&>);
    }

    struct tracked
    {
        inline static std::atomic<int> alive{0};
        inline static std::atomic<int> copies{0};

        tracked()
        {
            ++alive;
        }
        tracked(tracked const&)
        {
            if (++copies == 4)
                throw std::runtime_error("construction failed");
            ++alive;
        }
        ~tracked()
        {
            --alive;
        }
    };

    template <typename Policy>
    void test_cleanup(Policy policy)
    {
        std::vector<tracked> input(16);
        std::allocator<tracked> allocator;
        auto storage = allocator.allocate(16);
        tracked::copies = 0;
        bool caught = false;
        try
        {
            value(hpx::ranges::uninitialized_copy(
                policy, input, std::ranges::subrange(storage, storage + 16)));
        }
        catch (hpx::exception_list const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
        HPX_TEST_EQ(tracked::alive.load(), 16);
        allocator.deallocate(storage, 16);
    }
}    // namespace

int hpx_main()
{
    using namespace hpx::execution;
    test(seq);
    test(par);
    test(par_unseq);
    test(seq(task));
    test(par(task));
    test_cleanup(seq);
    test_cleanup(par);
    test_cleanup(seq(task));
    test_cleanup(par(task));
    HPX_TEST_EQ(tracked::alive.load(), 0);
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
