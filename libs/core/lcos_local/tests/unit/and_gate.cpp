//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// and_gate hides base_and_gate's lock-free public overloads of get_future()/
// get_shared_future()/set()/synchronize() (C++ name hiding), so tests against
// and_gate always need to pass a Lock&. The condition variable used internally
// to wake up synchronize() waiters only accepts
// std::unique_lock<hpx::spinlock>, so that is the lock type used throughout.
using lock_type = std::unique_lock<hpx::spinlock>;

///////////////////////////////////////////////////////////////////////////////
void test_get_future_and_set()
{
    hpx::lcos::local::and_gate gate(3);
    hpx::spinlock mtx;

    lock_type l(mtx);
    hpx::future<void> f = gate.get_future(l);
    HPX_TEST(f.valid());
    HPX_TEST(!f.is_ready());

    HPX_TEST(!gate.set(0, l));
    HPX_TEST(!f.is_ready());
    HPX_TEST(!gate.set(1, l));
    HPX_TEST(!f.is_ready());
    HPX_TEST(gate.set(2, l));    // last segment arrives, gate fires

    f.get();
}

void test_get_shared_future_and_set()
{
    hpx::lcos::local::and_gate gate(2);
    hpx::spinlock mtx;

    lock_type l(mtx);
    hpx::shared_future<void> const f = gate.get_shared_future(l);
    HPX_TEST(f.valid());
    HPX_TEST(!f.is_ready());

    HPX_TEST(!gate.set(0, l));
    HPX_TEST(!f.is_ready());
    HPX_TEST(gate.set(1, l));    // last segment arrives, gate fires

    f.get();
}

