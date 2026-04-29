//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental::detail {

    HPX_CXX_CORE_EXPORT template <typename Tag, typename IsPack, typename... Ts>
    struct partial_algorithm_base;

    HPX_CXX_CORE_EXPORT template <typename Tag, std::size_t... Is,
        typename... Ts>
    struct partial_algorithm_base<Tag, hpx::util::index_pack<Is...>, Ts...>
    {
    private:
        HPX_NO_UNIQUE_ADDRESS hpx::util::member_pack_for<std::decay_t<Ts>...>
            ts;

    public:
        template <typename... Us>
        constexpr HPX_FORCEINLINE auto invoke(Us&&... us) &&
        {
            return Tag{}(
                HPX_FORWARD(Us, us)..., HPX_MOVE(ts).template get<Is>()...);
        }

        template <typename... Ts_>
        explicit constexpr partial_algorithm_base(Ts_&&... ts)
          : ts(std::piecewise_construct, HPX_FORWARD(Ts_, ts)...)
        {
        }

        partial_algorithm_base(partial_algorithm_base&&) = default;
        partial_algorithm_base& operator=(partial_algorithm_base&&) = default;
        partial_algorithm_base(partial_algorithm_base const&) = delete;
        partial_algorithm_base& operator=(
            partial_algorithm_base const&) = delete;

        // clang-format off
        template <typename U,
            HPX_CONCEPT_REQUIRES_(
                hpx::execution::experimental::is_sender_v<U> &&
                hpx::execution::experimental::detail::
                    is_completion_scheduler_tag_invocable_v<
                        hpx::execution::experimental::set_value_t,
                        U, Tag, Ts...>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto operator|(
            U&& u, partial_algorithm_base p)
        {
            auto scheduler =
                hpx::execution::experimental::get_completion_scheduler<
                    hpx::execution::experimental::set_value_t>(
                    hpx::execution::experimental::get_env(u));

            return HPX_MOVE(p).invoke(HPX_MOVE(scheduler), HPX_FORWARD(U, u));
        }

        // clang-format off
        template <typename U,
            HPX_CONCEPT_REQUIRES_(
                hpx::execution::experimental::is_sender_v<U> &&
               !hpx::execution::experimental::detail::
                    is_completion_scheduler_tag_invocable_v<
                        hpx::execution::experimental::set_value_t,
                        U, Tag, Ts...>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto operator|(
            U&& u, partial_algorithm_base p)
        {
            return HPX_MOVE(p).invoke(HPX_FORWARD(U, u));
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Tag, typename... Ts>
    using partial_algorithm = partial_algorithm_base<Tag,
        hpx::util::make_index_pack_t<sizeof...(Ts)>, Ts...>;
}    // namespace hpx::execution::experimental::detail
