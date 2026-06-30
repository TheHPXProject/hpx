// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
// Copyright (c) 2016-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_WINDOWS)
#include <hpx/components/process/util/search_path.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/string_util.hpp>

#include <unistd.h>

#include <cstdlib>
#include <stdexcept>
#include <string>

namespace hpx { namespace components { namespace process { namespace posix {
    std::string search_path(std::string const& filename, std::string path)
    {
        if (path.empty())
        {
            path = ::getenv("PATH");
            if (path.empty())
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "process::search_path",
                    "Environment variable PATH not found");
            }
        }

        std::string result;
        typedef hpx::string_util::tokenizer<
            hpx::string_util::char_separator<char>>
            tokenizer;
        hpx::string_util::char_separator<char> sep(":");
        tokenizer tok(path, sep);
        for (tokenizer::iterator it = tok.begin(); it != tok.end(); ++it)
        {
            filesystem::path p = *it;
            p /= filename;
            if (!::access(p.c_str(), X_OK))
            {
                result = hpx::filesystem::to_string(p);
                break;
            }
        }
        return result;
    }
}}}}    // namespace hpx::components::process::posix

#endif
