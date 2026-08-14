//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>

#include <hpx/runtime_components/macros.hpp>

#include <type_traits>

namespace hpx::components::server {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename ConfigData, typename Derived = void>
    class distributed_metadata_base
      : public hpx::components::component_base<
            std::conditional_t<std::is_void_v<Derived>,
                distributed_metadata_base<ConfigData, Derived>, Derived>>
    {
    public:
        distributed_metadata_base()
        {
            HPX_ASSERT(false);
        }

        explicit distributed_metadata_base(ConfigData const& data)
          : data_(data)
        {
        }

        // Retrieve the configuration data.
        ConfigData get() const
        {
            return data_;
        }

        HPX_DEFINE_COMPONENT_DIRECT_ACTION(distributed_metadata_base, get)

    private:
        ConfigData data_;
    };
}    // namespace hpx::components::server
