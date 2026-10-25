# Copyright (c) 2024 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(cache_line_size_detect_cpp_code
    "
    #include <iostream>
    #include <new>
    int main()
    {
#if defined(HPX_HAVE_CXX17_HARDWARE_DESTRUCTIVE_INTERFERENCE_SIZE)
        std::cout << std::hardware_destructive_interference_size;
#else
#if defined(__s390__) || defined(__s390x__)
        std::cout << 256;    // assume 256 byte cache-line size
#elif defined(powerpc) || defined(__powerpc__) || defined(__ppc__)
        std::cout << 128;    // assume 128 byte cache-line size
#else
        std::cout << 64;     // assume 64 byte cache-line size
#endif
#endif
    }
"
)

function(cache_line_size output_var)
  if(NOT HPX_INTERNAL_CACHE_LINE_SIZE_DETECT)

    if(NOT CMAKE_CROSSCOMPILING)
      file(WRITE "${PROJECT_BINARY_DIR}/cache_line_size.cpp"
           "${cache_line_size_detect_cpp_code}"
      )

      if(HPX_WITH_CXX17_HARDWARE_DESTRUCTIVE_INTERFERENCE_SIZE)
        set(compile_definitions
            "-DHPX_HAVE_CXX17_HARDWARE_DESTRUCTIVE_INTERFERENCE_SIZE"
        )
      endif()

      try_run(
        run_result_unused compile_result_unused "${PROJECT_BINARY_DIR}" SOURCES
        "${PROJECT_BINARY_DIR}/cache_line_size.cpp"
        COMPILE_DEFINITIONS ${compile_definitions}
        CMAKE_FLAGS CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS
                    FALSE
        RUN_OUTPUT_VARIABLE CACHE_LINE_SIZE
      )
    endif()

    if(NOT CACHE_LINE_SIZE)
      set(CACHE_LINE_SIZE "64")
    endif()
    set(HPX_INTERNAL_CACHE_LINE_SIZE_DETECT
        ${CACHE_LINE_SIZE}
        CACHE INTERNAL ""
    )
  else()
    set(CACHE_LINE_SIZE ${HPX_INTERNAL_CACHE_LINE_SIZE_DETECT})
  endif()

  # the cache line size is used as an alignment (alignas), so it has to be a
  # power of two, written as a decimal number without leading zeros (C++ would
  # read 064 as an octal number)
  if(NOT CACHE_LINE_SIZE MATCHES "^[1-9][0-9]*$")
    hpx_error(
      "The cache line size is not a positive decimal number: ${CACHE_LINE_SIZE}"
    )
  endif()
  math(EXPR cache_line_size_mask
       "${CACHE_LINE_SIZE} & (${CACHE_LINE_SIZE} - 1)"
  )
  if(NOT cache_line_size_mask EQUAL 0)
    hpx_error(
      "The cache line size is not a power of two: ${CACHE_LINE_SIZE} (set by HPX_INTERNAL_CACHE_LINE_SIZE_DETECT)"
    )
  endif()

  set(${output_var}
      "${CACHE_LINE_SIZE}"
      PARENT_SCOPE
  )
endfunction()
