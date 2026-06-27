//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/executors/fork_join_executor.hpp>

#include <cstdint>
#include <ostream>
#include <type_traits>

namespace hpx::execution::experimental {

    std::ostream& operator<<(
        std::ostream& os, fork_join_executor::loop_schedule schedule)
    {
        switch (schedule)
        {
        case fork_join_executor::loop_schedule::static_:
            os << "static";
            break;
        case fork_join_executor::loop_schedule::dynamic:
            os << "dynamic";
            break;
        default:
            os << "<unknown>";
            break;
        }

        os << " ("
           << static_cast<std::uint16_t>(static_cast<
                  std::underlying_type_t<fork_join_executor::loop_schedule>>(
                  schedule))
           << ")";

        return os;
    }
}    // namespace hpx::execution::experimental
