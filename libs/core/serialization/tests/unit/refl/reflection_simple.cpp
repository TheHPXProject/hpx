//  Copyright (c) 2026 Ujjwal Shekhar
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <iostream>
#include <string>
#include <vector>

// A simple class that is NOT bitwise serializable.
// We will NOT provide a serialize() function for this.
class A
{
private:
    int a;
    std::string b;
    std::vector<std::string> c;

public:
    A() = default;

    A(int a, std::string const& b, std::vector<std::string> const& c)
      : a(a)
      , b(b)
      , c(c)
    {
    }

    // getters to test
    int get_a() const
    {
        return a;
    }
    std::string get_b() const
    {
        return b;
    }
    std::vector<std::string> get_c() const
    {
        return c;
    }

    // equality operator for testing
    bool operator==(A const& rhs) const
    {
        return a == rhs.a && b == rhs.b && c == rhs.c;
    }
};

int main()
{
    using simple_test_struct = A;

    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);

    simple_test_struct input_data(42, "hello reflection", {"6", "7", "6", "7"});

    // Serialize
    // This will fail to compile if reflection is not working,
    // as it will hit the static_assert in access.hpp
    oarchive << input_data;

    // Deserialize
    hpx::serialization::input_archive iarchive(buffer);
    simple_test_struct output_data;

    iarchive >> output_data;

    HPX_TEST_EQ(input_data.get_a(), output_data.get_a());
    HPX_TEST_EQ(input_data.get_b(), output_data.get_b());
    HPX_TEST_EQ(input_data.get_c().size(), output_data.get_c().size());

    HPX_TEST(input_data == output_data);

    return hpx::util::report_errors();
}
