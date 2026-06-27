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
#include <hpx/iostream/detail/adapter/mode_adapter.hpp>
#include <hpx/iostream/detail/adapter/output_iterator_adapter.hpp>
#include <hpx/iostream/detail/adapter/range_adapter.hpp>
#include <hpx/iostream/device/array.hpp>
#include <hpx/iostream/traits.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::iostream::detail {

    //------------------Definition of resolve-------------------------------------//
    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename T>
    struct resolve_traits
      : std::conditional<std::output_iterator<T, Ch>,
            output_iterator_adapter<Mode, Ch, T>, T const&>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename T>
        requires(!iostream::is_std_io_v<T> && !std::ranges::range<T>)
    constexpr resolve_traits<Mode, Ch, T>::type resolve(T const& t) noexcept
    {
        using return_type = resolve_traits<Mode, Ch, T>::type;
        return return_type(t);
    }

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename Tr>
    mode_adapter<Mode, std::basic_streambuf<Ch, Tr>> resolve(
        std::basic_streambuf<Ch, Tr>& sb) noexcept
    {
        return mode_adapter<Mode, std::basic_streambuf<Ch, Tr>>(std::ref(sb));
    }

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename Tr>
    mode_adapter<Mode, std::basic_istream<Ch, Tr>> resolve(
        std::basic_istream<Ch, Tr>& is) noexcept
    {
        return mode_adapter<Mode, std::basic_istream<Ch, Tr>>(std::ref(is));
    }

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename Tr>
    mode_adapter<Mode, std::basic_ostream<Ch, Tr>> resolve(
        std::basic_ostream<Ch, Tr>& os) noexcept
    {
        return mode_adapter<Mode, std::basic_ostream<Ch, Tr>>(std::ref(os));
    }

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename Tr>
    mode_adapter<Mode, std::basic_iostream<Ch, Tr>> resolve(
        std::basic_iostream<Ch, Tr>& io) noexcept
    {
        return mode_adapter<Mode, std::basic_iostream<Ch, Tr>>(std::ref(io));
    }

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, std::size_t N>
    array_adapter<Mode, Ch> resolve(Ch (&array)[N])
    {
        return array_adapter<Mode, Ch>(array);
    }

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename Iter>
    range_adapter<Mode, util::iterator_range<Iter>> resolve(
        util::iterator_range<Iter> const& rng)
    {
        return range_adapter<Mode, util::iterator_range<Iter>>(rng);
    }
}    // namespace hpx::iostream::detail

#include <hpx/config/warnings_suffix.hpp>
