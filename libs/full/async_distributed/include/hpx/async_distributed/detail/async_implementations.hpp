//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_distributed/detail/async_implementations_fwd.hpp>
#include <hpx/async_distributed/packaged_action.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/tracing.hpp>

#include <cstddef>
#include <utility>

namespace hpx::detail {

    /// \cond NOINTERNAL
    ///////////////////////////////////////////////////////////////////////////
    struct keep_id_alive
    {
        explicit keep_id_alive(hpx::id_type&& id) noexcept
          : id_(HPX_MOVE(id))
        {
        }

        constexpr void operator()() const noexcept {}

        hpx::id_type id_;
    };

    template <typename T>
    future<T> keep_alive(future<T>&& f, id_type id)
    {
        if (id.get_management_type() == hpx::id_type::management_type::managed)
        {
            traits::detail::get_shared_state(f)->set_on_completed(
                hpx::detail::keep_id_alive(HPX_MOVE(id)));
        }
        return HPX_MOVE(f);
    }

    struct keep_id_and_ptr_alive
    {
        explicit keep_id_and_ptr_alive(
            hpx::id_type&& id, components::pinned_ptr&& p) noexcept
          : id_(HPX_MOVE(id))
          , p_(HPX_MOVE(p))
        {
        }

        constexpr void operator()() const noexcept {}

        hpx::id_type id_;
        components::pinned_ptr p_;
    };

    template <typename T>
    future<T> keep_alive(future<T>&& f, id_type id, components::pinned_ptr&& p)
    {
        if (id.get_management_type() == hpx::id_type::management_type::managed)
        {
            traits::detail::get_shared_state(f)->set_on_completed(
                hpx::detail::keep_id_and_ptr_alive(HPX_MOVE(id), HPX_MOVE(p)));
        }
        return HPX_MOVE(f);
    }

    template <typename Result>
    class handle_managed_target
    {
    public:
        handle_managed_target(handle_managed_target const&) = delete;
        handle_managed_target(handle_managed_target&&) = delete;
        handle_managed_target& operator=(handle_managed_target const&) = delete;
        handle_managed_target& operator=(handle_managed_target&&) = delete;

        handle_managed_target(hpx::id_type const& id, future<Result>& f)
          : target_is_managed_(false)
          , id_(id)
          , f_(f)
        {
            if (id.get_management_type() ==
                hpx::id_type::management_type::managed)
            {
                unmanaged_id_ = hpx::id_type(
                    id.get_gid(), hpx::id_type::management_type::unmanaged);
                target_is_managed_ = true;
            }
        }

        ~handle_managed_target()
        {
            // keep id alive, if needed - this allows to send the destination
            // as an unmanaged id
            if (target_is_managed_)
            {
                if (auto const& state = traits::detail::get_shared_state(f_))
                {
                    HPX_ASSERT(id_);
                    HPX_ASSERT(unmanaged_id_);
                    state->set_on_completed(
                        hpx::detail::keep_id_alive(HPX_MOVE(id_)));
                }
            }
        }

        hpx::id_type const& get_id() const
        {
            return target_is_managed_ ? unmanaged_id_ : id_;
        }

        bool target_is_managed_;
        hpx::id_type id_;
        hpx::id_type unmanaged_id_;
        future<Result>& f_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Result>
    struct sync_local_invoke
    {
        template <typename... Ts>
        static hpx::future<Result> call(
            hpx::id_type const& /*id*/, naming::address&& addr, Ts&&... vs)
        {
            try
            {
                using remote_result_type = Action::remote_result_type;
                using get_remote_result_type =
                    traits::get_remote_result<Result, remote_result_type>;

                return make_ready_future(
                    get_remote_result_type::call(Action::execute_function(
                        addr.address_, addr.type_, HPX_FORWARD(Ts, vs)...)));
            }
            catch (...)
            {
                return make_exceptional_future<Result>(
                    std::current_exception());
            }
        }
    };

    template <typename Action>
    bool can_invoke_locally()
    {
        std::ptrdiff_t const requested_stack_size =
            threads::get_stack_size(traits::action_stacksize_v<Action>);
        constexpr bool df = traits::action_decorate_function<Action>::value;
        return !df && this_thread::get_stack_size() >= requested_stack_size &&
            this_thread::has_sufficient_stack_space(requested_stack_size);
    }

