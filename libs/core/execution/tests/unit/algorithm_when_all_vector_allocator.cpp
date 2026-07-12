//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies that when_all_vector correctly uses a custom allocator
// provided via the P2300 receiver environment (get_allocator query).

#include <hpx/config.hpp>

#include <hpx/functional/invoke.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

// Shared counters for all tracking_allocator rebinds
struct tracking_allocator_counts
{
    static std::atomic<std::size_t> allocate_count;
    static std::atomic<std::size_t> deallocate_count;

    static void reset() noexcept
    {
        allocate_count.store(0, std::memory_order_relaxed);
        deallocate_count.store(0, std::memory_order_relaxed);
    }
};

std::atomic<std::size_t> tracking_allocator_counts::allocate_count{0};
std::atomic<std::size_t> tracking_allocator_counts::deallocate_count{0};

// A tracking allocator that satisfies stdexec's __simple_allocator concept.
// Uses std::byte as value_type so that allocate()/deallocate() are valid.
// All rebinds share the same static counters.
template <typename T = std::byte>
struct tracking_allocator
{
    using value_type = T;

    tracking_allocator() noexcept = default;

    template <typename U>
    // cppcheck-suppress noExplicitConstructor
    tracking_allocator(tracking_allocator<U> const&) noexcept
    {
    }

    T* allocate(std::size_t n)
    {
        tracking_allocator_counts::allocate_count.fetch_add(
            1, std::memory_order_relaxed);
        return std::allocator<T>{}.allocate(n);
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        tracking_allocator_counts::deallocate_count.fetch_add(
            1, std::memory_order_relaxed);
        std::allocator<T>{}.deallocate(p, n);
    }

    friend bool operator==(
        tracking_allocator const&, tracking_allocator const&) noexcept
    {
        return true;
    }

    friend bool operator!=(
        tracking_allocator const&, tracking_allocator const&) noexcept
    {
        return false;
    }
};

// A receiver whose environment exposes a tracking_allocator via
// the P2300 get_allocator query.
template <typename F>
struct allocator_receiver
{
    using receiver_concept = ex::receiver_t;

    std::decay_t<F> f;
    std::atomic<bool>& set_value_called;

    template <typename E>
    void set_error(E&&) && noexcept
    {
        HPX_TEST(false);
    }

    void set_stopped() && noexcept
    {
        HPX_TEST(false);
    }

    template <typename... Ts>
    void set_value(Ts&&... ts) && noexcept
    {
        HPX_INVOKE(f, std::forward<Ts>(ts)...);
        set_value_called = true;
    }

    // Environment that provides the tracking allocator
    struct env_type
    {
        auto query(ex::get_allocator_t) const noexcept
        {
            return tracking_allocator<>{};
        }
    };

    env_type get_env() const noexcept
    {
        return {};
    }
};

int main()
{
    // Test 1: when_all_vector with 3 senders using a custom allocator.
    // Verify that the tracking allocator's allocate() is called.
    {
        tracking_allocator_counts::reset();

        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(
            std::vector{ex::just(10), ex::just(20), ex::just(30)});

        auto f = [](std::vector<int> v) {
            HPX_TEST_EQ(v.size(), std::size_t(3));
            HPX_TEST_EQ(v[0], 10);
            HPX_TEST_EQ(v[1], 20);
            HPX_TEST_EQ(v[2], 30);
        };

        {
            allocator_receiver<decltype(f)> r{f, set_value_called};
            auto os = ex::connect(std::move(s), std::move(r));
            ex::start(os);
        }

        HPX_TEST(set_value_called);
        HPX_TEST_LT(
            std::size_t(0), tracking_allocator_counts::allocate_count.load());
        HPX_TEST_LT(
            std::size_t(0), tracking_allocator_counts::deallocate_count.load());
    }

    // Test 2: when_all_vector with 0 senders (empty vector).
    // An allocation of size 0 happens for op states.
    {
        tracking_allocator_counts::reset();

        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(std::vector<decltype(ex::just(0))>{});

        auto f = [](std::vector<int> v) {
            HPX_TEST_EQ(v.size(), std::size_t(0));
        };

        {
            allocator_receiver<decltype(f)> r{f, set_value_called};
            auto os = ex::connect(std::move(s), std::move(r));
            ex::start(os);
        }

        HPX_TEST(set_value_called);
        HPX_TEST_EQ(
            tracking_allocator_counts::allocate_count.load(), std::size_t(1));
        HPX_TEST_EQ(
            tracking_allocator_counts::deallocate_count.load(), std::size_t(1));
    }

    // Test 3: when_all_vector with void senders and custom allocator.
    {
        tracking_allocator_counts::reset();

        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(
            std::vector{ex::just(), ex::just(), ex::just()});

        auto f = []() {};

        {
            allocator_receiver<decltype(f)> r{f, set_value_called};
            auto os = ex::connect(std::move(s), std::move(r));
            ex::start(os);
        }

        HPX_TEST(set_value_called);
        HPX_TEST_LT(
            std::size_t(0), tracking_allocator_counts::allocate_count.load());
        HPX_TEST_LT(
            std::size_t(0), tracking_allocator_counts::deallocate_count.load());
        HPX_TEST_EQ(tracking_allocator_counts::allocate_count.load(),
            tracking_allocator_counts::deallocate_count.load());
    }

    // Test 4: Verify allocate/deallocate counts are balanced.
    {
        tracking_allocator_counts::reset();

        std::atomic<bool> set_value_called{false};
        auto s = ex::when_all_vector(std::vector{ex::just(1), ex::just(2)});

        auto f = [](std::vector<int> v) {
            HPX_TEST_EQ(v.size(), std::size_t(2));
        };

        {
            allocator_receiver<decltype(f)> r{f, set_value_called};
            auto os = ex::connect(std::move(s), std::move(r));
            ex::start(os);
        }

        HPX_TEST(set_value_called);
        // Every allocate must be paired with a deallocate
        HPX_TEST_EQ(tracking_allocator_counts::allocate_count.load(),
            tracking_allocator_counts::deallocate_count.load());
    }

    return hpx::util::report_errors();
}
