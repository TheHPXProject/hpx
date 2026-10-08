# Copyright (c) 2014-2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_TOOLS)
  set(subdirs hpxdep inspect)
endif()

if(HPX_WITH_TESTS_BENCHMARKS)
  set(subdirs ${subdirs} perftests_ci)
endif()

foreach(subdir ${subdirs})
  add_hpx_pseudo_target(tools.${subdir})
  add_subdirectory(${subdir})
  add_hpx_pseudo_dependencies(tools tools.${subdir})
endforeach()
