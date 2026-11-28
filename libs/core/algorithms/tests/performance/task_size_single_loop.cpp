//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// How well task_size_chunk_size sizes a single loop over builtin elements: for
// each algorithm, number of elements and placement of the data, the run time
// with the cores and chunk size task_size_chunk_size chooses (with the
// configuration the application is given, for instance the file of hpx_tune)
// against the best of a sweep over the number of cores (1, 2, 4, ... and all)
// and the chunks per core (1, 2, 4, 8).
//
// The placements of the data before each timed run:
//   warm     the same loop ran right before with the same cores and chunks,
//            so each core finds its share in its own caches (where the rates
//            of a core's private cache levels are meant to apply)
//   foreign  all workers wrote the inputs right before, in blocks in the
//            reverse order of the workers, so the data is in other cores'
//            caches and the shared ones (a loop after one that ran on other
//            cores)
//   cold     all workers wrote a buffer of --evict_mb right before, so the
//            data is in memory
//
// The loops: copy, transform with std::plus, reduce with std::plus, equal,
// find (of a value that isn't there), minmax_element on doubles, equal on
// chars, and merge of two sorted halves of random doubles (whose comparisons
// mispredict their branches). They run on a fork_join_executor, with
// --executor=suspending_fork_join on a suspending_fork_join_executor (both
// with the static schedule, so that the same shape puts the same chunks on
// the same workers), or with --executor=parallel on the parallel executor.
//
// Prints one CSV line per case and shape, then per case the shape chosen, the
// best one and the time of the chosen over the best, and the geometric mean
// of that ratio per placement and per loop.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/numeric.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

namespace {

    // the shape of a loop as the executor parameters decided it
    struct decision
    {
        std::size_t cores = 0;
        std::size_t chunk_size = 0;
        std::int64_t iteration_ps = 0;
    };

    // task_size_chunk_size, recording what it decides
    struct recording_chunk_size
    {
        using uses_task_size = std::true_type;

        template <typename Executor, typename F>
        hpx::chrono::steady_duration measure_iteration(
            Executor&& exec, F&& f, std::size_t const count) const
        {
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

    // a given shape: the cores and the chunk size, whatever the loop body
    struct fixed_shape
    {
        using uses_task_size = std::true_type;

        template <typename Executor, typename F>
        hpx::chrono::steady_duration measure_iteration(
            Executor&&, F&&, std::size_t) const
        {
            return std::chrono::nanoseconds(1);
        }

        template <typename Executor>
        std::size_t processing_units_count(
            Executor&&, hpx::chrono::steady_duration const&, std::size_t) const
        {
            return cores;
        }

        template <typename Executor>
        std::size_t get_chunk_size(Executor&&,
            hpx::chrono::steady_duration const&, std::size_t, std::size_t) const
        {
            return chunk_size;
        }

        std::size_t cores;
        std::size_t chunk_size;
    };
}    // namespace

template <>
struct hpx::execution::experimental::is_executor_parameters<
    recording_chunk_size> : std::true_type
{
};

template <>
struct hpx::execution::experimental::is_executor_parameters<fixed_shape>
  : std::true_type
{
};

namespace {

    struct config
    {
        std::chrono::milliseconds time_per_case;
        std::size_t min_reps;
        std::size_t max_reps;
        std::size_t evict_bytes;
    };

    enum class placement
    {
        warm,
        foreign,
        cold
    };

    char const* placement_name(placement const p)
    {
        switch (p)
        {
        case placement::warm:
            return "warm";
        case placement::foreign:
            return "foreign";
        default:
            return "cold";
        }
    }

    // the data of a loop: inputs a and b (and their values, to write them
    // back), the output c, chars for equal on chars
    struct data
    {
        explicit data(std::size_t const n)
          : a(n)
          , b(n)
          , c(n)
          , chars_a(n, 'a')
          , chars_b(n, 'a')
        {
            std::mt19937_64 gen(42);
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            for (auto& x : a)
                x = dist(gen);
            b = a;
            // merge: two sorted halves of random values
            std::sort(b.begin(), b.begin() + n / 2);
            std::sort(b.begin() + n / 2, b.end());
            sorted_b = b;
            values_a = a;
        }

