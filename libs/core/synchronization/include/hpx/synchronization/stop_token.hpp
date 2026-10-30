//  Copyright (c) 2020-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file stop_token.hpp
/// \page hpx::nostopstate, hpx::stop_callback, hpx::stop_source, hpx::stop_token, hpx::nostopstate_t
/// \headerfile hpx/stop_token.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
// This header holds components that can be used to asynchronously request that
// an operation stops execution in a timely manner, typically because the result
// is no longer required. Such a request is called a stop request.
//
// stop_source, stop_token, and stop_callback implement semantics of shared
// ownership of a stop state. Any stop_source, stop_token, or stop_callback that
// shares ownership of the same stop state is an associated stop_source,
// stop_token, or stop_callback, respectively. The last remaining owner of the
// stop state automatically releases the resources associated with the stop
// state.
//
// A stop_token can be passed to an operation which can either:
//  - actively poll the token to check if there has been a stop request, or
//  - register a callback using the stop_callback class template which will
//      be called in the event that a stop request is made.
// A stop request made via a stop_source will be visible to all associated
// stop_token and stop_source objects. Once a stop request has been made it
// cannot be withdrawn (a subsequent stop request has no effect).
//
// Callbacks registered via a stop_callback object are called when a stop
// request is first made by any associated stop_source object.
//
// Calls to the functions request_stop, stop_requested, and stop_possible do not
// introduce data races. A call to request_stop that returns true synchronizes
// with a call to stop_requested on an associated stop_token or stop_source
// object that returns true. Registration of a callback synchronizes with the
// invocation of that callback.

namespace hpx {

    namespace detail {

        struct stop_state;
        HPX_CORE_EXPORT void intrusive_ptr_add_ref(stop_state* p) noexcept;
        HPX_CORE_EXPORT void intrusive_ptr_release(stop_state* p) noexcept;

        ///////////////////////////////////////////////////////////////////////
        struct stop_callback_base
        {
            stop_callback_base* next_ = nullptr;
            stop_callback_base** prev_ = nullptr;
            bool* is_removed_ = nullptr;

            std::atomic<bool> callback_finished_executing_{false};

            virtual void execute() noexcept = 0;

            void add_this_callback(stop_callback_base*& callbacks) noexcept;
            bool remove_this_callback() const noexcept;

            stop_callback_base() = default;

            stop_callback_base(stop_callback_base const&) = delete;
            stop_callback_base(stop_callback_base&&) = delete;
            stop_callback_base& operator=(stop_callback_base const&) = delete;
            stop_callback_base& operator=(stop_callback_base&&) = delete;

        protected:
            virtual ~stop_callback_base() = default;
        };

        ///////////////////////////////////////////////////////////////////////
        struct stop_state
        {
        private:
            using flag_t = std::uint64_t;

            // bits 0-30 - token ref count (31 bits)
            static constexpr flag_t token_ref_increment = 1ull;
            static constexpr flag_t token_ref_mask = 0x7fffffffull;    //-V112

            // bit 31 - stop-requested
            static constexpr flag_t stop_requested_flag = 1ull << 31;

            // bits 32-62 - source ref count (31 bits)
            static constexpr flag_t source_ref_increment = token_ref_increment
                << 32;    // -V112
            static constexpr flag_t source_ref_mask = token_ref_mask
                << 32;    // -V112
            // bit 63 - locked
            static constexpr flag_t locked_flag = stop_requested_flag
                << 32;    // -V112

        public:
            constexpr stop_state() noexcept
              : state_(token_ref_increment)
            {
            }

            stop_state(stop_state const&) = delete;
            stop_state(stop_state&&) = delete;
            stop_state& operator=(stop_state const&) = delete;
            stop_state& operator=(stop_state&&) = delete;

            ~stop_state()
            {
                HPX_ASSERT((state_.load(std::memory_order_relaxed) &
                               stop_state::locked_flag) == 0);
            }

            [[nodiscard]] bool stop_requested() const noexcept
            {
                return stop_requested(state_.load(std::memory_order_acquire));
            }

            // Returns: false if a stop request was not made and there are no
            //      associated stop_source objects; otherwise, true.
            [[nodiscard]] bool stop_possible() const noexcept
            {
                return stop_possible(state_.load(std::memory_order_acquire));
            }

