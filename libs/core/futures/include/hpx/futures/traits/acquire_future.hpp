//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/futures/traits/is_future.hpp>
#include <hpx/futures/traits/is_future_range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/util.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::traits {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
        struct acquire_future_impl;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct acquire_future : detail::acquire_future_impl<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using acquire_future_t = typename acquire_future<T>::type;

    HPX_CXX_CORE_EXPORT struct acquire_future_disp
    {
        template <typename T>
        HPX_FORCEINLINE acquire_future_t<T> operator()(T&& t) const
        {
            return acquire_future<T>()(HPX_FORWARD(T, t));
        }
    };

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T, typename Enable>
        struct acquire_future_impl
        {
            static_assert(!is_future_or_future_range_v<T>,
                "T is a future or future range and must be acquired "
                "using the hpx::traits::acquire_future<T> "
                "specialization, not the default implementation.");

            using type = T;

            template <typename T_>
            HPX_FORCEINLINE T operator()(T_&& value) const
            {
                return HPX_FORWARD(T_, value);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename R>
        struct acquire_future_impl<hpx::future<R>>
        {
            using type = hpx::future<R>;

            HPX_FORCEINLINE hpx::future<R> operator()(
                hpx::future<R>& future) const noexcept
            {
                return HPX_MOVE(future);
            }

            HPX_FORCEINLINE hpx::future<R> operator()(
                hpx::future<R>&& future) const noexcept
            {
                return HPX_MOVE(future);
            }
        };

        template <typename R>
        struct acquire_future_impl<hpx::shared_future<R>>
        {
            using type = hpx::shared_future<R>;

            HPX_FORCEINLINE hpx::shared_future<R> operator()(
                hpx::shared_future<R> const& future) const
            {
                return future;
            }

            HPX_FORCEINLINE hpx::shared_future<R> operator()(
                hpx::shared_future<R>&& future) const noexcept
            {
                return HPX_MOVE(future);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(push_back)

        ///////////////////////////////////////////////////////////////////////
        template <typename Range>
        struct acquire_future_impl<Range,
            std::enable_if_t<hpx::traits::is_future_range_v<Range>>>
        {
            using future_type =
                typename traits::future_range_traits<Range>::future_type;
            using type = Range;

            template <typename Range_>
            void transform_future_disp(Range_&& futures, Range& values) const
            {
                detail::reserve_if_random_access_by_range(values, futures);
                if constexpr (has_push_back_v<std::decay_t<Range_>>)
                {
                    std::transform(util::begin(futures), util::end(futures),
                        std::back_inserter(values), acquire_future_disp());
                }
                else
                {
                    std::transform(util::begin(futures), util::end(futures),
                        util::begin(values), acquire_future_disp());
                }
            }

            template <typename Range_>
            HPX_FORCEINLINE Range operator()(Range_&& futures) const
            {
                Range values;
                transform_future_disp(HPX_FORWARD(Range_, futures), values);
                return values;
            }
        };
    }    // namespace detail
}    // namespace hpx::traits
