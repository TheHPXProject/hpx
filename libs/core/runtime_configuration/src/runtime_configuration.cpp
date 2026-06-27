//  Copyright (c) 2005-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/prefix.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/string_util.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/runtime_configuration/agas_service_mode.hpp>
#include <hpx/runtime_configuration/component_registry_base.hpp>
#include <hpx/runtime_configuration/init_ini_data.hpp>
#include <hpx/runtime_configuration/plugin_registry_base.hpp>
#include <hpx/runtime_configuration/runtime_configuration.hpp>
#include <hpx/runtime_configuration/runtime_mode.hpp>
#include <hpx/version.hpp>

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
#include <hpx/itt_notify/detail/use_ittnotify_api.hpp>
#include <hpx/modules/itt_notify.hpp>
#endif

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(HPX_WINDOWS)
#include <process.h>
#elif defined(HPX_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if (defined(__linux) || defined(linux) || defined(__linux__))
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/types.h>
#endif

#if !defined(HPX_WINDOWS)
#if defined(HPX_DEBUG)
#define HPX_DLL_STRING "libhpxd" HPX_SHARED_LIB_EXTENSION
#else
#define HPX_DLL_STRING "libhpx" HPX_SHARED_LIB_EXTENSION
#endif
#elif defined(HPX_DEBUG)
#define HPX_DLL_STRING "hpxd" HPX_SHARED_LIB_EXTENSION
#else
#define HPX_DLL_STRING "hpx" HPX_SHARED_LIB_EXTENSION
#endif

#include <limits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    namespace detail {

        // CMake does not deal with explicit semicolons well, for this reason,
        // the paths are delimited with ':'. On Windows those need to be
        // converted to ';'.
        static std::string convert_delimiters(std::string paths)
        {
#if defined(HPX_WINDOWS)
            std::replace(paths.begin(), paths.end(), ':', ';');
#endif
            return paths;
        }
    }    // namespace detail

    // pre-initialize entries with compile time based values
    void runtime_configuration::pre_initialize_ini()
    {
        if (!need_to_call_pre_initialize)
            return;

        std::vector<std::string> lines = {
            // clang-format off
            // create an empty application section
            "[application]",

            // create system and application instance specific entries
            "[system]",
            "pid = " + std::to_string(getpid()),
            "prefix = " + find_prefix(),
#if defined(__linux) || defined(linux) || defined(__linux__)
            "executable_prefix = " + get_executable_prefix(argv0),
#else
            "executable_prefix = " + get_executable_prefix(),
#endif
            // create default installation location and logging settings
            "[hpx]",
            "location = ${HPX_LOCATION:$[system.prefix]}",
            "component_paths = ${HPX_COMPONENT_PATHS}",
            "component_base_paths = $[hpx.location]"    // NOLINT
                HPX_INI_PATH_DELIMITER "$[system.executable_prefix]",
            "component_path_suffixes = " +
                detail::convert_delimiters(HPX_DEFAULT_COMPONENT_PATH_SUFFIXES),
            "master_ini_path = $[hpx.location]" HPX_INI_PATH_DELIMITER
            "$[system.executable_prefix]/",
            "master_ini_path_suffixes = /share/" HPX_BASE_DIR_NAME
                HPX_INI_PATH_DELIMITER "/../share/" HPX_BASE_DIR_NAME,
#ifdef HPX_HAVE_ITTNOTIFY
            "use_itt_notify = ${HPX_HAVE_ITTNOTIFY:0}",
#endif
            "finalize_wait_time = ${HPX_FINALIZE_WAIT_TIME:-1.0}",
            "shutdown_timeout = ${HPX_SHUTDOWN_TIMEOUT:-1.0}",
            "shutdown_check_count = ${HPX_SHUTDOWN_CHECK_COUNT:10}",
#ifdef HPX_HAVE_VERIFY_LOCKS
#if defined(HPX_DEBUG)
            "lock_detection = ${HPX_LOCK_DETECTION:1}",
#else
            "lock_detection = ${HPX_LOCK_DETECTION:0}",
#endif
            "throw_on_held_lock = ${HPX_THROW_ON_HELD_LOCK:1}",
#endif
#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
#ifdef HPX_DEBUG
            "minimal_deadlock_detection = ${HPX_MINIMAL_DEADLOCK_DETECTION:1}",
#else
            "minimal_deadlock_detection = ${HPX_MINIMAL_DEADLOCK_DETECTION:0}",
#endif
#endif
#ifdef HPX_HAVE_SPINLOCK_DEADLOCK_DETECTION
#ifdef HPX_DEBUG
            "spinlock_deadlock_detection = "
            "${HPX_SPINLOCK_DEADLOCK_DETECTION:1}",
#else
            "spinlock_deadlock_detection = "
            "${HPX_SPINLOCK_DEADLOCK_DETECTION:0}",
#endif
            "spinlock_deadlock_detection_limit = "
            "${HPX_SPINLOCK_DEADLOCK_DETECTION_LIMIT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_SPINLOCK_DEADLOCK_DETECTION_LIMIT)) "}",
#endif
            "expect_connecting_localities = "
            "${HPX_EXPECT_CONNECTING_LOCALITIES:0}",

            // add placeholders for keys to be added by command line handling
            "os_threads = ${HPX_NUM_WORKER_THREADS:cores}",
            "cores = all",
            "localities = 1",
            "first_pu = 0",
            "runtime_mode = console",
            "scheduler = local-priority-fifo",
            "affinity = core",
            "pu_step = 1",
            "pu_offset = 0",
            "numa_sensitive = 0",
            "loopback_network = 0",
            "max_background_threads = "
            "${HPX_MAX_BACKGROUND_THREADS:$[hpx.os_threads]}",
            "max_idle_loop_count = ${HPX_MAX_IDLE_LOOP_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_IDLE_LOOP_COUNT_MAX)) "}",
            "max_busy_loop_count = ${HPX_MAX_BUSY_LOOP_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_BUSY_LOOP_COUNT_MAX)) "}",
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
            "max_idle_backoff_time = "
            "${HPX_MAX_IDLE_BACKOFF_TIME:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_IDLE_BACKOFF_TIME_MAX)) "}",
