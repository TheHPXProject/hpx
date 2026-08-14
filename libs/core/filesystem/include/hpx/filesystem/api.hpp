//  Copyright (c) 2019 Mikael Simberg
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  hpxinspect:nodeprecatedinclude:boost/filesystem.hpp

/// \file
/// This file provides a compatibility layer using Boost.Filesystem for the
/// C++17 filesystem library. It is *not* intended to be a complete
/// compatibility layer. It only contains functions required by the HPX
/// codebase. It also provides some functions only available in Boost.Filesystem
/// when using C++17 filesystem.

#pragma once

#include <hpx/config.hpp>
#include <hpx/filesystem/config/defines.hpp>

#if !defined(HPX_FILESYSTEM_HAVE_BOOST_FILESYSTEM_COMPATIBILITY)

#include <filesystem>
#include <string>
#include <system_error>

namespace hpx::filesystem {

    HPX_CXX_CORE_EXPORT using namespace std::filesystem;

    HPX_CXX_CORE_EXPORT using std::filesystem::canonical;

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline path initial_path()
    {
        static path ip = current_path();
        return ip;
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline std::string to_string(
        path const& p)
    {
#if defined(__GLIBCXX__) && __cplusplus >= 202400L
        return p.display_string();
#else
        return p.string();
#endif
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline std::string basename(path const& p)
    {
        return to_string(p.stem());
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline path canonical(
        path const& p, path const& base)
    {
        if (p.is_relative())
        {
            return canonical(base / p);
        }
        else
        {
            return canonical(p);
        }
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline path canonical(
        path const& p, path const& base, std::error_code& ec)
    {
        if (p.is_relative())
        {
            return canonical(base / p, ec);
        }
        else
        {
            return canonical(p, ec);
        }
    }
}    // namespace hpx::filesystem

#else

#include <hpx/config/detail/compat_error_code.hpp>

#include <boost/filesystem.hpp>

#include <system_error>

static_assert(BOOST_FILESYSTEM_VERSION == 3,
    "HPX requires Boost.Filesystem version 3 (or support for the C++17 "
    "filesystem library)");

namespace hpx::filesystem {

    HPX_CXX_CORE_EXPORT using namespace boost::filesystem;

    HPX_CXX_CORE_EXPORT using boost::filesystem::canonical;

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline std::string to_string(
        path const& p)
    {
        return p.string();
    }

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline path canonical(
        path const& p, path const& base, std::error_code& ec)
    {
        return canonical(p, base, compat_error_code(ec));
    }

    HPX_CXX_CORE_EXPORT using boost::filesystem::exists;

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline bool exists(
        path const& p, std::error_code& ec) noexcept
    {
        return exists(p, compat_error_code(ec));
    }

    HPX_CXX_CORE_EXPORT using boost::filesystem::is_regular_file;

    HPX_CXX_CORE_EXPORT [[nodiscard]] inline bool is_regular_file(
        path const& p, std::error_code& ec) noexcept
    {
        return is_regular_file(p, compat_error_code(ec));
    }
}    // namespace hpx::filesystem

#endif
