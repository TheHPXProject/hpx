//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/async_base/query_dispatch.hpp>
#include <hpx/execution/algorithms/detail/inject_scheduler.hpp>
#include <hpx/execution/algorithms/detail/partial_algorithm.hpp>
#include <hpx/execution/algorithms/detail/single_result.hpp>
#include <hpx/execution/algorithms/run_loop.hpp>
#include <hpx/execution_base/stdexec_forward.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    // enforce proper formatting
    namespace detail {

        // Recover the parent `run_loop&` from a `run_loop::scheduler`.
        //
        // P2300 deliberately does not provide a public API to obtain the owning
        // `run_loop&` from one of its schedulers. The only way to do this
        // against the current stdexec implementation is to read the private
        // `loop` member exposed by the scheduler's environment.
        //
        // The parameter is constrained to the concrete `run_loop::scheduler`
        // type (rather than a generic template) because the implementation
        // depends on `.loop` being the specific stdexec env layout.
        // `run_loop::scheduler::schedule()` is `noexcept`, so the unconditional
        // `noexcept` here is sound. The function is not marked `constexpr`
        // because the `stdexec::schedule` CPO wrapper is not declared
        // `constexpr` (GCC strict mode rejects calling it from a constexpr
        // context, even though the underlying member is constexpr).
        inline hpx::execution::experimental::run_loop&
        get_run_loop_from_scheduler(
            decltype(std::declval<hpx::execution::experimental::run_loop>()
                    .get_scheduler()) const& sched) noexcept
        {
            return static_cast<hpx::execution::experimental::run_loop&>(
                *hpx::execution::experimental::get_env(schedule(sched)).loop);
        }

        template <typename OperationState>
        void start_operation_state(OperationState& op_state) noexcept
        {
            if constexpr (requires { op_state.start(); })
            {
                op_state.start();
            }
            else
            {
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                hpx::execution::experimental::start(op_state);
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif
            }
        }

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
        HPX_CXX_CORE_EXPORT template <typename Data>
        struct future_receiver_base
        {
            using receiver_concept = hpx::execution::experimental::receiver_t;
            hpx::intrusive_ptr<Data> data;

        protected:
            // Note: MSVC toolset v142 fails compiling 'data->' below, using
            // '(*data).' instead
            template <typename U>
            void set_value_impl(U&& u) && noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() { (*data).set_value(HPX_FORWARD(U, u)); },
                    [&](std::exception_ptr ep) {
                        (*data).set_exception(HPX_MOVE(ep));
                    });
                data.reset();
            }

        public:
            void set_error(std::exception_ptr ep) && noexcept
            {
                data->set_exception(HPX_MOVE(ep));
                data.reset();
            }

            static void set_stopped() noexcept
            {
                std::terminate();
            }
        };
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif

        HPX_CXX_CORE_EXPORT template <typename T>
        struct future_receiver
          : future_receiver_base<hpx::lcos::detail::future_data_base<T>>
        {
            template <typename U>
            void set_value(U&& u) && noexcept
            {
                HPX_MOVE(*this).set_value_impl(HPX_FORWARD(U, u));
            }
        };

        template <>
        struct future_receiver<void>
          : future_receiver_base<hpx::lcos::detail::future_data_base<void>>
        {
            void set_value() && noexcept
            {
                HPX_MOVE(*this).set_value_impl(hpx::util::unused);
            }
        };

