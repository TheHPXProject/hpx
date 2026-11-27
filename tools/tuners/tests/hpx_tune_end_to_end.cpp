//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// An end-to-end test of hpx_tune's chunk and memory areas: one run of the
// tool writes a file with both kinds of values (the minimal chunk time per
// executor and pool size, the copy rates per cache level), the runtime is
// started with it, and library loops on doubles (hpx::copy, hpx::transform
// with std::plus) whose footprint per core falls in each cache level are
// observed through a parameters object that forwards to task_size_chunk_size
// and records its decisions: the bytes of an element, the time of an
// iteration, the cores and the chunk size. They have to be the ones the
// file's values give. A second process of this test, started without the
// file, checks that the same loops decide with HPX's compiled-in defaults.
// The test checks that HPX applies the values, not whether they are good for
// the machine.
//
// HPX_TUNE_EXECUTABLE and HPX_TUNE_TEST_DIRECTORY are set by CMake.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

namespace {

    std::string const areas_file =
        std::string(HPX_TUNE_TEST_DIRECTORY) + "/hpx_tune_end_to_end.ini";

    // the pool size of the test, as for the chunk area's smoke test
    constexpr std::size_t pool_size = 2;

    // HPX's compiled-in defaults (task_size_chunk_size_defaults.cpp)
    constexpr std::int64_t default_min_chunk_time_ps = 2'500'000;
    constexpr std::size_t default_chunks_per_core = 8;
    constexpr std::size_t default_copy_bytes_per_ns = 16;

    // the option that makes this test the process started without the file
    char const* const defaults_option = "--end-to-end-defaults";

    int run_command(std::string command)
    {
#if defined(_WIN32)
        // cmd.exe drops the first and the last quote of a command that
        // starts with one
        command = "\"" + command + "\"";
#endif
        std::cout << "running: " << command << std::endl;
        return std::system(command.c_str());
    }

    ///////////////////////////////////////////////////////////////////////////
    // What a loop decided, as task_size_chunk_size returned it to the
    // algorithm.
    struct decision
    {
        std::size_t bytes = 0;
        std::int64_t iteration_ps = 0;
        std::size_t cores = 0;
        std::size_t chunk_size = 0;
    };

    // A parameters object that forwards to task_size_chunk_size and records
    // its decisions. It is copied with the policy, so the record is shared.
    struct recording_chunk_size
    {
        using uses_task_size = std::true_type;

        template <typename Executor, typename F>
        hpx::chrono::steady_duration measure_iteration(
            Executor&& exec, F&& f, std::size_t const count) const
        {
            record->bytes =
                hpx::traits::get_task_bytes<std::decay_t<F>>::call(f);
            auto const time = tscs.measure_iteration(exec, f, count);
            record->iteration_ps = time.as_picoseconds().count();
            return time;
        }

        template <typename Executor>
        std::size_t processing_units_count(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t const count) const
        {
            record->cores =
                tscs.processing_units_count(exec, iteration_duration, count);
            return record->cores;
        }

        template <typename Executor>
        std::size_t get_chunk_size(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t const cores, std::size_t const count) const
        {
            record->chunk_size =
                tscs.get_chunk_size(exec, iteration_duration, cores, count);
            return record->chunk_size;
        }

        ex::task_size_chunk_size tscs;
        std::shared_ptr<decision> record = std::make_shared<decision>();
    };
}    // namespace

template <>
struct hpx::execution::experimental::is_executor_parameters<
    recording_chunk_size> : std::true_type
{
};

namespace {

    ///////////////////////////////////////////////////////////////////////////
    // The values the decision is computed from: the rates by footprint per
    // core, the minimal chunk time and the chunks per core.
    struct values
    {
        // (a core's share of a level, its rate), by ascending share; empty:
        // copy_bytes_per_ns for every footprint
        std::vector<std::pair<std::size_t, std::size_t>> levels;
        std::size_t memory_rate = default_copy_bytes_per_ns;
        std::size_t copy_bytes_per_ns = default_copy_bytes_per_ns;
        std::int64_t min_chunk_time_ps = default_min_chunk_time_ps;
        std::size_t chunks_per_core = default_chunks_per_core;

        std::size_t rate(std::size_t const footprint) const
        {
            if (levels.empty())
                return copy_bytes_per_ns;
            for (auto const& [share, level_rate] : levels)
            {
                if (footprint <= share)
                    return level_rate;
            }
            return memory_rate;
        }

