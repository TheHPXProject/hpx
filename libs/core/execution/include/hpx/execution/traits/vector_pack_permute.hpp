//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Lane-wise minimum and maximum of two packs, and a pack built lane by lane
// from a generator called with each lane's index as a compile-time constant
// (to permute and blend lanes):
//
//   hpx::parallel::traits::vector_pack_min(a, b)
//   hpx::parallel::traits::vector_pack_max(a, b)
//   hpx::parallel::traits::vector_pack_generate<Vector>(gen)
//
// gen(std::integral_constant<std::size_t, I>{}) returns the value of lane I.
// The backends std::experimental::simd (HPX_HAVE_DATAPAR_STD_EXPERIMENTAL_SIMD),
// the emulation (HPX_HAVE_DATAPAR_EMULATION) and EVE (HPX_HAVE_DATAPAR_EVE)
// provide these; Vc and SVE don't.

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)

#if !defined(__CUDACC__)
#include <hpx/execution/traits/detail/eve/vector_pack_permute.hpp>
#include <hpx/execution/traits/detail/simd/vector_pack_permute.hpp>
#endif

#endif