        std::vector<double> a, b, c;
        std::vector<double> values_a, sorted_b;
        std::vector<char> chars_a, chars_b;
    };

    // writes the given ranges with all workers, worker i the block that
    // worker workers - 1 - i reads in a loop of all of them
    template <typename Exec>
    void write_foreign(Exec& exec, std::size_t const workers, data& d)
    {
        std::size_t const n = d.a.size();
        hpx::experimental::for_loop(
            hpx::execution::par.on(exec).with(ex::static_chunk_size(1)),
            std::size_t(0), workers, [&](std::size_t const i) {
                std::size_t const block = workers - 1 - i;
                std::size_t const first = n * block / workers;
                std::size_t const last = n * (block + 1) / workers;
                std::copy(d.values_a.begin() + first, d.values_a.begin() + last,
                    d.a.begin() + first);
                std::copy(d.sorted_b.begin() + first, d.sorted_b.begin() + last,
                    d.b.begin() + first);
                std::fill(
                    d.chars_a.begin() + first, d.chars_a.begin() + last, 'a');
                std::fill(
                    d.chars_b.begin() + first, d.chars_b.begin() + last, 'a');
            });
    }

    // writes the eviction buffer with all workers
    template <typename Exec>
    void evict(Exec& exec, std::size_t const workers, std::vector<char>& buffer,
        char const value)
    {
        std::size_t const n = buffer.size();
        hpx::experimental::for_loop(
            hpx::execution::par.on(exec).with(ex::static_chunk_size(1)),
            std::size_t(0), workers, [&](std::size_t const i) {
                std::fill(buffer.begin() + n * i / workers,
                    buffer.begin() + n * (i + 1) / workers, value);
            });
    }

    // runs the loop of the given name with the given policy
    template <typename Policy>
    void run_loop(std::string const& name, Policy const& policy, data& d)
    {
        std::size_t const n = d.a.size();
        if (name == "copy")
        {
            hpx::copy(policy, d.a.begin(), d.a.end(), d.c.begin());
        }
        else if (name == "transform_plus")
        {
            hpx::transform(policy, d.a.begin(), d.a.end(), d.b.begin(),
                d.c.begin(), std::plus<>());
        }
        else if (name == "reduce_plus")
        {
            double const volatile sum =
                hpx::reduce(policy, d.a.begin(), d.a.end(), 0.0, std::plus<>());
            (void) sum;
        }
        else if (name == "equal")
        {
            bool const volatile same =
                hpx::equal(policy, d.a.begin(), d.a.end(), d.a.begin());
            (void) same;
        }
        else if (name == "equal_char")
        {
            bool const volatile same = hpx::equal(
                policy, d.chars_a.begin(), d.chars_a.end(), d.chars_b.begin());
            (void) same;
        }
        else if (name == "find")
        {
            bool const volatile found =
                hpx::find(policy, d.a.begin(), d.a.end(), -1.0) != d.a.end();
            (void) found;
        }
        else if (name == "minmax_element")
        {
            auto const r = hpx::minmax_element(policy, d.a.begin(), d.a.end());
            double const volatile m = *r.min;
            (void) m;
        }
        else if (name == "merge")
        {
            hpx::merge(policy, d.b.begin(), d.b.begin() + n / 2,
                d.b.begin() + n / 2, d.b.end(), d.c.begin());
        }
    }

    // the median time of a loop with the given policy in the given placement
    template <typename Exec, typename Policy>
    double time_loop(config const& cfg, Exec& exec, std::size_t const workers,
        std::string const& name, Policy const& policy, placement const where,
        data& d, std::vector<char>& buffer)
    {
        std::vector<double> times;
        auto const start = std::chrono::steady_clock::now();
        char value = 0;
        while (times.size() < cfg.min_reps ||
            (times.size() < cfg.max_reps &&
                std::chrono::steady_clock::now() - start < cfg.time_per_case))
        {
            switch (where)
            {
            case placement::warm:
                run_loop(name, policy, d);
                break;
            case placement::foreign:
                write_foreign(exec, workers, d);
                break;
            case placement::cold:
                evict(exec, workers, buffer, ++value);
                break;
            }

            auto const t0 = std::chrono::steady_clock::now();
            run_loop(name, policy, d);
            times.push_back(std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - t0)
                    .count());
        }