#endif
            "default_scheduler_mode = ${HPX_DEFAULT_SCHEDULER_MODE}",

        /// If HPX_HAVE_ATTACH_DEBUGGER_ON_TEST_FAILURE is set,
        /// then apply the test-failure value as default.
#if defined(HPX_HAVE_ATTACH_DEBUGGER_ON_TEST_FAILURE)
            "attach_debugger = ${HPX_ATTACH_DEBUGGER:test-failure}",
#else
            "attach_debugger = ${HPX_ATTACH_DEBUGGER}",
#endif
            "exception_verbosity = ${HPX_EXCEPTION_VERBOSITY:2}",
            "trace_depth = ${HPX_TRACE_DEPTH:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_HAVE_THREAD_BACKTRACE_DEPTH)) "}",
            "handle_signals = ${HPX_HANDLE_SIGNALS:1}",
            "handle_failed_new = ${HPX_HANDLE_FAILED_NEW:1}",

            // arity for collective operations implemented in a tree fashion
            "[hpx.lcos.collectives]",
            "arity = ${HPX_LCOS_COLLECTIVES_ARITY:32}",
            "cut_off = ${HPX_LCOS_COLLECTIVES_CUT_OFF:-1}",

            // connect back to the given latch if specified
            "[hpx.on_startup]",
            "wait_on_latch = ${HPX_ON_STARTUP_WAIT_ON_LATCH}",

#if defined(HPX_HAVE_NETWORKING)
            // by default, enable networking
            "[hpx.parcel]",
            "enable = 1",
#endif

            "[hpx.stacks]",
            "small_size = ${HPX_SMALL_STACK_SIZE:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_SMALL_STACK_SIZE)) "}",
            "medium_size = ${HPX_MEDIUM_STACK_SIZE:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_MEDIUM_STACK_SIZE)) "}",
            "large_size = ${HPX_LARGE_STACK_SIZE:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_LARGE_STACK_SIZE)) "}",
            "huge_size = ${HPX_HUGE_STACK_SIZE:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_HUGE_STACK_SIZE)) "}",
#if defined(__linux) || defined(linux) || defined(__linux__) ||                \
    defined(__FreeBSD__)
            "use_guard_pages = ${HPX_USE_GUARD_PAGES:1}",
#endif

            "[hpx.threadpools]",
#if defined(HPX_HAVE_IO_POOL)
            "io_pool_size = ${HPX_NUM_IO_POOL_SIZE:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_NUM_IO_POOL_SIZE)) "}",
#endif
#if defined(HPX_HAVE_NETWORKING)
            "parcel_pool_size = ${HPX_NUM_PARCEL_POOL_SIZE:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_NUM_PARCEL_POOL_SIZE)) "}",
#endif
#if defined(HPX_HAVE_TIMER_POOL)
            "timer_pool_size = ${HPX_NUM_TIMER_POOL_SIZE:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_NUM_TIMER_POOL_SIZE)) "}",
