//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/experimental/sandbox.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/topology.hpp>

#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace hpx::experimental::sandbox {

    void environment_info::print(std::ostream& os) const
    {
        hpx::util::format_to(os, "\n");
        hpx::util::format_to(
            os, "=== HPX Sandbox --- Environment {}\n", std::string(25, '='));
        hpx::util::format_to(os, "  Cores:           {}\n", cores);
        hpx::util::format_to(os, "  PUs:             {}\n", pus);
        hpx::util::format_to(os, "  NUMA domains:    {}\n", numa_nodes);
        hpx::util::format_to(os, "  HPX workers:     {}\n", hpx_workers);
        hpx::util::format_to(os, "  Sandbox:         {}\n",
            is_sandbox ? "yes (constrained environment detected)" :
                         "no (full hardware access)");
        hpx::util::format_to(os, "{}\n", std::string(56, '='));
    }

    void benchmark_report::print(std::ostream& os) const
    {
        hpx::util::format_to(os, "\n");
        hpx::util::format_to(
            os, "=== HPX Sandbox --- Benchmark {}\n", std::string(26, '='));
        hpx::util::format_to(os, "  Label:           \"{}\"\n", label);
        hpx::util::format_to(os, "  Iterations:      {}\n", iterations);
        hpx::util::format_to(os, "  Workers:         {}\n", num_workers);
        hpx::util::format_to(os, "{}\n", std::string(56, '-'));
        hpx::util::format_to(os, "  Sequential:      {:.3f} ms\n", seq_mean_ms);
        hpx::util::format_to(os, "  Parallel:        {:.3f} ms\n", par_mean_ms);
        hpx::util::format_to(os, "{}\n", std::string(56, '-'));
        hpx::util::format_to(os, "  Speedup:         {:.2f}x\n", speedup);
        hpx::util::format_to(
            os, "  Efficiency:      {:.1f}%\n", efficiency_pct);
        hpx::util::format_to(os, "{}\n", std::string(56, '-'));

        // Verdict
        hpx::util::format_to(os, "  Verdict:         ");
        if (efficiency_pct >= 80.0)
            hpx::util::format_to(os, "Excellent scaling\n");
        else if (efficiency_pct >= 60.0)
            hpx::util::format_to(os, "Good scaling\n");
        else if (efficiency_pct >= 40.0)
            hpx::util::format_to(os, "Moderate scaling (check granularity)\n");
        else if (speedup > 1.0)
            hpx::util::format_to(os, "Limited scaling (overhead-dominated)\n");
        else
            hpx::util::format_to(
                os, "No speedup (work too small or contention)\n");
        hpx::util::format_to(os, "{}\n", std::string(56, '='));
    }

    namespace detail {

        inline bool check_sandbox_heuristic()
        {
            if (std::getenv("COMPILER_EXPLORER") != nullptr)
                return true;
            if (std::getenv("HPX_SANDBOX") != nullptr)
                return true;
            return false;
        }
    }    // namespace detail

    environment_info detect_environment()
    {
        environment_info info;
        auto const& topo = hpx::threads::create_topology();
        info.cores = topo.get_number_of_cores();
        info.pus = topo.get_number_of_pus();
        info.numa_nodes = topo.get_number_of_numa_nodes();
        info.hpx_workers = hpx::get_num_worker_threads();
        info.is_sandbox = detail::check_sandbox_heuristic();
        return info;
    }
}    // namespace hpx::experimental::sandbox