            HPX_CORE_EXPORT bool request_stop() noexcept;

            void add_source_count() noexcept
            {
                state_.fetch_add(stop_state::source_ref_increment,
                    std::memory_order_relaxed);
            }

            void remove_source_count() noexcept
            {
                state_.fetch_sub(stop_state::source_ref_increment,
                    std::memory_order_acq_rel);
            }

            HPX_CORE_EXPORT bool add_callback(stop_callback_base* cb) noexcept;
            HPX_CORE_EXPORT void remove_callback(
                stop_callback_base const* cb) noexcept;

        private:
            [[nodiscard]] static bool is_locked(std::uint64_t state) noexcept
            {
                return (state & stop_state::locked_flag) != 0;
            }

            [[nodiscard]] static bool stop_requested(
                std::uint64_t state) noexcept
            {
                return (state & stop_state::stop_requested_flag) != 0;
            }

            [[nodiscard]] static bool stop_possible(
                std::uint64_t state) noexcept
            {
                // Stop may happen, if it has already been requested or if
                // there are still interrupt_source instances in existence.
                return stop_requested(state) ||
                    (state & stop_state::source_ref_mask) != 0;
            }

            // Effects: locks the state and atomically sets stop-requested
            //      signal
            //
            // Returns: false if stop was requested; otherwise, true.
            HPX_CORE_EXPORT bool lock_and_request_stop() noexcept;

            // Effect: locks the state if no stop was requested and stop is
            //      possible. Also executes callbacks if stop was requested.
            //
            // Returns: false if stop was requested or stop is not possible
            HPX_CORE_EXPORT bool lock_if_not_stopped(
                stop_callback_base* cb) noexcept;

        public:
            // Effect: locks the state
            HPX_CORE_EXPORT void lock() noexcept;

            HPX_FORCEINLINE void unlock() noexcept
            {
                state_.fetch_sub(locked_flag, std::memory_order_release);
            }

        private:
            friend struct scoped_lock_if_not_stopped;
            friend struct scoped_lock_and_request_stop;

            friend HPX_CORE_EXPORT void intrusive_ptr_add_ref(
                stop_state* p) noexcept;
            friend HPX_CORE_EXPORT void intrusive_ptr_release(
                stop_state* p) noexcept;

            std::atomic<std::uint64_t> state_;
            stop_callback_base* callbacks_ = nullptr;
            hpx::threads::thread_id_type signalling_thread_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Callback>
    class [[nodiscard]] stop_callback;

    ///////////////////////////////////////////////////////////////////////////
    //
    // 32.3.3, class stop_token
    //
    // The class stop_token provides an interface for querying whether a stop
    // request has been made (stop_requested) or can ever be made
    // (stop_possible) using an associated stop_source object (32.3.4). A
    // stop_token can also be passed to a stop_callback (32.3.5) constructor to
    // register a callback to be called when a stop request has been made from
    // an associated stop_source.