        // 1 to 3 for the level whose share holds the footprint, 4 for memory,
        // 0 without rates
        int level(std::size_t const footprint) const
        {
            if (levels.empty())
                return 0;
            for (std::size_t i = 0; i != levels.size(); ++i)
            {
                if (footprint <= levels[i].first)
                    return static_cast<int>(i) + 1;
            }
            return 4;
        }
    };

    std::size_t config_size(std::string const& key, std::size_t const value)
    {
        std::string const entry = hpx::get_config_entry(key, "");
        if (entry.empty())
            return value;
        return std::stoul(entry);
    }

    // a rate of the file, 0 if it has none or one the runtime ignores
    std::size_t config_rate(std::string const& key)
    {
        std::size_t const rate = config_size(key, 0);
        return rate > 4096 ? 0 : rate;
    }

    // a core's share of each cache level, as the runtime computes it
    std::vector<std::size_t> cache_shares()
    {
        std::vector<std::size_t> shares;
        auto const& topo = hpx::threads::get_topology();
        for (int level = 1; level <= 3; ++level)
        {
            hpx::threads::cache_info const info = topo.get_cache_info(0, level);
            if (info.size != 0 && info.cores != 0)
                shares.push_back(info.size / info.cores);
        }
        return shares;
    }

    // the values as the runtime has them from the file, computed from the
    // configuration and the topology, not from get_copy_bytes_per_ns
    values file_values()
    {
        values v;
        std::string const prefix = "hpx.task_size_chunk_size.";
        v.copy_bytes_per_ns =
            config_size(prefix + "copy_bytes_per_ns", v.copy_bytes_per_ns);
        v.chunks_per_core =
            config_size(prefix + "chunks_per_core", v.chunks_per_core);

        std::size_t const level_rates[] = {
            config_rate(prefix + "copy_bytes_per_ns_l1"),
            config_rate(prefix + "copy_bytes_per_ns_l2"),
            config_rate(prefix + "copy_bytes_per_ns_l3")};
        std::size_t const memory_rate =
            config_rate(prefix + "copy_bytes_per_ns_memory");
        if (memory_rate != 0 ||
            std::any_of(std::begin(level_rates), std::end(level_rates),
                [](std::size_t const r) { return r != 0; }))
        {
            std::vector<std::size_t> const shares = cache_shares();
            for (std::size_t i = 0; i != shares.size(); ++i)
            {
                v.levels.emplace_back(shares[i],
                    level_rates[i] != 0 ? level_rates[i] : v.copy_bytes_per_ns);
            }
            std::stable_sort(v.levels.begin(), v.levels.end(),
                [](auto const& lhs, auto const& rhs) {
                    return lhs.first < rhs.first;
                });
            v.memory_rate =
                memory_rate != 0 ? memory_rate : v.copy_bytes_per_ns;
        }

        // the fork-join executor's value for this pool size, else the one
        // for the sizes not measured
        std::string value = hpx::get_config_entry(prefix + "fork_join." +
                std::to_string(pool_size) + ".min_chunk_time_us",
            "");
        if (value.empty())
        {
            value = hpx::get_config_entry(
                prefix + "fork_join.min_chunk_time_us", "");
        }
        HPX_TEST_MSG(!value.empty(), "fork_join min_chunk_time_us");
        if (!value.empty())
        {
            std::chrono::nanoseconds const ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::duration<double, std::micro>(
                        std::stod(value)));
            v.min_chunk_time_ps = ns.count() * 1000;
        }
        return v;
    }

    ///////////////////////////////////////////////////////////////////////////
    // The decision task_size_chunk_size makes for count elements of the
    // given bytes on pool_size workers. The algorithm asks measure_iteration
    // for the time of an iteration, which takes the rate of the whole
    // footprint on one core to decide on the cores and then the rate of a
    // core's share of it, and, while that time gives fewer cores, the rate
    // of the larger share; it then asks processing_units_count and
    // get_chunk_size again with that time.
    struct expected_decision
    {
        decision d;
        int level = 0;    // of the footprint per core the time comes from
    };

    std::int64_t time_for(
        values const& v, std::size_t const bytes, std::size_t const footprint)
    {
        std::size_t const rate = v.rate(footprint);
        return static_cast<std::int64_t>((bytes * 1000 + rate) / (2 * rate));
    }

    std::size_t cores_for(values const& v, std::int64_t const iteration_ps,
        std::size_t const count)
    {
        std::int64_t const per_core =
            v.min_chunk_time_ps * static_cast<std::int64_t>(v.chunks_per_core);
        auto const iterations_per_core =
            static_cast<std::size_t>(per_core / iteration_ps);
        std::size_t const cores =
            iterations_per_core == 0 ? count : count / iterations_per_core;
        return std::clamp(cores, std::size_t(1), pool_size);
    }

    expected_decision expect(
        values const& v, std::size_t const bytes, std::size_t const count)
    {
        expected_decision e;
        e.d.bytes = bytes;

        std::int64_t time = time_for(v, bytes, count * bytes);
        std::size_t footprint = count * bytes;
        std::size_t cores = cores_for(v, time, count);
        if (cores > 1)
        {
            footprint = count * bytes / cores;
            time = time_for(v, bytes, footprint);
        }
        while (cores > 1)
        {
            std::size_t const next = cores_for(v, time, count);
            if (next >= cores)
                break;
            cores = next;
            footprint = count * bytes / cores;
            time = time_for(v, bytes, footprint);
        }
        e.d.iteration_ps = time;
        e.level = v.level(footprint);

        e.d.cores = cores_for(v, time, count);
        if (e.d.cores <= 1)
        {
            e.d.chunk_size = count;
        }
        else
        {
            std::size_t const num_chunks = v.chunks_per_core * e.d.cores;
            std::size_t const balanced = (count + num_chunks - 1) / num_chunks;
            auto const min_iterations = static_cast<std::size_t>(
                (v.min_chunk_time_ps + time - 1) / time);
            e.d.chunk_size =
                (std::min) (count, (std::max) (balanced, min_iterations));
        }
        return e;
    }

    bool same(decision const& lhs, decision const& rhs)
    {
        return lhs.bytes == rhs.bytes && lhs.iteration_ps == rhs.iteration_ps &&
            lhs.cores == rhs.cores && lhs.chunk_size == rhs.chunk_size;
    }

    // An element count whose footprint per core is in the middle of the
    // level (1 to 3, 4 for memory), away from a boundary of the rule: the
    // counts next to it decide the same cores and level. 0 if there is none.
    std::size_t count_for_level(values const& v,
        std::vector<std::size_t> const& shares, std::size_t const bytes,
        int const level)
    {
        if (shares.empty())
            return 0;
        std::size_t target = 0;
        if (level == 4)
        {
            target = 4 * *std::max_element(shares.begin(), shares.end());
        }
        else
        {
            auto const index = static_cast<std::size_t>(level) - 1;
            if (index >= shares.size())
                return 0;
            std::size_t const below = index == 0 ? 0 : shares[index - 1];
            target = (below + shares[index]) / 2;
        }

        for (std::size_t cores = 1; cores <= pool_size; ++cores)
        {
            std::size_t const count = target * cores / bytes;
            if (count < 2)
                continue;
            expected_decision const e = expect(v, bytes, count);
            if (e.d.cores != cores || (!v.levels.empty() && e.level != level))
                continue;
            expected_decision const lower = expect(v, bytes, count - 1);
            expected_decision const upper = expect(v, bytes, count + 1);
            if (lower.d.cores == e.d.cores && upper.d.cores == e.d.cores &&
                lower.level == e.level && upper.level == e.level)
            {
                return count;
            }
        }
        return 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Run the loop with the recording parameters on a fork-join executor and
    // return what it decided.
    template <typename Loop>
    decision observe(ex::fork_join_executor const& exec, Loop&& loop)
    {
        recording_chunk_size const recording;
        loop(hpx::execution::par.on(exec).with(recording));
        return *recording.record;
    }

    decision run_copy(ex::fork_join_executor const& exec, std::size_t count)
    {
        std::vector<double> a(count), b(count, 0.0);
        for (std::size_t i = 0; i != count; ++i)
            a[i] = static_cast<double>(i);
        decision const d = observe(exec, [&](auto const& policy) {
            hpx::copy(policy, a.begin(), a.end(), b.begin());
        });
        HPX_TEST_MSG(a == b, "copy");
        return d;
    }

    decision run_transform(
        ex::fork_join_executor const& exec, std::size_t count)
    {
        std::vector<double> a(count), b(count, 1.0), c(count, 0.0);
        for (std::size_t i = 0; i != count; ++i)
            a[i] = static_cast<double>(i);
        decision const d = observe(exec, [&](auto const& policy) {
            hpx::transform(policy, a.begin(), a.end(), b.begin(), c.begin(),
                std::plus<>());
        });
        std::size_t wrong = 0;
        for (std::size_t i = 0; i != count; ++i)
            wrong += c[i] != a[i] + 1.0 ? 1 : 0;
        HPX_TEST_EQ_MSG(wrong, std::size_t(0), "transform");
        return d;
    }

    void print(char const* what, decision const& d)
    {
        std::cout << "  " << what << ": " << d.bytes << " bytes, "
                  << d.iteration_ps << " ps, " << d.cores << " cores, chunk "
                  << d.chunk_size << "\n";
    }

    struct loop
    {
        char const* name;
        std::size_t bytes;
        decision (*run)(ex::fork_join_executor const&, std::size_t);
    };

    // Each loop at each level: the recorded decision is the expected one.
    // Returns the iteration times of the copy at the levels checked.
    std::vector<std::int64_t> check_loops(values const& v)
    {
        std::vector<std::size_t> const shares = cache_shares();
        ex::fork_join_executor const exec;
        loop const loops[] = {
            {"copy", 16, &run_copy}, {"transform", 24, &run_transform}};

        std::vector<std::int64_t> copy_times;
        for (int level = 1; level <= 4; ++level)
        {
            for (loop const& l : loops)
            {
                std::size_t const count =
                    count_for_level(v, shares, l.bytes, level);
                std::string const what = std::string(l.name) + " at level " +
                    std::to_string(level) + ", " + std::to_string(count) +
                    " elements";
                if (count == 0)
                {
                    std::cout << what
                              << ": no count away from a boundary, skipped\n";
                    continue;
                }
                expected_decision const e = expect(v, l.bytes, count);
                decision const d = l.run(exec, count);
                std::cout << what << "\n";
                print("expected", e.d);
                print("recorded", d);
                HPX_TEST_MSG(same(d, e.d), what.c_str());
                if (l.bytes == 16)
                    copy_times.push_back(d.iteration_ps);
            }
        }
        return copy_times;
    }

    ///////////////////////////////////////////////////////////////////////////
    // the runtime started with the tool's file
    int check_with_file()
    {
        values const v = file_values();
        HPX_TEST_MSG(!v.levels.empty(), "the file's rates per level");

        std::vector<std::int64_t> const times = check_loops(v);
        HPX_TEST_LTE(std::size_t(2), times.size());

        // the rates per level take effect: the copy's iteration time differs
        // between levels, unless the file gives every level the same rate
        bool const equal_rates = std::all_of(v.levels.begin(), v.levels.end(),
            [&](auto const& l) { return l.second == v.memory_rate; });
        if (equal_rates)
        {
            std::cout << "the file's rates are equal at every level, the "
                         "difference between levels isn't checked\n";
        }
        else
        {
            HPX_TEST_MSG(std::adjacent_find(times.begin(), times.end(),
                             std::not_equal_to<>()) != times.end(),
                "the iteration times differ between levels");
        }
        return hpx::local::finalize();
    }

    // the runtime started without the file: HPX's compiled-in defaults
    int check_with_defaults()
    {
        values const v;
        HPX_TEST_EQ(
            static_cast<std::int64_t>(
                ex::get_min_chunk_time(ex::fork_join_executor()).count()) *
                1000,
            v.min_chunk_time_ps);
        check_loops(v);
        return hpx::local::finalize();
    }
}    // namespace

