//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_ITTNOTIFY) && HPX_HAVE_ITTNOTIFY != 0

#include <hpx/config/thread_name.hpp>
#include <hpx/itt_notify/detail/use_ittnotify_api.hpp>
#include <hpx/modules/itt_notify.hpp>
#include <hpx/tracing/tracing.hpp>

#include <cstddef>
#include <map>
#include <string>
#include <utility>

namespace hpx::tracing {

    ////////////////////////////////////////////////////////////////////////////
    // itt_counters map for caching counter metadata
    // Note: This map is not protected against concurrent modifications.
    // It is assumed that counters are created during startup and only
    // read/sampled concurrently during execution.
    static std::map<std::string, util::itt::counter> itt_counters_;

    ////////////////////////////////////////////////////////////////////////////
    // loop_context

    loop_context::loop_context() noexcept
      : task_id("task_id")
      , task_phase("task_phase")
    {
    }

    loop_context::~loop_context() = default;

    ////////////////////////////////////////////////////////////////////////////
    // region

    util::itt::task region::make_task(
        loop_context& ctx, region_init_data const& data)
    {
        if (data.is_address_type)
        {
            return util::itt::task(ctx.thread_domain,
                util::itt::string_handle("address"), data.address);
        }
        if (data.handle)
        {
            return util::itt::task(ctx.thread_domain, data.handle);
        }
        return util::itt::task(
            ctx.thread_domain, util::itt::string_handle(data.name));
    }

    region::region(loop_context& ctx, region_init_data const& data, std::size_t)
      : cctx(ctx.stack_ctx, !data.is_stackless)
      , task(make_task(ctx, data))
    {
        task.add_metadata(ctx.task_id, data.thread_ptr);
        task.add_metadata(ctx.task_phase, data.thread_phase);
    }

    region::~region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // counters

    void create_counter(
        std::string const& full_name, std::string const& short_name) noexcept
    {
        if (use_ittnotify_api)
        {
            // check if the counter name already exists
            if (itt_counters_.find(full_name) == itt_counters_.end())
            {
                itt_counters_.insert(std::make_pair(full_name,
                    util::itt::counter(short_name.c_str(),
                        hpx::detail::thread_name().c_str(),
                        __itt_metadata_double)));
            }
        }
    }

    void sample_counter(
        std::string const& full_name, std::string const&, double value) noexcept
    {
        if (use_ittnotify_api)
        {
            auto it = itt_counters_.find(full_name);
            if (it != itt_counters_.end())
            {
                (*it).second.set_value(value);
            }
        }
    }

}    // namespace hpx::tracing

#endif
