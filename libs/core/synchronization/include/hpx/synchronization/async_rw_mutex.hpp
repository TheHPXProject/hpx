//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/synchronization/mutex.hpp>

#include <exception>
#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>

namespace hpx::experimental {

    namespace detail {

        HPX_CXX_CORE_EXPORT enum class async_rw_mutex_access_type {
            read,
            readwrite
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        struct async_rw_mutex_shared_state
        {
            using shared_state_ptr_type =
                std::shared_ptr<async_rw_mutex_shared_state>;

            hpx::optional<T> value;
            shared_state_ptr_type next_state;
            hpx::mutex mtx;
            hpx::detail::small_vector<
                hpx::move_only_function<void(shared_state_ptr_type)>, 1>
                continuations;

            async_rw_mutex_shared_state() = default;

            async_rw_mutex_shared_state(async_rw_mutex_shared_state&&) = delete;
            async_rw_mutex_shared_state& operator=(
                async_rw_mutex_shared_state&&) = delete;

            async_rw_mutex_shared_state(
                async_rw_mutex_shared_state const&) = delete;
            async_rw_mutex_shared_state& operator=(
                async_rw_mutex_shared_state const&) = delete;

            ~async_rw_mutex_shared_state()
            {
                // The last state does not have a next state. If this state has
                // a next state it must always have continuations.
                HPX_ASSERT((continuations.empty() && !next_state) ||
                    (!continuations.empty() && next_state));

                // This state must always have the value set by the time it is
                // destructed. If there is no next state the value is destructed
                // with this state.
                HPX_ASSERT(value);

                if (HPX_LIKELY(next_state))
                {
                    // The current state has now finished all accesses to the
                    // wrapped value, so we move the value to the next state.
                    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                    next_state->set_value(HPX_MOVE(value.value()));

                    for (auto& continuation : continuations)
                    {
                        continuation(next_state);
                    }
                }
            }

            template <typename U>
            void set_value(U&& u)
            {
                HPX_ASSERT(!value);
                value.emplace(HPX_FORWARD(U, u));
            }

            void set_next_state(
                std::shared_ptr<async_rw_mutex_shared_state> state) noexcept
            {
                // The next state should only be set once
                HPX_ASSERT(!next_state);
                next_state = HPX_MOVE(state);
            }

            template <typename F>
            void add_continuation(F&& continuation)
            {
                std::lock_guard<hpx::mutex> l(mtx);
                continuations.emplace_back(HPX_FORWARD(F, continuation));
            }
        };

        template <>
        struct async_rw_mutex_shared_state<void>
        {
            using shared_state_ptr_type =
                std::shared_ptr<async_rw_mutex_shared_state>;

            shared_state_ptr_type next_state;
            hpx::mutex mtx;
            hpx::detail::small_vector<
                hpx::move_only_function<void(shared_state_ptr_type)>, 1>
                continuations;

            async_rw_mutex_shared_state() = default;

            async_rw_mutex_shared_state(async_rw_mutex_shared_state&&) = delete;
            async_rw_mutex_shared_state& operator=(
                async_rw_mutex_shared_state&&) = delete;

            async_rw_mutex_shared_state(
                async_rw_mutex_shared_state const&) = delete;
            async_rw_mutex_shared_state& operator=(
                async_rw_mutex_shared_state const&) = delete;

            ~async_rw_mutex_shared_state()
            {
                // The last state does not have a next state. If this state has
                // a next state it must always have continuations.
                HPX_ASSERT((continuations.empty() && !next_state) ||
                    (!continuations.empty() && next_state));

                for (auto& continuation : continuations)
                {
                    continuation(next_state);
                }
            }

            void set_next_state(
                std::shared_ptr<async_rw_mutex_shared_state> state) noexcept
            {
                // The next state should only be set once
                HPX_ASSERT(!next_state);
                next_state = HPX_MOVE(state);
            }

            template <typename F>
            void add_continuation(F&& continuation)
            {
                std::lock_guard<hpx::mutex> l(mtx);
                continuations.emplace_back(HPX_FORWARD(F, continuation));
            }
        };

        HPX_CXX_CORE_EXPORT template <typename ReadWriteT, typename ReadT,
            async_rw_mutex_access_type AccessType>
        struct async_rw_mutex_access_wrapper;

        HPX_CXX_CORE_EXPORT template <typename ReadWriteT, typename ReadT>
        struct async_rw_mutex_access_wrapper<ReadWriteT, ReadT,
            async_rw_mutex_access_type::read>
        {
        private:
            using shared_state_type =
                std::shared_ptr<async_rw_mutex_shared_state<ReadWriteT>>;
            shared_state_type state;

        public:
            async_rw_mutex_access_wrapper() = delete;
            explicit async_rw_mutex_access_wrapper(
                shared_state_type state) noexcept
              : state(HPX_MOVE(state))
            {
            }
            async_rw_mutex_access_wrapper(
                async_rw_mutex_access_wrapper&&) = default;
            async_rw_mutex_access_wrapper& operator=(
                async_rw_mutex_access_wrapper&&) = default;
            async_rw_mutex_access_wrapper(
                async_rw_mutex_access_wrapper const&) = default;
            async_rw_mutex_access_wrapper& operator=(
                async_rw_mutex_access_wrapper const&) = default;

            ReadT& get() const
            {
                HPX_ASSERT(state);
                HPX_ASSERT(state->value);
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                return state->value.value();
            }

            operator ReadT&() const
            {
                return get();
            }
        };

        HPX_CXX_CORE_EXPORT template <typename ReadWriteT, typename ReadT>
        struct async_rw_mutex_access_wrapper<ReadWriteT, ReadT,
            async_rw_mutex_access_type::readwrite>
        {
        private:
            static_assert(!std::is_void_v<ReadWriteT>,
                "Cannot mix void and non-void type in "
                "async_rw_mutex_access_wrapper wrapper (ReadWriteT is void, "
                "ReadT is non-void)");
            static_assert(!std::is_void_v<ReadT>,
                "Cannot mix void and non-void type in "
                "async_rw_mutex_access_wrapper wrapper (ReadT is void, "
                "ReadWriteT is non-void)");

            using shared_state_type =
                std::shared_ptr<async_rw_mutex_shared_state<ReadWriteT>>;
            shared_state_type state;

        public:
            async_rw_mutex_access_wrapper() = delete;
            explicit async_rw_mutex_access_wrapper(
                shared_state_type state) noexcept
              : state(HPX_MOVE(state))
            {
            }
            async_rw_mutex_access_wrapper(
                async_rw_mutex_access_wrapper&&) = default;
            async_rw_mutex_access_wrapper& operator=(
                async_rw_mutex_access_wrapper&&) = default;
            async_rw_mutex_access_wrapper(
                async_rw_mutex_access_wrapper const&) = delete;
            async_rw_mutex_access_wrapper& operator=(
                async_rw_mutex_access_wrapper const&) = delete;

            ReadWriteT& get()
            {
                HPX_ASSERT(state);
                HPX_ASSERT(state->value);
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                return state->value.value();
            }

            operator ReadWriteT&()
            {
                return get();
            }
        };

        // The void wrappers for read and readwrite are identical, but must be
        // specialized separately to avoid ambiguity with the non-void
        // specializations above.
        template <>
        struct async_rw_mutex_access_wrapper<void, void,
            async_rw_mutex_access_type::read>
        {
        private:
            using shared_state_type =
                std::shared_ptr<async_rw_mutex_shared_state<void>>;
            shared_state_type state;

        public:
            async_rw_mutex_access_wrapper() = delete;
            explicit async_rw_mutex_access_wrapper(
                shared_state_type state) noexcept
              : state(HPX_MOVE(state))
            {
            }
            async_rw_mutex_access_wrapper(
                async_rw_mutex_access_wrapper&&) = default;
            async_rw_mutex_access_wrapper& operator=(
                async_rw_mutex_access_wrapper&&) = default;
            async_rw_mutex_access_wrapper(
                async_rw_mutex_access_wrapper const&) = default;
            async_rw_mutex_access_wrapper& operator=(
                async_rw_mutex_access_wrapper const&) = default;
        };

        template <>
        struct async_rw_mutex_access_wrapper<void, void,
            async_rw_mutex_access_type::readwrite>
        {
        private:
            using shared_state_type =
                std::shared_ptr<async_rw_mutex_shared_state<void>>;
            shared_state_type state;

        public:
            async_rw_mutex_access_wrapper() = delete;
            explicit async_rw_mutex_access_wrapper(
                shared_state_type state) noexcept
              : state(HPX_MOVE(state))
            {
            }
            async_rw_mutex_access_wrapper(
                async_rw_mutex_access_wrapper&&) = default;
            async_rw_mutex_access_wrapper& operator=(
                async_rw_mutex_access_wrapper&&) = default;
            async_rw_mutex_access_wrapper(
                async_rw_mutex_access_wrapper const&) = delete;
            async_rw_mutex_access_wrapper& operator=(
                async_rw_mutex_access_wrapper const&) = delete;
        };
    }    // namespace detail

