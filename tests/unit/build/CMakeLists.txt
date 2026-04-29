# Copyright (c) 2014 Thomas Heller
# Copyright (c) 2018 Nikunj Gupta
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# If build of external build tests is disabled, exit now
if(NOT HPX_WITH_TESTS_EXTERNAL_BUILD)
  return()
endif()

# Try building an external cmake based project ...
function(
  create_cmake_test
  name
  using_install_dir
  hpx_dir
  setup_type
  build_type
  test_dir
)
  set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
  set(ADDITIONAL_CMAKE_OPTIONS -DUSING_INSTALL_DIR=${using_install_dir})
  set(ADDITIONAL_CMAKE_OPTIONS -DSETUP_TYPE=${setup_type})
  if(CMAKE_TOOLCHAIN_FILE)
    set(ADDITIONAL_CMAKE_OPTIONS ${ADDITIONAL_CMAKE_OPTIONS}
                                 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
    )
  endif()
  if(CMAKE_MAKE_COMMAND)
    set(ADDITIONAL_CMAKE_OPTIONS ${ADDITIONAL_CMAKE_OPTIONS}
                                 -DCMAKE_MAKE_COMMAND=${CMAKE_MAKE_COMMAND}
    )
  endif()
  if(CMAKE_SYSROOT)
    set(ADDITIONAL_CMAKE_OPTIONS ${ADDITIONAL_CMAKE_OPTIONS}
                                 -DCMAKE_SYSROOT=${CMAKE_SYSROOT}
    )
  endif()
  if(NOT MSVC)
    set(ADDITIONAL_CMAKE_OPTIONS ${ADDITIONAL_CMAKE_OPTIONS}
                                 -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
    set(ADDITIONAL_CMAKE_OPTIONS ${ADDITIONAL_CMAKE_OPTIONS}
                                 -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    )
  else()
    set(ADDITIONAL_CMAKE_OPTIONS
        ${ADDITIONAL_CMAKE_OPTIONS}
        -DHPX_OUTPUT_DIRECTORY=${PROJECT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin
    )
  endif()
  set(test_dir "${PROJECT_SOURCE_DIR}/${test_dir}")
  if(HPX_WITH_CLANG_CUDA)
    set(cmake_cuda_compiler -DCMAKE_CUDA_COMPILER=${CMAKE_CUDA_COMPILER})
  endif()
  if(HPX_WITH_CXX_MODULES)
    set(ADDITIONAL_CMAKE_OPTIONS
        ${ADDITIONAL_CMAKE_OPTIONS} -DHPX_WITH_CXX_MODULES=ON
        -DHPX_CMAKE_LOGLEVEL=Debug
    )
  endif()
  add_custom_target(
    ${name}
    COMMAND
      "${CMAKE_CTEST_COMMAND}" --build-and-test "${test_dir}" "${build_dir}"
      --build-generator "${CMAKE_GENERATOR}" --build-noclean --build-options
      -DHPX_DIR=${hpx_dir} -DBOOST_ROOT=${BOOST_ROOT}
      ${ADDITIONAL_CMAKE_OPTIONS} -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS_SAFE}
      -DCMAKE_BUILD_TYPE=${build_type} ${cmake_cuda_compiler} --test-command
      "${CMAKE_CTEST_COMMAND}" --output-on-failure --verbose
    VERBATIM
  )
  add_dependencies(${name} hpx hpx_init hpx_wrap)

  if(HPX_WITH_DISTRIBUTED_RUNTIME)
    add_dependencies(${name} iostreams_component)
  endif()

  if(MSVC)
    set_target_properties(${name} PROPERTIES FOLDER "Tests/Unit/Build")
  endif()
endfunction()

function(create_pkgconfig_test name hpx_dir)
  set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
  add_custom_target(
    ${name}.make_build_dir
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${build_dir}"
    VERBATIM
  )
  set(PKGCONFIG_CXX_FLAGS "${CMAKE_CXX_FLAGS_SAFE}")
  if(HPX_WITH_CXX_MODULES)
    set(PKGCONFIG_CXX_FLAGS
        "${PKGCONFIG_CXX_FLAGS} -DHPX_HAVE_FORCE_NO_CXX_MODULES"
    )
  endif()

  add_custom_target(
    ${name}.make_compile
    COMMAND
      "${CMAKE_COMMAND}" -E chdir "${build_dir}" make -f
      "${PROJECT_SOURCE_DIR}/examples/hello_world_component/Makefile"
      SRC_DIR=${PROJECT_SOURCE_DIR}/examples/hello_world_component
      HPX_DIR=${hpx_dir} CXX=${CMAKE_CXX_COMPILER} CC=${CMAKE_C_COMPILER}
      "CXX_FLAGS=${PKGCONFIG_CXX_FLAGS}" PKG_CONFIG=${PKG_CONFIG_EXECUTABLE}
      BUILD_TYPE=$<CONFIGURATION>
    VERBATIM
  )
  add_dependencies(${name}.make_compile ${name}.make_build_dir hpx hpx_init)
  if(HPX_WITH_DISTRIBUTED_RUNTIME)
    add_dependencies(${name}.make_compile iostreams_component)
  endif()

  add_hpx_pseudo_target(${name})
  add_hpx_pseudo_dependencies(${name} ${name}.make_compile)
endfunction()

if(MSVC)
  set(build_types ${CMAKE_BUILD_TYPE})
else()
  set(build_types Debug Release RelWithDebInfo)
  # lld (which HIP uses) does not support the -Os flags in older versions. We do
  # not attempt to detect the linker in the general case, but disable the
  # MinSizeRel test here since we know that HIP only uses lld.
  if(NOT HPX_WITH_HIP OR (HIP_VERSION VERSION_GREATER_EQUAL "4.0.0"))
    list(APPEND build_types MinSizeRel)
  endif()
endif()

if(NOT MSVC)
  foreach(build_type ${build_types})
    string(TOLOWER "${build_type}" build_type_lc)

    create_cmake_test(
      cmake_${build_type_lc}_build_dir_targets_test FALSE
      "${PROJECT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}" TARGETS
      ${build_type} "examples/hello_world_component"
    )

    create_cmake_test(
      cmake_${build_type_lc}_build_dir_macros_test FALSE
      "${PROJECT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}" MACROS
      ${build_type} "examples/hello_world_component"
    )

    create_cmake_test(
      cmake_${build_type_lc}_install_dir_targets_test TRUE
      "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}" TARGETS
      ${build_type} "examples/hello_world_component"
    )

    create_cmake_test(
      cmake_${build_type_lc}_install_dir_macros_test TRUE
      "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}" MACROS
      ${build_type} "examples/hello_world_component"
    )

    if(HPX_WITH_DYNAMIC_HPX_MAIN)
      # Google test emulation
      create_cmake_test(
        cmake_${build_type_lc}_build_dir_gtest_emulation_test FALSE
        "${PROJECT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}" TARGETS
        ${build_type} "examples/gtest_emulation"
      )

      create_cmake_test(
        cmake_${build_type_lc}_install_dir_gtest_emulation_test TRUE
        "${CMAKE_INSTALL_FULL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}" TARGETS
        ${build_type} "examples/gtest_emulation"
      )
    endif()
  endforeach()
endif()

set(cmake_tests build_dir_targets install_dir_targets build_dir_macros
                install_dir_macros
)
if(HPX_WITH_DYNAMIC_HPX_MAIN)
  list(APPEND cmake_tests build_dir_gtest_emulation install_dir_gtest_emulation)
endif()

add_hpx_pseudo_target(tests.unit.build.cmake)
foreach(build_type ${build_types})
  string(TOLOWER "${build_type}" build_type_lc)
  add_hpx_pseudo_target(tests.unit.build.cmake.${build_type_lc})
  foreach(test ${cmake_tests})
    add_hpx_pseudo_target(tests.unit.build.cmake.${build_type_lc}.${test})
    add_hpx_pseudo_dependencies(
      tests.unit.build.cmake.${build_type_lc}.${test}
      cmake_${build_type_lc}_${test}_test
    )
    add_hpx_pseudo_dependencies(
      tests.unit.build.cmake.${build_type_lc}
      tests.unit.build.cmake.${build_type_lc}.${test}
    )
  endforeach()
  add_hpx_pseudo_dependencies(
    tests.unit.build.cmake tests.unit.build.cmake.${build_type_lc}
  )
endforeach()
add_hpx_pseudo_dependencies(tests.unit.build tests.unit.build.cmake)

if(HPX_WITH_PKGCONFIG AND HPX_WITH_DISTRIBUTED_RUNTIME)
  find_package(PkgConfig)
  if(PKGCONFIG_FOUND)
    create_pkgconfig_test(
      pkgconfig_build_dir_test "${PROJECT_BINARY_DIR}/lib/pkgconfig"
    )
    create_pkgconfig_test(
      pkgconfig_install_dir_test "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig"
    )

    set(pkgconfig_tests build_dir install_dir)
    add_hpx_pseudo_target(tests.unit.build.pkgconfig)
    foreach(test ${pkgconfig_tests})
      add_hpx_pseudo_target(tests.unit.build.pkgconfig.${test})
      add_hpx_pseudo_dependencies(
        tests.unit.build.pkgconfig.${test} pkgconfig_${test}_test
      )
      add_hpx_pseudo_dependencies(
        tests.unit.build.pkgconfig tests.unit.build.pkgconfig.${test}
      )
    endforeach()
    add_hpx_pseudo_dependencies(tests.unit.build tests.unit.build.pkgconfig)
  endif()
endif()

function(create_pkgconfig_generation_test name test_dir)
  set(build_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
  add_custom_target(
    ${name}
    COMMAND
      "${CMAKE_CTEST_COMMAND}" --build-and-test
      "${PROJECT_SOURCE_DIR}/${test_dir}" "${build_dir}" --build-generator
      "${CMAKE_GENERATOR}" --build-noclean --build-options
      -DHPX_SOURCE_DIR=${PROJECT_SOURCE_DIR} -DCMAKE_BUILD_TYPE=$<CONFIGURATION>
      --test-command "${CMAKE_CTEST_COMMAND}" --output-on-failure
    VERBATIM
  )

  if(MSVC)
    set_target_properties(${name} PROPERTIES FOLDER "Tests/Unit/Build")
  endif()
endfunction()

if(HPX_WITH_PKGCONFIG)
  create_pkgconfig_generation_test(
    tests.unit.build.pkgconfig_generation
    "tests/unit/build/pkgconfig_generation"
  )
  add_hpx_pseudo_dependencies(
    tests.unit.build tests.unit.build.pkgconfig_generation
  )
endif()
