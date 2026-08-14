// Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/components/process/util/windows/initializers/initializer_base.hpp>

#include <string>

namespace hpx::components::process::windows::initializers {

    class wait_on_latch : public initializer_base
    {
    public:
        wait_on_latch() = default;

        explicit wait_on_latch(std::string const& connect_to)
          : connect_to_(connect_to)
        {
        }

        template <typename WindowsExecutor>
        void on_CreateProcess_success(WindowsExecutor&) const
        {
            // wait for the newly launched HPX locality to connect back here
            hpx::distributed::latch l(2);
            l.register_as(connect_to_);
            l.arrive_and_wait();
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & connect_to_;
        }

        std::string connect_to_;
    };
}    // namespace hpx::components::process::windows::initializers

#endif
