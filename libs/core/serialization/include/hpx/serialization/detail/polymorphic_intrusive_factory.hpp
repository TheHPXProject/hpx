//  Copyright (c) 2014 Anton Bikineev
//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/serialization/macros.hpp>
#include <hpx/serialization/serialization_fwd.hpp>

#include <functional>
#include <string>
#include <unordered_map>

namespace hpx::serialization::detail {

    HPX_CXX_CORE_EXPORT class polymorphic_intrusive_factory
    {
    public:
        polymorphic_intrusive_factory(
            polymorphic_intrusive_factory const&) = delete;
        polymorphic_intrusive_factory(polymorphic_intrusive_factory&&) = delete;
        polymorphic_intrusive_factory& operator=(
            polymorphic_intrusive_factory const&) = delete;
        polymorphic_intrusive_factory& operator=(
            polymorphic_intrusive_factory&&) = delete;

        ~polymorphic_intrusive_factory() = default;

    private:
        using ctor_type = void* (*) ();
        using ctor_map_type =
            std::unordered_map<std::string, ctor_type, std::hash<std::string>>;

    public:
        polymorphic_intrusive_factory() = default;

        HPX_CORE_EXPORT static polymorphic_intrusive_factory& instance();

        HPX_CORE_EXPORT void register_class(
            std::string const& name, ctor_type fun);

        [[nodiscard]] HPX_CORE_EXPORT void* create(
            std::string const& name) const;

        template <typename T>
        [[nodiscard]] T* create(std::string const& name) const
        {
            return static_cast<T*>(create(name));
        }

    private:
        ctor_map_type map_;
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct register_class_name
    {
        register_class_name()
        {
            polymorphic_intrusive_factory::instance().register_class(
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
}    // namespace hpx::serialization::detail
