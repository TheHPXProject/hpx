//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2015 Andreas Schaefer
//  Copyright (c) 2022-2025 Hartmut Kaiser
//  Copyright (c) 2026 Ujjwal Shekhar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/serialization/access.hpp>
#include <hpx/serialization/detail/polymorphic_nonintrusive_factory.hpp>

#include <memory>
#include <type_traits>

namespace hpx::serialization::detail {
    template <typename Derived>
    void register_class<Derived>::save(output_archive& ar, void const* base)
    {
        hpx::serialization::access::serialize(
            ar, *static_cast<Derived*>(const_cast<void*>(base)), 0);
    }

    template <typename Derived>
    void register_class<Derived>::load(input_archive& ar, void* base)
    {
        hpx::serialization::access::serialize(
            ar, *static_cast<Derived*>(base), 0);
    }

    template <typename T>
    T* constructor_selector_ptr<T>::create(input_archive& ar)
    {
        std::unique_ptr<T> t;

        if constexpr (std::is_default_constructible_v<T>)
        {
            t.reset(new T);
        }
        else
        {
            using storage_type = hpx::aligned_storage_t<sizeof(T), alignof(T)>;
            t.reset(reinterpret_cast<T*>(new storage_type));
            load_construct_data(ar, t.get(), 0);
        }

        if constexpr (hpx::traits::is_nonintrusive_polymorphic_v<T>)
        {
            hpx::serialization::access::serialize(ar, *t, 0);
        }
        else
        {
            ar >> *t;
        }

        return t.release();
    }
}    // namespace hpx::serialization::detail
