# Copyright (c) 2007-2015 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# add subdirectories
set(subdirs algorithms block container_algorithms task_sizes unseq_algorithms)

if(HPX_WITH_DATAPAR)
  set(subdirs ${subdirs} datapar_algorithms)
endif()

foreach(subdir ${subdirs})
  add_hpx_pseudo_target(tests.unit.modules.algorithms.${subdir})
  add_subdirectory(${subdir})
  add_hpx_pseudo_dependencies(
    tests.unit.modules.algorithms tests.unit.modules.algorithms.${subdir}
  )
endforeach()
