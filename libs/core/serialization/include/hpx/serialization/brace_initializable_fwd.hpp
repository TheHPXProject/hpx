//  Copyright (c) 2019 Jan Melech
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_HAVE_CXX26_REFLECTION)

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename Archive, typename T>
    void serialize_struct(Archive& ar, T& t, unsigned int const);
}    // namespace hpx::serialization

#endif
