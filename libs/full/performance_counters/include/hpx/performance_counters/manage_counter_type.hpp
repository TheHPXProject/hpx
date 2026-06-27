////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

/// \file manage_counter_type.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/performance_counters/counters_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace hpx::performance_counters {

    /// \brief Install a new generic performance counter type in a way, which
    ///        will uninstall it automatically during shutdown.
    ///
    /// The function \a install_counter_type will register a new generic
    /// counter type based on the provided function. The counter
    /// type will be automatically unregistered during system shutdown. Any
    /// consumer querying any instance of this this counter type will cause the
    /// provided function to be called and the returned value to be exposed as
    /// the counter value.
    ///
    /// The counter type is registered such that there can be one counter
    /// instance per locality. The expected naming scheme for the counter
    /// instances is: \c '/objectname{locality#<*>/total}/countername' where
    /// '<*>' is a zero based integer identifying the locality the counter
    /// is created on.
    ///
    /// \param name   [in] The global virtual name of the counter type. This
    ///               name is expected to have the format /objectname/countername.
    /// \param counter_value [in] The function to call whenever the counter
    ///               value is requested by a consumer.
    /// \param helptext [in, optional] A longer descriptive text shown to the
    ///               user to explain the nature of the counters created from
    ///               this type.
    /// \param uom    [in] The unit of measure for the new performance counter
    ///               type.
    /// \param type   [in] Type for the new performance counter type.
    /// \param ec     [in,out] this represents the error status on exit,
    ///               if this is pre-initialized to \a hpx#throws
    ///               the function will throw on error instead.
    ///
    /// \note As long as \a ec is not pre-initialized to \a hpx::throws this
    ///       function doesn't throw but returns the result code using the
    ///       parameter \a ec. Otherwise it throws an instance of hpx::exception.
    ///
    /// \returns      If successful, this function returns \a valid_data,
    ///               otherwise it will either throw an exception or return an
    ///               error_code from the enum \a counter_status (also, see
    ///               note related to parameter \a ec).
    ///
    /// \note The counter type registry is a locality based service. You will
    ///       have to register each counter type on every locality where a
    ///       corresponding performance counter will be created.
    HPX_CXX_EXPORT HPX_EXPORT counter_status install_counter_type(
        std::string const& name,
        hpx::function<std::int64_t(bool)> const& counter_value,
        std::string const& helptext = "", std::string const& uom = "",
        counter_type type = counter_type::raw, error_code& ec = throws);

    /// Install a new generic performance counter type returning an
    /// array of values in a way, that will uninstall it automatically during
    /// shutdown.
    ///
    /// The function \a install_counter_type will register a new generic
    /// counter type that returns an array of values based on the provided
    /// function. The counter
    /// type will be automatically unregistered during system shutdown. Any
    /// consumer querying any instance of this this counter type will cause the
    /// provided function to be called and the returned array value to be
    /// exposed as the counter value.
    ///
    /// The counter type is registered such that there can be one counter
    /// instance per locality. The expected naming scheme for the counter
    /// instances is: \c '/objectname{locality#<*>/total}/countername' where
    /// '<*>' is a zero based integer identifying the locality the counter
    /// is created on.
    ///
    /// \param name   [in] The global virtual name of the counter type. This
    ///               name is expected to have the format /objectname/countername.
    /// \param counter_value [in] The function to call whenever the counter
    ///               value (array of values) is requested by a consumer.
    /// \param helptext [in, optional] A longer descriptive text shown to the
    ///               user to explain the nature of the counters created from
    ///               this type.
    /// \param uom    [in] The unit of measure for the new performance counter
    ///               type.
    /// \param ec     [in,out] this represents the error status on exit,
    ///               if this is pre-initialized to \a hpx#throws
    ///               the function will throw on error instead.
    ///
    /// \note As long as \a ec is not pre-initialized to \a hpx::throws this
    ///       function doesn't throw but returns the result code using the
    ///       parameter \a ec. Otherwise it throws an instance of hpx::exception.
    ///
    /// \returns      If successful, this function returns \a valid_data,
    ///               otherwise it will either throw an exception or return an
    ///               error_code from the enum \a counter_status (also, see
    ///               note related to parameter \a ec).
    ///
    /// \note The counter type registry is a locality based service. You will
    ///       have to register each counter type on every locality where a
    ///       corresponding performance counter will be created.
    HPX_CXX_EXPORT HPX_EXPORT counter_status install_counter_type(
        std::string const& name,
        hpx::function<std::vector<std::int64_t>(bool)> const& counter_value,
        std::string const& helptext = "", std::string const& uom = "",
        error_code& ec = throws);

    /// \brief Install a new performance counter type in a way, which will
    ///        uninstall it automatically during shutdown.
    ///
    /// The function \a install_counter_type will register a new counter type
    /// based on the provided \a counter_type_info. The counter type will be
    /// automatically unregistered during system shutdown.
    ///
    /// \param name   [in] The global virtual name of the counter type. This
    ///               name is expected to have the format /objectname/countername.
    /// \param type   [in] The type of the counters of  this counter_type.
    /// \param ec     [in,out] this represents the error status on exit,
    ///               if this is pre-initialized to \a hpx#throws
    ///               the function will throw on error instead.
    ///
    /// \returns      If successful, this function returns \a valid_data,
    ///               otherwise it will either throw an exception or return an
    ///               error_code from the enum \a counter_status (also, see
    ///               note related to parameter \a ec).
    ///
    /// \note The counter type registry is a locality based service. You will
    ///       have to register each counter type on every locality where a
    ///       corresponding performance counter will be created.
    ///
    /// \note As long as \a ec is not pre-initialized to \a hpx#throws this
    ///       function doesn't throw but returns the result code using the
    ///       parameter \a ec. Otherwise it throws an instance of hpx#exception.
    HPX_CXX_EXPORT HPX_EXPORT void install_counter_type(
        std::string const& name, counter_type type, error_code& ec = throws);

    /// \brief Install a new performance counter type in a way, which will
    ///        uninstall it automatically during shutdown.
    ///
    /// The function \a install_counter_type will register a new counter type
    /// based on the provided \a counter_type_info. The counter type will be
    /// automatically unregistered during system shutdown.
    ///
    /// \param name   [in] The global virtual name of the counter type. This
    ///               name is expected to have the format /objectname/countername.
    /// \param type   [in] The type of the counters of  this counter_type.
    /// \param helptext [in] A longer descriptive  text shown to the user to
    ///               explain the nature of the counters created from this
    ///               type.
    /// \param uom    [in] The unit of measure for the new performance counter
    ///               type.
    /// \param version [in] The version of the counter type. This is currently
    ///               expected to be set to HPX_PERFORMANCE_COUNTER_V1.
    /// \param ec     [in,out] this represents the error status on exit,
    ///               if this is pre-initialized to \a hpx#throws
    ///               the function will throw on error instead.
    ///
    /// \returns      If successful, this function returns \a valid_data,
    ///               otherwise it will either throw an exception or return an
    ///               error_code from the enum \a counter_status (also, see
    ///               note related to parameter \a ec).
    ///
    /// \note The counter type registry is a locality based service. You will
    ///       have to register each counter type on every locality where a
    ///       corresponding performance counter will be created.
    ///
    /// \note As long as \a ec is not pre-initialized to \a hpx#throws this
    ///       function doesn't throw but returns the result code using the
    ///       parameter \a ec. Otherwise it throws an instance of hpx#exception.
    HPX_CXX_EXPORT HPX_EXPORT counter_status install_counter_type(
        std::string const& name, counter_type type, std::string const& helptext,
        std::string const& uom = "",
        std::uint32_t version = HPX_PERFORMANCE_COUNTER_V1,
        error_code& ec = throws);

    /// \brief Install a new generic performance counter type in a way, which
    ///        will uninstall it automatically during shutdown.
    ///
    /// The function \a install_counter_type will register a new generic
    /// counter type based on the provided \a counter_type_info. The counter
    /// type will be automatically unregistered during system shutdown.
    ///
    /// \param name   [in] The global virtual name of the counter type. This
    ///               name is expected to have the format /objectname/countername.
    /// \param type   [in] The type of the counters of  this counter_type.
    /// \param helptext [in] A longer descriptive  text shown to the user to
    ///               explain the nature of the counters created from this
    ///               type.
    /// \param version [in] The version of the counter type. This is currently
    ///               expected to be set to HPX_PERFORMANCE_COUNTER_V1.
    /// \param create_counter [in] The function which will be called to create
    ///               a new instance of this counter type.
    /// \param discover_counters [in] The function will be called to discover
    ///               counter instances which can be created.
    /// \param uom    [in] The unit of measure of the counter type (default: "")
    /// \param ec     [in,out] this represents the error status on exit,
    ///               if this is pre-initialized to \a hpx#throws
    ///               the function will throw on error instead.
    ///
    /// \note As long as \a ec is not pre-initialized to \a hpx#throws this
    ///       function doesn't throw but returns the result code using the
    ///       parameter \a ec. Otherwise it throws an instance of hpx#exception.
    ///
    /// \returns      If successful, this function returns \a valid_data,
    ///               otherwise it will either throw an exception or return an
    ///               error_code from the enum \a counter_status (also, see
    ///               note related to parameter \a ec).
    ///
    /// \note The counter type registry is a locality based service. You will
    ///       have to register each counter type on every locality where a
    ///       corresponding performance counter will be created.
    HPX_CXX_EXPORT HPX_EXPORT counter_status install_counter_type(
        std::string const& name, counter_type type, std::string const& helptext,
        create_counter_func const& create_counter,
        discover_counters_func const& discover_counters,
        std::uint32_t version = HPX_PERFORMANCE_COUNTER_V1,
        std::string const& uom = "", error_code& ec = throws);

    /// \cond NOINTERNAL

    /// A small data structure holding all data needed to install a counter type
    HPX_CXX_EXPORT struct generic_counter_type_data
    {
        std::string name_;         ///< Name of the counter type
        counter_type type_;        ///< Type of the counter instances of this
                                   ///< counter type
        std::string helptext_;     ///< Longer descriptive text explaining the
                                   ///< counter type
        std::uint32_t version_;    ///< Version of this counter type definition
                                   ///< (default: HPX_PERFORMANCE_COUNTER_V1)
        create_counter_func create_counter_;
        ///< Function used to create a counter instance of this type.
        discover_counters_func discover_counters_;
        ///< Function used to discover all supported counter instances of
        ///< this type.
        std::string unit_of_measure_;
        ///< The textual representation of the unit of measure for counter
        ///< instances of this type. Use ISO unit names.
    };

    /// Install several new performance counter types in a way, which will
    /// uninstall them automatically during shutdown.
    HPX_CXX_EXPORT HPX_EXPORT void install_counter_types(
        generic_counter_type_data const* data, std::size_t count,
        error_code& ec = throws);

    /// \endcond
}    // namespace hpx::performance_counters
