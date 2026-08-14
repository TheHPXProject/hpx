//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/tracing.hpp>

#include <cstddef>
#include <memory>

namespace hpx::traits {

    // By default, we don't know anything about the function's name
    HPX_CXX_CORE_EXPORT template <typename F, typename Enable = void>
    struct get_function_annotation
    {
        static constexpr char const* call(F const& /*f*/) noexcept
        {
            return nullptr;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename F, typename Enable = void>
    struct get_function_annotation_tracing
    {
        static hpx::tracing::annotation_handle call(F const& f)
        {
            static hpx::tracing::annotation_handle sh =
                hpx::tracing::create_annotation_handle(
                    get_function_annotation<F>::call(f));
            return sh;
        }
    };
}    // namespace hpx::traits
