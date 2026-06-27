//  Copyright (c) 1998-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/component_type.hpp>

#include <cstddef>

namespace hpx::util {

    HPX_CXX_EXPORT struct wrapper_heap_base
    {
        struct heap_parameters
        {
            std::size_t capacity = 0;
            std::size_t element_alignment = 0;
            std::size_t element_size = 0;
        };

        virtual ~wrapper_heap_base() = default;

        virtual bool alloc(void** result, std::size_t count = 1) = 0;
        virtual bool did_alloc(void* p) const = 0;
        virtual void free(void* p, std::size_t count = 1) = 0;

        virtual naming::gid_type get_gid(
            void* p, components::component_type type) = 0;

        virtual std::size_t heap_count() const = 0;
        virtual std::size_t size() const = 0;
        virtual std::size_t free_size() const = 0;
    };
}    // namespace hpx::util
