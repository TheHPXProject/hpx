//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_HAVE_CXX26_REFLECTION)

#include <hpx/modules/preprocessor.hpp>
#include <hpx/runtime_components/reflect_client.hpp>

/// \brief Generates a reflection-based client for a component server.
///
/// Invoke at namespace scope after the server definition:
/// \code
///   HPX_CLIENT(compute_server)
///
///   // Instantiate component and get client:
///   auto c = hpx::components::make_client<compute_server>(locality);
///   auto f = c.add(42);   // async, returns future<int>
/// \endcode
///
/// The generated client type is hpx::components::reflect_client<Server>.
#define HPX_CLIENT(Server)                                                     \
    consteval                                                                  \
    {                                                                          \
        ::std::meta::define_aggregate(                                         \
            ^^::hpx::components::reflect_client<Server>,                       \
            ::hpx::components::detail::create_client_data_members<             \
                ^^Server>());                                                  \
    } /**/

#endif    // !HPX_COMPUTE_DEVICE_CODE && HPX_HAVE_CXX26_REFLECTION
