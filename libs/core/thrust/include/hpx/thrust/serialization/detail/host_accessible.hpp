//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>

#include <thrust/memory.h>

#include <cstddef>
#include <type_traits>

namespace hpx::serialization::detail {

    // Whether the elements are serialized as one array (zero-copy chunks
    // included), as std::vector does; others go element by element, with
    // the construction hooks of types that aren't default constructible.
    template <typename T>
    inline constexpr bool host_accessible_as_array =
        std::is_default_constructible_v<T> &&
        (hpx::traits::is_bitwise_serializable_v<T> ||
            !hpx::traits::is_not_bitwise_serializable_v<T>);

    // host_vector and universal_vector own host-accessible memory, which
    // lives in the parcel as long as the vector does: serialize it like a
    // std::vector, zero-copy chunks included.
    HPX_CXX_CORE_EXPORT template <typename Vector>
    void save_host_accessible(output_archive& ar, Vector const& v)
    {
        std::size_t const size = v.size();
        ar << size;
        if (size == 0)
        {
            return;
        }

        using value_type = Vector::value_type;
        if constexpr (host_accessible_as_array<value_type>)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
            // element by element, as std::vector does
            if (ar.disable_array_optimization() || ar.endianess_differs())
            {
                save_collection(ar, v);
                return;
            }
#endif
            ar << hpx::serialization::make_array(
                ::thrust::raw_pointer_cast(v.data()), size);
        }
        else
        {
            save_collection(ar, v);
        }
    }

    HPX_CXX_CORE_EXPORT template <typename Vector>
    void load_host_accessible(input_archive& ar, Vector& v)
    {
        std::size_t size = 0;
        ar >> size;

        using value_type = Vector::value_type;
        if constexpr (host_accessible_as_array<value_type>)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
            // element by element into fresh elements, as std::vector does
            if (ar.disable_array_optimization() || ar.endianess_differs())
            {
                load_collection(ar, v, size);
                return;
            }
#endif
            v.resize(size);
            if (size != 0)
            {
                ar >> hpx::serialization::make_array(
                          ::thrust::raw_pointer_cast(v.data()), size);
            }
        }
        else
        {
            load_collection(ar, v, size);
        }
    }
}    // namespace hpx::serialization::detail
