//  Copyright (c) 2017-2019 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/compute_local/macros.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/type_support.hpp>

#include <concepts>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(__linux) || defined(linux) || defined(__linux__)
#include <iostream>
#include <linux/unistd.h>
#include <sys/mman.h>
#endif

// Can be used to enable debugging of the allocator page mapping
//#define NUMA_BINDING_ALLOCATOR_INIT_MEMORY

namespace hpx {

    HPX_CXX_CORE_EXPORT inline hpx::debug::enable_print<
        NUMA_BINDING_ALLOCATOR_DEBUG>
        nba_deb("NUM_B_A");
}    // namespace hpx

namespace hpx::execution::experimental {

    HPX_CXX_CORE_EXPORT struct numa_binding_allocator_tag
    {
    };

    template <>
    struct HPX_EXPORT pool_numa_hint<numa_binding_allocator_tag>
    {
        // The call operator () must return an int type
        // The arguments must be const ref versions of the equivalent task arguments
        int operator()(int const& domain) const
        {
            nba_deb.debug(debug::str<>("pool_numa_hint"),
                "allocator returns domain ", domain);
            return domain;
        }
    };
}    // namespace hpx::execution::experimental

namespace hpx::compute::host {

    HPX_CXX_CORE_EXPORT template <typename T>
    struct numa_binding_helper
    {
        // After memory has been allocated, this operator will be called for
        // every page of memory in the allocation by one thread in each numa
        // domain (implying that this function will be called for each page) by
        // N threads, where N = number of domains. The return from this function
        // should be the domain number that should touch this page. The thread
        // with the matching domain will perform a memory read/write on the
        // page.
        virtual std::size_t operator()(T const* const /*base_ptr*/,
            T const* const /*page_ptr*/, std::size_t const /*page_size*/,
            std::size_t const /*domains*/) const
        {
            return 0;
        }

        // virtual destructor to quiet compiler warnings
        virtual ~numa_binding_helper() = default;

        // The allocator uses the pool name to get numa bitmap masks needed by
        // the allocation function. The "default" pool is assumed
        // @TODO use an executor to retrieve the pool name
        virtual std::string const& pool_name() const
        {
            return pool_name_;
        }

        // Return the total memory consumption in bytes
        virtual std::size_t memory_bytes() const
        {
            return 0;
        }

        // Using how many dimensions should this data be displayed
        // (This function is only required/used) for debug purposes
        virtual std::size_t array_rank() const
        {
            return 1;
        }

        // The number of elements along dimension x=0,y=1,z=2,...
        // This function is only required for debug purposes
        virtual std::size_t array_size(std::size_t /* axis */) const
        {
            return memory_bytes() / sizeof(T);
        };

        // When counting along elements in a given dimension,
        // how large a step should be taken in units of elements.
        // This should include padding along an axis
        // This function is only required for debug purposes
        virtual std::size_t memory_step(std::size_t /* axis */) const
        {
            return 1;
        };

        // When displaying the data, what step size should be used
        // This function is only required for debug purposes
        virtual std::size_t display_step(std::size_t /* axis */) const
        {
            return 1;
        };

        // This is for debug/information purposes only and anything may
        // be returned that helps illuminate the mapping of pages->indices
        virtual std::string description() const
        {
            return "";
        };

        std::string pool_name_ = "default";
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using numa_binding_helper_ptr = std::shared_ptr<numa_binding_helper<T>>;

    /// The numa_binding_allocator allocates memory using a policy based on
    /// hwloc flags for memory binding. This allocator can be used to request
    /// data that is bound to one or more numa domains via the bitmap mask
    /// supplied
    HPX_CXX_CORE_EXPORT template <typename T>
    struct numa_binding_allocator
    {
        using value_type = T;
        using pointer = T*;
        using const_pointer = T const*;
        using reference = T&;
        using const_reference = T const&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        //
        template <typename U>
        struct rebind
        {
            using other = numa_binding_allocator<U>;
        };

        using numa_binding_helper_ptr = std::shared_ptr<numa_binding_helper<T>>;

        // default constructor produces an unusable allocator
        numa_binding_allocator() = default;

        // construct without a memory binder function
        // only first touch or interleave policies are valid
        numa_binding_allocator(
            threads::hpx_hwloc_membind_policy policy, unsigned int flags)
          : policy_(policy)
          , flags_(flags)
        {
            nba_deb.debug("no binder function");
            HPX_ASSERT(
                policy_ != threads::hpx_hwloc_membind_policy::membind_user);
        }

