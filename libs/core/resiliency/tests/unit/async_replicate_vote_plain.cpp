//  Copyright (c) 2019 National Technology & Engineering Solutions of Sandia,
//                     LLC (NTESS).
//  Copyright (c) 2019 Nikunj Gupta
//  Copyright (c) 2018-2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resiliency.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <vector>

std::atomic<std::size_t> invocation_count{0};

int vote(std::vector<int> vect)
{
    return vect.at(0);
}

int universal_ans()
{
    auto const invocation =
        invocation_count.fetch_add(1, std::memory_order_relaxed);
    return invocation % 2 == 0 ? 42 : 84;
}

bool validate(int ans)
{
    return ans == 42;
}

int hpx_main()
{
    {
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_vote(
                10, &vote, &universal_ans);

        auto result = f.get();
        HPX_TEST(result == 42 || result == 84);
    }

    {
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_vote_validate(
                10, &vote, &validate, &universal_ans);

        auto result = f.get();
        HPX_TEST(result == 42 || result == 84);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST(hpx::local::init(hpx_main, argc, argv) == 0);
    return hpx::util::report_errors();
}
