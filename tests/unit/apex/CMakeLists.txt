#  Copyright (c) 2015 University of Oregon
#
# SPDX-License-Identifier: BSL-1.0
#  Distributed under the Boost Software License, Version 1.0. (See accompanying
#  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# We disable the annotation_check test as starting with APEX V2.5.0 the output
# is sorted by execution time and not by name.
#
# APEX might add an option to control the format of the generated output in the
# future, we should re-enable the test at that point.

set(tests apex_local_run)

if(HPX_WITH_DISTRIBUTED_RUNTIME)
  set(tests ${tests} apex_action_count)
endif()

foreach(test ${tests})
  set(sources ${test}.cpp)

  source_group("Source Files" FILES ${sources})

  # add example executable
  add_hpx_executable(
    ${test}_test INTERNAL_FLAGS
    SOURCES ${sources} ${${test}_FLAGS}
    EXCLUDE_FROM_ALL
    HPX_PREFIX ${HPX_BUILD_PREFIX}
    FOLDER "Tests/Unit/Apex" ${HPX_WITH_CXX_MODULES_OPTION}
  )

  add_hpx_unit_test("apex" ${test} ${${test}_PARAMETERS})

endforeach()

# cmake-format: off

#set(launch_types_
#    " "
#    "Executor "
#    "apply "
#    "async "
#    "deferred "
#    "fork "
#    "sync "
#)
#
## dataflow
#foreach(launch ${launch_types_})
#  set(REGEX_MATCH_D_ "${REGEX_MATCH_D_}.*1-${launch}Dataflow")
#endforeach()
#
## continuations
#foreach(launch ${launch_types_})
#  set(REGEX_MATCH_C_ "${REGEX_MATCH_C_}.*2-${launch}Continuation")
#endforeach()
#
## continuations with unwrapping
#foreach(launch ${launch_types_})
#  set(REGEX_MATCH_C_ "${REGEX_MATCH_C_}.*3-${launch}Unwrapping Continuation")
#endforeach()
#
## scoped_annotation with std::string and char*
#foreach(type "char" "string")
#  set(REGEX_MATCH_ANN_ "${REGEX_MATCH_ANN_}.*4-${type} scoped_annotation")
#endforeach()
#
#set_tests_properties(
#  tests.unit.apex.annotation_check
#  PROPERTIES PASS_REGULAR_EXPRESSION
#             "${REGEX_MATCH_D_}${REGEX_MATCH_C_}${REGEX_MATCH_ANN_}"
#)

# cmake-format: on
