//  Copyright (c) 2013-2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::invoke_fused, hpx::invoke_fused_r
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Tuple>
        struct fused_index_pack
          : util::make_index_pack<hpx::tuple_size<std::decay_t<Tuple>>::value>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename Tuple>
        using fused_index_pack_t = typename fused_index_pack<Tuple>::type;

        ///////////////////////////////////////////////////////////////////////
        template <typename F, typename Tuple, typename Is>
        struct invoke_fused_result_impl;

        template <typename F, typename Tuple, std::size_t... Is>
        struct invoke_fused_result_impl<F, Tuple&, util::index_pack<Is...>>
          : hpx::util::invoke_result<F,
                typename hpx::tuple_element<Is, Tuple>::type&...>
        {
        };

        template <typename F, typename Tuple, std::size_t... Is>
        struct invoke_fused_result_impl<F, Tuple&&, util::index_pack<Is...>>
          : hpx::util::invoke_result<F,
                typename hpx::tuple_element<Is, Tuple>::type&&...>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename F, typename Tuple>
        struct invoke_fused_result
          : invoke_fused_result_impl<F, Tuple&&, fused_index_pack_t<Tuple>>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename F, typename Tuple>
        using invoke_fused_result_t =
            typename invoke_fused_result<F, Tuple>::type;

        ///////////////////////////////////////////////////////////////////////
        template <std::size_t... Is, typename F, typename Tuple>
        constexpr HPX_HOST_DEVICE HPX_FORCEINLINE
            invoke_fused_result_t<F, Tuple>
            invoke_fused_impl(util::index_pack<Is...>, F&& f,
                Tuple&& t) noexcept(noexcept(HPX_INVOKE(HPX_FORWARD(F, f),
                hpx::get<Is>(HPX_FORWARD(Tuple, t))...)))
        {
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26800)    //  Use of a moved from object: '(*t)'
#endif

            return HPX_INVOKE(
                // NOLINTNEXTLINE(bugprone-use-after-move)
                HPX_FORWARD(F, f), hpx::get<Is>(HPX_FORWARD(Tuple, t))...);

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
        }
    }    // namespace detail

    /// Invokes the given callable object f with the content of
    /// the sequenced type t (tuples, pairs).
    ///
    /// \param f Must be a callable object. If f is a member function pointer,
    ///          the first argument in the sequenced type will be treated as
    ///          the callee (this object).
    ///
    /// \param t A type whose contents are accessible through a call
    ///          to hpx#get.
    ///
    /// \returns The result of the callable object when it's called with
    ///          the content of the given sequenced type.
    ///
    /// \throws std::exception like objects thrown by call to object f
    ///         with the arguments contained in the sequenceable type t.
    ///
    /// \note This function is similar to `std::apply` (C++17). The difference
    ///       between \c hpx::invoke and \c hpx::invoke_fused is that the later
    ///       unpacks the tuples while the former cannot. Turning a tuple into a
    ///       parameter pack is not a trivial operation which makes
    ///       \c hpx::invoke_fused rather useful.
    HPX_CXX_CORE_EXPORT template <typename F, typename Tuple>
    constexpr HPX_HOST_DEVICE HPX_FORCEINLINE
        detail::invoke_fused_result_t<F, Tuple>
        invoke_fused(F&& f, Tuple&& t) noexcept(noexcept(
            detail::invoke_fused_impl(detail::fused_index_pack_t<Tuple>{},
                HPX_FORWARD(F, f), HPX_FORWARD(Tuple, t))))
    {
        using index_pack = detail::fused_index_pack_t<Tuple>;
        return detail::invoke_fused_impl(
            index_pack{}, HPX_FORWARD(F, f), HPX_FORWARD(Tuple, t));
    }

    /// \copydoc invoke_fused
    ///
    /// \tparam R The result type of the function when it's called
    ///           with the content of the given sequenced type.
    /// \note The difference between \c hpx::invoke_fused and
    ///       \c hpx::invoke_fused_r is that the later allows to
    ///       specify the return type as well.
    HPX_CXX_CORE_EXPORT template <typename R, typename F, typename Tuple>
    constexpr HPX_HOST_DEVICE HPX_FORCEINLINE R
    invoke_fused_r(F&& f, Tuple&& t) noexcept(
        noexcept(detail::invoke_fused_impl(detail::fused_index_pack_t<Tuple>{},
            HPX_FORWARD(F, f), HPX_FORWARD(Tuple, t))))
    {
        using index_pack = detail::fused_index_pack_t<Tuple>;
        return util::void_guard<R>(),
               detail::invoke_fused_impl(
                   index_pack{}, HPX_FORWARD(F, f), HPX_FORWARD(Tuple, t));
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    namespace functional {

        HPX_CXX_CORE_EXPORT struct invoke_fused
        {
            template <typename F, typename Tuple>
            constexpr HPX_HOST_DEVICE HPX_FORCEINLINE
                hpx::detail::invoke_fused_result_t<F, Tuple>
                operator()(F&& f, Tuple&& t) const
                noexcept(noexcept(hpx::detail::invoke_fused_impl(
                    hpx::detail::fused_index_pack_t<Tuple>{}, HPX_FORWARD(F, f),
                    HPX_FORWARD(Tuple, t))))
            {
                using index_pack = hpx::detail::fused_index_pack_t<Tuple>;
                return hpx::detail::invoke_fused_impl(
                    index_pack{}, HPX_FORWARD(F, f), HPX_FORWARD(Tuple, t));
            }
        };

        HPX_CXX_CORE_EXPORT template <typename R>
        struct invoke_fused_r
        {
            template <typename F, typename Tuple>
            constexpr HPX_HOST_DEVICE HPX_FORCEINLINE R operator()(
                F&& f, Tuple&& t) const
                noexcept(noexcept(hpx::detail::invoke_fused_impl(
                    hpx::detail::fused_index_pack_t<Tuple>{}, HPX_FORWARD(F, f),
                    HPX_FORWARD(Tuple, t))))
            {
                using index_pack = hpx::detail::fused_index_pack_t<Tuple>;
                return util::void_guard<R>(),
                       hpx::detail::invoke_fused_impl(index_pack{},
                           HPX_FORWARD(F, f), HPX_FORWARD(Tuple, t));
            }
        };
    }    // namespace functional
    /// \endcond
}    // namespace hpx

