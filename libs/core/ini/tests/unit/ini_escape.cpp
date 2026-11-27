//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The escapes of the expansions in ini values: "$${" and "$$[" stand for a
// literal "${" and "$[", any other "$$" is left as it is.

#include <hpx/config.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/testing.hpp>

#include <string>

// an environment variable that is not set
#define UNSET "HPX_INI_ESCAPE_TEST_UNSET_VARIABLE"

void test_expansions()
{
    hpx::util::section sec;
    sec.add_entry("x.value", "value");
    sec.add_entry("a.reference", "$[x.value]");
    sec.add_entry("a.reference_default", "$[x.missing:fallback]");
    sec.add_entry("a.environment", "${" UNSET ":fallback}");

    // the expansions themselves are unchanged
    HPX_TEST_EQ(sec.get_entry("a.reference"), std::string("value"));
    HPX_TEST_EQ(sec.get_entry("a.reference_default"), std::string("fallback"));
    HPX_TEST_EQ(sec.get_entry("a.environment"), std::string("fallback"));
}

void test_escapes()
{
    hpx::util::section sec;
    sec.add_entry("x.value", "value");
    sec.add_entry("a.brace", "$${" UNSET ":fallback}");
    sec.add_entry("a.bracket", "$$[x.value]");
    sec.add_entry("a.both", "$${HOME} and $$[x.value] and $[x.value]");

    HPX_TEST_EQ(sec.get_entry("a.brace"), std::string("${" UNSET ":fallback}"));
    HPX_TEST_EQ(sec.get_entry("a.bracket"), std::string("$[x.value]"));
    HPX_TEST_EQ(sec.get_entry("a.both"),
        std::string("${HOME} and $[x.value] and value"));

    // a default value of a missing entry is expanded the same way
    HPX_TEST_EQ(
        sec.get_entry("a.missing", "$$[x.value]"), std::string("$[x.value]"));
}

void test_other_dollars()
{
    hpx::util::section sec;
    sec.add_entry("a.pid", "echo $$");
    sec.add_entry("a.double", "$$x");
    sec.add_entry("a.trailing", "costs 5$");
    sec.add_entry("a.triple", "$$${HOME}");

    // a "$$" that isn't followed by '{' or '[' is left as it is
    HPX_TEST_EQ(sec.get_entry("a.pid"), std::string("echo $$"));
    HPX_TEST_EQ(sec.get_entry("a.double"), std::string("$$x"));
    HPX_TEST_EQ(sec.get_entry("a.trailing"), std::string("costs 5$"));
    // "$" then the escape "$${": a literal "$${"
    HPX_TEST_EQ(sec.get_entry("a.triple"), std::string("$${HOME}"));
}

void test_nested_references()
{
    hpx::util::section sec;
    sec.add_entry("x.literal", "$${" UNSET ":fallback}");
    sec.add_entry("x.literal_bracket", "$$[x.literal]");
    sec.add_entry("x.doubled", "$$${HOME}");
    sec.add_entry("a.nested", "before $[x.literal] after");
    sec.add_entry("a.nested_bracket", "$[x.literal_bracket]");
    sec.add_entry("a.nested_doubled", "$[x.doubled]");
    sec.add_entry("a.twice", "$[a.nested]");

    // the referenced value is inserted as it reads, not expanded again
    HPX_TEST_EQ(sec.get_entry("a.nested"),
        std::string("before ${" UNSET ":fallback} after"));
    HPX_TEST_EQ(sec.get_entry("a.nested_bracket"), std::string("$[x.literal]"));
    HPX_TEST_EQ(sec.get_entry("a.nested_doubled"), std::string("$${HOME}"));
    // also through two references
    HPX_TEST_EQ(sec.get_entry("a.twice"),
        std::string("before ${" UNSET ":fallback} after"));
}

void test_add_entry_self_reference()
{
    hpx::util::section sec;

    // add_entry expands a reference to the entry's previous value and keeps
    // the escapes of what it stores
    sec.add_entry("a.list", "one");
    sec.add_entry("a.list", "$[a.list] two $${HOME}");
    HPX_TEST_EQ(sec.get_entry("a.list"), std::string("one two ${HOME}"));

    sec.add_entry("a.escaped", "$${HOME}");
    sec.add_entry("a.escaped", "$[a.escaped] more");
    HPX_TEST_EQ(sec.get_entry("a.escaped"), std::string("${HOME} more"));
}

void test_expand_functions()
{
    hpx::util::section sec;
    sec.add_entry("x.value", "value");

    HPX_TEST_EQ(sec.expand("$${HOME} $[x.value] $$[x.value]"),
        std::string("${HOME} value $[x.value]"));

    std::string in_place = "$${HOME} $[x.value]";
    sec.expand(in_place, static_cast<std::string::size_type>(-1));
    HPX_TEST_EQ(in_place, std::string("${HOME} value"));

    // only after the given position
    std::string partly = "$${A} $${B}";
    sec.expand(partly, 2);
    HPX_TEST_EQ(partly, std::string("$${A} ${B}"));
}

int main()
{
    test_expansions();
    test_escapes();
    test_other_dollars();
    test_nested_references();
    test_add_entry_self_reference();
    test_expand_functions();
    return hpx::util::report_errors();
}
