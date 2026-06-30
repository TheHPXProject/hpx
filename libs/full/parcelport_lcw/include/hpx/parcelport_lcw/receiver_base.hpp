//  Copyright (c) 2025 Jiakun Yan
//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCW)

#include <hpx/assert.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/parcelport_lcw/header.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <deque>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::lcw {
    class HPX_EXPORT parcelport;
    struct buffer_wrapper
    {
        struct fake_allocator
        {
        };
        using allocator_type = fake_allocator;
        void* ptr;
        size_t length;
        buffer_wrapper() = default;
        buffer_wrapper(buffer_wrapper const& wrapper) = default;
        buffer_wrapper& operator=(buffer_wrapper const& wrapper) = default;
        explicit buffer_wrapper(allocator_type const& alloc)
        {
            HPX_UNUSED(alloc);
            ptr = nullptr;
            length = 0;
        }
        buffer_wrapper(
            buffer_wrapper const& wrapper, allocator_type const& alloc)
        {
            HPX_UNUSED(alloc);
            ptr = wrapper.ptr;
            length = wrapper.length;
        }
        ~buffer_wrapper() {}
        char& operator[](size_t i) const
        {
            HPX_ASSERT(i < length);
            char* p = (char*) ptr;
            return p[i];
        }
        void* data() const
        {
            return ptr;
        }
        size_t size() const
        {
            return length;
        }
        void allocate(size_t size)
        {
            ptr = new char[size];
            length = size;
        }
        void free()
        {
            HPX_ASSERT(ptr != nullptr);
            delete[] (char*) ptr;
        }
    };

    struct receiver_base
    {
        using buffer_type = parcel_buffer;

        explicit receiver_base(parcelport* pp) noexcept
          : pp_(pp)
        {
        }

        virtual ~receiver_base() {}

        void run() noexcept {}

        virtual bool background_work() noexcept = 0;

    protected:
        parcelport* pp_;
    };

}    // namespace hpx::parcelset::policies::lcw

#endif
