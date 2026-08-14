//  Copyright (c) 2023-2024 Jiakun Yan
//  Copyright (c) 2013-2014 Hartmut Kaiser
//  Copyright (c) 2013-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)
#include <hpx/assert.hpp>
#include <hpx/modules/parcelset.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>

namespace hpx::parcelset::policies::lci {
    struct header
    {
        struct header_format_t
        {
            // signature for assert_valid
            int signature;
            // tag
            int tag;
            // non-zero-copy chunk size
            int numbytes_tchunk;
            // transmission chunk size
            size_t numbytes_nonzero_copy;
            // how many bytes in total (including zero-copy and non-zero-copy chunks)
            size_t numbytes;
            // zero-copy chunk number
            int numchunks_zero_copy;
            // non-zero-copy chunk number
            int numchunks_nonzero_copy;
            // whether piggyback data
            bool piggy_back_flag_data;
            // whether piggyback transmission chunk
            bool piggy_back_flag_tchunk;
        };

        template <typename buffer_type, typename ChunkType>
        static size_t get_header_size(
            parcel_buffer<buffer_type, ChunkType> const& buffer,
            size_t max_header_size) noexcept
        {
            HPX_ASSERT(max_header_size >= sizeof(header_format_t));

            size_t current_header_size = sizeof(header_format_t);
            if (buffer.data_.size() <= (max_header_size - current_header_size))
            {
                current_header_size += buffer.data_.size();
            }
            int num_zero_copy_chunks = buffer.num_chunks_.first;
            [[maybe_unused]] int num_non_zero_copy_chunks =
                buffer.num_chunks_.second;
            if (num_zero_copy_chunks != 0)
            {
                HPX_ASSERT(buffer.transmission_chunks_.size() ==
                    size_t(num_zero_copy_chunks + num_non_zero_copy_chunks));
                size_t tchunk_size = buffer.transmission_chunks_.size() *
                    sizeof(typename parcel_buffer<buffer_type,
                        ChunkType>::transmission_chunk_type);
                if (tchunk_size <= max_header_size - current_header_size)
                {
                    current_header_size += tchunk_size;
                }
            }
            return current_header_size;
        }

        template <typename buffer_type, typename ChunkType>
        header(parcel_buffer<buffer_type, ChunkType> const& buffer,
            void* header_buffer, size_t max_header_size) noexcept
        {
            HPX_ASSERT(max_header_size >= sizeof(header_format_t));
            data_ = static_cast<char*>(header_buffer);
            header_format_t* p_format_ =
                reinterpret_cast<header_format_t*>(data_);
            p_format_ = reinterpret_cast<header_format_t*>(header_buffer);
            memset(data_, 0, sizeof(header_format_t));
            size_t size = buffer.data_.size();
            size_t numbytes = buffer.data_size_;
            HPX_ASSERT(
                buffer.num_chunks_.first <= (std::numeric_limits<int>::max)());
            HPX_ASSERT(
                buffer.num_chunks_.second <= (std::numeric_limits<int>::max)());
            int num_zero_copy_chunks = buffer.num_chunks_.first;
            int num_non_zero_copy_chunks = buffer.num_chunks_.second;

            p_format_->signature = MAGIC_SIGNATURE;
            p_format_->numbytes_nonzero_copy = size;
            p_format_->numbytes = numbytes;
            p_format_->numchunks_zero_copy = num_zero_copy_chunks;
            p_format_->numchunks_nonzero_copy = num_non_zero_copy_chunks;
            p_format_->piggy_back_flag_data = false;
            p_format_->piggy_back_flag_tchunk = false;

            size_t current_header_size = sizeof(header_format_t);
            if (buffer.data_.size() <= (max_header_size - current_header_size))
            {
                p_format_->piggy_back_flag_data = true;
                std::memcpy(
                    &data_[current_header_size], &buffer.data_[0], size);
                current_header_size += size;
            }
            if (num_zero_copy_chunks != 0)
            {
                HPX_ASSERT(buffer.transmission_chunks_.size() ==
                    size_t(num_zero_copy_chunks + num_non_zero_copy_chunks));
                size_t tchunk_size = buffer.transmission_chunks_.size() *
                    sizeof(typename parcel_buffer<buffer_type,
                        ChunkType>::transmission_chunk_type);
                HPX_ASSERT(tchunk_size <= (std::numeric_limits<int>::max)());
                p_format_->numbytes_tchunk = tchunk_size;
                if (tchunk_size <= max_header_size - current_header_size)
                {
                    p_format_->piggy_back_flag_tchunk = true;
                    std::memcpy(&data_[current_header_size],
                        buffer.transmission_chunks_.data(), tchunk_size);
                }
            }
        }

        header() noexcept
        {
            data_ = nullptr;
        }

        explicit header(char* header_buffer) noexcept
        {
            data_ = static_cast<char*>(header_buffer);
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return data_ != nullptr && signature() == MAGIC_SIGNATURE;
        }

        void assert_valid() const noexcept
        {
            HPX_ASSERT(valid());
        }

        [[nodiscard]] char* data() noexcept
        {
            return data_;
        }

        [[nodiscard]] size_t size() noexcept
        {
            return sizeof(header_format_t) + piggy_back_size();
        }

        [[nodiscard]] int signature() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)->signature;
        }

        void set_tag(::lci::tag_t tag) noexcept
        {
            reinterpret_cast<header_format_t*>(data_)->tag = tag;
        }

        [[nodiscard]] int get_tag() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)->tag;
        }

        [[nodiscard]] size_t numbytes_nonzero_copy() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)
                ->numbytes_nonzero_copy;
        }

        [[nodiscard]] int numbytes_tchunk() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)->numbytes_tchunk;
        }

        [[nodiscard]] size_t numbytes() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)->numbytes;
        }

        [[nodiscard]] int num_zero_copy_chunks() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)
                ->numchunks_zero_copy;
        }

        [[nodiscard]] int num_non_zero_copy_chunks() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)
                ->numchunks_nonzero_copy;
        }

        [[nodiscard]] bool piggy_back_flag_data() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)
                ->piggy_back_flag_data;
        }

        [[nodiscard]] bool piggy_back_flag_tchunk() const noexcept
        {
            return reinterpret_cast<header_format_t*>(data_)
                ->piggy_back_flag_tchunk;
        }

        [[nodiscard]] char* piggy_back_address() noexcept
        {
            if (piggy_back_flag_data() || piggy_back_flag_tchunk())
                return &data_[sizeof(header_format_t)];
            return nullptr;
        }

        [[nodiscard]] size_t piggy_back_size() const noexcept
        {
            size_t result = 0;
            if (piggy_back_flag_data())
                result += numbytes_nonzero_copy();
            if (piggy_back_flag_tchunk())
                result += numbytes_tchunk();
            return result;
        }

        [[nodiscard]] char* piggy_back_data() const noexcept
        {
            if (piggy_back_flag_data())
                return &data_[sizeof(header_format_t)];
            return nullptr;
        }

        [[nodiscard]] char* piggy_back_tchunk() const noexcept
        {
            size_t current_header_size = sizeof(header_format_t);
            if (!piggy_back_flag_tchunk())
                return nullptr;
            if (piggy_back_flag_data())
                current_header_size += numbytes_nonzero_copy();
            return &data_[current_header_size];
        }

    private:
        // random magic number for assert_valid
        static constexpr int MAGIC_SIGNATURE = 19527;
        char* data_;
    };
}    // namespace hpx::parcelset::policies::lci

#endif
