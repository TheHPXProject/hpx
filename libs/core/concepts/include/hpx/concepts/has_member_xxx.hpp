//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2020-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <type_traits>

/// This macro creates a boolean unary meta-function which result is true if and
/// only if its parameter type has member function with member name (no matter
/// static it is or not). The generated trait ends up in a namespace where the
/// macro itself has been placed.
//
#define HPX_HAS_MEMBER_XXX_TRAIT_DEF_2(Prefix, Member)                         \
    namespace HPX_PP_CAT(HPX_PP_CAT(has_, Member), _detail) {                  \
        Prefix struct helper                                                   \
        {                                                                      \
            void Member(...);                                                  \
        };                                                                     \
                                                                               \
        Prefix template <typename T>                                           \
        struct helper_composed                                                 \
          : T                                                                  \
          , helper                                                             \
        {                                                                      \
        };                                                                     \
                                                                               \
        Prefix template <void (helper::*)(...)>                                \
        struct Member_function_holder                                          \
        {                                                                      \
        };                                                                     \
                                                                               \
        Prefix template <typename T,                                           \
            typename Ambiguous = Member_function_holder<&helper::Member>>      \
        struct impl : std::true_type                                           \
        {                                                                      \
        };                                                                     \
                                                                               \
        template <typename T>                                                  \
        struct impl<T,                                                         \
            Member_function_holder<&helper_composed<std::decay_t<T>>::Member>> \
          : std::false_type                                                    \
        {                                                                      \
        };                                                                     \
    }                                                                          \
                                                                               \
    Prefix template <typename T, typename Enable = void>                       \
    struct HPX_PP_CAT(has_, Member)                                            \
      : std::false_type                                                        \
    {                                                                          \
    };                                                                         \
                                                                               \
    template <typename T>                                                      \
    struct HPX_PP_CAT(has_, Member)<T, std::enable_if_t<std::is_class_v<T>>>   \
      : HPX_PP_CAT(HPX_PP_CAT(has_, Member), _detail)::impl<T>                 \
    {                                                                          \
    };                                                                         \
                                                                               \
    Prefix template <typename T>                                               \
    using HPX_PP_CAT(HPX_PP_CAT(has_, Member), _t) =                           \
        typename HPX_PP_CAT(has_, Member)<T>::type;                            \
                                                                               \
    Prefix template <typename T>                                               \
    inline constexpr bool HPX_PP_CAT(HPX_PP_CAT(has_, Member), _v) =           \
        HPX_PP_CAT(has_, Member)<T>::value;                                    \
    /**/

#define HPX_HAS_MEMBER_XXX_TRAIT_DEF(...)                                      \
    HPX_HAS_MEMBER_XXX_TRAIT_DEF_(__VA_ARGS__) /**/

#define HPX_HAS_MEMBER_XXX_TRAIT_DEF_(...)                                     \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_HAS_MEMBER_XXX_TRAIT_DEF_,                    \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_HAS_MEMBER_XXX_TRAIT_DEF_1(Member)                                 \
    HPX_HAS_MEMBER_XXX_TRAIT_DEF_2(HPX_PP_EMPTY(), Member) /**/
