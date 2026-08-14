//  Copyright (c) 2021-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// this trait has been inspired by:
// https://stackoverflow.com/questions/21379484/looking-for-an-is-allocator-type-trait-for-use-in-enable-if

#pragma once

#include <hpx/config.hpp>

#include <type_traits>
#include <utility>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct has_allocate
        {
        private:
            template <typename U>
            static std::false_type test(...);

            template <typename U>
            static std::true_type test(decltype(std::declval<U>().allocate(0)));

        public:
            static constexpr bool value = decltype(test<T>(nullptr))::value;
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        struct has_value_type
        {
        private:
            template <typename U>
            static std::false_type test(...);

            template <typename U>
            static std::true_type test(typename U::value_type*);

        public:
            static constexpr bool value = decltype(test<T>(nullptr))::value;
        };

        HPX_CXX_CORE_EXPORT template <typename T,
            bool HasAllocate = has_allocate<T>::value>
        struct has_deallocate
        {
        private:
            using pointer = decltype(std::declval<T>().allocate(0));

            template <typename Alloc, typename Pointer>
            static auto test(Alloc&& a, Pointer&& p)
                -> decltype(a.deallocate(p, 0), std::true_type());

            template <typename Alloc, typename Pointer>
            static auto test(Alloc const& a, Pointer&& p) -> std::false_type;

        public:
            static constexpr bool value = decltype(test<T>(
                std::declval<T>(), std::declval<pointer>()))::value;
        };

        template <typename T>
        struct has_deallocate<T, false>
        {
            static constexpr bool value = false;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_allocator
      : std::integral_constant<bool,
            detail::has_value_type<T>::value &&
                detail::has_allocate<T>::value &&
                detail::has_deallocate<T>::value>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_allocator_v = is_allocator<T>::value;
}    // namespace hpx::traits
