//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file tuned_setting.hpp
/// \brief A setting chosen by a rule, with the reason it was chosen.

#pragma once

#include <hpx/config.hpp>

#include <string>

namespace hpx::tuning {

    /// One setting chosen by a rule: the configuration key (for instance
    /// "hpx.min_idle_backoff_time"), its value as the configuration writes
    /// it, why the rule chose it, and whether it equals HPX's default.
    HPX_CXX_CORE_EXPORT struct tuned_setting
    {
        std::string key;
        std::string value;
        std::string reason;
        bool is_default = false;
    };
}    // namespace hpx::tuning
