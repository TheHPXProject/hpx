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

#include <hpx/modules/iostream.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cassert>
#include <ios>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::test {

    //
    // Model of Source which reads from an STL-compatible sequence
    // whose iterators are random-access iterators.
    //
    template <typename Container>
    class container_source
    {
    public:
        typedef typename Container::value_type char_type;
        typedef source_tag category;

        container_source(Container& container)
          : container_(&container)
          , pos_(0)
        {
        }

        container_source(container_source const&) = default;
        container_source(container_source&&) = default;
        container_source& operator=(container_source const&) = default;
        container_source& operator=(container_source&&) = default;

        ~container_source() = default;

        std::streamsize read(char_type* s, std::streamsize n)
        {
            using namespace std;
            std::streamsize amt =
                static_cast<std::streamsize>(container_->size() - pos_);
            std::streamsize result = (min) (n, amt);
            if (result != 0)
            {
                std::copy(container_->begin() + pos_,
                    container_->begin() + pos_ + result, s);
                pos_ += result;
                return result;
            }
            return -1;    // EOF
        }

        Container& container()
        {
            return *container_;
        }

    private:
        typedef typename Container::size_type size_type;
        Container* container_;
        size_type pos_;
    };

    //
    // Model of Sink which appends to an STL-compatible sequence.
    //
    template <typename Container>
    class container_sink
    {
    public:
        typedef typename Container::value_type char_type;
        typedef sink_tag category;

        container_sink(Container& container)
          : container_(&container)
        {
        }

        container_sink(container_sink const&) = default;
        container_sink(container_sink&&) = default;
        container_sink& operator=(container_sink const&) = default;
        container_sink& operator=(container_sink&&) = default;

        ~container_sink() = default;

        std::streamsize write(char_type const* s, std::streamsize n)
        {
            container_->insert(container_->end(), s, s + n);
            return n;
        }

        Container& container()
        {
            return *container_;
        }

    private:
        Container* container_;
    };

    //
    // Model of SeekableDevice which access an TL-compatible sequence whose
    // iterators are random-access iterators.
    //
    template <typename Container>
    class container_device
    {
    public:
        typedef typename Container::value_type char_type;
        typedef seekable_device_tag category;

        container_device(Container& container)
          : container_(&container)
          , pos_(0)
        {
        }

        container_device(container_device const&) = default;
        container_device(container_device&&) = default;
        container_device& operator=(container_device const&) = default;
        container_device& operator=(container_device&&) = default;

        ~container_device() = default;

        std::streamsize read(char_type* s, std::streamsize n)
        {
            using namespace std;
            std::streamsize amt =
                static_cast<std::streamsize>(container_->size() - pos_);
            std::streamsize result = (std::min) (n, amt);
            if (result != 0)
            {
                std::copy(container_->begin() + pos_,
                    container_->begin() + pos_ + result, s);
                pos_ += result;
                return result;
            }
            return -1;    // EOF
        }

        std::streamsize write(char_type const* s, std::streamsize n)
        {
            using namespace std;

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
            std::streamsize result = 0;
            if (pos_ != container_->size())
            {
                std::streamsize amt =
                    static_cast<std::streamsize>(container_->size() - pos_);
                result = (std::min) (n, amt);
                std::copy(s, s + result, container_->begin() + pos_);

                pos_ += result;
            }

            if (result < n)
            {
                container_->insert(container_->end(), s, s + n);
                pos_ = container_->size();
            }

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif
            return n;
        }

        stream_offset seek(stream_offset off, std::ios_base::seekdir way)
        {
            using namespace std;

            // Determine new value of pos_
            stream_offset next;
            if (way == std::ios_base::beg)
            {
                next = off;
            }
            else if (way == std::ios_base::cur)
            {
                next = pos_ + off;
            }
            else if (way == std::ios_base::end)
            {
                next = container_->size() + off - 1;
            }
            else
            {
                throw std::ios_base::failure("bad seek direction");
            }

            // Check for errors
            if (next < 0 ||
                next > static_cast<stream_offset>(container_->size()))
                throw std::ios_base::failure("bad seek offset");

            pos_ = next;
            return pos_;
        }

        Container& container()
        {
            return *container_;
        }

    private:
        typedef typename Container::size_type size_type;
        Container* container_;
        size_type pos_;
    };
}    // namespace hpx::iostream::test

#include <hpx/config/warnings_suffix.hpp>
