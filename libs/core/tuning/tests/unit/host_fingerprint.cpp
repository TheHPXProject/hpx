//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/tuning.hpp>

#include <hpx/hpx_main.hpp>

#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace tuning = hpx::tuning;

void test_add_and_find()
{
    tuning::host_fingerprint f;
    f.add("cpu_model", "a");
    f.add("host_name", "h", false);
    f.add("parcel_conduits", "ucx");
    HPX_TEST_EQ(f.entries.size(), std::size_t(3));

    // the same key replaces the value in place
    f.add("cpu_model", "b");
    HPX_TEST_EQ(f.entries.size(), std::size_t(3));
    HPX_TEST_EQ(f.entries[0].key, std::string("cpu_model"));
    HPX_TEST_EQ(f.entries[0].value, std::string("b"));

    HPX_TEST(f.find("host_name") != nullptr);
    HPX_TEST(!f.find("host_name")->compared);
    HPX_TEST(f.find("missing") == nullptr);
}

void test_describe_host()
{
    tuning::host_fingerprint const f =
        tuning::describe_host(hpx::threads::get_topology());

    // the core entries, each once
    std::set<std::string> keys;
    for (tuning::fingerprint_entry const& e : f.entries)
    {
        HPX_TEST_MSG(keys.insert(e.key).second, e.key.c_str());
        // keys are single words: no dot, no space
        HPX_TEST_EQ(e.key.find_first_of(". "), std::string::npos);
    }
    for (char const* key : {"host_name", "cpu_model", "architecture", "sockets",
             "numa_domains", "cores", "pus", "os_name", "os_release",
             "os_version", "hpx_version", "hpx_commit", "hpx_build_type",
             "compiler", "stdlib"})
    {
        HPX_TEST_MSG(f.find(key) != nullptr, key);
    }

    // what every machine has
    HPX_TEST(!f.find("cores")->value.empty() && f.find("cores")->value != "0");
    HPX_TEST(!f.find("hpx_version")->value.empty());
    HPX_TEST(!f.find("compiler")->value.empty());
    // only the host's name isn't compared
    for (tuning::fingerprint_entry const& e : f.entries)
        HPX_TEST_EQ(e.compared, e.key != "host_name");
}

void test_comments_and_section()
{
    tuning::host_fingerprint f;
    f.add("host_name", "node01", false);
    f.add("cpu_model", "Some CPU ${not} $[expanded] $$ costs 5$");

    std::vector<std::string> const comments = tuning::to_comments(f);
    HPX_TEST_EQ(comments.size(), std::size_t(2));
    HPX_TEST_EQ(comments[0], std::string("fingerprint: host_name = node01"));
    // the comment lines keep the value as it is
    HPX_TEST_EQ(comments[1],
        std::string("fingerprint: cpu_model = Some CPU ${not} $[expanded] $$ "
                    "costs 5$"));

    tuning::ini_section const section = tuning::to_section(f);
    HPX_TEST_EQ(section.name, std::string("hpx.tuning.fingerprint"));
    HPX_TEST_EQ(section.settings.size(), std::size_t(2));
    HPX_TEST_EQ(section.settings[1].key,
        std::string("hpx.tuning.fingerprint.cpu_model"));
    // a '$' before '{' or '[' doubled, any other '$' as it is
    HPX_TEST_EQ(section.settings[1].value,
        std::string("Some CPU $${not} $$[expanded] $$ costs 5$"));

    std::string const ini = tuning::to_ini({section}, {}, 2026);
    HPX_TEST(ini.find("\n[hpx.tuning.fingerprint]\n") != std::string::npos);
    HPX_TEST(ini.find("\nhost_name = node01\n") != std::string::npos);
    HPX_TEST(ini.find("\ncpu_model = Some CPU $${not} $$[expanded] $$ costs "
                      "5$\n") != std::string::npos);
}

void test_mismatches()
{
    tuning::host_fingerprint profile;
    profile.add("host_name", "node01", false);
    profile.add("cpu_model", "a");
    profile.add("cores", "64");
    profile.add("parcel_conduits", "ucx");

    tuning::host_fingerprint here = profile;
    here.add("host_name", "node02", false);
    HPX_TEST(tuning::mismatches(profile, here).empty());

    here.add("cores", "32");
    here.add("hpx_commit", "abc");
    here.entries.erase(here.entries.begin() + 3);    // parcel_conduits

    std::vector<std::string> const m = tuning::mismatches(profile, here);
    HPX_TEST_EQ(m.size(), std::size_t(3));
    HPX_TEST_EQ(m[0], std::string("cores: 64, here 32"));
    HPX_TEST_EQ(m[1], std::string("hpx_commit: not in the profile, here abc"));
    HPX_TEST_EQ(m[2], std::string("parcel_conduits: ucx, not here"));
}

int main()
{
    test_add_and_find();
    test_describe_host();
    test_comments_and_section();
    test_mismatches();
    return hpx::util::report_errors();
}
