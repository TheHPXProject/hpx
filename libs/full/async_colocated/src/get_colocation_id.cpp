//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/async_colocated/get_colocation_id.hpp>

namespace hpx {

    hpx::id_type get_colocation_id(
        launch::sync_policy, hpx::id_type const& id, error_code& ec)
    {
        return agas::get_colocation_id(launch::sync, id, ec);
    }

    hpx::future<id_type> get_colocation_id(hpx::id_type const& id)
    {
        auto result = agas::get_colocation_id(id);
        if (result.has_value())
        {
            return hpx::make_ready_future(HPX_MOVE(result).get_value());
        }

        auto f = HPX_MOVE(result).get_future();
        return f;
    }
}    // namespace hpx
