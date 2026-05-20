//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/performance_counter.hpp>
#include <hpx/performance_counters/server/arithmetics_counter_extended.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/count.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::server {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Statistic>
    arithmetics_counter_extended<Statistic>::arithmetics_counter_extended() =
        default;

    template <typename Statistic>
    arithmetics_counter_extended<Statistic>::arithmetics_counter_extended(
        counter_info const& info,
        std::vector<std::string> const& base_counter_names)
      : base_type_holder(info)
      , counters_(base_counter_names)
    {
        if (info.type_ != counter_type::aggregating)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "arithmetics_counter_extended<Statistic>::"
                "arithmetics_counter_extended",
                "unexpected counter type specified");
        }
    }

    namespace detail {
        template <typename Statistic>
        struct statistic_get_value;

        template <>
        struct statistic_get_value<boost::accumulators::tag::mean>
        {
            template <typename Accumulator>
            static double call(Accumulator const& accum)
            {
                return boost::accumulators::mean(accum);
            }
        };

        template <>
        struct statistic_get_value<boost::accumulators::tag::variance>
        {
            template <typename Accumulator>
            static double call(Accumulator const& accum)
            {
                return boost::accumulators::variance(accum);
            }
        };

        template <>
        struct statistic_get_value<boost::accumulators::tag::median>
        {
            template <typename Accumulator>
            static double call(Accumulator const& accum)
            {
                return boost::accumulators::median(accum);
            }
        };

        template <>
        struct statistic_get_value<boost::accumulators::tag::min>
        {
            template <typename Accumulator>
            static double call(Accumulator const& accum)
            {
                return (boost::accumulators::min) (accum);
            }
        };

        template <>
        struct statistic_get_value<boost::accumulators::tag::max>
        {
            template <typename Accumulator>
            static double call(Accumulator const& accum)
            {
                return (boost::accumulators::max) (accum);
            }
        };

        template <>
        struct statistic_get_value<boost::accumulators::tag::count>
        {
            template <typename Accumulator>
            static double call(Accumulator const& accum)
            {
                return static_cast<double>(boost::accumulators::count(accum));
            }
        };
    }    // namespace detail

    template <typename Statistic>
    hpx::performance_counters::counter_value
    arithmetics_counter_extended<Statistic>::get_counter_value(bool /* reset */)
    {
        std::vector<counter_value> base_values =
            counters_.get_counter_values(hpx::launch::sync);

        // apply arithmetic Statistic
        using accumulator_type = boost::accumulators::accumulator_set<double,
            boost::accumulators::stats<Statistic>>;

        accumulator_type accum;
        for (counter_value const& base_value : base_values)
        {
            accum(base_value.get_value<double>());
        }
        double value = detail::statistic_get_value<Statistic>::call(accum);

        if (base_values[0].scale_inverse_ &&
            static_cast<double>(base_values[0].scaling_) != 1.0)    //-V550
        {
            base_values[0].value_ = static_cast<std::int64_t>(
                value * static_cast<double>(base_values[0].scaling_));
        }
        else
        {
            base_values[0].value_ = static_cast<std::int64_t>(
                value / static_cast<double>(base_values[0].scaling_));
        }

        base_values[0].time_ =
            static_cast<std::int64_t>(hpx::get_system_uptime());
        base_values[0].count_ = counters_.get_invocation_count();

        return base_values[0];
    }

    template <typename Statistic>
    bool arithmetics_counter_extended<Statistic>::start()
    {
        return counters_.start(hpx::launch::sync);
    }

    template <typename Statistic>
    bool arithmetics_counter_extended<Statistic>::stop()
    {
        return counters_.stop(hpx::launch::sync);
    }

    template <typename Statistic>
    void arithmetics_counter_extended<Statistic>::reset_counter_value()
    {
        counters_.reset(hpx::launch::sync);
    }

    template <typename Statistic>
    void arithmetics_counter_extended<Statistic>::finalize()
    {
        base_performance_counter::finalize();
        base_type::finalize();
    }

    template <typename Statistic>
    naming::address
    arithmetics_counter_extended<Statistic>::get_current_address() const
    {
        return naming::address(
            naming::get_gid_from_locality_id(agas::get_locality_id()),
            components::get_component_type<arithmetics_counter_extended>(),
            const_cast<arithmetics_counter_extended*>(this));
    }
}    // namespace hpx::performance_counters::server

