//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2012-2017 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_distributed/base_lco.hpp>
#include <hpx/async_distributed/lcos_fwd.hpp>
#include <hpx/async_distributed/transfer_continuation_action.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::lcos {

    namespace detail {

        /// \cond NOINTERNAL
        template <typename ComponentTag, typename BaseLco>
        struct base_lco_wrapping_type;

        template <typename BaseLco>
        struct base_lco_wrapping_type<traits::detail::component_tag, BaseLco>
        {
            using type = components::component<BaseLco>;
        };

        template <typename BaseLco>
        struct base_lco_wrapping_type<traits::detail::managed_component_tag,
            BaseLco>
        {
            using type = components::managed_component<BaseLco>;
        };
        /// \endcond
    }    // namespace detail

    /// The \a base_lco_with_value class is the common base class for all LCO's
    /// synchronizing on a value.
    /// The \a RemoteResult template argument should be set to the type of the
    /// argument expected for the set_value action.
    ///
    /// \tparam RemoteResult The type of the result value to be carried back
    ///                      to the LCO instance.
    /// \tparam ComponentTag The tag type representing the type of the component
    ///                      (either component_tag or managed_component_tag).
    HPX_CXX_EXPORT template <typename Result, typename RemoteResult,
        typename ComponentTag>
    class base_lco_with_value
      : public base_lco
      , public ComponentTag
    {
    protected:
        using result_type = std::conditional_t<std::is_void_v<Result>,
            util::unused_type, Result>;

        /// Destructor, needs to be virtual to allow for clean destruction of
        /// derived objects
        ~base_lco_with_value() override = default;

        void set_event() override
        {
            if constexpr (std::is_default_constructible_v<RemoteResult>)
            {
                set_value(RemoteResult());
            }
            else
            {
                // this shouldn't ever be called
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "base_lco_with_value::set_event_nonvirt",
                    "attempt to use a non-default-constructible return type "
                    "with an action in a context where default-construction "
                    "would be required");
            }
        }

        virtual void set_value(RemoteResult&& result) = 0;

        virtual result_type get_value() = 0;
        virtual result_type get_value(error_code& /*ec*/)
        {
            return get_value();
        }

    public:
        // components must contain a typedef for wrapping_type defining the
        // managed_component type used to encapsulate instances of this
        // component
        using wrapping_type = detail::base_lco_wrapping_type<ComponentTag,
            base_lco_with_value>::type;

        using base_type_holder = base_lco_with_value;

        static components::component_type get_component_type() noexcept
        {
            return components::get_component_type<base_lco_with_value>();
        }
        static void set_component_type(components::component_type type)
        {
            components::set_component_type<base_lco_with_value>(type);
        }

        /// The \a function set_value_nonvirt is called whenever a
        /// \a set_value_action is applied on this LCO instance. This
        /// function just forwards to the virtual function \a set_value, which
        /// is overloaded by the derived concrete LCO.
        ///
        /// \param result [in] The result value to be transferred from the
        ///               remote operation back to this LCO instance.

        void set_value_nonvirt(RemoteResult&& result)
        {
            set_value(HPX_MOVE(result));
        }

        /// The \a function get_result_nonvirt is called whenever a
        /// \a get_result_action is applied on this LCO instance. This
        /// function just forwards to the virtual function \a get_result, which
        /// is overloaded by the derived concrete LCO.

        Result get_value_nonvirt()
        {
            // Use of the comma-operator in a tested expression causes the left
            // argument to be ignored when it has no side effects.
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 6319)
#endif

            return util::void_guard<Result>(), get_value();

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
        }

    public:
        /// The \a set_value_action may be used to trigger any LCO instances
        /// while carrying an additional parameter of any type.
        ///
        /// RemoteResult is taken by rvalue ref. This allows for perfect forwarding.
        /// When the action thread function is created, the values are moved into
        /// the called function. If we took it by const lvalue reference, we
        /// would disable the possibility to further move the result to the
        /// designated destination.
        ///
        /// \param RemoteResult [in] The type of the result to be transferred
        ///               back to this LCO instance.
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(
            base_lco_with_value, set_value_nonvirt, set_value_action)

        /// The \a get_value_action may be used to query the value this LCO
        /// instance exposes as its 'result' value.
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(
            base_lco_with_value, get_value_nonvirt, get_value_action)
    };

    /// The base_lco<void> specialization is used whenever the set_event action
    /// for a particular LCO doesn't carry any argument.
    ///
    /// \tparam void This specialization expects no result value and is almost
    ///              completely equivalent to the plain \a base_lco.
    template <typename ComponentTag>
    class base_lco_with_value<void, void, ComponentTag>
      : public base_lco
      , public ComponentTag
    {
    protected:
        /// Destructor, needs to be virtual to allow for clean destruction of
        /// derived objects
        ~base_lco_with_value() override = default;

    public:
        // components must contain a typedef for wrapping_type defining the
        // managed_component type used to encapsulate instances of this
        // component
        using wrapping_type = detail::base_lco_wrapping_type<ComponentTag,
            base_lco_with_value>::type;
        using base_type_holder = base_lco_with_value;

#if !defined(HPX_COMPUTE_DEVICE_CODE)
        // refer to base type for the corresponding implementation
        using set_value_action = base_lco::set_event_action;
#endif

        // dummy action definition
        void get_value() {}

        HPX_DEFINE_COMPONENT_DIRECT_ACTION(
            base_lco_with_value, get_value, get_value_action)
    };
}    // namespace hpx::lcos

