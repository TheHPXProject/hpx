//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>

namespace ex = hpx::execution::experimental;

// Dummy allocator for testing custom environment
struct custom_allocator
{
    using value_type = std::byte;

    custom_allocator() = default;

    template <typename U>
    constexpr custom_allocator(custom_allocator const&) noexcept
    {
    }

    [[nodiscard]] std::byte* allocate(std::size_t n)
    {
        return static_cast<std::byte*>(::operator new(n));
    }

    void deallocate(std::byte* p, std::size_t) noexcept
    {
        ::operator delete(p);
    }

    friend constexpr bool operator==(
        custom_allocator const&, custom_allocator const&) noexcept
    {
        return true;
    }

    friend constexpr bool operator!=(
        custom_allocator const&, custom_allocator const&) noexcept
    {
        return false;
    }
};

// Custom environment that overrides get_allocator
struct custom_env
{
    constexpr custom_allocator query(ex::get_allocator_t) const noexcept
    {
        return custom_allocator{};
    }
};

struct custom_sender
{
    using sender_concept = ex::sender_t;

    constexpr custom_env get_env() const noexcept
    {
        return custom_env{};
    }
};

// Custom scheduler wrapper that provides the custom environment
struct custom_scheduler
{
    friend constexpr bool operator==(
        custom_scheduler const&, custom_scheduler const&) noexcept
    {
        return true;
    }
    friend constexpr bool operator!=(
        custom_scheduler const&, custom_scheduler const&) noexcept
    {
        return false;
    }

    constexpr custom_sender schedule() const noexcept
    {
        return custom_sender{};
    }
};

#include <hpx/init.hpp>

int hpx_main()
{
    {
        // Test: hpx::execution::experimental::thread_pool_scheduler
        ex::thread_pool_scheduler sched{};

        // In P2300, get_allocator is queried on the environment of the sender
        // produced by the scheduler (or potentially on the environment of the
        // scheduler itself, if it provides one).
        auto sndr = ex::schedule(sched);
        auto env = ex::get_env(sndr);

        // The query should return std::allocator<std::byte> for thread_pool_scheduler
        auto alloc = ex::get_allocator(env);
        HPX_TEST((std::is_same_v<decltype(alloc), std::allocator<std::byte>>) );
    }

    {
        // Test: Custom scheduler wrapper overriding the allocator
        custom_scheduler sched{};
        auto sndr = ex::schedule(sched);
        auto env = ex::get_env(sndr);

        auto alloc = ex::get_allocator(env);
        HPX_TEST((std::is_same_v<decltype(alloc), custom_allocator>) );
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
