# Copyright (c) 2019-2023 ETH Zurich
# Copyright (c) 2019-2025 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

include(HPX_CollectStdHeaders)
include(HPX_ExportTargets)
include(HPX_Message)
include(HPX_Option)
include(HPX_PrintSummary)
include(HPX_CXXModules)
include(HPX_ConfigureIfChanged)

function(add_hpx_module libname modulename)
  # Retrieve arguments
  set(options CUDA CONFIG_FILES NO_CONFIG_IN_GENERATED_HEADERS)
  set(one_value_args GLOBAL_HEADER_GEN GLOBAL_HEADER_MODULE_GEN)
  set(multi_value_args
      SOURCES
      HEADERS
      MACRO_HEADERS
      COMPAT_HEADERS
      GENERATED_HEADERS
      OBJECTS
      DEPENDENCIES
      PRIVATE_DEPENDENCIES
      INCLUDE_DIRECTORIES
      MODULE_DEPENDENCIES
      CMAKE_SUBDIRS
      EXCLUDE_FROM_GLOBAL_HEADER
      ADD_TO_GLOBAL_HEADER
  )
  cmake_parse_arguments(
    ${modulename} "${options}" "${one_value_args}" "${multi_value_args}"
    ${ARGN}
  )
  if(${modulename}_UNPARSED_ARGUMENTS)
    message(
      AUTHOR_WARNING
        "Arguments were not used by the module: ${${modulename}_UNPARSED_ARGUMENTS}"
    )
  endif()

  # Global headers should be always generated except if explicitly disabled
  if("${${modulename}_GLOBAL_HEADER_GEN}" STREQUAL "")
    set(${modulename}_GLOBAL_HEADER_GEN ON)
  endif()

  string(TOUPPER ${libname} libname_upper)
  string(TOUPPER ${modulename} modulename_upper)

  # Mark the module as enabled (see hpx/libs/CMakeLists.txt)
  set(modules ${HPX_ENABLED_MODULES})
  list(APPEND modules ${modulename})
  list(SORT modules)
  list(REMOVE_DUPLICATES modules)

  set(HPX_ENABLED_MODULES
      ${modules}
      CACHE INTERNAL "List of enabled HPX modules" FORCE
  )

  set(modules ${HPX_${libname_upper}_ENABLED_MODULES})
  list(APPEND modules ${modulename})
  list(SORT modules)
  list(REMOVE_DUPLICATES modules)

  set(HPX_${libname_upper}_ENABLED_MODULES
      ${modules}
      CACHE INTERNAL "List of enabled HPX modules in the ${libname} library"
            FORCE
  )

  if(HPX_WITH_CXX_MODULES AND ${modulename}_GLOBAL_HEADER_MODULE_GEN)
    # Mark the module as exposing C++ modules
    set(cxx_modules ${HPX_ENABLED_CXX_MODULES})
    list(APPEND cxx_modules ${modulename})
    list(SORT cxx_modules)
    list(REMOVE_DUPLICATES cxx_modules)

    set(HPX_ENABLED_CXX_MODULES
        ${cxx_modules}
        CACHE INTERNAL "List of HPX modules that are exposed as a C++ module"
              FORCE
    )

    set(cxx_modules ${HPX_${libname_upper}_ENABLED_CXX_MODULES})
    list(APPEND cxx_modules ${modulename})
    list(SORT cxx_modules)
    list(REMOVE_DUPLICATES cxx_modules)

    set(HPX_${libname_upper}_ENABLED_CXX_MODULES
        ${cxx_modules}
        CACHE
          INTERNAL
          "List of HPX modules that are exposed as a C++ module in the ${libname} library"
          FORCE
    )
  endif()

  # Main directories of the module
  set(SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")
  set(HEADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include")

  hpx_debug("Add module ${modulename}: SOURCE_ROOT: ${SOURCE_ROOT}")
  hpx_debug("Add module ${modulename}: HEADER_ROOT: ${HEADER_ROOT}")

  if(HPX_WITH_MODULE_COMPATIBILITY_HEADERS AND ${modulename}_COMPAT_HEADERS)
    set(COMPAT_HEADER_ROOT "${CMAKE_CURRENT_BINARY_DIR}/include_compatibility")
    file(MAKE_DIRECTORY ${COMPAT_HEADER_ROOT})
    hpx_debug(
      "Add module ${modulename}: COMPAT_HEADER_ROOT: ${COMPAT_HEADER_ROOT}"
    )
  endif()

  set(all_headers ${${modulename}_HEADERS})

  # Write full path for the sources files
  list(TRANSFORM ${modulename}_SOURCES PREPEND ${SOURCE_ROOT}/ OUTPUT_VARIABLE
                                                               sources
  )
  list(TRANSFORM ${modulename}_HEADERS PREPEND ${HEADER_ROOT}/ OUTPUT_VARIABLE
                                                               headers
  )
  if(HPX_WITH_MODULE_COMPATIBILITY_HEADERS AND ${modulename}_COMPAT_HEADERS)
    string(REPLACE ";=>;" "=>" ${modulename}_COMPAT_HEADERS
                   "${${modulename}_COMPAT_HEADERS}"
    )
    foreach(compat_header IN LISTS ${modulename}_COMPAT_HEADERS)
      string(REPLACE "=>" ";" compat_header "${compat_header}")
      list(LENGTH compat_header compat_header_length)
      if(NOT compat_header_length EQUAL 2)
        message(FATAL_ERROR "Invalid compatibility header ${compat_header}")
      endif()

      set(config_file)
      if(NOT ${modulename}_NO_CONFIG_IN_GENERATED_HEADERS)
        set(config_file "#include <hpx/config.hpp>\n")
      endif()

      list(GET compat_header 0 old_header)
      list(GET compat_header 1 new_header)
      hpx_configure_if_changed(
        INPUT
          "${PROJECT_SOURCE_DIR}/cmake/templates/compatibility_header.hpp.in"
        OUTPUT "${COMPAT_HEADER_ROOT}/${old_header}"
        CONFIGURE_ARGS @ONLY
      )
      list(APPEND compat_headers "${COMPAT_HEADER_ROOT}/${old_header}")
      list(APPEND all_headers ${old_header})
    endforeach()
  endif()

  # capitalize string
  string(SUBSTRING ${libname} 0 1 first_letter)
  string(TOUPPER ${first_letter} first_letter)
  string(REGEX REPLACE "^.(.*)" "${first_letter}\\1" libname_cap "${libname}")

  # This header generation is disabled for config module specific generated
  # headers are included
  if(${modulename}_GLOBAL_HEADER_GEN)
    if("hpx/modules/${modulename}.hpp" IN_LIST all_headers)
      string(
        CONCAT
          error_message
          "Global header generation turned on for module ${modulename} but the "
          "header \"hpx/modules/${modulename}.hpp\" is also listed explicitly as"
          "a header. Turn off global header generation or remove the "
          "\"hpx/modules/${modulename}.hpp\" file."
      )
      hpx_error(${error_message})
    endif()

    # Add a global include file that include all module headers
    set(global_header
        "${CMAKE_CURRENT_BINARY_DIR}/include/hpx/modules/${modulename}.hpp"
    )
    if(NOT ${modulename}_NO_CONFIG_IN_GENERATED_HEADERS)
      set(module_headers "#include <hpx/config.hpp>\n\n")
      set(module_headers
          "${module_headers}#if defined(HPX_HAVE_MODULE_${modulename_upper})\n"
      )
      set(module_headers
          "${module_headers}#include <hpx/${modulename}/config/defines.hpp>\n"
      )
    endif()
    foreach(header_file ${${modulename}_HEADERS})
      # Exclude the files specified
      if((NOT (${header_file} IN_LIST ${modulename}_EXCLUDE_FROM_GLOBAL_HEADER))
         AND (NOT ("${header_file}" MATCHES "detail")
              OR ${header_file} IN_LIST ${modulename}_ADD_TO_GLOBAL_HEADER)
      )
        set(module_headers "${module_headers}#include <${header_file}>\n")
      endif()
    endforeach(header_file)
    if(NOT ${modulename}_NO_CONFIG_IN_GENERATED_HEADERS)
      set(module_headers "${module_headers}#endif\n")
    endif()

    # Decide output path and template file
    set(module_macro_headers
        "\n#include <hpx/${modulename}/config/defines.hpp>\n"
    )
    set(global_header
        "${CMAKE_CURRENT_BINARY_DIR}/include/hpx/modules/${modulename}.hpp"
    )
    if(HPX_WITH_CXX_MODULES AND ${modulename}_GLOBAL_HEADER_MODULE_GEN)
      # generate list of macro headers to #include
      foreach(header_file ${${modulename}_MACRO_HEADERS})
        set(module_macro_headers
            "${module_macro_headers}#include <${header_file}>\n"
        )
      endforeach()
      set(template_file "global_module_header_modules.hpp.in")
    else()
      set(template_file "global_module_header.hpp.in")
    endif()

    hpx_configure_if_changed(
      INPUT "${HPX_SOURCE_DIR}/cmake/templates/${template_file}"
      OUTPUT "${global_header}"
      CONFIGURE_ARGS @ONLY
    )
    set(generated_headers ${global_header})

    if(HPX_WITH_CXX_MODULES AND ${modulename}_GLOBAL_HEADER_MODULE_GEN)
      # collect all standard header files used by this module
      set(found_includes)
      hpx_collect_std_headers(
        ${modulename}
        SOURCES ${global_header}
        SOURCE_ROOT ${HPX_SOURCE_DIR}/libs/${libname}/${modulename}
        GENERATED_ROOT ${CMAKE_BINARY_DIR}/libs/${libname}/${modulename}
        FOUND_HEADERS found_includes
      )

      set(standard_headers ${HPX_STANDARD_HEADERS})
      list(APPEND standard_headers ${found_includes})
      list(SORT standard_headers)
      list(REMOVE_DUPLICATES standard_headers)

      set(HPX_STANDARD_HEADERS
          ${standard_headers}
          CACHE STRING "List of standard headers #included by HPX modules"
                FORCE
      )
    endif()
  endif()

  # some headers files have to be generated, remove the `.in` file extension
  if(${modulename}_GENERATED_HEADERS)
    set(generated_file_base "${CMAKE_CURRENT_BINARY_DIR}/include")
    if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
      foreach(file_to_generate ${${modulename}_GENERATED_HEADERS})
        configure_file(
          ${HEADER_ROOT}/${file_to_generate}.in
          ${generated_file_base}/${file_to_generate} COPYONLY
        )
        set(generated_headers ${generated_headers}
                              ${generated_file_base}/${file_to_generate}
        )
      endforeach()
    else()
      foreach(file_to_generate ${${modulename}_GENERATED_HEADERS})
        set(generated_headers ${generated_headers}
                              ${generated_file_base}/${file_to_generate}
        )
      endforeach()
    endif()
  endif()

  set(has_config_info)
  has_configuration_summary(${modulename} has_config_info)
  if(has_config_info)
    set(config_entries_source
        "${CMAKE_CURRENT_BINARY_DIR}/src/config_entries.cpp"
    )
  endif()

  # generate configuration header for this module
  set(config_header
      "${CMAKE_CURRENT_BINARY_DIR}/include/hpx/${modulename}/config/defines.hpp"
  )
  write_config_defines_file(
    NAMESPACE ${modulename_upper} FILENAME ${config_header}
  )
  set(generated_headers ${generated_headers} ${config_header})

  if(${modulename}_CONFIG_FILES)
    # Version file
    set(global_config_file
        ${CMAKE_CURRENT_BINARY_DIR}/include/hpx/config/version.hpp
    )
    hpx_configure_if_changed(
      INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/config_version.hpp.in"
      OUTPUT "${global_config_file}"
      CONFIGURE_ARGS @ONLY
    )
    set(generated_headers ${generated_headers} ${global_config_file})

    # Global config defines file (different from the one for each module)
    set(global_config_file
        ${CMAKE_CURRENT_BINARY_DIR}/include/hpx/config/defines.hpp
    )
    write_config_defines_file(
      TEMPLATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/config_defines.hpp.in"
      NAMESPACE default
      FILENAME "${global_config_file}"
    )
    set(generated_headers ${generated_headers} ${global_config_file})

    # Cacheline size definition
    set(cache_line_size_file
        ${CMAKE_CURRENT_BINARY_DIR}/include/hpx/config/cache_line_size.hpp
    )
    hpx_configure_if_changed(
      INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/templates/cache_line_size.hpp.in"
      OUTPUT "${cache_line_size_file}"
      CONFIGURE_ARGS @ONLY
    )
    set(generated_headers ${generated_headers} ${cache_line_size_file})

    # Generate empty placeholder files for hpx/config/std_headers.hpp and
    # hpx/config/modules_enabled.hpp. Those will be overwritten in
    # libs/CMakeLists.txt with some real content.
    if(HPX_WITH_CXX_MODULES)
      set(std_header_file
          "${CMAKE_CURRENT_BINARY_DIR}/include/hpx/config/std_headers.hpp"
      )
      if(NOT EXISTS "${std_header_file}")
        file(WRITE ${std_header_file} "")
      endif()
      set(generated_headers ${generated_headers} ${std_header_file})
    endif()

    set(modules_enabled_file
        "${CMAKE_CURRENT_BINARY_DIR}/include/hpx/config/modules_enabled.hpp"
    )
    if(NOT EXISTS "${modules_enabled_file}")
      file(WRITE ${modules_enabled_file} "")
    endif()
    set(generated_headers ${generated_headers} ${modules_enabled_file})
  endif()

  # collect zombie generated headers
  file(GLOB_RECURSE zombie_generated_headers
       ${CMAKE_CURRENT_BINARY_DIR}/include/*.hpp
       ${CMAKE_CURRENT_BINARY_DIR}/include_compatibility/*.hpp
  )
  list(
    REMOVE_ITEM
    zombie_generated_headers
    ${generated_headers}
    ${compat_headers}
    ${CMAKE_CURRENT_BINARY_DIR}/include/hpx/config/modules_enabled.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/hpx/config/config_strings.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/include/hpx/parcelset/static_parcelports.hpp
  )
  foreach(zombie_header IN LISTS zombie_generated_headers)
    hpx_warn("Removing zombie generated header: ${zombie_header}")
    file(REMOVE ${zombie_header})
  endforeach()

  # list all specified headers
  foreach(header_file ${headers})
    hpx_debug(${header_file})
  endforeach(header_file)

  # NOTE: We optionally keep the modules as static libraries as otherwise cmake
  # may run out of memory.
  if(HPX_WITH_MODULES_AS_STATIC_LIBRARIES)
    set(module_library_type STATIC)
  else()
    set(module_library_type OBJECT)
  endif()

  set(source_group_root ${SOURCE_ROOT})
  if(NOT sources AND NOT config_entries_source)
    set(source_group_root "${HPX_SOURCE_DIR}/libs/src/")
    set(sources "${source_group_root}/empty.cpp")
  endif()

  # create library modules
  add_library(
    hpx_${modulename}
    ${module_library_type}
    ${sources}
    ${config_entries_source}
    ${${modulename}_OBJECTS}
    ${headers}
    ${generated_headers}
    ${compat_headers}
  )

  if(HPX_WITH_CHECK_MODULE_DEPENDENCIES)
    # verify that all dependencies are from the same module category
    foreach(dep ${${modulename}_MODULE_DEPENDENCIES})
      # consider only module dependencies, not other targets
      string(FIND ${dep} "hpx_" find_index)
      if(${find_index} EQUAL 0)
        string(SUBSTRING ${dep} 4 -1 dep) # cut off leading "hpx_"
        list(FIND _hpx_${libname}_modules ${dep} dep_index)
        if(${dep_index} EQUAL -1)
          hpx_error(
            "The module hpx_${dep} should not be be listed in MODULE_DEPENDENCIES "
            "for '${libname}' module hpx_${modulename}"
          )
        endif()
      endif()
    endforeach()
  endif()

  target_link_libraries(
    hpx_${modulename} PUBLIC ${${modulename}_MODULE_DEPENDENCIES}
  )
  target_link_libraries(hpx_${modulename} PUBLIC ${${modulename}_DEPENDENCIES})
  target_link_libraries(
    hpx_${modulename} PRIVATE ${${modulename}_PRIVATE_DEPENDENCIES}
  )
  target_include_directories(
    hpx_${modulename} PUBLIC ${${modulename}_INCLUDE_DIRECTORIES}
  )
  target_include_directories(
    hpx_${modulename}
    PUBLIC $<BUILD_INTERFACE:${HEADER_ROOT}>
           $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
           $<INSTALL_INTERFACE:include>
  )

  target_link_libraries(
    hpx_${modulename}
    PUBLIC hpx_public_flags
    PRIVATE hpx_private_flags
    PUBLIC hpx_base_libraries
  )

  if(HPX_WITH_PRECOMPILED_HEADERS)
    target_precompile_headers(
      hpx_${modulename} REUSE_FROM hpx_precompiled_headers
    )
  endif()

  # All core modules depend on the config registry. We exclude the base modules
  # (config, preprocessor, config_registry) to avoid circular dependencies. The
  # registry is the sink for module-specific configurations; fundamental modules
  # must not depend on it to ensure they remain at the bottom of the dependency
  # graph.
  if("${libname}" STREQUAL "core"
     AND NOT
         ("${modulename}" STREQUAL "config_registry"
          OR "${modulename}" STREQUAL "config"
          OR "${modulename}" STREQUAL "preprocessor")
  )
    target_link_libraries(hpx_${modulename} PUBLIC hpx_config_registry)
  endif()

  if(HPX_WITH_MODULE_COMPATIBILITY_HEADERS AND ${modulename}_COMPAT_HEADERS)
    target_include_directories(
      hpx_${modulename} PUBLIC $<BUILD_INTERFACE:${COMPAT_HEADER_ROOT}>
    )
  endif()

  target_compile_definitions(
    hpx_${modulename} PRIVATE HPX_${libname_upper}_EXPORTS
  )

  if("${libname}" STREQUAL "full")
    # This is a temporary solution until all of HPX has been modularized as it
    # enables using header files from HPX for compiling this module.
    target_include_directories(hpx_${modulename} PRIVATE ${HPX_SOURCE_DIR})

    # We build the full library module using the C++ module definitions for
    # core.
    if(HPX_WITH_CXX_MODULES)
      hpx_configure_module_consumer(hpx_${modulename} hpx_core_module_if)
    endif()
  endif()

  add_hpx_source_group(
    NAME hpx_${modulename}
    ROOT ${HEADER_ROOT}/hpx
    CLASS "Header Files"
    TARGETS ${headers}
  )
  add_hpx_source_group(
    NAME hpx_${modulename}
    ROOT ${source_group_root}
    CLASS "Source Files"
    TARGETS ${sources}
  )
  if(HPX_WITH_MODULE_COMPATIBILITY_HEADERS AND ${modulename}_COMPAT_HEADERS)
    add_hpx_source_group(
      NAME hpx_${modulename}
      ROOT ${COMPAT_HEADER_ROOT}/hpx
      CLASS "Generated Files"
      TARGETS ${compat_headers}
    )
  endif()

  if(${modulename}_GLOBAL_HEADER_GEN OR ${modulename}_CONFIG_FILES)
    add_hpx_source_group(
      NAME hpx_${modulename}
      ROOT ${CMAKE_CURRENT_BINARY_DIR}/include/hpx
      CLASS "Generated Files"
      TARGETS ${generated_headers}
    )
  endif()
  add_hpx_source_group(
    NAME hpx_${modulename}
    ROOT ${CMAKE_CURRENT_BINARY_DIR}/include/hpx
    CLASS "Generated Files"
    TARGETS ${config_header}
  )

  set_target_properties(
    hpx_${modulename} PROPERTIES FOLDER "Core/Modules/${libname_cap}"
                                 POSITION_INDEPENDENT_CODE ON
  )

  if(HPX_WITH_UNITY_BUILD)
    set_target_properties(hpx_${modulename} PROPERTIES UNITY_BUILD ON)
  endif()

  if(MSVC)
    set_target_properties(
      hpx_${modulename}
      PROPERTIES COMPILE_PDB_NAME_DEBUG hpx_${modulename}d
                 COMPILE_PDB_NAME_RELWITHDEBINFO hpx_${modulename}
                 COMPILE_PDB_OUTPUT_DIRECTORY_DEBUG
                 ${CMAKE_CURRENT_BINARY_DIR}/Debug
                 COMPILE_PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO
                 ${CMAKE_CURRENT_BINARY_DIR}/RelWithDebInfo
    )
  endif()

  # cmake-format: off
  install(
    TARGETS hpx_${modulename}
    EXPORT HPXInternalTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${modulename}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT ${modulename}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT ${modulename}
    ${header_fileset_installation}
  )
  # cmake-format: on

  hpx_export_internal_targets(hpx_${modulename})

  # Install the headers from the source
  install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT ${modulename}
    PATTERN "*.hpp.in" EXCLUDE
  )

  # Install the compatibility headers from the source
  if(HPX_WITH_MODULE_COMPATIBILITY_HEADERS AND ${modulename}_COMPAT_HEADERS)
    install(
      DIRECTORY ${COMPAT_HEADER_ROOT}/hpx
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
      COMPONENT ${modulename}
    )
  endif()

  # Installing the generated header files from the build dir
  install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/hpx
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT ${modulename}
  )

  # install PDB if needed
  if(MSVC)
    foreach(cfg DEBUG;RELWITHDEBINFO)
      get_target_property(_pdb_file hpx_${modulename} COMPILE_PDB_NAME_${cfg})
      get_target_property(
        _pdb_dir hpx_${modulename} COMPILE_PDB_OUTPUT_DIRECTORY_${cfg}
      )
      install(
        FILES ${_pdb_dir}/${_pdb_file}.pdb
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        CONFIGURATIONS ${cfg}
        COMPONENT ${modulename}
        OPTIONAL
      )
    endforeach()
  endif()

  # install cmake files specific to this module
  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
    COMPONENT ${modulename}
    OPTIONAL
  )

  # Link modules to their higher-level libraries
  if(HPX_WITH_MODULES_AS_STATIC_LIBRARIES)
    if(UNIX OR MINGW)
      if(APPLE)
        set(_module_target "-Wl,-all_load" "hpx_${modulename}")
      else()
        # not apple, regular linux
        set(_module_target "-Wl,--whole-archive" "hpx_${modulename}"
                           "-Wl,--no-whole-archive"
        )
      endif()
    elseif(MSVC)
      set(_module_target hpx_${modulename})
      target_link_libraries(
        hpx_${libname}
        PRIVATE -WHOLEARCHIVE:$<TARGET_FILE:$<TARGET_NAME:hpx_${modulename}>>
      )
    endif()
    target_link_libraries(hpx_${libname} PRIVATE ${_module_target})

    get_target_property(
      _module_interface_include_directories hpx_${modulename}
      INTERFACE_INCLUDE_DIRECTORIES
    )
    target_include_directories(
      hpx_${libname} INTERFACE ${_module_interface_include_directories}
    )
  else()
    target_link_libraries(hpx_${libname} PUBLIC hpx_${modulename})
    target_link_libraries(hpx_${libname} PRIVATE ${${modulename}_OBJECTS})
  endif()

  set(_hpx_prev_scan "${CMAKE_CXX_SCAN_FOR_MODULES}")
  if(HPX_WITH_CXX_MODULES)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
  endif()

  foreach(dir ${${modulename}_CMAKE_SUBDIRS})
    add_subdirectory(${dir})
  endforeach(dir)

  # Restore previous default so sibling modules are unaffected.
  set(CMAKE_CXX_SCAN_FOR_MODULES "${_hpx_prev_scan}")

  create_configuration_summary(
    "    Module configuration (${modulename}):" "${modulename}"
  )

endfunction(add_hpx_module)
