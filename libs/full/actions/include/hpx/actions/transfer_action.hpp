//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//  Copyright (c)      2011 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file transfer_action.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions/base_action.hpp>
#include <hpx/actions/post_helper.hpp>
#include <hpx/actions/register_action.hpp>
#include <hpx/actions/transfer_base_action.hpp>
#include <hpx/modules/actions_base.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::actions {

    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action>
    struct transfer_action final : transfer_base_action<Action>
    {
        transfer_action(transfer_action const&) = delete;
        transfer_action(transfer_action&&) = delete;
        transfer_action& operator=(transfer_action const&) = delete;
        transfer_action& operator=(transfer_action&&) = delete;

        using base_type = transfer_base_action<Action>;

        // construct an empty transfer_action to avoid serialization overhead
        transfer_action() = default;

        ~transfer_action() noexcept override;

        // construct an action from its arguments
        template <typename... Ts>
        explicit transfer_action(Ts&&... vs);

        template <typename... Ts>
        explicit transfer_action(hpx::launch policy, Ts&&... vs);

        bool has_continuation() const override;

        /// The \a get_thread_function constructs a proper thread function for
        /// a \a thread, encapsulating the functionality and the arguments
        /// of the action it is called for.
        ///
        /// \param target
        /// \param lva    [in] This is the local virtual address of the
        ///               component the action has to be invoked on.
        /// \param comptype
        ///
        /// \returns      This function returns a proper thread function usable
        ///               for a \a thread.
        ///
        /// \note This \a get_thread_function will be invoked to retrieve the
        ///       thread function for an action which has to be invoked without
        ///       continuations.
        template <std::size_t... Is>
        threads::thread_function_type get_thread_function(
            util::index_pack<Is...>, hpx::id_type&& target,
            naming::address::address_type lva,
            naming::address::component_type comptype);

        threads::thread_function_type get_thread_function(hpx::id_type&& target,
            naming::address::address_type lva,
            naming::address::component_type comptype) override;

        template <std::size_t... Is>
        void schedule_thread(util::index_pack<Is...>,
            naming::gid_type const& target_gid,
            naming::address::address_type lva,
            naming::address::component_type comptype, std::size_t num_thread);

        // schedule a new thread
        void schedule_thread(naming::gid_type const& target_gid,
            naming::address::address_type lva,
            naming::address::component_type comptype,
            std::size_t num_thread) override;

        // serialization support
        // loading ...
        void load(hpx::serialization::input_archive& ar) override;

        // saving ...
        void save(hpx::serialization::output_archive& ar) override;

        void load_schedule(serialization::input_archive& ar,
            naming::gid_type&& target, naming::address_type lva,
            naming::component_type comptype, std::size_t num_thread,
            bool& deferred_schedule) override;
    };
    /// \endcond

    template <typename Action>
    template <typename... Ts>
    transfer_action<Action>::transfer_action(Ts&&... vs)
      : base_type(HPX_FORWARD(Ts, vs)...)
    {
    }

    template <typename Action>
    template <typename... Ts>
    transfer_action<Action>::transfer_action(hpx::launch policy, Ts&&... vs)
      : base_type(policy, HPX_FORWARD(Ts, vs)...)
    {
    }

    template <typename Action>
    bool transfer_action<Action>::has_continuation() const
    {
        return false;
    }

    template <typename Action>
    template <std::size_t... Is>
    threads::thread_function_type transfer_action<Action>::get_thread_function(
        util::index_pack<Is...>, hpx::id_type&& target,
        naming::address::address_type lva,
        naming::address::component_type comptype)
    {
        return base_type::derived_type::construct_thread_function(
            HPX_MOVE(target), lva, comptype,
            hpx::get<Is>(HPX_MOVE(this->arguments_))...);
    }

    template <typename Action>
    threads::thread_function_type transfer_action<Action>::get_thread_function(
        hpx::id_type&& target, naming::address::address_type lva,
        naming::address::component_type comptype)
    {
        return get_thread_function(
            typename util::make_index_pack<Action::arity>::type(),
            HPX_MOVE(target), lva, comptype);
    }

    template <typename Action>
    template <std::size_t... Is>
    void transfer_action<Action>::schedule_thread(util::index_pack<Is...>,
        naming::gid_type const& target_gid, naming::address::address_type lva,
        naming::address::component_type comptype, std::size_t /*num_thread*/)
    {
        hpx::id_type target;
        if (naming::detail::has_credits(target_gid))
        {
            target = hpx::id_type(
                target_gid, hpx::id_type::management_type::managed);
        }

        threads::thread_init_data data;
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        data.description = threads::thread_description(
            actions::detail::get_action_name<Action>(),
            actions::detail::get_action_name_tracing<Action>());
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
        data.parent_id = this->parent_id_;
        data.parent_locality_id = this->parent_locality_;
#endif
        data.timer_data = threads::thread_init_data::setup_timer_data(data);
        data.priority = this->priority_;
        data.stacksize = this->stacksize_;

        hpx::detail::post_helper<typename base_type::derived_type>::call(
            HPX_MOVE(data), HPX_MOVE(target), lva, comptype,
            HPX_MOVE(hpx::get<Is>(this->arguments_))...);
    }

    template <typename Action>
    void transfer_action<Action>::schedule_thread(
        naming::gid_type const& target_gid, naming::address::address_type lva,
        naming::address::component_type comptype, std::size_t num_thread)
    {
        schedule_thread(typename util::make_index_pack<Action::arity>::type(),
            target_gid, lva, comptype, num_thread);

        // keep track of number of invocations
        this->increment_invocation_count();
    }

    template <typename Action>
    void transfer_action<Action>::load(hpx::serialization::input_archive& ar)
    {
        this->load_base(ar);
    }

    template <typename Action>
    void transfer_action<Action>::save(hpx::serialization::output_archive& ar)
    {
        this->save_base(ar);
    }

    template <typename Action>
    void transfer_action<Action>::load_schedule(
        serialization::input_archive& ar, naming::gid_type&& target,
        naming::address_type lva, naming::component_type comptype,
        std::size_t num_thread, bool& deferred_schedule)
    {
        // First, serialize, then schedule
        load(ar);

        if (deferred_schedule)
        {
            // If this is a direct action and deferred schedule was requested,
            // i.e. if we are not the last parcel, return immediately
            if constexpr (base_type::direct_execution::value)
            {
                return;
            }

            // If this is not a direct action, we can safely set
            // deferred_schedule to false
            deferred_schedule = false;
        }

        schedule_thread(HPX_MOVE(target), lva, comptype, num_thread);
    }

    // define registration function
    template <typename Action>
    base_action* detail::register_action<Action>::create()
    {
        return new transfer_action<Action>{};
    }

    template <typename Action>
    transfer_action<Action>::~transfer_action() noexcept
    {
        // make sure proper register action function is instantiated
        [[maybe_unused]] auto* ptr = &detail::register_action<Action>::create;
    }
}    // namespace hpx::actions

/// \cond NOINTERNAL
namespace hpx::traits {

    template <typename Action>
    struct needs_automatic_registration<hpx::actions::transfer_action<Action>>
      : needs_automatic_registration<Action>
    {
    };
}    // namespace hpx::traits
/// \endcond

#include <hpx/config/warnings_suffix.hpp>

#endif
