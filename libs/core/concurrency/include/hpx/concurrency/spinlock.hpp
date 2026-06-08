////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>

#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/tracing.hpp>

#include <string>
#include <utility>

namespace hpx::util {

    // Lockable spinlock class
    HPX_CXX_CORE_EXPORT struct spinlock
    {
    public:
        spinlock(spinlock const&) = delete;
        spinlock(spinlock&&) = delete;
        spinlock& operator=(spinlock const&) = delete;
        spinlock& operator=(spinlock&&) = delete;

    private:
        hpx::util::detail::spinlock m;
        HPX_NO_UNIQUE_ADDRESS hpx::tracing::lock_context context_;

    public:
        spinlock() noexcept
          : context_("hpx::spinlock", this)
        {
        }

        explicit spinlock(char const* desc) noexcept
          : context_("util::spinlock#", desc, this)
        {
        }

        ~spinlock() {}

        void lock() noexcept(
            noexcept(util::register_lock(std::declval<spinlock*>())))
        {
            bool const run_after = context_.before_lock();
            m.lock();

            if (run_after)
                context_.after_lock();
            util::register_lock(this);
        }

        bool try_lock() noexcept(
            noexcept(util::register_lock(std::declval<spinlock*>())))
        {
            bool const run_after = context_.before_lock();

            if (m.try_lock())
            {
                if (run_after)
                    context_.after_try_lock(true);
                util::register_lock(this);
                return true;
            }
            if (run_after)
                context_.after_try_lock(false);
            return false;
        }

        void unlock() noexcept(
            noexcept(util::unregister_lock(std::declval<spinlock*>())))
        {
            context_.before_unlock();
            m.unlock();

            context_.after_unlock();
            util::unregister_lock(this);
        }
    };
}    // namespace hpx::util
