//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/statistics.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/registry.hpp>
#include <hpx/performance_counters/server/arithmetics_counter.hpp>
#include <hpx/performance_counters/server/arithmetics_counter_extended.hpp>
#include <hpx/performance_counters/server/elapsed_time_counter.hpp>
#include <hpx/performance_counters/server/raw_counter.hpp>
#include <hpx/performance_counters/server/raw_values_counter.hpp>
#include <hpx/performance_counters/server/statistics_counter.hpp>

#include <boost/accumulators/statistics/rolling_variance.hpp>
#include <boost/accumulators/statistics_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

    ///////////////////////////////////////////////////////////////////////////
    void registry::clear()
    {
        countertypes_.clear();
    }

    registry::counter_type_map_type::iterator registry::locate_counter_type(
        std::string const& type_name)
    {
        auto it = countertypes_.find(type_name);
        if (it == countertypes_.end())
        {
            // if the full type is not available, try to locate the object name
            // as a type only
            error_code ec(throwmode::lightweight);
            counter_path_elements p;
            get_counter_type_path_elements(type_name, p, ec);
            if (!ec)
                it = countertypes_.find("/" + p.objectname_);
        }
        return it;
    }

    registry::counter_type_map_type::const_iterator
    registry::locate_counter_type(std::string const& type_name) const
    {
        auto it = countertypes_.find(type_name);
        if (it == countertypes_.end())
        {
            // if the full type is not available, try to locate the object name
            // as a type only
            error_code ec(throwmode::lightweight);
            counter_path_elements p;
            get_counter_type_path_elements(type_name, p, ec);
            if (!ec)
                it = countertypes_.find("/" + p.objectname_);
        }
        return it;
    }

    ///////////////////////////////////////////////////////////////////////////
    counter_status registry::add_counter_type(counter_info const& info,
        create_counter_func const& create_counter_,
        discover_counters_func const& discover_counters_, error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it != countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::add_counter_type",
                "counter type already defined: {}", type_name);
            return counter_status::already_defined;
        }

        std::pair<counter_type_map_type::iterator, bool> p =
            countertypes_.emplace(type_name,
                counter_data(info, create_counter_, discover_counters_));

        if (!p.second)
        {
            LPCS_(warning).format(
                "failed to register counter type {}", type_name);
            return counter_status::invalid_data;
        }

        LPCS_(info).format("counter type {} registered", type_name);

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    /// \brief Call the supplied function for the given registered counter type.
    counter_status registry::discover_counter_type(std::string const& fullname,
        discover_counter_func discover_counter, discover_counters_mode mode,
        error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status = get_counter_type_name(fullname, type_name, ec);
        if (!status_is_valid(status))
            return status;

        if (type_name.find_first_of("*?[]") == std::string::npos)
        {
            auto it = locate_counter_type(type_name);
            if (it == countertypes_.end())
            {
                // compose a list of known counter types
                std::string types;
                auto end = countertypes_.end();
                for (auto it_ct = countertypes_.begin(); it_ct != end; ++it_ct)
                {
                    types += "  " + it_ct->first + "\n";
                }

                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "registry::discover_counter_type",
                    "unknown counter type: {}, known counter types: \n{}",
                    type_name, types);
                return counter_status::counter_type_unknown;
            }

            if (mode == discover_counters_mode::full)
            {
                using hpx::placeholders::_1;
                discover_counter = hpx::bind(
                    &expand_counter_info, _1, discover_counter, std::ref(ec));
            }

            counter_info info = it->second.info_;
            info.fullname_ = fullname;

            if (!it->second.discover_counters_.empty() &&
                !it->second.discover_counters_(
                    info, discover_counter, mode, ec))
            {
                return counter_status::invalid_data;
            }
        }
        else
        {
            std::string str_rx(util::regex_from_pattern(type_name, ec));
            if (ec)
                return counter_status::invalid_data;

            if (mode == discover_counters_mode::full)
            {
                using hpx::placeholders::_1;
                discover_counter = hpx::bind(
                    &expand_counter_info, _1, discover_counter, std::ref(ec));
            }

            // split name
            counter_path_elements p;
            get_counter_path_elements(fullname, p, ec);
            if (ec)
                return counter_status::invalid_data;

            bool found_one = false;
            std::regex rx(str_rx);

            counter_type_map_type::const_iterator end = countertypes_.end();
            for (counter_type_map_type::const_iterator it =
                     countertypes_.begin();
                it != end; ++it)
            {
                if (!std::regex_match(it->first, rx))
                    continue;
                found_one = true;

                // propagate parameters
                counter_info info = it->second.info_;
                if (!p.parameters_.empty())
                    info.fullname_ += "@" + p.parameters_;

                if (!it->second.discover_counters_.empty() &&
                    !it->second.discover_counters_(
                        info, discover_counter, mode, ec))
                {
                    return counter_status::invalid_data;
                }
            }

            if (!found_one)
            {
                // compose a list of known counter types
                std::string types;
                auto end_ct = countertypes_.end();
                for (auto it = countertypes_.begin(); it != end_ct; ++it)
                {
                    types += "  " + it->first + "\n";
                }

                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "registry::discover_counter_type",
                    "counter type {} does not match any known type, known "
                    "counter types: \n{}",
                    type_name, types);
                return counter_status::counter_type_unknown;
            }
        }

        if (&ec != &throws)
            ec = make_success_code();

        return counter_status::valid_data;
    }

    /// \brief Call the supplied function for all registered counter types.
    counter_status registry::discover_counter_types(
        discover_counter_func discover_counter, discover_counters_mode mode,
        error_code& ec) const
    {
        // Introducing this temporary silence a report about a potential memory
        // from clang's static analyzer
        discover_counter_func discover_counter_;
        if (mode == discover_counters_mode::full)
        {
            using hpx::placeholders::_1;
            discover_counter_ = hpx::bind(&expand_counter_info, _1,
                HPX_MOVE(discover_counter), std::ref(ec));
        }
        else
        {
            discover_counter_ = HPX_MOVE(discover_counter);
        }

        for (auto const& [k, v] : countertypes_)
        {
            if (!v.discover_counters_.empty() &&
                !v.discover_counters_(v.info_, discover_counter_, mode, ec))
            {
                return counter_status::invalid_data;
            }
        }

        if (&ec != &throws)
            ec = make_success_code();

        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    counter_status registry::get_counter_create_function(
        counter_info const& info, create_counter_func& func,
        error_code& ec) const
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            // compose a list of known counter types
            std::string types;
            auto end = countertypes_.end();
            for (auto it_ct = countertypes_.begin(); it_ct != end; ++it_ct)
            {
                types += "  " + it_ct->first + "\n";
            }

            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::get_counter_create_function",
                "counter type {} is not defined, known counter types: \n{}",
                type_name, types);
            return counter_status::counter_type_unknown;
        }

        if (it->second.create_counter_.empty())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::get_counter_create_function",
                "counter type {} has no associated create function", type_name);
            return counter_status::invalid_data;
        }

        func = it->second.create_counter_;

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    counter_status registry::get_counter_discovery_function(
        counter_info const& info, discover_counters_func& func,
        error_code& ec) const
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::get_counter_discovery_function",
                "counter type {} is not defined", type_name);
            return counter_status::counter_type_unknown;
        }

        if (it->second.discover_counters_.empty())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::get_counter_discovery_function",
                "counter type {} has no associated discovery function",
                type_name);
            return counter_status::invalid_data;
        }

        func = it->second.discover_counters_;

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    counter_status registry::remove_counter_type(
        counter_info const& info, error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::remove_counter_type", "counter type is not defined");
            return counter_status::counter_type_unknown;
        }

        LPCS_(info).format("counter type {} unregistered", type_name);

        countertypes_.erase(it);

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::int64_t wrap_counter(std::int64_t* p, bool /* reset */)
    {
        std::int64_t result = *p;
        *p = 0;
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    counter_status registry::create_raw_counter_value(counter_info const& info,
        std::int64_t* countervalue, naming::gid_type& id, error_code& ec)
    {
        hpx::function<std::int64_t(bool)> func(
            hpx::bind_front(wrap_counter, countervalue));
        return create_raw_counter(info, func, id, ec);
    }

    static std::int64_t wrap_raw_counter(
        hpx::function<std::int64_t()> const& f, bool)
    {
        return f();
    }

    static std::vector<std::int64_t> wrap_raw_values_counter(
        hpx::function<std::vector<std::int64_t>()> const& f, bool)
    {
        return f();
    }

    counter_status registry::create_raw_counter(counter_info const& info,
        hpx::function<std::int64_t()> const& f, naming::gid_type& id,
        error_code& ec)
    {
        hpx::function<std::int64_t(bool)> func(
            hpx::bind_front(&wrap_raw_counter, f));
        return create_raw_counter(info, func, id, ec);
    }

    namespace {

        constexpr bool is_not_counter_type(
            counter_type ct, counter_type ct1, counter_type ct2) noexcept
        {
            return ct != ct1 || ct != ct2;
        }
    }    // namespace

    counter_status registry::create_raw_counter(counter_info const& info,
        hpx::function<std::int64_t(bool)> const& f, naming::gid_type& id,
        error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_raw_counter", "unknown counter type {}",
                type_name);
            return counter_status::counter_type_unknown;
        }

        // make sure the counter type requested is supported
        if (is_not_counter_type(
                counter_type::raw, it->second.info_.type_, info.type_) &&
            is_not_counter_type(counter_type::monotonically_increasing,
                it->second.info_.type_, info.type_) &&
            is_not_counter_type(counter_type::aggregating,
                it->second.info_.type_, info.type_) &&
            is_not_counter_type(counter_type::elapsed_time,
                it->second.info_.type_, info.type_) &&
            is_not_counter_type(counter_type::average_count,
                it->second.info_.type_, info.type_) &&
            is_not_counter_type(counter_type::average_timer,
                it->second.info_.type_, info.type_))
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_raw_counter",
                "invalid counter type requested (only counter_type::raw, "
                "counter_type::monotonically_increasing, "
                "counter_type::aggregating, "
                "counter_type::elapsed_time, counter_type::average_count, or "
                "counter_type::average_timer are supported)");
            return counter_status::counter_type_unknown;
        }

        // make sure parent instance name is set properly
        counter_info complemented_info = info;
        complement_counter_info(complemented_info, it->second.info_, ec);
        if (ec)
            return counter_status::invalid_data;

        // create the counter as requested
        try
        {
            using counter_t = components::component<server::raw_counter>;
            id = components::server::construct<counter_t>(complemented_info, f);
        }
        catch (hpx::exception const& e)
        {
            id = naming::invalid_gid;    // reset result
            if (&ec == &throws)
                throw;
            ec = make_error_code(e.get_error(), e.what());
            LPCS_(warning).format("failed to create raw counter {} ({})",
                complemented_info.fullname_, e.what());
            return counter_status::invalid_data;
        }

        LPCS_(info).format(
            "raw counter {} created at {}", complemented_info.fullname_, id);

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    counter_status registry::create_raw_counter(counter_info const& info,
        hpx::function<std::vector<std::int64_t>()> const& f,
        naming::gid_type& id, error_code& ec)
    {
        hpx::function<std::vector<std::int64_t>(bool)> func(
            hpx::bind_front(&wrap_raw_values_counter, f));
        return create_raw_counter(info, func, id, ec);
    }

    counter_status registry::create_raw_counter(counter_info const& info,
        hpx::function<std::vector<std::int64_t>(bool)> const& f,
        naming::gid_type& id, error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_raw_counter", "unknown counter type {}",
                type_name);
            return counter_status::counter_type_unknown;
        }

        // make sure the counter type requested is supported
        if (!((counter_type::histogram == it->second.info_.type_ &&
                  counter_type::histogram == info.type_) ||
                (counter_type::raw_values == it->second.info_.type_ &&
                    counter_type::raw_values == info.type_)))
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_raw_counter",
                "invalid counter type requested (only counter_histogram or "
                "counter_raw_values are supported)");
            return counter_status::counter_type_unknown;
        }

        // make sure parent instance name is set properly
        counter_info complemented_info = info;
        complement_counter_info(complemented_info, it->second.info_, ec);
        if (ec)
            return counter_status::invalid_data;

        // create the counter as requested
        try
        {
            using counter_t = components::component<server::raw_values_counter>;
            id = components::server::construct<counter_t>(complemented_info, f);
        }
        catch (hpx::exception const& e)
        {
            id = naming::invalid_gid;    // reset result
            if (&ec == &throws)
                throw;
            ec = make_error_code(e.get_error(), e.what());
            LPCS_(warning).format("failed to create raw counter {} ({})",
                complemented_info.fullname_, e.what());
            return counter_status::invalid_data;
        }

        LPCS_(info).format(
            "raw counter {} created at {}", complemented_info.fullname_, id);

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    counter_status registry::create_counter(
        counter_info const& info, naming::gid_type& id, error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_counter", "unknown counter type {}",
                type_name);
            return counter_status::counter_type_unknown;
        }

        // make sure parent instance name is set properly
        counter_info complemented_info = info;
        complement_counter_info(complemented_info, it->second.info_, ec);
        if (ec)
            return counter_status::invalid_data;

        // create the counter as requested
        try
        {
            switch (complemented_info.type_)
            {
            case counter_type::elapsed_time:
            {
                using counter_t =
                    components::component<server::elapsed_time_counter>;
                id =
                    components::server::construct<counter_t>(complemented_info);
            }
            break;

            // NOLINTNEXTLINE(bugprone-branch-clone)
            case counter_type::raw:
                [[fallthrough]];
            case counter_type::monotonically_increasing:
                [[fallthrough]];
            case counter_type::aggregating:
                [[fallthrough]];
            case counter_type::average_count:
                [[fallthrough]];
            case counter_type::average_timer:
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "registry::create_counter",
                    "need function parameter for raw_counter");
                return counter_status::counter_type_unknown;

            default:
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "registry::create_counter",
                    "invalid counter type requested");
                return counter_status::counter_type_unknown;
            }
        }
        catch (hpx::exception const& e)
        {
            id = naming::invalid_gid;    // reset result
            if (&ec == &throws)
                throw;
            ec = make_error_code(e.get_error(), e.what());
            LPCS_(warning).format("failed to create counter {} ({})",
                complemented_info.fullname_, e.what());
            return counter_status::invalid_data;
        }

        LPCS_(info).format(
            "counter {} created at {}", complemented_info.fullname_, id);

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    /// \brief Create a new statistics performance counter instance based
    ///        on given base counter name and given base time interval
    ///        (milliseconds).
    counter_status registry::create_statistics_counter(counter_info const& info,
        std::string const& base_counter_name,
        std::vector<std::size_t> const& parameters, naming::gid_type& gid,
        error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_statistics_counter",
                "unknown counter type {}", type_name);
            return counter_status::counter_type_unknown;
        }

        // make sure the requested counter type is supported
        if (counter_type::aggregating != it->second.info_.type_ ||
            counter_type::aggregating != info.type_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_statistics_counter",
                "invalid counter type requested (only "
                "counter_type::aggregating is "
                "supported)");
            return counter_status::counter_type_unknown;
        }

        // make sure parent instance name is set properly
        counter_info complemented_info = info;
        complement_counter_info(complemented_info, it->second.info_, ec);
        if (ec)
            return counter_status::invalid_data;

        // split name
        counter_path_elements p;
        get_counter_path_elements(complemented_info.fullname_, p, ec);
        if (ec)
            return counter_status::invalid_data;

        // create the counter as requested
        try
        {
            // extract parameters
            std::size_t sample_interval = 1000;    // default sampling interval
            bool reset_base_counter = false;

            if (!parameters.empty())
                sample_interval = parameters[0];

            // create base counter only if it does not exist yet
            if (p.countername_ == "average")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        boost::accumulators::tag::mean>>;

                if (parameters.size() > 1)
                    reset_base_counter = parameters[1] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval, 0,
                    reset_base_counter);
            }
            else if (p.countername_ == "stddev")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        boost::accumulators::tag::variance>>;

                if (parameters.size() > 1)
                    reset_base_counter = parameters[1] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval, 0,
                    reset_base_counter);
            }
            else if (p.countername_ == "rolling_average")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        boost::accumulators::tag::rolling_mean>>;

                std::size_t window_size = 10;    // default rolling window size
                if (parameters.size() > 1)
                    window_size = parameters[1];

                if (parameters.size() > 2)
                    reset_base_counter = parameters[2] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval,
                    window_size, reset_base_counter);
            }
            else if (p.countername_ == "rolling_stddev")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        boost::accumulators::tag::rolling_variance>>;

                std::size_t window_size = 10;    // default rolling window size
                if (parameters.size() > 1)
                    window_size = parameters[1];

                if (parameters.size() > 2)
                    reset_base_counter = parameters[2] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval,
                    window_size, reset_base_counter);
            }
            else if (p.countername_ == "median")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        boost::accumulators::tag::median>>;

                if (parameters.size() > 1)
                    reset_base_counter = parameters[1] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval, 0,
                    reset_base_counter);
            }
            else if (p.countername_ == "max")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        boost::accumulators::tag::max>>;

                if (parameters.size() > 1)
                    reset_base_counter = parameters[1] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval, 0,
                    reset_base_counter);
            }
            else if (p.countername_ == "min")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        boost::accumulators::tag::min>>;

                if (parameters.size() > 1)
                    reset_base_counter = parameters[1] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval, 0,
                    reset_base_counter);
            }
            else if (p.countername_ == "rolling_min")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        hpx::util::tag::rolling_min>>;

                std::size_t window_size = 10;    // default rolling window size
                if (parameters.size() > 1)
                    window_size = parameters[1];

                if (parameters.size() > 2)
                    reset_base_counter = parameters[2] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval,
                    window_size, reset_base_counter);
            }
            else if (p.countername_ == "rolling_max")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::statistics_counter<
                        hpx::util::tag::rolling_max>>;

                std::size_t window_size = 10;    // default rolling window size
                if (parameters.size() > 1)
                    window_size = parameters[1];

                if (parameters.size() > 2)
                    reset_base_counter = parameters[2] != 0;

                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_name, sample_interval,
                    window_size, reset_base_counter);
            }
            else
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "registry::create_statistics_counter",
                    "invalid counter type requested: {}", p.countername_);
                return counter_status::counter_type_unknown;
            }
        }
        catch (hpx::exception const& e)
        {
            gid = naming::invalid_gid;    // reset result
            if (&ec == &throws)
                throw;

            ec = make_error_code(e.get_error(), e.what());
            LPCS_(warning).format("failed to create statistics counter {} ({})",
                complemented_info.fullname_, e.what());
            return counter_status::invalid_data;
        }

        LPCS_(info).format("statistics counter {} created at {}",
            complemented_info.fullname_, gid);

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    /// \brief Create a new arithmetics performance counter instance based
    ///        on given base counter names
    counter_status registry::create_arithmetics_counter(
        counter_info const& info,
        std::vector<std::string> const& base_counter_names,
        naming::gid_type& gid, error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_arithmetics_counter",
                "unknown counter type {}", type_name);
            return counter_status::counter_type_unknown;
        }

        // make sure the requested counter type is supported
        if (counter_type::aggregating != it->second.info_.type_ ||
            counter_type::aggregating != info.type_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_arithmetics_counter",
                "invalid counter type requested "
                "(only counter_type::aggregating is supported)");
            return counter_status::counter_type_unknown;
        }

        // make sure parent instance name is set properly
        counter_info complemented_info = info;
        complement_counter_info(complemented_info, it->second.info_, ec);
        if (ec)
            return counter_status::invalid_data;

        // split name
        counter_path_elements p;
        get_counter_path_elements(complemented_info.fullname_, p, ec);
        if (ec)
            return counter_status::invalid_data;

        // create the counter as requested
        try
        {
            // create base counter only if it does not exist yet
            if (p.countername_ == "add")
            {
                using counter_t =
                    hpx::components::component<hpx::performance_counters::
                            server::arithmetics_counter<std::plus<double>>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else if (p.countername_ == "subtract")
            {
                using counter_t =
                    hpx::components::component<hpx::performance_counters::
                            server::arithmetics_counter<std::minus<double>>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else if (p.countername_ == "multiply")
            {
                using counter_t = hpx::components::component<
                    hpx::performance_counters::server::arithmetics_counter<
                        std::multiplies<double>>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else if (p.countername_ == "divide")
            {
                using counter_t =
                    hpx::components::component<hpx::performance_counters::
                            server::arithmetics_counter<std::divides<double>>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "registry::create_arithmetics_counter",
                    "invalid counter type requested: {}", p.countername_);
                return counter_status::counter_type_unknown;
            }
        }
        catch (hpx::exception const& e)
        {
            gid = naming::invalid_gid;    // reset result
            if (&ec == &throws)
                throw;

            ec = make_error_code(e.get_error(), e.what());
            LPCS_(warning).format(
                "failed to create aggregating counter {} ({})",
                complemented_info.fullname_, e.what());
            return counter_status::invalid_data;
        }

        LPCS_(info).format("aggregating counter {} created at {}",
            complemented_info.fullname_, gid);

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    /// \brief Create a new arithmetics extended performance counter instance
    ///        based on given base counter names
    counter_status registry::create_arithmetics_counter_extended(
        counter_info const& info,
        std::vector<std::string> const& base_counter_names,
        naming::gid_type& gid, error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_arithmetics_counter_extended",
                "unknown counter type {}", type_name);
            return counter_status::counter_type_unknown;
        }

        // make sure the requested counter type is supported
        if (counter_type::aggregating != it->second.info_.type_ ||
            counter_type::aggregating != info.type_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::create_arithmetics_counter_extended",
                "invalid counter type requested "
                "(only counter_type::aggregating is supported)");
            return counter_status::counter_type_unknown;
        }

        // make sure parent instance name is set properly
        counter_info complemented_info = info;
        complement_counter_info(complemented_info, it->second.info_, ec);
        if (ec)
            return counter_status::invalid_data;

        // split name
        counter_path_elements p;
        get_counter_path_elements(complemented_info.fullname_, p, ec);
        if (ec)
            return counter_status::invalid_data;

        // create the counter as requested
        try
        {
            // create base counter only if it does not exist yet
            if (p.countername_ == "mean")
            {
                using counter_t =
                    hpx::components::component<hpx::performance_counters::
                            server::arithmetics_counter_extended<
                                boost::accumulators::tag::mean>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else if (p.countername_ == "variance")
            {
                using counter_t = hpx::components::component<
                    performance_counters::server::arithmetics_counter_extended<
                        boost::accumulators::tag::variance>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else if (p.countername_ == "median")
            {
                using counter_t = hpx::components::component<
                    performance_counters::server::arithmetics_counter_extended<
                        boost::accumulators::tag::median>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else if (p.countername_ == "min")
            {
                using counter_t = hpx::components::component<
                    performance_counters::server::arithmetics_counter_extended<
                        boost::accumulators::tag::min>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else if (p.countername_ == "max")
            {
                using counter_t = hpx::components::component<
                    performance_counters::server::arithmetics_counter_extended<
                        boost::accumulators::tag::max>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else if (p.countername_ == "count")
            {
                using counter_t = hpx::components::component<
                    performance_counters::server::arithmetics_counter_extended<
                        boost::accumulators::tag::count>>;
                gid = components::server::construct<counter_t>(
                    complemented_info, base_counter_names);
            }
            else
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "registry::create_arithmetics_counter",
                    "invalid counter type requested: {}", p.countername_);
                return counter_status::counter_type_unknown;
            }
        }
        catch (hpx::exception const& e)
        {
            gid = naming::invalid_gid;    // reset result
            if (&ec == &throws)
                throw;

            ec = make_error_code(e.get_error(), e.what());
            LPCS_(warning).format(
                "failed to create aggregating counter {} ({})",
                complemented_info.fullname_, e.what());
            return counter_status::invalid_data;
        }

        LPCS_(info).format("aggregating counter {} created at {}",
            complemented_info.fullname_, gid);

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Add an existing performance counter instance to the registry
    counter_status registry::add_counter(
        hpx::id_type const& id, counter_info const& info, error_code& ec)
    {
        // complement counter info data
        counter_info complemented_info = info;
        complement_counter_info(complemented_info, ec);
        if (ec)
            return counter_status::invalid_data;

        // create canonical type name
        std::string type_name;
        counter_status status =
            get_counter_type_name(complemented_info.fullname_, type_name, ec);
        if (!status_is_valid(status))
            return status;

        // make sure the type of the new counter is known to the registry
        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::add_counter", "unknown counter type {}", type_name);
            return counter_status::counter_type_unknown;
        }

        // register the canonical name with AGAS
        std::string name(complemented_info.fullname_);
        ensure_counter_prefix(name);    // pre-pend prefix, if necessary
        agas::register_name(launch::sync, name, id, ec);
        if (ec)
            return counter_status::invalid_data;

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    counter_status registry::remove_counter(
        counter_info const& info, hpx::id_type const& /* id */, error_code& ec)
    {
        // make sure parent instance name is set properly
        counter_info complemented_info = info;
        complement_counter_info(complemented_info, ec);
        if (ec)
            return counter_status::invalid_data;

        // create canonical name for the counter
        std::string name;
        counter_status status =
            get_counter_name(complemented_info.fullname_, name, ec);
        if (!status_is_valid(status))
            return status;

        // unregister this counter from AGAS
        ensure_counter_prefix(name);    // pre-pend prefix, if necessary
        agas::unregister_name(launch::sync, name, ec);
        if (ec)
        {
            LPCS_(warning).format(
                "failed to remove counter {}", complemented_info.fullname_);
            return counter_status::invalid_data;
        }

        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Retrieve counter type information for given counter name
    counter_status registry::get_counter_type(
        std::string const& name, counter_info& info, error_code& ec)
    {
        // create canonical type name
        std::string type_name;
        counter_status status = get_counter_type_name(name, type_name, ec);
        if (!status_is_valid(status))
            return status;

        // make sure the type of the counter is known to the registry
        auto it = locate_counter_type(type_name);
        if (it == countertypes_.end())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "registry::get_counter_type", "unknown counter type {}",
                type_name);
            return counter_status::counter_type_unknown;
        }

        info = it->second.info_;

        if (&ec != &throws)
            ec = make_success_code();
        return counter_status::valid_data;
    }

    registry& registry::instance()
    {
        static registry instance_;
        return instance_;
    }
}    // namespace hpx::performance_counters
