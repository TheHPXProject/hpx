# Copyright (c)      2020 Mikael Simberg
# Copyright (c) 2007-2026 Hartmut Kaiser
# Copyright (c) 2011-2014 Thomas Heller
# Copyright (c)      2024 Isidoros Tsaousis-Seiras
# Copyright (c) 2007-2008 Chirag Dekate
# Copyright (c)      2011 Bryce Lelbach
# Copyright (c)      2011 Vinay C Amatya
# Copyright (c)      2013 Jeroen Habraken
# Copyright (c) 2014-2016 Andreas Schaefer
# Copyright (c) 2017      Abhimanyu Rawat
# Copyright (c) 2017      Google
# Copyright (c) 2017      Taeguk Kwon
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

# Overrides must go before the project() statement, otherwise they are ignored.

# ##############################################################################
# C++ overrides
# ##############################################################################
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HPX_CXXOverrides.cmake"
)

# ##############################################################################
# Build type (needs to be handled before project command below)
# ##############################################################################
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE
        STRING
        "Configuration type (one of Debug, RelWithDebInfo, Release, MinSizeRel)"
        FORCE
  )
endif()

# ##############################################################################
# project metadata
# ##############################################################################
project(HPX CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# ##############################################################################
# Special handling for building tests/examples against a previously installed
# version of HPX
# ##############################################################################
if(HPX_WITH_INSTALLED_VERSION)
  include(cmake/installed_hpx.cmake)
  return()
endif()

# ##############################################################################
string(TIMESTAMP HPX_COPYRIGHT_YEAR "%Y")

# ##############################################################################
# Build all of HPX
# ##############################################################################
set(HPX_VERSION_MAJOR 2)
set(HPX_VERSION_MINOR 0)
set(HPX_VERSION_SUBMINOR 0)
set(HPX_VERSION_DATE 20250630)
set(HPX_VERSION_TAG "")

set(HPX_VERSION
    "${HPX_VERSION_MAJOR}.${HPX_VERSION_MINOR}.${HPX_VERSION_SUBMINOR}"
)
set(HPX_VERSION_FULL
    "${HPX_VERSION_MAJOR}.${HPX_VERSION_MINOR}.${HPX_VERSION_SUBMINOR}${HPX_VERSION_TAG}"
)

set(HPX_LIBRARY_VERSION "${HPX_VERSION}")
set(HPX_SOVERSION ${HPX_VERSION_MAJOR})
set(HPX_PACKAGE_NAME HPX)
# To keep track of the hpx_root when other subprojects are declared
set(HPX_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
set(HPX_BINARY_DIR "${PROJECT_BINARY_DIR}")

# ##############################################################################
# CMake configuration
# ##############################################################################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

include(GNUInstallDirs)
include(HPX_Utils)

# explicitly set certain policies
cmake_policy(VERSION 3.18)
hpx_set_cmake_policy(CMP0042 NEW)
hpx_set_cmake_policy(CMP0060 NEW)
hpx_set_cmake_policy(CMP0074 NEW)
hpx_set_cmake_policy(CMP0167 OLD)
hpx_set_cmake_policy(CMP0144 NEW)

# We save the passed compiler flag to a special variable. This is needed for our
# build system unit tests. Some flags might influence the created symbols
# (_GLIBCXX_DEBUG, I look at you)
set(CMAKE_CXX_FLAGS_SAFE ${CMAKE_CXX_FLAGS})
include(HPX_CheckCXXStandard)

include(CheckCXXCompilerFlag)
include(CMakeDependentOption)

# include additional macro definitions
include(HPX_AddConfigTest)
include(HPX_AddDefinitions)

hpx_force_out_of_tree_build(
  "This project requires an out-of-source-tree build. See README.rst. Clean your CMake cache and CMakeFiles if this message persists."
)

if(NOT HPX_CMAKE_LOGLEVEL)
  set(HPX_CMAKE_LOGLEVEL "WARN")
endif()

# print initial diagnostics
hpx_info("CMake version: ${CMAKE_VERSION}, generator: ${CMAKE_GENERATOR}")
hpx_info("HPX version: ${HPX_VERSION}")

# ##############################################################################
# reset cached variables that need to be re-filled
unset(HPX_COMPONENTS CACHE)
unset(HPX_EXPORT_TARGETS CACHE)
unset(HPX_EXPORT_INTERNAL_TARGETS CACHE)
unset(HPX_ENABLED_MODULES CACHE)
unset(HPX_CORE_ENABLED_MODULES CACHE)
unset(HPX_FULL_ENABLED_MODULES CACHE)
unset(HPX_STATIC_PARCELPORT_PLUGINS CACHE)

# ##############################################################################
# Set up dummy compiler flags targets
# ##############################################################################
include(HPX_CompilerFlagsTargets)

# ##############################################################################
# Setup platform for which HPX should be compiled for.
#
include(HPX_SetPlatform)
if("${HPX_PLATFORM_UC}" STREQUAL "ANDROID")
  unset(HPX_LIBRARY_VERSION)
  unset(HPX_SOVERSION)
endif()

if(MSVC)
  hpx_option(
    HPX_WITH_VS_STARTUP_PROJECT STRING
    "Define the startup project for the HPX solution (default: ALL_BUILD)."
    "ALL_BUILD" ADVANCED
  )
  if(HPX_WITH_VS_STARTUP_PROJECT)
    set(VS_STARTUP_PROJECT ${HPX_WITH_VS_STARTUP_PROJECT})
  endif()
endif()

# ##############################################################################
# Disable check for compiler compatibility by default
hpx_option(
  HPX_WITH_IGNORE_COMPILER_COMPATIBILITY BOOL
  "Ignore compiler incompatibility in dependent projects (default: ON)." ON
  ADVANCED
)

# ##############################################################################
# Set our build options cache variables which are customizable by users
#

hpx_option(
  HPX_WITH_DEPRECATION_WARNINGS BOOL
  "Enable warnings for deprecated facilities (default: ON)." ON ADVANCED
)

if(HPX_WITH_DEPRECATION_WARNINGS)
  # enable deprecation warnings globally
  hpx_add_config_cond_define(HPX_HAVE_DEPRECATION_WARNINGS 1)
endif()

# Generic build options
set(DEFAULT_MALLOC "system")
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  set(DEFAULT_MALLOC "tcmalloc")
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  set(HPX_WITH_STACKOVERFLOW_DETECTION_DEFAULT OFF)
  string(TOUPPER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_UC)
  if("${CMAKE_BUILD_TYPE_UC}" STREQUAL "DEBUG")
    set(HPX_WITH_STACKOVERFLOW_DETECTION_DEFAULT ON)
  endif()
  hpx_option(
    HPX_WITH_STACKOVERFLOW_DETECTION
    BOOL
    "Enable stackoverflow detection for HPX threads/coroutines (default: OFF, debug: ON)."
    ${HPX_WITH_STACKOVERFLOW_DETECTION_DEFAULT}
    ADVANCED
  )
  if(HPX_WITH_STACKOVERFLOW_DETECTION)
    hpx_add_config_define(HPX_HAVE_STACKOVERFLOW_DETECTION)
  endif()
endif()

hpx_option(
  HPX_WITH_MALLOC
  STRING
  "Define which allocator should be linked in. Options are: system, tcmalloc, jemalloc, mimalloc, tbbmalloc, and custom (default is: tcmalloc)"
  ${DEFAULT_MALLOC}
  STRINGS "system;tcmalloc;jemalloc;mimalloc;tbbmalloc;custom"
)

# On some systems jemalloc requires an explicit prefix for the API functions
# (i.e. 'malloc' is called 'je_malloc', etc.)
if(${HPX_WITH_MALLOC} STREQUAL "jemalloc")
  if(MSVC)
    set(HPX_WITH_JEMALLOC_PREFIX_DEFAULT "je_")
  else()
    set(HPX_WITH_JEMALLOC_PREFIX_DEFAULT "<none>")
  endif()
  hpx_option(
    HPX_WITH_JEMALLOC_PREFIX STRING
    "Optional naming prefix for jemalloc API functions"
    ${HPX_WITH_JEMALLOC_PREFIX_DEFAULT} ADVANCED
  )
endif()

# Logging configuration
hpx_option(
  HPX_WITH_LOGGING BOOL "Build HPX with logging enabled (default: ON)." ON
  ADVANCED
)
if(HPX_WITH_LOGGING)
  hpx_add_config_define(HPX_HAVE_LOGGING)
endif()

hpx_option(
  HPX_WITH_FAULT_TOLERANCE
  BOOL
  "Build HPX to tolerate failures of nodes, i.e. ignore errors in active communication channels (default: OFF)"
  OFF
  ADVANCED
)
if(HPX_WITH_FAULT_TOLERANCE)
  hpx_add_config_define(HPX_HAVE_FAULT_TOLERANCE)
endif()

# Compiler related build options
hpx_option(
  HPX_WITH_GCC_VERSION_CHECK BOOL
  "Don't ignore version reported by gcc (default: ON)" ON ADVANCED
)

hpx_option(
  HPX_WITH_COMPILER_WARNINGS BOOL "Enable compiler warnings (default: ON)" ON
  ADVANCED
)

hpx_option(
  HPX_WITH_COMPILER_WARNINGS_AS_ERRORS BOOL
  "Turn compiler warnings into errors (default: OFF)" OFF ADVANCED
)

hpx_option(
  HPX_WITH_EXECUTABLE_PREFIX STRING
  "Executable prefix (default none), 'hpx_' useful for system install." ""
  CATEGORY "Build Targets"
)

hpx_option(
  HPX_WITH_DOCUMENTATION BOOL "Build the HPX documentation (default OFF)." OFF
  CATEGORY "Build Targets"
)

if(HPX_WITH_DOCUMENTATION)
  set(valid_output_formats html singlehtml latexpdf man)
  hpx_option(
    HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS
    STRING
    "List of documentation output formats to generate. Valid options are ${valid_output_formats}. Multiple values can be separated with semicolons. (default html)."
    "html"
    CATEGORY "Build Targets"
  )

  foreach(output_format ${HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS})
    if(NOT ${output_format} IN_LIST valid_output_formats)
      hpx_error(
        "${output_format} is not a valid value for HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS. Valid output format are: ${valid_output_formats}."
      )
    endif()
  endforeach()
endif()

if(WIN32)
  set(HPX_WITH_PSEUDO_DEPENDENCIES
      OFF
      CACHE INTERNAL "" FORCE
  )
else()
  set(HPX_WITH_PSEUDO_DEPENDENCIES
      ON
      CACHE INTERNAL "" FORCE
  )
endif()

hpx_option(
  HPX_WITH_UNITY_BUILD BOOL
  "Enable unity build for certain build targets (default OFF)" OFF ADVANCED
)
if(HPX_WITH_UNITY_BUILD)
  set(HPX_WITH_UNITY_BUILD_OPTION UNITY_BUILD)
endif()

# ##############################################################################
# C++20 modules configuration
# ##############################################################################

hpx_option(
  HPX_WITH_CXX_MODULES BOOL "Enable exposing C++20 modules (default: OFF)." OFF
  ADVANCED
)
if(HPX_WITH_CXX_MODULES)
  include(HPX_CXXModules)
  hpx_check_cxx_modules_support()

  hpx_info(
    "HPX will build and install C++20 module interface units for downstream users."
  )
  hpx_add_config_define(HPX_HAVE_CXX_MODULES)

  set(HPX_WITH_CXX_MODULES_OPTION SCAN_FOR_MODULES ON)
else()
  hpx_info("HPX will be built and used with traditional headers only.")
endif()

hpx_option(
  HPX_WITH_PRECOMPILED_HEADERS
  BOOL
  "Enable precompiled headers for certain build targets (experimental) (default OFF)"
  OFF
  ADVANCED
)
if(HPX_WITH_PRECOMPILED_HEADERS)
  set(HPX_WITH_PRECOMPILED_HEADERS_INTERNAL ON)
  # Only create the targets here. They will be set up later once all modules are
  # known.
  add_library(hpx_precompiled_headers OBJECT libs/src/dummy.cpp)
  add_executable(hpx_exe_precompiled_headers libs/src/dummy.cpp)

  set_target_properties(hpx_precompiled_headers PROPERTIES FOLDER "Core")
  set_target_properties(hpx_exe_precompiled_headers PROPERTIES FOLDER "Core")
endif()

# ##############################################################################
# Dynamic hpx_main
# ##############################################################################

set(HPX_WITH_DYNAMIC_HPX_MAIN_DEFAULT OFF)
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" OR APPLE)
  set(HPX_WITH_DYNAMIC_HPX_MAIN_DEFAULT ON)
endif()

hpx_option(
  HPX_WITH_DYNAMIC_HPX_MAIN
  BOOL
  "Enable dynamic overload of system ``main()`` (Linux and Apple only, default: ON)"
  ${HPX_WITH_DYNAMIC_HPX_MAIN_DEFAULT}
  ADVANCED
)
if(HPX_WITH_DYNAMIC_HPX_MAIN)
  if(NOT HPX_WITH_DYNAMIC_HPX_MAIN_DEFAULT)
    hpx_error(
      "HPX_WITH_DYNAMIC_HPX_MAIN was set to ON, but the option is only available on Linux and Apple (this is \"${CMAKE_SYSTEM_NAME}\")."
    )
  endif()
  hpx_add_config_define(HPX_HAVE_DYNAMIC_HPX_MAIN)
endif()

# ##############################################################################
# Some platforms do not support dynamic linking. Enable this to link all
# libraries statically. This also changes some of the internals of HPX related
# to how components are loaded.
# ##############################################################################
hpx_option(
  HPX_WITH_STATIC_LINKING BOOL
  "Compile HPX statically linked libraries (Default: OFF)" OFF ADVANCED
)
if(HPX_WITH_STATIC_LINKING)
  hpx_add_config_define(HPX_HAVE_STATIC_LINKING)
  set(hpx_library_link_mode STATIC)
  set(BUILD_SHARED_LIBS OFF)
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

  if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    hpx_option(
      HPX_WITH_STATIC_EXE_LINKING BOOL
      "Compile HPX statically linked executables (Default: OFF)" OFF ADVANCED
    )

    if(HPX_WITH_STATIC_EXE_LINKING)
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
  endif()
else()
  set(hpx_library_link_mode SHARED)
endif()

# ##############################################################################
# The cmake Ninja generator runs out of memory if the modules are being built as
# OBJECT libraries. In this case the fallback is to build whole-archive STATIC
# libraries.
set(HPX_WITH_MODULES_AS_STATIC_LIBRARIES_DEFAULT OFF)
if("${CMAKE_GENERATOR}" STREQUAL "Ninja")
  set(HPX_WITH_MODULES_AS_STATIC_LIBRARIES_DEFAULT ON)
endif()

hpx_option(
  HPX_WITH_MODULES_AS_STATIC_LIBRARIES
  BOOL
  "Compile HPX modules as STATIC (whole-archive) libraries instead of OBJECT\
  libraries (Default: ${HPX_WITH_MODULES_AS_STATIC_LIBRARIES_DEFAULT})"
  ${HPX_WITH_MODULES_AS_STATIC_LIBRARIES_DEFAULT}
  ADVANCED
)

if(HPX_WITH_MODULES_AS_STATIC_LIBRARIES)
  hpx_info("Building modules as STATIC (whole-archive) libraries")
else()
  hpx_info("Building modules as OBJECT libraries")
endif()

# ##############################################################################
hpx_option(
  HPX_WITH_EXAMPLES BOOL "Build the HPX examples (default ON)" ON
  CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_TESTS BOOL "Build the HPX tests (default ON)" ON
  CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_TESTS_BENCHMARKS BOOL "Build HPX benchmark tests (default: ON)" ON
  ADVANCED CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_TESTS_REGRESSIONS BOOL "Build HPX regression tests (default: ON)" ON
  ADVANCED CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_TESTS_UNIT BOOL "Build HPX unit tests (default: ON)" ON ADVANCED
  CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_TESTS_HEADERS BOOL "Build HPX header tests (default: OFF)" OFF
  ADVANCED CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_TESTS_EXTERNAL_BUILD BOOL
  "Build external cmake build tests (default: ON)" ON ADVANCED
  CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_TESTS_EXAMPLES BOOL "Add HPX examples as tests (default: ON)" ON
  ADVANCED CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_TOOLS BOOL "Build HPX tools (default: OFF)" OFF ADVANCED
  CATEGORY "Build Targets"
)

hpx_option(
  HPX_WITH_COMPILE_ONLY_TESTS BOOL
  "Create build system support for compile time only HPX tests (default ON)" ON
  CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_FAIL_COMPILE_TESTS BOOL
  "Create build system support for fail compile HPX tests (default ON)" ON
  CATEGORY "Build Targets"
)
hpx_option(
  HPX_WITH_PARALLEL_LINK_JOBS
  STRING
  "Number of Parallel link jobs while building hpx (only for Ninja as generator) (default 2)"
  "2"
  CATEGORY "Build Targets"
)

if(CMAKE_GENERATOR MATCHES "Ninja")
  if(NOT HPX_WITH_PARALLEL_LINK_JOBS)
    set(HPX_WITH_PARALLEL_LINK_JOBS "2")
  endif()
  if(HPX_WITH_PARALLEL_LINK_JOBS)
    set_property(
      GLOBAL APPEND PROPERTY JOB_POOLS
                             link_job_pool=${HPX_WITH_PARALLEL_LINK_JOBS}
    )
    set(CMAKE_JOB_POOL_LINK link_job_pool)
  endif()
elseif(HPX_WITH_PARALLEL_LINK_JOBS)
  hpx_warn("Job pooling is only available with Ninja generators.")
endif()

# disable all tests if HPX_WITH_TESTS=OFF
if(NOT HPX_WITH_TESTS)
  hpx_set_option(
    HPX_WITH_TESTS_BENCHMARKS
    VALUE OFF
    FORCE
  )
  hpx_set_option(
    HPX_WITH_TESTS_REGRESSIONS
    VALUE OFF
    FORCE
  )
  hpx_set_option(
    HPX_WITH_TESTS_UNIT
    VALUE OFF
    FORCE
  )
  hpx_set_option(
    HPX_WITH_TESTS_HEADERS
    VALUE OFF
    FORCE
  )
  hpx_set_option(
    HPX_WITH_TESTS_EXTERNAL_BUILD
    VALUE OFF
    FORCE
  )
  hpx_set_option(
    HPX_WITH_TESTS_EXAMPLES
    VALUE OFF
    FORCE
  )
endif()

hpx_option(
  HPX_WITH_DISTRIBUTED_RUNTIME
  BOOL
  "Enable the distributed runtime (default: ON). Turning off the distributed runtime completely disallows the creation and use of components and actions. Turning this option off is experimental!"
  ON
  CATEGORY "Build Targets"
  ADVANCED
)

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  hpx_add_config_define(HPX_HAVE_DISTRIBUTED_RUNTIME)
endif()

# Enable IO-counters on linux systems only
set(HPX_WITH_IO_COUNTERS_DEFAULT OFF)
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND HPX_WITH_DISTRIBUTED_RUNTIME)
  set(HPX_WITH_IO_COUNTERS_DEFAULT ON)
endif()

hpx_option(
  HPX_WITH_IO_COUNTERS BOOL
  "Enable IO counters (default: ${HPX_WITH_IO_COUNTERS_DEFAULT})"
  ${HPX_WITH_IO_COUNTERS_DEFAULT} ADVANCED CATEGORY "Build Targets"
)
if(HPX_WITH_IO_COUNTERS AND HPX_WITH_DISTRIBUTED_RUNTIME)
  if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    hpx_error(
      "HPX_WITH_IO_COUNTERS was set to ON, but IO counters are only available on Linux (this is \"${CMAKE_SYSTEM_NAME}\")"
    )
  endif()
  hpx_add_config_define(HPX_HAVE_IO_COUNTERS)
endif()

set(HPX_FULL_RPATH_DEFAULT ON)
if(APPLE OR WIN32)
  set(HPX_FULL_RPATH_DEFAULT OFF)
endif()
hpx_option(
  HPX_WITH_FULL_RPATH
  BOOL
  "Build and link HPX libraries and executables with full RPATHs (default: ${HPX_FULL_RPATH_DEFAULT})"
  ${HPX_FULL_RPATH_DEFAULT}
  ADVANCED
)

# ##############################################################################
# HPX CUDA configuration
# ##############################################################################
set(CUDA_OPTION_STRING "Enable support for CUDA (default: OFF)")
set(HIP_OPTION_STRING "Enable compilation with HIPCC (default: OFF)")
hpx_option(HPX_WITH_CUDA BOOL "${CUDA_OPTION_STRING}" OFF ADVANCED)

# No need for the user to specify the option explicitly
hpx_option(HPX_WITH_HIP BOOL "${HIP_OPTION_STRING}" OFF ADVANCED)
if("${CMAKE_CXX_COMPILER}" MATCHES "hipcc$")
  set(HPX_WITH_HIP
      ON
      CACHE BOOL "${HIP_OPTION_STRING}" FORCE
  )
endif()

if(HPX_WITH_CUDA AND HPX_WITH_HIP)
  hpx_error(
    "HPX_WITH_CUDA=ON and HPX_WITH_HIP=ON. Only one of them can be on at the same time.\
 Note: HPX_WITH_HIP is automatically enabled when compiling with hipcc."
  )
endif()

# HPX THRUST configuration
set(HPX_WITH_THRUST_DEFAULT OFF)
if(HPX_WITH_CUDA)
  set(HPX_WITH_THRUST_DEFAULT ON)
endif()

hpx_option(
  HPX_WITH_THRUST
  BOOL
  "Enable support for NVIDIA Thrust integration (default: ON when CUDA is enabled, OFF otherwise)"
  ${HPX_WITH_THRUST_DEFAULT}
  ADVANCED
  CATEGORY "Build Targets"
)

# ## HPX STDEXEC configuration ##

hpx_option(
  HPX_WITH_FETCH_STDEXEC BOOL "Use FetchContent to fetch STDEXEC.(default: ON)"
  ON
  CATEGORY "Executor"
  ADVANCED
)

hpx_option(
  HPX_WITH_STDEXEC_TAG STRING "STDEXEC repository tag or branch" "main"
  CATEGORY "Executor"
)

# ##############################################################################
# HPX SYCL configuration
# ##############################################################################
set(SYCL_OPTION_STRING "Enable support for Sycl (default: OFF)")
hpx_option(HPX_WITH_SYCL BOOL "${SYCL_OPTION_STRING}" OFF ADVANCED)
set(SYCL_OPTION_STRING
    "Sycl compile flags for selecting specific targets (default: empty)"
)
hpx_option(HPX_WITH_SYCL_FLAGS STRING "${SYCL_OPTION_STRING}" "" ADVANCED)
set(HIPSYCL_OPTION_STRING "Use hipsycl cmake integration (default: OFF)")
hpx_option(HPX_WITH_HIPSYCL BOOL "${HIPSYCL_OPTION_STRING}" OFF ADVANCED)

# ##############################################################################
# pkgconfig file generation
# ##############################################################################
set(HPX_WITH_PKGCONFIG_DEFAULT ON)
if(APPLE
   OR MSVC
   OR HPX_WITH_CUDA
   OR HPX_WITH_HIP
   OR HPX_WITH_PARCELPORT_LCI
   OR HPX_WITH_PARCELPORT_LCW
)
  set(HPX_WITH_PKGCONFIG_DEFAULT OFF)
endif()

hpx_option(
  HPX_WITH_PKGCONFIG
  BOOL
  "Enable generation of pkgconfig files (default: ON on Linux without CUDA/HIP, otherwise OFF)"
  ${HPX_WITH_PKGCONFIG_DEFAULT}
  ADVANCED
)

# ##############################################################################
# Threadlevel Nice option
# ##############################################################################
hpx_option(
  HPX_WITH_NICE_THREADLEVEL
  BOOL
  "Set HPX worker threads to have high NICE level (may impact performance) (default: OFF)"
  OFF
  ADVANCED
)
if(HPX_WITH_NICE_THREADLEVEL)
  hpx_info("Nice threadlevel is enabled.")
  hpx_add_config_define(HPX_HAVE_NICE_THREADLEVEL)
else()
  hpx_info("Nice threadlevel is disabled.")
endif()

# ##############################################################################
# Utility configuration
# ##############################################################################
set(HPX_HIDDEN_VISIBILITY_DEFAULT ON)
if(CMAKE_COMPILER_IS_GNUCXX)
  if("${HPX_PLATFORM_UC}" STREQUAL "ANDROID")
    set(HPX_HIDDEN_VISIBILITY_DEFAULT OFF)
  endif()
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(HPX_HIDDEN_VISIBILITY_DEFAULT OFF)
endif()
if(APPLE)
  set(HPX_HIDDEN_VISIBILITY_DEFAULT OFF)
endif()

hpx_option(
  HPX_WITH_HIDDEN_VISIBILITY
  BOOL
  "Use -fvisibility=hidden for builds on platforms which support it (default ${HPX_HIDDEN_VISIBILITY_DEFAULT})"
  ${HPX_HIDDEN_VISIBILITY_DEFAULT}
  ADVANCED
)

hpx_option(
  HPX_WITH_AUTOMATIC_SERIALIZATION_REGISTRATION
  BOOL
  "Use automatic serialization registration for actions and functions. This affects compatibility between HPX applications compiled with different compilers (default ON)"
  ON
  ADVANCED
)
if(HPX_WITH_AUTOMATIC_SERIALIZATION_REGISTRATION)
  hpx_add_config_define(HPX_HAVE_AUTOMATIC_SERIALIZATION_REGISTRATION)
endif()

hpx_option(
  HPX_WITH_ZERO_COPY_SERIALIZATION_THRESHOLD
  STRING
  "The threshold in bytes to when perform zero copy optimizations (default: 8192)"
  "8192"
  ADVANCED
)
hpx_add_config_define(
  HPX_ZERO_COPY_SERIALIZATION_THRESHOLD
  ${HPX_WITH_ZERO_COPY_SERIALIZATION_THRESHOLD}
)

hpx_option(
  HPX_WITH_DISABLED_SIGNAL_EXCEPTION_HANDLERS
  BOOL
  "Disables the mechanism that produces debug output for caught signals and unhandled exceptions (default: OFF)"
  OFF
  ADVANCED
)
if(HPX_WITH_DISABLED_SIGNAL_EXCEPTION_HANDLERS)
  hpx_add_config_define(HPX_HAVE_DISABLED_SIGNAL_EXCEPTION_HANDLERS)
endif()

# Thread Manager related build options

set(HPX_MAX_CPU_COUNT_DEFAULT "")
hpx_option(
  HPX_WITH_MAX_CPU_COUNT
  STRING
  "HPX applications will not use more that this number of OS-Threads (empty string means dynamic) (default: \"${HPX_MAX_CPU_COUNT_DEFAULT}\")"
  "${HPX_MAX_CPU_COUNT_DEFAULT}"
  CATEGORY "Thread Manager"
  ADVANCED
)
if(HPX_WITH_MAX_CPU_COUNT)
  hpx_add_config_define(HPX_HAVE_MAX_CPU_COUNT ${HPX_WITH_MAX_CPU_COUNT})
endif()
if((NOT HPX_WITH_MAX_CPU_COUNT) OR (HPX_WITH_MAX_CPU_COUNT GREATER 64))
  hpx_add_config_define(HPX_HAVE_MORE_THAN_64_THREADS)
endif()

set(HPX_MAX_NUMA_DOMAIN_COUNT_DEFAULT "8")
hpx_option(
  HPX_WITH_MAX_NUMA_DOMAIN_COUNT
  STRING
  "HPX applications will not run on machines with more NUMA domains (default: ${HPX_MAX_NUMA_DOMAIN_COUNT_DEFAULT})"
  ${HPX_MAX_NUMA_DOMAIN_COUNT_DEFAULT}
  CATEGORY "Thread Manager"
  ADVANCED
)
hpx_add_config_define(
  HPX_HAVE_MAX_NUMA_DOMAIN_COUNT ${HPX_WITH_MAX_NUMA_DOMAIN_COUNT}
)

hpx_option(
  HPX_WITH_THREAD_STACK_MMAP BOOL
  "Use mmap for stack allocation on appropriate platforms" ON
  CATEGORY "Thread Manager"
  ADVANCED
)

hpx_option(
  HPX_WITH_THREAD_MANAGER_IDLE_BACKOFF BOOL
  "HPX scheduler threads do exponential backoff on idle queues (default: ON)" ON
  CATEGORY "Thread Manager"
  ADVANCED
)

hpx_option(
  HPX_WITH_STACKTRACES BOOL "Attach backtraces to HPX exceptions (default: ON)"
  ON
  CATEGORY "Thread Manager"
  ADVANCED
)

hpx_option(
  HPX_WITH_THREAD_BACKTRACE_ON_SUSPENSION BOOL
  "Enable thread stack back trace being captured on suspension (default: OFF)"
  OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

# We create a target to contain libraries like rt, dl etc. in order to remove
# global variables
add_library(hpx_base_libraries INTERFACE)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  target_link_libraries(hpx_base_libraries INTERFACE imf svml irng intlc)
endif()

if(HPX_WITH_STACKTRACES OR HPX_WITH_THREAD_BACKTRACE_ON_SUSPENSION)
  hpx_info("Stack traces are enabled.")
  hpx_add_config_define(HPX_HAVE_STACKTRACES)
  if(WIN32)
    target_link_libraries(hpx_base_libraries INTERFACE dbghelp)
  endif()

  hpx_option(
    HPX_WITH_THREAD_BACKTRACE_DEPTH STRING
    "Thread stack back trace depth being captured (default: 20)" "20"
    CATEGORY "Thread Manager"
    ADVANCED
  )
  hpx_add_config_define(
    HPX_HAVE_THREAD_BACKTRACE_DEPTH ${HPX_WITH_THREAD_BACKTRACE_DEPTH}
  )

  if(("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
     AND ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"
          OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  )

    # This option is OFF by default as we have seen random segfaults out in the
    # wild if this is enabled.
    hpx_option(
      HPX_WITH_STACKTRACES_STATIC_SYMBOLS BOOL
      "Thread stack back trace will resolve static symbols (default: OFF)" OFF
      CATEGORY "Thread Manager"
      ADVANCED
    )
    hpx_add_config_define(
      HPX_HAVE_STACKTRACES_STATIC_SYMBOLS
      ${HPX_WITH_STACKTRACES_STATIC_SYMBOLS}
    )

    # Demangling can segfault in certain configurations.
    hpx_option(
      HPX_WITH_STACKTRACES_DEMANGLE_SYMBOLS BOOL
      "Thread stack back trace symbols will be demangled (default: ON)" ON
      CATEGORY "Thread Manager"
      ADVANCED
    )
    if(HPX_WITH_STACKTRACES_DEMANGLE_SYMBOLS)
      hpx_add_config_define(HPX_HAVE_STACKTRACES_DEMANGLE_SYMBOLS)
    endif()
  endif()
endif()

if(HPX_WITH_THREAD_BACKTRACE_ON_SUSPENSION)
  hpx_add_config_define(HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION)

  hpx_option(
    HPX_WITH_THREAD_FULLBACKTRACE_ON_SUSPENSION BOOL
    "Enable thread stack back trace being captured on suspension (default: OFF)"
    OFF
    CATEGORY "Thread Manager"
    ADVANCED
  )
  if(HPX_WITH_THREAD_FULLBACKTRACE_ON_SUSPENSION)
    hpx_add_config_define(HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION)
  endif()
endif()

hpx_option(
  HPX_WITH_THREAD_TARGET_ADDRESS BOOL
  "Enable storing target address in thread for NUMA awareness (default: OFF)"
  OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_THREAD_TARGET_ADDRESS)
  hpx_add_config_define(HPX_HAVE_THREAD_TARGET_ADDRESS)
endif()

hpx_option(
  HPX_WITH_THREAD_QUEUE_WAITTIME BOOL
  "Enable collecting queue wait times for threads (default: OFF)" OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_THREAD_QUEUE_WAITTIME)
  hpx_add_config_define(HPX_HAVE_THREAD_QUEUE_WAITTIME)
endif()

hpx_option(
  HPX_WITH_THREAD_IDLE_RATES
  BOOL
  "Enable measuring the percentage of overhead times spent in the scheduler (default: OFF)"
  OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

hpx_option(
  HPX_WITH_THREAD_CREATION_AND_CLEANUP_RATES BOOL
  "Enable measuring thread creation and cleanup times (default: OFF)" OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_THREAD_IDLE_RATES)
  hpx_add_config_define(HPX_HAVE_THREAD_IDLE_RATES)
  if(HPX_WITH_THREAD_CREATION_AND_CLEANUP_RATES)
    hpx_add_config_define(HPX_HAVE_THREAD_CREATION_AND_CLEANUP_RATES)
  endif()
endif()

hpx_option(
  HPX_WITH_THREAD_CUMULATIVE_COUNTS
  BOOL
  "Enable keeping track of cumulative thread counts in the schedulers (default: ON)"
  ON
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_THREAD_CUMULATIVE_COUNTS)
  hpx_add_config_define(HPX_HAVE_THREAD_CUMULATIVE_COUNTS)
endif()

hpx_option(
  HPX_WITH_THREAD_STEALING_COUNTS
  BOOL
  "Enable keeping track of counts of thread stealing incidents in the schedulers (default: OFF)"
  OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_THREAD_STEALING_COUNTS)
  hpx_add_config_define(HPX_HAVE_THREAD_STEALING_COUNTS)
endif()

hpx_option(
  HPX_WITH_COROUTINE_COUNTERS BOOL
  "Enable keeping track of coroutine creation and rebind counts (default: OFF)"
  OFF
  CATEGORY "Thread Manager"
  ADVANCED
)
if(HPX_WITH_COROUTINE_COUNTERS)
  hpx_add_config_define(HPX_HAVE_COROUTINE_COUNTERS)
endif()

hpx_option(
  HPX_WITH_THREAD_LOCAL_STORAGE BOOL
  "Enable thread local storage for all HPX threads (default: OFF)" OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_THREAD_LOCAL_STORAGE)
  hpx_add_config_define(HPX_HAVE_THREAD_LOCAL_STORAGE)
endif()

hpx_option(
  HPX_WITH_SCHEDULER_LOCAL_STORAGE BOOL
  "Enable scheduler local storage for all HPX schedulers (default: OFF)" OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_SCHEDULER_LOCAL_STORAGE)
  hpx_add_config_define(HPX_HAVE_SCHEDULER_LOCAL_STORAGE)
endif()

set(HPX_WITH_WORK_REQUESTING_SCHEDULERS_DEFAULT ON)
if(APPLE)
  set(HPX_WITH_WORK_REQUESTING_SCHEDULERS_DEFAULT OFF)
endif()

hpx_option(
  HPX_WITH_WORK_REQUESTING_SCHEDULERS
  BOOL
  "Enable work requesting scheduler (default: ${HPX_WITH_WORK_REQUESTING_SCHEDULERS_DEFAULT})"
  ${HPX_WITH_WORK_REQUESTING_SCHEDULERS_DEFAULT}
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_WORK_REQUESTING_SCHEDULERS)
  hpx_add_config_define(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
endif()

hpx_option(
  HPX_WITH_SPINLOCK_POOL_NUM STRING
  "Number of elements a spinlock pool manages (default: 128)" 128
  CATEGORY "Thread Manager"
  ADVANCED
)

hpx_add_config_define(HPX_HAVE_SPINLOCK_POOL_NUM ${HPX_WITH_SPINLOCK_POOL_NUM})

hpx_option(
  HPX_WITH_SPINLOCK_DEADLOCK_DETECTION BOOL
  "Enable spinlock deadlock detection (default: OFF)" OFF
  CATEGORY "Thread Manager"
  ADVANCED
)

if(HPX_WITH_SPINLOCK_DEADLOCK_DETECTION)
  hpx_add_config_define(HPX_HAVE_SPINLOCK_DEADLOCK_DETECTION)
endif()

# Profiling related build options
hpx_option(
  HPX_WITH_APEX BOOL "Enable APEX instrumentation support." OFF
  CATEGORY "Profiling"
)

hpx_option(
  HPX_WITH_FETCH_APEX
  BOOL
  "Use FetchContent to fetch APEX. By default an installed APEX will be used. (default: OFF)"
  OFF
  CATEGORY "Build Targets"
  ADVANCED
)

if(HPX_WITH_APEX)
  hpx_add_config_define(HPX_HAVE_APEX) # tell HPX that we use APEX
  hpx_option(
    HPX_WITH_APEX_REPOSITORY STRING "APEX repository"
    "https://github.com/STEllAR-GROUP/apex.git" CATEGORY "Profiling"
  )
  hpx_option(
    HPX_WITH_APEX_TAG STRING "APEX repository tag or branch" "hpx_v1.11.0"
    CATEGORY "Profiling"
  )
  hpx_option(
    HPX_WITH_APEX_NO_UPDATE BOOL
    "Do not update code from remote APEX repository." OFF CATEGORY "Profiling"
  )
endif()

hpx_option(
  HPX_WITH_FETCH_BOOST
  BOOL
  "Use FetchContent to fetch Boost. By default an installed Boost will be used. (default: OFF)"
  OFF
  CATEGORY "Build Targets"
  ADVANCED
)

# Options for automatically fetching Asio
hpx_option(
  HPX_WITH_FETCH_ASIO
  BOOL
  "Use FetchContent to fetch Asio. By default an installed Asio will be used. (default: OFF)"
  OFF
  CATEGORY "Build Targets"
  ADVANCED
)

set(HPX_WITH_ASIO_TAG_DEFAULT "asio-1-30-2")
if(HPX_WITH_HIP)
  # newer versions of Asio currently fail when compiled with HIPCC
  set(HPX_WITH_ASIO_TAG_DEFAULT "asio-1-16-0")
endif()
hpx_option(
  HPX_WITH_ASIO_TAG STRING "Asio repository tag or branch"
  ${HPX_WITH_ASIO_TAG_DEFAULT}
  CATEGORY "Build Targets"
  ADVANCED
)

# Option for automatically fetching Hwloc
hpx_option(
  HPX_WITH_FETCH_HWLOC
  BOOL
  "Use FetchContent to fetch Hwloc. By default an installed Hwloc will be used. (default: OFF)"
  OFF
  CATEGORY "Build Targets"
  ADVANCED
)

hpx_option(
  HPX_WITH_NANOBENCH
  BOOL
  "Use Nanobench for performance tests. Nanobench will be fetched using FetchContent (default: OFF)"
  OFF
  CATEGORY "Build Targets"
  ADVANCED
)

# cmake-format: off
# LibCDS option
# NOTE: The libcds option is disabled for the 1.5.0 release as it is not ready
# for public consumption yet.
# hpx_option(
#   HPX_WITH_LIBCDS BOOL "Enable LibCDS support (experimental)." OFF
#   CATEGORY "Thread Manager" ADVANCED
# )
# if(HPX_WITH_LIBCDS)
#   hpx_option(
#     HPX_WITH_LIBCDS_GIT_REPOSITORY STRING
#     "Define the LibCDS git repository to use."
#     https://github.com/STEllAR-GROUP/libcds CATEGORY "Thread Manager" ADVANCED
#   )
#   hpx_option(
#     HPX_WITH_LIBCDS_GIT_TAG STRING "Define the LibCDS git tag to use." hpx-1.5
#     CATEGORY "Thread Manager" ADVANCED
#   )
#   include(HPX_SetupLibCDS)
#   if(NOT libcds_POPULATED)
#     hpx_error("HPX_WITH_LIBCDS was set to ON, but HPX failed to fetch LibCDS")
#   endif()
#   hpx_add_config_define(HPX_HAVE_LIBCDS) # tell HPX that we use LibCDS
# endif()
# cmake-format: on

hpx_option(
  HPX_WITH_PAPI BOOL "Enable the PAPI based performance counter." OFF
  CATEGORY "Profiling"
)
if(HPX_WITH_PAPI)
  if(NOT HPX_WITH_DISTRIBUTED_RUNTIME)
    hpx_error(
      "HPX_WITH_PAPI was set to ON, but PAPI cannot currently be used with HPX_WITH_DISTRIBUTED_RUNTIME=OFF"
    )
  endif()
  if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    hpx_error(
      "HPX_WITH_PAPI was set to ON, but PAPI can only be used on Linux (this is ${CMAKE_SYSTEM_NAME})"
    )
  endif()
  hpx_add_config_define(HPX_HAVE_PAPI)
endif()

hpx_option(
  HPX_WITH_ITTNOTIFY BOOL "Enable Amplifier (ITT) instrumentation support." OFF
  CATEGORY "Profiling"
)

hpx_option(
  HPX_LIKWID_WITH_LIKWID BOOL "Enable Likwid instrumentation support." OFF
  CATEGORY "Profiling"
)

hpx_option(
  HPX_TRACY_WITH_TRACY BOOL "Enable Tracy instrumentation support." OFF
  CATEGORY "Profiling"
)

# Experimental settings
hpx_option(
  HPX_WITH_IO_POOL
  BOOL
  "Disable internal IO thread pool, do not change if not absolutely necessary (default: ON)"
  ON
  CATEGORY "Thread Manager"
  ADVANCED
)
if(HPX_WITH_IO_POOL)
  hpx_add_config_define(HPX_HAVE_IO_POOL)
endif()

hpx_option(
  HPX_WITH_TIMER_POOL
  BOOL
  "Disable internal timer thread pool, do not change if not absolutely necessary (default: ON)"
  ON
  CATEGORY "Thread Manager"
  ADVANCED
)
if(HPX_WITH_TIMER_POOL)
  hpx_add_config_define(HPX_HAVE_TIMER_POOL)
endif()

# AGAS related build options
hpx_option(
  HPX_WITH_AGAS_DUMP_REFCNT_ENTRIES BOOL
  "Enable dumps of the AGAS refcnt tables to logs (default: OFF)" OFF
  CATEGORY "AGAS"
  ADVANCED
)
if(HPX_WITH_AGAS_DUMP_REFCNT_ENTRIES)
  hpx_add_config_define(HPX_HAVE_AGAS_DUMP_REFCNT_ENTRIES)
endif()

# Should networking be supported?
hpx_option(
  HPX_WITH_NETWORKING BOOL
  "Enable support for networking and multi-node runs (default: ON)" ON
  CATEGORY "Parcelport"
)

if(HPX_WITH_NETWORKING AND NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  hpx_error("HPX_WITH_NETWORKING=ON requires HPX_WITH_DISTRIBUTED_RUNTIME=ON")
endif()

if(HPX_WITH_APEX AND NOT HPX_WITH_DISTRIBUTED_RUNTIME)
  hpx_error("HPX_WITH_DISTRIBUTED_RUNTIME=OFF requires HPX_WITH_APEX=OFF")
endif()

if(HPX_WITH_NETWORKING)
  hpx_add_config_define(HPX_HAVE_NETWORKING)

  # Parcelport related build options
  set(_parcel_profiling_default OFF)
  if(HPX_WITH_APEX)
    set(_parcel_profiling_default ON)
  endif()

  hpx_option(
    HPX_WITH_PARCEL_PROFILING BOOL "Enable profiling data for parcels"
    ${_parcel_profiling_default}
    CATEGORY "Parcelport"
    ADVANCED
  )

  if(HPX_WITH_PARCEL_PROFILING)
    hpx_add_config_define(HPX_HAVE_PARCEL_PROFILING)
  endif()

  # Parcelport related build options and #define in global defines.hpp
  hpx_option(
    HPX_WITH_PARCELPORT_LIBFABRIC
    BOOL
    "Enable the libfabric based parcelport. This is currently an experimental feature"
    OFF
    CATEGORY "Parcelport"
    ADVANCED
  )
  if(HPX_WITH_PARCELPORT_LIBFABRIC)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_LIBFABRIC)
  endif()

  hpx_option(
    HPX_WITH_PARCELPORT_MPI BOOL "Enable the MPI based parcelport." OFF
    CATEGORY "Parcelport"
  )
  if(HPX_WITH_PARCELPORT_MPI)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_MPI)
  endif()

  # we have to decide here whether we want to have MPI futures
  hpx_option(
    HPX_WITH_ASYNC_MPI
    BOOL
    "Enable support for returning futures from MPI asynchronous calls (default: ON if networking+MPI already enabled)"
    ${HPX_WITH_PARCELPORT_MPI}
    CATEGORY "MPI"
  )

  hpx_option(
    HPX_WITH_PARCELPORT_LCI BOOL "Enable the LCI based parcelport." OFF
    CATEGORY "Parcelport"
  )
  if(HPX_WITH_PARCELPORT_LCI)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_LCI)
  endif()
  # Options for automatically fetching LCI
  hpx_option(
    HPX_WITH_FETCH_LCI
    BOOL
    "Use FetchContent to fetch LCI. By default an installed LCI will be used. (default: OFF)"
    OFF
    CATEGORY "Build Targets"
    ADVANCED
  )
  hpx_option(
    HPX_WITH_LCI_TAG STRING "LCI repository tag or branch"
    "f72956b4609c796b5faac1c24a782dd8ac824ad6" # latest commit as of 2025-12-10
    CATEGORY "Build Targets"
    ADVANCED
  )
  hpx_option(
    HPX_WITH_LCI_BOOTSTRAP_MPI BOOL
    "Configure the autofetched LCI with mpi bootstrap support (default: OFF)"
    OFF
    CATEGORY "Build Targets"
    ADVANCED
  )
  hpx_option(
    HPX_WITH_PARCELPORT_LCI_LOG STRING
    "Enable the LCI-parcelport-specific logger" OFF
    CATEGORY "Parcelport"
    ADVANCED
  )
  if(HPX_WITH_PARCELPORT_LCI_LOG)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_LCI_LOG)
  endif()
  hpx_option(
    HPX_WITH_PARCELPORT_LCI_PCOUNTER STRING
    "Enable the LCI-parcelport-specific performance counter" OFF
    CATEGORY "Parcelport"
    ADVANCED
  )
  if(HPX_WITH_PARCELPORT_LCI_PCOUNTER)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_LCI_PCOUNTER)
  endif()

  hpx_option(
    HPX_WITH_PARCELPORT_LCW BOOL "Enable the LCW based parcelport." OFF
    CATEGORY "Parcelport"
  )
  if(HPX_WITH_PARCELPORT_LCW)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_LCW)
  endif()
  # Options for automatically fetching LCI
  hpx_option(
    HPX_WITH_FETCH_LCW
    BOOL
    "Use FetchContent to fetch LCW. By default an installed LCW will be used. (default: OFF)"
    OFF
    CATEGORY "Build Targets"
    ADVANCED
  )
  hpx_option(
    HPX_WITH_LCW_TAG STRING "LCW repository tag or branch"
    "f26fdd068110ec62a56ecb364254c3ce0b101174" # latest commit as of 2025-12-10
    CATEGORY "Build Targets"
    ADVANCED
  )
  hpx_option(
    HPX_WITH_PARCELPORT_LCW_LOG STRING
    "Enable the LCW-parcelport-specific logger" OFF
    CATEGORY "Parcelport"
    ADVANCED
  )
  if(HPX_WITH_PARCELPORT_LCW_LOG)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_LCW_LOG)
  endif()
  hpx_option(
    HPX_WITH_PARCELPORT_LCW_PCOUNTER STRING
    "Enable the LCW-parcelport-specific performance counter" OFF
    CATEGORY "Parcelport"
    ADVANCED
  )
  if(HPX_WITH_PARCELPORT_LCW_PCOUNTER)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_LCW_PCOUNTER)
  endif()

  hpx_option(
    HPX_WITH_PARCELPORT_GASNET BOOL "Enable the GASNET based parcelport." OFF
    CATEGORY "Parcelport"
  )
  hpx_option(
    HPX_WITH_FETCH_GASNET
    BOOL
    "Use FetchContent to fetch GASNET. By default an installed GASNET will be used. (default: OFF)."
    OFF
    CATEGORY "Build Targets"
    ADVANCED
  )
  if(HPX_WITH_PARCELPORT_GASNET)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_GASNET)
    hpx_option(
      HPX_WITH_PARCELPORT_GASNET_CONDUIT STRING
      "Define which conduit to use for the gasnet parcelport" "udp"
      STRINGS "smp;udp;mpi;ofi;ucx"
    )
  endif()

  hpx_option(
    HPX_WITH_PARCELPORT_TCP BOOL "Enable the TCP based parcelport." ON
    CATEGORY "Parcelport"
  )
  if(HPX_WITH_PARCELPORT_TCP)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_TCP)
  endif()
  hpx_option(
    HPX_WITH_PARCELPORT_COUNTERS BOOL
    "Enable performance counters reporting parcelport statistics." OFF
    CATEGORY "Parcelport"
  )
  if(HPX_WITH_PARCELPORT_COUNTERS)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_COUNTERS)
  endif()
  hpx_option(
    HPX_WITH_PARCELPORT_ACTION_COUNTERS
    BOOL
    "Enable performance counters reporting parcelport statistics on a per-action basis."
    OFF
    CATEGORY "Parcelport"
  )
  if(HPX_WITH_PARCELPORT_ACTION_COUNTERS)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
  endif()
  if(HPX_WITH_NANOBENCH)
    hpx_add_config_define(HPX_HAVE_NANOBENCH)
    include(HPX_SetupNanobench)
  endif()
