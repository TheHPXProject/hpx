# Copyright (c)      2017 Thomas Heller
# Copyright (c)      2023 Christopher Taylor
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

macro(hpx_setup_gasnet)

  if(NOT TARGET PkgConfig::GASNET)

    find_package(PkgConfig REQUIRED QUIET COMPONENTS)
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)

    # Known-good build recipe for the PAR/mpi conduit this parcelport needs.
    # Defined up front so it can be reported both when the .pc file cannot be
    # found and when a found .pc turns out to be misconfigured.  GASNet emits
    # a static archive (libgasnet-mpi-par.a); -fPIC is what makes it linkable
    # into HPX's shared objects, so the compile flags are essential here.
    string(
      CONCAT
      GASNET_REBUILD_MESSAGE
      "Build GASNet as follows and make the .pc files visible via "
      "PKG_CONFIG_PATH or CMAKE_PREFIX_PATH:\n"
      "  export CXXFLAGS=-fPIC -O3; export CFLAGS=-fPIC -O3;\n"
      "  PMI_LIBS=probe CFLAGS=-fPIC CCFLAGS=-fPIC CXXFLAGS=-fPIC\n"
      "  ./configure --enable-par --disable-ucx --enable-mpi \\\n"
      "    --with-c-compiler=mpicc --with-cxx-compiler=mpicxx \\\n"
      "    --with-mpi=/opt/openmpi --enable-hwloc \\\n"
      "    --prefix=/opt/gasnet-par --with-cflags=-fPIC \\\n"
      "    --with-cxxflags=-fPIC --disable-udp --disable-ibv \\\n"
      "    --enable-pmi --with-pmi-home=/opt/pmix \\\n"
      "    --enable-segment-large --with-ldflags=-fPIC \\\n"
      "    --with-mpi-cflags=-fPIC && make -j4 && sudo make install\n"
      "  export PKG_CONFIG_PATH=/opt/ucx/lib/pkgconfig:"
      "/opt/openmpi/lib/pkgconfig:/opt/gasnet-par/lib/pkgconfig:$PKG_CONFIG_PATH"
    )

    if(GASNet_ROOT AND NOT "${GASNet_ROOT}" IN_LIST CMAKE_PREFIX_PATH)
      list(PREPEND CMAKE_PREFIX_PATH "${GASNet_ROOT}")
    endif()

    pkg_search_module(
      GASNET IMPORTED_TARGET GLOBAL
      gasnet-${HPX_WITH_PARCELPORT_GASNET_CONDUIT}-par
    )

    if(NOT GASNET_FOUND)
      message(
        FATAL_ERROR
          "GASNet (conduit '${HPX_WITH_PARCELPORT_GASNET_CONDUIT}', PAR "
          "threading mode) not found: no pkg-config module "
          "'gasnet-${HPX_WITH_PARCELPORT_GASNET_CONDUIT}-par'. Either the "
          "GASNet .pc directory is not on PKG_CONFIG_PATH/"
          "CMAKE_PREFIX_PATH, or GASNet was never built. "
          "${GASNET_REBUILD_MESSAGE}"
      )
    endif()

    if("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
      set(GASNET_MPI_FOUND TRUE)
      include(HPX_SetupMPI)
      hpx_setup_mpi()
      target_link_libraries(PkgConfig::GASNET INTERFACE Mpi::mpi)
    endif()

    # Validate the GASNet build configuration. HPX requires a GASNet built in
    # PAR threading mode with the mpi conduit, with position-independent code
    # (GASNet is consumed as a static archive; -fPIC is what makes it
    # linkable into HPX's shared objects), on top of a UCX built with
    # multithreading support.  The library artefacts themselves are whatever
    # the gasnet-<conduit>-par .pc resolves to, so only the pkg-config metadata
    # is validated here.

    set(GASNET_PC_NAME "gasnet-${HPX_WITH_PARCELPORT_GASNET_CONDUIT}-par")

    foreach(_var IN ITEMS GASNET_DEFINES GASNET_CFLAGS GASNET_LDFLAGS
                       GASNET_LIBS GASNET_INCLUDES GASNET_CC GASNET_CXX)
      execute_process(
        COMMAND "${PKG_CONFIG_EXECUTABLE}" --variable=${_var}
                "${GASNET_PC_NAME}"
        RESULT_VARIABLE GASNET_BUILD_PC_RESULT
        OUTPUT_VARIABLE GASNET_BUILD_${_var}
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    endforeach()
    execute_process(
      COMMAND "${PKG_CONFIG_EXECUTABLE}" --variable=GASNET_SPAWNER_PMI
              "${GASNET_PC_NAME}"
      RESULT_VARIABLE GASNET_BUILD_PC_RESULT
      OUTPUT_VARIABLE GASNET_BUILD_SPAWNER_PMI
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )

    # hard requirement: PAR threading mode
    string(REGEX MATCH "-DGASNET_PARSYNC([ ]|$)" GASNET_BUILD_PARSYNC
           "${GASNET_BUILD_GASNET_DEFINES}"
    )
    string(REGEX MATCH "-DGASNET_SEQ([ ]|$)" GASNET_BUILD_SEQ
           "${GASNET_BUILD_GASNET_DEFINES}"
    )
    string(REGEX MATCH "-DGASNET_PAR([ ]|$)" GASNET_BUILD_PAR
           "${GASNET_BUILD_GASNET_DEFINES}"
    )
    if(GASNET_BUILD_PARSYNC)
      message(
        FATAL_ERROR
          "GASNet was built in PARSYNC threading mode, but HPX requires PAR "
          "threading mode (concurrent access from multiple threads). "
          "${GASNET_REBUILD_MESSAGE}"
      )
    elseif(GASNET_BUILD_SEQ)
      message(
        FATAL_ERROR
          "GASNet was built in SEQ threading mode, but HPX requires PAR "
          "threading mode (concurrent access from multiple threads). "
          "${GASNET_REBUILD_MESSAGE}"
      )
    elseif(NOT GASNET_BUILD_PAR)
      message(
        FATAL_ERROR
          "Unable to determine the GASNet threading mode from "
          "'${GASNET_BUILD_GASNET_DEFINES}'. ${GASNET_REBUILD_MESSAGE}"
      )
    endif()

    # hard requirement (mpi conduit only): mpi conduit present
    if("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
      string(FIND "${GASNET_BUILD_GASNET_LIBS}" "gasnet-mpi" GASNET_BUILD_LIB)
      string(FIND "${GASNET_BUILD_GASNET_INCLUDES}" "mpi-conduit"
             GASNET_BUILD_INC
      )
      if(GASNET_BUILD_LIB EQUAL -1 OR GASNET_BUILD_INC EQUAL -1)
        message(
          FATAL_ERROR
            "'${GASNET_PC_NAME}' does not provide the mpi conduit (expected "
            "lib 'gasnet-mpi' and include dir 'mpi-conduit'). "
            "${GASNET_REBUILD_MESSAGE}"
        )
      endif()
    endif()

# hard requirement: position-independent code
    set(GASNET_BUILD_FLAGS "${GASNET_BUILD_GASNET_CFLAGS} \
${GASNET_BUILD_GASNET_LDFLAGS} ${GASNET_BUILD_GASNET_CC} \
${GASNET_BUILD_GASNET_CXX}")
    string(FIND "${GASNET_BUILD_FLAGS}" "-fPIC" GASNET_BUILD_PIC)
    if(GASNET_BUILD_PIC EQUAL -1)
      message(
        FATAL_ERROR
          "GASNet was built without -fPIC. HPX requires a position-"
          "independent GASNet build. ${GASNET_REBUILD_MESSAGE}"
      )
    endif()

    # Hard requirement under the mpi conduit: UCX built with multithreading.
    #
    # This is NOT GASNet's own ucx conduit -- the rebuild recipe above passes
    # --disable-ucx on purpose, since we drive GASNet through OpenMPI's mpi
    # conduit instead.  The UCX we require here is the one OpenMPI itself was
    # built against (hence /opt/ucx on PKG_CONFIG_PATH): gasnet-mpi links
    # through libmpi, so an MPI built on UCX pulls it in transitively and it
    # has to be MT-capable for HPX's threading model.  Do not "fix" this by
    # relaxing --disable-ucx or by dropping the check below; they are unrelated
    # knobs.
    if("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
      execute_process(
        COMMAND "${PKG_CONFIG_EXECUTABLE}" --variable=prefix ucx
        RESULT_VARIABLE GASNET_BUILD_PC_RESULT
        OUTPUT_VARIABLE GASNET_UCX_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      if(NOT GASNET_BUILD_PC_RESULT EQUAL 0 OR NOT GASNET_UCX_PREFIX)
        message(
          FATAL_ERROR
            "pkg-config could not locate the 'ucx' package. UCX is required "
            "under the GASNet mpi conduit. Build it as follows and add "
            "/opt/ucx/lib/pkgconfig to PKG_CONFIG_PATH:\n"
            "  ./configure --prefix=/opt/ucx --enable-mt\n"
            "  && make && sudo make install"
        )
      endif()
      find_program(
        GASNET_UCX_INFO ucx_info
        HINTS "${GASNET_UCX_PREFIX}/bin"
        NO_CACHE
      )
      if(NOT GASNET_UCX_INFO)
        message(
          FATAL_ERROR
            "Could not find the 'ucx_info' tool for the UCX installation at "
            "'${GASNET_UCX_PREFIX}'. It is needed to verify that UCX was "
            "built with multithreading support."
        )
      endif()
      execute_process(
        COMMAND "${GASNET_UCX_INFO}" -b
        RESULT_VARIABLE GASNET_UCX_INFO_RESULT
        OUTPUT_VARIABLE GASNET_UCX_BUILDINFO
        ERROR_QUIET
      )
      string(REGEX MATCH "#define[ \t]+ENABLE_MT[ \t]+1" GASNET_UCX_MT
             "${GASNET_UCX_BUILDINFO}"
      )
      if(NOT GASNET_UCX_INFO_RESULT EQUAL 0 OR NOT GASNET_UCX_MT)
        message(
          FATAL_ERROR
            "UCX at '${GASNET_UCX_PREFIX}' was not built with "
            "multithreading support ('ucx_info -b' does not report "
            "ENABLE_MT 1). HPX requires UCX built with --enable-mt:\n"
            "  ./configure --prefix=/opt/ucx --enable-mt\n"
            "  && make && sudo make install"
        )
      endif()
    endif()

    # informational: hwloc and PMI support as built
    string(FIND "${GASNET_BUILD_GASNET_LIBS} ${GASNET_BUILD_GASNET_INCLUDES}"
           "hwloc" GASNET_BUILD_HWLOC
    )
    if(GASNET_BUILD_HWLOC EQUAL -1)
      message(STATUS "GASNet built without hwloc support.")
    else()
      message(STATUS "GASNet built with hwloc support.")
    endif()
    if(GASNET_BUILD_SPAWNER_PMI STREQUAL "1")
      message(STATUS "GASNet built with PMI spawner support.")
    else()
      message(STATUS "GASNet built without PMI spawner support.")
    endif()

    # GASNet's .pc emits GCC's '--param <name>=<value>' as two whitespace-
    # separated tokens.  pkg_check_module splits Cflags on whitespace, so the
    # bare value ('inline-unit-growth=10000', 'large-function-growth=200000')
    # becomes a separate list element and is passed to the compiler as a
    # positional input filename -- g++ then fails with "linker input file not
    # found".  Re-join each such pair into a single SHELL: token so it reaches
    # the compiler as one flag.
    if(GASNET_CFLAGS)
      set(GASNET_BUILD_CFLAGS "")
      set(GASNET_BUILD_PARAM "")
      foreach(_flag IN LISTS GASNET_CFLAGS)
        if(GASNET_BUILD_PARAM)
          list(APPEND GASNET_BUILD_CFLAGS
               "SHELL:${GASNET_BUILD_PARAM} ${_flag}")
          set(GASNET_BUILD_PARAM "")
        elseif("${_flag}" STREQUAL "--param" OR "${_flag}" STREQUAL "-param")
          set(GASNET_BUILD_PARAM "${_flag}")
        else()
          list(APPEND GASNET_BUILD_CFLAGS "${_flag}")
        endif()
      endforeach()
      # A trailing '--param' with no value would otherwise be dropped.
      if(GASNET_BUILD_PARAM)
        list(APPEND GASNET_BUILD_CFLAGS "${GASNET_BUILD_PARAM}")
      endif()
    endif()

    if(GASNET_BUILD_CFLAGS)
      message(STATUS "GASNet compile flags: ${GASNET_BUILD_CFLAGS}")
      set_target_properties(
        PkgConfig::GASNET PROPERTIES INTERFACE_COMPILE_OPTIONS
                                     "${GASNET_BUILD_CFLAGS}"
      )
    endif()

    # Link libraries, link options and link directories come straight from the
    # IMPORTED_TARGET PkgConfig::GASNET, so consumers simply link that target.
  endif()

endmacro()
