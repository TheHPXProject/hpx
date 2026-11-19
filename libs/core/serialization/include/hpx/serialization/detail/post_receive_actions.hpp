//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

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

        // One of the steps has finished, with 'error' if it failed. The
        // completion keeps the first error: the data of the parcel (and of
        // the parcels after it, which may use the same objects) is
        // incomplete. Calls all waiters once the last step has finished;
        // rethrows the first exception a waiter throws after that.
        void step_done(std::exception_ptr error = nullptr);

        // calls f once all steps have finished, right away if they have
        void when_done(std::function<void()> f);

        // once all steps have finished: the first error of a step, if any
        [[nodiscard]] std::exception_ptr error();

    private:
        std::mutex mtx_;
        std::size_t pending_;
        bool done_ = false;
        std::exception_ptr error_;
        std::vector<std::function<void()>> waiters_;
    };

    // What a post-receive step calls once it has finished: done() if it
    // succeeded, done(error) if it failed.
    HPX_CXX_CORE_EXPORT class receive_step_done
    {
    public:
        receive_step_done() = default;

        template <typename F,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<F>, receive_step_done> &&
                std::is_invocable_v<F&, std::exception_ptr>>>
        // NOLINTNEXTLINE(bugprone-forwarding-reference-overload)
        receive_step_done(F&& f)
          : f_(HPX_FORWARD(F, f))
        {
        }

        void operator()(std::exception_ptr error = nullptr) const
        {
            f_(HPX_MOVE(error));
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return static_cast<bool>(f_);
        }

    private:
        std::function<void(std::exception_ptr)> f_;
    };

    // Work a load() function defers until the data of its zero-copy chunks
    // has arrived. With zero-copy receive, a parcel is de-serialized before
    // the networking layer has placed the chunk data at the addresses handed
    // to load_binary_chunk; a load() that can't use the final destination
    // (e.g. device memory) claims a staging buffer instead and registers here
    // the step that moves the data from there. The parcel starts these steps
    // once the data is complete; each step may finish asynchronously and
    // calls 'done' when it has (with the error if it failed), and the
    // parcel's action is scheduled after the last one (not at all if a step
    // failed).
    HPX_CXX_CORE_EXPORT class post_receive_actions
    {
    public:
        using done_type = receive_step_done;
        using action_type = std::function<void(done_type)>;
        using actions_type = std::vector<action_type>;

        post_receive_actions() = default;

        void add(action_type f)
        {
            actions_.push_back(HPX_MOVE(f));
        }

        // A step that moves data into the parcel's memory (e.g. receiving a
        // chunk of device memory over a device_chunk_transport): the steps
        // added with add() start once all of these have finished, they may
        // use the data (e.g. copy it from a staging buffer).
        void add_transfer(action_type f)
        {
            transfers_.push_back(HPX_MOVE(f));
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return actions_.empty() && transfers_.empty();
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
            transfers_.clear();
            last_.reset();
        }

    private:
        actions_type actions_;
        actions_type transfers_;
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

#include <hpx/config/warnings_suffix.hpp>