else(HPX_WITH_NETWORKING)
  # if networking is off,  then allow the option of using our asynchronous MPI
  # features
  hpx_option(
    HPX_WITH_ASYNC_MPI
    BOOL
    "Enable support for returning futures from MPI asynchronous calls (default: ON if networking+MPI already enabled)"
    OFF
    CATEGORY "MPI"
  )
endif(HPX_WITH_NETWORKING)

if((HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_MPI)
   OR HPX_WITH_ASYNC_MPI
   OR ("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
)
  # mpi parcelport settings
  hpx_option(
    HPX_WITH_PARCELPORT_MPI_ENV
    STRING
    "List of environment variables checked to detect MPI (default: MV2_COMM_WORLD_RANK;PMI_RANK;OMPI_COMM_WORLD_SIZE;ALPS_APP_PE;PMIX_RANK;PALS_NODEID)."
    "MV2_COMM_WORLD_RANK;PMI_RANK;OMPI_COMM_WORLD_SIZE;ALPS_APP_PE;PMIX_RANK;PALS_NODEID"
    CATEGORY "Parcelport"
    ADVANCED
  )

  # This list is to detect whether we run inside an mpi environment. If one of
  # those environment variables is set, the MPI parcelport is enabled by
  # default. PMI_RANK: Intel MPI and MVAPICH2 OMPI_COMM_WORLD_SIZE: OpenMPI
  # starting at version 1.3
  if(HPX_WITH_PARCELPORT_MPI_ENV)
    string(REPLACE ";" "," hpx_parcelport_mpi_env_
                   "${HPX_WITH_PARCELPORT_MPI_ENV}"
    )
    hpx_add_config_define(
      HPX_HAVE_PARCELPORT_MPI_ENV "\"${hpx_parcelport_mpi_env_}\""
    )
  endif()

  hpx_option(
    HPX_WITH_PARCELPORT_MPI_MULTITHREADED BOOL
    "Turn on MPI multithreading support (default: ON)." ON
    CATEGORY "Parcelport"
    ADVANCED
  )
  if(HPX_WITH_PARCELPORT_MPI_MULTITHREADED)
    hpx_add_config_define(HPX_HAVE_PARCELPORT_MPI_MULTITHREADED)
  endif()

  if(MSVC)
    # FIXME: add OpenMPI specific flag here for now as the
    # hpx_add_compile_flag() below does not add the extra options to the top
    # level directory
    hpx_add_config_define(OMPI_IMPORTS)
    if(HPX_WITH_CUDA)
      hpx_add_config_cond_define(NOMINMAX)
    endif()
  endif()
endif()

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCI)
  # lci parcelport settings
  hpx_option(
    HPX_WITH_PARCELPORT_LCI_ENV
    STRING
    "List of environment variables checked to detect LCI (default: MV2_COMM_WORLD_RANK;PMI_RANK;OMPI_COMM_WORLD_SIZE;ALPS_APP_PE;PMIX_RANK;PALS_NODEID;LCT_PMI_FILE_NRANKS)."
    "MV2_COMM_WORLD_RANK;PMI_RANK;OMPI_COMM_WORLD_SIZE;ALPS_APP_PE;PMIX_RANK;PALS_NODEID;LCT_PMI_FILE_NRANKS"
    CATEGORY "Parcelport"
    ADVANCED
  )

  # This list is to detect whether we run inside an lci environment. If one of
  # those environment variables is set, the LCI parcelport is enabled by
  # default. PMI_RANK: Intel MPI and MVAPICH2 OMPI_COMM_WORLD_SIZE: OpenMPI
  # starting at version 1.3
  if(HPX_WITH_PARCELPORT_LCI_ENV)
    string(REPLACE ";" "," hpx_parcelport_lci_env_
                   "${HPX_WITH_PARCELPORT_LCI_ENV}"
    )
    hpx_add_config_define(
      HPX_HAVE_PARCELPORT_LCI_ENV "\"${hpx_parcelport_lci_env_}\""
    )
  endif()