#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
        HPX_CXX_CORE_EXPORT template <typename T, typename Allocator,
            typename OperationState, typename Derived = void>
        struct future_data
          : hpx::lcos::detail::future_data_allocator<T, Allocator,
                std::conditional_t<std::is_void_v<Derived>,
                    future_data<T, Allocator, OperationState, Derived>,
                    Derived>>
        {
            future_data& operator=(future_data const&) = delete;
            future_data& operator=(future_data&&) = delete;

            using derived_type = std::conditional_t<std::is_void_v<Derived>,
                future_data, Derived>;
            using base_type = hpx::lcos::detail::future_data_allocator<T,
                Allocator, derived_type>;
            using operation_state_type = std::decay_t<OperationState>;
            using init_no_addref = base_type::init_no_addref;
            using other_allocator = std::allocator_traits<
                Allocator>::template rebind_alloc<future_data>;

            operation_state_type op_state;

            // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
            future_data(future_data const&) = delete;
            future_data(future_data&&) = delete;

            template <typename Sender>
            future_data(init_no_addref no_addref, other_allocator const& alloc,
                Sender&& sender)
              : base_type(no_addref, alloc)
              , op_state(hpx::execution::experimental::connect(
                    HPX_FORWARD(Sender, sender),
                    detail::future_receiver<T>{{this}}))
            {
                detail::start_operation_state(op_state);
            }
            // NOLINTEND(bugprone-crtp-constructor-accessibility)
        };

        HPX_CXX_CORE_EXPORT template <typename T, typename Allocator,
            typename OperationState>
        struct future_data_with_run_loop
          : future_data<T, Allocator, OperationState,
                future_data_with_run_loop<T, Allocator, OperationState>>
        {
            hpx::execution::experimental::run_loop& loop;

            using base_type = future_data<T, Allocator, OperationState,
                future_data_with_run_loop>;
            using init_no_addref = base_type::init_no_addref;
            using other_allocator = base_type::other_allocator;

            template <typename Sender>
            future_data_with_run_loop(init_no_addref no_addref,
                other_allocator const& alloc,
                decltype(std::declval<hpx::execution::experimental::run_loop>()
                        .get_scheduler()) const& sched,
                Sender&& sender)
              : base_type(no_addref, alloc, HPX_FORWARD(Sender, sender))
              , loop(get_run_loop_from_scheduler(sched))
            {
                this->set_on_completed([this]() { loop.finish(); });
            }

            hpx::util::unused_type* get_result_void(
                error_code& ec = throws) override
            {
                execute_deferred(ec);
                return this->base_type::get_result_void(ec);
            }

            void execute_deferred(error_code& = throws) override
            {
                loop.run();
            }
        };

        // Detects whether a sender's set_value completion scheduler is
        // run_loop_scheduler. Used by detail::make_future as a runtime
        // guard (see HPX_ASSERT_MSG below). The trait itself is SFINAE-
        // friendly so it can be safely instantiated during overload
        // resolution.
        template <typename Sender, typename = void>
        struct sender_completion_is_run_loop_scheduler : std::false_type
        {
        };

        template <typename Sender>
        struct sender_completion_is_run_loop_scheduler<Sender,
            std::void_t<
                decltype(hpx::execution::experimental::get_completion_scheduler<
                    hpx::execution::experimental::set_value_t>(hpx::execution::
                        experimental::get_env(std::declval<Sender>())))>>
          : std::is_same<std::decay_t<decltype(hpx::execution::experimental::
                                 get_completion_scheduler<
                                     hpx::execution::experimental::set_value_t>(
                                     hpx::execution::experimental::get_env(
                                         std::declval<Sender>())))>,
                hpx::execution::experimental::run_loop_scheduler>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Sender, typename Allocator>
        auto make_future(Sender&& sender, Allocator const& allocator)
        {
            // Debug-only runtime guard against the silent-hang case:
            // this 1-argument fallback constructs a future_data that
            // never drives loop.run(), so a sender whose set_value
            // completion scheduler is a run_loop_scheduler produces a
            // future that hangs in get(). The public make_future CPO
            // routes such senders to the scheduler-form overload via
            // tag_override_invoke, so reaching this body with a run-loop
            // completion scheduler indicates a direct call to
            // detail::make_future rather than the public API. HPX_ASSERT_MSG
            // is a no-op in release builds (HPX_DEBUG off), so this only
            // catches the misuse in debug; an unconditional throw would
            // also alter the release-mode failure shape and is left out
            // intentionally -- the assert exists to flag bypassing the
            // public CPO during development, not to harden release-mode
            // misuse. static_assert is avoided here because tag_priority's
            // SFINAE probing during overload resolution may instantiate
            // this body before the scheduler-form overload is selected.
            HPX_ASSERT_MSG(!sender_completion_is_run_loop_scheduler<
                               std::decay_t<Sender>>::value,
                "make_future(sender) cannot drive the parent run_loop when "
                "the sender's completion scheduler is a run_loop_scheduler. "
                "Use make_future(loop.get_scheduler(), sender) so the "
                "resulting future can drive the loop in its get().");

            using allocator_type = Allocator;

            using value_types = hpx::execution::experimental::value_types_of_t<
                std::decay_t<Sender>, hpx::execution::experimental::empty_env,
                meta::pack, meta::pack>;

            using result_type =
                std::decay_t<detail::single_result_t<value_types>>;
            using operation_state_type = hpx::util::invoke_result_t<
                hpx::execution::experimental::connect_t, Sender,
                detail::future_receiver<result_type>>;

            using shared_state =
                future_data<result_type, allocator_type, operation_state_type>;
            using init_no_addref = shared_state::init_no_addref;
            using other_allocator = std::allocator_traits<
                allocator_type>::template rebind_alloc<shared_state>;
            using allocator_traits = std::allocator_traits<other_allocator>;
            using unique_ptr = std::unique_ptr<shared_state,
                util::allocator_deleter<other_allocator>>;

            other_allocator alloc(allocator);
            unique_ptr p(allocator_traits::allocate(alloc, 1),
                hpx::util::allocator_deleter<other_allocator>{alloc});

            allocator_traits::construct(alloc, p.get(), init_no_addref{}, alloc,
                HPX_FORWARD(Sender, sender));

            return hpx::traits::future_access<future<result_type>>::create(
                p.release(), false);
        }
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Sender, typename Allocator>
        auto make_future_with_run_loop(
            decltype(std::declval<hpx::execution::experimental::run_loop>()
                    .get_scheduler()) const& sched,
            Sender&& sender, Allocator const& allocator)
        {
            using allocator_type = Allocator;

            using value_types = hpx::execution::experimental::value_types_of_t<
                std::decay_t<Sender>, hpx::execution::experimental::empty_env,
                meta::pack, meta::pack>;

            using result_type =
                std::decay_t<detail::single_result_t<value_types>>;
            using operation_state_type = hpx::util::invoke_result_t<
                hpx::execution::experimental::connect_t, Sender,
                detail::future_receiver<result_type>>;

            using shared_state = future_data_with_run_loop<result_type,
                allocator_type, operation_state_type>;
            using init_no_addref = shared_state::init_no_addref;
            using other_allocator = std::allocator_traits<
                allocator_type>::template rebind_alloc<shared_state>;
            using allocator_traits = std::allocator_traits<other_allocator>;
            using unique_ptr = std::unique_ptr<shared_state,
                util::allocator_deleter<other_allocator>>;

            other_allocator alloc(allocator);
            unique_ptr p(allocator_traits::allocate(alloc, 1),
                hpx::util::allocator_deleter<other_allocator>{alloc});

            allocator_traits::construct(alloc, p.get(), init_no_addref{}, alloc,
                sched, HPX_FORWARD(Sender, sender));

            return hpx::traits::future_access<future<result_type>>::create(
                p.release(), false);
        }
    }    // namespace detail
}    // namespace hpx::execution::experimental

