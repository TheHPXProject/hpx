//  windows_macro_check implementation  ------------------------------------------------//
//  Copyright Ste||ar Group

//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/filesystem.hpp>

#include <functional>
#include <string>
#include "boost/regex.hpp"
#include "function_hyper.hpp"
#include "windows_macro_check.hpp"

namespace fs = hpx::filesystem;

namespace {
    // Names the Windows SDK headers (windef.h, minwindef.h, winnt.h,
    // wingdi.h, rpcndr.h) define as macros or typedefs. Declaring any of them
    // in HPX code breaks or silently changes the build with MSVC (e.g. a
    // constant named SIZE collides with the SIZE typedef, error C2377).
    std::string const windows_names =
        "\\b(small|near|far|interface|SIZE|POINT|RECT|ERROR|DELETE|IN|OUT|"
        "OPTIONAL|CONST)\\b";

    boost::regex windows_macro_regex(
        // # undef followed by a windows macro name
        "(^\\s*#\\s*undef\\s*" + windows_names +
            ")"
            // or (ignored) single line comments (//), multi line comments
            // (/**/) and string literals
            "|(//[^\\n]*|/\\*.*?\\*/|\"(?:\\\\\\\\|\\\\\"|[^\"])*\")"
            // or a windows macro name
            "|(" +
            windows_names + ")",
        boost::regex::normal);

}    // unnamed namespace

namespace boost { namespace inspect {
    windows_macro_check::windows_macro_check()
      : m_files_with_errors(0)
    {
        register_signature(".c");
        register_signature(".cpp");
        register_signature(".cu");
        register_signature(".cxx");
        register_signature(".h");
        register_signature(".hpp");
        register_signature(".hxx");
        register_signature(".ipp");
        register_signature(".ixx"); 
        register_signature(".cppm");
    }

    void windows_macro_check::inspect(const string& library_name,
        const path& full_path,     // example: c:/foo/boost/filesystem/path.hpp
        const string& contents)    // contents of file to be inspected
    {
        if (contents.find("hpxinspect:"
                          "nowindows_macros") != string::npos)
            return;

        boost::sregex_iterator cur(
            contents.begin(), contents.end(), windows_macro_regex),
            end;

        long errors = 0;

        for (; cur != end; ++cur /*, ++m_files_with_errors*/)
        {
            if (!(*cur)[3].matched)
            {
                string::const_iterator it = contents.begin();
                string::const_iterator match_it = (*cur)[0].first;

                string::const_iterator line_start = it;

                string::size_type line_number = 1;
                for (; it != match_it; ++it)
                {
                    if (string::traits_type::eq(*it, '\n'))
                    {
                        ++line_number;
                        line_start = it + 1;    // could be end()
                    }
                }
                ++errors;
                error(library_name, full_path,
                    "Windows macro clash: " +
                        std::string((*cur)[0].first, (*cur)[0].second),
                    line_number);
            }
        }
        if (errors > 0)
        {
            ++m_files_with_errors;
        }
    }
}}    // namespace boost::inspect
