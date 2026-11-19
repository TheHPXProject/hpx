//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace hpx::serialization::detail {

    // The completion of a parcel's steps and of those of the parcels before
    // it in the same archive.
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT receive_completion
    {
    public:
        // 'steps' steps, complete after 'previous' (may be null)
        static std::shared_ptr<receive_completion> create(
            std::shared_ptr<receive_completion> previous, std::size_t steps);

        explicit receive_completion(std::size_t pending) noexcept
          : pending_(pending)
        {
        }

        // one of the steps has finished
        void step_done();

        // calls f once all steps have finished, right away if they have
        void when_done(std::function<void()> f);

    private:
        std::mutex mtx_;
        std::size_t pending_;
        bool done_ = false;
        std::vector<std::function<void()>> waiters_;
    };

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

        // What a parcel just de-serialized waits for: its own steps, and the
        // steps registered for the parcels de-serialized before it from the
        // same archive. Objects shared through pointers are loaded once, by
        // the first parcel referring to them, but the later ones use them as
        // well. 'completion' is null if there's nothing to wait for; the
        // parcel starts 'actions', each calls completion->step_done() when
        // it has finished.
        struct parcel_steps
        {
            actions_type actions;
            std::shared_ptr<receive_completion> completion;
        };
        [[nodiscard]] HPX_CORE_EXPORT parcel_steps take_parcel_steps();

        // a new message: nothing waits for the steps of earlier ones
        void reset() noexcept
        {
            actions_.clear();
            last_.reset();
        }

    private:
        actions_type actions_;
        std::shared_ptr<receive_completion> last_;
    };
}    // namespace hpx::serialization::detail

// This is explicitly instantiated to ensure that the id is stable across shared
// libraries.
template <>
struct hpx::util::extra_data_helper<
    hpx::serialization::detail::post_receive_actions>
{
    HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
    static void reset(serialization::detail::post_receive_actions* p) noexcept
    {
        p->reset();
    }
};
