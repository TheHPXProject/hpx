//  Copyright (c) 2013-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::mem_fn
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/invoke.hpp>

#include <utility>

/// Top level namespace
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename MemberPointer>
        struct mem_fn
        {
            constexpr explicit mem_fn(MemberPointer pm) noexcept
              : _pm(pm)
            {
            }

            constexpr mem_fn(mem_fn const& other) noexcept = default;
            constexpr mem_fn& operator=(mem_fn const& other) noexcept = default;

            template <typename... Ts>
            constexpr util::invoke_result_t<MemberPointer, Ts...> operator()(
                Ts&&... vs) const
            {
                return HPX_INVOKE(_pm, HPX_FORWARD(Ts, vs)...);
            }

            MemberPointer _pm;
        };
    }    // namespace detail

    /// \brief Function template \c hpx::mem_fn generates wrapper objects for pointers
    ///        to members, which can store, copy, and invoke a pointer to member.
    ///        Both references and pointers (including smart pointers) to an object
    ///        can be used when invoking a \c hpx::mem_fn.
    ///
    /// \param pm pointer to member that will be wrapped
    ///
    /// \return a call wrapper of unspecified type with the following member:
    ///         \code
    ///         template <typename... Ts>
    ///         constexpr typename util::invoke_result<MemberPointer, Ts...>::type
    ///         operator()(Ts&&... vs) noexcept;
    ///         \endcode
    ///         Let \c fn be the call wrapper returned by a call to \c hpx::mem_fn
    ///         with a pointer to member \c pm. Then the expression
    ///         \c fn(t,a2,...,aN) is equivalent to \c HPX_INVOKE(pm,t,a2,...,aN).
    ///         Thus, the return type of operator() is
    ///         \c std::result_of<decltype(pm)(Ts&&...)>::type or equivalently
    ///         \c std::invoke_result_t<decltype(pm),Ts&&...>, and the value in
    ///         \c noexcept specifier is equal to
    ///         \c std::is_nothrow_invocable_v<decltype(pm),Ts&&...>) .
    ///         Each argument in \c vs is perfectly forwarded,
    ///         as if by \c std::forward<Ts>(vs)... .
    HPX_CXX_CORE_EXPORT template <typename M, typename C>
    constexpr detail::mem_fn<M C::*> mem_fn(M C::* pm) noexcept
    {
        return detail::mem_fn<M C::*>(pm);
    }

    /// \copydoc hpx::mem_fn
    HPX_CXX_CORE_EXPORT template <typename R, typename C, typename... Ps>
    constexpr detail::mem_fn<R (C::*)(Ps...)> mem_fn(R (C::*pm)(Ps...)) noexcept
    {
        return detail::mem_fn<R (C::*)(Ps...)>(pm);
    }

    /// \copydoc hpx::mem_fn
    HPX_CXX_CORE_EXPORT template <typename R, typename C, typename... Ps>
    constexpr detail::mem_fn<R (C::*)(Ps...) const> mem_fn(
        R (C::*pm)(Ps...) const) noexcept
    {
        return detail::mem_fn<R (C::*)(Ps...) const>(pm);
    }
}    // namespace hpx

/// \cond NOINTERN
namespace hpx::util {

    template <typename M, typename C>
    HPX_DEPRECATED_V(
        1, 9, "hpx::util::mem_fn is deprecated, use hpx::mem_fn instead")
    constexpr hpx::detail::mem_fn<M C::*> mem_fn(M C::* pm)
    {
        return hpx::detail::mem_fn<M C::*>(pm);
    }

    template <typename R, typename C, typename... Ps>
    HPX_DEPRECATED_V(
        1, 9, "hpx::util::mem_fn is deprecated, use hpx::mem_fn instead")
    constexpr hpx::detail::mem_fn<R (C::*)(Ps...)> mem_fn(R (C::*pm)(Ps...))
    {
        return hpx::detail::mem_fn<R (C::*)(Ps...)>(pm);
    }

    template <typename R, typename C, typename... Ps>
    HPX_DEPRECATED_V(
        1, 9, "hpx::util::mem_fn is deprecated, use hpx::mem_fn instead")
    constexpr hpx::detail::mem_fn<R (C::*)(Ps...) const> mem_fn(
        R (C::*pm)(Ps...) const)
    {
        return hpx::detail::mem_fn<R (C::*)(Ps...) const>(pm);
    }
}    // namespace hpx::util
