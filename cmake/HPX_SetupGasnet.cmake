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
          "GASNet (conduit '${HPX_WITH_PARCELPORT_GASNET_CONDUIT}') not found! "
          "Install GASNet built with that conduit and make its pkgconfig "
          "directory visible via PKG_CONFIG_PATH or CMAKE_PREFIX_PATH."
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
    # and as shared libraries, on top of an UCX built with multithreading
    # support.

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

    string(
      CONCAT
      GASNET_REBUILD_MESSAGE
      "Rebuild GASNet as follows and make the .pc files visible via "
      "PKG_CONFIG_PATH or CMAKE_PREFIX_PATH:\n"
      "  export CXXFLAGS=-fPIC -O3; export CFLAGS=-fPIC -O3;\n"
      "  PMI_LIBS=probe CFLAGS=-fPIC CCFLAGS=-fPIC CXXFLAGS=-fPIC\n"
      "  ./configure --enable-par --disable-ucx --enable-mpi \\\n"
      "    --with-mpi=/opt/openmpi --enable-hwloc \\\n"
      "    --prefix=/opt/gasnet-par --with-cflags=-fPIC \\\n"
      "    --with-cxxflags=-fPIC --disable-udp --disable-ibv \\\n"
      "    --enable-shared --enable-pmi --with-pmi-home=/opt/pmix \\\n"
      "    --enable-segment-large --with-ldflags=-fPIC \\\n"
      "    --with-mpi-cflags=-fPIC && make -j4 && sudo make install"
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

    # hard requirement: shared libraries
    set(GASNET_BUILD_SHARED_LIB "")
    foreach(_dir IN LISTS GASNET_LIBRARY_DIRS)
      file(
        GLOB GASNET_BUILD_GLOB
        "${_dir}/libgasnet-${HPX_WITH_PARCELPORT_GASNET_CONDUIT}-par.so*"
      )
      if(GASNET_BUILD_GLOB)
        list(GET GASNET_BUILD_GLOB 0 GASNET_BUILD_SHARED_LIB)
        break()
      endif()
    endforeach()
    if(NOT GASNET_BUILD_SHARED_LIB)
      message(
        FATAL_ERROR
          "GASNet was not built as a shared library (no "
          "libgasnet-${HPX_WITH_PARCELPORT_GASNET_CONDUIT}-par.so in "
          "'${GASNET_LIBRARY_DIRS}'). HPX requires --enable-shared. "
          "${GASNET_REBUILD_MESSAGE}"
      )
    endif()

    # hard requirement (mpi conduit only): UCX built with multithreading
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

    if(GASNET_CFLAGS)
      set(IS_PARAM "0")
      set(PARAM_FOUND "0")
      set(NEWPARAM "")
      set(IDX 0)
      set(FLAG_LIST "")

      foreach(X IN ITEMS ${GASNET_CFLAGS})
        string(FIND "${X}" "--param" PARAM_FOUND)
        if(NOT "${PARAM_FOUND}" EQUAL "-1")
          set(IS_PARAM "1")
          set(NEWPARAM "SHELL:${X}")
        endif()
        if("${PARAM_FOUND}" EQUAL "-1"
           AND "${IS_PARAM}" EQUAL "0"
           OR "${IS_PARAM}" EQUAL "-1"
        )
          list(APPEND FLAG_LIST "${X}")
          set(IS_PARAM "0")
        elseif("${PARAM_FOUND}" EQUAL "-1" AND "${IS_PARAM}" EQUAL "1")
          list(APPEND FLAG_LIST "${NEWPARAM} ${X}")
          set(NEWPARAM "")
          set(IS_PARAM "0")
        endif()
      endforeach()

      list(LENGTH GASNET_CFLAGS IDX)
      foreach(X RANGE ${IDX})
        list(POP_FRONT GASNET_CFLAGS NEWPARAM)
      endforeach()

      foreach(X IN ITEMS ${FLAG_LIST})
        list(APPEND GASNET_CFLAGS "${X}")
      endforeach()
    endif()

    if(GASNET_CFLAGS_OTHER)
      set(IS_PARAM "0")
      set(PARAM_FOUND "0")
      set(NEWPARAM "")
      set(IDX 0)
      set(FLAG_LIST "")

      foreach(X IN ITEMS ${GASNET_CFLAGS_OTHER})
        string(FIND "${X}" "--param" PARAM_FOUND)
        if(NOT "${PARAM_FOUND}" EQUAL "-1")
          set(IS_PARAM "1")
          set(NEWPARAM "SHELL:${X}")
        endif()
        if("${PARAM_FOUND}" EQUAL "-1"
           AND "${IS_PARAM}" EQUAL "0"
           OR "${IS_PARAM}" EQUAL "-1"
        )
          list(APPEND FLAG_LIST "${X}")
          set(IS_PARAM "0")
        elseif("${PARAM_FOUND}" EQUAL "-1" AND "${IS_PARAM}" EQUAL "1")
          list(APPEND FLAG_LIST "${NEWPARAM} ${X}")
          set(NEWPARAM "")
          set(IS_PARAM "0")
        endif()
      endforeach()

      list(LENGTH GASNET_CFLAGS_OTHER IDX)
      foreach(X RANGE ${IDX})
        list(POP_FRONT GASNET_CFLAGS_OTHER NEWPARAM)
      endforeach()

      foreach(X IN ITEMS ${FLAG_LIST})
        list(APPEND GASNET_CFLAGS_OTHER "${X}")
      endforeach()
    endif()

    if(GASNET_LDFLAGS)
      set(IS_PARAM "0")
      set(PARAM_FOUND "0")
      set(NEWPARAM "")
      set(IDX 0)
      set(DIRIDX 0)
      set(FLAG_LIST "")
      set(DIR_LIST "")
      set(LIB_LIST "")

      foreach(X IN ITEMS ${GASNET_LDFLAGS})
        string(FIND "${X}" "--param" PARAM_FOUND)
        string(FIND "${X}" "-lgasnet" IDX)
        string(FIND "${X}" "-l" LIDX)
        string(FIND "${X}" "-L" DIRIDX)
        if(NOT "${PARAM_FOUND}" EQUAL "-1")
          set(IS_PARAM "1")
          set(NEWPARAM "SHELL:${X}")
        endif()
        if("${PARAM_FOUND}" EQUAL "-1"
           AND "${IDX}" EQUAL "-1"
           AND "${IS_PARAM}" EQUAL "0"
           OR "${IS_PARAM}" EQUAL "-1"
        )
          list(APPEND FLAG_LIST "${X}")
          set(IS_PARAM "0")
        elseif("${PARAM_FOUND}" EQUAL "-1" AND "${IS_PARAM}" EQUAL "1")
          list(APPEND FLAG_LIST "${NEWPARAM} ${X}")
          set(NEWPARAM "")
          set(IS_PARAM "0")
        elseif(NOT "${IDX}" EQUAL "-1" AND NOT "${LIDX}" EQUAL "-1")
          set(TMPSTR "")
          string(REPLACE "-l" "" TMPSTR "${X}")
          list(APPEND LIB_LIST "${TMPSTR}")
          set(IDX 0)
        elseif("${IDX}" EQUAL "-1" AND NOT "${LIDX}" EQUAL "-1")
          list(APPEND FLAG_LIST "${X}")
        endif()
        if(NOT "${DIRIDX}" EQUAL "-1")
          set(TMPSTR "")
          string(REPLACE "-L" "" TMPSTR "${X}")
          list(APPEND DIR_LIST "${TMPSTR}")
        endif()
      endforeach()

      set(IDX 0)
      list(LENGTH GASNET_LDFLAGS IDX)
      foreach(X RANGE ${IDX})
        list(POP_FRONT GASNET_LDFLAGS NEWPARAM)
      endforeach()

      foreach(X IN ITEMS ${FLAG_LIST})
        list(APPEND GASNET_LDFLAGS "${X}")
      endforeach()

      set(IDX 0)
      list(LENGTH LIB_LIST IDX)
      if(NOT "${IDX}" EQUAL "0")
        set(IDX 0)

        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
          set(NEWLINK "SHELL:-Wl,--whole-archive ")
          foreach(X IN ITEMS ${LIB_LIST})
            set(DIRSTR "")
            string(REPLACE ";" " " DIRSTR "${DIR_LIST}")
            foreach(Y IN ITEMS ${DIR_LIST})
              find_library(
                FOUND_LIB
                NAMES ${X} "lib${X}" "lib${X}.a"
                PATHS ${Y}
                HINTS ${Y} NO_CACHE
                NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH
              )

              list(LENGTH FOUND_LIB IDX)
              if(NOT "${IDX}" EQUAL "0")
                string(APPEND NEWLINK "${FOUND_LIB}")
                set(FOUND_LIB "")
              endif()
            endforeach()
          endforeach()
          string(APPEND NEWLINK " -Wl,--no-whole-archive")
          string(FIND "SHELL:-Wl,--whole-archive  -Wl,--no-whole-archive"
                      "${NEWLINK}" IDX
          )
          if("${IDX}" EQUAL "-1")
            list(APPEND GASNET_LDFLAGS "${NEWLINK}")
          endif()
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
          if(APPLE)
            set(NEWLINK "SHELL:-Wl,-force_load,")
          else()
            set(NEWLINK "SHELL: ")
          endif()
          foreach(X IN ITEMS ${LIB_LIST})
            set(DIRSTR "")
            string(REPLACE ";" " " DIRSTR "${DIR_LIST}")
            foreach(Y IN ITEMS ${DIR_LIST})
              find_library(
                FOUND_LIB
                NAMES ${X} "lib${X}" "lib${X}.a"
                PATHS ${Y}
                HINTS ${Y} NO_CACHE
                NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH
              )

              list(LENGTH FOUND_LIB IDX)
              if(NOT "${IDX}" EQUAL "0")
                string(APPEND NEWLINK "${FOUND_LIB}")
                set(FOUND_LIB "")
              endif()
            endforeach()
          endforeach()
          string(FIND "SHELL:" "${NEWLINK}" IDX)
          if("${IDX}" EQUAL "-1")
            list(APPEND GASNET_LDFLAGS "${NEWLINK}")
          endif()
        endif()
      endif()
    endif()

    if(GASNET_LDFLAGS_OTHER)
      unset(FOUND_LIB)
      set(IS_PARAM "0")
      set(PARAM_FOUND "0")
      set(NEWPARAM "")
      set(IDX 0)
      set(DIRIDX 0)
      set(FLAG_LIST "")
      set(DIR_LIST "")
      set(LIB_LIST "")

      foreach(X IN ITEMS ${GASNET_LDFLAGS_OTHER})
        string(FIND "${X}" "--param" PARAM_FOUND)
        string(FIND "${X}" "-lgasnet" IDX)
        string(FIND "${X}" "-L" DIRIDX)
        if(NOT "${PARAM_FOUND}" EQUAL "-1")
          set(IS_PARAM "1")
          set(NEWPARAM "SHELL:${X}")
        endif()
        if("${PARAM_FOUND}" EQUAL "-1"
           AND "${IDX}" EQUAL "-1"
           AND "${IS_PARAM}" EQUAL "0"
           OR "${IS_PARAM}" EQUAL "-1"
        )
          list(APPEND FLAG_LIST "${X}")
          set(IS_PARAM "0")
        elseif("${PARAM_FOUND}" EQUAL "-1" AND "${IS_PARAM}" EQUAL "1")
          list(APPEND FLAG_LIST "${NEWPARAM} ${X}")
          set(NEWPARAM "")
          set(IS_PARAM "0")
        elseif(NOT "${IDX}" EQUAL "-1" AND NOT "${LIDX}" EQUAL "-1")
          set(TMPSTR "")
          string(REPLACE "-l" "" TMPSTR "${X}")
          list(APPEND LIB_LIST "${TMPSTR}")
          set(IDX 0)
        elseif("${IDX}" EQUAL "-1" AND NOT "${LIDX}" EQUAL "-1")
          list(APPEND FLAG_LIST "${X}")
        endif()
        if(NOT "${DIRIDX}" EQUAL "-1")
          set(TMPSTR "")
          string(REPLACE "-L" "" TMPSTR "${X}")
          list(APPEND DIR_LIST "${TMPSTR}")
        endif()
      endforeach()

      set(IDX 0)
      list(LENGTH GASNET_LDFLAGS_OTHER IDX)
      foreach(X RANGE ${IDX})
        list(POP_FRONT GASNET_LDFLAGS_OTHER NEWPARAM)
      endforeach()

      foreach(X IN ITEMS ${FLAG_LIST})
        list(APPEND GASNET_LDFLAGS_OTHER "${X}")
      endforeach()

      set(IDX 0)
      list(LENGTH LIB_LIST IDX)
      if(NOT "${IDX}" EQUAL "0")
        set(IDX 0)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
          set(NEWLINK "SHELL:-Wl,--whole-archive ")
          foreach(X IN ITEMS ${LIB_LIST})
            set(DIRSTR "")
            string(REPLACE ";" " " DIRSTR "${DIR_LIST}")
            foreach(Y IN ITEMS ${DIR_LIST})
              find_library(
                FOUND_LIB
                NAMES ${X} "lib${X}" "lib${X}.a"
                PATHS ${Y}
                HINTS ${Y} NO_CACHE
                NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH
              )

              list(LENGTH FOUND_LIB IDX)
              if(NOT "${IDX}" EQUAL "0")
                string(APPEND NEWLINK "${FOUND_LIB}")
                set(FOUND_LIB "")
              endif()
            endforeach()
          endforeach()
          string(APPEND NEWLINK " -Wl,--no-whole-archive")

          string(FIND "SHELL:-Wl,--whole-archive  -Wl,--no-whole-archive"
                      "${NEWLINK}" IDX
          )
          if("${IDX}" EQUAL "-1")
            list(APPEND GASNET_LDFLAGS_OTHER "${NEWLINK}")
          endif()
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
          if(APPLE)
            set(NEWLINK "SHELL:-Wl,-force_load,")
          else()
            set(NEWLINK "SHELL: ")
          endif()
          foreach(X IN ITEMS ${LIB_LIST})
            set(DIRSTR "")
            string(REPLACE ";" " " DIRSTR "${DIR_LIST}")
            foreach(Y IN ITEMS ${DIR_LIST})
              find_library(
                FOUND_LIB
                NAMES ${X} "lib${X}" "lib${X}.a"
                PATHS ${Y}
                HINTS ${Y} NO_CACHE
                NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH
              )

              list(LENGTH FOUND_LIB IDX)
              if(NOT "${IDX}" EQUAL "0")
                string(APPEND NEWLINK "${FOUND_LIB}")
                set(FOUND_LIB "")
              endif()
            endforeach()
          endforeach()
          string(FIND "SHELL:" "${NEWLINK}" IDX)
          if("${IDX}" EQUAL "-1")
            list(APPEND GASNET_LDFLAGS "${NEWLINK}")
          endif()
        endif()
      endif()

    endif()

    if(GASNET_STATIC_CFLAGS)
      set(IS_PARAM "0")
      set(PARAM_FOUND "0")
      set(NEWPARAM "")
      set(IDX 0)
      set(FLAG_LIST "")

      foreach(X IN ITEMS ${GASNET_STATIC_CFLAGS})
        string(FIND "${X}" "--param" PARAM_FOUND)
        if(NOT "${PARAM_FOUND}" EQUAL "-1")
          set(IS_PARAM "1")
          set(NEWPARAM "SHELL:${X}")
        endif()
        if("${PARAM_FOUND}" EQUAL "-1"
           AND "${IS_PARAM}" EQUAL "0"
           OR "${IS_PARAM}" EQUAL "-1"
        )
          list(APPEND FLAG_LIST "${X}")
          set(IS_PARAM "0")
        elseif("${PARAM_FOUND}" EQUAL "-1" AND "${IS_PARAM}" EQUAL "1")
          list(APPEND FLAG_LIST "${NEWPARAM} ${X}")
          set(NEWPARAM "")
          set(IS_PARAM "0")
        endif()
      endforeach()

      list(LENGTH GASNET_STATIC_CFLAGS IDX)
      foreach(X RANGE ${IDX})
        list(POP_FRONT GASNET_STATIC_CFLAGS NEWPARAM)
      endforeach()

      foreach(X IN ITEMS ${FLAG_LIST})
        list(APPEND GASNET_STATIC_CFLAGS "${X}")
      endforeach()
    endif()

    if(GASNET_STATIC_CFLAGS_OTHER)
      set(IS_PARAM "0")
      set(PARAM_FOUND "0")
      set(NEWPARAM "")
      set(IDX 0)
      set(FLAG_LIST "")

      foreach(X IN ITEMS ${GASNET_STATIC_CFLAGS_OTHER})
        string(FIND "${X}" "--param" PARAM_FOUND)
        if(NOT "${PARAM_FOUND}" EQUAL "-1")
          set(IS_PARAM "1")
          set(NEWPARAM "SHELL:${X}")
        endif()
        if("${PARAM_FOUND}" EQUAL "-1"
           AND "${IS_PARAM}" EQUAL "0"
           OR "${IS_PARAM}" EQUAL "-1"
        )
          list(APPEND FLAG_LIST "${X}")
          set(IS_PARAM "0")
        elseif("${PARAM_FOUND}" EQUAL "-1" AND "${IS_PARAM}" EQUAL "1")
          list(APPEND FLAG_LIST "${NEWPARAM} ${X}")
          set(NEWPARAM "")
          set(IS_PARAM "0")
        endif()
      endforeach()

      list(LENGTH GASNET_STATIC_CFLAGS_OTHER IDX)
      foreach(X RANGE ${IDX})
        list(POP_FRONT GASNET_STATIC_CFLAGS_OTHER NEWPARAM)
      endforeach()

      foreach(X IN ITEMS ${FLAG_LIST})
        list(APPEND GASNET_STATIC_CFLAGS_OTHER "${X}")
      endforeach()
    endif()

    if(GASNET_STATIC_LDFLAGS)
      unset(FOUND_LIB)
      set(IS_PARAM "0")
      set(PARAM_FOUND "0")
      set(NEWPARAM "")
      set(IDX 0)
      set(DIRIDX 0)
      set(FLAG_LIST "")
      set(DIR_LIST "")
      set(LIB_LIST "")

      foreach(X IN ITEMS ${GASNET_STATIC_LDFLAGS})
        string(FIND "${X}" "--param" PARAM_FOUND)
        string(FIND "${X}" "-lgasnet" IDX)
        string(FIND "${X}" "-L" DIRIDX)
        if(NOT "${PARAM_FOUND}" EQUAL "-1")
          set(IS_PARAM "1")
          set(NEWPARAM "SHELL:${X}")
        endif()
        if("${PARAM_FOUND}" EQUAL "-1"
           AND "${IDX}" EQUAL "-1"
           AND "${IS_PARAM}" EQUAL "0"
           OR "${IS_PARAM}" EQUAL "-1"
        )
          list(APPEND FLAG_LIST "${X}")
          set(IS_PARAM "0")
        elseif("${PARAM_FOUND}" EQUAL "-1" AND "${IS_PARAM}" EQUAL "1")
          list(APPEND FLAG_LIST "${NEWPARAM} ${X}")
          set(NEWPARAM "")
          set(IS_PARAM "0")
        elseif(NOT "${IDX}" EQUAL "-1" AND NOT "${LIDX}" EQUAL "-1")
          set(TMPSTR "")
          string(REPLACE "-l" "" TMPSTR "${X}")
          list(APPEND LIB_LIST "${TMPSTR}")
          set(IDX 0)
        elseif("${IDX}" EQUAL "-1" AND NOT "${LIDX}" EQUAL "-1")
          list(APPEND FLAG_LIST "${X}")
        endif()
        if(NOT "${DIRIDX}" EQUAL "-1")
          set(TMPSTR "")
          string(REPLACE "-L" "" TMPSTR "${X}")
          list(APPEND DIR_LIST "${TMPSTR}")
        endif()
      endforeach()

      set(IDX 0)
      list(LENGTH GASNET_STATIC_LDFLAGS IDX)
      foreach(X RANGE ${IDX})
        list(POP_FRONT GASNET_STATIC_LDFLAGS NEWPARAM)
      endforeach()

      foreach(X IN ITEMS ${FLAG_LIST})
        list(APPEND GASNET_STATIC_LDFLAGS "${X}")
      endforeach()

      set(IDX 0)
      list(LENGTH LIB_LIST IDX)
      if(NOT "${IDX}" EQUAL "0")
        set(IDX 0)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
          set(NEWLINK "SHELL:-Wl,--whole-archive ")
          foreach(X IN ITEMS ${LIB_LIST})
            set(DIRSTR "")
            string(REPLACE ";" " " DIRSTR "${DIR_LIST}")
            foreach(Y IN ITEMS ${DIR_LIST})
              find_library(
                FOUND_LIB
                NAMES ${X} "lib${X}" "lib${X}.a"
                PATHS ${Y}
                HINTS ${Y} NO_CACHE
                NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH
              )

              list(LENGTH FOUND_LIB IDX)

              if(NOT "${IDX}" EQUAL "0")
                string(APPEND NEWLINK "${FOUND_LIB}")
                set(FOUND_LIB "")
              endif()
            endforeach()
          endforeach()
          string(APPEND NEWLINK " -Wl,--no-whole-archive")

          string(FIND "SHELL:-Wl,--whole-archive  -Wl,--no-whole-archive"
                      "${NEWLINK}" IDX
          )
          if("${IDX}" EQUAL "-1")
            list(APPEND GASNET_STATIC_LDFLAGS "${NEWLINK}")
          endif()
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
          if(APPLE)
            set(NEWLINK "SHELL:-Wl,-force_load,")
          else()
            set(NEWLINK "SHELL: ")
          endif()
          foreach(X IN ITEMS ${LIB_LIST})
            set(DIRSTR "")
            string(REPLACE ";" " " DIRSTR "${DIR_LIST}")
            foreach(Y IN ITEMS ${DIR_LIST})
              find_library(
                FOUND_LIB
                NAMES ${X} "lib${X}" "lib${X}.a"
                PATHS ${Y}
                HINTS ${Y} NO_CACHE
                NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH
              )

              list(LENGTH FOUND_LIB IDX)
              if(NOT "${IDX}" EQUAL "0")
                string(APPEND NEWLINK "${FOUND_LIB}")
                set(FOUND_LIB "")
              endif()
            endforeach()
          endforeach()
          string(FIND "SHELL:" "${NEWLINK}" IDX)
          if("${IDX}" EQUAL "-1")
            list(APPEND GASNET_LDFLAGS "${NEWLINK}")
          endif()
        endif()
      endif()
    endif()

    if(GASNET_STATIC_LDFLAGS_OTHER)
      unset(FOUND_LIB)
      set(IS_PARAM "0")
      set(PARAM_FOUND "0")
      set(NEWPARAM "")
      set(IDX 0)
      set(DIRIDX 0)
      set(FLAG_LIST "")
      set(DIR_LIST "")
      set(LIB_LIST "")

      foreach(X IN ITEMS ${GASNET_STATIC_LDFLAGS_OTHER})
        string(FIND "${X}" "--param" PARAM_FOUND)
        string(FIND "${X}" "-lgasnet" IDX)
        string(FIND "${X}" "-L" DIRIDX)
        if(NOT "${PARAM_FOUND}" EQUAL "-1")
          set(IS_PARAM "1")
          set(NEWPARAM "SHELL:${X}")
        endif()
        if("${PARAM_FOUND}" EQUAL "-1"
           AND "${IDX}" EQUAL "-1"
           AND "${IS_PARAM}" EQUAL "0"
           OR "${IS_PARAM}" EQUAL "-1"
        )
          list(APPEND FLAG_LIST "${X}")
          set(IS_PARAM "0")
        elseif("${PARAM_FOUND}" EQUAL "-1" AND "${IS_PARAM}" EQUAL "1")
          list(APPEND FLAG_LIST "${NEWPARAM} ${X}")
          set(NEWPARAM "")
          set(IS_PARAM "0")
        elseif(NOT "${IDX}" EQUAL "-1" AND NOT "${LIDX}" EQUAL "-1")
          set(TMPSTR "")
          string(REPLACE "-l" "" TMPSTR "${X}")
          list(APPEND LIB_LIST "${TMPSTR}")
          set(IDX 0)
        elseif("${IDX}" EQUAL "-1" AND NOT "${LIDX}" EQUAL "-1")
          list(APPEND FLAG_LIST "${X}")
        endif()
        if(NOT "${DIRIDX}" EQUAL "-1")
          set(TMPSTR "")
          string(REPLACE "-L" "" TMPSTR "${X}")
          list(APPEND DIR_LIST "${TMPSTR}")
        endif()
      endforeach()

      set(IDX 0)
      list(LENGTH GASNET_STATIC_LDFLAGS_OTHER IDX)
      foreach(X RANGE ${IDX})
        list(POP_FRONT GASNET_STATIC_LDFLAGS_OTHER NEWPARAM)
      endforeach()

      foreach(X IN ITEMS ${FLAG_LIST})
        list(APPEND GASNET_STATIC_LDFLAGS_OTHER "${X}")
      endforeach()

      set(IDX 0)
      list(LENGTH LIB_LIST IDX)
      if(NOT "${IDX}" EQUAL "0")
        set(IDX 0)
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
          set(NEWLINK "SHELL:-Wl,--whole-archive ")
          foreach(X IN ITEMS ${LIB_LIST})
            set(DIRSTR "")
            string(REPLACE ";" " " DIRSTR "${DIR_LIST}")
            foreach(Y IN ITEMS ${DIR_LIST})
              find_library(
                FOUND_LIB
                NAMES ${X} "lib${X}" "lib${X}.a"
                PATHS ${Y}
                HINTS ${Y} NO_CACHE
                NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH
              )

              list(LENGTH FOUND_LIB IDX)

              message(STATUS "${FOUND_LIB} ${X}")
              if(NOT "${IDX}" EQUAL "0")
                string(APPEND NEWLINK "${FOUND_LIB}")
                set(FOUND_LIB "")
              endif()
            endforeach()
          endforeach()
          string(APPEND NEWLINK " -Wl,--no-whole-archive")
          string(FIND "SHELL:-Wl,--whole-archive  -Wl,--no-whole-archive"
                      "${NEWLINK}" IDX
          )
          if("${IDX}" EQUAL "-1")
            list(APPEND GASNET_STATIC_LDFLAGS_OTHER "${NEWLINK}")
          endif()
        elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
          if(APPLE)
            set(NEWLINK "SHELL:-Wl,-force_load,")
          else()
            set(NEWLINK "SHELL: ")
          endif()
          foreach(X IN ITEMS ${LIB_LIST})
            set(DIRSTR "")
            string(REPLACE ";" " " DIRSTR "${DIR_LIST}")
            foreach(Y IN ITEMS ${DIR_LIST})
              find_library(
                FOUND_LIB
                NAMES ${X} "lib${X}" "lib${X}.a"
                PATHS ${Y}
                HINTS ${Y} NO_CACHE
                NO_CMAKE_FIND_ROOT_PATH NO_DEFAULT_PATH
              )

              list(LENGTH FOUND_LIB IDX)
              if(NOT "${IDX}" EQUAL "0")
                string(APPEND NEWLINK "${FOUND_LIB}")
                set(FOUND_LIB "")
              endif()
            endforeach()
          endforeach()
          string(FIND "SHELL:" "${NEWLINK}" IDX)
          if("${IDX}" EQUAL "-1")
            list(APPEND GASNET_LDFLAGS "${NEWLINK}")
          endif()
        endif()
      endif()
    endif()

    set_target_properties(
      PkgConfig::GASNET PROPERTIES INTERFACE_COMPILE_OPTIONS "${GASNET_CFLAGS}"
    )
    set_target_properties(
      PkgConfig::GASNET PROPERTIES INTERFACE_LINK_OPTIONS "${GASNET_LDFLAGS}"
    )
    set_target_properties(
      PkgConfig::GASNET PROPERTIES INTERFACE_LINK_DIRECTORIES
                                   "${GASNET_LIBRARY_DIRS}"
    )

  endif()

endmacro()
