//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011 Katelyn Kufahl
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_TCP)
#include <hpx/assert.hpp>
#include <hpx/modules/asio.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/parcelport_tcp/locality.hpp>

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
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::tcp {

    class sender : public parcelset::parcelport_connection<sender>
    {
        using postprocess_handler_type =
            hpx::move_only_function<void(std::error_code const&)>;

    public:
        // Construct a sending parcelport_connection with the given io_context.
        sender(::asio::io_context& io_service,
            parcelset::locality const& locality_id,
            [[maybe_unused]] parcelset::parcelport* pp)
          : socket_(io_service)
          , ack_(false)
          , there_(locality_id)
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
          , pp_(pp)
#endif
        {
        }

        ~sender()
        {
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
        }

        // Get the socket associated with the parcelport_connection.
        ::asio::ip::tcp::socket& socket() noexcept
        {
            return socket_;
        }

        parcelset::locality const& destination() const noexcept
        {
            return there_;
        }

        void verify_(
            [[maybe_unused]] parcelset::locality const& parcel_locality_id)
            const
        {
#if defined(HPX_DEBUG)
            std::error_code ec;
            ::asio::ip::tcp::socket::endpoint_type const endpoint =
                socket_.remote_endpoint(ec);

            locality const& impl = parcel_locality_id.get<locality>();

            // We just ignore failures here. Those are the reason for
            // remote endpoint not connected errors which occur
            // when the runtime is in hpx::state::shutdown
            if (!ec)
            {
                HPX_ASSERT(hpx::util::cleanup_ip_address(impl.address()) ==
                    hpx::util::cleanup_ip_address(
                        endpoint.address().to_string()));
                HPX_ASSERT(impl.port() == endpoint.port());
            }
#endif
        }

        template <typename Handler, typename ParcelPostprocess>
        void async_write(
            Handler&& handler, ParcelPostprocess&& parcel_postprocess)
        {
#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
            HPX_ASSERT(state_ == state_send_pending);
#endif
            HPX_ASSERT(!buffer_.data_.empty());
            HPX_ASSERT(!handler_);
            HPX_ASSERT(!postprocess_handler_);

            handler_ = HPX_FORWARD(Handler, handler);
            postprocess_handler_ =
                HPX_FORWARD(ParcelPostprocess, parcel_postprocess);
            HPX_ASSERT(handler_);
            HPX_ASSERT(postprocess_handler_);

#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
            state_ = state_async_write;
#endif
            /// Increment sends and begin timer.
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            buffer_.data_point_.time_ = timer_.elapsed_nanoseconds();
#endif
            // Write the serialized data to the socket. We use "gather-write"
            // to send both the header and the data in a single write operation.
            std::vector<::asio::const_buffer> buffers;
            buffers.emplace_back(&buffer_.size_, sizeof(buffer_.size_));
            buffers.emplace_back(
                &buffer_.data_size_, sizeof(buffer_.data_size_));

            // add chunk description
            buffers.emplace_back(
                &buffer_.num_chunks_, sizeof(buffer_.num_chunks_));

            std::vector<parcel_buffer_type::transmission_chunk_type>& chunks =
                buffer_.transmission_chunks_;
            if (!chunks.empty())
            {
                buffers.emplace_back(chunks.data(),
                    chunks.size() *
                        sizeof(parcel_buffer_type::transmission_chunk_type));

                // add main buffer holding data which was serialized normally
                buffers.emplace_back(::asio::buffer(buffer_.data_));

                // now add chunks themselves, those hold zero-copy serialized chunks
                for (serialization::serialization_chunk& c : buffer_.chunks_)
                {
                    if (c.type_ ==
                            serialization::chunk_type::chunk_type_pointer ||
                        c.type_ ==
                            serialization::chunk_type::chunk_type_const_pointer)
                    {
                        buffers.emplace_back(c.data_.cpos_, c.size_);
                    }
                }
            }
            else
            {
                // add main buffer holding data which was serialized normally
                buffers.emplace_back(::asio::buffer(buffer_.data_));
            }

            // this additional wrapping of the handler into a bind object is
            // needed to keep  this parcelport_connection object alive for the
            // whole write operation
            void (sender::*f)(std::error_code const&, std::size_t) =
                &sender::handle_write;

            ::asio::async_write(socket_, buffers,
                hpx::bind(f, shared_from_this(), hpx::placeholders::_1,
                    hpx::placeholders::_2));
        }

    private:
        static void reset_handler(postprocess_handler_type handler)
        {
            handler.reset();
        }

        /// handle completed write operation
        void handle_write(std::error_code const& e, std::size_t /* bytes */)
        {
#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
            state_ = state_handle_write;
#endif
            // just call initial handler
            handler_(e);

            postprocess_handler_type handler;
            std::swap(handler, handler_);

            if (threads::threadmanager_is(hpx::state::running))
            {
                // the handler needs to be reset on an HPX thread (it destroys
                // the parcel, which in turn might invoke HPX functions)
                threads::thread_init_data data(
                    threads::make_thread_function_nullary(
                        &sender::reset_handler, HPX_MOVE(handler)),
                    "sender::reset_handler");
                threads::register_thread(data);
            }
            else
            {
                reset_handler(HPX_MOVE(handler));
            }

            if (e)
            {
                // inform post-processing handler of error as well
                hpx::move_only_function<void(std::error_code const&,
                    parcelset::locality const&, std::shared_ptr<sender>)>
                    postprocess_handler;
                std::swap(postprocess_handler, postprocess_handler_);
                postprocess_handler(e, there_, shared_from_this());
                return;
            }

            // complete data point and push back onto gatherer
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            buffer_.data_point_.time_ =
                timer_.elapsed_nanoseconds() - buffer_.data_point_.time_;
            pp_->add_sent_data(buffer_.data_point_);
#endif

            // now handle the acknowledgment byte which is sent by the receiver
#if defined(__linux) || defined(linux) || defined(__linux__)
            ::asio::detail::socket_option::boolean<IPPROTO_TCP, TCP_QUICKACK>
                quickack(true);
            socket_.set_option(quickack);
#endif

            void (sender::*f)(std::error_code const&) =
                &sender::handle_read_ack;

            ::asio::async_read(socket_, asio::buffer(&ack_, sizeof(ack_)),
                hpx::bind(f, shared_from_this(), placeholders::_1));
        }

        void handle_read_ack(std::error_code const& e)
        {
#if defined(HPX_TRACK_STATE_OF_OUTGOING_TCP_CONNECTION)
            state_ = state_handle_read_ack;
#endif
            buffer_.clear();

            // Call post-processing handler, which will send remaining pending
            // parcels. Pass along the connection so it can be reused if more
            // parcels have to be sent.
            hpx::move_only_function<void(std::error_code const&,
                parcelset::locality const&, std::shared_ptr<sender>)>
                postprocess_handler;
            std::swap(postprocess_handler, postprocess_handler_);
            postprocess_handler(e, there_, shared_from_this());
        }

        // Socket for the parcelport_connection.
        ::asio::ip::tcp::socket socket_;

        bool ack_;

        // the other (receiving) end of this connection
        parcelset::locality there_;

        // Counters and their data containers.
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        hpx::chrono::high_resolution_timer timer_;
        parcelset::parcelport* pp_;
#endif

        postprocess_handler_type handler_;
        hpx::move_only_function<void(std::error_code const&,
            parcelset::locality const&, std::shared_ptr<sender>)>
            postprocess_handler_;
    };
}    // namespace hpx::parcelset::policies::tcp

#endif
