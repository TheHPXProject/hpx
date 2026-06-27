//  Copyright (c) 2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename T, std::size_t N>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE T* begin_impl(
            T (&array)[N], long) noexcept
        {
            return &array[0];
        }

        template <typename T, std::size_t N>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE T* end_impl(
            T (&array)[N], long) noexcept
        {
            return &array[N];
        }

        template <typename T, std::size_t N>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE std::size_t
        size_impl(T const (&)[N], long) noexcept
        {
            return N;
        }

        template <typename T, std::size_t N>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE bool empty_impl(
            T const (&)[N], long) noexcept
        {
            return false;
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename C, typename R = decltype(std::declval<C&>().begin())>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R begin_impl(
            C& c, long) noexcept(noexcept(c.begin()))
        {
            return c.begin();
        }

        template <typename C, typename R = decltype(std::declval<C&>().end())>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R end_impl(
            C& c, long) noexcept(noexcept(c.begin()))
        {
            return c.end();
        }

        template <typename C,
            typename R = decltype(std::declval<C const&>().size())>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R size_impl(
            C const& c, long) noexcept(noexcept(c.size()))
        {
            return c.size();
        }

        template <typename C,
            typename R = decltype(std::declval<C const&>().empty())>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R empty_impl(
            C const& c, long) noexcept(noexcept(c.empty()))
        {
            return c.empty();
        }

        template <typename C,
            typename R = decltype(std::declval<C&>().subrange(
                std::declval<std::ptrdiff_t>(), std::declval<std::size_t>()))>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R subrange_impl(
            C& c, std::ptrdiff_t delta, std::size_t size,
            long) noexcept(noexcept(c.subrange(delta, size)))
        {
            return c.subrange(delta, size);
        }

        template <typename C,
            typename R = decltype(std::declval<C&>().iterate())>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R iterate_impl(
            C& c, long) noexcept(noexcept(c.iterate()))
        {
            return c.iterate();
        }

        ///////////////////////////////////////////////////////////////////////
        namespace range_impl {

            struct fallback
            {
                template <typename T>
                fallback(T const&)
                {
                }
            };

            fallback begin(fallback);

            template <typename C,
                typename R = decltype(begin(std::declval<C&>()))>
            [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R
            begin_impl(C& c, int) noexcept(noexcept(begin(c)))
            {
                return begin(c);
            }

            fallback end(fallback);

            template <typename C,
                typename R = decltype(end(std::declval<C&>()))>
            [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R end_impl(
                C& c, int) noexcept(noexcept(end(c)))
            {
                return end(c);
            }

            fallback subrange(fallback, std::ptrdiff_t, std::size_t);

            // clang-format off
             template <typename C,
                typename R = decltype(
                    subrange(std::declval<C&>(), std::declval<std::ptrdiff_t>(),
                        std::declval<std::size_t>()))>
            // clang-format on
            [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R
            subrange_impl(C& c, std::ptrdiff_t delta, std::size_t size,
                int) noexcept(noexcept(subrange(c, delta, size)))
            {
                return subrange(c, delta, size);
            }

            fallback empty(fallback);

            template <typename C,
                typename R = decltype(empty(std::declval<C&>()))>
            [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R
            empty_impl(C& c) noexcept(noexcept(empty(c)))
            {
                return empty(c);
            }

            fallback size(fallback);

            template <typename C,
                typename R = decltype(size(std::declval<C&>()))>
            [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R size_impl(
                C& c) noexcept(noexcept(size(c)))
            {
                return size(c);
            }

            fallback iterate(fallback);

            template <typename C,
                typename R = decltype(iterate(std::declval<C&>()))>
            [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R
            iterate_impl(C& c, int) noexcept(noexcept(iterate(c)))
            {
                return iterate(c);
            }
        }    // namespace range_impl

        HPX_CXX_CORE_EXPORT using range_impl::begin_impl;
        HPX_CXX_CORE_EXPORT using range_impl::end_impl;
        HPX_CXX_CORE_EXPORT using range_impl::iterate_impl;
        HPX_CXX_CORE_EXPORT using range_impl::subrange_impl;

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
        inline constexpr bool has_size_v = false;

        template <typename T>
        inline constexpr bool has_size_v<T,
            std::void_t<decltype(size(std::declval<T const&>()))>> = true;

        template <typename C>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE std::ptrdiff_t
        size_impl(C const& c, int)
        {
            if constexpr (has_size_v<C>)
            {
                return range_impl::size_impl(c);
            }
            else
            {
                return std::distance(begin_impl(c, 0L), end_impl(c, 0L));
            }
        }

        HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
        inline constexpr bool has_empty_v = false;

        template <typename T>
        inline constexpr bool has_empty_v<T,
            std::void_t<decltype(empty(std::declval<T const&>()))>> = true;

        template <typename C>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE bool empty_impl(
            C const& c, int)
        {
            if constexpr (has_empty_v<C>)
            {
                return range_impl::empty_impl(c);
            }
            else
            {
                return begin_impl(c, 0L) == end_impl(c, 0L);
            }
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        struct result_of_begin
        {
            using type = decltype(detail::begin_impl(std::declval<T&>(), 0L));
        };

        HPX_CXX_CORE_EXPORT template <typename T,
            typename Iter = typename result_of_begin<T>::type>
        struct iterator
        {
            using type = Iter;
        };

        template <typename T>
        struct iterator<T, range_impl::fallback>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        struct result_of_end
        {
            using type = decltype(detail::end_impl(std::declval<T&>(), 0L));
        };

        HPX_CXX_CORE_EXPORT template <typename T,
            typename Iter = typename result_of_end<T>::type>
        struct sentinel
        {
            using type = Iter;
        };

        template <typename T>
        struct sentinel<T, range_impl::fallback>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        struct result_of_subrange
        {
            using type = decltype(detail::subrange_impl(std::declval<T&>(),
                std::declval<std::ptrdiff_t>(), std::declval<std::size_t>(),
                0L));
        };

        HPX_CXX_CORE_EXPORT template <typename T,
            typename R = typename result_of_subrange<T>::type>
        struct subrange
        {
            using type = R;
        };

        template <typename T>
        struct subrange<T, range_impl::fallback>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        struct result_of_iterate
        {
            using type = decltype(detail::iterate_impl(std::declval<T&>(), 0L));
        };

        HPX_CXX_CORE_EXPORT template <typename T,
            typename R = typename result_of_iterate<T>::type>
        struct iterate
        {
            using type = R;
        };

        template <typename T>
        struct iterate<T, range_impl::fallback>
        {
        };

        ///////////////////////////////////////////////////////////////////////////
        // return whether a given type is a range generator (i.e. exposes supports
        // an iterate function that returns a range
        HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
        struct is_range_generator : std::false_type
        {
        };

        template <typename T>
        inline constexpr bool is_range_generator_v =
            is_range_generator<T>::value;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    namespace range_adl {

        HPX_CXX_CORE_EXPORT template <typename C,
            typename Iterator = typename detail::iterator<C>::type>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE Iterator begin(
            C& c) noexcept(noexcept(detail::begin_impl(c, 0L)))
        {
            return detail::begin_impl(c, 0L);
        }

        HPX_CXX_CORE_EXPORT template <typename C,
            typename Iterator = typename detail::iterator<C const>::type>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE Iterator begin(
            C const& c) noexcept(noexcept(detail::begin_impl(c, 0L)))
        {
            return detail::begin_impl(c, 0L);
        }

        HPX_CXX_CORE_EXPORT template <typename C,
            typename Sentinel = typename detail::sentinel<C>::type>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE Sentinel end(
            C& c) noexcept(noexcept(detail::end_impl(c, 0L)))
        {
            return detail::end_impl(c, 0L);
        }

        HPX_CXX_CORE_EXPORT template <typename C,
            typename Sentinel = typename detail::sentinel<C const>::type>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE Sentinel end(
            C const& c) noexcept(noexcept(detail::end_impl(c, 0L)))
        {
            return detail::end_impl(c, 0L);
        }

        HPX_CXX_CORE_EXPORT template <typename C,
            typename Enable = std::enable_if_t < std::ranges::range<C> ||
                detail::is_range_generator_v<C> >>
                    [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE
                        std::size_t
                        size(C const& c) noexcept(
                            noexcept(detail::size_impl(c, 0L)))
        {
            return detail::size_impl(c, 0L);
        }

        HPX_CXX_CORE_EXPORT template <typename C,
            typename Enable = std::enable_if_t < std::ranges::range<C> ||
                detail::is_range_generator_v<C> >>
                    [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE bool
                    empty(C const& c) noexcept(
                        noexcept(detail::empty_impl(c, 0L)))
        {
            return detail::empty_impl(c, 0L);
        }

        HPX_CXX_CORE_EXPORT template <typename C,
            typename Range = typename detail::subrange<C const>::type>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE Range subrange(
            C const& c, std::ptrdiff_t delta,
            std::size_t size) noexcept(noexcept(detail::subrange_impl(c,
            static_cast<std::ptrdiff_t>(0), static_cast<std::size_t>(0), 0L)))
        {
            return detail::subrange_impl(c, delta, size, 0L);
        }

        HPX_CXX_CORE_EXPORT template <typename C,
            typename R = typename detail::iterate<C const>::type>
        [[nodiscard]] HPX_HOST_DEVICE constexpr HPX_FORCEINLINE R iterate(
            C const& c) noexcept(noexcept(detail::iterate_impl(c, 0L)))
        {
            return detail::iterate_impl(c, 0L);
        }
    }    // namespace range_adl

    HPX_CXX_CORE_EXPORT using namespace range_adl;

    namespace detail {

        template <typename T>
        struct is_range_generator<T,
            std::enable_if_t<std::ranges::range<decltype(hpx::util::iterate(
                std::declval<T&>()))>>> : std::true_type
        {
        };
    }    // namespace detail
}    // namespace hpx::util
