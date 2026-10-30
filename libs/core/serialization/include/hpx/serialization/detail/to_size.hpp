//  Copyright (c) 2026 Abhishek Kumar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>

namespace hpx::serialization::detail {

    // Collection sizes travel as std::uint64_t so that the wire format does
    // not depend on the width of std::size_t at either end. A 64 bit sender
    // can therefore hand a 32 bit receiver a size this process could never
    // hold. Truncating it would turn an allocation that has to fail into one
    // that succeeds at the wrong size, so refuse the archive instead.
    [[nodiscard]] inline std::size_t to_size(std::uint64_t const size) noexcept(
        sizeof(std::size_t) >= sizeof(std::uint64_t))
    {
        if constexpr (sizeof(std::size_t) >= sizeof(std::uint64_t))
        {
            // every size that can be stored can also be represented here,
            // there is nothing to check
            return static_cast<std::size_t>(size);
        }
        else
        {
            std::size_t const result = static_cast<std::size_t>(size);
            if (result != size)
            {
                HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                    "hpx::serialization::detail::to_size",
                    "the archive holds a collection of {} elements, which does "
                    "not fit into std::size_t on this platform",
                    size);
            }
            return result;
        }
    }

    // Same conversion for values that are a limit rather than a length. One
    // that is larger than this host can represent simply means the limit is
    // never reached, so saturate instead of refusing the archive.
    [[nodiscard]] constexpr std::size_t clamp_to_size(
        std::uint64_t const size) noexcept
    {
        if constexpr (sizeof(std::size_t) >= sizeof(std::uint64_t))
        {
            return static_cast<std::size_t>(size);
        }
        else
        {
            constexpr std::uint64_t max_size = static_cast<std::uint64_t>(
                (std::numeric_limits<std::size_t>::max)());
            return static_cast<std::size_t>(size < max_size ? size : max_size);
        }
    }
}    // namespace hpx::serialization::detail
