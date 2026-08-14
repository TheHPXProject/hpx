//  Copyright (c) 2022 A Kishore Kumar
//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::util::detail {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_VECTOR_REDUCTION)
    HPX_CXX_CORE_EXPORT template <typename T, typename Operation,
        template <typename = void> typename Op>
    inline constexpr bool is_operation_v =
        std::is_same_v<Operation, Op<T>> || std::is_same_v<Operation, Op<>>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_arithmetic_plus_reduction_v =
        std::is_arithmetic_v<T> && is_operation_v<T, Operation, std::plus>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_arithmetic_minus_reduction_v =
        std::is_arithmetic_v<T> && is_operation_v<T, Operation, std::minus>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_arithmetic_multiplies_reduction_v =
        std::is_arithmetic_v<T> &&
        is_operation_v<T, Operation, std::multiplies>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_arithmetic_bit_and_reduction_v =
        std::is_arithmetic_v<T> && is_operation_v<T, Operation, std::bit_and>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_arithmetic_bit_or_reduction_v =
        std::is_arithmetic_v<T> && is_operation_v<T, Operation, std::bit_or>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_arithmetic_bit_xor_reduction_v =
        std::is_arithmetic_v<T> && is_operation_v<T, Operation, std::bit_xor>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_arithmetic_logical_and_reduction_v =
        std::is_arithmetic_v<T> &&
        is_operation_v<T, Operation, std::logical_and>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_arithmetic_logical_or_reduction_v =
        std::is_arithmetic_v<T> &&
        is_operation_v<T, Operation, std::logical_or>;

    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_not_omp_reduction_v =
        !is_arithmetic_plus_reduction_v<T, Operation> &&
        !is_arithmetic_minus_reduction_v<T, Operation> &&
        !is_arithmetic_multiplies_reduction_v<T, Operation> &&
        !is_arithmetic_bit_and_reduction_v<T, Operation> &&
        !is_arithmetic_bit_or_reduction_v<T, Operation> &&
        !is_arithmetic_bit_xor_reduction_v<T, Operation> &&
        !is_arithmetic_logical_and_reduction_v<T, Operation> &&
        !is_arithmetic_logical_or_reduction_v<T, Operation>;
#else
    HPX_CXX_CORE_EXPORT template <typename T, typename Operation>
    inline constexpr bool is_not_omp_reduction_v = true;
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Will only be called when the iterators are all random access
    HPX_CXX_CORE_EXPORT struct unseq_reduce_n
    {
#if defined(HPX_HAVE_VECTOR_REDUCTION)
        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_plus_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce /* */, Convert conv)
        {
            HPX_VECTOR_REDUCTION(+ : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init += HPX_INVOKE(conv, *it);
                ++it;
            }
            return init;
        }

        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_minus_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce /* */, Convert conv)
        {
            HPX_VECTOR_REDUCTION(- : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init -= HPX_INVOKE(conv, *it);
                ++it;
            }
            return init;
        }

        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_multiplies_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce /* */, Convert conv)
        {
            HPX_VECTOR_REDUCTION(* : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init *= HPX_INVOKE(conv, *it);
                ++it;
            }
            return init;
        }

        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_bit_and_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce /* */, Convert conv)
        {
            HPX_VECTOR_REDUCTION(& : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init &= HPX_INVOKE(conv, *it);
                ++it;
            }
            return init;
        }

        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_bit_or_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce /* */, Convert conv)
        {
            HPX_VECTOR_REDUCTION(| : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init |= HPX_INVOKE(conv, *it);
                ++it;
            }
            return init;
        }

        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_bit_xor_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce /* */, Convert conv)
        {
            HPX_VECTOR_REDUCTION(^ : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init ^= HPX_INVOKE(conv, *it);
                ++it;
            }
            return init;
        }

        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_logical_and_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce /* */, Convert conv)
        {
            HPX_VECTOR_REDUCTION(&& : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init = HPX_INVOKE(conv, *it) && init;
                ++it;
            }
            return init;
        }

        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_logical_or_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce /* */, Convert conv)
        {
            HPX_VECTOR_REDUCTION(|| : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init = HPX_INVOKE(conv, *it) || init;
                ++it;
            }
            return init;
        }
