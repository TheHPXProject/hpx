# Copyright (c) 2026 the-ivii
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.18)
project(hpx_godbolt_minimal_smoketest CXX)

find_package(HPX REQUIRED)

add_executable(hello_world hello_world.cpp)
target_link_libraries(hello_world PRIVATE HPX::hpx)

add_executable(hello_wrap hello_wrap.cpp)
target_link_libraries(hello_wrap PRIVATE HPX::hpx HPX::wrap_main)
