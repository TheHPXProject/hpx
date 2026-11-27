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
    # found and when a found .pc turns out to be misconfigured.  GASNet emits a
    # static archive (libgasnet-mpi-par.a); -fPIC is what makes it linkable into
    # HPX's shared objects, so the compile flags are essential here.
    string(
      CONCAT
        GASNET_REBUILD_MESSAGE
        "Build GASNet as follows and make the .pc files visible via "
        "PKG_CONFIG_PATH or CMAKE_PREFIX_PATH:\n"
        "  export CXXFLAGS='-fPIC -O3'; export CFLAGS='-fPIC -O3';\n"
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

    # GASNet's .pc records the runtime of the compiler GASNet was built with
    # (e.g. '-L/usr/lib/gcc/x86_64-redhat-linux/11 -lgcc -lpthread'). With a
    # different compiler for HPX, that directory makes the linker pick up the
    # older compiler's libstdc++ (undefined references to newer symbols). The
    # compiler driver adds its own runtime, so drop these entries.
    get_target_property(
      _gasnet_link_libraries PkgConfig::GASNET INTERFACE_LINK_LIBRARIES
    )
    get_target_property(
      _gasnet_link_directories PkgConfig::GASNET INTERFACE_LINK_DIRECTORIES
    )
    if(_gasnet_link_libraries)
      list(FILTER _gasnet_link_libraries EXCLUDE REGEX
           "(^|/)lib(gcc|gcc_s|gcc_eh|pthread)\\.(a|so)"
      )
      set_target_properties(
        PkgConfig::GASNET PROPERTIES INTERFACE_LINK_LIBRARIES
                                     "${_gasnet_link_libraries}"
      )
    endif()
    # libs/CMakeLists.txt also adds GASNET_LIBRARY_DIRS to hpx_core directly
    if(NOT _gasnet_link_directories)
      set(_gasnet_link_directories)
    endif()
    set(_gasnet_kept_directories)
    foreach(_dir IN LISTS _gasnet_link_directories GASNET_LIBRARY_DIRS)
      if(EXISTS "${_dir}/libgcc.a")
        message(STATUS "GASNet: dropping compiler runtime directory ${_dir}")
      elseif(NOT "${_dir}" IN_LIST _gasnet_kept_directories)
        list(APPEND _gasnet_kept_directories "${_dir}")
      endif()
    endforeach()
    set_target_properties(
      PkgConfig::GASNET PROPERTIES INTERFACE_LINK_DIRECTORIES
                                   "${_gasnet_kept_directories}"
    )
    set(GASNET_LIBRARY_DIRS
        "${_gasnet_kept_directories}"
        CACHE INTERNAL ""
    )
    set(GASNET_LIBRARY_DIRS "${_gasnet_kept_directories}")
    find_package(Threads REQUIRED)
    target_link_libraries(PkgConfig::GASNET INTERFACE Threads::Threads)

    if("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
      set(GASNET_MPI_FOUND TRUE)
      include(HPX_SetupMPI)
      hpx_setup_mpi()
      target_link_libraries(PkgConfig::GASNET INTERFACE Mpi::mpi)
    endif()

    # GASNet's pkg-config files list its own compiler tuning flags such as
    # "--param name=value" in both the compile and the link flags. pkg-config
    # splits them into two list elements, and CMake de-duplicates the repeated
    # "--param" tokens, which leaves a bare "name=value" behind that the
    # compiler and linker take for an input file. These flags only tune the
    # GASNet build itself, so drop the pairs from both properties here, in one
    # place.
    foreach(_gasnet_prop INTERFACE_COMPILE_OPTIONS INTERFACE_LINK_OPTIONS)
      get_target_property(_gasnet_opts PkgConfig::GASNET ${_gasnet_prop})
      if(_gasnet_opts)
        set(_gasnet_filtered_opts)
        set(_gasnet_skip_next FALSE)
        foreach(_gasnet_opt IN LISTS _gasnet_opts)
          if(_gasnet_skip_next)
            set(_gasnet_skip_next FALSE)
          elseif("${_gasnet_opt}" STREQUAL "--param")
            set(_gasnet_skip_next TRUE)
          else()
            list(APPEND _gasnet_filtered_opts "${_gasnet_opt}")
          endif()
        endforeach()
        set_property(
          TARGET PkgConfig::GASNET PROPERTY ${_gasnet_prop}
                                            "${_gasnet_filtered_opts}"
        )
      endif()
    endforeach()

    # Validate the GASNet build configuration. HPX requires a GASNet built in
    # PAR threading mode with the mpi conduit, with position-independent code
    # (GASNet is consumed as a static archive; -fPIC is what makes it linkable
    # into HPX's shared objects), on top of a UCX built with multithreading
    # support.  The library artefacts themselves are whatever the
    # gasnet-<conduit>-par .pc resolves to, so only the pkg-config metadata is
    # validated here.

    set(GASNET_PC_NAME "gasnet-${HPX_WITH_PARCELPORT_GASNET_CONDUIT}-par")

    foreach(
      _var IN
      ITEMS GASNET_DEFINES
            GASNET_CFLAGS
            GASNET_LDFLAGS
            GASNET_LIBS
            GASNET_INCLUDES
            GASNET_CC
            GASNET_CXX
    )
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
      OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
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
    set(GASNET_BUILD_FLAGS
        "${GASNET_BUILD_GASNET_CFLAGS} \
${GASNET_BUILD_GASNET_LDFLAGS} ${GASNET_BUILD_GASNET_CC} \
${GASNET_BUILD_GASNET_CXX}"
    )
    string(FIND "${GASNET_BUILD_FLAGS}" "-fPIC" GASNET_BUILD_PIC)
    if(GASNET_BUILD_PIC EQUAL -1)
      message(
        FATAL_ERROR "GASNet was built without -fPIC. HPX requires a position-"
                    "independent GASNet build. ${GASNET_REBUILD_MESSAGE}"
      )
    endif()

    # Under the mpi conduit, an MPI library built on UCX needs a UCX built with
    # multithreading support for HPX's threading model: gasnet-mpi links through
    # libmpi, which pulls in the UCX it was built with. This is not GASNet's own
    # ucx conduit. MPI libraries that don't use UCX (for instance MPICH with
    # OFI) are fine, so this only warns.
    if("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
      execute_process(
        COMMAND "${PKG_CONFIG_EXECUTABLE}" --variable=prefix ucx
        RESULT_VARIABLE GASNET_BUILD_PC_RESULT
        OUTPUT_VARIABLE GASNET_UCX_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
      )
      if(NOT GASNET_BUILD_PC_RESULT EQUAL 0 OR NOT GASNET_UCX_PREFIX)
        message(
          STATUS
            "GASNet mpi conduit: no UCX found through pkg-config, not checking "
            "whether the MPI library uses a UCX with multithreading support"
        )
      else()
        find_program(
          GASNET_UCX_INFO ucx_info HINTS "${GASNET_UCX_PREFIX}/bin" NO_CACHE
        )
        if(NOT GASNET_UCX_INFO)
          message(
            WARNING
              "Could not find the 'ucx_info' tool for the UCX installation at "
              "'${GASNET_UCX_PREFIX}', not checking whether it was built with "
              "multithreading support. If the MPI library uses this UCX, it "
              "must have been configured with --enable-mt."
          )
        else()
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
              WARNING
                "UCX at '${GASNET_UCX_PREFIX}' was not built with "
                "multithreading support ('ucx_info -b' does not report "
                "ENABLE_MT 1). If the MPI library uses this UCX, rebuild UCX "
                "with --enable-mt:\n"
                "  ./configure --prefix=/opt/ucx --enable-mt "
                "&& make && make install"
            )
          endif()
        endif()
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

    # Link libraries, link options and link directories come straight from the
    # IMPORTED_TARGET PkgConfig::GASNET, so consumers simply link that target.
  endif()

endmacro()
