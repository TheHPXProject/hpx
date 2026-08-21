//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/modules/async_distributed.hpp>

#include <hpx/agas_base/detail/hosted_component_namespace.hpp>
#include <hpx/agas_base/server/component_namespace.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace hpx::agas::detail {

    hosted_component_namespace::hosted_component_namespace(
        naming::address const& addr)
      : gid_(naming::gid_type(agas::component_ns_msb, agas::component_ns_lsb),
            hpx::id_type::management_type::unmanaged)
      , addr_(addr)
    {
    }

    components::component_type hosted_component_namespace::bind_prefix(
        [[maybe_unused]] std::string const& key,
        [[maybe_unused]] std::uint32_t prefix)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        constexpr server::component_namespace::bind_prefix_action action;
        return hpx::wait_or_handle_timeout(
            hpx::async(action, gid_, key, prefix),
            "hosted_component_namespace::bind_prefix");
#else
        HPX_ASSERT(false);
        return components::component_type{};
#endif
    }

    components::component_type hosted_component_namespace::bind_name(
        [[maybe_unused]] std::string const& name)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        constexpr server::component_namespace::bind_name_action action;
        return hpx::wait_or_handle_timeout(hpx::async(action, gid_, name),
            "hosted_component_namespace::bind_name");
#else
        HPX_ASSERT(false);
        return components::component_type{};
#endif
    }

    std::vector<std::uint32_t> hosted_component_namespace::resolve_id(
        [[maybe_unused]] components::component_type key)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        constexpr server::component_namespace::resolve_id_action action;
        return hpx::wait_or_handle_timeout(hpx::async(action, gid_, key),
            "hosted_component_namespace::resolve_id");
#else
        HPX_ASSERT(false);
        return std::vector<std::uint32_t>{1, std::uint32_t(0)};
#endif
    }

    bool hosted_component_namespace::unbind(
        [[maybe_unused]] std::string const& key)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        constexpr server::component_namespace::unbind_action action;
        return hpx::wait_or_handle_timeout(hpx::async(action, gid_, key),
            "hosted_component_namespace::unbind");
#else
        HPX_ASSERT(false);
        return true;
#endif
    }

    void hosted_component_namespace::iterate_types(
        [[maybe_unused]] iterate_types_function_type const& f)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        constexpr server::component_namespace::iterate_types_action action;
        return hpx::wait_or_handle_timeout(hpx::async(action, gid_, f),
            "hosted_component_namespace::iterate_types");
#else
        HPX_ASSERT(false);
#endif
    }

    std::string hosted_component_namespace::get_component_type_name(
        [[maybe_unused]] components::component_type type)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        constexpr server::component_namespace::get_component_type_name_action
            action;
        return hpx::wait_or_handle_timeout(hpx::async(action, gid_, type),
            "hosted_component_namespace::get_component_type_name");
#else
        HPX_ASSERT(false);
        return std::string{};
#endif
    }

    hpx::future<std::uint32_t> hosted_component_namespace::get_num_localities(
        [[maybe_unused]] components::component_type type)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        server::component_namespace::get_num_localities_action action;
        return hpx::async(action, gid_, type);
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(std::uint32_t(1));
#endif
    }
}    // namespace hpx::agas::detail

#endif
