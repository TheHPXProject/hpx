//  module_std_include_check header  ----------------------------------------//

//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "inspector.hpp"

#include <string>

namespace boost { namespace inspect {

    class module_std_include_check : public inspector
    {
        long m_errors;

    public:
        module_std_include_check();

        virtual char const* name() const
        {
            return "*MI*";
        }

        virtual char const* desc() const
        {
            return "unguarded standard library #include in converted C++ "
                   "module";
        }

        virtual void inspect(std::string const& library_name,
            path const& full_path, std::string const& contents);

        virtual void print_summary(std::ostream& out)
        {
            out << "  " << m_errors
                << " unguarded standard library #include's in converted modules"
                << line_break();
        }

        virtual ~module_std_include_check() {}
    };
}}    // namespace boost::inspect
