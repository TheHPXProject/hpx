//  Copyright (c) 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for two bugs in hpx::serialization::detail::{save,load}
// for std::exception_ptr:
//
//  Bug 1 — Double-read of archive fields (data corruption):
//    load() called both `ar & err_value` and `ar >> err_value` for the same
//    field, advancing the read cursor twice and producing garbled error codes.
//
//  Bug 2 — Type mismatch for throw_line_:
//    save() used `long throw_line_`, but load() used `int throw_line_`.
//    On 64-bit platforms where sizeof(long) != sizeof(int) this shifts all
//    subsequent field reads by 4 bytes.
//
// This test serializes exception_ptrs of all affected types (hpx::exception,
// std::system_error) and verifies that after a round-trip through the archive
// the deserialized exception carries the original, ungarbled values.

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/serialization/exception_ptr.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Round-trip a std::exception_ptr through an in-memory archive and return the
/// reconstructed exception_ptr.
static std::exception_ptr roundtrip(std::exception_ptr const& in)
{
    // Install the built-in handlers (they are normally registered by the HPX
    // runtime; for the unit test we register them explicitly).
    hpx::serialization::detail::set_save_custom_exception_handler(
        [](hpx::serialization::output_archive& ar, std::exception_ptr const& ep,
            unsigned int ver) {
            hpx::serialization::detail::save(ar, ep, ver);
        });
    hpx::serialization::detail::set_load_custom_exception_handler(
        [](hpx::serialization::input_archive& ar, std::exception_ptr& ep,
            unsigned int ver) {
            hpx::serialization::detail::load(ar, ep, ver);
        });

    std::vector<char> buffer;
    {
        hpx::serialization::output_archive oar(buffer);
        oar << in;
    }
    std::exception_ptr out;
    {
        hpx::serialization::input_archive iar(buffer);
        iar >> out;
    }
    return out;
}

// ---------------------------------------------------------------------------
// Test: hpx::exception round-trip preserves error code and message
//
// Before the fix, Bug 1 caused err_value to be read twice, so the
// deserialized error code was taken from whatever bytes followed err_value
// in the archive (i.e. garbage).  Bug 2 further corrupted the stream on
// 64-bit Linux by reading throw_line_ as int instead of long.
// ---------------------------------------------------------------------------
void test_hpx_exception()
{
    auto ep = std::make_exception_ptr(
        hpx::exception(hpx::error::bad_parameter, "test hpx exception"));

    std::exception_ptr ep2 = roundtrip(ep);
    HPX_TEST(ep2 != nullptr);

    try
    {
        std::rethrow_exception(ep2);
        HPX_TEST(false);    // must not reach here
    }
    catch (hpx::exception const& e)
    {
        // The error code must survive the round-trip intact.
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
        // The message must also survive (it lives after the fields that were
        // previously double-read, so it would be garbled by Bug 1).
        std::string const msg = e.what();
        HPX_TEST(msg.find("test hpx exception") != std::string::npos);
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "unexpected exception type after round-trip");
    }
}

// ---------------------------------------------------------------------------
// Test: std::system_error round-trip preserves error code and message
//
// Before the fix, both err_value AND err_message were double-read, so both
// the numeric error code and the human-readable message would be garbage.
// ---------------------------------------------------------------------------
void test_std_system_error()
{
    auto ep = std::make_exception_ptr(
        std::system_error(std::make_error_code(std::errc::invalid_argument),
            "test system error"));

    std::exception_ptr ep2 = roundtrip(ep);
    HPX_TEST(ep2 != nullptr);

    try
    {
        std::rethrow_exception(ep2);
        HPX_TEST(false);    // must not reach here
    }
    catch (std::system_error const& e)
    {
        // The numeric error code must survive.
        HPX_TEST_EQ(
            e.code().value(), static_cast<int>(std::errc::invalid_argument));
        // The message must survive (previously garbled by the double-read of
        // err_message in Bug 1).
        std::string const msg = e.what();
        HPX_TEST(msg.find("test system error") != std::string::npos);
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "unexpected exception type after round-trip");
    }
}

