# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Finds UCX for the UCX device conduit (PkgConfig::UCX). It must be the UCX
# library of the MPI library if that is UCX based (one process can't use two
# different UCX libraries): look next to MPI first, then UCX_ROOT, then the
# default pkg-config paths. HPX_UCX_ROOT remembers the UCX that was found (for
# projects using an installed HPX).
macro(hpx_setup_ucx)
  if(NOT TARGET PkgConfig::UCX)
    find_package(PkgConfig REQUIRED)

    set(_hpx_ucx_pkgconfig_dirs)
    foreach(_hpx_ucx_root ${HPX_UCX_ROOT} ${UCX_ROOT} $ENV{UCX_ROOT}
                          ${MPI_HOME} $ENV{MPI_HOME}
    )
      list(APPEND _hpx_ucx_pkgconfig_dirs "${_hpx_ucx_root}/lib/pkgconfig"
           "${_hpx_ucx_root}/lib64/pkgconfig"
      )
    endforeach()

    set(_hpx_pkg_config_path "$ENV{PKG_CONFIG_PATH}")
    string(REPLACE ";" ":" _hpx_ucx_pkgconfig_path "${_hpx_ucx_pkgconfig_dirs}")
    set(ENV{PKG_CONFIG_PATH}
        "${_hpx_ucx_pkgconfig_path}:${_hpx_pkg_config_path}"
    )
    pkg_check_modules(UCX REQUIRED IMPORTED_TARGET GLOBAL ucx)
    set(ENV{PKG_CONFIG_PATH} "${_hpx_pkg_config_path}")

    set(HPX_UCX_ROOT
        "${UCX_PREFIX}"
        CACHE INTERNAL "" FORCE
    )
  endif()
endmacro()
