//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/runtime_local/config_entry.hpp>
#include <hpx/runtime_local/custom_exception_info.hpp>
#include <hpx/runtime_local/debugging.hpp>
#include <hpx/runtime_local/get_locality_id.hpp>
#include <hpx/runtime_local/get_worker_thread_num.hpp>
#include <hpx/runtime_local/runtime_local.hpp>
#include <hpx/runtime_local/state.hpp>
#include <hpx/version.hpp>

#if defined(HPX_WINDOWS)
#include <process.h>
#elif defined(HPX_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <typeinfo>
#include <utility>
#include <vector>

#if defined(HPX_WINDOWS)
#include <excpt.h>
#undef exception_info
#endif

namespace hpx {

    char const* get_runtime_state_name(state st) noexcept;

    ///////////////////////////////////////////////////////////////////////////
    // For testing purposes we sometimes expect to see exceptions, allow those
    // to go through without attaching a debugger.
    namespace {

        std::atomic<bool> expect_exception_flag(false);
    }

    bool expect_exception(bool flag)
    {
        return expect_exception_flag.exchange(flag);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Extract the diagnostic information embedded in the given exception and
    // return a string holding a formatted message.
    std::string default_diagnostic_information(std::exception_ptr const& e)
    {
        try
        {
            if (e)
                std::rethrow_exception(e);
        }
        catch (std::exception const& ex)
        {
            return {ex.what()};
        }
        return {"<unknown>"};
    }

    std::string diagnostic_information(hpx::exception_info const& xi)
    {
        int const verbosity = util::from_string<int>(
            get_config_entry("hpx.exception_verbosity", "2"));

        std::ostringstream strm;
        strm << "\n";

        // add full build information
        if (verbosity >= 2)
        {
            strm << full_build_string();

            if (std::string const* env = xi.get<hpx::detail::throw_env>();
                env && !env->empty())
            {
                strm << "{env}: " << *env;
            }
        }

        if (verbosity >= 1)
        {
            std::string const* back_trace =
                xi.get<hpx::detail::throw_stacktrace>();
            if (back_trace && !back_trace->empty())
            {
                // FIXME: add indentation to stack frame information
                strm << "{stack-trace}: " << *back_trace << "\n";
            }

            if (std::uint32_t const* locality =
                    xi.get<hpx::detail::throw_locality>())
            {
                strm << "{locality-id}: " << *locality << "\n";
            }

            std::string const* hostname_ =
                xi.get<hpx::detail::throw_hostname>();
            if (hostname_ && !hostname_->empty())
                strm << "{hostname}: " << *hostname_ << "\n";

            std::int64_t const* pid_ = xi.get<hpx::detail::throw_pid>();
            if (pid_ && -1 != *pid_)
                strm << "{process-id}: " << *pid_ << "\n";

            bool thread_info = false;
            constexpr auto thread_prefix = "{os-thread}: ";
            if (std::size_t const* shepherd =
                    xi.get<hpx::detail::throw_shepherd>();
                shepherd && static_cast<std::size_t>(-1) != *shepherd)
            {
                strm << thread_prefix << *shepherd;
                thread_info = true;
            }

            std::string const thread_name = hpx::get_thread_name();
            if (!thread_info)
                strm << thread_prefix;
            else
                strm << ", ";
            strm << thread_name << "\n";

            std::size_t const* thread_id =
                xi.get<hpx::detail::throw_thread_id>();
            if (thread_id && *thread_id)
            {
                strm << "{thread-id}: ";
                hpx::util::format_to(strm, "{:016x}\n", *thread_id);
            }

            std::string const* thread_description =
                xi.get<hpx::detail::throw_thread_name>();
            if (thread_description && !thread_description->empty())
                strm << "{thread-description}: " << *thread_description << "\n";

            if (std::string const* state = xi.get<hpx::detail::throw_state>())
                strm << "{state}: " << *state << "\n";

            if (std::string const* auxinfo =
                    xi.get<hpx::detail::throw_auxinfo>())
            {
                strm << "{auxinfo}: " << *auxinfo << "\n";
            }
        }

        if (std::string const* file = xi.get<hpx::detail::throw_file>())
            strm << "{file}: " << *file << "\n";

        if (long const* line = xi.get<hpx::detail::throw_line>())
            strm << "{line}: " << *line << "\n";

        if (std::string const* function = xi.get<hpx::detail::throw_function>())
            strm << "{function}: " << *function << "\n";

        // Try a cast to std::exception - this should handle boost.system
        // error codes in addition to the standard library exceptions.
        if (auto const* se = dynamic_cast<std::exception const*>(&xi))
            strm << "{what}: " << se->what() << "\n";

        return strm.str();
    }
}    // namespace hpx

namespace hpx::util {

