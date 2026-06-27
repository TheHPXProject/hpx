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

        void terminate();

        void start_counters(error_code& ec = throws);
        void stop_counters(error_code& ec = throws);
        void reset_counters(error_code& ec = throws);
        void reinit_counters(bool reset = true, error_code& ec = throws);
        bool evaluate_counters(bool reset = false,
            char const* description = nullptr, bool force = false,
            error_code& ec = throws);

    protected:
        void find_counters();

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

        interval_timer timer_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
