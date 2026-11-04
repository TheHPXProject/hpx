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

#include <hpx/iostream/categories.hpp>

#include <algorithm>
#include <ios>
#include <type_traits>

namespace hpx::iostream::detail {

    HPX_CXX_CORE_EXPORT template <typename Mode, typename Ch, typename OutIt>
    class output_iterator_adapter
    {
    public:
        static_assert(std::is_convertible_v<Mode, output>);

        using char_type = Ch;
        using category = sink_tag;

        explicit output_iterator_adapter(OutIt out)
          : out_(out)
        {
        }

        std::streamsize write(char_type const* s, std::streamsize const n)
        {
            std::copy(s, s + n, out_);
            return n;
        }

    private:
        OutIt out_;
    };
}    // namespace hpx::iostream::detail
