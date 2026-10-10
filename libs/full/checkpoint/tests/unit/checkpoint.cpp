// Copyright (c) 2018 Adrian Serio
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This example tests the functionality of save_checkpoint and
// restore_checkpoint.
//

#include <hpx/hpx_main.hpp>

#include <hpx/modules/checkpoint.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

using hpx::util::checkpoint;
using hpx::util::prepare_checkpoint;
using hpx::util::restore_checkpoint;
using hpx::util::save_checkpoint;

// Main
int main()
{
    //[check_test_1
    char character = 'd';
    int integer = 10;
    float flt = 10.01f;
    bool boolean = true;
    std::string str = "I am a string of characters";
    std::vector<char> vec(str.begin(), str.end());
    checkpoint archive;

    // Test 1
    //  test basic functionality
    hpx::shared_future<checkpoint> f_archive = save_checkpoint(
        std::move(archive), character, integer, flt, boolean, str, vec);
    //]

    auto&& data = f_archive.get();

    // test restore_data_size API
    checkpoint c = prepare_checkpoint(
        hpx::launch::sync, character, integer, flt, boolean, str, vec);
    HPX_TEST(c.size() == data.size());

    //[check_test_2
    char character2;
    int integer2;
    float flt2;
    bool boolean2;
    std::string str2;
    std::vector<char> vec2;

    restore_checkpoint(data, character2, integer2, flt2, boolean2, str2, vec2);
    //]

    HPX_TEST_EQ(character, character2);
    HPX_TEST_EQ(integer, integer2);
    HPX_TEST_EQ(flt, flt2);
    HPX_TEST_EQ(boolean, boolean2);
    HPX_TEST_EQ(str, str2);
    HPX_TEST(vec == vec2);

    // Test 2
    //  test assignment operator
    char character3;
    int integer3;
    float flt3;
    bool boolean3;
    std::string str3;
    std::vector<char> vec3;
    checkpoint archive2;

    archive2 = data;
    restore_checkpoint(
        archive2, character3, integer3, flt3, boolean3, str3, vec3);

    HPX_TEST_EQ(character, character3);
    HPX_TEST_EQ(integer, integer3);
    HPX_TEST_EQ(flt, flt3);
    HPX_TEST_EQ(boolean, boolean3);
    HPX_TEST_EQ(str, str3);
    HPX_TEST(vec == vec3);

    // Test 3
    //  test move semantics
    hpx::future<checkpoint> archive3 =
        save_checkpoint(checkpoint(), vec, integer);
    restore_checkpoint(archive3.get(), vec2, integer2);

    HPX_TEST(vec == vec2);
    HPX_TEST_EQ(integer, integer2);

    // Test 4
    //  test sync policy
    std::vector<int> test_vec;
    for (int c = 0; c < 101; c++)
    {
        test_vec.push_back(c);
    }
    checkpoint archive4 =
        save_checkpoint(hpx::launch::sync, checkpoint(), test_vec);
    std::vector<int> test_vec2;
    restore_checkpoint(archive4, test_vec2);

    HPX_TEST(test_vec == test_vec2);

    checkpoint archive5(archive2);

    // Test 6
    //  test the operator= constructor
    checkpoint archive6;
    archive6 = archive5;

    HPX_TEST(archive6 == archive5);

    // Test 5
    //  test creation of a checkpoint from a checkpoint
    //  test proper handling of futures
    hpx::future<std::vector<int>> test_vec2_future =
        hpx::make_ready_future(test_vec2);
    hpx::future<checkpoint> f_check =
        save_checkpoint(std::move(archive5), test_vec2_future);
    hpx::future<std::vector<int>> test_vec3_future;
    restore_checkpoint(f_check.get(), test_vec3_future);

    HPX_TEST(test_vec2 == test_vec3_future.get());

    // Test 7
    //  test writing to a file
    //  test .begin() and .end() iterators
    //  test checkpoint(std::move(vector<char>)) constructor
    //[check_test_4
    std::ofstream test_file_7("checkpoint_test_file.txt");
    std::vector<float> vec7{1.02f, 1.03f, 1.04f, 1.05f};
    hpx::future<checkpoint> fut_7 = save_checkpoint(vec7);
    checkpoint archive7 = fut_7.get();
    std::copy(archive7.begin(),    // Write data to ofstream
        archive7.end(),            // ie. the file
        std::ostream_iterator<char>(test_file_7));
    test_file_7.close();

    std::vector<float> vec7_1;
    std::vector<char> char_vec;
    std::ifstream test_file_7_1("checkpoint_test_file.txt");
    if (test_file_7_1)
    {
        test_file_7_1.seekg(0, test_file_7_1.end);
        auto length = test_file_7_1.tellg();
        test_file_7_1.seekg(0, test_file_7_1.beg);
        char_vec.resize(length);
        test_file_7_1.read(char_vec.data(), length);
    }
    checkpoint archive7_1(std::move(char_vec));    // Write data to checkpoint
    restore_checkpoint(archive7_1, vec7_1);
    //]

    HPX_TEST(vec7 == vec7_1);

    // Test 8
    //  test policies
    int a8 = 10, b8 = 20, c8 = 30;
    hpx::future<checkpoint> f_8 =
        save_checkpoint(hpx::launch::async, a8, b8, c8);
    int a8_1, b8_1, c8_1;
    restore_checkpoint(f_8.get(), a8_1, b8_1, c8_1);
    checkpoint archive8 = save_checkpoint(hpx::launch::sync, a8_1, b8_1, c8_1);
    int a8_2, b8_2, c8_2;
    restore_checkpoint(archive8, a8_2, b8_2, c8_2);

    HPX_TEST_EQ(a8, a8_2);
    HPX_TEST_EQ(b8, b8_2);
    HPX_TEST_EQ(c8, c8_2);

    // Cleanup
    std::remove("checkpoint_test_file.txt");

    // Test 9
    //  test operator<< and operator>> overloads
    //[check_test_3
    double a9 = 1.0, b9 = 1.1, c9 = 1.2;
    std::ofstream test_file_9("test_file_9.txt");
    hpx::future<checkpoint> f_9 = save_checkpoint(a9, b9, c9);
    test_file_9 << f_9.get();
    test_file_9.close();

    double a9_1, b9_1, c9_1;
    std::ifstream test_file_9_1("test_file_9.txt");
    checkpoint archive9;
    test_file_9_1 >> archive9;
    restore_checkpoint(archive9, a9_1, b9_1, c9_1);
    //]

    HPX_TEST_EQ(a9, a9_1);
    HPX_TEST_EQ(b9, b9_1);
    HPX_TEST_EQ(c9, c9_1);

    // Cleanup
    std::remove("test_file_9.txt");

    // Test 10
    //  test checkpoint(vector<char>&) constructor
    std::ofstream test_file_10("test_file_10.txt");
    std::vector<float> vec10{1.02f, 1.03f, 1.04f, 1.05f};
    hpx::future<checkpoint> fut_10 = save_checkpoint(vec10);
    checkpoint archive10 = fut_10.get();
    std::copy(archive10.begin(),    // Write data to ofstream
        archive10.end(),            // ie. the file
        std::ostream_iterator<char>(test_file_10));
    test_file_10.close();

    std::vector<float> vec10_1;
    std::vector<char> char_vec_10;
    std::ifstream test_file_10_1("test_file_10.txt");
    if (test_file_10_1)
    {
        test_file_10_1.seekg(0, test_file_10_1.end);
        auto length = test_file_10_1.tellg();
        test_file_10_1.seekg(0, test_file_10_1.beg);
        char_vec_10.resize(length);
        test_file_10_1.read(char_vec_10.data(), length);
    }
    checkpoint archive10_1(char_vec_10);    // Write data to checkpoint
    restore_checkpoint(archive10_1, vec10_1);

    HPX_TEST(vec10 == vec10_1);

    // Cleanup
    std::remove("test_file_10.txt");

    // Test 11
    //  a length that cannot be used must not be read as one
    {
        std::ofstream test_file_11("test_file_11.txt", std::ios::binary);
        std::int64_t const bad_length = -1;
        test_file_11.write(
            reinterpret_cast<char const*>(&bad_length), sizeof(std::int64_t));
        test_file_11.close();

        std::ifstream test_file_11_1("test_file_11.txt", std::ios::binary);
        checkpoint archive11;

        bool caught = false;
        try
        {
            test_file_11_1 >> archive11;
        }
        catch (hpx::exception const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
        HPX_TEST(archive11.size() == 0);

        std::remove("test_file_11.txt");
    }

    // reading past the end of a stream is not an error, it just leaves the
    // stream in a failed state
    {
        std::ofstream test_file_12("test_file_12.txt", std::ios::binary);
        test_file_12.close();

        std::ifstream test_file_12_1("test_file_12.txt", std::ios::binary);
        checkpoint archive12;
        test_file_12_1 >> archive12;

        HPX_TEST(!test_file_12_1);
        HPX_TEST(archive12.size() == 0);

        std::remove("test_file_12.txt");
    }

    // test nullary versions of the API
    {
        hpx::future<checkpoint> f = save_checkpoint();

        auto&& cn = f.get();
        HPX_TEST(cn.size() == 0);

        cn = prepare_checkpoint(hpx::launch::sync, std::move(cn));
        HPX_TEST(cn.size() == 0);

        restore_checkpoint(cn);

        cn = prepare_checkpoint(hpx::launch::sync);
        HPX_TEST(cn.size() == 0);

        restore_checkpoint(cn);
    }

    {
        hpx::future<checkpoint> f = save_checkpoint(checkpoint{});

        auto&& cn = f.get();
        HPX_TEST(cn.size() == 0);
    }

    {
        checkpoint cn = save_checkpoint(hpx::launch::sync);
        HPX_TEST(cn.size() == 0);
    }

    {
        checkpoint cn = save_checkpoint(hpx::launch::sync, checkpoint{});
        HPX_TEST(cn.size() == 0);
    }

    return hpx::util::report_errors();
}