endif()

if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCW)
  # lcw parcelport settings
  hpx_option(
    HPX_WITH_PARCELPORT_LCW_ENV
    STRING
    "List of environment variables checked to detect LCW (default: MV2_COMM_WORLD_RANK;PMI_RANK;OMPI_COMM_WORLD_SIZE;ALPS_APP_PE;PMIX_RANK;PALS_NODEID;LCT_PMI_FILE_NRANKS)."
    "MV2_COMM_WORLD_RANK;PMI_RANK;OMPI_COMM_WORLD_SIZE;ALPS_APP_PE;PMIX_RANK;PALS_NODEID;LCT_PMI_FILE_NRANKS"
    CATEGORY "Parcelport"
    ADVANCED
  )

  # This list is to detect whether we run inside an lcw environment. If one of
  # those environment variables is set, the LCW parcelport is enabled by
  # default. PMI_RANK: Intel MPI and MVAPICH2 OMPI_COMM_WORLD_SIZE: OpenMPI
  # starting at version 1.3
  if(HPX_WITH_PARCELPORT_LCW_ENV)
    string(REPLACE ";" "," hpx_parcelport_lcw_env_
                   "${HPX_WITH_PARCELPORT_LCW_ENV}"
    )
    hpx_add_config_define(
      HPX_HAVE_PARCELPORT_LCW_ENV "\"${hpx_parcelport_lcw_env_}\""
    )
  endif()
