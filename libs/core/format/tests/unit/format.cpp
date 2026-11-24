//  Copyright (c) 2018 Agustin Berge
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/hpx_main.hpp>

#include <ctime>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

    // a replacement field that is never closed has to be rejected the same way
    // a stray '}' is, with a std::runtime_error saying 'bad format string'
    template <typename... Ts>
    void test_bad_format(char const* format_str, Ts const&... ts)
    {
        try
        {
            (void) hpx::util::format(format_str, ts...);
            HPX_TEST_MSG(false, format_str);    // must not return normally
        }
        catch (std::runtime_error const& e)
        {
            HPX_TEST_EQ_MSG(std::string(e.what()),
                std::string("bad format string"), format_str);
        }
    }
}    // namespace

int main(int, char*[])
{
    {
        HPX_TEST_EQ((hpx::util::format("Hello")), "Hello");
        HPX_TEST_EQ(
            (hpx::util::format("Hello, {}!", "world")), "Hello, world!");
        HPX_TEST_EQ(
            (hpx::util::format("The number is {}", 1)), "The number is 1");
    }

    {
        HPX_TEST_EQ((hpx::util::format("{} {}", 1, 2)), "1 2");
        HPX_TEST_EQ((hpx::util::format("{} {1}", 1, 2)), "1 1");
        HPX_TEST_EQ((hpx::util::format("{2} {}", 1, 2)), "2 2");
        HPX_TEST_EQ((hpx::util::format("{2} {1}", 1, 2)), "2 1");

        HPX_TEST_EQ((hpx::util::format("{:}", 42)), "42");
        HPX_TEST_EQ((hpx::util::format("{:04}", 42)), "0042");
        HPX_TEST_EQ((hpx::util::format("{2:04}", 42, 43)), "0043");

        HPX_TEST_EQ((hpx::util::format("{:x}", 42)), "2a");
        HPX_TEST_EQ((hpx::util::format("{:04x}", 42)), "002a");
        HPX_TEST_EQ((hpx::util::format("{2:04x}", 42, 43)), "002b");

        HPX_TEST_EQ((hpx::util::format("{:#x}", 42)), "0x2a");
        HPX_TEST_EQ((hpx::util::format("{:#06x}", 42)), "0x002a");
        HPX_TEST_EQ((hpx::util::format("{2:#06x}", 42, 43)), "0x002b");
    }

    {
        HPX_TEST_EQ((hpx::util::format("{} {}", true, false)), "1 0");
    }

    {
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        char buffer[64] = {};
        std::strftime(buffer, 64, "%c", &tm);
        HPX_TEST_EQ((hpx::util::format("{}", tm)), buffer);

        std::strftime(buffer, 64, "%A %c", &tm);
        HPX_TEST_EQ((hpx::util::format("{:%A %c}", tm)), buffer);
    }

    {
        using hpx::util::format_join;
        std::vector<int> const vs = {42, 43};
        HPX_TEST_EQ((hpx::util::format("{}", format_join(vs, ""))), "4243");
        HPX_TEST_EQ((hpx::util::format("{}", format_join(vs, ","))), "42,43");
        HPX_TEST_EQ((hpx::util::format("{:x}", format_join(vs, ""))), "2a2b");
        HPX_TEST_EQ(
            (hpx::util::format("{:04x}", format_join(vs, ","))), "002a,002b");
    }

    {
        HPX_TEST_EQ((hpx::util::format("{{ {}", 1)), "{ 1");
        HPX_TEST_EQ((hpx::util::format("}} {}", 1)), "} 1");
        HPX_TEST_EQ((hpx::util::format("{{}} {}", 1)), "{} 1");
        HPX_TEST_EQ((hpx::util::format("{} {{}}", 1)), "1 {}");
        HPX_TEST_EQ((hpx::util::format("{} {{", 1)), "1 {");
        HPX_TEST_EQ((hpx::util::format("{} }}", 1)), "1 }");
        HPX_TEST_EQ((hpx::util::format("{{{1}}}", 2)), "{2}");
    }

    {
        // replacement fields that are never closed by a '}'; the ones with
        // an explicit argument index used to never return
        test_bad_format("{", 42);
        test_bad_format("abc {", 42);
        test_bad_format("{:x", 42);
        test_bad_format("{1", 42);
        test_bad_format("abc {1", 42);
        test_bad_format("{2:04", 42, 43);

        // well-formed fields next to an unterminated one are still rejected
        test_bad_format("{} {", 42);
        test_bad_format("{1} {1", 42);

        // a single brace at the end of the string
        test_bad_format("}", 42);
        test_bad_format("abc }", 42);
    }

    return hpx::util::report_errors();
}
