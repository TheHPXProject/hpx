//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/is_sorted.hpp>
#include <hpx/parallel/algorithms/detail/spin_sort.hpp>
#include <hpx/parallel/util/merge_four.hpp>
#include <hpx/parallel/util/merge_vector.hpp>
#include <hpx/parallel/util/range.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT inline constexpr std::uint32_t
        sample_sort_limit_per_task = 1 << 16;

    /// \struct sample_sort
    /// \brief This a structure for to implement a sample sort, exception
    ///        safe
    /// \tparam
    /// \remarks
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
        typename Compare>
    struct sample_sort_helper
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        using range_it = util::range<Iter, Sent>;
        using range_buf = util::range<value_type*>;

        std::uint32_t nthreads;
        std::uint32_t nintervals;
        bool construct = false;
        bool owner = false;
        Compare comp;
        range_it global_range;
        range_buf global_buf;

        std::vector<std::vector<range_it>> vv_range_it;
        std::vector<std::vector<range_buf>> vv_range_buf;
        std::vector<range_it> vrange_it_ini;
        std::vector<range_buf> vrange_buf_ini;
        std::atomic<std::uint32_t> njob;

        template <typename Exec>
        void initial_configuration(Exec&);

        template <typename Exec>
        void operator()(Exec&& exec, Iter first, Sent last, value_type* paux,
            std::size_t naux, std::size_t chunk_size);

        /// \brief constructor of the typename
        ///
        /// \param [in] cmp : object for to Compare two elements
        /// \param [in] nthreads : define the number of threads to use
        ///              in the process. By default is the number of thread HW
        sample_sort_helper(Compare cmp, std::uint32_t nthreads);

        /// \brief destructor of the typename. The utility is to destroy the
        ///        temporary buffer used in the sorting process
        ~sample_sort_helper();

        /// \brief this a function to assign to each thread in the first merge
        inline void execute_first()
        {
            std::uint32_t job = 0;
            while ((job = njob++) < nintervals)
            {
                parallel::util::uninit_merge_level4(vrange_buf_ini[job],
                    vv_range_it[job], vv_range_buf[job], comp);
            }
        }

        /// \brief this is a function to assign each thread the final merge
        void execute()
        {
            std::uint32_t job = 0;
            while ((job = njob++) < nintervals)
            {
                parallel::util::merge_vector4(vrange_buf_ini[job],
                    vrange_it_ini[job], vv_range_buf[job], vv_range_it[job],
                    comp);
            }
        }

        /// \brief Implement the merge of the initially sparse ranges
        template <typename Exec>
        void first_merge(Exec& exec)
        {
            njob = 0;

            auto shape = hpx::util::iterator_range(
                hpx::util::counting_iterator(static_cast<std::uint32_t>(0)),
                hpx::util::counting_iterator(nthreads));

            hpx::wait_all(execution::bulk_async_execute(
                exec, [this](std::uint32_t) { this->execute_first(); }, shape));

            construct = true;
        }

        /// \brief Implement the final merge of the ranges
        /// \exception
        /// \return
        /// \remarks
        template <typename Exec>
        inline void final_merge(Exec& exec)
        {
            njob = 0;

            auto shape = hpx::util::iterator_range(
                hpx::util::counting_iterator(static_cast<std::uint32_t>(0)),
                hpx::util::counting_iterator(nthreads));

            hpx::wait_all(execution::bulk_async_execute(
                exec, [this](std::uint32_t) { this->execute(); }, shape));
        }
    };

    /// \brief constructor of the typename
    ///
    /// \param [in] cmp : object for to Compare two elements
    /// \param [in] nthreads : nthreads object for to define the number of threads
    ///            to use in the process. By default is the number of thread HW
    template <typename Iter, typename Sent, typename Compare>
    sample_sort_helper<Iter, Sent, Compare>::sample_sort_helper(
        Compare cmp, std::uint32_t nthreads)
      : nthreads(nthreads)
      , nintervals(0)
      , construct(false)
      , owner(false)
      , comp(cmp)
      , global_buf(nullptr, nullptr)
      , njob(0)
    {
    }

    template <typename Iter, typename Sent, typename Compare>
    template <typename Exec>
    void sample_sort_helper<Iter, Sent, Compare>::operator()(Exec&& exec,
        Iter first, Sent last, value_type* paux, std::size_t naux,
        std::size_t chunk_size)
    {
        global_range = range_it(first, last);

        HPX_ASSERT(last - first >= 0);
        std::size_t nelem = static_cast<std::size_t>(last - first);

        // Adjust when there are many threads and only a few elements
        while (nelem > chunk_size &&
            static_cast<std::size_t>(nthreads) *
                    static_cast<std::size_t>(nthreads) >
                nelem >> 3)
        {
            nthreads /= 2;
        }

        nintervals = nthreads << 3;

        if (nthreads < 2 || nelem <= chunk_size)
        {
            spin_sort(first, last, comp);
            return;
        }

        if (detail::is_sorted_sequential(first, last, comp))
        {
            return;
        }

        if (paux != nullptr)
        {
            HPX_ASSERT(naux != 0);
            global_buf = range_buf(paux, paux + naux);
            owner = false;
        }
        else
        {
            // acquire uninitialized memory
            value_type* ptr = static_cast<value_type*>(
                std::malloc(sizeof(value_type) * nelem));
            if (ptr == nullptr)
            {
                throw std::bad_alloc();
            }
            global_buf = range_buf(ptr, ptr + nelem);
            owner = true;
        }

        // processing
        initial_configuration(exec);
        first_merge(exec);
        final_merge(exec);
    }

    /// \brief destructor of the typename. The utility is to destroy the temporary
    ///        buffer used in the sorting process
    template <typename Iter, typename Sent, typename Compare>
    sample_sort_helper<Iter, Sent, Compare>::~sample_sort_helper(void)
    {
        if (construct)
        {
            parallel::util::destroy_range(global_buf);
            construct = false;
        }

        if (owner)
        {
            std::free(global_buf.begin());
        }
    }

    /// \class less_ptr_no_null
    ///
    /// \remarks this is the comparison object for pointers. Receive a object
    ///          for to compare the objects pointed. The pointers can't be
    ///          nullptr
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Comp>
    struct less_ptr_no_null
    {
        Comp comp;

        inline less_ptr_no_null(Comp comp)
          : comp(HPX_MOVE(comp))
        {
        }

        inline bool operator()(Iter t1, Iter t2) const
        {
            return comp(*t1, *t2);
        }
    };

    /// Create the internal data structures, and obtain the initial set of
    ///        ranges to merge
    /// \exception
    /// \return
    /// \remarks
    template <typename Iter, typename Sent, typename Compare>
    template <typename Exec>
    void sample_sort_helper<Iter, Sent, Compare>::initial_configuration(
        Exec& exec)
    {
        std::vector<range_it> vmem_thread;
        std::vector<range_buf> vbuf_thread;
        std::size_t const nelem = global_range.size();

        std::size_t chunk_size = nelem / nthreads;
        Iter it_first = global_range.begin();
        value_type* buf_first = global_buf.begin();

        for (std::uint32_t i = 0; i < nthreads - 1;
            ++i, it_first += chunk_size, buf_first += chunk_size)
        {
            vmem_thread.emplace_back(it_first, it_first + chunk_size);
            vbuf_thread.emplace_back(buf_first, buf_first + chunk_size);
        }

        vmem_thread.emplace_back(it_first, global_range.end());
        vbuf_thread.emplace_back(buf_first, global_buf.end());

        // Sorting of the ranges
        auto shape = hpx::util::iterator_range(
            hpx::util::counting_iterator(static_cast<std::uint32_t>(0)),
            hpx::util::counting_iterator(nthreads));

        hpx::wait_all(execution::bulk_async_execute(
            exec,
            [&, this](std::uint32_t i) {
                spin_sort(vmem_thread[i].begin(), vmem_thread[i].end(), comp,
                    vbuf_thread[i]);
            },
            shape));

        // Obtain the vector of milestones
        std::vector<Iter> vsample;
        vsample.reserve(nthreads * (nintervals - 1));

        for (std::uint32_t i = 0; i < nthreads; ++i)
        {
            std::size_t const distance = vmem_thread[i].size() / nintervals;
            for (std::size_t j = 1, pos = distance; j < nintervals;
                ++j, pos += distance)
            {
                vsample.push_back(vmem_thread[i].begin() + pos);
            }
        }

        typedef less_ptr_no_null<Iter, Compare> compare_ptr;
        spin_sort(vsample.begin(), vsample.end(), compare_ptr(comp));

        // Create the final milestone vector
        std::vector<Iter> vmilestone;
        vmilestone.reserve(nintervals);

        for (std::uint32_t pos = nthreads >> 1; pos < vsample.size();
            pos += nthreads)
        {
            vmilestone.push_back(vsample[pos]);
        }

        // Creation of the first vector of ranges
        std::vector<std::vector<util::range<Iter>>> vv_range_first(nthreads);

        for (std::uint32_t i = 0; i < nthreads; ++i)
        {
            Iter itaux = vmem_thread[i].begin();
            for (std::uint32_t k = 0; k < nintervals - 1; ++k)
            {
                // Pass the milestone iterator so upper_bound does not add
                // const to the element passed to a projection.
                Iter it2 = std::upper_bound(itaux, vmem_thread[i].end(),
                    vmilestone[k], [this](Iter milestone, auto&& value) {
                        return comp(
                            *milestone, HPX_FORWARD(decltype(value), value));
                    });

                vv_range_first[i].emplace_back(itaux, it2);
                itaux = it2;
            }
            vv_range_first[i].emplace_back(itaux, vmem_thread[i].end());
        }

        // Copy in buffer and creation of the final matrix of ranges
        vv_range_it.resize(nintervals);
        vv_range_buf.resize(nintervals);
        vrange_it_ini.reserve(nintervals);
        vrange_buf_ini.reserve(nintervals);

        for (std::uint32_t i = 0; i < nintervals; ++i)
        {
            vv_range_it[i].reserve(nthreads);
            vv_range_buf[i].reserve(nthreads);
        }

        Iter it = global_range.begin();
        value_type* it_buf = global_buf.begin();

        for (std::uint32_t k = 0; k < nintervals; ++k)
        {
            std::size_t nelem_interval = 0;

            for (std::uint32_t i = 0; i < nthreads; ++i)
            {
                size_t const nelem_range = vv_range_first[i][k].size();
                if (nelem_range != 0)
                {
                    vv_range_it[k].push_back(vv_range_first[i][k]);
                }
                nelem_interval += nelem_range;
            }

            vrange_it_ini.emplace_back(it, it + nelem_interval);
            vrange_buf_ini.emplace_back(it_buf, it_buf + nelem_interval);

            it += nelem_interval;
            it_buf += nelem_interval;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Exec, typename Iter, typename Sent,
        typename Compare, typename Value>
    void sample_sort(Exec&& exec, Iter first, Sent last, Compare&& comp,
        std::uint32_t num_threads, Value* paux, std::size_t naux,
        std::size_t chunk_size)
    {
        using sample_sort_helper_t =
            sample_sort_helper<Iter, Sent, std::decay_t<Compare>>;

        sample_sort_helper_t sorter(HPX_FORWARD(Compare, comp), num_threads);
        sorter(HPX_FORWARD(Exec, exec), first, last, paux, naux, chunk_size);
    }

    HPX_CXX_CORE_EXPORT template <typename Exec, typename Iter, typename Sent,
        typename Compare>
    void sample_sort(Exec&& exec, Iter first, Sent last, Compare&& comp,
        std::uint32_t num_threads)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;

        return sample_sort(HPX_FORWARD(Exec, exec), first, last,
            HPX_FORWARD(Compare, comp), num_threads,
            static_cast<value_type*>(nullptr), static_cast<std::size_t>(0),
            static_cast<std::size_t>(sample_sort_limit_per_task));
    }

    HPX_CXX_CORE_EXPORT template <typename Exec, typename Iter, typename Sent,
        typename Compare>
    void sample_sort(Exec&& exec, Iter first, Sent last, Compare&& comp,
        std::uint32_t num_threads,
        util::range<typename std::iterator_traits<Iter>::value_type*>
            range_buf_initial,
        std::size_t chunk_size = 0)
    {
        if (chunk_size == 0)
        {
            chunk_size = sample_sort_limit_per_task;
        }

        return sample_sort(HPX_FORWARD(Exec, exec), first, last,
            HPX_FORWARD(Compare, comp), num_threads, range_buf_initial.begin(),
            range_buf_initial.size(), chunk_size);
    }

    HPX_CXX_CORE_EXPORT template <typename Exec, typename Iter, typename Sent>
    void sample_sort(
        Exec&& exec, Iter first, Sent last, std::uint32_t num_threads)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        using compare = std::less<value_type>;

        return sample_sort(HPX_FORWARD(Exec, exec), first, last, compare{},
            num_threads, static_cast<value_type*>(nullptr),
            static_cast<std::size_t>(0),
            static_cast<std::size_t>(sample_sort_limit_per_task));
    }

    HPX_CXX_CORE_EXPORT template <typename Exec, typename Iter, typename Sent>
    void sample_sort(Exec&& exec, Iter first, Sent last)
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;
        using compare = std::less<value_type>;

        return sample_sort(HPX_FORWARD(Exec, exec), first, last, compare{},
            (std::uint32_t) hpx::threads::hardware_concurrency(),
            static_cast<value_type*>(nullptr), static_cast<std::size_t>(0),
            static_cast<std::size_t>(sample_sort_limit_per_task));
    }
}    // namespace hpx::parallel::detail
