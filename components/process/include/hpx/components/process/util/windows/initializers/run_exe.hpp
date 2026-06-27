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
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/components/process/util/windows/initializers/initializer_base.hpp>

#include <string>

namespace hpx::components::process::windows::initializers {

    template <typename String>
    class run_exe_ : public initializer_base
    {
    public:
        run_exe_() = default;

        explicit run_exe_(String const& s)
          : s_(s)
        {
        }

        template <typename WindowsExecutor>
        void on_CreateProcess_setup(WindowsExecutor& e) const noexcept
        {
            e.exe = s_.c_str();
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & s_;
        }

        String s_;
    };

#if defined(_UNICODE) || defined(UNICODE)
    inline auto run_exe(const wchar_t* ws)
    {
        return run_exe_<std::wstring>(ws);
    }

    inline auto run_exe(std::wstring const& ws)
    {
        return run_exe_<std::wstring>(ws);
    }

    inline auto run_exe(filesystem::path const& p)
    {
        return run_exe_<std::wstring>(p.wstring());
    }
#endif

    inline auto run_exe(const char* s)
    {
        return run_exe_<std::string>(s);
    }

    inline auto run_exe(std::string const& s)
    {
        return run_exe_<std::string>(s);
    }

    inline auto run_exe(filesystem::path const& p)
    {
        return run_exe_<std::string>(p.string());
    }
}    // namespace hpx::components::process::windows::initializers

#endif
