# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# the rules, from the measurements of the parcelport study
set(tests parcel_settings peer_nodes)

# the transfers between two localities
set(transfers_PARAMETERS LOCALITIES 2 THREADS_PER_LOCALITY 2)
set(tests ${tests} transfers)

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  set(folder_name "Tests/Unit/Modules/Full/ParcelTuning")

  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    FOLDER ${folder_name}
  )

  add_hpx_unit_test("modules.parcel_tuning" ${test} ${${test}_PARAMETERS})
endforeach()
