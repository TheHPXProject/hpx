//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2011 Katelyn Kufahl
//  Copyright (c) 2011 Bryce Lelbach
//
//  Parts of this code were taken from the Boost.Asio library
//  Copyright (c) 2003-2007 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_TCP)
#include <hpx/assert.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/parcelport_tcp/connection_handler.hpp>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#endif
#include <asio/buffer.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

// The asio support includes termios.h.
// The termios.h file on ppc64le defines these macros, which
// are also used by blaze, blaze_tensor as Template names.
// Make sure we undefine them before continuing.
#undef VT1
#undef VT2

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::tcp {

    class connection_handler;

    class receiver : public parcelport_connection<receiver>
    {
    public:
        receiver(::asio::io_context& io_service, std::uint64_t max_inbound_size,
            connection_handler& parcelport)
          : socket_(io_service)
          , max_inbound_size_(max_inbound_size)
          , ack_(false)
          , parcelport_(parcelport)
          , operation_in_flight_(0)
        {
        }

        receiver(receiver const&) = delete;
        receiver(receiver&&) = delete;
        receiver& operator=(receiver const&) = delete;
        receiver& operator=(receiver&&) = delete;

        ~receiver()
        {
            shutdown();
        }

        // Get the socket associated with the parcelport_connection.
        ::asio::ip::tcp::socket& socket() noexcept
        {
            return socket_;
        }

        // Asynchronously read a data structure from the socket.
        template <typename Handler>
        void async_read(Handler handler)
        {
            HPX_ASSERT(buffer_.data_.empty());

            // Store the time of the begin of the read operation
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            parcelset::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds();
            data.serialization_time_ = 0;
            data.bytes_ = 0;
            data.num_parcels_ = 0;
#endif
            parcels_.clear();
            chunk_buffers_.clear();

            // Issue a read operation to read the message size.
            using ::asio::buffer;
            std::vector<::asio::mutable_buffer> buffers;
            buffers.emplace_back(&buffer_.size_, sizeof(buffer_.size_));
            buffers.emplace_back(
                &buffer_.data_size_, sizeof(buffer_.data_size_));

            buffers.emplace_back(
                &buffer_.num_chunks_, sizeof(buffer_.num_chunks_));

            {
                std::unique_lock lk(mtx_);
                if (!socket_.is_open())
                {
                    lk.unlock();
                    // report this problem back to the handler
                    handler(::asio::error::make_error_code(
                        ::asio::error::not_connected));
                    return;
                }
#if defined(__linux) || defined(linux) || defined(__linux__)
                ::asio::detail::socket_option::boolean<IPPROTO_TCP,
                    TCP_QUICKACK>
                    quickack(true);
                socket_.set_option(quickack);
#endif

                void (receiver::*f)(std::error_code const&, std::size_t,
                    Handler) = &receiver::handle_read_header<Handler>;

                ::asio::async_read(socket_, buffers,
                    hpx::bind(f, shared_from_this(),
                        placeholders::_1,    // error
                        placeholders::_2,    // bytes_transferred
                        util::protect(handler)));
            }
        }

        void shutdown()
        {
            std::lock_guard lk(mtx_);

            // gracefully and portably shutdown the socket
            if (socket_.is_open())
            {
                // NOLINTBEGIN(bugprone-unused-return-value)
                std::error_code ec;
                socket_.shutdown(::asio::ip::tcp::socket::shutdown_both, ec);

                // close the socket to give it back to the OS
                socket_.close(ec);
                // NOLINTEND(bugprone-unused-return-value)
            }

            hpx::util::yield_while(
                [this]() { return operation_in_flight_ != 0; },
                "tcp::receiver::shutdown");
        }

    private:
        // Handle a completed read of the message size from the message header.
        template <typename Handler>
        void handle_read_header(std::error_code const& e,
            std::size_t /* bytes_transferred */, Handler handler)
        {
            HPX_ASSERT(operation_in_flight_ == 0);
            if (e)
            {
                handler(e);
            }
            else
            {
                ++operation_in_flight_;

                // Determine the length of the serialized data.
                std::uint64_t const inbound_size = buffer_.size_;

                // check for the message exceeding the given limit (only if
                // given)
                if (max_inbound_size_ != 0 && inbound_size > max_inbound_size_)
                {
                    // report this problem back to the handler
                    handler(::asio::error::make_error_code(
                        ::asio::error::operation_not_supported));
                    return;
                }

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
                buffer_.data_point_.bytes_ =
                    static_cast<std::size_t>(inbound_size);
#endif
                // receive buffers
                std::vector<::asio::mutable_buffer> buffers;

                // determine the size of the chunk buffer
                auto const num_zero_copy_chunks = static_cast<std::size_t>(
                    static_cast<std::uint32_t>(buffer_.num_chunks_.first));
                auto const num_non_zero_copy_chunks = static_cast<std::size_t>(
                    static_cast<std::uint32_t>(buffer_.num_chunks_.second));

                void (receiver::*f)(std::error_code const&, Handler);

                if (num_zero_copy_chunks != 0)
                {
                    using transmission_chunk_type =
                        parcel_buffer_type::transmission_chunk_type;

                    std::vector<transmission_chunk_type>& chunks =
                        buffer_.transmission_chunks_;

                    chunks.resize(static_cast<std::size_t>(
                        num_zero_copy_chunks + num_non_zero_copy_chunks));

                    buffers.emplace_back(chunks.data(),
                        chunks.size() * sizeof(transmission_chunk_type));

                    // add main buffer holding data that was serialized normally
                    buffer_.data_.resize(
                        static_cast<std::size_t>(inbound_size));
                    buffers.emplace_back(::asio::buffer(buffer_.data_));

                    // Start an asynchronous call to receive the data.
                    f = &receiver::handle_read_chunk_data<Handler>;
                }
                else
                {
                    // add main buffer holding data that was serialized normally
                    buffer_.data_.resize(
                        static_cast<std::size_t>(inbound_size));
                    buffers.emplace_back(::asio::buffer(buffer_.data_));

                    // Start an asynchronous call to receive the data.
                    f = &receiver::handle_read_data<Handler>;
                }

                {
                    std::unique_lock lk(mtx_);
                    if (!socket_.is_open())
                    {
                        lk.unlock();

                        // report this problem back to the handler
                        handler(::asio::error::make_error_code(
                            ::asio::error::not_connected));
                        return;
                    }

#if defined(__linux) || defined(linux) || defined(__linux__)
                    ::asio::detail::socket_option::boolean<IPPROTO_TCP,
                        TCP_QUICKACK>
                        quickack(true);
                    socket_.set_option(quickack);
#endif
                    ::asio::async_read(socket_, buffers,
                        hpx::bind(f, shared_from_this(),
                            placeholders::_1,    // error,
                            util::protect(handler)));
                }
            }
        }

        // Handle a completed read of message data.
        template <typename Handler>
        void handle_read_chunk_data(std::error_code const& e, Handler handler)
        {
            if (e)
            {
                handler(e);
                --operation_in_flight_;
                buffer_ = parcel_buffer_type();
                parcels_.clear();
            }
            else
            {
                // receive buffers
                std::vector<::asio::mutable_buffer> buffers;

                // add appropriately sized chunk buffers for the zero-copy data
                auto const num_zero_copy_chunks = static_cast<std::size_t>(
                    static_cast<std::uint32_t>(buffer_.num_chunks_.first));

                buffer_.chunks_.resize(num_zero_copy_chunks);

                if (parcelport_.allow_zero_copy_receive_optimizations())
                {
                    // De-serialize the parcels such that all data but the
                    // zero-copy chunks are in place. This de-serialization also
                    // allocates all zero-chunk buffers and stores those in the
                    // chunks array for the subsequent networking to place the
                    // received data directly.
                    for (std::size_t i = 0; i != num_zero_copy_chunks; ++i)
                    {
                        auto const chunk_size = static_cast<std::size_t>(
                            buffer_.transmission_chunks_[i].second);
                        buffer_.chunks_[i] =
                            serialization::create_pointer_chunk(
                                nullptr, chunk_size);
                    }

                    parcels_ = decode_parcels_zero_copy(parcelport_, buffer_);

                    // note that at this point, buffer_.chunks_ will have
                    // entries for all chunks, including the non-zero-copy ones

                    [[maybe_unused]] auto const num_non_zero_copy_chunks =
                        static_cast<std::size_t>(static_cast<std::uint32_t>(
                            buffer_.num_chunks_.second));

                    HPX_ASSERT(
                        num_zero_copy_chunks + num_non_zero_copy_chunks ==
                        buffer_.chunks_.size());

                    std::size_t zero_copy_chunks = 0;
                    for (auto& c : buffer_.chunks_)
                    {
                        if (c.type_ ==
                            serialization::chunk_type::chunk_type_index)
                        {
                            continue;    // skip non-zero-copy chunks
                        }

                        auto const chunk_size = static_cast<std::size_t>(
                            buffer_.transmission_chunks_[zero_copy_chunks++]
                                .second);

                        HPX_ASSERT_MSG(
                            c.data() != nullptr && c.size() == chunk_size,
                            "zero-copy chunk buffers should have been "
                            "initialized during de-serialization");

                        buffers.emplace_back(c.data(), chunk_size);
                    }
                    HPX_ASSERT(zero_copy_chunks == num_zero_copy_chunks);
                }
                else
                {
                    chunk_buffers_.resize(num_zero_copy_chunks);
                    for (std::size_t i = 0; i != num_zero_copy_chunks; ++i)
                    {
                        auto const chunk_size = static_cast<std::size_t>(
                            buffer_.transmission_chunks_[i].second);

                        chunk_buffers_[i].resize(chunk_size);
                        buffers.emplace_back(
                            chunk_buffers_[i].data(), chunk_size);

                        buffer_.chunks_[i] =
                            serialization::create_pointer_chunk(
                                chunk_buffers_[i].data(), chunk_size);
                    }
                }

                // Start an asynchronous call to receive the zero-copy data.
                {
                    void (receiver::*f)(std::error_code const&, Handler) =
                        &receiver::handle_read_data<Handler>;

                    std::unique_lock lk(mtx_);
                    if (!socket_.is_open())
                    {
                        lk.unlock();

                        // report this problem back to the handler
                        handler(::asio::error::make_error_code(
                            ::asio::error::not_connected));
                        return;
                    }

#if defined(__linux) || defined(linux) || defined(__linux__)
                    ::asio::detail::socket_option::boolean<IPPROTO_TCP,
                        TCP_QUICKACK>
                        quickack(true);
                    socket_.set_option(quickack);
#endif
                    ::asio::async_read(socket_, buffers,
                        hpx::bind(f, shared_from_this(),
                            placeholders::_1,    // error,
                            util::protect(handler)));
                }
            }
        }

        // Handle a completed read of message data.
        template <typename Handler>
        void handle_read_data(std::error_code const& e, Handler handler)
        {
            if (e)
            {
                handler(e);
                --operation_in_flight_;
                buffer_ = parcel_buffer_type();
                parcels_.clear();
                chunk_buffers_.clear();
            }
            else
            {
                // complete data point and pass it along
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
                buffer_.data_point_.time_ =
                    timer_.elapsed_nanoseconds() - buffer_.data_point_.time_;
#endif
                // now send acknowledgment byte
                void (receiver::*f)(std::error_code const&, Handler) =
                    &receiver::handle_write_ack<Handler>;

                if (parcels_.empty())
                {
                    // decode and handle received data
                    HPX_ASSERT(buffer_.num_chunks_.first == 0 ||
                        !parcelport_.allow_zero_copy_receive_optimizations());
                    handle_received_parcels(
                        decode_parcels(parcelport_, HPX_MOVE(buffer_)));
                }
                else
                {
                    // handle the received zero-copy parcels.
                    HPX_ASSERT(buffer_.num_chunks_.first != 0 &&
                        parcelport_.allow_zero_copy_receive_optimizations());
                    handle_received_parcels(HPX_MOVE(parcels_));
                }

                ack_ = true;
                {
                    std::unique_lock lk(mtx_);
                    if (!socket_.is_open())
                    {
                        lk.unlock();

                        // report this problem back to the handler
                        handler(::asio::error::make_error_code(
                            ::asio::error::not_connected));
                        return;
                    }

                    ::asio::async_write(socket_,
                        ::asio::buffer(&ack_, sizeof(ack_)),
                        hpx::bind(f, shared_from_this(),
                            placeholders::_1,    // error,
                            util::protect(handler)));
                }
            }
        }

        template <typename Handler>
        void handle_write_ack(std::error_code const& e, Handler handler)
        {
            HPX_ASSERT(operation_in_flight_ != 0);

            // Inform caller that data has been received ok.
            handler(e);
            --operation_in_flight_;

            buffer_ = parcel_buffer_type();
            parcels_.clear();
            chunk_buffers_.clear();

            // Issue a read operation to read the next parcel.
            if (!e)
            {
                async_read(handler);
            }
        }

        // Socket for the parcelport_connection.
        ::asio::ip::tcp::socket socket_;

        std::uint64_t max_inbound_size_;

        bool ack_;

        // The handler used to process the incoming request.
        connection_handler& parcelport_;

        // Counters and timers for parcels received.
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        hpx::chrono::high_resolution_timer timer_;
#endif
        hpx::spinlock mtx_;
        hpx::util::atomic_count operation_in_flight_;

        std::vector<parcelset::parcel> parcels_;
        std::vector<std::vector<char>> chunk_buffers_;
    };
}    // namespace hpx::parcelset::policies::tcp

#endif