        std::sort(times.begin(), times.end());
        std::size_t const middle = times.size() / 2;
        return times.size() % 2 != 0 ?
            times[middle] :
            (times[middle - 1] + times[middle]) / 2.0;
    }

    struct case_result
    {
        std::string loop;
        std::size_t elements;
        placement where;
        decision chosen;
        double chosen_us;
        std::size_t best_cores;
        std::size_t best_chunk;
        double best_us;
    };

    template <typename Exec>
    case_result run_case(config const& cfg, Exec& exec,
        std::size_t const workers, std::string const& name,
        std::size_t const elements, placement const where, data& d,
        std::vector<char>& buffer)
    {
        auto const base = hpx::execution::par.on(exec);

        recording_chunk_size const recording;
        case_result result{name, elements, where, {}, 0.0, 0, 0, 0.0};
        result.chosen_us = time_loop(
            cfg, exec, workers, name, base.with(recording), where, d, buffer);
        result.chosen = *recording.record;
        std::cout << name << ',' << elements << ',' << placement_name(where)
                  << ",chosen," << result.chosen.cores << ','
                  << result.chosen.chunk_size << ',' << result.chosen_us
                  << std::endl;

        result.best_us = result.chosen_us;
        result.best_cores = result.chosen.cores;
        result.best_chunk = result.chosen.chunk_size;

        std::vector<std::size_t> core_counts;
        for (std::size_t cores = 1; cores < workers; cores *= 2)
            core_counts.push_back(cores);
        core_counts.push_back(workers);

        for (std::size_t const cores : core_counts)
        {
            for (std::size_t const cpc :
                std::initializer_list<std::size_t>{1, 2, 4, 8})
            {
                if (cores == 1 && cpc != 1)
                    continue;
                std::size_t const chunks = cores * cpc;
                std::size_t const chunk =
                    cores == 1 ? elements : (elements + chunks - 1) / chunks;
                double const us = time_loop(cfg, exec, workers, name,
                    base.with(fixed_shape{cores, chunk}), where, d, buffer);
                std::cout << name << ',' << elements << ','
                          << placement_name(where) << ",fixed," << cores << ','
                          << chunk << ',' << us << std::endl;
                if (us < result.best_us)
                {
                    result.best_us = us;
                    result.best_cores = cores;
                    result.best_chunk = chunk;
                }
            }
        }
        return result;
    }

    void print_summary(std::vector<case_result> const& results)
    {
        std::cout << "\nloop,elements,placement,iteration_ps,chosen_cores,"
                     "chosen_chunk,chosen_us,best_cores,best_chunk,best_us,"
                     "chosen_over_best"
                  << std::endl;
        std::map<std::string, std::pair<double, std::size_t>> by_placement,
            by_loop;
        for (case_result const& r : results)
        {
            double const ratio = r.chosen_us / r.best_us;
            std::cout << r.loop << ',' << r.elements << ','
                      << placement_name(r.where) << ',' << r.chosen.iteration_ps
                      << ',' << r.chosen.cores << ',' << r.chosen.chunk_size
                      << ',' << r.chosen_us << ',' << r.best_cores << ','
                      << r.best_chunk << ',' << r.best_us << ',' << ratio
                      << std::endl;
            auto& p = by_placement[placement_name(r.where)];
            p.first += std::log(ratio);
            ++p.second;
            auto& l = by_loop[r.loop];
            l.first += std::log(ratio);
            ++l.second;
        }

        std::cout << "\ngeomean of chosen over best" << std::endl;
        for (auto const& [key, value] : by_placement)
            std::cout << key << ','
                      << std::exp(
                             value.first / static_cast<double>(value.second))
                      << std::endl;
        for (auto const& [key, value] : by_loop)
            std::cout << key << ','
                      << std::exp(
                             value.first / static_cast<double>(value.second))
                      << std::endl;
    }