endif()

# External libraries/frameworks used by sme of the examples and benchmarks
hpx_option(
  HPX_WITH_EXAMPLES_OPENMP BOOL
  "Enable examples requiring OpenMP support (default: OFF)." OFF
  CATEGORY "Build Targets"
  ADVANCED
)
if(HPX_WITH_EXAMPLES_OPENMP)
  find_package(OpenMP REQUIRED)
endif()
hpx_option(
  HPX_WITH_EXAMPLES_TBB BOOL
  "Enable examples requiring TBB support (default: OFF)." OFF
  CATEGORY "Build Targets"
  ADVANCED
)
if(HPX_WITH_EXAMPLES_TBB)
  find_package(TBB)
  if(NOT Tbb_FOUND)
    set(HPX_WITH_EXAMPLES_TBB OFF)
  endif()
endif()
hpx_option(
  HPX_WITH_EXAMPLES_QTHREADS BOOL
  "Enable examples requiring QThreads support (default: OFF)." OFF
  CATEGORY "Build Targets"
  ADVANCED
)
if(HPX_WITH_EXAMPLES_QTHREADS)
  find_package(QThreads)
  if(NOT Qthreads_FOUND)
    set(HPX_WITH_EXAMPLES_QTHREADS OFF)
  endif()
endif()

