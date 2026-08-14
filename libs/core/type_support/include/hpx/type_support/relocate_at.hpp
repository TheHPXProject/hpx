//  Copyright (c) 2023 Isidoros Tsaousis-Seiras
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/type_support/construct_at.hpp>
#include <hpx/type_support/is_relocatable.hpp>
#include <hpx/type_support/is_trivially_relocatable.hpp>

#include <concepts>
#include <cstring>
#include <memory>
#include <type_traits>

namespace hpx::detail {

    // since c++20 std::destroy_at can be used on array types, destructing each
    // element
    HPX_CXX_CORE_EXPORT template <std::destructible T>
    struct destroy_guard
    {
        T* t;
        explicit destroy_guard(T* t)
          : t(t)
        {
        }
        ~destroy_guard()
        {
            std::destroy_at(t);
        }
    };
}    // namespace hpx::detail

#if defined(HPX_HAVE_P1144_STD_RELOCATE_AT)
HPX_CXX_CORE_EXPORT using std::relocate;
HPX_CXX_CORE_EXPORT using std::relocate_at;
#else

namespace hpx::experimental {

    namespace detail {

        // The condition to use memmove is:
        //         hpx::is_trivially_relocatable_v<T> &&
        //         !std::is_volatile_v<T>
        //
        // The reason for the volatile check is that memmove is not allowed to
        // change the value of a volatile object.
        HPX_CXX_CORE_EXPORT template <typename T>
        concept relocate_using_memmove =
            is_trivially_relocatable_v<T> && !std::is_volatile_v<T>;

        HPX_CXX_CORE_EXPORT template <typename T>
            requires(relocate_using_memmove<T>)
        T* relocate_at_helper(T* src, T* dst) noexcept
        {
            auto dst_void = const_cast<void*>(static_cast<void const*>(dst));
            auto src_void = const_cast<void*>(static_cast<void const*>(src));

            std::memmove(dst_void, src_void, sizeof(T));

            return std::launder(dst);
        };

        // this is move and destroy
        HPX_CXX_CORE_EXPORT template <typename T>
            requires(!relocate_using_memmove<T>)
        T* relocate_at_helper(T* src, T* dst) noexcept(
            std::is_nothrow_move_constructible_v<T>)
        {
            hpx::detail::destroy_guard g(src);
            return hpx::construct_at(dst, HPX_MOVE(*src));
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        T relocate_helper(T* src) noexcept(
            std::is_nothrow_move_constructible_v<T>)
        {
            hpx::detail::destroy_guard g(src);
            return HPX_MOVE(*src);
        }

        // P1144 also proposes a version of relocate that does not call the
        // move constructor and instead memmoves the bytes of src to dest.
        //
        // Giving an interface like:
        //
        //     T dest = relocate(std::addressof(src));
        //
        // That results in a valid T object (dest) without calling any
        // constructor or destructor.
        //
        // This is not possible to do with the current C++ standard.
        //
        // One of the proposed ways to implement this uses a hypothetical
        // attribute "do_not_construct" and NRVO.
        //
        // Implementation:
        //
        // template <class T, std::enable_if_t<relocate_using_memmove<T>, int> = 0>
        // T relocate(T* source)
        // {
        //     __attribute__((do_not_construct)) T t; // hypothetical attribute
        //     std::memmove(std::addressof(t), source, sizeof(T));
        //     return t;  // NRVO
        // }
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    T* relocate_at(T* src, T* dst) noexcept(
        // noexcept if the memmove path is taken or if the move path is noexcept
        noexcept(detail::relocate_at_helper(src, dst)))
    {
        static_assert(
            is_relocatable_v<T>, "T(std::move(*src)) must be well-formed");

        return detail::relocate_at_helper(src, dst);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    T relocate(T* src) noexcept(noexcept(detail::relocate_helper(src)))
    {
        static_assert(
            is_relocatable_v<T>, "T(std::move(*src)) must be well-formed");

        return detail::relocate_helper(src);
    }

#endif    // !defined(HPX_HAVE_P1144_STD_RELOCATE_AT)

}    // namespace hpx::experimental
