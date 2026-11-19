# Copyright (c)      2025 Aditya Sapra
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples.modules.thrust)
  add_hpx_pseudo_dependencies(examples.modules examples.modules.thrust)
  if((HPX_WITH_CUDA OR HPX_WITH_HIP) AND HPX_WITH_THRUST)
    set(thrust_examples device_fill host_fill device_copy device_transform
                        async_fill_n reverse_reduce
    )

    foreach(example ${thrust_examples})
      set(source ${example}.cu)
      source_group("Source Files" FILES ${source})

      add_hpx_executable(
        ${example} INTERNAL_FLAGS
        SOURCES ${source}
        EXCLUDE_FROM_ALL
        HPX_PREFIX ${HPX_BUILD_PREFIX}
        FOLDER "Examples/Modules/Core/Thrust"
      )

      add_hpx_pseudo_dependencies(examples.modules.thrust ${example})
    endforeach()
  endif()
  if(HPX_WITH_TESTS AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples.modules.thrust)
    add_hpx_pseudo_dependencies(
      tests.examples.modules tests.examples.modules.thrust
    )
  endif()
endif()
