//  Copyright (c) 2022 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#pragma once

#include <cassert>
#include <exception>
#include <iostream>
#include <vector>

#include <hpx/assert.hpp>

/// Fills the input vectors a and b with the predefined pattarn (a[i] = i).
/// Resets device_results to 0 just in case of reusage.
void fill_vector_add_input(std::vector<size_t>& a, std::vector<size_t>& b,
    std::vector<size_t>& device_results)
{
    HPX_ASSERT(a.size() == b.size());
    HPX_ASSERT(device_results.size() == a.size());
    for (size_t i = 0; i < a.size(); i++)
    {
        a.at(i) = i;
        b.at(i) = i;
        device_results.at(i) = 0;
    }
}
/// Compare the device_results with a sequential vector_add version. Calls
/// std::terminate and prints an error message with any differences are detected
void check_vector_add_results(std::vector<size_t> const& a,
    std::vector<size_t> const& b, std::vector<size_t> const& device_results)
{
    HPX_ASSERT(a.size() == b.size());
    HPX_ASSERT(device_result.size() == a.size());
    std::vector<size_t> add_sequential(device_results.size());
    for (size_t i = 0; i < add_sequential.size(); i++)
        add_sequential.at(i) = a.at(i) + b.at(i);
    for (size_t i = 0; i < add_sequential.size(); i++)
    {
        if (device_results.at(i) != add_sequential.at(i))
        {
            std::cerr << "Vector add failed on device.\n ";
            std::terminate();
        }
    }
    std::cout << "OKAY: Vector add results correct!\n";
}
/// Print the first and last few results of the vector add kernel results.
void print_vector_results(std::vector<size_t> const& a,
    std::vector<size_t> const& b, std::vector<size_t> const& device_results)
{
    HPX_ASSERT(a.size() == b.size());
    HPX_ASSERT(device_result.size() == a.size());
    std::cout << "Results: " << std::endl;
    for (size_t i = 0; i < 3; i++)
    {
        std::cout << "[" << i << "]: " << a[i] << " + " << b[i] << " = "
                  << device_results[i] << "\n";
    }
    std::cout << "...\n";
    for (size_t i = 3; i > 0; i--)
    {
        std::cout << "[" << device_results.size() - i
                  << "]: " << a[device_results.size() - i] << " + "
                  << b[device_results.size() - i] << " = "
                  << device_results[device_results.size() - i] << "\n";
    }
}
