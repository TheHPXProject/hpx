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
#include <hpx/modules/string_util.hpp>

#include <memory>

#include <cstddef>
#include <string>
#include <vector>

namespace hpx::components::process::posix::initializers {

    class set_cmd_line : public initializer_base
    {
    public:
        explicit set_cmd_line(std::string const& s)
        {
            split_command_line(s);
            init_command_line_arguments();
        }

        template <class PosixExecutor>
        void on_exec_setup(PosixExecutor& e) const
        {
            e.cmd_line = cmd_line_.get();
        }

    private:
        void split_command_line(std::string const& s)
        {
            typedef hpx::string_util::tokenizer<
                hpx::string_util::escaped_list_separator<char>>
                tokenizer;
            hpx::string_util::escaped_list_separator<char> sep('\\', ' ', '\"');
            tokenizer tok(s, sep);
            args_.assign(tok.begin(), tok.end());
        }

        void init_command_line_arguments()
        {
            cmd_line_.reset(new char*[args_.size() + 1]);
            std::ptrdiff_t i = 0;
            for (std::string const& s : args_)
                cmd_line_[i++] = const_cast<char*>(s.c_str());
            cmd_line_[i] = nullptr;
        }

        friend class hpx::serialization::access;

        template <typename Archive>
        void save(Archive& ar, unsigned const) const
        {
            ar & args_;
        }

        template <typename Archive>
        void load(Archive& ar, unsigned int const)
        {
            ar & args_;
            init_command_line_arguments();
        }

        HPX_SERIALIZATION_SPLIT_MEMBER()

        std::vector<std::string> args_;
        std::shared_ptr<char*[]> cmd_line_;
    };

}    // namespace hpx::components::process::posix::initializers

#endif