    // This is a local helper used to get the backtrace on a new stack if
    // possible.
    static std::string trace_on_new_stack(
        std::size_t frames_no = HPX_HAVE_THREAD_BACKTRACE_DEPTH)
    {
#if defined(HPX_HAVE_STACKTRACES)
        if (frames_no == 0)
        {
            return {};
        }

        backtrace bt(frames_no);

        // avoid infinite recursion on handling errors
        if (auto const* self = threads::get_self_ptr(); nullptr == self ||
            self->get_thread_id() == threads::invalid_thread_id)
        {
            return bt.trace();
        }

        lcos::local::futures_factory<std::string()> p(
            [&bt] { return bt.trace(); });

        error_code ec(throwmode::lightweight);
        threads::thread_id_ref_type const tid =
            p.post("hpx::util::trace_on_new_stack",
                launch::fork_policy(threads::thread_priority::default_,
                    threads::thread_stacksize::medium),
                ec);
        if (ec)
            return "<couldn't retrieve stack backtrace>";

        // make sure this thread is executed last
        hpx::this_thread::yield_to(thread::id(tid));

        return p.get_future().get(ec);
#else
        return {};
#endif
    }
}    // namespace hpx::util

namespace hpx::detail {

    void pre_exception_handler()
    {
        if (!expect_exception_flag.load(std::memory_order_relaxed))
        {
            hpx::util::may_attach_debugger("exception");
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // report an early or late exception and abort
    void report_exception_and_continue(std::exception const& e)
    {
        pre_exception_handler();

        std::cerr << e.what() << "\n" << std::flush;
    }

    void report_exception_and_continue(std::exception_ptr const& e)
    {
        pre_exception_handler();

        std::cerr << diagnostic_information(e) << "\n" << std::flush;
    }

    void report_exception_and_continue(hpx::exception const& e)
    {
        pre_exception_handler();

        std::cerr << diagnostic_information(e) << "\n" << std::flush;
    }

    void report_exception_and_terminate(std::exception const& e)
    {
        report_exception_and_continue(e);
        std::abort();
    }

    void report_exception_and_terminate(std::exception_ptr const& e)
    {
        report_exception_and_continue(e);
        std::abort();
    }

    void report_exception_and_terminate(hpx::exception const& e)
    {
        report_exception_and_continue(e);
        std::abort();
    }

    hpx::exception_info construct_exception_info(std::string const& func,
        std::string const& file, std::int64_t line,
        std::string const& back_trace, std::uint32_t node,
        std::string const& hostname, std::int64_t pid, std::size_t shepherd,
        std::size_t thread_id, std::string const& thread_name,
        std::string const& env, std::string const& config,
        std::string const& state_name, std::string const& auxinfo)
    {
        return hpx::exception_info().set(
            hpx::detail::throw_stacktrace(back_trace),
            hpx::detail::throw_locality(node),
            hpx::detail::throw_hostname(hostname), hpx::detail::throw_pid(pid),
            hpx::detail::throw_shepherd(shepherd),
            hpx::detail::throw_thread_id(thread_id),
            hpx::detail::throw_thread_name(thread_name),
            hpx::detail::throw_function(func), hpx::detail::throw_file(file),
            hpx::detail::throw_line(static_cast<long>(line)),
            hpx::detail::throw_env(env), hpx::detail::throw_config(config),
            hpx::detail::throw_state(state_name),
            hpx::detail::throw_auxinfo(auxinfo));
    }

    template <typename Exception>
    std::exception_ptr construct_exception(
        Exception const& e, hpx::exception_info info)
    {
        // create a std::exception_ptr object encapsulating the Exception to
        // be thrown and annotate it with all the local information we have
        try
        {
            throw_with_info(e, HPX_MOVE(info));
        }
        catch (...)
        {
            return std::current_exception();
        }

        HPX_UNREACHABLE;    // -V779

        // need this return to silence a warning with icc
        return {};
    }

    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        hpx::exception const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::system_error const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::exception const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        hpx::detail::std_exception const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::bad_exception const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        hpx::detail::bad_exception const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::bad_typeid const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        hpx::detail::bad_typeid const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::bad_cast const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        hpx::detail::bad_cast const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::bad_alloc const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        hpx::detail::bad_alloc const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::logic_error const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::runtime_error const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::out_of_range const&, hpx::exception_info info);
    template HPX_CORE_EXPORT std::exception_ptr construct_exception(
        std::invalid_argument const&, hpx::exception_info info);

    ///////////////////////////////////////////////////////////////////////////
    //  Figure out the size of the given environment
    static std::size_t get_arraylen(char** array)
    {
        std::size_t count = 0;
        if (nullptr != array)
        {
            while (nullptr != array[count])
                ++count;    // simply count the environment strings
        }
        return count;
    }

    std::string get_execution_environment()
    {
        std::vector<std::string> env;

#if defined(HPX_WINDOWS)
        std::size_t len = get_arraylen(_environ);
        env.reserve(len);
        std::copy(&_environ[0], &_environ[len], std::back_inserter(env));
#elif defined(linux) || defined(__linux) || defined(__linux__) ||              \
    defined(__AIX__)
        std::size_t len = get_arraylen(environ);
        env.reserve(len);
        std::copy(&environ[0], &environ[len], std::back_inserter(env));
#elif defined(__FreeBSD__)
        std::size_t len = get_arraylen(freebsd_environ);
        env.reserve(len);
        std::copy(&freebsd_environ[0], &freebsd_environ[len],
            std::back_inserter(env));
#elif defined(__APPLE__)
        std::size_t len = get_arraylen(environ);
        env.reserve(len);
        std::copy(&environ[0], &environ[len], std::back_inserter(env));
#else
#error "Don't know, how to access the execution environment on this platform"
#endif

        std::sort(env.begin(), env.end());

        static constexpr char const* ignored_env_patterns[] = {
            "DOCKER", "GITHUB_TOKEN"};
        std::string retval = hpx::util::format("{} entries:\n", env.size());
        for (std::string const& s : env)
        {
            if (std::all_of(std::begin(ignored_env_patterns),
                    std::end(ignored_env_patterns), [&s](auto const e) {
                        return s.find(e) == std::string::npos;
                    }))
            {
                retval += "  " + s + "\n";
            }
        }
        return retval;
    }

    hpx::exception_info custom_exception_info(std::string const& func,
        std::string const& file, long line, std::string const& auxinfo)
    {
        std::int64_t const pid = ::getpid();

        auto const trace_depth =
            util::from_string<std::size_t>(get_config_entry(
                "hpx.trace_depth", HPX_HAVE_THREAD_BACKTRACE_DEPTH));

        std::string const back_trace(
            hpx::util::trace_on_new_stack(trace_depth));

        std::string state_name("not running");
        std::string hostname;
        if (hpx::runtime const* rt = get_runtime_ptr())
        {
            state const rts_state = rt->get_state();
            state_name = get_runtime_state_name(rts_state);

            if (rts_state >= state::initialized && rts_state < state::stopped)
            {
                hostname = get_runtime().here();
            }
        }

        // if this is not a HPX thread we do not need to query neither for
        // the shepherd thread nor for the thread id
        error_code ec(throwmode::lightweight);
        std::uint32_t const node = get_locality_id(ec);

        auto shepherd = static_cast<std::size_t>(-1);
        threads::thread_id_type thread_id;
        threads::thread_description thread_name;

        threads::thread_self const* self = threads::get_self_ptr();
        if (nullptr != self)
        {
            if (threads::threadmanager_is(hpx::state::running))
                shepherd = hpx::get_worker_thread_num();

            thread_id = threads::get_self_id();
            thread_name = threads::get_thread_description(thread_id);
        }

        std::string const env(get_execution_environment());
        std::string const config(configuration_string());

        return hpx::exception_info().set(
            hpx::detail::throw_stacktrace(back_trace),
            hpx::detail::throw_locality(node),
            hpx::detail::throw_hostname(hostname), hpx::detail::throw_pid(pid),
            hpx::detail::throw_shepherd(shepherd),
            hpx::detail::throw_thread_id(
                reinterpret_cast<std::size_t>(thread_id.get())),
            hpx::detail::throw_thread_name(threads::as_string(thread_name)),
            hpx::detail::throw_function(func), hpx::detail::throw_file(file),
            hpx::detail::throw_line(line), hpx::detail::throw_env(env),
            hpx::detail::throw_config(config),
            hpx::detail::throw_state(state_name),
            hpx::detail::throw_auxinfo(auxinfo));
    }
}    // namespace hpx::detail

///////////////////////////////////////////////////////////////////////////////
namespace hpx {
    /// Return the host-name of the locality where the exception was thrown.
    std::string get_error_host_name(hpx::exception_info const& xi)
    {
        std::string const* hostname_ = xi.get<hpx::detail::throw_hostname>();
        if (hostname_ && !hostname_->empty())
            return *hostname_;
        return {};
    }

