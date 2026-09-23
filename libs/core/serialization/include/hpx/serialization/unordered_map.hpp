//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/detail/serialize_collection.hpp>
#include <hpx/serialization/detail/to_size.hpp>
#include <hpx/serialization/map.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/serialize.hpp>

#include <cstdint>
#include <unordered_map>
#include <utility>

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename Key, typename Value, typename Hash,
        typename KeyEqual, typename Alloc>
    void serialize(input_archive& ar,
        std::unordered_map<Key, Value, Hash, KeyEqual, Alloc>& t, unsigned)
    {
        std::uint64_t size;
        ar >> size;

        detail::load_collection(ar, t, detail::to_size(size));
    }

    HPX_CXX_CORE_EXPORT template <typename Key, typename Value, typename Hash,
        typename KeyEqual, typename Alloc>
    void serialize(output_archive& ar,
        std::unordered_map<Key, Value, Hash, KeyEqual, Alloc> const& t,
        unsigned)
    {
        std::uint64_t const size = t.size();
        ar << size;
        if (size == 0)
            return;

        detail::save_collection(ar, t);
    }

    HPX_CXX_EXPORT template <typename Key, typename Value, typename Hash,
        typename KeyEqual, typename Alloc>
    void serialize(input_archive& ar,
        std::unordered_multimap<Key, Value, Hash, KeyEqual, Alloc>& t, unsigned)
    {
        std::uint64_t size;
        ar >> size;

        detail::load_collection(ar, t, detail::to_size(size));
    }

    HPX_CXX_EXPORT template <typename Key, typename Value, typename Hash,
        typename KeyEqual, typename Alloc>
    void serialize(output_archive& ar,
        std::unordered_multimap<Key, Value, Hash, KeyEqual, Alloc> const& t,
        unsigned)
    {
        std::uint64_t const size = t.size();
        ar << size;
        if (size == 0)
            return;

        detail::save_collection(ar, t);
    }
}    // namespace hpx::serialization
