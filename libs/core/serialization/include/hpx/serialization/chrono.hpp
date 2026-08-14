//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/serialization_fwd.hpp>

#include <chrono>

namespace hpx::serialization {

    // std::chrono::duration: only the underlying (integral or floating-point)
    // rep needs to be transferred, Period is a compile-time ratio baked into
    // the type rather than runtime data.
    HPX_CXX_CORE_EXPORT template <typename Rep, typename Period>
    void serialize(
        input_archive& ar, std::chrono::duration<Rep, Period>& d, unsigned)
    {
        Rep count;
        ar >> count;
        d = std::chrono::duration<Rep, Period>(count);
    }

    HPX_CXX_CORE_EXPORT template <typename Rep, typename Period>
    void serialize(output_archive& ar,
        std::chrono::duration<Rep, Period> const& d, unsigned)
    {
        ar << d.count();
    }

    // std::chrono::time_point: round-trips correctly within a single
    // process/clock instance (e.g. serialized to a file and read back, or
    // immediately deserialized into the same running process). This does
    // *not* make cross-locality transfer of a time_point meaningful for
    // clocks whose epoch is unspecified and independent per process (in
    // particular std::chrono::steady_clock and std::chrono::high_resolution_clock):
    // a time_point serialized on one locality does not correspond to the same
    // instant when deserialized on another. std::chrono::system_clock::time_point
    // is generally safe to interpret across processes since it is
    // conventionally tied to the Unix epoch.
    HPX_CXX_CORE_EXPORT template <typename Clock, typename Duration>
    void serialize(input_archive& ar,
        std::chrono::time_point<Clock, Duration>& tp, unsigned)
    {
        Duration d;
        ar >> d;
        tp = std::chrono::time_point<Clock, Duration>(d);
    }

    HPX_CXX_CORE_EXPORT template <typename Clock, typename Duration>
    void serialize(output_archive& ar,
        std::chrono::time_point<Clock, Duration> const& tp, unsigned)
    {
        ar << tp.time_since_epoch();
    }
}    // namespace hpx::serialization