///////////////////////////////////////////////////////////////////////////////
void test_set_error_paths()
{
    hpx::lcos::local::and_gate gate(2);
    hpx::spinlock mtx;

    // set() with an out-of-range index
    {
        lock_type l(mtx);
        hpx::error_code ec(hpx::throwmode::lightweight);
        HPX_TEST(!gate.set(5, l, nullptr, ec));
        HPX_TEST(ec);
        HPX_TEST_EQ(ec.value(), static_cast<int>(hpx::error::bad_parameter));

        // set() unlocks l before reporting the error
        HPX_TEST(!l.owns_lock());
    }

    // set() an index successfully
    {
        lock_type l(mtx);
        HPX_TEST(!gate.set(0, l));
    }

    // set() the same index a second time
    {
        lock_type l(mtx);
        hpx::error_code ec(hpx::throwmode::lightweight);
        HPX_TEST(!gate.set(0, l, nullptr, ec));
        HPX_TEST(ec);
        HPX_TEST_EQ(ec.value(), static_cast<int>(hpx::error::bad_parameter));

        // set() unlocks l before reporting the error
        HPX_TEST(!l.owns_lock());
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_synchronize_error_paths()
{
    hpx::lcos::local::and_gate gate;
    hpx::spinlock mtx;

    {
        lock_type l(mtx);
        gate.next_generation(l);    // generation: 1 -> 2
        gate.next_generation(l);    // generation: 2 -> 3
        HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(3));
    }

    // requesting a generation smaller than the current one is a sequencing
    // error
    {
        lock_type l(mtx);
        hpx::error_code ec(hpx::throwmode::lightweight);
        gate.synchronize(1, l, "test_synchronize_error_paths", ec);
        HPX_TEST(ec);
        HPX_TEST_EQ(ec.value(), static_cast<int>(hpx::error::invalid_status));

        // synchronize() unlocks l before reporting the error
        HPX_TEST(!l.owns_lock());
    }

    // synchronizing on the current generation returns immediately, without
    // blocking, and without an error
    {
        lock_type l(mtx);
        hpx::error_code ec(hpx::throwmode::lightweight);
        gate.synchronize(3, l, "test_synchronize_error_paths", ec);
        HPX_TEST(!ec);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Equivalent to test_set_error_paths(), but omitting the explicit ec argument
// to verify that the same error conditions are reported by throwing
// hpx::exception instead.
void test_set_error_paths_throws()
{
    hpx::lcos::local::and_gate gate(2);
    hpx::spinlock mtx;

    // set() with an out-of-range index
    {
        lock_type l(mtx);
        HPX_TEST_THROW(gate.set(5, l), hpx::exception);
    }
    {
        lock_type l(mtx);
        try
        {
            gate.set(5, l);
            HPX_TEST(false);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
        }
    }

    // set() an index successfully
    {
        lock_type l(mtx);
        HPX_TEST(!gate.set(0, l));
    }

    // set() the same index a second time
    {
        lock_type l(mtx);
        HPX_TEST_THROW(gate.set(0, l), hpx::exception);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Equivalent to test_synchronize_error_paths(), but omitting the explicit ec
// argument to verify that the same error conditions are reported by throwing
// hpx::exception instead.
void test_synchronize_error_paths_throws()
{
    hpx::lcos::local::and_gate gate;
    hpx::spinlock mtx;

    {
        lock_type l(mtx);
        gate.next_generation(l);    // generation: 1 -> 2
        gate.next_generation(l);    // generation: 2 -> 3
        HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(3));
    }

    // requesting a generation smaller than the current one is a sequencing
    // error
    {
        lock_type l(mtx);
        HPX_TEST_THROW(
            gate.synchronize(1, l, "test_synchronize_error_paths_throws"),
            hpx::exception);
    }
    {
        lock_type l(mtx);
        try
        {
            gate.synchronize(1, l, "test_synchronize_error_paths_throws");
            HPX_TEST(false);
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_EQ(e.get_error(), hpx::error::invalid_status);
        }
    }

    // synchronizing on the current generation returns immediately, without
    // blocking, and without an error
    {
        lock_type l(mtx);
        gate.synchronize(3, l, "test_synchronize_error_paths_throws");
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_next_generation_triggers_waiters()
{
    hpx::lcos::local::base_and_gate<hpx::spinlock> gate;
    HPX_TEST_EQ(gate.generation(), static_cast<std::size_t>(1));

    hpx::future<void> f = hpx::async([&gate]() { gate.synchronize(2); });

    // the future must not complete before the gate's generation counter
    // reaches the requested value
    HPX_TEST(!f.is_ready());

    std::size_t const gen = gate.next_generation();
    HPX_TEST_EQ(gen, static_cast<std::size_t>(2));

    f.get();
}

void test_next_generation_error_paths()
{
    hpx::lcos::local::base_and_gate<hpx::spinlock> gate;

    gate.next_generation();    // generation: 1 -> 2
    gate.next_generation();    // generation: 2 -> 3
    HPX_TEST_EQ(gate.generation(), static_cast<std::size_t>(3));

    hpx::error_code ec(hpx::throwmode::lightweight);
    std::size_t const gen = gate.next_generation(1, ec);
    HPX_TEST(ec);
    HPX_TEST_EQ(ec.value(), static_cast<int>(hpx::error::invalid_status));
    HPX_TEST_EQ(gen, gate.generation());
    HPX_TEST_EQ(gate.generation(), static_cast<std::size_t>(3));    // unchanged
}

///////////////////////////////////////////////////////////////////////////////
// next_generation(new_generation) assigns the requested value and then
// increments it, so requesting generation 10 leaves the gate at 11, not 10.
void test_next_generation_explicit_value()
{
    hpx::lcos::local::base_and_gate<hpx::spinlock> gate;

    gate.next_generation();    // generation: 1 -> 2
    gate.next_generation();    // generation: 2 -> 3
    HPX_TEST_EQ(gate.generation(), static_cast<std::size_t>(3));

    std::size_t const gen = gate.next_generation(10);
    HPX_TEST_EQ(gen, static_cast<std::size_t>(11));
    HPX_TEST_EQ(gate.generation(), static_cast<std::size_t>(11));
}

///////////////////////////////////////////////////////////////////////////////
// Equivalent to test_next_generation_error_paths(), but omitting the explicit
// ec argument to verify that the same error condition is reported by throwing
// hpx::exception instead.
void test_next_generation_error_paths_throws()
{
    hpx::lcos::local::base_and_gate<hpx::spinlock> gate;

    gate.next_generation();    // generation: 1 -> 2
    gate.next_generation();    // generation: 2 -> 3
    HPX_TEST_EQ(gate.generation(), static_cast<std::size_t>(3));

    HPX_TEST_THROW(gate.next_generation(1), hpx::exception);

    try
    {
        gate.next_generation(1);
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::invalid_status);
    }

    HPX_TEST_EQ(gate.generation(), static_cast<std::size_t>(3));    // unchanged
}

///////////////////////////////////////////////////////////////////////////////
void test_generation_accessor()
{
    hpx::lcos::local::and_gate gate;
    hpx::spinlock mtx;

    lock_type l(mtx);
    HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(1));

    std::size_t const gen = gate.next_generation(l);
    HPX_TEST_EQ(gen, static_cast<std::size_t>(2));
    HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(2));
}

///////////////////////////////////////////////////////////////////////////////
void test_move_construction()
{
    hpx::lcos::local::and_gate gate(2);
    hpx::spinlock mtx;

    lock_type l(mtx);
    gate.next_generation(l);
    HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(2));

    hpx::lcos::local::and_gate const moved(std::move(gate));
    HPX_TEST_EQ(moved.generation(l), static_cast<std::size_t>(2));

    // NOLINTNEXTLINE(bugprone-use-after-move)
    HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(-1));
}

void test_move_assignment()
{
    hpx::lcos::local::and_gate gate(2);
    hpx::spinlock mtx;

    lock_type l(mtx);
    gate.next_generation(l);
    HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(2));

    hpx::lcos::local::and_gate target;
    target = std::move(gate);

    HPX_TEST_EQ(target.generation(l), static_cast<std::size_t>(2));

    // NOLINTNEXTLINE(bugprone-use-after-move)
    HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(-1));
}

