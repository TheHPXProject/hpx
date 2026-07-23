//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/errors/error_code.hpp>
#include <hpx/errors/exception.hpp>

#include <exception>
#include <string>
#include <system_error>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    /// \cond NOINTERNAL
    inline constexpr char const* const error_names[] = {
        /*  0 */ "success",
        /*  1 */ "no_success",
        /*  2 */ "not_implemented",
        /*  3 */ "out_of_memory",
        /*  4 */ "bad_action_code",
        /*  5 */ "bad_component_type",
        /*  6 */ "network_error",
        /*  7 */ "version_too_new",
        /*  8 */ "version_too_old",
        /*  9 */ "version_unknown",
        /* 10 */ "unknown_component_address",
        /* 11 */ "duplicate_component_address",
        /* 12 */ "invalid_status",
        /* 13 */ "bad_parameter",
        /* 14 */ "internal_server_error",
        /* 15 */ "service_unavailable",
        /* 16 */ "bad_request",
        /* 17 */ "repeated_request",
        /* 18 */ "lock_error",
        /* 19 */ "duplicate_console",
        /* 20 */ "no_registered_console",
        /* 21 */ "startup_timed_out",
        /* 22 */ "uninitialized_value",
        /* 23 */ "bad_response_type",
        /* 24 */ "deadlock",
        /* 25 */ "assertion_failure",
        /* 26 */ "null_thread_id",
        /* 27 */ "invalid_data",
        /* 28 */ "yield_aborted",
        /* 29 */ "dynamic_link_failure",
        /* 30 */ "commandline_option_error",
        /* 31 */ "serialization_error",
        /* 32 */ "unhandled_exception",
        /* 33 */ "kernel_error",
        /* 34 */ "broken_task",
        /* 35 */ "task_moved",
        /* 36 */ "task_already_started",
        /* 37 */ "future_already_retrieved",
        /* 38 */ "promise_already_satisfied",
        /* 39 */ "future_does_not_support_cancellation",
        /* 40 */ "future_can_not_be_cancelled",
        /* 41 */ "no_state",
        /* 42 */ "broken_promise",
        /* 43 */ "thread_resource_error",
        /* 44 */ "future_cancelled",
        /* 45 */ "thread_cancelled",
        /* 46 */ "thread_not_interruptable",
        /* 47 */ "duplicate_component_id",
        /* 48 */ "unknown_error",
        /* 49 */ "bad_plugin_type",
        /* 50 */ "filesystem_error",
        /* 51 */ "bad_function_call",
        /* 52 */ "task_canceled_exception",
        /* 53 */ "task_block_not_active",
        /* 54 */ "out_of_range",
        /* 55 */ "length_error",
        /* 56 */ "migration_needs_retry",
        /* 57 */ "stale_state",

        /*    */ ""};
    /// \endcond

    // Return a textual representation of a given error code
    char const* get_error_name(error value) noexcept
    {
        if (value >= hpx::error::success && value < hpx::error::last_error)
        {
            return error_names[static_cast<int>(value)];
        }
        if (value & hpx::error::system_error_flag)
        {
            return "system_error";
        }
        return "unknown";
    }

    namespace detail {

        class hpx_category : public std::error_category
        {
        public:
            [[nodiscard]] char const* name() const noexcept override
            {
                return "HPX";
            }

            [[nodiscard]] std::string message(int value) const override
            {
                if (value >= hpx::error::success &&
                    value < hpx::error::last_error)
                {
                    return std::string("HPX(") + error_names[value] + ")";
                }
                if (value & hpx::error::system_error_flag)
                {
                    return "HPX(system_error)";
                }
                return "HPX(unknown_error)";
            }
        };

        struct lightweight_hpx_category final : hpx_category
        {
        };

        // this doesn't add any text to the exception what() message
        class hpx_category_rethrow : public std::error_category
        {
        public:
            [[nodiscard]] char const* name() const noexcept override
            {
                return "";
            }

            [[nodiscard]] std::string message(int) const noexcept override
            {
                return {};
            }
        };

        struct lightweight_hpx_category_rethrow final : hpx_category_rethrow
        {
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    std::error_category const& get_hpx_category() noexcept
    {
        static detail::hpx_category hpx_category;
        return hpx_category;
    }

    std::error_category const& get_hpx_rethrow_category() noexcept
    {
        static detail::hpx_category_rethrow hpx_category_rethrow;
        return hpx_category_rethrow;
    }

    std::error_category const& get_lightweight_hpx_category() noexcept
    {
        static detail::lightweight_hpx_category lightweight_hpx_category;
        return lightweight_hpx_category;
    }

    std::error_category const& get_lightweight_hpx_rethrow_category() noexcept
    {
        static detail::lightweight_hpx_category_rethrow
            lightweight_hpx_category_rethrow;
        return lightweight_hpx_category_rethrow;
    }

    std::error_category const& get_hpx_category(throwmode mode) noexcept
    {
        switch (mode)
        {
        case throwmode::rethrow:
            return get_hpx_rethrow_category();

        case throwmode::lightweight:
            return get_lightweight_hpx_category();

        case throwmode::lightweight_rethrow:
            return get_lightweight_hpx_rethrow_category();

        case throwmode::plain:
            break;
        }
        return get_hpx_category();
    }

    throwmode get_throwmode(error_code const& ec)
    {
        std::error_category const& category = ec.category();
        if (category == get_lightweight_hpx_rethrow_category())
        {
            return throwmode::lightweight_rethrow;
        }
        if (category == get_lightweight_hpx_category())
        {
            return throwmode::lightweight;
        }
        if (category == get_hpx_rethrow_category())
        {
            return throwmode::rethrow;
        }
        return throwmode::plain;
    }

    ///////////////////////////////////////////////////////////////////////////
    error_code::error_code(error e, throwmode mode)
      : std::error_code(make_system_error_code(e, mode))
    {
        if (e != hpx::error::success && e != hpx::error::no_success &&
            !(mode & throwmode::lightweight))
        {
            exception_ = detail::get_exception(e, "", mode);
        }
    }

    error_code::error_code(
        error e, char const* func, char const* file, long line, throwmode mode)
      : std::error_code(make_system_error_code(e, mode))
    {
        if (e != hpx::error::success && e != hpx::error::no_success &&
            !(mode & throwmode::lightweight))
        {
            exception_ = detail::get_exception(e, "", mode, func, file, line);
        }
    }

    error_code::error_code(error e, char const* msg, throwmode mode)
      : std::error_code(make_system_error_code(e, mode))
    {
        if (e != hpx::error::success && e != hpx::error::no_success &&
            !(mode & throwmode::lightweight))
        {
            exception_ = detail::get_exception(e, msg, mode);
        }
    }

    error_code::error_code(error e, char const* msg, char const* func,
        char const* file, long line, throwmode mode)
      : std::error_code(make_system_error_code(e, mode))
    {
        if (e != hpx::error::success && e != hpx::error::no_success &&
            !(mode & throwmode::lightweight))
        {
            exception_ = detail::get_exception(e, msg, mode, func, file, line);
        }
    }

    error_code::error_code(error e, std::string const& msg, throwmode mode)
      : std::error_code(make_system_error_code(e, mode))
    {
        if (e != hpx::error::success && e != hpx::error::no_success &&
            !(mode & throwmode::lightweight))
        {
            exception_ = detail::get_exception(e, msg, mode);
        }
    }

    error_code::error_code(error e, std::string const& msg, char const* func,
        char const* file, long line, throwmode mode)
      : std::error_code(make_system_error_code(e, mode))
    {
        if (e != hpx::error::success && e != hpx::error::no_success &&
            !(mode & throwmode::lightweight))
        {
            exception_ = detail::get_exception(e, msg, mode, func, file, line);
        }
    }

    error_code::error_code(int err, hpx::exception const& e)
    {
        this->std::error_code::assign(err, get_hpx_category());
        exception_ = std::make_exception_ptr(e);
    }

    error_code::error_code(std::exception_ptr const& e)
      : std::error_code(
            make_system_error_code(get_error(e), throwmode::rethrow))
      , exception_(e)
    {
    }

    void error_code::assign(
        error e, std::string const& msg, throwmode const mode)
    {
        exception_ = {};

        this->std::error_code::assign(
            static_cast<int>(e), get_hpx_category(mode));

        if (e != hpx::error::success && e != hpx::error::no_success &&
            !(mode & throwmode::lightweight))
        {
            exception_ = detail::get_exception(e, msg, mode);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    std::string error_code::get_message() const
    {
        if (exception_)
        {
            try
            {
                std::rethrow_exception(exception_);
            }
            catch (std::exception const& be)
            {
                return be.what();
            }
        }
        return get_error_what(*this);    // provide at least minimal error text
    }

    ///////////////////////////////////////////////////////////////////////////
    error_code::error_code(error_code const& rhs)
      : std::error_code(rhs.value() == hpx::error::success ?
                static_cast<std::error_code const&>(make_success_code(
                    (rhs.category() == get_lightweight_hpx_rethrow_category()) ?
                        hpx::throwmode::lightweight_rethrow :
                        (rhs.category() == get_lightweight_hpx_category()) ?
                        hpx::throwmode::lightweight :
                        hpx::throwmode::plain)) :
                static_cast<std::error_code const&>(rhs))
      , exception_(rhs.exception_)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    error_code& error_code::operator=(error_code const& rhs)
    {
        if (this != &rhs)
        {
            if (rhs.value() == hpx::error::success)
            {
                // if the rhs is a success code, we maintain our throw mode
                this->std::error_code::operator=(make_success_code(
                    (category() == get_lightweight_hpx_rethrow_category()) ?
                        hpx::throwmode::lightweight_rethrow :
                        (category() == get_lightweight_hpx_category()) ?
                        hpx::throwmode::lightweight :
                        hpx::throwmode::plain));
            }
            else
            {
                this->std::error_code::operator=(rhs);
            }
            exception_ = rhs.exception_;
        }
        return *this;
    }
    /// \endcond
}    // namespace hpx
