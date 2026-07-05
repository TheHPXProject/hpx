//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_TRACY)

#include <hpx/tracing/tracing.hpp>

#include <cstddef>
#include <string>

namespace hpx::tracing {

    ////////////////////////////////////////////////////////////////////////////
    // region

    hpx::tracy::region region::create_tracy_region(
        region_init_data const& data, std::size_t const num_thread) noexcept
    {
        bool const enabled = data.name != nullptr && !data.is_stackless;
        return hpx::tracy::region(
            data.name, num_thread, data.thread_phase, enabled);
    }

    region::region(loop_context&, region_init_data const& data,
        std::size_t const num_thread) noexcept
      : impl(create_tracy_region(data, num_thread))
    {
    }

    region::~region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // mark_event

    mark_event::mark_event(char const* name) noexcept
      : impl(name)
    {
    }

    mark_event::~mark_event() = default;

    ////////////////////////////////////////////////////////////////////////////
    // fiber_region

    hpx::tracy::fiber_region fiber_region::create_tracy_fiber_region(
        fiber_region_init_data const& data,
        std::size_t const num_thread) noexcept
    {
        bool const enabled = data.name != nullptr && !data.is_stackless;
        char const* fiber_name = enabled ? data.fiber_name : nullptr;

        // Use num_thread as color seed so each worker thread gets a distinct
        // color on the fiber track in Tracy.
        auto const color =
            static_cast<std::size_t>(num_thread + 1) * 0x9e3779b9;
        return hpx::tracy::fiber_region(fiber_name, data.name, color, enabled);
    }

    fiber_region::fiber_region(fiber_region_init_data const& data,
        std::size_t const num_thread) noexcept
      : impl(create_tracy_fiber_region(data, num_thread))
    {
    }

    fiber_region::~fiber_region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // fiber_suspend_region

    fiber_suspend_region::fiber_suspend_region(char const* desc) noexcept
      : impl(desc)
    {
    }

    fiber_suspend_region::~fiber_suspend_region() = default;

    ////////////////////////////////////////////////////////////////////////////
    // lock_context

    lock_context::lock_context(
        char const* name, void const* /* addr */) noexcept
      : impl(hpx::tracy::create(name))
    {
    }

    lock_context::lock_context(
        char const* prefix, char const* suffix, void const* /* addr */) noexcept
      : impl(hpx::tracy::create(std::string(prefix) + suffix))
    {
    }

    lock_context::~lock_context()
    {
        hpx::tracy::destroy(impl);
    }

    bool lock_context::before_lock() const noexcept
    {
        return hpx::tracy::lock_prepare(impl);
    }

    void lock_context::after_lock() const noexcept
    {
        hpx::tracy::lock_acquired(impl);
    }

    void lock_context::after_try_lock(bool acquired) const noexcept
    {
        hpx::tracy::lock_acquired(impl, acquired);
    }

    void lock_context::before_unlock() const noexcept {}

    void lock_context::after_unlock() const noexcept
    {
        hpx::tracy::lock_released(impl);
    }

    ////////////////////////////////////////////////////////////////////////////
    // set_thread_name

    void set_thread_name(char const* name) noexcept
    {
        hpx::tracy::set_thread_name(name);
    }

    ////////////////////////////////////////////////////////////////////////////
    // rename_region

    char const* rename_region(char const* name) noexcept
    {
        return hpx::tracy::detail::rename_region(name);
    }

    ////////////////////////////////////////////////////////////////////////////
    // counters

    void create_counter(
        std::string const&, std::string const& short_name) noexcept
    {
        hpx::tracy::create_counter(short_name);
    }

    void sample_counter(std::string const&, std::string const& short_name,
        double value) noexcept
    {
        hpx::tracy::sample_value(short_name, value);
    }

}    // namespace hpx::tracing

#endif
