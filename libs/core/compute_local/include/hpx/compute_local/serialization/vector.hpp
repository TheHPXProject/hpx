//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/config/endian.hpp>
#include <hpx/assert.hpp>
#include <hpx/compute_local/vector.hpp>
#include <hpx/modules/serialization.hpp>

#include <type_traits>

namespace hpx::serialization {

#if !defined(__CUDA_ARCH__)
    // load compute::vector<T>
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
        void load_impl(input_archive& ar, compute::vector<T, Allocator>& vs,
            std::false_type)
        {
            // normal load ...
            using value_type = compute::vector<T, Allocator>::value_type;
            using size_type = compute::vector<T, Allocator>::size_type;

            size_type size;
            value_type v;

            ar >> size;    //-V128
            if (size == 0)
                return;

            vs.resize(size);
            for (size_type i = 0; i != size; ++i)
            {
                ar >> v;
                vs[i] = v;
            }
        }

        HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
        void load_impl(
            input_archive& ar, compute::vector<T, Allocator>& v, std::true_type)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
            if (ar.disable_array_optimization() || ar.endianess_differs())
            {
                load_impl(ar, v, std::false_type());
                return;
            }
#else
            HPX_ASSERT(
                !(ar.disable_array_optimization() || ar.endianess_differs()));
#endif
            // bitwise load ...
            using value_type = compute::vector<T, Allocator>::value_type;
            using size_type = compute::vector<T, Allocator>::size_type;

            size_type size;
            ar >> size;    //-V128
            if (size == 0)
            {
                return;
            }

            v.resize(size);
            load_binary(ar, detail::array_of_fundamental_type_v<T>,
                v.device_data(), v.size() * sizeof(value_type));
        }
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(
        input_archive& ar, compute::vector<T, Allocator>& v, unsigned)
    {
        using element_type = std::remove_const_t<
            typename compute::vector<T, Allocator>::value_type>;

        using use_optimized = std::integral_constant<bool,
            std::is_default_constructible_v<element_type> &&
                (hpx::traits::is_bitwise_serializable_v<element_type> ||
                    !hpx::traits::is_not_bitwise_serializable_v<element_type>)>;

        v.clear();
        detail::load_impl(ar, v, use_optimized());
    }

    // save compute::vector<T>
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
        void save_impl(output_archive& ar,
            compute::vector<T, Allocator> const& vs, std::false_type)
        {
            // normal save ...
            for (auto const& v : vs)
            {
                ar << v;
            }
        }

        HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
        void save_impl(output_archive& ar,
            compute::vector<T, Allocator> const& v, std::true_type)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
            if (ar.disable_array_optimization() || ar.endianess_differs())
            {
                save_impl(ar, v, std::false_type());
                return;
            }
#else
            HPX_ASSERT(
                !(ar.disable_array_optimization() || ar.endianess_differs()));
#endif
            // bitwise save ...
            using value_type = compute::vector<T, Allocator>::value_type;
            save_binary(ar, detail::array_of_fundamental_type_v<T>,
                v.device_data(), v.size() * sizeof(value_type));
        }
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(
        output_archive& ar, compute::vector<T, Allocator> const& v, unsigned)
    {
        using element_type = std::remove_const_t<
            typename compute::vector<T, Allocator>::value_type>;

        using use_optimized = std::integral_constant<bool,
            std::is_default_constructible_v<element_type> &&
                (hpx::traits::is_bitwise_serializable_v<element_type> ||
                    !hpx::traits::is_not_bitwise_serializable_v<element_type>)>;

        ar << v.size();    //-V128
        if (v.empty())
        {
            return;
        }

        detail::save_impl(ar, v, use_optimized());
    }
#else
    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(input_archive&, compute::vector<T, Allocator>&, unsigned)
    {
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Allocator>
    void serialize(
        output_archive&, compute::vector<T, Allocator> const&, unsigned)
    {
    }
#endif
}    // namespace hpx::serialization
