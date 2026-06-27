//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c) 2011-2012 Maciej Brodowicz
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_PAPI)
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/components/performance_counters/papi/server/papi.hpp>
#include <hpx/components/performance_counters/papi/util/papi.hpp>

#include <cctype>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality
HPX_REGISTER_COMPONENT_MODULE_DYNAMIC()

///////////////////////////////////////////////////////////////////////////////
typedef hpx::components::component<
    hpx::performance_counters::papi::server::papi_counter>
    papi_counter_type;

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace performance_counters { namespace papi {
    using hpx::performance_counters::counter_info;
    using hpx::performance_counters::counter_path_elements;
    using hpx::program_options::options_description;
    using hpx::program_options::variables_map;
    using hpx::util::thread_mapper;
    using util::papi_call;

#define NS_STR "hpx::performance_counters::papi::"

    // create PAPI counter
    hpx::naming::gid_type create_papi_counter(
        counter_info const& info, hpx::error_code& ec)
    {
        // verify the validity of the counter instance name
        hpx::performance_counters::counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
            return hpx::naming::invalid_gid;

        if (paths.parentinstance_is_basename_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                NS_STR "create_papi_counter()",
                "unsupported counter instance parent name: " +
                    paths.parentinstancename_);
            return hpx::naming::invalid_gid;
        }

        // validate thread label taken from counter name
        std::string label;
        std::uint32_t tix = util::get_counter_thread(paths, label);
        if (tix == thread_mapper::invalid_index)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                NS_STR "create_papi_counter()",
                "cannot find thread specified in " + info.fullname_);
        }
        // create a local PAPI counter component
        hpx::naming::gid_type id;
        try
        {
            id = hpx::components::server::construct<papi_counter_type>(info);
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

    // bool is true when group is enumerated
    typedef std::pair<std::string, bool> thread_category;

    template <typename T>
    void discard_result(T const&)
    {
    }

    // find threads or their categories
    void find_threads(std::set<std::string>& tdesc,
        hpx::performance_counters::discover_counters_mode mode)
    {
        hpx::util::thread_mapper& tm = get_runtime().get_thread_mapper();
        std::string label;
        std::uint32_t tix = 0;
        while (!((label = tm.get_thread_label(tix++)).empty()))
        {
            if (mode == discover_counters_mode::minimal)
            {
                size_t pos = label.find_last_of('#');
                if (pos == label.npos)
                    tdesc.insert(label);
                else
                    tdesc.insert(label.substr(0, pos) + "#*");
            }
            else
                tdesc.insert(label);
        }
    }

    template <class T>
    bool discover_events(counter_path_elements& cpe,
        std::set<std::string> const& tdesc,
        hpx::performance_counters::counter_info& info, T& gen,
        hpx::performance_counters::discover_counter_func const& f,
        hpx::error_code& ec)
    {
        for (auto gi = hpx::util::generator_iterator<T>(&gen); *gi != nullptr;
            ++gi)
        {
            std::set<std::string>::const_iterator it;
            // iterate over known thread names
            for (it = tdesc.begin(); it != tdesc.end(); ++it)
            {
                cpe.instancename_ = *it;
                cpe.countername_ = (*gi)->symbol;

                hpx::performance_counters::counter_status status =
                    get_counter_name(cpe, info.fullname_, ec);
                if (!status_is_valid(status))
                    return false;
                std::string evstr((*gi)->long_descr);
                info.helptext_ = "returns the count of occurrences of \"" +
                    evstr + "\" in " + (*it) + " instance";
                if (!f(info, std::ref(ec)) || ec)
                    return false;
            }
        }
        return true;
    }

    // discover available PAPI counters
    static bool discover_papi_counters_helper(
        hpx::performance_counters::counter_info const& info,
        hpx::error_code& ec,
        hpx::performance_counters::discover_counter_func const& f,
        hpx::performance_counters::discover_counters_mode mode)
    {
        hpx::performance_counters::counter_info cnt_info = info;

        // decompose the counter name
        counter_path_elements p;
        hpx::performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);
        if (!status_is_valid(status))
            return false;

        if (p.objectname_ == "papi" && p.parentinstancename_.empty() &&
            p.instancename_.empty() && p.countername_.empty())
        {    // discover all available PAPI counters
            // obtain known OS thread labels
            std::set<std::string> tdesc;
            find_threads(tdesc, mode);

            // fill in common path segments for all counters
            counter_path_elements cpe;
            cpe.objectname_ = p.objectname_;
            cpe.parentinstancename_ = "locality#*";
            cpe.parentinstanceindex_ = -1;
            cpe.instanceindex_ = -1;

            // enumerate PAPI presets
            {
                util::all_preset_info_gen gen;
                if (!discover_events(cpe, tdesc, cnt_info, gen, f, ec))
                    return false;
            }
            // enumerate PAPI native events
            for (int ci = 0; ci < PAPI_num_components(); ++ci)
            {
                util::native_info_gen gen(ci);
                if (!discover_events(cpe, tdesc, cnt_info, gen, f, ec))
                    return false;
            }
        }
        else
        {    // no wildcards expected here
            if (p.instanceindex_ < 0 || p.parentinstanceindex_ < 0)
            {
                if (!f(cnt_info, ec) || ec)
                    return false;
            }
            else
            {
                hpx::util::thread_mapper& tm =
                    get_runtime().get_thread_mapper();
                std::string lab =
                    p.instancename_ + "#" + std::to_string(p.instanceindex_);
                if (p.objectname_ == "papi" &&
                    p.parentinstancename_ == "locality" &&
                    tm.get_thread_index(lab) != tm.invalid_index &&
                    !p.countername_.empty())
                {    // validate specific PAPI event
                    int code;
                    if (PAPI_event_name_to_code(
                            const_cast<char*>(p.countername_.c_str()), &code) !=
                        PAPI_OK)
                        return false;
                    hpx::performance_counters::counter_status status =
                        get_counter_name(p, cnt_info.fullname_, ec);
                    if (!status_is_valid(status))
                        return false;
                    // this is just validation, no helptext required
                    if (!f(cnt_info, ec) || ec)
                        return false;
                }
                // unsupported path components, let runtime handle this
                else if (!f(cnt_info, ec) || ec)
                {
                    return false;
                }
            }
        }

        if (&ec != &hpx::throws)
            ec = hpx::make_success_code();
        return true;
    }

    // wrap the actual discoverer allowing HPX to expand all wildcards it knows about
    bool discover_papi_counters(
        hpx::performance_counters::counter_info const& info,
        hpx::performance_counters::discover_counter_func const& f,
        hpx::performance_counters::discover_counters_mode mode,
        hpx::error_code& ec)
    {
        return performance_counters::locality_thread_counter_discoverer(info,
            hpx::bind_back(&discover_papi_counters_helper, f, mode), mode, ec);
    }

    // enable multiplexing with specified period for thread tix
    bool enable_multiplexing(int evset, int interval)
    {
        switch (PAPI_get_multiplex(evset))
        {
        case PAPI_OK:    // not yet multiplexed
            if (PAPI_assign_eventset_component(evset, 0) != PAPI_OK)
                return false;
            if (!interval)
            {    // enable multiplexing with default interval
                return PAPI_set_multiplex(evset) != PAPI_OK;
            }
            break;
        case PAPI_EINVAL:    // already multiplexed
            if (!interval)
                return true;
            break;    // still need to change interval
        default:      // error
            return false;
        }
        // force the requested interval
        PAPI_option_t popt;
        popt.multiplex.eventset = evset;
        popt.multiplex.ns = interval;
        popt.multiplex.flags = PAPI_MULTIPLEX_DEFAULT;
        return PAPI_set_opt(PAPI_MULTIPLEX, &popt) == PAPI_OK;
    }

    // startup function for PAPI counter component
    void startup()
    {
        using namespace hpx::performance_counters;

        // define & install generic PAPI counter type
        generic_counter_type_data const papi_cnt_type = {"/papi",
            counter_type::raw,
            "the current count of occurrences of a specific PAPI event",
            performance_counters::HPX_PERFORMANCE_COUNTER_V1,
            &create_papi_counter, &discover_papi_counters, ""};
        install_counter_types(&papi_cnt_type, 1);

        // deferred options
        variables_map vm = util::get_options();
        if (vm.count("hpx:papi-event-info"))
        {
            std::string v = vm["hpx:papi-event-info"].as<std::string>();
            util::list_events(v);
        }
    }

    bool check_startup(
        hpx::startup_function_type& startup_func, bool& pre_startup)
    {
        // PAPI initialization
        if (PAPI_is_initialized() == PAPI_NOT_INITED)
        {
            if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_success,
                    "hpx::performance_counters::papi::check_startup()",
                    "PAPI library initialization failed (version mismatch)");
            }
        }
        // retrieve command line
        variables_map vm = util::get_options();

        if (util::check_options(vm))
        {    // perform full module startup (counters will be used)
            startup_func = startup;
            pre_startup = true;
            return true;
        }

        return false;
    }

}}}    // namespace hpx::performance_counters::papi

///////////////////////////////////////////////////////////////////////////////
// register a startup function for PAPI performance counter
HPX_REGISTER_STARTUP_MODULE_DYNAMIC(
    hpx::performance_counters::papi::check_startup)

// register related command line options
HPX_REGISTER_COMMANDLINE_MODULE_DYNAMIC(
    hpx::performance_counters::papi::util::get_options_description)

#endif
