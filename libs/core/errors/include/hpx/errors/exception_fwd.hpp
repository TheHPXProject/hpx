//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file exception_fwd.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/errors/error.hpp>

#include <cstdint>
#include <exception>
#include <string>

namespace hpx {

    /// \cond NOINTERNAL
    // forward declaration
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT error_code;

    HPX_CXX_CORE_EXPORT class HPX_ALWAYS_EXPORT exception;

    HPX_CXX_CORE_EXPORT struct HPX_ALWAYS_EXPORT thread_interrupted;

    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT exception_list;
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Encode error category for new error_code.
    HPX_CXX_CORE_EXPORT enum class throwmode : std::uint8_t {
        plain = 0,
        rethrow = 1,

        // do not generate an exception for this error_code
        lightweight = 0x80,

        /// \cond NODETAIL
        lightweight_rethrow = 0x81    // lightweight | rethrow
        /// \endcond
    };

    // 26827: Did you forget to initialize an enum constant, or intend to use
    // another type?.
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26827)
#endif
    HPX_CXX_CORE_EXPORT constexpr bool operator&(
        throwmode lhs, throwmode rhs) noexcept
    {
        return static_cast<int>(lhs) & static_cast<int>(rhs);
    }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Predefined error_code object used as "throw on error" tag.
    ///
    /// The predefined hpx::error_code object \a hpx::throws is supplied for use
    /// as a "throw on error" tag.
    ///
    /// Functions that specify an argument in the form 'error_code& ec=throws'
    /// (with appropriate namespace qualifiers), have the following error
    /// handling semantics:
    ///
    /// If &ec != &throws and an error occurred: ec.value() returns the
    /// implementation specific error number for the particular error that
    /// occurred and ec.category() returns the error_category for ec.value().
    ///
    /// If &ec != &throws and an error did not occur, ec.clear().
    ///
    /// If an error occurs and &ec == &throws, the function throws an exception
    /// of type \a hpx::exception or of a type derived from it. The exception's
    /// \a get_errorcode() member function returns a reference to a
    /// \a hpx::error_code object with the behavior as specified above.
    ///
#if defined(HPX_COMPUTE_DEVICE_CODE) && !defined(HPX_HAVE_HIP)
    // We can't actually refer to this in device code. This is only to satisfy
    // the compiler.
    extern HPX_DEVICE error_code throws;
#else
    HPX_CXX_CORE_EXPORT extern HPX_CORE_EXPORT error_code throws;
#endif

    /// \cond NOINTERNAL
    namespace detail {

        ////////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Exception>
        [[nodiscard]] HPX_CORE_EXPORT std::exception_ptr
        construct_lightweight_exception(Exception const& e);

        extern template HPX_CORE_EXPORT std::exception_ptr
        construct_lightweight_exception(hpx::exception_list const&);
        extern template HPX_CORE_EXPORT std::exception_ptr
        construct_lightweight_exception(hpx::thread_interrupted const&);
    }    // namespace detail
    /// \endcond
}    // namespace hpx

#include <hpx/errors/throw_exception.hpp>
