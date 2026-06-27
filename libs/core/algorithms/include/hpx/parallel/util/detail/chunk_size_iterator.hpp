//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/util.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::util::detail {

    HPX_CXX_CORE_EXPORT template <typename IterOrR, typename Enable = void>
    struct chunk_size_iterator_category;

    template <typename IterOrR>
    struct chunk_size_iterator_category<IterOrR,
        std::enable_if_t<std::is_integral_v<IterOrR>>>
    {
        using type = std::random_access_iterator_tag;
    };

    template <typename Iterator>
    struct chunk_size_iterator_category<Iterator,
        std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
    {
        using type = hpx::traits::iter_category_t<Iterator>;
    };

    template <typename Range>
    struct chunk_size_iterator_category<Range,
        std::enable_if_t<std::ranges::range<Range>>>
    {
        using type = hpx::traits::range_category_t<Range>;
    };

    template <typename Range>
    struct chunk_size_iterator_category<Range,
        std::enable_if_t<hpx::traits::is_range_generator_v<Range>>>
    {
        using type = std::random_access_iterator_tag;
    };

    HPX_CXX_CORE_EXPORT template <typename IterOrR>
    using chunk_size_iterator_category_t =
        chunk_size_iterator_category<IterOrR>::type;

    HPX_CXX_CORE_EXPORT template <typename IterOrR, typename Enable = void>
    struct iterator_type;

    template <typename T>
    struct iterator_type<T,
        std::enable_if_t<std::is_integral_v<T> ||
            hpx::traits::is_range_generator_v<T>>>
    {
        using type = T;
    };

    template <typename Iterator>
    struct iterator_type<Iterator,
        std::enable_if_t<hpx::traits::is_iterator_v<Iterator>>>
    {
        using type = Iterator;
    };

    template <typename Range>
    struct iterator_type<Range, std::enable_if_t<std::ranges::range<Range>>>
    {
        using type = std::ranges::iterator_t<Range>;
    };

    HPX_CXX_CORE_EXPORT template <typename IterOrR>
    using iterator_type_t = iterator_type<IterOrR>::type;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename IterOrR>
    struct chunk_size_iterator
      : hpx::util::iterator_facade<chunk_size_iterator<IterOrR>,
            hpx::tuple<IterOrR, std::size_t>,
            chunk_size_iterator_category_t<IterOrR>,
            hpx::tuple<IterOrR, std::size_t>>
    {
    private:
        using base_type = hpx::util::iterator_facade<chunk_size_iterator,
            hpx::tuple<IterOrR, std::size_t>,
            chunk_size_iterator_category_t<IterOrR>,
            hpx::tuple<IterOrR, std::size_t>>;

        static_assert(std::is_integral_v<IterOrR> ||
            hpx::traits::is_iterator_v<IterOrR> ||
            std::ranges::range<IterOrR> ||
            hpx::traits::is_range_generator_v<IterOrR>);

        static constexpr bool is_iterator = hpx::traits::is_iterator_v<IterOrR>;

        HPX_HOST_DEVICE static constexpr std::size_t get_last_chunk_size(
            std::size_t count, std::size_t chunk_size) noexcept
        {
            auto const remainder =
                static_cast<std::ptrdiff_t>(count % chunk_size);
            if (remainder != 0)
            {
                return remainder;
            }
            return chunk_size;
        }

        HPX_HOST_DEVICE static constexpr std::size_t get_current(
            std::size_t current, std::size_t chunk_size) noexcept
        {
            return (current + chunk_size - 1) / chunk_size * chunk_size;
        }

    public:
        HPX_HOST_DEVICE chunk_size_iterator() = default;

        HPX_HOST_DEVICE chunk_size_iterator(IterOrR it, std::size_t chunk_size,
            std::size_t count = 0, std::size_t current = 0) noexcept
          : data_(it, 0)
          , chunk_size_((std::max) ((std::min) (chunk_size, count),
                static_cast<std::size_t>(1)))
          , last_chunk_size_(get_last_chunk_size(count, chunk_size))
          , count_(count)
          , current_(get_current(current, chunk_size))
        {
            if (current_ >= count_)
            {
                // reached the end of the sequence
                target() = next(target(), 0, 0);
                chunk() = 0;
            }
            else if (current_ == count_ - last_chunk_size_)
            {
                // reached last chunk
                target() = next(target(), 0, last_chunk_size_);
                chunk() = last_chunk_size_;
            }
            else
            {
                // normal chunk
                target() = next(target(), 0, chunk_size_);
                chunk() = chunk_size_;
            }
        }

        using use_brackets_proxy = std::false_type;

    private:
        HPX_HOST_DEVICE IterOrR& target() noexcept
        {
            return hpx::get<0>(data_);
        }
        HPX_HOST_DEVICE constexpr IterOrR target() const noexcept
        {
            return hpx::get<0>(data_);
        }

        HPX_HOST_DEVICE std::size_t& chunk() noexcept
        {
            return hpx::get<1>(data_);
        }
        HPX_HOST_DEVICE constexpr std::size_t chunk() const noexcept
        {
            return hpx::get<1>(data_);
        }

    protected:
        friend class hpx::util::iterator_core_access;

        template <typename IterOrR1>
        HPX_HOST_DEVICE constexpr bool equal(
            chunk_size_iterator<IterOrR1> const& other) const noexcept
        {
            return current_ == other.current_;
        }

        HPX_HOST_DEVICE constexpr typename base_type::reference dereference()
            const noexcept
        {
            return data_;
        }

    private:
        static constexpr IterOrR next(IterOrR const& target, std::size_t first,
            [[maybe_unused]] std::size_t size)
        {
            if constexpr (is_iterator || std::is_integral_v<IterOrR>)
            {
                return parallel::detail::next(target, first);
            }
            else
            {
                return hpx::util::subrange(target, first, size);
            }
        }

    protected:
        HPX_HOST_DEVICE void increment(std::size_t offset) noexcept
        {
            current_ += offset + chunk_size_;
            if (current_ >= count_)
            {
                // reached the end of the sequence
                target() = next(target(), offset + last_chunk_size_, 0);
                chunk() = 0;
            }
            else if (current_ == count_ - last_chunk_size_)
            {
                // reached last chunk
                target() =
                    next(target(), offset + chunk_size_, last_chunk_size_);
                chunk() = last_chunk_size_;
            }
            else
            {
                // normal chunk
                HPX_ASSERT(current_ < count_ - last_chunk_size_);
                target() = next(target(), offset + chunk_size_, chunk_size_);
                chunk() = chunk_size_;
            }
        }

        HPX_HOST_DEVICE void increment() noexcept
        {
            increment(0);
        }

        HPX_HOST_DEVICE void decrement(std::size_t offset) noexcept
        {
            current_ -= offset + chunk_size_;
            if (current_ == 0)
            {
                // reached the start of the sequence
                chunk() = current_ + chunk_size_ >= count_ ? last_chunk_size_ :
                                                             chunk_size_;
            }
            else if (current_ == count_ - last_chunk_size_)
            {
                // reached last chunk (was at end)
                chunk() = last_chunk_size_;
            }
            else
            {
                // normal chunk
                HPX_ASSERT(current_ < count_ - last_chunk_size_);
                chunk() = chunk_size_;
            }

            target() = next(target(),
                -static_cast<std::ptrdiff_t>(offset + chunk()), chunk());
        }

        template <typename Iter = IterOrR>
        // clang-format off
            requires (std::bidirectional_iterator<
                                      iterator_type_t<Iter>> ||
                hpx::traits::belongs_to_iterator_category_v<
                    iterator_type_t<Iter>, std::bidirectional_iterator_tag> ||
                hpx::traits::is_range_generator_v<Iter> ||
                std::is_integral_v<Iter>
            )
        // clang-format on
        HPX_HOST_DEVICE void decrement() noexcept
        {
            decrement(0);
        }

        template <typename Iter = IterOrR>
        // clang-format off
            requires (std::random_access_iterator<
                                      iterator_type_t<Iter>> ||
                hpx::traits::belongs_to_iterator_category_v<
                    iterator_type_t<Iter>, std::random_access_iterator_tag> ||
                hpx::traits::is_range_generator_v<Iter> ||
                std::is_integral_v<Iter>
            )
        // clang-format on
        HPX_HOST_DEVICE void advance(std::ptrdiff_t n) noexcept
        {
            // prepare next value
            if (n > 0)
            {
                increment((n - 1) * chunk_size_);
            }
            else if (n < 0)
            {
                decrement(-(n + 1) * chunk_size_);
            }
        }

        template <typename Iter = IterOrR>
        // clang-format off
            requires (std::random_access_iterator<
                                      iterator_type_t<Iter>> ||
                hpx::traits::belongs_to_iterator_category_v<
                    iterator_type_t<Iter>, std::random_access_iterator_tag> ||
                hpx::traits::is_range_generator_v<Iter> ||
                std::is_integral_v<Iter>
            )
        // clang-format on
        HPX_HOST_DEVICE constexpr std::ptrdiff_t distance_to(
            chunk_size_iterator const& rhs) const noexcept
        {
            return static_cast<std::ptrdiff_t>(
                (rhs.current_ - current_ + chunk_size_ - 1) / chunk_size_);
        }

    private:
        hpx::tuple<IterOrR, std::size_t> data_;
        std::size_t chunk_size_ = 0;
        std::size_t last_chunk_size_ = 0;
        std::size_t count_ = 0;
        std::size_t current_ = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename IterOrR>
    struct chunk_size_idx_iterator
      : hpx::util::iterator_facade<chunk_size_idx_iterator<IterOrR>,
            hpx::tuple<IterOrR, std::size_t, std::size_t>,
            chunk_size_iterator_category_t<IterOrR>,
            hpx::tuple<IterOrR, std::size_t, std::size_t>>
    {
    private:
        using base_type = hpx::util::iterator_facade<chunk_size_idx_iterator,
            hpx::tuple<IterOrR, std::size_t, std::size_t>,
            chunk_size_iterator_category_t<IterOrR>,
            hpx::tuple<IterOrR, std::size_t, std::size_t>>;

        static_assert(std::is_integral_v<IterOrR> ||
            hpx::traits::is_iterator_v<IterOrR> ||
            std::ranges::range<IterOrR> ||
            hpx::traits::is_range_generator_v<IterOrR>);

        static constexpr bool is_iterator = hpx::traits::is_iterator_v<IterOrR>;

        HPX_HOST_DEVICE static constexpr std::size_t get_last_chunk_size(
            std::size_t count, std::size_t chunk_size) noexcept
        {
            auto const remainder =
                static_cast<std::ptrdiff_t>(count % chunk_size);
            if (remainder != 0)
            {
                return remainder;
            }
            return chunk_size;
        }

        HPX_HOST_DEVICE static constexpr std::size_t get_current(
            std::size_t current, std::size_t chunk_size) noexcept
        {
            return (current + chunk_size - 1) / chunk_size * chunk_size;
        }

    public:
        HPX_HOST_DEVICE chunk_size_idx_iterator() = default;

        HPX_HOST_DEVICE chunk_size_idx_iterator(IterOrR it,
            std::size_t chunk_size, std::size_t count = 0,
            std::size_t current = 0, std::size_t base_idx = 0)
          : data_(it, 0, base_idx)
          , chunk_size_((std::max) ((std::min) (chunk_size, count),
                static_cast<std::size_t>(1)))
          , last_chunk_size_(get_last_chunk_size(count, chunk_size))
          , count_(count)
          , current_(get_current(current, chunk_size))
        {
            if (current_ >= count_)
            {
                // reached the end of the sequence
                target() = next(target(), 0, 0);
                chunk() = 0;
            }
            else if (current_ == count_ - last_chunk_size_)
            {
                // reached last chunk
                target() = next(target(), 0, last_chunk_size_);
                chunk() = last_chunk_size_;
            }
            else
            {
                // normal chunk
                HPX_ASSERT(current_ < count_ - last_chunk_size_);
                target() = next(target(), 0, chunk_size_);
                chunk() = chunk_size_;
            }
        }

        using use_brackets_proxy = std::false_type;

    private:
        HPX_HOST_DEVICE IterOrR& target() noexcept
        {
            return hpx::get<0>(data_);
        }
        HPX_HOST_DEVICE constexpr IterOrR target() const noexcept
        {
            return hpx::get<0>(data_);
        }

        HPX_HOST_DEVICE std::size_t& chunk() noexcept
        {
            return hpx::get<1>(data_);
        }
        HPX_HOST_DEVICE constexpr std::size_t chunk() const noexcept
        {
            return hpx::get<1>(data_);
        }

        HPX_HOST_DEVICE std::size_t& base_index() noexcept
        {
            return hpx::get<2>(data_);
        }

        HPX_HOST_DEVICE constexpr std::size_t base_index() const noexcept
        {
            return hpx::get<2>(data_);
        }

    protected:
        friend class hpx::util::iterator_core_access;

        template <typename IterOrR1>
        HPX_HOST_DEVICE constexpr bool equal(
            chunk_size_idx_iterator<IterOrR1> const& other) const noexcept
        {
            return current_ == other.current_;
        }

        HPX_HOST_DEVICE constexpr typename base_type::reference dereference()
            const noexcept
        {
            return data_;
        }

    private:
        static constexpr IterOrR next(IterOrR const& target, std::size_t first,
            [[maybe_unused]] std::size_t size)
        {
            if constexpr (is_iterator || std::is_integral_v<IterOrR>)
            {
                return parallel::detail::next(target, first);
            }
            else
            {
                return hpx::util::subrange(target, first, size);
            }
        }

    protected:
        HPX_HOST_DEVICE void increment(std::size_t offset) noexcept
        {
            base_index() += offset + chunk_size_;
            current_ += offset + chunk_size_;
            if (current_ >= count_)
            {
                // reached the end of the sequence
                target() = next(target(), offset + last_chunk_size_, 0);
                chunk() = 0;
            }
            else if (current_ == count_ - last_chunk_size_)
            {
                // reached last chunk
                target() =
                    next(target(), offset + chunk_size_, last_chunk_size_);
                chunk() = last_chunk_size_;
            }
            else
            {
                // normal chunk
                HPX_ASSERT(current_ < count_ - last_chunk_size_);
                target() = next(target(), offset + chunk_size_, chunk_size_);
                chunk() = chunk_size_;
            }
        }

        HPX_HOST_DEVICE void increment() noexcept
        {
            increment(0);
        }

        HPX_HOST_DEVICE void decrement(std::size_t offset) noexcept
        {
            base_index() -= offset + chunk_size_;
            current_ -= offset + chunk_size_;
            if (current_ == 0)
            {
                // reached the start of the sequence
                chunk() = base_index() + chunk_size_ >= count_ ?
                    last_chunk_size_ :
                    chunk_size_;
            }
            else if (current_ == count_ - last_chunk_size_)
            {
                // reached last chunk (was at end)
                chunk() = last_chunk_size_;
            }
            else
            {
                // normal chunk
                HPX_ASSERT(current_ < count_ - last_chunk_size_);
                chunk() = chunk_size_;
            }

            target() = next(target(),
                -static_cast<std::ptrdiff_t>(offset + chunk()), chunk());
        }

        template <typename Iter = IterOrR>
        // clang-format off
            requires (std::bidirectional_iterator<
                                      iterator_type_t<Iter>> ||
                hpx::traits::belongs_to_iterator_category_v<
                    iterator_type_t<Iter>, std::bidirectional_iterator_tag> ||
                hpx::traits::is_range_generator_v<Iter> ||
                std::is_integral_v<Iter>
            )
        // clang-format on
        HPX_HOST_DEVICE void decrement() noexcept
        {
            decrement(0);
        }

        template <typename Iter = IterOrR>
        // clang-format off
            requires (std::random_access_iterator<
                                      iterator_type_t<Iter>> ||
                hpx::traits::belongs_to_iterator_category_v<
                    iterator_type_t<Iter>, std::random_access_iterator_tag> ||
                hpx::traits::is_range_generator_v<Iter> ||
                std::is_integral_v<Iter>
            )
        // clang-format on
        HPX_HOST_DEVICE void advance(std::ptrdiff_t n) noexcept
        {
            // prepare next value
            if (n > 0)
            {
                increment((n - 1) * chunk_size_);
            }
            else if (n < 0)
            {
                decrement(-(n + 1) * chunk_size_);
            }
        }

        template <typename Iter = IterOrR>
        // clang-format off
            requires (std::random_access_iterator<
                                      iterator_type_t<Iter>> ||
                hpx::traits::belongs_to_iterator_category_v<
                    iterator_type_t<Iter>, std::random_access_iterator_tag> ||
                hpx::traits::is_range_generator_v<Iter> ||
                std::is_integral_v<Iter>
            )
        // clang-format on
        HPX_HOST_DEVICE constexpr std::ptrdiff_t distance_to(
            chunk_size_idx_iterator const& rhs) const noexcept
        {
            return static_cast<std::ptrdiff_t>(
                (rhs.current_ - current_ + chunk_size_ - 1) / chunk_size_);
        }

    private:
        hpx::tuple<IterOrR, std::size_t, std::size_t> data_;
        std::size_t chunk_size_ = 0;
        std::size_t last_chunk_size_ = 0;
        std::size_t count_ = 0;
        std::size_t current_ = 0;
    };
}    // namespace hpx::parallel::util::detail
