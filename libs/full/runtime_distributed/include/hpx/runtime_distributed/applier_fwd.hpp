
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file applier_fwd.hpp

#pragma once

#include <hpx/config.hpp>

namespace hpx {

    /// \namespace applier
    ///
    /// The namespace \a applier contains all definitions needed for the
    /// class \a hpx#applier#applier and its related functionality. This
    /// namespace is part of the HPX core module.
    namespace applier {

        HPX_CXX_EXPORT class HPX_EXPORT applier;

        /// The function \a get_applier returns a reference to the (thread
        /// specific) applier instance.
        HPX_CXX_EXPORT HPX_EXPORT applier& get_applier();

        /// The function \a get_applier returns a pointer to the (thread
        /// specific) applier instance. The returned pointer is NULL if the
        /// current thread is not known to HPX or if the runtime system is not
        /// active.
        HPX_CXX_EXPORT HPX_EXPORT applier* get_applier_ptr();
    }    // namespace applier
}    // namespace hpx
