//  Copyright (c) 1998-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components_base/server/wrapper_heap_base.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/itt_notify.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

#define HPX_DEBUG_WRAPPER_HEAP 0

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::detail {

    ///////////////////////////////////////////////////////////////////////////
    namespace one_size_heap_allocators {

        ///////////////////////////////////////////////////////////////////////
        // simple allocator which gets the memory from the default malloc,
        // but which does not reallocate the heap (it doesn't grow)
        struct fixed_mallocator
        {
            static void* alloc(std::size_t size)
            {
                return alloc_.allocate(size);
            }
            static void free(void* p, std::size_t count) noexcept
            {
                alloc_.deallocate(static_cast<char*>(p), count);
            }
            static constexpr void* realloc(std::size_t&, void*) noexcept
            {
                // normally this should return ::realloc(p, size), but we are
                // not interested in growing the allocated heaps, so we just
                // return nullptr
                return nullptr;
            }

            HPX_EXPORT static util::internal_allocator<char> alloc_;
        };
    }    // namespace one_size_heap_allocators

    ///////////////////////////////////////////////////////////////////////////
    class HPX_EXPORT wrapper_heap : public util::wrapper_heap_base
    {
    public:
        using allocator_type = one_size_heap_allocators::fixed_mallocator;
        using mutex_type = hpx::spinlock;
        using heap_parameters = wrapper_heap_base::heap_parameters;

#if HPX_DEBUG_WRAPPER_HEAP != 0
        enum guard_value
        {
            initial_value = 0xcc,    // memory has been initialized
            freed_value = 0xdd,      // memory has been freed
        };
#endif

        explicit wrapper_heap(char const* class_name, std::size_t count,
            heap_parameters const& parameters);

        wrapper_heap();

        wrapper_heap(wrapper_heap const&) = delete;
        wrapper_heap(wrapper_heap&&) = delete;
        wrapper_heap& operator=(wrapper_heap const&) = delete;
        wrapper_heap& operator=(wrapper_heap&&) = delete;

        ~wrapper_heap() override;

        std::size_t size() const override;
        std::size_t free_size() const override;

        bool is_empty() const;
        bool has_allocatable_slots() const;

        bool alloc(void** result, std::size_t count = 1) override;
        void free(void* p, std::size_t count = 1) override;
        bool did_alloc(void* p) const override;

        // Get the global id of the managed_component instance given by the
        // parameter p.
        //
        // The pointer given by the parameter p must have been allocated by
        // this instance of a wrapper_heap
        naming::gid_type get_gid(
            void* p, components::component_type type) override;

        void set_gid(naming::gid_type const& g);

    protected:
        bool free_pool();

        bool init_pool();
        void tidy();

        char* pool_ = nullptr;
        heap_parameters const parameters_;
        util::cache_aligned_data_derived<std::atomic<char*>> first_free_;
        util::cache_aligned_data_derived<std::atomic<std::size_t>> free_size_;

        // these values are used for AGAS registration of all elements of this
        // managed_component heap
        mutable util::cache_aligned_data_derived<mutex_type> mtx_;
        naming::gid_type base_gid_;

    public:
        std::string const class_name_;
#if defined(HPX_DEBUG)
        std::size_t alloc_count_ = 0;
        std::size_t free_count_ = 0;
        std::size_t heap_count_ = 0;
#endif

        std::size_t heap_count() const override;

    private:
        HPX_NO_UNIQUE_ADDRESS util::itt::heap_function heap_alloc_function_;
        HPX_NO_UNIQUE_ADDRESS util::itt::heap_function heap_free_function_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // heap using malloc and friends
    template <typename T>
    class fixed_wrapper_heap : public wrapper_heap
    {
        using base_type = wrapper_heap;
        using heap_parameters = base_type::heap_parameters;

    public:
        using value_type = T;

        explicit fixed_wrapper_heap(char const* class_name, std::size_t count,
            heap_parameters const& parameters)
          : base_type(class_name, count, parameters)
        {
        }
    };
}    // namespace hpx::components::detail

#include <hpx/config/warnings_suffix.hpp>
