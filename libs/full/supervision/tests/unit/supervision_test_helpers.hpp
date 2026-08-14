//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/supervision.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
// publish_event() now validates lifecycle event transitions and rejects
// invalid ones (see hpx::supervision::is_valid_transition()). Each test
// therefore needs its own private target: reusing hpx::find_here() as a
// shared key across independent tests would let one test's published
// history make a later test's (otherwise legal) first event look like an
// illegal transition. make_test_target() hands out a fresh id that is only
// ever used as a lookup key by the supervision manager, so it does not need
// to name a real, live component.
inline hpx::id_type make_test_target()
{
    static std::atomic<std::uint64_t> counter{1};
    hpx::naming::gid_type const gid(
        0x1ull, counter.fetch_add(1, std::memory_order_relaxed));
    return hpx::id_type(gid, hpx::id_type::management_type::unmanaged);
}

// Reach `running` via a legal path: started -> running.
inline void reach_running(
    hpx::id_type const& locality, hpx::id_type const& target)
{
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::started);
    hpx::supervision::publish_event(
        hpx::launch::sync, locality, target, hpx::supervision::event::running);
}

// Same as reach_running(), but publishing both events under the given epoch
// rather than the default (0) epoch.
inline void reach_running_at_epoch(hpx::id_type const& locality,
    hpx::id_type const& target, std::uint64_t const epoch)
{
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::started, epoch);
    hpx::supervision::publish_event(hpx::launch::sync, locality, target,
        hpx::supervision::event::running, epoch);
}

///////////////////////////////////////////////////////////////////////////////
template <typename... Args>
void print(Args... args)
{
    bool first = true;
    (...,
        (first ? (first = false, std::cout << args) :
                 (std::cout << ", " << args)));
}

#define HPX_SUPERVISION_TEST_RUN(func, ...)                                    \
    do                                                                         \
    {                                                                          \
        std::cout << HPX_PP_STRINGIZE(func) << "(";                            \
        print(__VA_ARGS__);                                                    \
        std::cout << ")\n";                                                    \
        func(__VA_ARGS__);                                                     \
    } while (false) /**/

#endif
