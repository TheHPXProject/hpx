////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <hpx/config.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/version.hpp>

#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/manage_counter_type.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hpx::performance_counters {

    struct manage_counter_type
    {
        manage_counter_type(counter_info const& info)
          : status_(counter_status::invalid_data)
          , info_(info)
        {
        }

        ~manage_counter_type()
        {
            hpx::error_code ec;
            uninstall(ec);
        }

        counter_status install(error_code& ec = throws)
        {
            if (counter_status::invalid_data != status_)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status,
                    "manage_counter_type::install",
                    "counter type {} has been already installed.",
                    info_.fullname_);
                return counter_status::invalid_data;
            }

            return status_ = add_counter_type(info_, ec);
        }

        counter_status install(create_counter_func const& create_counter,
            discover_counters_func const& discover_counters,
            error_code& ec = throws)
        {
            if (counter_status::invalid_data != status_)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status,
                    "manage_counter_type::install",
                    "generic counter type {} has been already installed.",
                    info_.fullname_);
                return counter_status::invalid_data;
            }

            return status_ = add_counter_type(
                       info_, create_counter, discover_counters, ec);
        }

        void uninstall(error_code& ec = throws)
        {
            if (counter_status::invalid_data != status_)
                remove_counter_type(info_, ec);    // ignore errors
        }

    private:
        counter_status status_;
        counter_info info_;
    };

    namespace {

        void counter_type_shutdown(
            std::shared_ptr<manage_counter_type> const& p)
        {
            error_code ec(throwmode::lightweight);
            p->uninstall(ec);
        }
    }    // namespace

    ///////////////////////////////////////////////////////////////////////////
    counter_status install_counter_type(std::string const& name,
        counter_type type, std::string const& helptext, std::string const& uom,
        std::uint32_t version, error_code& ec)
    {
        counter_info info(type, name, helptext,
            version ? version : HPX_PERFORMANCE_COUNTER_V1, uom);
        std::shared_ptr<manage_counter_type> p =
            std::make_shared<manage_counter_type>(info);

        // Install the counter type.
        p->install(ec);

        // Register the shutdown function which will clean up this counter type.
        get_runtime().add_shutdown_function(
            hpx::bind_front(&counter_type_shutdown, p));
        return counter_status::valid_data;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Install a new generic performance counter type in a way, which will
    // uninstall it automatically during shutdown.
    counter_status install_counter_type(std::string const& name,
        counter_type type, std::string const& helptext,
        create_counter_func const& create_counter,
        discover_counters_func const& discover_counters, std::uint32_t version,
        std::string const& uom, error_code& ec)
    {
        counter_info info(type, name, helptext,
            version ? version : HPX_PERFORMANCE_COUNTER_V1, uom);
        std::shared_ptr<manage_counter_type> p =
            std::make_shared<manage_counter_type>(info);

        // Install the counter type.
        p->install(create_counter, discover_counters, ec);

        // Register the shutdown function which will clean up this counter type.
        get_runtime().add_shutdown_function(
            hpx::bind_front(&counter_type_shutdown, p));
        return counter_status::valid_data;
    }

    // Install a new generic performance counter type which uses a function to
    // provide the data in a way, which will uninstall it automatically during
    // shutdown.
    counter_status install_counter_type(std::string const& name,
        hpx::function<std::int64_t(bool)> const& counter_value,
        std::string const& helptext, std::string const& uom, counter_type type,
        error_code& ec)
    {
        using hpx::placeholders::_1;
        using hpx::placeholders::_2;
        return install_counter_type(name, type, helptext,
            hpx::bind(&hpx::performance_counters::locality_raw_counter_creator,
                _1, counter_value, _2),
            &hpx::performance_counters::locality_counter_discoverer,
            HPX_PERFORMANCE_COUNTER_V1, uom, ec);
    }

    counter_status install_counter_type(std::string const& name,
        hpx::function<std::vector<std::int64_t>(bool)> const& counter_value,
        std::string const& helptext, std::string const& uom, error_code& ec)
    {
        using hpx::placeholders::_1;
        using hpx::placeholders::_2;
        return install_counter_type(name, counter_type::raw_values, helptext,
            hpx::bind(
                &hpx::performance_counters::locality_raw_values_counter_creator,
                _1, counter_value, _2),
            &hpx::performance_counters::locality_counter_discoverer,
            HPX_PERFORMANCE_COUNTER_V1, uom, ec);
    }

    /// Install several new performance counter types in a way, which will
    /// uninstall them automatically during shutdown.
    void install_counter_types(generic_counter_type_data const* data,
        std::size_t count, error_code& ec)
    {
        for (std::size_t i = 0; i < count; ++i)
        {
            install_counter_type(data[i].name_, data[i].type_,
                data[i].helptext_, data[i].create_counter_,
                data[i].discover_counters_, data[i].version_,
                data[i].unit_of_measure_, ec);
            if (ec)
                break;
        }
    }
}    // namespace hpx::performance_counters
