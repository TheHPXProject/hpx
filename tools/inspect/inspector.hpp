//  inspector header  --------------------------------------------------------//

//  Copyright Beman Dawes 2002.
//  Copyright Rene Rivera 2004.
//  Copyright Gennaro Prota 2006.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/filesystem.hpp>

#include <cstddef>
#include <iostream>
#include <ostream>
#include <set>
#include <string>

using hpx::filesystem::path;
using std::string;

namespace boost { namespace inspect {
    typedef std::set<string> string_set;

    char const* line_break();

    path search_root_path();
    path search_root_git_path();
    std::string search_root_git_commit();
    std::string search_root_git_blob_prefix();

    class inspector
    {
    protected:
        inspector() {}

    public:
        virtual ~inspector() {}

        virtual char const* name() const = 0;    // example: "tab-check"
        virtual char const* desc() const = 0;    // example: "verify no tabs"

        // always called:
        virtual void inspect(string const& /*library_name*/,    // "filesystem"
            path const& /*full_path*/)
        {
        }    // "c:/foo/boost/filesystem/path.hpp"

        // called only for registered leaf() signatures:
        virtual void inspect(string const& library_name,    // "filesystem"
            path const& full_path,     // "c:/foo/boost/filesystem/path.hpp"
            string const& contents)    // contents of file
            = 0;

        // called after all paths visited, but still in time to call error():
        virtual void close() {}

        virtual void print_summary(std::ostream& out) = 0;

        // callback used by constructor to register leaf() signature.
        // Signature can be a full file name (Jamfile) or partial (.cpp)
        void register_signature(string const& signature);
        void register_skip_signature(string const& signature);
        string_set const& signatures() const
        {
            return m_signatures;
        }
        string_set const& skip_signatures() const
        {
            return m_skip_signatures;
        }

        // report error callback (from inspect(), close() ):
        void error(string const& library_name, path const& full_path,
            string const& msg,
            std::size_t line_number =
                0);    // 0 if not available or not applicable

    private:
        string_set m_signatures;
        string_set m_skip_signatures;
    };

    // for inspection of header files
    class header_inspector : public inspector
    {
    public:
        // registers the basic set of known source signatures
        header_inspector();
    };

    // for inspection of source code of one form or other
    class source_inspector : public header_inspector
    {
    public:
        // registers the basic set of known source signatures
        source_inspector();
    };

    // for inspection of hypertext, specifically html
    class hypertext_inspector : public inspector
    {
    public:
        // registers the set of known html source signatures
        hypertext_inspector();
    };

    inline string relative_to(path const& src_arg, path const& base_arg)
    {
        path base(base_arg);
        base = base.lexically_normal();
        string::size_type pos(base.string().size());
        string src_arg_s(src_arg.string());
        path src;
        if (pos < src_arg_s.size())
            src = path(src_arg.string().substr(pos));
        else
            src = path(src_arg_s);
        src = src.lexically_normal();
        return src.string();
    }

    string impute_library(path const& full_dir_path);

}}    // namespace boost::inspect
