//  Copyright (c) 2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <type_traits>

/// This macro creates a boolean unary meta-function such that for any type X,
/// has_name<X>::value == true if and only if X is a class type and has a nested
/// type member x::name. The generated trait ends up in a namespace where the
/// macro itself has been placed.
#define HPX_HAS_XXX_TRAIT_DEF_2(Prefix, Name)                                  \
    Prefix template <typename T, typename Enable = void>                       \
    struct HPX_PP_CAT(has_, Name)                                              \
      : std::false_type                                                        \
    {                                                                          \
    };                                                                         \
                                                                               \
    template <typename T>                                                      \
    struct HPX_PP_CAT(has_, Name)<T, std::void_t<typename T::Name>>            \
      : std::true_type                                                         \
    {                                                                          \
    };                                                                         \
                                                                               \
    Prefix template <typename T>                                               \
    using HPX_PP_CAT(HPX_PP_CAT(has_, Name), _t) =                             \
        typename HPX_PP_CAT(has_, Name)<T>::type;                              \
                                                                               \
    Prefix template <typename T>                                               \
    inline constexpr bool HPX_PP_CAT(HPX_PP_CAT(has_, Name), _v) =             \
        HPX_PP_CAT(has_, Name)<T>::value;                                      \
    /**/

#define HPX_HAS_XXX_TRAIT_DEF(...)                                             \
    HPX_HAS_XXX_TRAIT_DEF_(__VA_ARGS__)                                        \
    /**/

#define HPX_HAS_XXX_TRAIT_DEF_(...)                                            \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_HAS_XXX_TRAIT_DEF_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))       \
    /**/

#define HPX_HAS_XXX_TRAIT_DEF_1(Name)                                          \
    HPX_HAS_XXX_TRAIT_DEF_2(HPX_PP_EMPTY(), Name)                              \
    /**/
