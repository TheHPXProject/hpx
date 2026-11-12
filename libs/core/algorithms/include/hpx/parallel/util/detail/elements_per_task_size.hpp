//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>

namespace hpx::parallel::util::detail {

    // The number of elements a call of a loop body of the given task size
    // handles when the elements of Iter are processed with the execution
    // policy ExPolicy. It is one, unless the policy hands several elements to
    // a call (the datapar policies, see datapar/elements_per_call.hpp) or
    // lets the compiler vectorize the loop (the unsequenced policies, see
    // unseq/elements_per_call.hpp).
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Enable = void>
    struct policy_elements_per_call
    {
        [[nodiscard]] static constexpr std::size_t call(
            hpx::threads::task_size) noexcept
        {
            return 1;
        }
    };

    // The number of elements the loop body F of the given task size handles
    // in the time that size stands for, when the elements of Iter are
    // processed with the execution policy ExPolicy. By default, this is what
    // the policy hands to a call. A loop body that knows better, for instance
    // a copy of a few bytes, specializes this for its type; the
    // specialization replaces the policy's number.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename F,
        typename Enable = void>
    struct elements_per_call
    {
        [[nodiscard]] static std::size_t call(
            F const&, hpx::threads::task_size const size) noexcept
        {
            return policy_elements_per_call<ExPolicy, Iter>::call(size);
        }
    };

    // A loop body that was given a size handles as many elements as the
    // function it wraps would with that size.
    template <typename ExPolicy, typename Iter, typename F>
    struct elements_per_call<ExPolicy, Iter,
        hpx::threads::detail::size_of_function<F>>
    {
        [[nodiscard]] static std::size_t call(
            hpx::threads::detail::size_of_function<F> const& f,
            hpx::threads::task_size const size) noexcept
        {
            return elements_per_call<ExPolicy, Iter, F>::call(f.f, size);
        }
    };

    // The iterator type of the elements the partitioners are given, either
    // as an iterator or as a range.
    HPX_CXX_CORE_EXPORT template <typename IterOrR, typename Enable = void>
    struct partition_iterator
    {
        using type = IterOrR;
    };

    template <typename R>
        requires(std::ranges::range<R>)
    struct partition_iterator<R>
    {
        using type = std::ranges::iterator_t<R>;
    };

    // The number of elements the loop body f handles in the time its task
    // size stands for, when the elements of IterOrR (an iterator or a range)
    // are processed with the execution policy ExPolicy.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename IterOrR,
        typename F>
    [[nodiscard]] std::size_t elements_per_task_size(F const& f) noexcept
    {
        using function_type = std::decay_t<F>;
        return elements_per_call<std::decay_t<ExPolicy>,
            typename partition_iterator<std::decay_t<IterOrR>>::type,
            function_type>::call(f,
            hpx::traits::get_task_size<function_type>::call(f));
    }
}    // namespace hpx::parallel::util::detail

// the specializations for the unsequenced and the datapar policies, which
// have to be visible wherever the partitioners use the traits
#include <hpx/parallel/unseq/elements_per_call.hpp>
#if defined(HPX_HAVE_DATAPAR)
#include <hpx/parallel/datapar/elements_per_call.hpp>
#endif
