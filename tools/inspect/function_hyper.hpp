//  Hyperlink Function  ------------------------------------------//

//  Copyright (c) 2015 Brandon Cordes
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/filesystem.hpp>

#include "inspector.hpp"

#include <string>

using hpx::filesystem::path;

// When you have a specific line and the line is the location of the link
inline std::string linelink(path const& full_path, std::string const& linenumb)
{
    std::string blob_prefix = boost::inspect::search_root_git_blob_prefix();
    if (blob_prefix.empty())
    {
        return linenumb;
    }

    std::string location = boost::inspect::relative_to(
        full_path, boost::inspect::search_root_git_path());
    return "<a href = \"" + blob_prefix + location + "#L" + linenumb + "\">" +
        linenumb + "</a>";
}

// When you have a specific line, but a word is the location of the link
inline std::string wordlink(
    path const& full_path, std::string const& linenumb, std::string const& word)
{
    std::string blob_prefix = boost::inspect::search_root_git_blob_prefix();
    if (blob_prefix.empty())
    {
        return word;
    }

    std::string location = boost::inspect::relative_to(
        full_path, boost::inspect::search_root_git_path());
    return "<a href = \"" + blob_prefix + location + "#L" + linenumb + "\">" +
        word + "</a>";
}

// When you don't have a specific line
inline std::string loclink(path const& full_path, std::string const& word)
{
    std::string blob_prefix = boost::inspect::search_root_git_blob_prefix();
    if (blob_prefix.empty())
    {
        return word;
    }

    std::string location = boost::inspect::relative_to(
        full_path, boost::inspect::search_root_git_path());
    return "<a href = \"" + blob_prefix + location + "\">" + word + "</a>";
}