#endif

            "[hpx.thread_queue]",
            "max_thread_count = ${HPX_THREAD_QUEUE_MAX_THREAD_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_MAX_THREAD_COUNT)) "}",
            "min_tasks_to_steal_pending = "
            "${HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_PENDING:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_PENDING)) "}",
            "min_tasks_to_steal_staged = "
            "${HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_STAGED:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_MIN_TASKS_TO_STEAL_STAGED)) "}",
            "min_add_new_count = "
            "${HPX_THREAD_QUEUE_MIN_ADD_NEW_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_MIN_ADD_NEW_COUNT)) "}",
            "max_add_new_count = "
            "${HPX_THREAD_QUEUE_MAX_ADD_NEW_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_MAX_ADD_NEW_COUNT)) "}",
            "min_delete_count = "
            "${HPX_THREAD_QUEUE_MIN_DELETE_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_MIN_DELETE_COUNT)) "}",
            "max_delete_count = "
            "${HPX_THREAD_QUEUE_MAX_DELETE_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_MAX_THREAD_COUNT)) "}",
            "max_terminated_threads = "
            "${HPX_THREAD_QUEUE_MAX_TERMINATED_THREADS:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_MAX_TERMINATED_THREADS)) "}",
            "init_threads_count = "
            "${HPX_THREAD_QUEUE_INIT_THREADS_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_INIT_THREADS_COUNT)) "}",
            "cached_threads_count = "
            "${HPX_THREAD_QUEUE_CACHED_THREADS_COUNT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_THREAD_QUEUE_CACHED_THREADS_COUNT)) "}",

            "[hpx.commandline]",
            // enable aliasing
            "aliasing = ${HPX_COMMANDLINE_ALIASING:1}",

            // allow for unknown options to be passed through
            "allow_unknown = ${HPX_COMMANDLINE_ALLOW_UNKNOWN:0}",

            // allow for command line options to be passed through the
            // environment
            "prepend_options = ${HPX_COMMANDLINE_OPTIONS}",

            // predefine command line aliases
            "[hpx.commandline.aliases]",
            "-a = --hpx:agas",
            "-c = --hpx:console",
            "-h = --hpx:help",
            "-I = --hpx:ini",
            "-l = --hpx:localities",
            "-p = --hpx:app-config",
            "-q = --hpx:queuing",
            "-r = --hpx:run-agas-server",
            "-t = --hpx:threads",
            "-v = --hpx:version",
            "-w = --hpx:worker",
            "-x = --hpx:hpx",
            "-0 = --hpx:node=0",
            "-1 = --hpx:node=1",
            "-2 = --hpx:node=2",
            "-3 = --hpx:node=3",
            "-4 = --hpx:node=4",
            "-5 = --hpx:node=5",
            "-6 = --hpx:node=6",
            "-7 = --hpx:node=7",
            "-8 = --hpx:node=8",
            "-9 = --hpx:node=9",

            "[hpx.agas]",
            // 'address' has deliberately no default, see
            // command_line_handling.cpp
            "address = ${HPX_AGAS_SERVER_ADDRESS}",
            "port = ${HPX_AGAS_SERVER_PORT:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_INITIAL_IP_PORT)) "}",
            "max_pending_refcnt_requests = "
            "${HPX_AGAS_MAX_PENDING_REFCNT_REQUESTS:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(
                    HPX_INITIAL_AGAS_MAX_PENDING_REFCNT_REQUESTS)) "}",
            "service_mode = hosted",
            "local_cache_size = ${HPX_AGAS_LOCAL_CACHE_SIZE:" HPX_PP_STRINGIZE(
                HPX_PP_EXPAND(HPX_AGAS_LOCAL_CACHE_SIZE)) "}",
            "use_range_caching = ${HPX_AGAS_USE_RANGE_CACHING:1}",
            "use_caching = ${HPX_AGAS_USE_CACHING:1}",

            "[hpx.components]",
            "load_external = ${HPX_LOAD_EXTERNAL_COMPONENTS:1}",

            "[hpx.components.barrier]",
            "name = hpx",
            "path = $[hpx.location]/bin/" HPX_DLL_STRING,
            "enabled = 1",

            "[hpx.components.hpx_lcos_server_latch]",
            "name = hpx",
            "path = $[hpx.location]/bin/" HPX_DLL_STRING,
            "enabled = 1",

            "[hpx.components.raw_counter]",
            "name = hpx",
            "path = $[hpx.location]/bin/" HPX_DLL_STRING,
            "enabled = 1",

            "[hpx.components.average_count_counter]",
            "name = hpx",
            "path = $[hpx.location]/bin/" HPX_DLL_STRING,
            "enabled = 1",

            "[hpx.components.elapsed_time_counter]",
            "name = hpx",
            "path = $[hpx.location]/bin/" HPX_DLL_STRING,
            "enabled = 1"
            // clang-format on
        };

        lines.insert(lines.end(), extra_static_ini_defs.begin(),
            extra_static_ini_defs.end());

        // don't overload user overrides
        this->parse("<static defaults>", lines, false, false, false);

        need_to_call_pre_initialize = false;
    }

    void runtime_configuration::post_initialize_ini(std::string& hpx_ini_file_,
        std::vector<std::string> const& cmdline_ini_defs_)
    {
        util::init_ini_data_base(*this, hpx_ini_file_);
        need_to_call_pre_initialize = true;

        // let the command line override the config file.
        if (!cmdline_ini_defs_.empty())
        {
            // do not weed out comments
            this->parse(
                "<command line definitions>", cmdline_ini_defs_, true, false);
            need_to_call_pre_initialize = true;
        }
    }

    void runtime_configuration::pre_initialize_logging_ini()
    {
#if defined(HPX_HAVE_LOGGING)
        std::vector<std::string> const lines = {
        // clang-format off
#define HPX_TIMEFORMAT "$hh:$mm.$ss.$mili"
#define HPX_LOGFORMAT "(T%locality%/%hpxthread%.%hpxphase%/%hpxcomponent%) "

            // general logging
            "[hpx.logging]",
            "level = ${HPX_LOGLEVEL:0}",
            "destination = ${HPX_LOGDESTINATION:console}",
            "format = ${HPX_LOGFORMAT:" HPX_LOGFORMAT
                "P%parentloc%/%hpxparent%.%hpxparentphase% %time%("
                HPX_TIMEFORMAT ") [%idx%]|\\n}",

            // general console logging
            "[hpx.logging.console]",
            "level = ${HPX_LOGLEVEL:$[hpx.logging.level]}",
#if defined(ANDROID) || defined(__ANDROID__)
            "destination = ${HPX_CONSOLE_LOGDESTINATION:android_log}",
#else
            "destination = ${HPX_CONSOLE_LOGDESTINATION:"
                "file(hpx.$[system.pid].log)}",
#endif
            "format = ${HPX_CONSOLE_LOGFORMAT:|}",

            // logging related to timing
            "[hpx.logging.timing]",
            "level = ${HPX_TIMING_LOGLEVEL:-1}",
            "destination = ${HPX_TIMING_LOGDESTINATION:console}",
            "format = ${HPX_TIMING_LOGFORMAT:" HPX_LOGFORMAT
                "P%parentloc%/%hpxparent%.%hpxparentphase% %time%("
                HPX_TIMEFORMAT ") [%idx%] [TIM] |\\n}",

            // console logging related to timing
            "[hpx.logging.console.timing]",
            "level = ${HPX_TIMING_LOGLEVEL:$[hpx.logging.timing.level]}",
#if defined(ANDROID) || defined(__ANDROID__)
            "destination = ${HPX_CONSOLE_TIMING_LOGDESTINATION:android_log}",
#else
            "destination = ${HPX_CONSOLE_TIMING_LOGDESTINATION:"
                "file(hpx.timing.$[system.pid].log)}",
#endif
            "format = ${HPX_CONSOLE_TIMING_LOGFORMAT:|}",

            // logging related to AGAS
            "[hpx.logging.agas]",
            "level = ${HPX_AGAS_LOGLEVEL:-1}",
            "destination = ${HPX_AGAS_LOGDESTINATION:"
                "file(hpx.agas.$[system.pid].log)}",
            "format = ${HPX_AGAS_LOGFORMAT:" HPX_LOGFORMAT
                "P%parentloc%/%hpxparent%.%hpxparentphase% %time%("
                    HPX_TIMEFORMAT ") [%idx%][AGAS] |\\n}",

            // console logging related to AGAS
            "[hpx.logging.console.agas]",
            "level = ${HPX_AGAS_LOGLEVEL:$[hpx.logging.agas.level]}",
#if defined(ANDROID) || defined(__ANDROID__)
            "destination = ${HPX_CONSOLE_AGAS_LOGDESTINATION:android_log}",
#else
            "destination = ${HPX_CONSOLE_AGAS_LOGDESTINATION:"
                "file(hpx.agas.$[system.pid].log)}",
#endif
            "format = ${HPX_CONSOLE_AGAS_LOGFORMAT:|}",

            // logging related to the parcel transport
            "[hpx.logging.parcel]",
            "level = ${HPX_PARCEL_LOGLEVEL:-1}",
            "destination = ${HPX_PARCEL_LOGDESTINATION:"
                "file(hpx.parcel.$[system.pid].log)}",
            "format = ${HPX_PARCEL_LOGFORMAT:" HPX_LOGFORMAT
                "P%parentloc%/%hpxparent%.%hpxparentphase% %time%("
                HPX_TIMEFORMAT ") [%idx%][  PT] |\\n}",

            // console logging related to the parcel transport
            "[hpx.logging.console.parcel]",
            "level = ${HPX_PARCEL_LOGLEVEL:$[hpx.logging.parcel.level]}",
#if defined(ANDROID) || defined(__ANDROID__)
            "destination = ${HPX_CONSOLE_PARCEL_LOGDESTINATION:android_log}",
#else
            "destination = ${HPX_CONSOLE_PARCEL_LOGDESTINATION:"
                "file(hpx.parcel.$[system.pid].log)}",
#endif
            "format = ${HPX_CONSOLE_PARCEL_LOGFORMAT:|}",

            // logging related to applications
            "[hpx.logging.application]",
            "level = ${HPX_APP_LOGLEVEL:-1}",
            "destination = ${HPX_APP_LOGDESTINATION:console}",
            "format = ${HPX_APP_LOGFORMAT:" HPX_LOGFORMAT
                "P%parentloc%/%hpxparent%.%hpxparentphase% %time%("
                HPX_TIMEFORMAT ") [%idx%] [APP] |\\n}",

            // console logging related to applications
            "[hpx.logging.console.application]",
            "level = ${HPX_APP_LOGLEVEL:$[hpx.logging.application.level]}",
#if defined(ANDROID) || defined(__ANDROID__)
            "destination = ${HPX_CONSOLE_APP_LOGDESTINATION:android_log}",
#else
            "destination = ${HPX_CONSOLE_APP_LOGDESTINATION:"
                "file(hpx.application.$[system.pid].log)}",
#endif
            "format = ${HPX_CONSOLE_APP_LOGFORMAT:|}",

            // logging of debug channel
            "[hpx.logging.debuglog]",
            "level = ${HPX_DEB_LOGLEVEL:-1}",
            "destination = ${HPX_DEB_LOGDESTINATION:console}",
            "format = ${HPX_DEB_LOGFORMAT:" HPX_LOGFORMAT
                "P%parentloc%/%hpxparent%.%hpxparentphase% %time%("
                HPX_TIMEFORMAT ") [%idx%] [DEB] |\\n}",

            "[hpx.logging.console.debuglog]",
            "level = ${HPX_DEB_LOGLEVEL:$[hpx.logging.debuglog.level]}",
#if defined(ANDROID) || defined(__ANDROID__)
            "destination = ${HPX_CONSOLE_DEB_LOGDESTINATION:android_log}",
#else
            "destination = ${HPX_CONSOLE_DEB_LOGDESTINATION:"
                "file(hpx.debuglog.$[system.pid].log)}",
#endif
            "format = ${HPX_CONSOLE_DEB_LOGFORMAT:|}"

#undef HPX_TIMEFORMAT
#undef HPX_LOGFORMAT
            // clang-format on
        };

        // don't overload user overrides
        this->parse("<static logging defaults>", lines, false, false);
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    // load information about statically known components
    void runtime_configuration::load_components_static(
        std::vector<components::static_factory_load_data_type> const&
            static_modules)
    {
        std::vector<std::shared_ptr<components::component_registry_base>>
            registries;
        for (components::static_factory_load_data_type const& d :
            static_modules)
        {
            auto new_registries = util::load_component_factory_static(
                *this, d.name, d.get_factory);
            registries.reserve(registries.size() + new_registries.size());
            std::copy(new_registries.begin(), new_registries.end(),
                std::back_inserter(registries));
        }

        // read system and user ini files _again_, to allow the user to
        // overwrite the settings from the default component ini's.
        util::init_ini_data_base(*this, hpx_ini_file);

        // let the command line override the config file.
        if (!cmdline_ini_defs.empty())
            parse("<command line definitions>", cmdline_ini_defs, true, false);

        // merge all found ini files of all components
        util::merge_component_inis(*this);

        need_to_call_pre_initialize = true;

        // invoke last reconfigure
        reconfigure();
        for (auto const& registry : registries)
        {
            registry->register_component_type();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // collect all directories where to use for the search for plugins
    void runtime_configuration::load_component_path(
        std::vector<std::shared_ptr<plugins::plugin_registry_base>>&
            plugin_registries,
        std::vector<std::shared_ptr<components::component_registry_base>>&
            component_registries,
        std::string const& path, std::set<std::string>& component_paths,
        std::map<std::string, filesystem::path>& basenames)
    {
        namespace fs = filesystem;

        using plugin_list_type =
            std::vector<std::shared_ptr<plugins::plugin_registry_base>>;

        if (!path.empty())
        {
            fs::path const this_p(path);
            std::error_code fsec;
            fs::path canonical_p =
                fs::canonical(this_p, fs::initial_path(), fsec);
            if (fsec)
                canonical_p = this_p;

            if (auto const [it, ok] =
                    component_paths.emplace(canonical_p.string());
                ok)
            {
                // have all path elements, now find ini files in there...
                fs::path const this_path(*it);
                if (fs::exists(this_path, fsec) && !fsec)
                {
                    plugin_list_type tmp_regs =
                        util::init_ini_data_default(this_path.string(), *this,
                            basenames, modules_, component_registries);

                    std::copy(tmp_regs.begin(), tmp_regs.end(),
                        std::back_inserter(plugin_registries));
                }
            }
        }
    }

    void runtime_configuration::load_component_paths(
        std::vector<std::shared_ptr<plugins::plugin_registry_base>>&
            plugin_registries,
        std::vector<std::shared_ptr<components::component_registry_base>>&
            component_registries,
        std::string const& component_base_paths,
        std::string const& component_path_suffixes,
        std::set<std::string>& component_paths,
        std::map<std::string, filesystem::path>& basenames)
    {
        // try to build default ini structure from shared libraries in default
        // installation location, this allows to install simple components
        // without the need to install an ini file split of the separate paths
        // from the given path list
        hpx::string_util::char_separator sep(HPX_INI_PATH_DELIMITER);
        hpx::string_util::tokenizer tok_path(component_base_paths, sep);
        hpx::string_util::tokenizer tok_suffixes(component_path_suffixes, sep);
        auto end_path = tok_path.end();
        auto end_suffixes = tok_suffixes.end();

        for (auto it = tok_path.begin(); it != end_path; ++it)
        {
            std::string const& path = *it;
            if (tok_suffixes.begin() != tok_suffixes.end())
            {
                for (auto jt = tok_suffixes.begin(); jt != end_suffixes; ++jt)
                {
                    std::string p = path;
                    p += *jt;
                    load_component_path(plugin_registries, component_registries,
                        p, component_paths, basenames);
                }
            }
            else
            {
                load_component_path(plugin_registries, component_registries,
                    path, component_paths, basenames);
            }
        }
    }

    // load information about dynamically discovered plugins
    std::vector<std::shared_ptr<plugins::plugin_registry_base>>
    runtime_configuration::load_modules(
        std::vector<std::shared_ptr<components::component_registry_base>>&
            component_registries)
    {
        typedef std::vector<std::shared_ptr<plugins::plugin_registry_base>>
            plugin_list_type;

        // protect against duplicate paths
        std::set<std::string> component_paths;

        // list of base names avoiding loading a module more than once
        std::map<std::string, filesystem::path> basenames;

        // plugin registry object
        plugin_list_type plugin_registries;

        // load plugin paths from component_base_paths and suffixes
        std::string const component_base_paths(
            get_entry("hpx.component_base_paths", HPX_DEFAULT_COMPONENT_PATH));
        std::string const component_path_suffixes(
            get_entry("hpx.component_path_suffixes", "/lib/hpx"));

        load_component_paths(plugin_registries, component_registries,
            component_base_paths, component_path_suffixes, component_paths,
            basenames);

        // load additional explicit plugin paths from plugin_paths key
        std::string const plugin_paths(get_entry("hpx.component_paths", ""));
        load_component_paths(plugin_registries, component_registries,
            plugin_paths, "", component_paths, basenames);

        // Pull in any statically registered plugin modules. This covers
        // three cases:
        //   - static HPX build: no shared libraries to dlopen, the static
        //     map is the only source of plugins.
        //   - plugin baked into the application exe (dynamic HPX build): the
        //     exe's static ctor pushed an entry here; the DLL scan above
        //     never saw it.
        //   - plugin shared libraries dlopen'd by the scan: their file-scope
        //     ctors also push here, so we skip those via modules_ (keyed by
        //     module name) to avoid duplicate registry objects.
        for (components::static_factory_load_data_type const& d :
            components::get_static_plugin_module_data())
        {
            if (modules_.find(d.name) != modules_.end())
                continue;    // already loaded via DLL scan

            auto new_registries =
                util::load_plugin_factory_static(*this, d.name, d.get_factory);
            plugin_registries.reserve(
                plugin_registries.size() + new_registries.size());
            std::move(new_registries.begin(), new_registries.end(),
                std::back_inserter(plugin_registries));
        }

        // read system and user ini files _again_, to allow the user to
        // overwrite the settings from the default component ini's.
        util::init_ini_data_base(*this, hpx_ini_file);

        // let the command line override the config file.
        if (!cmdline_ini_defs.empty())
            parse("<command line definitions>", cmdline_ini_defs, true, false);

        // merge all found ini files of all components
        util::merge_component_inis(*this);

        need_to_call_pre_initialize = true;

        // invoke reconfigure
        reconfigure();

        return plugin_registries;
    }

    ///////////////////////////////////////////////////////////////////////////
    runtime_configuration::runtime_configuration(char const* argv0_,
        runtime_mode const mode,
        std::vector<std::string> extra_static_ini_defs_)
      : extra_static_ini_defs(HPX_MOVE(extra_static_ini_defs_))
      , mode_(mode)
      , num_localities(0)
      , num_os_threads(0)
      , small_stacksize(HPX_SMALL_STACK_SIZE)
      , medium_stacksize(HPX_MEDIUM_STACK_SIZE)
      , large_stacksize(HPX_LARGE_STACK_SIZE)
      , huge_stacksize(HPX_HUGE_STACK_SIZE)
      , need_to_call_pre_initialize(true)
#if defined(__linux) || defined(linux) || defined(__linux__)
      , argv0(argv0_)
#endif
    {
        (void) argv0_;

        pre_initialize_ini();

        // set global config options
#if HPX_HAVE_ITTNOTIFY != 0
        use_ittnotify_api = get_itt_notify_mode();
#endif
        HPX_ASSERT(init_small_stack_size() >= HPX_SMALL_STACK_SIZE);

        small_stacksize = init_small_stack_size();
        medium_stacksize = init_medium_stack_size();
        large_stacksize = init_large_stack_size();
        HPX_ASSERT(init_huge_stack_size() <= HPX_HUGE_STACK_SIZE);
        huge_stacksize = init_huge_stack_size();
    }

    ///////////////////////////////////////////////////////////////////////////
    void runtime_configuration::reconfigure(std::string ini_file)
    {
        hpx_ini_file = HPX_MOVE(ini_file);
        reconfigure();
    }

    void runtime_configuration::reconfigure(std::vector<std::string> ini_defs)
    {
        cmdline_ini_defs = HPX_MOVE(ini_defs);
        reconfigure();
    }

    void runtime_configuration::reconfigure()
    {
        pre_initialize_ini();
        pre_initialize_logging_ini();
        post_initialize_ini(hpx_ini_file, cmdline_ini_defs);

        // set global config options
#if HPX_HAVE_ITTNOTIFY != 0
        use_ittnotify_api = get_itt_notify_mode();
#endif
        HPX_ASSERT(init_small_stack_size() >= HPX_SMALL_STACK_SIZE);

        small_stacksize = init_small_stack_size();
        medium_stacksize = init_medium_stack_size();
        large_stacksize = init_large_stack_size();
        huge_stacksize = init_huge_stack_size();
    }

    std::size_t runtime_configuration::get_ipc_data_buffer_cache_size() const
    {
        if (util::section const* sec = get_section("hpx.parcel.ipc");
            nullptr != sec)
        {
            return hpx::util::get_entry_as<std::size_t>(*sec,
                "data_buffer_cache_size",
                HPX_PARCEL_IPC_DATA_BUFFER_CACHE_SIZE);
        }
        return HPX_PARCEL_IPC_DATA_BUFFER_CACHE_SIZE;
    }

    agas::service_mode runtime_configuration::get_agas_service_mode() const
    {
        // load all components as described in the configuration information
        if (util::section const* sec = get_section("hpx.agas"); nullptr != sec)
        {
            std::string const m = sec->get_entry("service_mode", "hosted");

            if (m == "hosted")
            {
                return agas::service_mode::hosted;
            }
            else if (m == "bootstrap")
            {
                return agas::service_mode::bootstrap;
            }
            else
            {
                // REVIEW: exception type is overused
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "runtime_configuration::get_agas_service_mode",
                    "invalid AGAS router mode \"{}\"", m);
            }
        }
        return agas::service_mode::hosted;
    }

    std::uint32_t runtime_configuration::get_num_localities() const
    {
        if (num_localities == 0)
        {
            if (util::section const* sec = get_section("hpx"); nullptr != sec)
            {
                num_localities = hpx::util::get_entry_as<std::uint32_t>(
                    *sec, "localities", 1);
            }
        }

        HPX_ASSERT(num_localities != 0);
        return num_localities;
    }

    void runtime_configuration::set_num_localities(
        std::uint32_t const num_localities_)
    {
        // this function should not be called on the AGAS server
        HPX_ASSERT(agas::service_mode::bootstrap != get_agas_service_mode());
        num_localities = num_localities_;

        if (util::section* sec = get_section("hpx"); nullptr != sec)
        {
            sec->add_entry("localities", std::to_string(num_localities));
        }
    }

    // this function should figure out whether networking has to be enabled.
    bool runtime_configuration::enable_networking() const
    {
#if defined(HPX_HAVE_NETWORKING)
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
            // get the number of initial localities
            if (hpx::util::get_entry_as<std::uint32_t>(*sec, "localities", 1) >
                1)
            {
                return true;
            }

            // on localities other than locality zero the number of
            // localities might not have been initialized yet
            if (hpx::util::get_entry_as<std::int32_t>(*sec, "node", -1) > 0)
            {
                return true;
            }

            // get whether localities are expected to connect
            if (hpx::util::get_entry_as<std::int32_t>(
                    *sec, "expect_connecting_localities", 0) != 0)
            {
                return true;
            }

            // for any runtime mode except 'console' networking should be
            // enabled as well
            if (hpx::util::get_entry_as<std::string>(
                    *sec, "runtime_mode", "") != "console")
            {
                return true;
            }

            // whether the user has explicitly asked for network enabled
            if (hpx::util::get_entry_as<std::int32_t>(
                    *sec, "loopback_network", 0) != 0)
            {
                return true;
            }
        }
#endif
        return false;
    }

    std::uint32_t runtime_configuration::get_first_used_core() const
    {
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
            return hpx::util::get_entry_as<std::uint32_t>(
                *sec, "first_used_core", 0);
        }
        return 0;
    }

    void runtime_configuration::set_first_used_core(
        std::uint32_t const first_used_core)
    {
        if (util::section* sec = get_section("hpx"); nullptr != sec)
        {
            sec->add_entry("first_used_core", std::to_string(first_used_core));
        }
    }

    std::size_t runtime_configuration::get_agas_local_cache_size(
        std::size_t const dflt) const
    {
        std::size_t cache_size = dflt;

        if (util::section const* sec = get_section("hpx.agas"); nullptr != sec)
        {
            cache_size = hpx::util::get_entry_as<std::size_t>(
                *sec, "local_cache_size", cache_size);
        }

        if (cache_size != ~static_cast<std::size_t>(0) && cache_size < 16ul)
        {
            cache_size = 16;    // limit lower bound
        }
        return cache_size;
    }

    bool runtime_configuration::get_agas_caching_mode() const
    {
        if (util::section const* sec = get_section("hpx.agas"); nullptr != sec)
        {
            return hpx::util::get_entry_as<int>(*sec, "use_caching", 1) != 0;
        }
        return false;
    }

    bool runtime_configuration::get_agas_range_caching_mode() const
    {
        if (util::section const* sec = get_section("hpx.agas"); nullptr != sec)
        {
            return hpx::util::get_entry_as<int>(*sec, "use_range_caching", 1) !=
                0;
        }
        return false;
    }

    std::size_t runtime_configuration::get_agas_max_pending_refcnt_requests()
        const
    {
        if (util::section const* sec = get_section("hpx.agas"); nullptr != sec)
        {
            return hpx::util::get_entry_as<std::size_t>(*sec,
                "max_pending_refcnt_requests",
                HPX_INITIAL_AGAS_MAX_PENDING_REFCNT_REQUESTS);
        }
        return HPX_INITIAL_AGAS_MAX_PENDING_REFCNT_REQUESTS;
    }

    bool runtime_configuration::get_itt_notify_mode() const
    {
#if HPX_HAVE_ITTNOTIFY != 0
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
            return hpx::util::get_entry_as<int>(*sec, "use_itt_notify", 0) != 0;
        }