///////////////////////////////////////////////////////////////////////////////
// Verify that concurrent set() invocations on a base_and_gate<hpx::spinlock>
// (which is internally synchronized) only trigger the completion callback
// exactly once, no matter how many threads race to deliver the last segment.
void test_concurrent_set()
{
    constexpr std::size_t num_threads = 4;

    hpx::lcos::local::base_and_gate<hpx::spinlock> gate(num_threads);
    std::atomic<std::size_t> fired(0);

    auto callback = [&fired](auto&, auto&, hpx::error_code const&) { ++fired; };

    std::vector<hpx::future<void>> results;
    results.reserve(num_threads);
    for (std::size_t i = 0; i != num_threads; ++i)
    {
        results.push_back(
            hpx::async([&gate, &callback, i]() { gate.set(i, callback); }));
    }

    hpx::wait_all(results);

    HPX_TEST_EQ(fired.load(), static_cast<std::size_t>(1));
}

///////////////////////////////////////////////////////////////////////////////
// Verify that an and_gate can be reused across several generations, similar to
// how a barrier is reused across several rounds of synchronization.
void test_repeated_generations()
{
    constexpr std::size_t num_participants = 3;

    hpx::lcos::local::and_gate gate(num_participants);
    hpx::spinlock mtx;

    for (std::size_t cycle = 0; cycle != 3; ++cycle)
    {
        lock_type l(mtx);

        hpx::future<void> f = gate.get_future(l);
        HPX_TEST(!f.is_ready());

        for (std::size_t i = 0; i != num_participants; ++i)
        {
            bool const last_segment = (i + 1 == num_participants);
            HPX_TEST_EQ(gate.set(i, l), last_segment);
            HPX_TEST(last_segment || !f.is_ready());
        }

        f.get();

        gate.next_generation(l);
    }

    lock_type l(mtx);
    HPX_TEST_EQ(gate.generation(l), static_cast<std::size_t>(1 + 3));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_get_future_and_set();
    test_get_shared_future_and_set();
    test_set_error_paths();
    test_set_error_paths_throws();
    test_synchronize_error_paths();
    test_synchronize_error_paths_throws();
    test_next_generation_triggers_waiters();
    test_next_generation_error_paths();
    test_next_generation_error_paths_throws();
    test_next_generation_explicit_value();
    test_generation_accessor();
    test_move_construction();
    test_move_assignment();
    test_concurrent_set();
    test_repeated_generations();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
