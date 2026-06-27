//  Copyright (c) 2019-2020 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/resiliency.hpp>
#include <hpx/modules/resiliency_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <random>
#include <vector>

std::random_device rd;
std::mt19937 mt(rd());
std::uniform_real_distribution<double> dist(1.0, 10.0);

int universal_ans()
{
    if (dist(mt) > 5)
        return 42;
    return 84;
}

HPX_PLAIN_ACTION(universal_ans, universal_action)

bool validate(int ans)
{
    return ans == 42;
}

int vote(std::vector<int>&& results)
{
    return results.at(0);
}

int hpx_main()
{
    std::vector<hpx::id_type> locals = hpx::find_all_localities();

    // Allow a task to replicate on the same locality if there is only 1 locality
    if (locals.size() == 1)
    {
        locals.insert(locals.end(), 9, hpx::find_here());
    }

    {
        universal_action action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate(locals, action);

        try
        {
            f.get();
        }
        catch (hpx::resiliency::experimental::abort_replicate_exception const&)
        {
            HPX_TEST(true);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
    }

    {
        universal_action action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_validate(
                locals, &validate, action);

        try
        {
            f.get();
        }
        catch (hpx::resiliency::experimental::abort_replicate_exception const&)
        {
            HPX_TEST(true);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
    }

    {
        universal_action action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_vote(
                locals, &vote, action);

        try
        {
            f.get();
        }
        catch (hpx::resiliency::experimental::abort_replicate_exception const&)
        {
            HPX_TEST(true);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
    }

    {
        universal_action action;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replicate_vote_validate(
                locals, &vote, &validate, action);

        try
        {
            f.get();
        }
        catch (hpx::resiliency::experimental::abort_replicate_exception const&)
        {
            HPX_TEST(true);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST(hpx::init(argc, argv) == 0);
    return hpx::util::report_errors();
}

#endif
