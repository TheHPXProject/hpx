//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/threading_base/scheduler_tuning.hpp>

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::threads::policies {

    scheduler_tuning_parameters::scheduler_tuning_parameters(
        std::size_t const num_workers)
      : values_(num_workers)
    {
        infos_.reserve(max_parameters);
    }

    std::size_t scheduler_tuning_parameters::add_setting(
        tuning_parameter_info const& info)
    {
        if (infos_.size() == max_parameters)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "scheduler_tuning_parameters::add",
                "too many tunable settings (at most {}), can't add {}",
                max_parameters, info.name);
        }
        if (find(info.name) != npos)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "scheduler_tuning_parameters::add",
                "the tunable setting {} was already added", info.name);
        }

        std::size_t const id = infos_.size();
        infos_.push_back(info);
        return id;
    }

    std::size_t scheduler_tuning_parameters::find(
        std::string_view const name) const noexcept
    {
        auto const it = std::find_if(infos_.begin(), infos_.end(),
            [&](tuning_parameter_info const& info) {
                return name == info.name;
            });
        return it != infos_.end() ?
            static_cast<std::size_t>(it - infos_.begin()) :
            npos;
    }

    scheduler_tuner::~scheduler_tuner() = default;

    ///////////////////////////////////////////////////////////////////////////
    recording_tuner::recording_tuner(std::ostream& out,
        std::chrono::milliseconds const period, bool const totals,
        std::string pool_name)
      : out_(out)
      , period_(period)
      , totals_(totals)
      , pool_name_(HPX_MOVE(pool_name))
      , start_(std::chrono::steady_clock::now())
    {
    }

    void recording_tuner::attach(scheduler_tuning_parameters&) {}

    std::chrono::milliseconds recording_tuner::update_period() const
    {
        return period_;
    }

    void recording_tuner::update(scheduler_tuning_counters const& counters,
        scheduler_tuning_parameters& parameters)
    {
        std::size_t const num_workers = counters.num_workers();
        if (last_.empty())
        {
            write_header(parameters);
            // the earlier counts, per worker and for external
            last_.assign(
                (num_workers + 1) * scheduler_tuning_counters::size, 0);
            totals_since_start_.assign(last_.size(), 0);
        }

        auto const ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_)
                            .count();

        for (std::size_t w = 0; w <= num_workers; ++w)
        {
            bool const external = w == num_workers;
            std::size_t const worker =
                external ? scheduler_tuning_counters::external : w;

            out_ << ms << ',' << pool_name_ << ',';
            if (external)
                out_ << "external";
            else
                out_ << w;

            std::size_t const first = w * scheduler_tuning_counters::size;
            for (std::size_t i = 0; i != scheduler_tuning_counters::size; ++i)
            {
                // the difference is right also if the counter wrapped around
                std::uint32_t const value =
                    counters.get(worker, static_cast<scheduler_counter>(i));
                std::uint32_t const delta = value - last_[first + i];
                last_[first + i] = value;
                totals_since_start_[first + i] += delta;

                out_ << ',';
                if (totals_)
                    out_ << totals_since_start_[first + i];
                else
                    out_ << delta;
            }

            // the settings this worker decided with (none for the threads
            // from outside the pool)
            for (std::size_t id = 0; id != parameters.size(); ++id)
            {
                out_ << ',';
                if (!external)
                {
                    write_value(parameters, w, id);
                }
            }
            out_ << '\n';
        }
        out_.flush();
    }

    void recording_tuner::write_value(
        scheduler_tuning_parameters const& parameters, std::size_t const worker,
        std::size_t const id)
    {
        // each setting as its type; a setting for the whole pool has the same
        // value for every worker
        if (parameters.info(id).type == tuning_parameter_type::real)
        {
            out_ << parameters.get<float>(worker, id);
        }
        else
        {
            out_ << parameters.get<std::uint32_t>(worker, id);
        }
    }

    void recording_tuner::write_header(
        scheduler_tuning_parameters const& parameters)
    {
        out_ << "ms,pool,worker";
        for (std::size_t i = 0; i != scheduler_tuning_counters::size; ++i)
        {
            out_ << ','
                 << get_scheduler_counter_name(
                        static_cast<scheduler_counter>(i));
        }
        for (std::size_t id = 0; id != parameters.size(); ++id)
        {
            out_ << ',' << parameters.info(id).name;
        }
        out_ << '\n';
    }

    ///////////////////////////////////////////////////////////////////////////
    file_recording_tuner::file_recording_tuner(std::string const& file_name,
        std::chrono::milliseconds const period, bool const totals,
        std::string pool_name)
      : file_(file_name, std::ios::out | std::ios::trunc)
      , recorder_(file_, period, totals, HPX_MOVE(pool_name))
    {
        // fail when the scheduler is created rather than on the first update
        if (!file_)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "file_recording_tuner",
                "can't open {} (see hpx.scheduler_tuner.file)", file_name);
        }
    }

    void file_recording_tuner::attach(scheduler_tuning_parameters& parameters)
    {
        recorder_.attach(parameters);
    }

    std::chrono::milliseconds file_recording_tuner::update_period() const
    {
        return recorder_.update_period();
    }

    void file_recording_tuner::update(scheduler_tuning_counters const& counters,
        scheduler_tuning_parameters& parameters)
    {
        recorder_.update(counters, parameters);
    }

    namespace {

        // keeps the values the settings were configured with
        struct static_tuner final : scheduler_tuner
        {
            void attach(scheduler_tuning_parameters&) override {}
        };

        std::chrono::milliseconds read_period(std::string const& value)
        {
            long long ms = 0;
            auto const [ptr, ec] =
                std::from_chars(value.data(), value.data() + value.size(), ms);
            if (ec != std::errc() || ptr != value.data() + value.size() ||
                ms <= 0)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "recording_tuner",
                    "invalid hpx.scheduler_tuner.period: {} (expected a "
                    "number of milliseconds greater than 0)",
                    value);
            }
            return std::chrono::milliseconds(ms);
        }

        // The "recording" tuner: a file_recording_tuner,
        // configured with hpx.scheduler_tuner.<key>:
        //   period   ms between rows (100)
        //   file     the CSV file (scheduler-tuning-<pool>.csv, or
        //            scheduler-tuning.csv if the pool isn't known)
        //   columns  deltas: the counts since the last row (default), or
        //            totals: the counts since the start
        std::unique_ptr<scheduler_tuner> make_recording_tuner(
            scheduler_tuner_context const& context)
        {
            // the runtime configuration has the entry, empty by default
            std::string file = context.get_entry("file", "");
            if (file.empty())
            {
                file = context.pool_name.empty() ?
                    std::string("scheduler-tuning.csv") :
                    "scheduler-tuning-" + context.pool_name + ".csv";
            }
            return std::make_unique<file_recording_tuner>(file,
                read_period(context.get_entry("period", "100")),
                context.get_entry("columns", "deltas") == "totals",
                context.pool_name);
        }

        struct tuner_registry
        {
            tuner_registry()
            {
                factories.emplace("static", [](scheduler_tuner_context const&) {
                    return std::make_unique<static_tuner>();
                });
                factories.emplace("recording", &make_recording_tuner);
            }

            std::mutex mtx;
            std::map<std::string, scheduler_tuner_factory, std::less<>>
                factories;
        };

        tuner_registry& get_tuner_registry()
        {
            static tuner_registry registry;
            return registry;
        }
    }    // namespace

    void register_scheduler_tuner(
        std::string const& name, scheduler_tuner_factory factory)
    {
        auto& registry = get_tuner_registry();
        std::lock_guard<std::mutex> l(registry.mtx);
        registry.factories.insert_or_assign(name, HPX_MOVE(factory));
    }

    std::unique_ptr<scheduler_tuner> create_scheduler_tuner(
        std::string const& name, scheduler_tuner_context const& context)
    {
        auto& registry = get_tuner_registry();
        scheduler_tuner_factory factory;
        {
            std::lock_guard<std::mutex> l(registry.mtx);
            auto const it = registry.factories.find(name);
            if (it == registry.factories.end())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "create_scheduler_tuner",
                    "unknown scheduler tuner: {} (see "
                    "hpx.scheduler_tuner)",
                    name);
            }
            factory = it->second;
        }
        return factory(context);
    }
}    // namespace hpx::threads::policies
