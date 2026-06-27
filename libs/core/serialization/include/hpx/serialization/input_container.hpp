//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/serialization/binary_filter.hpp>
#include <hpx/serialization/container.hpp>
#include <hpx/serialization/serialization_chunk.hpp>
#include <hpx/serialization/traits/serialization_access_data.hpp>

#include <cstddef>    // for size_t
#include <cstdint>
#include <memory>
#include <vector>

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename Container>
    struct input_container final : erased_input_container
    {
    private:
        using access_traits = traits::serialization_access_data<Container>;

        [[nodiscard]] constexpr std::size_t get_chunk_size(
            std::size_t const chunk) const noexcept
        {
            return (*chunks_)[chunk].size_;
        }

        [[nodiscard]] constexpr chunk_type get_chunk_type(
            std::size_t const chunk) const noexcept
        {
            return (*chunks_)[chunk].type_;
        }

        constexpr chunk_data const& get_chunk_data(
            std::size_t const chunk) const noexcept
        {
            return (*chunks_)[chunk].data_;
        }

        constexpr chunk_data& get_chunk_data(std::size_t const chunk) noexcept
        {
            return (*chunks_)[chunk].data_;
        }

        [[nodiscard]] constexpr std::size_t get_num_chunks() const noexcept
        {
            return chunks_->size();
        }

    public:
        input_container(
            Container const& cont, std::size_t const inbound_data_size) noexcept
          : cont_(cont)
          , current_(0)
          , decompressed_size_(inbound_data_size)
          , zero_copy_serialization_threshold_(
                HPX_ZERO_COPY_SERIALIZATION_THRESHOLD)
          , chunks_(nullptr)
          , current_chunk_(static_cast<std::size_t>(-1))
          , current_chunk_size_(0)
        {
        }

        input_container(Container const& cont,
            std::vector<serialization_chunk>* chunks,
            std::size_t const inbound_data_size) noexcept
          : cont_(cont)
          , current_(0)
          , decompressed_size_(inbound_data_size)
          , zero_copy_serialization_threshold_(
                HPX_ZERO_COPY_SERIALIZATION_THRESHOLD)
          , chunks_(nullptr)
          , current_chunk_(static_cast<std::size_t>(-1))
          , current_chunk_size_(0)
        {
            if (chunks && chunks->size() != 0)
            {
                chunks_ = chunks;
                current_chunk_ = 0;
            }
        }

        void set_filter(binary_filter* filter) override
        {
            filter_.reset(filter);
            if (filter)
            {
                current_ = access_traits::init_data(
                    cont_, filter_.get(), current_, decompressed_size_);

                if (decompressed_size_ < current_)
                {
                    HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                        "input_container::set_filter",
                        "archive data binary stream is too short");
                }
            }
        }

        void set_zero_copy_serialization_threshold(
            std::size_t const zero_copy_serialization_threshold) override
        {
            zero_copy_serialization_threshold_ =
                zero_copy_serialization_threshold;
            if (zero_copy_serialization_threshold_ == 0)
            {
                zero_copy_serialization_threshold_ =
                    HPX_ZERO_COPY_SERIALIZATION_THRESHOLD;
            }
        }

        std::size_t load_binary(detail::fundamental_types const expected,
            void* address, std::size_t count) override
        {
            HPX_ASSERT(address != nullptr || count == 0);

            std::size_t type_info_count = 0;
            if (filter_)
            {
                if (expected != detail::fundamental_types::none)
                {
                    detail::fundamental_types t;
                    filter_->load(&t, sizeof(t));
                    if (t != expected)
                    {
                        HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                            "input_container::load_binary",
                            "Unexpected type found in received buffer (actual: "
                            "{}, expected: {}).",
                            static_cast<std::uint8_t>(t),
                            static_cast<std::uint8_t>(expected));
                    }
                    type_info_count = sizeof(t);
                }

                if (address)
                {
                    filter_->load(address, count);
                }
                return count + type_info_count;
            }

            if (expected != detail::fundamental_types::none)
            {
                type_info_count = sizeof(detail::fundamental_types);
            }

            std::size_t new_current = current_ + count + type_info_count;
            if (new_current > access_traits::size(cont_))
            {
                HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                    "input_container::load_binary",
                    "archive data binary stream is too short");
            }

            if (expected != detail::fundamental_types::none)
            {
                detail::fundamental_types t;
                access_traits::read(cont_, sizeof(t), current_, &t);
                if (t != expected)
                {
                    HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                        "input_container::load_binary",
                        "Unexpected type found in received buffer (actual: "
                        "{}, expected: {}).",
                        static_cast<std::uint8_t>(t),
                        static_cast<std::uint8_t>(expected));
                }
            }

            if (address)
            {
                access_traits::read(
                    cont_, count, current_ + type_info_count, address);
            }

            current_ = new_current;

            count += type_info_count;
            if (chunks_ != nullptr)
            {
                current_chunk_size_ += count;

                // make sure we switch to the next serialization_chunk if
                // necessary
                std::size_t const current_chunk_size =
                    get_chunk_size(current_chunk_);
                if (current_chunk_size != 0 &&
                    current_chunk_size_ >= current_chunk_size)
                {
                    // raise an error if we read past the serialization_chunk
                    if (current_chunk_size_ > current_chunk_size)
                    {
                        HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                            "input_container::load_binary",
                            "archive data binary stream structure "
                            "mismatch");
                    }
                    ++current_chunk_;
                    current_chunk_size_ = 0;
                }
            }

            return count;
        }

        std::size_t load_binary_chunk(detail::fundamental_types const expected,
            void* address, std::size_t const count,
            bool const allow_zero_copy_receive) override
        {
            HPX_ASSERT(static_cast<std::int64_t>(count) >= 0);

            if (chunks_ == nullptr ||
                count < zero_copy_serialization_threshold_ || filter_)
            {
                // fall back to serialization_chunk-less archive
                return this->input_container::load_binary(
                    expected, address, count);
            }

            // verify type information if needed
            std::size_t type_info_size = 0;
            if (expected != detail::fundamental_types::none)
            {
                type_info_size =
                    this->input_container::load_binary(expected, nullptr, 0);
            }

            HPX_ASSERT(current_chunk_ != static_cast<std::size_t>(-1));
            HPX_ASSERT(get_chunk_type(current_chunk_) ==
                chunk_type::chunk_type_pointer);

            if (get_chunk_size(current_chunk_) != count)
            {
                HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                    "input_container::load_binary_chunk",
                    "archive data binary stream data chunk size mismatch");
            }

            auto*& buffer = get_chunk_data(current_chunk_).pos_;
            if (allow_zero_copy_receive)
            {
                // If the receiving end supports zero-copy serialization of
                // larger chunks, the de-serialization pass should not copy
                // the data, but simply return the address of the buffer
                // where the data will be placed by the networking layer.
                HPX_ASSERT(buffer == nullptr);
                buffer = address;
            }
            else
            {
                // Unfortunately we can't implement a zero-copy policy on
                // the receiving end as the parcelport doesn't support this.
                // The memory was already allocated by the serialization
                // code, thus we copy the received data.
                HPX_ASSERT(buffer != nullptr);
                traits::detail::copy_serialized_data(address, buffer, count);
            }
            ++current_chunk_;

            // we have read data from the main serialization chunk only if the
            // type information was stored
            return type_info_size;
        }

        Container const& cont_;
        std::size_t current_;
        std::unique_ptr<binary_filter> filter_;
        std::size_t decompressed_size_;
        std::size_t zero_copy_serialization_threshold_;

        std::vector<serialization_chunk>* chunks_;
        std::size_t current_chunk_;
        std::size_t current_chunk_size_;
    };
}    // namespace hpx::serialization
