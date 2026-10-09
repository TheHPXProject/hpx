// Copyright (c) 2018 Adrian Serio
// Copyright (c) 2018-2026 Hartmut Kaiser
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/checkpoint/checkpoint.hpp>
#include <hpx/modules/errors.hpp>

#include <cstddef>
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
        if (!ist)
        {
            // the stream ran out before a length could be read, a partial
            // read leaves a length behind that means nothing
            ckp.data_.clear();
            return ist;
        }

        // The length is read off a stream, so a corrupt or truncated file can
        // hold one that is negative, or one that is larger than this platform
        // can address. Sizing a buffer from either turns an allocation that
        // has to fail into one that succeeds at the wrong size.
        auto const size = static_cast<std::uint64_t>(length);
        if (length < 0 || static_cast<std::size_t>(size) != size)
        {
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "hpx::util::operator>>(std::istream&, checkpoint&)",
                "the stream holds a checkpoint of {} bytes, which is not a "
                "length this platform can use",
                length);
        }

        ckp.data_.resize(static_cast<std::size_t>(size));

        // Read in the next checkpoint
        ist.read(ckp.data(), length);
        return ist;
    }
}    // namespace hpx::util
