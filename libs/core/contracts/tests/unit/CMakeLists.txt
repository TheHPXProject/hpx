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

foreach(test ${contract_tests})
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${test}.cpp
    NOLIBS
    DEPENDENCIES hpx_core
    EXCLUDE_FROM_ALL
    FOLDER "Tests/Unit/Modules/Core/Contracts/"
  )
  add_hpx_unit_test("modules.contracts" ${test})
endforeach()

if(HPX_HAVE_CXX26_CONTRACTS)
  # Native C++26 contracts always fail when violated, regardless of build config
  set_tests_properties(
    tests.unit.modules.contracts.declaration_contracts_fail_pre
    PROPERTIES WILL_FAIL ON
  )
  set_tests_properties(
    tests.unit.modules.contracts.declaration_contracts_fail_post
    PROPERTIES WILL_FAIL ON
  )
  # HPX_CONTRACT_ASSERT still routes through HPX_ASSERT in native mode — Debug
  # only
  set_tests_properties(
    tests.unit.modules.contracts.declaration_contracts_fail_contract_assert
    PROPERTIES WILL_FAIL $<$<CONFIG:Debug>:ON>
  )
else()
  # Fallback mode: HPX_PRE / HPX_POST are no-ops so fail_pre / fail_post
  # succeed. HPX_CONTRACT_ASSERT routes through invoke_violation_handler, whose
  # default handler aborts unless HPX_WITH_CONTRACTS_MODE=OBSERVE.
  if(NOT HPX_WITH_CONTRACTS_MODE STREQUAL "OBSERVE"
     AND NOT HPX_WITH_CONTRACTS_MODE STREQUAL "IGNORE"
  )
    set_tests_properties(
      tests.unit.modules.contracts.declaration_contracts_fail_contract_assert
      PROPERTIES WILL_FAIL ON
    )
    set_tests_properties(
      tests.unit.modules.contracts.default_handler_aborts PROPERTIES WILL_FAIL
                                                                     ON
    )
  endif()

  # fallback_contracts_fail uses HPX_ASSERT directly, so it only fails in Debug
  set_tests_properties(
    tests.unit.modules.contracts.fallback_contracts_fail
    PROPERTIES WILL_FAIL $<$<CONFIG:Debug>:ON>
  )
endif()
