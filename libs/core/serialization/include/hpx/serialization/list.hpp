//  Copyright (c) 2015 Thomas Heller
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/detail/serialize_collection.hpp>
#include <hpx/serialization/detail/to_size.hpp>
#include <hpx/serialization/serialize.hpp>

#include <cstdint>
#include <list>

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(input_archive& ar, std::list<T, Allocator>& ls, unsigned)
    {
        // normal load ...
        std::uint64_t size;
        ar >> size;    //-V128
        if (size == 0)
            return;

        detail::load_collection(ar, ls, detail::to_size(size));
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(output_archive& ar,
        std::list<T, Allocator> const& ls,    //-V826
        unsigned)
    {
        // normal save ...
        std::uint64_t const size = ls.size();
        ar << size;
        if (size == 0)
            return;

        detail::save_collection(ar, ls);
    }
}    // namespace hpx::serialization
