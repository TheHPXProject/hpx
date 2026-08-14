//  Copyright (c) 1998-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/server/wrapper_heap_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <cstddef>
#include <list>
#include <memory>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    HPX_CXX_EXPORT class HPX_EXPORT one_size_heap_list
    {
    public:
        using list_type = std::list<std::shared_ptr<util::wrapper_heap_base>>;
        using iterator = list_type::iterator;
        using const_iterator = list_type::const_iterator;

        using mutex_type = hpx::shared_mutex;

        using heap_parameters = wrapper_heap_base::heap_parameters;

    private:
        template <typename Heap>
        static std::shared_ptr<util::wrapper_heap_base> create_heap(
            char const* name, [[maybe_unused]] std::size_t counter,
            heap_parameters parameters)
        {
#if defined(HPX_DEBUG)
            return std::make_shared<Heap>(name, counter, parameters);
#else
            return std::make_shared<Heap>(name, 0, parameters);
#endif
        }

    public:
        one_size_heap_list();

        template <typename Heap>
        explicit one_size_heap_list(
            char const* class_name, heap_parameters parameters, Heap* = nullptr)
          : class_name_(class_name)
          , create_heap_(&one_size_heap_list::create_heap<Heap>)
          , parameters_(parameters)
        {
        }

        template <typename Heap>
        explicit one_size_heap_list(std::string const& class_name,
            heap_parameters parameters, Heap* = nullptr)
          : class_name_(class_name)
          , create_heap_(&one_size_heap_list::create_heap<Heap>)
          , parameters_(parameters)
        {
        }

        ~one_size_heap_list() noexcept;

        // operations
        void* alloc(std::size_t count = 1);

        // need to reschedule if not using boost::mutex
        bool reschedule(void* p, std::size_t count);

        void free(void* p, std::size_t count = 1);

        bool did_alloc(void* p) const;

        std::string name() const;

    protected:
        mutable mutex_type rwlock_;
        list_type heap_list_;

    private:
        std::string const class_name_;

    public:
#if defined(HPX_DEBUG)
        std::size_t alloc_count_ = 0;
        std::size_t free_count_ = 0;
        std::size_t heap_count_ = 0;
        std::size_t max_alloc_count_ = 0;
#endif
        std::shared_ptr<util::wrapper_heap_base> (*create_heap_)(
            char const*, std::size_t, heap_parameters) = nullptr;

        heap_parameters const parameters_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
