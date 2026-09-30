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
