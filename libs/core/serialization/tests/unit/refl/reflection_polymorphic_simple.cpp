//  Copyright (c) 2026 Ujjwal Shekhar
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>
#include <iostream>
#include <memory>
#include <meta>
#include <string>
#include <vector>

struct Base
{
    int base_val = 100;
    virtual ~Base() = default;
};
HPX_POLYMORPHIC_AUTO_REGISTER(Base)

class DerivedA : public Base
{
private:
    int private_a = 42;
    std::string secret_msg = "shhh";

public:
    void set_data(int a, std::string s)
    {
        private_a = a;
        secret_msg = s;
    }
    int get_a() const
    {
        return private_a;
    }
    std::string get_msg() const
    {
        return secret_msg;
    }
};
HPX_POLYMORPHIC_AUTO_REGISTER(DerivedA)

class DerivedB : public Base
{
private:
    double private_b = 3.14;

public:
    void set_b(double b)
    {
        private_b = b;
    }
    double get_b() const
    {
        return private_b;
    }
};
HPX_POLYMORPHIC_AUTO_REGISTER(DerivedB)

int main()
{
    std::vector<char> buffer;

    {
        std::unique_ptr<Base> input = std::make_unique<DerivedA>();
        static_cast<DerivedA*>(input.get())->set_data(99, "reflection_works");

        {
            hpx::serialization::output_archive oarchive(buffer);
            oarchive << input;
        }

        std::unique_ptr<Base> output;
        {
            hpx::serialization::input_archive iarchive(buffer);
            iarchive >> output;
        }

        HPX_TEST(output != nullptr);
        auto* check = dynamic_cast<DerivedA*>(output.get());
        HPX_TEST(check != nullptr);
        if (check)
        {
            HPX_TEST_EQ(check->get_a(), 99);
            HPX_TEST_EQ(check->get_msg(), "reflection_works");
        }
    }

    return hpx::util::report_errors();
}
