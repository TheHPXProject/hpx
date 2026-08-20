//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2007 Richard D Guidry Jr
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011 Katelyn Kufahl
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_TCP)
#include <hpx/parcelport_tcp/locality.hpp>
#include <hpx/parcelport_tcp/sender.hpp>
#include <hpx/parcelset/parcelport_impl.hpp>
#include <hpx/parcelset_base/locality.hpp>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#endif
#include <asio/ip/host_name.hpp>
#include <asio/ip/tcp.hpp>

#include <cstddef>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    namespace policies::tcp {

        class receiver;
        class HPX_EXPORT connection_handler;
    }    // namespace policies::tcp

    template <>
    struct connection_handler_traits<policies::tcp::connection_handler>
    {
        using connection_type = policies::tcp::sender;
        using send_early_parcel = std::true_type;
        using do_background_work = std::false_type;
        using send_immediate_parcels = std::false_type;
        using is_connectionless = std::false_type;

        static constexpr const char* type() noexcept
        {
            return "tcp";
        }

        static constexpr const char* pool_name() noexcept
        {
            return "parcel-pool-tcp";
        }

        static constexpr const char* pool_name_postfix() noexcept
        {
            return "-tcp";
        }
    };

    namespace policies::tcp {

        parcelset::locality parcelport_address(
            util::runtime_configuration const& ini);

        class HPX_EXPORT connection_handler
          : public parcelport_impl<connection_handler>
        {
            using base_type = parcelport_impl<connection_handler>;

        public:
            static std::vector<std::string> runtime_configuration()
            {
                std::vector<std::string> lines;
                return lines;
            }

            connection_handler(util::runtime_configuration const& ini,
                threads::policies::callback_notifier const& notifier);

            ~connection_handler();

            // Start the handling of connections.
            bool do_run();

            // Stop the handling of connections.
            void do_stop();

            // Return the name of this locality
            std::string get_locality_name() const override
            {
                return ::asio::ip::host_name();
            }

            std::shared_ptr<sender> create_connection(
                parcelset::locality const& l, error_code& ec);

            parcelset::locality agas_locality(
                util::runtime_configuration const& ini) const override;

            parcelset::locality create_locality() const override;

        private:
            void handle_accept(std::error_code const& e,
                std::shared_ptr<receiver> receiver_conn);
            void handle_read_completion(std::error_code const& e,
                std::shared_ptr<receiver> const& receiver_conn);

            /// Acceptor used to listen for incoming connections.
            ::asio::ip::tcp::acceptor* acceptor_;

            /// The list of accepted connections
            mutable hpx::spinlock connections_mtx_;

            using accepted_connections_set =
                std::set<std::shared_ptr<receiver>>;
            accepted_connections_set accepted_connections_;

#if defined(HPX_HOLDON_TO_OUTGOING_CONNECTIONS)
            using write_connections_set = std::set<std::weak_ptr<sender>>;
            write_connections_set write_connections_;
#endif
        };
    }    // namespace policies::tcp
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

#endif
