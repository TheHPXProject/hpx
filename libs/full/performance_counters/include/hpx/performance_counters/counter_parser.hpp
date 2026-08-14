//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <string>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

    HPX_CXX_EXPORT struct instance_name
    {
        std::string name_;
        std::string index_;
        bool basename_ = false;
    };

    HPX_CXX_EXPORT struct instance_elements
    {
        instance_name parent_;
        instance_name child_;
        instance_name subchild_;
    };

    HPX_CXX_EXPORT struct path_elements
    {
        std::string object_;
        instance_elements instance_;
        std::string counter_;
        std::string parameters_;
    };

    HPX_CXX_EXPORT HPX_EXPORT bool parse_counter_name(
        std::string const& name, path_elements& elements);
}    // namespace hpx::performance_counters
