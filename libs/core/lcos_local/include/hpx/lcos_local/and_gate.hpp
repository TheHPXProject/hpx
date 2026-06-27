//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/lcos_local/conditional_cv.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <mutex>
#include <type_traits>
#include <utility>

namespace hpx::lcos::local {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Mutex = hpx::spinlock>
    struct base_and_gate
    {
    protected:
        using mutex_type = Mutex;

    private:
        struct condition_list_entry : conditional_cv
        {
            condition_list_entry() = default;

            condition_list_entry* prev = nullptr;
            condition_list_entry* next = nullptr;
        };

        using condition_list_type =
            hpx::detail::intrusive_list<condition_list_entry>;

    public:
        // This constructor initializes the base_and_gate object from the number
        // of participants to synchronize the control flow with.
        explicit base_and_gate(std::size_t count = 0)
          : received_segments_(count)
          , promise_(std::allocator_arg,
                hpx::util::thread_local_caching_allocator<
                    hpx::lockfree::variable_size_stack,
                    hpx::util::internal_allocator<>>{})
          , generation_(1)
        {
        }

        base_and_gate(base_and_gate const& rhs) = delete;
        base_and_gate(base_and_gate&& rhs) noexcept
          : mtx_()
          , received_segments_(HPX_MOVE(rhs.received_segments_))
          , promise_(HPX_MOVE(rhs.promise_))
          , generation_(rhs.generation_)
          , conditions_(HPX_MOVE(rhs.conditions_))
        {
            rhs.generation_ = static_cast<std::size_t>(-1);
        }

        base_and_gate& operator=(base_and_gate const& rhs) = delete;
        base_and_gate& operator=(base_and_gate&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::lock_guard<mutex_type> l(rhs.mtx_);
                mtx_ = mutex_type();
                received_segments_ = HPX_MOVE(rhs.received_segments_);
                promise_ = HPX_MOVE(rhs.promise_);
                generation_ = rhs.generation_;
                rhs.generation_ = static_cast<std::size_t>(-1);
                conditions_ = HPX_MOVE(rhs.conditions_);
            }
            return *this;
        }

        ~base_and_gate() = default;

    protected:
        template <typename Lock>
        bool trigger_conditions(Lock& l)
        {
            bool triggered = false;
            if (!conditions_.empty())
            {
                condition_list_entry* next;
                for (auto* c = conditions_.front(); c != nullptr; c = next)
                {
                    // item may be deleted during processing
                    next = c->next;
                    triggered |= c->set(l);
                }
            }
            return triggered;
        }

    protected:
        // Get a future allowing to wait for the gate to fire
        template <typename OuterLock, typename Ptr = std::nullptr_t>
        hpx::future<void> get_future(OuterLock& outer_lock,
            Ptr generation_value = nullptr, error_code& ec = hpx::throws)
        {
            std::unique_lock<mutex_type> l(mtx_);

            // re-check/trigger condition, if needed
            trigger_conditions(outer_lock);

            if constexpr (!std::is_same_v<std::nullptr_t, Ptr>)
            {
                HPX_ASSERT(generation_ != static_cast<std::size_t>(-1) &&
                    generation_ != 0);

                *generation_value = generation_;
            }

            return promise_.get_future(ec);
        }

    public:
        template <typename Ptr = std::nullptr_t>
        hpx::future<void> get_future(
            Ptr generation_value = nullptr, error_code& ec = hpx::throws)
        {
            hpx::no_mutex mtx;
            std::unique_lock<hpx::no_mutex> lk(mtx);
            return get_future(lk, generation_value, ec);
        }

    protected:
        // Get a shared future allowing to wait for the gate to fire
        template <typename OuterLock, typename Ptr = std::nullptr_t>
        hpx::shared_future<void> get_shared_future(OuterLock& outer_lock,
            Ptr generation_value = nullptr, error_code& ec = hpx::throws)
        {
            std::unique_lock<mutex_type> l(mtx_);

            // re-check/trigger condition, if needed
            trigger_conditions(outer_lock);

            if constexpr (!std::is_same_v<std::nullptr_t, Ptr>)
            {
                HPX_ASSERT(generation_ != static_cast<std::size_t>(-1) &&
                    generation_ != 0);
                *generation_value = generation_;
            }

            return promise_.get_shared_future(ec);
        }

