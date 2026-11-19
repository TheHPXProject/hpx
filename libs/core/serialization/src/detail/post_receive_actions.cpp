//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/serialization/detail/post_receive_actions.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    extra_data_id_type extra_data_helper<
        serialization::detail::post_receive_actions>::id() noexcept
    {
        static std::uint8_t id = 0;
        return &id;
    }
}    // namespace hpx::util

namespace hpx::serialization::detail {

    post_receive_actions::parcel_steps post_receive_actions::take_parcel_steps()
    {
        if (actions_.empty())
        {
            // only the steps of earlier parcels, if any
            return {{}, last_};
        }

        auto completion = receive_completion::create(last_, actions_.size());
        last_ = completion;
        return {HPX_MOVE(actions_), HPX_MOVE(completion)};
    }

    std::shared_ptr<receive_completion> receive_completion::create(
        std::shared_ptr<receive_completion> previous, std::size_t const steps)
    {
        auto result =
            std::make_shared<receive_completion>(steps + (previous ? 1 : 0));
        if (previous)
        {
            // counts as one more step
            previous->when_done([result]() { result->step_done(); });
        }
        return result;
    }

    void receive_completion::step_done()
    {
        std::vector<std::function<void()>> waiters;
        {
            std::lock_guard<std::mutex> l(mtx_);
            if (--pending_ != 0)
            {
                return;
            }
            done_ = true;
            waiters.swap(waiters_);
        }
        for (auto& f : waiters)
        {
            f();
        }
    }

    void receive_completion::when_done(std::function<void()> f)
    {
        {
            std::lock_guard<std::mutex> l(mtx_);
            if (!done_)
            {
                waiters_.push_back(HPX_MOVE(f));
                return;
            }
        }
        f();
    }
}    // namespace hpx::serialization::detail