    /// The \a stop_token class provides the means to check if a stop request has
    /// been made or can be made, for its associated \a hpx::stop_source object.
    /// It is essentially a thread-safe "view" of the associated stop-state.
    /// The stop_token can also be passed to the constructor of \a
    /// hpx::stop_callback, such that the callback will be invoked if the \a
    /// stop_token's associated \a hpx::stop_source is requested to stop. And \a
    /// stop_token can be passed to the interruptible waiting functions of \a
    /// hpx::condition_variable_any, to interrupt the condition variable's wait
    /// if stop is requested.
    ///
    /// \note A \a stop_token object is not generally constructed independently,
    ///       but rather retrieved from a \a hpx::jthread or \a hpx::stop_source.
    ///       This makes it share the same associated stop-state as the \a
    ///       hpx::jthread or \a hpx::stop_source.
    HPX_CXX_CORE_EXPORT class stop_token
    {
    private:
        template <typename Callback>
        friend class stop_callback;
        friend class stop_source;

    public:
        template <typename Callback>
        using callback_type = stop_callback<Callback>;

        // 32.3.3.1 constructors, copy, and assignment

        // Postconditions: stop_possible() is false and stop_requested() is
        //      false. [Note: Because the created stop_token object can never
        //      receive a stop request, no resources are allocated for a stop
        //      state]
        constexpr stop_token() noexcept = default;

        stop_token(stop_token const& rhs) = default;
        stop_token(stop_token&&) noexcept = default;

        stop_token& operator=(stop_token const& rhs) = default;
        stop_token& operator=(stop_token&&) noexcept = default;

        // Effects: Releases ownership of the stop state, if any.
        ~stop_token() = default;

        // Effects: Exchanges the values of *this and rhs.
        /// swaps two stop_token objects
        void swap(stop_token& s) noexcept
        {
            std::swap(state_, s.state_);
        }

        // 32.3.3.2 stop handling

        // Returns: true if *this has ownership of a stop state that has
        //      received a stop request; otherwise, false.
        /// checks whether the associated stop-state has been requested to stop
        [[nodiscard]] bool stop_requested() const noexcept
        {
            return !!state_ && state_->stop_requested();
        }

        //  Returns: false if:
        //      (2.1) *this does not have ownership of a stop state, or
        //      (2.2) a stop request was not made and there are no associated
        //            stop_source objects; otherwise, true.
        /// checks whether associated stop-state can be requested to stop
        [[nodiscard]] bool stop_possible() const noexcept
        {
            return !!state_ && state_->stop_possible();
        }

        // 32.3.3.3 Comparisons

        // Returns: true if lhs and rhs have ownership of the same stop state or
        //      if both lhs and rhs do not have ownership of a stop state;
        //      otherwise false.
        [[nodiscard]] friend constexpr bool operator==(
            stop_token const& lhs, stop_token const& rhs) noexcept
        {
            return lhs.state_ == rhs.state_;
        }

        // Returns: !(lhs==rhs).
        [[nodiscard]] friend constexpr bool operator!=(
            stop_token const& lhs, stop_token const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

    private:
        explicit stop_token(
            hpx::intrusive_ptr<detail::stop_state> state) noexcept
          : state_(HPX_MOVE(state))
        {
        }

    private:
        hpx::intrusive_ptr<detail::stop_state> state_;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    // 32.3.4, class stop_source
    //
    // The class stop_source implements the semantics of making a
    // stop request. A stop request made on a stop_source object is
    // visible to all associated stop_source and stop_token (32.3.3)
    // objects. Once a stop request has been made it cannot be
    // withdrawn (a subsequent stop request has no effect).

    // no-shared-stop-state indicator
    /// Unit type intended for use as a placeholder in hpx::stop_source
    /// non-default constructor, that makes the constructed hpx::stop_source
    /// empty with no associated stop-state.
    HPX_CXX_CORE_EXPORT struct nostopstate_t
    {
        explicit nostopstate_t() = default;
    };

    /// This is a constant object instance of hpx::nostopstate_t for use in
    /// constructing an empty hpx::stop_source, as a placeholder value in the
    /// non-default constructor.
    HPX_CXX_CORE_EXPORT inline constexpr nostopstate_t nostopstate{};

    /// The \a stop_source class provides the means to issue a stop request,
    /// such as for \a hpx::jthread cancellation. A stop request made for one
    /// \a stop_source object is visible to all \a stop_sources and \a
    /// hpx::stop_tokens of the same associated stop-state; any \a
    /// hpx::stop_callback(s) registered for associated \a hpx::stop_token(s)
    /// will be invoked, and any hpx::condition_variable_any objects waiting on
    /// associated \a hpx::stop_token(s) will be awoken. Once a stop is
    /// requested, it cannot be withdrawn. Additional stop requests have no
    /// effect.
    ///
    /// \note For the purposes of \a hpx::jthread cancellation the \a
    ///       stop_source object should be retrieved from the hpx::jthread
    ///       object using \a get_stop_source(); or stop should be requested
    ///       directly from the \a hpx::jthread object using \a request_stop().
    ///       This will then use the same associated stop-state as that passed
    ///       into the \a hpx::jthread's invoked function argument (i.e., the
    ///       function being executed on its thread). For other uses, however, a
    ///       \a stop_source can be constructed separately using the default
    ///       constructor, which creates new stop-state.
    HPX_CXX_CORE_EXPORT class stop_source
    {
    public:
        // 32.3.4.1 constructors, copy, and assignment

        // Effects: Initializes *this to have ownership of a new stop state.
        //
        // Postconditions: stop_possible() is true and stop_requested() is false.
        //
        // Throws: std::bad_alloc if memory could not be allocated for the stop
        //      state.
        stop_source()
          : state_(new detail::stop_state, false)
        {
            state_->add_source_count();
        }

        // Postconditions: stop_possible() is false and stop_requested() is
        //      false. [Note: No resources are allocated for the state.]
        explicit stop_source(nostopstate_t) noexcept {}

        stop_source(stop_source const& rhs) noexcept
          : state_(rhs.state_)
        {
            if (state_)
                state_->add_source_count();
        }
        stop_source(stop_source&&) noexcept = default;

        stop_source& operator=(stop_source const& rhs) noexcept
        {
            state_ = rhs.state_;
            if (state_)
                state_->add_source_count();
            return *this;
        }
        stop_source& operator=(stop_source&&) noexcept = default;

        // Effects: Releases ownership of the stop state, if any.
        ~stop_source()
        {
            if (state_)
                state_->remove_source_count();
        }

        // Effects: Exchanges the values of *this and rhs.
        /// swaps two stop_source objects
        void swap(stop_source& s) noexcept
        {
            std::swap(state_, s.state_);
        }

        // 32.3.4.2 stop handling

        // Returns: stop_token() if stop_possible() is false; otherwise a new
        //      associated stop_token object.
        /// returns a stop_token for the associated stop-state
        [[nodiscard]] stop_token get_token() const noexcept
        {
            if (!stop_possible())
            {
                return {};
            }
            return stop_token(state_);
        }

        // Returns: true if *this has ownership of a stop state; otherwise, false.
        /// checks whether associated stop-state can be requested to stop
        [[nodiscard]] bool stop_possible() const noexcept
        {
            return !!state_;
        }

        // Returns: true if *this has ownership of a stop state that has
        //      received a stop request; otherwise, false.
        /// checks whether the associated stop-state has been requested to stop
        [[nodiscard]] bool stop_requested() const noexcept
        {
            return !!state_ && state_->stop_requested();
        }

        // Effects: If *this does not have ownership of a stop state, returns
        //      false. Otherwise, atomically determines whether the owned stop
        //      state has received a stop request, and if not, makes a stop
        //      request. The determination and making of the stop request are an
        //      atomic read-modify-write operation (6.9.2.1). If the request was
        //      made, the callbacks registered by associated stop_callback
        //      objects are synchronously called. If an invocation of a callback
        //      exits via an exception then std::terminate is called (14.6.1).
        //      [Note: A stop request includes notifying all condition variables
        //      of type condition_variable_any temporarily registered during an
        //      interruptible wait (32.6.4.2). end note]
        //
        // Postconditions: stop_possible() is false or stop_requested() is true.
        //
        // Returns: true if this call made a stop request; otherwise false
        /// makes a stop request for the associated stop-state, if any
        bool request_stop() const noexcept
        {
            return !!state_ && state_->request_stop();
        }

        // 32.3.4.3 Comparisons

        // Returns: true if lhs and rhs have ownership of the same stop state or
        //      if both lhs and rhs do not have ownership of a stop state;
        //      otherwise false.
        [[nodiscard]] friend bool operator==(
            stop_source const& lhs, stop_source const& rhs) noexcept
        {
            return lhs.state_ == rhs.state_;
        }

        [[nodiscard]] friend bool operator!=(
            stop_source const& lhs, stop_source const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

    private:
        hpx::intrusive_ptr<detail::stop_state> state_;
    };

    ///////////////////////////////////////////////////////////////////////////
    //
    // 32.3.5, class stop_callback
    //
    HPX_CXX_CORE_EXPORT template <typename Callback>
    class [[nodiscard]] stop_callback : private detail::stop_callback_base
    {
    public:
        using callback_type = Callback;

        // 32.3.5.1 constructors and destructor

        // Constraints: Callback and CB satisfy constructible_from<Callback, CB>.
        //
        // Preconditions: Callback and CB model constructible_from<Callback, CB>.
        //
        // Effects: Initializes callback with HPX_FORWARD(CB, cb). If
        //      st.stop_requested() is true, then HPX_FORWARD(CB, cb)() is
        //      evaluated in the current thread before the constructor returns.
        //      Otherwise, if st has ownership of a stop state, acquires shared
        //      ownership of that stop state and registers the callback with
        //      that stop state such that HPX_FORWARD(CB, cb)() is evaluated by
        //      the first call to request_stop() on an associated stop_source.
        //
        // Remarks: If evaluating HPX_FORWARD(CB, cb)() exits via
        //      an exception, then std::terminate is called (14.6.1).
        //
        // Throws: Any exception thrown by the initialization of callback.
        template <typename CB,
            typename Enable =
                std::enable_if_t<std::is_constructible_v<Callback, CB>>>
        explicit stop_callback(stop_token const& st, CB&& cb) noexcept(
            std::is_nothrow_constructible_v<Callback, CB>)
          : callback_(HPX_FORWARD(CB, cb))
          , state_(st.state_)
        {
            if (state_ && !state_->add_callback(this))
                state_.reset();
        }

        template <typename CB,
            typename Enable =
                std::enable_if_t<std::is_constructible_v<Callback, CB>>>
        explicit stop_callback(stop_token&& st, CB&& cb) noexcept(
            std::is_nothrow_constructible_v<Callback, CB>)
          : callback_(HPX_FORWARD(CB, cb))
          , state_(HPX_MOVE(st.state_))
        {
            if (state_ && !state_->add_callback(this))
                state_.reset();
        }

        // Effects: Unregisters the callback from the owned stop state, if any.
        //      The destructor does not block waiting for the execution of
        //      another callback registered by an associated stop_callback. If
        //      the callback is concurrently executing on another thread, then
        //      the return from the invocation of callback strongly happens
        //      before (6.9.2.1) callback is destroyed. If callback is executing
        //      on the current thread, then the destructor does not block (3.6)
        //      waiting for the return from the invocation of callback. Releases
        //      ownership of the stop state, if any.
        ~stop_callback()
        {
            if (state_)
                state_->remove_callback(this);
        }

        stop_callback(stop_callback const&) = delete;
        stop_callback(stop_callback&&) = delete;

        stop_callback& operator=(stop_callback const&) = delete;
        stop_callback& operator=(stop_callback&&) = delete;

    private:
        void execute() noexcept override
        {
            callback_();
        }

    private:
        HPX_NO_UNIQUE_ADDRESS Callback callback_;
        hpx::intrusive_ptr<detail::stop_state> state_;
    };

    ////////////////////////////////////////////////////////////////////////////
    //
    // Mandates: stop_callback is instantiated with an argument for the
    //      template parameter Callback that satisfies both invocable and
    //      destructible.
    //
    // Expects: stop_callback is instantiated with an argument for the
    //      template parameter Callback that models both invocable and
    //      destructible.

    /// The \a stop_callback class template provides an RAII object type that
    /// registers a callback function for an associated \a hpx::stop_token
    /// object, such that the callback function will be invoked when the \a
    /// hpx::stop_token's associated \a hpx::stop_source is requested to stop.
    /// Callback functions registered via \a stop_callback's constructor are
    /// invoked either in the same thread that successfully invokes \a
    /// request_stop() for a \a hpx::stop_source of the \a stop_callback's
    /// associated \a hpx::stop_token; or if stop has already been requested
    /// prior to the constructor's registration, then the callback is invoked in
    /// the thread constructing the \a stop_callback. More than one \a
    /// stop_callback can be created for the same \a hpx::stop_token, from the
    /// same or different threads concurrently. No guarantee is provided for the
    /// order in which they will be executed, but they will be invoked
    /// synchronously; except for \a stop_callback(s) constructed after stop has
    /// already been requested for the \a hpx::stop_token, as described
    /// previously. If an invocation of a callback exits via an exception then
    /// hpx::terminate is called.
    /// \a hpx::stop_callback is not \a CopyConstructible, \a CopyAssignable, \a
    /// MoveConstructible, nor \a MoveAssignable. The template param Callback
    /// type must be both \a invocable and \a destructible. Any return value is
    /// ignored.
    HPX_CXX_CORE_EXPORT template <typename Callback>
    stop_callback(stop_token, Callback) -> stop_callback<Callback>;

    // 32.3.3.4 Specialized algorithms

    // Effects: Equivalent to: x.swap(y).
    HPX_CXX_CORE_EXPORT inline void swap(
        stop_token& lhs, stop_token& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    // 32.3.4.4 Specialized algorithms

    // Effects: Equivalent to: x.swap(y).
    HPX_CXX_CORE_EXPORT inline void swap(
        stop_source& lhs, stop_source& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}    // namespace hpx

////////////////////////////////////////////////////////////////////////////////
// Extensions to <stop_token> as proposed by P2300
namespace hpx::p2300_stop_token {

    // [stoptoken.inplace], class in_place_stop_token
    HPX_CXX_CORE_EXPORT class in_place_stop_token;

    // [stopsource.inplace], class in_place_stop_source
    HPX_CXX_CORE_EXPORT class in_place_stop_source;

    // [stopcallback.inplace], class template in_place_stop_callback
    HPX_CXX_CORE_EXPORT template <typename Callback>
    class [[nodiscard]] in_place_stop_callback;

    // [stoptoken.never], class never_stop_token
    //
    // The class never_stop_token provides an implementation of the
    // unstoppable_token concept. It provides a stop token interface,
    // but also provides static information that a stop is never
    // possible nor requested.
    //
    HPX_CXX_CORE_EXPORT struct never_stop_token
    {
    private:
        struct callback_impl
        {
            template <typename Callback>
            explicit constexpr callback_impl(
                never_stop_token, Callback&&) noexcept
            {
            }
        };

    public:
        template <typename>
        using callback_type = callback_impl;

        [[nodiscard]] static constexpr bool stop_requested() noexcept
        {
            return false;
        }

        [[nodiscard]] static constexpr bool stop_possible() noexcept
        {
            return false;
        }

        [[nodiscard]] friend constexpr bool operator==(
            never_stop_token, never_stop_token) noexcept
        {
            return true;
        }
        [[nodiscard]] friend constexpr bool operator!=(
            never_stop_token, never_stop_token) noexcept
        {
            return false;
        }
    };

    // [stopsource.inplace], class in_place_stop_source
    //
    // The class in_place_stop_source implements the semantics of making a
    // stop request, without the need for a dynamic allocation of a shared
    // state. A stop request made on a in_place_stop_source object is visible
    // to all associated in_place_stop_token ([stoptoken.inplace]) objects.
    // Once a stop request has been made it cannot be withdrawn (a subsequent
    // stop request has no effect). All uses of in_place_stop_token objects
    // associated with a given in_place_stop_source object must happen before
    // the invocation of the destructor of that in_place_stop_token object.
    //
    HPX_CXX_CORE_EXPORT class in_place_stop_source
    {
    public:
        in_place_stop_source() noexcept
        {
            state_.add_source_count();
        }
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
        ~in_place_stop_source()
        {
            state_.remove_source_count();
        }
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif

        in_place_stop_source(in_place_stop_source const&) = delete;
        in_place_stop_source(in_place_stop_source&&) noexcept = delete;

        in_place_stop_source& operator=(in_place_stop_source const&) = delete;
        in_place_stop_source& operator=(
            in_place_stop_source&&) noexcept = delete;

        [[nodiscard]] in_place_stop_token get_token() const noexcept;

        // Effects: Atomically determines whether the stop state inside *this
        // has received a stop request, and if not, makes a stop request. The
        // determination and making of the stop request are an atomic
        // read-modify-write operation ([intro.races]). If the request was made,
        // the callbacks registered by associated in_place_stop_callback objects
        // are synchronously called. If an invocation of a callback exits via an
        // exception then terminate is invoked ([except.terminate]).
        //
        // Postconditions: stop_possible() is false and stop_requested() is
        // true.
        //
        // Returns: true if this call made a stop request; otherwise false.
        //
        bool request_stop() noexcept
        {
            return state_.request_stop();
        }

        // Returns: true if the stop state inside *this has not yet received
        // a stop request; otherwise, false.
        [[nodiscard]] bool stop_requested() const noexcept
        {
            return state_.stop_requested();
        }

        // Returns: true if the stop state inside *this has not yet received
        // a stop request; otherwise, false.
        [[nodiscard]] bool stop_possible() const noexcept
        {
            return state_.stop_possible();
        }

    private:
        friend class in_place_stop_token;
        friend struct hpx::detail::stop_callback_base;

        template <typename>
        friend class in_place_stop_callback;

        bool register_callback(hpx::detail::stop_callback_base* cb) noexcept
        {
            return state_.add_callback(cb);
        }

        void remove_callback(hpx::detail::stop_callback_base* cb) noexcept
        {
            state_.remove_callback(cb);
        }

        hpx::detail::stop_state state_;
    };

    // [stoptoken.inplace], class in_place_stop_token
    //
    // The class in_place_stop_token provides an interface for querying
    // whether a stop request has been made (stop_requested) or can ever
    // be made (stop_possible) using an associated in_place_stop_source
    // object ([stopsource.inplace]). An in_place_stop_token can also be
    // passed to an in_place_stop_callback ([stopcallback.inplace])
    // constructor to register a callback to be called when a stop
    // request has been made from an associated in_place_stop_source.
    //
    HPX_CXX_CORE_EXPORT class in_place_stop_token
    {
    public:
        template <typename Callback>
        using callback_type = in_place_stop_callback<Callback>;

        // Effects: initializes source_ with nullptr.
        constexpr in_place_stop_token() noexcept
          : source_(nullptr)
        {
        }

        ~in_place_stop_token() = default;

        in_place_stop_token(in_place_stop_token const& rhs) noexcept = default;

        in_place_stop_token(in_place_stop_token&& rhs) noexcept
          : source_(std::exchange(rhs.source_, nullptr))
        {
        }

        in_place_stop_token& operator=(
            in_place_stop_token const& rhs) noexcept = default;

        in_place_stop_token& operator=(in_place_stop_token&& rhs) noexcept
        {
            source_ = std::exchange(rhs.source_, nullptr);
            return *this;
        }

        // Returns: source_ != nullptr && source_->stop_requested().
        //
        // Remarks: If source_ != nullptr, then any calls to this function
        // must strongly happen before the beginning of invocation of the
        // destructor of *source_.
        //
        [[nodiscard]] bool stop_requested() const noexcept
        {
            return source_ != nullptr && source_->stop_requested();
        }

        // Returns: source_ != nullptr && source_->stop_possible().
        //
        // Remarks: If source_ != nullptr, then any calls to this function
        // must strongly happen before the beginning of invocation of the
        // destructor of *source_.
        //
        [[nodiscard]] bool stop_possible() const noexcept
        {
            return source_ != nullptr && source_->stop_possible();
        }

        // Effects: Exchanges the values of source_ and rhs.source_.
        void swap(in_place_stop_token& rhs) noexcept
        {
            std::swap(source_, rhs.source_);
        }

        [[nodiscard]] friend constexpr bool operator==(
            in_place_stop_token const& lhs,
            in_place_stop_token const& rhs) noexcept
        {
            return lhs.source_ == rhs.source_;
        }
        [[nodiscard]] friend constexpr bool operator!=(
            in_place_stop_token const& lhs,
            in_place_stop_token const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        friend void swap(
            in_place_stop_token& x, in_place_stop_token& y) noexcept
        {
            x.swap(y);
        }

    private:
        friend class in_place_stop_source;

        template <typename>
        friend class in_place_stop_callback;

        // Effects: initializes source_ with source.
        explicit in_place_stop_token(
            in_place_stop_source const* source) noexcept
          : source_(source)
        {
        }

        in_place_stop_source const* source_;
    };

    inline in_place_stop_token in_place_stop_source::get_token() const noexcept
    {
        return in_place_stop_token{this};
    }

    // [stopcallback.inplace], class template in_place_stop_callback
    //
    // Mandates: in_place_stop_callback is instantiated with an argument for the
    // template parameter Callback that satisfies both invocable and
    // destructible.
    //
    // Preconditions: in_place_stop_callback is instantiated with an argument
    // for the template parameter Callback that models both invocable and
    // destructible.
    //
    // Recommended practice: Implementation should use the storage of the
    // in_place_stop_callback objects to store the state necessary for their
    // association with an in_place_stop_source object.
    //
    HPX_CXX_CORE_EXPORT template <typename Callback>
    class [[nodiscard]] in_place_stop_callback
      : private hpx::detail::stop_callback_base
    {
    public:
        using callback_type = Callback;

        // Constraints: Callback and CB satisfy constructible_from<Callback, CB>.
        //
        // Preconditions: Callback and CB model constructible_from<Callback, CB>.
        //
        // Effects: Initializes callback_ with std::forward<C>(cb). If
        // st.stop_requested() is true, then std::forward<Callback>(callback_)()
        // is evaluated in the current thread before the constructor returns.
        // Otherwise, if st has an associated in_place_stop_source object,
        // registers the callback with the stop state of the in_place_stop_source
        // that st is associated with such that std::forward<Callback>(callback_)()
        // is evaluated by the first call to request_stop() on an associated
        // in_place_stop_source. The in_place_stop_callback object being
        // initialized becomes associated with the in_place_stop_source object
        // that st is associated with, if any.
        //
        // Throws: Any exception thrown by the initialization of callback_.
        //
        // Remarks: If evaluating std::forward<Callback>(callback_)() exits via
        // an exception, then terminate is invoked ([except.terminate]).
        //
        template <typename CB,
            typename Enable =
                std::enable_if_t<std::is_constructible_v<Callback, CB>>>
        explicit in_place_stop_callback(in_place_stop_token const& st,
            CB&& cb) noexcept(std::is_nothrow_constructible_v<Callback, CB>)
          : callback_(HPX_FORWARD(CB, cb))
          , source_(const_cast<in_place_stop_source*>(st.source_))
        {
            if (source_ != nullptr)
            {
                source_->register_callback(this);
            }
        }

        template <typename CB,
            typename Enable =
                std::enable_if_t<std::is_constructible_v<Callback, CB>>>
        explicit in_place_stop_callback(in_place_stop_token&& st,
            CB&& cb) noexcept(std::is_nothrow_constructible_v<Callback, CB>)
          : callback_(HPX_FORWARD(CB, cb))
          , source_(const_cast<in_place_stop_source*>(st.source_))
        {
            if (source_ != nullptr)
            {
                st.source_ = nullptr;
                source_->register_callback(this);
            }
        }

        // Effects: Unregisters the callback from the stop state of the
        // associated in_place_stop_source object, if any. The destructor does
        // not block waiting for the execution of another callback registered by
        // an associated stop_callback. If callback_ is concurrently executing
        // on another thread, then the return from the invocation of callback_
        // strongly happens before ([intro.races]) callback_ is destroyed. If
        // callback_ is executing on the current thread, then the destructor
        // does not block ([defns.block]) waiting for the return from the
        // invocation of callback_.
        //
        // Remarks: A program has undefined behavior if the invocation of this
        // function does not strongly happen before the beginning of the
        // invocation of the destructor of the associated in_place_stop_source
        // object, if any.
        //
        ~in_place_stop_callback()
        {
            if (source_ != nullptr)
                source_->remove_callback(this);
        }

        in_place_stop_callback(in_place_stop_callback const&) = delete;
        in_place_stop_callback(in_place_stop_callback&&) noexcept = delete;

        in_place_stop_callback& operator=(
            in_place_stop_callback const&) = delete;
        in_place_stop_callback& operator=(
            in_place_stop_callback&&) noexcept = delete;

    private:
        void execute() noexcept override
        {
            HPX_FORWARD(Callback, callback_)();
        }

        HPX_NO_UNIQUE_ADDRESS Callback callback_;
        in_place_stop_source* source_;
    };

    HPX_CXX_CORE_EXPORT template <typename Callback>
    in_place_stop_callback(in_place_stop_token, Callback)
        -> in_place_stop_callback<Callback>;

}    // namespace hpx::p2300_stop_token

// For now, import all facilities as proposed by P2300 into hpx::experimental
namespace hpx::experimental {

    HPX_CXX_CORE_EXPORT using namespace hpx::p2300_stop_token;
}    // namespace hpx::experimental
