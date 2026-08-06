//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/supervision_dispatch/shadow_id.hpp>

#include <ostream>

namespace hpx::supervision {

    std::ostream& operator<<(std::ostream& strm, shadow_id const& id)
    {
        return strm << id.get();
    }

    std::ostream& operator<<(std::ostream& strm, joined_peer const& peer)
    {
        return strm << peer.shadow  << " (" << peer.target << ")";
    }
}    // namespace hpx::supervision
