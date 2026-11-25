//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/parallel/algorithms/detail/sample_sort.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT inline constexpr std::size_t
        stable_sort_limit_per_task = 1 << 16;

    /// \struct parallel_stable_sort
    ///
    /// This a structure for to implement a parallel stable sort exception safe
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
        typename Compare>
    struct parallel_stable_sort_helper
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;

        util::range<Iter, Sent> range_initial;
        Compare comp;
        std::size_t nelem;
        value_type* ptr;

        parallel_stable_sort_helper(Iter first, Sent last, Compare cmp);

        // / brief Perform sorting operation
        template <typename Exec>
        Iter operator()(
            Exec&& exec, std::uint32_t nthreads, std::size_t chunk_size);

        /// \brief destructor of the typename. The utility is to destroy the
        ///        temporary buffer used in the sorting process
        ~parallel_stable_sort_helper()
        {
            if (ptr != nullptr)
            {
                std::free(ptr);
            }
        }
    };    // end struct parallel_stable_sort

    /// \brief constructor of the typename
    ///
    /// \param [in] first : range of elements to sort
    /// \param [in] last : range of elements to sort
    /// \param [in] comp : object for to compare two elements
    template <typename Iter, typename Sent, typename Compare>
    parallel_stable_sort_helper<Iter, Sent,
        Compare>::parallel_stable_sort_helper(Iter first, Sent last,
        Compare comp)
      : range_initial(first, last)
      , comp(comp)
      , nelem(range_initial.size())
      , ptr(nullptr)
    {
        HPX_ASSERT(range_initial.size() >= 0);
    }

    template <typename Iter, typename Sent, typename Compare>
    template <typename Exec>
    Iter parallel_stable_sort_helper<Iter, Sent, Compare>::operator()(
        Exec&& exec, std::uint32_t nthreads, std::size_t chunk_size)
    {
        try
        {
            std::size_t nptr = (nelem + 1) >> 1;
            Iter last = range_initial.begin() + nelem;

            if (nelem < chunk_size || nthreads < 2)
            {
                spin_sort(range_initial.begin(), range_initial.end(), comp);
                return last;
            }

            if (detail::is_sorted_sequential(
                    range_initial.begin(), range_initial.end(), comp))
            {
                return last;
            }

            // leave memory uninitialized, sample_sort will manage construction
            // etc.
            ptr = static_cast<value_type*>(
                std::malloc(sizeof(value_type) * nptr));
            if (ptr == nullptr)
            {
                throw std::bad_alloc();
            }

            // Parallel Process
            util::range<Iter, Sent> range_first(
                range_initial.begin(), range_initial.begin() + nptr);
            util::range<Iter, Sent> range_second(
                range_initial.begin() + nptr, range_initial.end());
            util::range<value_type*> range_buffer(ptr, ptr + nptr);

            sample_sort(exec, range_initial.begin(),
                range_initial.begin() + nptr, comp, nthreads, range_buffer,
                chunk_size);

            sample_sort(exec, range_initial.begin() + nptr, range_initial.end(),
                comp, nthreads, range_buffer, chunk_size);

            range_buffer = parallel::util::init_move(range_buffer, range_first);
            range_initial = parallel::util::half_merge(
                range_initial, range_buffer, range_second, comp);

            return last;
        }
        catch (std::bad_alloc const&)
        {
            throw;
        }
        catch (hpx::exception_list const&)
        {
            throw;
        }
        catch (...)
        {
            throw hpx::exception_list(std::current_exception());
        }
    }

    HPX_CXX_CORE_EXPORT template <typename Exec, typename Iter, typename Sent,
        typename Compare>
    Iter parallel_stable_sort(Exec&& exec, Iter first, Sent last,
        std::size_t cores, std::size_t chunk_size, Compare&& comp)
    {
        using parallel_stable_sort_helper_t =
            parallel_stable_sort_helper<Iter, Sent, std::decay_t<Compare>>;

        parallel_stable_sort_helper_t sorter(
            first, last, HPX_FORWARD(Compare, comp));

        return sorter(HPX_FORWARD(Exec, exec),
            static_cast<std::uint32_t>(cores), chunk_size);
    }

    HPX_CXX_CORE_EXPORT template <typename Exec, typename Iter, typename Sent>
    Iter parallel_stable_sort(Exec&& exec, Iter first, Sent last)
    {
        using compare =
            std::less<typename std::iterator_traits<Iter>::value_type>;

        return parallel_stable_sort(HPX_FORWARD(Exec, exec), first, last,
            hpx::threads::hardware_concurrency(), stable_sort_limit_per_task,
            compare{});
    }
}    // namespace hpx::parallel::detail
