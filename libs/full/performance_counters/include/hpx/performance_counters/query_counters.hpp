//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>

#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/performance_counters/counters_fwd.hpp>
#include <hpx/performance_counters/performance_counter_set.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT class HPX_EXPORT query_counters
    {
        // avoid warning about using this in member initializer list
        query_counters* this_()
        {
            return this;
        }

    public:
        query_counters(std::vector<std::string> const& names,
            std::vector<std::string> const& reset_names, std::int64_t interval,
            std::string const& dest, std::string const& form,
            std::vector<std::string> const& shortnames, bool csv_header,
            bool print_counters_locally, bool counter_types);
        ~query_counters();

        void start();
        void stop_evaluating_counters(bool terminate = false);
        bool evaluate(bool force = false);

        /// \brief Return the number of performance counters currently held
        ///        by this object.
        std::size_t size() const;

        void terminate();

        void start_counters(error_code& ec = throws);
        void stop_counters(error_code& ec = throws);
        void reset_counters(error_code& ec = throws);
        void reinit_counters(bool reset = true, error_code& ec = throws);
        bool evaluate_counters(bool reset = false,
            char const* description = nullptr, bool force = false,
            error_code& ec = throws);

    protected:
        /// \brief Resolve the counter names this object was constructed
        ///        with.
        ///
        /// Throws (or sets \a ec, for the counters_.add_counters()
        /// overloads that take one) if any of the requested (non-empty)
        /// name lists fails to resolve, e.g. an invalid exact counter name
        /// or a malformed pattern. A wild-card pattern that legitimately
        /// matches nothing yet is not treated as a failure (see #4627).
        void find_counters();

        /// \brief Re-run discovery for the counter names this object was
        ///        constructed with and merge any newly found counters into
        ///        the existing set.
        ///
        /// Counters requested by wild-card patterns (for instance
        /// \c /apex/*) can be registered with HPX only after program
        /// startup, e.g. because the counter is not known to its provider
        /// until the counter is sampled for the first time. Since
        /// find_counters() only resolves names once, at startup, such
        /// counters would never be picked up. Calling refresh_counters()
        /// again before a final evaluation, such as the one performed when
        /// counters are printed at shutdown, allows those late counters to
        /// be discovered and included as well. Counters that were already
        /// part of the set are left untouched.
        ///
        /// \returns false if any of the requested (non-empty) name lists
        ///          failed to resolve without error, or if starting any
        ///          newly discovered counters failed; true otherwise. The
        ///          registry generation snapshot taken before discovery is
        ///          only cached (last_known_generation_) when this returns
        ///          true, so a partial/failed refresh is retried on the
        ///          next evaluation rather than being silently forgotten.
        bool refresh_counters();

        bool print_raw_counters(bool destination_is_cout, bool reset,
            bool no_output, char const* description,
            std::vector<performance_counters::counter_info> const& infos,
            error_code& ec);
        bool print_array_counters(bool destination_is_cout, bool reset,
            bool no_output, char const* description,
            std::vector<performance_counters::counter_info> const& infos,
            error_code& ec);

        template <typename Stream>
        void print_headers(Stream& output,
            std::vector<performance_counters::counter_info> const& infos);

        template <typename Stream, typename Future>
        void print_values(Stream* output, std::vector<Future>&&,
            std::vector<std::size_t>&& indices,
            std::vector<performance_counters::counter_info> const& infos);

        template <typename Stream>
        void print_value(Stream* out,
            performance_counters::counter_info const& infos,
            performance_counters::counter_value const& value);
        template <typename Stream>
        void print_value(Stream* out,
            performance_counters::counter_info const& infos,
            performance_counters::counter_values_array const& value);

        template <typename Stream>
        void print_name_csv(Stream& out, std::string const& name);

        template <typename Stream>
        void print_value_csv(Stream* out,
            performance_counters::counter_info const& infos,
            performance_counters::counter_value const& value);
        template <typename Stream>
        void print_value_csv(Stream* out,
            performance_counters::counter_info const& infos,
            performance_counters::counter_values_array const& value);

        template <typename Stream>
        void print_name_csv_short(Stream& out, std::string const& name);

    private:
        using mutex_type = hpx::mutex;
        mutex_type mtx_;

        std::vector<std::string> names_;
        std::vector<std::string> reset_names_;
        performance_counters::performance_counter_set counters_;

        std::string destination_;
        std::string format_;
        std::vector<std::string> counter_shortnames_;
        bool csv_header_;
        bool print_counters_locally_;
        bool counter_types_;

        // Whether start() has run. A wildcard pattern such as /apex/* may
        // legitimately match no counters at all when start() runs, so
        // counters_.size() == 0 cannot be used to tell "start() was never
        // called" apart from "start() found nothing (yet)"; see #4627.
        std::atomic<bool> started_;

        // The performance_counters::registry generation last observed by
        // refresh_counters(). Compared against registry::instance()
        // .generation() so that periodic evaluations can cheaply detect
        // "nothing new was registered since last time" (a single atomic
        // load) without paying for a full, AGAS-touching re-discovery on
        // every tick; see #4627.
        std::atomic<std::uint64_t> last_known_generation_;

        interval_timer timer_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
