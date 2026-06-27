//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components/client_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstdint>
#include <string>
#include <utility>

namespace hpx::util {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    extra_data_id_type
    extra_data_helper<lcos::detail::registered_name_tracker>::id() noexcept
    {
        static std::uint8_t id = 0;
        return &id;
    }

    void extra_data_helper<lcos::detail::registered_name_tracker>::reset(
        lcos::detail::registered_name_tracker* registered_name) noexcept
    {
        if (registered_name != nullptr && !registered_name->empty())
        {
            std::string name;
            std::swap(name, *registered_name);

            error_code ec(throwmode::lightweight);
            agas::unregister_name(launch::sync, name, ec);
        }
    }
}    // namespace hpx::util

namespace hpx::lcos::detail {

    void future_data<hpx::id_type>::tidy() const noexcept
    {
        hpx::util::extra_data_helper<registered_name_tracker>::reset(
            try_get_extra_data<registered_name_tracker>());
    }

    std::string const& future_data<hpx::id_type>::get_registered_name()
        const noexcept
    {
        if (auto const* registered_name =
                try_get_extra_data<registered_name_tracker>())
        {
            return *registered_name;
        }

        static std::string empty_string;
        return empty_string;
    }

    void future_data<hpx::id_type>::set_registered_name(std::string name)
    {
        auto& registered_name = get_extra_data<registered_name_tracker>();
        registered_name = HPX_MOVE(name);
    }

    bool future_data<hpx::id_type>::register_as(
        std::string name, bool manage_lifetime)
    {
        auto& registered_name = get_extra_data<registered_name_tracker>();

        HPX_ASSERT(registered_name.empty());    // call only once
        registered_name = HPX_MOVE(name);

        hpx::id_type id = *this->get_result();
        if (!manage_lifetime)
        {
            id = hpx::unmanaged(id);
        }
        return hpx::agas::register_name(launch::sync, registered_name, id);
    }
}    // namespace hpx::lcos::detail
