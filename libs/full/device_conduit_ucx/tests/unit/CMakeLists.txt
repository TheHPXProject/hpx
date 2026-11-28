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

  # the workers' endpoints with peer error handling as those to other nodes (by
  # default the conduit fails the sends to a removed worker itself)
  add_hpx_unit_test(
    "modules.device_conduit_ucx" locality_removed_same_node_peer
    ${locality_removed_PARAMETERS}
    --hpx:ini=hpx.parcel.conduit.ucx.same_node_error_handling=peer
    EXECUTABLE locality_removed
    PSEUDO_DEPS_NAME locality_removed
  )
  add_hpx_pseudo_dependencies(
    tests.unit.modules.device_conduit_ucx.locality_removed_same_node_peer
    locality_removed_worker
  )
endif()

# host chunks through the conduit, over every parcelport the test runs with
if(HPX_WITH_DEVICE_CONDUIT_UCX)
  add_hpx_executable(
    host_chunks_test INTERNAL_FLAGS
    SOURCES host_chunks.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/DeviceConduitUCX"
  )

  add_hpx_unit_test(
    "modules.device_conduit_ucx" host_chunks
    LOCALITIES 2
    THREADS_PER_LOCALITY 4
    RUN_SERIAL
  )

  # host chunks through the conduit by destination class (the localities of the
  # test on one node): on for the same node, off for other nodes, and the other
  # way round
  add_hpx_executable(
    destination_classes_test INTERNAL_FLAGS
    SOURCES destination_classes.cpp
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/DeviceConduitUCX"
  )

  add_hpx_unit_test(
    "modules.device_conduit_ucx" destination_classes
    LOCALITIES 2
    THREADS_PER_LOCALITY 2
    RUN_SERIAL
  )
  add_hpx_unit_test(
    "modules.device_conduit_ucx" destination_classes_other_nodes
    EXECUTABLE destination_classes
    PSEUDO_DEPS_NAME destination_classes
    LOCALITIES 2
    THREADS_PER_LOCALITY 2
    RUN_SERIAL
    ARGS --same-node-conduit=0
  )

  # the conduit the configuration selects (by default: UCX); the test asks
  # mpi_base whether HPX uses MPI
  if(HPX_WITH_PARCELPORT_MPI OR HPX_WITH_ASYNC_MPI)
    set(configured_conduit_DEPENDENCIES Mpi::mpi)
  endif()
  add_hpx_executable(
    configured_conduit_test INTERNAL_FLAGS
    SOURCES configured_conduit.cpp
    DEPENDENCIES ${configured_conduit_DEPENDENCIES}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Modules/Full/DeviceConduitUCX"
  )

  add_hpx_unit_test(
    "modules.device_conduit_ucx" configured_conduit
    LOCALITIES 2
    THREADS_PER_LOCALITY 2
  )
endif()
