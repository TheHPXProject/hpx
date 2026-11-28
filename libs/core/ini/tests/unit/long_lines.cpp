//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Long lines in an ini file, parsed on an HPX thread (as the runtime reads a
// --hpx:config file again when it loads its components): the comments are
// weeded out without recursing per character, and what comes before a '#'
// is kept.

#include <hpx/modules/ini.hpp>
#include <hpx/modules/init_runtime_local.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <string>
#include <vector>

int hpx_main()
{
    std::string const long_text(100000, 'x');

    std::vector<std::string> const lines = {
        "# " + long_text,
        "[long]",
        "# " + long_text,
        "value = " + long_text,
        "commented = before # " + long_text,
        "   # an indented comment",
        "short = 1",
        "hash = a#b",
        "#",
        "trailing = 1#",
    };

    hpx::util::section ini;
    ini.parse("<long lines>", lines, false);

    HPX_TEST(ini.has_section("long"));
    HPX_TEST_EQ(ini.get_entry("long.value", ""), long_text);
    HPX_TEST_EQ(ini.get_entry("long.commented", ""), std::string("before"));
    HPX_TEST_EQ(ini.get_entry("long.short", ""), std::string("1"));
    HPX_TEST_EQ(ini.get_entry("long.hash", ""), std::string("a"));
    HPX_TEST_EQ(ini.get_entry("long.trailing", ""), std::string("1"));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
