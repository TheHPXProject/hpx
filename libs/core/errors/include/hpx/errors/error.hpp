//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//  Copyright (c) 2014      Anuj R. Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file error.hpp

#pragma once

#include <hpx/config.hpp>

#include <cstdint>
#include <system_error>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Possible error conditions
    ///
    /// This enumeration lists all possible error conditions which can be
    /// reported from any of the API functions.
    HPX_CXX_CORE_EXPORT enum class error : std::int16_t {
        success = 0,
        ///< The operation was successful
        no_success = 1,
        ///< The operation did failed, but not in an unexpected manner
        not_implemented = 2,
        ///< The operation is not implemented
        out_of_memory = 3,
        ///< The operation caused an out of memory condition
        bad_action_code = 4,
        ///<
        bad_component_type = 5,
        ///< The specified component type is not known or otherwise invalid
        network_error = 6,
        ///< A generic network error occurred
        version_too_new = 7,
        ///< The version of the network representation for this object is too new
        version_too_old = 8,
        ///< The version of the network representation for this object is too old
        version_unknown = 9,
        ///< The version of the network representation for this object is unknown
        unknown_component_address = 10,
        ///<
        duplicate_component_address = 11,
        ///< The given global id has already been registered
        invalid_status = 12,
        ///< The operation was executed in an invalid status
        bad_parameter = 13,
        ///< One of the supplied parameters is invalid
        internal_server_error = 14,
        ///<
        service_unavailable = 15,
        ///<
        bad_request = 16,
        ///<
        repeated_request = 17,
        ///<
        lock_error = 18,
        ///<
        duplicate_console = 19,
        ///< There is more than one console locality
        no_registered_console = 20,
        ///< There is no registered console locality available
        startup_timed_out = 21,
        ///<
        uninitialized_value = 22,
        ///<
        bad_response_type = 23,
        ///<
        deadlock = 24,
        ///<
        assertion_failure = 25,
        ///<
        null_thread_id = 26,
        ///< Attempt to invoke an API function from a non-HPX thread
        invalid_data = 27,
        ///<
        yield_aborted = 28,
        ///< The yield operation was aborted
        dynamic_link_failure = 29,
        ///<
        commandline_option_error = 30,
        ///< One of the options given on the command line is erroneous
        serialization_error = 31,
        ///< There was an error during serialization of this object
        unhandled_exception = 32,
        ///< An unhandled exception has been caught
        kernel_error = 33,
        ///< The OS kernel reported an error
        broken_task = 34,
        ///< The task associated with this future object is not available anymore
        task_moved = 35,
        ///< The task associated with this future object has been moved
        task_already_started = 36,
        ///< The task associated with this future object has already been started
        future_already_retrieved = 37,
        ///< The future object has already been retrieved
        promise_already_satisfied = 38,
        ///< The value for this future object has already been set
        future_does_not_support_cancellation = 39,
        ///< The future object does not support cancellation
        future_can_not_be_cancelled = 40,
        ///< The future can't be canceled at this time
        no_state = 41,
        ///< The future object has no valid shared state
        broken_promise = 42,
        ///< The promise has been deleted
        thread_resource_error = 43,
        ///<
        future_cancelled = 44,
        ///<
        thread_cancelled = 45,
        ///<
        thread_not_interruptable = 46,
        ///<
        duplicate_component_id = 47,
        ///< The component type has already been registered
        unknown_error = 48,
        ///< An unknown error occurred
        bad_plugin_type = 49,
        ///< The specified plugin type is not known or otherwise invalid
        filesystem_error = 50,
        ///< The specified file does not exist or other filesystem related error
        bad_function_call = 51,
        ///< equivalent of std::bad_function_call
        task_canceled_exception = 52,
        ///< parallel::task_canceled_exception
        task_block_not_active = 53,
        ///< task_region is not active
        out_of_range = 54,
        ///< Equivalent to std::out_of_range
        length_error = 55,
        ///< Equivalent to std::length_error

        migration_needs_retry = 56,    ///< migration failed because of global
                                       ///< race, retry

        stale_state = 57,    ///< The queried state may be stale, e.g. because
                             ///< no state has been recorded yet or a
                             ///< preceding notification has not yet been
                             ///< delivered

        /// \cond NOINTERNAL
        last_error = 58,

        system_error_flag = 0x4000L,

        // force this enum type to be at least 16 bits.
        error_upper_bound = 0x7fffL
        /// \endcond
    };

    HPX_CXX_CORE_EXPORT constexpr bool operator==(int lhs, error rhs) noexcept
    {
        return lhs == static_cast<int>(rhs);
    }

    HPX_CXX_CORE_EXPORT constexpr bool operator==(error lhs, int rhs) noexcept
    {
        return static_cast<int>(lhs) == rhs;
    }

    HPX_CXX_CORE_EXPORT constexpr bool operator!=(int lhs, error rhs) noexcept
    {
        return !(lhs == rhs);
    }

    HPX_CXX_CORE_EXPORT constexpr bool operator!=(error lhs, int rhs) noexcept
    {
        return !(lhs == rhs);
    }

    HPX_CXX_CORE_EXPORT constexpr bool operator<(int lhs, error rhs) noexcept
    {
        return lhs < static_cast<int>(rhs);
    }

    HPX_CXX_CORE_EXPORT constexpr bool operator>=(int lhs, error rhs) noexcept
    {
        return !(lhs < rhs);
    }

    HPX_CXX_CORE_EXPORT constexpr int operator&(error lhs, error rhs) noexcept
    {
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }

    HPX_CXX_CORE_EXPORT constexpr int operator&(int lhs, error rhs) noexcept
    {
        return lhs & static_cast<int>(rhs);
    }

    HPX_CXX_CORE_EXPORT constexpr int operator|=(int& lhs, error rhs) noexcept
    {
        lhs = lhs | static_cast<int>(rhs);
        return lhs;
    }

