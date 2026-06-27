//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/performance_counters/counters_fwd.hpp>

#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

    ///////////////////////////////////////////////////////////////////////////
    // Discoverer functions to be registered with counter types

    /// Default discovery function for performance counters; to be registered
    /// with the counter types. It will pass the \a counter_info and the
    /// \a error_code to the supplied function.
    HPX_CXX_EXPORT HPX_EXPORT bool default_counter_discoverer(
        counter_info const&, discover_counter_func const&,
        discover_counters_mode, error_code&);

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>(locality#<locality_id>/total)/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT bool locality_counter_discoverer(
        counter_info const&, discover_counter_func const&,
        discover_counters_mode, error_code&);

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>(locality#<locality_id>/pool#<pool_name>/total)/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT bool locality_pool_counter_discoverer(
        counter_info const&, discover_counter_func const&,
        discover_counters_mode, error_code&);

    /// Default discoverer function for AGAS performance counters; to be
    /// registered with the counter types. It is suitable to be used for all
    /// counters following the naming scheme:
    ///
    ///   /<objectname>{locality#0/total}/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT bool locality0_counter_discoverer(
        counter_info const&, discover_counter_func const&,
        discover_counters_mode, error_code&);

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>(locality#<locality_id>/worker-thread#<threadnum>)/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT bool locality_thread_counter_discoverer(
        counter_info const&, discover_counter_func const&,
        discover_counters_mode, error_code&);

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>{locality#<locality_id>/pool#<poolname>/thread#<threadnum>}/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT bool locality_pool_thread_counter_discoverer(
        counter_info const& info, discover_counter_func const& f,
        discover_counters_mode mode, error_code& ec);

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>{locality#<locality_id>/pool#<poolname>/thread#<threadnum>}/<instancename>
    ///
    /// This is essentially the same as above just that locality#*/total is not
    /// supported.
    HPX_CXX_EXPORT HPX_EXPORT bool
    locality_pool_thread_no_total_counter_discoverer(counter_info const& info,
        discover_counter_func const& f, discover_counters_mode mode,
        error_code& ec);

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /<objectname>(locality#<locality_id>/numa-node#<threadnum>)/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT bool locality_numa_counter_discoverer(
        counter_info const&, discover_counter_func const&,
        discover_counters_mode, error_code&);

    ///////////////////////////////////////////////////////////////////////////
    /// Creation function for raw counters. The passed function is encapsulating
    /// the actual value to monitor. This function checks the validity of the
    /// supplied counter name, it has to follow the scheme:
    ///
    ///   /<objectname>(locality#<locality_id>/total)/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type locality_raw_counter_creator(
        counter_info const&, hpx::function<std::int64_t(bool)> const&,
        error_code&);

    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type
    locality_raw_values_counter_creator(counter_info const&,
        hpx::function<std::vector<std::int64_t>(bool)> const&, error_code&);

    ///////////////////////////////////////////////////////////////////////////
    /// Creation function for raw counters. The passed function is encapsulating
    /// the actual value to monitor. This function checks the validity of the
    /// supplied counter name, it has to follow the scheme:
    ///
    ///   /agas(<objectinstance>/total)/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type agas_raw_counter_creator(
        counter_info const&, error_code&, char const* const);

    /// Default discoverer function for performance counters; to be registered
    /// with the counter types. It is suitable to be used for all counters
    /// following the naming scheme:
    ///
    ///   /agas(<objectinstance>/total)/<instancename>
    ///
    HPX_CXX_EXPORT HPX_EXPORT bool agas_counter_discoverer(counter_info const&,
        discover_counter_func const&, discover_counters_mode, error_code&);

    ///////////////////////////////////////////////////////////////////////////
    // Creation function for action invocation counters.
    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type
    local_action_invocation_counter_creator(counter_info const&, error_code&);

    // Discoverer function for action invocation counters.
    HPX_CXX_EXPORT HPX_EXPORT bool local_action_invocation_counter_discoverer(
        counter_info const&, discover_counter_func const&,
        discover_counters_mode, error_code&);

#if defined(HPX_HAVE_NETWORKING)
    // clang-format off
    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type
    remote_action_invocation_counter_creator(counter_info const&, error_code&);
    // clang-format on

    // Discoverer function for action invocation counters.
    HPX_CXX_EXPORT HPX_EXPORT bool remote_action_invocation_counter_discoverer(
        counter_info const&, discover_counter_func const&,
        discover_counters_mode, error_code&);

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
    ///////////////////////////////////////////////////////////////////////////
    // Creation function for per-action parcel data counters
    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type per_action_data_counter_creator(
        counter_info const& info,
        hpx::function<std::int64_t(std::string const&, bool)> const& f,
        error_code& ec);

    // Discoverer function for per-action parcel data counters
    HPX_CXX_EXPORT HPX_EXPORT bool per_action_data_counter_discoverer(
        counter_info const& info, discover_counter_func const& f,
        discover_counters_mode mode, error_code& ec);
#endif
#endif
}    // namespace hpx::performance_counters
