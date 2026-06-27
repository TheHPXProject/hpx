//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/thread_support.hpp>

#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/performance_counter.hpp>
#include <hpx/performance_counters/server/statistics_counter.hpp>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <boost/accumulators/statistics/rolling_variance.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/variance.hpp>

#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include <boost/accumulators/statistics/median.hpp>
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

#include <hpx/modules/statistics.hpp>

#include <boost/spirit/home/x3/char.hpp>
#include <boost/spirit/home/x3/core.hpp>
#include <boost/spirit/home/x3/numeric.hpp>
#include <boost/spirit/home/x3/operator.hpp>

#include <boost/version.hpp>

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::server {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {
        template <typename Statistic>
        struct counter_type_from_statistic;

        template <>
        struct counter_type_from_statistic<boost::accumulators::tag::mean>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = boost::accumulators::tag::mean;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag>>;

            counter_type_from_statistic(std::size_t /*parameter2*/) {}

            double get_value() override
            {
                return boost::accumulators::mean(accum_);
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return false;
            }

        private:
            accumulator_type accum_;
        };

        template <>
        struct counter_type_from_statistic<boost::accumulators::tag::variance>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = boost::accumulators::tag::variance;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag>>;

            counter_type_from_statistic(std::size_t /*parameter2*/) {}

            double get_value() override
            {
                return sqrt(boost::accumulators::variance(accum_));
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return false;
            }

        private:
            accumulator_type accum_;
        };

        template <>
        struct counter_type_from_statistic<boost::accumulators::tag::median>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = boost::accumulators::tag::median;
            using aggregating_type_tag =
                boost::accumulators::with_p_square_quantile;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag(
                        aggregating_type_tag)>>;

            counter_type_from_statistic(std::size_t /*parameter2*/) {}

            double get_value() override
            {
                return boost::accumulators::median(accum_);
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return false;
            }

        private:
            accumulator_type accum_;
        };

        template <>
        struct counter_type_from_statistic<
            boost::accumulators::tag::rolling_mean>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = boost::accumulators::tag::rolling_mean;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag>>;

            counter_type_from_statistic(std::size_t parameter2)
              : accum_(boost::accumulators::tag::rolling_window::window_size =
                           parameter2)
            {
                if (parameter2 == 0)
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "counter_type_from_statistic<Statistic>",
                        "base rolling window size is specified to be zero");
                }
            }

            double get_value() override
            {
                return boost::accumulators::rolling_mean(accum_);
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return false;
            }

        private:
            accumulator_type accum_;
        };

        template <>
        struct counter_type_from_statistic<
            boost::accumulators::tag::rolling_variance>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = boost::accumulators::tag::rolling_variance;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag>>;

            counter_type_from_statistic(std::size_t parameter2)
              : accum_(boost::accumulators::tag::rolling_window::window_size =
                           parameter2)
            {
                if (parameter2 == 0)
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "counter_type_from_statistic<Statistic>",
                        "base rolling window size is specified to be zero");
                }
            }

            double get_value() override
            {
                return sqrt(boost::accumulators::rolling_variance(accum_));
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return false;
            }

        private:
            accumulator_type accum_;
        };

        template <>
        struct counter_type_from_statistic<boost::accumulators::tag::max>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = boost::accumulators::tag::max;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag>>;

            counter_type_from_statistic(std::size_t /*parameter2*/) {}

            double get_value() override
            {
                return (boost::accumulators::max) (accum_);
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return true;
            }

        private:
            accumulator_type accum_;
        };

        template <>
        struct counter_type_from_statistic<boost::accumulators::tag::min>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = boost::accumulators::tag::min;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag>>;

            counter_type_from_statistic(std::size_t /*parameter2*/) {}

            double get_value() override
            {
                return (boost::accumulators::min) (accum_);
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return true;
            }

        private:
            accumulator_type accum_;
        };

        template <>
        struct counter_type_from_statistic<hpx::util::tag::rolling_min>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = hpx::util::tag::rolling_min;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag>>;

            counter_type_from_statistic(std::size_t parameter2)
              : accum_(boost::accumulators::tag::rolling_window::window_size =
                           parameter2)
            {
                if (parameter2 == 0)
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "counter_type_from_statistic<Statistic>",
                        "base rolling window size is specified to be zero");
                }
            }

            double get_value() override
            {
                return hpx::util::rolling_min(accum_);
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return false;
            }

        private:
            accumulator_type accum_;
        };

        template <>
        struct counter_type_from_statistic<hpx::util::tag::rolling_max>
          : counter_type_from_statistic_base
        {
            using aggregating_tag = hpx::util::tag::rolling_max;
            using accumulator_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::stats<aggregating_tag>>;

            counter_type_from_statistic(std::size_t parameter2)
              : accum_(boost::accumulators::tag::rolling_window::window_size =
                           parameter2)
            {
                if (parameter2 == 0)
                {
                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "counter_type_from_statistic<Statistic>",
                        "base rolling window size is specified to be zero");
                }
            }

            double get_value() override
            {
                return hpx::util::rolling_max(accum_);
            }

            void add_value(double value) override
            {
                accum_(value);
            }

            bool need_reset() const override
            {
                return false;
            }

        private:
            accumulator_type accum_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Statistic>
    statistics_counter<Statistic>::statistics_counter()
      : has_prev_value_(false)
      , parameter1_(0)
      , parameter2_(0)
      , reset_base_counter_(false)
    {
    }

    template <typename Statistic>
    statistics_counter<Statistic>::statistics_counter(counter_info const& info,
        std::string const& base_counter_name, std::size_t parameter1,
        std::size_t parameter2, bool reset_base_counter)
      : base_type_holder(info)
      , timer_(hpx::bind_front(&statistics_counter::evaluate, this_()),
            hpx::bind_front(&statistics_counter::on_terminate, this_()),
            1000 * parameter1, info.fullname_, true)
      , base_counter_name_(ensure_counter_prefix(base_counter_name))
      , value_(new detail::counter_type_from_statistic<Statistic>(parameter2))
      , has_prev_value_(false)
      , parameter1_(parameter1)
      , parameter2_(parameter2)
      , reset_base_counter_(reset_base_counter)
    {
        if (parameter1 == 0)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "statistics_counter<Statistic>::statistics_counter",
                "base interval is specified to be zero");
        }

        if (info.type_ != counter_type::aggregating)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "statistics_counter<Statistic>::statistics_counter",
                "unexpected counter type specified");
        }

        // make sure this counter starts collecting data
        statistics_counter<Statistic>::start();
    }

    template <typename Statistic>
    hpx::performance_counters::counter_value
    statistics_counter<Statistic>::get_counter_value(bool reset)
    {
        std::lock_guard<mutex_type> l(mtx_);

        hpx::performance_counters::counter_value value;

        prev_value_.value_ = static_cast<std::int64_t>(value_->get_value());
        prev_value_.status_ = counter_status::new_data;
        prev_value_.time_ = static_cast<std::int64_t>(hpx::get_system_uptime());
        prev_value_.count_ = ++invocation_count_;
        has_prev_value_ = true;

        value = prev_value_;    // return value

        if (reset || value_->need_reset())
        {
            value_.reset(new detail::counter_type_from_statistic<Statistic>(
                parameter2_));    // reset accumulator
            value_->add_value(static_cast<double>(prev_value_.value_));
            // start off with last base value
        }

        return value;
    }

    template <typename Statistic>
    bool statistics_counter<Statistic>::evaluate()
    {
        // gather current base value
        counter_value base_value;
        if (!evaluate_base_counter(base_value))
            return false;

        // simply average the measured base counter values since it got queried
        // for the last time
        counter_value value;
        if (base_value.scaling_ != prev_value_.scaling_ ||
            base_value.scale_inverse_ != prev_value_.scale_inverse_)
        {
            // not supported right now
            HPX_THROW_EXCEPTION(hpx::error::not_implemented,
                "statistics_counter<Statistic>::evaluate",
                "base counter should keep scaling constant over time");
            return false;
        }
        else
        {
            // accumulate new value
            std::lock_guard<mutex_type> l(mtx_);
            value_->add_value(static_cast<double>(base_value.value_));
        }
        return true;
    }

    template <typename Statistic>
    bool statistics_counter<Statistic>::ensure_base_counter()
    {
        // lock here to avoid checking out multiple reference counted GIDs
        // from AGAS. This
        std::unique_lock<mutex_type> l(mtx_);

        if (!base_counter_id_)
        {
            // get or create the base counter
            error_code ec(throwmode::lightweight);
            hpx::id_type base_counter_id;
            {
                // We need to unlock the lock here since get_counter might suspend
                unlock_guard<std::unique_lock<mutex_type>> unlock(l);
                base_counter_id = get_counter(base_counter_name_, ec);
            }

            // After reacquiring the lock, we need to check again if base_counter_id_
            // hasn't been set yet
            if (!base_counter_id_)
            {
                base_counter_id_ = base_counter_id;
            }
            else
            {
                // If it was set already by a different thread, return true.
                return true;
            }

            if (HPX_UNLIKELY(ec || !base_counter_id_))
            {
                // base counter could not be retrieved
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "statistics_counter<Statistic>::evaluate_base_counter",
                    "could not get or create performance counter: '{}'",
                    base_counter_name_);
                return false;
            }
        }

        return true;
    }

    template <typename Statistic>
    bool statistics_counter<Statistic>::evaluate_base_counter(
        counter_value& value)
    {
        // query the actual value
        if (!base_counter_id_ && !ensure_base_counter())
            return false;

        performance_counters::performance_counter c(base_counter_id_);
        value = c.get_counter_value(launch::sync, reset_base_counter_);

        if (!has_prev_value_)
        {
            has_prev_value_ = true;
            prev_value_ = value;
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Start and stop this counter. We dispatch the calls to the base counter
    // and control our own interval_timer.
    template <typename Statistic>
    bool statistics_counter<Statistic>::start()
    {
        if (!timer_.is_started())
        {
            // start base counter
            if (!base_counter_id_ && !ensure_base_counter())
                return false;

            performance_counters::performance_counter c(base_counter_id_);
            bool result = c.start(launch::sync);
            if (result)
            {
                // acquire the current value of the base counter
                counter_value base_value;
                if (evaluate_base_counter(base_value))
                {
                    std::lock_guard<mutex_type> l(mtx_);
                    value_->add_value(static_cast<double>(base_value.value_));
                    prev_value_ = base_value;
                }

                // start timer
                timer_.start();
            }
            else
            {
                // start timer even if base counter does not support being
                // start/stop operations
                timer_.start(true);
            }
            return result;
        }
        return false;
    }

    template <typename Statistic>
    bool statistics_counter<Statistic>::stop()
    {
        if (timer_.is_started())
        {
            timer_.stop();

            if (!base_counter_id_ && !ensure_base_counter())
                return false;

            performance_counters::performance_counter c(base_counter_id_);
            return c.stop(launch::sync);
        }
        return false;
    }

    template <typename Statistic>
    void statistics_counter<Statistic>::reset_counter_value()
    {
        std::lock_guard<mutex_type> l(mtx_);

        // reset accumulator
        value_.reset(
            new detail::counter_type_from_statistic<Statistic>(parameter2_));

        // start off with last base value
        value_->add_value(static_cast<double>(prev_value_.value_));
    }

    template <typename Statistic>
    void statistics_counter<Statistic>::on_terminate()
    {
    }

    template <typename Statistic>
    void statistics_counter<Statistic>::finalize()
    {
        base_performance_counter::finalize();
        base_type::finalize();
    }

    template <typename Statistic>
    naming::address statistics_counter<Statistic>::get_current_address() const
    {
        return naming::address(
            naming::get_gid_from_locality_id(agas::get_locality_id()),
            components::get_component_type<statistics_counter>(),
            const_cast<statistics_counter*>(this));
    }
}    // namespace hpx::performance_counters::server

///////////////////////////////////////////////////////////////////////////////
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    boost::accumulators::tag::mean>;
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    boost::accumulators::tag::variance>;
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    boost::accumulators::tag::rolling_variance>;
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    boost::accumulators::tag::median>;
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    boost::accumulators::tag::rolling_mean>;
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    boost::accumulators::tag::max>;
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    boost::accumulators::tag::min>;
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    hpx::util::tag::rolling_min>;
template class HPX_EXPORT hpx::performance_counters::server::statistics_counter<
    hpx::util::tag::rolling_max>;

///////////////////////////////////////////////////////////////////////////////
// Average
using average_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<boost::accumulators::tag::mean>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(average_count_counter_type,
    average_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(average_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// Rolling variance
using rolling_variance_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<boost::accumulators::tag::rolling_variance>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(rolling_variance_count_counter_type,
    rolling_variance_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(rolling_variance_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// Variance
using variance_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<boost::accumulators::tag::variance>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(variance_count_counter_type,
    variance_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(variance_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// Rolling average
using rolling_mean_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<boost::accumulators::tag::rolling_mean>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(rolling_mean_count_counter_type,
    rolling_mean_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(rolling_mean_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// Median
using median_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<boost::accumulators::tag::median>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(median_count_counter_type,
    median_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(median_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// Max
using max_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<boost::accumulators::tag::max>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(max_count_counter_type,
    max_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(max_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// Min
using min_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<boost::accumulators::tag::min>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(min_count_counter_type,
    min_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(min_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// Rolling min
using rolling_min_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<hpx::util::tag::rolling_min>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(rolling_min_count_counter_type,
    rolling_min_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(rolling_min_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
// Rolling max
using rolling_max_count_counter_type =
    hpx::components::component<hpx::performance_counters::server::
            statistics_counter<hpx::util::tag::rolling_max>>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(rolling_max_count_counter_type,
    rolling_max_count_counter, "base_performance_counter",
    hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(rolling_max_count_counter_type::wrapped_type)

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::detail {

    /// Creation function for aggregating performance counters to be registered
    /// with the counter types.
    naming::gid_type statistics_counter_creator(
        counter_info const& info, error_code& ec)
    {
        if (info.type_ != counter_type::aggregating)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "statistics_counter_creator", "invalid counter type requested");
            return naming::invalid_gid;
        }

        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
            return naming::invalid_gid;

        if (!paths.parentinstance_is_basename_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "statistics_counter_creator",
                "invalid aggregate counter "
                "name (instance name must be valid base counter name)");
            return naming::invalid_gid;
        }

        std::string base_name;
        get_counter_name(paths.parentinstancename_, base_name, ec);
        if (ec)
            return naming::invalid_gid;

        std::vector<std::size_t> parameters;
        if (!paths.parameters_.empty())
        {
            // try to interpret the additional parameters
            namespace x3 = boost::spirit::x3;
            if (!x3::parse(paths.parameters_.begin(), paths.parameters_.end(),
                    x3::uint_ % ',', parameters))
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "statistics_counter_creator",
                    "invalid parameter specification format for "
                    "this counter: {}",
                    paths.parameters_);
                return naming::invalid_gid;
            }
            if (paths.countername_.find("rolling") != std::string::npos)
            {
                if (parameters.size() > 3)
                {
                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "statistics_counter_creator",
                        "too many parameter specifications for "
                        "this counter: {}",
                        paths.parameters_);
                    return naming::invalid_gid;
                }
            }
            else if (parameters.size() > 2)
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "statistics_counter_creator",
                    "too many parameter specifications for "
                    "this counter: {}",
                    paths.parameters_);
                return naming::invalid_gid;
            }
        }
        else if (paths.countername_.find("rolling") != std::string::npos)
        {
            parameters.push_back(1000);    // sample interval
            parameters.push_back(10);      // rolling window
            parameters.push_back(0);       // don't reset underlying counter
        }
        else
        {
            parameters.push_back(1000);    // sample interval
            parameters.push_back(0);       // don't reset underlying counter
        }

        return create_statistics_counter(info, base_name, parameters, ec);
    }
}    // namespace hpx::performance_counters::detail
