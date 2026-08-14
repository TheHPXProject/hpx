//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/runtime_local/custom_exception_info.hpp>
#include <hpx/runtime_local/detail/serialize_exception.hpp>

#if ASIO_HAS_BOOST_THROW_EXCEPTION != 0
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/exception.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>
#include <typeinfo>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::runtime_local::detail {

    ///////////////////////////////////////////////////////////////////////////
    void save_custom_exception(hpx::serialization::output_archive& ar,
        std::exception_ptr const& ep, unsigned int /* version */)
    {
        hpx::util::exception_type type =
            hpx::util::exception_type::unknown_exception;
        std::string what;
        hpx::error err_value = hpx::error::success;
        std::string err_message;

        std::uint32_t throw_locality_ = 0;
        std::string throw_hostname_;
        std::int64_t throw_pid_ = -1;
        std::size_t throw_shepherd_ = 0;
        std::size_t throw_thread_id_ = 0;
        std::string throw_thread_name_;
        std::string throw_function_;
        std::string throw_file_;
        std::string throw_back_trace_;
        std::int64_t throw_line_ = 0;
        std::string throw_env_;
        std::string throw_config_;
        std::string throw_state_;
        std::string throw_auxinfo_;

        // retrieve information related to exception_info
        try
        {
            std::rethrow_exception(ep);
        }
        catch (exception_info const& xi)
        {
            if (std::string const* function =
                    xi.get<hpx::detail::throw_function>())
            {
                throw_function_ = *function;
            }

            if (std::string const* file = xi.get<hpx::detail::throw_file>())
            {
                throw_file_ = *file;
            }

            if (long const* line = xi.get<hpx::detail::throw_line>())
            {
                throw_line_ = *line;
            }

            if (std::uint32_t const* locality =
                    xi.get<hpx::detail::throw_locality>())
            {
                throw_locality_ = *locality;
            }

            if (std::string const* hostname_ =
                    xi.get<hpx::detail::throw_hostname>())
            {
                throw_hostname_ = *hostname_;
            }

            if (std::int64_t const* pid_ = xi.get<hpx::detail::throw_pid>())
            {
                throw_pid_ = *pid_;
            }

            if (std::size_t const* shepherd =
                    xi.get<hpx::detail::throw_shepherd>())
            {
                throw_shepherd_ = *shepherd;
            }

            if (std::size_t const* thread_id =
                    xi.get<hpx::detail::throw_thread_id>())
            {
                throw_thread_id_ = *thread_id;
            }

            if (std::string const* thread_name =
                    xi.get<hpx::detail::throw_thread_name>())
            {
                throw_thread_name_ = *thread_name;
            }

            if (std::string const* back_trace =
                    xi.get<hpx::detail::throw_stacktrace>())
            {
                throw_back_trace_ = *back_trace;
            }

            if (std::string const* env_ = xi.get<hpx::detail::throw_env>())
            {
                throw_env_ = *env_;
            }

            if (std::string const* config_ =
                    xi.get<hpx::detail::throw_config>())
            {
                throw_config_ = *config_;
            }

            if (std::string const* state_ = xi.get<hpx::detail::throw_state>())
            {
                throw_state_ = *state_;
            }

            if (std::string const* auxinfo_ =
                    xi.get<hpx::detail::throw_auxinfo>())
            {
                throw_auxinfo_ = *auxinfo_;
            }
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (...)
        {    //-V565
            // do nothing
        }

        // figure out concrete underlying exception type
        try
        {
            std::rethrow_exception(ep);
        }
        catch (hpx::thread_interrupted const&)
        {
            type = hpx::util::exception_type::hpx_thread_interrupted_exception;
            what = "hpx::thread_interrupted";
            err_value = hpx::error::thread_cancelled;
        }
        catch (hpx::exception const& e)
        {
            type = hpx::util::exception_type::hpx_exception;
            what = e.what();
            err_value = e.get_error();
        }
        catch (std::system_error const& e)
        {
            type = hpx::util::exception_type::std_system_error;
            what = e.what();
            err_value = static_cast<hpx::error>(e.code().value());
            err_message = e.code().message();
        }
        catch (std::runtime_error const& e)
        {
            type = hpx::util::exception_type::std_runtime_error;
            what = e.what();
        }
        catch (std::invalid_argument const& e)
        {
            type = hpx::util::exception_type::std_invalid_argument;
            what = e.what();
        }
        catch (std::out_of_range const& e)
        {
            type = hpx::util::exception_type::std_out_of_range;
            what = e.what();
        }
        catch (std::logic_error const& e)
        {
            type = hpx::util::exception_type::std_logic_error;
            what = e.what();
        }
        catch (std::bad_alloc const& e)
        {
            type = hpx::util::exception_type::std_bad_alloc;
            what = e.what();
        }
        catch (std::bad_cast const& e)
        {
            type = hpx::util::exception_type::std_bad_cast;
            what = e.what();
        }
        catch (std::bad_typeid const& e)
        {
            type = hpx::util::exception_type::std_bad_typeid;
            what = e.what();
        }
        catch (std::bad_exception const& e)
        {
            type = hpx::util::exception_type::std_bad_exception;
            what = e.what();
        }
        catch (std::exception const& e)
        {
            type = hpx::util::exception_type::std_exception;
            what = e.what();
        }
#if ASIO_HAS_BOOST_THROW_EXCEPTION != 0
        catch (boost::exception const& e)
        {
            type = hpx::util::boost_exception;
            what = boost::diagnostic_information(e);
        }
#endif
        catch (...)
        {
            type = hpx::util::exception_type::unknown_exception;
            what = "unknown exception";
        }

        ar & type & what & throw_function_ & throw_file_ & throw_line_ &
            throw_locality_ & throw_hostname_ & throw_pid_ & throw_shepherd_ &
            throw_thread_id_ & throw_thread_name_ & throw_back_trace_ &
            throw_env_ & throw_config_ & throw_state_ & throw_auxinfo_;

        if (hpx::util::exception_type::hpx_exception == type)
        {
            ar << static_cast<std::int16_t>(err_value);
        }
        else if (hpx::util::exception_type::std_system_error == type)
        {
            ar << static_cast<std::int16_t>(err_value) << err_message;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    void load_custom_exception(hpx::serialization::input_archive& ar,
        std::exception_ptr& e, unsigned int /*version*/)
    {
        hpx::util::exception_type type =
            hpx::util::exception_type::unknown_exception;
        std::string what;
        hpx::error err_value = hpx::error::success;
        std::string err_message;

        std::uint32_t throw_locality_ = 0;
        std::string throw_hostname_;
        std::int64_t throw_pid_ = -1;
        std::size_t throw_shepherd_ = 0;
        std::size_t throw_thread_id_ = 0;
        std::string throw_thread_name_;
        std::string throw_function_;
        std::string throw_file_;
        std::string throw_back_trace_;
        std::int64_t throw_line_ = 0;
        std::string throw_env_;
        std::string throw_config_;
        std::string throw_state_;
        std::string throw_auxinfo_;

        ar & type & what & throw_function_ & throw_file_ & throw_line_ &
            throw_locality_ & throw_hostname_ & throw_pid_ & throw_shepherd_ &
            throw_thread_id_ & throw_thread_name_ & throw_back_trace_ &
            throw_env_ & throw_config_ & throw_state_ & throw_auxinfo_;

        if (hpx::util::exception_type::hpx_exception == type)
        {
            std::int16_t error_code = 0;
            ar >> error_code;
            err_value = static_cast<hpx::error>(error_code);
        }
        else if (hpx::util::exception_type::std_system_error == type)
        {
            std::int16_t error_code = 0;
            ar >> error_code >> err_message;
            err_value = static_cast<hpx::error>(error_code);
        }

        switch (type)
        {
        default:
        case hpx::util::exception_type::std_exception:
            [[fallthrough]];

        case hpx::util::exception_type::unknown_exception:
            e = hpx::detail::construct_exception(
                hpx::detail::std_exception(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        // standard exceptions
        case hpx::util::exception_type::std_runtime_error:
            e = hpx::detail::construct_exception(std::runtime_error(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        case hpx::util::exception_type::std_invalid_argument:
            e = hpx::detail::construct_exception(std::invalid_argument(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        case hpx::util::exception_type::std_out_of_range:
            e = hpx::detail::construct_exception(std::out_of_range(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        case hpx::util::exception_type::std_logic_error:
            e = hpx::detail::construct_exception(std::logic_error(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        case hpx::util::exception_type::std_bad_alloc:
            e = hpx::detail::construct_exception(hpx::detail::bad_alloc(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        case hpx::util::exception_type::std_bad_cast:
            e = hpx::detail::construct_exception(hpx::detail::bad_cast(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        case hpx::util::exception_type::std_bad_typeid:
            e = hpx::detail::construct_exception(hpx::detail::bad_typeid(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;
        case hpx::util::exception_type::std_bad_exception:
            e = hpx::detail::construct_exception(
                hpx::detail::bad_exception(what),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

#if ASIO_HAS_BOOST_THROW_EXCEPTION != 0
        // boost exceptions
        case hpx::util::boost_exception:
            HPX_ASSERT(false);    // shouldn't happen
            break;
#endif

        // boost::system::system_error
        case hpx::util::exception_type::boost_system_error:
            [[fallthrough]];

        // std::system_error
        case hpx::util::exception_type::std_system_error:
            e = hpx::detail::construct_exception(
                std::system_error(static_cast<int>(err_value),
                    std::system_category(), err_message),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        // hpx::exception
        case hpx::util::exception_type::hpx_exception:
            e = hpx::detail::construct_exception(
                hpx::exception(err_value, what, hpx::throwmode::rethrow),
                hpx::detail::construct_exception_info(throw_function_,
                    throw_file_, throw_line_, throw_back_trace_,
                    throw_locality_, throw_hostname_, throw_pid_,
                    throw_shepherd_, throw_thread_id_, throw_thread_name_,
                    throw_env_, throw_config_, throw_state_, throw_auxinfo_));
            break;

        // hpx::thread_interrupted
        case hpx::util::exception_type::hpx_thread_interrupted_exception:
            e = hpx::detail::construct_lightweight_exception(
                hpx::thread_interrupted());
            break;
        }
    }
}    // namespace hpx::runtime_local::detail

#include <hpx/config/warnings_suffix.hpp>