/// \cond NOINTERN
namespace hpx::util {

    template <typename F, typename Tuple>
    HPX_DEPRECATED_V(1, 9,
        "hpx::util::invoke_fused is deprecated, use hpx::invoke_fused instead")
    constexpr HPX_HOST_DEVICE HPX_FORCEINLINE
        typename hpx::detail::invoke_fused_result<F, Tuple>::type
        invoke_fused(F&& f, Tuple&& t) noexcept(
            noexcept(hpx::detail::invoke_fused_impl(
                hpx::detail::fused_index_pack_t<Tuple>{}, HPX_FORWARD(F, f),
                HPX_FORWARD(Tuple, t))))
    {
        using index_pack = hpx::detail::fused_index_pack_t<Tuple>;
        return hpx::detail::invoke_fused_impl(
            index_pack{}, HPX_FORWARD(F, f), HPX_FORWARD(Tuple, t));
    }

    template <typename R, typename F, typename Tuple>
    HPX_DEPRECATED_V(1, 9,
        "hpx::util::invoke_fused_r is deprecated, use hpx::invoke_fused_r "
        "instead")
    constexpr HPX_HOST_DEVICE HPX_FORCEINLINE R
        invoke_fused_r(F&& f, Tuple&& t) noexcept(
            noexcept(hpx::detail::invoke_fused_impl(
                hpx::detail::fused_index_pack_t<Tuple>{}, HPX_FORWARD(F, f),
                HPX_FORWARD(Tuple, t))))
    {
        using index_pack = hpx::detail::fused_index_pack_t<Tuple>;
        return util::void_guard<R>(),
               hpx::detail::invoke_fused_impl(
                   index_pack{}, HPX_FORWARD(F, f), HPX_FORWARD(Tuple, t));
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    namespace functional {

        using invoke_fused HPX_DEPRECATED_V(1, 9,
            "hpx::util::invoke_fused is deprecated, use hpx::invoke_fused "
            "instead") = hpx::functional::invoke_fused;

        template <typename R>
        using invoke_fused_r HPX_DEPRECATED_V(1, 9,
            "hpx::util::invoke_fused_r is deprecated, use hpx::invoke_fused_r "
            "instead") = hpx::functional::invoke_fused_r<R>;
    }    // namespace functional
    /// \endcond
}    // namespace hpx::util
/// \endcond