        // construct using a memory binder function for placing pages
        // according to a user defined pattern using first touch policy
        numa_binding_allocator(numa_binding_helper_ptr bind_func,
            threads::hpx_hwloc_membind_policy policy, unsigned int flags)
          : binding_helper_(bind_func)
          , policy_(policy)
          , flags_(flags)
        {
            nba_deb.debug("allocator");
        }

        // copy constructor
        numa_binding_allocator(numa_binding_allocator const& rhs)
          : binding_helper_(rhs.binding_helper_)
          , policy_(rhs.policy_)
          , flags_(rhs.flags_)
        {
            nba_deb.debug("Copy allocator");
        }

        // copy constructor using rebind type
        template <typename U>
            requires(!std::same_as<T, U>)
        numa_binding_allocator(numa_binding_allocator<U> const& rhs)
          : binding_helper_(rhs.binding_helper_)
          , policy_(rhs.policy_)
          , flags_(rhs.flags_)
          , init_mutex()
        {
            nba_deb.debug("Copy allocator rebind");
        }

        // Move constructor
        numa_binding_allocator(numa_binding_allocator&& rhs) noexcept
          : binding_helper_(HPX_MOVE(rhs.binding_helper_))
          , policy_(rhs.policy_)
          , flags_(rhs.flags_)
          , init_mutex()
        {
            nba_deb.debug("Move constructor");
        }

        // Assignment operator
        numa_binding_allocator& operator=(numa_binding_allocator const& rhs)
        {
            binding_helper_ = rhs.binding_helper_;
            policy_ = rhs.policy_;
            flags_ = rhs.flags_;

            nba_deb.debug("Assignment operator");
            return *this;
        }

        // Move assignment
        numa_binding_allocator& operator=(numa_binding_allocator&& rhs) noexcept
        {
            binding_helper_ = rhs.binding_helper_;
            policy_ = rhs.policy_;
            flags_ = rhs.flags_;

            nba_deb.debug("Move assignment");
            return *this;
        }

        // Returns the actual address of x even in presence of overloaded
        // operator&
        static pointer address(reference x) noexcept
        {
            return &x;
        }

        // Returns the actual address of x even in presence of overloaded
        // operator&
        static const_pointer address(const_reference x) noexcept
        {
            return &x;
        }

        // Allocates n * sizeof(T) bytes of uninitialized storage and then
        // spawns threads to touch memory if membind_user is selected
        pointer allocate(size_type n)
        {
            pointer result = nullptr;

            if (policy_ ==
                threads::hpx_hwloc_membind_policy::membind_firsttouch)
            {
                threads::hwloc_bitmap_ptr const bitmap =
                    threads::get_thread_manager().get_pool_numa_bitmap(
                        binding_helper_->pool_name());
                //
                result =
                    static_cast<pointer>(threads::topology().allocate_membind(
                        n * sizeof(T), bitmap, policy_, 0));
                nba_deb.debug(debug::str<>("alloc:firsttouch"),
                    debug::hex<12, void*>(result));
            }
            else if (policy_ ==
                threads::hpx_hwloc_membind_policy::membind_interleave)
            {
                threads::hwloc_bitmap_ptr const bitmap =
                    threads::get_thread_manager().get_pool_numa_bitmap(
                        binding_helper_->pool_name());
                //
                result =
                    static_cast<pointer>(threads::topology().allocate_membind(
                        n * sizeof(T), bitmap, policy_, 0));
                nba_deb.debug(debug::str<>("alloc:interleave"),
                    debug::hex<12, void*>(result));
            }
            else if (policy_ == threads::hpx_hwloc_membind_policy::membind_user)
            {
                threads::hwloc_bitmap_ptr const bitmap =
                    threads::get_thread_manager().get_pool_numa_bitmap(
                        binding_helper_->pool_name());
                //
                result = static_cast<pointer>(
                    threads::topology().allocate_membind(n * sizeof(T), bitmap,
                        threads::hpx_hwloc_membind_policy::membind_firsttouch,
                        0));
                nba_deb.debug(debug::str<>("alloc:user(bind)"),
                    debug::hex<12, void*>(result));

#if defined(NUMA_ALLOCATOR_LINUX) && defined(MADV_NOHUGEPAGE)
                // if Transparent Huge Pages (THP) are enabled, this prevents
                // pages from being merged into a single numa bound block
                int ret = madvise(result, n * sizeof(T), MADV_NOHUGEPAGE);

                // a return of -1 probably means there are no transparent huge
                // pages, so they can't be disabled, we can ignore it
                if ((ret != 0) && (ret != -1))
                {
                    std::cerr << "ERROR: MADVISE " << strerror(ret)
                              << std::endl;
                }
#endif
                initialize_pages(result, n);
            }
            return result;
        }

