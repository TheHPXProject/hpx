# Copyright (c) 2026 TheHPXProject
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# hpx_tune: measures the machine and writes HPX settings for it as an ini file,
# by area (see the tuning module, libs/core/tuning)

# the parcel area starts the localities of its measurements with hpxrun.py
if(NOT (Python_FOUND OR PYTHONINTERP_FOUND))
  hpx_error(
    "hpx_tune needs a Python interpreter: it starts the localities of its "
    "measurements with hpxrun.py (configure with HPX_WITH_TOOLS=OFF to build "
    "without the tools)"
  )
endif()

add_hpx_executable(
  hpx_tune INTERNAL_FLAGS
  SOURCES area.cpp child.cpp hpx_tune.cpp idle_area.cpp
  HEADERS area.hpp child.hpp idle_area.hpp
  NOLIBS
  FOLDER "Tools/Tuners"
)

target_include_directories(hpx_tune PRIVATE ${PROJECT_BINARY_DIR})
target_link_libraries(hpx_tune PRIVATE hpx_core)
# the parcel area runs children of two localities (the full runtime, without
# wrapping main: the tool's own process doesn't start the runtime)
if(HPX_WITH_DISTRIBUTED_RUNTIME AND HPX_WITH_NETWORKING)
  target_sources(hpx_tune PRIVATE parcel_area.cpp parcel_area.hpp)
  target_link_libraries(hpx_tune PRIVATE HPX::hpx)
  # where hpxrun.py is: with multi-configuration generators the tool is in
  # <build>/<config>/bin, hpxrun.py in <build>/bin
  target_compile_definitions(
    hpx_tune
    PRIVATE
      HPX_TUNE_PYTHON="${Python_EXECUTABLE}"
      HPX_TUNE_HPXRUN_BUILD="${PROJECT_BINARY_DIR}/bin/hpxrun.py"
      HPX_TUNE_HPXRUN_INSTALL="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/hpxrun.py"
  )
endif()

add_hpx_pseudo_dependencies(tools.tuners hpx_tune)

# the smoke tests start the tool and HPX applications: only on request
if(HPX_WITH_TESTS
   AND HPX_WITH_TESTS_UNIT
   AND HPX_WITH_TUNING_TESTS
)
  add_subdirectory(tests)
endif()
