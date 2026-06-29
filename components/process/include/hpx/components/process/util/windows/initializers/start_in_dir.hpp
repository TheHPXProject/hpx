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
    class start_in_dir_ : public initializer_base
    {
    public:
        start_in_dir_() = default;

        explicit start_in_dir_(String const& s)
          : s_(s)
        {
        }

        template <typename WindowsExecutor>
        void on_CreateProcess_setup(WindowsExecutor& e) const noexcept
        {
            e.work_dir = s_.c_str();
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned const)
        {
            ar & s_;
        }

        String s_;
    };

#if defined(_UNICODE) || defined(UNICODE)
    inline auto start_in_dir(const wchar_t* ws)
    {
        return start_in_dir_<std::wstring>(ws);
    }

    inline auto start_in_dir(std::wstring const& ws)
    {
        return start_in_dir_<std::wstring>(ws);
    }

    inline auto start_in_dir(filesystem::path const& p)
    {
        return start_in_dir_<std::wstring>(p.wstring());
    }
#endif

    inline auto start_in_dir(const char* s)
    {
        return start_in_dir_<std::string>(s);
    }

    inline auto start_in_dir(std::string const& s)
    {
        return start_in_dir_<std::string>(s);
    }

    inline auto start_in_dir(filesystem::path const& p)
    {
        return start_in_dir_<std::string>(hpx::filesystem::to_string(p));
    }
}    // namespace hpx::components::process::windows::initializers

#endif
