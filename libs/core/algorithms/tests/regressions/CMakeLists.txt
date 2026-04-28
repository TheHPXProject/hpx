#Copyright(c) 2014 - 2025 Hartmut Kaiser
#
#SPDX - License - Identifier : BSL - 1.0
#Distributed under the Boost Software License, Version 1.0.(See accompanying
#file LICENSE_1_0.txt or copy at http:    //www.boost.org/LICENSE_1_0.txt)

set(tests chunk_size_variable_regression count_3646 fill_executor_5016
        findfirstof_more_searched_for find_proxy_support
            for_each_annotated_function for_each_on_main_thread for_loop_2281
                for_loop_5735 for_loop_with_auto_chunk_size includes_empty_ranges
                    is_sorted_until_chunk_boundary minimal_findend
                        mismatch_differently_sized_ranges num_cores reduce_3641
                            scan_different_inits scan_non_commutative
                                scan_shortlength search_larger_2nd_range
                                    search_zerolength set_operations_3442
                                        stable_merge_2964 static_chunker_2282
                                            transform_inclusive_scan_4786
                                                transform_inclusive_scan_4787
                                                    ranges_facilities)

    if (HPX_WITH_DATAPAR) list(
        APPEND tests counting_iterator_datapar for_each_datapar) endif()

        foreach (test ${tests}) set(sources ${test}.cpp)

            set(${test} _PARAMETERS THREADS_PER_LOCALITY 4)

                source_group("Source Files" FILES ${sources})

#add example executable
                    add_hpx_executable(${test} _test INTERNAL_FLAGS SOURCES ${
                        sources} EXCLUDE_FROM_ALL ${${test} _FLAGS} FOLDER
                        "Tests/Regressions/Modules/Core/Algorithms/")

                        target_link_libraries(${test} _test PRIVATE
                                hpx_iterator_support_test_utilities)

                            add_hpx_regression_test("modules.algorithms" ${
                                test} ${${test} _PARAMETERS}) endforeach()