hpx_option(
  HPX_WITH_EXAMPLES_HDF5 BOOL
  "Enable examples requiring HDF5 support (default: OFF)." OFF
  CATEGORY "Build Targets"
  ADVANCED
)
if(HPX_WITH_EXAMPLES_HDF5)
  find_package(HDF5 COMPONENTS CXX)
  if(NOT Hdf5_FOUND)
    set(HPX_WITH_EXAMPLES_HDF5 OFF)
  endif()
endif()

# Disabling the Qt example on BG/Q as GUIs don't make sense there anyways
if(NOT "${HPX_PLATFORM_UC}" STREQUAL "BLUEGENEQ")
  hpx_option(
    HPX_WITH_EXAMPLES_QT4 BOOL
    "Enable examples requiring Qt4 support (default: OFF)." OFF
    CATEGORY "Build Targets"
    ADVANCED
  )
  if(HPX_WITH_EXAMPLES_QT4)
    find_package(Qt4)
    if(NOT Qt4_FOUND)
      set(HPX_WITH_EXAMPLES_QT4 OFF)
    endif()
  endif()
endif()

# Debugging related build options
hpx_option(
  HPX_WITH_CONTRACTS BOOL "Enable C++ contracts support in HPX" OFF
  CATEGORY "Debugging"
)

if(HPX_WITH_CONTRACTS)
  hpx_add_config_define(HPX_HAVE_CONTRACTS)
endif()

hpx_option(
  HPX_WITH_ASSERTS_AS_CONTRACT_ASSERTS BOOL
  "Swap hpx_assert with hpx_contract_assert" OFF CATEGORY "Debugging"
)

if(HPX_WITH_ASSERTS_AS_CONTRACT_ASSERTS)
  hpx_add_config_define(HPX_HAVE_ASSERTS_AS_CONTRACT_ASSERTS)
endif()

hpx_option(
  HPX_WITH_VALGRIND BOOL "Enable Valgrind instrumentation support." OFF
  CATEGORY "Debugging"
)

hpx_option(
  HPX_WITH_SANITIZERS BOOL "Configure with sanitizer instrumentation support."
  OFF CATEGORY "Debugging"
)

hpx_option(
  HPX_WITH_VERIFY_LOCKS BOOL
  "Enable lock verification code (default: OFF, enabled in debug builds)" OFF
  CATEGORY "Debugging"
  ADVANCED
)
hpx_option(
  HPX_WITH_VERIFY_LOCKS_BACKTRACE
  BOOL
  "Enable thread stack back trace being captured on lock registration (to be used in combination with HPX_WITH_VERIFY_LOCKS=ON, default: OFF)"
  OFF
  CATEGORY "Debugging"
  ADVANCED
)
hpx_option(
  HPX_WITH_THREAD_DEBUG_INFO
  BOOL
  "Enable thread debugging information (default: OFF, implicitly enabled in debug builds)"
  OFF
  CATEGORY "Debugging"
  ADVANCED
)
hpx_option(
  HPX_WITH_THREAD_GUARD_PAGE BOOL "Enable thread guard page (default: ON)" ON
  CATEGORY "Debugging"
  ADVANCED
)

if(HPX_WITH_VERIFY_LOCKS)
  hpx_add_config_define(HPX_HAVE_VERIFY_LOCKS)
  if(HPX_WITH_VERIFY_LOCKS_BACKTRACE)
    hpx_add_config_define(HPX_HAVE_VERIFY_LOCKS_BACKTRACE)
  endif()
endif()

# Additional debug support
if(NOT WIN32 AND HPX_WITH_THREAD_GUARD_PAGE)
  hpx_add_config_define(HPX_HAVE_THREAD_GUARD_PAGE)
endif()

if(NOT WIN32 AND HPX_WITH_THREAD_STACK_MMAP)
  hpx_add_config_define(HPX_HAVE_THREAD_STACK_MMAP)
endif()

if(HPX_WITH_THREAD_MANAGER_IDLE_BACKOFF)
  hpx_add_config_define(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
endif()

hpx_option(
  HPX_WITH_THREAD_DESCRIPTION_FULL BOOL
  "Use function address for thread description (default: OFF)" OFF
  CATEGORY "Debugging"
  ADVANCED
)

hpx_option(
  HPX_WITH_ATTACH_DEBUGGER_ON_TEST_FAILURE BOOL
  "Break the debugger if a test has failed  (default: OFF)" OFF
  CATEGORY "Debugging"
  ADVANCED
)
if(HPX_WITH_ATTACH_DEBUGGER_ON_TEST_FAILURE)
  hpx_add_config_define(HPX_HAVE_ATTACH_DEBUGGER_ON_TEST_FAILURE)
endif()

hpx_option(
  HPX_WITH_TESTS_DEBUG_LOG BOOL
  "Turn on debug logs (--hpx:debug-hpx-log) for tests (default: OFF)" OFF
  CATEGORY "Debugging"
  ADVANCED
)

hpx_option(
  HPX_WITH_TESTS_DEBUG_LOG_DESTINATION STRING
  "Destination for test debug logs (default: cout)" "cout"
  CATEGORY "Debugging"
  ADVANCED
)

hpx_option(
  HPX_WITH_TESTS_COMMAND_LINE STRING
  "Add given command line options to all tests run" ""
  CATEGORY "Debugging"
  ADVANCED
)

hpx_option(
  HPX_WITH_TESTS_MAX_THREADS_PER_LOCALITY
  STRING
  "Maximum number of threads to use for tests (default: 0, use the number of threads specified by the test)"
  0
  CATEGORY "Debugging"
  ADVANCED
)

hpx_option(
  HPX_WITH_PARALLEL_TESTS_BIND_NONE
  BOOL
  "Pass --hpx:bind=none to tests that may run in parallel (cmake -j flag) (default: OFF)"
  OFF
  CATEGORY "Debugging"
  ADVANCED
)

# If APEX is defined, the action timers need thread debug info.
if(HPX_WITH_APEX)
  hpx_add_config_define(HPX_HAVE_THREAD_DESCRIPTION)
  if(HPX_WITH_THREAD_DESCRIPTION_FULL)
    hpx_add_config_define(HPX_HAVE_THREAD_DESCRIPTION_FULL)
  endif()
endif()

if(HPX_WITH_THREAD_DEBUG_INFO)
  hpx_add_config_define(HPX_HAVE_THREAD_TARGET_ADDRESS)
  hpx_add_config_define(HPX_HAVE_THREAD_PARENT_REFERENCE)
  hpx_add_config_define(HPX_HAVE_THREAD_PHASE_INFORMATION)
  hpx_add_config_define(HPX_HAVE_THREAD_DESCRIPTION)
  if(HPX_WITH_THREAD_DESCRIPTION_FULL)
    hpx_add_config_define(HPX_HAVE_THREAD_DESCRIPTION_FULL)
  endif()
  hpx_add_config_define(HPX_HAVE_THREAD_MINIMAL_DEADLOCK_DETECTION)
endif()

# run hpx_main on all localities by default (will be removed after V1.11.0)
hpx_option(
  HPX_WITH_RUN_MAIN_EVERYWHERE
  BOOL
  "Run hpx_main by default on all localities (default: OFF, deprecated, will be removed)."
  OFF
  ADVANCED
)
if(HPX_WITH_RUN_MAIN_EVERYWHERE)
  hpx_info("Note: the configuration parameter HPX_WITH_RUN_MAIN_EVERYWHERE is\n"
           "     deprecated and will be removed in the future."
  )
  hpx_add_config_define(HPX_HAVE_RUN_MAIN_EVERYWHERE)
endif()

if(HPX_WITH_NETWORKING)
  # Options for our plugins
  hpx_option(
    HPX_WITH_COMPRESSION_BZIP2 BOOL
    "Enable bzip2 compression for parcel data (default: OFF)." OFF ADVANCED
  )
  hpx_option(
    HPX_WITH_COMPRESSION_SNAPPY BOOL
    "Enable snappy compression for parcel data (default: OFF)." OFF ADVANCED
  )
  hpx_option(
    HPX_WITH_COMPRESSION_ZLIB BOOL
    "Enable zlib compression for parcel data (default: OFF)." OFF ADVANCED
  )

  # Parcel coalescing is used by the main HPX library, enable it always
  hpx_option(
    HPX_WITH_PARCEL_COALESCING BOOL
    "Enable the parcel coalescing plugin (default: ON)." ON ADVANCED
  )
  if(HPX_WITH_PARCEL_COALESCING)
    hpx_add_config_define(HPX_HAVE_PARCEL_COALESCING)
    # Adaptive parcel coalescing related metrics counters are enabled only if
    # both parcel coalescing plugin and thread idle rate counters are enabled.
    if(HPX_WITH_THREAD_IDLE_RATES)
      hpx_option(
        HPX_WITH_BACKGROUND_THREAD_COUNTERS
        BOOL
        "Enable performance counters related to adaptive parcel coalescing (default: OFF)."
        OFF
        ADVANCED
      )
      if(HPX_WITH_BACKGROUND_THREAD_COUNTERS)
        hpx_add_config_define(HPX_HAVE_BACKGROUND_THREAD_COUNTERS)
      endif()
    endif()
  endif()
endif()

# Developer tools
hpx_option(
  HPX_WITH_VIM_YCM BOOL
  "Generate HPX completion file for VIM YouCompleteMe plugin" OFF ADVANCED
)

# ##############################################################################
# Backwards compatibility options (edit for each release)

# BADBAD: This enables an overload of swap which is necessary to work around the
# problems caused by zip_iterator not being a real random access iterator.
# Dereferencing zip_iterator does not yield a true reference but only a
# temporary tuple holding true references.
#
# A real fix for this problem is proposed in PR0022R0
# (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0022r0.html)
#
hpx_option(
  HPX_WITH_TUPLE_RVALUE_SWAP
  BOOL
  "Enable swapping of rvalue tuples (needed for parallel::sort_by_key, default: ON)."
  ON
  CATEGORY "Utility"
  ADVANCED
)
if(HPX_WITH_TUPLE_RVALUE_SWAP)
  hpx_add_config_define(HPX_HAVE_TUPLE_RVALUE_SWAP)
endif()

hpx_option(
  HPX_WITH_MODULE_COMPATIBILITY_HEADERS BOOL
  "Generate backwards-compatibility headers for HPX Modules (default: OFF)" OFF
  ADVANCED
)

# ##############################################################################
# Special diagnostic flags, do not enable by default, only if needed
hpx_option(
  HPX_WITH_CHECK_MODULE_DEPENDENCIES
  BOOL
  "Verify that no modules are cross-referenced from a different module category \
  (default: OFF)"
  OFF
  ADVANCED
)

# Check for compiler compatibility
#

# Check if the selected compiler versions are supposed to work with our codebase
if(CMAKE_COMPILER_IS_GNUCXX AND HPX_WITH_GCC_VERSION_CHECK)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
    hpx_error(
      "GCC 7.0 or higher is required. Specify HPX_WITH_GCC_VERSION_CHECK=OFF to ignore this error."
    )
  endif()
endif()

if(MSVC)
  if(MSVC_VERSION LESS 1900)
    hpx_error("MSVC x64 2015 or higher is required.")
  elseif(NOT CMAKE_CL_64)
    hpx_warn(
      "MSVC (32Bit) will compile but will fail running larger applications because of limitations in the Windows OS."
    )
  endif()
endif()

# Setup platform specific compiler options and check for compatible compilers
if("${HPX_PLATFORM_UC}" STREQUAL "NATIVE")
  hpx_info("Compiling with the native toolset")
elseif("${HPX_PLATFORM_UC}" STREQUAL "ANDROID")
  hpx_info("Compiling for Android devices")
elseif("${HPX_PLATFORM_UC}" STREQUAL "XEONPHI")
  hpx_info("Compiling for Intel Xeon Phi devices")
  if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))
    hpx_error("HPX on the MIC can only be compiled with the Intel compiler.")
  endif()
elseif("${HPX_PLATFORM_UC}" STREQUAL "BLUEGENEQ")
  if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    hpx_error("HPX on the BG/Q can only be compiled with bgclang")
  endif()
  hpx_info("Compiling for BlueGene/Q")
endif()

# ##############################################################################
# Some special handling of the compilation is need on build infrastructure for
# generating packages for target architecture, see issue #3575

hpx_option(
  HPX_WITH_BUILD_BINARY_PACKAGE
  BOOL
  "Build HPX on the build infrastructure on any LINUX distribution (default: OFF)."
  OFF
  ADVANCED
)

