//  Copyright (c) 2026 Ujjwal Shekhar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// test for availability of std::experimental::meta_xxx

#include <meta>
int main()
{
    constexpr auto r = ^^int;
    typename[:r:] x = 42;
    typename[:^^char:] c = '*';

    return 0;
}