///////////////////////////////////////////////////////////////////////////////
template class HPX_EXPORT hpx::performance_counters::server::
    arithmetics_counter_extended<boost::accumulators::tag::mean>;
template class HPX_EXPORT hpx::performance_counters::server::
    arithmetics_counter_extended<boost::accumulators::tag::variance>;
template class HPX_EXPORT hpx::performance_counters::server::
    arithmetics_counter_extended<boost::accumulators::tag::median>;
template class HPX_EXPORT hpx::performance_counters::server::
    arithmetics_counter_extended<boost::accumulators::tag::min>;
template class HPX_EXPORT hpx::performance_counters::server::
    arithmetics_counter_extended<boost::accumulators::tag::max>;
template class HPX_EXPORT hpx::performance_counters::server::
    arithmetics_counter_extended<boost::accumulators::tag::count>;

///////////////////////////////////////////////////////////////////////////////
// /arithmetic/mean
using mean_arithmetics_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            arithmetics_counter_extended<boost::accumulators::tag::mean>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(mean_arithmetics_counter_type,
    mean_arithmetics_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(mean_arithmetics_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// /arithmetic/variance
using variance_arithmetics_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            arithmetics_counter_extended<boost::accumulators::tag::variance>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(variance_arithmetics_counter_type,
    variance_arithmetics_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(variance_arithmetics_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// /arithmetic/median
using median_arithmetics_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            arithmetics_counter_extended<boost::accumulators::tag::median>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(median_arithmetics_counter_type,
    median_arithmetics_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(median_arithmetics_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// /arithmetic/min
using min_arithmetics_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            arithmetics_counter_extended<boost::accumulators::tag::min>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(min_arithmetics_counter_type,
    min_arithmetics_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(min_arithmetics_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// /arithmetic/max
using max_arithmetics_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            arithmetics_counter_extended<boost::accumulators::tag::max>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(max_arithmetics_counter_type,
    max_arithmetics_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(max_arithmetics_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// /arithmetic/count
using count_arithmetics_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            arithmetics_counter_extended<boost::accumulators::tag::count>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(count_arithmetics_counter_type,
    count_arithmetics_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(count_arithmetics_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::detail {

    // Creation function for aggregating performance counters to be registered
    // with the counter types.
    naming::gid_type arithmetics_counter_extended_creator(
        counter_info const& info, error_code& ec)
    {
        if (info.type_ != counter_type::aggregating)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "arithmetics_counter_extended_creator",
                "invalid counter type requested");
            return naming::invalid_gid;
        }

        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
            return naming::invalid_gid;

        if (!paths.parameters_.empty())
        {
            // try to interpret the additional parameter as a list of
            // two performance counter names
            std::vector<std::string> names;
            hpx::string_util::split(
                names, paths.parameters_, hpx::string_util::is_any_of(","));

            if (names.empty())
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "arithmetics_counter_extended_creator",
                    "the parameter specification for an arithmetic counter "
                    "has to expand to at least one counter name: {}",
                    paths.parameters_);
                return naming::invalid_gid;
            }

            for (std::string const& name : names)
            {
                counter_path_elements paths;
                if (counter_status::valid_data !=
                        get_counter_path_elements(name, paths, ec) ||
                    ec)
                {
                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "arithmetics_counter_extended_creator",
                        "the given (expanded) counter name is not "
                        "a validly formed performance counter name: {}",
                        name);
                    return naming::invalid_gid;
                }
            }

            return create_arithmetics_counter_extended(info, names, ec);
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "arithmetics_counter_extended_creator",
            "the parameter specification for an arithmetic counter "
            "has to be a comma separated list of performance "
            "counter names, none is given: {}",
            remove_counter_prefix(info.fullname_));
        return naming::invalid_gid;
    }
}    // namespace hpx::performance_counters::detail
