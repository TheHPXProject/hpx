//  (C) Copyright 2005 Matthias Troyer and Dave Abrahams
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2015 Andreas Schaefer
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/serialization/detail/allow_zero_copy_receive.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/serialize.hpp>
#include <hpx/serialization/traits/is_bitwise_serializable.hpp>
#include <hpx/serialization/traits/is_not_bitwise_serializable.hpp>

#include <array>
#include <cstddef>
#include <type_traits>

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename T>
    class array
    {
    public:
        using value_type = T;

        constexpr array(value_type* t, std::size_t s) noexcept
          : m_t(t)
          , m_element_count(s)
        {
        }

        [[nodiscard]] constexpr value_type* address() const noexcept
        {
            return m_t;
        }

        [[nodiscard]] constexpr std::size_t count() const noexcept
        {
            return m_element_count;
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned int)
        {
#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
            if (ar.disable_array_optimization() || ar.endianess_differs())
            {
                // normal serialization
                for (std::size_t i = 0; i != m_element_count; ++i)
                {
                    // clang-format off
                    ar & m_t[i];
                    // clang-format on
                }
                return;
            }
#else
            HPX_ASSERT(
                !(ar.disable_array_optimization() || ar.endianess_differs()));
#endif
            using element_type = std::remove_const_t<T>;

            constexpr bool use_optimized =
                std::is_default_constructible_v<element_type> &&
                (hpx::traits::is_bitwise_serializable_v<element_type> ||
                    !hpx::traits::is_not_bitwise_serializable_v<element_type>);

            if constexpr (use_optimized)
            {
                // try using chunking
                if constexpr (std::is_same_v<Archive, input_archive>)
                {
                    bool allow_zero_copy_receive =
                        ar.template try_get_extra_data<
                            detail::allow_zero_copy_receive>() != nullptr;

                    ar.load_binary_chunk(
                        detail::array_of_fundamental_type_v<element_type>, m_t,
                        m_element_count * sizeof(T), allow_zero_copy_receive);
                }
                else
                {
                    ar.save_binary_chunk(
                        detail::array_of_fundamental_type_v<element_type>, m_t,
                        m_element_count * sizeof(T));
                }
            }
            else
            {
                // normal serialization
                for (std::size_t i = 0; i != m_element_count; ++i)
                {
                    // clang-format off
                    ar & m_t[i];
                    // clang-format on
                }
            }
        }

    private:
        value_type* m_t;
        std::size_t m_element_count;
    };

    // make_array function
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE constexpr array<T> make_array(
        T* begin, std::size_t size) noexcept
    {
        return array<T>(begin, size);
    }

    // implement serialization for std::array
    HPX_CXX_CORE_EXPORT template <typename Archive, typename T, std::size_t N>
    void serialize(
        Archive& ar, std::array<T, N>& a, unsigned int const /* version */)
    {
        // clang-format off
        ar & hpx::serialization::make_array(a.data(), a.size());
        // clang-format on
    }

    // allow our array to be serialized as prvalue compiler should support good
    // ADL implementation, but it is rather for all hpx serialization library
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE output_archive& operator<<(output_archive& ar, array<T> t)
    {
        ar.save(t);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE input_archive& operator>>(input_archive& ar, array<T> t)
    {
        ar.load(t);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE output_archive& operator&(    //-V524
        output_archive& ar, array<T> t)
    {
        ar.save(t);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE input_archive& operator&(    //-V524
        input_archive& ar, array<T> t)
    {
        ar.load(t);
        return ar;
    }

    // serialize plain arrays:
    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N>
    HPX_FORCEINLINE output_archive& operator<<(output_archive& ar, T (&t)[N])
    {
        array<T> array = make_array(t, N);
        ar.save(array);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N>
    HPX_FORCEINLINE input_archive& operator>>(input_archive& ar, T (&t)[N])
    {
        array<T> array = make_array(t, N);
        ar.load(array);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N>
    HPX_FORCEINLINE output_archive& operator&(    //-V524
        output_archive& ar, T (&t)[N])
    {
        array<T> array = make_array(t, N);
        ar.save(array);
        return ar;
    }

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N>
    HPX_FORCEINLINE input_archive& operator&(    //-V524
        input_archive& ar, T (&t)[N])
    {
        array<T> array = make_array(t, N);
        ar.load(array);
        return ar;
    }
}    // namespace hpx::serialization
