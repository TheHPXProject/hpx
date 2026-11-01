//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2005-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/iostream/read.hpp>
#include <hpx/iostream/seek.hpp>
#include <hpx/iostream/traits.hpp>
#include <hpx/iostream/write.hpp>

#include <ios>

namespace hpx::iostream {

    HPX_CXX_CORE_EXPORT template <typename Device>
    class non_blocking_adapter
    {
    public:
        using char_type = char_type_of_t<Device>;

        struct category
          : mode_of_t<Device>
          , device_tag
        {
        };

        explicit non_blocking_adapter(Device& dev) noexcept
          : device_(dev)
        {
        }

        std::streamsize read(char_type* s, std::streamsize const n)
        {
            std::streamsize result = 0;
            while (result < n)
            {
                std::streamsize const amt =
                    iostream::read(device_, s + result, n - result);
                if (amt == -1)
                    break;
                result += amt;
            }
            return result != 0 ? result : -1;
        }

        std::streamsize write(char_type const* s, std::streamsize n)
        {
            std::streamsize result = 0;
            while (result < n)
            {
                std::streamsize const amt =
                    iostream::write(device_, s + result, n - result);

                // write errors, like EOF on read, need to be handled.
                if (amt == -1)
                    break;
                result += amt;
            }
            return result;
        }

        std::streampos seek(stream_offset off, std::ios_base::seekdir way,
            std::ios_base::openmode which = std::ios_base::in |
                std::ios_base::out)
        {
            return iostream::seek(device_, off, way, which);
        }

    public:
        Device& device_;
    };
}    // namespace hpx::iostream
