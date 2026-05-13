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
#include <hpx/modules/functional.hpp>

namespace hpx { namespace program_options {
    class variables_map;
    class options_description;
}}    // namespace hpx::program_options

#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include <hpx/modules/resource_partitioner_mode.hpp>
#include <hpx/modules/runtime_mode.hpp>

// Forward declaration of hpx::runtime to avoid pulling in the full runtime
// header -- the full type is only needed in the .cpp implementations.
namespace hpx {

    class runtime;

    using startup_function_type = hpx::move_only_function<void()>;
    using shutdown_function_type = hpx::move_only_function<void()>;

    namespace resource {

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
                "unknown HPX application";
            inline char* default_argv[2] = {app_name, nullptr};
            HPX_CXX_CORE_EXPORT [[maybe_unused]] inline char** dummy_argv =
                default_argv;

            // "unknown HPX application" is specific to an application and therefore
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
            {
                std::strncpy(detail::app_name, "unknown HPX application",
                    sizeof(detail::app_name) - 1);
            }

            std::reference_wrapper<
                hpx::program_options::options_description const>
                desc_cmdline = detail::default_desc("unknown HPX application");
            std::vector<std::string> cfg;
            mutable startup_function_type startup;
            mutable shutdown_function_type shutdown;
            hpx::resource::partitioner_mode rp_mode =
                ::hpx::resource::partitioner_mode::default_;
            hpx::local::detail::rp_callback_type rp_callback;
        };

        namespace detail {

            HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT int run_or_start(
                hpx::function<int(
                    hpx::program_options::variables_map& vm)> const& f,
                int argc, char** argv, init_params const& params,
                bool blocking);

            HPX_CXX_CORE_EXPORT int init_start_impl(
                hpx::function<int(hpx::program_options::variables_map&)> const&
                    f,
                int argc, char** argv, init_params const& params,
                bool blocking);
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
