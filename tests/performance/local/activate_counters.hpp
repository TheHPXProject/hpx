//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/performance_counters.hpp>

#include <cstddef>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx { namespace util {

    ///////////////////////////////////////////////////////////////////////////
    class activate_counters
    {
    public:
        explicit activate_counters(std::vector<std::string> const& names);

        ~activate_counters();

    protected:
        void find_counters();
        bool find_counter(
            performance_counters::counter_info const& info, error_code& ec);

        void start_counters(error_code& ec = throws);
        void stop_counters(error_code& ec = throws);

    public:
        void reset_counters(error_code& ec = throws);

        typedef std::vector<future<performance_counters::counter_value>>
            async_counter_values_type;

        typedef std::vector<performance_counters::counter_value>
            sync_counter_values_type;

        async_counter_values_type evaluate_counters(
            launch::async_policy, bool reset = false, error_code& ec = throws);

        sync_counter_values_type evaluate_counters(
            launch::sync_policy, bool reset = false, error_code& ec = throws);

        std::string name(std::size_t i) const
        {
            return names_[i];
        }

        hpx::id_type id(std::size_t i) const
        {
            return ids_[i];
        }

        std::string unit_of_measure(std::size_t i) const
        {
            return uoms_[i];
        }

    private:
        std::vector<std::string> names_;    // counter instance names
        std::vector<hpx::id_type> ids_;     // gids of counter instances
        std::vector<std::string> uoms_;     // units of measure
    };
}}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
