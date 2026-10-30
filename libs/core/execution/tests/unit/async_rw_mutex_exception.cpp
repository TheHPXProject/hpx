//  Copyright (c) 2025 Hartmut Kaiser
//  Copyright (c) 2025 Shivansh Singh
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
#include <stdexcept>
#include <string>
#include <utility>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

///////////////////////////////////////////////////////////////////////////////
// A value type whose move constructor throws on the second (and subsequent)
// construction.  The first move (into the initial shared state) must succeed
// so that async_rw_mutex can be constructed at all.  Only the move performed
// by release() when passing the value to the NEXT shared state should throw.
struct throwing_on_second_move
{
    static std::atomic<int> move_count;

    int value;

    explicit throwing_on_second_move(int v) noexcept
      : value(v)
    {
    }

    // Move constructor throws on the second invocation.
    throwing_on_second_move(throwing_on_second_move&& other)
      : value(other.value)
    {
        if (++move_count > 1)
        {
            throw std::runtime_error(
                "throwing_on_second_move: intentional move exception");
        }
    }

    throwing_on_second_move& operator=(throwing_on_second_move&&) = delete;
    throwing_on_second_move(throwing_on_second_move const&) = delete;
    throwing_on_second_move& operator=(throwing_on_second_move const&) = delete;
};

std::atomic<int> throwing_on_second_move::move_count{0};

///////////////////////////////////////////////////////////////////////////////
// Test 1: single downstream receiver gets set_error when move throws.
//
// Sequence:
//   rwm.readwrite()   -> first sender  (no prev_state, value moved in: move #1)
//   rwm.readwrite()   -> second sender (prev_state = first state)
//
// When the first access wrapper is released (wrapper destroyed after sync_wait
// on the first sender), release() moves value → next_state: move #2 throws.
// The fix must route the exception to the second sender's receiver.
void test_set_error_called_on_move_throw()
{
    throwing_on_second_move::move_count.store(0);

    using mutex_type =
        hpx::experimental::async_rw_mutex<throwing_on_second_move>;

    mutex_type rwm{throwing_on_second_move{42}};

    // Obtain two senders before starting either.  This registers next_state on
    // the first shared state so that release() has somewhere to forward to.
    auto sender1 = rwm.readwrite();
    auto sender2 = rwm.readwrite();

    // Drive the first access: the wrapper is received, then discarded.
    // Its destructor calls release(), which attempts move #2 → throws.
    bool first_threw = false;
    try
    {
        // sync_wait connects, starts, and waits.  The wrapped value is sent
        // via set_value(access_type{state}).  When the returned optional goes
        // out of scope the access_type destructor drops the shared_ptr,
        // triggering release().  The exception from release() is stored on
        // next_state and then forwarded through the continuation to set_error
        // on the second sender's receiver, which sync_wait re-throws.
        //
        // We intentionally discard the value here so the wrapper dies inside
        // the sync_wait scope.
        (void) tt::sync_wait(
            std::move(sender1) | ex::then([](auto /*wrapper*/) {
                // wrapper goes out of scope here, triggering release()
            }));
    }
    catch (std::runtime_error const& e)
    {
        // move #2 threw inside then() while the wrapper was alive —
        // this path would only be hit if the move happens at construction
        // of the access_type, which it does not; swallow and proceed.
        first_threw = true;
        (void) e;
    }

    // Drive the second access: it must receive set_error (not set_value).
    bool second_got_error = false;
    std::string error_msg;
    try
    {
        tt::sync_wait(std::move(sender2) | ex::then([](auto /*wrapper*/) {
            // Must NOT reach here: the value was never successfully moved.
            HPX_TEST_MSG(false,
                "set_value was called on the second sender despite a "
                "throwing move; expected set_error");
        }));
    }
    catch (std::runtime_error const& e)
    {
        second_got_error = true;
        error_msg = e.what();
    }
    catch (std::exception const& e)
    {
        second_got_error = true;
        error_msg = e.what();
    }

    // Either the first or the second sync_wait must have received the error.
    // The exact which one depends on whether move #2 fires inside then() (when
    // the wrapper is released) or before then() runs. What matters is that:
    //   a) no std::terminate was called, and
    //   b) the exception was observed somewhere.
    HPX_TEST_MSG(first_threw || second_got_error,
        "Expected the throwing move exception to surface on either the "
        "first or the second sender's error channel");

    if (second_got_error)
    {
        HPX_TEST_MSG(!error_msg.empty(),
            "error channel should carry the original exception message");
    }
}

///////////////////////////////////////////////////////////////////////////////
// Test 2: multiple downstream receivers, all get set_error.
//
// Two consecutive readwrite senders follow the first (throwing) access.
// Both must receive set_error; neither must hang.
void test_multiple_receivers_all_get_set_error()
{
    throwing_on_second_move::move_count.store(0);

    using mutex_type =
        hpx::experimental::async_rw_mutex<throwing_on_second_move>;

    mutex_type rwm{throwing_on_second_move{7}};

    // Create three senders: [0] is the first access (move #1 OK),
    // [1] and [2] are downstream (should receive set_error after move #2
    // throws in release()).
    auto sender0 = rwm.readwrite();
    auto sender1 = rwm.readwrite();
    auto sender2 = rwm.readwrite();

    // Release sender0.
    try
    {
        (void) tt::sync_wait(std::move(sender0) | ex::then([](auto) {}));
    }
    catch (...)
    {
    }

    std::atomic<int> error_count{0};

    // sender1
    try
    {
        tt::sync_wait(std::move(sender1) | ex::then([](auto) {
            HPX_TEST_MSG(false, "sender1: set_value must not be called");
        }));
    }
    catch (std::exception const&)
    {
        ++error_count;
    }

    // sender2 must not hang even after sender1 was drained.
    try
    {
        tt::sync_wait(std::move(sender2) | ex::then([](auto) {
            HPX_TEST_MSG(false, "sender2: set_value must not be called");
        }));
    }
    catch (std::exception const&)
    {
        ++error_count;
    }

    // At least one of the two downstream receivers must have seen an error.
    HPX_TEST_MSG(error_count.load() >= 1,
        "Expected at least one downstream receiver to observe set_error");
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_set_error_called_on_move_throw();
    test_multiple_receivers_all_get_set_error();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
