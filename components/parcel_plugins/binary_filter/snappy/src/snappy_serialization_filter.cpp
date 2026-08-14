//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_COMPRESSION_SNAPPY)
#include <snappy.h>

#include <hpx/modules/errors.hpp>
#include <hpx/modules/plugin_factories.hpp>

#include <hpx/binary_filter/snappy_serialization_filter.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_PLUGIN_MODULE();
HPX_REGISTER_BINARY_FILTER_FACTORY(
    hpx::plugins::compression::snappy_serialization_filter,
    snappy_serialization_filter);

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins::compression {

    void snappy_serialization_filter::set_max_length(std::size_t size)
    {
        buffer_.reserve(size);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::size_t snappy_serialization_filter::init_data(
        void const* buffer, std::size_t size, std::size_t buffer_size)
    {
        buffer_.resize(buffer_size);
        snappy::RawUncompress(
            static_cast<char const*>(buffer), size, buffer_.data());
        current_ = 0;
        return buffer_.size();
    }

    ///////////////////////////////////////////////////////////////////////////
    void snappy_serialization_filter::load(void* dst, std::size_t dst_count)
    {
        if (current_ + dst_count > buffer_.size())
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "snappy_serialization_filter::load",
                "archive data bstream is too short");
            return;
        }

        std::memcpy(dst, &buffer_[current_], dst_count);
        current_ += dst_count;
    }

    ///////////////////////////////////////////////////////////////////////////
    void snappy_serialization_filter::save(
        void const* src, std::size_t src_count)
    {
        char const* src_begin = static_cast<char const*>(src);
        std::copy(
            src_begin, src_begin + src_count, std::back_inserter(buffer_));
    }

    ///////////////////////////////////////////////////////////////////////////
    bool snappy_serialization_filter::flush(
        void* dst, std::size_t dst_count, std::size_t& written)
    {
        // make sure we have enough memory
        std::size_t needed = snappy::MaxCompressedLength(buffer_.size());
        if (needed > dst_count)
        {
            written = 0;
            return false;
        }

        // compress everything in one go
        char* dst_begin = static_cast<char*>(dst);
        char const* src_begin = buffer_.data();
        size_t compressed_length = 0;
        snappy::RawCompress(
            src_begin, buffer_.size(), dst_begin, &compressed_length);

        if (compressed_length > dst_count)
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "snappy_serialization_filter::flush",
                "compression failure, flushing did not reach end of data");
            return false;
        }

        written = compressed_length;
        return true;
    }
}    // namespace hpx::plugins::compression

#endif
