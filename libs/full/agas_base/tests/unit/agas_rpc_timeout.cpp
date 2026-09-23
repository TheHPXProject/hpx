//  Copyright (c) 2026 Apoorv Shah
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/agas_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <thread>

int hpx_main()
{
    {
        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(60000));
    }

    {
        hpx::chrono::steady_duration const custom_timeout(
            std::chrono::milliseconds(12345));

        HPX_TEST(hpx::agas::set_rpc_timeout(custom_timeout));

        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(12345));
    }

    {
        hpx::chrono::steady_duration const negative_timeout(
            std::chrono::milliseconds(-1));

        HPX_TEST(!hpx::agas::set_rpc_timeout(negative_timeout));

        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(12345));
    }

    {
        hpx::chrono::steady_duration const sub_ms_negative_timeout(
            hpx::chrono::steady_clock::duration(-1));

        HPX_TEST(!hpx::agas::set_rpc_timeout(sub_ms_negative_timeout));

        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(12345));
    }

    {
        hpx::chrono::steady_duration const zero_timeout(
            std::chrono::milliseconds(0));

        HPX_TEST(!hpx::agas::set_rpc_timeout(zero_timeout));

        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(12345));
    }

    return hpx::finalize();
}

#if defined(HPX_HAVE_NETWORKING)
static bool pre_startup_tested = false;
#endif

int main(int argc, char* argv[])
{
#if defined(HPX_HAVE_NETWORKING)
    // Issue #7480: Test exact bootstrap scenario where
    // threads::get_self_ptr() == nullptr and hpx::is_starting() == true.
    // Pre-startup functions execute during bootstrap.
    hpx::register_pre_startup_function([]() {
        HPX_TEST(hpx::is_starting());

        auto const old_timeout = hpx::agas::get_rpc_timeout();
        hpx::agas::set_rpc_timeout(
            hpx::chrono::steady_duration(std::chrono::milliseconds(100)));

        // Launch an OS thread during pre-startup:
        // On this OS thread, `threads::get_self_ptr()` is `nullptr`
        // and `hpx::is_starting()` is `true`.
        std::thread os_thread([]() {
            HPX_TEST(nullptr == hpx::threads::get_self_ptr());
            HPX_TEST(hpx::is_starting());

            // Deliberately unresolved future representing an in-flight
            // bootstrap RPC that does not complete before the timeout.
            hpx::promise<hpx::parcelset::endpoints_type> p;
            hpx::future<hpx::parcelset::endpoints_type> f = p.get_future();

            // The old code spun in `while (!endpoints_future.is_ready())`.
            // The fixed code calls `wait_or_handle_timeout(...)`.
            // It throws `future_wait_timed_out` when the wait expires.
            bool caught_timeout = false;
            hpx::chrono::high_resolution_timer timer;
            try
            {
                hpx::wait_or_handle_timeout(HPX_MOVE(f),
                    "hosted_locality_namespace::resolve_locality",
                    hpx::agas::get_rpc_timeout());
            }
            catch (hpx::exception const& e)
            {
                auto const elapsed = timer.elapsed();
                if (e.get_error() == hpx::error::future_wait_timed_out)
                {
                    caught_timeout = true;
                }
                std::string const what = e.what();
                HPX_TEST(what.find("future.wait_for timed out") !=
                    std::string::npos);
                HPX_TEST_EQ(hpx::get_error_function_name(e),
                    std::string("hosted_locality_namespace::resolve_locality"));
                // Ensure it actually waited for the configured timeout duration
                HPX_TEST(elapsed >= 0.05);
            }
            HPX_TEST(caught_timeout);
        });
        os_thread.join();
        hpx::agas::set_rpc_timeout(old_timeout);
        pre_startup_tested = true;
    });
#endif

    HPX_TEST_EQ(hpx::init(argc, argv), 0);

#if defined(HPX_HAVE_NETWORKING)
    HPX_TEST(pre_startup_tested);
#endif

    return hpx::util::report_errors();
}
