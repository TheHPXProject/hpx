//  Copyright (c)      2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/actions_base/actions_base_fwd.hpp>
#include <hpx/actions_base/actions_base_support.hpp>
#include <hpx/actions_base/macros.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hpx::actions::detail {

    HPX_CXX_EXPORT struct action_registry
    {
        action_registry(action_registry const&) = delete;
        action_registry(action_registry&&) = delete;
        action_registry& operator=(action_registry const&) = delete;
        action_registry& operator=(action_registry&&) = delete;

        using ctor_t = base_action* (*) ();
        using typename_to_ctor_t =
            std::unordered_map<std::string, std::pair<ctor_t, ctor_t>>;
        using typename_to_id_t = std::unordered_map<std::string, std::uint32_t>;
        using cache_t = std::vector<std::pair<ctor_t, ctor_t>>;

        static constexpr std::uint32_t invalid_id = ~0;

        HPX_EXPORT action_registry();
        HPX_EXPORT ~action_registry();

        HPX_EXPORT void register_factory(
            std::string const& type_name, ctor_t ctor, ctor_t ctor_cont);
        HPX_EXPORT void register_typename(
            std::string const& type_name, std::uint32_t id);
        HPX_EXPORT void fill_missing_typenames();
        HPX_EXPORT std::uint32_t try_get_id(std::string const& type_name) const;
        HPX_EXPORT std::vector<std::string> get_unassigned_typenames() const;

        HPX_EXPORT static std::uint32_t get_id(std::string const& type_name);
        HPX_EXPORT static base_action* create(
            std::uint32_t id, bool, std::string const* name = nullptr);

        HPX_EXPORT static action_registry& instance();

        void cache_id(std::uint32_t id, ctor_t ctor, ctor_t ctor_cont);
        std::string collect_registered_typenames() const;

        std::uint32_t max_id_;
        typename_to_ctor_t typename_to_ctor_;
        typename_to_id_t typename_to_id_;
        cache_t cache_;
    };

    HPX_CXX_EXPORT template <std::uint32_t Id>
    HPX_ALWAYS_EXPORT std::string get_action_name_id();

    HPX_CXX_EXPORT template <std::uint32_t Id>
    struct add_constant_entry
    {
        add_constant_entry(add_constant_entry const&) = delete;
        add_constant_entry(add_constant_entry&&) = delete;
        add_constant_entry& operator=(add_constant_entry const&) = delete;
        add_constant_entry& operator=(add_constant_entry&&) = delete;

        add_constant_entry();
        ~add_constant_entry() = default;

        static add_constant_entry instance;
    };

    template <std::uint32_t Id>
    add_constant_entry<Id> add_constant_entry<Id>::instance;

    template <std::uint32_t Id>
    add_constant_entry<Id>::add_constant_entry()
    {
        action_registry::instance().register_typename(
            get_action_name_id<Id>(), Id);
    }
}    // namespace hpx::actions::detail

#endif
