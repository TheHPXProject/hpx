//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/serialization/detail/polymorphic_intrusive_factory.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/traits/polymorphic_traits.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::serialization::detail {

    HPX_CXX_CORE_EXPORT class id_registry
    {
    public:
        id_registry(id_registry const&) = delete;
        id_registry(id_registry&&) = delete;
        id_registry& operator=(id_registry const&) = delete;
        id_registry& operator=(id_registry&&) = delete;

        ~id_registry() = default;

        using ctor_t = void* (*) ();
        using typename_to_ctor_t = std::map<std::string, ctor_t>;
        using typename_to_id_t = std::map<std::string, std::uint32_t>;
        using cache_t = std::vector<ctor_t>;

        static constexpr std::uint32_t invalid_id = ~0u;

        HPX_CORE_EXPORT void register_factory_function(
            std::string const& type_name, ctor_t ctor);

        HPX_CORE_EXPORT void register_typename(
            std::string const& type_name, std::uint32_t id);

        HPX_CORE_EXPORT void fill_missing_typenames();

        [[nodiscard]] HPX_CORE_EXPORT std::uint32_t try_get_id(
            std::string const& type_name) const;

        [[nodiscard]] constexpr std::uint32_t get_max_registered_id()
            const noexcept
        {
            return max_id;
        }

        [[nodiscard]] HPX_CORE_EXPORT std::vector<std::string>
        get_unassigned_typenames() const;

        HPX_CORE_EXPORT static id_registry& instance();

    private:
        id_registry() noexcept
          : max_id(0u)
        {
        }

        friend struct ::hpx::util::static_<id_registry>;
        friend class polymorphic_id_factory;

        HPX_CORE_EXPORT void cache_id(std::uint32_t id, ctor_t ctor);

        std::uint32_t max_id;
        typename_to_ctor_t typename_to_ctor;
        typename_to_id_t typename_to_id;
        cache_t cache;
    };

    HPX_CXX_CORE_EXPORT class polymorphic_id_factory
    {
    public:
        polymorphic_id_factory(polymorphic_id_factory const&) = delete;
        polymorphic_id_factory(polymorphic_id_factory&&) = delete;
        polymorphic_id_factory& operator=(
            polymorphic_id_factory const&) = delete;
        polymorphic_id_factory& operator=(polymorphic_id_factory&&) = delete;

        ~polymorphic_id_factory() = default;

    private:
        using ctor_t = id_registry::ctor_t;
        using typename_to_ctor_t = id_registry::typename_to_ctor_t;
        using typename_to_id_t = id_registry::typename_to_id_t;
        using cache_t = id_registry::cache_t;

    public:
        template <class T>
        [[nodiscard]] static T* create(
            std::uint32_t const id, std::string const* name = nullptr)
        {
            return static_cast<T*>(get_ctor_function(id, name));
        }

        [[nodiscard]] HPX_CORE_EXPORT static std::uint32_t get_id(
            std::string const& type_name);

    private:
        polymorphic_id_factory() = default;

        HPX_CORE_EXPORT static polymorphic_id_factory& instance();
        [[nodiscard]] HPX_CORE_EXPORT static ctor_t get_ctor_function(
            std::uint32_t id, std::string const* name);

        [[nodiscard]] HPX_CORE_EXPORT static std::string
        collect_registered_typenames();

        friend struct hpx::util::static_<polymorphic_id_factory>;
    };

    template <typename T>
    struct register_class_name<T,
        std::enable_if_t<traits::is_serialized_with_id_v<T>>>
    {
        register_class_name()
        {
            id_registry::instance().register_factory_function(
                T::hpx_serialization_get_name_impl(), &factory_function);
        }

        [[nodiscard]] static void* factory_function()
        {
            return new T;
        }

        static register_class_name& instance()
        {
            static register_class_name instance_;
            return instance_;
        }
    };

    HPX_CXX_CORE_EXPORT template <std::uint32_t desc>
    [[nodiscard]] std::string get_constant_entry_name();

    HPX_CXX_CORE_EXPORT template <std::uint32_t Id>
    struct add_constant_entry
    {
        add_constant_entry()
        {
            id_registry::instance().register_typename(
                get_constant_entry_name<Id>(), Id);
        }

        static add_constant_entry& instance()
        {
            static add_constant_entry instance_;
            return instance_;
        }
    };
}    // namespace hpx::serialization::detail

#include <hpx/config/warnings_suffix.hpp>
