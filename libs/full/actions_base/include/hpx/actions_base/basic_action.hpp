//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c)      2011 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/actions_base/basic_action.hpp
/// \page HPX_REGISTER_ACTION_DECLARATION, HPX_REGISTER_ACTION
/// \headerfile hpx/components.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/actions_base_fwd.hpp>
#include <hpx/actions_base/actions_base_support.hpp>
#include <hpx/actions_base/basic_action_fwd.hpp>
#include <hpx/actions_base/detail/action_factory.hpp>
#include <hpx/actions_base/detail/invocation_count_registry.hpp>
#include <hpx/actions_base/detail/per_action_data_counter_registry.hpp>
#include <hpx/actions_base/macros.hpp>
#include <hpx/actions_base/preassigned_action_id.hpp>
#include <hpx/actions_base/traits/action_continuation.hpp>
#include <hpx/actions_base/traits/action_priority.hpp>
#include <hpx/actions_base/traits/action_remote_result.hpp>
#include <hpx/actions_base/traits/action_stacksize.hpp>
#include <hpx/actions_base/traits/action_trigger_continuation_fwd.hpp>
#include <hpx/actions_base/traits/is_distribution_policy.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::actions {

    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename Action>
        struct action_invoke
        {
            naming::address_type lva;
            naming::component_type comptype;

            template <typename... Ts>
            HPX_FORCEINLINE Action::internal_result_type operator()(
                Ts&&... vs) const
            {
                return Action::invoke(lva, comptype, HPX_FORWARD(Ts, vs)...);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        /// The \a thread_function will be registered as the thread
        /// function of a thread. It encapsulates the execution of the
        /// original function (given by \a func).
        template <typename Action>
        class thread_function
        {
        public:
            template <typename... Ts>
            explicit thread_function(hpx::id_type&& target,
                naming::address_type lva, naming::component_type comptype,
                Ts&&... vs)
              : target_(HPX_MOVE(target))
              , lva_(lva)
              , comptype_(comptype)
              , args_(HPX_FORWARD(Ts, vs)...)
            {
            }

            threads::thread_result_type operator()(
                threads::thread_restart_state)
            {
                try
                {
                    LTM_(debug).format(
                        "Executing {}.", Action::get_action_name(lva_));

                    // invoke the action, ignoring the return value
                    hpx::invoke_fused(action_invoke<Action>{lva_, comptype_},
                        HPX_MOVE(args_));
                }
                // NOLINTNEXTLINE(bugprone-empty-catch)
                catch (hpx::thread_interrupted const&)
                {    //-V565
                     /* swallow this exception */
                }
                catch (std::exception const& e)
                {
                    LTM_(error).format(
                        "Unhandled exception while executing {}: {}",
                        Action::get_action_name(lva_), e.what());

                    // report this error to the console in any case
                    hpx::report_error(std::current_exception());
                }
                catch (...)
                {
                    LTM_(error).format("Unhandled exception while executing {}",
                        Action::get_action_name(lva_));

                    // report this error to the console in any case
                    hpx::report_error(std::current_exception());
                }

                // Verify that there are no more registered locks for this
                // OS-thread. This will throw if there are still any locks
                // held.
                util::force_error_on_lock();

                return threads::thread_result_type(
                    threads::thread_schedule_state::terminated,
                    threads::invalid_thread_id);
            }

        private:
            // This holds the target alive, if necessary.
            hpx::id_type target_;
            naming::address_type lva_;
            naming::component_type comptype_;
            Action::arguments_type args_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action>
        class continuation_thread_function
        {
        public:
            template <typename... Ts>
            explicit continuation_thread_function(hpx::id_type&& target,
                Action::continuation_type&& cont, naming::address_type lva,
                naming::component_type comptype, Ts&&... vs)
              : target_(HPX_MOVE(target))
              , cont_(HPX_MOVE(cont))
              , lva_(lva)
              , comptype_(comptype)
              , args_(HPX_FORWARD(Ts, vs)...)
            {
            }

            template <typename State,
                typename Enable = std::enable_if_t<
                    std::is_same_v<State, threads::thread_restart_state>>>
            threads::thread_result_type operator()(State)
            {
                LTM_(debug).format("Executing {} with continuation({})",
                    Action::get_action_name(lva_), cont_.get_id());

                traits::action_trigger_continuation<
                    typename Action::continuation_type>::call(HPX_MOVE(cont_),
                    hpx::functional::invoke_fused{},
                    action_invoke<Action>{lva_, comptype_}, HPX_MOVE(args_));

                return {threads::thread_schedule_state::terminated,
                    threads::invalid_thread_id};
            }

        private:
            // This holds the target alive, if necessary.
            hpx::id_type target_;
            Action::continuation_type cont_;
            naming::address_type lva_;
            naming::component_type comptype_;
            Action::arguments_type args_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct is_non_const_reference
          : std::integral_constant<bool,
                std::is_lvalue_reference_v<T> &&
                    !std::is_const_v<std::remove_reference_t<T>>>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        inline std::string make_action_name(std::string_view action_name)
        {
            return hpx::util::format("action({})", action_name);
        }
    }    // namespace detail

    template <typename Component, typename R, typename... Args,
        typename Derived>
    struct basic_action<Component, R(Args...), Derived>
    {
#if !defined(HPX_SERIALIZATION_HAVE_ALLOW_RAW_POINTER_SERIALIZATION)
        // Flag the use of raw pointer types as action arguments
        static_assert(!util::any_of_v<std::is_pointer<Args>...>,
            "Using raw pointers as arguments for actions is not supported.");
#endif

        // Flag the use of array types as action arguments
        static_assert(
            !util::any_of_v<std::is_array<std::remove_reference_t<Args>>...>,
            "Using arrays as arguments for actions is not supported.");

        // Flag the use of non-const reference types as action arguments
        static_assert(!util::any_of_v<detail::is_non_const_reference<Args>...>,
            "Using non-const references as arguments for actions is not "
            "supported.");

        using component_type = Component;
        using derived_type = Derived;

        // result_type represents the type returned when invoking operator()
        using result_type = traits::promise_local_result<R>::type;

        // The remote_result_type is the remote type for the type_continuation
        using remote_result_type = traits::action_remote_result<R>::type;

        // The local_result_type is the local type for the type_continuation
        using local_result_type =
            traits::promise_local_result<remote_result_type>::type;

        using continuation_type =
            traits::action_continuation<basic_action>::type;

        static constexpr std::size_t arity = sizeof...(Args);

        using internal_result_type = R;
        using arguments_type = hpx::tuple<std::decay_t<Args>...>;

        using action_tag = void;

        ///////////////////////////////////////////////////////////////////////
        static std::string get_action_name(naming::address_type /*lva*/)
        {
            return detail::make_action_name(detail::get_action_name<Derived>());
        }

        template <typename... Ts>
        static R invoke(naming::address_type /*lva*/,
            naming::component_type /*comptype*/, Ts&&... /*vs*/);

        template <typename... Ts>
        static remote_result_type invoker(naming::address_type lva,
            naming::component_type comptype, Ts&&... vs)
        {
            if constexpr (std::is_void_v<R>)
            {
                Derived::invoke(lva, comptype, std::forward<Ts>(vs)...);
                return util::unused;
            }
            else
            {
                return Derived::invoke(lva, comptype, std::forward<Ts>(vs)...);
            }
        }

    public:
        // This static construct_thread_function allows to construct
        // a proper thread function for a thread without having to
        // instantiate the base_action type. This is used by the applier in
        // case no continuation has been supplied.
        template <typename... Ts>
        static threads::thread_function_type construct_thread_function(
            hpx::id_type target, naming::address_type lva,
            naming::component_type comptype, Ts&&... vs)
        {
            using thread_function = detail::thread_function<Derived>;
            return traits::action_decorate_function<Derived>::call(lva,
                thread_function(
                    HPX_MOVE(target), lva, comptype, HPX_FORWARD(Ts, vs)...));
        }

        // This static construct_thread_function allows to construct
        // a proper thread function for a thread without having to
        // instantiate the base_action type. This is used by the applier in
        // case a continuation has been supplied
        template <typename... Ts>
        static threads::thread_function_type construct_thread_function(
            hpx::id_type target, continuation_type&& cont,
            naming::address_type lva, naming::component_type comptype,
            Ts&&... vs)
        {
            using thread_function =
                detail::continuation_thread_function<Derived>;
            return traits::action_decorate_function<Derived>::call(lva,
                thread_function(HPX_MOVE(target), HPX_MOVE(cont), lva, comptype,
                    HPX_FORWARD(Ts, vs)...));
        }

        // direct execution
        template <typename... Ts>
        static HPX_FORCEINLINE remote_result_type execute_function(
            naming::address_type lva, naming::component_type comptype,
            Ts&&... vs)
        {
            LTM_(debug).format("basic_action::execute_function {}",
                Derived::get_action_name(lva));

            return invoker(lva, comptype, HPX_FORWARD(Ts, vs)...);
        }

    private:
        ///////////////////////////////////////////////////////////////////////
        template <typename IdOrPolicy, typename Policy, typename... Ts>
        HPX_FORCEINLINE static result_type sync_invoke(Policy const& policy,
            IdOrPolicy const& id_or_policy, error_code&, Ts&&... vs)
        {
            return hpx::sync<basic_action>(
                policy, id_or_policy, HPX_FORWARD(Ts, vs)...);
        }

    public:
        ///////////////////////////////////////////////////////////////////////
        template <typename... Ts>
        HPX_FORCEINLINE result_type operator()(launch policy,
            hpx::id_type const& id, error_code& ec, Ts&&... vs) const
        {
            return sync_invoke(policy, id, ec, HPX_FORWARD(Ts, vs)...);
        }

        template <typename... Ts>
        HPX_FORCEINLINE result_type operator()(
            hpx::id_type const& id, error_code& ec, Ts&&... vs) const
        {
            return sync_invoke(launch::sync, id, ec, HPX_FORWARD(Ts, vs)...);
        }

        template <typename... Ts>
        HPX_FORCEINLINE result_type operator()(
            launch policy, hpx::id_type const& id, Ts&&... vs) const
        {
            return sync_invoke(policy, id, throws, HPX_FORWARD(Ts, vs)...);
        }

        template <typename... Ts>
        HPX_FORCEINLINE result_type operator()(
            hpx::id_type const& id, Ts&&... vs) const
        {
            return sync_invoke(
                launch::sync, id, throws, HPX_FORWARD(Ts, vs)...);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>, result_type>
        operator()(launch policy, DistPolicy const& dist_policy, error_code& ec,
            Ts&&... vs) const
        {
            return sync_invoke(policy, dist_policy, ec, HPX_FORWARD(Ts, vs)...);
        }

        template <typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>, result_type>
        operator()(
            DistPolicy const& dist_policy, error_code& ec, Ts&&... vs) const
        {
            return sync_invoke(
                launch::sync, dist_policy, ec, HPX_FORWARD(Ts, vs)...);
        }

        template <typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>, result_type>
        operator()(
            launch policy, DistPolicy const& dist_policy, Ts&&... vs) const
        {
            return sync_invoke(
                policy, dist_policy, throws, HPX_FORWARD(Ts, vs)...);
        }

        template <typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>, result_type>
        operator()(DistPolicy const& dist_policy, Ts&&... vs) const
        {
            return sync_invoke(
                launch::sync, dist_policy, throws, HPX_FORWARD(Ts, vs)...);
        }

        ///////////////////////////////////////////////////////////////////////
        /// retrieve component type
        static int get_component_type() noexcept
        {
            return static_cast<int>(
                components::get_component_type<Component>());
        }

        using direct_execution = std::false_type;

        // The function \a get_action_type returns whether this action needs
        // to be executed in a new thread or directly.
        static constexpr actions::action_flavor get_action_type() noexcept
        {
            return actions::action_flavor::plain_action;
        }

        /// Extract the current invocation count for this action
        static std::int64_t get_invocation_count(bool reset)
        {
            return util::get_and_reset_value(invocation_count_, reset);
        }

    private:
        static std::atomic<std::int64_t> invocation_count_;

    protected:
        static void increment_invocation_count()
        {
            ++invocation_count_;
        }
    };

    template <typename Component, typename R, typename... Args,
        typename Derived>
    std::atomic<std::int64_t>
        basic_action<Component, R(Args...), Derived>::invocation_count_(0);

    namespace detail {

        HPX_CXX_EXPORT template <typename Action>
        void register_local_action_invocation_count(
            invocation_count_registry& registry)
        {
            registry.register_class(
                hpx::actions::detail::get_action_name<Action>(),
                &Action::get_invocation_count);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT template <typename Action, typename Derived = void>
        struct action_type
        {
            using type = Derived;
        };

        template <typename Action>
        struct action_type<Action, void>
        {
            using type = Action;
        };

        HPX_CXX_EXPORT template <typename Action, typename Derived>
        using action_type_t = action_type<Action, Derived>::type;

    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename TF, TF F, typename Derived = void>
    struct action;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename TF, TF F, typename Derived = void>
    // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
    struct direct_action
      : action<TF, F,
            detail::action_type_t<direct_action<TF, F, Derived>, Derived>>
    {
        using derived_type = detail::action_type_t<direct_action, Derived>;
        using direct_execution = std::true_type;

        /// The function \a get_action_type returns whether this action needs
        /// to be executed in a new thread or directly.
        static constexpr actions::action_flavor get_action_type() noexcept
        {
            return actions::action_flavor::direct_action;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Base template allowing to generate a concrete action type from a function
    // pointer. It is instantiated only if the supplied pointer is not a
    // supported function pointer.
    HPX_CXX_EXPORT template <typename TF, TF F, typename Derived = void,
        typename Direct = std::false_type>
    struct make_action
    {
        using type = action<TF, F, Derived>;
    };

    template <typename TF, TF F, typename Derived>
    struct make_action<TF, F, Derived, std::true_type>
    {
        using type = direct_action<TF, F, Derived>;
    };

    HPX_CXX_EXPORT template <typename TF, TF F, typename Derived = void,
        typename Direct = std::false_type>
    using make_action_t = make_action<TF, F, Derived, Direct>::type;

    HPX_CXX_EXPORT template <typename TF, TF F, typename Derived = void>
    struct make_direct_action : make_action<TF, F, Derived, std::true_type>
    {
    };

    HPX_CXX_EXPORT template <typename TF, TF F, typename Derived>
    using make_direct_action_t = make_direct_action<TF, F, Derived>::type;

    /// \endcond
}    // namespace hpx::actions
// namespace hpx::actions

#include <hpx/config/warnings_suffix.hpp>

/// \cond NOINTERNAL
