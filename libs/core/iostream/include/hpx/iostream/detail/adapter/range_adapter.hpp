//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>

#include <hpx/iostream/categories.hpp>
#include <hpx/iostream/detail/error.hpp>
#include <hpx/iostream/positioning.hpp>
#include <hpx/modules/iterator_support.hpp>

#include <algorithm>
#include <cstddef>
#include <iosfwd>
#include <iterator>
#include <ranges>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::detail {

    //
    // Template name: range_adapter
    // Description: Device based on an instance of boost::iterator_range.
    // Template parameters:
    //     Mode - A mode tag.
    //     Range - An instance of iterator_range.
    //
    HPX_CXX_CORE_EXPORT template <typename Mode, typename Range>
    class range_adapter
    {
        using iterator = std::ranges::iterator_t<Range>;
        using iter_cat = std::iterator_traits<iterator>::iterator_category;

    public:
        using char_type = std::iterator_traits<iterator>::value_type;

        struct category
          : Mode
          , device_tag
        {
        };

        explicit range_adapter(Range const& rng)
          : first_(rng.begin())
          , cur_(rng.begin())
          , last_(rng.end())
        {
        }

        range_adapter(iterator first, iterator last)
          : first_(first)
          , cur_(first)
          , last_(last)
        {
        }

        std::streamsize read(char_type* s, std::streamsize const n)
        {
            if constexpr (std::is_convertible_v<iter_cat,
                              std::random_access_iterator_tag>)
            {
                std::streamsize result =
                    (std::min) (static_cast<std::streamsize>(last_ - cur_), n);
                if (result)
                    std::copy(cur_, cur_ + result, s);
                cur_ += result;
                return result != 0 ? result : -1;
            }
            else
            {
                std::streamsize rem = n;    // No. of chars remaining.
                while (cur_ != last_ && rem-- > 0)
                    *s++ = *cur_++;
                return n - rem != 0 ? n - rem : -1;
            }
        }

        std::streamsize write(char_type const* s, std::streamsize n)
        {
            if constexpr (std::is_convertible_v<iter_cat,
                              std::random_access_iterator_tag>)
            {
                std::streamsize count =
                    (std::min) (static_cast<std::streamsize>(last_ - cur_), n);
                std::copy(s, s + count, cur_);
                cur_ += count;
                if (count < n)
                    throw write_area_exhausted();
                return n;
            }
            else
            {
                while (cur_ != last_ && n-- > 0)
                    *cur_++ = *s++;
                if (cur_ == last_ && n > 0)
                    throw write_area_exhausted();
                return n;
            }
        }

        std::streampos seek(stream_offset off, std::ios::seekdir const way)
            requires(std::is_convertible_v<iter_cat,
                std::random_access_iterator_tag>)
        {
            using namespace std;
            switch (way)
            {
            case std::ios::beg:
                if (off > last_ - first_ || off < 0)
                    throw bad_seek();
                cur_ = first_ + off;
                break;
            case std::ios::cur:
            {
                std::ptrdiff_t const newoff = cur_ - first_ + off;
                if (newoff > last_ - first_ || newoff < 0)
                    throw bad_seek();
                cur_ += off;
                break;
            }
            case std::ios::end:
                if (last_ - first_ + off < 0 || off > 0)
                    throw bad_seek();
                cur_ = last_ + off;
                break;
            default:
                HPX_ASSERT(false);
            }
            return offset_to_position(cur_ - first_);
        }

    private:
        iterator first_, cur_, last_;
    };
}    // namespace hpx::iostream::detail

#include <hpx/config/warnings_suffix.hpp>
