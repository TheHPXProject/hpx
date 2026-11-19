//  Copyright (c) 2021-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/parallel/task_group.hpp>

#include <exception>

namespace hpx::experimental {

    ///////////////////////////////////////////////////////////////////////////
    task_group::task_group()
      : latch_(1)
      , has_arrived_(false)
    {
    }

#if defined(HPX_DEBUG)
    task_group::~task_group()
    {
        // wait() must have been called
        HPX_ASSERT(latch_.is_ready());
    }
#else
    task_group::~task_group() = default;
#endif

    void task_group::wait()
    {
        bool expected = false;
        if (has_arrived_.compare_exchange_strong(expected, true))
        {
            latch_.arrive_and_wait();
            if (errors_.size() != 0)
            {
                throw errors_;
            }

            if (auto const state = HPX_MOVE(state_))
            {
                state->set_value(hpx::util::unused);
            }
        }
    }

    void task_group::add_exception(std::exception_ptr p)
    {
        errors_.add(HPX_MOVE(p));
    }

    void task_group::serialize(
        serialization::output_archive& ar, unsigned const)
    {
        if (!latch_.is_ready())
        {
            ar.request_await();
            if (ar.is_preprocessing())
            {
                using init_no_addref = shared_state_type::init_no_addref;
                state_.reset(new shared_state_type(init_no_addref{}), false);
                preprocess_future(ar, *state_);
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
        state_.reset();
    }
}    // namespace hpx::experimental
