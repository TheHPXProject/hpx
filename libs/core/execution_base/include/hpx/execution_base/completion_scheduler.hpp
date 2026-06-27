//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <hpx/execution_base/stdexec_forward.hpp>
#include <utility>

namespace hpx::execution::experimental { namespace detail {

    // clang-format off
    template <typename CPO, typename Sender>
    concept has_completion_scheduler_v = requires(Sender&& s) {
        {
            hpx::execution::experimental::get_completion_scheduler<CPO>(
                hpx::execution::experimental::get_env(std::forward<Sender>(s)))
        } -> hpx::execution::experimental::scheduler;
    };

    HPX_CXX_CORE_EXPORT template <typename ReceiverCPO, typename Sender,
        typename AlgorithmCPO, typename... Ts>
    concept is_completion_scheduler_tag_invocable_v = requires(
        AlgorithmCPO alg, Sender&& snd, Ts&&... ts) {
        tag_invoke(alg,
            hpx::execution::experimental::get_completion_scheduler<ReceiverCPO>(
                hpx::execution::experimental::get_env(snd)),
            std::forward<Sender>(snd), std::forward<Ts>(ts)...);
    };
    // clang-format on
}}    // namespace hpx::execution::experimental::detail
