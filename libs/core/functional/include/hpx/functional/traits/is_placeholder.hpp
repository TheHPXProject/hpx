//  Copyright (c) 2013-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// make inspect happy: hpxinspect:nodeprecatedname:boost::is_placeholder

/// \file is_placeholder.hpp
/// \page hpx::is_placeholder
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/config/defines.hpp>

#if defined(HPX_FUNCTIONAL_HAVE_BOOST_PLACEHOLDERS)
#include <boost/bind/arg.hpp>
#endif

#include <functional>
#include <type_traits>

namespace hpx {
#if defined(DOXYGEN)
    /// \brief If \c T is a standard, Boost, or HPX placeholder (_1, _2, _3, ...)
    ///        then this template is derived from \c std::integral_constant<int,1>,
    ///        \c std::integral_constant<int,2>, \c std::integral_constant<int,3>,
    ///        respectively. Otherwise, it is derived from
    ///        \c std::integral_constant<int,0>.
    /// \details The template may be specialized for any user-defined \c T type: the
    ///          specialization must satisfy \a UnaryTypeTrait with base characteristic
    ///          of \c std::integral_constant<int,N> with N>0 to indicate that
    ///          \c T should be treated as N'th placeholder type.
    ///          \c hpx::bind uses \c hpx::is_placeholder to detect placeholders for
    ///          unbound arguments.
    template <typename T>
    struct is_placeholder;
#else

#if defined(HPX_FUNCTIONAL_HAVE_BOOST_PLACEHOLDERS)
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_placeholder
      : std::integral_constant<int,
            std::is_placeholder_v<T> != 0 ? std::is_placeholder_v<T> :
                                            boost::is_placeholder<T>::value>
    {
    };
#else
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_placeholder : std::is_placeholder<T>
    {
    };
#endif

    template <typename T>
    struct is_placeholder<T const> : is_placeholder<T>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr int is_placeholder_v = is_placeholder<T>::value;
#endif
}    // namespace hpx