        // Deallocates the storage referenced by the pointer p, which must be a
        // pointer obtained by an earlier call to allocate(). The argument n
        // must be equal to the first argument of the call to allocate() that
        // originally produced p; otherwise, the behavior is undefined.
        static void deallocate(pointer p, size_type n) noexcept
        {
            try
            {
                nba_deb.debug(debug::str<>("deallocate"),
                    "calling membind for size (bytes) ",
                    debug::hex<2>(n * sizeof(T)));
#ifdef NUMA_BINDING_ALLOCATOR_DEBUG_PAGE_BINDING
                display_binding(p, binding_helper_);
#endif
                threads::create_topology().deallocate(p, n * sizeof(T));
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
            catch (...)
            {
                // just ignore errors from create_topology
            }
        }

        // Returns the maximum theoretically possible value of n, for which the
        // call allocate(n, 0) could succeed. In most implementations, this
        // returns std::numeric_limits<size_type>::max() / sizeof(value_type).
        static size_type max_size() noexcept
        {
            return (std::numeric_limits<size_type>::max)();
        }

        // Constructs an object of type T in allocated uninitialized storage
        // pointed to by p, using placement-new
        template <typename U, typename... A>
        void construct(U* p, A&&... args)
        {
            hpx::construct_at(p, HPX_FORWARD(A, args)...);
        }

        template <class U>
        void destroy(U* const p)
        {
            std::destroy_at(p);
        }

        // a utility function that is slightly faster than the hwloc provided one
        // @TODO, move this into hpx::topology for cleanliness
        static int get_numa_domain(void* page)
        {
            HPX_ASSERT((reinterpret_cast<std::size_t>(page) &
                           (threads::get_memory_page_size() - 1)) == 0);

#if defined(NUMA_ALLOCATOR_LINUX)
            // This is an optimized version of the hwloc equivalent
            void* pages[1] = {page};
            int status[1] = {-1};
            if (syscall(__NR_move_pages, 0, 1, pages, nullptr, status, 0) == 0)
            {
                if (status[0] >= 0 &&
                    status[0] <= HPX_HAVE_MAX_NUMA_DOMAIN_COUNT)
                {
                    return status[0];
                }
                return -1;
            }
            HPX_THROW_EXCEPTION(hpx::error::kernel_error, "get_numa_domain",
                "Error getting numa domain with syscall");
#else
            return threads::get_topology().get_numa_domain(page);
#endif
        }

        static std::string get_page_numa_domains(
            [[maybe_unused]] void* addr, [[maybe_unused]] std::size_t len)
        {
#if defined(NUMA_ALLOCATOR_LINUX)
            // @TODO replace with topology::page_size
            int pagesize = static_cast<int>(threads::get_memory_page_size());
            HPX_ASSERT((std::size_t(addr) & (pagesize - 1)) == 0);

            std::size_t count = (len + pagesize - 1) / pagesize;
            std::vector<void*> pages(count, nullptr);
            std::vector<int> status(count, 0);

            for (std::size_t i = 0; i < count; i++)
                pages[i] = ((char*) addr) + i * pagesize;

            if (syscall(__NR_move_pages, 0, count, pages.data(), nullptr,
                    status.data(), 0) < 0)
            {
                HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                    "get_page_numa_domains",
                    "Error getting numa domains with syscall");
            }

            std::stringstream temp;
            temp << "Numa page binding for page count " << count << "\n";
            for (std::size_t i = 0; i < count; i++)
            {
                if (status[i] >= 0)
                    temp << status[i];
                else
                    temp << "-";
            }
            return temp.str();
#else
            // On platforms without NUMA support (e.g., macOS), return a fallback
            // string with all zeros, assuming a single NUMA domain
            std::size_t const pagesize = threads::get_memory_page_size();
            std::size_t const count = (len + pagesize - 1) / pagesize;
            std::stringstream temp;
            temp << "Numa page binding for page count " << count << "\n";
            for (std::size_t i = 0; i < count; i++)
            {
                temp << "0";
            }
            return temp.str();
#endif
        }

