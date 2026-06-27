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

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)
#include <hpx/modules/serialization.hpp>

#include <hpx/components/process/util/windows/initializers/initializer_base.hpp>

#include <memory>
#include <string>

namespace hpx::components::process::windows::initializers {

    template <typename String>
    class set_cmd_line_ : public initializer_base
    {
    public:
        explicit set_cmd_line_(String const& s)
          : cmd_line_(s)
        {
        }

        template <typename WindowsExecutor>
        void on_CreateProcess_setup(WindowsExecutor& e) const noexcept
        {
            e.cmd_line = cmd_line_.c_str();
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & cmd_line_;
        }

        String cmd_line_;
    };

#if defined(_UNICODE) || defined(UNICODE)
    inline auto set_cmd_line(const wchar_t* ws)
    {
        return set_cmd_line_<std::wstring>(ws);
    }

    inline auto set_cmd_line(std::wstring const& ws)
    {
        return set_cmd_line_<std::wstring>(ws);
    }
#endif

    inline auto set_cmd_line(const char* s)
    {
        return set_cmd_line_<std::string>(s);
    }

    inline auto set_cmd_line(std::string const& s)
    {
        return set_cmd_line_<std::string>(s);
    }
}    // namespace hpx::components::process::windows::initializers

#endif
