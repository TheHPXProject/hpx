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
#include <hpx/iostream/categories.hpp>

#include <ios>

namespace hpx::iostream {

    HPX_CXX_CORE_EXPORT template <typename Container>
    class back_insert_device
    {
    public:
        using char_type = Container::value_type;
        using category = sink_tag;

        constexpr back_insert_device(Container& cnt) noexcept
          : container(&cnt)
        {
        }

        std::streamsize write(char_type const* s, std::streamsize n)
        {
            container->insert(container->end(), s, s + n);
            return n;
        }

    protected:
        Container* container;
    };

    HPX_CXX_CORE_EXPORT template <typename Container>
    constexpr back_insert_device<Container> back_inserter(
        Container& cnt) noexcept
    {
        return back_insert_device<Container>(cnt);
    }
}    // namespace hpx::iostream
