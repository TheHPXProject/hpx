//  Copyright (c) 2017 Agustin Berge
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/iterator_support/range.hpp>
#include <hpx/iterator_support/traits/is_iterator.hpp>
#include <hpx/iterator_support/traits/is_range.hpp>

#include <cstddef>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT template <typename Iterator,
        typename Sentinel = Iterator>
    class iterator_range
    {
        static_assert(hpx::traits::is_iterator_v<Iterator>);
        static_assert(std::sentinel_for<Sentinel, Iterator>);

    public:
        HPX_HOST_DEVICE iterator_range() = default;

        HPX_HOST_DEVICE constexpr iterator_range(
            Iterator iterator, Sentinel sentinel) noexcept
          : _iterator(HPX_MOVE(iterator))
          , _sentinel(HPX_MOVE(sentinel))
        {
        }

        // clang-format off
        template <typename Range,
            typename Enable =
                std::enable_if_t<
                    std::ranges::range<std::decay_t<Range>> &&
                   !std::is_same_v<iterator_range, std::decay_t<Range>>>>
        // clang-format on
        HPX_HOST_DEVICE explicit constexpr iterator_range(Range&& r) noexcept
          : iterator_range(util::begin(r), util::end(r))
        {
        }

        [[nodiscard]] HPX_HOST_DEVICE constexpr Iterator begin() const
        {
            return _iterator;
        }

        [[nodiscard]] HPX_HOST_DEVICE constexpr Iterator end() const
        {
            return _sentinel;
        }

        [[nodiscard]] HPX_HOST_DEVICE constexpr std::ptrdiff_t size() const
        {
            return std::ranges::distance(_iterator, _sentinel);
        }

        [[nodiscard]] HPX_HOST_DEVICE constexpr bool empty() const
        {
            return _iterator == _sentinel;
        }

    private:
        Iterator _iterator;
        Sentinel _sentinel;
    };

    HPX_CXX_CORE_EXPORT template <typename Range>
    iterator_range(Range& r) -> iterator_range<std::ranges::iterator_t<Range>>;

    HPX_CXX_CORE_EXPORT template <typename Range>
    iterator_range(Range const& r)
        -> iterator_range<std::ranges::iterator_t<Range const>>;

    HPX_CXX_CORE_EXPORT template <typename Iterator, typename Sentinel>
    iterator_range(Iterator it, Sentinel sent)
        -> iterator_range<Iterator, Sentinel>;

    template <typename Range,
        typename Iterator = std::ranges::iterator_t<Range>,
        typename Sentinel = std::ranges::iterator_t<Range>>
    HPX_DEPRECATED_V(1, 9,
        "hpx::util::make_iterator_range is deprecated, use "
        "hpx::util::iterator_range instead")
    HPX_HOST_DEVICE constexpr std::enable_if_t<std::ranges::range<Range>,
        iterator_range<Iterator, Sentinel>> make_iterator_range(Range&
            r) noexcept
    {
        return iterator_range<Iterator, Sentinel>(util::begin(r), util::end(r));
    }

    template <typename Range,
        typename Iterator = std::ranges::iterator_t<Range const>,
        typename Sentinel = std::ranges::iterator_t<Range const>>
    HPX_DEPRECATED_V(1, 9,
        "hpx::util::make_iterator_range is deprecated, use "
        "hpx::util::iterator_range instead")
    HPX_HOST_DEVICE constexpr std::enable_if_t<std::ranges::range<Range>,
        iterator_range<Iterator, Sentinel>> make_iterator_range(Range const&
            r) noexcept
    {
        return iterator_range<Iterator, Sentinel>(util::begin(r), util::end(r));
    }

    template <typename Iterator, typename Sentinel = Iterator>
    HPX_DEPRECATED_V(1, 9,
        "hpx::util::make_iterator_range is deprecated, use "
        "hpx::util::iterator_range instead")
    HPX_HOST_DEVICE constexpr std::enable_if_t<traits::is_iterator_v<Iterator>,
        iterator_range<Iterator, Sentinel>> make_iterator_range(Iterator it,
        Sentinel sent) noexcept
    {
        return iterator_range<Iterator, Sentinel>(HPX_MOVE(it), HPX_MOVE(sent));
    }
}    // namespace hpx::util

namespace hpx::ranges {

    HPX_CXX_CORE_EXPORT template <typename I, typename S = I>
    using subrange_t = hpx::util::iterator_range<I, S>;
}    // namespace hpx::ranges
