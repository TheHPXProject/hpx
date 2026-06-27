//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/serialization.hpp>

#include <type_traits>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for streaming with util::any
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct supports_streaming_with_any
      : std::true_type    // the default is to support streaming
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // Customization point for streaming with util::any, we don't want
    // serialization::serialize_buffer to be streamable
    template <typename T, typename Allocator>
    struct supports_streaming_with_any<
        serialization::serialize_buffer<T, Allocator>> : std::false_type
    {
    };
}    // namespace hpx::traits
