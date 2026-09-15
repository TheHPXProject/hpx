//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test exercises the packaged_action posting failure path used by
// hpx::async_cb (see hpx::detail::sync_local_invoke_cb::call and the
// hpx::detail::async_cb_impl overloads in
// async_distributed/detail/async_implementations.hpp). Posting a component
// action to a plain locality id (rather than to an id referencing an actual
// instance of that component) makes hpx::traits::action_is_target_valid fail,
// which causes p.post_cb()/ p.post_p_cb() to throw synchronously. That
// exception is caught by hpx::detail::try_catch_exception_ptr and stored on the
// packaged_action via set_exception(), so it must be reported by future::get()
// -- and, for the synchronous launch policy, the failure must be visible
// without ever waiting on the (already broken) posting operation.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <system_error>

///////////////////////////////////////////////////////////////////////////////
struct decrement_server
  : hpx::components::managed_component_base<decrement_server>
{
    std::int32_t call(std::int32_t i) const
    {
        return i - 1;
    }

    HPX_DEFINE_COMPONENT_ACTION(decrement_server, call)
};

using server_type = hpx::components::managed_component<decrement_server>;
HPX_REGISTER_COMPONENT(server_type, decrement_server)

using call_action = decrement_server::call_action;
HPX_REGISTER_ACTION_DECLARATION(call_action)
HPX_REGISTER_ACTION(call_action)

///////////////////////////////////////////////////////////////////////////////
std::atomic<int> callback_called(0);

#if defined(HPX_HAVE_NETWORKING)
void cb(std::error_code const&, hpx::parcelset::parcel const&)
{
    ++callback_called;
}
#else
void cb()
{
    ++callback_called;
}
#endif

///////////////////////////////////////////////////////////////////////////////
// A plain locality id is not a valid target for a component action (it does
// not reference an instance of decrement_server), so posting the action to
// it must fail with hpx::error::bad_parameter, synchronously, before any
// actual posting/scheduling takes place.
template <typename Future>
void check_reports_posting_exception(Future& f)
{
    bool caught_exception = false;
    hpx::error err = hpx::error::success;

    try
    {
        f.get();
    }
    catch (hpx::exception const& e)
    {
        caught_exception = true;
        err = e.get_error();
    }

    HPX_TEST(caught_exception);
    HPX_TEST(err == hpx::error::bad_parameter);
}

void test_sync_post_failure(hpx::id_type const& target)
{
    callback_called.store(0);

    hpx::future<std::int32_t> f =
        hpx::async_cb<call_action>(hpx::launch::sync, target, &cb, 42);

    // The posting failure is detected while constructing the packaged_action
    // (synchronously), so the future must already carry the exception -- no
    // blocking wait on an outstanding posting operation is needed or
    // performed.
    HPX_TEST(f.is_ready());
    HPX_TEST(f.has_exception());

    check_reports_posting_exception(f);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    HPX_TEST_EQ(callback_called.load(), 0);
}

void test_async_post_failure(hpx::id_type const& target)
{
    callback_called.store(0);

    hpx::future<std::int32_t> f =
        hpx::async_cb<call_action>(hpx::launch::async, target, &cb, 42);

    check_reports_posting_exception(f);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    HPX_TEST_EQ(callback_called.load(), 0);
}

void test_deferred_post_failure(hpx::id_type const& target)
{
    callback_called.store(0);

    hpx::future<std::int32_t> f =
        hpx::async_cb<call_action>(hpx::launch::deferred, target, &cb, 42);

    check_reports_posting_exception(f);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    HPX_TEST_EQ(callback_called.load(), 0);
}

int hpx_main()
{
    // hpx::find_here() refers to this locality itself, not to any
    // decrement_server instance, so it is an invalid target for call_action.
    hpx::id_type const invalid_target = hpx::find_here();

    test_sync_post_failure(invalid_target);
    test_async_post_failure(invalid_target);
    test_deferred_post_failure(invalid_target);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
