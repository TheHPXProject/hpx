//  Copyright (c) 2016-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/is_value_proxy.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Pred, typename Proj>
    struct invoke_projected
    {
        using pred_type = std::decay_t<Pred>;
        using proj_type = std::decay_t<Proj>;

        pred_type pred_;
        proj_type proj_;

        template <typename Pred_, typename Proj_>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr invoke_projected(
            Pred_&& pred, Proj_&& proj)
          : pred_(HPX_FORWARD(Pred_, pred))
          , proj_(HPX_FORWARD(Proj_, proj))
        {
        }

        template <typename T>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) operator()(
            T&& t)
        {
            return HPX_INVOKE(pred_, HPX_INVOKE(proj_, HPX_FORWARD(T, t)));
        }

        template <typename T>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) operator()(
            T&& t, T&& u)
        {
            return HPX_INVOKE(pred_, HPX_INVOKE(proj_, HPX_FORWARD(T, t)),
                HPX_INVOKE(proj_, HPX_FORWARD(T, u)));
        }
    };

    template <typename Pred>
    struct invoke_projected<Pred, hpx::identity>
    {
        using pred_type = std::decay_t<Pred>;

        pred_type pred_;

        template <typename Pred_>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr invoke_projected(
            Pred_&& pred, hpx::identity)
          : pred_(HPX_FORWARD(Pred_, pred))
        {
        }

        template <typename T>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) operator()(
            T&& t)
        {
            return HPX_INVOKE(pred_, HPX_FORWARD(T, t));
        }

        template <typename T>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) operator()(
            T&& t, T&& u)
        {
            return HPX_INVOKE(pred_, HPX_FORWARD(T, t), HPX_FORWARD(T, u));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F, typename Proj = hpx::identity>
    struct invoke_projected_ind
    {
        using pred_type = std::decay_t<F>;
        using proj_type = std::decay_t<Proj>;

        pred_type pred_;
        proj_type proj_;

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) operator()(
            Iter curr)
        {
            using value_type = hpx::traits::iter_reference_t<Iter>;
            if constexpr (hpx::traits::is_value_proxy_v<value_type>)
            {
                auto tmp = HPX_INVOKE(proj_, *curr);
                return HPX_INVOKE(pred_, tmp);
            }
            else
            {
                return HPX_INVOKE(pred_, HPX_INVOKE(proj_, *curr));
            }
        }
    };

    template <typename F>
    struct invoke_projected_ind<F, hpx::identity>
    {
        using pred_type = std::decay_t<F>;

        pred_type pred_;

        HPX_HOST_DEVICE HPX_FORCEINLINE explicit constexpr invoke_projected_ind(
            F& f) noexcept
          : pred_(f)
        {
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr invoke_projected_ind(
            F& f, hpx::identity) noexcept
          : pred_(f)
        {
        }

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr decltype(auto) operator()(
            Iter curr)
        {
            using value_type = hpx::traits::iter_reference_t<Iter>;
            if constexpr (hpx::traits::is_value_proxy_v<value_type>)
            {
                auto tmp = *curr;
                return HPX_INVOKE(pred_, tmp);
            }
            else
            {
                return HPX_INVOKE(pred_, *curr);
            }
        }
    };
}    // namespace hpx::parallel::util
