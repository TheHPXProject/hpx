//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c) 2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/futures/traits/future_access.hpp>
#include <hpx/futures/traits/future_traits.hpp>
#include <hpx/futures/traits/is_future.hpp>
#include <hpx/futures/traits/is_future_range.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/util.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::traits {

    namespace detail {

        template <typename T, typename Enable = void>
        struct acquire_shared_state_impl;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct acquire_shared_state
      : detail::acquire_shared_state_impl<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using acquire_shared_state_t = typename acquire_shared_state<T>::type;

    HPX_CXX_CORE_EXPORT struct acquire_shared_state_disp
    {
        template <typename T>
        HPX_FORCEINLINE acquire_shared_state_t<T> operator()(T&& t) const
        {
            return acquire_shared_state<T>()(HPX_FORWARD(T, t));
        }
    };

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename Enable>
        struct acquire_shared_state_impl
        {
            static_assert(!traits::detail::is_future_or_future_range_v<T>,
                "T is a future or future range and must be acquired "
                "using the hpx::traits::acquire_shared_state<T> "
                "specialization, not the default implementation.");

            using type = T;

            template <typename T_>
            HPX_FORCEINLINE T operator()(T_&& value) const
            {
                return HPX_FORWARD(T_, value);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Future>
        struct acquire_shared_state_impl<Future,
            std::enable_if_t<is_future_v<Future>>>
        {
            using type = traits::detail::shared_state_ptr_t<
                traits::future_traits_t<Future>> const&;

            HPX_FORCEINLINE type operator()(Future const& f) const
            {
                return traits::future_access<Future>::get_shared_state(f);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Cont>
        struct is_static_array : std::false_type
        {
        };

        template <typename T, std::size_t N>
        struct is_static_array<std::array<T, N>> : std::true_type
        {
        };

        template <typename Range>
        struct acquire_shared_state_impl<Range,
            std::enable_if_t<traits::is_future_range_v<Range>>>
        {
            using type = traits::detail::shared_state_ptr_for_t<Range>;

            template <typename Range_>
            HPX_FORCEINLINE type operator()(Range_&& futures) const
            {
                if constexpr (detail::is_static_array<type>::value)
                {
                    type values;
                    std::transform(util::begin(futures), util::end(futures),
                        util::begin(values), acquire_shared_state_disp());
                    return values;
                }
                else
                {
                    type values;
                    detail::reserve_if_random_access_by_range(values, futures);

                    std::transform(util::begin(futures), util::end(futures),
                        std::back_inserter(values),
                        acquire_shared_state_disp());
                    return values;
                }
            }
        };

        template <typename Iterator>
        struct acquire_shared_state_impl<Iterator,
            std::enable_if_t<traits::is_iterator_v<Iterator>>>
        {
            using future_type =
                typename std::iterator_traits<Iterator>::value_type;
            using shared_state_ptr =
                traits::detail::shared_state_ptr_for_t<future_type>;
            using type = std::vector<shared_state_ptr>;

            template <typename Iter>
            HPX_FORCEINLINE type operator()(Iter begin, Iter end) const
            {
                type values;
                detail::reserve_if_random_access_by_range(values, begin, end);

                std::transform(begin, end, std::back_inserter(values),
                    acquire_shared_state_disp());

                return values;
            }

            template <typename Iter>
            HPX_FORCEINLINE type operator()(Iter begin, std::size_t count) const
            {
                type values;
                values.reserve(count);

                for (std::size_t i = 0; i != count; ++i)
                {
                    values.push_back(acquire_shared_state_disp()(*begin++));
                }

                return values;
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        HPX_FORCEINLINE acquire_shared_state_t<T> get_shared_state(T&& t)
        {
            return acquire_shared_state<T>()(HPX_FORWARD(T, t));
        }

        HPX_CXX_CORE_EXPORT template <typename R>
        HPX_FORCEINLINE
            hpx::intrusive_ptr<lcos::detail::future_data_base<R>> const&
            get_shared_state(
                hpx::intrusive_ptr<lcos::detail::future_data_base<R>> const& t)
        {
            return t;    // NOLINT(bugprone-return-const-ref-from-parameter)
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Future>
        struct wait_get_shared_state
        {
            HPX_FORCEINLINE
            traits::detail::shared_state_ptr_for_t<Future> const& operator()(
                Future const& f) const
            {
                return traits::detail::get_shared_state(f);
            }
        };
    }    // namespace detail
}    // namespace hpx::traits
