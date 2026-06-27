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
#include <hpx/iostream/config/defines.hpp>

#include <cstddef>
#include <cstdio>
#include <string>
#include <type_traits>

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
#include <cwchar>
#endif

namespace hpx::iostream {

    HPX_CXX_CORE_EXPORT inline constexpr int WOULD_BLOCK = EOF - 1;

    HPX_CXX_CORE_EXPORT template <typename Ch>
    struct char_traits;

    template <>
    struct char_traits<char> : std::char_traits<char>
    {
        [[nodiscard]] static constexpr char newline() noexcept
        {
            return '\n';
        }
        [[nodiscard]] static constexpr int good() noexcept
        {
            return '\n';
        }
        [[nodiscard]] static constexpr int would_block() noexcept
        {
            return WOULD_BLOCK;
        }
        [[nodiscard]] static constexpr bool is_good(int const c) noexcept
        {
            return c != EOF && c != WOULD_BLOCK;
        }
        [[nodiscard]] static constexpr bool is_eof(int const c) noexcept
        {
            return c == EOF;
        }
        [[nodiscard]] static constexpr bool would_block(int const c) noexcept
        {
            return c == WOULD_BLOCK;
        }
    };

#if defined(HPX_IOSTREAM_HAVE_WIDE_STREAMS)
    HPX_CXX_CORE_EXPORT inline constexpr std::wint_t WWOULD_BLOCK = WEOF - 1;

    template <>
    struct char_traits<wchar_t> : std::char_traits<wchar_t>
    {
        [[nodiscard]] static constexpr wchar_t newline() noexcept
        {
            return L'\n';
        }
        [[nodiscard]] static constexpr std::wint_t good() noexcept
        {
            return L'\n';
        }
        [[nodiscard]] static constexpr std::wint_t would_block() noexcept
        {
            return WWOULD_BLOCK;
        }
        [[nodiscard]] static constexpr bool is_good(std::wint_t c) noexcept
        {
            return c != WEOF && c != WWOULD_BLOCK;
        }
        [[nodiscard]] static constexpr bool is_eof(std::wint_t c) noexcept
        {
            return c == WEOF;
        }
        [[nodiscard]] static constexpr bool would_block(std::wint_t c) noexcept
        {
            return c == WWOULD_BLOCK;
        }
    };
#endif

    namespace detail {

        //
        // Template name: translate_char.
        // Description: Translates a character or an end-of-file indicator from the
        //      int_type of one character traits type to the int_type of another.
        //
        HPX_CXX_CORE_EXPORT template <typename SourceTr, typename TargetTr>
        decltype(auto) translate_int_type(typename SourceTr::int_type c)
        {
            if constexpr (std::is_same_v<SourceTr, TargetTr>)
            {
                return c;
            }
            else
            {
                return SourceTr::eq_int_type(SourceTr::eof()) ?
                    TargetTr::eof() :
                    TargetTr::to_int_type(SourceTr::to_char_type(c));
            }
        }
    }    // namespace detail
}    // namespace hpx::iostream
