//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/threading_base/scheduler_tuning.hpp>
#include <hpx/threading_base/scheduler_tuning_counters.hpp>
#include <hpx/threading_base/tuners/recording_tuner.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hpx::threads::policies {

    namespace {

        // A field of a CSV row: as it is, or in double quotes (with its
        // double quotes doubled) if it contains a comma, a double quote or a
        // line break, for instance a pool name of an application.
        void write_csv_field(std::ostream& out, std::string_view const field)
        {
            if (field.find_first_of(",\"\r\n") == std::string_view::npos)
            {
                out << field;
                return;
            }
            out << '"';
            for (char const c : field)
            {
                if (c == '"')
                    out << '"';
                out << c;
            }
            out << '"';
        }
    }    // namespace

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

            out_ << ms << ',';
            write_csv_field(out_, pool_name_);
            out_ << ',';
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
            out_ << ',';
            write_csv_field(out_, parameters.info(id).name);
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
}    // namespace hpx::threads::policies