    /// Read-write mutex where access is granted to a value through senders.
    ///
    /// The wrapped value is accessed through read and readwrite, both of which
    /// return senders which call set_value on a connected receiver when the
    /// wrapped value is safe to read or write. The senders send the value
    /// through a wrapper type which is implicitly convertible to a reference of
    /// the wrapped value. Read-only senders send wrappers that are convertible
    /// to const references.
    ///
    /// A read-write sender gives exclusive access to the wrapped value, while a
    /// read-only sender gives shared (with other read-only senders) access to
    /// the value.
    ///
    /// A void mutex acts as a mutex around some user-managed resource, i.e. the
    /// void mutex does not manage any value and the types sent by the senders
    /// are not convertible. The types sent are copyable and release access to
    /// the protected resource when released.
    ///
    /// The order in which senders call set_value is determined by the order in
    /// which the senders are retrieved from the mutex. Connecting and starting
    /// the senders is thread-safe.
    ///
    /// Retrieving senders from the mutex is not thread-safe.
    ///
    /// The mutex is movable and non-copyable.
    HPX_CXX_CORE_EXPORT template <typename ReadWriteT = void,
        typename ReadT = ReadWriteT,
        typename Allocator = hpx::util::internal_allocator<>>
    class async_rw_mutex;

    // Implementation details:
    //
    // The async_rw_mutex protects access to a given resource using two
    // reference counted shared states, the current and the previous state. Each
    // shared state guards access to the next stage; when the shared state goes
    // out of scope it triggers continuations for the next stage.
    //
    // When read-write access is required a sender is created which holds on to
    // the newly created shared state for the read-write access and the previous
    // state. When the sender is connected to a receiver, a callback is added to
    // the previous shared state's destructor. The callback holds the new state,
    // and passes a wrapper holding the shared state to set_value. Once the
    // receiver which receives the wrapper has let the wrapper go out of scope
    // (and all other references to the shared state are out of scope), the new
    // shared state will again trigger its continuations.
    //
    // When read-only access is required and the previous access was read-only
    // the procedure is the same as for read-write access. When read-only access
    // follows a previous read-only access the shared state is reused between
    // all consecutive read-only accesses, such that multiple read-only accesses
    // can run concurrently, and the next access (which must be read-write) is
    // triggered once all instances of that shared state have gone out of scope.
    //
    // The protected value is moved from state to state and is released when the
    // last shared state is destroyed.

