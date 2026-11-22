//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

namespace hpx::serialization::detail {

    /// Attached to an output archive that saves without the preprocessing
    /// pass. A save function that needs that pass (an id_type that is not
    /// unmanaged, a future or task_group that is not ready) sets needs_await
    /// and saves nothing, with no side effect. The code that attached it
    /// reads needs_await, and if it is set, discards the output and the
    /// archive and runs the preprocessing pass. The marker cannot be
    /// removed, and reset() of the archive clears needs_await, so the
    /// archive must not be used for another pass.
    HPX_CXX_CORE_EXPORT struct optimistic_save
    {
        bool needs_await = false;
    };
}    // namespace hpx::serialization::detail

// This is explicitly instantiated to ensure that the id is stable across shared
// libraries.
template <>
struct hpx::util::extra_data_helper<hpx::serialization::detail::optimistic_save>
{
    HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
    static constexpr void reset(
        serialization::detail::optimistic_save* data) noexcept
    {
        data->needs_await = false;
    }
};
