//  Copyright (c) 2016-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCEL_COALESCING)
#include <hpx/modules/errors.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/modules/performance_counters.hpp>

#include <hpx/parcel_coalescing/counter_registry.hpp>

#include <cstdint>
#include <mutex>
#include <regex>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins::parcel {

    coalescing_counter_registry& coalescing_counter_registry::instance()
    {
        hpx::util::static_<coalescing_counter_registry, tag> registry;
        return registry.get();
    }

    ///////////////////////////////////////////////////////////////////////////
    void coalescing_counter_registry::register_action(std::string const& name,
        get_counter_type const& num_parcels,
        get_counter_type const& num_messages,
        get_counter_type const& num_parcels_per_message,
        get_counter_type const& average_time_between_parcels,
        get_counter_values_creator_type const&
            time_between_parcels_histogram_creator)
    {
        if (name.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_counter_registry::register_action",
                "Cannot register an action with an empty name");
        }

        std::lock_guard<mutex_type> l(mtx_);

        auto it = map_.find(name);
        if (it == map_.end())
        {
            counter_functions data = {num_parcels, num_messages,
                num_parcels_per_message, average_time_between_parcels,
                time_between_parcels_histogram_creator, 0, 0, 1};

            map_.emplace(name, HPX_MOVE(data));
        }
        else
        {
            // replace the existing functions
            it->second.num_parcels = num_parcels;
            it->second.num_messages = num_messages;
            it->second.num_parcels_per_message = num_parcels_per_message;
            it->second.average_time_between_parcels =
                average_time_between_parcels;
            it->second.time_between_parcels_histogram_creator =
                time_between_parcels_histogram_creator;

            if (it->second.min_boundary != it->second.max_boundary)
            {
                // instantiate actual histogram collection
                coalescing_counter_registry::get_counter_values_type result;
                time_between_parcels_histogram_creator(it->second.min_boundary,
                    it->second.max_boundary, it->second.num_buckets, result);
            }

            // silence warnings
            (void) it->second.num_parcels;
            (void) it->second.num_messages;
            (void) it->second.num_parcels_per_message;
            (void) it->second.average_time_between_parcels;
            (void) it->second.time_between_parcels_histogram_creator;
        }
    }

    void coalescing_counter_registry::register_action(std::string const& name)
    {
        if (name.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_counter_registry::register_action",
                "Cannot register an action with an empty name");
        }

        std::lock_guard<mutex_type> l(mtx_);

        auto it = map_.find(name);
        if (it == map_.end())
        {
            map_.emplace(name, counter_functions());
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    coalescing_counter_registry::get_counter_type
    coalescing_counter_registry::get_parcels_counter(
        std::string const& name) const
    {
        std::unique_lock<mutex_type> l(mtx_);

        map_type::const_iterator it = map_.find(name);
        if (it == map_.end())
        {
            l.unlock();
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_counter_registry::get_num_parcels_counter",
                "unknown action type");
        }
        return it->second.num_parcels;
    }

    coalescing_counter_registry::get_counter_type
    coalescing_counter_registry::get_messages_counter(
        std::string const& name) const
    {
        std::unique_lock<mutex_type> l(mtx_);

        map_type::const_iterator it = map_.find(name);
        if (it == map_.end())
        {
            l.unlock();
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_counter_registry::get_num_messages_counter",
                "unknown action type");
        }
        return it->second.num_messages;
    }

    coalescing_counter_registry::get_counter_type
    coalescing_counter_registry::get_parcels_per_message_counter(
        std::string const& name) const
    {
        std::unique_lock<mutex_type> l(mtx_);

        map_type::const_iterator it = map_.find(name);
        if (it == map_.end())
        {
            l.unlock();
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_counter_registry::get_num_messages_counter",
                "unknown action type");
        }
        return it->second.num_parcels_per_message;
    }

    coalescing_counter_registry::get_counter_type
    coalescing_counter_registry::get_average_time_between_parcels_counter(
        std::string const& name) const
    {
        std::unique_lock<mutex_type> l(mtx_);

        map_type::const_iterator it = map_.find(name);
        if (it == map_.end())
        {
            l.unlock();
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_counter_registry::"
                "get_average_time_between_parcels_counter",
                "unknown action type");
        }
        return it->second.average_time_between_parcels;
    }

    coalescing_counter_registry::get_counter_values_type
    coalescing_counter_registry::get_time_between_parcels_histogram_counter(
        std::string const& name, std::int64_t min_boundary,
        std::int64_t max_boundary, std::int64_t num_buckets)
    {
        std::unique_lock<mutex_type> l(mtx_);

        map_type::iterator it = map_.find(name);
        if (it == map_.end())
        {
            l.unlock();
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_counter_registry::"
                "get_time_between_parcels_histogram_counter",
                "unknown action type");
        }

        if (it->second.time_between_parcels_histogram_creator.empty())
        {
            // no parcel of this type has been sent yet
            it->second.min_boundary = min_boundary;
            it->second.max_boundary = max_boundary;
            it->second.num_buckets = num_buckets;
            return coalescing_counter_registry::get_counter_values_type();
        }

        coalescing_counter_registry::get_counter_values_type result;
        it->second.time_between_parcels_histogram_creator(
            min_boundary, max_boundary, num_buckets, result);
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool coalescing_counter_registry::counter_discoverer(
        performance_counters::counter_info const& info,
        performance_counters::counter_path_elements& p,
        performance_counters::discover_counter_func const& f,
        performance_counters::discover_counters_mode mode, error_code& ec)
    {
        if (mode == performance_counters::discover_counters_mode::minimal ||
            p.parentinstancename_.empty() || p.instancename_.empty())
        {
            if (p.parentinstancename_.empty())
            {
                p.parentinstancename_ = "locality#*";
                p.parentinstanceindex_ = -1;
            }

            if (p.instancename_.empty())
            {
                p.instancename_ = "total";
                p.instanceindex_ = -1;
            }
        }

        if (p.parameters_.empty())
        {
            if (mode == performance_counters::discover_counters_mode::minimal)
            {
                std::string fullname;
                performance_counters::get_counter_name(p, fullname, ec);
                if (ec)
                    return false;

                performance_counters::counter_info cinfo = info;
                cinfo.fullname_ = fullname;
                return f(cinfo, ec) && !ec;
            }

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
            p.parameters_ = "*";
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif
        }

        std::string parameters = p.parameters_;
        std::string additional_parameters;

        std::string::size_type pos = parameters.find_first_of(',');
        if (pos != std::string::npos)
        {
            additional_parameters = parameters.substr(pos);
            parameters = parameters.substr(0, pos);
        }

        if (parameters.find_first_of("*?[]") != std::string::npos)
        {
            std::string str_rx(util::regex_from_pattern(parameters, ec));
            if (ec)
                return false;

            std::regex rx(str_rx);

            std::vector<performance_counters::counter_path_elements> counters;

            {
                std::unique_lock<mutex_type> l(mtx_);

                map_type::const_iterator end = map_.end();
                for (map_type::const_iterator it = map_.begin(); it != end;
                    ++it)
                {
                    if (!std::regex_match(it->first, rx))
                        continue;

                    performance_counters::counter_path_elements cp = p;
                    cp.parameters_ = it->first;
                    if (!additional_parameters.empty())
                        cp.parameters_ += additional_parameters;

                    counters.push_back(HPX_MOVE(cp));
                }
            }

            if (counters.empty())
            {
                // compose a list of known action types
                std::string types;

                {
                    std::unique_lock<mutex_type> l(mtx_);
                    map_type::const_iterator end = map_.end();
                    for (map_type::const_iterator it = map_.begin(); it != end;
                        ++it)
                    {
                        types += "  " + it->first + "\n";
                    }
                }

                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "coalescing_counter_registry::counter_discoverer",
                    "action type {} does not match any known type, "
                    "known action types: \n{}",
                    p.parameters_, types);
                return false;
            }

            for (auto&& cp : counters)
            {
                // propagate parameters
                std::string fullname;

                performance_counters::get_counter_name(cp, fullname, ec);
                if (ec)
                    return false;

                performance_counters::counter_info cinfo = info;
                cinfo.fullname_ = fullname;

                if (!f(cinfo, ec) || ec)
                    return false;
            }

            if (&ec != &throws)
                ec = make_success_code();

            return true;
        }

        {
            std::unique_lock<mutex_type> l(mtx_);

            // use given action type directly
            map_type::const_iterator it = map_.find(parameters);
            if (it == map_.end())
            {
                // compose a list of known action types
                std::string types;
                map_type::const_iterator end = map_.end();
                for (map_type::const_iterator it_ct = map_.begin();
                    it_ct != end; ++it_ct)
                {
                    types += "  " + it_ct->first + "\n";
                }

                l.unlock();
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "coalescing_counter_registry::counter_discoverer",
                    "action type {} does not match any known type, "
                    "known action types: \n{}",
                    p.parameters_, types);
                return false;
            }
        }

        // propagate parameters
        std::string fullname;
        performance_counters::get_counter_name(p, fullname, ec);
        if (ec)
            return false;

        performance_counters::counter_info cinfo = info;
        cinfo.fullname_ = fullname;

        if (!f(cinfo, ec))
            return false;

        if (ec)
            return false;

        if (&ec != &throws)
            ec = make_success_code();

        return true;
    }
}    // namespace hpx::plugins::parcel

#endif
