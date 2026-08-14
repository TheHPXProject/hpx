# Copyright (c) 2026 Pratyksh Gupta
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

cmake_minimum_required(VERSION 3.18)
project(test_package CXX)

find_package(HPX REQUIRED)

add_executable(test_package test_package.cpp)
target_link_libraries(test_package HPX::hpx HPX::iostreams_component)
