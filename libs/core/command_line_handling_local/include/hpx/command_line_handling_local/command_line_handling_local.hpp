//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/util.hpp>

#include <cstddef>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::local::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT command_line_handling
    {
        command_line_handling(hpx::util::runtime_configuration rtcfg,
            std::vector<std::string> ini_config,
            hpx::function<int(hpx::program_options::variables_map& vm)>
                hpx_main_f);

        int call(hpx::program_options::options_description const& desc_cmdline,
            int argc, char** argv);

        hpx::program_options::variables_map vm_;
        hpx::util::runtime_configuration rtcfg_;

        std::vector<std::string> ini_config_;
        hpx::function<int(hpx::program_options::variables_map& vm)> hpx_main_f_;

        std::size_t num_threads_;
        std::size_t num_cores_;
        std::size_t pu_step_;
        std::size_t pu_offset_;
        std::string queuing_;
        std::string affinity_domain_;
        std::string affinity_bind_;
        std::size_t numa_sensitive_;
        bool use_process_mask_;
        bool cmd_line_parsed_;
        bool info_printed_;

    protected:
        // Helper functions for checking command line options
        void check_affinity_domain() const;
        void check_affinity_description() const;
        void check_pu_offset() const;
        void check_pu_step() const;

        bool handle_arguments(util::manage_config& cfgmap,
            hpx::program_options::variables_map& vm,
            std::vector<std::string>& ini_config);

        static void enable_logging_settings(
            hpx::program_options::variables_map& vm,
            std::vector<std::string>& ini_config);

        void store_command_line(int argc, char** argv);
        void store_unregistered_options(std::string const& cmd_name, int argc,
            char* argv[], std::vector<std::string> const& unregistered_options);
        bool handle_help_options(
            hpx::program_options::options_description const& help);

        void handle_attach_debugger();

        std::vector<std::string> preprocess_config_settings(
            int argc, char** argv);

        int finalize_commandline_handling(int argc, char** argv,
            hpx::program_options::options_description const& help,
            std::vector<std::string> const& unregistered_options);

        void reconfigure(util::manage_config& cfgmap,
            hpx::program_options::variables_map& prevm);

        void handle_high_priority_threads(
            hpx::program_options::variables_map const& vm,
            std::vector<std::string>& ini_config) const;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string
    runtime_configuration_string(command_line_handling const& cfg);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::vector<std::string>
    prepend_options(std::vector<std::string>&& args, std::string&& options);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string convert_to_log_file(
        std::string const& dest);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t handle_num_cores_default(
        util::manage_config& cfgmap,
        hpx::program_options::variables_map const& vm, std::size_t num_threads,
        std::size_t num_default_cores);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
    get_number_of_default_threads(bool use_process_mask);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_number_of_default_cores(
        bool use_process_mask);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void print_config(
        std::vector<std::string> const& ini_config);
}    // namespace hpx::local::detail

#include <hpx/config/warnings_suffix.hpp>
