////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/include/client.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/threading_base.hpp>
#ifdef HPX_HAVE_MODULE_LIKWID
#include <hpx/modules/likwid.hpp>
#endif

#include "stubs/managed_refcnt_checker.hpp"

#include <utility>

namespace hpx { namespace test {

    struct managed_refcnt_monitor
      : components::client_base<managed_refcnt_monitor,
            stubs::managed_refcnt_checker>
    {
        typedef components::client_base<managed_refcnt_monitor,
            stubs::managed_refcnt_checker>
            base_type;

    private:
        hpx::distributed::promise<void> flag_promise_;
        hpx::future<void> flag_;
        hpx::id_type const locality_;

    public:
        typedef server::managed_refcnt_checker server_type;

        managed_refcnt_monitor(hpx::future<id_type>&& id)
          : base_type(std::move(id))
        {
        }

        /// Create a new component on the target locality.
        explicit managed_refcnt_monitor(naming::gid_type const& locality)
          : base_type()
          , flag_promise_()
          , flag_(flag_promise_.get_future())
          , locality_(naming::get_locality_from_gid(locality),
                hpx::id_type::management_type::unmanaged)
        {
            static_cast<base_type&>(*this) =
                hpx::new_<server::managed_refcnt_checker>(
                    locality_, flag_promise_.get_id());
        }

        /// Create a new component on the target locality.
        explicit managed_refcnt_monitor(hpx::id_type const& locality)
          : base_type()
          , flag_promise_()
          , flag_(flag_promise_.get_future())
          , locality_(naming::get_locality_from_id(locality))
        {
            static_cast<base_type&>(*this) =
                hpx::new_<server::managed_refcnt_checker>(
                    locality_, flag_promise_.get_id());
        }

        hpx::future<void> take_reference_async(hpx::id_type const& gid)
        {
            return this->base_type::take_reference_async(get_id(), gid);
        }

        void take_reference(hpx::id_type const& gid)
        {
            return this->base_type::take_reference(get_id(), gid);
        }

        bool is_ready()
        {
            // Flush pending reference counting operations on the target locality.
            agas::garbage_collect(locality_);

            return flag_.is_ready();
        }

        template <typename Duration>
        bool is_ready(Duration const& d)
        {
            // Flush pending reference counting operations on the target locality.
            agas::garbage_collect(locality_);

            // keep ourselves alive
            threads::thread_id_ref_type self_id = threads::get_self_id();

            // Schedule a wakeup.
            threads::set_thread_state(
                self_id.noref(), d, threads::thread_schedule_state::pending);

#ifdef HPX_HAVE_MODULE_LIKWID
            hpx::likwid::suspend_region region;
#endif
            // Suspend this thread.
            threads::get_self().yield(threads::thread_result_type(
                threads::thread_schedule_state::suspended,
                hpx::threads::invalid_thread_id));

            return flag_.is_ready();
        }
    };

    struct managed_object
      : components::client_base<managed_object, stubs::managed_refcnt_checker>
    {
        typedef components::client_base<managed_object,
            stubs::managed_refcnt_checker>
            base_type;

    public:
        typedef server::managed_refcnt_checker server_type;

        managed_object(hpx::future<id_type>&& id)
          : base_type(std::move(id))
        {
        }

        /// Create a new component on the target locality.
        explicit managed_object(naming::gid_type const& locality)
          : base_type(hpx::new_<server::managed_refcnt_checker>(
                hpx::id_type(
                    locality, hpx::id_type::management_type::unmanaged),
                hpx::invalid_id))
        {
        }

        /// Create a new component on the target locality.
        explicit managed_object(hpx::id_type const& locality)
          : base_type(hpx::new_<server::managed_refcnt_checker>(
                locality, hpx::invalid_id))
        {
        }
    };

}}    // namespace hpx::test

#endif
