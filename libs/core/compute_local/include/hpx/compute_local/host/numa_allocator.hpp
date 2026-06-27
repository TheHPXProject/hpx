//  Copyright (c) 2015 Thomas Heller
//  Copyright (c) 2015-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <limits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
//
// WARNING: This class is highly experimental, it might not do what you expect
//
///////////////////////////////////////////////////////////////////////////////

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Executors>
    class numa_allocator
    {
        using executor_type = typename Executors::value_type;

    public:
        // typedefs
        using value_type = T;
        using pointer = value_type*;
        using const_pointer = value_type const*;
        using reference = value_type&;
        using const_reference = value_type const&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        // convert an allocator<T> to allocator<U>
        template <typename U>
        struct rebind
        {
            using other = numa_allocator<U, Executors>;
        };

        numa_allocator(
            Executors const& executors, hpx::threads::topology& /* topo */)
          : executors_(executors)
        {
        }

        numa_allocator(numa_allocator const& rhs)
          : executors_(rhs.executors_)
        {
        }

        numa_allocator& operator=(numa_allocator const& rhs) = delete;

        template <typename U>
        numa_allocator(numa_allocator<U, Executors> const& rhs)
          : executors_(rhs.executors_)
        {
        }

        // address
        static pointer address(reference r) noexcept
        {
            return &r;
        }
        static const_pointer address(const_reference r) noexcept
        {
            return &r;
        }

        // memory allocation
        pointer allocate(size_type cnt, void const* = nullptr)
        {
            // allocate memory using the singleton topology
            auto& topo = hpx::threads::get_topology();
            pointer p = static_cast<pointer>(topo.allocate(cnt * sizeof(T)));

            // first touch policy, distribute evenly onto executors
            // last partition receives any remainder elements to avoid
            // leaving trailing elements untouched (which would cause
            // incorrect NUMA domain bindings)
            std::size_t const num_exec = executors_.size();
            std::size_t const part_size = cnt / num_exec;
            std::vector<hpx::future<void>> first_touch;
            first_touch.reserve(num_exec);

            for (std::size_t i = 0; i != num_exec; ++i)
            {
                pointer begin = p + i * part_size;
                // last executor covers any remainder
                pointer end =
                    (i + 1 == num_exec) ? (p + cnt) : (begin + part_size);
                first_touch.push_back(hpx::for_each(
                    hpx::execution::par(hpx::execution::task).on(executors_[i]),
                    begin, end,
#if defined(HPX_DEBUG)
                    [this, i]
#else
                    []
#endif
                    (T& val) {
                        // touch first byte of every object
                        *reinterpret_cast<char*>(&val) = 0;

#if defined(HPX_DEBUG)
                        auto& topo_ = hpx::threads::get_topology();
                        // make sure memory was placed appropriately
                        hpx::threads::mask_type const mem_mask =
                            topo_.get_thread_affinity_mask_from_lva(&val);

                        std::size_t thread_num = hpx::get_worker_thread_num();
                        hpx::threads::mask_cref_type thread_mask =
                            hpx::execution::experimental::get_pu_mask(
                                executors_[i], topo_, thread_num);

                        HPX_ASSERT(threads::mask_size(mem_mask) ==
                            threads::mask_size(thread_mask));
                        HPX_ASSERT(threads::bit_and(mem_mask, thread_mask,
                            threads::mask_size(mem_mask)));
#endif
                    }));
            }

            if (hpx::wait_all_nothrow(first_touch))
            {
                for (auto&& f : first_touch)
                {
                    f.get();    // rethrow exceptions
                }
            }

            // return the overall memory block
            return p;
        }

        void deallocate(pointer p, size_type cnt) noexcept
        {
            hpx::threads::get_topology().deallocate(p, cnt * sizeof(T));
        }

        // size
        static size_type max_size() noexcept
        {
            return (std::numeric_limits<size_type>::max)() / sizeof(T);
        }

        // construction/destruction
        static void construct(pointer p, T const& t)
        {
            hpx::construct_at(p, t);
        }
        static void destroy(pointer p) noexcept
        {
            p->~T();
        }

        friend constexpr bool operator==(
            numa_allocator const&, numa_allocator const&) noexcept
        {
            return true;
        }

        friend constexpr bool operator!=(
            numa_allocator const& l, numa_allocator const& r) noexcept
        {
            return !(l == r);
        }

    private:
        template <typename, typename>
        friend class numa_allocator;

        Executors const& executors_;
    };
}    // namespace hpx::parallel::util
// namespace hpx::parallel::util
