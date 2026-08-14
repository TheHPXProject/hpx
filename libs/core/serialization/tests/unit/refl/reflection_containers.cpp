//  Copyright (c) 2026 Ujjwal Shekhar
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <iostream>
#include <string>
#include <vector>

struct person
{
    int age;
    std::string name;

    person()
      : age(0)
      , name("")
    {
    }
    person(int a, std::string n)
      : age(a)
      , name(n)
    {
    }

    bool operator==(person const& rhs) const
    {
        return age == rhs.age && name == rhs.name;
    }
};

class A
{
public:
    int a;
    std::array<person, 2> b;

    A() = default;
    A(int a, std::array<person, 2> const& b)
      : a(a)
      , b(b)
    {
    }

    bool operator==(A const& rhs) const
    {
        return a == rhs.a && b == rhs.b;
    }
};

int main()
{
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);

    // Initializing with known values
    A input_data(42, {{{10, "p1"}, {20, "p2"}}});
    oarchive << input_data;

    hpx::serialization::input_archive iarchive(buffer, buffer.size());
    A output_data;
    iarchive >> output_data;

    // Diagnostic Prints
    std::cout << "Input  m[0]: " << input_data.b[0].age << ", "
              << input_data.b[0].name << std::endl;
    std::cout << "Output m[0]: " << output_data.b[0].age << ", "
              << output_data.b[0].name << std::endl;

    HPX_TEST_EQ(input_data.a, output_data.a);
    HPX_TEST_EQ(input_data.b[0].age, output_data.b[0].age);
    HPX_TEST_EQ(input_data.b[0].name, output_data.b[0].name);

    return hpx::util::report_errors();
}
