////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/trigger_lco.hpp>
#include <hpx/modules/naming_base.hpp>

#include <exception>
#include <utility>

namespace hpx::applier {

    HPX_CXX_EXPORT template <typename Arg0>
    inline void trigger(hpx::id_type const& k, Arg0&& arg0)
    {
        set_lco_value(k, HPX_FORWARD(Arg0, arg0));
    }

    HPX_CXX_EXPORT inline void trigger(hpx::id_type const& k)
    {
        trigger_lco_event(k);
    }

    HPX_CXX_EXPORT inline void trigger_error(
        hpx::id_type const& k, std::exception_ptr const& e)
    {
        set_lco_error(k, e);
    }

    HPX_CXX_EXPORT inline void trigger_error(
        hpx::id_type const& k, std::exception_ptr&& e)
    {
        set_lco_error(k, e);
    }
}    // namespace hpx::applier
