//  Copyright (c) 2023-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/allocator_support/config/defines.hpp>

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::util {

#if defined(HPX_ALLOCATOR_SUPPORT_HAVE_CACHING) &&                             \
    !((defined(HPX_HAVE_CUDA) && defined(__CUDACC__)) ||                       \
        defined(HPX_HAVE_HIP))

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <template <typename, typename> class Stack,
        typename Allocator = std::allocator<char>,
        std::size_t DefaultCapacity = 100>
    struct thread_local_caching_allocator
    {
        HPX_NO_UNIQUE_ADDRESS Allocator alloc;

        using traits = std::allocator_traits<Allocator>;

        using value_type = traits::value_type;
        using pointer = traits::pointer;
        using const_pointer = traits::const_pointer;
        using size_type = traits::size_type;
        using difference_type = traits::difference_type;

        template <typename U>
        struct rebind
        {
            using other = thread_local_caching_allocator<Stack,
                typename traits::template rebind_alloc<U>, DefaultCapacity>;
        };

        using is_always_equal = traits::is_always_equal;
        using propagate_on_container_copy_assignment =
            traits::propagate_on_container_copy_assignment;
        using propagate_on_container_move_assignment =
            traits::propagate_on_container_move_assignment;
        using propagate_on_container_swap = traits::propagate_on_container_swap;

    private:
        struct allocated_cache
        {
            using cached_entry = std::pair<pointer, size_type>;

            explicit allocated_cache(Allocator const& a,
                std::size_t const cap) noexcept(noexcept(std::
                    is_nothrow_copy_constructible_v<Allocator>))
              : alloc(a)
              , data(0)
              , cached(0)
              , capacity(cap)
            {
            }

            allocated_cache(allocated_cache const&) = delete;
            allocated_cache(allocated_cache&&) = delete;
            allocated_cache& operator=(allocated_cache const&) = delete;
            allocated_cache& operator=(allocated_cache&&) = delete;

            ~allocated_cache()
            {
                clear_cache();
            }

            pointer allocate(size_type n)
            {
                // Search for an entry with matching size. We try popping until
                // we find matching size or data empty. Popped non-matching
                // entries are temporarily stored and then pushed back to
                // preserve cache contents.
                cached_entry pair;

                bool found = false;
                std::vector<cached_entry> temp;
                while (data.pop(pair))
                {
                    if (pair.second == n)
                    {
                        found = true;
                        break;
                    }
                    temp.emplace_back(HPX_MOVE(pair));
                }

                // push back the non-matching entries
                for (auto& p : temp)
                {
                    // best-effort: if push throws, deallocate to avoid leak
                    try
                    {
                        data.push(HPX_MOVE(p));
                    }
                    catch (...)
                    {
                        // If push throws, deallocate immediately to not lose memory.
                        try
                        {
                            traits::deallocate(alloc, p.first, p.second);
                        }
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                        catch (...)
                        {
                            // swallow
                        }
                    }
                }

                if (found)
                {
                    --cached;
                    return pair.first;
                }

                return traits::allocate(alloc, n);
            }

            void deallocate(pointer p, size_type n)
            {
                if (cached.load(std::memory_order_relaxed) < capacity)
                {
                    try
                    {
                        data.push(std::make_pair(p, n));
                        ++cached;
                        return;
                    }
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                    catch (...)
                    {
                        // fallthrough to direct deallocate on push failure
                    }
                }

                // either cache full or push failed: deallocate immediately
                traits::deallocate(alloc, p, n);
            }

        private:
            void clear_cache() noexcept
            {
                cached_entry p;
                while (data.pop(p))
                {
                    try
                    {
                        traits::deallocate(alloc, p.first, p.second);
                    }
                    // NOLINTNEXTLINE(bugprone-empty-catch)
                    catch (...)
                    {
                        // swallow all exceptions during thread shutdown
                    }
                }
                cached.store(0, std::memory_order_relaxed);
            }

            HPX_NO_UNIQUE_ADDRESS Allocator alloc;
            Stack<cached_entry, Allocator> data;
            std::atomic<std::size_t> cached;
            std::size_t const capacity = DefaultCapacity;
        };

        allocated_cache& cache()
        {
            thread_local allocated_cache allocated_data(alloc, DefaultCapacity);
            return allocated_data;
        }

    public:
        // clang-format off
        explicit thread_local_caching_allocator(
            Allocator const& alloc = Allocator{})
            noexcept(noexcept(std::is_nothrow_copy_constructible_v<Allocator>))
          : alloc(alloc)
        {
        }

        template <typename Alloc>
        explicit thread_local_caching_allocator(
            thread_local_caching_allocator<Stack, Alloc, DefaultCapacity> const& rhs)
            noexcept(noexcept(std::is_nothrow_copy_constructible_v<Alloc>))
          : alloc(rhs.alloc)
        {
        }
        // clang-format on

        [[nodiscard]] static constexpr pointer address(value_type& x) noexcept
        {
            return std::addressof(x);
        }

        [[nodiscard]] static constexpr const_pointer address(
            value_type const& x) noexcept
        {
            return std::addressof(x);
        }

        [[nodiscard]] pointer allocate(size_type n, void const* = nullptr)
        {
            if (max_size() < n)
            {
                throw std::bad_array_new_length();
            }
            return cache().allocate(n);
        }

        void deallocate(pointer p, size_type n)
        {
            cache().deallocate(p, n);
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept
        {
            return traits::max_size(alloc);
        }

        template <typename U, typename... Args>
        void construct(U* p, Args&&... args)
        {
            traits::construct(alloc, p, HPX_FORWARD(Args, args)...);
        }

        template <typename U>
        void destroy(U* p) noexcept
        {
            traits::destroy(alloc, p);
        }

        [[nodiscard]] friend constexpr bool operator==(
            thread_local_caching_allocator const& lhs,
            thread_local_caching_allocator const& rhs) noexcept
        {
            return lhs.alloc == rhs.alloc;
        }

        [[nodiscard]] friend constexpr bool operator!=(
            thread_local_caching_allocator const& lhs,
            thread_local_caching_allocator const& rhs) noexcept
        {
            return !(lhs == rhs);
        }
    };
#else
    HPX_CXX_CORE_EXPORT template <template <typename, typename> class Stack,
        typename Allocator = std::allocator<char>>
    using thread_local_caching_allocator = Allocator;
#endif
}    // namespace hpx::util
