//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/tuning.hpp>

#include "area.hpp"
#include "child.hpp"
#include "idle_area.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

namespace po = hpx::program_options;
namespace tuning = hpx::tuning;

namespace hpx_tune {

    namespace {

        // a configuration given as <scheduler>[:<tuner>]
        struct configuration
        {
            std::string scheduler;
            std::string tuner;
        };

        configuration parse_configuration(std::string const& text)
        {
            auto const colon = text.find(':');
            if (colon == std::string::npos)
                return configuration{text, ""};
            return configuration{text.substr(0, colon), text.substr(colon + 1)};
        }

        // a measurement with about three significant digits: 15626, 55.2,
        // 1.31
        std::string number(double const value)
        {
            double const magnitude = value < 0 ? -value : value;
            int const decimals = magnitude >= 100.0 ? 0 :
                magnitude >= 10.0                   ? 1 :
                                                      2;
            std::ostringstream os;
            os << std::fixed << std::setprecision(decimals) << value;
            return os.str();
        }

        std::string describe(tuning::summary const& s, char const* unit)
        {
            return number(s.median) + " " + unit + " (quartiles " +
                number(s.q1) + " to " + number(s.q3) + ")";
        }

        // the idle area's part of the report: what it tuned for, every
        // measurement and the settings with their reasons
        std::string report(std::vector<configuration> const& configurations,
            std::vector<std::size_t> const& pool_sizes,
            tuning::summary const& clock, tuning::summary const& spin,
            tuning::idle_measurements const& measured,
            std::vector<tuning::tuned_setting> const& settings)
        {
            std::vector<std::string> tuned_for;
            for (configuration const& c : configurations)
            {
                // the tuner as the section names it ("none" for none)
                tuned_for.push_back(tuning::json_object({
                    {"scheduler", tuning::json_string(c.scheduler)},
                    {"tuner",
                        tuning::json_string(
                            c.tuner.empty() ? "none" : c.tuner)},
                }));
            }
            // the pool sizes asked for, empty for pools of any size
            std::vector<std::string> sizes;
            for (std::size_t const size : pool_sizes)
            {
                if (size != tuning::any_pool_size)
                    sizes.push_back(std::to_string(size));
            }
            std::vector<std::string> timed_waits;
            for (tuning::timed_wait const& w : measured.timed_waits)
            {
                timed_waits.push_back(tuning::json_object({
                    {"requested_us", tuning::json_number(w.requested_us)},
                    {"actual_us", tuning::to_json(w.actual_us)},
                }));
            }
            std::vector<std::string> chosen;
            for (tuning::tuned_setting const& s : settings)
                chosen.push_back(tuning::to_json(s));

            return tuning::json_object({
                {"configurations", tuning::json_array(tuned_for)},
                {"pool_sizes", tuning::json_array(sizes)},
                {"clock_read_ns", tuning::to_json(clock)},
                {"spin_rate_rounds_per_us", tuning::to_json(spin)},
                {"timed_waits", tuning::json_array(timed_waits)},
                {"wake_up",
                    tuning::json_object({
                        {"asleep_us", tuning::to_json(measured.wake.asleep_us)},
                        {"spinning_us",
                            tuning::to_json(measured.wake.spinning_us)},
                    })},
                {"settings", tuning::json_array(chosen)},
            });
        }
    }    // namespace

    po::options_description idle_options()
    {
        po::options_description desc("Options of the idle area (--idle)");
        // clang-format off
        desc.add_options()
            ("configuration,c", po::value<std::vector<std::string>>(),
                "a scheduler and its tuner as <scheduler>[:<tuner>], as "
                "--hpx:queuing and hpx.scheduler_tuner name them; may be "
                "given more than once (default: local-priority-fifo)")
            ("pool-size,p", po::value<std::vector<std::size_t>>(),
                "a pool size in workers to write settings for; may be given "
                "more than once (default: one section for pools of any "
                "size)")
            ("check-ini", po::value<std::vector<std::string>>(),
                "with --check, for tests: a setting the children get with "
                "--hpx:ini, so that they run with another value than the "
                "file says; may be given more than once");
        // clang-format on
        return desc;
    }

