//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assertion/source_location.hpp>

namespace hpx::contracts {

    HPX_CXX_CORE_EXPORT enum class contract_kind { pre, post, assertion };

    HPX_CXX_CORE_EXPORT struct violation_info
    {
        contract_kind kind;
        char const* condition;
        hpx::source_location location;
    };

    HPX_CXX_CORE_EXPORT using violation_handler_t =
        void (*)(violation_info const&);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT violation_handler_t
    set_violation_handler(violation_handler_t handler) noexcept;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT violation_handler_t
    get_violation_handler() noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void default_violation_handler(
        violation_info const& info);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void invoke_violation_handler(
        violation_info const& info);

}    // namespace hpx::contracts
