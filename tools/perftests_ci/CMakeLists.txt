# Copyright (c) 2020 ETH Zurich
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

string(TOLOWER "${CMAKE_BUILD_TYPE}" PERFTEST_BUILD_TYPE)

if(HPX_WITH_HIP OR HPX_WITH_CUDA)
  if(TARGET Cuda::cuda)
    set(PYUTILS_COMPILER
        "${CMAKE_CUDA_COMPILER} ${CMAKE_CUDA_COMPILER_VERSION} (${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION})"
    )
  else()
    set(PYUTILS_COMPILER
        "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CUDA_COMPILER} ${CMAKE_CUDA_COMPILER_VERSION})"
    )
  endif()
else()
  set(PYUTILS_COMPILER "${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_VERSION}")
endif()