    // The settings measured so far don't depend on the scheduler or the pool
    // size, so every row gets the same ones, and without --pool-size one row
    // per configuration holds for pools of any size (a pool of half a NUMA
    // domain gets them as well as a whole one).
    area_result tune_idle(area_context const& context)
    {
        po::variables_map const& vm = context.vm;
        tuning::microbenchmark_options const& options = context.options;
        bool const verbose = context.verbose;

        std::vector<std::size_t> pool_sizes = {tuning::any_pool_size};
        if (vm.count("pool-size"))
            pool_sizes = vm["pool-size"].as<std::vector<std::size_t>>();

        std::vector<configuration> configurations;
        if (vm.count("configuration"))
        {
            for (auto const& c :
                vm["configuration"].as<std::vector<std::string>>())
            {
                configurations.push_back(parse_configuration(c));
            }
        }
        else
        {
            configurations.push_back(configuration{"local-priority-fifo", ""});
        }

        // phase 2: the measurements without a runtime
        tuning::summary const clock = tuning::measure_clock_read(options);
        tuning::summary const spin = tuning::measure_spin_rate(options);
        tuning::idle_measurements measured;
        measured.timed_waits = tuning::measure_timed_waits(1024.0, options);
        measured.wake = tuning::measure_wake_cost(options);

        if (verbose)
        {
            std::cout << "clock read: " << describe(clock, "ns") << "\n"
                      << "spin rate: " << describe(spin, "rounds/us") << "\n";
            for (tuning::timed_wait const& w : measured.timed_waits)
            {
                std::cout << "timed wait of " << number(w.requested_us)
                          << " us: " << describe(w.actual_us, "us") << "\n";
            }
            std::cout << "wake up, asleep: "
                      << describe(measured.wake.asleep_us, "us")
                      << "\nwake up, spinning: "
                      << describe(measured.wake.spinning_us, "us") << "\n";
        }

        // phase 4: the rules
        std::vector<tuning::tuned_setting> const settings =
            tuning::derive_idle_settings(measured);

        std::vector<tuning::tuning_row> rows;
        for (configuration const& c : configurations)
        {
            for (std::size_t const size : pool_sizes)
            {
                rows.push_back(
                    tuning::tuning_row{c.scheduler, c.tuner, size, settings});
            }
        }

        bool const any_size = pool_sizes.size() == 1 &&
            pool_sizes.front() == tuning::any_pool_size;
        std::cout << "idle: " << rows.size() << " sections ("
                  << configurations.size() << " configurations, "
                  << (any_size ?
                             std::string("any pool size") :
                             std::to_string(pool_sizes.size()) + " pool sizes")
                  << ")\n";
        for (tuning::tuned_setting const& s : settings)
        {
            std::cout << "  " << s.key << " = " << s.value << " (" << s.reason
                      << (s.is_default ? ", HPX's default" : "") << ")\n";
        }

        return area_result{tuning::to_sections(rows),
            {"idle: clock read " + describe(clock, "ns"),
                "idle: spin rate " + describe(spin, "rounds/us")},
            report(
                configurations, pool_sizes, clock, spin, measured, settings)};
    }
}    // namespace hpx_tune

namespace hpx_tune {

    namespace {

        char const* const longest_key = "max_idle_backoff_time";
        char const* const spin_key = "min_idle_backoff_time";

        // The plain settings the children of the check run with: values the
        // tool doesn't write (it writes a longest step of whole milliseconds
        // and a spin of at least the shortest sleep), so that a value read
        // back shows where it came from.
        constexpr double plain_longest = 777.0;
        constexpr double plain_spin = 33.0;

        // the scheduler whose tuner is named in its rows; all other
        // schedulers' rows have the tuner "none" (see #8259)
        char const* const tuned_scheduler_name = "local-priority-tuned-fifo";

        // one row of the file: its settings that the runtime takes, that is
        // those that are a number and not negative (as the lookup reads
        // them), missing otherwise
        struct idle_row
        {
            std::optional<double> longest;
            std::optional<double> spin;
        };

        // the rows of one scheduler and tuner: for pools of any size, and
        // per pool size
        struct idle_rows
        {
            std::optional<idle_row> any_size;
            std::map<std::size_t, idle_row> by_size;
        };

