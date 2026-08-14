//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <cstdint>

#include "server/sine.hpp"

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality, We register the module dynamically
// as no executable links against it.
HPX_REGISTER_COMPONENT_MODULE_DYNAMIC()

///////////////////////////////////////////////////////////////////////////////
typedef hpx::components::component<
    ::performance_counters::sine::server::sine_counter>
    sine_counter_type;

///////////////////////////////////////////////////////////////////////////////
namespace performance_counters { namespace sine {
    ///////////////////////////////////////////////////////////////////////////
    // This function will be invoked whenever the implicit counter is queried.
    std::int64_t immediate_sine(bool)
    {
        static std::uint64_t started_at =
            hpx::chrono::high_resolution_clock::now();

        std::uint64_t up_time =
            hpx::chrono::high_resolution_clock::now() - started_at;
        return std::int64_t(
            std::sin(static_cast<double>(up_time) / 1e10) * 100000.);
    }

    ///////////////////////////////////////////////////////////////////////////
    // This will be called to return special command line options supported by
    // this component.
    hpx::program_options::options_description command_line_options()
    {
        hpx::program_options::options_description opts(
            "Additional command line options for the sine component");
        opts.add_options()("sine",
            "enables the performance counters implemented by the "
            "sine component");
        return opts;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parse the command line to figure out whether the sine performance
    // counters need to be created.
    bool need_perf_counters()
    {
        using hpx::program_options::options_description;
        using hpx::program_options::variables_map;
        using hpx::util::retrieve_commandline_arguments;

        // Retrieve command line using the HPX.ProgramOptions library.
        variables_map vm;
        if (!retrieve_commandline_arguments(command_line_options(), vm))
        {
            HPX_THROW_EXCEPTION(hpx::error::commandline_option_error,
                "sine::need_perf_counters",
                "Failed to handle command line options");
            return false;
        }

        // We enable the performance counters if --sine is specified on the
        // command line.
        return vm.count("sine") != 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Discoverer for the explicit (hand-rolled performance counter. The
    // purpose of this function is to invoke the supplied function f for all
    // allowed counter instance names supported by the counter type this
    // function has been registered with.
    bool explicit_sine_counter_discoverer(
        hpx::performance_counters::counter_info const& info,
        hpx::performance_counters::discover_counter_func const& f,
        hpx::performance_counters::discover_counters_mode mode,
        hpx::error_code& ec)
    {
        hpx::performance_counters::counter_info i = info;

        // compose the counter name templates
        hpx::performance_counters::counter_path_elements p;
        hpx::performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);
        if (!status_is_valid(status))
            return false;

        if (mode ==
                hpx::performance_counters::discover_counters_mode::minimal ||
            p.parentinstancename_.empty() || p.instancename_.empty())
        {
            if (p.parentinstancename_.empty())
            {
                p.parentinstancename_ = "locality#*";
                p.parentinstanceindex_ = -1;
            }

            if (p.instancename_.empty())
            {
                p.instancename_ = "instance#*";
                p.instanceindex_ = -1;
            }

            status = get_counter_name(p, i.fullname_, ec);
            if (!status_is_valid(status) || !f(i, ec) || ec)
                return false;
        }
        else if (p.instancename_ == "instance#*")
        {
            HPX_ASSERT(mode ==
                hpx::performance_counters::discover_counters_mode::full);

            // FIXME: expand for all instances
            p.instancename_ = "instance";
            p.instanceindex_ = 0;
            status = get_counter_name(p, i.fullname_, ec);
            if (!status_is_valid(status) || !f(i, ec) || ec)
                return false;
        }
        else if (!f(i, ec) || ec)
        {
            return false;
        }

        if (&ec != &hpx::throws)
            ec = hpx::make_success_code();

        return true;    // everything is ok
    }

    ///////////////////////////////////////////////////////////////////////////
    // Creation function for explicit sine performance counter. It's purpose is
    // to create and register a new instance of the given name (or reuse an
    // existing instance).
    hpx::naming::gid_type explicit_sine_counter_creator(
        hpx::performance_counters::counter_info const& info,
        hpx::error_code& ec)
    {
        // verify the validity of the counter instance name
        hpx::performance_counters::counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
            return hpx::naming::invalid_gid;

        if (paths.parentinstance_is_basename_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "sine::explicit_sine_counter_creator",
                "invalid counter instance parent name: " +
                    paths.parentinstancename_);
            return hpx::naming::invalid_gid;
        }

        // create individual counter
        if (paths.instancename_ == "instance" && paths.instanceindex_ != -1)
        {
            // make sure parent instance name is set properly
            hpx::performance_counters::counter_info complemented_info = info;
            complement_counter_info(complemented_info, info, ec);
            if (ec)
                return hpx::naming::invalid_gid;

            // create the counter as requested
            hpx::naming::gid_type id;
            try
            {
                // create the 'sine' performance counter component locally, we
                // only get here if this instance does not exist yet
                id = hpx::components::server::construct<sine_counter_type>(
                    complemented_info);
            }
            catch (hpx::exception const& e)
            {
                if (&ec == &hpx::throws)
                    throw;
                ec = make_error_code(e.get_error(), e.what());
                return hpx::naming::invalid_gid;
            }

            if (&ec != &hpx::throws)
                ec = hpx::make_success_code();
            return id;
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "sine::explicit_sine_counter_creator",
            "invalid counter instance name: " + paths.instancename_);
        return hpx::naming::invalid_gid;
    }

    ///////////////////////////////////////////////////////////////////////////
    // This function will be registered as a startup function for HPX below.
    //
    // That means it will be executed in a HPX-thread before hpx_main, but after
    // the runtime has been initialized and started.
    void startup()
    {
        using namespace hpx::performance_counters;
        using hpx::placeholders::_1;
        using hpx::placeholders::_2;

        // define the counter types
        generic_counter_type_data const counter_types[] = {
            // We assume that valid counter names have the following scheme:
            //
            //  /sine(locality#<locality_id>/instance#<instance_id>)/immediate/explicit
            //
            // where '<locality_id>' is the number of the locality the
            // counter has to be instantiated on and '<instance_id>' is the
            // instance number to use for the particular counter. We allow
            // any arbitrary number of instances.
            {"/sine/immediate/explicit", counter_type::raw,
                "returns the current value of a sine wave calculated over "
                "an arbitrary time line (explicit, hand-rolled version)",
                hpx::performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                &explicit_sine_counter_creator,
                &explicit_sine_counter_discoverer, ""},
            // We assume that valid counter names have the following scheme:
            //
            //  /sine(locality#<locality_id>/total)/immediate/implicit
            //
            // where '<locality_id>' is the number of the locality the
            // counter has to be instantiated on. The function 'immediate_sine'
            // is used as the source of counter data for the created counter.
            {"/sine/immediate/implicit", counter_type::raw,
                "returns the current value of a sine wave calculated over "
                "an arbitrary time line (implicit version, using HPX "
                "facilities)",
                hpx::performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                hpx::bind(
                    &hpx::performance_counters::locality_raw_counter_creator,
                    _1, &immediate_sine, _2),
                &hpx::performance_counters::locality_counter_discoverer, ""}};

        // Install the counter types, de-installation of the types is handled
        // automatically.
        install_counter_types(
            counter_types, sizeof(counter_types) / sizeof(counter_types[0]));
    }

    ///////////////////////////////////////////////////////////////////////////
    bool get_startup(
        hpx::startup_function_type& startup_func, bool& pre_startup)
    {
        // exit silently if this gets loaded outside of the sine_client example
        if (hpx::get_config_entry("hpx.components.sine.enabled", "0") == "0")
        {
            return false;
        }

        // check whether the performance counters need to be enabled
        if (!need_perf_counters())
        {
            HPX_THROW_EXCEPTION(hpx::error::dynamic_link_failure,
                "performance_counters::sine::get_startup",
                "the sine component is not enabled on the commandline "
                "(--sine), bailing out");
            return false;
        }

        // return our startup-function if performance counters are required
        startup_func = startup;    // function to run during startup
        pre_startup = true;        // run 'startup' as pre-startup function
        return true;
    }
}}    // namespace performance_counters::sine

///////////////////////////////////////////////////////////////////////////////
// Register a startup function which will be called as a HPX-thread during
// runtime startup. We use this function to register our performance counter
// type and performance counter instances.
//
// Note that this macro can be used not more than once in one module.
HPX_REGISTER_STARTUP_MODULE_DYNAMIC(::performance_counters::sine::get_startup)

///////////////////////////////////////////////////////////////////////////////
// Register a function to be called to populate the special command line
// options supported by this component.
//
// Note that this macro can be used not more than once in one module.
HPX_REGISTER_COMMANDLINE_MODULE_DYNAMIC(
    ::performance_counters::sine::command_line_options)
#endif
