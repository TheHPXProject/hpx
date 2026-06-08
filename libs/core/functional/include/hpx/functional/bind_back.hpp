//  Copyright (c) 2011-2012 Thomas Heller
//  Copyright (c) 2013-2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file bind_back.hpp
/// \page hpx::bind_back
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/functional/one_shot.hpp>
#include <hpx/functional/traits/get_function_address.hpp>
#include <hpx/functional/traits/get_function_annotation.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::detail {

    template <typename F, typename Ts, typename... Us>
    struct invoke_bound_back_result;

    template <typename F, typename... Ts, typename... Us>
    struct invoke_bound_back_result<F, util::pack<Ts...>, Us...>
      : util::invoke_result<F, Us..., Ts...>
    {
    };

    template <typename F, typename Ts, typename... Us>
    using invoke_bound_back_result_t =
        typename invoke_bound_back_result<F, Ts, Us...>::type;

    ///////////////////////////////////////////////////////////////////////
    template <typename F, typename Is, typename... Ts>
    class bound_back;

    template <typename F, std::size_t... Is, typename... Ts>
    class bound_back<F, util::index_pack<Is...>, Ts...>
    {
    public:
        bound_back() = default;    // needed for serialization

        template <typename F_, typename... Ts_>
            requires(std::is_constructible_v<F, F_>)
        constexpr explicit bound_back(F_&& f, Ts_&&... vs)
          : _f(HPX_FORWARD(F_, f))
          , _args(std::piecewise_construct, HPX_FORWARD(Ts_, vs)...)
        {
        }

#if !defined(__NVCC__) && !defined(__CUDACC__)
        bound_back(bound_back const&) = default;
        bound_back(bound_back&&) = default;
#else
        constexpr HPX_HOST_DEVICE bound_back(bound_back const& other)
          : _f(other._f)
          , _args(other._args)
        {
        }

        constexpr HPX_HOST_DEVICE bound_back(bound_back&& other) noexcept
          : _f(HPX_MOVE(other._f))
          , _args(HPX_MOVE(other._args))
        {
        }
#endif

        bound_back& operator=(bound_back const&) = delete;
        bound_back& operator=(bound_back&&) = delete;

        ~bound_back() = default;

        template <typename... Us>
        constexpr HPX_HOST_DEVICE
            invoke_bound_back_result_t<F&, util::pack<Ts&...>, Us&&...>
            operator()(Us&&... vs) &
        {
            return HPX_INVOKE(
                _f, HPX_FORWARD(Us, vs)..., _args.template get<Is>()...);
        }

        template <typename... Us>
        constexpr HPX_HOST_DEVICE invoke_bound_back_result_t<F const&,
            util::pack<Ts const&...>, Us&&...>
        operator()(Us&&... vs) const&
        {
            return HPX_INVOKE(
                _f, HPX_FORWARD(Us, vs)..., _args.template get<Is>()...);
        }

        template <typename... Us>
        constexpr HPX_HOST_DEVICE
            invoke_bound_back_result_t<F&&, util::pack<Ts&&...>, Us&&...>
            operator()(Us&&... vs) &&
        {
            return HPX_INVOKE(HPX_MOVE(_f), HPX_FORWARD(Us, vs)...,
                HPX_MOVE(_args).template get<Is>()...);
        }

        template <typename... Us>
        constexpr HPX_HOST_DEVICE invoke_bound_back_result_t<F const&&,
            util::pack<Ts const&&...>, Us&&...>
        operator()(Us&&... vs) const&&
        {
            return HPX_INVOKE(HPX_MOVE(_f), HPX_FORWARD(Us, vs)...,
                HPX_MOVE(_args).template get<Is>()...);
        }

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
                hpx::tracing::create_annotation_handle("bound_back");
            return sh;
#endif
        }

    private:
        F _f;
        util::member_pack_for<Ts...> _args;
    };
}    // namespace hpx::detail

namespace hpx {

    /// \brief Function templates \c bind_back generate a forwarding call wrapper
    ///        for \c f. Calling this wrapper is equivalent to invoking \c f with its
    ///        last \c sizeof...(Ts) parameters bound to \c vs.
    ///
    /// \param f    Callable object (function object, pointer to function,
    ///             reference to function, pointer to member function, or pointer
    ///             to data member) that will be bound to some arguments
    /// \param vs   list of the arguments to bind to the last \c sizeof...(Ts)
    ///             parameters of \c f
    /// \returns    A function object of type \c T that is unspecified, except that
    ///             the types of objects returned by two calls to \c hpx::bind_back
    ///             with the same arguments are the same.
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    constexpr hpx::detail::bound_back<std::decay_t<F>,
        util::make_index_pack_t<sizeof...(Ts)>, util::decay_unwrap_t<Ts>...>
    bind_back(F&& f, Ts&&... vs)
    {
        using result_type = hpx::detail::bound_back<std::decay_t<F>,
            util::make_index_pack_t<sizeof...(Ts)>,
            util::decay_unwrap_t<Ts>...>;

        return result_type(HPX_FORWARD(F, f), HPX_FORWARD(Ts, vs)...);
    }

    // nullary functions do not need to be bound again
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr std::decay_t<F> bind_back(F&& f)    //-V524
    {
        return HPX_FORWARD(F, f);
    }
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    struct get_function_address<hpx::detail::bound_back<F, Ts...>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            hpx::detail::bound_back<F, Ts...> const& f) noexcept
        {
            return f.get_function_address();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    struct get_function_annotation<hpx::detail::bound_back<F, Ts...>>
    {
        [[nodiscard]] static constexpr char const* call(
            hpx::detail::bound_back<F, Ts...> const& f) noexcept
        {
            return f.get_function_annotation();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    struct get_function_annotation_tracing<hpx::detail::bound_back<F, Ts...>>
    {
        [[nodiscard]] static hpx::tracing::annotation_handle call(
            hpx::detail::bound_back<F, Ts...> const& f) noexcept
        {
            return f.get_function_annotation_tracing();
        }
    };
}    // namespace hpx::traits
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::serialization {

    // serialization of the bound_back object
    HPX_CXX_CORE_EXPORT template <typename Archive, typename F, typename... Ts>
    void serialize(Archive& ar, ::hpx::detail::bound_back<F, Ts...>& bound,
        unsigned int const version = 0)
    {
        bound.serialize(ar, version);
    }
}    // namespace hpx::serialization
