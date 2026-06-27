//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:nodeprecatedinclude:boost/system/error_code.hpp
// hpxinspect:nodeprecatedname:boost::system::error_code
// hpxinspect:nodeprecatedinclude:boost/system/system_error.hpp
// hpxinspect:nodeprecatedname:boost::system::system_error

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>

#include <hpx/asio/asio_util.hpp>
#include <hpx/asio/tcp_util.hpp>

#include <asio/io_context.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/address_v6.hpp>
#include <asio/ip/host_name.hpp>
#include <asio/ip/tcp.hpp>

#include <cstdint>
#include <exception>
#include <string>
#include <system_error>

#if defined(HPX_WINDOWS) && !defined(HPX_HAVE_STATIC_LINKING)
// Prevent asio from initializing Winsock, the object must be constructed
// before any Asio's own global objects. With MSVC, this may be accomplished
// by adding the following code to the DLL:

#if defined(HPX_MSVC_WARNING_PRAGMA)
#pragma warning(push)
#pragma warning(disable : 4073)
#endif
#pragma init_seg(lib)
::asio::detail::winsock_init<>::manual manual_winsock_init;
#if defined(HPX_MSVC_WARNING_PRAGMA)
#pragma warning(pop)
#endif
#endif

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    bool get_endpoint(std::string const& addr, std::uint16_t port,
        ::asio::ip::tcp::endpoint& ep, bool force_ipv4)
    {
        using namespace ::asio::ip;
        std::error_code ec;
#if ASIO_VERSION >= 103400
        address_v4 const addr4 =    //-V821
            make_address_v4(addr.c_str(), ec);
#else
        address_v4 const addr4 =    //-V821
            address_v4::from_string(addr.c_str(), ec);
#endif
        if (!ec)
        {    // it's an IPV4 address
            ep = tcp::endpoint(address(addr4), port);
            return true;
        }

        if (!force_ipv4)
        {
#if ASIO_VERSION >= 103400
            address_v6 const addr6 =    //-V821
                make_address_v6(addr.c_str(), ec);
#else
            address_v6 const addr6 =    //-V821
                address_v6::from_string(addr.c_str(), ec);
#endif
            if (!ec)
            {    // it's an IPV6 address
                ep = tcp::endpoint(address(addr6), port);
                return true;
            }
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::string get_endpoint_name(::asio::ip::tcp::endpoint const& ep)
    {
        return ep.address().to_string();
    }

    ///////////////////////////////////////////////////////////////////////////
    // properly resolve a give host name to the corresponding IP address
    ::asio::ip::tcp::endpoint resolve_hostname(std::string const& hostname,
        std::uint16_t port, ::asio::io_context& io_service, bool force_ipv4)
    {
        using ::asio::ip::tcp;

        // collect errors here
        exception_list errors;

        // try to directly create an endpoint from the address
        try
        {
            tcp::endpoint ep;
            if (util::get_endpoint(hostname, port, ep))
                return ep;
        }
        catch (std::system_error const&)
        {
            errors.add(std::current_exception());
        }

        // it's not an address, try to treat it as a host name
        try
        {
            // resolve the given address
            tcp::resolver resolver(io_service);

#if ASIO_VERSION >= 103400
            auto resolver_results = resolver.resolve(
                ::asio::ip::tcp::v4(), hostname, std::to_string(port));

            auto it = resolver_results.begin();
            auto end = resolver_results.begin();

            // skip ipv6 results, if required
            if (it == end && !force_ipv4)
            {
                resolver_results = resolver.resolve(
                    ::asio::ip::tcp::v6(), hostname, std::to_string(port));
                it = resolver_results.begin();
            }

            HPX_ASSERT(it != end);
            return *it;
#else
            tcp::resolver::query query(hostname, std::to_string(port));
            ::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);

            // skip ipv6 results, if required
            if (force_ipv4)
            {
                while (it != tcp::resolver::iterator() &&
                    !it->endpoint().address().is_v4())
                {
                    ++it;
                }
            }
            HPX_ASSERT(it != ::asio::ip::tcp::resolver::iterator());
            return *it;
#endif
        }
        catch (std::system_error const&)
        {
            errors.add(std::current_exception());
        }

        // report errors
        HPX_THROW_EXCEPTION(hpx::error::network_error, "util::resolve_hostname",
            "{} (while trying to resolve: {}:{})", errors.get_message(),
            hostname, port);
    }

    ///////////////////////////////////////////////////////////////////////////
    // return the public IP address of the local node
    std::string resolve_public_ip_address()
    {
        using ::asio::ip::tcp;

        // collect errors here
        exception_list errors;

        try
        {
            ::asio::io_context io_service;
            tcp::resolver resolver(io_service);

#if ASIO_VERSION >= 103400
            auto resolver_results = resolver.resolve(
                ::asio::ip::tcp::v4(), asio::ip::host_name(), "");
            auto it = resolver_results.begin();
            if (it == resolver_results.end())
            {
                resolver_results = resolver.resolve(
                    ::asio::ip::tcp::v6(), asio::ip::host_name(), "");
                it = resolver_results.begin();
            }
#else
            tcp::resolver::query query(::asio::ip::host_name(), "");
            tcp::resolver::iterator it = resolver.resolve(query);
#endif
            tcp::endpoint endpoint = *it;
            return endpoint.address().to_string();
        }
        catch (std::system_error const&)
        {
            errors.add(std::current_exception());
        }

        // report errors
        HPX_THROW_EXCEPTION(hpx::error::network_error,
            "util::resolve_public_ip_address",
            "{} (while trying to resolve public ip address)",
            errors.get_message());
    }

    ///////////////////////////////////////////////////////////////////////
    // Take an ip v4 or v6 address and "standardize" it for comparison checks
    // note that this code doesn't work as expected if we use the boost
    // inet_pton functions on linux. see issue #2177 for further info
    std::string cleanup_ip_address(std::string const& addr)
    {
        char buf[sizeof(struct in6_addr)];
        int i = 0, domain[2] = {AF_INET, AF_INET6};
        char str[INET6_ADDRSTRLEN];

#if defined(HPX_WINDOWS)
        unsigned long scope_id = 0;
        std::error_code ec;
#endif

        for (/**/; i < 2; ++i)
        {
#if defined(HPX_WINDOWS)
            int const s = ::asio::detail::socket_ops::inet_pton(
                domain[i], addr.data(), buf, &scope_id, ec);
            if (s > 0 && !ec)
                break;
#else
            int const s = inet_pton(domain[i], addr.data(), buf);
            if (s > 0)
                break;
#endif
        }
        if (i == 2)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "cleanup_ip_address",
                "Invalid IP address string");
        }

