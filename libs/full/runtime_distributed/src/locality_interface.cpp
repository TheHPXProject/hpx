//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/runtime_distributed.hpp>

#include <hpx/runtime_distributed/runtime_fwd.hpp>

#include <cstddef>
#include <string>
#include <system_error>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::detail {

    // forward declaration only
    void dijkstra_make_black();
}    // namespace hpx::detail

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parcelset {

    namespace detail::impl {

        parcelset::parcel create_parcel()
        {
            return parcelset::parcel(new detail::parcel());
        }

        locality create_locality(std::string const& name)
        {
            HPX_ASSERT(get_runtime_ptr());
            return get_runtime_distributed()
                .get_parcel_handler()
                .create_locality(name);
        }

        parcel_write_handler_type set_parcel_write_handler(
            parcel_write_handler_type const& f)
        {
            runtime_distributed* rt = get_runtime_distributed_ptr();
            if (nullptr != rt)
                return rt->get_parcel_handler().set_write_handler(f);

            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "hpx::set_default_parcel_write_handler",
                "the runtime system is not operational at this point");
        }

        ///////////////////////////////////////////////////////////////////////////
        policies::message_handler* get_message_handler(char const* action,
            char const* type, std::size_t num, std::size_t interval,
            locality const& loc, error_code& ec)
        {
            return get_runtime_distributed()
                .get_parcel_handler()
                .get_message_handler(action, type, num, interval, loc, ec);
        }

        void register_message_handler(char const* message_handler_type,
            char const* action, error_code& ec)
        {
            runtime_distributed* rtd = get_runtime_distributed_ptr();
            if (nullptr != rtd)
            {
                return rtd->register_message_handler(
                    message_handler_type, action, ec);
            }

            // store the request for later
            get_message_handler_registrations().emplace_back(
                message_handler_type, action);
        }

        parcelset::policies::message_handler* create_message_handler(
            char const* message_handler_type, char const* action,
            parcelset::parcelport* pp, std::size_t num_messages,
            std::size_t interval, error_code& ec)
        {
            runtime_distributed* rtd = get_runtime_distributed_ptr();
            if (nullptr != rtd)
            {
                return rtd->create_message_handler(message_handler_type, action,
                    pp, num_messages, interval, ec);
            }

            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "create_message_handler",
                "the runtime system is not available at this time");
            return nullptr;
        }

        ///////////////////////////////////////////////////////////////////////
        void put_parcel(parcelset::parcel&& p, write_handler_type&& f)
        {
            parcelset::parcelhandler& ph =
                hpx::get_runtime_distributed().get_parcel_handler();
            ph.put_parcel(HPX_MOVE(p), HPX_MOVE(f));
        }

        void sync_put_parcel(parcelset::parcel&& p)
        {
            parcelset::parcelhandler& ph =
                hpx::get_runtime_distributed().get_parcel_handler();
            ph.sync_put_parcel(HPX_MOVE(p));
        }

        ///////////////////////////////////////////////////////////////////////
        void parcel_route_handler(
            std::error_code const& ec, parcelset::parcel const& p)
        {
            parcelhandler& ph =
                hpx::get_runtime_distributed().get_parcel_handler();

            // invoke the original handler
            ph.invoke_write_handler(ec, p);

            // inform termination detection of a sent message
            if (!p.does_termination_detection())
            {
                hpx::detail::dijkstra_make_black();
            }
        }
    }    // namespace detail::impl

    // initialize locality interface function pointers in parcelset modules
    struct HPX_EXPORT locality_interface_functions
    {
        locality_interface_functions()
        {
            detail::create_parcel = &detail::impl::create_parcel;
            detail::create_locality = &detail::impl::create_locality;
            detail::set_parcel_write_handler =
                &detail::impl::set_parcel_write_handler;

            detail::get_message_handler = &detail::impl::get_message_handler;
            detail::register_message_handler =
                &detail::impl::register_message_handler;
            detail::create_message_handler =
                &detail::impl::create_message_handler;

            detail::put_parcel = &detail::impl::put_parcel;
            detail::sync_put_parcel = &detail::impl::sync_put_parcel;

            detail::parcel_route_handler_func =
                &detail::impl::parcel_route_handler;
        }
    };

    locality_interface_functions& locality_init()
    {
        static locality_interface_functions locality_init_;
        return locality_init_;
    }
}    // namespace hpx::parcelset

#endif
