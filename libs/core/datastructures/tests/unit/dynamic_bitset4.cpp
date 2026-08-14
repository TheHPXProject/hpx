//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0 Distributed under the Boost Software
//  License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// This code was adapted from boost dynamic_bitset
//
// Copyright (c) 2001 Jeremy Siek
// Copyright (c) 2003-2006 Gennaro Prota

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/filesystem.hpp>

#include <cstddef>    // for std::size_t
#include <fstream>
#include <locale>
#include <sstream>
#include <stdexcept>    // for std::logic_error
#include <string>

#include "bitset_test.hpp"

std::wstring widen_string(
    std::string const& str, std::locale const& loc = std::locale())
{
    std::wstring result;
    std::string::size_type const len = str.length();
    if (len != 0)
    {
        typedef std::ctype<wchar_t> ct_type;
        typedef std::wstring::traits_type tr_type;
        ct_type const& ct = std::use_facet<ct_type>(loc);

        result.resize(len);
        for (std::size_t i = 0; i < len; ++i)
            tr_type::assign(result[i], ct.widen(str[i]));
    }
    return result;
}

template <typename Block>
void run_test_cases()
{
    typedef hpx::detail::dynamic_bitset<Block> bitset_type;
    typedef bitset_test<bitset_type> Tests;

    //=====================================================================
    // Test stream operator<<
    {
        // The test "variables" are: the stream type and its state, the
        // exception mask, the width, the fill char and the padding side (left/right)

        std::ios::iostate masks[] = {std::ios::goodbit, std::ios::eofbit,
            std::ios::failbit, std::ios::eofbit | std::ios::failbit};

        static std::string strings[] = {std::string(""), std::string("0"),
            std::string("1"), std::string("11100"), get_long_string()};

        char fill_chars[] = {'*', 'x', ' '};

        std::size_t num_masks = sizeof(masks) / sizeof(masks[0]);
        std::size_t num_strings = sizeof(strings) / sizeof(strings[0]);
        std::size_t num_chars = sizeof(fill_chars) / sizeof(fill_chars[0]);

        std::fstream not_good_stream(
            "dynamic_bitset_tests - this file shouldn't exist", std::ios::in);

        for (std::size_t mi = 0; mi < num_masks; ++mi)
        {
            for (std::size_t si = 0; si < num_strings; ++si)
            {
                std::streamsize slen = (std::streamsize) (strings[si].length());

                HPX_ASSERT((std::numeric_limits<std::streamsize>::max)() >=
                    (std::streamsize) (1 + slen * 2));

                for (std::size_t ci = 0; ci < num_chars; ++ci)
                {
                    // note how "negative widths" are tested too
                    std::streamsize const widths[] = {
                        -1 - slen / 2, 0, slen / 2, 1 + slen * 2};
                    std::size_t num_widths = sizeof(widths) / sizeof(widths[0]);

                    for (std::size_t wi = 0; wi < num_widths; ++wi)
                    {
                        std::streamsize w = widths[wi];
                        {
                            // test 0 - stream !good()
                            if (not_good_stream.good())
                                throw std::logic_error(
                                    "Error in operator << tests"
                                    " - please, double check");
                            bitset_type b(strings[si]);
                            not_good_stream.width(w);
                            not_good_stream.fill(fill_chars[ci]);
                            try
                            {
                                not_good_stream.exceptions(masks[mi]);
                            }
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                            catch (...)
                            {
                            }

                            Tests::stream_inserter(
                                b, not_good_stream, "<unused_string>");
                        }
#if !defined(HPX_GCC_VERSION)
                        {
                            // test 1a - file stream
                            scoped_temp_file stf;
                            bitset_type b(strings[si]);
                            std::ofstream file(
                                hpx::filesystem::to_string(stf.path()).c_str(),
                                std::ios::trunc);
                            file.width(w);
                            file.fill(fill_chars[ci]);
                            file.exceptions(masks[mi]);
                            Tests::stream_inserter(b, file,
                                hpx::filesystem::to_string(stf.path()).c_str());
                        }
                        {
                            //NOTE: there are NO string stream tests
                        }
                        {
                            // test 1b - wide file stream
                            scoped_temp_file stf;
                            bitset_type b(strings[si]);
                            std::wofstream file(
                                hpx::filesystem::to_string(stf.path()).c_str());
                            file.width(w);
                            file.fill(fill_chars[ci]);
                            file.exceptions(masks[mi]);
                            Tests::stream_inserter(b, file,
                                hpx::filesystem::to_string(stf.path()).c_str());
                        }
#endif
                    }
                }
            }
        }    // for (; mi..)
    }

    //=====================================================================
    // Test stream operator>>
    {
        // The test "variables" are: the stream type, the exception mask,
        // the actual contents (and/or state) of the stream, and width.
        //
        // With few exceptions, each test case consists of writing a different
        // assortment of digits and "whitespaces" to a text stream and then checking
        // that what was written gets read back unchanged. That's NOT guaranteed by
        // the standard, unless the assortment always ends with a '\n' and satisfies
        // other conditions (see C99, 7.19.2/2), however it works in practice and is
        // a good "real life" test. Some characters, such as '\v' and '\f', are not
        // used exactly because they are the ones which will most likely give problems
        // on some systems (for instance '\f' could actually be written as a sequence
        // of new-lines, and we could never be able to read it back)
        //
        // Note how the bitset object is not initially empty. That helps checking
        // that it isn't erroneously clear()ed by operator>>.

        std::ios::iostate masks[] = {std::ios::goodbit, std::ios::eofbit,
            std::ios::failbit, std::ios::eofbit | std::ios::failbit};

        std::string const spaces = "\t\n ";    //"\t\n\v\f ";

        std::string const long_string = get_long_string();
        static std::string strings[] = {// empty string
            std::string(""),
            // no bitset
            spaces,
            // no bitset
            std::string("x"), std::string("\t  xyz"),

            // bitset of size 1
            std::string("0"), std::string("1"),

            std::string("  0  "), std::string("  1  "), spaces + "1",
            "1" + spaces, spaces + "1" + spaces, std::string("  x1x  "),
            std::string("  1x  "),

            // long bitset
            long_string, "  " + long_string + " xyz", spaces + long_string,
            spaces + long_string + spaces};

        //-----------------------------------------------------

        std::stringstream not_good_stream;
        not_good_stream << "test";
        std::string sink;
        not_good_stream >> sink;    // now the stream should be in eof state

        std::size_t const num_masks = sizeof(masks) / sizeof(masks[0]);
        std::size_t const num_strings = sizeof(strings) / sizeof(strings[0]);

        for (std::size_t mi = 0; mi < num_masks; ++mi)
        {
            for (std::size_t si = 0; si < num_strings; ++si)
            {
                std::streamsize const slen =
                    (std::streamsize) (strings[si].length());
                HPX_ASSERT((std::numeric_limits<std::streamsize>::max)() >=
                    (std::streamsize) (1 + slen * 2));

                std::streamsize widths[] = {
                    -1, 0, slen / 2, slen, 1 + slen * 2};
                std::size_t num_widths = sizeof(widths) / sizeof(widths[0]);

                for (std::size_t wi = 0; wi < num_widths; ++wi)
                {
                    std::streamsize const w = widths[wi];

                    // test 0 - !good() stream
                    {
                        if (not_good_stream.good())
                            throw std::logic_error("Error in operator >> tests"
                                                   " - please, double check");
                        bitset_type b(1, 15ul);    // note: b is not empty
                        not_good_stream.width(w);
                        try
                        {
                            not_good_stream.exceptions(masks[mi]);
                        }
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                        catch (...)
                        {
                        }
                        std::string irrelevant;
                        Tests::stream_extractor(b, not_good_stream, irrelevant);
                    }
#if !defined(HPX_GCC_VERSION)
                    // test 1a - (narrow) file stream
                    {
                        scoped_temp_file stf;
                        bitset_type b(1, 255ul);
                        {
                            std::ofstream f(
                                hpx::filesystem::to_string(stf.path()).c_str());
                            f << strings[si];
                        }

                        std::ifstream f(
                            hpx::filesystem::to_string(stf.path()).c_str());
                        f.width(w);
                        f.exceptions(masks[mi]);
                        Tests::stream_extractor(b, f, strings[si]);
                    }
                    // test 2a - stringstream
                    {
                        bitset_type b(1, 255ul);
                        std::istringstream stream(strings[si]);
                        stream.width(w);
                        stream.exceptions(masks[mi]);
                        Tests::stream_extractor(b, stream, strings[si]);
                    }

                    // test 1b - wchar_t file stream
                    {
                        scoped_temp_file stf;
                        std::wstring wstr = widen_string(strings[si]);
                        bitset_type b(1, 255ul);
                        {
                            std::basic_ofstream<wchar_t> of(
                                hpx::filesystem::to_string(stf.path()).c_str());
                            of << wstr;
                        }

                        std::basic_ifstream<wchar_t> f(
                            hpx::filesystem::to_string(stf.path()).c_str());
                        f.width(w);
                        f.exceptions(masks[mi]);
                        Tests::stream_extractor(b, f, wstr);
                    }
                    // test 2b - wstringstream
                    {
                        bitset_type b(1, 255ul);
                        std::wstring wstr = widen_string(strings[si]);

                        std::wistringstream wstream(wstr);
                        wstream.width(w);
                        wstream.exceptions(masks[mi]);
                        Tests::stream_extractor(b, wstream, wstr);
                    }
#endif
                }
            }
        }    // for ( mi = 0; ...)
    }
}

int main()
{
    run_test_cases<unsigned char>();
    run_test_cases<unsigned short>();
    run_test_cases<unsigned int>();
    run_test_cases<unsigned long>();
    run_test_cases<unsigned long long>();

    return hpx::util::report_errors();
}