    public:
        template <typename Ptr = std::nullptr_t>
        hpx::shared_future<void> get_shared_future(
            Ptr generation_value = nullptr, error_code& ec = hpx::throws)
        {
            hpx::no_mutex mtx;
            std::unique_lock<hpx::no_mutex> lk(mtx);
            return get_shared_future(lk, generation_value, ec);
        }

    protected:
        // Set the data which has to go into the segment \a which.
        template <typename OuterLock, typename F>
        bool set(std::size_t which, OuterLock& outer_lock, F&& f,
            error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(outer_lock);

            {
                std::unique_lock<mutex_type> l(mtx_);
                if (which >= received_segments_.size())
                {
                    // out of bounds index
                    l.unlock();
                    outer_lock.unlock();
                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "base_and_gate<>::set",
                        "index is out of range for this base_and_gate");
                    return false;
                }
                if (received_segments_.test(which))
                {
                    // segment already filled, logic error
                    l.unlock();
                    outer_lock.unlock();
                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "base_and_gate<>::set",
                        "input with the given index has already been "
                        "triggered");
                    return false;
                }

                if (&ec != &throws)
                    ec = make_success_code();

                // set the corresponding bit
                received_segments_.set(which);

                if (received_segments_.count() == received_segments_.size())
                {
                    // we have received the last missing segment
                    using allocator_type =
                        hpx::util::thread_local_caching_allocator<
                            hpx::lockfree::variable_size_stack,
                            hpx::util::internal_allocator<>>;

                    hpx::promise<void> p(std::allocator_arg, allocator_type{});
                    std::swap(p, promise_);
                    received_segments_.reset();    // reset data store

                    l.unlock();

                    p.set_value();    // fire event

                    if constexpr (!std::is_same_v<std::nullptr_t,
                                      std::decay_t<F>>)
                    {
                        // invoke callback with the outer lock being held
                        HPX_FORWARD(F, f)(outer_lock, *this, ec);
                    }

                    return true;
                }
            }
            return false;
        }

    public:
        template <typename F = std::nullptr_t>
        bool set(std::size_t which, F&& f = nullptr, error_code& ec = throws)
        {
            hpx::no_mutex mtx;
            std::unique_lock<hpx::no_mutex> lk(mtx);
            return set(which, lk, HPX_FORWARD(F, f), ec);
        }

    protected:
        bool test_condition(std::size_t generation_value) const noexcept
        {
            return generation_value <= generation_;
        }

        struct manage_condition
        {
            manage_condition(
                base_and_gate& gate, condition_list_entry& cond) noexcept
              : this_(gate)
              , e_(cond)
            {
                this_.conditions_.push_back(cond);
            }

            manage_condition(manage_condition const&) = delete;
            manage_condition(manage_condition&&) = delete;
            manage_condition& operator=(manage_condition const&) = delete;
            manage_condition& operator=(manage_condition&&) = delete;

            ~manage_condition()
            {
                this_.conditions_.erase(&e_);
            }

            template <typename Condition, typename Lock>
            void wait(Condition&& func, Lock& l)
            {
                return e_.wait(HPX_FORWARD(Condition, func), l);
            }

            base_and_gate& this_;
            condition_list_entry& e_;
        };

    protected:
        template <typename Lock>
        void synchronize(std::size_t generation_value, Lock& l,
            char const* function_name = "base_and_gate<>::synchronize",
            error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(l);

            if (generation_value < generation_)
            {
                l.unlock();
                HPX_THROWS_IF(ec, hpx::error::invalid_status, function_name,
                    "sequencing error, generational counter too small");
                return;
            }

            // make sure this set operation has not arrived ahead of time
            if (!test_condition(generation_value))
            {
                condition_list_entry c;
                manage_condition cond(*this, c);

                cond.wait(hpx::bind_front(&base_and_gate::test_condition, this,
                              generation_value),
                    l);
            }

            if (&ec != &throws)
                ec = make_success_code();
        }

    public:
        // Wait for the generational counter to reach the requested stage.
        void synchronize(std::size_t generation_value,
            char const* function_name = "base_and_gate<>::synchronize",
            error_code& ec = throws)
        {
            std::unique_lock<mutex_type> l(mtx_);
            synchronize(generation_value, l, function_name, ec);
        }

    public:
        template <typename Lock>
        std::size_t next_generation(
            Lock& l, std::size_t new_generation, error_code& ec = throws)
        {
            HPX_ASSERT_OWNS_LOCK(l);

            HPX_ASSERT(generation_ != static_cast<std::size_t>(-1));

            if (new_generation != static_cast<std::size_t>(-1))
            {
                if (new_generation < generation_)
                {
                    l.unlock();
                    HPX_THROWS_IF(ec, hpx::error::invalid_status,
                        "and_gate::next_generation",
                        "sequencing error, new generational counter value too "
                        "small");
                    return generation_;
                }
                generation_ = new_generation;
            }

            std::size_t const retval = ++generation_;

            trigger_conditions(l);    // re-check/trigger condition, if needed

            return retval;
        }

        std::size_t next_generation(
            std::size_t new_generation = static_cast<std::size_t>(-1),
            error_code& ec = throws)
        {
            std::unique_lock<mutex_type> l(mtx_);
            return next_generation(l, new_generation, ec);
        }

        template <typename Lock>
        std::size_t generation(Lock& l) const noexcept
        {
            HPX_ASSERT_OWNS_LOCK(l);
            return generation_;
        }

        std::size_t generation() const
        {
            std::lock_guard<mutex_type> l(mtx_);
            return generation(l);
        }

    protected:
        template <typename OuterLock, typename Lock>
        void init_locked(OuterLock& outer_lock, Lock& l, std::size_t count,
            error_code& ec = throws)
        {
            if (0 != received_segments_.count())
            {
                // reset happens while part of the slots are filled
                l.unlock();
                outer_lock.unlock();
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "base_and_gate<>::init",
                    "initializing this base_and_gate while slots are filled");
                return;
            }

            if (received_segments_.size() != count)
            {
                received_segments_.resize(count);    // resize the bitmap
            }
            received_segments_.reset();    // reset all existing bits

            if (&ec != &throws)
                ec = make_success_code();
        }

    private:
        HPX_NO_UNIQUE_ADDRESS mutable mutex_type mtx_;
        hpx::detail::dynamic_bitset<> received_segments_;
        hpx::promise<void> promise_;
        std::size_t generation_;
        condition_list_type conditions_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Note: This type is not thread-safe. It has to be protected from
    //       concurrent access by different threads by the code using instances
    //       of this type.
    HPX_CXX_CORE_EXPORT struct and_gate : base_and_gate<hpx::no_mutex>
    {
    private:
        using base_type = base_and_gate<hpx::no_mutex>;

    public:
        explicit and_gate(std::size_t count = 0)
          : base_type(count)
        {
        }

        and_gate(and_gate const&) = delete;
        and_gate(and_gate&& rhs) = default;
        and_gate& operator=(and_gate const&) = delete;
        and_gate& operator=(and_gate&& rhs) = default;

        ~and_gate() = default;

        template <typename Lock, typename Ptr = std::nullptr_t>
        hpx::future<void> get_future(Lock& l, Ptr generation_value = nullptr,
            error_code& ec = hpx::throws)
        {
            return this->base_type::get_future(l, generation_value, ec);
        }

        template <typename Lock, typename Ptr = std::nullptr_t>
        hpx::shared_future<void> get_shared_future(Lock& l,
            Ptr generation_value = nullptr, error_code& ec = hpx::throws)
        {
            return this->base_type::get_shared_future(l, generation_value, ec);
        }

        template <typename Lock, typename F = std::nullptr_t>
        bool set(std::size_t which, Lock& l, F&& f = nullptr,
            error_code& ec = hpx::throws)
        {
            return this->base_type::set(which, l, HPX_FORWARD(F, f), ec);
        }

        template <typename Lock>
        void synchronize(std::size_t generation_value, Lock& l,
            char const* function_name = "and_gate::synchronize",
            error_code& ec = throws)
        {
            this->base_type::synchronize(
                generation_value, l, function_name, ec);
        }
    };
}    // namespace hpx::lcos::local