        std::optional<double> setting_value(
            hpx::util::section const& section, char const* key)
        {
            if (!section.has_entry(key))
                return std::nullopt;
            // as the runtime reads it (hpx::util::get_entry_as)
            double const value =
                hpx::util::from_string<double>(section.get_entry(key), -1.0);
            if (value < 0.0)
                return std::nullopt;
            return value;
        }

        idle_row read_row(hpx::util::section const& section)
        {
            return idle_row{setting_value(section, longest_key),
                setting_value(section, spin_key)};
        }

        bool is_number(std::string const& text)
        {
            return !text.empty() &&
                std::all_of(text.begin(), text.end(),
                    [](char c) { return c >= '0' && c <= '9'; });
        }

        // the file's hpx.idle_tuning.<scheduler>.<tuner>[.<workers>], by
        // scheduler and tuner
        std::map<std::pair<std::string, std::string>, idle_rows> read_rows(
            hpx::util::section const& file)
        {
            std::map<std::pair<std::string, std::string>, idle_rows> rows;
            if (!file.has_section("hpx.idle_tuning"))
                return rows;
            hpx::util::section const* const table =
                file.get_section("hpx.idle_tuning");
            for (auto const& [scheduler, scheduler_section] :
                table->get_sections())
            {
                for (auto const& [tuner, tuner_section] :
                    scheduler_section.get_sections())
                {
                    idle_rows& r = rows[{scheduler, tuner}];
                    if (!tuner_section.get_entries().empty())
                        r.any_size = read_row(tuner_section);
                    for (auto const& [size, size_section] :
                        tuner_section.get_sections())
                    {
                        if (is_number(size))
                        {
                            r.by_size[std::stoul(size)] =
                                read_row(size_section);
                        }
                    }
                }
            }
            return rows;
        }

        // the value the lookup should give a pool of the given size: the
        // row of its size, else the row for any size, else the plain value
        std::pair<double, std::string> expected_value(idle_rows const& rows,
            std::size_t const workers,
            std::optional<double> idle_row::* const setting, double const plain)
        {
            if (auto const it = rows.by_size.find(workers);
                it != rows.by_size.end() && it->second.*setting)
            {
                return {*(it->second.*setting),
                    "the row of " + std::to_string(workers) + " workers"};
            }
            if (rows.any_size && (*rows.any_size).*setting)
                return {*((*rows.any_size).*setting), "the row for any size"};
            return {plain, "the plain setting"};
        }

        std::string value_text(double const value)
        {
            std::ostringstream os;
            os << value;
            return os.str();
        }

        // the two pool sizes of the child of one scheduler and tuner: the
        // first one with a row of its own if the file has one that fits
        // (smallest first), the second one without; 0 if there is no room
        // for a second pool
        std::pair<std::size_t, std::size_t> pool_sizes_for(
            idle_rows const& rows, std::size_t const cores)
        {
            std::size_t first = 0;
            for (auto const& [size, row] : rows.by_size)
            {
                if (size + 1 <= cores)
                {
                    first = size;
                    break;
                }
            }
            if (first == 0)
                first = cores >= 3 ? 2 : 1;
            std::size_t second = 0;
            for (std::size_t size = 1; first + size <= cores; ++size)
            {
                if (rows.by_size.count(size) == 0)
                {
                    second = size;
                    break;
                }
            }
            return {first, second};
        }

#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
        // The settings a scheduler was constructed with are protected
        // members of scheduler_base; a pointer to them is formed through a
        // derived class (never instantiated) and applied to a pool's
        // scheduler.
        struct idle_settings_of : hpx::threads::policies::scheduler_base
        {
            static double longest(
                hpx::threads::policies::scheduler_base const& s)
            {
                return s.*(&idle_settings_of::max_idle_backoff_time_);
            }
            static double spin(hpx::threads::policies::scheduler_base const& s)
            {
                return s.*(&idle_settings_of::min_idle_backoff_time_);
            }
        };

