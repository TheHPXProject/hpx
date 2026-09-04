//  Copyright (c) 2026 Apoorv Shah
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/agas_base/agas_fwd.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>

#include <chrono>
#include <cstdint>

int hpx_main()
{
    {
        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(5000));
    }

    {
        hpx::chrono::steady_duration const custom_timeout(
            std::chrono::milliseconds(12345));

        hpx::agas::set_rpc_timeout(custom_timeout);

        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(12345));
    }

    {
        hpx::chrono::steady_duration const negative_timeout(
            std::chrono::milliseconds(-1));

        hpx::agas::set_rpc_timeout(negative_timeout);

        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(12345));
    }

    {
        hpx::chrono::steady_duration const sub_ms_negative_timeout(
            hpx::chrono::steady_clock::duration(-1));

        hpx::agas::set_rpc_timeout(sub_ms_negative_timeout);

        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(12345));
    }

    {
        hpx::chrono::steady_duration const zero_timeout(
            std::chrono::milliseconds(0));

        hpx::agas::set_rpc_timeout(zero_timeout);

        auto const current_timeout = hpx::agas::get_rpc_timeout();
        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            current_timeout.value())
                            .count();

        HPX_TEST_EQ(ms, std::int64_t(0));
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
