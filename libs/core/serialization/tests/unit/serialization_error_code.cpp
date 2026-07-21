//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <system_error>
#include <vector>

void test_hpx_category()
{
    hpx::error_code const ec(
        hpx::error::bad_parameter, "some message", hpx::throwmode::plain);

    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << ec;

    hpx::serialization::input_archive iarchive(buffer);
    hpx::error_code ec2;
    iarchive >> ec2;

    auto msg1 = ec.get_message();
    auto msg2 = ec2.get_message();
    HPX_TEST_EQ(ec.value(), ec2.value());
    HPX_TEST(ec.category() == ec2.category());
    HPX_TEST_EQ(ec.get_message(), ec2.get_message());
}

void test_generic_category_large_value()
{
    hpx::error_code ec;
    static_cast<std::error_code&>(ec) =
        std::error_code(100000, std::generic_category());

    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << ec;

    hpx::serialization::input_archive iarchive(buffer);
    hpx::error_code ec2;
    iarchive >> ec2;

    HPX_TEST_EQ(ec.value(), ec2.value());
    HPX_TEST(ec.category() == ec2.category());
    HPX_TEST(ec2.category() == std::generic_category());
    HPX_TEST_EQ(ec.get_message(), ec2.get_message());
}

void test_system_category_large_value()
{
    hpx::error_code ec;
    static_cast<std::error_code&>(ec) =
        std::error_code(100001, std::system_category());

    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << ec;

    hpx::serialization::input_archive iarchive(buffer);
    hpx::error_code ec2;
    iarchive >> ec2;

    HPX_TEST_EQ(ec.value(), ec2.value());
    HPX_TEST(ec.category() == ec2.category());
    HPX_TEST(ec2.category() == std::system_category());
    HPX_TEST_EQ(ec.get_message(), ec2.get_message());
}

int main()
{
    test_hpx_category();
    test_generic_category_large_value();
    test_system_category_large_value();

    return hpx::util::report_errors();
}
