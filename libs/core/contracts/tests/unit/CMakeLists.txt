# Copyright (c) 2025 Alexandros Papadakis
# Copyright (c) 2026 The STE||AR-Group
# HPX Contracts Module Unit Tests
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Declaration contracts tests (C++26 declaration syntax) These run when
# __cpp_contracts is available
set(contract_tests
    declaration_contracts_succeed
    declaration_contracts_fail_pre
    declaration_contracts_fail_post
    declaration_contracts_fail_contract_assert
    fallback_contracts_succeed
    fallback_contracts_fail
    disabled_contracts
    violation_handler_invoked
    default_handler_aborts
    handler_receives_correct_info
)

if(HPX_HAVE_CXX26_CONTRACTS)
  # Native C++26 contracts always fail when violated, regardless of build config
  set(declaration_contracts_fail_pre_FLAGS FAILURE_EXPECTED)
  set(declaration_contracts_fail_post_FLAGS FAILURE_EXPECTED)
  set(declaration_contracts_fail_contract_assert_FLAGS
      $<$<CONFIG:Debug>:FAILURE_EXPECTED>
  )
else()
  # Fallback mode: HPX_PRE / HPX_POST are no-ops so fail_pre / fail_post
  # succeed. HPX_CONTRACT_ASSERT routes through invoke_violation_handler, whose
  # default handler aborts unless HPX_WITH_CONTRACTS_MODE=OBSERVE.
  if(NOT HPX_WITH_CONTRACTS_MODE STREQUAL "OBSERVE"
     AND NOT HPX_WITH_CONTRACTS_MODE STREQUAL "IGNORE"
  )
    set(declaration_contracts_fail_contract_assert_FLAGS FAILURE_EXPECTED)
    set(default_handler_aborts_assert_FLAGS FAILURE_EXPECTED)
  endif()

  # fallback_contracts_fail uses HPX_ASSERT directly, so it only fails in Debug
  set(fallback_contracts_fail_PARAMETERS FAILURE_EXPECTED)
endif()

foreach(test ${contract_tests})
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${test}.cpp
    NOLIBS
    DEPENDENCIES hpx_core
    EXCLUDE_FROM_ALL
    FOLDER "Tests/Unit/Modules/Core/Contracts/"
  )
  add_hpx_unit_test("modules.contracts" ${test} ${${test}_PARAMETERS})
endforeach()
