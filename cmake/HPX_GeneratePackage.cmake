# Copyright (c) 2018 Christopher Hinz
# Copyright (c) 2014 Thomas Heller
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(CMakePackageConfigHelpers)
include(HPX_GeneratePackageUtils)

set(CMAKE_DIR
    "cmake-${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}"
    CACHE STRING "directory (in share), where to put FindHPX cmake module"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/${HPX_PACKAGE_NAME}ConfigVersion.cmake"
  VERSION ${HPX_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Export HPXInternalTargets in the build directory. Use the EXPORT signature so
# CMake also generates the per-target C++ module metadata files.
set(_cxx_modules_directory_arg)
if(HPX_WITH_CXX_MODULES)
  set(_cxx_modules_directory_arg CXX_MODULES_DIRECTORY cxx-modules)
endif()
export(
  TARGETS ${HPX_EXPORT_INTERNAL_TARGETS}
  NAMESPACE HPXInternal::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXInternalTargets.cmake"
       ${_cxx_modules_directory_arg}
)

# Export HPXInternalTargets in the install directory
install(
  EXPORT HPXInternalTargets
  NAMESPACE HPXInternal::
  FILE HPXInternalTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
  COMPONENT cmake
)

# Export HPXTargets in the build directory. Use the EXPORT signature so CMake
# also generates the per-target C++ module metadata files.
export(
  TARGETS ${HPX_EXPORT_TARGETS}
  NAMESPACE HPX::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXTargets.cmake"
       ${_cxx_modules_directory_arg}
)

# Add aliases with the namespace for use within HPX
foreach(export_target ${HPX_EXPORT_TARGETS})
  add_library(HPX::${export_target} ALIAS ${export_target})
endforeach()

foreach(export_target ${HPX_EXPORT_INTERNAL_TARGETS})
  add_library(HPXInternal::${export_target} ALIAS ${export_target})
endforeach()

# Export HPXTargets in the install directory
install(
  EXPORT HPXTargets
  NAMESPACE HPX::
  FILE HPXTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
  COMPONENT cmake
)

# Install dir
set(HPX_CONFIG_IS_INSTALL ON)
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/templates/${HPX_PACKAGE_NAME}Config.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${HPX_PACKAGE_NAME}Config.cmake"
  ESCAPE_QUOTES
  @ONLY
)
set(HPX_CONF_PREFIX ${CMAKE_INSTALL_PREFIX})
if(HPX_WITH_PKGCONFIG)
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/templates/hpxcxx.in
    "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/hpxcxx" @ONLY
  )
endif()

# Build dir
set(HPX_CONFIG_IS_INSTALL OFF)
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/templates/${HPX_PACKAGE_NAME}Config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/${HPX_PACKAGE_NAME}Config.cmake"
  ESCAPE_QUOTES
  @ONLY
)
set(HPX_CONF_PREFIX ${PROJECT_BINARY_DIR})
if(HPX_WITH_PKGCONFIG)
  configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/templates/hpxcxx.in
    "${CMAKE_CURRENT_BINARY_DIR}/bin/hpxcxx" @ONLY
  )
endif()
unset(HPX_CONFIG_IS_INSTALL)

# Configure macros for the install dir ...
set(HPX_CMAKE_MODULE_PATH "\${CMAKE_CURRENT_LIST_DIR}")
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/templates/HPXMacros.cmake.in
  "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/HPXMacros.cmake" ESCAPE_QUOTES
  @ONLY
)
# ... and the build dir
set(HPX_CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake"
                          "${HPX_CMAKE_ADDITIONAL_MODULE_PATH_BUILD}"
)
configure_file(
  ${CMAKE_CURRENT_LIST_DIR}/templates/HPXMacros.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/HPXMacros.cmake"
  ESCAPE_QUOTES @ONLY
)

install(
  FILES
    "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/${HPX_PACKAGE_NAME}Config.cmake"
    "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/HPXMacros.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/lib/cmake/${HPX_PACKAGE_NAME}/${HPX_PACKAGE_NAME}ConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
  COMPONENT cmake
)

if(HPX_WITH_PKGCONFIG)
  install(
    FILES "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/hpxcxx"
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT runtime
    PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE
  )

  add_library(hpx_pkgconfig_application INTERFACE)
  target_link_libraries(
    hpx_pkgconfig_application INTERFACE hpx hpx_wrap hpx_init
  )
  target_compile_definitions(
    hpx_pkgconfig_application INTERFACE HPX_APPLICATION_EXPORTS
  )
  target_compile_options(
    hpx_pkgconfig_application INTERFACE "-std=c++${HPX_CXX_STANDARD}"
  )

  add_library(hpx_pkgconfig_component INTERFACE)
  target_compile_definitions(
    hpx_pkgconfig_component INTERFACE HPX_COMPONENT_EXPORTS
  )
  target_link_libraries(hpx_pkgconfig_component INTERFACE hpx)
  target_compile_options(
    hpx_pkgconfig_component INTERFACE "-std=c++${HPX_CXX_STANDARD}"
  )

  set(exclude_targets hpx_interface Threads::Threads)

  # Generate the pkconfig files for HPX_APPLICATION (both for build and install)
  hpx_generate_pkgconfig_from_target(
    hpx_pkgconfig_application hpx_application TRUE EXCLUDE ${exclude_targets}
  )
  hpx_generate_pkgconfig_from_target(
    hpx_pkgconfig_application hpx_application FALSE EXCLUDE ${exclude_targets}
  )
  # Generate the pkconfig files for HPX_COMPONENT (both for build and install)
  hpx_generate_pkgconfig_from_target(
    hpx_pkgconfig_component hpx_component TRUE EXCLUDE ${exclude_targets}
  )
  hpx_generate_pkgconfig_from_target(
    hpx_pkgconfig_component hpx_component FALSE EXCLUDE ${exclude_targets}
  )

  get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(is_multi_config)
    set(config_types ${CMAKE_CONFIGURATION_TYPES})
    if(NOT config_types)
      set(config_types Debug Release RelWithDebInfo MinSizeRel)
    endif()

    foreach(config_type ${config_types})
      string(TOLOWER ${config_type} config_lower)
      install(
        FILES ${OUTPUT_DIR_PC}/hpx_application_${config_lower}.pc
              ${OUTPUT_DIR_PC}/hpx_component_${config_lower}.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        COMPONENT pkgconfig
      )
    endforeach()
  else()
    string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)
    install(
      FILES ${OUTPUT_DIR_PC}/hpx_application_${build_type}.pc
            ${OUTPUT_DIR_PC}/hpx_component_${build_type}.pc
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
      COMPONENT pkgconfig
    )
  endif()
  # Temporary (to deprecate gradually)
  install(
    FILES ${OUTPUT_DIR_PC}/hpx_application.pc ${OUTPUT_DIR_PC}/hpx_component.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    CONFIGURATIONS Release
    COMPONENT pkgconfig
  )
endif()
