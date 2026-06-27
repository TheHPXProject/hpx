# Copyright (c) 2011 Bryce Lelbach
# Copyright (c) 2014 Thomas Heller
# Copyright (c) 2017 Denis Blank
# Copyright (c) 2017 Google
# Copyright (c) 2017 Taeguk Kwon
# Copyright (c) 2020 Giannis Gonidelis
# Copyright (c) 2021-2025 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(HPX_ADDCONFIGTEST_LOADED TRUE)

include(CheckLibraryExists)

function(add_hpx_config_test variable)
  set(options FILE EXECUTE CUDA)
  set(one_value_args SOURCE ROOT CMAKECXXFEATURE)
  set(multi_value_args
      CXXFLAGS
      INCLUDE_DIRECTORIES
      LINK_DIRECTORIES
      COMPILE_DEFINITIONS
      LIBRARIES
      ARGS
      DEFINITIONS
      REQUIRED
  )
  cmake_parse_arguments(
    ${variable} "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN}
  )

  set(_run_msg)
  # Check CMake feature tests if the user didn't override the value of this
  # variable:
  if(NOT DEFINED ${variable} AND NOT ${variable}_CUDA)
    if(${variable}_CMAKECXXFEATURE)
      # We don't have to run our own feature test if there is a corresponding
      # cmake feature test and cmake reports the feature is supported on this
      # platform.
      list(FIND CMAKE_CXX_COMPILE_FEATURES ${${variable}_CMAKECXXFEATURE} __pos)
      if(NOT ${__pos} EQUAL -1)
        set(${variable}
            TRUE
            CACHE INTERNAL ""
        )
        set(_run_msg "Success (cmake feature test)")
      endif()
    endif()
  endif()

  if(NOT DEFINED ${variable})
    file(MAKE_DIRECTORY
         "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/config_tests"
    )

    string(TOLOWER "${variable}" variable_lc)
    if(${variable}_FILE)
      if(${variable}_ROOT)
        set(test_source "${${variable}_ROOT}/share/hpx/${${variable}_SOURCE}")
      else()
        set(test_source "${PROJECT_SOURCE_DIR}/${${variable}_SOURCE}")
      endif()
    else()
      if(${variable}_CUDA)
        set(test_source
            "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/config_tests/${variable_lc}.cu"
        )
      else()
        set(test_source
            "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/config_tests/${variable_lc}.cpp"
        )
      endif()
      file(WRITE "${test_source}" "${${variable}_SOURCE}\n")
    endif()
    set(test_binary
        ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/config_tests/${variable_lc}
    )

    get_directory_property(CONFIG_TEST_INCLUDE_DIRS INCLUDE_DIRECTORIES)
    get_directory_property(CONFIG_TEST_LINK_DIRS LINK_DIRECTORIES)
    set(COMPILE_DEFINITIONS_TMP)
    set(CONFIG_TEST_COMPILE_DEFINITIONS)
    get_directory_property(COMPILE_DEFINITIONS_TMP COMPILE_DEFINITIONS)
    foreach(def IN LISTS COMPILE_DEFINITIONS_TMP
                         ${variable}_COMPILE_DEFINITIONS
    )
      set(CONFIG_TEST_COMPILE_DEFINITIONS
          "${CONFIG_TEST_COMPILE_DEFINITIONS} -D${def}"
      )
    endforeach()
    get_property(
      HPX_TARGET_COMPILE_OPTIONS_PUBLIC_VAR GLOBAL
      PROPERTY HPX_TARGET_COMPILE_OPTIONS_PUBLIC
    )
    get_property(
      HPX_TARGET_COMPILE_OPTIONS_PRIVATE_VAR GLOBAL
      PROPERTY HPX_TARGET_COMPILE_OPTIONS_PRIVATE
    )
    set(HPX_TARGET_COMPILE_OPTIONS_VAR
        ${HPX_TARGET_COMPILE_OPTIONS_PUBLIC_VAR}
        ${HPX_TARGET_COMPILE_OPTIONS_PRIVATE_VAR}
    )
    foreach(_flag ${HPX_TARGET_COMPILE_OPTIONS_VAR})
      if(NOT "${_flag}" MATCHES "^\\$.*")
        set(CONFIG_TEST_COMPILE_DEFINITIONS
            "${CONFIG_TEST_COMPILE_DEFINITIONS} ${_flag}"
        )
      endif()
    endforeach()

    set(CONFIG_TEST_INCLUDE_DIRS ${CONFIG_TEST_INCLUDE_DIRS}
                                 ${${variable}_INCLUDE_DIRECTORIES}
    )
    set(CONFIG_TEST_LINK_DIRS ${CONFIG_TEST_LINK_DIRS}
                              ${${variable}_LINK_DIRECTORIES}
    )

    set(CONFIG_TEST_LINK_LIBRARIES ${${variable}_LIBRARIES})

    set(additional_cmake_flags)
    if(MSVC)
      set(additional_cmake_flags "-WX")
    else()
      set(additional_cmake_flags "-Werror")
    endif()

    if(${variable}_EXECUTE)
      if(NOT CMAKE_CROSSCOMPILING)
        set(CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} ${additional_cmake_flags} ${${variable}_CXXFLAGS}"
        )
        # cmake-format: off
        try_run(
          ${variable}_RUN_RESULT ${variable}_COMPILE_RESULT
          ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/config_tests
          ${test_source}
          COMPILE_DEFINITIONS ${CONFIG_TEST_COMPILE_DEFINITIONS}
          CMAKE_FLAGS
            "-DINCLUDE_DIRECTORIES=${CONFIG_TEST_INCLUDE_DIRS}"
            "-DLINK_DIRECTORIES=${CONFIG_TEST_LINK_DIRS}"
            "-DLINK_LIBRARIES=${CONFIG_TEST_LINK_LIBRARIES}"
          CXX_STANDARD ${HPX_CXX_STANDARD}
          CXX_STANDARD_REQUIRED ON
          CXX_EXTENSIONS FALSE
          RUN_OUTPUT_VARIABLE ${variable}_OUTPUT
          ARGS ${${variable}_ARGS}
        )
        # cmake-format: on
        if(${variable}_COMPILE_RESULT AND NOT ${variable}_RUN_RESULT)
          set(${variable}_RESULT TRUE)
        else()
          set(${variable}_RESULT FALSE)
        endif()
      else()
        set(${variable}_RESULT FALSE)
      endif()
    else()
      if(HPX_WITH_CUDA)
        set(cuda_parameters CUDA_STANDARD ${CMAKE_CUDA_STANDARD})
      endif()
      set(CMAKE_CXX_FLAGS
          "${CMAKE_CXX_FLAGS} ${additional_cmake_flags} ${${variable}_CXXFLAGS}"
      )
      set(CMAKE_CUDA_FLAGS
          "${CMAKE_CUDA_FLAGS} ${additional_cmake_flags} ${${variable}_CXXFLAGS}"
      )
      # cmake-format: off
      try_compile(
        ${variable}_RESULT
        ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/config_tests
        ${test_source}
        COMPILE_DEFINITIONS ${CONFIG_TEST_COMPILE_DEFINITIONS}
        CMAKE_FLAGS
          "-DINCLUDE_DIRECTORIES=${CONFIG_TEST_INCLUDE_DIRS}"
          "-DLINK_DIRECTORIES=${CONFIG_TEST_LINK_DIRS}"
          "-DLINK_LIBRARIES=${CONFIG_TEST_LINK_LIBRARIES}"
        OUTPUT_VARIABLE ${variable}_OUTPUT
        CXX_STANDARD ${HPX_CXX_STANDARD}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS FALSE
        ${cuda_parameters}
        COPY_FILE ${test_binary}
      )
      # cmake-format: on
      hpx_debug("Compile test: ${variable}")
      hpx_debug("Compilation output: ${${variable}_OUTPUT}")
    endif()

    set(_run_msg "Success")
  else()
    set(${variable}_RESULT ${${variable}})
    if(NOT _run_msg)
      set(_run_msg "pre-set to ${${variable}}")
    endif()
  endif()

  string(TOUPPER "${variable}" variable_uc)
  set(_msg "Performing Test ${variable_uc}")

  if(${variable}_RESULT)
    set(_msg "${_msg} - ${_run_msg}")
  else()
    set(_msg "${_msg} - Failed")
  endif()

  set(${variable}
      ${${variable}_RESULT}
      CACHE INTERNAL ""
  )
  hpx_info(${_msg})

  if(${variable}_RESULT)
    foreach(definition ${${variable}_DEFINITIONS})
      hpx_add_config_define(${definition})
    endforeach()
  elseif(${variable}_REQUIRED)
    hpx_warn("Test failed, detailed output:\n\n${${variable}_OUTPUT}")
    hpx_error(${${variable}_REQUIRED})
  endif()
