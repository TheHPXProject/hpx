//  Copyright (c) 1998-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/component_type.hpp>
#include <hpx/components_base/server/one_size_heap_list.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::detail {

    ///////////////////////////////////////////////////////////////////////////
    // list of managed_component heaps
    HPX_CXX_EXPORT template <typename Heap>
    class wrapper_heap_list : public util::one_size_heap_list
    {
        using base_type = util::one_size_heap_list;
        using value_type = typename Heap::value_type;

        using storage_type = hpx::aligned_storage_t<sizeof(value_type),
            std::alignment_of_v<value_type>>;

        enum
        {
            // default initial number of elements
            heap_capacity = 0xFFF,
            // Alignment of one element
            heap_element_alignment = std::alignment_of_v<value_type>,    //-V103
            // size of one element in the heap
            heap_element_size = sizeof(storage_type)
        };

    public:
        wrapper_heap_list()
          : base_type(
                get_component_type_name(
                    get_component_type<typename value_type::wrapped_type>()),
                base_type::heap_parameters{
                    heap_capacity, heap_element_alignment, heap_element_size},
                static_cast<Heap*>(nullptr))
          , type_(get_component_type<typename value_type::wrapped_type>())
        {
        }

        naming::gid_type get_gid(void* p) const
        {
            std::shared_lock<hpx::shared_mutex> sl(rwlock_);
            for (auto const& e : heap_list_)
            {
                if (e->did_alloc(p))
                {
                    return e->get_gid(p, type_);
                }
            }
            return naming::invalid_gid;
        }

    private:
        components::component_type type_;
    };
}    // namespace hpx::components::detail
