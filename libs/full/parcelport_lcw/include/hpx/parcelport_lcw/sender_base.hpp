//  Copyright (c) 2025 Jiakun Yan
//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCW)

#include <hpx/assert.hpp>
#include <hpx/modules/command_line_handling.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/modules/lcw_base.hpp>
#include <hpx/parcelport_lcw/header.hpp>
#include <hpx/parcelport_lcw/locality.hpp>

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::lcw {
    class HPX_EXPORT parcelport;
    struct sender_connection_base;
    struct sender_base
    {
        using connection_type = sender_connection_base;
        using connection_ptr = std::shared_ptr<connection_type>;

        sender_base() = delete;
        sender_base(parcelport* pp) noexcept
          : pp_(pp)
        {
        }
        virtual ~sender_base() {}

        void run() noexcept {}

        virtual connection_ptr create_connection(
            int dest, parcelset::parcelport* pp) = 0;

        bool background_work(size_t num_thread) noexcept;

        // connectionless interface
        using parcel_buffer_type = parcel_buffer;
        using callback_fn_type =
            hpx::move_only_function<void(error_code const&)>;

        bool send_immediate(parcelset::parcelport* pp,
            parcelset::locality const& dest, parcel_buffer_type buffer,
            callback_fn_type&& callbackFn);

        parcelport* pp_;
    };

}    // namespace hpx::parcelset::policies::lcw

#endif
