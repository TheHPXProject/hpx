# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Builds HPX on Windows with clang as the CUDA compiler (instead of nvcc, which
# can't compile stdexec with MSVC as the host compiler). CMake requires the same
# clang front end for all languages, so C and C++ are compiled by clang/clang++
# too (GNU-like command line, MSVC ABI and standard library).
#
# Use it with the Ninja generator from a Visual Studio developer prompt (for the
# MSVC standard library and the Windows SDK):
#
# ~~~
#   cmake -G Ninja
#     -DCMAKE_TOOLCHAIN_FILE=<hpx>/cmake/toolchains/Windows-Clang-CUDA.cmake
#     -DHPX_WITH_CUDA=ON -DCMAKE_CUDA_ARCHITECTURES=<arch> ...
# ~~~
#
# HPX_CLANG_ROOT selects the clang installation (the directory containing
# bin/clang.exe), the default is the newest clang of an LLVM installation or a
# Visual Studio installation, otherwise the one on the PATH. The CUDA toolkit is
# taken from CUDAToolkit_ROOT or the CUDA_PATH environment variable.

set(HPX_CLANG_ROOT
    ""
    CACHE PATH "The clang installation to use (containing bin/clang.exe)"
)

if(HPX_CLANG_ROOT)
  # only the clang of the given installation
  set(_hpx_clang_search PATHS "${HPX_CLANG_ROOT}/bin" NO_DEFAULT_PATH)
else()
  # the newest clang of an LLVM or a Visual Studio installation
  file(
    GLOB
    _hpx_clang_candidates
    "$ENV{ProgramFiles}/LLVM/bin/clang.exe"
    "$ENV{ProgramFiles}/Microsoft Visual Studio/*/*/VC/Tools/Llvm/x64/bin/clang.exe"
  )
  set(_hpx_clang_hints)
  set(_hpx_clang_newest 0)
  foreach(_hpx_clang IN LISTS _hpx_clang_candidates)
    execute_process(
      COMMAND "${_hpx_clang}" -dumpversion
      OUTPUT_VARIABLE _hpx_clang_version
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _hpx_clang_result
    )
    if(_hpx_clang_result EQUAL 0 AND _hpx_clang_version VERSION_GREATER
                                     _hpx_clang_newest
    )
      set(_hpx_clang_newest "${_hpx_clang_version}")
      get_filename_component(_hpx_clang_hints "${_hpx_clang}" DIRECTORY)
    endif()
  endforeach()
  unset(_hpx_clang)
  unset(_hpx_clang_candidates)
  unset(_hpx_clang_newest)
  unset(_hpx_clang_result)
  unset(_hpx_clang_version)
  set(_hpx_clang_search HINTS ${_hpx_clang_hints})
  unset(_hpx_clang_hints)
endif()

find_program(
  HPX_CLANG_EXECUTABLE clang.exe ${_hpx_clang_search}
  DOC "The clang executable used for C, C++ and CUDA"
)
find_program(
  HPX_CLANGXX_EXECUTABLE clang++.exe ${_hpx_clang_search}
  DOC "The clang++ executable used for C++"
)
unset(_hpx_clang_search)

if(NOT HPX_CLANG_EXECUTABLE OR NOT HPX_CLANGXX_EXECUTABLE)
  message(FATAL_ERROR "clang was not found, set HPX_CLANG_ROOT")
endif()

# an older clang doesn't know the CUDA versions HPX requires, and CMake would
# fail on it with less helpful errors (see also cmake/HPX_SetupCUDA.cmake)
execute_process(
  COMMAND "${HPX_CLANG_EXECUTABLE}" -dumpversion
  OUTPUT_VARIABLE _hpx_clang_version
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(_hpx_clang_version VERSION_LESS 22.1)
  message(
    FATAL_ERROR
      "Building HPX with CUDA on Windows requires clang 22.1 or newer, found ${_hpx_clang_version} (${HPX_CLANG_EXECUTABLE}), set HPX_CLANG_ROOT"
  )
endif()
unset(_hpx_clang_version)

set(CMAKE_C_COMPILER "${HPX_CLANG_EXECUTABLE}")
set(CMAKE_CXX_COMPILER "${HPX_CLANGXX_EXECUTABLE}")
set(CMAKE_CUDA_COMPILER "${HPX_CLANG_EXECUTABLE}")

if(NOT CUDAToolkit_ROOT AND DEFINED ENV{CUDA_PATH})
  file(TO_CMAKE_PATH "$ENV{CUDA_PATH}" CUDAToolkit_ROOT)
endif()

if(CUDAToolkit_ROOT)
  set(CUDAToolkit_ROOT
      "${CUDAToolkit_ROOT}"
      CACHE PATH "The CUDA toolkit to use"
  )
  # clang finds the CUDA toolkit in the standard locations only
  string(APPEND CMAKE_CUDA_FLAGS_INIT " \"--cuda-path=${CUDAToolkit_ROOT}\"")
endif()

# clang supports a CUDA toolkit newer than it knows about for most uses
string(APPEND CMAKE_CUDA_FLAGS_INIT " -Wno-unknown-cuda-version")

# the compiler checks (try_compile) load this file again: they must select the
# same clang and CUDA toolkit
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES HPX_CLANG_ROOT
     CUDAToolkit_ROOT
)
