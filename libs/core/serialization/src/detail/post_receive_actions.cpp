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
#include <exception>
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

    namespace {

        // Runs 'actions' and calls 'done' once all of them have finished,
        // with the first error if one failed.
        void run_all(post_receive_actions::actions_type& actions,
            post_receive_actions::done_type done)
        {
            if (actions.empty())
            {
                done();
                return;
            }

            struct state
            {
                std::mutex mtx;
                std::size_t pending;
                std::exception_ptr error;
                post_receive_actions::done_type done;
            };
            auto s = std::make_shared<state>();
            s->pending = actions.size();
            s->done = HPX_MOVE(done);

            for (auto& action : actions)
            {
                action([s](std::exception_ptr error) {
                    std::exception_ptr first;
                    {
                        std::lock_guard<std::mutex> l(s->mtx);
                        if (error && !s->error)
                        {
                            s->error = HPX_MOVE(error);
                        }
                        if (--s->pending != 0)
                        {
                            return;
                        }
                        first = s->error;
                    }
                    s->done(HPX_MOVE(first));
                });
            }
        }
    }    // namespace

    post_receive_actions::parcel_steps post_receive_actions::take_parcel_steps()
    {
        if (!transfers_.empty())
        {
            // one step: the transfers, then the other steps
            auto transfers =
                std::make_shared<actions_type>(HPX_MOVE(transfers_));
            auto actions = std::make_shared<actions_type>(HPX_MOVE(actions_));
            transfers_.clear();
            actions_.clear();
            actions_.push_back([transfers, actions](done_type done) {
                run_all(*transfers,
                    [actions, done = HPX_MOVE(done)](
                        std::exception_ptr error) mutable {
                        // the data is incomplete if a transfer failed
                        if (error)
                        {
                            done(HPX_MOVE(error));
                            return;
                        }
                        run_all(*actions, HPX_MOVE(done));
                    });
            });
        }

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
            // counts as one more step, which fails if the previous parcel's
            // steps did (it is called by 'previous')
            previous->when_done([result, prev = previous.get()]() {
                result->step_done(prev->error());
            });
        }
        return result;
    }

    void receive_completion::step_done(std::exception_ptr error)
    {
        std::vector<std::function<void()>> waiters;
        {
            std::lock_guard<std::mutex> l(mtx_);
            if (error && !error_)
            {
                error_ = HPX_MOVE(error);
            }
            if (--pending_ != 0)
            {
                return;
            }
            done_ = true;
            waiters.swap(waiters_);
        }

        // all waiters run (e.g. those releasing the parcels after this one),
        // also if one of them throws
        std::exception_ptr first;
        for (auto& f : waiters)
        {
            try
            {
                f();
            }
            catch (...)
            {
                if (!first)
                {
                    first = std::current_exception();
                }
            }
        }
        if (first)
        {
            std::rethrow_exception(first);
        }
    }

    std::exception_ptr receive_completion::error()
    {
        std::lock_guard<std::mutex> l(mtx_);
        return error_;
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
