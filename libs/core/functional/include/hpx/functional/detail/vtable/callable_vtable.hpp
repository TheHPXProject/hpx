//  Copyright (c) 2011 Thomas Heller
//  Copyright (c) 2013-2023 Hartmut Kaiser
//  Copyright (c) 2014-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/detail/empty_function.hpp>
#include <hpx/functional/detail/vtable/vtable.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/functional/traits/get_function_address.hpp>
#include <hpx/functional/traits/get_function_annotation.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <utility>

namespace hpx::util::detail {

    struct empty_function;

    ///////////////////////////////////////////////////////////////////////////
    struct callable_info_vtable
    {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        template <typename T>
        HPX_FORCEINLINE static std::size_t _get_function_address(void* f)
        {
            return traits::get_function_address<T>::call(vtable::get<T>(f));
        }
        std::size_t (*get_function_address)(void*) = nullptr;

        template <typename T>
        HPX_FORCEINLINE static char const* _get_function_annotation(void* f)
        {
            return traits::get_function_annotation<T>::call(vtable::get<T>(f));
        }
        char const* (*get_function_annotation)(void*) = nullptr;

        template <typename T>
        HPX_FORCEINLINE static hpx::tracing::annotation_handle
        _get_function_annotation_tracing(void* f)
        {
            return traits::get_function_annotation_tracing<T>::call(
                vtable::get<T>(f));
        }
        hpx::tracing::annotation_handle (*get_function_annotation_tracing)(
            void*) = nullptr;
#endif

        template <typename T>
        explicit constexpr callable_info_vtable(construct_vtable<T>) noexcept
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
          : get_function_address(
                &callable_info_vtable::_get_function_address<T>)
          , get_function_annotation(
                &callable_info_vtable::_get_function_annotation<T>)
          , get_function_annotation_tracing(
                &callable_info_vtable::_get_function_annotation_tracing<T>)
#endif
        {
        }

        explicit constexpr callable_info_vtable(
            construct_vtable<empty_function>) noexcept
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Sig>
    struct callable_vtable;

    template <typename R, typename... Ts>
    struct callable_vtable<R(Ts...)>
    {
        template <typename T>
        HPX_FORCEINLINE static R _invoke(void* f, Ts&&... vs)
        {
            return HPX_INVOKE_R(R, vtable::get<T>(f), HPX_FORWARD(Ts, vs)...);
        }
        R (*invoke)(void*, Ts&&...);

        template <typename T>
        explicit constexpr callable_vtable(construct_vtable<T>) noexcept
          : invoke(&callable_vtable::_invoke<T>)
        {
        }

        static R _empty_invoke(void*, Ts&&...)
        {
            return throw_bad_function_call<R>();
        }

        explicit constexpr callable_vtable(
            construct_vtable<empty_function>) noexcept
          : invoke(&callable_vtable::_empty_invoke)
        {
        }
    };
}    // namespace hpx::util::detail
