//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// measure_transfers between two localities: every size measured in both
// directions, every round's data arrived right.

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/parcel_tuning.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <vector>

int hpx_main()
{
    std::vector<hpx::id_type> const remotes = hpx::find_remote_localities();
    HPX_TEST(!remotes.empty());
    if (!remotes.empty())
    {
        hpx::tuning::transfer_options options;
        options.sizes = {1024, 65536, std::size_t(1) << 20};
        options.rounds = 7;
        options.warmup = 1;
        options.min_rounds = 3;

        std::vector<hpx::tuning::transfer_point> const points =
            hpx::tuning::measure_transfers(remotes.front(), options);
        HPX_TEST_EQ(points.size(), options.sizes.size());
        for (std::size_t i = 0; i != points.size(); ++i)
        {
            hpx::tuning::transfer_point const& p = points[i];
            HPX_TEST_EQ(p.size, options.sizes[i]);
            HPX_TEST_EQ(p.errors, std::size_t(0));
            HPX_TEST(p.get_us.count >= options.min_rounds);
            HPX_TEST(p.set_us.count >= options.min_rounds);
            HPX_TEST(p.get_us.median > 0.0);
            HPX_TEST(p.set_us.median > 0.0);
            HPX_TEST(p.conduit_chunks_per_round >= 0.0);
        }
    }
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
#else
int main()
{
    return 0;
}
#endif
