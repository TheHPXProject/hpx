//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011-2017 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components_base/agas_interface.hpp>
#include <hpx/components_base/component_type.hpp>
#include <hpx/components_base/components_base_fwd.hpp>
#include <hpx/components_base/server/component_heap.hpp>
#include <hpx/components_base/server/create_component_fwd.hpp>
#include <hpx/components_base/server/wrapper_heap.hpp>
#include <hpx/components_base/server/wrapper_heap_list.hpp>
#include <hpx/components_base/traits/is_component.hpp>
#include <hpx/components_base/traits/managed_component_policies.hpp>
#include <hpx/modules/errors.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail_adl_barrier {

        template <typename BackPtrTag>
        struct init;

        template <>
        struct init<traits::construct_with_back_ptr>
        {
            template <typename Component, typename Managed>
            static constexpr void call(
                Component* /* component */, Managed* /* this_ */) noexcept
            {
            }

            template <typename Component, typename Managed, typename... Ts>
            static void call_new(
                Component*& component, Managed* this_, Ts&&... vs)
            {
                using wrapped_type = typename Managed::wrapped_type;
                component = new wrapped_type(this_, HPX_FORWARD(Ts, vs)...);
            }
        };

        template <>
        struct init<traits::construct_without_back_ptr>
        {
            template <typename Component, typename Managed>
            static void call(Component* component, Managed* this_)
            {
                component->set_back_ptr(this_);
            }

            template <typename Component, typename Managed, typename... Ts>
            static void call_new(
                Component*& component, Managed* this_, Ts&&... vs)
            {
                using wrapped_type = typename Managed::wrapped_type;
                component = new wrapped_type(HPX_FORWARD(Ts, vs)...);
                component->set_back_ptr(this_);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // This is used by the component implementation to decide whether to
        // delete the managed_component instance it depends on.
        template <typename DtorTag>
        struct destroy_backptr;

        template <>
        struct destroy_backptr<traits::managed_object_is_lifetime_controlled>
        {
            template <typename BackPtr>
            static void call(BackPtr* back_ptr)
            {
                // The managed_component's controls the lifetime of the
                // component implementation.
                back_ptr->finalize();
                std::destroy_at(back_ptr);
                component_heap<typename BackPtr::wrapped_type>().free(back_ptr);
            }
        };

        template <>
        struct destroy_backptr<traits::managed_object_controls_lifetime>
        {
            template <typename BackPtr>
            static constexpr void call(BackPtr*) noexcept
            {
                // The managed_component's lifetime is controlled by the
                // component implementation. Do nothing.
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // This is used by the managed_component to decide whether to
        // delete the component implementation depending on it.
        template <typename DtorTag>
        struct manage_lifetime;

        template <>
        struct manage_lifetime<traits::managed_object_is_lifetime_controlled>
        {
            template <typename Component>
            static constexpr void call(Component*) noexcept
            {
                // The managed_component's lifetime is controlled by the
                // component implementation. Do nothing.
            }

            template <typename Component>
            static void addref(Component* component) noexcept
            {
                intrusive_ptr_add_ref(component);
            }

            template <typename Component>
            static void release(Component* component) noexcept
            {
                intrusive_ptr_release(component);
            }
        };

        template <>
        struct manage_lifetime<traits::managed_object_controls_lifetime>
        {
            template <typename Component>
            static void call(Component* component) noexcept(
                noexcept(component->finalize()))
            {
                // The managed_component controls the lifetime of the
                // component implementation.
                component->finalize();
                delete component;
            }

            template <typename Component>
            static constexpr void addref(Component*) noexcept
            {
            }

            template <typename Component>
            static constexpr void release(Component*) noexcept
            {
            }
        };
    }    // namespace detail_adl_barrier

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT struct base_managed_component
          : traits::detail::managed_component_tag
        {
            // finalize() will be called just before the instance gets destructed
            static constexpr void finalize() noexcept {}

            static void mark_as_migrated() noexcept
            {
                // If this assertion is triggered then this component instance
                // is being migrated even if the component type has not been
                // enabled to support migration.
                HPX_ASSERT(false);
            }

            static void on_migrated() noexcept
            {
                // If this assertion is triggered then this component instance
                // is being migrated even if the component type has not been
                // enabled to support migration.
                HPX_ASSERT(false);
            }
        };
    }    // namespace detail

    HPX_CXX_EXPORT template <typename Component, typename Wrapper,
        typename CtorPolicy, typename DtorPolicy>
    // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
    class managed_component_base : public detail::base_managed_component
    {
    public:
        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        managed_component_base(managed_component_base const&) = delete;
        managed_component_base(managed_component_base&&) = delete;
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

        managed_component_base& operator=(
            managed_component_base const&) = delete;
        managed_component_base& operator=(managed_component_base&&) = delete;

        using this_component_type =
            std::conditional_t<std::is_void_v<Component>,
                managed_component_base, Component>;

        using wrapped_type = this_component_type;

        using has_managed_component_base = void;
        using ctor_policy = CtorPolicy;
        using dtor_policy = DtorPolicy;

        // make sure that we have a back_ptr whenever we need to control the
        // lifetime of the managed_component
        static_assert(
            (std::is_same_v<ctor_policy, traits::construct_without_back_ptr> ||
                std::is_same_v<dtor_policy,
                    traits::managed_object_controls_lifetime>),
            "std::is_same_v<ctor_policy, construct_without_back_ptr> || "
            "std::is_same_v<dtor_policy, managed_object_controls_lifetime>");

        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        constexpr managed_component_base() noexcept = default;

        explicit managed_component_base(
            managed_component<Component, Wrapper>* back_ptr) noexcept
          : back_ptr_(back_ptr)
        {
            HPX_ASSERT(back_ptr);
        }
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

        // The implementation of the component is responsible for deleting the
        // actual managed component object
        ~managed_component_base()
        {
            detail_adl_barrier::destroy_backptr<dtor_policy>::call(back_ptr_);
        }

        // components must contain a typedef for wrapping_type defining the
        // managed_component type used to encapsulate instances of this
        // component
        using wrapping_type = managed_component<Component, Wrapper>;
        using base_type_holder = Component;

        hpx::id_type get_unmanaged_id() const;
        hpx::id_type get_id() const;

    protected:
        naming::gid_type get_base_gid() const;

        template <typename>
        friend struct detail_adl_barrier::init;

        void set_back_ptr(
            components::managed_component<Component, Wrapper>* bp) noexcept
        {
            HPX_ASSERT(nullptr == back_ptr_);
            HPX_ASSERT(bp);
            back_ptr_ = bp;
        }

    private:
        managed_component<Component, Wrapper>* back_ptr_ = nullptr;
    };

    // reference counting
    HPX_CXX_EXPORT template <typename Component, typename Derived>
    void intrusive_ptr_add_ref(
        managed_component<Component, Derived>* p) noexcept
    {
        detail_adl_barrier::manage_lifetime<
            traits::managed_component_dtor_policy_t<Component>>::addref(p
                ->component_);
    }

    HPX_CXX_EXPORT template <typename Component, typename Derived>
    void intrusive_ptr_release(
        managed_component<Component, Derived>* p) noexcept
    {
        detail_adl_barrier::manage_lifetime<
            traits::managed_component_dtor_policy_t<Component>>::release(p
                ->component_);
    }

    namespace detail {

        template <typename T>
        class fixed_wrapper_heap;
    }

    ///////////////////////////////////////////////////////////////////////////
    /// The managed_component template is used as an indirection layer
    /// for components allowing to gracefully handle the access to non-existing
    /// components.
    ///
    /// Additionally, it provides memory management capabilities for the
    /// wrapping instances, and it integrates the memory management with the
    /// AGAS service. Every instance of a managed_component gets assigned
    /// a global id.
    /// The provided memory management allocates the managed_component
    /// instances from a special heap, ensuring fast allocation and avoids a
    /// full network round trip to the AGAS service for each of the allocated
    /// instances.
    ///
    /// \tparam Component Component type
    /// \tparam Derived Most derived component type
    ///
    HPX_CXX_EXPORT template <typename Component, typename Derived>
    class managed_component
    {
    public:
        managed_component(managed_component const&) = delete;
        managed_component(managed_component&&) = delete;
        managed_component& operator=(managed_component const&) = delete;
        managed_component& operator=(managed_component&&) = delete;

        using derived_type = std::conditional_t<std::is_void_v<Derived>,
            managed_component, Derived>;

        using wrapped_type = Component;
        using type_holder = Component;
        using base_type_holder = typename Component::base_type_holder;

        using heap_type =
            detail::wrapper_heap_list<detail::fixed_wrapper_heap<derived_type>>;
        using value_type = derived_type;

        /// Construct a managed_component instance holding a
        /// wrapped instance. This constructor takes ownership of the
        /// passed pointer.
        ///
        /// \param comp [in] The pointer to the wrapped instance. The
        ///             managed_component takes ownership of this pointer.
        explicit managed_component(Component* comp)
          : component_(comp)
        {
            detail_adl_barrier::
                init<traits::managed_component_ctor_policy_t<Component>>::call(
                    component_, this);
            intrusive_ptr_add_ref(this);
        }

        // Construct a managed_component instance holding a new wrapped instance
        managed_component()
        {
            detail_adl_barrier::init<traits::managed_component_ctor_policy_t<
                Component>>::call_new(component_, this);
            intrusive_ptr_add_ref(this);
        }

        template <typename T, typename... Ts,
            typename Enable = std::enable_if_t<
                !std::is_same_v<std::decay_t<T>, managed_component>>>
        explicit managed_component(T&& t, Ts&&... ts)
        {
            detail_adl_barrier::init<traits::managed_component_ctor_policy_t<
                Component>>::call_new(component_, this, HPX_FORWARD(T, t),
                HPX_FORWARD(Ts, ts)...);
            intrusive_ptr_add_ref(this);
        }

        // The destructor releases any wrapped instances
        ~managed_component()
        {
            intrusive_ptr_release(this);
            detail_adl_barrier::manage_lifetime<
                traits::managed_component_dtor_policy_t<Component>>::
                call(component_);
        }

        //finalize() will be called just before the instance gets destructed
        static constexpr void finalize() noexcept {}

        /// \brief Return a pointer to the wrapped instance
        /// \note  Caller must check validity of returned pointer
        constexpr Component* get() noexcept
        {
            return component_;
        }
        constexpr Component const* get() const noexcept
        {
            return component_;
        }

        Component* get_checked()
        {
            if (!component_)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "managed_component<Component, Derived>::get_checked",
                    "component pointer ({}) is invalid (gid: {})",
                    components::get_component_type_name(
                        components::get_component_type<wrapped_type>()),
                    get_base_gid());
            }
            return get();
        }

        Component const* get_checked() const
        {
            if (!component_)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "managed_component<Component, Derived>::get_checked",
                    "component pointer ({}) is invalid (gid: {})",
                    components::get_component_type_name(
                        components::get_component_type<wrapped_type>()),
                    get_base_gid());
            }
            return get();
        }

        ///////////////////////////////////////////////////////////////////////
        // The managed_component behaves just like the wrapped object
        Component* operator->()
        {
            return get_checked();
        }

        Component const* operator->() const
        {
            return get_checked();
        }

        ///////////////////////////////////////////////////////////////////////
        Component& operator*()
        {
            return *get_checked();
        }

        Component const& operator*() const
        {
            return *get_checked();
        }

        ///////////////////////////////////////////////////////////////////////
        /// \brief Return the global id of this \a future instance
        hpx::id_type get_unmanaged_id() const
        {
            return hpx::id_type(
                get_base_gid(), hpx::id_type::management_type::unmanaged);
        }

        naming::gid_type get_base_gid(
            naming::gid_type const& assign_gid = naming::invalid_gid) const
        {
            if (assign_gid)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "managed_component::get_base_gid",
                    "managed_components must be assigned new gids on creation");
            }

            return component_heap<managed_component>().get_gid(
                const_cast<managed_component*>(this));
        }

        // reference counting
        template <typename C, typename D>
        friend void intrusive_ptr_add_ref(managed_component<C, D>* p) noexcept;

        template <typename C, typename D>
        friend void intrusive_ptr_release(managed_component<C, D>* p) noexcept;

    protected:
        Component* component_ = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Component, typename Wrapper, typename CtorPolicy,
        typename DtorPolicy>
    hpx::id_type managed_component_base<Component, Wrapper, CtorPolicy,
        DtorPolicy>::get_unmanaged_id() const
    {
        HPX_ASSERT(back_ptr_);
        return back_ptr_->get_unmanaged_id();
    }

    template <typename Component, typename Wrapper, typename CtorPolicy,
        typename DtorPolicy>
    hpx::id_type managed_component_base<Component, Wrapper, CtorPolicy,
        DtorPolicy>::get_id() const
    {
        // all credits should have been taken already
        naming::gid_type gid =
            static_cast<Component const&>(*this).get_base_gid();

        // The underlying heap will always give us a full set of credits, but
        // those are valid for the first invocation of get_base_gid() only. We
        // have to get rid of those credits and properly replenish those.
        naming::detail::strip_credits_from_gid(gid);

        // any invocation causes the credits to be replenished
        agas::replenish_credits(gid);
        return hpx::id_type(gid, hpx::id_type::management_type::managed);
    }

    template <typename Component, typename Wrapper, typename CtorPolicy,
        typename DtorPolicy>
    naming::gid_type managed_component_base<Component, Wrapper, CtorPolicy,
        DtorPolicy>::get_base_gid() const
    {
        HPX_ASSERT(back_ptr_);
        return back_ptr_->get_base_gid();
    }
}    // namespace hpx::components
