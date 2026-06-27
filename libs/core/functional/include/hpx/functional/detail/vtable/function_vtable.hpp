//  Copyright (c) 2011 Thomas Heller
//  Copyright (c) 2013-2022 Hartmut Kaiser
//  Copyright (c) 2014-2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/detail/vtable/callable_vtable.hpp>
#include <hpx/functional/detail/vtable/copyable_vtable.hpp>
#include <hpx/functional/detail/vtable/vtable.hpp>

#include <type_traits>

namespace hpx::util::detail {

    struct function_base_vtable
      : vtable
      , copyable_vtable
      , callable_info_vtable
    {
        template <typename T>
        constexpr function_base_vtable(
            construct_vtable<T>, std::integral_constant<bool, true>) noexcept
          : vtable(construct_vtable<T>())
          , copyable_vtable(construct_vtable<T>())
          , callable_info_vtable(construct_vtable<T>())
        {
        }

        template <typename T>
        constexpr function_base_vtable(
            construct_vtable<T>, std::integral_constant<bool, false>) noexcept
          : vtable(construct_vtable<T>())
          , copyable_vtable(nullptr)
          , callable_info_vtable(construct_vtable<T>())
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Sig, bool Copyable = true>
    struct function_vtable;

    template <typename Sig>
    struct function_vtable<Sig, /*Copyable*/ false>
      : function_base_vtable
      , callable_vtable<Sig>
    {
        using copyable_tag = std::integral_constant<bool, false>;

        template <typename T>
        explicit constexpr function_vtable(construct_vtable<T>) noexcept
          : function_base_vtable(construct_vtable<T>(), copyable_tag{})
          , callable_vtable<Sig>(construct_vtable<T>())
        {
        }

        template <typename T, typename CopyableTag>
        constexpr function_vtable(construct_vtable<T>, CopyableTag) noexcept
          : function_base_vtable(construct_vtable<T>(), CopyableTag{})
          , callable_vtable<Sig>(construct_vtable<T>())
        {
        }
    };

    template <typename Sig>
    struct function_vtable<Sig, /*Copyable*/ true> : function_vtable<Sig, false>
    {
        using copyable_tag = std::integral_constant<bool, true>;

        template <typename T>
        explicit constexpr function_vtable(construct_vtable<T>) noexcept
          : function_vtable<Sig, false>(construct_vtable<T>(), copyable_tag{})
        {
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Sig>
    using unique_function_vtable = function_vtable<Sig, false>;
}    // namespace hpx::util::detail
