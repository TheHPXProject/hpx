//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/flattened_data.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    enum class flattened_payload_mode : std::uint8_t
    {
        enabled
    };

    template <typename T>
    struct flattened_data
    {
        std::vector<T> data;

        // Prefix boundaries into data. A boundary may describe a logical site
        // row or an exchange segment, depending on the collective phase.
        std::vector<std::uint32_t> offsets;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const)
        {
            ar & data & offsets;
        }
    };

    [[nodiscard]] inline std::uint32_t checked_flattened_offset(
        std::size_t const offset)
    {
        if (offset > (std::numeric_limits<std::uint32_t>::max)())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::collectives::detail::flattened_data",
                "a flattened hierarchical collective payload cannot exceed "
                "UINT32_MAX elements");
        }

        return static_cast<std::uint32_t>(offset);
    }

    [[nodiscard]] inline std::size_t checked_flattened_sum(
        std::size_t const lhs, std::size_t const rhs)
    {
        constexpr std::size_t max_offset =
            (std::numeric_limits<std::uint32_t>::max)();
        if (lhs > max_offset || rhs > max_offset - lhs)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::collectives::detail::flattened_data",
                "a flattened hierarchical collective payload cannot exceed "
                "UINT32_MAX elements");
        }
        return lhs + rhs;
    }

    [[nodiscard]] inline std::size_t checked_flattened_product(
        std::size_t const lhs, std::size_t const rhs)
    {
        constexpr std::size_t max_offset =
            (std::numeric_limits<std::uint32_t>::max)();
        if (rhs != 0 && lhs > max_offset / rhs)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::collectives::detail::flattened_data",
                "a flattened hierarchical collective payload cannot exceed "
                "UINT32_MAX elements");
        }
        return lhs * rhs;
    }

    template <typename T>
    [[nodiscard]] bool is_valid_flattened_data(
        flattened_data<T> const& value) noexcept
    {
        if (value.offsets.empty() || value.offsets.front() != 0 ||
            value.offsets.back() != value.data.size())
        {
            return false;
        }

        for (std::size_t i = 1; i != value.offsets.size(); ++i)
        {
            if (value.offsets[i] < value.offsets[i - 1])
            {
                return false;
            }
        }

        return true;
    }

    template <typename T>
    void validate_flattened_data(
        flattened_data<T> const& value, char const* const operation)
    {
        if (!is_valid_flattened_data(value))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, operation,
                "the flattened hierarchical collective payload has invalid "
                "offsets");
        }
    }

    template <typename T, typename Iterator>
    void append_flattened_range(
        std::vector<T>& destination, Iterator first, Iterator const last)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            for (; first != last; ++first)
            {
                destination.push_back(static_cast<bool>(*first));
            }
        }
        else
        {
            destination.insert(destination.end(),
                std::make_move_iterator(first), std::make_move_iterator(last));
        }
    }

    template <typename T, typename U>
    void append_flattened_value(std::vector<T>& destination, U&& value)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            destination.push_back(static_cast<bool>(value));
        }
        else
        {
            destination.emplace_back(HPX_FORWARD(U, value));
        }
    }

    template <typename T>
    flattened_data<T> make_flattened_rows(std::vector<T>&& values)
    {
        flattened_data<T> result;
        result.data = HPX_MOVE(values);
        static_cast<void>(checked_flattened_offset(result.data.size()));
        result.offsets.reserve(result.data.size() + 1);
        for (std::size_t i = 0; i <= result.data.size(); ++i)
        {
            result.offsets.push_back(checked_flattened_offset(i));
        }
        return result;
    }

    template <typename T>
    flattened_data<T> make_flattened_row(std::vector<T>&& values)
    {
        flattened_data<T> result;
        result.data = HPX_MOVE(values);
        result.offsets = {0, checked_flattened_offset(result.data.size())};
        return result;
    }

    template <typename T>
    flattened_data<std::decay_t<T>> make_flattened_value(T&& value)
    {
        flattened_data<std::decay_t<T>> result;
        result.data.emplace_back(HPX_FORWARD(T, value));
        result.offsets = {0, 1};
        return result;
    }

    template <typename T>
    flattened_data<T> merge_flattened_data(
        std::vector<flattened_data<T>>& values)
    {
        std::size_t total_size = 0;
        std::size_t total_rows = 0;
        for (auto const& value : values)
        {
            HPX_ASSERT(is_valid_flattened_data(value));
            total_size = checked_flattened_sum(total_size, value.data.size());
            total_rows =
                checked_flattened_sum(total_rows, value.offsets.size() - 1);
        }

        flattened_data<T> result;
        result.data.reserve(total_size);
        result.offsets.reserve(total_rows + 1);
        result.offsets.push_back(0);

        for (auto& value : values)
        {
            std::size_t const base = result.data.size();
            append_flattened_range(
                result.data, value.data.begin(), value.data.end());

            for (std::size_t i = 1; i != value.offsets.size(); ++i)
            {
                result.offsets.push_back(checked_flattened_offset(
                    base + static_cast<std::size_t>(value.offsets[i])));
            }
        }

        HPX_ASSERT(is_valid_flattened_data(result));
        return result;
    }

    template <typename T>
    flattened_data<T> slice_flattened_data(flattened_data<T>& value,
        std::size_t const slice, std::size_t const num_slices)
    {
        HPX_ASSERT(is_valid_flattened_data(value));
        HPX_ASSERT(num_slices != 0 && slice < num_slices);

        std::size_t const num_rows = value.offsets.size() - 1;
        std::size_t const division_steps = num_rows / num_slices;
        std::size_t const remainder = num_rows % num_slices;
        std::size_t const first_row =
            slice * division_steps + (std::min) (slice, remainder);
        std::size_t const row_count =
            division_steps + (slice < remainder ? 1 : 0);
        std::size_t const last_row = first_row + row_count;

        std::size_t const first = value.offsets[first_row];
        std::size_t const last = value.offsets[last_row];

        flattened_data<T> result;
        result.data.reserve(last - first);
        append_flattened_range(
            result.data, value.data.begin() + first, value.data.begin() + last);
        result.offsets.reserve(row_count + 1);
        for (std::size_t i = first_row; i <= last_row; ++i)
        {
            result.offsets.push_back(
                static_cast<std::uint32_t>(value.offsets[i] - first));
        }

        HPX_ASSERT(is_valid_flattened_data(result));
        return result;
    }

    template <typename T>
    flattened_data<T> select_flattened_column(
        std::vector<flattened_data<T>>& values, std::size_t const column)
    {
        std::size_t const num_columns = values.size();
        HPX_ASSERT(num_columns != 0 && column < num_columns);

        std::size_t total_size = 0;
        for (auto const& value : values)
        {
            HPX_ASSERT(is_valid_flattened_data(value));
            std::size_t const num_segments = value.offsets.size() - 1;
            HPX_ASSERT(num_segments % num_columns == 0);

            std::size_t const num_rows = num_segments / num_columns;
            for (std::size_t row = 0; row != num_rows; ++row)
            {
                std::size_t const segment = row * num_columns + column;
                total_size = checked_flattened_sum(total_size,
                    value.offsets[segment + 1] - value.offsets[segment]);
            }
        }

        flattened_data<T> result;
        result.data.reserve(total_size);
        result.offsets.reserve(num_columns + 1);
        result.offsets.push_back(0);

        for (auto& value : values)
        {
            std::size_t const num_rows =
                (value.offsets.size() - 1) / num_columns;
            for (std::size_t row = 0; row != num_rows; ++row)
            {
                std::size_t const segment = row * num_columns + column;
                append_flattened_range(result.data,
                    value.data.begin() + value.offsets[segment],
                    value.data.begin() + value.offsets[segment + 1]);
            }
            result.offsets.push_back(
                checked_flattened_offset(result.data.size()));
        }

        HPX_ASSERT(is_valid_flattened_data(result));
        return result;
    }

    template <typename T>
    T unwrap_flattened_value(flattened_data<T>&& value)
    {
        HPX_ASSERT(is_valid_flattened_data(value));
        HPX_ASSERT(value.offsets.size() == 2 && value.data.size() == 1);

        if constexpr (std::is_same_v<T, bool>)
        {
            return static_cast<bool>(value.data.front());
        }
        else
        {
            return HPX_MOVE(value.data.front());
        }
    }

    template <typename T>
    std::vector<T> unwrap_flattened_row(flattened_data<T>&& value)
    {
        HPX_ASSERT(is_valid_flattened_data(value));
        HPX_ASSERT(value.offsets.size() == 2);
        return HPX_MOVE(value.data);
    }
}    // namespace hpx::collectives::detail
