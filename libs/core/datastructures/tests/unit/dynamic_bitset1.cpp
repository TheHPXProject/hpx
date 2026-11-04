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
// Copyright (c) 2014 Ahmed Charles
// Copyright (c) 2014 Riccardo Marcangelo
// Copyright (c) 2014 Glen Joseph Fernandes (glenjofe@gmail.com)

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>

#include "bitset_test.hpp"

#include <cstddef>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

template <typename T>
class minimal_allocator
{
public:
    using value_type = T;

    minimal_allocator() = default;

    template <typename U>
    minimal_allocator(minimal_allocator<U> const&)
    {
    }

    T* allocate(std::size_t n)
    {
        void* p = std::malloc(sizeof(T) * n);
        if (!p)
        {
            throw std::bad_alloc();
        }
        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t)
    {
        std::free(p);
    }
};

#define HPX_BITSET_TEST_COUNT(x) (sizeof(x) / sizeof(x[0]))

template <typename Tests, typename String>
void run_string_tests(String const& s)
{
    std::size_t const len = s.length();
    std::size_t const step = len / 4 ? len / 4 : 1;

    // bitset length determined by the string-related arguments
    std::size_t i;
    for (i = 0; i <= len / 2; i += step)
    {
        Tests::from_string(s, i, len / 2);        // len/2 - i bits
        Tests::from_string(s, i, len);            // len - i   bits
        Tests::from_string(s, i, 1 + len * 2);    // len - i   bits
    }

    // bitset length explicitly specified
    for (i = 0; i <= len / 2; i += step)
    {
        for (std::size_t sz = 0; sz <= len * 4; sz += step * 2)
        {
            Tests::from_string(s, i, len / 2, sz);
            Tests::from_string(s, i, len, sz);
            Tests::from_string(s, i, 1 + len * 2, sz);
        }
    }
}

// tests the do-the-right-thing constructor dispatch
template <typename Tests, typename T>
void run_numeric_ctor_tests()
{
    int const bits_per_block = Tests::bits_per_block;
    int const width = std::numeric_limits<T>::digits;
    T const ma = (std::numeric_limits<T>::max)();
    T const mi = (std::numeric_limits<T>::min)();

    int sizes[] = {0, 7 * width / 10, width, 13 * width / 10, 3 * width,
        7 * bits_per_block / 10, bits_per_block, 13 * bits_per_block / 10,
        3 * bits_per_block};

    T const numbers[] = {T(-1), T(-3), T(-8), T(-15), T(mi / 2), T(mi), T(0),
        T(1), T(3), T(8), T(15), T(ma / 2), T(ma)};

    for (std::size_t s = 0; s < HPX_BITSET_TEST_COUNT(sizes); ++s)
    {
        for (std::size_t n = 0; n < HPX_BITSET_TEST_COUNT(numbers); ++n)
        {
            // can match ctor from ulong or templated one
            Tests::from_unsigned_long(sizes[s], numbers[n]);

            typedef std::size_t compare_type;
            compare_type const sz = sizes[s];

            // this condition is to be sure that size is representable in T, so
            // that for signed T's we avoid implementation-defined behavior [if
            // ma is larger than what std::size_t can hold then this is ok for
            // our purposes: our sizes are anyhow < max(size_t)], which in turn
            // could make the first argument of from_unsigned_long() a small
            // negative, later converted to a very large unsigned. Example:
            // signed 8-bit char (CHAR_MAX=127), bits_per_block=64, sz = 192 >
            // 127.
            bool const fits = sz <= static_cast<compare_type>(ma);

            if (fits)
            {
                // can match templated ctor only (so we test dispatching)
                Tests::from_unsigned_long(static_cast<T>(sizes[s]), numbers[n]);
            }
        }
    }
}

