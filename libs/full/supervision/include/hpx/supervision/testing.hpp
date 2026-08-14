//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::supervision::testing::set_register_observer_snapshot_hook
/// \file hpx/supervision/testing.hpp
/// \headerfile hpx/supervision.hpp

#pragma once

#include <hpx/config.hpp>

#include <functional>

namespace hpx::supervision::testing {

    /// Set the hook invoked after an observer snapshot is captured.
    ///
    /// \param hook Hook to invoke. An empty function clears the hook.
    ///
    /// \note Used for testing purposes only.
    HPX_CXX_EXPORT HPX_EXPORT void set_register_observer_snapshot_hook(
        std::function<void()> hook);
}    // namespace hpx::supervision::testing
