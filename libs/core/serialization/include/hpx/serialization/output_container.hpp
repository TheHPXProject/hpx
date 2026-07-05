//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2014 Thomas Heller
//  Copyright (c)      2015 Anton Bikineev
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/serialization/binary_filter.hpp>
#include <hpx/serialization/container.hpp>
#include <hpx/serialization/serialization_chunk.hpp>
#include <hpx/serialization/traits/serialization_access_data.hpp>

#include <cstddef>    // for size_t
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::serialization {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT struct basic_chunker
        {
            explicit constexpr basic_chunker(
                std::vector<serialization_chunk>*) noexcept
            {
            }

            [[nodiscard]] static constexpr std::size_t get_chunk_size() noexcept
            {
                return 0;
            }

            static constexpr void set_chunk_size(std::size_t) noexcept {}

            [[nodiscard]] static constexpr chunk_type get_chunk_type() noexcept
            {
                return chunk_type::chunk_type_index;
            }

            [[nodiscard]] static constexpr std::size_t
            get_chunk_data_index() noexcept
            {
                return 0;
            }

            [[nodiscard]] static constexpr std::size_t get_num_chunks() noexcept
            {
                return 1;
            }

            static constexpr void push_back(
                serialization_chunk&& /*chunk*/) noexcept
            {
            }

            static constexpr void reset() noexcept {}
        };

        HPX_CXX_CORE_EXPORT struct vector_chunker
        {
            explicit constexpr vector_chunker(
                std::vector<serialization_chunk>* chunks) noexcept
              : chunks_(chunks)
            {
            }

            [[nodiscard]] std::size_t get_chunk_size() const noexcept
            {
                return chunks_->back().size_;
            }

            void set_chunk_size(std::size_t const size) const noexcept
            {
                chunks_->back().size_ = size;
            }

            [[nodiscard]] chunk_type get_chunk_type() const noexcept
            {
                return chunks_->back().type_;
            }

            [[nodiscard]] std::size_t get_chunk_data_index() const noexcept
            {
                return chunks_->back().data_.index_;
            }

            [[nodiscard]] std::size_t get_num_chunks() const noexcept
            {
                return chunks_->size();
            }

            void push_back(serialization_chunk&& chunk) const
            {
                chunks_->push_back(HPX_MOVE(chunk));
            }

            void reset() const
            {
                chunks_->clear();
                chunks_->push_back(create_index_chunk(0, 0));
            }

            std::vector<serialization_chunk>* chunks_;
        };

        HPX_CXX_CORE_EXPORT struct counting_chunker
        {
            explicit constexpr counting_chunker(
                std::vector<serialization_chunk>*) noexcept
              : chunk_()
              , num_chunks_(0)
            {
            }

            [[nodiscard]] constexpr std::size_t get_chunk_size() const noexcept
            {
                return chunk_.size_;
            }

            void set_chunk_size(std::size_t const size) noexcept
            {
                chunk_.size_ = size;
            }

            [[nodiscard]] constexpr chunk_type get_chunk_type() const noexcept
            {
                return chunk_.type_;
            }

            [[nodiscard]] constexpr std::size_t get_chunk_data_index()
                const noexcept
            {
                return chunk_.data_.index_;
            }

            [[nodiscard]] constexpr std::size_t get_num_chunks() const noexcept
            {
                return num_chunks_;
            }

            void push_back(serialization_chunk&& chunk) noexcept
            {
                chunk_ = HPX_MOVE(chunk);
                ++num_chunks_;
            }

            void reset() noexcept
            {
                chunk_ = create_index_chunk(0, 0);
                num_chunks_ = 1;
            }

            serialization_chunk chunk_;
            std::size_t num_chunks_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Container, typename Chunker>
    struct output_container : erased_output_container
    {
        using access_traits = traits::serialization_access_data<Container>;

        explicit output_container(Container& cont,
            std::vector<serialization_chunk>* chunks = nullptr,
            std::size_t const zero_copy_serialization_threshold = 0) noexcept
          : cont_(cont)
          , current_(0)
          , chunker_(chunks)
          , zero_copy_serialization_threshold_(
                zero_copy_serialization_threshold)
        {
            if (zero_copy_serialization_threshold_ == 0)
            {
                zero_copy_serialization_threshold_ =
                    HPX_ZERO_COPY_SERIALIZATION_THRESHOLD;
            }
            chunker_.reset();
        }

        void flush() override
        {
            HPX_ASSERT(
                chunker_.get_chunk_type() == chunk_type::chunk_type_index ||
                chunker_.get_chunk_size() != 0);

            // complement current serialization_chunk by setting its length
            if (chunker_.get_chunk_type() == chunk_type::chunk_type_index)
            {
                HPX_ASSERT(chunker_.get_chunk_size() == 0);

                chunker_.set_chunk_size(
                    current_ - chunker_.get_chunk_data_index());
            }
        }

        [[nodiscard]] std::size_t get_num_chunks() const noexcept override
        {
            return chunker_.get_num_chunks();
        }

        void reset() override
        {
            chunker_.reset();
            access_traits::reset(cont_);
        }

        void set_filter(binary_filter* /* filter */) override
        {
            HPX_ASSERT(chunker_.get_num_chunks() == 1 &&
                chunker_.get_chunk_size() == 0);
            chunker_.reset();
        }

        std::size_t save_binary(detail::fundamental_types t,
            void const* address, std::size_t count) override
        {
            HPX_ASSERT(address != nullptr || count == 0);

            // store type information if needed
            std::size_t type_info_size = 0;
            if (t != detail::fundamental_types::none)
            {
                type_info_size = sizeof(t);
            }

            // make sure there is a current serialization_chunk descriptor
            // available
            if ((chunker_.get_chunk_type() == chunk_type::chunk_type_pointer ||
                    chunker_.get_chunk_type() ==
                        chunk_type::chunk_type_const_pointer) ||
                chunker_.get_chunk_size() != 0)
            {
                // add a new serialization_chunk, the chunk size will be set at
                // the end
                chunker_.push_back(create_index_chunk(current_, 0));
            }

            std::size_t new_current = current_ + count + type_info_size;
            if (access_traits::size(cont_) < new_current)
            {
                access_traits::resize(cont_, count + type_info_size);
            }

            if (t != detail::fundamental_types::none)
            {
                access_traits::write(cont_, sizeof(t), current_, &t);
            }

            if (address)
            {
                access_traits::write(
                    cont_, count, current_ + type_info_size, address);
            }

            current_ = new_current;

            return count + type_info_size;
        }

        std::size_t save_binary_chunk(detail::fundamental_types const t,
            void const* address, std::size_t const count) override
        {
            if (count < zero_copy_serialization_threshold_)
            {
                // fall back to serialization_chunk-less archive, return the
                // number of bytes the container has grown
                return this->output_container::save_binary(t, address, count);
            }

            // store type information if needed
            std::size_t type_info_size = 0;
            if (t != detail::fundamental_types::none)
            {
                type_info_size =
                    this->output_container::save_binary(t, nullptr, 0);
            }

            HPX_ASSERT(
                chunker_.get_chunk_type() == chunk_type::chunk_type_index ||
                chunker_.get_chunk_size() != 0);

            // complement current serialization_chunk by setting its length
            if (chunker_.get_chunk_type() == chunk_type::chunk_type_index)
            {
                HPX_ASSERT(chunker_.get_chunk_size() == 0);

                chunker_.set_chunk_size(
                    current_ - chunker_.get_chunk_data_index());
            }

            // add a new serialization_chunk referring to the external buffer
            chunker_.push_back(create_pointer_chunk(address, count));

            // the container did grow only if the type information needed to
            // be stored
            return type_info_size;
        }

        [[nodiscard]] bool is_preprocessing() const noexcept override
        {
            return access_traits::is_preprocessing();
        }

    protected:
        Container& cont_;
        std::size_t current_;
        Chunker chunker_;
        std::size_t zero_copy_serialization_threshold_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Container, typename Chunker>
    struct filtered_output_container final
      : output_container<Container, Chunker>
    {
        using access_traits = traits::serialization_access_data<Container>;
        using base_type = output_container<Container, Chunker>;

        explicit filtered_output_container(Container& cont,
            std::vector<serialization_chunk>* chunks = nullptr,
            std::size_t zero_copy_serialization_threshold = 0) noexcept
          : base_type(cont, chunks, zero_copy_serialization_threshold)
          , start_compressing_at_(0)
          , filter_(nullptr)
        {
        }

        void flush() override
        {
            std::size_t written = 0;

            if (access_traits::size(this->cont_) < this->current_)
                access_traits::resize(this->cont_, this->current_);

            this->current_ = start_compressing_at_;

            do
            {
                bool const flushed = access_traits::flush(filter_, this->cont_,
                    this->current_,
                    access_traits::size(this->cont_) - this->current_, written);

                this->current_ += written;
                if (flushed)
                    break;

                // resize container
                std::size_t const size = access_traits::size(this->cont_);
                access_traits::resize(this->cont_, 2 * size);

            } while (true);

            // truncate container
            access_traits::resize(this->cont_, this->current_);
        }

        void set_filter(binary_filter* filter) override
        {
            HPX_ASSERT(nullptr == filter_ && filter != nullptr);
            filter_ = filter;
            start_compressing_at_ = this->current_;

            this->base_type::set_filter(nullptr);
        }

        std::size_t save_binary(detail::fundamental_types t,
            void const* address, std::size_t count) override
        {
            HPX_ASSERT(count != 0);

            std::size_t type_info_size = 0;
            if (t != detail::fundamental_types::none)
            {
                if (filter_ != nullptr)
                {
                    filter_->save(&t, sizeof(t));
                }
                type_info_size = sizeof(t);
            }

            // during construction the filter may not have been set yet
            if (filter_ != nullptr)
            {
                filter_->save(address, count);
            }

            count += type_info_size;
            this->current_ += count;

            return count;
        }

        std::size_t save_binary_chunk(detail::fundamental_types t,
            void const* address, std::size_t count) override
        {
            if (count < this->zero_copy_serialization_threshold_)
            {
                // fall back to serialization_chunk-less archive
                return this->filtered_output_container::save_binary(
                    t, address, count);
            }
            return this->base_type::save_binary_chunk(t, address, count);
        }

    protected:
        std::size_t start_compressing_at_;
        binary_filter* filter_;
    };
}    // namespace hpx::serialization
