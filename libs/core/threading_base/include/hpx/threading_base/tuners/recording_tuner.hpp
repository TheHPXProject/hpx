//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file recording_tuner.hpp
/// \brief The tuners that record what a scheduler counted (recording_tuner,
///        and file_recording_tuner, registered as "recording").

#pragma once

#include <hpx/config.hpp>
#include <hpx/threading_base/scheduler_tuning.hpp>
#include <hpx/threading_base/scheduler_tuning_counters.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ostream>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads::policies {

    /// A tuner that keeps the configured values, as the static tuner does,
    /// and writes what the scheduler counted to a stream every update
    /// period: CSV rows with the time in ms since the tuner was created, the
    /// pool, the worker (a number, or external for the threads from outside
    /// the pool), the counters (see scheduler_counter), and the values of the
    /// settings the worker decided with. It collects what a learning tuner
    /// would see, to design one from real workloads. The stream belongs to
    /// the caller (see file_recording_tuner for one that writes to a file of
    /// its own).
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT recording_tuner final
      : public scheduler_tuner
    {
    public:
        /// Write to out, which has to outlive the tuner's pool (until the
        /// pool has stopped). The tuner writes from one HPX thread at a time
        /// and flushes after each update. The tuners of several pools may run
        /// their updates at the same time, and a row is written with several
        /// insertions, so the caller of tuners that share a stream has to
        /// make sure that their updates (and the header written by the first
        /// one) don't run at the same time: with a string or file stream two
        /// tuners writing at once are a data race, and even a synchronized
        /// stream such as std::cout would interleave their rows. The
        /// "recording" tuner writes to a file of its own (see
        /// file_recording_tuner).
        recording_tuner(std::ostream& out, std::chrono::milliseconds period,
            bool totals = false, std::string pool_name = {});

        void attach(scheduler_tuning_parameters& parameters) override;

        [[nodiscard]] std::chrono::milliseconds update_period() const override;

        /// Write one row per worker and one for external: the counts since
        /// the last update, or since the start if totals was given.
        void update(scheduler_tuning_counters const& counters,
            scheduler_tuning_parameters& parameters) override;

    private:
        void write_header(scheduler_tuning_parameters const& parameters);
        void write_value(scheduler_tuning_parameters const& parameters,
            std::size_t worker, std::size_t id);

        std::ostream& out_;
        std::chrono::milliseconds const period_;
        bool const totals_;
        std::string const pool_name_;
        std::chrono::steady_clock::time_point const start_;
        // the counters at the last update, and the counts since the start
        // (the counters wrap around, these don't)
        std::vector<std::uint32_t> last_;
        std::vector<std::uint64_t> totals_since_start_;
    };

    /// A recording_tuner that writes to a file of its own, which it opens
    /// (and truncates) when it is created. Registered as "recording" (see
    /// hpx.scheduler_tuner.file).
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT file_recording_tuner final
      : public scheduler_tuner
    {
    public:
        /// Throws hpx::exception (bad_parameter) if the file can't be opened.
        file_recording_tuner(std::string const& file_name,
            std::chrono::milliseconds period, bool totals = false,
            std::string pool_name = {});

        // recorder_ writes to file_
        file_recording_tuner(file_recording_tuner const&) = delete;
        file_recording_tuner(file_recording_tuner&&) = delete;
        file_recording_tuner& operator=(file_recording_tuner const&) = delete;
        file_recording_tuner& operator=(file_recording_tuner&&) = delete;

        ~file_recording_tuner() override = default;

        void attach(scheduler_tuning_parameters& parameters) override;

        [[nodiscard]] std::chrono::milliseconds update_period() const override;

        void update(scheduler_tuning_counters const& counters,
            scheduler_tuning_parameters& parameters) override;

    private:
        std::ofstream file_;          // opened before recorder_ is created
        recording_tuner recorder_;    // writes to file_
    };
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
