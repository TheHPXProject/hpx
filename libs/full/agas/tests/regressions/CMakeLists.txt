# Copyright (c) 2020-2022 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests duplicate_id_registration_1596 id_type_ref_counting_1032
          pass_by_value_id_type_action send_gid_keep_component_1624
)

if(HPX_WITH_NETWORKING)
  set(tests ${tests} register_with_basename_1804)
  set(register_with_basename_1804_PARAMETERS LOCALITIES 2)
endif()

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_TCP)
  set(tests ${tests} departed_locality_7384)

  # add executable needed for the departed_locality_7384 test
  add_hpx_executable(
    departed_locality_7384_child INTERNAL_FLAGS
    SOURCES departed_locality_7384_child.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/AGAS"
  )

  set(departed_locality_7384_FLAGS DEPENDENCIES process_component)
  set(departed_locality_7384_PARAMETERS
      RUN_SERIAL
      --launch=$<TARGET_FILE:departed_locality_7384_child>
      --hpx:expect-connecting-localities
      # Force use of the TCP parcelport
      --hpx:ini=hpx.parcel.tcp.priority=1000
      --hpx:ini=hpx.parcel.bootstrap=tcp
  )

  set(tests ${tests} os_assigned_parcelport_port_7406)

  # add executable needed for the os_assigned_parcelport_port_7406 test
  add_hpx_executable(
    os_assigned_parcelport_port_7406_child INTERNAL_FLAGS
    SOURCES os_assigned_parcelport_port_7406_child.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/AGAS"
  )

  set(os_assigned_parcelport_port_7406_FLAGS DEPENDENCIES process_component)
  set(os_assigned_parcelport_port_7406_PARAMETERS
      RUN_SERIAL
      --launch=$<TARGET_FILE:os_assigned_parcelport_port_7406_child>
      --hpx:expect-connecting-localities
      # Force use of the TCP parcelport
      --hpx:ini=hpx.parcel.tcp.priority=1000
      --hpx:ini=hpx.parcel.bootstrap=tcp
  )
endif()

set(id_type_ref_counting_1032_PARAMETERS THREADS_PER_LOCALITY 1)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Regressions/Modules/Full/AGAS"
  )

  add_hpx_regression_test("modules.agas" ${test} ${${test}_PARAMETERS})
endforeach()

add_hpx_regression_test(
  "" id_type_ref_counting_1032_4
  EXECUTABLE id_type_ref_counting_1032
  PSEUDO_DEPS_NAME id_type_ref_counting_1032
  THREADS_PER_LOCALITY 4
)

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_TCP)
  add_hpx_pseudo_dependencies(
    tests.regressions.modules.agas.departed_locality_7384
    departed_locality_7384_child
  )
  add_hpx_pseudo_dependencies(
    tests.regressions.modules.agas.os_assigned_parcelport_port_7406
    os_assigned_parcelport_port_7406_child
  )
endif()
