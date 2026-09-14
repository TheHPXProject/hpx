//  Copyright (c) 2026 Abhishek Kumar
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file locality_disconnected.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <cstdint>
#include <exception>
#include <source_location>

namespace hpx::detail {
    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    // The dispatch guard below is duplicated across all of the async/post/sync
    // implementations. Centralizing it here keeps the (single) preprocessor
    // gate in one place and makes sure all call sites report the same
    // diagnostics.
    //
    // Note that hpx::parcelset::locality_was_disconnected is declared only if
    // networking is enabled, while HPX_HAVE_FORCE_DISCONNECT does not imply
    // HPX_HAVE_NETWORKING. Both conditions are therefore tested here.

    /// Returns whether the given locality has been forcefully disconnected.
    /// Folds into a constant \a false if the corresponding functionality has
    /// not been enabled.
    HPX_FORCEINLINE bool locality_is_disconnected(
        [[maybe_unused]] std::uint32_t const locality_id)
    {
#if defined(HPX_HAVE_FORCE_DISCONNECT) && defined(HPX_HAVE_NETWORKING)
        return parcelset::locality_was_disconnected(locality_id);
#else
        return false;
#endif
    }

    /// \copydoc locality_is_disconnected(std::uint32_t)
    HPX_FORCEINLINE bool locality_is_disconnected(hpx::id_type const& id)
    {
        return locality_is_disconnected(naming::get_locality_id_from_id(id));
    }

    ///////////////////////////////////////////////////////////////////////////
    // The two helpers below report the location of their caller, just like
    // HPX_THROW_EXCEPTION and HPX_GET_EXCEPTION do for their expansion site.
    // The reported function defaults to the caller's own name; pass \a func
    // explicitly to override it.

    /// Throw hpx::error::locality_was_disconnected on behalf of the caller.
    HPX_CXX_EXPORT [[noreturn]] HPX_EXPORT void throw_locality_disconnected(
        hpx::id_type const& id, char const* func = nullptr,
        std::source_location const& location = std::source_location::current());

    /// Return an exception_ptr referring to
    /// hpx::error::locality_was_disconnected on behalf of the caller.
    HPX_CXX_EXPORT [[nodiscard]] HPX_EXPORT std::exception_ptr
    get_locality_disconnected_exception(hpx::id_type const& id,
        char const* func = nullptr,
        std::source_location const& location = std::source_location::current());

    /// \endcond
}    // namespace hpx::detail
