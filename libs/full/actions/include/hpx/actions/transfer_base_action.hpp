//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c) 2011-2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file transfer_action.hpp

#pragma once

#include <hpx/config/defines.hpp>

#include <hpx/actions/actions_fwd.hpp>
#include <hpx/actions/base_action.hpp>
#include <hpx/actions/register_action.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/modules/parcelset_base.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::actions {

    ///////////////////////////////////////////////////////////////////////////
    // If one or more arguments of the action are non-default-constructible,
    // the transfer_action does not store the argument tuple directly but a
    // unique_ptr to the tuple instead.
    namespace detail {

        template <typename Args>
        struct argument_holder
        {
            argument_holder() = default;

            explicit argument_holder(Args&& args)
              : data_(new Args(HPX_MOVE(args)))
            {
            }

            template <typename... Ts>
            explicit argument_holder(Ts&&... ts)
              : data_(new Args(HPX_FORWARD(Ts, ts)...))
            {
            }

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const)
            {
                // clang-format off
                ar & data_;
                // clang-format on
            }

            HPX_HOST_DEVICE HPX_FORCEINLINE Args& data()
            {
                HPX_ASSERT(!!data_);
                return *data_;
            }

            HPX_HOST_DEVICE HPX_FORCEINLINE Args const& data() const
            {
                HPX_ASSERT(!!data_);
                return *data_;
            }

        private:
            std::unique_ptr<Args> data_;
        };
    }    // namespace detail
}    // namespace hpx::actions

namespace hpx {

    HPX_CXX_EXPORT template <std::size_t I, typename Args>
    constexpr HPX_HOST_DEVICE HPX_FORCEINLINE hpx::tuple_element_t<I, Args>&
    get(hpx::actions::detail::argument_holder<Args>& t)
    {
        return hpx::tuple_element<I, Args>::get(t.data());
    }

    HPX_CXX_EXPORT template <std::size_t I, typename Args>
    constexpr HPX_HOST_DEVICE HPX_FORCEINLINE
        hpx::tuple_element_t<I, Args> const&
        get(hpx::actions::detail::argument_holder<Args> const& t)
    {
        return hpx::tuple_element<I, Args>::get(t.data());
    }

    HPX_CXX_EXPORT template <std::size_t I, typename Args>
    constexpr HPX_HOST_DEVICE HPX_FORCEINLINE hpx::tuple_element_t<I, Args>&&
    get(hpx::actions::detail::argument_holder<Args>&& t)
    {
        return std::forward<hpx::tuple_element_t<I, Args>>(
            hpx::get<I>(t.data()));
    }

    HPX_CXX_EXPORT template <std::size_t I, typename Args>
    constexpr HPX_HOST_DEVICE HPX_FORCEINLINE
        hpx::tuple_element_t<I, Args> const&&
        get(hpx::actions::detail::argument_holder<Args> const&& t)
    {
        return std::forward<hpx::tuple_element_t<I, Args> const>(
            hpx::get<I>(t.data()));
    }
}    // namespace hpx

namespace hpx::actions {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action>
    struct transfer_base_action : base_action_data
    {
        transfer_base_action(transfer_base_action const&) = delete;
        transfer_base_action(transfer_base_action&&) = delete;
        transfer_base_action& operator=(transfer_base_action const&) = delete;
        transfer_base_action& operator=(transfer_base_action&&) = delete;

        using component_type = typename Action::component_type;
        using derived_type = typename Action::derived_type;
        using result_type = typename Action::result_type;
        using arguments_base_type = typename Action::arguments_type;
        using arguments_type =
            std::conditional_t<std::is_constructible_v<arguments_base_type>,
                arguments_base_type,
                detail::argument_holder<arguments_base_type>>;

        using continuation_type =
            typename traits::action_continuation<Action>::type;

        // This is the priority value this action has been instantiated with
        // (statically). This value might be different from the priority member
        // holding the runtime value an action has been created with
        static constexpr threads::thread_priority priority_value =
            traits::action_priority_v<Action>;

        // This is the stacksize value this action has been instantiated with
        // (statically). This value might be different from the stacksize member
        // holding the runtime value an action has been created with
        static constexpr threads::thread_stacksize stacksize_value =
            traits::action_stacksize_v<Action>;

        using direct_execution = typename Action::direct_execution;

        // construct an empty transfer_action to avoid serialization overhead
        transfer_base_action() = default;

        // construct an action from its arguments
        template <typename T, typename... Ts,
            typename = std::enable_if_t<
                !std::is_convertible_v<std::decay_t<T>, hpx::launch>>>
        explicit transfer_base_action(T&& t, Ts&&... vs)
          : base_action_data(threads::thread_priority::default_,
                threads::thread_stacksize::default_)
          , arguments_(HPX_FORWARD(T, t), HPX_FORWARD(Ts, vs)...)
        {
        }