#endif
        return false;
    }

    // Enable lock detection during suspension
    bool runtime_configuration::enable_lock_detection() const
    {
#ifdef HPX_HAVE_VERIFY_LOCKS
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
            return hpx::util::get_entry_as<int>(*sec, "lock_detection", 0) != 0;
        }
#endif
        return false;
    }

    // Enable minimal deadlock detection for HPX threads
    bool runtime_configuration::enable_minimal_deadlock_detection() const
    {
#ifdef HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
#ifdef HPX_DEBUG
            return hpx::util::get_entry_as<int>(
                       *sec, "minimal_deadlock_detection", 1) != 0;
#else
            return hpx::util::get_entry_as<int>(
                       *sec, "minimal_deadlock_detection", 0) != 0;
#endif
        }

#ifdef HPX_DEBUG
        return true;
#else
        return false;
#endif

#else
        return false;
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    bool runtime_configuration::enable_spinlock_deadlock_detection() const
    {
#ifdef HPX_HAVE_SPINLOCK_DEADLOCK_DETECTION
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
#ifdef HPX_DEBUG
            return hpx::util::get_entry_as<int>(
                       *sec, "spinlock_deadlock_detection", 1) != 0;
#else
            return hpx::util::get_entry_as<int>(
                       *sec, "spinlock_deadlock_detection", 0) != 0;
#endif
        }

