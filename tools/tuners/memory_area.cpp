//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/init_runtime_local.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"
#include "child.hpp"
#include "memory_area.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace po = hpx::program_options;
namespace tuning = hpx::tuning;

namespace hpx_tune {

    namespace {

        // a value with about three significant digits
        std::string number(double const value)
        {
            double const magnitude = value < 0 ? -value : value;
            int const decimals = magnitude >= 100.0 ? 0 :
                magnitude >= 10.0                   ? 1 :
                magnitude >= 1.0                    ? 2 :
                                                      3;
            std::ostringstream os;
            os << std::fixed << std::setprecision(decimals) << value;
            return os.str();
        }

        // the levels task_size_chunk_size has entries for
        constexpr int max_level = 3;

        std::string level_name(int const level)
        {
            return level == 0 ? "memory" : "l" + std::to_string(level);
        }

        std::string rate_key(int const level)
        {
            return "hpx.task_size_chunk_size.copy_bytes_per_ns_" +
                level_name(level);
        }

        // What is timed for each level: a copy that reads and writes the
        // footprint (half of it copied to the other half). A core's share of
        // a level is the size of one cache divided by the cores that share
        // it, which is what task_size_chunk_size compares a loop's footprint
        // per core with; the child runs on one core of an otherwise idle
        // machine, so it times a footprint that is inside the level but
        // outside the level below: four times the level below, at most half
        // of the level. Memory: four times the largest cache.
        struct footprint
        {
            int level;            // 0 for memory
            std::size_t share;    // a core's share of the level
            std::size_t bytes;    // the bytes the copy reads and writes
        };

        std::vector<footprint> footprints(
            tuning::machine_description const& machine)
        {
            std::vector<footprint> result;
            std::size_t below = 0;
            std::size_t largest = 0;
            for (tuning::cache_level const& c : machine.caches)
            {
                largest = (std::max) (largest, c.size);
                if (c.level > max_level || c.cores == 0)
                    continue;
                std::size_t bytes = c.size / 2;
                if (below != 0)
                    bytes = (std::min) (bytes, 4 * below);
                result.push_back({c.level, c.size / c.cores, bytes});
                below = c.size;
            }
            std::size_t const memory =
                largest == 0 ? std::size_t(512) << 20 : 4 * largest;
            result.push_back(
                {0, (std::numeric_limits<std::size_t>::max)(), memory});
            return result;
        }

        // the options a child of the memory area understands besides HPX's
        po::options_description child_options()
        {
            po::options_description desc(
                "Options of a child of the memory area");
            // clang-format off
            desc.add_options()
                ("child", po::value<std::string>(),
                    "run as a child of the given area")
                ("memory-footprint", po::value<std::vector<std::string>>(),
                    "<level>:<bytes> to time (repeatable)")
                ("repeats", po::value<std::size_t>()->default_value(21),
                    "the samples of each footprint")
                ("memory-verify",
                    "read back the rates task_size_chunk_size uses instead "
                    "of timing copies (hpx_tune --check)")
                ("memory-share", po::value<std::vector<std::string>>(),
                    "<level>:<bytes per core> to read back the rate of");
            // clang-format on
            return desc;
        }

        // The rate of a copy of half of the footprint to the other half on
        // this core, in bytes copied per nanosecond: the median of repeats
        // samples, each copying at least 64 MB in total.
        tuning::summary time_copy(
            std::size_t const footprint_bytes, std::size_t const repeats)
        {
            std::size_t const half =
                (std::max) (footprint_bytes / 2, std::size_t(64));
            std::vector<char> src(half, 1), dst(half, 0);
            std::size_t const copies =
                (std::max) (std::size_t(1), (std::size_t(64) << 20) / half);

            // warm up: touch both buffers
            for (std::size_t i = 0; i != 2; ++i)
                std::memcpy(dst.data(), src.data(), half);

            std::vector<double> rates;
            rates.reserve(repeats);
            for (std::size_t r = 0; r != repeats; ++r)
            {
                auto const start = std::chrono::steady_clock::now();
                for (std::size_t i = 0; i != copies; ++i)
                {
                    std::memcpy(dst.data(), src.data(), half);
                    // keep the copies from being merged
                    src[i % half] = static_cast<char>(dst[(i + 1) % half] + 1);
                }
                std::chrono::duration<double, std::nano> const elapsed =
                    std::chrono::steady_clock::now() - start;
                rates.push_back(static_cast<double>(half) *
                    static_cast<double>(copies) /
                    (std::max) (elapsed.count(), 1.0));
            }
            return tuning::summarize(HPX_MOVE(rates));
        }