#define HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG                                \
    "The unscoped hpx::error names are deprecated. Please use "                \
    "hpx::error::<value> instead."

    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error success = error::success;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error no_success = error::no_success;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error not_implemented = error::not_implemented;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error out_of_memory = error::out_of_memory;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error bad_action_code = error::bad_action_code;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error bad_component_type = error::bad_component_type;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error network_error = error::network_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error version_too_new = error::version_too_new;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error version_too_old = error::version_too_old;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error version_unknown = error::version_unknown;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error unknown_component_address =
        error::unknown_component_address;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error duplicate_component_address =
        error::duplicate_component_address;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error invalid_status = error::invalid_status;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error bad_parameter = error::bad_parameter;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error internal_server_error = error::internal_server_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error service_unavailable = error::service_unavailable;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error bad_request = error::bad_request;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error repeated_request = error::repeated_request;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error lock_error = error::lock_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error duplicate_console = error::duplicate_console;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error no_registered_console = error::no_registered_console;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error startup_timed_out = error::startup_timed_out;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error uninitialized_value = error::uninitialized_value;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error bad_response_type = error::bad_response_type;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error deadlock = error::deadlock;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error assertion_failure = error::assertion_failure;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error null_thread_id = error::null_thread_id;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error invalid_data = error::invalid_data;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error yield_aborted = error::yield_aborted;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error dynamic_link_failure = error::dynamic_link_failure;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error commandline_option_error =
        error::commandline_option_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error serialization_error = error::serialization_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error unhandled_exception = error::unhandled_exception;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error kernel_error = error::kernel_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error broken_task = error::broken_task;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error task_moved = error::task_moved;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error task_already_started = error::task_already_started;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error future_already_retrieved =
        error::future_already_retrieved;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error promise_already_satisfied =
        error::promise_already_satisfied;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error future_does_not_support_cancellation =
        error::future_does_not_support_cancellation;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error future_can_not_be_cancelled =
        error::future_can_not_be_cancelled;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error no_state = error::no_state;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error broken_promise = error::broken_promise;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error thread_resource_error = error::thread_resource_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error future_cancelled = error::future_cancelled;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error thread_cancelled = error::thread_cancelled;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error thread_not_interruptable =
        error::thread_not_interruptable;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error duplicate_component_id =
        error::duplicate_component_id;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error unknown_error = error::unknown_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error bad_plugin_type = error::bad_plugin_type;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error filesystem_error = error::filesystem_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error bad_function_call = error::bad_function_call;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error task_canceled_exception =
        error::task_canceled_exception;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error task_block_not_active = error::task_block_not_active;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error out_of_range = error::out_of_range;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error length_error = error::length_error;
    HPX_DEPRECATED_V(1, 9, HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG)
    inline constexpr error migration_needs_retry = error::migration_needs_retry;

#undef HPX_ERROR_UNSCOPED_ENUM_DEPRECATION_MSG

    // Return a textual representation of a given error code
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* get_error_name(
        error e) noexcept;

}    // namespace hpx

/// \cond NOEXTERNAL
namespace std {

    // make sure our errors get recognized by the Boost.System library
    template <>
    struct is_error_code_enum<hpx::error>
    {
        static constexpr bool value = true;
    };

    template <>
    struct is_error_condition_enum<hpx::error>
    {
        static constexpr bool value = true;
    };
}    // namespace std
/// \endcond
