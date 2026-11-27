//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <hpx/hpx_main.hpp>

#include <limits>
#include <string>

namespace tuning = hpx::tuning;

void test_string()
{
    HPX_TEST_EQ(tuning::json_string(""), std::string("\"\""));
    HPX_TEST_EQ(tuning::json_string("Intel(R) Xeon(R)"),
        std::string("\"Intel(R) Xeon(R)\""));
    // a quote, a backslash and control characters are escaped
    HPX_TEST_EQ(tuning::json_string("a\"b\\c"), std::string(R"("a\"b\\c")"));
    HPX_TEST_EQ(
        tuning::json_string("1\n2\t3\r"), std::string(R"("1\n2\t3\r")"));
    HPX_TEST_EQ(tuning::json_string(std::string("\x01\x1f", 2)),
        std::string(R"("\u0001\u001f")"));
    // other bytes stay as they are
    HPX_TEST_EQ(tuning::json_string("\xc2\xb5s ${x}"),
        std::string("\"\xc2\xb5s ${x}\""));
}

void test_number()
{
    HPX_TEST_EQ(tuning::json_number(3.0), std::string("3"));
    HPX_TEST_EQ(tuning::json_number(0.25), std::string("0.25"));
    HPX_TEST_EQ(tuning::json_number(-1.5), std::string("-1.5"));
    HPX_TEST_EQ(tuning::json_number(0.1), std::string("0.1"));
    HPX_TEST_EQ(tuning::json_number(std::numeric_limits<double>::infinity()),
        std::string("null"));
    HPX_TEST_EQ(tuning::json_number(std::numeric_limits<double>::quiet_NaN()),
        std::string("null"));
}

void test_object_and_array()
{
    HPX_TEST_EQ(tuning::json_object({}), std::string("{}"));
    HPX_TEST_EQ(tuning::json_object({{"a", "1"}, {"b", "\"x\""}}),
        std::string(R"({"a": 1, "b": "x"})"));
    HPX_TEST_EQ(tuning::json_array({}), std::string("[]"));
    HPX_TEST_EQ(tuning::json_array({"1", "{}"}), std::string("[1, {}]"));
}

void test_summary()
{
    tuning::summary const s = tuning::summarize({5, 1, 4, 2, 3});
    HPX_TEST_EQ(tuning::to_json(s),
        std::string(R"({"count": 5, "min": 1, "q1": 2, "median": 3, )"
                    R"("q3": 4, "p90": 4.6, "max": 5})"));
}

void test_setting()
{
    tuning::tuned_setting const s{
        "hpx.min_idle_backoff_time", "50", "the \"default\"", true};
    HPX_TEST_EQ(tuning::to_json(s),
        std::string(R"({"key": "hpx.min_idle_backoff_time", "value": "50", )"
                    R"("reason": "the \"default\"", "is_default": true})"));
}

void test_fingerprint()
{
    tuning::host_fingerprint f;
    HPX_TEST_EQ(tuning::to_json(f), std::string("{}"));

    f.add("host_name", "node1", false);
    f.add("cores", "16");
    HPX_TEST_EQ(tuning::to_json(f),
        std::string(R"({"host_name": {"value": "node1", "compared": false}, )"
                    R"("cores": {"value": "16", "compared": true}})"));
}

int main()
{
    test_string();
    test_number();
    test_object_and_array();
    test_summary();
    test_setting();
    test_fingerprint();

    return hpx::util::report_errors();
}
