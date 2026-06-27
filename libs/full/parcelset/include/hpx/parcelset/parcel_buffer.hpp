//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/serialization.hpp>

#include <hpx/modules/parcelset_base.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace hpx::parcelset {

    HPX_CXX_EXPORT template <typename BufferType = std::vector<char>,
        typename ChunkType = serialization::serialization_chunk>
    struct parcel_buffer
    {
        using count_chunks_type = std::pair<std::uint32_t, std::uint32_t>;
        using transmission_chunk_type = std::pair<std::uint64_t, std::uint64_t>;
        using allocator_type = typename BufferType::allocator_type;

        explicit parcel_buffer(
            allocator_type const& allocator = allocator_type())
          : data_(allocator)
          , num_chunks_(count_chunks_type(0, 0))
          , size_(0)
          , data_size_(0)
          , header_size_(0)
        {
        }

        explicit parcel_buffer(BufferType const& data,
            allocator_type const& allocator = allocator_type())
          : data_(data, allocator)
          , num_chunks_(count_chunks_type(0, 0))
          , size_(0)
          , data_size_(0)
          , header_size_(0)
        {
        }

        explicit parcel_buffer(BufferType&& data,
            allocator_type const& /*allocator*/ = allocator_type())
          : data_(HPX_MOVE(data))
          , num_chunks_(count_chunks_type(0, 0))
          , size_(0)
          , data_size_(0)
          , header_size_(0)
        {
        }

        parcel_buffer(parcel_buffer&& other) = default;
        parcel_buffer& operator=(parcel_buffer&& other) = default;

        void clear()
        {
            data_.clear();
            chunks_.clear();
            transmission_chunks_.clear();
            num_chunks_ = count_chunks_type(0, 0);
            size_ = 0;
            data_size_ = 0;
            header_size_ = 0;
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            data_point_ = parcelset::data_point();
#endif
        }

        BufferType data_;

        std::vector<ChunkType> chunks_;
        std::vector<transmission_chunk_type> transmission_chunks_;

        // pair of (zero-copy, non-zero-copy) chunks
        count_chunks_type num_chunks_;

        // non-zero-copy chunk size (data_.size())
        std::uint64_t size_;

        // how many bytes in total (including zero-copy and non-zero-copy chunks)
        std::uint64_t data_size_;
        std::uint64_t header_size_;

        /// Counters and their data containers.
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        parcelset::data_point data_point_;
#endif
    };
}    // namespace hpx::parcelset

#endif
