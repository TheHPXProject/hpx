//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/task_group.hpp>

#include <atomic>
#include <exception>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hpx::experimental {

    ///////////////////////////////////////////////////////////////////////////
    task_group::task_group()
      : state_(std::make_shared<detail::task_group_shared_state>())
    {
    }

#if defined(HPX_DEBUG)
    task_group::~task_group()
    {
        // wait() or wait_as_sender() must have been called
        HPX_ASSERT(!state_ || state_->latch_.is_ready() ||
            state_->wait_called_.load(std::memory_order_relaxed));
    }
#else
    task_group::~task_group() = default;
#endif

    void task_group::wait()
    {
        state_->wait_called_.store(true, std::memory_order_release);

        // 1. Drain and sync_wait any accumulated P2300 senders
        std::exception_ptr sender_error;
        try
        {
            auto sender = detail::drain_task_group_senders(state_);
            hpx::this_thread::experimental::sync_wait(HPX_MOVE(sender));
        }
        catch (...)
        {
            sender_error = std::current_exception();
        }

        // 2. Wait for any legacy executor tasks tracked by latch_
        bool expected = false;
        if (state_->has_arrived_.compare_exchange_strong(expected, true))
        {
            state_->latch_.arrive_and_wait();
            if (auto const state = HPX_MOVE(state_->state_))
            {
                state->set_value(hpx::util::unused);
            }
        }
        else
        {
            state_->latch_.wait();
            if (auto const state = HPX_MOVE(state_->state_))
            {
                state->set_value(hpx::util::unused);
            }
        }

        if (state_->errors_.size() != 0)
        {
            throw state_->errors_;
        }

        if (sender_error)
        {
            std::rethrow_exception(HPX_MOVE(sender_error));
        }
    }

    void task_group::add_exception(std::exception_ptr p)
    {
        state_->add_exception(HPX_MOVE(p));
    }

    void task_group::serialize(
        serialization::output_archive& ar, unsigned const)
    {
        if (!state_->latch_.is_ready() ||
            !state_->senders_drained_.load(std::memory_order_acquire))
        {
            if (ar.is_preprocessing())
            {
                using init_no_addref = detail::task_group_shared_state::
                    shared_state_type::init_no_addref;
                state_->state_.reset(
                    new detail::task_group_shared_state::shared_state_type(
                        init_no_addref{}),
                    false);
                preprocess_future(ar, *state_->state_);
            }
            else
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "task_group::serialize",
                    "task_group must be ready in order for it to be "
                    "serialized");
            }
            return;
        }

        // the state is not needed anymore
        state_->state_.reset();
    }
}    // namespace hpx::experimental