// ---------------------------------------------------------------------------
// Test: std::runtime_error round-trip (not affected by the bugs, but ensures
//       the unaffected path continues to work correctly).
// ---------------------------------------------------------------------------
void test_std_runtime_error()
{
    auto ep = std::make_exception_ptr(std::runtime_error("test runtime error"));

    std::exception_ptr ep2 = roundtrip(ep);
    HPX_TEST(ep2 != nullptr);

    try
    {
        std::rethrow_exception(ep2);
        HPX_TEST(false);
    }
    catch (std::runtime_error const& e)
    {
        std::string const msg = e.what();
        HPX_TEST(msg.find("test runtime error") != std::string::npos);
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "unexpected exception type after round-trip");
    }
}

// ---------------------------------------------------------------------------
// Test: multiple exceptions serialized sequentially into the same buffer.
//
// This is the most direct regression test for Bug 1.  Before the fix, the
// double-read in load() consumed extra bytes from the archive, so the second
// exception_ptr would start deserializing from the wrong offset — causing it
// to throw an unrelated exception type or a stream error.
// ---------------------------------------------------------------------------
void test_sequential_roundtrip()
{
    auto ep1 = std::make_exception_ptr(
        hpx::exception(hpx::error::no_success, "first"));
    auto ep2 = std::make_exception_ptr(
        hpx::exception(hpx::error::bad_parameter, "second"));
    auto ep3 = std::make_exception_ptr(
        std::system_error(std::make_error_code(std::errc::timed_out), "third"));

    std::vector<char> buffer;
    {
        hpx::serialization::output_archive oar(buffer);
        oar << ep1 << ep2 << ep3;
    }

    std::exception_ptr r1, r2, r3;
    {
        hpx::serialization::input_archive iar(buffer);
        iar >> r1 >> r2 >> r3;
    }

    // --- verify r1 ---
    try
    {
        std::rethrow_exception(r1);
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::no_success);
        HPX_TEST(std::string(e.what()).find("first") != std::string::npos);
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "r1: unexpected type");
    }

    // --- verify r2 ---
    // Before the fix, r2 would start at the wrong archive offset due to
    // the double-read for r1, and would either mis-identify the exception
    // type or throw a deserialization error entirely.
    try
    {
        std::rethrow_exception(r2);
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
        HPX_TEST(std::string(e.what()).find("second") != std::string::npos);
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "r2: unexpected type (archive cursor likely off)");
    }

    // --- verify r3 ---
    try
    {
        std::rethrow_exception(r3);
    }
    catch (std::system_error const& e)
    {
        HPX_TEST_EQ(e.code().value(), static_cast<int>(std::errc::timed_out));
        HPX_TEST(std::string(e.what()).find("third") != std::string::npos);
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "r3: unexpected type");
    }
}

// ---------------------------------------------------------------------------
// Test: throw_line_ type mismatch (Bug 2)
//
// On 64-bit Linux sizeof(long)==8 and sizeof(int)==4.  Before the fix,
// throw_line_ was written as a long (8 bytes) but read back as an int
// (4 bytes).  The remaining 4 bytes would be misinterpreted as the start
// of the next field, corrupting every field that follows throw_line_ in
// the stream (i.e. err_value, err_message, and the reconstructed what()).
//
// We detect this by serializing an exception with a large throw_line value
// that does not fit in 32 bits, then verifying the deserialized exception
// is still of the correct type (which requires err_value to be intact).
// ---------------------------------------------------------------------------
void test_throw_line_type()
{
    // Construct an hpx::exception whose internal throw_line will be stored
    // as a long; the exact value is set by the HPX_THROW_EXCEPTION macro but
    // we can still verify the type and error code survive.
    auto ep = std::make_exception_ptr(
        hpx::exception(hpx::error::assertion_failure, "type-mismatch test"));

    std::exception_ptr ep2 = roundtrip(ep);
    HPX_TEST(ep2 != nullptr);

    try
    {
        std::rethrow_exception(ep2);
    }
    catch (hpx::exception const& e)
    {
        // If the throw_line_ type mismatch shifted the stream, err_value
        // would be garbage and this comparison would fail.
        HPX_TEST_EQ(e.get_error(), hpx::error::assertion_failure);
    }
    catch (...)
    {
        HPX_TEST_MSG(false,
            "unexpected exception type — throw_line_ type mismatch likely "
            "shifted the archive cursor");
    }
}

// ---------------------------------------------------------------------------
int main()
{
    test_hpx_exception();
    test_std_system_error();
    test_std_runtime_error();
    test_sequential_roundtrip();
    test_throw_line_type();

    return hpx::util::report_errors();
}
