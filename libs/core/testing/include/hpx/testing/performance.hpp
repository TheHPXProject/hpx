//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/command_line_handling_local.hpp>
#include <hpx/modules/functional.hpp>

#include <cstddef>
#include <string>

namespace hpx::util {
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT inline bool detailed_;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT inline bool print_cdash_img;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT inline std::string test_name_;
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void perftests_cfg(
        hpx::program_options::options_description& cmdline);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void perftests_init(
        hpx::program_options::variables_map const& vm, const std::string test_name);
#if defined(HPX_HAVE_NANOBENCH)
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void perftests_report(
        std::string const& name, std::string const& exec,
        std::size_t const steps, hpx::function<void()>&& test);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void perftests_print_times(
        std::ostream& strm, char const* templ);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void perftests_print_times(
        std::ostream& strm);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void perftests_print_times();
#else
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void perftests_report(
        std::string const& name, std::string const& exec,
        std::size_t const steps, hpx::function<void()>&& test);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void perftests_print_times();
#endif
}    // namespace hpx::util