# ##############################################################################
# C++ feature tests
# ##############################################################################

# Sometimes we need to be able to explicitly disable supporting the
# [[no_unique_address]] attribute. This may be of use when mixing compilers, for
# instance.
hpx_option(
  HPX_WITH_SUPPORT_NO_UNIQUE_ADDRESS_ATTRIBUTE BOOL
  "Enable the use of the [[no_unique_address]] attribute (default: ON)" ON
  ADVANCED
)

hpx_option(
  HPX_WITH_CXX20_STD_EXECUTION_POLICES
  BOOL
  "Support using std execution policies for HPX parallel algorithms (default: ON)."
  ON
  ADVANCED
)
if(HPX_WITH_CXX20_STD_EXECUTION_POLICES)
  hpx_add_config_define(HPX_HAVE_CXX20_STD_EXECUTION_POLICES)
endif()

# Need to include the CUDA setup before the config test to enable the CUDA
# language
include(HPX_SetupCUDA)
include(HPX_PerformCxxFeatureTests)
hpx_perform_cxx_feature_tests()

include(TargetArch)
target_architecture(__target_arch)

include(CacheLineSize)
cache_line_size(__cache_line_size)

# ##############################################################################
# Set configuration option to use Boost.Context or not. This depends on the
# platform.
set(__use_generic_coroutine_context OFF)
if(APPLE)
  set(__use_generic_coroutine_context ON)
endif()
if("${HPX_PLATFORM_UC}" STREQUAL "BLUEGENEQ")
  set(__use_generic_coroutine_context ON)
endif()
# If compiling for riscv64, arm or arm64 (including armv5, armv6, armv7),
# automatically bake in Boost.Context
if(${__target_arch} STREQUAL "riscv64"
   OR ${__target_arch} STREQUAL "arm64"
   OR ${__target_arch} STREQUAL "arm"
   OR ${__target_arch} STREQUAL "armv5"
   OR ${__target_arch} STREQUAL "armv6"
   OR ${__target_arch} STREQUAL "armv7"
)
  set(__use_generic_coroutine_context ON)
endif()

hpx_option(
  HPX_WITH_GENERIC_CONTEXT_COROUTINES
  BOOL
  "Use Boost.Context as the underlying coroutines context switch implementation."
  ${__use_generic_coroutine_context}
  ADVANCED
)

# ##############################################################################
# check for miscellaneous things
# ##############################################################################

hpx_check_for_mm_prefetch(DEFINITIONS HPX_HAVE_MM_PREFETCH)

hpx_check_for_pthread_setname_np(DEFINITIONS HPX_WITH_PTHREAD_SETNAME_NP)

hpx_check_for_stable_inplace_merge(DEFINITIONS HPX_HAVE_STABLE_INPLACE_MERGE)

if(NOT HPX_WITH_STABLE_INPLACE_MERGE)
  hpx_warn(
    "The standard library you are using (libc++ version < 6) does not have a stable inplace_merge implementation."
  )
endif()

# ##############################################################################
# HPX datapar configuration
# ##############################################################################
include(HPX_SetupDatapar)

# ##############################################################################
# Check for misc system headers
# ##############################################################################

hpx_check_for_unistd_h(DEFINITIONS HPX_HAVE_UNISTD_H)

if(NOT WIN32)
  # ############################################################################
  # Macro definitions for system headers
  # ############################################################################
  add_definitions(-D_GNU_SOURCE)

  # ############################################################################
  # System libraries
  # ############################################################################
  if(NOT MSVC)
    if(HPX_CXX11_STD_ATOMIC_LIBRARIES)
      target_link_libraries(
        hpx_base_libraries INTERFACE ${HPX_CXX11_STD_ATOMIC_LIBRARIES}
      )
    endif()
  endif()

  if(HPX_HAVE_LIBSUPCPP)
    target_link_libraries(hpx_base_libraries INTERFACE supc++)
  endif()

  if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    target_link_libraries(hpx_base_libraries INTERFACE dl)
  endif()

  if(NOT APPLE AND NOT ("${HPX_PLATFORM_UC}" STREQUAL "ANDROID"))
    target_link_libraries(hpx_base_libraries INTERFACE rt)
  endif()

  if("${HPX_PLATFORM_UC}" STREQUAL "ANDROID")
    target_link_libraries(hpx_base_libraries INTERFACE log)
  endif()

  if(APPLE)
    hpx_add_compile_flag_if_available(-ftemplate-depth=256)
  endif()
endif()

