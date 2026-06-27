//  Copyright (c) 2015-2025 Hartmut Kaiser
//  Copyright (c) 2011 Thomas Heller
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/detail/post.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT template <typename Action, typename Is, typename... Ts>
        class bound_action;

        template <typename Action, std::size_t... Is, typename... Ts>
        class bound_action<Action, util::index_pack<Is...>, Ts...>
        {
        public:
            using result_type = traits::promise_local_result_t<typename hpx::
                    traits::extract_action<Action>::remote_result_type>;

        public:
            // default constructor is needed for serialization
            bound_action() = default;

            template <typename Derived, typename... Ts_>
            explicit bound_action(Derived /*action*/, Ts_&&... vs)
              : _args(std::piecewise_construct, HPX_FORWARD(Ts_, vs)...)
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            bound_action(bound_action const&) = default;
            bound_action(bound_action&&) = default;
#else
            HPX_HOST_DEVICE bound_action(bound_action const& other)
              : _args(other._args)
            {
            }

            HPX_HOST_DEVICE bound_action(bound_action&& other)
              : _args(HPX_MOVE(other._args))
            {
            }
#endif

            bound_action& operator=(bound_action const&) = delete;

#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26800)    //  Use of a moved from object: '(*<vs_0>)'
#endif

            template <typename... Us>
            HPX_FORCEINLINE bool post(Us&&... vs) const
            {
                return hpx::post<Action>(
                    detail::bind_eval<Ts const&, sizeof...(Us)>::call(
                        // NOLINTNEXTLINE(bugprone-use-after-move)
                        _args.template get<Is>(), HPX_FORWARD(Us, vs)...)...);
            }

            template <typename... Us>
            HPX_FORCEINLINE bool post_c(
                hpx::id_type const& cont, Us&&... vs) const
            {
                return hpx::post_c<Action>(cont,
                    detail::bind_eval<Ts const&, sizeof...(Us)>::call(
                        // NOLINTNEXTLINE(bugprone-use-after-move)
                        _args.template get<Is>(), HPX_FORWARD(Us, vs)...)...);
            }

            template <typename Continuation, typename... Us>
            HPX_FORCEINLINE
                std::enable_if_t<traits::is_continuation_v<Continuation>, bool>
                post_c(Continuation&& cont, Us&&... vs) const
            {
                return hpx::post<Action>(HPX_FORWARD(Continuation, cont),
                    detail::bind_eval<Ts const&, sizeof...(Us)>::call(
                        // NOLINTNEXTLINE(bugprone-use-after-move)
                        _args.template get<Is>(), HPX_FORWARD(Us, vs)...)...);
            }

            template <typename... Us>
            HPX_FORCEINLINE hpx::future<result_type> async(Us&&... vs) const
            {
                return hpx::async<Action>(
                    detail::bind_eval<Ts const&, sizeof...(Us)>::call(
                        // NOLINTNEXTLINE(bugprone-use-after-move)
                        _args.template get<Is>(), HPX_FORWARD(Us, vs)...)...);
            }

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

            template <typename... Us>
            HPX_FORCEINLINE result_type operator()(Us&&... vs) const
            {
                return async(HPX_FORWARD(Us, vs)...).get();
            }

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const /*version*/)
            {
                // clang-format off
                ar & _args;
                // clang-format on
            }

        private:
            util::member_pack_for<Ts...> _args;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
        requires(traits::is_action_v<std::decay_t<Action>>)
    detail::bound_action<std::decay_t<Action>,
        util::make_index_pack_t<sizeof...(Ts)>,
        hpx::util::decay_unwrap_t<Ts>...>
    bind(Ts&&... vs)
    {
        using result_type = detail::bound_action<std::decay_t<Action>,
            util::make_index_pack_t<sizeof...(Ts)>,
            hpx::util::decay_unwrap_t<Ts>...>;

        return result_type(Action(), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    detail::bound_action<Derived, util::make_index_pack_t<sizeof...(Ts)>,
        std::decay_t<Ts>...>
    bind(hpx::actions::basic_action<Component, Signature, Derived> action,
        Ts&&... vs)
    {
        using result_type = detail::bound_action<Derived,
            util::make_index_pack_t<sizeof...(Ts)>,
            hpx::util::decay_unwrap_t<Ts>...>;

        return result_type(
            static_cast<Derived const&>(action), HPX_FORWARD(Ts, vs)...);
    }
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Is, typename... Ts>
    struct is_bind_expression<hpx::detail::bound_action<Action, Is, Ts...>>
      : std::true_type
    {
    };

    template <typename Action, typename Is, typename... Ts>
    struct is_bound_action<hpx::detail::bound_action<Action, Is, Ts...>>
      : std::true_type
    {
    };
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx::serialization {

    // serialization of the bound action object
    HPX_CXX_EXPORT template <typename Archive, typename F, typename... Ts>
    void serialize(Archive& ar, ::hpx::detail::bound_action<F, Ts...>& bound,
        unsigned int const version = 0)
    {
        bound.serialize(ar, version);
    }
}    // namespace hpx::serialization
