# Copyright (c) 2020-2026 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.modules.execution_distributed)
  add_hpx_pseudo_dependencies(
    examples.modules examples.modules.execution_distributed
  )
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.modules.execution_distributed)
    add_hpx_pseudo_dependencies(
      tests.examples.modules tests.examples.modules.execution_distributed
    )
  endif()
endif()
