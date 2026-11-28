//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The packs of the native and of the fixed size policies must be recognized
// as vector packs. If they aren't, the datapar algorithms silently take their
// scalar path for (par_)fixed_size_simd<N>, which only shows in the timings:
// with libstdc++ the fixed size packs weren't recognized, because the N of
// simd_abi::fixed_size is an int there and the trait deduced a std::size_t.

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>

// the std::experimental::simd based backends (std, emulation, SVE); Vc and EVE
// map the packs to their own types
#if defined(HPX_HAVE_DATAPAR_EXPERIMENTAL_SIMD)
using hpx::parallel::traits::is_scalar_vector_pack_v;
using hpx::parallel::traits::is_vector_pack_v;
using hpx::parallel::traits::vector_pack_type_t;

template <typename T, std::size_t N = 0>
constexpr bool is_pack = is_vector_pack_v<vector_pack_type_t<T, N>> &&
    !is_scalar_vector_pack_v<vector_pack_type_t<T, N>>;

static_assert(!is_vector_pack_v<int>);
static_assert(is_scalar_vector_pack_v<int>);

// the native packs
static_assert(is_pack<int>);
static_assert(is_pack<double>);

// fixed size packs
static_assert(is_pack<int, 8>);
static_assert(is_pack<std::int64_t, 4>);
static_assert(is_pack<float, 16>);
static_assert(is_pack<std::uint8_t, 32>);
#endif

int hpx_main()
{
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