#ifdef HPX_DEBUG
        return true;
#else
        return false;
#endif

#else
        return false;
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    std::size_t runtime_configuration::get_spinlock_deadlock_detection_limit()
        const
    {
#ifdef HPX_HAVE_SPINLOCK_DEADLOCK_DETECTION
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
            return hpx::util::get_entry_as<std::size_t>(*sec,
                "spinlock_deadlock_detection_limit",
                HPX_SPINLOCK_DEADLOCK_DETECTION_LIMIT);
        }
        return HPX_SPINLOCK_DEADLOCK_DETECTION_LIMIT;
#else
        return static_cast<std::size_t>(-1);
#endif
    }

    std::size_t runtime_configuration::trace_depth() const
    {
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
            return hpx::util::get_entry_as<std::size_t>(
                *sec, "trace_depth", HPX_HAVE_THREAD_BACKTRACE_DEPTH);
        }
        return HPX_HAVE_THREAD_BACKTRACE_DEPTH;
    }

    std::size_t runtime_configuration::get_os_thread_count() const
    {
        if (num_os_threads == 0)
        {
            if (util::section const* sec = get_section("hpx"); nullptr != sec)
            {
                num_os_threads = hpx::util::get_entry_as<std::uint32_t>(
                    *sec, "os_threads", 1);
            }
            else
            {
                num_os_threads = 1;
            }
        }
        return static_cast<std::size_t>(num_os_threads);
    }

    std::string runtime_configuration::get_cmd_line() const
    {
        if (util::section const* sec = get_section("hpx"); nullptr != sec)
        {
            return sec->get_entry("cmd_line", "");
        }
        return "";
    }

    // Return the configured sizes of the known thread pools
    std::size_t runtime_configuration::get_thread_pool_size(
        char const* poolname) const
    {
        if (util::section const* sec = get_section("hpx.threadpools");
            nullptr != sec)
        {
            return hpx::util::get_entry_as<std::size_t>(
                *sec, std::string(poolname) + "_size", 2);
        }
        return 2;    // the default size for all pools is 2
    }

    // Return the endianness to be used for out-serialization
    std::string runtime_configuration::get_endian_out() const
    {
        if (util::section const* sec = get_section("hpx.parcel");
            nullptr != sec)
        {
            return sec->get_entry(
                "endian_out", endian::native == endian::big ? "big" : "little");
        }
        return endian::native == endian::big ? "big" : "little";
    }

    // Will return the stack size to use for all HPX-threads.
    std::ptrdiff_t runtime_configuration::init_stack_size(char const* entryname,
        char const* defaultvaluestr, std::ptrdiff_t const defaultvalue) const
    {
        if (util::section const* sec = get_section("hpx.stacks");
            nullptr != sec)
        {
            std::string const entry =
                sec->get_entry(entryname, defaultvaluestr);
            char* endptr = nullptr;
            std::ptrdiff_t const val =
                std::strtoll(entry.c_str(), &endptr, /*base:*/ 0);
            return endptr != entry.c_str() ? val : defaultvalue;
        }
        return defaultvalue;
    }

