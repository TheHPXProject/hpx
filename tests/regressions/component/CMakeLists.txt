# Copyright (c) 2007-2020 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    agas_background_gc_7358
    bulk_new_3054
    client_base_registration
    create_n_components_2323
    moveonly_constructor_arguments_1405
    new_2848
    new_4369
    returned_client_2150
)

if(HPX_WITH_NETWORKING)
  set(tests ${tests} create_remote_component_2334)
  set(bulk_new_3054_PARAMETERS LOCALITIES 2)
  set(create_remote_component_2334_PARAMETERS LOCALITIES 2)
  set(new_2848_PARAMETERS LOCALITIES 2)
  set(new_4349_PARAMETERS LOCALITIES 4)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Components" ${HPX_WITH_CXX_MODULES_OPTION}
  )

  add_hpx_regression_test("components" ${test} ${${test}_PARAMETERS})

endforeach()
