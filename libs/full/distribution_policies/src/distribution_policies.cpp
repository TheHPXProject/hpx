//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/distribution_policies/colocating_distribution_policy.hpp>
#include <hpx/distribution_policies/container_distribution_policy.hpp>
#include <hpx/distribution_policies/default_distribution_policy.hpp>
#include <hpx/distribution_policies/explicit_container_distribution_policy.hpp>
#include <hpx/distribution_policies/target_distribution_policy.hpp>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx {

    container_distribution_policy const container_layout{};
    explicit_container_distribution_policy const explicit_container_layout{};

    namespace components {

        colocating_distribution_policy const colocated{};
        default_distribution_policy const default_layout{};
        target_distribution_policy const target{};
    }    // namespace components
}    // namespace hpx

#include <hpx/config/warnings_suffix.hpp>