    HPX_CXX_CORE_EXPORT template <typename Allocator>
    class async_rw_mutex<void, void, Allocator>
    {
    private:
        template <detail::async_rw_mutex_access_type AccessType>
        struct sender;

        using shared_state_type = detail::async_rw_mutex_shared_state<void>;
        using shared_state_ptr_type = std::shared_ptr<shared_state_type>;

    public:
        using read_type = void;
        using readwrite_type = void;

        using read_access_type =
            detail::async_rw_mutex_access_wrapper<readwrite_type, read_type,
                detail::async_rw_mutex_access_type::read>;
        using readwrite_access_type =
            detail::async_rw_mutex_access_wrapper<readwrite_type, read_type,
                detail::async_rw_mutex_access_type::readwrite>;

        using allocator_type = Allocator;

        explicit async_rw_mutex(allocator_type const& alloc = {})
          : alloc(alloc)
        {
        }
        async_rw_mutex(async_rw_mutex&&) = default;
        async_rw_mutex& operator=(async_rw_mutex&&) = default;
        async_rw_mutex(async_rw_mutex const&) = delete;
        async_rw_mutex& operator=(async_rw_mutex const&) = delete;

        sender<detail::async_rw_mutex_access_type::read> read()
        {
            if (prev_access == detail::async_rw_mutex_access_type::readwrite)
            {
                prev_state = HPX_MOVE(state);
                state = std::allocate_shared<shared_state_type, allocator_type>(
                    alloc);
                prev_access = detail::async_rw_mutex_access_type::read;

                // Only the first access has no previous shared state. When
                // there is a previous state we set the next state so that the
                // value can be passed from the previous state to the next
                // state.
                if (HPX_LIKELY(prev_state))
                {
                    prev_state->set_next_state(state);
                }
            }
            return {prev_state, state};
        }

        sender<detail::async_rw_mutex_access_type::readwrite> readwrite()
        {
            prev_state = HPX_MOVE(state);
            state =
                std::allocate_shared<shared_state_type, allocator_type>(alloc);
            prev_access = detail::async_rw_mutex_access_type::readwrite;

            // Only the first access has no previous shared state. When there is
            // a previous state we set the next state so that the value can be
            // passed from the previous state to the next state.
            if (HPX_LIKELY(prev_state))
            {
                prev_state->set_next_state(state);
            }
            return {HPX_MOVE(prev_state), state};
        }

