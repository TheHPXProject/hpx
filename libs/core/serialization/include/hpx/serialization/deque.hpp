//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/detail/serialize_collection.hpp>
#include <hpx/serialization/detail/to_size.hpp>
#include <hpx/serialization/serialization_fwd.hpp>

#include <cstdint>
#include <deque>

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(
        input_archive& ar, std::deque<T, Allocator>& d, unsigned)    //-V826
    {
        // normal load ...
        std::uint64_t size;
        ar >> size;    //-V128
        if (size == 0)
            return;

        detail::load_collection(ar, d, detail::to_size(size));
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(output_archive& ar,
        std::deque<T, Allocator> const& d,    //-V826
        unsigned)
    {
        // normal save ...
        std::uint64_t const size = d.size();
        ar << size;
        if (d.empty())
            return;

        detail::save_collection(ar, d);
    }
}    // namespace hpx::serialization
