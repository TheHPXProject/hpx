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
#include <hpx/modules/iostream.hpp>

#include <hpx/components/process/util/windows/initializers/initializer_base.hpp>

namespace hpx::components::process::posix::initializers {

    class bind_stdin : public initializer_base
    {
    public:
        explicit bind_stdin(hpx::iostream::file_descriptor_source const& source)
          : source_(source)
        {
        }

        template <class WindowsExecutor>
        void on_CreateProcess_setup(WindowsExecutor& e) const
        {
            ::SetHandleInformation(
                source_.handle(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            e.startup_info.hStdInput = source_.handle();
            e.startup_info.dwFlags |= STARTF_USESTDHANDLES;
            e.inherit_handles = true;
        }

    private:
        hpx::iostream::file_descriptor_source source_;
    };
}    // namespace hpx::components::process::posix::initializers

#endif