    std::vector<std::string> split(std::string const& list)
    {
        std::vector<std::string> items;
        std::stringstream in(list);
        std::string item;
        while (std::getline(in, item, ','))
        {
            if (!item.empty())
                items.push_back(item);
        }
        return items;
    }

    template <typename Exec>
    void run_all(config const& cfg, Exec& exec,
        std::vector<std::string> const& loops,
        std::vector<std::size_t> const& sizes,
        std::vector<placement> const& placements)
    {
        std::size_t const workers = hpx::get_num_worker_threads();
        bool const any_cold = std::find(placements.begin(), placements.end(),
                                  placement::cold) != placements.end();
        std::vector<char> buffer(any_cold ? cfg.evict_bytes : 0);

        std::cout << "loop,elements,placement,shape,cores,chunk,median_us"
                  << std::endl;
        std::vector<case_result> results;
        for (std::size_t const elements : sizes)
        {
            data d(elements);
            for (std::string const& name : loops)
            {
                for (placement const where : placements)
                {
                    results.push_back(run_case(
                        cfg, exec, workers, name, elements, where, d, buffer));
                }
            }
        }
        print_summary(results);
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map& vm)
{
    config const cfg{
        std::chrono::milliseconds(vm["time_per_case_ms"].as<int>()),
        vm["min_reps"].as<std::size_t>(), vm["max_reps"].as<std::size_t>(),
        vm["evict_mb"].as<std::size_t>() << 20};

    std::vector<std::string> const loops = split(vm["loops"].as<std::string>());
    std::vector<std::size_t> sizes;
    for (std::string const& s : split(vm["elements"].as<std::string>()))
        sizes.push_back(static_cast<std::size_t>(std::stoull(s)));
    std::vector<placement> placements;
    for (std::string const& s : split(vm["placements"].as<std::string>()))
    {
        placements.push_back(s == "warm" ? placement::warm :
                s == "foreign"           ? placement::foreign :
                                           placement::cold);
    }

    // the fork-join executors with the static schedule, so that the same
    // shape puts the same chunks on the same workers
    std::string const executor = vm["executor"].as<std::string>();
    if (executor == "parallel")
    {
        hpx::execution::parallel_executor exec;
        run_all(cfg, exec, loops, sizes, placements);
    }
    else if (executor == "suspending_fork_join")
    {
        ex::suspending_fork_join_executor exec(
            hpx::threads::thread_priority::default_,
            hpx::threads::thread_stacksize::small_,
            ex::suspending_fork_join_executor::loop_schedule::static_);
        run_all(cfg, exec, loops, sizes, placements);
    }
    else if (executor == "fork_join")
    {
        ex::fork_join_executor exec(hpx::threads::thread_priority::bound,
            hpx::threads::thread_stacksize::small_,
            ex::fork_join_executor::loop_schedule::static_);
        run_all(cfg, exec, loops, sizes, placements);
    }
    else
    {
        std::cerr << "unknown executor: " << executor << std::endl;
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("loops", value<std::string>()->default_value(
            "copy,transform_plus,reduce_plus,equal,equal_char,find,"
            "minmax_element,merge"),
            "the loops to run, separated by commas")
        ("elements", value<std::string>()->default_value(
            "16384,65536,262144,1048576,4194304"),
            "the numbers of elements, separated by commas")
        ("placements", value<std::string>()->default_value("warm,foreign,cold"),
            "the placements of the data, separated by commas")
        ("executor", value<std::string>()->default_value("fork_join"),
            "fork_join, suspending_fork_join or parallel (default: fork_join)")
        ("time_per_case_ms", value<int>()->default_value(100),
            "time to repeat each loop and shape for, in milliseconds "
            "(default: 100)")
        ("min_reps", value<std::size_t>()->default_value(5),
            "minimal number of repetitions (default: 5)")
        ("max_reps", value<std::size_t>()->default_value(200),
            "maximal number of repetitions (default: 200)")
        ("evict_mb", value<std::size_t>()->default_value(256),
            "the buffer written to evict the caches for the cold placement, "
            "in megabytes (default: 256)")
        ;
    // clang-format on

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
