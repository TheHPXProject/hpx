//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file scheduler_tuning.hpp
/// \brief The settings a scheduler reads where it makes its decisions, and
///        the interface of the tuners that may change them.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/functional.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads::policies {

    /// Describes a setting a scheduler reads where it makes a decision (for
    /// instance whether to wake up a sleeping worker) and that a tuner may
    /// change while the scheduler runs.
    HPX_CXX_CORE_EXPORT struct tuning_parameter_info
    {
        /// The name of the configuration entry the setting is read from (for
        /// instance "hpx.min_idle_backoff_time"), which also
        /// identifies it for tuners.
        char const* name;

        /// The range a tuner may move the setting in.
        double min_value;
        double max_value;

        /// Whether each worker has its own value (read where that worker
        /// makes a decision), or one value holds for the whole pool (read
        /// where no worker index is known, for instance when a thread is
        /// created from outside the pool).
        bool per_worker;
    };

    /// The current values of the tunable settings of a scheduler, one set
    /// per worker. A scheduler registers its settings while it is
    /// constructed (see add) and reads them where it makes its decisions
    /// (see get). A tuner (see scheduler_tuner) may change them (see set)
    /// at any time; the scheduler sees a change at its next decision.
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT scheduler_tuning_parameters
    {
    public:
        /// The number of settings a scheduler can register.
        static constexpr std::size_t max_parameters = 16;

        /// Returned by find for an unknown setting.
        static constexpr std::size_t npos = static_cast<std::size_t>(-1);

        explicit scheduler_tuning_parameters(std::size_t num_workers);

        // owned by the scheduler, which hands out references to it
        scheduler_tuning_parameters(
            scheduler_tuning_parameters const&) = delete;
        scheduler_tuning_parameters(scheduler_tuning_parameters&&) = delete;
        scheduler_tuning_parameters& operator=(
            scheduler_tuning_parameters const&) = delete;
        scheduler_tuning_parameters& operator=(
            scheduler_tuning_parameters&&) = delete;

        /// Register a setting with its initial value (usually the value of
        /// its configuration entry) for all workers. Returns the index the
        /// scheduler reads it with. Throws if more than max_parameters
        /// settings are registered or if a setting is registered twice.
        std::size_t add(tuning_parameter_info const& info, double value);

        /// The number of registered settings.
        [[nodiscard]] std::size_t size() const noexcept
        {
            return infos_.size();
        }

        /// The number of workers the settings are kept for.
        [[nodiscard]] std::size_t num_workers() const noexcept
        {
            return values_.size();
        }

        /// The description of the given setting.
        [[nodiscard]] tuning_parameter_info const& info(
            std::size_t id) const noexcept
        {
            HPX_ASSERT(id < infos_.size());
            return infos_[id];
        }

        /// The index of the setting with the given name, or npos.
        [[nodiscard]] std::size_t find(std::string_view name) const noexcept;

        /// The current value of the given setting for the given worker.
        [[nodiscard]] double get(
            std::size_t worker, std::size_t id) const noexcept
        {
            HPX_ASSERT(worker < values_.size() && id < infos_.size());
            return values_[worker].data_[id].load(std::memory_order_relaxed);
        }

        /// The current value of the given setting that holds for the whole
        /// pool (see tuning_parameter_info::per_worker).
        [[nodiscard]] double get(std::size_t id) const noexcept
        {
            HPX_ASSERT(id < infos_.size() && !infos_[id].per_worker);
            // set_all keeps the values of all workers equal
            return values_[0].data_[id].load(std::memory_order_relaxed);
        }

        /// Change the given setting for the given worker. The value is
        /// clamped to the range of the setting. Settings that hold for the
        /// whole pool can only be changed with set_all.
        void set(std::size_t worker, std::size_t id, double value) noexcept;

        /// Change the given setting for all workers.
        void set_all(std::size_t id, double value) noexcept;

    private:
        std::vector<tuning_parameter_info> infos_;

        // the values of a worker are read by that worker where it makes its
        // decisions and written only by a tuner, keep them on their own lines
        using worker_values = std::array<std::atomic<double>, max_parameters>;
        std::vector<util::aligned_line_data<worker_values>> values_;
    };

    /// A tuner decides the values of the tunable settings of a scheduler
    /// (see scheduler_tuning_parameters). It is selected per thread pool by
    /// name with the configuration entry hpx.scheduler_tuner (see
    /// register_scheduler_tuner). The default tuner, "static", keeps the
    /// values the settings were configured with.
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT scheduler_tuner
    {
    public:
        scheduler_tuner() = default;
        scheduler_tuner(scheduler_tuner const&) = delete;
        scheduler_tuner(scheduler_tuner&&) = delete;
        scheduler_tuner& operator=(scheduler_tuner const&) = delete;
        scheduler_tuner& operator=(scheduler_tuner&&) = delete;

        virtual ~scheduler_tuner();

        /// Called once the scheduler has registered its settings, before
        /// its workers start.
        virtual void attach(scheduler_tuning_parameters& parameters) = 0;
    };

    /// Creates a tuner.
    HPX_CXX_CORE_EXPORT using scheduler_tuner_factory =
        hpx::function<std::unique_ptr<scheduler_tuner>()>;

    /// Make a tuner available under the given name, for use with the
    /// configuration entry hpx.scheduler_tuner. Register tuners before the
    /// runtime is started. A tuner registered under an existing name
    /// replaces the earlier one.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void register_scheduler_tuner(
        std::string const& name, scheduler_tuner_factory factory);

    /// Create the tuner registered under the given name ("static" is always
    /// available). Throws hpx::exception (bad_parameter) for an unknown name.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::unique_ptr<scheduler_tuner>
    create_scheduler_tuner(std::string const& name);
}    // namespace hpx::threads::policies

#include <hpx/config/warnings_suffix.hpp>
