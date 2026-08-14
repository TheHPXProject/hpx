// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)
#include <hpx/modules/errors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/string_util.hpp>

#include <hpx/components/process/util/windows/search_path.hpp>

#include <shellapi.h>

#include <array>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <system_error>

namespace hpx::components::process::windows {

#if defined(_UNICODE) || defined(UNICODE)
    std::wstring search_path(const std::wstring& filename, std::wstring path)
    {
        if (path.empty())
        {
            path = ::_wgetenv(L"PATH");
            if (path.empty())
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "process::search_path",
                    "Environment variable PATH not found");
            }
        }

        typedef hpx::string_util::tokenizer<
            hpx::string_util::char_separator<wchar_t>,
            std::wstring::const_iterator, std::wstring>
            tokenizer;
        hpx::string_util::char_separator<wchar_t> sep(L";");
        tokenizer tok(path, sep);
        for (tokenizer::iterator it = tok.begin(); it != tok.end(); ++it)
        {
            filesystem::path p = *it;
            p /= filename;
            std::array<std::wstring, 4> extensions = {
                L"", L".exe", L".com", L".bat"};
            for (std::array<std::wstring, 4>::iterator it2 = extensions.begin();
                it2 != extensions.end(); ++it2)
            {
                filesystem::path p2 = p;
                p2 += *it2;
                filesystem::error_code ec;
                bool file = filesystem::is_regular_file(p2, ec);
                if (!ec && file &&
                    SHGetFileInfoW(p2.c_str(), 0, 0, 0, SHGFI_EXETYPE))
                {
                    return p2.wstring();
                }
            }
        }
        return L"";
    }
#endif

    std::string search_path(const std::string& filename, std::string path)
    {
        if (path.empty())
        {
            auto* env = ::getenv("PATH");
            if (env != nullptr)
            {
                path = env;
            }

            if (path.empty())
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "process::search_path",
                    "Environment variable PATH not found");
            }
        }

        hpx::string_util::char_separator<char> sep(";");
        hpx::string_util::tokenizer tok(path, sep);
        for (auto it = tok.begin(); it != tok.end(); ++it)
        {
            filesystem::path p = *it;
            p /= filename;
            std::array<std::string, 4> extensions =    //-V112
                {{"", ".exe", ".com", ".bat"}};
            for (std::array<std::string, 4>::iterator it2 = extensions.begin();
                it2 != extensions.end(); ++it2)
            {
                filesystem::path p2 = p;
                p2 += *it2;
                std::error_code ec;
                bool file = filesystem::is_regular_file(p2, ec);
                if (!ec && file &&
                    SHGetFileInfoA(hpx::filesystem::to_string(p2).c_str(), 0,
                        nullptr,    //-V575
                        0, SHGFI_EXETYPE))
                {
                    return hpx::filesystem::to_string(p2);
                }
            }
        }
        return "";
    }
}    // namespace hpx::components::process::windows

#endif
