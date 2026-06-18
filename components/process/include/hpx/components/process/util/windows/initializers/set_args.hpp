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

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

namespace hpx::components::process::windows::initializers {

    template <typename Range>
    class set_args_ : public initializer_base
    {
    private:
        using ConstIterator = Range::const_iterator;
        using String = Range::value_type;
        using Char = String::value_type;
        using OStringStream = std::basic_ostringstream<Char>;

    public:
        set_args_() = default;

        explicit set_args_(Range const& args)
        {
            ConstIterator it = std::begin(args);
            ConstIterator end = std::end(args);
            if (it != end)
            {
                exe_ = *it;
                OStringStream os;
                for (; it != end; ++it)
                {
                    auto end = std::end(*it);
                    if (std::find(std::begin(*it), end,
                            static_cast<Char>(' ')) != end)
                    {
                        os << static_cast<Char>('"') << *it
                           << static_cast<Char>('"');
                    }
                    else
                    {
                        os << *it;
                    }
                    os << static_cast<Char>(' ');
                }
                cmd_line_ = os.str();
            }
            else
            {
                exe_.clear();
                cmd_line_.clear();
            }
        }

        template <typename WindowsExecutor>
        void on_CreateProcess_setup(WindowsExecutor& e) const noexcept
        {
            e.cmd_line = const_cast<char*>(cmd_line_.c_str());
            if (!e.exe && !exe_.empty())
                e.exe = exe_.c_str();
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
                ar & exe_ & cmd_line_;
            // clang-format on
        }

        String exe_;
        String cmd_line_;
    };

    template <typename Range>
    auto set_args(Range const& range)
    {
        return set_args_<Range>(range);
    }
}    // namespace hpx::components::process::windows::initializers

#endif
