//  Copyright (c) 2026 Ujjwal Shekhar
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <iostream>
#include <string>
#include <vector>

struct person_pod
{
    int age;
    std::string name;

    bool operator==(person_pod const& rhs) const
    {
        return age == rhs.age && name == rhs.name;
    }
};

struct final_array_boss
{
    std::array<person_pod, 2> pod_array;
    std::array<std::vector<int>, 2> array_of_vectors;
    std::vector<std::array<int, 3>> vector_of_arrays;

    bool operator==(final_array_boss const& rhs) const
    {
        return pod_array == rhs.pod_array &&
            array_of_vectors == rhs.array_of_vectors &&
            vector_of_arrays == rhs.vector_of_arrays;
    }

    void print() const
    {
        std::cout << "POD Array:" << std::endl;
        for (auto const& p : pod_array)
        {
            std::cout << "Age: " << p.age << ", Name: " << p.name << std::endl;
        }

        std::cout << "Array of Vectors:" << std::endl;
        for (auto const& vec : array_of_vectors)
        {
            std::cout << "Vector: ";
            for (auto const& val : vec)
            {
                std::cout << val << " ";
            }
            std::cout << std::endl;
        }

        std::cout << "Vector of Arrays:" << std::endl;
        for (auto const& arr : vector_of_arrays)
        {
            std::cout << "Array: ";
            for (auto const& val : arr)
            {
                std::cout << val << " ";
            }
            std::cout << std::endl;
        }
    }
};

int main()
{
    final_array_boss input;
    input.pod_array = {{{25, std::string(1000, 'a')}, {30, "Bob"}}};
    input.array_of_vectors = {{{1, 2, 3}, {4, 5, 6}}};
    input.vector_of_arrays = {{{10, 20, 30}}, {{40, 50, 60}}};

    // Serialize
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << input;

    // Deserialize
    hpx::serialization::input_archive iarchive(buffer);
    final_array_boss output;
    iarchive >> output;

    // Reporting
    std::cout << "--- Array Test ---" << std::endl;
    std::cout << "POD Array Match: " << (input.pod_array == output.pod_array)
              << std::endl;
    std::cout << "Array of Vectors Match: "
              << (input.array_of_vectors == output.array_of_vectors)
              << std::endl;
    std::cout << "Vector of Arrays Match: "
              << (input.vector_of_arrays == output.vector_of_arrays)
              << std::endl;

    // Print
    input.print();
    output.print();

    HPX_TEST(input == output);

    return hpx::util::report_errors();
}
