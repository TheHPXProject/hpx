//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/agas_base.hpp>

namespace hpx {

    namespace agas {

        HPX_CXX_EXPORT struct HPX_EXPORT addressing_service;
    }    // namespace agas

    namespace naming {

        HPX_CXX_EXPORT HPX_EXPORT agas::addressing_service& get_agas_client();
        HPX_CXX_EXPORT HPX_EXPORT agas::addressing_service*
        get_agas_client_ptr();
    }    // namespace naming
}    // namespace hpx
