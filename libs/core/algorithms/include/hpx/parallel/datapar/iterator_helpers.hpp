//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::util::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_lvalue_ref : std::is_lvalue_reference<T>
    {
    };

    template <typename... Ts>
    struct is_lvalue_ref<hpx::tuple<Ts...>>
      : hpx::util::all_of<std::is_lvalue_reference<Ts>...>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename It>
    inline constexpr bool dereference_is_lvalue_ref_v =
        is_lvalue_ref<decltype(*std::declval<It&>())>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter>
    struct is_data_aligned_impl
    {
        static HPX_FORCEINLINE bool call(Iter& it) noexcept
        {
            using value_type =
                std::iterator_traits<std::remove_const_t<Iter>>::value_type;
            using pack_type = traits::vector_pack_type_t<value_type>;

            return (reinterpret_cast<std::uintptr_t>(std::addressof(*it)) &
                       (traits::vector_pack_alignment<pack_type>::value - 1)) ==
                0;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Iter>
    HPX_FORCEINLINE bool is_data_aligned(Iter& it) noexcept
    {
        return is_data_aligned_impl<Iter>::call(it);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
    struct iterators_datapar_compatible_impl
    {
        using iterator1_type = std::decay_t<Iter1>;
        using iterator2_type = std::decay_t<Iter2>;

        using value1_type = std::iterator_traits<iterator1_type>::value_type;
        using value2_type = std::iterator_traits<iterator2_type>::value_type;

        using pack1_type = traits::vector_pack_type_t<value1_type>;
        using pack2_type = traits::vector_pack_type_t<value2_type>;

        using type = std::integral_constant<bool,
            traits::vector_pack_size_v<pack1_type> ==
                    traits::vector_pack_size_v<pack2_type> &&
                traits::vector_pack_alignment_v<pack1_type> ==
                    traits::vector_pack_alignment_v<pack2_type>>;
    };

    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
    struct iterators_datapar_compatible
      : iterators_datapar_compatible_impl<Iter1, Iter2>::type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
    inline constexpr bool iterators_datapar_compatible_v =
        iterators_datapar_compatible<Iter1, Iter2>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Enable = void>
    struct iterator_datapar_compatible_impl
      : std::is_arithmetic<typename std::iterator_traits<Iter>::value_type>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Iter, typename Enable = void>
    struct iterator_datapar_compatible : std::false_type
    {
    };

    template <typename Iter>
    struct iterator_datapar_compatible<Iter,
        std::enable_if_t<std::random_access_iterator<Iter>>>
      : iterator_datapar_compatible_impl<std::decay_t<Iter>>::type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Iter>
    inline constexpr bool iterator_datapar_compatible_v =
        iterator_datapar_compatible<Iter>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter, bool IsConst = false,
        typename Enable = void>
    struct datapar_loop_step
    {
        using value_type = std::iterator_traits<Iter>::value_type;

        using V1 = traits::vector_pack_type_t<value_type, 1>;
        using V = traits::vector_pack_type_t<value_type>;

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(
            F&& f, Iter& it)
        {
            V1 tmp(traits::vector_pack_load<V1, value_type>::unaligned(it));
            if constexpr (IsConst)
            {
                HPX_INVOKE(f, static_cast<V1 const*>(&tmp));
            }
            else
            {
                HPX_INVOKE(f, &tmp);
                if constexpr (dereference_is_lvalue_ref_v<Iter>)
                {
                    traits::vector_pack_store<V1, value_type>::unaligned(
                        tmp, it);
                }
            }
            ++it;
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void callv(
            F&& f, Iter& it)
        {
            V tmp(traits::vector_pack_load<V, value_type>::aligned(it));
            if constexpr (IsConst)
            {
                HPX_INVOKE(f, static_cast<V const*>(&tmp));
            }
            else
            {
                HPX_INVOKE(f, &tmp);
                if constexpr (dereference_is_lvalue_ref_v<Iter>)
                {
                    traits::vector_pack_store<V, value_type>::aligned(tmp, it);
                }
            }
            std::advance(it, traits::vector_pack_size_v<V>);
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void calls(
            F&& f, Iter& it)
        {
            constexpr auto size = traits::vector_pack_size_v<V>;

            Iter it1 = it;

            V tmp;
            for (std::size_t e = 0; e != size; (void) ++it1, ++e)
                traits::set(tmp, e, *it1);

            if constexpr (IsConst)
            {
                HPX_INVOKE(f, static_cast<V const*>(&tmp));
                std::advance(it, size);
            }
            else
            {
                HPX_INVOKE(f, &tmp);

                if constexpr (dereference_is_lvalue_ref_v<Iter>)
                {
                    for (std::size_t e = 0; e != size; (void) ++it, ++e)
                        *it = traits::get(tmp, e);
                }
                else
                {
                    std::advance(it, size);
                }
            }
        }
    };

    template <typename I, bool IsConst>
    struct datapar_loop_step<I, IsConst,
        std::enable_if_t<std::is_integral_v<I>>>
    {
        using V1 = traits::vector_pack_type_t<I, 1>;
        using V = traits::vector_pack_type_t<I>;

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(F&& f, I& i)
        {
            V1 tmp(i);
            HPX_INVOKE(f, tmp);
            ++i;
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void callv(F&& f, I& i)
        {
            constexpr auto size = traits::vector_pack_size_v<V>;
            V tmp;
            for (std::size_t e = 0; e != size; ++e)
                traits::set(tmp, e, static_cast<I>(i + e));
            HPX_INVOKE(f, tmp);
            i += size;
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void calls(F&& f, I& i)
        {
            callv(HPX_FORWARD(F, f), i);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Enable = void>
    struct datapar_loop_pred_step
    {
        using value_type = std::iterator_traits<Iter>::value_type;

        using V1 = traits::vector_pack_type_t<value_type, 1>;
        using V = traits::vector_pack_type_t<value_type>;

        // Return -1 if the element does not satisfy predicate.
        // Return 0 if predicate satisfies.
        // Note 0 is treated as index since call1() is on scalars,
        // the first element satisfying the predicate would be 0.
        template <typename Pred>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr int call1(
            Pred&& pred, Iter& it)
        {
            V1 tmp(traits::vector_pack_load<V1, value_type>::unaligned(it));
            int const idx = HPX_INVOKE(pred, &tmp);
            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                traits::vector_pack_store<V1, value_type>::unaligned(tmp, it);
            }
            return idx;
        }

        // Return -1 if no element of the vector register satisfies predicate.
        // Returns index to the first element that satisfies the predicate.
        template <typename Pred>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr int callv(
            Pred&& pred, Iter& it)
        {
            V tmp(traits::vector_pack_load<V, value_type>::aligned(it));
            int const idx = HPX_INVOKE(pred, &tmp);
            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                traits::vector_pack_store<V, value_type>::aligned(tmp, it);
            }
            return idx;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Enable = void>
    struct datapar_loop_step_ind
    {
        using value_type = std::iterator_traits<Iter>::value_type;

        using V1 = traits::vector_pack_type_t<value_type, 1>;
        using V = traits::vector_pack_type_t<value_type>;

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(
            F&& f, Iter& it)
        {
            V1 tmp(traits::vector_pack_load<V1, value_type>::unaligned(it));
            HPX_INVOKE(f, tmp);
            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                traits::vector_pack_store<V1, value_type>::unaligned(tmp, it);
            }
            ++it;
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void callv(
            F&& f, Iter& it)
        {
            V tmp(traits::vector_pack_load<V, value_type>::aligned(it));
            HPX_INVOKE(f, tmp);
            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                traits::vector_pack_store<V, value_type>::aligned(tmp, it);
            }
            std::advance(it, traits::vector_pack_size_v<V>);
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void calls(
            F&& f, Iter& it)
        {
            constexpr auto size = traits::vector_pack_size_v<V>;

            Iter it1 = it;

            V tmp;
            for (std::size_t e = 0; e != size; (void) ++it1, ++e)
                traits::set(tmp, e, *it1);

            HPX_INVOKE(f, tmp);

            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                for (std::size_t e = 0; e != size; (void) ++it, ++e)
                    *it = traits::get(tmp, e);
            }
            else
            {
                std::advance(it, size);
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter>
    struct datapar_loop_idx_step
    {
        using value_type = std::iterator_traits<Iter>::value_type;

        using V1 = traits::vector_pack_type_t<value_type, 1>;
        using V = traits::vector_pack_type_t<value_type>;

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(
            F&& f, Iter& it, std::size_t base_idx)
        {
            V1 tmp(traits::vector_pack_load<V1, value_type>::unaligned(it));
            HPX_INVOKE(f, tmp, base_idx);
            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                traits::vector_pack_store<V1, value_type>::unaligned(tmp, it);
            }
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void callv(
            F&& f, Iter& it, std::size_t base_idx)
        {
            V tmp(traits::vector_pack_load<V, value_type>::aligned(it));
            HPX_INVOKE(f, tmp, base_idx);
            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                traits::vector_pack_store<V, value_type>::aligned(tmp, it);
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Enable = void>
    struct datapar_loop_step_tok
    {
        using value_type = std::iterator_traits<Iter>::value_type;

        using V1 = traits::vector_pack_type_t<value_type, 1>;
        using V = traits::vector_pack_type_t<value_type>;

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(
            F&& f, Iter& it)
        {
            V1 tmp(traits::vector_pack_load<V1, value_type>::unaligned(it));
            HPX_INVOKE(f, &tmp);
            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                traits::vector_pack_store<V1, value_type>::unaligned(tmp, it);
            }
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr std::size_t callv(
            F&& f, Iter& it)
        {
            V tmp(traits::vector_pack_load<V, value_type>::aligned(it));
            HPX_INVOKE(f, &tmp);
            if constexpr (dereference_is_lvalue_ref_v<Iter>)
            {
                traits::vector_pack_store<V, value_type>::aligned(tmp, it);
            }
            return traits::vector_pack_size_v<V>;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename V1, typename V2>
    struct invoke_vectorized_in2
    {
        template <typename F, typename Iter1, typename Iter2>
        static constexpr hpx::util::invoke_result_t<F, V1*, V2*> call_aligned(
            F&& f, Iter1& it1, Iter2& it2)
        {
            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            using value_type1 = std::iterator_traits<Iter1>::value_type;
            using value_type2 = std::iterator_traits<Iter2>::value_type;

            V1 tmp1(traits::vector_pack_load<V1, value_type1>::aligned(it1));
            V2 tmp2(traits::vector_pack_load<V2, value_type2>::aligned(it2));

            std::advance(it1, traits::vector_pack_size_v<V1>);
            std::advance(it2, traits::vector_pack_size_v<V2>);

            return HPX_INVOKE(HPX_FORWARD(F, f), &tmp1, &tmp2);
        }

        template <typename F, typename Iter1, typename Iter2>
        static constexpr hpx::util::invoke_result_t<F, V1*, V2*> call_unaligned(
            F&& f, Iter1& it1, Iter2& it2)
        {
            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            using value_type1 = std::iterator_traits<Iter1>::value_type;
            using value_type2 = std::iterator_traits<Iter2>::value_type;

            V1 tmp1(traits::vector_pack_load<V1, value_type1>::unaligned(it1));
            V2 tmp2(traits::vector_pack_load<V2, value_type2>::unaligned(it2));

            std::advance(it1, traits::vector_pack_size_v<V1>);
            std::advance(it2, traits::vector_pack_size_v<V2>);

            return HPX_INVOKE(HPX_FORWARD(F, f), &tmp1, &tmp2);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename V1, typename V2>
    struct invoke_vectorized_in2_ind
    {
        template <typename F, typename Iter1, typename Iter2>
        static constexpr auto call_aligned(F&& f, Iter1& it1, Iter2& it2)
        {
            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            using value_type1 = std::iterator_traits<Iter1>::value_type;
            using value_type2 = std::iterator_traits<Iter2>::value_type;

            V1 tmp1(traits::vector_pack_load<V1, value_type1>::aligned(it1));
            V2 tmp2(traits::vector_pack_load<V2, value_type2>::aligned(it2));

            std::advance(it1, traits::vector_pack_size_v<V1>);
            std::advance(it2, traits::vector_pack_size_v<V2>);

            return HPX_INVOKE(HPX_FORWARD(F, f), tmp1, tmp2);
        }

        template <typename F, typename Iter1, typename Iter2>
        static constexpr auto call_unaligned(F&& f, Iter1& it1, Iter2& it2)
        {
            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            using value_type1 = std::iterator_traits<Iter1>::value_type;
            using value_type2 = std::iterator_traits<Iter2>::value_type;

            V1 tmp1(traits::vector_pack_load<V1, value_type1>::unaligned(it1));
            V2 tmp2(traits::vector_pack_load<V2, value_type2>::unaligned(it2));

            std::advance(it1, traits::vector_pack_size_v<V1>);
            std::advance(it2, traits::vector_pack_size_v<V2>);

            return HPX_INVOKE(HPX_FORWARD(F, f), tmp1, tmp2);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
    struct datapar_loop_step2
    {
        using value1_type = std::iterator_traits<Iter1>::value_type;
        using value2_type = std::iterator_traits<Iter2>::value_type;

        using V11 = traits::vector_pack_type_t<value1_type, 1>;
        using V12 = traits::vector_pack_type_t<value2_type, 1>;

        using V1 = traits::vector_pack_type_t<value1_type>;
        using V2 = traits::vector_pack_type_t<value2_type>;

        template <typename F>
        HPX_HOST_DEVICE
            HPX_FORCEINLINE static constexpr hpx::util::invoke_result<F, V11*,
                V12*>
            call1(F&& f, Iter1& it1, Iter2& it2)
        {
            return invoke_vectorized_in2<V11, V12>::call_unaligned(
                HPX_FORWARD(F, f), it1, it2);
        }

        template <typename F>
        HPX_HOST_DEVICE
            HPX_FORCEINLINE static constexpr hpx::util::invoke_result<F, V1*,
                V2*>
            callv(F&& f, Iter1& it1, Iter2& it2)
        {
            HPX_ASSERT(is_data_aligned(it1) && is_data_aligned(it2));
            return invoke_vectorized_in2<V1, V2>::call_aligned(
                HPX_FORWARD(F, f), it1, it2);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Iter1, typename Iter2>
    struct datapar_loop_step2_ind
    {
        using value1_type = std::iterator_traits<Iter1>::value_type;
        using value2_type = std::iterator_traits<Iter2>::value_type;

        using V11 = traits::vector_pack_type_t<value1_type, 1>;
        using V12 = traits::vector_pack_type_t<value2_type, 1>;

        using V1 = traits::vector_pack_type_t<value1_type>;
        using V2 = traits::vector_pack_type_t<value2_type>;

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr auto call1(
            F&& f, Iter1& it1, Iter2& it2)
        {
            return invoke_vectorized_in2_ind<V11, V12>::call_unaligned(
                HPX_FORWARD(F, f), it1, it2);
        }

        template <typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr auto callv(
            F&& f, Iter1& it1, Iter2& it2)
        {
            return invoke_vectorized_in2_ind<V1, V2>::call_aligned(
                HPX_FORWARD(F, f), it1, it2);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename V>
    struct invoke_vectorized_inout1
    {
        template <typename F, typename InIter, typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call_aligned(
            F&& f, InIter& it, OutIter& dest)
        {
            using value_type = std::iterator_traits<InIter>::value_type;

            V tmp(traits::vector_pack_load<V, value_type>::aligned(it));

            auto ret = HPX_INVOKE(f, &tmp);
            if constexpr (dereference_is_lvalue_ref_v<OutIter>)
            {
                traits::vector_pack_store<decltype(ret), value_type>::aligned(
                    ret, dest);
            }
            std::advance(it, traits::vector_pack_size_v<V>);
            std::advance(dest, traits::vector_pack_size_v<decltype(ret)>);
        }

        template <typename F, typename InIter, typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call_unaligned(
            F&& f, InIter& it, OutIter& dest)
        {
            using value_type = std::iterator_traits<InIter>::value_type;

            V tmp(traits::vector_pack_load<V, value_type>::unaligned(it));

            auto ret = HPX_INVOKE(f, &tmp);
            if constexpr (dereference_is_lvalue_ref_v<OutIter>)
            {
                traits::vector_pack_store<decltype(ret), value_type>::unaligned(
                    ret, dest);
            }
            std::advance(it, traits::vector_pack_size_v<V>);
            std::advance(dest, traits::vector_pack_size_v<decltype(ret)>);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename V>
    struct invoke_vectorized_inout1_ind
    {
        template <typename F, typename InIter, typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call_aligned(
            F&& f, InIter& it, OutIter& dest)
        {
            using value_type = std::iterator_traits<InIter>::value_type;

            V tmp(traits::vector_pack_load<V, value_type>::aligned(it));

            auto ret = HPX_INVOKE(f, tmp);
            if constexpr (dereference_is_lvalue_ref_v<OutIter>)
            {
                traits::vector_pack_store<decltype(ret), value_type>::aligned(
                    ret, dest);
            }
            std::advance(it, traits::vector_pack_size_v<V>);
            std::advance(dest, traits::vector_pack_size_v<decltype(ret)>);
        }

        template <typename F, typename InIter, typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call_unaligned(
            F&& f, InIter& it, OutIter& dest)
        {
            using value_type = std::iterator_traits<InIter>::value_type;

            V tmp(traits::vector_pack_load<V, value_type>::unaligned(it));

            auto ret = HPX_INVOKE(f, tmp);
            if constexpr (dereference_is_lvalue_ref_v<OutIter>)
            {
                traits::vector_pack_store<decltype(ret), value_type>::unaligned(
                    ret, dest);
            }
            std::advance(it, traits::vector_pack_size_v<V>);
            std::advance(dest, traits::vector_pack_size_v<decltype(ret)>);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename V1, typename V2>
    struct invoke_vectorized_inout2
    {
        template <typename F, typename InIter1, typename InIter2,
            typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call_aligned(
            F&& f, InIter1& it1, InIter2& it2, OutIter& dest)
        {
            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            using value_type1 = std::iterator_traits<InIter1>::value_type;
            using value_type2 = std::iterator_traits<InIter2>::value_type;

            V1 tmp1(traits::vector_pack_load<V1, value_type1>::aligned(it1));
            V2 tmp2(traits::vector_pack_load<V2, value_type2>::aligned(it2));

            auto ret = HPX_INVOKE(f, &tmp1, &tmp2);
            if constexpr (dereference_is_lvalue_ref_v<OutIter>)
            {
                traits::vector_pack_store<decltype(ret), value_type1>::aligned(
                    ret, dest);
            }
            std::advance(it1, traits::vector_pack_size_v<V1>);
            std::advance(it2, traits::vector_pack_size_v<V2>);
            std::advance(dest, traits::vector_pack_size_v<decltype(ret)>);
        }

        template <typename F, typename InIter1, typename InIter2,
            typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call_unaligned(
            F&& f, InIter1& it1, InIter2& it2, OutIter& dest)
        {
            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            using value_type1 = std::iterator_traits<InIter1>::value_type;
            using value_type2 = std::iterator_traits<InIter2>::value_type;

            V1 tmp1(traits::vector_pack_load<V1, value_type1>::unaligned(it1));
            V2 tmp2(traits::vector_pack_load<V2, value_type2>::unaligned(it2));

            auto ret = HPX_INVOKE(f, &tmp1, &tmp2);
            if constexpr (dereference_is_lvalue_ref_v<OutIter>)
            {
                traits::vector_pack_store<decltype(ret),
                    value_type1>::unaligned(ret, dest);
            }
            std::advance(it1, traits::vector_pack_size_v<V1>);
            std::advance(it2, traits::vector_pack_size_v<V2>);
            std::advance(dest, traits::vector_pack_size_v<decltype(ret)>);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename V1, typename V2>
    struct invoke_vectorized_inout2_ind
    {
        template <typename F, typename InIter1, typename InIter2,
            typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call_aligned(
            F&& f, InIter1& it1, InIter2& it2, OutIter& dest)
        {
            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            using value_type1 = std::iterator_traits<InIter1>::value_type;
            using value_type2 = std::iterator_traits<InIter2>::value_type;

            V1 tmp1(traits::vector_pack_load<V1, value_type1>::aligned(it1));
            V2 tmp2(traits::vector_pack_load<V2, value_type2>::aligned(it2));

            auto ret = HPX_INVOKE(f, tmp1, tmp2);
            if constexpr (dereference_is_lvalue_ref_v<OutIter>)
            {
                traits::vector_pack_store<decltype(ret), value_type1>::aligned(
                    ret, dest);
            }
            std::advance(it1, traits::vector_pack_size_v<V1>);
            std::advance(it2, traits::vector_pack_size_v<V2>);
            std::advance(dest, traits::vector_pack_size_v<decltype(ret)>);
        }

        template <typename F, typename InIter1, typename InIter2,
            typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call_unaligned(
            F&& f, InIter1& it1, InIter2& it2, OutIter& dest)
        {
            static_assert(traits::vector_pack_size_v<V1> ==
                    traits::vector_pack_size_v<V2>,
                "the sizes of the vector-packs should be equal");

            using value_type1 = std::iterator_traits<InIter1>::value_type;
            using value_type2 = std::iterator_traits<InIter2>::value_type;

            V1 tmp1(traits::vector_pack_load<V1, value_type1>::unaligned(it1));
            V2 tmp2(traits::vector_pack_load<V2, value_type2>::unaligned(it2));

            auto ret = HPX_INVOKE(f, tmp1, tmp2);
            if constexpr (dereference_is_lvalue_ref_v<OutIter>)
            {
                traits::vector_pack_store<decltype(ret),
                    value_type1>::unaligned(ret, dest);
            }
            std::advance(it1, traits::vector_pack_size_v<V1>);
            std::advance(it2, traits::vector_pack_size_v<V2>);
            std::advance(dest, traits::vector_pack_size_v<decltype(ret)>);
        }
    };

    HPX_CXX_CORE_EXPORT struct datapar_transform_loop_step
    {
        template <typename F, typename InIter, typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(
            F&& f, InIter& it, OutIter& dest)
        {
            using value_type = std::iterator_traits<InIter>::value_type;

            using V1 = traits::vector_pack_type_t<value_type, 1>;

            invoke_vectorized_inout1<V1>::call_unaligned(
                HPX_FORWARD(F, f), it, dest);
        }

        template <typename F, typename InIter1, typename InIter2,
            typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(
            F&& f, InIter1& it1, InIter2& it2, OutIter& dest)
        {
            using value_type1 = std::iterator_traits<InIter1>::value_type;
            using value_type2 = std::iterator_traits<InIter2>::value_type;

            using V1 = traits::vector_pack_type_t<value_type1, 1>;
            using V2 = traits::vector_pack_type_t<value_type2, 1>;

            invoke_vectorized_inout2<V1, V2>::call_unaligned(
                HPX_FORWARD(F, f), it1, it2, dest);
        }

        ///////////////////////////////////////////////////////////////////
        template <typename F, typename InIter, typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void callv(
            F&& f, InIter& it, OutIter& dest)
        {
            using value_type = std::iterator_traits<InIter>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            HPX_ASSERT(is_data_aligned(it) && is_data_aligned(dest));
            invoke_vectorized_inout1<V>::call_aligned(
                HPX_FORWARD(F, f), it, dest);
        }

        template <typename F, typename InIter1, typename InIter2,
            typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void callv(
            F&& f, InIter1& it1, InIter2& it2, OutIter& dest)
        {
            using value1_type = std::iterator_traits<InIter1>::value_type;
            using value2_type = std::iterator_traits<InIter2>::value_type;

            using V1 = traits::vector_pack_type_t<value1_type>;
            using V2 = traits::vector_pack_type_t<value2_type>;

            HPX_ASSERT(is_data_aligned(it1) && is_data_aligned(it2) &&
                is_data_aligned(dest));
            invoke_vectorized_inout2<V1, V2>::call_aligned(
                HPX_FORWARD(F, f), it1, it2, dest);
        }
    };

    HPX_CXX_CORE_EXPORT struct datapar_transform_loop_step_ind
    {
        template <typename F, typename InIter, typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(
            F&& f, InIter& it, OutIter& dest)
        {
            using value_type = std::iterator_traits<InIter>::value_type;

            using V1 = traits::vector_pack_type_t<value_type, 1>;

            invoke_vectorized_inout1_ind<V1>::call_unaligned(
                HPX_FORWARD(F, f), it, dest);
        }

        template <typename F, typename InIter1, typename InIter2,
            typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void call1(
            F&& f, InIter1& it1, InIter2& it2, OutIter& dest)
        {
            using value_type1 = std::iterator_traits<InIter1>::value_type;
            using value_type2 = std::iterator_traits<InIter2>::value_type;

            using V1 = traits::vector_pack_type_t<value_type1, 1>;
            using V2 = traits::vector_pack_type_t<value_type2, 1>;

            invoke_vectorized_inout2_ind<V1, V2>::call_unaligned(
                HPX_FORWARD(F, f), it1, it2, dest);
        }

        ///////////////////////////////////////////////////////////////////
        template <typename F, typename InIter, typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void callv(
            F&& f, InIter& it, OutIter& dest)
        {
            using value_type = std::iterator_traits<InIter>::value_type;

            using V = traits::vector_pack_type_t<value_type>;

            HPX_ASSERT(is_data_aligned(it) && is_data_aligned(dest));
            invoke_vectorized_inout1_ind<V>::call_aligned(
                HPX_FORWARD(F, f), it, dest);
        }

        template <typename F, typename InIter1, typename InIter2,
            typename OutIter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static constexpr void callv(
            F&& f, InIter1& it1, InIter2& it2, OutIter& dest)
        {
            using value1_type = std::iterator_traits<InIter1>::value_type;
            using value2_type = std::iterator_traits<InIter2>::value_type;

            using V1 = traits::vector_pack_type_t<value1_type>;
            using V2 = traits::vector_pack_type_t<value2_type>;

            HPX_ASSERT(is_data_aligned(it1) && is_data_aligned(it2) &&
                is_data_aligned(dest));
            invoke_vectorized_inout2_ind<V1, V2>::call_aligned(
                HPX_FORWARD(F, f), it1, it2, dest);
        }
    };
}    // namespace hpx::parallel::util::detail

#endif