        void initialize_pages(pointer p, size_t n) const
        {
            std::unique_lock<std::mutex> lk(init_mutex);

            threads::hwloc_bitmap_ptr const bitmap =
                threads::get_thread_manager().get_pool_numa_bitmap(
                    binding_helper_->pool_name());
            std::vector<threads::hwloc_bitmap_ptr> nodesets =
                create_nodesets(bitmap);

            using namespace parallel::execution;
            using allocator_hint_type =
                hpx::execution::experimental::pool_numa_hint<
                    hpx::execution::experimental::numa_binding_allocator_tag>;

            // Warning :low priority tasks are used here, because the scheduler
            // does not steal across numa domains for those tasks, so they are
            // sure to remain on the right queue and be executed on the right
            // domain.
            hpx::execution::experimental::guided_pool_executor<
                allocator_hint_type>
                numa_executor(&hpx::resource::get_thread_pool(
                                  binding_helper_->pool_name()),
                    threads::thread_priority::bound);

            nba_deb.debug("Launching First-Touch tasks");
            // for each numa domain, we must launch a task to 'touch' the memory
            // that should be bound to the domain
            std::vector<hpx::future<void>> tasks;
            for (size_type i = 0; i < nodesets.size(); ++i)
            {
#ifdef NUMA_BINDING_ALLOCATOR_DEBUG_PAGE_BINDING
                size_type i1 = hwloc_bitmap_first(nodesets[i]->get_bmp());
                size_type i2 = hwloc_bitmap_last(nodesets[i]->get_bmp());
                HPX_ASSERT(i1 == i2);
#endif
                size_type domain = hwloc_bitmap_first(nodesets[i]->get_bmp());
                nba_deb.debug("Launching First-Touch task for domain ",
                    debug::dec<2>(domain), " ", nodesets[i]);
                auto f1 = hpx::async(numa_executor,
                    hpx::bind(&numa_binding_allocator::touch_pages, this, p, n,
                        binding_helper_, placeholders::_1, nodesets),
                    domain);
                nba_deb.debug(debug::str<>("First-Touch"),
                    "add task future to vector for domain ",
                    debug::dec<2>(domain), " ", nodesets[i]);
                tasks.push_back(HPX_MOVE(f1));
            }
            hpx::wait_all(tasks);
            nba_deb.debug(debug::str<>("First-Touch"), "Done tasks");
        }

        std::string display_binding(pointer p, numa_binding_helper_ptr helper)
        {
            std::unique_lock<std::mutex> lk(init_mutex);
            //
            std::ostringstream display;
            auto N = helper->array_rank();
            display << helper->description() << "\n";
            std::size_t pagesize = threads::get_memory_page_size();
            pointer p2 = p;
            if (N == 2)
            {
                std::size_t Nc = helper->array_size(0);
                std::size_t const Nr = helper->array_size(1);
                std::size_t xinc =
                    (std::min) (helper->display_step(0), pagesize);
                std::size_t const yinc =
                    (std::min) (helper->display_step(1), pagesize);
                std::size_t const xoff = helper->memory_step(0);
                std::size_t const yoff = helper->memory_step(1);
                std::size_t m = helper->memory_bytes();
#ifdef numa_binding_allocator_PRETTY_DISPLAY
                display << ' ';
                for (std::size_t c = 0; c < Nc; c += xinc)
                {
                    if (c % (xinc * 8) == 0)
                        display << '|';
                    else
                        display << '.';
                }
#endif
                display << '\n';
                for (std::size_t r = 0; r < Nr; r += yinc)
                {
#ifdef numa_binding_allocator_PRETTY_DISPLAY
                    if (r % (xinc * 8) == 0)
                        display << '-';
                    else
                        display << '.';
#endif
                    //
                    for (std::size_t c = 0; c < Nc; c += xinc)
                    {
                        p2 = p + (c * xoff) + (r * yoff);
                        if (p2 >= (p + m))
                        {
                            display << '*';
                        }
                        else
                        {
                            size_type const dom = get_numa_domain(p2);
                            if (dom == static_cast<size_type>(-1))
                            {
                                display << '-';
                            }
                            else
                                display << std::hex << dom;
                        }
                    }
                    display << "\n";
                }
            }
            else
            {
                display << "Not yet implemented output for non 2D arrays"
                        << "\n";
            }
            return display.str();
        }

