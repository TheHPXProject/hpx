//  Copyright (c)      2020 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/functional.hpp>
#include <hpx/modules/program_options.hpp>

/// \cond NOINTERNAL
namespace hpx {

    struct init_params;
}

namespace hpx::detail {

    HPX_CXX_EXPORT HPX_EXPORT int run_or_start(
        hpx::function<int(hpx::program_options::variables_map& vm)> const& f,
        int argc, char** argv, init_params const& params, bool blocking);

    HPX_CXX_EXPORT HPX_EXPORT int init_impl(
        hpx::function<int(hpx::program_options::variables_map&)> const& f,
        int argc, char** argv, init_params const& params,
        char const* hpx_prefix, char** env);

    HPX_CXX_EXPORT HPX_EXPORT bool start_impl(
        hpx::function<int(hpx::program_options::variables_map&)> const& f,
        int argc, char** argv, init_params const& params,
        char const* hpx_prefix, char** env);
}    // namespace hpx::detail
/// \endcond
