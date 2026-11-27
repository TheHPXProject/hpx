//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/tuning/chunk_costs.hpp>
#include <hpx/tuning/ini_file.hpp>
#include <hpx/tuning/statistics.hpp>
#include <hpx/tuning/tuned_setting.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hpx::tuning {

    std::vector<std::string> chunk_executors()
    {
        return {"parallel", "fork_join", "suspending_fork_join",
            "shared_suspending_fork_join"};
    }

    namespace {

        // The regions of one executor for each number of indices: a few to
        // warm up, then repeats samples of options.regions regions each.
        template <typename Executor>
        void measure_executor(Executor const& exec, std::string const& name,
            std::size_t const pool_size, chunk_cost_options const& options,
            std::vector<region_time>& times)
        {
            std::size_t const largest = pool_size *
                (options.multiples.empty() ?
                        1 :
                        *std::max_element(options.multiples.begin(),
                            options.multiples.end()));
            std::vector<std::size_t> data(largest, 0);
            auto body = [&data](std::size_t const i) { data[i] += i; };

            for (std::size_t const multiple : options.multiples)
            {
                std::size_t const indices = multiple * pool_size;
                hpx::util::counting_shape const shape(std::size_t(0), indices);

                for (std::size_t i = 0; i != 10; ++i)
                {
                    hpx::parallel::execution::bulk_sync_execute(
                        exec, body, shape);
                }

                std::vector<double> samples;
                samples.reserve(options.repeats);
                for (std::size_t r = 0; r != options.repeats; ++r)
                {
                    auto const start = std::chrono::steady_clock::now();
                    for (std::size_t i = 0; i != options.regions; ++i)
                    {
                        hpx::parallel::execution::bulk_sync_execute(
                            exec, body, shape);
                    }
                    std::chrono::duration<double, std::micro> const elapsed =
                        std::chrono::steady_clock::now() - start;
                    samples.push_back(elapsed.count() /
                        static_cast<double>(
                            (std::max) (options.regions, std::size_t(1))));
                }
                times.push_back(region_time{
                    name, pool_size, indices, summarize(HPX_MOVE(samples))});
            }
        }

        // a value with about three significant digits: 2.50, 0.593, 15.6
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
    }    // namespace

    std::vector<region_time> measure_region_times(
        chunk_cost_options const& options)
    {
        namespace ex = hpx::execution::experimental;

        std::size_t const pool_size =
            ex::processing_units_count(hpx::execution::parallel_executor());
        std::vector<region_time> times;

        // one executor after the other, each destroyed before the next is
        // created, so that the waiting workers of one don't disturb the
        // others (the shared executor's workers stay until the runtime
        // stops, so it comes last)
        measure_executor(hpx::execution::parallel_executor(), "parallel",
            pool_size, options, times);
        {
            ex::fork_join_executor const exec;
            measure_executor(exec, "fork_join", pool_size, options, times);
        }
        {
            ex::suspending_fork_join_executor const exec;
            measure_executor(
                exec, "suspending_fork_join", pool_size, options, times);
        }
        measure_executor(ex::shared_suspending_fork_join_executor(),
            "shared_suspending_fork_join", pool_size, options, times);
        return times;
    }

    std::string to_line(region_time const& time)
    {
        std::ostringstream os;
        os << std::setprecision(17) << time.executor << ' ' << time.pool_size
           << ' ' << time.indices << ' ' << time.time_us.count << ' '
           << time.time_us.min << ' ' << time.time_us.q1 << ' '
           << time.time_us.median << ' ' << time.time_us.q3 << ' '
           << time.time_us.p90 << ' ' << time.time_us.max;
        return os.str();
    }

    std::optional<region_time> from_line(std::string const& line)
    {
        std::istringstream is(line);
        region_time time;
        if (!(is >> time.executor >> time.pool_size >> time.indices >>
                time.time_us.count >> time.time_us.min >> time.time_us.q1 >>
                time.time_us.median >> time.time_us.q3 >> time.time_us.p90 >>
                time.time_us.max))
        {
            return std::nullopt;
        }
        std::string rest;
        if (is >> rest)
            return std::nullopt;
        return time;
    }

    std::vector<chunk_cost> fit_chunk_costs(
        std::vector<region_time> const& times)
    {
        // the region times of each executor and pool size, in the order of
        // their first appearance
        std::vector<std::pair<std::string, std::size_t>> order;
        std::map<std::pair<std::string, std::size_t>,
            std::vector<region_time const*>>
            groups;
        for (region_time const& t : times)
        {
            auto const key = std::make_pair(t.executor, t.pool_size);
            auto& group = groups[key];
            if (group.empty())
                order.push_back(key);
            group.push_back(&t);
        }

        std::vector<chunk_cost> costs;
        for (auto const& key : order)
        {
            auto group = groups[key];
            std::size_t const n = key.second;
            if (group.size() < 2 || n == 0)
                continue;
            std::sort(group.begin(), group.end(),
                [](region_time const* a, region_time const* b) {
                    return a->indices < b->indices;
                });

            // t = r + k * o, k = indices / n, by least squares
            double mean_k = 0.0;
            double mean_t = 0.0;
            for (region_time const* t : group)
            {
                mean_k +=
                    static_cast<double>(t->indices) / static_cast<double>(n);
                mean_t += t->time_us.median;
            }
            mean_k /= static_cast<double>(group.size());
            mean_t /= static_cast<double>(group.size());

            double sxx = 0.0;
            double sxy = 0.0;
            for (region_time const* t : group)
            {
                double const k =
                    static_cast<double>(t->indices) / static_cast<double>(n) -
                    mean_k;
                sxx += k * k;
                sxy += k * (t->time_us.median - mean_t);
            }

            chunk_cost cost;
            cost.executor = key.first;
            cost.pool_size = n;
            cost.chunk_us = sxx > 0.0 ? sxy / sxx : 0.0;
            cost.region_us = mean_t - cost.chunk_us * mean_k;

            // the median for as many indices as workers, else for the
            // fewest indices measured
            cost.time_at_pool_size_us = group.front()->time_us.median;
            for (region_time const* t : group)
            {
                double const k =
                    static_cast<double>(t->indices) / static_cast<double>(n);
                double const fitted = cost.region_us + k * cost.chunk_us;
                cost.residual_us = (std::max) (cost.residual_us,
                    t->time_us.median > fitted ? t->time_us.median - fitted :
                                                 fitted - t->time_us.median);
                if (t->indices == n)
                    cost.time_at_pool_size_us = t->time_us.median;
                if (t->time_us.median > 0.0)
                {
                    cost.spread = (std::max) (cost.spread,
                        (t->time_us.q3 - t->time_us.q1) / t->time_us.median);
                }
            }
            costs.push_back(cost);
        }
        return costs;
    }

    char const* to_string(chunk_rule const rule) noexcept
    {
        return rule == chunk_rule::fitted ? "fitted" : "ratio";
    }

    std::optional<chunk_rule> chunk_rule_from_string(
        std::string const& name) noexcept
    {
        if (name == "fitted")
            return chunk_rule::fitted;
        if (name == "ratio")
            return chunk_rule::ratio;
        return std::nullopt;
    }

    std::vector<ini_section> chunk_sections(
        std::vector<chunk_cost> const& costs,
        double const default_min_chunk_time_us, chunk_rule const rule)
    {
        // parallel_executor's costs for each pool size
        std::map<std::size_t, chunk_cost const*> reference;
        for (chunk_cost const& c : costs)
        {
            if (c.executor == "parallel")
                reference[c.pool_size] = &c;
        }

        struct value
        {
            std::size_t pool_size;
            double min_chunk_time_us;
            std::string reason;
        };
        std::vector<std::pair<std::string, std::vector<value>>> executors;

        for (chunk_cost const& c : costs)
        {
            auto const ref = reference.find(c.pool_size);
            if (ref == reference.end())
                continue;
            chunk_cost const& p = *ref->second;
            double const n = static_cast<double>(c.pool_size);
            std::string const workers =
                " at " + std::to_string(c.pool_size) + " workers";

            value v{c.pool_size, default_min_chunk_time_us, ""};
            if (rule == chunk_rule::fitted)
            {
                double const p_cost = p.chunk_us + p.region_us / n;
                double const f =
                    p_cost > 0.0 ? default_min_chunk_time_us / p_cost : 0.0;
                if (c.executor != "parallel")
                    v.min_chunk_time_us = f * (c.chunk_us + c.region_us / n);
                v.reason = "region " + number(c.region_us) + " us, chunk " +
                    number(c.chunk_us) + " us, f " + number(f) + workers +
                    " (fitted; residual " + number(c.residual_us) +
                    " us, spread " + number(100.0 * c.spread) + "%)";
            }
            else
            {
                if (c.executor != "parallel" && p.time_at_pool_size_us > 0.0)
                {
                    v.min_chunk_time_us = default_min_chunk_time_us *
                        c.time_at_pool_size_us / p.time_at_pool_size_us;
                }
                v.reason = "region of " + std::to_string(c.pool_size) +
                    " indices " + number(c.time_at_pool_size_us) +
                    " us against " + number(p.time_at_pool_size_us) +
                    " us of parallel_executor" + workers + " (ratio; spread " +
                    number(100.0 * c.spread) + "%)";
            }

            auto it = std::find_if(executors.begin(), executors.end(),
                [&](auto const& e) { return e.first == c.executor; });
            if (it == executors.end())
            {
                executors.emplace_back(c.executor, std::vector<value>{});
                it = std::prev(executors.end());
            }
            it->second.push_back(HPX_MOVE(v));
        }

        std::vector<ini_section> sections;
        for (auto& [executor, values] : executors)
        {
            bool const is_default = executor == "parallel";
            std::string const name = "hpx.task_size_chunk_size." + executor;

            std::sort(values.begin(), values.end(),
                [](value const& a, value const& b) {
                    return a.pool_size < b.pool_size;
                });

            // for the pool sizes that weren't measured: the largest one's
            value const& largest = values.back();
            sections.push_back(ini_section{name,
                {tuned_setting{name + ".min_chunk_time_us",
                    number(largest.min_chunk_time_us),
                    "the value of " + std::to_string(largest.pool_size) +
                        " workers, for the pool sizes not measured",
                    is_default}}});

            for (value const& v : values)
            {
                std::string const size_name =
                    name + "." + std::to_string(v.pool_size);
                sections.push_back(ini_section{size_name,
                    {tuned_setting{size_name + ".min_chunk_time_us",
                        number(v.min_chunk_time_us), v.reason, is_default}}});
            }
        }
        return sections;
    }
}    // namespace hpx::tuning
