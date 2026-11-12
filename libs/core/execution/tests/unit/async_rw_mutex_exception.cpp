//  Copyright (c) 2025 Hartmut Kaiser
//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Regression test for:
//   async_rw_mutex exception propagation through release()
//
// When the move of the protected value from one shared state to the next
// throws during release(), the fix in async_rw_mutex_shared_state ensures:
//   1. The exception is stored on next_state via set_error().
//   2. The continuation loop is NOT skipped (no silent hang).
//   3. Each downstream continuation detects state->exception and calls
//      set_error() on its receiver rather than constructing a valueless
//      access wrapper (which would throw std::bad_optional_access).
//   4. sync_wait re-throws the stored exception so the caller can catch it.

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

///////////////////////////////////////////////////////////////////////////////
// A value type whose move constructor throws only during release() when moving
// to the next shared state.
struct throwing_on_second_move
{
    static std::atomic<int> move_count;

    static void reset() noexcept
    {
        move_count.store(0);
    }

    int value;

    explicit throwing_on_second_move(int v) noexcept
      : value(v)
    {
    }

    // Move constructor throws only after reset() has been called.
    throwing_on_second_move(throwing_on_second_move&& other)
      : value(other.value)
    {
        if (move_count.load() >= 0)
        {
            int current_count = ++move_count;
            if (current_count > 0)
            {
                throw std::runtime_error(
                    "throwing_on_second_move: intentional move exception");
            }
        }
    }

    throwing_on_second_move& operator=(throwing_on_second_move&&) = delete;
    throwing_on_second_move(throwing_on_second_move const&) = delete;
    throwing_on_second_move& operator=(throwing_on_second_move const&) = delete;
};

std::atomic<int> throwing_on_second_move::move_count{-1};

///////////////////////////////////////////////////////////////////////////////
// Test 1: single downstream receiver gets set_error when move throws.
//
// Sequence:
//   rwm.readwrite()   -> first sender  (no prev_state, value moved in)
//   rwm.readwrite()   -> second sender (prev_state = first state)
//
// When the first access wrapper is released (wrapper destroyed after sync_wait
// on the first sender), release() moves value -> next_state, which throws.
// The fix must route the exception to the second sender's receiver.
void test_set_error_called_on_move_throw()
{
    throwing_on_second_move::move_count.store(-1);

    using mutex_type =
        hpx::experimental::async_rw_mutex<throwing_on_second_move>;

    mutex_type rwm{throwing_on_second_move{42}};

    // Obtain two senders before starting either. This registers next_state on
    // the first shared state so that release() has somewhere to forward to.
    auto sender1 = rwm.readwrite();
    auto sender2 = rwm.readwrite();

    throwing_on_second_move::reset();

    // Drive the first access: it should complete normally via set_value.
    // When the wrapper goes out of scope, release() attempts to move the value
    // to next_state, which throws the intentional exception.
    bool first_completed = false;
    try
    {
        tt::sync_wait(
            std::move(sender1) | ex::then([&first_completed](auto /*wrapper*/) {
                first_completed = true;
            }));
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "first sender threw unexpectedly");
    }
    HPX_TEST(first_completed);

    // Drive the second access: it must receive set_error with the original
    // std::runtime_error from the move failure in release().
    bool second_got_expected_error = false;
    try
    {
        tt::sync_wait(std::move(sender2) | ex::then([](auto /*wrapper*/) {
            // Must NOT reach here: the value was never successfully moved.
            HPX_TEST_MSG(false,
                "set_value was called on the second sender despite a "
                "throwing move; expected set_error");
        }));
    }
    catch (std::bad_optional_access const&)
    {
        HPX_TEST_MSG(false,
            "second sender received std::bad_optional_access; expected "
            "std::runtime_error");
    }
    catch (std::runtime_error const& e)
    {
        HPX_TEST_EQ(std::string(e.what()),
            std::string("throwing_on_second_move: intentional move exception"));
        second_got_expected_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false,
            "second sender threw unexpected exception type instead of "
            "std::runtime_error");
    }

    HPX_TEST(second_got_expected_error);
}

