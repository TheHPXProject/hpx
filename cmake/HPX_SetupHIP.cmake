# Copyright (c)      2020 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

if(HPX_WITH_HIP AND NOT TARGET roc::hipblas)

  if(HPX_WITH_CUDA)
    hpx_error(
      "Both HPX_WITH_CUDA and HPX_WITH_HIP are ON. Please choose one of \
    them for HPX to work properly"
    )
  endif(HPX_WITH_CUDA)

  # Needed on rostam
  list(APPEND CMAKE_PREFIX_PATH $ENV{HIP_PATH}/lib/cmake/hip)
  list(APPEND CMAKE_PREFIX_PATH $ENV{DEVICE_LIB_PATH}/cmake/AMDDeviceLibs)
  list(APPEND CMAKE_PREFIX_PATH $ENV{DEVICE_LIB_PATH}/cmake/amd_comgr)
  list(APPEND CMAKE_PREFIX_PATH $ENV{DEVICE_LIB_PATH}/cmake/hsa-runtime64)
  # Setup hipblas (creates roc::hipblas)
  find_package(hipblas HINTS $ENV{HIPBLAS_ROOT} CONFIG)
  if(NOT hipblas_FOUND)
    hpx_warn(
      "Hipblas could not be found, the blas parts will therefore be disabled.\n\
      You can reconfigure specifying HIPBLAS_ROOT environment variable to enable hipblas"
    )
    set(HPX_WITH_GPUBLAS OFF)
  else()
    set(HPX_WITH_GPUBLAS ON)
    if(NOT HPX_FIND_PACKAGE)
      hpx_add_config_define(HPX_HAVE_GPUBLAS)
    endif()
  endif()

  if(NOT HPX_FIND_PACKAGE)
    # The cmake variables are supposed to be cached no need to redefine them
    set(HPX_WITH_COMPUTE ON)
    hpx_add_config_define(HPX_HAVE_COMPUTE)
    hpx_add_config_define(HPX_HAVE_HIP)
  endif()

endif()
