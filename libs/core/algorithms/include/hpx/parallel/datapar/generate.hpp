//  Copyright (c) 2021 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/generate.hpp>
#include <hpx/parallel/datapar/handle_local_exceptions.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename Iterator>
    struct datapar_generate_helper
    {
        using iterator_type = std::decay_t<Iterator>;
        using value_type =
            typename std::iterator_traits<iterator_type>::value_type;
        using V =
            typename hpx::parallel::traits::vector_pack_type<value_type>::type;

        static constexpr std::size_t size = traits::vector_pack_size_v<V>;

        template <typename Iter, typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static typename std::enable_if_t<
            hpx::parallel::util::detail::iterator_datapar_compatible<
                Iter>::value,
            Iter>
        call(Iter first, std::size_t count, F&& f)
        {
            std::size_t len = count;
            for (; !hpx::parallel::util::detail::is_data_aligned(first) &&
                len != 0;
                --len)
            {
                *first++ = f.template operator()<value_type>();
            }

            for (std::int64_t len_v = std::int64_t(len - size + 1); len_v > 0;
                len_v -= size, len -= size)
            {
                auto tmp = f.template operator()<V>();
                traits::vector_pack_store<V, value_type>::aligned(tmp, first);
                std::advance(first, size);
            }

            for (/* */; len != 0; --len)
            {
                *first++ = f.template operator()<value_type>();
            }
            return first;
        }

        template <typename Iter, typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static typename std::enable_if_t<
            !hpx::parallel::util::detail::iterator_datapar_compatible<
                Iter>::value,
            Iter>
        call(Iter first, std::size_t count, F&& f)
        {
            while (count--)
            {
                *first++ = f.template operator()<value_type>();
            }
            return first;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct datapar_generate
    {
        template <typename ExPolicy, typename Iter, typename Sent, typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
            ExPolicy&&, Iter first, Sent last, F&& f)
        {
            std::size_t count = hpx::parallel::detail::distance(first, last);
            return datapar_generate_helper<Iter>::call(
                first, count, HPX_FORWARD(F, f));
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE
        typename std::enable_if<hpx::is_vectorpack_execution_policy_v<ExPolicy>,
            Iter>::type
        tag_invoke(sequential_generate_t, ExPolicy&& policy, Iter first,
            Sent last, F&& f)
    {
        return datapar_generate::call(
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(F, f));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct datapar_generate_n
    {
        template <typename ExPolicy, typename Iter, typename F>
        HPX_HOST_DEVICE HPX_FORCEINLINE static Iter call(
            ExPolicy&&, Iter first, std::size_t count, F&& f)
        {
            return datapar_generate_helper<Iter>::call(
                first, count, HPX_FORWARD(F, f));
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F>
    HPX_HOST_DEVICE HPX_FORCEINLINE
        typename std::enable_if<hpx::is_vectorpack_execution_policy_v<ExPolicy>,
            Iter>::type
        tag_invoke(sequential_generate_n_t, ExPolicy&& policy, Iter first,
            std::size_t count, F&& f)
    {
        return datapar_generate_n::call(
            HPX_FORWARD(ExPolicy, policy), first, count, HPX_FORWARD(F, f));
    }
}    // namespace hpx::parallel::detail

#endif
