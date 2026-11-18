//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for issue #7811: Scheduler mode setters apply PU masks to the
// wrong workers, and write past modes_ with several pools

#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

using hpx::threads::policies::scheduler_mode;
constexpr auto flag = scheduler_mode::enable_elasticity;
constexpr auto flag2 = scheduler_mode::assign_work_thread_parent;

int hpx_main()
{
    auto& default_pool = hpx::resource::get_thread_pool("default");
    auto const* default_sched = default_pool.get_scheduler();
    std::size_t const default_threads = default_pool.get_os_thread_count();

    auto& other_pool = hpx::resource::get_thread_pool("other");
    auto const* other_sched = other_pool.get_scheduler();
    std::size_t const other_threads = other_pool.get_os_thread_count();

    HPX_TEST_EQ(default_threads, std::size_t(2));
    HPX_TEST_EQ(other_threads, std::size_t(2));

    // Initially neither pool should have flag set on any worker
    for (std::size_t i = 0; i != default_threads; ++i)
    {
        HPX_TEST(!default_sched->has_scheduler_mode(flag, i));
        HPX_TEST(!default_sched->has_scheduler_mode(flag2, i));
    }
    for (std::size_t i = 0; i != other_threads; ++i)
    {
        HPX_TEST(!other_sched->has_scheduler_mode(flag, i));
        HPX_TEST(!other_sched->has_scheduler_mode(flag2, i));
    }

    // 1. A mask holding only the PU of worker 1 of the default pool
    auto const default_mask =
        hpx::resource::get_partitioner().get_pool_pus_mask("default");
    hpx::threads::mask_type mask_default_w1;
    hpx::threads::resize(
        mask_default_w1, hpx::threads::mask_size(default_mask));
    std::size_t n = 0;
    for (std::size_t i = 0; i != hpx::threads::mask_size(default_mask); ++i)
    {
        if (hpx::threads::test(default_mask, i))
        {
            if (n == 1)
            {
                hpx::threads::set(mask_default_w1, i);
                break;
            }
            ++n;
        }
    }

    hpx::threads::add_scheduler_mode(flag, mask_default_w1);

    // Worker 1 of default pool should have flag set, worker 0 should not
    HPX_TEST(!default_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(default_sched->has_scheduler_mode(flag, 1));

    // Other pool workers should remain unaffected
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 1));

    // 2. Remove flag using mask_default_w1
    hpx::threads::remove_scheduler_mode(flag, mask_default_w1);
    HPX_TEST(!default_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(!default_sched->has_scheduler_mode(flag, 1));
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 1));

    // 3. Overload without mask: adds flag for all used PUs across all pools.
    // Previously this caused an assertion failure
    // (count(pu_mask) <= modes_.size()) or out-of-bounds write past modes_.
    hpx::threads::add_scheduler_mode(flag);

    HPX_TEST(default_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(default_sched->has_scheduler_mode(flag, 1));
    HPX_TEST(other_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(other_sched->has_scheduler_mode(flag, 1));

    // 4. Overload without mask: remove flag for all used PUs across all pools
    hpx::threads::remove_scheduler_mode(flag);

    HPX_TEST(!default_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(!default_sched->has_scheduler_mode(flag, 1));
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 1));

    // 5. Target worker 0 of other pool
    auto const other_mask =
        hpx::resource::get_partitioner().get_pool_pus_mask("other");
    hpx::threads::mask_type mask_other_w0;
    hpx::threads::resize(mask_other_w0, hpx::threads::mask_size(other_mask));
    for (std::size_t i = 0; i != hpx::threads::mask_size(other_mask); ++i)
    {
        if (hpx::threads::test(other_mask, i))
        {
            hpx::threads::set(mask_other_w0, i);
            break;
        }
    }

    hpx::threads::add_scheduler_mode(flag, mask_other_w0);
    HPX_TEST(!default_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(!default_sched->has_scheduler_mode(flag, 1));
    HPX_TEST(other_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 1));

    hpx::threads::remove_scheduler_mode(flag, mask_other_w0);
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 0));

    // 6. Test set_scheduler_mode with a partial mask
    hpx::threads::set_scheduler_mode(flag, mask_default_w1);
    HPX_TEST(!default_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(default_sched->has_scheduler_mode(flag, 1));
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 0));
    HPX_TEST(!other_sched->has_scheduler_mode(flag, 1));

    // 7. Test add_remove_scheduler_mode with a partial mask:
    // Remove flag and add flag2 on worker 1 of default pool
    hpx::threads::add_remove_scheduler_mode(flag2, flag, mask_default_w1);
    HPX_TEST(!default_sched->has_scheduler_mode(flag, 1));
    HPX_TEST(!default_sched->has_scheduler_mode(flag2, 0));
    HPX_TEST(default_sched->has_scheduler_mode(flag2, 1));
    HPX_TEST(!other_sched->has_scheduler_mode(flag2, 0));
    HPX_TEST(!other_sched->has_scheduler_mode(flag2, 1));

    return hpx::local::finalize();
}

namespace {

    std::vector<char*> remove_threads_option(int argc, char* argv[])
    {
        std::vector<char*> result;
        result.reserve(static_cast<std::size_t>(argc) + 1);
        result.push_back(argv[0]);

        for (int i = 1; i != argc; ++i)
        {
            std::string_view const arg(argv[i]);
            if (arg == "--hpx:threads" || arg == "-t")
            {
                if (i + 1 != argc)
                {
                    ++i;
                }
                continue;
            }

            if (arg.starts_with("--hpx:threads="))
            {
                continue;
            }

            result.push_back(argv[i]);
        }

        result.push_back(nullptr);
        return result;
    }
}    // namespace

int main(int argc, char* argv[])
{
    if (hpx::threads::hardware_concurrency() < 4)
    {
        return hpx::util::report_errors();
    }

    auto sanitized_argv = remove_threads_option(argc, argv);
    int sanitized_argc = static_cast<int>(sanitized_argv.size()) - 1;

    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=4"};
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("other");
        std::size_t threads_added = 0;
        for (auto const& numa : rp.numa_domains())
        {
            for (auto const& core : numa.cores())
            {
                for (auto const& pu : core.pus())
                {
                    if (threads_added < 2)
                    {
                        rp.add_resource(pu, "other");
                        ++threads_added;
                    }
                }
            }
        }
    };

    HPX_TEST_EQ(hpx::local::init(
                    hpx_main, sanitized_argc, sanitized_argv.data(), init_args),
        0);
    return hpx::util::report_errors();
}
