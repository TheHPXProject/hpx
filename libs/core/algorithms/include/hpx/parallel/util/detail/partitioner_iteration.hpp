//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/datastructures/traits/is_tuple_like.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/type_support.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::util::detail {

    // Hand-crafted function object allowing to replace a more complex
    // bind(hpx::functional::invoke_fused(), f1, _1)
    HPX_CXX_CORE_EXPORT template <typename Result, typename F>
    struct partitioner_iteration
    {
        std::decay_t<F> f_;

        // Overload for tuple-like types - unpack using index_pack
        template <typename T>
            requires(hpx::traits::is_tuple_like_v<std::decay_t<T>>)
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Result operator()(T&& t)
        {
            using embedded_index_pack_type = hpx::util::make_index_pack<
                hpx::tuple_size<std::decay_t<T>>::value>;

            // NOLINTBEGIN(bugprone-use-after-move)
            if constexpr (std::invocable<F, embedded_index_pack_type, T&&>)
            {
                return HPX_INVOKE_R(
                    Result, f_, embedded_index_pack_type{}, HPX_FORWARD(T, t));
            }
            else
            {
                return (*this)(embedded_index_pack_type{}, HPX_FORWARD(T, t));
            }
            // NOLINTEND(bugprone-use-after-move)
        }

        // Overload for non-tuple types (std::size_t from stdexec bulk)
        template <typename T>
            requires(!hpx::traits::is_tuple_like_v<std::decay_t<T>>)
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Result operator()(T&& t)
        {
            return HPX_INVOKE_R(Result, f_, HPX_FORWARD(T, t));
        }

        template <std::size_t... Is, typename... Ts>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Result operator()(
            hpx::util::index_pack<Is...>, hpx::tuple<Ts...>& t)
        {
            return hpx::util::void_guard<Result>(),
                   HPX_INVOKE(f_, hpx::get<Is>(t)...);
        }

        template <std::size_t... Is, typename... Ts>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Result operator()(
            hpx::util::index_pack<Is...>, hpx::tuple<Ts...> const& t)
        {
            return hpx::util::void_guard<Result>(),
                   HPX_INVOKE(f_, hpx::get<Is>(t)...);
        }

        template <std::size_t... Is, typename... Ts>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Result operator()(
            hpx::util::index_pack<Is...>, hpx::tuple<Ts...>&& t)
        {
            // NOLINTBEGIN(bugprone-use-after-move)
            return hpx::util::void_guard<Result>(),
                   HPX_INVOKE(f_, hpx::get<Is>(HPX_MOVE(t))...);
            // NOLINTEND(bugprone-use-after-move)
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & f_;
        }
    };
}    // namespace hpx::parallel::util::detail

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
#include <hpx/modules/functional.hpp>

namespace hpx::traits {

    HPX_CXX_CORE_EXPORT template <typename Result, typename F>
    struct get_function_address<
        parallel::util::detail::partitioner_iteration<Result, F>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            parallel::util::detail::partitioner_iteration<Result, F> const&
                f) noexcept
        {
            return get_function_address<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Result, typename F>
    struct get_function_annotation<
        parallel::util::detail::partitioner_iteration<Result, F>>
    {
        [[nodiscard]] static constexpr char const* call(
            parallel::util::detail::partitioner_iteration<Result, F> const&
                f) noexcept
        {
            return get_function_annotation<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Result, typename F>
    struct get_function_annotation_tracing<
        parallel::util::detail::partitioner_iteration<Result, F>>
    {
        [[nodiscard]] static hpx::tracing::annotation_handle call(
            parallel::util::detail::partitioner_iteration<Result, F> const&
                f) noexcept
        {
            return get_function_annotation_tracing<std::decay_t<F>>::call(f.f_);
        }
    };

}    // namespace hpx::traits

#endif
