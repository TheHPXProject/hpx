//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/serialization/array.hpp>
#include <hpx/serialization/detail/serialize_collection.hpp>
#include <hpx/serialization/detail/to_size.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/serialize.hpp>
#include <hpx/serialization/traits/is_bitwise_serializable.hpp>
#include <hpx/serialization/traits/is_not_bitwise_serializable.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename Allocator>
    void serialize(input_archive& ar, std::vector<bool, Allocator>& v, unsigned)
    {
        std::uint64_t size = 0;
        ar >> size;    //-V128

        v.clear();
        if (size == 0)
        {
            return;
        }

        std::size_t const count = detail::to_size(size);

        // normal load ... no chance of doing bitwise here ...
        v.reserve(count);
        for (std::size_t i = 0; i != count; ++i)
        {
            bool b = false;
            ar >> b;
            v.push_back(b);
        }
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(input_archive& ar, std::vector<T, Allocator>& v, unsigned)
    {
        v.clear();

        std::uint64_t size;
        ar >> size;    //-V128
        if (size == 0)
        {
            return;
        }

        std::size_t const count = detail::to_size(size);

        using element_type =
            std::remove_const_t<typename std::vector<T, Allocator>::value_type>;

        constexpr bool use_optimized =
            std::is_default_constructible_v<element_type> &&
            (hpx::traits::is_bitwise_serializable_v<element_type> ||
                !hpx::traits::is_not_bitwise_serializable_v<element_type>);

        if constexpr (use_optimized)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
            if (ar.disable_array_optimization() || ar.endianess_differs())
            {
                detail::load_collection(ar, v, count);
                return;
            }
#else
            HPX_ASSERT(
                !(ar.disable_array_optimization() || ar.endianess_differs()));
#endif
            // bitwise load ...
            if (v.size() < count)
            {
                v.resize(count);
            }

            ar >> hpx::serialization::make_array(v.data(), v.size());
        }
        else
        {
            // normal load ...
            detail::load_collection(ar, v, count);
        }
    }

    HPX_CXX_CORE_EXPORT template <typename Allocator>
    void serialize(
        output_archive& ar, std::vector<bool, Allocator> const& v, unsigned)
    {
        std::uint64_t const size = v.size();
        ar << size;
        if (size == 0)
        {
            return;
        }

        // normal save ... no chance of doing bitwise here ...
        for (std::size_t i = 0; i < v.size(); ++i)
        {
            bool b = v[i];
            ar << b;
        }
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(
        output_archive& ar, std::vector<T, Allocator> const& v, unsigned)
    {
        std::uint64_t size = v.size();
        ar << size;
        if (size == 0)
        {
            return;
        }

        using element_type =
            std::remove_const_t<typename std::vector<T, Allocator>::value_type>;

        constexpr bool use_optimized =
            std::is_default_constructible_v<element_type> &&
            (hpx::traits::is_bitwise_serializable_v<element_type> ||
                !hpx::traits::is_not_bitwise_serializable_v<element_type>);

        if constexpr (use_optimized)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
            if (ar.disable_array_optimization() || ar.endianess_differs())
            {
                detail::save_collection(ar, v);
                return;
            }
#else
            HPX_ASSERT(
                !(ar.disable_array_optimization() || ar.endianess_differs()));
#endif
            // bitwise (zero-copy) save ...
            ar << hpx::serialization::make_array(v.data(), v.size());
        }
        else
        {
            // normal save ...
            detail::save_collection(ar, v);
        }
    }
}    // namespace hpx::serialization