        // "<level>:<bytes>"
        std::pair<int, std::size_t> parse_pair(std::string const& text)
        {
            auto const colon = text.find(':');
            if (colon == std::string::npos)
                throw std::invalid_argument(
                    "expected <level>:<bytes>: " + text);
            return {std::stoi(text.substr(0, colon)),
                static_cast<std::size_t>(std::stoull(text.substr(colon + 1)))};
        }

        // a child's line: "rate <level> <bytes> <min> <median> <max>" or
        // "verify <level> <bytes per ns>"
        struct child_rate
        {
            int level = 0;
            std::size_t bytes = 0;
            tuning::summary rate;
        };

        std::string report(std::vector<footprint> const& fps,
            std::size_t const runs,
            std::vector<std::vector<child_rate>> const& all,
            std::map<int, double> const& chosen,
            std::vector<tuning::ini_section> const& sections)
        {
            std::vector<std::string> levels;
            for (footprint const& f : fps)
            {
                std::vector<std::string> per_run;
                std::vector<double> medians;
                for (std::vector<child_rate> const& run : all)
                {
                    for (child_rate const& c : run)
                    {
                        if (c.level == f.level)
                        {
                            per_run.push_back(tuning::to_json(c.rate));
                            medians.push_back(c.rate.median);
                        }
                    }
                }
                auto const it = chosen.find(f.level);
                levels.push_back(tuning::json_object(
                    {{"level", tuning::json_string(level_name(f.level))},
                        {"share_bytes",
                            f.level == 0 ? "null" : std::to_string(f.share)},
                        {"footprint_bytes", std::to_string(f.bytes)},
                        {"runs", tuning::json_array(per_run)},
                        {"medians_between_runs",
                            tuning::to_json(tuning::summarize(medians))},
                        {"bytes_per_ns",
                            it == chosen.end() ?
                                "null" :
                                tuning::json_number(it->second)}}));
            }
            std::vector<std::string> settings;
            for (tuning::ini_section const& s : sections)
            {
                for (tuning::tuned_setting const& setting : s.settings)
                {
                    settings.push_back(tuning::json_object(
                        {{"key", tuning::json_string(setting.key)},
                            {"value", tuning::json_string(setting.value)},
                            {"reason", tuning::json_string(setting.reason)}}));
                }
            }
            return tuning::json_object({{"runs", std::to_string(runs)},
                {"levels", tuning::json_array(levels)},
                {"settings", tuning::json_array(settings)}});
        }

        // the cache entries of the fingerprint (compared: a file from a
        // machine with other caches has other rates)
        void add_fingerprint(tuning::machine_description const& machine,
            tuning::host_fingerprint& fingerprint)
        {
            std::size_t line_size = 0;
            for (tuning::cache_level const& c : machine.caches)
            {
                if (c.level > max_level)
                    continue;
                fingerprint.add("memory_cache_l" + std::to_string(c.level),
                    std::to_string(c.size));
                if (line_size == 0)
                    line_size = c.line_size;
            }
            if (line_size != 0)
                fingerprint.add("memory_line_size", std::to_string(line_size));
        }
    }    // namespace

    po::options_description memory_options()
    {
        po::options_description desc("Options of the memory area");
        // clang-format off
        desc.add_options()
            ("memory-runs", po::value<std::size_t>()->default_value(3),
                "how many children time the copies, one after the other; the "
                "median of their rates is used");
        // clang-format on
        return desc;
    }