///////////////////////////////////////////////////////////////////////////////
// Test 2: multiple downstream receivers, all get set_error.
//
// Two consecutive readwrite senders follow the first (throwing) access.
// Both must receive set_error; neither must hang.
void test_multiple_receivers_all_get_set_error()
{
    throwing_on_second_move::move_count.store(-1);

    using mutex_type =
        hpx::experimental::async_rw_mutex<throwing_on_second_move>;

    mutex_type rwm{throwing_on_second_move{7}};

    // Create three senders: [0] is the first access,
    // [1] and [2] are downstream (should receive set_error after the move
    // throws in release()).
    auto sender0 = rwm.readwrite();
    auto sender1 = rwm.readwrite();
    auto sender2 = rwm.readwrite();

    throwing_on_second_move::reset();

    // Release sender0.
    try
    {
        (void) tt::sync_wait(std::move(sender0) | ex::then([](auto) {}));
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "sender0 threw unexpectedly");
    }

    std::atomic<int> error_count{0};

    // sender1
    try
    {
        tt::sync_wait(std::move(sender1) | ex::then([](auto) {
            HPX_TEST_MSG(false, "sender1: set_value must not be called");
        }));
    }
    catch (std::bad_optional_access const&)
    {
        HPX_TEST_MSG(false,
            "sender1 caught std::bad_optional_access; expected "
            "intentional move exception");
    }
    catch (std::runtime_error const& e)
    {
        HPX_TEST_EQ(std::string(e.what()),
            std::string("throwing_on_second_move: intentional move exception"));
        ++error_count;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "sender1 caught unexpected exception");
    }

    // sender2 must not hang even after sender1 was drained.
    try
    {
        tt::sync_wait(std::move(sender2) | ex::then([](auto) {
            HPX_TEST_MSG(false, "sender2: set_value must not be called");
        }));
    }
    catch (std::bad_optional_access const&)
    {
        HPX_TEST_MSG(false,
            "sender2 caught std::bad_optional_access; expected "
            "intentional move exception");
    }
    catch (std::runtime_error const& e)
    {
        HPX_TEST_EQ(std::string(e.what()),
            std::string("throwing_on_second_move: intentional move exception"));
        ++error_count;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "sender2 caught unexpected exception");
    }

    // Exactly both downstream receivers must have observed the intentional error.
    HPX_TEST_EQ(error_count.load(), 2);
}

///////////////////////////////////////////////////////////////////////////////
struct error_checking_receiver
{
    std::atomic<int>& error_count;

    void set_value(auto&&...) noexcept
    {
        HPX_TEST_MSG(false, "set_value must not be called");
    }

    void set_error(std::exception_ptr ep) noexcept
    {
        try
        {
            if (ep)
            {
                std::rethrow_exception(ep);
            }
            else
            {
                HPX_TEST_MSG(false, "Receiver received null exception_ptr");
            }
        }
        catch (std::bad_optional_access const&)
        {
            HPX_TEST_MSG(false,
                "Receiver caught std::bad_optional_access; expected "
                "intentional move exception");
        }
        catch (std::runtime_error const& e)
        {
            HPX_TEST_EQ(std::string(e.what()),
                std::string(
                    "throwing_on_second_move: intentional move exception"));
            ++error_count;
        }
        catch (...)
        {
            HPX_TEST_MSG(false, "Receiver caught unexpected exception");
        }
    }

    void set_stopped() noexcept
    {
        HPX_TEST_MSG(false, "set_stopped must not be called");
    }
};

///////////////////////////////////////////////////////////////////////////////
// Test 3: multiple concurrent downstream readers, all get set_error.
//
// Two concurrent read senders follow the first (throwing) access.
// Both must receive set_error via copy; neither must hang, terminate, or see
// std::bad_optional_access.
void test_multiple_readers_all_get_set_error()
{
    throwing_on_second_move::move_count.store(-1);

    using mutex_type =
        hpx::experimental::async_rw_mutex<throwing_on_second_move>;

    std::optional<mutex_type> rwm;
    rwm.emplace(throwing_on_second_move{99});

    auto sender0 = rwm->readwrite();
    auto sender1 = rwm->read();
    auto sender2 = rwm->read();

    // Resetting rwm drops its internal references to the shared states
    // so that sender0 is the sole owner of the initial state.
    rwm.reset();

    throwing_on_second_move::reset();

    std::atomic<int> error_count{0};

    auto op1 =
        ex::connect(std::move(sender1), error_checking_receiver{error_count});
    auto op2 =
        ex::connect(std::move(sender2), error_checking_receiver{error_count});

    ex::start(op1);
    ex::start(op2);

    try
    {
        (void) tt::sync_wait(std::move(sender0) | ex::then([](auto) {}));
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "sender0 threw unexpectedly");
    }

    HPX_TEST_EQ(error_count.load(), 2);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_set_error_called_on_move_throw();
    test_multiple_receivers_all_get_set_error();
    test_multiple_readers_all_get_set_error();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