    private:
        template <detail::async_rw_mutex_access_type AccessType>
        struct sender
        {
            shared_state_ptr_type prev_state;
            shared_state_ptr_type state;

            using access_type =
                detail::async_rw_mutex_access_wrapper<readwrite_type, read_type,
                    AccessType>;
            using sender_concept = hpx::execution::experimental::sender_t;

            template <typename Env>
            friend auto tag_invoke(
                hpx::execution::experimental::get_completion_signatures_t,
                sender const&, Env const&)
                -> hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(access_type),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>;

            template <typename R>
            struct operation_state
            {
                std::decay_t<R> r;
                shared_state_ptr_type prev_state;
                shared_state_ptr_type state;

                template <typename R_>
                operation_state(R_&& r, shared_state_ptr_type prev_state,
                    shared_state_ptr_type state)
                  : r(HPX_FORWARD(R_, r))
                  , prev_state(HPX_MOVE(prev_state))
                  , state(HPX_MOVE(state))
                {
                }

                operation_state(operation_state&&) = delete;
                operation_state& operator=(operation_state&&) = delete;
                operation_state(operation_state const&) = delete;
                operation_state& operator=(operation_state const&) = delete;

                friend void tag_invoke(hpx::execution::experimental::start_t,
                    operation_state& os) noexcept
                {
                    HPX_ASSERT_MSG(os.state,
                        "async_rw_lock::sender::operation_state state is "
                        "empty, was the sender already started?");

                    auto continuation =
                        [r = HPX_MOVE(os.r)](
                            shared_state_ptr_type state) mutable {
                            try
                            {
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(r), access_type{HPX_MOVE(state)});
                            }
                            catch (...)
                            {
                                hpx::execution::experimental::set_error(
                                    HPX_MOVE(r), std::current_exception());
                            }
                        };

                    if (os.prev_state)
                    {
                        os.prev_state->add_continuation(HPX_MOVE(continuation));

                        // We release prev_state here to allow continuations to
                        // run. The operation state may otherwise keep it alive
                        // longer than needed.
                        os.prev_state.reset();
                    }
                    else
                    {
                        // There is no previous state on the first access. We
                        // can immediately trigger the continuation.
                        continuation(HPX_MOVE(os.state));
                    }
                }
            };

            template <typename R>
            friend auto tag_invoke(
                hpx::execution::experimental::connect_t, sender&& s, R&& r)
            {
                return operation_state<R>{HPX_FORWARD(R, r),
                    HPX_MOVE(s.prev_state), HPX_MOVE(s.state)};
            }
        };

        allocator_type alloc;

        detail::async_rw_mutex_access_type prev_access =
            detail::async_rw_mutex_access_type::readwrite;

        shared_state_ptr_type prev_state;
        shared_state_ptr_type state;
    };

    HPX_CXX_CORE_EXPORT template <typename ReadWriteT, typename ReadT,
        typename Allocator>
    class async_rw_mutex
    {
    private:
        static_assert(!std::is_void_v<ReadWriteT>,
            "Cannot mix void and non-void type in async_rw_mutex (ReadWriteT "
            "is void, ReadT is non-void)");
        static_assert(!std::is_void_v<ReadT>,
            "Cannot mix void and non-void type in async_rw_mutex (ReadT is "
            "void, ReadWriteT is non-void)");

        template <detail::async_rw_mutex_access_type AccessType>
        struct sender;

    public:
        using read_type = std::decay_t<ReadT> const;
        using readwrite_type = std::decay_t<ReadWriteT>;
        using value_type = readwrite_type;

        using read_access_type =
            detail::async_rw_mutex_access_wrapper<readwrite_type, read_type,
                detail::async_rw_mutex_access_type::read>;
        using readwrite_access_type =
            detail::async_rw_mutex_access_wrapper<readwrite_type, read_type,
                detail::async_rw_mutex_access_type::readwrite>;

        using allocator_type = Allocator;

        async_rw_mutex() = delete;
        template <typename U,
            typename = std::enable_if_t<
                !std::is_same_v<std::decay_t<U>, async_rw_mutex>>>
        explicit async_rw_mutex(U&& u, allocator_type const& alloc = {})
          : value(HPX_FORWARD(U, u))
          , alloc(alloc)
        {
        }
        async_rw_mutex(async_rw_mutex&&) = default;
        async_rw_mutex& operator=(async_rw_mutex&&) = default;
        async_rw_mutex(async_rw_mutex const&) = delete;
        async_rw_mutex& operator=(async_rw_mutex const&) = delete;