    /// Return the locality where the exception was thrown.
    std::uint32_t get_error_locality_id(hpx::exception_info const& xi) noexcept
    {
        if (std::uint32_t const* locality =
                xi.get<hpx::detail::throw_locality>())
        {
            return *locality;
        }

        // same as naming::invalid_locality_id
        return ~static_cast<std::uint32_t>(0);
    }

    /// Return the (operating system) process id of the locality where the
    /// exception was thrown.
    std::int64_t get_error_process_id(hpx::exception_info const& xi) noexcept
    {
        if (std::int64_t const* pid = xi.get<hpx::detail::throw_pid>())
            return *pid;
        return -1;
    }

    /// Return the environment of the OS-process at the point the exception
    /// was thrown.
    std::string get_error_env(hpx::exception_info const& xi)
    {
        if (std::string const* env = xi.get<hpx::detail::throw_env>();
            env && !env->empty())
        {
            return *env;
        }

        return "<unknown>";
    }

    /// Return the stack backtrace at the point the exception was thrown.
    std::string get_error_backtrace(hpx::exception_info const& xi)
    {
        if (std::string const* back_trace =
                xi.get<hpx::detail::throw_stacktrace>();
            back_trace && !back_trace->empty())
        {
            return *back_trace;
        }

        return {};
    }

