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

#include <type_traits>
#include <utility>

namespace hpx::components::process::windows::initializers {

    template <typename Handler>
    class on_CreateProcess_error_ : public initializer_base
    {
    public:
        on_CreateProcess_error_() = default;

        explicit on_CreateProcess_error_(Handler handler)
          : handler_(HPX_MOVE(handler))
        {
        }

        template <class WindowsExecutor>
        void on_CreateProcess_error(WindowsExecutor& e) const
        {
            handler_(e);
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned const)
        {
            ar & handler_;
        }

        Handler handler_;
    };

    template <typename Handler>
    auto on_CreateProcess_error(Handler&& handler)
    {
        return on_CreateProcess_error_<std::decay_t<Handler>>(
            HPX_FORWARD(Handler, handler));
    }
}    // namespace hpx::components::process::windows::initializers

#endif
