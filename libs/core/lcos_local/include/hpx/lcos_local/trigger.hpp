//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/lcos_local/conditional_trigger.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <mutex>
#include <utility>

namespace hpx::lcos::local {

    ///////////////////////////////////////////////////////////////////////////
    /// Provides a mutex-protected base implementation of a local trigger.
    HPX_CXX_CORE_EXPORT template <typename Mutex = hpx::spinlock>
    struct base_trigger
    {
    protected:
        using mutex_type = Mutex;

    private:
        struct condition_list_entry : conditional_trigger
        {
            condition_list_entry() = default;

            condition_list_entry* prev = nullptr;
            condition_list_entry* next = nullptr;
        };

        using condition_list_type =
            hpx::detail::intrusive_list<condition_list_entry>;

    public:
        base_trigger() noexcept
          : generation_(0)
        {
        }

        base_trigger(base_trigger&& rhs) noexcept
          : mtx_()
          , promise_(HPX_MOVE(rhs.promise_))
          , generation_(rhs.generation_)
          , conditions_(HPX_MOVE(rhs.conditions_))
        {
            rhs.generation_ = static_cast<std::size_t>(-1);
        }

        base_trigger& operator=(base_trigger&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::lock_guard<mutex_type> l(rhs.mtx_);
                mtx_ = mutex_type();
                promise_ = HPX_MOVE(rhs.promise_);
                generation_ = rhs.generation_;
                rhs.generation_ = static_cast<std::size_t>(-1);
                conditions_ = HPX_MOVE(rhs.conditions_);
            }
            return *this;
        }

    protected:
        bool trigger_conditions(error_code& ec = throws)
        {
            bool triggered = false;
            error_code rc(throwmode::lightweight);
            condition_list_entry* next = conditions_.front();
            for (auto* c = next; c != nullptr; c = next)
            {
                // item me be deleted during processing
                next = c->next;
                triggered |= c->set(rc);

                if (rc && (&ec != &throws))
                {
                    ec = rc;
                }
            }
            return triggered;
        }

    public:
        /// \brief get a future allowing to wait for the trigger to fire
        hpx::future<void> get_future(std::size_t* generation_value = nullptr,
            error_code& ec = hpx::throws)
        {
            std::lock_guard<mutex_type> l(mtx_);

            HPX_ASSERT(generation_ != static_cast<std::size_t>(-1));
            ++generation_;

            trigger_conditions(ec);    // re-check/trigger condition, if needed
            if (!ec)
            {
                if (generation_value)
                {
                    *generation_value = generation_;
                }
                return promise_.get_future(ec);
            }
            return hpx::future<void>();
        }

        /// \brief Trigger this object.
        bool set(error_code& ec = throws)
        {
            std::lock_guard<mutex_type> l(mtx_);

            if (&ec != &throws)
                ec = make_success_code();

            promise_.set_value();    // fire event
            promise_ = hpx::promise<void>();

            if (&ec != &throws)
                ec = make_success_code();

            return true;
        }

    private:
        bool test_condition(std::size_t const generation_value) const noexcept
        {
            return generation_value <= generation_;
        }

        struct manage_condition
        {
            manage_condition(
                base_trigger& gate, condition_list_entry& cond) noexcept
              : this_(gate)
              , e_(cond)
            {
                this_.conditions_.push_back(cond);
            }

            ~manage_condition()
            {
                this_.conditions_.erase(&e_);
            }

            template <typename Condition>
            hpx::future<void> get_future(
                Condition&& func, error_code& ec = hpx::throws)
            {
                return e_.get_future(HPX_FORWARD(Condition, func), ec);
            }

            base_trigger& this_;
            condition_list_entry& e_;
        };

    public:
        /// \brief Wait for the generational counter to reach the requested
        ///        stage.
        void synchronize(std::size_t generation_value,
            char const* function_name = "trigger::synchronize",
            error_code& ec = throws)
        {
            std::unique_lock<mutex_type> l(mtx_);
            synchronize(generation_value, l, function_name, ec);
        }

    protected:
        template <typename Lock>
        void synchronize(std::size_t generation_value, Lock& l,
            char const* function_name = "trigger::synchronize",
            error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(l);

            if (generation_value < generation_)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status, function_name,
                    "sequencing error, generational counter too small");
                return;
            }

            // make sure this set operation has not arrived ahead of time
            if (!test_condition(generation_value))
            {
                condition_list_entry c;
                manage_condition cond(*this, c);

                hpx::future<void> f = cond.get_future(hpx::bind_front(
                    &base_trigger::test_condition, this, generation_value));

                {
                    hpx::unlock_guard<Lock> ul(l);
                    f.get();
                }    // make sure lock gets re-acquired
            }

            if (&ec != &throws)
                ec = make_success_code();
        }

    public:
        std::size_t next_generation()
        {
            std::lock_guard<mutex_type> l(mtx_);
            HPX_ASSERT(generation_ != static_cast<std::size_t>(-1));
            std::size_t retval = ++generation_;

            trigger_conditions();    // re-check/trigger condition, if needed

            return retval;
        }

        std::size_t generation() const
        {
            std::lock_guard<mutex_type> l(mtx_);
            return generation_;
        }

    private:
        mutable mutex_type mtx_;
        hpx::promise<void> promise_;
        std::size_t generation_;
        condition_list_type conditions_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Note: This type is not thread-safe. It has to be protected from
    //       concurrent access by different threads by the code using instances
    //       of this type.
    HPX_CXX_CORE_EXPORT struct trigger : public base_trigger<hpx::no_mutex>
    {
    private:
        using base_type = base_trigger<hpx::no_mutex>;

    public:
        trigger() = default;

        trigger(trigger&& rhs) noexcept
          : base_type(HPX_MOVE(static_cast<base_type&>(rhs)))
        {
        }

        trigger& operator=(trigger&& rhs) noexcept
        {
            if (this != &rhs)
                static_cast<base_type&>(*this) = HPX_MOVE(rhs);
            return *this;
        }

        template <typename Lock>
        void synchronize(std::size_t generation_value, Lock& l,
            char const* function_name = "trigger::synchronize",
            error_code& ec = throws)
        {
            this->base_type::synchronize(
                generation_value, l, function_name, ec);
        }
    };
}    // namespace hpx::lcos::local