if(WIN32)
  if(MSVC)
    hpx_add_target_compile_option(-Ox PUBLIC CONFIGURATIONS Release)

    # even VS2017 has an ICE when compiling with -Ob2
    hpx_add_target_compile_option(-Ob1 PUBLIC CONFIGURATIONS Release)

    # VS2012 and above has a special flag for improving the debug experience by
    # adding more symbol information to the build (-d2Zi)
    hpx_add_target_compile_option(-d2Zi+ PUBLIC CONFIGURATIONS RelWithDebInfo)

    # VS2013 and above know how to do link time constant data segment folding
    # VS2013 update 2 and above know how to remove debug information for
    # non-referenced functions and data (-Zc:inline)
    hpx_add_target_compile_option(-Zc:inline PUBLIC)
    hpx_add_target_compile_option(
      -Gw PUBLIC CONFIGURATIONS Release RelWithDebInfo MinSizeRel
    )
    hpx_add_target_compile_option(-Zo PUBLIC CONFIGURATIONS RelWithDebInfo)
    if("${HPX_WITH_DATAPAR_BACKEND}" STREQUAL "VC")
      hpx_add_target_compile_option(-std:c++latest PUBLIC)
      hpx_add_config_cond_define(_HAS_AUTO_PTR_ETC 1)
    endif()

    # Exceptions
    hpx_add_target_compile_option(-EHsc)
    if(MSVC_VERSION GREATER_EQUAL 1900)
      if(NOT (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"
              AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
      )
        # assume conforming (throwing) operator new implementations
        hpx_add_target_compile_option(-Zc:throwingNew PUBLIC)
      endif()

      # enable faster linking (requires VS2015 Update1) disabled for now as this
      # flag crashes debugger hpx_remove_link_flag(/debug CONFIGURATIONS Debug)
      # hpx_add_link_flag(/debug:fastlink CONFIGURATIONS Debug)

      # Update 2 requires to set _ENABLE_ATOMIC_ALIGNMENT_FIX for it to compile
      # atomics
      hpx_add_config_cond_define(_ENABLE_ATOMIC_ALIGNMENT_FIX)

      # Update 3 allows to flag rvalue misuses and enforces strict string const-
      # qualification conformance
      hpx_add_target_compile_option(-Zc:rvalueCast PUBLIC)
      hpx_add_target_compile_option(-Zc:strictStrings PUBLIC)

      # Update 8 requires to set _ENABLE_EXTENDED_ALIGNED_STORAGE for it to
      # compile atomics
      hpx_add_config_cond_define(_ENABLE_EXTENDED_ALIGNED_STORAGE)

      # Make sure that __cplusplus is properly defined
      hpx_add_target_compile_option(-Zc:__cplusplus PUBLIC)

      # stdexec requires the conforming MSVC preprocessor.
      if(NOT (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"
              AND "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
      )
        hpx_add_target_compile_option(-Zc:preprocessor PUBLIC)
      endif()

      # Silence C++17 deprecation warnings
      hpx_add_config_cond_define(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)

      # Silence C++20 deprecation warnings
      hpx_add_config_cond_define(_SILENCE_ALL_CXX20_DEPRECATION_WARNINGS)

      # Silence C++23 deprecation warnings
      hpx_add_config_cond_define(_SILENCE_ALL_CXX23_DEPRECATION_WARNINGS)

      # ASan is available in Visual Studion starting V16.8
      if((MSVC_VERSION GREATER_EQUAL 1928) AND HPX_WITH_SANITIZERS)
        hpx_add_target_compile_option(
          -fsanitize=address PUBLIC CONFIGURATIONS RelWithDebInfo Debug
        )
      endif()
    endif()

    # Runtime type information
    hpx_add_target_compile_option(-GR PUBLIC)
    # Multiprocessor build
    hpx_add_target_compile_option(-MP PUBLIC)
    # Increase the maximum size of object file sections
    hpx_add_target_compile_option(-bigobj PUBLIC)
  elseif(MINGW)
    if(HPX_CXX11_STD_ATOMIC_LIBRARIES)
      target_link_libraries(
        hpx_base_libraries INTERFACE ${HPX_CXX11_STD_ATOMIC_LIBRARIES}
      )
    endif()
  endif()

  target_link_libraries(hpx_base_libraries INTERFACE psapi shlwapi)

  # ############################################################################
  # Macro definitions for system headers
  # ############################################################################
  add_definitions(-D_WINDOWS)
  add_definitions(-D_WIN32)
  hpx_add_config_cond_define(_WIN32_WINNT 0x0602)
  hpx_add_config_cond_define(_SCL_SECURE_NO_WARNINGS)
  hpx_add_config_cond_define(_CRT_SECURE_NO_WARNINGS)
  hpx_add_config_cond_define(_SCL_SECURE_NO_DEPRECATE)
  hpx_add_config_cond_define(_CRT_SECURE_NO_DEPRECATE)
  hpx_add_config_cond_define(_CRT_NONSTDC_NO_WARNINGS)
  hpx_add_config_cond_define(_WINSOCK_DEPRECATED_NO_WARNINGS)
  hpx_add_config_cond_define(_CRT_NON_CONFORMING_SWPRINTFS)
  hpx_add_config_cond_define(_SILENCE_FPOS_SEEKPOS_DEPRECATION_WARNING)

  # ############################################################################
  # Boost
  # ############################################################################

  hpx_add_config_cond_define(BOOST_USE_WINDOWS_H)
  if(NOT CMAKE_CL_64)
    hpx_add_config_cond_define(BOOST_NO_ALIGNMENT)
  endif()
  if(NOT HPX_WITH_GENERIC_CONTEXT_COROUTINES)
    hpx_add_config_define(HPX_HAVE_FIBER_BASED_COROUTINES)
  endif()
  hpx_add_config_cond_define(PSAPI_VERSION 1)
endif()

# Configure Warnings
if(HPX_WITH_COMPILER_WARNINGS)
  if(MSVC) # Adding special warning settings for the MSVC compiler ...
    hpx_add_compile_flag(-W3)

    # MSVC2012/2013 are overeager to report 'qualifier applied to function type
    # has no meaning; ignored'
    hpx_add_compile_flag(-wd4180)

    # Boost.Lockfree triggers 'warning C4307: '+' : integral constant overflow'
    # which is benign
    hpx_add_compile_flag(-wd4307)

    # object allocated on the heap may not be aligned
    hpx_add_compile_flag(-wd4316)

    # max symbol length exceeded
    hpx_add_compile_flag(-wd4503)

    # 'int': forcing value to bool 'true' or 'false' (performance warning)
    hpx_add_compile_flag(-wd4800)

    # vcpkg enables the /utf-8 option which causes (benign) warnings in the
    # Spirit headers: The file contains a character starting at offset ... that
    # is illegal in the current source character set
    hpx_add_compile_flag(-wd4828)

    if("${HPX_WITH_DATAPAR_BACKEND}" STREQUAL "VC")
      # unary minus operator applied to unsigned type, result still unsigned
      hpx_add_compile_flag(-wd4146)

      # '<=': signed/unsigned mismatch
      hpx_add_compile_flag(-wd4018)

      # 'return': conversion from 'short' to 'Vc_1::schar', possible loss of
      # data
      hpx_add_compile_flag(-wd4244)
    endif()

    # when compiling with modules enabled, the following warning can't be
    # disabled inline
    if(HPX_WITH_CXX_MODULES)
      # 'symbol' needs to have dll-interface to be used by clients of 'symbol'
      hpx_add_compile_flag(-wd4251)
      # non dll-interface struct 'symbol' used as base for dll-interface class
      # 'symbol'
      hpx_add_compile_flag(-wd4275)
    endif()

  else() # Trial and error approach for any other compiler ...
    hpx_add_compile_flag_if_available(-Wall)
    hpx_add_compile_flag_if_available(-Wno-strict-aliasing)
    hpx_add_compile_flag_if_available(-Wno-sign-promo)
    hpx_add_compile_flag_if_available(-Wno-attributes)
    hpx_add_compile_flag_if_available(-Wno-cast-align)

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
      hpx_add_compile_flag_if_available(-Wno-use-after-free)
      if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_GREATER_EQUAL "12.1")
        # gcc V12.1 and above complain about possible ABI incompatibilities
        # caused by the use of std::destructive_interference_size
        hpx_add_compile_flag_if_available(-Wno-interference-size)
      endif()
      if("${CMAKE_CXX_COMPILER_VERSION}" VERSION_GREATER_EQUAL "13.0")
        # gcc V13.x is overeager to report dangling references where there are
        # none
        hpx_add_compile_flag_if_available(-Wextra)
        hpx_add_compile_flag_if_available(-Wpedantic)
        hpx_add_compile_flag_if_available(-Wno-self-move)
      endif()
    else()
      hpx_add_compile_flag_if_available(-Wextra)
      hpx_add_compile_flag_if_available(-Wpedantic)
    endif()

    # We do not in general guarantee ABI compatibility between C++ standards, so
    # we ignore this warning
    hpx_add_compile_flag_if_available(-Wno-noexcept-type)

    # Be extra strict about format checks Boost.Logging is built on fprintf,
    # sadly
    hpx_add_compile_flag_if_available(-Wformat=2)
    hpx_add_compile_flag_if_available(-Wno-format-nonliteral)

    # Self initialization is dangerous
    hpx_add_compile_flag_if_available(-Winit-self)

    # For portability
    hpx_add_compile_flag_if_available(-Wdouble-promotion)

    # Warn about casting that violates qualifiers or alignment
    hpx_add_compile_flag_if_available(-Wcast-qual)
    if(NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      # Clang is overeager in reporting cast alignment problems in Boost
      hpx_add_compile_flag_if_available(-Wcast-align)
    endif()

    # False positive when build with Vc and Clang
    if(NOT (("${HPX_WITH_DATAPAR_BACKEND}" STREQUAL "VC")
            AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    )
      hpx_add_compile_flag_if_available(-Werror=trampolines)
    endif()
    hpx_add_compile_flag_if_available(-Werror=parentheses)
    hpx_add_compile_flag_if_available(-Werror=reorder)
    hpx_add_compile_flag_if_available(-Werror=return-type)
    hpx_add_compile_flag_if_available(-Werror=sequence-point)
    hpx_add_compile_flag_if_available(-Werror=uninitialized)
    hpx_add_compile_flag_if_available(-Werror=format)
    hpx_add_compile_flag_if_available(-Werror=missing-braces)
    hpx_add_compile_flag_if_available(-Werror=sign-compare)
  endif()
endif()

# Configure compiler warnings as errors
if(HPX_WITH_COMPILER_WARNINGS_AS_ERRORS)
  if(MSVC)
    hpx_add_compile_flag(-WX)
  else()
    hpx_add_compile_flag_if_available(-Werror)
  endif()
endif()

# Diagnostics
if(MSVC)
  # Display full paths in diagnostics
  hpx_add_compile_flag(-FC)

  # compile all of HPX in non-permissive mode
  hpx_add_target_compile_option(-permissive- PUBLIC)
  if(CMAKE_CL_64)
    set(__target_arch "x86_64")
  else()
    set(__target_arch "x86")
  endif()
  hpx_info("Architecture detected: ${__target_arch}")
else()
  # Show the flags that toggle each warning
  hpx_add_compile_flag_if_available(-fdiagnostics-show-option)

  # VLAs are a GNU extensions that we forbid as they are not supported on MSVC
  hpx_add_compile_flag_if_available(-Werror=vla)
  # No return statement in a non-void function can lead to garbage return values
  # in GCC.
  hpx_add_compile_flag_if_available(-Werror=return-type)

  # We get false positives all over the place with this.
  if(CMAKE_COMPILER_IS_GNUCXX)
    hpx_add_compile_flag_if_available(-Wno-unused-but-set-parameter)
    hpx_add_compile_flag_if_available(-Wno-unused-but-set-variable)
    # Uninitialized variables are bad, earlier compilers issue spurious warnings
    hpx_add_compile_flag_if_available(-Werror=uninitialized)
    hpx_add_compile_flag_if_available(-Wno-unused-local-typedefs)
    # -Werror=maybe-uninitialized leads to false positives.
    hpx_add_compile_flag_if_available(-Wno-maybe-uninitialized)
  endif()

  # False positive when building with Vc and Clang
  if(NOT (("${HPX_WITH_DATAPAR_BACKEND}" STREQUAL "VC")
          AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  )
    # Silence warning about __sync_fetch_and_nand changing semantics
    hpx_add_compile_flag_if_available(-Wno-sync-nand)
  endif()

  # Silence warnings about deleting polymorphic objects with non-virtual dtors.
  # These come from within Boost.
  if(CMAKE_COMPILER_IS_GNUCXX)
    hpx_add_compile_flag_if_available(-Wno-delete-non-virtual-dtor)
  endif()

  # Check if our libraries have unresolved symbols if(NOT APPLE AND NOT
  # HPX_WITH_APEX)
  if(NOT APPLE
     AND NOT WIN32
     AND NOT HPX_WITH_SANITIZERS
  )
    hpx_add_link_flag_if_available(
      -Wl,-z,defs TARGETS SHARED_LIBRARY EXECUTABLE
    )
  endif()
  if(WIN32)
    target_link_libraries(hpx_base_libraries INTERFACE psapi WS2_32 mswsock)
  endif()

  if("${HPX_PLATFORM_UC}" STREQUAL "BLUEGENEQ")
    hpx_add_compile_flag_if_available(-Wno-deprecated-register)
  endif()

  if(HPX_WITH_HIDDEN_VISIBILITY)
    hpx_add_compile_flag_if_available(-fvisibility=hidden)
    hpx_add_link_flag_if_available(
      -fvisibility=hidden TARGETS SHARED_LIBRARY EXECUTABLE
    )
    hpx_add_config_define(HPX_HAVE_ELF_HIDDEN_VISIBILITY)
  endif()

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    hpx_add_compile_flag_if_available(-Wno-cast-align)
  endif()

  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    # Disable the following warnings: #1170: invalid redeclaration of nested
    # class
    hpx_add_compile_flag_if_available(-wd1170)
    # #858: type qualifier on return type is meaningless
    hpx_add_compile_flag_if_available(-wd858)
    # #1098: the qualifier on this friend declaration is ignored
    hpx_add_compile_flag_if_available(-wd1098)
    # #488: template parameter not used in declaring the parameter type
    hpx_add_compile_flag_if_available(-wd488)
    # #2203: cast discards qualifiers from target type (needed for mvapich2 mpi
    # header)
    hpx_add_compile_flag_if_available(-wd2203)
    # #2536: cannot specify explicit initializer for arrays
    hpx_add_compile_flag_if_available(-wd2536)
    # #1286: invalid attribute
    hpx_add_compile_flag_if_available(-wd1286)
  endif()

  set(__has_timestamp_support OFF)

  if("${__target_arch}" STREQUAL "i386"
     OR "${__target_arch}" STREQUAL "ix86"
     OR "${__target_arch}" STREQUAL "x86_64"
     OR "${__target_arch}" STREQUAL "ia64"
  )

    # rdtsc is an x86 instruction that reads the value of a CPU time stamp
    # counter. rdtscp is an extension to rdtsc. The difference is that rdtscp is
    # a serializing instruction.
    hpx_cpuid("rdtsc" HPX_WITH_RDTSC DEFINITIONS HPX_HAVE_RDTSC)

    # One can not assume if RDTSCP is available on the hardware of the build
    # infrastructure, that it will be available on all potential target
    # hardware, see Issue  #3575
    if(NOT ${HPX_WITH_BUILD_BINARY_PACKAGE})

      # XeonPhi's do not support RDTSCP
      if(NOT ("${HPX_PLATFORM_UC}" STREQUAL "XEONPHI"))
        hpx_cpuid("rdtscp" HPX_WITH_RDTSCP DEFINITIONS HPX_HAVE_RDTSCP)
      endif()

    endif()
    if(HPX_WITH_RDTSC OR HPX_WITH_RDTSCP)
      set(__has_timestamp_support ON)
    endif()
  elseif(
    "${__target_arch}" STREQUAL "arm"
    OR "${__target_arch}" STREQUAL "armv5"
    OR "${__target_arch}" STREQUAL "armv6"
    OR "${__target_arch}" STREQUAL "armv7"
    OR "${__target_arch}" STREQUAL "arm64"
  )
    set(__has_timestamp_support ON)
  elseif("${__target_arch}" STREQUAL "ppc" OR "${__target_arch}" STREQUAL
                                              "ppc64"
  )
    set(__has_timestamp_support ON)
  elseif("${__target_arch}" STREQUAL "riscv64")
    set(__has_timestamp_support ON)
  elseif("${__target_arch}" STREQUAL "bgq")
    set(__has_timestamp_support ON)
  elseif("${__target_arch}" STREQUAL "s390x")
    set(__has_timestamp_support ON)
  endif()

  hpx_info("Architecture detected: ${__target_arch}")
  if(NOT __has_timestamp_support)
    hpx_warn(
      "No timestamp support is available; some performance counters may report incorrect results"
    )
  endif()
endif()
hpx_info("Cacheline size detected: ${__cache_line_size}")

# store target architecture for later use
set(HPX_WITH_TARGET_ARCHITECTURE
    ${__target_arch}
    CACHE INTERNAL "" FORCE
)

# Compatibility with using Boost.FileSystem, introduced in V1.4.0
set(__filesystem_compatibility_default ON)
if(HPX_WITH_CXX17_FILESYSTEM)
  set(__filesystem_compatibility_default OFF)
endif()
hpx_option(
  HPX_FILESYSTEM_WITH_BOOST_FILESYSTEM_COMPATIBILITY
  BOOL
  "Enable Boost.FileSystem compatibility. (default: ${__filesystem_compatibility_default})"
  ${__filesystem_compatibility_default}
  ADVANCED
  CATEGORY "Modules"
)

# Compatibility with using Boost.Iterator traversal tags, introduced in V1.7.0
hpx_option(
  HPX_ITERATOR_SUPPORT_WITH_BOOST_ITERATOR_TRAVERSAL_TAG_COMPATIBILITY BOOL
  "Enable Boost.Iterator traversal tag compatibility. (default: OFF)" OFF
  ADVANCED CATEGORY "Modules"
)
if(HPX_ITERATOR_SUPPORT_WITH_BOOST_ITERATOR_TRAVERSAL_TAG_COMPATIBILITY)
  hpx_add_config_define_namespace(
    DEFINE HPX_ITERATOR_SUPPORT_HAVE_BOOST_ITERATOR_TRAVERSAL_TAG_COMPATIBILITY
    NAMESPACE ITERATOR_SUPPORT
  )
endif()

# ##############################################################################
# Find Our dependencies: These are all dependencies needed to build the core
# library. Dependencies that are only needed by plugins, examples or tests
# should be found separately in the appropriate subdirectory.

include(HPX_SetupThreads)

# Setup our required Boost libraries.
include(HPX_SetupBoost)
include(HPX_SetupBoostFilesystem)

# Set up standalone Asio
include(HPX_SetupAsio)

# Find all allocators which are currently supported.
include(HPX_SetupAllocator)

if(HPX_WITH_FETCH_HWLOC)
  set(HPX_HWLOC_INSTALL_PATH ${CMAKE_INSTALL_PREFIX}/hwloc_installed)
endif()
include(HPX_SetupHwloc)

# reset external source lists
add_hpx_library_sources_noglob(hpx_external)
add_hpx_library_headers_noglob(hpx_external)

# Setup plugins (set here cause if we include it inside plugins, it will not be
# defined in src/CMakeLists.txt where we call add_static_parcelports)
include(HPX_SetupMPI) # must come before APEX
if((HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_MPI)
   OR HPX_WITH_ASYNC_MPI
   OR ("${HPX_WITH_PARCELPORT_GASNET_CONDUIT}" STREQUAL "mpi")
)
  hpx_setup_mpi()
endif()
if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCI)
  include(HPX_SetupLCI)
  hpx_setup_lci()
endif()
if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_LCW)
  include(HPX_SetupLCW)
  hpx_setup_lcw()
endif()
if(HPX_WITH_NETWORKING AND HPX_WITH_PARCELPORT_GASNET)
  include(HPX_SetupGasnet)
  hpx_setup_gasnet()
endif()

# Setup packages and subprojects
include(HPX_SetupSYCL)
include(HPX_SetupHIP)
include(HPX_SetupApex)
include(HPX_SetupPapi)
include(HPX_SetupValgrind)
if(HPX_WITH_CUDA OR HPX_WITH_HIP)
  hpx_add_config_define(HPX_HAVE_GPU_SUPPORT)
endif()

# Setup NVIDIA's stdexec if requested
include(HPX_SetupStdexec)

if(HPX_WITH_SANITIZERS)
  hpx_add_config_define(HPX_HAVE_SANITIZERS)
endif()

if(HPX_WITH_VIM_YCM)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

# ##############################################################################
# Check Build Options based on the found dependencies. We also check for errors
# with incompatible options with the currently selected platform.
#

if(HPX_WITH_GENERIC_CONTEXT_COROUTINES)
  # Check if we can use generic coroutine contexts without any problems
  if(NOT (Boost_CONTEXT_FOUND OR TARGET Boost::context))
    hpx_error(
      "The usage of Boost.Context was selected but Boost.Context was not found."
    )
  endif()
  hpx_add_config_define(HPX_HAVE_GENERIC_CONTEXT_COROUTINES)
endif()

# ##############################################################################

# ##############################################################################
# HPX_PREFIX The prefix is the default search path for HPX plugins
# ##############################################################################
if("${HPX_PLATFORM_UC}" STREQUAL "ANDROID")
  set(HPX_PREFIX "lib")
  set(HPX_BUILD_PREFIX "lib")
else()
  set(HPX_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(HPX_BUILD_PREFIX "${PROJECT_BINARY_DIR}")
endif()

# Note: on windows systems the ':' will be converted to a ';' at runtime
hpx_add_config_cond_define(
  HPX_DEFAULT_COMPONENT_PATH_SUFFIXES
  "\"/${CMAKE_INSTALL_LIBDIR}/hpx:/${CMAKE_INSTALL_BINDIR}/hpx:/lib/hpx:/bin/hpx\""
)

# ##############################################################################
# search path configuration
# ##############################################################################
if(HPX_WITH_FULL_RPATH)
  include(HPX_SetFullRPATH)
endif()

# ##############################################################################
# Git commit detection
# ##############################################################################
include(HPX_GitCommit)
hpx_add_config_define(HPX_HAVE_GIT_COMMIT "\"${HPX_WITH_GIT_COMMIT}\"")

include(HPX_SetOutputPaths)

# ##############################################################################
# Fixing git tag, if necessary
hpx_debug("HPX_WITH_GIT_TAG: " ${HPX_WITH_GIT_TAG})
hpx_debug("HPX_WITH_GIT_BRANCH: " ${HPX_WITH_GIT_BRANCH})

if(HPX_WITH_GIT_BRANCH AND ((NOT HPX_WITH_GIT_TAG) OR "${HPX_WITH_GIT_TAG}x"
                                                      STREQUAL "x")
)
  string(REGEX MATCH "^release-[0-9]+.[0-9]+.X$" matched ${HPX_WITH_GIT_BRANCH})
  if(matched)
    set(HPX_WITH_GIT_TAG "v${HPX_VERSION_FULL}")
    hpx_info("Setting HPX_WITH_GIT_TAG to: " "${HPX_WITH_GIT_TAG}")
  endif()
endif()

# ##############################################################################
# Add custom targets for tests
# ##############################################################################
if(HPX_WITH_TESTS)
  add_hpx_pseudo_target(tests)
  if(HPX_WITH_TESTS_UNIT)
    add_hpx_pseudo_target(tests.unit)
    add_hpx_pseudo_dependencies(tests tests.unit)
  endif()
  if(HPX_WITH_TESTS_REGRESSIONS)
    add_hpx_pseudo_target(tests.regressions)
    add_hpx_pseudo_dependencies(tests tests.regressions)
  endif()
  if(HPX_WITH_TESTS_BENCHMARKS)
    add_hpx_pseudo_target(tests.performance)
    add_hpx_pseudo_dependencies(tests tests.performance)
  endif()
  if(HPX_WITH_TESTS_HEADERS)
    add_hpx_pseudo_target(tests.headers)
    add_hpx_pseudo_dependencies(tests tests.headers)
  endif()
  if(HPX_WITH_EXAMPLES AND HPX_WITH_TESTS_EXAMPLES)
    add_hpx_pseudo_target(tests.examples)
    add_hpx_pseudo_dependencies(tests tests.examples)
  endif()

  enable_testing()
  include(CTest)
endif()

if(HPX_WITH_EXAMPLES)
  add_hpx_pseudo_target(examples)
endif()

# ##############################################################################
# Debug library postfix
# ##############################################################################
# Only set debug postfix for Debug builds to maintain ABI separation
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config OR CMAKE_BUILD_TYPE STREQUAL "Debug")
  hpx_option(
    HPX_WITH_DEBUG_POSTFIX STRING
    "Postfix for debug libraries. (default: \"d\")" "d" ADVANCED
    CATEGORY "Build Targets"
  )
  set(CMAKE_DEBUG_POSTFIX "${HPX_WITH_DEBUG_POSTFIX}")
  if(NOT "${HPX_WITH_DEBUG_POSTFIX}" STREQUAL "")
    hpx_add_config_define(HPX_HAVE_DEBUG_POSTFIX "${HPX_WITH_DEBUG_POSTFIX}")
  endif()
endif()

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  if(HPX_WITH_COMPRESSION_BZIP2)
    hpx_add_config_define(HPX_HAVE_COMPRESSION_BZIP2)
  endif()
  if(HPX_WITH_COMPRESSION_SNAPPY)
    hpx_add_config_define(HPX_HAVE_COMPRESSION_SNAPPY)
  endif()
  if(HPX_WITH_COMPRESSION_ZLIB)
    hpx_add_config_define(HPX_HAVE_COMPRESSION_ZLIB)
  endif()
endif()

# ##############################################################################
# Add core dependency
# ##############################################################################
add_hpx_pseudo_target(core)

# ##############################################################################
# Add libraries
# ##############################################################################
# collect names of static parcelports
set(HPX_STATIC_PARCELPORT_PLUGINS
    ""
    CACHE INTERNAL "" FORCE
)
add_subdirectory(libs)

# ##############################################################################
# Configure parcelports
# ##############################################################################
include(HPX_ParcelPorts)

# ##############################################################################
# Add static init and wrap library
# ##############################################################################
add_subdirectory(init)
add_subdirectory(wrap)

# ##############################################################################
# Documentation toolchain (Sphinx, Doxygen, Breathe)
# ##############################################################################
include(HPX_Documentation)

# ##############################################################################
# Add core configuration
# ##############################################################################

# ##############################################################################
# Add components
# ##############################################################################
add_subdirectory(components)

# ##############################################################################
# Tests
# ##############################################################################
if(POLICY CMP0148)
  find_package(Python COMPONENTS Interpreter)
  if(NOT Python_FOUND)
    hpx_warn(
      "A python interpreter could not be found. The test suite can not be run automatically."
    )
  endif()

else()
  find_package(PythonInterp)
  if(NOT PYTHONINTERP_FOUND)
    hpx_warn(
      "A python interpreter could not be found. The test suite can not be run automatically."
    )
  else()
    set(Python_EXECUTABLE ${PYTHON_EXECUTABLE})
  endif()
endif()

if(HPX_WITH_TESTS)
  # pseudo_target added above
  add_subdirectory(tests)
endif()

# ##############################################################################
# Target specification
# ##############################################################################
if(HPX_WITH_EXAMPLES)
  add_subdirectory(examples)
endif()

if(HPX_WITH_DOCUMENTATION)
  add_subdirectory(docs)
endif()

# this must come after docs as it creates a dependency on the target git_docs
if(HPX_WITH_TOOLS OR HPX_WITH_TESTS_BENCHMARKS)
  add_hpx_pseudo_target(tools)
  add_subdirectory(tools)
endif()

# Configure hpxrun.py
if(PYTHONINTERP_FOUND OR Python_FOUND)
  configure_file(
    "${PROJECT_SOURCE_DIR}/cmake/templates/hpxrun.py.in"
    "${PROJECT_BINARY_DIR}/bin/hpxrun.py" @ONLY
  )
endif()

# Set up precompiled headers
if(HPX_WITH_PRECOMPILED_HEADERS)
  set(system_precompiled_headers
      <algorithm>
      <array>
      <atomic>
      <bitset>
      <cassert>
      <cctype>
      <cerrno>
      <chrono>
      <climits>
      <cmath>
      <complex>
      <condition_variable>
      <cstddef>
      <cstdint>
      <cstdio>
      <cstdlib>
      <cstring>
      <ctime>
      <deque>
      <exception>
      <execution>
      <forward_list>
      <fstream>
      <functional>
      <iomanip>
      <ios>
      <iosfwd>
      <iostream>
      <iterator>
      <limits>
      <list>
      <locale>
      <map>
      <memory>
      <mutex>
      <new>
      <numeric>
      <ostream>
      <random>
      <regex>
      <set>
      <shared_mutex>
      <sstream>
      <stack>
      <stdexcept>
      <string>
      <string_view>
      <system_error>
      <thread>
      <tuple>
      <type_traits>
      <typeinfo>
      <unordered_map>
      <unordered_set>
      <utility>
      <variant>
      <vector>
      <hwloc.h>
      <asio/basic_waitable_timer.hpp>
      <asio/io_context.hpp>
      <asio/ip/tcp.hpp>
      <boost/config.hpp>
      <boost/fusion/include/vector.hpp>
      <boost/optional.hpp>
      <boost/spirit/home/x3.hpp>
  )

  if(HPX_WITH_NANOBENCH)
    set(system_precompiled_headers ${system_precompiled_headers} <nanobench.h>)
  endif()

  if(HPX_WITH_CXX17_FILESYSTEM)
    list(APPEND system_precompiled_headers <filesystem>)
  endif()

  set(system_precompiled_headers_dependencies
      hpx_dependencies_boost hpx_private_flags hpx_public_flags Asio::asio
  )

  target_link_libraries(
    hpx_precompiled_headers
    PRIVATE hpx_public_flags hpx_private_flags hpx_base_libraries
            ${system_precompiled_headers_dependencies}
  )
  target_precompile_headers(
    hpx_precompiled_headers PRIVATE ${system_precompiled_headers}
  )

  # Headers that should be precompiled for things depending on HPX (executables,
  # libraries).
  set(hpx_precompiled_headers_modules ${HPX_ENABLED_MODULES})
  # The init_runtime modules cannot be precompiled because they use macros that
  # can depend on the application (HPX_PREFIX and HPX_APPLICATION_NAME_DEFAULT).
  list(REMOVE_ITEM hpx_precompiled_headers_modules init_runtime
       init_runtime_local
  )
  # config, version, and the include modules don't have hpx/module headers.
  list(REMOVE_ITEM hpx_precompiled_headers_modules config include include_local
       version
  )
  list(TRANSFORM hpx_precompiled_headers_modules PREPEND "<hpx/modules/")
  list(TRANSFORM hpx_precompiled_headers_modules APPEND ".hpp>")

  # We do not link to HPX::hpx as it will set HPX_APPLICATION_NAME_DEFAULT and
  # HPX_PREFIX to values that don't match those used in tests. We instead set
  # them manually here.
  target_link_libraries(hpx_exe_precompiled_headers PRIVATE hpx_full)
  target_compile_definitions(
    hpx_exe_precompiled_headers PRIVATE "HPX_PREFIX=\"${HPX_BUILD_PREFIX}\""
                                        "HPX_APPLICATION_EXPORTS"
  )
  target_precompile_headers(
    hpx_exe_precompiled_headers PRIVATE ${hpx_precompiled_headers_modules}
  )
endif()

# ##############################################################################
# installation instructions
# ##############################################################################
if(PYTHONINTERP_FOUND OR Python_FOUND)
  install(
    FILES "${PROJECT_BINARY_DIR}/bin/hpxrun.py"
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
endif()

install(
  # install all hpx header files
  DIRECTORY hpx/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hpx
  COMPONENT core
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN ".git" EXCLUDE
  PATTERN "CMakeFiles" EXCLUDE
  PATTERN "CTestFiles" EXCLUDE
)

# Install all HPX header that have been configured using various cmake options
install(
  DIRECTORY "${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/hpx/"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hpx
  COMPONENT core
  FILES_MATCHING
  PATTERN "*.hpp"
  PATTERN ".git" EXCLUDE
  PATTERN "CMakeFiles" EXCLUDE
  PATTERN "CTestFiles" EXCLUDE
)

install(
  # Install all HPX cmake utility files
  DIRECTORY cmake/
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${HPX_PACKAGE_NAME}
  COMPONENT core
  PATTERN "templates" EXCLUDE
  PATTERN "packaging" EXCLUDE
  PATTERN ".git" EXCLUDE
)

install(
  FILES "${PROJECT_SOURCE_DIR}/LICENSE_1_0.txt"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/hpx
  COMPONENT license
)

if(HPX_WITH_DOCUMENTATION)
  install(
    FILES "${PROJECT_SOURCE_DIR}/docs/index.html"
    DESTINATION ${CMAKE_INSTALL_DATADIR}/hpx/
    COMPONENT docs
  )

  if("html" IN_LIST HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    install(
      DIRECTORY "${PROJECT_BINARY_DIR}/share/hpx/docs/html/"
      DESTINATION ${CMAKE_INSTALL_DATADIR}/hpx/docs/html
      COMPONENT docs
      PATTERN "*.buildinfo" EXCLUDE
    )
  endif()

  if("singlehtml" IN_LIST HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    install(
      DIRECTORY "${PROJECT_BINARY_DIR}/share/hpx/docs/singlehtml/"
      DESTINATION ${CMAKE_INSTALL_DATADIR}/hpx/docs/singlehtml
      COMPONENT docs
      PATTERN "*.buildinfo" EXCLUDE
    )
  endif()

  if("latexpdf" IN_LIST HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    install(
      FILES "${PROJECT_BINARY_DIR}/share/hpx/docs/latexpdf/latex/HPX.pdf"
      DESTINATION ${CMAKE_INSTALL_DATADIR}/hpx/docs/pdf/
      COMPONENT docs
      OPTIONAL
    )
  endif()

  if("man" IN_LIST HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS)
    install(
      FILES "${PROJECT_BINARY_DIR}/share/hpx/docs/man/hpx.1"
      DESTINATION ${CMAKE_INSTALL_MANDIR}/
      COMPONENT docs
    )
  endif()
endif()

if(HPX_WITH_VIM_YCM)
  set(build_dir_file ${PROJECT_BINARY_DIR}/.ycm_extra_conf.py)
  set(source_dir_file ${PROJECT_SOURCE_DIR}/.ycm_extra_conf.py)
  configure_file(
    ${PROJECT_SOURCE_DIR}/tools/vim/.ycm_extra_conf.py ${build_dir_file} @ONLY
  )
  add_custom_target(
    configure_ycm
    COMMAND ${CMAKE_COMMAND} -E copy ${build_dir_file} ${source_dir_file}
    COMMENT "Copying YCM config file to source directory"
    VERBATIM
  )
  hpx_info(
    "VIM YouCompleteMe: run 'make configure_ycm' to copy config file to source directory and enable support in YCM. To enable automatic loading of configure file, add to your .vimrc option: \"let g:ycm_extra_conf_globlist = ['${PROJECT_SOURCE_DIR}/*']\""
  )
endif()

# ##############################################################################
# Add rpm packaging

hpx_option(
  HPX_WITH_RPM BOOL "Enable or disable the generation of rpm packages" OFF
  ADVANCED
)

if(HPX_WITH_RPM)
  add_subdirectory(cmake/packaging/rpm)
endif()

# ##############################################################################
# print overall configuration summary
include(HPX_PrintSummary)
create_configuration_summary("Configuration summary:\n--" "hpx")

include(HPX_ExportTargets)

# Modules can't link to this if not exported
install(
  TARGETS hpx_base_libraries
  EXPORT HPXInternalTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT hpx_base_libraries
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT hpx_base_libraries
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT hpx_base_libraries
)
hpx_export_internal_targets(hpx_base_libraries)

# ##############################################################################
# store cache vars and their values in order for them to be forwarded to the
# projects (needs to be before the HPX_GeneratePackage call)
include(HPX_ForwardCacheVariables)

# ##############################################################################
# External build system support (FindHPX.cmake and pkg-config).
include(HPX_GeneratePackage)

message("")
message("HPX will be installed to ${CMAKE_INSTALL_PREFIX}")
message("")
