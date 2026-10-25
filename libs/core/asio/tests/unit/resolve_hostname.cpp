//  Copyright (c) 2026 Akagra Mishra
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Host name resolution must return what the system resolver reports: the
// first endpoint (or the first IPv4 endpoint if force_ipv4 is set), and
// connect_begin/accept_begin must enumerate all of them.

#include <hpx/asio/asio_util.hpp>
#include <hpx/modules/testing.hpp>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

int main()
{
    std::string const host = "localhost";
    std::uint16_t const port = 7910;

    ::asio::io_context io_service;
    ::asio::ip::tcp::resolver resolver(io_service);
    auto const expected = resolver.resolve(host, std::to_string(port),
        ::asio::ip::tcp::resolver::address_configured);
    HPX_TEST(!expected.empty());

    // without force_ipv4, the first resolved endpoint is used
    HPX_TEST_EQ(hpx::util::resolve_hostname(host, port, io_service, false),
        expected.begin()->endpoint());

    // with force_ipv4, the first IPv4 endpoint is used
    for (auto const& entry : expected)
    {
        if (entry.endpoint().address().is_v4())
        {
            HPX_TEST_EQ(
                hpx::util::resolve_hostname(host, port, io_service, true),
                entry.endpoint());
            break;
        }
    }

    // connect_begin/accept_begin enumerate all resolved endpoints
    std::size_t count = 0;
    for (auto it = hpx::util::connect_begin(host, port, io_service);
        it != hpx::util::connect_end(); ++it)
    {
        ++count;
    }
    HPX_TEST_EQ(count, expected.size());

    count = 0;
    for (auto it = hpx::util::accept_begin(host, port, io_service);
        it != hpx::util::accept_end(); ++it)
    {
        ++count;
    }
    HPX_TEST_EQ(count, expected.size());

    return hpx::util::report_errors();
}
