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
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads::policies {

    /// What the value of a setting is. A scheduler reads each setting as
    /// its type (see scheduler_tuning_parameters::get), so that its
    /// decisions don't convert values.
    HPX_CXX_CORE_EXPORT enum class tuning_parameter_type : std::uint8_t {
        /// a switch, count or size, up to 2^32 - 1, kept as a
        /// std::uint32_t
        integer = 0,

        /// a value with a fraction (for instance a time in microseconds),
        /// kept as a float
        real = 1
    };

    /// The types the value of a setting is read and changed as.
    HPX_CXX_CORE_EXPORT template <typename T>
    concept tuning_value_type =
        std::same_as<T, std::uint32_t> || std::same_as<T, float>;

    /// The type of the settings that keep values of the given type.
    HPX_CXX_CORE_EXPORT template <tuning_value_type T>
    inline constexpr tuning_parameter_type tuning_parameter_type_of =
        std::same_as<T, float> ? tuning_parameter_type::real :
                                 tuning_parameter_type::integer;

    /// A value of a setting of either type, in the one word a setting keeps
    /// its value in (see scheduler_tuning_parameters).
    HPX_CXX_CORE_EXPORT class tuning_value
    {
    public:
        constexpr tuning_value() noexcept = default;

        explicit constexpr tuning_value(std::uint32_t const value) noexcept
          : bits_(value)
        {
        }

        explicit constexpr tuning_value(float const value) noexcept
          : bits_(hpx::bit_cast<std::uint32_t>(value))
        {
        }

        /// The value as the type it was made from.
        template <tuning_value_type T>
        [[nodiscard]] constexpr T as() const noexcept
        {
            return hpx::bit_cast<T>(bits_);
        }

    private:
        std::uint32_t bits_ = 0;
    };

    /// Describes a setting a scheduler reads where it makes a decision (for
    /// instance whether to wake up a sleeping worker) and that a tuner may
    /// change while the scheduler runs (see scheduler_tuning_parameters::add).
    HPX_CXX_CORE_EXPORT struct tuning_parameter_info
    {
        /// The name of the configuration entry the setting is read from (for
        /// instance "hpx.min_idle_backoff_time"), which also
        /// identifies it for tuners.
        char const* name;

        /// The range a tuner may move the setting in, of the type of the
        /// setting.
        tuning_value min_value;
        tuning_value max_value;

        /// Whether each worker has its own value (read where that worker
        /// makes a decision), or one value holds for the whole pool (read
        /// where no worker index is known, for instance when a thread is
        /// created from outside the pool).
        bool per_worker;

        /// What its value is.
        tuning_parameter_type type;

        /// The given value of the type of the setting within its range.
        template <tuning_value_type T>
        [[nodiscard]] constexpr T clamped(T const value) const noexcept
        {
            HPX_ASSERT(type == tuning_parameter_type_of<T>);
            if constexpr (std::same_as<T, float>)
            {
                if (std::isnan(value))
                {
                    // Fall back to min_value if value is NaN (e.g. from
                    // division by zero in a dynamic tuner), as default_value
                    // is not stored in tuning_parameter_info and min_value is a
                    // safe conservative fallback.
                    return min_value.as<float>();
                }
            }
            return std::clamp(value, min_value.as<T>(), max_value.as<T>());
        }
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

        /// Register the setting with the given name (the name of its
        /// configuration entry), the range a tuner may move it in, and its
        /// initial value for all workers (usually the value of its
        /// configuration entry). The range and the value are of the type of
        /// the setting (see tuning_parameter_type_of). Returns the index the
        /// scheduler reads it with. Throws if more than max_parameters
        /// settings are registered or if a setting is registered twice.
        template <tuning_value_type T>
        std::size_t add(char const* name, T const min_value, T const max_value,
            bool const per_worker, T const value)
        {
            HPX_ASSERT(min_value <= max_value);
            std::size_t const id = add_setting(
                {name, tuning_value(min_value), tuning_value(max_value),
                    per_worker, tuning_parameter_type_of<T>});
            set_all(id, value);
            return id;
        }

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

        /// The current value of the given setting for the given worker, as
        /// the type of the setting. This is what a scheduler reads where it
        /// decides.
        template <tuning_value_type T>
        [[nodiscard]] T get(std::size_t worker, std::size_t id) const noexcept
        {
            HPX_ASSERT(worker < values_.size() && id < infos_.size() &&
                infos_[id].type == tuning_parameter_type_of<T>);
            return values_[worker]
                .data_[id]
                .load(std::memory_order_relaxed)
                .as<T>();
        }

        /// The current value of the given setting that holds for the whole
        /// pool (see tuning_parameter_info::per_worker), as the type of the
        /// setting.
        template <tuning_value_type T>
        [[nodiscard]] T get(std::size_t id) const noexcept
        {
            HPX_ASSERT(id < infos_.size() && !infos_[id].per_worker);
            // set_all keeps the values of all workers equal
            return get<T>(0, id);
        }

        /// Change the given setting for the given worker, with a value of
        /// the type of the setting. The value is clamped to the range of the
        /// setting. Settings that hold for the whole pool can only be
        /// changed with set_all.
        template <tuning_value_type T>
        void set(std::size_t worker, std::size_t id, T value) noexcept
        {
            HPX_ASSERT(worker < values_.size() && id < infos_.size() &&
                infos_[id].per_worker);
            values_[worker].data_[id].store(
                tuning_value(infos_[id].clamped(value)),
                std::memory_order_relaxed);
        }

        /// Change the given setting for all workers, with a value of the
        /// type of the setting.
        template <tuning_value_type T>
        void set_all(std::size_t id, T value) noexcept
        {
            HPX_ASSERT(id < infos_.size());
            tuning_value const in_range(infos_[id].clamped(value));
            for (auto& values : values_)
            {
                values.data_[id].store(in_range, std::memory_order_relaxed);
            }
        }

    private:
        // adds the description of a setting, returns its index
        std::size_t add_setting(tuning_parameter_info const& info);

        std::vector<tuning_parameter_info> infos_;

        // the values of a worker are read by that worker where it makes its
        // decisions and written only by a tuner, keep them on their own lines
        using worker_values =
            std::array<std::atomic<tuning_value>, max_parameters>;
        static_assert(std::atomic<tuning_value>::is_always_lock_free);
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
