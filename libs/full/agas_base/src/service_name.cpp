//  Copyright (c) 2026 Abhishek Kumar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/format.hpp>

#include <hpx/agas_base/agas_fwd.hpp>

#include <string>

namespace hpx::agas {

    std::string service_name_prefix()
    {
        return hpx::util::format(
            service_name, is_connecting() ? get_locality_id() : 0);
    }

    std::string service_instance_name(
        char const* servicename, char const* namespace_service_name)
    {
        std::string instance_name = service_name_prefix();
        instance_name += servicename;
        instance_name += namespace_service_name;
        return instance_name;
    }
}    // namespace hpx::agas
