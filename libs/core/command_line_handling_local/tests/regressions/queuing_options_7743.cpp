//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace {

    int hpx_main()
    {
        return hpx::local::finalize();
    }

    std::vector<char*> remove_queuing_option(int argc, char* argv[])
    {
        std::vector<char*> result;
        result.reserve(static_cast<std::size_t>(argc) + 1);
        result.push_back(argv[0]);

        for (int i = 1; i != argc; ++i)
        {
            std::string_view const arg(argv[i]);
            if (arg == "--hpx:queuing")
            {
                if (i + 1 != argc)
                {
                    ++i;
                }
                continue;
            }

            if (arg.starts_with("--hpx:queuing="))
            {
                continue;
            }

            result.push_back(argv[i]);
        }

        result.push_back(nullptr);
        return result;
    }

    void test_queuing(int argc, char* argv[], std::string const& scheduler,
        bool const high_priority_threads, int const expected_result)
    {
        hpx::local::init_params init_args;
        init_args.cfg = {"--hpx:queuing=" + scheduler};
        if (high_priority_threads)
        {
            init_args.cfg.emplace_back("--hpx:high-priority-threads=1");
        }

        HPX_TEST_EQ(
            hpx::local::init(hpx_main, argc, argv, init_args), expected_result);
    }
}    // namespace

int main(int argc, char* argv[])
{
    // Test configurations may inject a scheduler through
    // HPX_WITH_TESTS_COMMAND_LINE. Each case below supplies its own scheduler,
    // so retain all external arguments except that scheduler option.
    std::vector<char*> test_argv = remove_queuing_option(argc, argv);
    argc = static_cast<int>(test_argv.size() - 1);
    argv = test_argv.data();

    // A leading exclamation mark forces the command-line value to override
    // configuration without becoming part of the scheduler name.
    test_queuing(argc, argv, "!local", false, 0);
    test_queuing(argc, argv, "!local-priority-fifo", true, 0);

    test_queuing(argc, argv, "local-priority-fifo", true, 0);
    test_queuing(argc, argv, "local-priority-fifo-double", true, 0);
    test_queuing(argc, argv, "static-priority", true, 0);

#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    test_queuing(argc, argv, "local-priority-lifo", true, 0);
    test_queuing(argc, argv, "abp-priority-fifo", true, 0);
    test_queuing(argc, argv, "abp-priority-lifo", true, 0);
    test_queuing(argc, argv, "a", true, 0);
#endif

    // Scheduler names may be abbreviated. The abbreviation must be resolved
    // in the same order as the full scheduler names.
    test_queuing(argc, argv, "local-p", true, 0);
    test_queuing(argc, argv, "static-p", true, 0);

#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
    test_queuing(argc, argv, "local-workrequesting-fifo", true, 0);
    test_queuing(argc, argv, "local-workrequesting-mc", true, 0);
    test_queuing(argc, argv, "local-w", true, 0);

#if defined(HPX_HAVE_CXX11_STD_ATOMIC_128BIT)
    test_queuing(argc, argv, "local-workrequesting-lifo", true, 0);
#endif
#endif

    test_queuing(argc, argv, "local", true, -1);
    test_queuing(argc, argv, "l", true, -1);
    test_queuing(argc, argv, "static", true, -1);
    test_queuing(argc, argv, "s", true, -1);
    test_queuing(argc, argv, "shared-priority", true, -1);

    return hpx::util::report_errors();
}
