# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Localities that leave while the conduit moves data to them: the worker
# localities connect to the test (hpx::force_disconnect needs supervision)
if(HPX_WITH_DEVICE_CONDUIT_UCX
   AND HPX_WITH_SUPERVISION
   AND HPX_WITH_PARCELPORT_TCP
)
  add_hpx_executable(
    locality_removed_worker INTERNAL_FLAGS
    SOURCES locality_removed_worker.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/DeviceConduitUCX"
  )

  add_hpx_executable(
    locality_removed_test INTERNAL_FLAGS
    SOURCES locality_removed.cpp
    DEPENDENCIES process_component supervision_dispatch_component
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/DeviceConduitUCX"
  )

  # cmake-format: off
  set(locality_removed_PARAMETERS
      RUN_SERIAL THREADS_PER_LOCALITY 4
      --launch=$<TARGET_FILE:locality_removed_worker>
      # the TCP parcelport, which connecting localities use
      --hpx:ini=hpx.parcel.tcp.priority=1000
      --hpx:ini=hpx.parcel.bootstrap=tcp
  )
  # cmake-format: on

  add_hpx_unit_test(
    "modules.device_conduit_ucx" locality_removed
    ${locality_removed_PARAMETERS}
  )
  add_hpx_pseudo_dependencies(
    tests.unit.modules.device_conduit_ucx.locality_removed
    locality_removed_worker
  )
endif()
