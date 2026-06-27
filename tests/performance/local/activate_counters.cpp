//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013      Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>

#include "activate_counters.hpp"

#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hpx { namespace util {

    activate_counters::activate_counters(std::vector<std::string> const& names)
      : names_(names)
    {
        start_counters();
    }

    activate_counters::~activate_counters()
    {
        stop_counters();
    }

    bool activate_counters::find_counter(
        performance_counters::counter_info const& info, error_code& ec)
    {
        hpx::id_type id = performance_counters::get_counter(info.fullname_, ec);
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "activate_counters::find_counter",
                hpx::util::format("unknown performance counter: '{1}' ({2})",
                    info.fullname_, ec.get_message()));
            return false;
        }

        names_.push_back(info.fullname_);
        ids_.push_back(id);
        uoms_.push_back(info.unit_of_measure_);
        return true;
    }

    void activate_counters::find_counters()
    {
        std::vector<std::string> names;
        std::swap(names, names_);

        names_.reserve(names.size());
        if (ids_.empty())
        {
            performance_counters::discover_counter_func func(
                hpx::bind_front(&activate_counters::find_counter, this));

            ids_.reserve(names.size());
            uoms_.reserve(names.size());
            for (std::string& name : names)
            {
                // do INI expansion on counter name
                util::expand(name);

                // find matching counter type
                performance_counters::discover_counter_type(name, func,
                    performance_counters::discover_counters_mode::full);
            }
        }

        HPX_ASSERT(ids_.size() == names_.size());
        HPX_ASSERT(ids_.size() == uoms_.size());
    }

    ///////////////////////////////////////////////////////////////////////////
    void activate_counters::start_counters(error_code& ec)
    {
        using performance_counters::performance_counter;

        // add counter prefix, if necessary
        for (std::string& name : names_)
            performance_counters::ensure_counter_prefix(name);

        find_counters();

        // Query the performance counters.
        std::vector<future<bool>> started;

        started.reserve(ids_.size());
        for (std::size_t i = 0; i != ids_.size(); ++i)
        {
            performance_counter c(ids_[i]);
            started.push_back(c.start());
        }

        // wait for all counters to be started
        hpx::wait_all(started);

        for (future<bool>& f : started)
        {
            if (f.has_exception())
            {
                if (&ec == &hpx::throws)
                {
                    f.get();
                }
                else
                {
                    ec = make_error_code(f.get_exception_ptr());
                }
                return;
            }
        }
    }

    void activate_counters::stop_counters(error_code& ec)
    {
        if (ids_.empty())
        {
            // start has not been called yet
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "activate_counters::stop_counters",
                "The counters to be evaluated have not been initialized yet");
            return;
        }

        // Query the performance counters.
        using performance_counters::performance_counter;
        std::vector<future<bool>> stopped;

        stopped.reserve(ids_.size());
        for (std::size_t i = 0; i != ids_.size(); ++i)
        {
            performance_counter c(ids_[i]);
            stopped.push_back(c.stop());
        }

        // wait for all counters to be started
        hpx::wait_all(stopped);

        ids_.clear();    // give up control over all performance counters

        for (future<bool>& f : stopped)
        {
            if (f.has_exception())
            {
                if (&ec == &hpx::throws)
                {
                    f.get();
                }
                else
                {
                    ec = make_error_code(f.get_exception_ptr());
                }
                return;
            }
        }
    }

    void activate_counters::reset_counters(error_code& ec)
    {
        if (ids_.empty())
        {
            // start has not been called yet
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "activate_counters::reset_counters",
                "The counters to be evaluated have not been initialized yet");
            return;
        }

        // Query the performance counters.
        using performance_counters::performance_counter;
        std::vector<future<void>> reset;

        reset.reserve(ids_.size());
        for (std::size_t i = 0; i != ids_.size(); ++i)
        {
            performance_counter c(ids_[i]);
            reset.push_back(c.reset());
        }

        // wait for all counters to be started
        hpx::wait_all(reset);

        for (future<void>& f : reset)
        {
            if (f.has_exception())
            {
                if (&ec == &hpx::throws)
                {
                    f.get();
                }
                else
                {
                    ec = make_error_code(f.get_exception_ptr());
                }
                return;
            }
        }
    }

    std::vector<future<performance_counters::counter_value>>
    activate_counters::evaluate_counters(
        launch::async_policy, bool reset, error_code& ec)
    {
        std::vector<future<performance_counters::counter_value>> values;

        if (ids_.empty())
        {
            // start has not been called yet
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "activate_counters::evaluate_counters_async",
                "The counters to be evaluated have not been initialized yet");
            return values;
        }

        values.reserve(ids_.size());
        using hpx::performance_counters::performance_counter;
        for (std::size_t i = 0; i != ids_.size(); ++i)
        {
            performance_counter c(ids_[i]);
            values.push_back(c.get_counter_value(reset));
        }
        return values;
    }

    std::vector<performance_counters::counter_value>
    activate_counters::evaluate_counters(
        launch::sync_policy, bool reset, error_code& ec)
    {
        std::vector<future<performance_counters::counter_value>> futures =
            evaluate_counters(launch::async, reset, ec);

        return unwrap(futures);
    }
}}    // namespace hpx::util
