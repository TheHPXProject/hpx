//  Copyright (c) 2011-2012 Thomas Heller
//  Copyright (c) 2013-2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file bind.hpp
/// \page hpx::bind, hpx::placeholders::_1, hpx::placeholders::_2, ..., hpx::placeholders::_9
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/functional/traits/get_function_address.hpp>
#include <hpx/functional/traits/get_function_annotation.hpp>
#include <hpx/functional/traits/is_action.hpp>
#include <hpx/functional/traits/is_bind_expression.hpp>
#include <hpx/functional/traits/is_placeholder.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <std::size_t I>
        struct placeholder
        {
            static constexpr std::size_t value = I;
        };

        template <>
        struct placeholder<0>;    // not a valid placeholder

    }    // namespace detail

    /// The hpx::placeholders namespace contains the placeholder objects [_1,
    /// ..., _N] where N is an implementation defined maximum number.
    ///
    /// When used as an argument in a hpx::bind expression, the placeholder
    /// objects are stored in the generated function object, and when that
    /// function object is invoked with unbound arguments, each placeholder _N
    /// is replaced by the corresponding Nth unbound argument.
    ///
    /// The types of the placeholder objects are DefaultConstructible and
    /// CopyConstructible, their default copy/move constructors do not throw
    /// exceptions, and for any placeholder _N, the type
    /// hpx::is_placeholder<decltype(_N)> is defined, where
    /// hpx::is_placeholder<decltype(_N)> is derived from
    /// std::integral_constant<int, N>.
    namespace placeholders {

        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<1> _1 = {};
        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<2> _2 = {};
        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<3> _3 = {};
        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<4> _4 = {};
        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<5> _5 = {};
        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<6> _6 = {};
        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<7> _7 = {};
        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<8> _8 = {};
        HPX_CXX_CORE_EXPORT inline constexpr detail::placeholder<9> _9 = {};
    }    // namespace placeholders

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <std::size_t I>
        struct bind_eval_placeholder
        {
            template <typename T, typename... Us>
            static constexpr HPX_HOST_DEVICE decltype(auto) call(
                T&& /*t*/, Us&&... vs)
            {
                return util::member_pack_for<Us&&...>(
                    std::piecewise_construct, HPX_FORWARD(Us, vs)...)
                    .template get<I>();
            }
        };

        HPX_CXX_CORE_EXPORT template <typename T, std::size_t NumUs,
            typename TD = std::decay_t<T>, typename Enable = void>
        struct bind_eval
        {
            template <typename... Us>
            static constexpr HPX_HOST_DEVICE T&& call(T&& t, Us&&... /*vs*/)
            {
                return HPX_FORWARD(T, t);
            }
        };

        template <typename T, std::size_t NumUs, typename TD>
        struct bind_eval<T, NumUs, TD,
            std::enable_if_t<hpx::is_placeholder_v<TD> != 0 &&
                (hpx::is_placeholder_v<TD> <= NumUs)>>
          : bind_eval_placeholder<
                static_cast<std::size_t>(hpx::is_placeholder_v<TD>) - 1>
        {
        };

        template <typename T, std::size_t NumUs, typename TD>
        struct bind_eval<T, NumUs, TD,
            std::enable_if_t<hpx::is_bind_expression_v<TD>>>
        {
            template <typename... Us>
            static constexpr HPX_HOST_DEVICE util::invoke_result_t<T, Us...>
            call(T&& t, Us&&... vs)
            {
                return HPX_INVOKE(HPX_FORWARD(T, t), HPX_FORWARD(Us, vs)...);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename F, typename Ts, typename... Us>
        struct invoke_bound_result;

        template <typename F, typename... Ts, typename... Us>
        struct invoke_bound_result<F, util::pack<Ts...>, Us...>
          : util::invoke_result<F,
                decltype(bind_eval<Ts, sizeof...(Us)>::call(
                    std::declval<Ts>(), std::declval<Us>()...))...>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename F, typename Ts, typename... Us>
        using invoke_bound_result_t =
            typename invoke_bound_result<F, Ts, Us...>::type;

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename F, typename Is, typename... Ts>
        class bound;

        template <typename F, std::size_t... Is, typename... Ts>
        class bound<F, util::index_pack<Is...>, Ts...>
        {
        public:
            bound() = default;    // needed for serialization

            template <typename F_, typename... Ts_,
                typename = std::enable_if_t<std::is_constructible_v<F, F_>>>
            constexpr explicit bound(F_&& f, Ts_&&... vs)
              : _f(HPX_FORWARD(F_, f))
              , _args(std::piecewise_construct, HPX_FORWARD(Ts_, vs)...)
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            bound(bound const&) = default;
            bound(bound&&) = default;
#else
            constexpr HPX_HOST_DEVICE bound(bound const& other)
              : _f(other._f)
              , _args(other._args)
            {
            }

            constexpr HPX_HOST_DEVICE bound(bound&& other) noexcept
              : _f(HPX_MOVE(other._f))
              , _args(HPX_MOVE(other._args))
            {
            }
#endif

            bound& operator=(bound const&) = delete;
            bound& operator=(bound&&) = delete;

            ~bound() = default;

#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26800)    //  Use of a moved from object: '(*<vs_0>)'
#endif

            // NOLINTBEGIN(bugprone-use-after-move)
            template <typename... Us>
            constexpr HPX_HOST_DEVICE
                invoke_bound_result_t<F&, util::pack<Ts&...>, Us&&...>
                operator()(Us&&... vs) &
            {
                return HPX_INVOKE(_f,
                    detail::bind_eval<Ts&, sizeof...(Us)>::call(
                        _args.template get<Is>(), HPX_FORWARD(Us, vs)...)...);
            }

            template <typename... Us>
            constexpr HPX_HOST_DEVICE invoke_bound_result_t<F const&,
                util::pack<Ts const&...>, Us&&...>
            operator()(Us&&... vs) const&
            {
                return HPX_INVOKE(_f,
                    detail::bind_eval<Ts const&, sizeof...(Us)>::call(
                        _args.template get<Is>(), HPX_FORWARD(Us, vs)...)...);
            }

            template <typename... Us>
            constexpr HPX_HOST_DEVICE
                invoke_bound_result_t<F&&, util::pack<Ts&&...>, Us&&...>
                operator()(Us&&... vs) &&
            {
                return HPX_INVOKE(HPX_MOVE(_f),
                    detail::bind_eval<Ts, sizeof...(Us)>::call(
                        HPX_MOVE(_args).template get<Is>(),
                        HPX_FORWARD(Us, vs)...)...);
            }

            template <typename... Us>
            constexpr HPX_HOST_DEVICE invoke_bound_result_t<F const&&,
                util::pack<Ts const&&...>, Us&&...>
            operator()(Us&&... vs) const&&
            {
                return HPX_INVOKE(HPX_MOVE(_f),
                    detail::bind_eval<Ts const, sizeof...(Us)>::call(
                        HPX_MOVE(_args).template get<Is>(),
                        HPX_FORWARD(Us, vs)...)...);
            }
            // NOLINTEND(bugprone-use-after-move)

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const /*version*/)
            {
                // clang-format off
                ar & _f;
                ar & _args;
                // clang-format on
            }

            [[nodiscard]] constexpr std::size_t get_function_address()
                const noexcept
            {
                return traits::get_function_address<F>::call(_f);
            }

            [[nodiscard]] constexpr char const* get_function_annotation()
                const noexcept
            {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                return traits::get_function_annotation<F>::call(_f);
#else
                return nullptr;
#endif
            }

            [[nodiscard]] hpx::tracing::annotation_handle
            get_function_annotation_tracing() const
            {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                return traits::get_function_annotation_tracing<F>::call(_f);
#else
                static auto sh =
                    hpx::tracing::create_annotation_handle("bound");
                return sh;
#endif
            }

        private:
            F _f;
            util::member_pack_for<Ts...> _args;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The function template \a bind generates a forwarding call wrapper for \a f.
    /// Calling this wrapper is equivalent to invoking \a f with some of its
    /// arguments bound to \a vs.
    ///
    /// \param f    Callable object (function object, pointer to function,
    ///             reference to function, pointer to member function, or pointer
    ///             to data member) that will be bound to some arguments
    /// \param vs   list of arguments to bind, with the unbound arguments replaced
    ///             by the placeholders _1, _2, _3... of namespace \a hpx::placeholders
    /// \returns    A function object of unspecified type \a T, for which
    ///             \code hpx::is_bind_expression<T>::value == true. \endcode
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
        requires(!traits::is_action_v<std::decay_t<F>>)
    constexpr detail::bound<std::decay_t<F>,
        util::make_index_pack_t<sizeof...(Ts)>, util::decay_unwrap_t<Ts>...>
    bind(F&& f, Ts&&... vs)
    {
        using result_type = detail::bound<std::decay_t<F>,
            util::make_index_pack_t<sizeof...(Ts)>,
            util::decay_unwrap_t<Ts>...>;

        return result_type(HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...);
    }
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    template <typename F, typename... Ts>
    struct is_bind_expression<hpx::detail::bound<F, Ts...>> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    template <std::size_t I>
    struct is_placeholder<hpx::detail::placeholder<I>>
      : std::integral_constant<int, I>
    {
    };
}    // namespace hpx

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    struct get_function_address<hpx::detail::bound<F, Ts...>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            hpx::detail::bound<F, Ts...> const& f) noexcept
        {
            return f.get_function_address();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    struct get_function_annotation<hpx::detail::bound<F, Ts...>>
    {
        [[nodiscard]] static constexpr char const* call(
            hpx::detail::bound<F, Ts...> const& f) noexcept
        {
            return f.get_function_annotation();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    struct get_function_annotation_tracing<hpx::detail::bound<F, Ts...>>
    {
        [[nodiscard]] static hpx::tracing::annotation_handle call(
            hpx::detail::bound<F, Ts...> const& f) noexcept
        {
            return f.get_function_annotation_tracing();
        }
    };
}    // namespace hpx::traits
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::serialization {

    // serialization of the bound object
    HPX_CXX_CORE_EXPORT template <typename Archive, typename F, typename... Ts>
    void serialize(Archive& ar, ::hpx::detail::bound<F, Ts...>& bound,
        unsigned int const version = 0)
    {
        bound.serialize(ar, version);
    }

    // serialization of placeholders is trivial, just provide empty functions
    HPX_CXX_CORE_EXPORT template <typename Archive, std::size_t I>
    constexpr void serialize(Archive& /* ar */,
        ::hpx::detail::placeholder<I>& /*placeholder*/,
        unsigned int const /*version*/ = 0) noexcept
    {
    }
}    // namespace hpx::serialization