    /// Return the sequence number of the OS-thread used to execute HPX-threads
    /// from which the exception was thrown.
    std::size_t get_error_os_thread(hpx::exception_info const& xi) noexcept
    {
        if (std::size_t const* shepherd = xi.get<hpx::detail::throw_shepherd>();
            shepherd && static_cast<std::size_t>(-1) != *shepherd)
        {
            return *shepherd;
        }
        return static_cast<std::size_t>(-1);
    }

    /// Return the unique thread id of the HPX-thread from which the exception
    /// was thrown.
    std::size_t get_error_thread_id(hpx::exception_info const& xi) noexcept
    {
        if (std::size_t const* thread_id =
                xi.get<hpx::detail::throw_thread_id>();
            thread_id && *thread_id)
        {
            return *thread_id;
        }
        return static_cast<std::size_t>(-1);
    }

    /// Return any addition thread description of the HPX-thread from which the
    /// exception was thrown.
    std::string get_error_thread_description(hpx::exception_info const& xi)
    {
        if (std::string const* thread_description =
                xi.get<hpx::detail::throw_thread_name>();
            thread_description && !thread_description->empty())
        {
            return *thread_description;
        }
        return {};
    }

    /// Return the HPX configuration information point from which the
    /// exception was thrown.
    std::string get_error_config(hpx::exception_info const& xi)
    {
        if (std::string const* config_info =
                xi.get<hpx::detail::throw_config>();
            config_info && !config_info->empty())
        {
            return *config_info;
        }
        return {};
    }

    /// Return the HPX runtime state information at which the exception was
    /// thrown.
    std::string get_error_state(hpx::exception_info const& xi)
    {
        if (std::string const* state_info = xi.get<hpx::detail::throw_state>();
            state_info && !state_info->empty())
        {
            return *state_info;
        }
        return {};
    }
}    // namespace hpx