#if defined(HPX_WINDOWS)
        if (::asio::detail::socket_ops::inet_ntop(
                domain[i], buf, str, INET6_ADDRSTRLEN, scope_id, ec) == nullptr)
        {
#else
        if (inet_ntop(domain[i], buf, str, INET6_ADDRSTRLEN) == nullptr)
        {
#endif
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "cleanup_ip_address",
                "inet_ntop failure");
        }
        return {str};
    }

    detail::endpoint_iterator_type connect_begin(std::string const& address,
        std::uint16_t port, ::asio::io_context& io_service)
    {
        using ::asio::ip::tcp;

        // collect errors here
        exception_list errors;

        std::string port_str(std::to_string(port));

        // try to directly create an endpoint from the address
        try
        {
            tcp::endpoint ep;
            if (util::get_endpoint(address, port, ep))
            {
#if ASIO_VERSION >= 103400
                auto resolver_results =
                    tcp::resolver::results_type::create(ep, address, port_str);
                return resolver_results.begin();
#else
                return {
                    tcp::resolver::results_type::create(ep, address, port_str)};
#endif
            }
        }
        catch (std::system_error const&)
        {
            errors.add(std::current_exception());
        }

        // it's not an address, try to treat it as a host name
        try
        {
            // resolve the given address
            tcp::resolver resolver(io_service);

#if ASIO_VERSION >= 103400
            auto resolver_results = resolver.resolve(::asio::ip::tcp::v4(),
                !address.empty() ? address : ::asio::ip::host_name(), port_str);
            auto it = resolver_results.begin();
            if (it == resolver_results.end())
            {
                resolver_results = resolver.resolve(::asio::ip::tcp::v6(),
                    !address.empty() ? address : ::asio::ip::host_name(),
                    port_str);
                it = resolver_results.begin();
            }
            return it;
#else
            tcp::resolver::query query(
                !address.empty() ? address : ::asio::ip::host_name(), port_str);
            return {resolver.resolve(query)};
#endif
        }
        catch (std::system_error const&)
        {
            errors.add(std::current_exception());
        }

        // report errors
        HPX_THROW_EXCEPTION(hpx::error::network_error, "connect_begin",
            "{} (while trying to connect to: {}:{})", errors.get_message(),
            address, port);
    }

    detail::endpoint_iterator_type accept_begin(std::string const& address,
        std::uint16_t port, ::asio::io_context& io_service)
    {
        using ::asio::ip::tcp;

        // collect errors here
        exception_list errors;

        std::string port_str(std::to_string(port));

        // try to directly create an endpoint from the address
        try
        {
            tcp::endpoint ep;
            if (util::get_endpoint(address, port, ep))
            {
#if ASIO_VERSION >= 103400
                auto resolver_results =
                    tcp::resolver::results_type::create(ep, address, port_str);
                return resolver_results.begin();
#else
                return {
                    tcp::resolver::results_type::create(ep, address, port_str)};
#endif
            }
        }
        catch (std::system_error const&)
        {
            errors.add(std::current_exception());
        }

        // it's not an address, try to treat it as a host name
        try
        {
            // resolve the given address
            tcp::resolver resolver(io_service);
#if ASIO_VERSION >= 103400
            auto resolver_results =
                resolver.resolve(::asio::ip::tcp::v4(), address, port_str);
            auto it = resolver_results.begin();
            if (it == resolver_results.end())
            {
                resolver_results =
                    resolver.resolve(::asio::ip::tcp::v6(), address, port_str);
                it = resolver_results.begin();
            }
            return it;
#else
            tcp::resolver::query query(address, port_str);
            return {resolver.resolve(query)};
#endif
        }
        catch (std::system_error const&)
        {
            errors.add(std::current_exception());
        }

        // it's not a host name either, create a custom iterator allowing to
        // filter the returned endpoints, for this we use "localhost" as the
        // address to enumerate endpoints
        try
        {
            // resolve the given address
            tcp::resolver resolver(io_service);

#if ASIO_VERSION >= 103400
            auto resolver_results = resolver.resolve(
                ::asio::ip::tcp::v4(), ::asio::ip::host_name(), port_str);
            auto it = resolver_results.begin();
            if (it == resolver_results.end())
            {
                resolver_results = resolver.resolve(
                    ::asio::ip::tcp::v6(), ::asio::ip::host_name(), port_str);
                it = resolver_results.begin();
            }
            return it;
#else
            tcp::resolver::query query(::asio::ip::host_name(), port_str);
            return {resolver.resolve(query)};
#endif
        }
        catch (std::system_error const&)
        {
            errors.add(std::current_exception());
        }

        // report errors
        HPX_THROW_EXCEPTION(hpx::error::network_error, "accept_begin",
            "{} (while trying to resolve: {}:{}))", errors.get_message(),
            address, port);
    }
}    // namespace hpx::util

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    // Addresses are supposed to have the format <hostname>[:port]
    bool split_ip_address(
        std::string const& v, std::string& host, std::uint16_t& port)
    {
        std::string::size_type const p = v.find_last_of(':');

        try
        {
            std::string tmp_host;
            std::uint16_t tmp_port = 0;

            if (p != std::string::npos)
            {
                if (v.find_first_of(':') != p)
                {
                    // IPv6
                    std::string::size_type const begin_of_address =
                        v.find_first_of('[');
                    if (begin_of_address != std::string::npos)
                    {
                        // IPv6 with a port has to be written as: [address]:port
                        std::string::size_type const end_of_address =
                            v.find_last_of(']');
                        if (end_of_address == std::string::npos)
                            return false;

                        tmp_host =
                            v.substr(begin_of_address + 1, end_of_address - 1);
                        if (end_of_address < p)
                        {
                            tmp_port = hpx::util::from_string<std::uint16_t>(
                                v.substr(p + 1));
                        }
                    }
                    else
                    {
                        // IPv6 without a port
                        tmp_host = v;
                    }
                }
                else
                {
                    // IPv4
                    tmp_host = v.substr(0, p);
                    tmp_port =
                        hpx::util::from_string<std::uint16_t>(v.substr(p + 1));
                }
            }
            else
            {
                tmp_host = v;
            }

            if (!tmp_host.empty())
            {
                host = tmp_host;
                if (tmp_port)
                    port = tmp_port;
            }
        }
        catch (hpx::util::bad_lexical_cast const& /*e*/)
        {
            // port number is invalid
            return false;
        }
        return true;
    }
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
