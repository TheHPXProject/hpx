// Copyright (c) 2006, 2007 Julio M. Merino Vidal
// Copyright (c) 2008 Ilya Sokolov, Boris Schaeling
// Copyright (c) 2009 Boris Schaeling
// Copyright (c) 2010 Felipe Tanus, Boris Schaeling
// Copyright (c) 2011, 2012 Jeff Flinn, Boris Schaeling
//
//  SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_WINDOWS)
#include <hpx/components/process/util/posix/initializers/initializer_base.hpp>
#include <hpx/modules/serialization.hpp>

#include <cerrno>
#include <string>
#include <unistd.h>

namespace hpx { namespace components { namespace process { namespace posix {

    namespace initializers {

        class start_in_dir : public initializer_base
        {
        public:
            start_in_dir() {}

            explicit start_in_dir(std::string const& s)
              : s_(s)
            {
            }

            template <class PosixExecutor>
            void on_exec_setup(PosixExecutor& e) const
            {
                if (::chdir(s_.c_str()) == -1)
                {
                    e.exec_error = errno;
                    e.exec_error_origin = PosixExecutor::error_origin::chdir;
                }
            }

        private:
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive& ar, unsigned const)
            {
                ar & s_;
            }

            std::string s_;
        };

}}}}}    // namespace hpx::components::process::posix::initializers

#endif
