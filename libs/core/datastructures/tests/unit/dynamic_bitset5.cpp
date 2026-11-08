//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// This code was adapted from boost dynamic_bitset
//
// Copyright (c) 2001 Jeremy Siek
// Copyright (c) 2003-2006 Gennaro Prota
// Copyright (c) 2015 Seth Heeren

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <vector>

template <typename Block>
struct SerializableType
{
    hpx::detail::dynamic_bitset<Block> x;

private:
    friend class hpx::serialization::access;

    template <class Archive>
    void serialize(Archive& ar, unsigned int const)
    {
        // clang-format off
        ar & x;
        // clang-format on
    }
};

template <typename Block>
void test_serialization()
{
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);

    SerializableType<Block> ao;
    for (int i = 0; i < 128; ++i)
    {
        ao.x.resize(11 * i, i % 2);
    }
    oarchive << ao;

    hpx::serialization::input_archive iarchive(buffer);
    SerializableType<Block> ai;
    iarchive >> ai;

    HPX_TEST_EQ(ao.x, ai.x);
}

template <typename Block>
void run_test_cases()
{
    test_serialization<Block>();
}

int main()
{
    run_test_cases<unsigned char>();
    run_test_cases<unsigned short>();
    run_test_cases<unsigned int>();
    run_test_cases<unsigned long>();
    run_test_cases<unsigned long long>();

    return hpx::util::report_errors();
}