endfunction()

# ##############################################################################
function(hpx_cpuid target variable)
  add_hpx_config_test(
    ${variable}
    SOURCE cmake/tests/cpuid.cpp
    COMPILE_DEFINITIONS "${boost_include_dir}" "${include_dir}"
    FILE EXECUTE
    ARGS "${target}" ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_unistd_h)
  add_hpx_config_test(
    HPX_WITH_UNISTD_H
    SOURCE cmake/tests/unistd_h.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_builtin_forward_move)
  add_hpx_config_test(
    HPX_WITH_BUILTIN_FORWARD_MOVE
    SOURCE cmake/tests/builtin_forward_move.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_builtin_frame_address)
  add_hpx_config_test(
    HPX_WITH_BUILTIN_FRAME_ADDRESS
    SOURCE cmake/tests/builtin_frame_address.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_libfun_std_experimental_optional)
  add_hpx_config_test(
    HPX_WITH_LIBFUN_EXPERIMENTAL_OPTIONAL
    SOURCE cmake/tests/libfun_std_experimental_optional.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
# ATOMIC_FLAG_INIT is deprecated starting C++20. Here we check whether using it
# will cause failures (-Werror,-Wdeprecated-pragma), so we check for it
# separately.
function(hpx_check_for_cxx11_atomic_init_flag)
  add_hpx_config_test(
    HPX_WITH_CXX11_ATOMIC_INIT_FLAG
    SOURCE cmake/tests/cxx11_atomic_init_flag.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx11_std_atomic)
  # Make sure HPX_HAVE_LIBATOMIC is removed from the cache if necessary
  if(NOT HPX_WITH_CXX11_ATOMIC)
    unset(HPX_CXX11_STD_ATOMIC_LIBRARIES CACHE)
  endif()

  # we shouldn't delete this key if the user has pre-set the atomic libraries to
  # use
  if(HPX_CXX11_STD_ATOMIC_LIBRARIES)
    set(__std_atomic_libraries
        TRUE
        CACHE STRING "std::atomics need separate library" FORCE
    )
  endif()

  # first see if we can build atomics with no -latomics
  add_hpx_config_test(
    HPX_WITH_CXX11_ATOMIC
    SOURCE cmake/tests/cxx11_std_atomic.cpp
    LIBRARIES ${HPX_CXX11_STD_ATOMIC_LIBRARIES}
    FILE ${ARGN}
  )

  if(NOT MSVC)
    # Sometimes linking against libatomic is required, if the platform doesn't
    # support lock-free atomics. We already know that MSVC works
    if(NOT HPX_WITH_CXX11_ATOMIC)
      set(HPX_CXX11_STD_ATOMIC_LIBRARIES
          atomic
          CACHE STRING "std::atomics need separate library" FORCE
      )
      unset(HPX_WITH_CXX11_ATOMIC CACHE)
      add_hpx_config_test(
        HPX_WITH_CXX11_ATOMIC
        SOURCE cmake/tests/cxx11_std_atomic.cpp
        LIBRARIES ${HPX_CXX11_STD_ATOMIC_LIBRARIES}
        FILE ${ARGN}
      )
      if(HPX_WITH_CXX11_ATOMIC)
        set(__std_atomic_libraries
            TRUE
            CACHE STRING "std::atomics need separate library" FORCE
        )
      endif()
    endif()
  endif()

  if(NOT HPX_WITH_CXX11_ATOMIC)
    hpx_error("HPX needs support for C++11 std::atomic")
  endif()
endfunction()

# Separately check for 128 bit atomics
function(hpx_check_for_cxx11_std_atomic_128bit)
  add_hpx_config_test(
    HPX_WITH_CXX11_ATOMIC_128BIT
    SOURCE cmake/tests/cxx11_std_atomic_128bit.cpp
    LIBRARIES ${HPX_CXX11_STD_ATOMIC_LIBRARIES}
    FILE EXECUTE
  )
  if(NOT MSVC)
    # Sometimes linking against libatomic is required, if the platform doesn't
    # support lock-free atomics. We already know that MSVC works
    if(NOT HPX_WITH_CXX11_ATOMIC_128BIT)
      set(HPX_CXX11_STD_ATOMIC_LIBRARIES
          atomic
          CACHE STRING "std::atomics need separate library" FORCE
      )
      unset(HPX_WITH_CXX11_ATOMIC_128BIT CACHE)
      add_hpx_config_test(
        HPX_WITH_CXX11_ATOMIC_128BIT
        SOURCE cmake/tests/cxx11_std_atomic_128bit.cpp
        LIBRARIES ${HPX_CXX11_STD_ATOMIC_LIBRARIES}
        FILE EXECUTE
      )
      if(NOT HPX_WITH_CXX11_ATOMIC_128BIT)
        # Adding -latomic did not help, so we don't attempt to link to it later
        # but only if normal atomics don't require it
        if(NOT __std_atomic_libraries)
          unset(HPX_CXX11_STD_ATOMIC_LIBRARIES CACHE)
        endif()
        unset(HPX_WITH_CXX11_ATOMIC_128BIT CACHE)
      endif()
    endif()
  endif()
  unset(__std_atomic_libraries CACHE)
endfunction()

# ##############################################################################
function(hpx_check_for_cxx14_delete_operator_with_size)
  add_hpx_config_test(
    HPX_WITH_CXX14_DELETE_OPERATOR_WITH_SIZE
    SOURCE cmake/tests/cxx14_delete_operator_with_size.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_c11_aligned_alloc)
  add_hpx_config_test(
    HPX_WITH_C11_ALIGNED_ALLOC
    SOURCE cmake/tests/c11_aligned_alloc.cpp
    FILE ${ARGN}
  )
endfunction()

function(hpx_check_for_cxx17_std_aligned_alloc)
  add_hpx_config_test(
    HPX_WITH_CXX17_STD_ALIGNED_ALLOC
    SOURCE cmake/tests/cxx17_std_aligned_alloc.cpp
    FILE ${ARGN}
  )
endfunction()

function(hpx_check_for_cxx17_std_execution_policies)
  add_hpx_config_test(
    HPX_WITH_CXX17_STD_EXECUTION_POLICES
    SOURCE cmake/tests/cxx17_std_execution_policies.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx11_std_quick_exit)
  add_hpx_config_test(
    HPX_WITH_CXX11_STD_QUICK_EXIT
    SOURCE cmake/tests/cxx11_std_quick_exit.cpp
    FILE EXECUTE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx17_aligned_new)
  add_hpx_config_test(
    HPX_WITH_CXX17_ALIGNED_NEW
    SOURCE cmake/tests/cxx17_aligned_new.cpp
    FILE ${ARGN}
    REQUIRED
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx17_filesystem)
  add_hpx_config_test(
    HPX_WITH_CXX17_FILESYSTEM
    SOURCE cmake/tests/cxx17_filesystem.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx17_hardware_destructive_interference_size)
  add_hpx_config_test(
    HPX_WITH_CXX17_HARDWARE_DESTRUCTIVE_INTERFERENCE_SIZE
    SOURCE cmake/tests/cxx17_hardware_destructive_interference_size.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx17_std_transform_scan)
  add_hpx_config_test(
    HPX_WITH_CXX17_STD_TRANSFORM_SCAN_ALGORITHMS
    SOURCE cmake/tests/cxx17_std_transform_scan_algorithms.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx17_std_scan)
  add_hpx_config_test(
    HPX_WITH_CXX17_STD_SCAN_ALGORITHMS
    SOURCE cmake/tests/cxx17_std_scan_algorithms.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx17_copy_elision)
  add_hpx_config_test(
    HPX_WITH_CXX17_COPY_ELISION
    SOURCE cmake/tests/cxx17_copy_elision.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx17_optional_copy_elision)
  add_hpx_config_test(
    HPX_WITH_CXX17_OPTIONAL_COPY_ELISION
    SOURCE cmake/tests/cxx17_optional_copy_elision.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx20_coroutines)
  if(NOT HPX_WITH_CXX20_COROUTINES)
    unset(HPX_CXX20_COROUTINES_FLAGS)
  endif()

  add_hpx_config_test(
    HPX_WITH_CXX20_COROUTINES
    SOURCE cmake/tests/cxx20_coroutines.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_pthread_setname_np)
  add_hpx_config_test(
    HPX_WITH_PTHREAD_SETNAME_NP
    SOURCE cmake/tests/set_thread_name.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx20_experimental_simd)
  add_hpx_config_test(
    HPX_WITH_CXX20_EXPERIMENTAL_SIMD
    SOURCE cmake/tests/cxx20_experimental_simd.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx20_no_unique_address_attribute)
  add_hpx_config_test(
    HPX_WITH_CXX20_NO_UNIQUE_ADDRESS_ATTRIBUTE
    SOURCE cmake/tests/cxx20_no_unique_address_attribute.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx20_std_execution_policies)
  add_hpx_config_test(
    HPX_WITH_CXX20_STD_EXECUTION_POLICES
    SOURCE cmake/tests/cxx20_std_execution_policies.cpp
    FILE ${ARGN}
  )
endfunction()

function(hpx_check_for_cxx23_std_generator)
  add_hpx_config_test(
    HPX_WITH_CXX23_STD_GENERATOR
    SOURCE cmake/tests/cxx23_std_generator.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx23_deprecated_std_aligned_storage)
  add_hpx_config_test(
    HPX_WITH_CXX23_DEPRECATED_STD_ALIGNED_STORAGE
    SOURCE cmake/tests/cxx23_deprecated_std_aligned_storage.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx26_experimental_scope)
  add_hpx_config_test(
    HPX_WITH_CXX26_EXPERIMENTAL_SCOPE
    SOURCE cmake/tests/cxx26_experimental_scope.cpp
    FILE ${ARGN}
  )
endfunction()

function(hpx_check_for_cxx26_contracts)
  add_hpx_config_test(
    HPX_WITH_CXX26_CONTRACTS
    SOURCE cmake/tests/cxx26_contracts.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx26_reflection)
  add_hpx_config_test(
    HPX_WITH_CXX26_REFLECTION
    SOURCE cmake/tests/cxx26_reflection.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx_lambda_capture_decltype)
  add_hpx_config_test(
    HPX_WITH_CXX_LAMBDA_CAPTURE_DECLTYPE
    SOURCE cmake/tests/cxx_lambda_capture_decltype.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_mm_prefetch)
  add_hpx_config_test(
    HPX_WITH_MM_PREFETCH
    SOURCE cmake/tests/mm_prefetch.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx23_std_unordered_transparent_erase)
  add_hpx_config_test(
    HPX_WITH_CXX23_STD_UNORDERED_TRANSPARENT_ERASE
    SOURCE cmake/tests/cxx23_std_unordered_transparent_erase.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_cxx26_std_unordered_transparent_lookup)
  add_hpx_config_test(
    HPX_WITH_CXX26_STD_UNORDERED_TRANSPARENT_LOOKUP
    SOURCE cmake/tests/cxx26_std_unordered_transparent_lookup.cpp
    FILE ${ARGN}
  )
endfunction()

# ##############################################################################
function(hpx_check_for_stable_inplace_merge)
  add_hpx_config_test(
    HPX_WITH_STABLE_INPLACE_MERGE
    SOURCE cmake/tests/stable_inplace_merge.cpp
    FILE ${ARGN}
  )
endfunction()
