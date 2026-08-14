//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

//  Copyright 2001, 2003, 2004, 2012 Daryle Walker.

//  See <http://www.boost.org/libs/utility/> for the library's home page.

#pragma once

#include <hpx/config.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::util {
    namespace detail {

        //  Unmarked-type comparison class template  -----------------------------//

        // Type-trait to check if two type expressions have the same raw type.

        // Contributed by Daryle Walker, based on a work-around by Luc Danton
        HPX_CXX_EXPORT template <typename T, typename U>
        struct is_related : std::is_same<std::decay_t<T>, std::decay_t<U>>
        {
        };

        //  Enable-if-on-unidentical-unmarked-type class template  ---------------//

        // Enable-if on the first two type expressions NOT having the same raw
        // type.

        // Contributed by Daryle Walker, based on a work-around by Luc Danton

        HPX_CXX_EXPORT template <typename... T>
        struct is_unrelated : std::true_type
        {
        };

        template <typename T, typename U, typename... U2>
        struct is_unrelated<T, U, U2...> : std::negation<is_related<T, U>>
        {
        };
    }    // namespace detail

    //  Base-from-member class template  -----------------------------------------//

    // Helper to initialize a base object so a derived class can use this
    // object in the initialization of another base class.  Used by
    // Dietmar Kuehl from ideas by Ron Klatcho to solve the problem of a base
    // class needing to be initialized by a member.

    // Contributed by Daryle Walker

    HPX_CXX_EXPORT template <typename MemberType, int UniqueId = 0>
    class base_from_member
    {
    protected:
        MemberType member;

        // no std::is_nothrow_constructible nor std::forward needed
        // clang-format off
        template <typename... T>
            requires(detail::is_unrelated<base_from_member, T...>::value)
        explicit constexpr base_from_member(T&&... x) noexcept(
            noexcept(::new (nullptr) MemberType(static_cast<T&&>(x)...)))
          : member(static_cast<T&&>(x)...)
        {
        }
        // clang-format on
    };

    template <typename MemberType, int UniqueId>
    class base_from_member<MemberType&, UniqueId>
    {
    protected:
        MemberType& member;

        explicit constexpr base_from_member(MemberType& x) noexcept
          : member(x)
        {
        }
    };
}    // namespace hpx::util
