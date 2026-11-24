//  Copyright (c) 2026 Vaatsalya Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/util.hpp>

#include <string>

namespace {
    // An invalid pattern must be reported through the error_code and
    // must not produce a result.
    void test_invalid_pattern(std::string const& pattern)
    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        std::string const result = hpx::util::regex_from_pattern(pattern, ec);

        HPX_TEST(ec);
        HPX_TEST_EQ(ec.value(), static_cast<int>(hpx::error::bad_parameter));
        HPX_TEST(result.empty());
    }

    void test_valid_pattern(
        std::string const& pattern, std::string const& expected_regex)
    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        std::string const result = hpx::util::regex_from_pattern(pattern, ec);

        HPX_TEST(!ec);
        HPX_TEST_EQ(result, expected_regex);
    }
}    // namespace

int main()
{
    // character set that is never closed because the pattern ends right
    // after the opening bracket
    test_invalid_pattern("[");
    test_invalid_pattern("foo[");

    // other unterminated or empty character sets
    test_invalid_pattern("[a");
    test_invalid_pattern("[!");
    test_invalid_pattern("[]");

    // a throwing call must throw bad_parameter
    HPX_TEST_THROW(hpx::util::regex_from_pattern("["), hpx::exception);

    // well-formed patterns keep working
    test_valid_pattern("[abc]", "[abc]");
    test_valid_pattern("[!abc]", "[^abc]");
    test_valid_pattern("foo*", "foo.*");

    return hpx::util::report_errors();
}
