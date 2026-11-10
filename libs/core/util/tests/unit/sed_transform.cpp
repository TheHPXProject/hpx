//  Copyright (c) 2026 Vaatsalya Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/testing.hpp>
#include <hpx/modules/util.hpp>

#include <exception>
#include <string>

namespace {

    // a well-formed expression has to be split into its search and replace
    // parts
    void test_valid(std::string const& input,
        std::string const& expected_search, std::string const& expected_replace)
    {
        std::string search;
        std::string replace;
        HPX_TEST_MSG(hpx::util::parse_sed_expression(input, search, replace),
            input.c_str());
        HPX_TEST_EQ_MSG(search, expected_search, input.c_str());
        HPX_TEST_EQ_MSG(replace, expected_replace, input.c_str());
    }

    // a malformed expression has to be rejected by returning false, it must
    // never throw
    void test_invalid(std::string const& input)
    {
        try
        {
            std::string search;
            std::string replace;
            HPX_TEST_MSG(
                !hpx::util::parse_sed_expression(input, search, replace),
                input.c_str());

            // the transform constructed from it has to be empty
            hpx::util::sed_transform const transform(input);
            HPX_TEST_MSG(!transform, input.c_str());
        }
        catch (std::exception const&)
        {
            // report which input threw
            HPX_TEST_MSG(false, input.c_str());
        }
    }
}    // namespace

int main()
{
    // well-formed expressions
    test_valid("s/abc/def/", "abc", "def");
    test_valid("s/abc/def", "abc", "def");
    test_valid("s/a/b", "a", "b");
    test_valid("s/abc/", "abc", "");
    test_valid("s/a/", "a", "");
    test_valid(R"(s/a\/b/c/)", R"(a\/b)", "c");

    // the search part is never closed by a '/'
    test_invalid("s/a");
    test_invalid("s/abc");

    // other malformed expressions
    test_invalid("");
    test_invalid("s");
    test_invalid("s/");
    test_invalid("x/a/b/");
    test_invalid(R"(s/a\)");
    test_invalid(R"(s/\/)");

    // a valid transform is applied to its input
    {
        hpx::util::sed_transform const transform("s/a/x/");
        HPX_TEST(static_cast<bool>(transform));
        HPX_TEST_EQ(transform("abc"), std::string("xbc"));
    }

    return hpx::util::report_errors();
}
