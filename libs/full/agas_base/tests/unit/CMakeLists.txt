# Copyright (c) 2020 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests agas_rpc_timeout agas_rpc_timeout_ini)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Full/AgasBase")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
    DEPENDENCIES hpx_agas_base
  )

  add_hpx_unit_test("modules.agas_base" ${test} ${${test}_PARAMETERS})
  add_hpx_pseudo_dependencies(tests.unit.modules.agas_base ${test}_test)
endforeach()
