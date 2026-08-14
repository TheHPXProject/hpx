//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/performance_counters/counters_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

#if defined(DOXYGEN)
    ///////////////////////////////////////////////////////////////////////////
    enum class counter_type
    {
        /// \a text shows a variable-length text string. It does not
        /// deliver calculated values.
        ///
        /// Formula:  None
        /// Average:  None
        /// Type:     Text
        text,

        /// \a raw shows the last observed value only. It does
        /// not deliver an average.
        ///
        /// Formula:  None. Shows raw data as collected.
        /// Average:  None
        /// Type:     Instantaneous
        raw,

        /// \a monotonically_increasing shows the cumulatively accumulated
        /// observed value. It does not deliver an average.
        ///
        /// Formula:  None. Shows cumulatively accumulated data as collected.
        /// Average:  None
        /// Type:     Instantaneous
        monotonically_increasing,

        /// \a average_base is used as the base data (denominator) in the
        /// computation of time or count averages for the
        /// \a counter_type::average_count and \a counter_type::average_timer
        /// counter types.
        /// This counter type collects the last observed value only.
        ///
        /// Formula:  None. This counter uses raw data in factional calculations
        /// without delivering an output.
        /// Average:  SUM (N) / x Type:
        /// Instantaneous
        average_base,

        /// \a average_count shows how many items are processed, on
        /// average, during an operation. Counters of this type display a ratio
        /// of the items processed (such as bytes sent) to the number of
        /// operations completed. The ratio is calculated by comparing the
        /// number of items processed during the last interval to the number of
        /// operations completed during the last interval.
        ///
        /// Formula:  (N1 - N0) / (D1 - D0), where the numerator (N) represents
        ///           the number of items processed during the last sample
        ///           interval, and the denominator (D) represents the number
        ///           of operations completed during the last two sample
        ///           intervals.
        /// Average:  (Nx - N0) / (Dx - D0)
        /// Type:     Average
        average_count,

        /// \a aggregating applies a function to an embedded counter
        /// instance. The embedded counter is usually evaluated repeatedly
        /// after a fixed (but configurable) time interval.
        ///
        /// Formula:  F(Nx)
        aggregating,

        /// \a average_timer measures the average time it takes to
        /// complete a process or operation. Counters of this type display a
        /// ratio of the total elapsed time of the sample interval to the
        /// number of processes or operations completed during that time. This
        /// counter type measures time in ticks of the system clock. The
        /// variable F represents the number of ticks per second. The value of
        /// F is factored into the equation so that the result is displayed in
        /// seconds.
        ///
        /// Formula:  ((N1 - N0) / F) / (D1 - D0), where the numerator (N)
        ///           represents the number of ticks counted during the last
        ///           sample interval, the variable F represents the frequency
        ///           of the ticks, and the denominator (D) represents the
        ///           number of operations completed during the last sample
        ///           interval.
        /// Average:  ((Nx - N0) / F) / (Dx - D0)
        /// Type:     Average
        average_timer,

        /// \a elapsed_time shows the total time between when the
        /// component or process started and the time when this value is
        /// calculated. The variable F represents the number of time units that
        /// elapse in one second. The value of F is factored into the equation
        /// so that the result is displayed in seconds.
        ///
        /// Formula:  (D0 - N0) / F, where the nominator (D) represents the
        ///           current time, the numerator (N) represents the time the
        ///           object was started, and the variable F represents the
        ///           number of time units that elapse in one second.
        /// Average:  (Dx - N0) / F
        /// Type:     Difference
        elapsed_time,

        /// \a histogram exposes a histogram of the measured values
        /// instead of a single value as many of the other counter types.
        /// Counters of this type expose a \a counter_value_array instead of a
        /// \a counter_value. Those will also not implement the
        /// \a get_counter_value() functionality. The results are exposed
        /// through a separate \a get_counter_values_array() function.
        ///
        /// The first three values in the returned array represent the lower
        /// and upper boundaries, and the size of the histogram buckets. All
        /// remaining values in the returned array represent the number of
        /// measurements for each of the buckets in the histogram.
        histogram,

        /// \a raw_values exposes an array of measured values
        /// instead of a single value as many of the other counter types.
        /// Counters of this type expose a \a counter_value_array instead of a
        /// \a counter_value. Those will also not implement the
        /// \a get_counter_value() functionality. The results are exposed
        /// through a separate \a get_counter_values_array() function.
        raw_values
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Status and error codes used by the functions related to
    ///        performance counters.
    enum class counter_status
    {
        valid_data,         ///< No error occurred, data is valid
        new_data,           ///< Data is valid and different from last call
        invalid_data,       ///< Some error occurred, data is not value
        already_defined,    ///< The type or instance already has been defined
        counter_unknown,    ///< The counter instance is unknown
        counter_type_unknown,    ///< The counter type is unknown
        generic_error            ///< A unknown error occurred
    };
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT inline constexpr char const counter_prefix[] = "/counters";
    HPX_CXX_EXPORT inline constexpr std::size_t counter_prefix_len =
        std::size(counter_prefix) - 1;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT inline bool status_is_valid(counter_status s)
    {
        return s == counter_status::valid_data || s == counter_status::new_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    /// A counter_type_path_elements holds the elements of a full name for a
    /// counter type. Generally, a full name of a counter type has the
    /// structure:
    ///
    ///    /objectname/countername
    ///
    /// i.e.
    ///    /queue/length
    ///
    HPX_CXX_EXPORT struct counter_type_path_elements
    {
        counter_type_path_elements() = default;

        counter_type_path_elements(std::string const& objectname,
            std::string const& countername, std::string const& parameters)
          : objectname_(objectname)
          , countername_(countername)
          , parameters_(parameters)
        {
        }

        std::string objectname_;     ///< the name of the performance object
        std::string countername_;    ///< contains the counter name
        std::string parameters_;     ///< optional parameters for the
                                     ///< counter instance

    protected:
        // serialization support
        friend class hpx::serialization::access;

        HPX_EXPORT void serialize(
            serialization::output_archive& ar, unsigned int) const;
        HPX_EXPORT void serialize(
            serialization::input_archive& ar, unsigned int);
    };

    ///////////////////////////////////////////////////////////////////////////
    /// A counter_path_elements holds the elements of a full name for a counter
    /// instance. Generally, a full name of a counter instance has the
    /// structure:
    ///
    ///    /objectname{parentinstancename#parentindex/instancename#instanceindex}
    ///      /countername#parameters
    ///
    /// i.e.
    ///    /queue{localityprefix/thread#2}/length
    ///
    HPX_CXX_EXPORT struct counter_path_elements : counter_type_path_elements
    {
        using base_type = counter_type_path_elements;

        counter_path_elements()
          : parentinstanceindex_(-1)
          , instanceindex_(-1)
          , subinstanceindex_(-1)
          , parentinstance_is_basename_(false)
        {
        }

        counter_path_elements(std::string const& objectname,
            std::string const& countername, std::string const& parameters,
            std::string const& parentname, std::string const& instancename,
            std::int64_t parentindex = -1, std::int64_t instanceindex = -1,
            bool parentinstance_is_basename = false)
          : counter_type_path_elements(objectname, countername, parameters)
          , parentinstancename_(parentname)
          , instancename_(instancename)
          , subinstancename_()
          , parentinstanceindex_(parentindex)
          , instanceindex_(instanceindex)
          , subinstanceindex_(-1)
          , parentinstance_is_basename_(parentinstance_is_basename)
        {
        }

        counter_path_elements(std::string const& objectname,
            std::string const& countername, std::string const& parameters,
            std::string const& parentname, std::string const& instancename,
            std::string const& subinstancename, std::int64_t parentindex = -1,
            std::int64_t instanceindex = -1, std::int64_t subinstanceindex = -1,
            bool parentinstance_is_basename = false)
          : counter_type_path_elements(objectname, countername, parameters)
          , parentinstancename_(parentname)
          , instancename_(instancename)
          , subinstancename_(subinstancename)
          , parentinstanceindex_(parentindex)
          , instanceindex_(instanceindex)
          , subinstanceindex_(subinstanceindex)
          , parentinstance_is_basename_(parentinstance_is_basename)
        {
        }

        std::string parentinstancename_;    ///< the name of the parent instance
        std::string instancename_;          ///< the name of the object instance
        std::string
            subinstancename_;    ///< the name of the object sub-instance
        std::int64_t parentinstanceindex_;    ///< the parent instance index
        std::int64_t instanceindex_;          ///< the instance index
        std::int64_t subinstanceindex_;       ///< the sub-instance index
        bool parentinstance_is_basename_;     ///< the parentinstancename_
        ///member holds a base counter name

    private:
        // serialization support
        friend class hpx::serialization::access;

        HPX_EXPORT void serialize(
            serialization::output_archive& ar, unsigned int);
        HPX_EXPORT void serialize(
            serialization::input_archive& ar, unsigned int);
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT struct counter_info
    {
        explicit counter_info(counter_type type = counter_type::raw)
          : type_(type)
          , version_(HPX_PERFORMANCE_COUNTER_V1)
          , status_(counter_status::invalid_data)
        {
        }

        explicit counter_info(std::string const& name)
          : type_(counter_type::raw)
          , version_(HPX_PERFORMANCE_COUNTER_V1)
          , status_(counter_status::invalid_data)
          , fullname_(name)
        {
        }

        counter_info(counter_type type, std::string const& name,
            std::string const& helptext = "",
            std::uint32_t version = HPX_PERFORMANCE_COUNTER_V1,
            std::string const& uom = "")
          : type_(type)
          , version_(version)
          , status_(counter_status::invalid_data)
          , fullname_(name)
          , helptext_(helptext)
          , unit_of_measure_(uom)
        {
        }

        counter_type type_;        ///< The type of the described counter
        std::uint32_t version_;    ///< The version of the described counter
                                   ///< using the 0xMMmmSSSS scheme
        counter_status status_;    ///< The status of the counter object
        std::string fullname_;     ///< The full name of this counter
        std::string
            helptext_;    ///< The full descriptive text for this counter
        std::string
            unit_of_measure_;    ///< The unit of measure for this counter

    private:
        // serialization support
        friend class hpx::serialization::access;

        HPX_EXPORT void serialize(
            serialization::output_archive& ar, unsigned int) const;
        HPX_EXPORT void serialize(
            serialization::input_archive& ar, unsigned int);
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \brief This declares the type of a function, which will be
    ///        called by HPX whenever a new performance counter instance of a
    ///        particular type needs to be created.
    HPX_CXX_EXPORT using create_counter_func =
        hpx::function<naming::gid_type(counter_info const&, error_code&)>;

    ///////////////////////////////////////////////////////////////////////////
    /// \brief This declares a type of a function, which will be passed to
    ///        a \a discover_counters_func in order to be called for each
    ///        discovered performance counter instance.
    HPX_CXX_EXPORT using discover_counter_func =
        hpx::function<bool(counter_info const&, error_code&)>;

    /// \brief This declares the type of a function, which will be called by
    ///        HPX whenever it needs to discover all performance counter
    ///        instances of a particular type.
    HPX_CXX_EXPORT using discover_counters_func =
        hpx::function<bool(counter_info const&, discover_counter_func const&,
            discover_counters_mode, error_code&)>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT struct counter_value
    {
        counter_value(std::int64_t value = 0, std::int64_t scaling = 1,
            bool scale_inverse = false)
          : time_()
          , count_(0)
          , value_(value)
          , scaling_(scaling)
          , status_(counter_status::new_data)
          , scale_inverse_(scale_inverse)
        {
        }

        std::uint64_t time_;       ///< The local time when data was collected
        std::uint64_t count_;      ///< The invocation counter for the data
        std::int64_t value_;       ///< The current counter value
        std::int64_t scaling_;     ///< The scaling of the current counter value
        counter_status status_;    ///< The status of the counter value
        bool scale_inverse_;       ///< If true, value_ needs to be divided by
                                   ///< scaling_, otherwise it has to be
                                   ///< multiplied.

        /// \brief Retrieve the 'real' value of the counter_value, converted to
        ///        the requested type \a T
        template <typename T>
        T get_value(error_code& ec = throws) const
        {
            if (!status_is_valid(status_))
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status,
                    "counter_value::get_value<T>",
                    "counter value is in invalid status");
                return T();
            }

            T val = static_cast<T>(value_);

            if (scaling_ != 1)
            {
                if (scaling_ == 0)
                {
                    HPX_THROWS_IF(ec, hpx::error::uninitialized_value,
                        "counter_value::get_value<T>",
                        "scaling should not be zero");
                    return T();
                }

                // calculate and return the real counter value
                if (scale_inverse_)
                    return val / static_cast<T>(scaling_);

                return val * static_cast<T>(scaling_);
            }
            return val;
        }

    private:
        // serialization support
        friend class hpx::serialization::access;

        HPX_EXPORT void serialize(
            serialization::output_archive& ar, unsigned int const) const;
        HPX_EXPORT void serialize(
            serialization::input_archive& ar, unsigned int const);
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT struct counter_values_array
    {
        counter_values_array(
            std::int64_t scaling = 1, bool scale_inverse = false)
          : time_()
          , count_(0)
          , values_()
          , scaling_(scaling)
          , status_(counter_status::new_data)
          , scale_inverse_(scale_inverse)
        {
        }

        counter_values_array(std::vector<std::int64_t>&& values,
            std::int64_t scaling = 1, bool scale_inverse = false)
          : time_()
          , count_(0)
          , values_(HPX_MOVE(values))
          , scaling_(scaling)
          , status_(counter_status::new_data)
          , scale_inverse_(scale_inverse)
        {
        }

        counter_values_array(std::vector<std::int64_t> const& values,
            std::int64_t scaling = 1, bool scale_inverse = false)
          : time_()
          , count_(0)
          , values_(values)
          , scaling_(scaling)
          , status_(counter_status::new_data)
          , scale_inverse_(scale_inverse)
        {
        }

        std::uint64_t time_;     ///< The local time when data was collected
        std::uint64_t count_;    ///< The invocation counter for the data
        std::vector<std::int64_t> values_;    ///< The current counter values
        std::int64_t scaling_;    ///< The scaling of the current counter values
        counter_status status_;    ///< The status of the counter value
        bool scale_inverse_;       ///< If true, value_ needs to be divided by
                                   ///< scaling_, otherwise it has to be
                                   ///< multiplied.

        /// \brief Retrieve the 'real' value of the counter_value, converted to
        ///        the requested type \a T
        template <typename T>
        T get_value(std::size_t index, error_code& ec = throws) const
        {
            if (!status_is_valid(status_))
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status,
                    "counter_values_array::get_value<T>",
                    "counter value is in invalid status");
                return T();
            }
            if (index >= values_.size())
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "counter_values_array::get_value<T>",
                    "index out of bounds");
                return T();
            }

            T val = static_cast<T>(values_[index]);

            if (scaling_ != 1)
            {
                if (scaling_ == 0)
                {
                    HPX_THROWS_IF(ec, hpx::error::uninitialized_value,
                        "counter_values_array::get_value<T>",
                        "scaling should not be zero");
                    return T();
                }

                // calculate and return the real counter value
                if (scale_inverse_)
                    return val / static_cast<T>(scaling_);

                return val * static_cast<T>(scaling_);
            }
            return val;
        }

    private:
        // serialization support
        friend class hpx::serialization::access;

        HPX_EXPORT void serialize(
            serialization::output_archive& ar, unsigned int const) const;
        HPX_EXPORT void serialize(
            serialization::input_archive& ar, unsigned int const);
    };
}    // namespace hpx::performance_counters

#include <hpx/config/warnings_suffix.hpp>
