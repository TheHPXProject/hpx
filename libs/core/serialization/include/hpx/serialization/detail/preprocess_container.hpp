//  Copyright (c) 2015-2025 Hartmut Kaiser
//  Copyright (c) 2015-2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/serialization/traits/serialization_access_data.hpp>

#include <cstddef>
#include <type_traits>

////////////////////////////////////////////////////////////////////////////////
namespace hpx::serialization::detail {

    // This 'container' is used to gather the required archive size for a given
    // type before it is serialized.
    HPX_CXX_CORE_EXPORT class preprocess_container
    {
    public:
        preprocess_container() = default;

        [[nodiscard]] constexpr std::size_t size() const noexcept
        {
            return size_;
        }
        void resize(std::size_t const size) noexcept
        {
            size_ = size;
        }

        void reset() noexcept
        {
            size_ = 0;
        }

        static constexpr bool is_preprocessing() noexcept
        {
            return true;
        }

    private:
        std::size_t size_ = 0;
    };
}    // namespace hpx::serialization::detail

template <>
struct hpx::traits::serialization_access_data<
    hpx::serialization::detail::preprocess_container>
  : default_serialization_access_data<
        hpx::serialization::detail::preprocess_container>
{
    using preprocessing_only = std::true_type;

    [[nodiscard]] static constexpr bool is_preprocessing() noexcept
    {
        return true;
    }

    [[nodiscard]] static constexpr std::size_t size(
        hpx::serialization::detail::preprocess_container const& cont) noexcept
    {
        return cont.size();
    }

    static void resize(hpx::serialization::detail::preprocess_container& cont,
        std::size_t const count)
    {
        return cont.resize(cont.size() + count);
    }

    static void reset(
        hpx::serialization::detail::preprocess_container& cont) noexcept
    {
        cont.reset();
    }
};
