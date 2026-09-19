//  Copyright (c) 2017 Christopher Taylor
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/detail/to_size.hpp>
#include <hpx/serialization/serialization_fwd.hpp>
#include <hpx/serialization/traits/is_bitwise_serializable.hpp>

#include <cstddef>
#include <cstdint>
#include <valarray>

namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename T>
    void serialize(input_archive& ar, std::valarray<T>& arr, int /* version */)
    {
        std::uint64_t sz = 0;
        ar >> sz;

        std::size_t const count = detail::to_size(sz);
        arr.resize(count);

        if (count == 0)
            return;

        for (std::size_t i = 0; i != count; ++i)
            ar >> arr[i];
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    void serialize(
        output_archive& ar, std::valarray<T> const& arr, int /* version */)
    {
        auto const sz = static_cast<std::uint64_t>(arr.size());
        ar << sz;
        if (sz == 0)
            return;

        for (auto const& v : arr)
            ar << v;
    }
}    // namespace hpx::serialization
