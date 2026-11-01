//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2018 Mario Suvajac
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.)

// See http://www.boost.org/libs/iostreams for documentation.

#include <hpx/hpx_main.hpp>
#include <hpx/modules/iostream.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <ios>

// Source that reads only one byte every time read() is called.
class read_one_source
{
public:
    using char_type = char;
    using category = hpx::iostream::source_tag;

    template <std::size_t N>
    explicit read_one_source(char const (&data)[N])
      : data_size_m(N)
      , data_m(data)
      , pos_m(0)
    {
    }

    std::streamsize read(char* s, std::streamsize n)
    {
        if (pos_m < data_size_m && n > 0)
        {
            *s = data_m[pos_m++];
            return 1;
        }
        else
        {
            return -1;
        }
    }

private:
    std::size_t data_size_m;
    char const* data_m;
    std::size_t pos_m;
};

void nonblocking_read_test()
{
    constexpr int data_size_k = 100;

    char data[data_size_k];
    std::copy(hpx::util::counting_iterator<char>(0),
        hpx::util::counting_iterator<char>(data_size_k), data);

    read_one_source src(data);
    hpx::iostream::non_blocking_adapter<read_one_source> nb(src);

    char read_data[data_size_k];
    std::streamsize amt = hpx::iostream::read(nb, read_data, data_size_k);

    HPX_TEST_EQ(amt, data_size_k);

    for (int i = 0; i < data_size_k; ++i)
    {
        HPX_TEST_EQ(std::char_traits<char>::to_int_type(read_data[i]), i);
    }
}

int main(int, char*[])
{
    nonblocking_read_test();
    return hpx::util::report_errors();
}