int main(int argc, char* argv[])
{
    // the process started without the file
    bool const defaults = std::any_of(argv + 1, argv + argc,
        [](char const* arg) { return std::strcmp(arg, defaults_option) == 0; });
    if (defaults)
    {
        std::vector<char*> args;
        for (int i = 0; i != argc; ++i)
        {
            if (std::strcmp(argv[i], defaults_option) != 0)
                args.push_back(argv[i]);
        }
        args.push_back(nullptr);
        HPX_TEST_EQ(hpx::local::init(check_with_defaults,
                        static_cast<int>(args.size() - 1), args.data()),
            0);
        return hpx::util::report_errors();
    }

    // one run of both areas, small enough for a test
    HPX_TEST_EQ(run_command("\"" + std::string(HPX_TUNE_EXECUTABLE) +
                    "\" --memory --memory-runs 1 --chunk --chunk-pool-size " +
                    std::to_string(pool_size) +
                    " --chunk-runs 1 --chunk-regions 50 --repeats 3"
                    " --output \"" +
                    areas_file + "\""),
        0);

    // the runtime started with the file
    std::string const config = "--hpx:config=" + areas_file;
    std::string const threads = "--hpx:threads=" + std::to_string(pool_size);
    {
        std::vector<char*> args(argv, argv + argc);
        args.push_back(const_cast<char*>(config.c_str()));
        args.push_back(const_cast<char*>(threads.c_str()));
        args.push_back(nullptr);
        HPX_TEST_EQ(hpx::local::init(check_with_file,
                        static_cast<int>(args.size() - 1), args.data()),
            0);
    }

    // this test once more, without the file
    HPX_TEST_EQ(run_command("\"" + std::string(argv[0]) + "\" " +
                    defaults_option + " " + threads),
        0);

    return hpx::util::report_errors();
}