    protected:
        static std::vector<threads::hwloc_bitmap_ptr> create_nodesets(
            threads::hwloc_bitmap_ptr const& bitmap)
        {
            // for each numa domain, we need a nodeset object
            threads::mask_type const numa_mask =
                threads::get_topology().bitmap_to_mask(
                    bitmap->get_bmp(), HWLOC_OBJ_NUMANODE);

            nba_deb.debug(debug::str<>("Pool numa mask"), numa_mask);

            std::vector<threads::hwloc_bitmap_ptr> nodesets;
            for (size_type i = 0; i < threads::mask_size(numa_mask); ++i)
            {
                if (threads::test(numa_mask, i))
                {
                    hwloc_bitmap_t bmp = hwloc_bitmap_alloc();
                    hwloc_bitmap_zero(bmp);
                    hwloc_bitmap_set(bmp, static_cast<unsigned>(i));
                    nodesets.push_back(
                        std::make_shared<threads::hpx_hwloc_bitmap_wrapper>(
                            bmp));
                    nba_deb.debug(
                        debug::str<>("Node mask"), i, " is ", nodesets.back());
                }
            }
            return nodesets;
        }

        void touch_pages(pointer p, size_t n, numa_binding_helper_ptr helper,
            size_type numa_domain,
            std::vector<threads::hwloc_bitmap_ptr> const& nodesets) const
        {
            size_type const pagesize = threads::get_memory_page_size();
            size_type const pageN = pagesize / sizeof(T);
            size_type const num_pages =
                (n * sizeof(T) + pagesize - 1) / pagesize;
            pointer page_ptr = p;
            HPX_ASSERT(reinterpret_cast<std::intptr_t>(p) % pagesize == 0);

            nba_deb.debug(debug::str<>("Touch pages"), "for numa ",
                debug::dec<2>(numa_domain));
            for (size_type i = 0; i < num_pages; ++i)
            {
                // we pass the base pointer and current page pointer
                size_type const dom =
                    helper->operator()(p, page_ptr, pagesize, nodesets.size());
                if (dom == numa_domain)
                {
                    HPX_ASSERT((reinterpret_cast<std::size_t>(page_ptr) &
                                   (threads::get_memory_page_size() - 1)) == 0);
                    // Write the first byte of the page to trigger first-touch
                    // without reading uninitialized memory (which would be UB
                    // for non-trivial T and can be optimized away by the
                    // compiler even for trivial types).
                    *reinterpret_cast<char volatile*>(page_ptr) = 0;
#ifdef NUMA_BINDING_ALLOCATOR_INIT_MEMORY
                    // show which cpu is actually being used
#if defined(NUMA_ALLOCATOR_LINUX)
                    std::size_t Vmem = sched_getcpu();
#else
                    std::size_t Vmem = numa_domain;
#endif
                    pointer elem_ptr = page_ptr;
                    for (size_type j = 0; j < pageN; ++j)
                    {
                        *elem_ptr++ = T(Vmem);
                    }
#endif
                }
                page_ptr += pageN;
            }
        }

        // This is obsolete but kept for possible future use
        void bind_pages(pointer p, size_t n, numa_binding_helper_ptr helper,
            size_type numa_domain,
            std::vector<threads::hwloc_bitmap_ptr> const& nodesets) const
        {
            size_type const pagesize = threads::get_memory_page_size();
            size_type const pageN = pagesize / sizeof(T);
            size_type const num_pages =
                (n * sizeof(T) + pagesize - 1) / pagesize;
            pointer page_ptr = p;
            HPX_ASSERT(reinterpret_cast<std::intptr_t>(p) % pagesize == 0);

            nba_deb.debug(debug::str<>("Bind pages "), "for numa ",
                debug::dec<2>(numa_domain));
            for (size_type i = 0; i < num_pages; ++i)
            {
                // we pass the base pointer and current page pointer
                size_type const dom =
                    helper->operator()(p, page_ptr, pagesize, nodesets.size());
                if (dom == numa_domain)
                {
                    threads::topology().set_area_membind_nodeset(
                        page_ptr, pagesize, nodesets[dom]->get_bmp());
                }
                page_ptr += pageN;
            }
        }

    public:
        // return the binding helper cast to a specific type
        template <typename Binder>
        std::shared_ptr<Binder> get_binding_helper_cast() const
        {
            return std::dynamic_pointer_cast<Binder>(binding_helper_);
        }

        std::shared_ptr<numa_binding_helper<T>> binding_helper_;
        threads::hpx_hwloc_membind_policy policy_;
        unsigned int flags_;

    private:
        mutable std::mutex init_mutex;
    };
}    // namespace hpx::compute::host