        // the child's lines: "pool <name> <workers> <longest> <spin>" per
        // pool, and for the tuned scheduler "tuned <name> <workers> <same>
        // <longest> <spin>", its tuning parameters of worker 0 and whether
        // every worker has the same ("tuned <name> <workers> none" if it has
        // no idle parameters)
        int write_idle_settings()
        {
            namespace policies = hpx::threads::policies;
            std::string const prefix = child_line_prefix("idle");
            // every digit of a setting, so that the parent compares values
            std::cout << std::setprecision(17);
            for (std::size_t i = 0; i != hpx::resource::get_num_thread_pools();
                ++i)
            {
                std::string const name = hpx::resource::get_pool_name(i);
                policies::scheduler_base const* const scheduler =
                    hpx::resource::get_thread_pool(name).get_scheduler();
                std::size_t const workers =
                    hpx::resource::get_num_threads(name);
                std::cout << prefix << "pool " << name << " " << workers << " "
                          << idle_settings_of::longest(*scheduler) << " "
                          << idle_settings_of::spin(*scheduler) << "\n";

                auto const* const tuned = dynamic_cast<
                    policies::local_priority_tuned_scheduler<> const*>(
                    scheduler);
                if (tuned == nullptr)
                    continue;
                auto const& parameters = tuned->get_tuning_parameters();
                std::size_t const longest_id =
                    parameters.find("hpx.max_idle_backoff_time");
                std::size_t const spin_id =
                    parameters.find("hpx.min_idle_backoff_time");
                if (longest_id == policies::scheduler_tuning_parameters::npos ||
                    spin_id == policies::scheduler_tuning_parameters::npos)
                {
                    std::cout << prefix << "tuned " << name << " " << workers
                              << " none\n";
                    continue;
                }
                float const longest = parameters.get<float>(0, longest_id);
                float const spin = parameters.get<float>(0, spin_id);
                bool same = true;
                for (std::size_t w = 1; w != workers; ++w)
                {
                    same = same &&
                        parameters.get<float>(w, longest_id) == longest &&
                        parameters.get<float>(w, spin_id) == spin;
                }
                std::cout << prefix << "tuned " << name << " " << workers << " "
                          << (same ? 1 : 0) << " " << longest << " " << spin
                          << "\n";
            }
            std::cout << std::flush;
            return hpx::local::finalize();
        }
#endif
    }    // namespace

    int idle_child_main(int argc, char* argv[])
    {
        po::options_description desc("Options of a child of the idle area");
        // clang-format off
        desc.add_options()
            ("child", po::value<std::string>(),
                "run as a child of the given area")
            ("idle-first-pool", po::value<std::size_t>()->default_value(0),
                "the workers of the default pool; the others go to a pool "
                "'second' (0: one pool)");
        // clang-format on

        std::size_t first = 0;
        for (int i = 1; i < argc; ++i)
        {
            std::string const arg = argv[i];
            std::string const option = "--idle-first-pool=";
            if (arg.compare(0, option.size(), option) == 0)
                first = std::stoul(arg.substr(option.size()));
        }

        hpx::local::init_params params;
        params.desc_cmdline = desc;
        if (first != 0)
        {
            // two pools of the scheduler of --hpx:queuing, one worker per
            // core, the first processing unit of each
            params.rp_callback = [first](hpx::resource::partitioner& rp,
                                     po::variables_map const&) {
                std::size_t const threads = rp.get_number_requested_threads();
                rp.create_thread_pool("default");
                if (threads > first)
                    rp.create_thread_pool("second");
                std::size_t added = 0;
                for (hpx::resource::numa_domain const& d : rp.numa_domains())
                {
                    for (hpx::resource::core const& c : d.cores())
                    {
                        if (added < threads && !c.pus().empty())
                        {
                            rp.add_resource(c.pus().front(),
                                added < first ? "default" : "second");
                        }
                        ++added;
                    }
                }
            };
        }

#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
        return hpx::local::init(
            [](po::variables_map&) { return write_idle_settings(); }, argc,
            argv, params);
#else
        return hpx::local::init(
            [](po::variables_map&) { return hpx::local::finalize(); }, argc,
            argv, params);
#endif
    }