namespace hpx::traits::detail {

    template <typename T, typename Allocator, typename OperationState,
        typename NewAllocator>
    struct shared_state_allocator<hpx::execution::experimental::detail::
                                      future_data<T, Allocator, OperationState>,
        NewAllocator>
    {
        using type = hpx::execution::experimental::detail::future_data<T,
            NewAllocator, OperationState>;
    };

    template <typename T, typename Allocator, typename OperationState,
        typename NewAllocator>
    struct shared_state_allocator<
        hpx::execution::experimental::detail::future_data_with_run_loop<T,
            Allocator, OperationState>,
        NewAllocator>
    {
        using type =
            hpx::execution::experimental::detail::future_data_with_run_loop<T,
                NewAllocator, OperationState>;
    };
}    // namespace hpx::traits::detail

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // execution::make_future is a sender consumer that submits the work
    // described by the provided sender for execution, similarly to
    // ensure_started, except that it returns a future that provides an optional
    // tuple of values that were sent by the provided sender on its completion
    // of work.
    //
    // Where 4.20.1 execution::schedule and 4.20.3 execution::transfer_just are
    // meant to enter the domain of senders, make_future is meant to exit the
    // domain of senders, retrieving the result of the task graph.
    //
    // If the provided sender sends an error instead of values, make_future
    // stores that error as an exception in the future, or the original
    // exception if the error is of type std::exception_ptr.
    //
    // If the provided sender sends the "stopped" signal instead of values,
    // make_future calls std::terminate.
    //
    HPX_CXX_CORE_EXPORT inline constexpr struct make_future_t final
      : hpx::functional::detail::tag_priority<make_future_t>
    {
    private:
        template <typename Scheduler, typename Sender,
            typename Allocator = hpx::util::internal_allocator<>>
        friend constexpr auto tag_invoke(make_future_t tag, Scheduler&& sched,
            Sender&& sender, Allocator const& allocator = Allocator{})
            requires(
                has_query_v<Scheduler, make_future_t, Sender, Allocator const&>)
        {
            return HPX_FORWARD(Scheduler, sched)
                .query(tag, HPX_FORWARD(Sender, sender), allocator);
        }

        // clang-format off
        template <typename Sender,
            typename Allocator = hpx::util::internal_allocator<>,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender> &&
                hpx::traits::is_allocator_v<Allocator> &&
                experimental::detail::is_completion_scheduler_tag_invocable_v<
                    hpx::execution::experimental::set_value_t,
                    Sender, make_future_t, Allocator
                >
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_override_invoke(make_future_t,
            Sender&& sender, Allocator const& allocator = Allocator{})
        {
            auto scheduler =
                hpx::execution::experimental::get_completion_scheduler<
                    hpx::execution::experimental::set_value_t>(
                    hpx::execution::experimental::get_env(sender));

            return hpx::functional::tag_invoke(make_future_t{},
                HPX_MOVE(scheduler), HPX_FORWARD(Sender, sender), allocator);
        }

        // clang-format off
        template <typename Sender,
            typename Allocator = hpx::util::internal_allocator<>,
            HPX_CONCEPT_REQUIRES_(
                is_sender_v<Sender> &&
                hpx::traits::is_allocator_v<Allocator>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(make_future_t,
            Sender&& sender, Allocator const& allocator = Allocator{})
        {
            return detail::make_future(HPX_FORWARD(Sender, sender), allocator);
        }

        // clang-format off
        template <typename Scheduler,
            typename Allocator = hpx::util::internal_allocator<>,
            HPX_CONCEPT_REQUIRES_(
                hpx::execution::experimental::is_scheduler_v<Scheduler> &&
                hpx::traits::is_allocator_v<Allocator>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(make_future_t,
            Scheduler&& scheduler, Allocator const& allocator = Allocator{})
        {
            return hpx::execution::experimental::detail::inject_scheduler<
                make_future_t, Scheduler, Allocator>{
                HPX_FORWARD(Scheduler, scheduler), allocator};
        }

        // clang-format off
        template <typename Allocator = hpx::util::internal_allocator<>,
            HPX_CONCEPT_REQUIRES_(
                hpx::traits::is_allocator_v<Allocator>
            )>
        // clang-format on
        friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
            make_future_t, Allocator const& allocator = Allocator{})
        {
            return detail::partial_algorithm<make_future_t, Allocator>{
                allocator};
        }
    } make_future{};

    template <typename Sender, typename Allocator>
    auto run_loop::run_loop_scheduler::query(
        make_future_t, Sender&& sender, Allocator const& allocator) const
    {
        return detail::make_future_with_run_loop(
            *this, HPX_FORWARD(Sender, sender), allocator);
    }
}    // namespace hpx::execution::experimental
