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
#include <string>
#include <vector>

int hpx_main()
{
    auto const current_timeout = hpx::agas::get_rpc_timeout();
    auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_timeout.value())
                        .count();

    HPX_TEST_EQ(ms, std::int64_t(8888));

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.agas.rpc_timeout = 8888"};

    hpx::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
