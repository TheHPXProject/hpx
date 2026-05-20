//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/runtime_components/console_error_sink.hpp>
#include <hpx/runtime_components/server/console_error_sink.hpp>

#include <exception>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components {

    // Stub function which applies the console_error_sink action.
    void console_error_sink(
        hpx::id_type const& dst, std::exception_ptr const& e)
    {
        // Report the error only if the thread-manager is up.
        if (threads::threadmanager_is(hpx::state::running))
        {
            if (threads::get_self_ptr())
            {
                hpx::async<server::console_error_sink_action>(dst, e).get();
            }
            else
            {
                // FIXME: This should use a sync_put_parcel.
                hpx::post<server::console_error_sink_action>(dst, e);
            }
        }
    }

    /// Stub function which applies the console_error_sink action.
    ///
    /// \note Must be called from a HPX-thread.
    void console_error_sink(std::exception_ptr const& e)
    {
        if (HPX_UNLIKELY(!threads::get_self_ptr()))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "components::console_error_sink",
                "console_error_sink was not called from a HPX-thread");
        }

        // Report the error only if the thread-manager is up.
        if (threads::threadmanager_is(hpx::state::running))
        {
            // retrieve console locality
            naming::gid_type console_gid;
            naming::get_agas_client().get_console_locality(console_gid);
            hpx::id_type dst(
                console_gid, hpx::id_type::management_type::unmanaged);

            hpx::async<server::console_error_sink_action>(dst, e).get();
        }
    }
}    // namespace hpx::components