namespace hpx::traits {

    // define component type data base entry generator
    template <typename Result, typename RemoteResult, typename Enable>
    struct component_type_database<
        hpx::lcos::base_lco_with_value<Result, RemoteResult,
            traits::detail::managed_component_tag>,
        Enable>
    {
        static constexpr components::component_type get() noexcept
        {
            return to_int(
                hpx::components::component_enum_type::base_lco_with_value);
        }

        static void set(components::component_type)
        {
            HPX_ASSERT(false);
        }
    };

    template <typename Result, typename RemoteResult, typename Enable>
    struct component_type_database<
        hpx::lcos::base_lco_with_value<Result, RemoteResult,
            traits::detail::component_tag>,
        Enable>
    {
        static constexpr components::component_type get() noexcept
        {
            return to_int(hpx::components::component_enum_type::
                    base_lco_with_value_unmanaged);
        }

        static void set(components::component_type)
        {
            HPX_ASSERT(false);
        }
    };
}    // namespace hpx::traits

namespace hpx::components::detail {

    template <typename Result, typename RemoteResult>
    struct HPX_ALWAYS_EXPORT component_heap_impl<
        hpx::components::managed_component<hpx::lcos::base_lco_with_value<
            Result, RemoteResult, traits::detail::managed_component_tag>>>
    {
        using valid = void;
        using component_type =
            hpx::components::managed_component<hpx::lcos::base_lco_with_value<
                Result, RemoteResult, traits::detail::managed_component_tag>>;

        static component_type::heap_type& call()
        {
            util::reinitializable_static<typename component_type::heap_type>
                heap;
            return heap.get();
        }
    };

    template <typename Result, typename RemoteResult>
    struct HPX_ALWAYS_EXPORT component_heap_impl<
        hpx::components::managed_component<hpx::lcos::base_lco_with_value<
            Result, RemoteResult, traits::detail::component_tag>>>
    {
        using valid = void;
        using component_type =
            hpx::components::managed_component<hpx::lcos::base_lco_with_value<
                Result, RemoteResult, traits::detail::component_tag>>;

        static component_type::heap_type& call()
        {
            util::reinitializable_static<typename component_type::heap_type>
                heap;
            return heap.get();
        }
    };
}    // namespace hpx::components::detail

#if !defined(HPX_HAVE_STATIC_LINKING)

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(hpx::naming::gid_type, gid_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    std::vector<hpx::naming::gid_type>, vector_gid_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    hpx::id_type, hpx::naming::gid_type, id_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(hpx::id_type, naming_id_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::vector<hpx::id_type>,
    std::vector<hpx::naming::gid_type>, vector_id_gid_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    std::vector<hpx::id_type>, vector_id_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    hpx::util::unused_type, hpx_unused_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(float)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(double)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::int8_t, int8_t)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::uint8_t, uint8_t)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::int16_t, int16_t)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::uint16_t, uint16_t)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::int32_t, int32_t)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::uint32_t, uint32_t)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::int64_t, int64_t)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::uint64_t, uint64_t)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(bool)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    std::vector<bool>, vector_bool_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    std::vector<std::uint32_t>, vector_std_uint32_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(hpx::util::section, hpx_section)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(std::string, std_string)

#endif
