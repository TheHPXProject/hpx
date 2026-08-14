//  Copyright (c) 2016-2025 Hartmut Kaiser
//  Copyright (c) 2016 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>

// Select a copy tag type to enable optimization of copy/move operations if the
// iterators are pointers and if the value_type is layout compatible.

namespace hpx::traits {

    HPX_CXX_CORE_EXPORT struct general_pointer_tag
    {
    };

    HPX_CXX_CORE_EXPORT struct trivially_copyable_pointer_tag
      : general_pointer_tag
    {
    };

    HPX_CXX_CORE_EXPORT struct relocatable_pointer_tag : general_pointer_tag
    {
    };

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T,
            bool IsEnum = std::is_enum_v<T>>
        struct unwrap_enum
        {
            using type = std::underlying_type_t<T>;
        };

        template <typename T>
        struct unwrap_enum<T, false>
        {
            using type = T;
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        using unwrap_enum_t = typename unwrap_enum<T>::type;

        HPX_CXX_CORE_EXPORT template <typename Source, typename Dest>
        struct pointer_category_helper
        {
            using source = unwrap_enum_t<Source>;
            using dest = unwrap_enum_t<Dest>;

            // clang-format off
            static constexpr bool compatible_types =
                sizeof(source) == sizeof(dest) &&
                std::is_integral_v<source> && std::is_integral_v<dest> &&
               !std::is_volatile_v<source> && !std::is_volatile_v<dest> &&
                std::is_same_v<bool, source> == std::is_same_v<bool, dest>;
            // clang-format on

            using type = std::conditional_t<compatible_types,
                trivially_copyable_pointer_tag, general_pointer_tag>;
        };

        // every type is layout-compatible with itself
        template <typename T>
        struct pointer_category_helper<T, T>
        {
            using type = std::conditional_t<std::is_trivially_copyable_v<T>,
                trivially_copyable_pointer_tag, general_pointer_tag>;
        };

        // pointers are layout compatible
        template <typename T>
        struct pointer_category_helper<T*, T const*>
        {
            using type = trivially_copyable_pointer_tag;
        };

        template <typename T>
        struct pointer_category_helper<T*, T volatile*>
        {
            using type = trivially_copyable_pointer_tag;
        };

        template <typename T>
        struct pointer_category_helper<T*, T const volatile*>
        {
            using type = trivially_copyable_pointer_tag;
        };

        HPX_CXX_CORE_EXPORT template <typename Source, typename Dest>
        using pointer_category_helper_t =
            typename pointer_category_helper<Source, Dest>::type;

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
        inline constexpr bool iterators_are_contiguous_v =
            is_contiguous_iterator_v<Iter1> && is_contiguous_iterator_v<Iter2>;

        HPX_CXX_CORE_EXPORT template <typename Source, typename Dest,
            bool Contiguous = iterators_are_contiguous_v<Source, Dest>>
        struct pointer_move_category
        {
            using type = std::conditional_t<
                std::is_trivially_assignable_v<iter_reference_t<Dest>,
                    std::remove_reference_t<iter_reference_t<Source>>>,
                pointer_category_helper_t<iter_value_t<Source>,
                    iter_value_t<Dest>>,
                general_pointer_tag>;
        };

        template <typename Source, typename Dest>
        struct pointer_move_category<Source, Dest, false>
        {
            using type = general_pointer_tag;
        };

        HPX_CXX_CORE_EXPORT template <typename Source, typename Dest,
            bool Contiguous = iterators_are_contiguous_v<Source, Dest>>
        struct pointer_copy_category
        {
            using type = std::conditional_t<
                std::is_trivially_assignable_v<iter_reference_t<Dest>,
                    iter_reference_t<Source>>,
                pointer_category_helper_t<iter_value_t<Source>,
                    iter_value_t<Dest>>,
                general_pointer_tag>;
        };

        template <typename Source, typename Dest>
        struct pointer_copy_category<Source, Dest, false>
        {
            using type = general_pointer_tag;
        };

        HPX_CXX_CORE_EXPORT template <typename Source, typename Dest,
            bool Contiguous = iterators_are_contiguous_v<Source, Dest>>
        struct pointer_relocate_category
        {
            using type = std::conditional_t<
                std::is_same_v<iter_value_t<Source>, iter_value_t<Dest>> &&
                    hpx::experimental::is_relocatable_v<iter_value_t<Source>>,
                relocatable_pointer_tag, general_pointer_tag>;
        };

        template <typename Source, typename Dest>
        struct pointer_relocate_category<Source, Dest, false>
        {
            using type = general_pointer_tag;
        };
    }    // namespace detail

    // isolate iterators that refer to contiguous trivially copyable sequences or
    // which are pointers and their value_types are assignable
    HPX_CXX_CORE_EXPORT template <typename Source, typename Dest,
        typename Enable = void>
    struct pointer_copy_category : detail::pointer_copy_category<Source, Dest>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Source, typename Dest>
    using pointer_copy_category_t =
        typename pointer_copy_category<Source, Dest>::type;

    HPX_CXX_CORE_EXPORT template <typename Source, typename Dest,
        typename Enable = void>
    struct pointer_move_category : detail::pointer_move_category<Source, Dest>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Source, typename Dest>
    using pointer_move_category_t =
        typename pointer_move_category<Source, Dest>::type;

    HPX_CXX_CORE_EXPORT template <typename Source, typename Dest,
        typename Enable = void>
    struct pointer_relocate_category
      : detail::pointer_relocate_category<Source, Dest>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Source, typename Dest>
    using pointer_relocate_category_t =
        typename pointer_relocate_category<Source, Dest>::type;

    // Allow for matching of iterator<T const> to iterator<T> while calculating
    // pointer category.
    HPX_CXX_CORE_EXPORT template <typename Iterator, typename Enable = void>
    struct remove_const_iterator_value_type
    {
        using type = Iterator;
    };

    HPX_CXX_CORE_EXPORT template <typename Iterator>
    using remove_const_iterator_value_type_t =
        typename remove_const_iterator_value_type<Iterator>::type;
}    // namespace hpx::traits