        template <typename... Ts>
        explicit transfer_base_action(hpx::launch policy, Ts&&... vs)
          : base_action_data(policy.priority(), policy.stacksize())
          , arguments_(HPX_FORWARD(Ts, vs)...)
        {
        }

        //
        ~transfer_base_action() noexcept override
        {
            detail::register_action<derived_type>::instance.instantiate();
        }

        /// retrieve component type
        static int get_static_component_type()
        {
            return derived_type::get_component_type();
        }

    private:
        /// The function \a get_component_type returns the \a component_type
        /// of the component this action belongs to.
        int get_component_type() const override
        {
            return derived_type::get_component_type();
        }

        /// The function \a get_action_name returns the name of this action
        /// (mainly used for debugging and logging purposes).
        char const* get_action_name() const override
        {
            return detail::get_action_name<derived_type>();
        }

        /// The function \a get_serialization_id returns the id which has been
        /// associated with this action (mainly used for serialization purposes).
        std::uint32_t get_action_id() const override
        {
            return detail::get_action_id<derived_type>();
        }

        /// The function \a get_action_type returns whether this action needs
        /// to be executed in a new thread or directly.
        action_flavor get_action_type() const override
        {
            return derived_type::get_action_type();
        }

        /// Return whether the embedded action is part of termination detection
        bool does_termination_detection() const override
        {
            return traits::action_does_termination_detection<
                derived_type>::call();
        }

        /// Return whether the given object was migrated
        std::pair<bool, components::pinned_ptr> was_object_migrated(
            hpx::naming::gid_type const& id,
            naming::address::address_type lva) override
        {
            return traits::action_was_object_migrated<derived_type>::call(
                id, lva);
        }

        /// Return a pointer to the filter to be used while serializing an
        /// instance of this action type.
        serialization::binary_filter* get_serialization_filter() const override
        {
            return traits::action_serialization_filter<derived_type>::call();
        }

        /// Return an embedded parcel if available (e.g. routing action).
        hpx::optional<parcelset::parcel> get_embedded_parcel() const override
        {
            return traits::action_get_embedded_parcel<Action>::call(*this);
        }

        /// Return a pointer to the message handler to be used for this action.
        parcelset::policies::message_handler* get_message_handler(
            parcelset::locality const& loc) const override
        {
            return traits::action_message_handler<derived_type>::call(loc);
        }

    public:
        /// retrieve the N's argument
        template <std::size_t N>
        constexpr typename hpx::tuple_element<N, arguments_type>::type const&
        get() const
        {
            return hpx::get<N>(arguments_);
        }

        /// Extract the current invocation count for this action
        static std::int64_t get_invocation_count(bool reset)
        {
            return util::get_and_reset_value(invocation_count_, reset);
        }

        // serialization support
        // loading ...
        void load_base(hpx::serialization::input_archive& ar)
        {
            ar >> arguments_;
            this->base_action_data::load_base(ar);
        }

        // saving ...
        void save_base(hpx::serialization::output_archive& ar) const
        {
            ar << arguments_;
            this->base_action_data::save_base(ar);
        }

    protected:
        arguments_type arguments_;

    private:
        static std::atomic<std::int64_t> invocation_count_;

    protected:
        static void increment_invocation_count()
        {
            ++invocation_count_;
        }
    };

    template <typename Action>
    std::atomic<std::int64_t> transfer_base_action<Action>::invocation_count_(
        0);

    namespace detail {

        template <typename Action>
        void register_remote_action_invocation_count(
            invocation_count_registry& registry)
        {
            registry.register_class(
                hpx::actions::detail::get_action_name<Action>(),
                &transfer_base_action<Action>::get_invocation_count);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <std::size_t N, typename Action>
    constexpr hpx::tuple_element_t<N,
        typename transfer_action<Action>::arguments_type> const&
    get(transfer_base_action<Action> const& args)
    {
        return args.template get<N>();
    }
}    // namespace hpx::actions

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
#include <hpx/modules/actions_base.hpp>

/// \cond NOINTERNAL
HPX_CXX_EXPORT template <typename Action>
void hpx::actions::detail::register_per_action_data_counter_types(
    hpx::actions::detail::per_action_data_counter_registry& registry)
{
    registry.register_class(hpx::actions::detail::get_action_name<Action>());
}
/// \endcond

#endif    // HPX_HAVE_PARCELPORT_ACTION_COUNTERS

#endif    // HPX_HAVE_NETWORKING