#endif
        template <typename Iter1, typename T, typename Convert, typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_not_omp_reduction_v<T, Reduce>, T>
        reduce(Iter1 it, std::size_t count, T init, Reduce r, Convert conv)
        {
            constexpr std::size_t block_size = HPX_LANE_SIZE / sizeof(T);

            // To small, just run sequential
            if (count <= 2 * block_size)
            {
                for (std::size_t i = 0; i != count; ++i)
                {
                    init = HPX_INVOKE(r, init, HPX_INVOKE(conv, *it));
                    ++it;
                }
            }
            else
            {
                alignas(HPX_LANE_SIZE) std::uint8_t block[HPX_LANE_SIZE] = {};
                T* tblock = reinterpret_cast<T*>(block);

                // Initialize block[i] = r(f(2*i), f(2*i + 1))
                for (std::size_t i = 0; i != 2 * block_size; i += 2)
                {
                    hpx::construct_at(tblock + i / 2,
                        HPX_INVOKE(r, HPX_INVOKE(conv, *it),
                            HPX_INVOKE(conv, *(it + 1))));
                    it += 2;
                }

                // Vectorized loop
                std::size_t const limit = block_size * (count / block_size);
                for (std::size_t i = 2 * block_size; i != limit;
                    i += block_size)
                {
                    HPX_VECTORIZE
                    for (std::size_t j = 0; j != block_size; ++j)
                    {
                        tblock[j] = HPX_INVOKE(
                            r, tblock[j], HPX_INVOKE(conv, *(it + j)));
                    }
                    it += block_size;
                }

                // Remainder
                count -= limit;

                HPX_VECTORIZE
                for (std::size_t i = 0; i != count; ++i)
                {
                    tblock[i] = HPX_INVOKE(r, tblock[i], HPX_INVOKE(conv, *it));
                    ++it;
                }

                // Merge
                for (std::size_t i = 0; i != block_size; ++i)
                {
                    init = HPX_INVOKE(r, init, tblock[i]);
                }

                // Cleanup resources
                HPX_VECTORIZE
                for (std::size_t i = 0; i < block_size; ++i)
                {
                    std::destroy_at(std::addressof(tblock[i]));
                }
            }
            return init;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Will only be called when the iterators are all random access
    HPX_CXX_CORE_EXPORT struct unseq_binary_reduce_n
    {
#if defined(HPX_HAVE_VECTOR_REDUCTION)
        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_plus_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce /* */,
            Convert conv)
        {
            HPX_VECTOR_REDUCTION(+ : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init += HPX_INVOKE(conv, *it1, *it2);
                ++it1, ++it2;
            }
            return init;
        }

        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_minus_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce /* */,
            Convert conv)
        {
            HPX_VECTOR_REDUCTION(- : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init -= HPX_INVOKE(conv, *it1, *it2);
                ++it1, ++it2;
            }
            return init;
        }

        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_multiplies_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce /* */,
            Convert conv)
        {
            HPX_VECTOR_REDUCTION(* : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init *= HPX_INVOKE(conv, *it1, *it2);
                ++it1, ++it2;
            }
            return init;
        }

        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_bit_and_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce /* */,
            Convert conv)
        {
            HPX_VECTOR_REDUCTION(& : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init &= HPX_INVOKE(conv, *it1, *it2);
                ++it1, ++it2;
            }
            return init;
        }

        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_bit_or_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce /* */,
            Convert conv)
        {
            HPX_VECTOR_REDUCTION(| : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init |= HPX_INVOKE(conv, *it1, *it2);
                ++it1, ++it2;
            }
            return init;
        }

        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_bit_xor_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce /* */,
            Convert conv)
        {
            HPX_VECTOR_REDUCTION(^ : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init ^= HPX_INVOKE(conv, *it1, *it2);
                ++it1, ++it2;
            }
            return init;
        }

        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_logical_and_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce /* */,
            Convert conv)
        {
            HPX_VECTOR_REDUCTION(&& : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init = HPX_INVOKE(conv, *it1, *it2) && init;
                ++it1, ++it2;
            }
            return init;
        }

        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_arithmetic_logical_or_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce /* */,
            Convert conv)
        {
            HPX_VECTOR_REDUCTION(|| : init)
            for (std::size_t i = 0; i != count; ++i)
            {
                init = HPX_INVOKE(conv, *it1, *it2) || init;
                ++it1, ++it2;
            }
            return init;
        }
#endif
        template <typename Iter1, typename Iter2, typename T, typename Convert,
            typename Reduce>
        HPX_HOST_DEVICE HPX_FORCEINLINE static std::enable_if_t<
            is_not_omp_reduction_v<T, Reduce>, T>
        reduce(Iter1 it1, Iter2 it2, std::size_t count, T init, Reduce r,
            Convert conv)
        {
            constexpr std::size_t block_size =
                (std::max) (HPX_LANE_SIZE / (sizeof(T) * 8),
                    static_cast<std::size_t>(1));

            // To small, just run sequential
            if (count <= 2 * block_size)
            {
                for (std::size_t i = 0; i != count; ++i)
                {
                    init = HPX_INVOKE(r, init, HPX_INVOKE(conv, *it1, *it2));
                    ++it1, ++it2;
                }
            }
            else
            {
                alignas(HPX_LANE_SIZE) std::uint8_t block[HPX_LANE_SIZE] = {};
                T* tblock = reinterpret_cast<T*>(block);

                // Initialize block[i] = r(f(2*i), f(2*i + 1))
                for (std::size_t i = 0; i != block_size; ++i)
                {
                    hpx::construct_at(tblock + i,
                        HPX_INVOKE(r, HPX_INVOKE(conv, *it1, *it2),
                            HPX_INVOKE(conv, *(it1 + 1), *(it2 + 1))));

                    it1 += 2;
                    it2 += 2;
                }

                // Vectorized loop
                std::size_t const limit = block_size * (count / block_size);
                for (std::size_t i = 2 * block_size; i != limit;
                    i += block_size)
                {
                    HPX_VECTORIZE
                    for (std::size_t j = 0; j != block_size; ++j)
                    {
                        tblock[j] = HPX_INVOKE(r, tblock[j],
                            HPX_INVOKE(conv, *(it1 + j), *(it2 + j)));
                    }
                    it1 += block_size;
                    it2 += block_size;
                }

                // Remainder
                count -= limit;

                HPX_VECTORIZE
                for (std::size_t i = 0; i != count; ++i)
                {
                    tblock[i] =
                        HPX_INVOKE(r, tblock[i], HPX_INVOKE(conv, *it1, *it2));
                    ++it1, ++it2;
                }

                // Merge
                for (std::size_t i = 0; i != block_size; ++i)
                {
                    init = HPX_INVOKE(r, init, tblock[i]);
                }

                // Cleanup resources
                HPX_VECTORIZE
                for (std::size_t i = 0; i != block_size; ++i)
                {
                    std::destroy_at(std::addressof(tblock[i]));
                }
            }
            return init;
        }
    };
}    // namespace hpx::parallel::util::detail
