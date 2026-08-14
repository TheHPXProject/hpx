//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file lcos_local/and_gate.hpp
/// \page hpx::lcos::local::and_gate
/// \headerfile hpx/modules/lcos_local.hpp

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
    /// \brief A gate that waits for a fixed number of participants to "arrive"
    ///        before releasing all waiters (an AND-synchronization primitive).
    ///
    /// The base_and_gate implements a synchronization construct similar to a
    /// countdown latch/barrier: it is initialized with a number of expected
    /// participants ("segments"). Each participant signals its arrival, and
    /// once all expected participants have arrived, futures obtained from
    /// get_future() become ready and callbacks passed to set() are invoked.
    ///
    /// After firing, the gate can be reused for subsequent synchronization
    /// rounds. The generation is advanced explicitly with next_generation().
    /// Callers can use the generation number to guard against stale/late
    /// notifications from a previous round.
    ///
    /// \tparam Mutex The mutex type protecting internal state. Defaults to
    ///               hpx::spinlock.
    ///
    /// \note Not copyable, but movable. Moving invalidates the moved-from
    ///       object's generation (set to an invalid-generation sentinel).
    ///
    /// \note Typically used via hpx::lcos::local::and_gate rather than
    ///       directly.
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
        /// \brief Construct a base_and_gate from the number of participants to
        ///        synchronize the control flow with.
        ///
        /// \param count The number of inputs that need to be received
        ///              before the gate is triggered.
        explicit base_and_gate(std::size_t const count = 0)
          : received_segments_(count)
          , promise_(std::allocator_arg,
                hpx::util::thread_local_caching_allocator<
                    hpx::lockfree::variable_size_stack,
                    hpx::util::internal_allocator<>>{})
          , generation_(1)
        {
        }

        /// base_and_gate is not copyable nor is it copy-assignable.
        base_and_gate(base_and_gate const& rhs) = delete;
        base_and_gate& operator=(base_and_gate const& rhs) = delete;

        /// \brief Move-construct a base_and_gate, leaving \p rhs in a
        ///        moved-from state.
        ///
        /// \param rhs The base_and_gate to move from.
        base_and_gate(base_and_gate&& rhs) noexcept
          : mtx_()
          , received_segments_(HPX_MOVE(rhs.received_segments_))
          , promise_(HPX_MOVE(rhs.promise_))
          , generation_(rhs.generation_)
          , conditions_(HPX_MOVE(rhs.conditions_))
        {
            rhs.generation_ = static_cast<std::size_t>(-1);
        }

        /// \brief Move-assign a base_and_gate, leaving \p rhs in a
        ///        moved-from state.
        ///
        /// \param rhs The base_and_gate to move from.
        ///
        /// \returns A reference to \c *this.
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
        bool trigger_conditions(Lock& l) noexcept
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
        /// \brief Get a future allowing to wait for the gate to fire.
        ///
        /// \tparam Ptr Type of the (optional) pointer used to receive the
        ///             current generation value.
        ///
        /// \param generation_value [out] Optional pointer to a variable
        ///                         that will receive the current generation
        ///                         value of the gate.
        /// \param ec Used to hold error information if the operation fails.
        ///
        /// \returns A future that becomes ready once the gate has fired.
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
        /// \brief Get a shared future allowing to wait for the gate to fire.
        ///
        /// \tparam Ptr Type of the (optional) pointer used to receive the
        ///             current generation value.
        ///
        /// \param generation_value [out] Optional pointer to a variable
        ///                         that will receive the current generation
        ///                         value of the gate.
        /// \param ec Used to hold error information if the operation fails.
        ///
        /// \returns A shared future that becomes ready once the gate has fired.
        template <typename Ptr = std::nullptr_t>
        hpx::shared_future<void> get_shared_future(
            Ptr generation_value = nullptr, error_code& ec = hpx::throws)
        {
            hpx::no_mutex mtx;
            std::unique_lock<hpx::no_mutex> lk(mtx);
            return get_shared_future(lk, generation_value, ec);
        }

    protected:
        template <typename OuterLock, typename F>
        bool set(std::size_t const which, OuterLock& outer_lock, F&& f,
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
        /// \brief Mark the segment \p which as received.
        ///
        /// \tparam F Type of the (optional) callback function invoked once
        ///           the gate is triggered.
        ///
        /// \param which The zero-based index of the segment (input) to set.
        /// \param f Optional callback that is invoked, with the outer lock
        ///          held, when this call causes the gate to fire.
        /// \param ec Used to hold error information if the operation fails.
        ///
        /// \returns \c true if this call caused the gate to fire (i.e. all
        ///          inputs have now been received), \c false otherwise.
        template <typename F = std::nullptr_t>
        bool set(std::size_t which, F&& f = nullptr, error_code& ec = throws)
        {
            hpx::no_mutex mtx;
            std::unique_lock<hpx::no_mutex> lk(mtx);
            return set(which, lk, HPX_FORWARD(F, f), ec);
        }

    protected:
        bool test_condition(std::size_t const generation_value) const noexcept
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
        /// \brief Wait for the generational counter to reach the requested
        ///        stage.
        ///
        /// \param generation_value The generation the caller waits for the
        ///                         gate to reach.
        /// \param function_name The name to be used for error reporting.
        /// \param ec Used to hold error information if the operation fails.
        void synchronize(std::size_t generation_value,
            char const* function_name = "base_and_gate<>::synchronize",
            error_code& ec = throws)
        {
            std::unique_lock<mutex_type> l(mtx_);
            synchronize(generation_value, l, function_name, ec);
        }

    public:
        /// \brief Advance the generational counter of the gate, unblocking any
        ///        waiters whose requested generation has now been reached.
        ///
        /// \tparam Lock Type of the lock object used by the caller to
        ///              protect the gate; must already be locked when this
        ///              overload is called.
        ///
        /// \param l The lock object that is currently holding the gate's
        ///          associated (outer) lock.
        /// \param new_generation The generation value the gate's counter
        ///                       should be reset to before being advanced; if
        ///                       not given the gate is simply advanced to the
        ///                       next generation. The returned generation value
        ///                       is one beyond this value.
        /// \param ec Used to hold error information if the operation fails.
        ///
        /// \returns The new generation value of the gate.
        template <typename Lock>
            requires(!std::is_integral_v<std::decay_t<Lock>>)
        std::size_t next_generation(Lock& l,
            std::size_t const new_generation = static_cast<std::size_t>(-1),
            error_code& ec = throws)
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

        /// \brief Advance the generational counter of the gate, unblocking any
        ///        waiters whose requested generation has now been reached.
        ///
        /// \param new_generation The generation value the gate's counter
        ///                       should be reset to before being advanced; if
        ///                       not given the gate is simply advanced to the
        ///                       next generation. The returned generation value
        ///                       is one beyond this value.
        /// \param ec Used to hold error information if the operation fails.
        ///
        /// \returns The new generation value of the gate.
        std::size_t next_generation(
            std::size_t new_generation = static_cast<std::size_t>(-1),
            error_code& ec = throws)
        {
            std::unique_lock<mutex_type> l(mtx_);
            return next_generation(l, new_generation, ec);
        }

        /// \brief Query the current generation value of the gate.
        ///
        /// \tparam Lock Type of the lock object used by the caller to
        ///              protect the gate; must already be locked when this
        ///              overload is called.
        ///
        /// \param l The lock object that is currently holding the gate's
        ///          associated (outer) lock.
        ///
        /// \returns The current generation value of the gate.
        template <typename Lock>
        std::size_t generation(Lock& l) const noexcept
        {
            HPX_ASSERT_OWNS_LOCK(l);
            return generation_;
        }

        /// \brief Query the current generation value of the gate.
        ///
        /// \returns The current generation value of the gate.
        std::size_t generation() const noexcept
        {
            std::lock_guard<mutex_type> l(mtx_);
            return generation(l);
        }

    protected:
        template <typename OuterLock, typename Lock>
        void init_locked(OuterLock& outer_lock, Lock& l,
            std::size_t const count, error_code& ec = throws)
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
    /// \brief An AND-gate synchronization primitive that fires once all of its
    ///        expected inputs have been received.
    ///
    /// \note This type is not thread-safe. It has to be protected from
    ///       concurrent access by different threads by the code using instances
    ///       of this type.
    HPX_CXX_CORE_EXPORT struct and_gate : base_and_gate<hpx::no_mutex>
    {
    private:
        using base_type = base_and_gate<hpx::no_mutex>;

    public:
        /// \brief Construct an and_gate from the number of participants to
        ///        synchronize the control flow with.
        ///
        /// \param count The number of inputs that need to be received
        ///              before the gate is triggered.
        explicit and_gate(std::size_t const count = 0)
          : base_type(count)
        {
        }

        /// and_gate is not copyable nor is it copy-assignable.
        and_gate(and_gate const&) = delete;
        and_gate& operator=(and_gate const&) = delete;

        /// and_gate is movable and move-assignable.
        and_gate(and_gate&& rhs) = default;
        and_gate& operator=(and_gate&& rhs) = default;

        ~and_gate() = default;

        /// \brief Get a future allowing to wait for the gate to fire.
        ///
        /// \tparam Lock Type of the lock object used by the caller to
        ///              protect the gate.
        /// \tparam Ptr Type of the (optional) pointer used to receive the
        ///             current generation value.
        ///
        /// \param l The lock object that is currently holding the gate's
        ///          associated (outer) lock.
        /// \param generation_value [out] Optional pointer to a variable
        ///                         that will receive the current generation
        ///                         value of the gate.
        /// \param ec Used to hold error information if the operation fails.
        ///
        /// \returns A future that becomes ready once the gate has fired.
        template <typename Lock, typename Ptr = std::nullptr_t>
        hpx::future<void> get_future(Lock& l, Ptr generation_value = nullptr,
            error_code& ec = hpx::throws)
        {
            return this->base_type::get_future(l, generation_value, ec);
        }

        /// \brief Get a shared future allowing to wait for the gate to fire.
        ///
        /// \tparam Lock Type of the lock object used by the caller to
        ///              protect the gate.
        /// \tparam Ptr Type of the (optional) pointer used to receive the
        ///             current generation value.
        ///
        /// \param l The lock object that is currently holding the gate's
        ///          associated (outer) lock.
        /// \param generation_value [out] Optional pointer to a variable
        ///                         that will receive the current generation
        ///                         value of the gate.
        /// \param ec Used to hold error information if the operation fails.
        ///
        /// \returns A shared future that becomes ready once the gate has fired.
        template <typename Lock, typename Ptr = std::nullptr_t>
        hpx::shared_future<void> get_shared_future(Lock& l,
            Ptr generation_value = nullptr, error_code& ec = hpx::throws)
        {
            return this->base_type::get_shared_future(l, generation_value, ec);
        }

        /// \brief Set the data which has to go into the segment \p which.
        ///
        /// \tparam Lock Type of the lock object used by the caller to
        ///              protect the gate.
        /// \tparam F Type of the (optional) callback function invoked once
        ///           the gate is triggered.
        ///
        /// \param which The zero-based index of the segment (input) to set.
        /// \param l The lock object that is currently holding the gate's
        ///          associated (outer) lock.
        /// \param f Optional callback that is invoked, with \p l held, when
        ///          this call causes the gate to fire.
        /// \param ec Used to hold error information if the operation fails.
        ///
        /// \returns \c true if this call caused the gate to fire (i.e. all
        ///          inputs have now been received), \c false otherwise.
        template <typename Lock, typename F = std::nullptr_t>
        bool set(std::size_t which, Lock& l, F&& f = nullptr,
            error_code& ec = hpx::throws)
        {
            return this->base_type::set(which, l, HPX_FORWARD(F, f), ec);
        }

        /// \brief Wait for the generational counter to reach the requested
        ///        stage.
        ///
        /// \tparam Lock Type of the lock object used by the caller to
        ///              protect the gate.
        ///
        /// \param generation_value The generation the caller waits for the
        ///                         gate to reach.
        /// \param l The lock object that is currently holding the gate's
        ///          associated (outer) lock.
        /// \param function_name The name to be used for error reporting.
        /// \param ec Used to hold error information if the operation fails.
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
