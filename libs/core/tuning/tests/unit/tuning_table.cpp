//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <hpx/hpx_main.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace tuning = hpx::tuning;

void test_section_name()
{
    tuning::tuning_row row;
    row.scheduler = "local-priority-fifo";
    row.pool_size = 20;
    HPX_TEST_EQ(tuning::section_name(row),
        std::string("hpx.idle_tuning.local-priority-fifo.none.20"));

    row.scheduler = "local-priority-tuned-fifo";
    row.tuner = "adaptive-spin";
    row.pool_size = 40;
    HPX_TEST_EQ(tuning::section_name(row),
        std::string(
            "hpx.idle_tuning.local-priority-tuned-fifo.adaptive-spin.40"));

    // a row for pools of every size has no size in its name
    row.pool_size = tuning::any_pool_size;
    HPX_TEST_EQ(tuning::section_name(row),
        std::string("hpx.idle_tuning.local-priority-tuned-fifo.adaptive-spin"));
}

void test_to_ini()
{
    tuning::tuning_row row;
    row.scheduler = "local-priority-fifo";
    row.pool_size = 20;
    row.settings.push_back(tuning::tuned_setting{
        "hpx.min_idle_backoff_time", "55", "shortest sleep 55.2 us", false});
    row.settings.push_back(tuning::tuned_setting{
        "hpx.max_idle_backoff_time", "1000", "shortest sleep 55.2 us", true});

    std::string const ini = tuning::to_ini({row}, {"written by a test"}, 2026);

    auto const has = [&](char const* text) {
        bool const found = ini.find(text) != std::string::npos;
        HPX_TEST_MSG(found, text);
    };
    has("# Copyright (c) 2026 TheHPXProject\n");
    has("# SPDX-License-Identifier: BSL-1.0\n");
    has("# Distributed under the Boost Software License, Version 1.0.");
    has("# written by a test\n");
    has("\n[hpx.idle_tuning.local-priority-fifo.none.20]\n");
    has("# shortest sleep 55.2 us\nmin_idle_backoff_time = 55\n");
    has("# shortest sleep 55.2 us (HPX's default)\n"
        "max_idle_backoff_time = 1000\n");

    // the license header comes first
    HPX_TEST_EQ(ini.find("# Copyright"), std::string::size_type(0));
    // keys are written without "hpx."
    HPX_TEST_EQ(ini.find("hpx.min_idle_backoff_time"), std::string::npos);

    // the table is written as its sections
    std::vector<tuning::ini_section> const sections =
        tuning::to_sections({row});
    HPX_TEST_EQ(sections.size(), std::size_t(1));
    HPX_TEST_EQ(sections[0].name, tuning::section_name(row));
    HPX_TEST_EQ(sections[0].settings.size(), row.settings.size());
    HPX_TEST_EQ(tuning::to_ini(sections, {"written by a test"}, 2026), ini);
}

// long comments (a reason, a comment line, one word longer than a line) are
// broken into lines of at most max_comment_line characters, every word kept
void test_long_comments()
{
    std::string reason;
    for (int i = 0; i != 40; ++i)
        reason += "word" + std::to_string(i) + " ";
    std::string const long_word(250, 'x');

    tuning::ini_section section{"hpx.parcel", {}};
    section.settings.push_back(
        tuning::tuned_setting{"hpx.parcel.enable", "1", reason, true});
    std::string const ini =
        tuning::to_ini({section}, {reason, long_word}, 2026);

    std::size_t begin = 0;
    std::size_t lines = 0;
    std::string comments;
    // the pieces of the long word: comment lines of nothing but 'x'
    std::size_t long_word_pieces = 0;
    while (begin < ini.size())
    {
        std::size_t const end = ini.find('\n', begin);
        std::string const line = ini.substr(begin, end - begin);
        HPX_TEST_LTE(line.size(), tuning::max_comment_line);
        if (line.size() > 2 && line[0] == '#')
        {
            std::string const text = line.substr(line.find_first_not_of("# "));
            comments += text + " ";
            if (text.find_first_not_of('x') == std::string::npos)
                long_word_pieces += text.size();
        }
        ++lines;
        begin = end + 1;
    }
    HPX_TEST_LT(std::size_t(12), lines);

    // every word is still there, the long word in pieces
    for (int i = 0; i != 40; ++i)
    {
        HPX_TEST(comments.find("word" + std::to_string(i) + " ") !=
            std::string::npos);
    }
    HPX_TEST(comments.find("(HPX's default)") != std::string::npos);
    HPX_TEST_EQ(long_word_pieces, long_word.size());
    HPX_TEST(ini.find("\nenable = 1\n") != std::string::npos);
}

void test_plain_sections()
{
    // sections of another area, in one file with an idle tuning section
    tuning::ini_section parcel{"hpx.parcel.tcp", {}};
    parcel.settings.push_back(tuning::tuned_setting{
        "hpx.parcel.tcp.enable", "1", "fastest for large messages", true});
    parcel.settings.push_back(tuning::tuned_setting{
        "hpx.parcel.bootstrap", "tcp", "the only parcelport", false});

    tuning::tuning_row row;
    row.scheduler = "local-priority-fifo";
    row.pool_size = 4;
    row.settings.push_back(tuning::tuned_setting{
        "hpx.min_idle_backoff_time", "55", "shortest sleep 55.2 us", false});

    std::vector<tuning::ini_section> sections = tuning::to_sections({row});
    sections.push_back(parcel);
    std::string const ini = tuning::to_ini(sections, {}, 2027);

    auto const has = [&](char const* text) {
        bool const found = ini.find(text) != std::string::npos;
        HPX_TEST_MSG(found, text);
    };
    has("# Copyright (c) 2027 TheHPXProject\n");
    has("\n[hpx.idle_tuning.local-priority-fifo.none.4]\n"
        "# shortest sleep 55.2 us\nmin_idle_backoff_time = 55\n");
    // a key in its own section is written relative to it
    has("\n[hpx.parcel.tcp]\n"
        "# fastest for large messages (HPX's default)\nenable = 1\n");
    // another key loses only the leading "hpx."
    has("# the only parcelport\nparcel.bootstrap = tcp\n");
    // without comment lines the header isn't followed by an empty comment
    HPX_TEST_EQ(ini.find("#\n\n"), std::string::npos);
    // the sections in the given order
    HPX_TEST(ini.find("[hpx.idle_tuning.") < ini.find("[hpx.parcel.tcp]"));
}

int main()
{
    test_section_name();
    test_to_ini();
    test_long_comments();
    test_plain_sections();
    return hpx::util::report_errors();
}
