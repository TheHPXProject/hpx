//  Copyright (c) 2026 Nick Derise
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// test for availability of std::filesystem::path::display_string()

#include <filesystem>

int main()
{
    std::filesystem::path p;
    (void) p.display_string();
    return 0;
}
