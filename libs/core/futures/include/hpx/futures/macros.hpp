//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

// from hpx/futures/future.hpp
#define HPX_MAKE_EXCEPTIONAL_FUTURE(T, errorcode, f, msg)                      \
    ::hpx::make_exceptional_future<T>(HPX_GET_EXCEPTION(errorcode, f, msg)) /**/
