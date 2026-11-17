//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <functional>
#include <utility>
#include <vector>

namespace hpx::serialization::detail {

    // Work a load() function defers until the data of its zero-copy chunks
    // has arrived. With zero-copy receive, a parcel is de-serialized before
    // the networking layer has placed the chunk data at the addresses handed
    // to load_binary_chunk; a load() that can't use the final destination
    // (e.g. device memory) claims a staging buffer instead and registers here
    // the step that moves the data from there. The parcel starts these steps
    // once the data is complete; each step may finish asynchronously and
    // calls 'done' when it has, and the parcel's action is scheduled after
    // the last one.
    HPX_CXX_CORE_EXPORT class post_receive_actions
    {
    public:
        using done_type = std::function<void()>;
        using action_type = std::function<void(done_type)>;
        using actions_type = std::vector<action_type>;

        post_receive_actions() = default;

        void add(action_type f)
        {
            actions_.push_back(HPX_MOVE(f));
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return actions_.empty();
        }

        // hand the actions to the parcel that was just de-serialized
        [[nodiscard]] actions_type move_actions() noexcept
        {
            return HPX_MOVE(actions_);
        }

    private:
        actions_type actions_;
    };
}    // namespace hpx::serialization::detail

// This is explicitly instantiated to ensure that the id is stable across shared
// libraries.
template <>
struct hpx::util::extra_data_helper<
    hpx::serialization::detail::post_receive_actions>
{
    HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
    static constexpr void reset(
        serialization::detail::post_receive_actions*) noexcept
    {
    }
};
