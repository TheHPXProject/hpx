//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <meta>
struct marker
{
};
[[= marker{}]] int f()
{
    return 0;
}
consteval bool has_annotation()
{
    for (auto a : std::meta::annotations_of(^^f))
        return true;
    return false;
}
static_assert(
    has_annotation(), "std::meta::annotations_of must report [[=marker{}]]");

int main()
{
    return 0;
}
