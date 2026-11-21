//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The archives of thrust_pooled_allocator.cu: nvcc (13.4) aborts compiling the
// construction of an output_archive or input_archive, the host compiler
// creates them here.

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>

#include <vector>

namespace test {

    hpx::serialization::output_archive* make_output_archive(
        std::vector<char>& buffer)
    {
        return new hpx::serialization::output_archive(buffer);
    }

    hpx::serialization::input_archive* make_input_archive(
        std::vector<char>& buffer)
    {
        return new hpx::serialization::input_archive(buffer);
    }

    void destroy(hpx::serialization::output_archive* archive) noexcept
    {
        delete archive;
    }

    void destroy(hpx::serialization::input_archive* archive) noexcept
    {
        delete archive;
    }
}    // namespace test