        sender<detail::async_rw_mutex_access_type::read> read()
        {
            if (prev_access == detail::async_rw_mutex_access_type::readwrite)
            {
                prev_state = HPX_MOVE(state);
                state = std::allocate_shared<shared_state_type, allocator_type>(
                    alloc);
                prev_access = detail::async_rw_mutex_access_type::read;

                // Only the first access has no previous shared state. When
                // there is a previous state we set the next state so that the
                // value can be passed from the previous state to the next
                // state. When there is no previous state we need to move the
                // value to the first state.
                if (HPX_LIKELY(prev_state))
                {
                    prev_state->set_next_state(state);
                }
                else
                {
                    state->set_value(HPX_MOVE(value));
                }
            }
            return {prev_state, state};
        }

        sender<detail::async_rw_mutex_access_type::readwrite> readwrite()
        {
            prev_state = HPX_MOVE(state);
            state =
                std::allocate_shared<shared_state_type, allocator_type>(alloc);

            // Only the first access has no previous shared state. When there is
            // a previous state we set the next state so that the value can be
            // passed from the previous state to the next state. When there is
            // no previous state we need to move the value to the first state.
            if (HPX_LIKELY(prev_state))    //-V1051
            {
                prev_state->set_next_state(state);
            }
            else
            {
                state->set_value(HPX_MOVE(value));
            }
            prev_access = detail::async_rw_mutex_access_type::readwrite;
            return {HPX_MOVE(prev_state), state};
        }

    private:
        using shared_state_type =
            detail::async_rw_mutex_shared_state<value_type>;
        using shared_state_ptr_type = std::shared_ptr<shared_state_type>;

        template <detail::async_rw_mutex_access_type AccessType>
        struct sender
        {
            shared_state_ptr_type prev_state;
            shared_state_ptr_type state;

            using access_type =
                detail::async_rw_mutex_access_wrapper<readwrite_type, read_type,
                    AccessType>;

            using sender_concept = hpx::execution::experimental::sender_t;

            template <typename Env>
            friend auto tag_invoke(
                hpx::execution::experimental::get_completion_signatures_t,
                sender const&, Env)
                -> hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(access_type),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr)>;

            template <typename R>
            struct operation_state
            {
                std::decay_t<R> r;
                shared_state_ptr_type prev_state;
                shared_state_ptr_type state;

                template <typename R_>
                operation_state(R_&& r, shared_state_ptr_type prev_state,
                    shared_state_ptr_type state)
                  : r(HPX_FORWARD(R_, r))
                  , prev_state(HPX_MOVE(prev_state))
                  , state(HPX_MOVE(state))
                {
                }

                operation_state(operation_state&&) = delete;
                operation_state& operator=(operation_state&&) = delete;
                operation_state(operation_state const&) = delete;
                operation_state& operator=(operation_state const&) = delete;

                friend void tag_invoke(hpx::execution::experimental::start_t,
                    operation_state& os) noexcept
                {
                    HPX_ASSERT_MSG(os.state,
                        "async_rw_lock::sender::operation_state state is "
                        "empty, was the sender already started?");

                    auto continuation =
                        [r = HPX_MOVE(os.r)](
                            shared_state_ptr_type state) mutable {
                            try
                            {
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(r), access_type{HPX_MOVE(state)});
                            }
                            catch (...)
                            {
                                hpx::execution::experimental::set_error(
                                    HPX_MOVE(r), std::current_exception());
                            }
                        };

                    if (os.prev_state)
                    {
                        os.prev_state->add_continuation(HPX_MOVE(continuation));
                        // We release prev_state here to allow continuations to
                        // run. The operation state may otherwise keep it alive
                        // longer than needed.
                        os.prev_state.reset();
                    }
                    else
                    {
                        // There is no previous state on the first access. We
                        // can immediately trigger the continuation.
                        continuation(HPX_MOVE(os.state));
                    }
                }
            };

            template <typename R>
            friend auto tag_invoke(
                hpx::execution::experimental::connect_t, sender&& s, R&& r)
            {
                return operation_state<R>{HPX_FORWARD(R, r),
                    HPX_MOVE(s.prev_state), HPX_MOVE(s.state)};
            }
        };

        value_type value;
        allocator_type alloc;

        detail::async_rw_mutex_access_type prev_access =
            detail::async_rw_mutex_access_type::readwrite;

        shared_state_ptr_type prev_state;
        shared_state_ptr_type state;
    };
}    // namespace hpx::experimental
