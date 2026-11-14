//  Copyright (c) 2026 STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Demonstrating #8070: Runtime hangs, busy, when a startup function throws on
// one of two localities

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>

constexpr char const* error_message = "simulated startup failure on locality 1";
static std::uint32_t recorded_locality_id = static_cast<std::uint32_t>(-1);

void fail_on_locality_1()
{
    recorded_locality_id = hpx::get_locality_id();
    if (recorded_locality_id == 1)
    {
        throw std::runtime_error(error_message);
    }
}

int hpx_main()
{
    // hpx_main must never be invoked if startup failed on any locality
    HPX_TEST(false);
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::register_startup_function(&fail_on_locality_1);

    bool caught_remote_exception = false;
    bool caught_local_exception = false;
    try
    {
        hpx::init(argc, argv);
    }
    catch (hpx::exception const& e)
    {
        // Locality 0 must receive the remote failure notification
        if (e.get_error() == hpx::error::invalid_status)
        {
            caught_remote_exception = true;
        }
    }
    catch (std::exception const& e)
    {
        // Locality 1 must catch its own rethrown runtime_error
        if (std::string(e.what()) == error_message)
        {
            caught_local_exception = true;
        }
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    // Verify per-locality exception invariant
    if (recorded_locality_id == 0)
    {
        HPX_TEST(caught_remote_exception);
        HPX_TEST(!caught_local_exception);
    }
    else if (recorded_locality_id == 1)
    {
        HPX_TEST(caught_local_exception);
        HPX_TEST(!caught_remote_exception);
    }
    else
    {
        HPX_TEST(false);
    }

    return hpx::util::report_errors();
}
#endif
