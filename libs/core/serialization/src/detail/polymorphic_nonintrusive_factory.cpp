//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2015 Anton Bikineev
//  Copyright (c) 2015 Andreas Schaefer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/errors.hpp>
#include <hpx/serialization/detail/polymorphic_nonintrusive_factory.hpp>
#include <hpx/serialization/serialize.hpp>
#include <hpx/serialization/string.hpp>

#include <string>
#include <typeinfo>

namespace hpx::serialization::detail {

    polymorphic_nonintrusive_factory&
    polymorphic_nonintrusive_factory::instance()
    {
        static polymorphic_nonintrusive_factory factory;
        return factory;
    }

    void polymorphic_nonintrusive_factory::register_class(
        std::type_info const& typeinfo, std::string const& class_name,
        function_bunch_type const& bunch)
    {
        if (!typeinfo.name() && std::string(typeinfo.name()).empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "polymorphic_nonintrusive_factory::register_class",
                "Cannot register a factory with an empty type name");
        }
        if (class_name.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "polymorphic_nonintrusive_factory::register_class",
                "Cannot register a factory with an empty name");
        }

        if (auto const it = map_.find(class_name); it == map_.end())
        {
            map_[class_name] = bunch;
        }

        if (auto const jt = typeinfo_map_.find(typeinfo.name());
            jt == typeinfo_map_.end())
        {
            typeinfo_map_[typeinfo.name()] = class_name;
        }
    }

    void* polymorphic_nonintrusive_factory::load_create(
        input_archive& ar, std::string const&) const
    {
        std::string class_name;
        ar >> class_name;

        function_bunch_type const& bunch = map_.at(class_name);
        return bunch.create_function(ar);
    }

    void polymorphic_nonintrusive_factory::load_void(input_archive& ar,
        [[maybe_unused]] std::string const& name, void* p) const
    {
        std::string class_name;
        ar >> class_name;

        if (std::string const expected_class_name = typeinfo_map_.at(name);
            class_name != expected_class_name)
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "polymorphic_nonintrusive_factory::register_class",
                "Unexpected (non-matching) type received (actual: {}, "
                "expected: {})",
                class_name, expected_class_name);
        }

        map_.at(class_name).load_function(ar, p);
    }

    void polymorphic_nonintrusive_factory::save_void(
        output_archive& ar, std::string const& name, void const* p) const
    {
        std::string const class_name = typeinfo_map_.at(name);
        ar << class_name;

        map_.at(class_name).save_function(ar, p);
    }
}    // namespace hpx::serialization::detail
