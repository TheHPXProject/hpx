# Copyright (c) 2007-2012 Hartmut Kaiser
# Copyright (c) 2011-2013 Thomas Heller
# Copyright (c) 2007-2008 Chirag Dekate
# Copyright (c)      2011 Bryce Lelbach
# Copyright (c)      2011 Vinay C Amatya
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_AddDefinitions)

# In case find_package(HPX) is called multiple times
if(NOT TARGET hpx_dependencies_allocator)

  if(NOT HPX_WITH_MALLOC)
    set(HPX_WITH_MALLOC
        CACHE
          STRING
          "Use the specified allocator. Supported allocators are tcmalloc, jemalloc, tbbmalloc and system."
          ${DEFAULT_MALLOC}
    )
    set(allocator_error
        "The default allocator for your system is ${DEFAULT_MALLOC}, but ${DEFAULT_MALLOC} could not be found. "
        "The system allocator has poor performance. As such ${DEFAULT_MALLOC} is a strong optional requirement. "
        "Being aware of the performance hit, you can override this default and get rid of this dependency by setting -DHPX_WITH_MALLOC=system. "
        "Valid options for HPX_WITH_MALLOC are: system, tcmalloc, jemalloc, mimalloc, tbbmalloc, and custom"
    )
  else()
    set(allocator_error
        "HPX_WITH_MALLOC was set to ${HPX_WITH_MALLOC}, but ${HPX_WITH_MALLOC} could not be found. "
        "Valid options for HPX_WITH_MALLOC are: system, tcmalloc, jemalloc, mimalloc, tbbmalloc, and custom"
    )
  endif()

  string(TOUPPER "${HPX_WITH_MALLOC}" HPX_WITH_MALLOC_UPPER)

  add_library(hpx_dependencies_allocator INTERFACE IMPORTED)

  if(NOT HPX_WITH_MALLOC_DEFAULT)

    # ##########################################################################
    # TCMALLOC
    if("${HPX_WITH_MALLOC_UPPER}" STREQUAL "TCMALLOC")
      find_package(TCMalloc)
      if(NOT Tcmalloc_LIBRARIES)
        hpx_error(${allocator_error})
      endif()

      target_link_libraries(
        hpx_dependencies_allocator INTERFACE ${Tcmalloc_LIBRARIES}
      )

      if(MSVC)
        target_compile_options(
          hpx_dependencies_allocator INTERFACE /INCLUDE:__tcmalloc
        )
      endif()
      set(_use_custom_allocator TRUE)
    endif()

    # ##########################################################################
    # JEMALLOC
    if("${HPX_WITH_MALLOC_UPPER}" STREQUAL "JEMALLOC")
      find_package(Jemalloc)
      if(NOT Jemalloc_LIBRARIES)
        hpx_error(${allocator_error})
      endif()
      target_include_directories(
        hpx_dependencies_allocator INTERFACE ${Jemalloc_INCLUDE_DIR}
                                             ${Jemalloc_ADDITIONAL_INCLUDE_DIR}
      )
      target_link_libraries(
        hpx_dependencies_allocator INTERFACE ${Jemalloc_LIBRARIES}
      )
    endif()

    # ##########################################################################
    # MIMALLOC
    if("${HPX_WITH_MALLOC_UPPER}" STREQUAL "MIMALLOC")
      find_package(mimalloc)
      if(NOT mimalloc_FOUND)
        hpx_error(${allocator_error})
      endif()
      target_link_libraries(hpx_dependencies_allocator INTERFACE mimalloc)
      set(hpx_MALLOC_LIBRARY mimalloc)
      if(MSVC)
        target_compile_options(
          hpx_dependencies_allocator INTERFACE /INCLUDE:mi_version
        )
      endif()
      set(_use_custom_allocator TRUE)
    endif()

    # ##########################################################################
    # TBBMALLOC
    if("${HPX_WITH_MALLOC_UPPER}" STREQUAL "TBBMALLOC")
      find_package(TBBmalloc)
      if(NOT Tbbmalloc_LIBRARY AND NOT Tbbmalloc_PROXY_LIBRARY)
        hpx_error(${allocator_error})
      endif()
      if(MSVC)
        target_compile_options(
          hpx_dependencies_allocator INTERFACE /INCLUDE:__TBB_malloc_proxy
        )
      endif()
      target_link_libraries(
        hpx_dependencies_allocator INTERFACE ${Tbbmalloc_LIBRARY}
                                             ${Tbbmalloc_PROXY_LIBRARY}
      )
    endif()

    if("${HPX_WITH_MALLOC_UPPER}" STREQUAL "CUSTOM")
      set(_use_custom_allocator TRUE)
    endif()

  else()

    set(HPX_WITH_MALLOC ${HPX_WITH_MALLOC_DEFAULT})

  endif(NOT HPX_WITH_MALLOC_DEFAULT)

  if("${HPX_WITH_MALLOC_UPPER}" MATCHES "SYSTEM")
    if(NOT MSVC)
      hpx_warn(
        "HPX will perform poorly without tcmalloc, jemalloc, or mimalloc. See docs for more info."
      )
    endif()
    set(_use_custom_allocator FALSE)
  endif()

  hpx_info("Using ${HPX_WITH_MALLOC} allocator.")

  # convey selected allocator type to the build configuration
  hpx_add_config_define(HPX_HAVE_MALLOC "\"${HPX_WITH_MALLOC}\"")
  if(${HPX_WITH_MALLOC} STREQUAL "jemalloc")
    if(NOT ("${HPX_WITH_JEMALLOC_PREFIX}" STREQUAL "<none>")
       AND NOT ("${HPX_WITH_JEMALLOC_PREFIX}x" STREQUAL "x")
    )
      hpx_add_config_define(
        HPX_HAVE_JEMALLOC_PREFIX ${HPX_WITH_JEMALLOC_PREFIX}
      )
    endif()

  endif()

endif()
