# Copyright (c) 2019 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(MSVC
   AND HPX_WITH_STATIC_LINKING
   AND NOT HPX_WITH_CXX_MODULES
)
  set(tests autolink)

  set(autolink_SOURCES autolink.cpp autolink_exports.cpp
                       autolink_no_autolink.cpp autolink_no_lib.cpp
  )

  foreach(test ${tests})
    if(DEFINED ${test}_SOURCES)
      set(sources ${${test}_SOURCES})
    else()
      set(sources ${test}.cpp)
    endif()

    source_group("Source Files" FILES ${sources})

    set(folder_name "Tests/Unit/Modules/Core/Config")

    add_hpx_executable(
      ${test}_test INTERNAL_FLAGS
      SOURCES ${sources}
      EXCLUDE_FROM_ALL
      FOLDER ${folder_name}
    )

    add_hpx_unit_test("modules.config" ${test} ${${test}_PARAMETERS})
  endforeach()
endif()