    area_result tune_memory(area_context const& context)
    {
        tuning::machine_description const& machine = context.machine;
        std::size_t const runs = context.vm["memory-runs"].as<std::size_t>();
        if (runs == 0)
            throw std::invalid_argument("--memory-runs: must be at least 1");

        std::vector<footprint> const fps = footprints(machine);
        std::vector<std::string> args = {"--hpx:threads=1",
            "--repeats=" + std::to_string(context.options.repeats)};
        for (footprint const& f : fps)
        {
            args.push_back("--memory-footprint=" + std::to_string(f.level) +
                ":" + std::to_string(f.bytes));
        }

        std::vector<std::vector<child_rate>> all;
        for (std::size_t run = 0; run != runs; ++run)
        {
            std::vector<child_rate> rates;
            for (std::string const& line : run_child("memory", args))
            {
                std::istringstream in(line);
                std::string tag;
                child_rate c;
                if (in >> tag >> c.level >> c.bytes >> c.rate.min >>
                        c.rate.median >> c.rate.max &&
                    tag == "rate")
                {
                    rates.push_back(c);
                }
            }
            all.push_back(HPX_MOVE(rates));
        }

        // per level the median of the runs' medians
        std::map<int, double> chosen;
        for (footprint const& f : fps)
        {
            std::vector<double> medians;
            for (std::vector<child_rate> const& run : all)
                for (child_rate const& c : run)
                    if (c.level == f.level)
                        medians.push_back(c.rate.median);
            if (!medians.empty())
                chosen[f.level] = tuning::summarize(medians).median;
        }
        if (chosen.find(0) == chosen.end())
            throw std::runtime_error(
                "the memory area's children reported no rate");

        tuning::ini_section section{"hpx.task_size_chunk_size", {}};
        std::vector<std::string> comments;
        for (footprint const& f : fps)
        {
            auto const it = chosen.find(f.level);
            if (it == chosen.end())
                continue;
            std::size_t const rate =
                (std::clamp) (static_cast<std::size_t>(it->second + 0.5),
                    std::size_t(1), std::size_t(4096));
            std::string const reason = "a copy of " +
                std::to_string(f.bytes / 2) + " bytes on one core, " +
                (f.level == 0 ?
                        std::string("in memory") :
                        "in L" + std::to_string(f.level) + " (a core's share " +
                            std::to_string(f.share) + " bytes)") +
                ", median of " + std::to_string(runs) + " runs";
            section.settings.push_back(
                {rate_key(f.level), std::to_string(rate), reason, false});
            if (f.level == 0)
            {
                section.settings.push_back(
                    {"hpx.task_size_chunk_size.copy_bytes_per_ns",
                        std::to_string(rate),
                        "the rate for data in memory, as the default means",
                        rate == 16});
            }
            comments.push_back("memory: " + level_name(f.level) + ": " +
                number(it->second) + " bytes per ns (" + reason + ")");
        }
        add_fingerprint(machine, context.fingerprint);

        std::cout << "memory: " << section.settings.size() << " settings ("
                  << runs << " runs)\n";
        for (tuning::tuned_setting const& s : section.settings)
            std::cout << "  " << s.key << " = " << s.value << " (" << s.reason
                      << ")\n";
        if (context.verbose)
        {
            for (std::size_t run = 0; run != all.size(); ++run)
                for (child_rate const& c : all[run])
                    std::cout << "memory: run " << run << ", "
                              << level_name(c.level) << ", " << c.bytes
                              << " bytes: " << number(c.rate.median)
                              << " bytes per ns (" << number(c.rate.min)
                              << " to " << number(c.rate.max) << ")\n";
        }

        std::vector<tuning::ini_section> sections{section};
        std::string json = report(fps, runs, all, chosen, sections);
        return area_result{
            HPX_MOVE(sections), HPX_MOVE(comments), HPX_MOVE(json)};
    }

    int memory_child_main(int argc, char* argv[])
    {
        po::options_description const desc = child_options();
        hpx::local::init_params params;
        params.desc_cmdline = desc;

        return hpx::local::init(
            [](po::variables_map& vm) {
                std::string const prefix = child_line_prefix("memory");
                namespace exd = hpx::execution::experimental::detail;
                if (vm.count("memory-verify"))
                {
                    if (vm.count("memory-share"))
                    {
                        for (std::string const& s :
                            vm["memory-share"].as<std::vector<std::string>>())
                        {
                            auto const [level, bytes] = parse_pair(s);
                            std::cout << prefix << "verify " << level << " "
                                      << exd::get_copy_bytes_per_ns(bytes)
                                      << "\n";
                        }
                    }
                    std::cout << prefix << "verify 0 "
                              << exd::get_copy_bytes_per_ns(
                                     (std::numeric_limits<std::size_t>::max)())
                              << "\n"
                              << std::flush;
                    return hpx::local::finalize();
                }

                std::size_t const repeats = vm["repeats"].as<std::size_t>();
                if (vm.count("memory-footprint"))
                {
                    for (std::string const& s :
                        vm["memory-footprint"].as<std::vector<std::string>>())
                    {
                        auto const [level, bytes] = parse_pair(s);
                        tuning::summary const rate = time_copy(bytes, repeats);
                        std::cout << prefix << "rate " << level << " " << bytes
                                  << " " << rate.min << " " << rate.median
                                  << " " << rate.max << "\n";
                    }
                }
                std::cout << std::flush;
                return hpx::local::finalize();
            },
            argc, argv, params);
    }

