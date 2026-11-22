//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_COMPRESSION_BZIP2)

#include <hpx/modules/parcelset_base.hpp>

///////////////////////////////////////////////////////////////////////////////
#define HPX_ACTION_USES_BZIP2_COMPRESSION(action)                              \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct action_serialization_filter</**/ action>                        \
        {                                                                      \
            /* Note that the caller is responsible for deleting the filter */  \
            /* instance returned from this function */                         \
            static serialization::binary_filter* call()                        \
            {                                                                  \
                return ::hpx::create_binary_filter(                            \
                    "bzip2_serialization_filter", true);                       \
            }                                                                  \
        };                                                                     \
    }                                                                          \
    /**/

#else

#define HPX_ACTION_USES_BZIP2_COMPRESSION(action)

#endif
