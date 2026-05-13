//  Copyright (c)      2021 ETH Zurich
//  Copyright (c)      2018 Mikael Simberg
//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2010-2011 Phillip LeBlanc, Dylan Stark
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/init_runtime_local/detail/init_logging.hpp>
#include <hpx/init_runtime_local/macros.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/prefix.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/program_options.hpp>

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Forward declarations — replaces sub-module includes to satisfy C++20
// module boundary requirements. Full definitions are available in the
// corresponding .cpp files via their umbrella module includes.
namespace hpx {

    class runtime;

    using startup_function_type = hpx::move_only_function<void()>;
    using shutdown_function_type = hpx::move_only_function<void()>;

    namespace resource {

        enum class partitioner_mode : std::int8_t;
        class partitioner;
    }    // namespace resource
}    // namespace hpx

#if defined(__FreeBSD__)
HPX_CXX_CORE_EXPORT extern HPX_CORE_EXPORT char** freebsd_environ;
HPX_CXX_CORE_EXPORT extern char** environ;
#endif

#include <hpx/config/warnings_prefix.hpp>

namespace hpx {

    namespace detail {

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int init_helper(
            hpx::program_options::variables_map&,
            hpx::function<int(int, char**)> const&);

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void on_exit() noexcept;
        HPX_CXX_CORE_EXPORT [[noreturn]] HPX_CORE_EXPORT void on_abort(
            int signal) noexcept;
    }    // namespace detail

    namespace local {

        namespace detail {

            HPX_CXX_CORE_EXPORT struct dump_config
            {
                explicit dump_config(hpx::runtime const& rt)
                  : rt_(std::cref(rt))
                {
                }

                HPX_CORE_EXPORT void operator()() const;

                std::reference_wrapper<hpx::runtime const> rt_;
            };

            // Default params to initialize the init_params struct
            HPX_CXX_CORE_EXPORT [[maybe_unused]] inline int dummy_argc = 1;
            HPX_CXX_CORE_EXPORT [[maybe_unused]] inline char app_name[256] =
                HPX_APPLICATION_STRING;
            inline char* default_argv[2] = {app_name, nullptr};
            HPX_CXX_CORE_EXPORT [[maybe_unused]] inline char** dummy_argv =
                default_argv;

            // HPX_APPLICATION_STRING is specific to an application and therefore
            // cannot be in the source file
            HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT
                hpx::program_options::options_description const&
                default_desc(char const*);

            // Utilities to init the thread_pools of the resource partitioner
            HPX_CXX_CORE_EXPORT using rp_callback_type =
                hpx::function<void(hpx::resource::partitioner&,
                    hpx::program_options::variables_map const&)>;
        }    // namespace detail

        HPX_CXX_CORE_EXPORT struct init_params
        {
            init_params()
              : rp_mode(static_cast<hpx::resource::partitioner_mode>(0))
            {
                std::strncpy(detail::app_name, HPX_APPLICATION_STRING,
                    sizeof(detail::app_name) - 1);
            }

            std::reference_wrapper<
                hpx::program_options::options_description const>
                desc_cmdline = detail::default_desc(HPX_APPLICATION_STRING);
            std::vector<std::string> cfg;
            mutable startup_function_type startup;
            mutable shutdown_function_type shutdown;
            hpx::resource::partitioner_mode rp_mode;
            hpx::local::detail::rp_callback_type rp_callback;
        };

        namespace detail {

            HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int run_or_start(
                hpx::function<int(
                    hpx::program_options::variables_map& vm)> const& f,
                int argc, char** argv, init_params const& params,
                bool blocking);

