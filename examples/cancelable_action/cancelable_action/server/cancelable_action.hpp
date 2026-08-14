//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/threads.hpp>
#include <hpx/include/util.hpp>

#include <memory>
#include <mutex>

///////////////////////////////////////////////////////////////////////////////
namespace examples::server {

    ///////////////////////////////////////////////////////////////////////////
    inline void delay(int c)
    {
#if defined(HPX_CLANG_VERSION) && (HPX_CLANG_VERSION >= 100000)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#elif defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 100000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvolatile"
#endif
        double volatile d = 0.;
        for (int i = 0; i < c; ++i)
            d += 1 / (2. * i + 1);
        (void) d;
#if defined(HPX_CLANG_VERSION) && (HPX_CLANG_VERSION >= 100000)
#pragma clang diagnostic pop
#elif defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 100000
#pragma GCC diagnostic pop
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    class cancelable_action
      : public hpx::components::component_base<cancelable_action>
    {
    private:
        struct reset_id
        {
            explicit reset_id(cancelable_action& this_)
              : outer_(this_)
            {
                auto const mtx = outer_.mtx_;
                std::lock_guard<hpx::mutex> l(*mtx);

                [[maybe_unused]] hpx::thread::id const old_value = outer_.id_;
                outer_.id_ = hpx::this_thread::get_id();
                HPX_ASSERT(old_value == hpx::thread::id());
            }
            ~reset_id()
            {
                auto const mtx = outer_.mtx_;
                std::lock_guard<hpx::mutex> l(*mtx);

                [[maybe_unused]] hpx::thread::id const old_value = outer_.id_;
                outer_.id_ = hpx::thread::id();
                HPX_ASSERT(old_value != hpx::thread::id());
            }

            cancelable_action& outer_;
        };

    public:
        cancelable_action()
          : mtx_(std::make_shared<hpx::mutex>())
        {
        }

        // Do some lengthy work
        void do_it()
        {
            reset_id r(*this);    // manage thread id

            while (true)
            {
                // do something useful ;-)
                delay(1000);

                // check whether this thread was interrupted and throw a special
                // exception if it was interrupted
                hpx::this_thread::suspend();    // interruption_point();
            }
        }

        // Cancel the lengthy action above
        void cancel_it() const
        {
            // Make sure id_ has been set
            hpx::util::yield_while([this]() {
                auto const mtx = mtx_;
                std::lock_guard<hpx::mutex> l(*mtx);
                return id_ == hpx::thread::id();
            });

            auto const mtx = mtx_;

            std::unique_lock<hpx::mutex> l(*mtx);
            auto const id = id_;

            if (id != hpx::thread::id())
            {
                l.unlock();
                hpx::thread::interrupt(id);
            }
        }

        HPX_DEFINE_COMPONENT_ACTION(cancelable_action, do_it, do_it_action)
        HPX_DEFINE_COMPONENT_ACTION(
            cancelable_action, cancel_it, cancel_it_action)

    private:
        std::shared_ptr<hpx::mutex> mtx_;
        hpx::thread::id id_;
    };
}    // namespace examples::server

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_ACTION_DECLARATION(
    examples::server::cancelable_action::do_it_action,
    cancelable_action_do_it_action)
HPX_REGISTER_ACTION_DECLARATION(
    examples::server::cancelable_action::cancel_it_action,
    cancelable_action_cancel_it_action)

#endif
