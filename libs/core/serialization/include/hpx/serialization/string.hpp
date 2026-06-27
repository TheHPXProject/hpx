//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/serialization/basic_archive.hpp>
#include <hpx/serialization/serialization_fwd.hpp>

#include <cstdint>
#include <string>

namespace hpx::serialization {

    // load string
    HPX_CXX_CORE_EXPORT template <typename Char, typename CharTraits,
        typename Allocator>
    void serialize(input_archive& ar,
        std::basic_string<Char, CharTraits, Allocator>& s, unsigned)
    {
        std::uint64_t size = 0;
        ar >> size;    //-V128

        s.clear();
        if (s.size() < size)
            s.resize(size);

        load_binary(ar, detail::array_of_fundamental_type_v<Char>, &s[0],
            size * sizeof(Char));
    }

    // save string
    HPX_CXX_CORE_EXPORT template <typename Char, typename CharTraits,
        typename Allocator>
    void serialize(output_archive& ar,
        std::basic_string<Char, CharTraits, Allocator> const& s, unsigned)
    {
        std::uint64_t const size = s.size();
        ar << size;
        save_binary(ar, detail::array_of_fundamental_type_v<Char>, s.data(),
            s.size() * sizeof(Char));
    }
}    // namespace hpx::serialization
