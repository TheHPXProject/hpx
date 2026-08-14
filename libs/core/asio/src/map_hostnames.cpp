//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/asio/asio_util.hpp>
#include <hpx/asio/map_hostnames.hpp>
#include <hpx/asio/tcp_util.hpp>

#include <cstdint>
#include <iostream>
#include <string>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace hpx::util {

    std::string map_hostnames::map(
        std::string host_name, std::uint16_t port) const
    {
        if (host_name == "localhost")
        {
            // map local host to loopback ip address (that's a quick hack that
            // will be removed as soon as we figure out why name resolution does
            // not handle this anymore)
            if (debug_)
            {
                std::cerr << "resolved: 'localhost' to: 127.0.0.1" << std::endl;
            }
            return "127.0.0.1";
        }

        if (!!transform_)
        {
            // if the transform is not empty
            host_name = transform_(host_name);
            if (debug_)
            {
                std::cerr << "host_name(transformed): " << host_name
                          << std::endl;
            }
        }

        // do full host name resolution
        ::asio::io_context io_service;
        ::asio::ip::tcp::endpoint const ep = util::resolve_hostname(
            prefix_ + host_name + suffix_, port, io_service, ipv4_);

        std::string resolved_addr(util::get_endpoint_name(ep));
        if (debug_)
        {
            std::cerr << "resolved: '" << prefix_ + host_name + suffix_
                      << "' to: " << resolved_addr << std::endl;
        }

        return resolved_addr;
    }
}    // namespace hpx::util
