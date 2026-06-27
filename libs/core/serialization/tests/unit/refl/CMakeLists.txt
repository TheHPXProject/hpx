# Copyright (c) 2026 Ujjwal Shekhar
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(tests
    reflection_array
    reflection_containers
    reflection_pointers
    reflection_polymorphic
    reflection_polymorphic_simple
    reflection_simple
    reflection_nested_object
    reflection_nested_templ_object
    reflection_function_names
    reflection_static_member_functions
    reflection_qualified_name_of
)

if(HPX_SERIALIZATION_WITH_ALLOW_AUTO_GENERATE)
  foreach(test ${tests})
    set(sources ${test}.cpp)

    source_group("Source Files" FILES ${sources})

    # add example executable
    add_hpx_executable(
      ${test}_test INTERNAL_FLAGS
      SOURCES ${sources} ${${test}_FLAGS}
      EXCLUDE_FROM_ALL NOLIBS
      DEPENDENCIES hpx_core
      FOLDER "Tests/Unit/Modules/Core/Serialization/Refl"
    )

    add_hpx_unit_test("modules.serialization" ${test} ${${test}_PARAMETERS})
    target_compile_definitions(${test}_test PRIVATE -DHPX_MODULE_STATIC_LINKING)

  endforeach()
endif()