template <typename Block>
void run_test_cases()
{
    typedef hpx::detail::dynamic_bitset<Block> bitset_type;
    typedef bitset_test<bitset_type> Tests;
    int const bits_per_block = bitset_type::bits_per_block;

    std::string const long_string = get_long_string();
    Block const all_1s = static_cast<Block>(-1);

    //=====================================================================
    // Test construction from unsigned long
    {
        // NOTE:
        //
        // 1. keep this in sync with the numeric types supported
        //    for constructor dispatch (of course)
        // 2. bool is tested separately; ugly and inelegant, but
        //    we don't have much time to think of a better solution
        //    which is likely to work on broken compilers
        //
        int const sizes[] = {0, 1, 3, 7 * bits_per_block / 10, bits_per_block,
            13 * bits_per_block / 10, 3 * bits_per_block};

        bool const values[] = {false, true};

        for (std::size_t s = 0; s < HPX_BITSET_TEST_COUNT(sizes); ++s)
        {
            for (std::size_t v = 0; v < HPX_BITSET_TEST_COUNT(values); ++v)
            {
                Tests::from_unsigned_long(sizes[s], values[v]);
                Tests::from_unsigned_long(sizes[s] != 0, values[v]);
            }
        }

        run_numeric_ctor_tests<Tests, char>();
        run_numeric_ctor_tests<Tests, signed char>();
        run_numeric_ctor_tests<Tests, short int>();
        run_numeric_ctor_tests<Tests, int>();
        run_numeric_ctor_tests<Tests, long int>();

        run_numeric_ctor_tests<Tests, unsigned char>();
        run_numeric_ctor_tests<Tests, unsigned short>();
        run_numeric_ctor_tests<Tests, unsigned int>();
        run_numeric_ctor_tests<Tests, unsigned long>();

        run_numeric_ctor_tests<Tests, long long>();
        run_numeric_ctor_tests<Tests, unsigned long long>();
    }
    //=====================================================================
    // Test construction from a string
    {
        run_string_tests<Tests>(std::string(""));    // empty string
        run_string_tests<Tests>(std::string("1"));

        run_string_tests<Tests>(long_string);

        // Note that these are _valid_ arguments
        Tests::from_string(std::string("x11y"), 1, 2);
        Tests::from_string(std::string("x11"), 1, 10);
        Tests::from_string(std::string("x11"), 1, 10, 10);
    }
    //=====================================================================
    // test from_block_range
    {
        std::vector<Block> blocks;
        Tests::from_block_range(blocks);
    }
    {
        std::vector<Block> blocks(3);
        blocks[0] = static_cast<Block>(0);
        blocks[1] = static_cast<Block>(1);
        blocks[2] = all_1s;
        Tests::from_block_range(blocks);
    }
    {
        unsigned int const n = (std::numeric_limits<unsigned char>::max)();
        std::vector<Block> blocks(n);
        for (typename std::vector<Block>::size_type i = 0; i < n; ++i)
            blocks[i] = static_cast<Block>(i);
        Tests::from_block_range(blocks);
    }
    //=====================================================================
    // test to_block_range
    {
        bitset_type b;
        Tests::to_block_range(b);
    }
    {
        bitset_type b(1, 1ul);
        Tests::to_block_range(b);
    }
    {
        bitset_type b(long_string);
        Tests::to_block_range(b);
    }

    //=====================================================================
    // Test copy constructor
    {
        hpx::detail::dynamic_bitset<Block> b;
        Tests::copy_constructor(b);
    }
    {
        hpx::detail::dynamic_bitset<Block> b(std::string("0"));
        Tests::copy_constructor(b);
    }
    {
        hpx::detail::dynamic_bitset<Block> b(long_string);
        Tests::copy_constructor(b);
    }
    //=====================================================================
    // Test copy assignment operator
    {
        bitset_type a, b;
        Tests::copy_assignment_operator(a, b);
    }
    {
        bitset_type a(std::string("1")), b(std::string("0"));
        Tests::copy_assignment_operator(a, b);
    }
    {
        bitset_type a(long_string), b(long_string);
        Tests::copy_assignment_operator(a, b);
    }
    {
        bitset_type a;
        bitset_type b(long_string);    // b greater than a, a empty
        Tests::copy_assignment_operator(a, b);
    }
    {
        bitset_type a(std::string("0"));
        bitset_type b(long_string);    // b greater than a
        Tests::copy_assignment_operator(a, b);
    }

    //=====================================================================
    // Test move constructor
    {
        hpx::detail::dynamic_bitset<Block> b;
        Tests::move_constructor(b);
    }
    {
        hpx::detail::dynamic_bitset<Block> b(std::string("0"));
        Tests::move_constructor(b);
    }
    {
        hpx::detail::dynamic_bitset<Block> b(long_string);
        Tests::move_constructor(b);
    }
    //=====================================================================
    // Test move assignment operator
    {
        bitset_type a, b;
        Tests::move_assignment_operator(a, b);
    }
    {
        bitset_type a(std::string("1")), b(std::string("0"));
        Tests::move_assignment_operator(a, b);
    }
    {
        bitset_type a(long_string), b(long_string);
        Tests::move_assignment_operator(a, b);
    }
    {
        bitset_type a;
        bitset_type b(long_string);    // b greater than a, a empty
        Tests::move_assignment_operator(a, b);
    }
    {
        bitset_type a(std::string("0"));
        bitset_type b(long_string);    // b greater than a
        Tests::move_assignment_operator(a, b);
    }

    //=====================================================================
    // Test swap
    {
        bitset_type a;
        bitset_type b(std::string("1"));
        Tests::swap(a, b);
        Tests::swap(b, a);
        Tests::swap(a, a);
    }
    {
        bitset_type a;
        bitset_type b(long_string);
        Tests::swap(a, b);
        Tests::swap(b, a);
    }
    {
        bitset_type a(std::string("0"));
        bitset_type b(long_string);
        Tests::swap(a, b);
        Tests::swap(b, a);
        Tests::swap(a, a);
        Tests::swap(b, b);
    }
    //=====================================================================
    // Test resize
    {
        hpx::detail::dynamic_bitset<Block> a;
        Tests::resize(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("0"));
        Tests::resize(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("1"));
        Tests::resize(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(long_string);
        Tests::resize(a);
    }
    //=====================================================================
    // Test clear
    {
        hpx::detail::dynamic_bitset<Block> a;
        Tests::clear(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(long_string);
        Tests::clear(a);
    }
    //=====================================================================
    // Test pop back
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("01"));
        Tests::pop_back(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("10"));
        Tests::pop_back(a);
    }
    {
        int const size_to_fill_all_blocks = 4 * bits_per_block;
        hpx::detail::dynamic_bitset<Block> a(size_to_fill_all_blocks, 255ul);
        Tests::pop_back(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(long_string);
        Tests::pop_back(a);
    }
    //=====================================================================
    // Test append bit
    {
        hpx::detail::dynamic_bitset<Block> a;
        Tests::append_bit(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("0"));
        Tests::append_bit(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("1"));
        Tests::append_bit(a);
    }
    {
        int const size_to_fill_all_blocks = 4 * bits_per_block;
        hpx::detail::dynamic_bitset<Block> a(size_to_fill_all_blocks, 255ul);
        Tests::append_bit(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(long_string);
        Tests::append_bit(a);
    }
    //=====================================================================
    // Test append block
    {
        hpx::detail::dynamic_bitset<Block> a;
        Tests::append_block(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("0"));
        Tests::append_block(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("1"));
        Tests::append_block(a);
    }
    {
        int const size_to_fill_all_blocks = 4 * bits_per_block;
        hpx::detail::dynamic_bitset<Block> a(size_to_fill_all_blocks, 15ul);
        Tests::append_block(a);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(long_string);
        Tests::append_block(a);
    }
    //=====================================================================
    // Test append block range
    {
        hpx::detail::dynamic_bitset<Block> a;
        std::vector<Block> blocks;
        Tests::append_block_range(a, blocks);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("0"));
        std::vector<Block> blocks(3);
        blocks[0] = static_cast<Block>(0);
        blocks[1] = static_cast<Block>(1);
        blocks[2] = all_1s;
        Tests::append_block_range(a, blocks);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(std::string("1"));
        unsigned int const n = (std::numeric_limits<unsigned char>::max)();
        std::vector<Block> blocks(n);
        for (typename std::vector<Block>::size_type i = 0; i < n; ++i)
            blocks[i] = static_cast<Block>(i);
        Tests::append_block_range(a, blocks);
    }
    {
        hpx::detail::dynamic_bitset<Block> a;
        a.append(Block(1));
        a.append(Block(2));
        Block x[] = {3, 4, 5};
        std::size_t sz = sizeof(x) / sizeof(x[0]);
        std::vector<Block> blocks(x, x + sz);
        Tests::append_block_range(a, blocks);
    }
    {
        hpx::detail::dynamic_bitset<Block> a(long_string);
        std::vector<Block> blocks(3);
        blocks[0] = static_cast<Block>(0);
        blocks[1] = static_cast<Block>(1);
        blocks[2] = all_1s;
        Tests::append_block_range(a, blocks);
    }
    //=====================================================================
    // Test bracket operator
    {
        hpx::detail::dynamic_bitset<Block> b1;
        std::vector<bool> bitvec1;
        Tests::operator_bracket(b1, bitvec1);
    }
    {
        hpx::detail::dynamic_bitset<Block> b(std::string("1"));
        std::vector<bool> bit_vec(1, true);
        Tests::operator_bracket(b, bit_vec);
    }
    {
        hpx::detail::dynamic_bitset<Block> b(long_string);
        std::size_t n = long_string.size();
        std::vector<bool> bit_vec(n);
        for (std::size_t i = 0; i < n; ++i)
            bit_vec[i] = long_string[n - 1 - i] == '0' ? 0 : 1;
        Tests::operator_bracket(b, bit_vec);
    }
    {
        typedef hpx::detail::dynamic_bitset<Block, minimal_allocator<Block>>
            Bitset;
        Bitset b;
        bitset_test<Bitset>::max_size(b);
    }
    // Test copy-initialize with default constructor
    {
        hpx::detail::dynamic_bitset<Block> b[1] = {};
        (void) b;
    }
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