            HPX_CXX_CORE_EXPORT inline int init_start_impl(
                hpx::function<int(hpx::program_options::variables_map&)> const&
                    f,
                int argc, char** argv, init_params const& params, bool blocking)
            {
                if (argc == 0 || argv == nullptr)
                {
                    argc = dummy_argc;
                    argv = dummy_argv;
                }

                util::set_hpx_prefix(HPX_PREFIX);
#if defined(__FreeBSD__)
                freebsd_environ = environ;
#endif
                // set a handler for std::abort
                [[maybe_unused]] auto prev_sh =
                    std::signal(SIGABRT, hpx::detail::on_abort);

                [[maybe_unused]] auto ret = std::atexit(hpx::detail::on_exit);
                HPX_ASSERT_MSG(ret == 0, "std::atexit returned error code");

#if defined(HPX_HAVE_CXX11_STD_QUICK_EXIT)
                ret = std::at_quick_exit(hpx::detail::on_exit);
                HPX_ASSERT_MSG(
                    ret == 0, "std::at_quick_exit returned error code");
#endif
                return run_or_start(f, argc, argv, params, blocking);
            }
        }    // namespace detail

        HPX_CXX_CORE_EXPORT inline int init(
            std::function<int(hpx::program_options::variables_map&)> f,
            int argc, char** argv, init_params const& params = init_params())
        {
            return detail::init_start_impl(
                HPX_MOVE(f), argc, argv, params, true);
        }

        HPX_CXX_CORE_EXPORT inline int init(std::function<int(int, char**)> f,
            int argc, char** argv, init_params const& params = init_params())
        {
            hpx::function<int(hpx::program_options::variables_map&)> main_f =
                hpx::bind_back(hpx::detail::init_helper, HPX_MOVE(f));
            return detail::init_start_impl(
                HPX_MOVE(main_f), argc, argv, params, true);
        }

        HPX_CXX_CORE_EXPORT inline int init(std::function<int()> f, int argc,
            char** argv, init_params const& params = init_params())
        {
            hpx::function<int(hpx::program_options::variables_map&)> main_f =
                hpx::bind(HPX_MOVE(f));
            return detail::init_start_impl(
                HPX_MOVE(main_f), argc, argv, params, true);
        }

        HPX_CXX_CORE_EXPORT inline int init(std::nullptr_t, int argc,
            char** argv, init_params const& params = init_params())
        {
            hpx::function<int(hpx::program_options::variables_map&)> main_f;
            return detail::init_start_impl(
                HPX_MOVE(main_f), argc, argv, params, true);
        }

        HPX_CXX_CORE_EXPORT inline bool start(
            std::function<int(hpx::program_options::variables_map&)> f,
            int argc, char** argv, init_params const& params = init_params())
        {
            return 0 ==
                detail::init_start_impl(HPX_MOVE(f), argc, argv, params, false);
        }

        HPX_CXX_CORE_EXPORT inline bool start(std::function<int(int, char**)> f,
            int argc, char** argv, init_params const& params = init_params())
        {
            hpx::function<int(hpx::program_options::variables_map&)> main_f =
                hpx::bind_back(hpx::detail::init_helper, HPX_MOVE(f));
            return 0 ==
                detail::init_start_impl(
                    HPX_MOVE(main_f), argc, argv, params, false);
        }

        HPX_CXX_CORE_EXPORT inline bool start(std::function<int()> f, int argc,
            char** argv, init_params const& params = init_params())
        {
            hpx::function<int(hpx::program_options::variables_map&)> main_f =
                hpx::bind(HPX_MOVE(f));
            return 0 ==
                detail::init_start_impl(
                    HPX_MOVE(main_f), argc, argv, params, false);
        }

        HPX_CXX_CORE_EXPORT inline bool start(std::nullptr_t, int argc,
            char** argv, init_params const& params = init_params())
        {
            hpx::function<int(hpx::program_options::variables_map&)> main_f;
            return 0 ==
                detail::init_start_impl(
                    HPX_MOVE(main_f), argc, argv, params, false);
        }

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int finalize(
            error_code& ec = throws);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int stop(error_code& ec = throws);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int suspend(
            error_code& ec = throws);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int resume(error_code& ec = throws);
    }    // namespace local

    // Allow applications to add a finalizer if HPX_MAIN is set
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT extern void (*on_finalize)();
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
