# Copyright (c) 2026 The STE||AR Group
# Copyright (c) 2026 Vansh Dobhal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_Message)
include(HPX_Option)

if(HPX_WITH_TESTS)
  if(HPX_WITH_TESTS_UNIT)
    add_hpx_pseudo_target(tests.unit.modules.tracy)
    add_hpx_pseudo_dependencies(tests.unit.modules tests.unit.modules.tracy)
    add_subdirectory(unit)
  endif()
endif()
