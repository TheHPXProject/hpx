//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/segmented_iterator_traits.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <iterator>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Enable = void>
    struct projected_iterator
    {
        using type = std::decay_t<T>;
    };

    // For segmented iterators, we consider the local_raw_iterator instead of
    // the given one.
    template <typename Iterator>
    struct projected_iterator<Iterator,
        std::enable_if_t<is_segmented_iterator_v<Iterator>>>
    {
        using local_iterator =
            typename segmented_iterator_traits<Iterator>::local_iterator;

        using type = typename segmented_local_iterator_traits<
            local_iterator>::local_raw_iterator;
    };

    template <typename Iterator>
    struct projected_iterator<Iterator,
        std::void_t<typename std::decay_t<Iterator>::proxy_type>>
    {
        using type = typename std::decay_t<Iterator>::proxy_type;
    };

    template <typename Iterator>
    using projected_iterator_t = typename projected_iterator<Iterator>::type;
}    // namespace hpx::traits

namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename F, typename Iter, typename Enable = void>
        struct projected_result_of;

        template <typename Proj, typename Iter>
        struct projected_result_of<Proj, Iter,
            std::enable_if_t<hpx::traits::is_iterator_v<Iter>>>
          : hpx::util::invoke_result<Proj, hpx::traits::iter_reference_t<Iter>>
        {
        };

        template <typename Projected>
        struct projected_result_of_indirect
          : projected_result_of<typename Projected::projector_type,
                typename Projected::iterator_type>
        {
        };

#if defined(HPX_HAVE_DATAPAR)
        // This is being instantiated if a vector pack execution policy is used
        // with a zip_iterator. In this case the function object is invoked
        // with a tuple<datapar<T>...> instead of just a tuple<T...>
        template <typename Proj, typename ValueType, typename Enable = void>
        struct projected_result_of_vector_pack_
          : hpx::util::invoke_result<Proj,
                typename hpx::parallel::traits::vector_pack_load<
                    typename hpx::parallel::traits::vector_pack_type<
                        ValueType>::type,
                    ValueType>::value_type&>
        {
        };

        template <typename Projected, typename Enable = void>
        struct projected_result_of_vector_pack;

        template <typename Projected>
        struct projected_result_of_vector_pack<Projected,
            std::void_t<typename Projected::iterator_type>>
          : projected_result_of_vector_pack_<typename Projected::projector_type,
                hpx::traits::iter_value_t<typename Projected::iterator_type>>
        {
        };
#endif
    }    // namespace detail

    template <typename F, typename Iter, typename Enable = void>
    struct projected_result_of
      : detail::projected_result_of<std::decay_t<F>, std::decay_t<Iter>>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename F, typename Iter, typename Enable = void>
        struct is_projected : std::false_type
        {
        };

        // the given projection function is valid, if it can be invoked using
        // the dereferenced iterator type and if the projection does not return
        // void

        // clang-format off
        template <typename Proj, typename Iter>
        struct is_projected<Proj, Iter,
            std::enable_if_t<
                hpx::traits::is_iterator_v<Iter> &&
                hpx::is_invocable_v<Proj, hpx::traits::iter_reference_t<Iter>>
             >>
          : std::integral_constant<bool,
                !std::is_void_v<hpx::util::invoke_result_t<
                    Proj, hpx::traits::iter_reference_t<Iter>>>>
        {
        };
        // clang-format on

        template <typename Projected, typename Enable = void>
        struct is_projected_indirect : std::false_type
        {
        };

        template <typename Projected>
        struct is_projected_indirect<Projected,
            std::void_t<typename Projected::projector_type>>
          : detail::is_projected<typename Projected::projector_type,
                typename Projected::iterator_type>
        {
        };
    }    // namespace detail

    template <typename F, typename Iter, typename Enable = void>
    struct is_projected
      : detail::is_projected<std::decay_t<F>,
            hpx::traits::projected_iterator_t<Iter>>
    {
    };

    template <typename F, typename Iter>
    inline constexpr bool is_projected_v = is_projected<F, Iter>::value;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Proj, typename Iter>
    struct projected
    {
        using projector_type = std::decay_t<Proj>;
        using iterator_type = hpx::traits::projected_iterator_t<Iter>;
    };

    template <typename Projected, typename Enable = void>
    struct is_projected_indirect : detail::is_projected_indirect<Projected>
    {
    };

    template <typename Projected, typename Enable = void>
    struct is_projected_zip_iterator : std::false_type
    {
    };

    template <typename Projected>
    struct is_projected_zip_iterator<Projected,
        std::void_t<typename Projected::iterator_type>>
      : hpx::traits::is_zip_iterator<typename Projected::iterator_type>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename F, typename... Args>
        struct is_indirect_callable_impl : hpx::is_invocable<F, Args...>
        {
        };

        template <typename ExPolicy, typename F, typename ProjectedPack,
            typename Enable = void>
        struct is_indirect_callable : std::false_type
        {
        };

        template <typename ExPolicy, typename F, typename... Projected>
        struct is_indirect_callable<ExPolicy, F, hpx::util::pack<Projected...>,
            std::enable_if_t<
                hpx::util::all_of_v<is_projected_indirect<Projected>...> &&
                (!hpx::is_vectorpack_execution_policy_v<ExPolicy> ||
                    !hpx::util::all_of_v<
                        is_projected_zip_iterator<Projected>...>)>>
          : is_indirect_callable_impl<F,
                typename projected_result_of_indirect<Projected>::type...>
        {
        };

#if defined(HPX_HAVE_DATAPAR)
        // Vector pack execution policies used with zip-iterators require
        // special handling because zip_iterator<>::reference is not a real
        // reference type.
        template <typename ExPolicy, typename F, typename... Projected>
        struct is_indirect_callable<ExPolicy, F, hpx::util::pack<Projected...>,
            std::enable_if_t<
                hpx::util::all_of_v<is_projected_indirect<Projected>...> &&
                hpx::is_vectorpack_execution_policy_v<ExPolicy> &&
                hpx::util::all_of_v<is_projected_zip_iterator<Projected>...>>>
          : is_indirect_callable_impl<F,
                typename projected_result_of_vector_pack<Projected>::type...>
        {
        };
#endif
    }    // namespace detail

    template <typename ExPolicy, typename F, typename... Projected>
    struct is_indirect_callable
      : detail::is_indirect_callable<std::decay_t<ExPolicy>, std::decay_t<F>,
            hpx::util::pack<std::decay_t<Projected>...>>
    {
    };

    template <typename ExPolicy, typename F, typename... Projected>
    inline constexpr bool is_indirect_callable_v =
        is_indirect_callable<ExPolicy, F, Projected...>::value;
}    // namespace hpx::parallel::traits