    verify_result verify_idle(verify_context const& context)
    {
        verify_result result;
#if !defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
        result.lines.push_back(
            "this HPX has no idle backoff (HPX_WITH_THREAD_MANAGER_IDLE_BACKOFF"
            "=OFF): nothing to check");
        return result;
#else
        hpx::util::section file;
        file.read(context.file);
        auto const all_rows = read_rows(file);
        if (all_rows.empty())
        {
            result.lines.push_back("no idle rows in the file");
            return result;
        }

        std::vector<std::string> extra;
        if (context.context.vm.count("check-ini"))
        {
            for (std::string const& s :
                context.context.vm["check-ini"].as<std::vector<std::string>>())
            {
                extra.push_back("--hpx:ini=" + s);
            }
        }

        std::size_t const cores =
            (std::max) (std::size_t(1), context.context.machine.cores);
        for (auto const& [key, rows] : all_rows)
        {
            auto const& [scheduler, tuner] = key;
            std::string const row_name = scheduler + "." + tuner;
            bool const tuned = scheduler == tuned_scheduler_name;
            if (!tuned && tuner != "none")
            {
                result.lines.push_back(row_name + ": never taken (only " +
                    tuned_scheduler_name + " has tuners), not checked");
                continue;
            }

            auto const [first, second] = pool_sizes_for(rows, cores);
            std::vector<std::string> args = {
                "--hpx:threads=" + std::to_string(first + second),
                "--hpx:config=" + context.file, "--hpx:queuing=" + scheduler,
                "--hpx:ini=hpx.max_idle_backoff_time=" +
                    value_text(plain_longest),
                "--hpx:ini=hpx.min_idle_backoff_time=" + value_text(plain_spin),
                "--idle-first-pool=" + std::to_string(second == 0 ? 0 : first)};
            if (tuned)
                args.push_back("--hpx:ini=hpx.scheduler_tuner=" + tuner);
            args.insert(args.end(), extra.begin(), extra.end());

            std::vector<std::string> lines;
            try
            {
                lines = run_child("idle", args);
            }
            catch (std::exception const& e)
            {
                result.failures.push_back(row_name + ": " + e.what());
                continue;
            }

            std::size_t pools = 0;
            for (std::string const& line : lines)
            {
                std::istringstream in(line);
                std::string tag, pool;
                in >> tag >> pool;
                double longest = 0.0, spin = 0.0;
                std::size_t workers = 0;
                int same = 1;
                if (tag == "pool")
                {
                    if (!(in >> workers >> longest >> spin))
                        continue;
                    ++pools;
                }
                else if (tag == "tuned")
                {
                    std::string word;
                    if (!(in >> workers >> word))
                        continue;
                    if (word == "none")
                    {
                        result.failures.push_back(row_name + ", pool " + pool +
                            ": the tuned scheduler has no idle tuning "
                            "parameters");
                        continue;
                    }
                    same = word == "1" ? 1 : 0;
                    if (!(in >> longest >> spin))
                        continue;
                }
                else
                {
                    continue;
                }

                std::string const where = row_name + ", pool " + pool + " of " +
                    std::to_string(workers) + " workers" +
                    (tag == "tuned" ? ", tuning parameters" : "");
                for (auto const& [setting, member, plain, value] :
                    {std::tuple{longest_key, &idle_row::longest, plain_longest,
                         longest},
                        std::tuple{
                            spin_key, &idle_row::spin, plain_spin, spin}})
                {
                    auto const [expected, from] =
                        expected_value(rows, workers, member, plain);
                    std::string const text = where + ": " + setting + " " +
                        value_text(value) + ", " + from + " says " +
                        value_text(expected);
                    if (std::abs(value - expected) < 0.5)
                        result.lines.push_back(text);
                    else
                        result.failures.push_back(text);
                }
                if (same == 0)
                {
                    result.failures.push_back(
                        where + ": the workers' parameters differ");
                }
            }
            std::size_t const expected_pools = second == 0 ? 1 : 2;
            if (pools != expected_pools)
            {
                result.failures.push_back(row_name + ": " +
                    std::to_string(pools) + " pools read back, expected " +
                    std::to_string(expected_pools));
            }
        }
        return result;
#endif
    }
}    // namespace hpx_tune

// the first area: it measures in this process, on which nothing of HPX runs;
// its check runs children with the runtime
HPX_TUNE_REGISTER_AREA(idle,
    hpx_tune::area{"idle", "tune the scheduler's idle settings", 0,
        &hpx_tune::idle_options, &hpx_tune::tune_idle,
        &hpx_tune::idle_child_main, &hpx_tune::verify_idle})
