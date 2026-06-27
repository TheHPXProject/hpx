//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstdint>
#include <string>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/version.hpp>

/* The asio support includes termios.h.
 * The termios.h file on ppc64le defines these macros, which
 * are also used by blaze, blaze_tensor as Template names.
 * Make sure we undefine them before continuing. */
#undef VT1
#undef VT2

namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CORE_EXPORT bool get_endpoint(std::string const& addr,
        std::uint16_t port, ::asio::ip::tcp::endpoint& ep,
        bool force_ipv4 = false);

    HPX_CORE_EXPORT std::string get_endpoint_name(
        ::asio::ip::tcp::endpoint const& ep);

    ///////////////////////////////////////////////////////////////////////////
    // properly resolve a give host name to the corresponding IP address
    HPX_CORE_EXPORT ::asio::ip::tcp::endpoint resolve_hostname(
        std::string const& hostname, std::uint16_t port,
        ::asio::io_context& io_service, bool force_ipv4 = false);

    namespace detail {

#if ASIO_VERSION >= 103400
        using endpoint_iterator_type =
            ::asio::ip::basic_resolver_iterator<asio::ip::tcp>;
#else
        using endpoint_iterator_type = ::asio::ip::tcp::resolver::iterator;
#endif
    }    // namespace detail

    [[nodiscard]] HPX_CORE_EXPORT detail::endpoint_iterator_type connect_begin(
        std::string const& address, std::uint16_t port,
        ::asio::io_context& io_service);

    /// \brief Returns an iterator which when dereferenced will give an
    ///        endpoint suitable for a call to connect() related to this
    ///        locality
    template <typename Locality>
    [[nodiscard]] detail::endpoint_iterator_type connect_begin(
        Locality const& loc, ::asio::io_context& io_service)
    {
        return connect_begin(loc.address(), loc.port(), io_service);
    }

    [[nodiscard]] inline detail::endpoint_iterator_type connect_end()
    {
        return {};
    }

    [[nodiscard]] HPX_CORE_EXPORT detail::endpoint_iterator_type accept_begin(
        std::string const& address, std::uint16_t port,
        ::asio::io_context& io_service);

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Returns an iterator which when dereferenced will give an
    ///        endpoint suitable for a call to accept() related to this
    ///        locality
    template <typename Locality>
    [[nodiscard]] detail::endpoint_iterator_type accept_begin(
        Locality const& loc, ::asio::io_context& io_service)
    {
        return accept_begin(loc.address(), loc.port(), io_service);
    }

    [[nodiscard]] inline detail::endpoint_iterator_type accept_end()    //-V524
    {
        return {};
    }
}    // namespace hpx::util