    template <typename Action>
    struct sync_local_invoke<Action, void>
    {
        template <typename... Ts>
        static hpx::future<void> call(
            hpx::id_type const& /*id*/, naming::address&& addr, Ts&&... vs)
        {
            try
            {
                Action::execute_function(
                    addr.address_, addr.type_, HPX_FORWARD(Ts, vs)...);

                return make_ready_future();
            }
            catch (...)
            {
                return make_exceptional_future<void>(std::current_exception());
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename Result>
    struct sync_local_invoke_cb
    {
        template <typename Callback, typename... Ts>
        static hpx::future<Result> call(hpx::id_type const& id,
            naming::address&& addr, Callback&& cb, Ts&&... vs)
        {
            future<Result> f;
            {
                handle_managed_target<Result> hmt(id, f);

                using allocator_type =
                    hpx::util::thread_local_caching_allocator<
                        hpx::lockfree::variable_size_stack,
                        hpx::util::internal_allocator<>>;
                lcos::packaged_action<Action, Result> p(
                    std::allocator_arg, allocator_type{});

                f = p.get_future();
                p.post_cb(HPX_MOVE(addr), hmt.get_id(),
                    HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
                f.wait();
            }
            return f;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_remote_impl(launch::sync_policy, hpx::id_type const& id,
        naming::address&& addr, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = action_type::local_result_type;

        future<result_type> f;
        {
            handle_managed_target<result_type> hmt(id, f);

            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;
            lcos::packaged_action<action_type, result_type> p(
                std::allocator_arg, allocator_type{});

            f = p.get_future();
            p.post(HPX_MOVE(addr), hmt.get_id(), HPX_FORWARD(Ts, vs)...);
            f.wait();
        }
        return f;
    }

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_remote_impl(launch::async_policy policy, hpx::id_type const& id,
        naming::address&& addr, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = action_type::local_result_type;

        hpx::future<result_type> f;
        {
            handle_managed_target<result_type> hmt(id, f);

            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;
            lcos::packaged_action<action_type, result_type> p(
                std::allocator_arg, allocator_type{});

            f = p.get_future();
            p.post_p(
                HPX_MOVE(addr), hmt.get_id(), policy, HPX_FORWARD(Ts, vs)...);
        }
        return f;
    }

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_remote_impl(launch::deferred_policy policy, hpx::id_type const& id,
        naming::address&& addr, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = action_type::local_result_type;

        hpx::future<result_type> f;
        {
            handle_managed_target<result_type> hmt(id, f);

            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;
            lcos::packaged_action<action_type, result_type> p(
                std::allocator_arg, allocator_type{});

            f = p.get_future();
            p.post_deferred(
                HPX_MOVE(addr), hmt.get_id(), policy, HPX_FORWARD(Ts, vs)...);
        }
        return f;
    }

    // generic function for dynamic launch policy
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_remote_impl(launch policy, hpx::id_type const& id,
        naming::address&& addr, Ts&&... vs)
    {
        if (policy == launch::sync)
        {
            return async_remote_impl<Action>(
                launch::sync, id, HPX_MOVE(addr), HPX_FORWARD(Ts, vs)...);
        }
        if (hpx::has_async_policy(policy))
        {
            return async_remote_impl<Action>(
                launch::async, id, HPX_MOVE(addr), HPX_FORWARD(Ts, vs)...);
        }
        if (policy == launch::deferred)
        {
            return async_remote_impl<Action>(
                launch::deferred, id, HPX_MOVE(addr), HPX_FORWARD(Ts, vs)...);
        }

        HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "async_remote_impl",
            "unknown launch policy");
    }

    ///////////////////////////////////////////////////////////////////////////
    // do local invocation, if possible
    template <typename Action>
    struct action_invoker
    {
        using remote_result_type = Action::remote_result_type;
        using result_type = Action::local_result_type;
        using get_remote_result_type =
            traits::get_remote_result<result_type, remote_result_type>;

        template <typename... Ts>
        HPX_FORCEINLINE result_type operator()(
            naming::address::address_type lva,
            naming::address::component_type comptype, Ts&&... vs) const
        {
            return get_remote_result_type::call(
                Action::invoker(lva, comptype, HPX_FORWARD(Ts, vs)...));
        }
    };
}    // namespace hpx::detail

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
///////////////////////////////////////////////////////////////////////////////
namespace hpx::traits {

    template <typename Action>
    struct get_function_address<hpx::detail::action_invoker<Action>>
    {
        static constexpr std::size_t call(
            hpx::detail::action_invoker<Action> const&) noexcept
        {
            return reinterpret_cast<std::size_t>(&Action::invoker);
        }
    };

    template <typename Action>
    struct get_function_annotation<hpx::detail::action_invoker<Action>>
    {
        static constexpr char const* call(
            hpx::detail::action_invoker<Action> const&) noexcept
        {
            return hpx::actions::detail::get_action_name<Action>();
        }
    };

    template <typename Action>
    struct get_function_annotation_tracing<hpx::detail::action_invoker<Action>>
    {
        static hpx::tracing::annotation_handle call(
            hpx::detail::action_invoker<Action> const&) noexcept
        {
            return hpx::actions::detail::get_action_name_tracing<Action>();
        }
    };
}    // namespace hpx::traits
#endif

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_local_impl(launch policy, hpx::id_type const& id,
        naming::address& addr, std::pair<bool, components::pinned_ptr>& r,
        Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = action_type::local_result_type;

        if (policy == launch::sync || action_type::direct_execution::value)
        {
            return hpx::detail::sync_local_invoke<action_type,
                result_type>::call(id, HPX_MOVE(addr), HPX_FORWARD(Ts, vs)...);
        }
        if (hpx::has_async_policy(policy))
        {
            return keep_alive(
                hpx::async(policy, action_invoker<action_type>(), addr.address_,
                    addr.type_, HPX_FORWARD(Ts, vs)...),
                id, HPX_MOVE(r.second));
        }

        HPX_ASSERT(policy == launch::deferred);

        return keep_alive(
            hpx::async(launch::deferred, action_invoker<action_type>(),
                addr.address_, addr.type_, HPX_FORWARD(Ts, vs)...),
            id, HPX_MOVE(r.second));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Launch, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_impl(Launch&& policy, hpx::id_type const& id, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using component_type = action_type::component_type;

        [[maybe_unused]] std::pair<bool, components::pinned_ptr> r;
        naming::address addr;

        if constexpr (traits::component_supports_migration<
                          component_type>::call())
        {
            auto f = [id](naming::address const& addr) {
                return traits::action_was_object_migrated<Action>::call(
                    id, addr.address_);
            };

            if (agas::is_local_address_cached(id, addr, r, HPX_MOVE(f)) &&
                can_invoke_locally<action_type>() && !r.first)
            {
                // route launch policy through component
                launch const adapted_policy =
                    traits::action_select_direct_execution<Action>::call(
                        policy, addr.address_);

                return async_local_impl<Action>(
                    adapted_policy, id, addr, r, HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr) &&
                can_invoke_locally<action_type>())
            {
                // route launch policy through component
                launch const adapted_policy =
                    traits::action_select_direct_execution<Action>::call(
                        policy, addr.address_);

                return async_local_impl<Action>(
                    adapted_policy, id, addr, r, HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }

        // Note: the pinned_ptr is still being held, if necessary
        return async_remote_impl<Action>(HPX_FORWARD(Launch, policy), id,
            HPX_MOVE(addr), HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// \note This function is part of the invocation policy implemented by
    ///       this class
    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_cb_impl(
        launch policy, hpx::id_type const& id, Callback&& cb, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = action_type::local_result_type;
        using component_type = action_type::component_type;

        [[maybe_unused]] std::pair<bool, components::pinned_ptr> r;
        naming::address addr;

        if constexpr (traits::component_supports_migration<
                          component_type>::call())
        {
            auto f = [id](naming::address const& addr) {
                return traits::action_was_object_migrated<action_type>::call(
                    id, addr.address_);
            };

            if (agas::is_local_address_cached(id, addr, r, HPX_MOVE(f)) &&
                can_invoke_locally<action_type>() && !r.first)
            {
                // route launch policy through component
                policy =
                    traits::action_select_direct_execution<action_type>::call(
                        policy, addr.address_);

                if (policy == launch::sync ||
                    action_type::direct_execution::value)
                {
                    return sync_local_invoke_cb<action_type, result_type>::call(
                        id, HPX_MOVE(addr), HPX_FORWARD(Callback, cb),
                        HPX_FORWARD(Ts, vs)...);
                }
            }

            // fall through
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr) &&
                can_invoke_locally<action_type>())
            {
                // route launch policy through component
                policy =
                    traits::action_select_direct_execution<action_type>::call(
                        policy, addr.address_);

                if (policy == launch::sync ||
                    action_type::direct_execution::value)
                {
                    return sync_local_invoke_cb<action_type, result_type>::call(
                        id, HPX_MOVE(addr), HPX_FORWARD(Callback, cb),
                        HPX_FORWARD(Ts, vs)...);
                }
            }

            // fall through
        }

        // Note: the pinned_ptr is still being held, if necessary
        future<result_type> f;
        {
            handle_managed_target<result_type> hmt(id, f);

            if (policy == launch::sync || hpx::has_async_policy(policy))
            {
                using allocator_type =
                    hpx::util::thread_local_caching_allocator<
                        hpx::lockfree::variable_size_stack,
                        hpx::util::internal_allocator<>>;
                lcos::packaged_action<action_type, result_type> p(
                    std::allocator_arg, allocator_type{});

                f = p.get_future();
                p.post_p_cb(HPX_MOVE(addr), hmt.get_id(), policy,
                    HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
                if (policy == launch::sync)
                    f.wait();
            }
            else if (policy == launch::deferred)
            {
                using allocator_type =
                    hpx::util::thread_local_caching_allocator<
                        hpx::lockfree::variable_size_stack,
                        hpx::util::internal_allocator<>>;
                lcos::packaged_action<action_type, result_type> p(
                    std::allocator_arg, allocator_type{});

                f = p.get_future();
                p.post_deferred_cb(HPX_MOVE(addr), hmt.get_id(), policy,
                    HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
            }
            else
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "async_cb_impl",
                    "unknown launch policy");
            }
        }
        return f;
    }

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_cb_impl(hpx::launch::sync_policy policy, hpx::id_type const& id,
        Callback&& cb, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = action_type::local_result_type;
        using component_type = action_type::component_type;

        [[maybe_unused]] std::pair<bool, components::pinned_ptr> r;
        naming::address addr;

        if constexpr (traits::component_supports_migration<
                          component_type>::call())
        {
            auto f = [id](naming::address const& addr) {
                return traits::action_was_object_migrated<Action>::call(
                    id, addr.address_);
            };

            if (agas::is_local_address_cached(id, addr, r, HPX_MOVE(f)) &&
                can_invoke_locally<action_type>() && !r.first)
            {
                return sync_local_invoke_cb<action_type, result_type>::call(id,
                    HPX_MOVE(addr), HPX_FORWARD(Callback, cb),
                    HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr) &&
                can_invoke_locally<action_type>())
            {
                return sync_local_invoke_cb<action_type, result_type>::call(id,
                    HPX_MOVE(addr), HPX_FORWARD(Callback, cb),
                    HPX_FORWARD(Ts, vs)...);
            }

            // fall through
        }

        // Note: the pinned_ptr is still being held, if necessary
        future<result_type> f;
        {
            handle_managed_target<result_type> hmt(id, f);

            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;
            lcos::packaged_action<action_type, result_type> p(
                std::allocator_arg, allocator_type{});

            f = p.get_future();
            p.post_p_cb(HPX_MOVE(addr), hmt.get_id(), policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
            f.wait();
        }
        return f;
    }

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_cb_impl(hpx::launch::async_policy async_policy,
        hpx::id_type const& id, Callback&& cb, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = action_type::local_result_type;
        using component_type = action_type::component_type;

        [[maybe_unused]] std::pair<bool, components::pinned_ptr> r;
        naming::address addr;

        if constexpr (traits::component_supports_migration<
                          component_type>::call())
        {
            auto f = [id](naming::address const& addr) {
                return traits::action_was_object_migrated<Action>::call(
                    id, addr.address_);
            };

            if (agas::is_local_address_cached(id, addr, r, HPX_MOVE(f)) &&
                can_invoke_locally<action_type>() && !r.first)
            {
                launch const policy =
                    traits::action_select_direct_execution<Action>::call(
                        async_policy, addr.address_);

                if (policy == launch::sync ||
                    action_type::direct_execution::value)
                {
                    return sync_local_invoke_cb<action_type, result_type>::call(
                        id, HPX_MOVE(addr), HPX_FORWARD(Callback, cb),
                        HPX_FORWARD(Ts, vs)...);
                }
            }

            // fall through
        }
        else
        {
            // non-migratable objects
            if (agas::is_local_address_cached(id, addr) &&
                can_invoke_locally<action_type>())
            {
                launch const policy =
                    traits::action_select_direct_execution<Action>::call(
                        async_policy, addr.address_);

                if (policy == launch::sync ||
                    action_type::direct_execution::value)
                {
                    return sync_local_invoke_cb<action_type, result_type>::call(
                        id, HPX_MOVE(addr), HPX_FORWARD(Callback, cb),
                        HPX_FORWARD(Ts, vs)...);
                }
            }

            // fall through
        }

        // Note: the pinned_ptr is still being held, if necessary
        future<result_type> f;
        {
            handle_managed_target<result_type> hmt(id, f);

            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;
            lcos::packaged_action<action_type, result_type> p(
                std::allocator_arg, allocator_type{});

            f = p.get_future();
            p.post_p_cb(HPX_MOVE(addr), hmt.get_id(), async_policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }
        return f;
    }

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    hpx::future<
        typename hpx::traits::extract_action_t<Action>::local_result_type>
    async_cb_impl(hpx::launch::deferred_policy policy, hpx::id_type const& id,
        Callback&& cb, Ts&&... vs)
    {
        using action_type = hpx::traits::extract_action_t<Action>;
        using result_type = action_type::local_result_type;

        naming::address addr;
        [[maybe_unused]] bool result = agas::is_local_address_cached(id, addr);

        future<result_type> f;
        {
            handle_managed_target<result_type> hmt(id, f);

            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;
            lcos::packaged_action<action_type, result_type> p(
                std::allocator_arg, allocator_type{});

            f = p.get_future();
            p.post_deferred_cb(HPX_MOVE(addr), hmt.get_id(), policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }
        return f;
    }
    /// \endcond
}    // namespace hpx::detail
