# Copyright (c) 2020-2023 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.modules.supervision)
  add_hpx_pseudo_dependencies(examples.modules examples.modules.supervision)
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.modules.supervision)
    add_hpx_pseudo_dependencies(
      tests.examples.modules tests.examples.modules.supervision
    )
  endif()
endif()
