//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/runtime_components/console_logging.hpp>
#include <hpx/runtime_components/server/console_logging.hpp>

#include <cstddef>
#include <mutex>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// definitions related to console logging

namespace hpx::util::detail {

    struct log_lock_tag
    {
    };

    hpx::util::spinlock& get_log_lock()
    {
        hpx::util::static_<hpx::util::spinlock, log_lock_tag> lock;
        return lock.get();
    }
}    // namespace hpx::util::detail

///////////////////////////////////////////////////////////////////////////////
// This must be in global namespace
HPX_REGISTER_ACTION_ID(hpx::components::server::console_logging_action<>,
    console_logging_action, hpx::actions::console_logging_action_id)

///////////////////////////////////////////////////////////////////////////////
namespace hpx { namespace components { namespace server {

    ///////////////////////////////////////////////////////////////////////////
    // implementation of console based logging
    void console_logging(messages_type const& msgs)
    {
        std::lock_guard<util::spinlock> l(util::detail::get_log_lock());

        using hpx::get;

        for (message_type const& msg : msgs)
        {
            logging_destination const dest = get<0>(msg);
            std::size_t const level = get<1>(msg);
            std::string const& s = get<2>(msg);

            switch (dest)
            {
            // NOLINTNEXTLINE(bugprone-branch-clone)
            case logging_destination::hpx:
                LHPX_CONSOLE_(level) << s;
                break;

            case logging_destination::timing:
                LTIM_CONSOLE_(level) << s;
                break;

            case logging_destination::agas:
                LAGAS_CONSOLE_(level) << s;
                break;

            case logging_destination::parcel:
                LPT_CONSOLE_(level) << s;
                break;

            case logging_destination::app:
                LAPP_CONSOLE_(level) << s;
                break;

            case logging_destination::debuglog:
                LDEB_CONSOLE_ << s;
                break;

            default:
                HPX_UNUSED(level);
                break;
            }
        }
    }
}}}    // namespace hpx::components::server
