//  Copyright (c) 2026 adhithyaragavan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for #7287: vector_pack_load<V, T>::unaligned used to
// broadcast a single scalar (*iter) across all lanes of the pack instead of
// loading the N consecutive elements starting at that address. The
// symmetric bug existed in vector_pack_store<V, T>::unaligned, which wrote
// the same broadcast pattern instead of storing each lane back to its own
// position. Both are fixed for the EVE and std-experimental-simd backends.
//
// This test loads a full-width, non-uniform pack via ::unaligned, checks
// every lane against the source data (a broadcast bug would make every
// lane equal to data[0]), flips the sign of each lane, stores the pack back
// via ::unaligned, and checks every element in memory was updated
// independently (a broadcast bug would write the same value everywhere).

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <vector>

int hpx_main()
{
    using value_type = int;
    using V = hpx::parallel::traits::vector_pack_type_t<value_type>;

    std::size_t const width = hpx::parallel::traits::vector_pack_size_v<V>;

    // Non-uniform, strictly increasing data -- a broadcast bug would
    // collapse every lane to data[0].
    std::vector<value_type> data(width);
    for (std::size_t i = 0; i != width; ++i)
    {
        data[i] = static_cast<value_type>(i + 1);
    }

    auto it = data.begin();
    V pack =
        hpx::parallel::traits::vector_pack_load<V, value_type>::unaligned(it);

    for (std::size_t i = 0; i != width; ++i)
    {
        HPX_TEST_EQ(hpx::parallel::traits::get(pack, i), data[i]);
    }

    // Negate each lane and store back; a broadcast bug would overwrite the
    // whole range with a single (repeated) value instead of updating each
    // element independently.
    for (std::size_t i = 0; i != width; ++i)
    {
        hpx::parallel::traits::set(
            pack, i, -hpx::parallel::traits::get(pack, i));
    }

    hpx::parallel::traits::vector_pack_store<V, value_type>::unaligned(
        pack, it);

    for (std::size_t i = 0; i != width; ++i)
    {
        HPX_TEST_EQ(data[i], -static_cast<value_type>(i + 1));
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
