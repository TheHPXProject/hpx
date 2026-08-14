//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/execution/algorithms/detail/is_negative.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename InputIterator, typename Distance>
    HPX_HOST_DEVICE constexpr void advance_impl(
        InputIterator& i, Distance n, std::random_access_iterator_tag) noexcept
    {
        i += n;
    }

    HPX_CXX_CORE_EXPORT template <typename InputIterator, typename Distance>
    HPX_HOST_DEVICE constexpr void advance_impl(
        InputIterator& i, Distance n, std::bidirectional_iterator_tag) noexcept
    {
        if (n < 0)
        {
            while (n++)
                --i;
        }
        else
        {
            while (n--)
                ++i;
        }
    }

    HPX_CXX_CORE_EXPORT template <typename InputIterator, typename Distance>
    HPX_HOST_DEVICE constexpr void advance_impl(
        InputIterator& i, Distance n, std::input_iterator_tag) noexcept
    {
#if defined(HPX_INTEL_VERSION)
#pragma warning(push)
#pragma warning(disable : 186)
#endif
        HPX_ASSERT(n >= 0);
#if defined(HPX_INTEL_VERSION)
#pragma warning(pop)
#endif
        while (n--)
            ++i;
    }

    HPX_CXX_CORE_EXPORT template <typename InputIterator, typename Distance>
    HPX_HOST_DEVICE constexpr void advance(
        InputIterator& i, Distance n) noexcept
    {
        advance_impl(i, n,
            typename std::iterator_traits<InputIterator>::iterator_category());
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iterable, typename Enable = void>
    struct calculate_distance
    {
        template <typename T1, typename T2>
        HPX_FORCEINLINE constexpr static std::size_t call(T1 t1,
            T2 t2) noexcept(noexcept(std::declval<T1>() - std::declval<T2>()))
        {
            return std::size_t(t2 - t1);
        }
    };

    template <typename Iterable>
    struct calculate_distance<Iterable,
        std::enable_if_t<hpx::traits::is_iterator_v<Iterable>>>
    {
        template <typename Iter1, typename Iter2>
        HPX_FORCEINLINE constexpr static std::size_t call(
            Iter1 iter1, Iter2 iter2)
        {
            return std::distance(iter1, iter2);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Iterable>
    HPX_FORCEINLINE constexpr std::size_t distance(
        Iterable iter1, Iterable iter2)
    {
        return calculate_distance<std::decay_t<Iterable>>::call(iter1, iter2);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iterable, typename Enable = void>
    struct calculate_next
    {
        template <typename T, typename Stride>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr static T call(
            T t1, Stride offset)
        {
            return T(t1 + offset);
        }

        template <typename T, typename Stride>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr static T call(
            T t, std::size_t max_count, Stride& offset)
        {
            if constexpr (std::is_signed_v<Stride>)
            {
                if (is_negative(offset))
                {
                    offset = Stride(negate(
                        // NVCC seems to have a bug with std::min...
                        max_count < std::size_t(negate(offset)) ?
                            max_count :
                            negate(offset)));
                    return T(t + offset);
                }

                // NVCC seems to have a bug with std::min...
                offset = Stride(
                    max_count < std::size_t(offset) ? max_count : offset);
                return T(t + offset);
            }
            else
            {
                // NVCC seems to have a bug with std::min...
                offset = max_count < offset ? max_count : offset;
                return T(t + offset);
            }
        }
    };

    template <typename Iterable>
    struct calculate_next<Iterable,
        std::enable_if_t<hpx::traits::is_iterator_v<Iterable> &&
            !std::bidirectional_iterator<Iterable>>>
    {
        template <typename Iter, typename Stride>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr static Iter call(
            Iter iter, Stride offset)
        {
#if (defined(HPX_HAVE_CUDA) && defined(__CUDACC__)) || defined(HPX_HAVE_HIP)
            hpx::parallel::detail::advance(iter, offset);
#else
            std::advance(iter, offset);
#endif
            return iter;
        }

        template <typename Iter, typename Stride>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr static Iter call(
            Iter iter, std::size_t max_count, Stride& offset)
        {
            // anything less than a bidirectional iterator does not support
            // negative offsets
            HPX_ASSERT(!std::is_signed_v<Stride> || !is_negative(offset));

            // NVCC seems to have a bug with std::min...
            offset =
                Stride(max_count < std::size_t(offset) ? max_count : offset);

            // advance through the end or max number of elements
            for (Stride count = offset; count != 0; (void) ++iter, --count)
                /**/;

            return iter;
        }
    };

    template <typename Iterable>
    struct calculate_next<Iterable,
        std::enable_if_t<std::bidirectional_iterator<Iterable>>>
    {
        template <typename Iter, typename Stride>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr static Iter call(
            Iter iter, Stride offset)
        {
#if (defined(HPX_HAVE_CUDA) && defined(__CUDACC__)) || defined(HPX_HAVE_HIP)
            hpx::parallel::detail::advance(iter, offset);
#else
            std::advance(iter, offset);
#endif
            return iter;
        }

        template <typename Iter, typename Stride>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr static Iter call(
            Iter iter, std::size_t max_count, Stride& offset)
        {
            if constexpr (std::is_signed_v<Stride>)
            {
                // advance through the end or max number of elements
                if (!is_negative(offset))
                {
                    // NVCC seems to have a bug with std::min...
                    offset = Stride(
                        max_count < std::size_t(offset) ? max_count : offset);
#if (defined(HPX_HAVE_CUDA) && defined(__CUDACC__)) || defined(HPX_HAVE_HIP)
                    hpx::parallel::detail::advance(iter, offset);
#else
                    std::advance(iter, offset);
#endif
                }
                else
                {
                    offset = negate(Stride(
                        // NVCC seems to have a bug with std::min...
                        max_count < negate(offset) ? max_count :
                                                     negate(offset)));
#if (defined(HPX_HAVE_CUDA) && defined(__CUDACC__)) || defined(HPX_HAVE_HIP)
                    hpx::parallel::detail::advance(iter, offset);
#else
                    std::advance(iter, offset);
#endif
                }
            }
            else
            {
                // advance through the end or max number of elements
                // NVCC seems to have a bug with std::min...
                offset = Stride(
                    max_count < std::size_t(offset) ? max_count : offset);
#if (defined(HPX_HAVE_CUDA) && defined(__CUDACC__)) || defined(HPX_HAVE_HIP)
                hpx::parallel::detail::advance(iter, offset);
#else
                std::advance(iter, offset);
#endif
            }
            return iter;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Iterable, typename Stride>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iterable next(
        Iterable iter, Stride offset)
    {
        return calculate_next<std::decay_t<Iterable>>::call(iter, offset);
    }

    HPX_CXX_CORE_EXPORT template <typename Iterable, typename Stride>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iterable next(
        Iterable iter, std::size_t max_count, Stride offset)
    {
        return calculate_next<std::decay_t<Iterable>>::call(
            iter, max_count, offset);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct equal_to
    {
        template <typename T1, typename T2,
            typename Enable = std::enable_if_t<
                hpx::traits::is_equality_comparable_with_v<T1, T2>>>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
            T1&& t1, T2&& t2) const
            noexcept(noexcept(std::declval<T1>() == std::declval<T2>()))
        {
            return t1 == t2;
        }
    };

    HPX_CXX_CORE_EXPORT struct not_equal_to
    {
        template <typename T1, typename T2,
            typename Enable = std::enable_if_t<
                hpx::traits::is_equality_comparable_with_v<T1, T2>>>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
            T1&& t1, T2&& t2) const
            noexcept(noexcept(std::declval<T1>() != std::declval<T2>()))
        {
            return t1 != t2;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Value>
    struct compare_to
    {
        HPX_HOST_DEVICE HPX_FORCEINLINE explicit compare_to(
            Value&& val) noexcept
          : value_(HPX_MOVE(val))
        {
        }
        HPX_HOST_DEVICE HPX_FORCEINLINE explicit compare_to(Value const& val)
          : value_(val)
        {
        }

        template <typename T>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
            T const& t) const
            noexcept(noexcept(std::declval<Value>() == std::declval<T>()))
                -> decltype(std::declval<Value>() == t)
        {
            return value_ == t;
        }

        Value value_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct less
    {
        template <typename T1, typename T2>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
            T1&& t1, T2&& t2) const
            noexcept(noexcept(std::declval<T1>() < std::declval<T2>()))
        {
            return HPX_FORWARD(T1, t1) < HPX_FORWARD(T2, t2);
        }
    };

    HPX_CXX_CORE_EXPORT struct greater
    {
        template <typename T1, typename T2>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
            T1&& t1, T2&& t2) const
            noexcept(noexcept(std::declval<T1>() > std::declval<T2>()))
        {
            return HPX_FORWARD(T1, t1) > HPX_FORWARD(T2, t2);
        }
    };

    HPX_CXX_CORE_EXPORT struct greater_equal
    {
        template <typename T1, typename T2>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
            T1&& t1, T2&& t2) const
            noexcept(noexcept(std::declval<T1>() >= std::declval<T2>()))
        {
            return HPX_FORWARD(T1, t1) >= HPX_FORWARD(T2, t2);
        }
    };

    HPX_CXX_CORE_EXPORT struct less_equal
    {
        template <typename T1, typename T2>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
            T1&& t1, T2&& t2) const
            noexcept(noexcept(std::declval<T1>() <= std::declval<T2>()))
        {
            return HPX_FORWARD(T1, t1) <= HPX_FORWARD(T2, t2);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct min_of
    {
        constexpr T operator()(T const& t1, T const& t2) const
            noexcept(noexcept(std::declval<T>() < std::declval<T>()))
        {
            // NVCC seems to have a bug with std::min...
            return t1 < t2 ? t1 : t2;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    struct max_of
    {
        constexpr T operator()(T const& t1, T const& t2) const
            noexcept(noexcept(std::declval<T>() > std::declval<T>()))
        {
            return (std::max) (t1, t2);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct plus
    {
        template <typename T1, typename T2>
        constexpr auto operator()(T1 const& t1, T2 const& t2) const
            noexcept(noexcept(std::declval<T1>() + std::declval<T2>()))
                -> decltype(t1 + t2)
        {
            return t1 + t2;
        }
    };

    HPX_CXX_CORE_EXPORT struct minus
    {
        template <typename T1, typename T2>
        constexpr auto operator()(T1 const& t1, T2 const& t2) const
            noexcept(noexcept(std::declval<T1>() - std::declval<T2>()))
                -> decltype(t1 - t2)
        {
            return t1 - t2;
        }
    };

    HPX_CXX_CORE_EXPORT struct multiplies
    {
        template <typename T1, typename T2>
        constexpr auto operator()(T1 const& t1, T2 const& t2) const
            noexcept(noexcept(std::declval<T1>() * std::declval<T2>()))
                -> decltype(t1 * t2)
        {
            return t1 * t2;
        }
    };

    HPX_CXX_CORE_EXPORT struct divides
    {
        template <typename T1, typename T2>
        constexpr auto operator()(T1 const& t1, T2 const& t2) const
            noexcept(noexcept(std::declval<T1>() / std::declval<T2>()))
                -> decltype(t1 / t2)
        {
            return t1 / t2;
        }
    };
}    // namespace hpx::parallel::detail

namespace hpx::ranges {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT using equal_to = hpx::parallel::detail::equal_to;
    HPX_CXX_CORE_EXPORT using not_equal_to =
        hpx::parallel::detail::not_equal_to;
    HPX_CXX_CORE_EXPORT using less = hpx::parallel::detail::less;
    HPX_CXX_CORE_EXPORT using greater = hpx::parallel::detail::greater;
    HPX_CXX_CORE_EXPORT using greater_equal =
        hpx::parallel::detail::greater_equal;
    HPX_CXX_CORE_EXPORT using less_equal = hpx::parallel::detail::less_equal;
}    // namespace hpx::ranges