    verify_result verify_memory(verify_context const& context)
    {
        tuning::machine_description const& machine = context.context.machine;
        hpx::util::section file;
        file.read(context.file);

        std::vector<std::string> args = {"--hpx:threads=1",
            "--hpx:config=" + context.file, "--memory-verify"};
        std::map<int, std::size_t> shares;
        for (footprint const& f : footprints(machine))
        {
            if (f.level == 0)
                continue;
            shares[f.level] = f.share;
            args.push_back("--memory-share=" + std::to_string(f.level) + ":" +
                std::to_string(f.share));
        }

        // what the file says for a level: its entry, else copy_bytes_per_ns
        auto const file_rate = [&](int const level) -> std::size_t {
            std::string const key = rate_key(level);
            std::string value =
                file.has_entry(key) ? file.get_entry(key) : std::string();
            if (value.empty() &&
                file.has_entry("hpx.task_size_chunk_size.copy_bytes_per_ns"))
                value = file.get_entry(
                    "hpx.task_size_chunk_size.copy_bytes_per_ns");
            return value.empty() ? 0 : std::stoul(value);
        };

        verify_result result;
        std::vector<std::string> items;
        add_fingerprint(machine, context.context.fingerprint);
        for (std::string const& line : run_child("memory", args))
        {
            std::istringstream in(line);
            std::string tag;
            int level = 0;
            std::size_t runtime = 0;
            if (!(in >> tag >> level >> runtime) || tag != "verify")
                continue;
            // the runtime takes the first level, from L1 up, whose share
            // the footprint fits: where a lower level's share is at least
            // this one's, that level's rate
            int effective = level;
            for (auto const& [l, share] : shares)
            {
                if (level != 0 && l < level && share >= shares[level])
                {
                    effective = l;
                    break;
                }
            }
            std::size_t const expected = file_rate(effective);
            bool const holds = expected == 0 || expected == runtime;
            std::string const where = level_name(level) +
                (level == 0 ? std::string() :
                              " (a core's share " +
                            std::to_string(shares[level]) + " bytes)");
            result.lines.push_back(where + ": " + std::to_string(runtime) +
                " bytes per ns" +
                (expected == 0 ?
                        std::string(" (the file gives none)") :
                        " (the file: " + std::to_string(expected) + ")"));
            if (!holds)
            {
                result.failures.push_back(where + ": the runtime uses " +
                    std::to_string(runtime) + " bytes per ns, the file says " +
                    std::to_string(expected));
            }
            items.push_back(tuning::json_object(
                {{"level", tuning::json_string(level_name(level))},
                    {"runtime_bytes_per_ns", std::to_string(runtime)},
                    {"file_bytes_per_ns",
                        expected == 0 ? "null" : std::to_string(expected)},
                    {"holds", holds ? "true" : "false"}}));
        }
        if (items.empty())
            result.failures.push_back("the child reported no rate");
        result.report =
            tuning::json_object({{"levels", tuning::json_array(items)}});
        return result;
    }
}    // namespace hpx_tune

// after the idle area (0), before the chunk area (10): its children start the
// HPX runtime, the chunk area's values don't depend on it
HPX_TUNE_REGISTER_AREA(memory,
    hpx_tune::area{"memory",
        "tune the rates at which a core copies data in its caches and in "
        "memory",
        5, &hpx_tune::memory_options, &hpx_tune::tune_memory,
        &hpx_tune::memory_child_main, &hpx_tune::verify_memory})
