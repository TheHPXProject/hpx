//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file recording_tuner.hpp
/// \brief Recording what a scheduler counted and what its tuner decided: the
///        mixins recorded and recorded_to_file, and the recording tuners
///        built with them (recording_tuner, and file_recording_tuner,
///        registered as "recording").

#pragma once

#include <hpx/config.hpp>
#include <hpx/threading_base/scheduler_tuning.hpp>
#include <hpx/threading_base/scheduler_tuning_counters.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads::policies {

    /// Writes what a scheduler counted, and the values of its settings, to a
    /// stream: CSV rows with the time in ms since the recorder was created,
    /// the pool, the worker (a number, or external for the threads from
    /// outside the pool), the counters (see scheduler_counter), and the
    /// values of the settings the worker decided with. The stream belongs to
    /// the caller and has to outlive the recorder.
    ///
    /// A recorder writes from one thread at a time and flushes after each
    /// record. The tuners of several pools may run their updates at the same
    /// time, and a row is written with several insertions, so the caller of
    /// recorders that share a stream has to make sure that they (and the
    /// header written by the first one) don't write at the same time: with a
    /// string or file stream two recorders writing at once are a data race,
    /// and even a synchronized stream such as std::cout would interleave
    /// their rows. The "recording" tuner writes to a file of its own.
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT tuner_recorder
    {
    public:
        explicit tuner_recorder(
            std::ostream& out, bool totals = false, std::string pool_name = {});

        /// Write one row per worker and one for external: the counts since
        /// the last record, or since the start if totals was given, and the
        /// current values of the settings. The first record writes the
        /// header first.
        void record(scheduler_tuning_counters const& counters,
            scheduler_tuning_parameters const& parameters);

    private:
        void write_header(scheduler_tuning_parameters const& parameters);
        void write_value(scheduler_tuning_parameters const& parameters,
            std::size_t worker, std::size_t id);

        std::ostream& out_;
        bool const totals_;
        std::string const pool_name_;
        std::chrono::steady_clock::time_point const start_;
        // the counters at the last record, and the counts since the start
        // (the counters wrap around, these don't)
        std::vector<std::uint32_t> last_;
        std::vector<std::uint64_t> totals_since_start_;
    };

    /// The file a recorder writes to (see recorded_to_file), opened (and
    /// truncated) when it is created.
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT recording_file
    {
    public:
        /// Throws hpx::exception (bad_parameter) if the file can't be opened.
        explicit recording_file(std::string const& file_name);

        [[nodiscard]] std::ostream& stream() noexcept
        {
            return file_;
        }

    private:
        std::ofstream file_;
    };

    /// The tuner Base, and a recorder (see tuner_recorder) that writes what
    /// the scheduler counted and the settings after every update of Base:
    /// the settings as Base decided them. The update period is that of Base,
    /// or the given one if Base has none (as static_tuner, the default).
    ///
    /// recorded is a mixin: it forwards attach, update_period and update to
    /// Base with qualified, so static, calls, and adds the recording. Layers
    /// of mixins on a tuner make one type with one virtual entry point, the
    /// call of the scheduler into the outermost layer (see
    /// make_scheduler_tuner).
    HPX_CXX_CORE_EXPORT template <typename Base = static_tuner>
    class recorded : public Base
    {
    public:
        /// Write to out (see tuner_recorder); the arguments after pool_name
        /// construct Base.
        template <typename... Ts>
        recorded(std::ostream& out, std::chrono::milliseconds const period,
            bool const totals, std::string pool_name, Ts&&... ts)
          : Base(HPX_FORWARD(Ts, ts)...)
          , recorder_(out, totals, HPX_MOVE(pool_name))
          , period_(period)
        {
        }

        void attach(scheduler_tuning_parameters& parameters) override
        {
            Base::attach(parameters);
        }

        [[nodiscard]] std::chrono::milliseconds update_period() const override
        {
            auto const period = Base::update_period();
            return period.count() > 0 ? period : period_;
        }

        void update(scheduler_tuning_counters const& counters,
            scheduler_tuning_parameters& parameters) override
        {
            Base::update(counters, parameters);
            recorder_.record(counters, parameters);
        }

        [[nodiscard]] bool records() const noexcept override
        {
            return true;
        }

    private:
        tuner_recorder recorder_;
        std::chrono::milliseconds const period_;
    };

    /// recorded, writing to a file of its own (see recording_file).
    HPX_CXX_CORE_EXPORT template <typename Base = static_tuner>
    class recorded_to_file
      : private recording_file
      , public recorded<Base>
    {
    public:
        /// Throws hpx::exception (bad_parameter) if the file can't be opened.
        /// The arguments after pool_name construct Base.
        template <typename... Ts>
        recorded_to_file(std::string const& file_name,
            std::chrono::milliseconds const period, bool const totals,
            std::string pool_name, Ts&&... ts)
          : recording_file(file_name)
          , recorded<Base>(recording_file::stream(), period, totals,
                HPX_MOVE(pool_name), HPX_FORWARD(Ts, ts)...)
        {
        }
    };

    /// A tuner that keeps the configured values, as the static tuner does,
    /// and writes what the scheduler counted to a stream every update period
    /// (see tuner_recorder). It collects what a learning tuner would see, to
    /// design one from real workloads.
    HPX_CXX_CORE_EXPORT using recording_tuner = recorded<>;

    /// A recording_tuner that writes to a file of its own. Registered as
    /// "recording" (see hpx.scheduler_tuner.file).
    HPX_CXX_CORE_EXPORT using file_recording_tuner = recorded_to_file<>;

    /// How hpx.scheduler_tuner.record asks a tuner to record: the file, the
    /// update period for a tuner without one (hpx.scheduler_tuner.period),
    /// and whether to write the counts since the start
    /// (hpx.scheduler_tuner.columns=totals).
    HPX_CXX_CORE_EXPORT struct tuner_recording_options
    {
        std::string file;
        std::chrono::milliseconds period;
        bool totals;
    };

    /// The recording the given context asks for with
    /// hpx.scheduler_tuner.record=<prefix> (the file is <prefix>-<pool>.csv,
    /// or <prefix>.csv if the pool isn't known), none if the entry is empty.
    /// Throws hpx::exception (bad_parameter) for an invalid period.
    HPX_CXX_CORE_EXPORT
    [[nodiscard]] HPX_CORE_EXPORT std::optional<tuner_recording_options>
    get_tuner_recording_options(scheduler_tuner_context const& context);

    /// Create a tuner of type Tuner (from the arguments after context) for
    /// the factory of a registered tuner (see register_scheduler_tuner): a
    /// recorded_to_file<Tuner> if the context asks for recording
    /// (hpx.scheduler_tuner.record), a Tuner otherwise. Either way the
    /// scheduler's call into the tuner is its only virtual call, the
    /// recording calls Tuner statically.
    HPX_CXX_CORE_EXPORT template <typename Tuner, typename... Ts>
    [[nodiscard]] std::unique_ptr<scheduler_tuner> make_scheduler_tuner(
        scheduler_tuner_context const& context, Ts&&... ts)
    {
        if (auto const options = get_tuner_recording_options(context))
        {
            return std::make_unique<recorded_to_file<Tuner>>(options->file,
                options->period, options->totals, context.pool_name,
                HPX_FORWARD(Ts, ts)...);
        }
        return std::make_unique<Tuner>(HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
