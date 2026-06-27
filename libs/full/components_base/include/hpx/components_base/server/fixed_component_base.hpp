//  Copyright (c) 2011      Bryce Lelbach
//  Copyright (c) 2007-2026 Hartmut Kaiser
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
#include <hpx/components_base/traits/is_component.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/naming_base.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
    class fixed_component_base : public traits::detail::fixed_component_tag
    {
        using this_component_type =
            std::conditional_t<std::is_void_v<Component>, fixed_component_base,
                Component>;

        constexpr Component& derived() noexcept
        {
            return static_cast<Component&>(*this);
        }
        constexpr Component const& derived() const noexcept
        {
            return static_cast<Component const&>(*this);
        }

    public:
        using wrapped_type = this_component_type;
        using base_type_holder = this_component_type;
        using wrapping_type = fixed_component<this_component_type>;

        // Construct an empty fixed_component
        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        constexpr fixed_component_base(
            std::uint64_t msb, std::uint64_t lsb) noexcept
          : msb_(msb)
          , lsb_(lsb)
        {
        }

        ~fixed_component_base() = default;

        /// \brief finalize() will be called just before the instance gets
        ///        destructed
        void finalize() const
        {
            /// Unbind the GID if it's not this instantiations fixed gid and is
            /// is not invalid.
            if (naming::invalid_gid != gid_)
            {
                error_code ec(throwmode::lightweight);    // ignore errors
                agas::unbind_gid_local(gid_, ec);

                gid_ = naming::gid_type();    // invalidate GID
            }
        }

        // Return the component's fixed GID.
        naming::gid_type get_base_gid(
            naming::gid_type const& assign_gid = naming::invalid_gid) const
        {
            if (assign_gid)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "fixed_component_base::get_base_gid",
                    "fixed_components must be assigned new gids on creation");
            }

            if (!gid_)
            {
                naming::address const addr(
                    naming::get_gid_from_locality_id(agas::get_locality_id()),
                    components::get_component_type<wrapped_type>(),
                    const_cast<this_component_type*>(
                        static_cast<this_component_type const*>(this)));

                gid_ = naming::gid_type(msb_, lsb_);

                // Try to bind the preset GID first
                if (!agas::bind_gid_local(gid_, addr))
                {
                    naming::gid_type const g = gid_;
                    gid_ = naming::gid_type();    // invalidate GID

                    HPX_THROW_EXCEPTION(hpx::error::duplicate_component_address,
                        "fixed_component_base<Component>::get_base_gid",
                        "could not bind_gid(local): {}", g);
                }
            }
            return gid_;
        }

        hpx::id_type get_id() const
        {
            // fixed_address components are created without any credits
            naming::gid_type gid = derived().get_base_gid();
            HPX_ASSERT(!naming::detail::has_credits(gid));

            agas::replenish_credits(gid);
            return hpx::id_type(gid, hpx::id_type::management_type::managed);
        }

        hpx::id_type get_unmanaged_id() const
        {
            return hpx::id_type(derived().get_base_gid(),
                hpx::id_type::management_type::unmanaged);
        }

        void set_locality_id(std::uint32_t locality_id, error_code& ec = throws)
        {
            if (gid_)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status,
                    "fixed_component_base::set_locality_id",
                    "can't change locality_id after GID has already been "
                    "registered");
            }
            else
            {
                // erase current locality_id and replace with given one
                msb_ = naming::replace_locality_id(msb_, locality_id);
            }
        }

        static void mark_as_migrated() noexcept
        {
            // If this assertion is triggered then this component instance is
            // being migrated even if the component type has not been enabled to
            // support migration.
            HPX_ASSERT(false);
        }

        static void on_migrated() noexcept
        {
            // If this assertion is triggered then this component instance is
            // being migrated even if the component type has not been enabled to
            // support migration.
            HPX_ASSERT(false);
        }

    private:
        mutable naming::gid_type gid_;
        std::uint64_t msb_;
        std::uint64_t lsb_;
    };

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        struct fixed_heap
        {
            static void* alloc(std::size_t) noexcept
            {
                HPX_ASSERT(false);    // this shouldn't ever be called
                return nullptr;
            }
            static void free(void*, std::size_t) noexcept
            {
                HPX_ASSERT(false);    // this shouldn't ever be called
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component>
    class fixed_component : public Component
    {
    public:
        using type_holder = Component;
        using component_type = fixed_component<Component>;
        using derived_type = component_type;
        using heap_type = detail::fixed_heap;

        /// \brief  The function \a create is used for allocation and
        ///         initialization of instances of the derived components.
        static Component* create(std::size_t /* count */) noexcept
        {
            HPX_ASSERT(false);    // this shouldn't ever be called
            return nullptr;
        }

        /// \brief  The function \a destroy is used for destruction and
        ///         de-allocation of instances of the derived components.
        static void destroy(
            Component* /* p */, std::size_t /* count */ = 1) noexcept
        {
            HPX_ASSERT(false);    // this shouldn't ever be called
        }
    };
}    // namespace hpx::components
