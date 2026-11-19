//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>

#include <climits>
#include <cstddef>
#include <cstdint>

#if CHAR_BIT != 8
#error This code assumes an eight-bit byte.
#endif

namespace hpx::serialization {

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT union chunk_data
    {
        std::size_t index_;    // position inside the data buffer //-V117
        void const* cpos_;     // const pointer to external data buffer //-V117
        void* pos_;            // pointer to external data buffer //-V117
    };

    HPX_CXX_CORE_EXPORT enum class chunk_type : std::uint8_t {
        chunk_type_index = 0,
        chunk_type_const_pointer = 1,
        chunk_type_pointer = 2
    };

    /// The memory a pointer chunk refers to. Parcelports and the transports
    /// below them must not read or write device memory from the host: they
    /// either hand it to a network library that handles device pointers or
    /// stage it through host memory. The sender marks the chunks of device
    /// memory it hands over, the receiving side the chunks it receives into
    /// device memory (see allow_device_receive). Only the local side's memory
    /// is recorded, it is not sent.
    HPX_CXX_CORE_EXPORT enum class chunk_memory : std::uint8_t {
        host = 0,
        device = 1
    };

    HPX_CXX_CORE_EXPORT struct serialization_chunk
    {
        chunk_data data_;     // index or pointer
        std::size_t size_;    // size of serialization_chunk starting pos_
        chunk_type type_;     // chunk_type
        chunk_memory memory_ = chunk_memory::host;    // pointer chunks only

        /// Active union member depends on \a type_: use this for both
        /// pointer kinds (mutable storage is returned as \c void const*).
        [[nodiscard]] constexpr void const* data() const noexcept
        {
            HPX_ASSERT(type_ == chunk_type::chunk_type_pointer ||
                type_ == chunk_type::chunk_type_const_pointer);
            if (type_ == chunk_type::chunk_type_const_pointer)
            {
                return data_.cpos_;
            }
            return data_.pos_;
        }

        /// Mutable view; only valid for \c chunk_type_pointer chunks.
        [[nodiscard]] constexpr void* data() noexcept
        {
            HPX_ASSERT(type_ == chunk_type::chunk_type_pointer);
            return data_.pos_;
        }

        [[nodiscard]] constexpr std::size_t size() const noexcept
        {
            return size_;
        }

        /// Whether the chunk refers to device memory.
        [[nodiscard]] constexpr bool is_device_memory() const noexcept
        {
            return memory_ == chunk_memory::device;
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr serialization_chunk
    create_index_chunk(std::size_t index, std::size_t size) noexcept
    {
        serialization_chunk retval = {{0}, size, chunk_type::chunk_type_index};
        retval.data_.index_ = index;
        return retval;
    }

    /// Zero-copy chunk referring to immutable caller-owned memory (save path).
    /// Parcelports must treat this like \c chunk_type_pointer for transmission.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr serialization_chunk
    create_const_pointer_chunk(void const* pos, std::size_t size,
        chunk_memory memory = chunk_memory::host) noexcept
    {
        serialization_chunk retval = {
            {0}, size, chunk_type::chunk_type_const_pointer, memory};
        retval.data_.cpos_ = pos;
        return retval;
    }

    /// Zero-copy chunk referring to mutable memory (typically receive path).
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr serialization_chunk
    create_pointer_chunk(void* pos, std::size_t size,
        chunk_memory memory = chunk_memory::host) noexcept
    {
        serialization_chunk retval = {
            {0}, size, chunk_type::chunk_type_pointer, memory};
        retval.data_.pos_ = pos;
        return retval;
    }

    /// Backward-compatible overload for const storage; same as
    /// \c create_const_pointer_chunk.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr serialization_chunk
    create_pointer_chunk(void const* pos, std::size_t size) noexcept
    {
        return create_const_pointer_chunk(pos, size);
    }

    /// Create a mutable-pointer chunk with a null address.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr serialization_chunk
    create_pointer_chunk(std::nullptr_t, std::size_t size) noexcept
    {
        return create_pointer_chunk(static_cast<void*>(nullptr), size);
    }
}    // namespace hpx::serialization
