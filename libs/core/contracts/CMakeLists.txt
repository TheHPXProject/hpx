# Copyright (c) 2025 The STE||AR-Group
# Copyright (c) 2025 Alexandros Papadakis
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(HPX_WITH_CXX26_CONTRACTS)
  # Map existing HPX_ASSERT to contract_assert
  hpx_option(
    HPX_CONTRACTS_WITH_ASSERTS_AS_CONTRACT_ASSERTS BOOL
    "Map existing HPX_ASERT to contract_assert. (default: ON)" ON ADVANCED
    CATEGORY "Modules"
    MODULE CONTRACTS
  )

  if(HPX_CONTRACTS_WITH_ASSERTS_AS_CONTRACT_ASSERTS)
    hpx_add_config_define_namespace(
      DEFINE HPX_CONTRACTS_HAVE_ASSERTS_AS_CONTRACT_ASSERTS NAMESPACE CONTRACTS
    )
  endif()
else()
  hpx_option(
    HPX_WITH_CONTRACTS_MODE
    STRING
    "Contract evaluation mode for pre-C++26 compilers, options are: ENFORCE, QUICK_ENFORCE, OBSERVE, IGNORE"
    "ENFORCE"
    STRINGS "ENFORCE;QUICK_ENFORCE;OBSERVE;IGNORE"
  )

  # Note: values start at 1 because hpx_add_config_define_namespace treats a
  # numeric "0" as falsy and would emit a valueless #define.

  # define readable contract mode constants
  hpx_add_config_define_namespace(
    DEFINE HPX_HAVE_CONTRACTS_MODE_IGNORE
    VALUE 1
    NAMESPACE CONTRACTS
  )
  hpx_add_config_define_namespace(
    DEFINE HPX_HAVE_CONTRACTS_MODE_OBSERVE
    VALUE 2
    NAMESPACE CONTRACTS
  )
  hpx_add_config_define_namespace(
    DEFINE HPX_HAVE_CONTRACTS_MODE_ENFORCE
    VALUE 3
    NAMESPACE CONTRACTS
  )
  hpx_add_config_define_namespace(
    DEFINE HPX_HAVE_CONTRACTS_MODE_QUICK_ENFORCE
    VALUE 4
    NAMESPACE CONTRACTS
  )

  if(HPX_WITH_CONTRACTS_MODE STREQUAL "ENFORCE")
    hpx_add_config_define_namespace(
      DEFINE HPX_HAVE_CONTRACTS_MODE
      VALUE HPX_HAVE_CONTRACTS_MODE_ENFORCE
      NAMESPACE CONTRACTS
    )
  elseif(HPX_WITH_CONTRACTS_MODE STREQUAL "QUICK_ENFORCE")
    hpx_add_config_define_namespace(
      DEFINE HPX_HAVE_CONTRACTS_MODE
      VALUE HPX_HAVE_CONTRACTS_MODE_QUICK_ENFORCE
      NAMESPACE CONTRACTS
    )
  elseif(HPX_WITH_CONTRACTS_MODE STREQUAL "OBSERVE")
    hpx_add_config_define_namespace(
      DEFINE HPX_HAVE_CONTRACTS_MODE
      VALUE HPX_HAVE_CONTRACTS_MODE_OBSERVE
      NAMESPACE CONTRACTS
    )
  elseif(HPX_WITH_CONTRACTS_MODE STREQUAL "IGNORE")
    hpx_add_config_define_namespace(
      DEFINE HPX_HAVE_CONTRACTS_MODE
      VALUE HPX_HAVE_CONTRACTS_MODE_IGNORE
      NAMESPACE CONTRACTS
    )
  else()
    hpx_error(
      "HPX_WITH_CONTRACTS_MODE must be one of ENFORCE, QUICK_ENFORCE, OBSERVE, or IGNORE"
    )
  endif()
endif()

# Default location is $HPX_ROOT/libs/contracts/include
set(contracts_headers hpx/contracts.hpp hpx/contracts/macros.hpp
                      hpx/contracts/violation_handler.hpp
)
set(contracts_exclude_global_header hpx/contracts.hpp)
set(contracts_macro_headers hpx/contracts/macros.hpp)

# Default location is $HPX_ROOT/libs/contracts/src
set(contracts_sources violation_handler.cpp)

include(HPX_AddModule)
add_hpx_module(
  core contracts
  GLOBAL_HEADER_GEN ON
  GLOBAL_HEADER_MODULE_GEN ON
  EXCLUDE_FROM_GLOBAL_HEADER ${contracts_exclude_global_header}
  SOURCES ${contracts_sources}
  HEADERS ${contracts_headers}
  MACRO_HEADERS ${contracts_macro_headers}
  COMPAT_HEADERS ${contracts_compat_headers}
  MODULE_DEPENDENCIES hpx_config hpx_format hpx_preprocessor hpx_assertion
  CMAKE_SUBDIRS examples tests
)
