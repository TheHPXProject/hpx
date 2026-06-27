//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/performance_counters/counters.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT class registry
    {
    private:
        struct counter_data
        {
            counter_data(counter_info const& info,
                create_counter_func const& create_counter,
                discover_counters_func const& discover_counters)
              : info_(info)
              , create_counter_(create_counter)
              , discover_counters_(discover_counters)
            {
            }

            counter_info info_;
            create_counter_func create_counter_;
            discover_counters_func discover_counters_;
        };
        using counter_type_map_type = std::map<std::string, counter_data>;

    public:
        registry() = default;

        /// \brief Reset registry by deleting all stored counter types
        void clear();

        /// \brief Add a new performance counter type to the (local) registry
        counter_status add_counter_type(counter_info const& info,
            create_counter_func const& create_counter,
            discover_counters_func const& discover_counters,
            error_code& ec = throws);

        /// \brief Call the supplied function for all registered counter types.
        counter_status discover_counter_types(
            discover_counter_func discover_counter, discover_counters_mode mode,
            error_code& ec = throws) const;

        /// \brief Call the supplied function for the given registered counter type.
        counter_status discover_counter_type(std::string const& fullname,
            discover_counter_func discover_counter, discover_counters_mode mode,
            error_code& ec = throws);

        counter_status discover_counter_type(counter_info const& info,
            discover_counter_func const& f, discover_counters_mode mode,
            error_code& ec = throws)
        {
            return discover_counter_type(info.fullname_, f, mode, ec);
        }

        /// \brief Retrieve the counter creation function which is associated
        ///        with a given counter type.
        counter_status get_counter_create_function(counter_info const& info,
            create_counter_func& create_counter, error_code& ec = throws) const;

        /// \brief Retrieve the counter discovery function which is associated
        ///        with a given counter type.
        counter_status get_counter_discovery_function(counter_info const& info,
            discover_counters_func& func, error_code& ec) const;

        /// \brief Remove an existing counter type from the (local) registry
        ///
        /// \note This doesn't remove existing counters of this type, it just
        ///       inhibits defining new counters using this type.
        counter_status remove_counter_type(
            counter_info const& info, error_code& ec = throws);

        /// \brief Create a new performance counter instance of type
        ///        raw_counter based on given counter value
        counter_status create_raw_counter_value(counter_info const& info,
            std::int64_t* countervalue, naming::gid_type& id,
            error_code& ec = throws);

        /// \brief Create a new performance counter instance of type
        ///        raw_counter based on given function returning the counter
        ///        value
        counter_status create_raw_counter(counter_info const& info,
            hpx::function<std::int64_t()> const& f, naming::gid_type& id,
            error_code& ec = throws);

        /// \brief Create a new performance counter instance of type
        ///        raw_counter based on given function returning the counter
        ///        value
        counter_status create_raw_counter(counter_info const& info,
            hpx::function<std::int64_t(bool)> const& f, naming::gid_type& id,
            error_code& ec = throws);

        /// \brief Create a new performance counter instance of type
        ///        raw_counter based on given function returning the counter
        ///        value
        counter_status create_raw_counter(counter_info const& info,
            hpx::function<std::vector<std::int64_t>()> const& f,
            naming::gid_type& id, error_code& ec = throws);

        /// \brief Create a new performance counter instance of type
        ///        raw_counter based on given function returning the counter
        ///        value
        counter_status create_raw_counter(counter_info const& info,
            hpx::function<std::vector<std::int64_t>(bool)> const& f,
            naming::gid_type& id, error_code& ec = throws);

        /// \brief Create a new performance counter instance based on given
        ///        counter info
        counter_status create_counter(counter_info const& info,
            naming::gid_type& id, error_code& ec = throws);

        /// \brief Create a new statistics performance counter instance based
        ///        on given base counter name and given base time interval
        ///        (milliseconds).
        counter_status create_statistics_counter(counter_info const& info,
            std::string const& base_counter_name,
            std::vector<std::size_t> const& parameters, naming::gid_type& id,
            error_code& ec = throws);

        /// \brief Create a new arithmetics performance counter instance based
        ///        on given base counter names
        counter_status create_arithmetics_counter(counter_info const& info,
            std::vector<std::string> const& base_counter_names,
            naming::gid_type& id, error_code& ec = throws);

        /// \brief Create a new extended arithmetics performance counter
        ///        instance based on given base counter names
        counter_status create_arithmetics_counter_extended(
            counter_info const& info,
            std::vector<std::string> const& base_counter_names,
            naming::gid_type& id, error_code& ec = throws);

        /// \brief Add an existing performance counter instance to the registry
        counter_status add_counter(hpx::id_type const& id,
            counter_info const& info, error_code& ec = throws);

        /// \brief remove the existing performance counter from the registry
        counter_status remove_counter(counter_info const& info,
            hpx::id_type const& id, error_code& ec = throws);

        /// \brief Retrieve counter type information for given counter name
        counter_status get_counter_type(std::string const& name,
            counter_info& info, error_code& ec = throws);

    protected:
        counter_type_map_type::iterator locate_counter_type(
            std::string const& type_name);
        counter_type_map_type::const_iterator locate_counter_type(
            std::string const& type_name) const;

    private:
        counter_type_map_type countertypes_;

    public:
        static registry& instance();
    };
}    // namespace hpx::performance_counters
