//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/flattened_data.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/collectives/detail/hierarchical_helpers.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    HPX_CXX_EXPORT [[nodiscard]] inline std::size_t checked_data_size_sum(
        std::size_t const lhs, std::size_t const rhs)
    {
        constexpr std::size_t max_size =
            (std::numeric_limits<std::size_t>::max)();
        if (rhs > max_size - lhs)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::collectives::detail::checked_data_size_sum",
                "the hierarchical collective payload size "
                "overflows size_t");
        }
        return lhs + rhs;
    }

    HPX_CXX_EXPORT [[nodiscard]] inline std::size_t checked_data_size_product(
        std::size_t const lhs, std::size_t const rhs)
    {
        constexpr std::size_t max_size =
            (std::numeric_limits<std::size_t>::max)();
        if (rhs != 0 && lhs > max_size / rhs)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::collectives::detail::checked_data_size_product",
                "the hierarchical collective payload size "
                "overflows size_t");
        }
        return lhs * rhs;
    }

    template <typename T, typename Iterator>
    void append_data_range(
        std::vector<T>& destination, Iterator first, Iterator const last)
    {
        auto const size = std::distance(first, last);
        HPX_ASSERT(size >= 0);
        destination.reserve(checked_data_size_sum(
            destination.size(), static_cast<std::size_t>(size)));

        // Inserting a range can instantiate vector's move-assignment path
        // even at end(). Append element-wise so internal carriers require
        // only move construction. vector<bool> additionally needs proxy
        // conversion.
        for (; first != last; ++first)
        {
            destination.emplace_back(handle_bool<T>(HPX_MOVE(*first)));
        }
    }

    // A compact carrier for equally sized logical rows. Gather and scatter
    // preserve a uniform row width, so a row count is sufficient to derive
    // every boundary without allocating or serializing an offset table.
    HPX_CXX_EXPORT template <typename T>
    struct uniform_rows
    {
        uniform_rows() = default;

        uniform_rows(
            std::vector<T>&& values, std::size_t const num_rows) noexcept
          : data_(HPX_MOVE(values))
          , num_rows_(num_rows)
        {
            HPX_ASSERT(is_valid());
        }

        explicit uniform_rows(std::vector<T>&& values) noexcept
          : data_(HPX_MOVE(values))
          , num_rows_(data_.size())
        {
        }

        template <typename Iterator>
        uniform_rows(
            std::size_t const num_rows, Iterator first, Iterator const last)
          : num_rows_(num_rows)
        {
            append_data_range(data_, first, last);
            HPX_ASSERT(is_valid());
        }

        explicit uniform_rows(T const& value)
          : num_rows_(1)
        {
            data_.emplace_back(value);
        }

        explicit uniform_rows(T&& value)
          : num_rows_(1)
        {
            data_.emplace_back(HPX_MOVE(value));
        }

        explicit uniform_rows(std::vector<uniform_rows<T>>&& values)
        {
            constexpr char const* operation =
                "hpx::collectives::detail::uniform_rows::uniform_rows";

            if (values.size() == 1)
            {
                values.front().validate(operation);
                data_ = HPX_MOVE(values.front().data_);
                num_rows_ = values.front().num_rows_;
                return;
            }

            std::size_t total_size = 0;
            std::size_t total_rows = 0;
            std::size_t expected_row_width = 0;
            bool have_row_width = false;

            for (auto const& value : values)
            {
                value.validate(operation);
                total_size =
                    checked_data_size_sum(total_size, value.data_.size());
                total_rows = checked_data_size_sum(total_rows, value.num_rows_);

                if (value.num_rows_ != 0)
                {
                    std::size_t const current_row_width = value.row_width();
                    if (have_row_width &&
                        current_row_width != expected_row_width)
                    {
                        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                            operation,
                            "all uniform hierarchical collective rows must "
                            "have the same width");
                    }
                    expected_row_width = current_row_width;
                    have_row_width = true;
                }
            }

            data_.reserve(total_size);
            for (auto& value : values)
            {
                append_data_range(
                    data_, value.data_.begin(), value.data_.end());
            }
            num_rows_ = total_rows;

            HPX_ASSERT(is_valid());
        }

        [[nodiscard]] bool is_valid() const noexcept
        {
            return num_rows_ == 0 ? data_.empty() :
                                    data_.size() % num_rows_ == 0;
        }

        void validate(char const* const operation) const
        {
            if (!is_valid())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, operation,
                    "the uniform-row hierarchical collective payload has an "
                    "invalid row count");
            }
        }

        [[nodiscard]] std::size_t row_width() const
        {
            HPX_ASSERT(is_valid());
            return num_rows_ == 0 ? 0 : data_.size() / num_rows_;
        }

        [[nodiscard]] uniform_rows extract(
            std::size_t const slice, std::size_t const num_slices) &&
        {
            // This deliberately moves the selected elements out of the
            // carrier. Communicator finalizers invoke this under the server
            // lock, and each site consumes a disjoint row range exactly once.
            HPX_ASSERT(is_valid());
            HPX_ASSERT(num_slices != 0 && slice < num_slices);

            std::size_t const division_steps = num_rows_ / num_slices;
            std::size_t const remainder = num_rows_ % num_slices;
            std::size_t const first_row =
                slice * division_steps + (std::min) (slice, remainder);
            std::size_t const row_count =
                division_steps + (slice < remainder ? 1 : 0);
            std::size_t const width = row_width();
            std::size_t const first =
                checked_data_size_product(first_row, width);
            std::size_t const count =
                checked_data_size_product(row_count, width);
            std::size_t const last = checked_data_size_sum(first, count);

            return uniform_rows(
                row_count, data_.begin() + first, data_.begin() + last);
        }

        [[nodiscard]] T unwrap_value() &&
        {
            HPX_ASSERT(is_valid());
            HPX_ASSERT(num_rows_ == 1 && data_.size() == 1);

            return handle_bool<T>(HPX_MOVE(data_.front()));
        }

        [[nodiscard]] std::vector<T> unwrap_row() &&
        {
            HPX_ASSERT(is_valid());
            HPX_ASSERT(num_rows_ == 1);
            return HPX_MOVE(data_);
        }

        [[nodiscard]] std::vector<T> unwrap_values() &&
        {
            HPX_ASSERT(is_valid());
            HPX_ASSERT(num_rows_ == data_.size());
            return HPX_MOVE(data_);
        }

        [[nodiscard]] std::vector<T> release_data() &&
        {
            HPX_ASSERT(is_valid());
            return HPX_MOVE(data_);
        }

        [[nodiscard]] std::vector<T> const& data() const noexcept
        {
            return data_;
        }

        [[nodiscard]] std::size_t num_rows() const noexcept
        {
            return num_rows_;
        }

    private:
        friend class hpx::serialization::access;

        std::vector<T> data_;
        std::size_t num_rows_ = 0;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const)
        {
            ar & data_ & num_rows_;
        }
    };

    template <typename T>
    struct is_uniform_rows : std::false_type
    {
    };

    template <typename T>
    struct is_uniform_rows<uniform_rows<T>> : std::true_type
    {
    };

    HPX_CXX_EXPORT template <typename T>
    inline constexpr bool is_uniform_rows_v =
        is_uniform_rows<std::decay_t<T>>::value;

    // A compact carrier for variable-sized logical segments. Hierarchical
    // all-to-all uses one segment per source-row/destination-group pair, so
    // prefix offsets retain the segment boundaries without nested vectors.
    HPX_CXX_EXPORT template <typename T>
    struct ragged_rows
    {
        ragged_rows()
          : offsets_{0, 0}
        {
        }

        ragged_rows(std::vector<T>&& values,
            std::vector<std::size_t>&& offsets) noexcept
          : data_(HPX_MOVE(values))
          , offsets_(HPX_MOVE(offsets))
        {
            HPX_ASSERT(is_valid());
        }

        ragged_rows(
            std::vector<ragged_rows<T>>& values, std::size_t const column)
        {
            // Communicator finalizers call this under the server lock. Each
            // site consumes a disjoint column, so moved elements are visited
            // exactly once even though the shared carriers remain in place.
            // The result has one segment per input contributor, containing
            // all of that contributor's selected-column rows.
            constexpr char const* operation =
                "hpx::collectives::detail::ragged_rows::ragged_rows";
            std::size_t const num_columns = values.size();
            HPX_ASSERT(num_columns != 0 && column < num_columns);

            std::size_t total_size = 0;
            for (auto const& value : values)
            {
                value.validate_for_exchange(num_columns, operation);
                std::size_t const num_source_rows =
                    value.num_segments() / num_columns;
                for (std::size_t row = 0; row != num_source_rows; ++row)
                {
                    std::size_t const segment = row * num_columns + column;
                    total_size = checked_data_size_sum(
                        total_size, value.segment_size(segment));
                }
            }

            data_.reserve(total_size);
            offsets_.reserve(
                checked_data_size_sum(num_columns, std::size_t{1}));
            offsets_.push_back(0);

            for (auto& value : values)
            {
                std::size_t const num_source_rows =
                    value.num_segments() / num_columns;
                for (std::size_t row = 0; row != num_source_rows; ++row)
                {
                    std::size_t const segment = row * num_columns + column;
                    append_data_range(data_,
                        value.data_.begin() + value.offsets_[segment],
                        value.data_.begin() + value.offsets_[segment + 1]);
                }
                offsets_.push_back(data_.size());
            }

            HPX_ASSERT(is_valid());
        }

        [[nodiscard]] bool is_valid() const noexcept
        {
            return offsets_.size() >= 2 && offsets_.front() == 0 &&
                offsets_.back() == data_.size() &&
                std::is_sorted(offsets_.begin(), offsets_.end());
        }

        void validate(char const* const operation) const
        {
            if (!is_valid())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, operation,
                    "the ragged hierarchical collective payload has invalid "
                    "offsets");
            }
        }

        void validate_for_exchange(
            std::size_t const num_columns, char const* const operation) const
        {
            HPX_ASSERT(num_columns != 0);
            validate(operation);
            if (num_segments() % num_columns != 0)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter, operation,
                    "the ragged payload must contain a complete set of "
                    "destination-group segments for every source row");
            }
        }

        [[nodiscard]] std::size_t num_segments() const noexcept
        {
            return offsets_.empty() ? 0 : offsets_.size() - 1;
        }

        [[nodiscard]] std::size_t segment_size(std::size_t const segment) const
        {
            HPX_ASSERT(is_valid());
            HPX_ASSERT(segment < num_segments());
            return offsets_[segment + 1] - offsets_[segment];
        }

        [[nodiscard]] std::vector<T> release_data() &&
        {
            HPX_ASSERT(is_valid());
            return HPX_MOVE(data_);
        }

        [[nodiscard]] std::vector<T> const& data() const noexcept
        {
            return data_;
        }

        [[nodiscard]] std::vector<std::size_t> const& offsets() const noexcept
        {
            return offsets_;
        }

    private:
        friend class hpx::serialization::access;

        std::vector<T> data_;
        std::vector<std::size_t> offsets_;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const)
        {
            ar & data_ & offsets_;
        }
    };

    template <typename T>
    struct is_ragged_rows : std::false_type
    {
    };

    template <typename T>
    struct is_ragged_rows<ragged_rows<T>> : std::true_type
    {
    };

    HPX_CXX_EXPORT template <typename T>
    inline constexpr bool is_ragged_rows_v =
        is_ragged_rows<std::decay_t<T>>::value;

}    // namespace hpx::collectives::detail
