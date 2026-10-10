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
    default_handler_aborts
    fallback_contracts_succeed
)

if(NOT HPX_WITH_CONTRACTS)
  # Contracts are disabled: HPX_PRE / HPX_POST are no-ops (even with native
  # contracts available) and HPX_CONTRACT_ASSERT falls back to HPX_ASSERT,
  # which aborts only in Debug. Violation handlers are never called, so the
  # handler tests are not built.
  set(contract_tests ${contract_tests} disabled_contracts
                     fallback_contracts_fail
  )

  set(default_handler_aborts_PARAMETERS FAILURE_EXPECTED_DEBUG)
  set(declaration_contracts_fail_contract_assert_PARAMETERS
      FAILURE_EXPECTED_DEBUG
  )
  set(fallback_contracts_fail_PARAMETERS FAILURE_EXPECTED_DEBUG)
elseif(HPX_WITH_CXX26_CONTRACTS)
  set(contract_tests ${contract_tests} handler_receives_correct_info
                     violation_handler_invoked
  )

  # Native C++26 contracts always fail when violated, regardless of build config
  set(default_handler_aborts_PARAMETERS FAILURE_EXPECTED)
  set(declaration_contracts_fail_pre_PARAMETERS FAILURE_EXPECTED)
  set(declaration_contracts_fail_post_PARAMETERS FAILURE_EXPECTED)
  set(declaration_contracts_fail_contract_assert_PARAMETERS FAILURE_EXPECTED)
else()
  # some of the tests should run only if native contracts are not available
  set(contract_tests
      ${contract_tests} disabled_contracts fallback_contracts_fail
      handler_receives_correct_info violation_handler_invoked
  )

  # Fallback mode: HPX_PRE / HPX_POST are no-ops so fail_pre / fail_post
  # succeed.
  #
  # HPX_CONTRACT_ASSERT routes through invoke_violation_handler, whose default
  # handler aborts unless HPX_WITH_CONTRACTS_MODE=OBSERVE.
  string(FIND "${HPX_WITH_CONTRACTS_MODE}" "ENFORCE" _is_enforce)
  if(NOT (_is_enforce EQUAL -1))
    set(default_handler_aborts_PARAMETERS FAILURE_EXPECTED)
    set(fallback_contracts_fail_PARAMETERS FAILURE_EXPECTED)
    set(declaration_contracts_fail_contract_assert_PARAMETERS FAILURE_EXPECTED)
  endif()

  # violation handlers will not be called
  if(HPX_WITH_CONTRACTS_MODE STREQUAL "IGNORE")
    # HPX_CONTRACT_ASSERT falls back to HPX_ASSERT, which aborts only in Debug
    set(default_handler_aborts_PARAMETERS FAILURE_EXPECTED_DEBUG)
    set(declaration_contracts_fail_contract_assert_PARAMETERS
        FAILURE_EXPECTED_DEBUG
    )
    set(fallback_contracts_fail_PARAMETERS FAILURE_EXPECTED_DEBUG)
    set(handler_receives_correct_info_PARAMETERS FAILURE_EXPECTED)
    set(violation_handler_invoked_PARAMETERS FAILURE_EXPECTED)
  endif()

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
