//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <memory>

namespace hpx::traits {

    // By default, we return the address of the object which is used to invoke
    // the trait.
    HPX_CXX_CORE_EXPORT template <typename F, typename Enable = void>
    struct get_function_address
    {
        static std::size_t call(F const& f) noexcept
        {
            return reinterpret_cast<std::size_t>(std::addressof(f));
        }
    };

    // For global (and static) functions we return the function address itself
    template <typename R, typename... Ts>
    struct get_function_address<R (*)(Ts...)>
    {
        static std::size_t call(R (*f)(Ts...)) noexcept
        {
            return reinterpret_cast<std::size_t>(f);
        }
    };

    // For member functions we return the first sizeof(void*) bytes of the
    // storage the member function pointer occupies.
    //
    // BADBAD: This invokes undefined behavior. What it does is to access the
    //         first sizeof(void*) bytes of the member function pointer. In our
    //         case this is not a problem as the value will be used to resolve a
    //         (debug) symbol only.
    //
    //         The value is never dereferenced or used in any other way. So the
    //         worst what can happen is that no matching symbol is found.
    //
    //         On some systems and for certain types of member function pointers
    //         this might even give the correct value representing the symbol
    //         corresponding to the function.
    //
    // clang-format off
    template <typename R, typename Obj, typename... Ts>
    struct get_function_address<R (Obj::*)(Ts...)>
    {
        static std::size_t call(R (Obj::*f)(Ts...)) noexcept
        {
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wstrict-aliasing"
#endif

#if defined(__GNUG__) && !defined(__INTEL_COMPILER) &&                         \
    (!defined(__NVCC__) || (__CUDACC_VER_MAJOR__ >= 8))
#  if defined(HPX_GCC_DIAGNOSTIC_PRAGMA_CONTEXTS)
#    pragma GCC diagnostic push
#  endif
#  pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

            return reinterpret_cast<std::size_t>(*reinterpret_cast<void**>(&f));

#if defined(__GNUG__) && !defined(__INTEL_COMPILER) &&                         \
    (!defined(__NVCC__) || (__CUDACC_VER_MAJOR__ >= 8))
#  if defined(HPX_GCC_DIAGNOSTIC_PRAGMA_CONTEXTS)
#    pragma GCC diagnostic pop
#  endif
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
        }
    };

    template <typename R, typename Obj, typename... Ts>
    struct get_function_address<R (Obj::*)(Ts...) const>
    {
        static std::size_t call(R (Obj::*f)(Ts...) const) noexcept
        {
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wstrict-aliasing"
#endif

#if defined(__GNUG__) && !defined(__INTEL_COMPILER) &&                         \
    (!defined(__NVCC__) || (__CUDACC_VER_MAJOR__ >= 8))
#  if defined(HPX_GCC_DIAGNOSTIC_PRAGMA_CONTEXTS)
#    pragma GCC diagnostic push
#  endif
#  pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

            return reinterpret_cast<std::size_t>(*reinterpret_cast<void**>(&f));

#if defined(__GNUG__) && !defined(__INTEL_COMPILER) &&                         \
    (!defined(__NVCC__) || (__CUDACC_VER_MAJOR__ >= 8))
#  if defined(HPX_GCC_DIAGNOSTIC_PRAGMA_CONTEXTS)
#    pragma GCC diagnostic pop
#  endif
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif
        }
    };
    // clang-format on
}    // namespace hpx::traits
