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

#include <cstddef>
#include <cstring>
#include <iterator>
#include <numeric>
#include <vector>

namespace hpx::components::process::windows::initializers {

    template <typename Range, bool Unicode>
    class set_env_ : public initializer_base
    {
    private:
        using String = Range::value_type;
        using Char = String::value_type;

        static std::size_t get_size(std::size_t len, String const& s)
        {
            return len + s.size() + 1;
        }

        struct copy_env
        {
            explicit copy_env(Char* curr)
              : curr_(curr)
            {
            }

            void operator()(String const& s)
            {
                std::memcpy(curr_, s.c_str(), s.size());
                curr_ += s.size();
                *curr_++ = 0;
            }

            Char* curr_;
        };

    public:
        set_env_()
        {
            env_.resize(1);
            env_[0] = 0;
        }

        explicit set_env_(Range const& envs)
        {
            std::size_t s = std::accumulate(
                envs.begin(), envs.end(), std::size_t(0), &set_env_::get_size);

            env_.resize(s + 1);
            std::for_each(envs.begin(), envs.end(), copy_env(env_.data()));
            env_[env_.size() - 1] = 0;
        }

        template <typename WindowsExecutor>
        void on_CreateProcess_setup(WindowsExecutor& e) const noexcept
        {
            e.env = LPVOID(env_.data());
            if (Unicode)
                e.creation_flags |= CREATE_UNICODE_ENVIRONMENT;
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned const)
        {
            ar & env_;
        }

        std::vector<Char> env_;
    };

#if defined(_UNICODE) || defined(UNICODE)
    template <typename Range>
    auto set_env(const Range& envs)
    {
        return set_env_<Range, true>(envs);
    }
#endif

    template <typename Range>
    auto set_env(const Range& envs)
    {
        return set_env_<Range, false>(envs);
    }
}    // namespace hpx::components::process::windows::initializers

#endif
