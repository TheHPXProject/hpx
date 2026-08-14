// Copyright (c) 2018 Adrian Serio
// Copyright (c) 2018-2026 Hartmut Kaiser
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/checkpoint/checkpoint.hpp>

#include <cstdint>
#include <iosfwd>

namespace hpx::util {

    // Stream Overloads
    std::ostream& operator<<(std::ostream& ost, checkpoint const& ckp)
    {
        // Write the size of the checkpoint to the file
        std::int64_t size = static_cast<std::int64_t>(ckp.size());
        ost.write(reinterpret_cast<char const*>(&size), sizeof(std::int64_t));

        // Write the file to the stream
        ost.write(ckp.data(), static_cast<std::streamsize>(ckp.size()));
        return ost;
    }

    std::istream& operator>>(std::istream& ist, checkpoint& ckp)
    {
        // Read in the size of the next checkpoint
        std::int64_t length = 0;
        ist.read(reinterpret_cast<char*>(&length), sizeof(std::int64_t));
        ckp.data_.resize(length);

        // Read in the next checkpoint
        ist.read(ckp.data(), length);
        return ist;
    }
}    // namespace hpx::util
