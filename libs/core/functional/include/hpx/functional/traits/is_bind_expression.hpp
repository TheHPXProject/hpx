//  Copyright (c) 2013-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file is_bind_expression.hpp
/// \page hpx::is_bind_expression
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>

#include <functional>

namespace hpx {

    /// \brief If \c T is the type produced by a call to \c hpx::bind, this
    ///        template is derived from \c std::true_type. For any other type,
    ///        this template is derived from \c std::false_type.
    ///
    /// \details This template may be specialized for a user-defined type \c T
    ///          to implement \a UnaryTypeTrait with base characteristic of
    ///          \c std::true_type to indicate that \c T should be treated by
    ///          \c hpx::bind as if it were the type of a bind subexpression:
    ///          when a bind-generated function object is invoked, a bound
    ///          argument of this type will be invoked as a function object and
    ///          will be given all the unbound arguments passed to the
    ///          bind-generated object.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_bind_expression : std::is_bind_expression<T>
    {
    };

    template <typename T>
    struct is_bind_expression<T const> : is_bind_expression<T>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_bind_expression_v = is_bind_expression<T>::value;
}    // namespace hpx