#if defined(__linux) || defined(linux) || defined(__linux__) ||                \
    defined(__FreeBSD__)
    bool runtime_configuration::use_stack_guard_pages() const
    {
        if (util::section const* sec = get_section("hpx.stacks");
            nullptr != sec)
        {
            return hpx::util::get_entry_as<int>(*sec, "use_guard_pages", 1) !=
                0;
        }
        return true;    // default is true
    }
#endif

    std::ptrdiff_t runtime_configuration::init_small_stack_size() const
    {
        return init_stack_size("small_size",
            HPX_PP_STRINGIZE(HPX_SMALL_STACK_SIZE), HPX_SMALL_STACK_SIZE);
    }

    std::ptrdiff_t runtime_configuration::init_medium_stack_size() const
    {
        return init_stack_size("medium_size",
            HPX_PP_STRINGIZE(HPX_MEDIUM_STACK_SIZE), HPX_MEDIUM_STACK_SIZE);
    }

    std::ptrdiff_t runtime_configuration::init_large_stack_size() const
    {
        return init_stack_size("large_size",
            HPX_PP_STRINGIZE(HPX_LARGE_STACK_SIZE), HPX_LARGE_STACK_SIZE);
    }

    std::ptrdiff_t runtime_configuration::init_huge_stack_size() const
    {
        return init_stack_size("huge_size",
            HPX_PP_STRINGIZE(HPX_HUGE_STACK_SIZE), HPX_HUGE_STACK_SIZE);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Return maximally allowed message size (zero for unlimited)
    std::uint64_t runtime_configuration::get_max_inbound_message_size(
        std::string const& type) const
    {
        if (util::section const* sec = get_section("hpx.parcel." + type);
            nullptr != sec)
        {
            return hpx::util::get_entry_as<std::uint64_t>(
                *sec, "max_message_size", HPX_PARCEL_MAX_MESSAGE_SIZE);
        }
        return HPX_PARCEL_MAX_MESSAGE_SIZE;    // default is 1GByte
    }

    std::uint64_t runtime_configuration::get_max_outbound_message_size(
        std::string const& type) const
    {
        if (util::section const* sec = get_section("hpx.parcel." + type);
            nullptr != sec)
        {
            if (std::uint64_t const maxsize =
                    hpx::util::get_entry_as<std::uint64_t>(*sec,
                        "max_outbound_message_size",
                        HPX_PARCEL_MAX_OUTBOUND_MESSAGE_SIZE);
                maxsize > 0)
            {
                return maxsize;
            }
        }
        return HPX_PARCEL_MAX_OUTBOUND_MESSAGE_SIZE;    // default is 1GByte
    }

    ///////////////////////////////////////////////////////////////////////////
    bool runtime_configuration::load_application_configuration(
        char const* filename, error_code& ec)
    {
        try
        {
            section appcfg(filename);
            section applroot;
            applroot.add_section("application", appcfg);
            this->section::merge(applroot);
        }
        catch (hpx::exception const& e)
        {
            // file doesn't exist or is ill-formed
            if (&ec == &throws)
                throw;
            ec = make_error_code(
                e.get_error(), e.what(), hpx::throwmode::rethrow);
            return false;
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::ptrdiff_t runtime_configuration::get_stack_size(
        threads::thread_stacksize const stacksize) const
    {
        switch (stacksize)
        {
        case threads::thread_stacksize::medium:
            return medium_stacksize;

        case threads::thread_stacksize::large:
            return large_stacksize;

        case threads::thread_stacksize::huge:
            return huge_stacksize;

        case threads::thread_stacksize::nostack:
            return (std::numeric_limits<std::ptrdiff_t>::max)();

        case threads::thread_stacksize::unknown:
        case threads::thread_stacksize::current:
        default:
            [[fallthrough]];
        case threads::thread_stacksize::small_:
            break;
        }
        return small_stacksize;
    }
}    // namespace hpx::util
