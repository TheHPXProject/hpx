//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2015 Andreas Schaefer
//  Copyright (c) 2022-2025 Hartmut Kaiser
//  Copyright (c) 2026 Ujjwal Shekhar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/serialization/detail/non_default_constructible.hpp>
#include <hpx/serialization/macros.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/traits/needs_automatic_registration.hpp>
#include <hpx/serialization/traits/polymorphic_traits.hpp>

#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>

#include <hpx/config/warnings_prefix.hpp>

#if defined(HPX_HAVE_CXX26_REFLECTION)
#include <hpx/serialization/detail/refl_qualified_name_of.hpp>
#endif

namespace hpx::serialization::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct get_serialization_name
#ifdef HPX_DISABLE_AUTOMATIC_SERIALIZATION_REGISTRATION
        ;
#else
    {

#if defined(HPX_HAVE_CXX26_REFLECTION)
        [[nodiscard]] constexpr char const* operator()() const noexcept
        {
            // Unless the user specifically creates a specialization of this
            // we should be able to auto generate a full string description
            // for the type T using reflection and display_string_of()

            return qualified_name_of<T>::get();
        }
#else
        [[nodiscard]] char const* operator()() const noexcept
        {
            // If you encounter this assert while compiling code, that means
            // that you have a HPX_REGISTER_ACTION macro somewhere in a source
            // file, but the header in which the action is defined misses a
            // HPX_REGISTER_ACTION_DECLARATION
            static_assert(traits::needs_automatic_registration_v<T>,
                "HPX_REGISTER_ACTION_DECLARATION missing");
            return util::debug::type_id<T>();
        }
#endif
    };
#endif

    struct function_bunch_type
    {
        using save_function_type = void (*)(output_archive&, void const* base);
        using load_function_type = void (*)(input_archive&, void* base);
        using create_function_type = void* (*) (input_archive&);

        save_function_type save_function;
        load_function_type load_function;
        create_function_type create_function;
    };

    template <typename T>
    class constructor_selector_ptr
    {
    public:
        [[nodiscard]] static T* create(input_archive& ar);
    };

    HPX_CXX_CORE_EXPORT class polymorphic_nonintrusive_factory
    {
    public:
        polymorphic_nonintrusive_factory(
            polymorphic_nonintrusive_factory const&) = delete;
        polymorphic_nonintrusive_factory(
            polymorphic_nonintrusive_factory&&) = delete;
        polymorphic_nonintrusive_factory& operator=(
            polymorphic_nonintrusive_factory const&) = delete;
        polymorphic_nonintrusive_factory& operator=(
            polymorphic_nonintrusive_factory&&) = delete;

        ~polymorphic_nonintrusive_factory() = default;

        using serializer_map_type = std::unordered_map<std::string,
            function_bunch_type, std::hash<std::string>>;
        using serializer_typeinfo_map_type = std::unordered_map<std::string,
            std::string, std::hash<std::string>>;

        HPX_CORE_EXPORT static polymorphic_nonintrusive_factory& instance();

        HPX_CORE_EXPORT void register_class(std::type_info const& typeinfo,
            std::string const& class_name, function_bunch_type const& bunch);

        // the following templates are defined in *.ipp file
        template <typename T>
        void save(output_archive& ar, T const& t)
        {
            // It's safe to call typeid here. The typeid(t) return value is only
            // used for local lookup to the portable string that goes over the
            // wire
            save_void(ar, typeid(t).name(), &t);
        }

        template <typename T>
        void load(input_archive& ar, T& t)
        {
            load_void(ar, typeid(t).name(), &t);
        }

        // use raw pointer to construct either shared_ptr or intrusive_ptr from
        // it
        template <typename T>
        [[nodiscard]] T* load(input_archive& ar)
        {
            return static_cast<T*>(load_create(ar, typeid(T).name()));
        }

    private:
        HPX_CORE_EXPORT void save_void(
            output_archive& ar, std::string const&, void const*) const;
        HPX_CORE_EXPORT void load_void(
            input_archive& ar, std::string const&, void*) const;
        HPX_CORE_EXPORT void* load_create(
            input_archive& ar, std::string const&) const;

        polymorphic_nonintrusive_factory() = default;

        serializer_map_type map_;
        serializer_typeinfo_map_type typeinfo_map_;
    };

    HPX_CXX_CORE_EXPORT template <typename Derived>
    struct register_class
    {
        static void save(output_archive& ar, void const* base);
        static void load(input_archive& ar, void* base);

        // this function is needed for pointer type serialization
        [[nodiscard]] static void* create(input_archive& ar)
        {
            return constructor_selector_ptr<Derived>::create(ar);
        }

        register_class()
        {
            static constexpr function_bunch_type bunch = {
                &register_class<Derived>::save, &register_class<Derived>::load,
                &register_class<Derived>::create};

            // It's safe to call typeid here. The typeid(t) return value is only
            // used for local lookup to the portable string that goes over the
            // wire
            polymorphic_nonintrusive_factory::instance().register_class(
                typeid(Derived), get_serialization_name<Derived>()(), bunch);
        }

        static register_class& instance()
        {
            static register_class instance_;
            return instance_;
        }
    };
}    // namespace hpx::serialization::detail

#include <hpx/config/warnings_suffix.hpp>
