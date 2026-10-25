//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/for_loop_reduction_max.hpp
/// \page hpx::experimental::reduction_max
/// \headerfile hpx/algorithm.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/parallel/algorithms/for_loop_reduction.hpp>

namespace hpx::experimental {

    /// The function template \a reduction_max returns a reduction object of
    /// unspecified type having a value type and encapsulating an identity value
    /// for the reduction, it uses std::max_of{} as its combiner function, and a
    /// live-out object from which the initial value is obtained and into which
    /// the final value is stored.
    ///
    /// A parallel algorithm uses the \a reduction_max object by allocating an
    /// unspecified number of instances, called views, of the reduction's value
    /// type. Each view is initialized with the reduction object's identity
    /// value, except that the live-out object (which was initialized by the
    /// caller) comprises one of the views. The algorithm passes a reference to
    /// a view to each application of an element-access function, ensuring that
    /// no two concurrently-executing invocations share the same view. A view
    /// can be shared between two applications that do not execute concurrently,
    /// but initialization is performed only once per view.
    ///
    /// Modifications to the view by the application of element access functions
    /// accumulate as partial results. At some point before the algorithm
    /// returns, the partial results are combined, two at a time, using the
    /// std::max_of{} operation until a single value remains, which is then
    /// assigned back to the live-out object.
    ///
    /// \tparam T       The value type to be used by the induction object.
    /// \tparam Op      The type of the binary function (object) used to
    ///                 perform the reduction operation.
    ///
    /// \param var      [in,out] The life-out value to use for the reduction
    ///                 object. This will hold the reduced value after the
    ///                 algorithm is finished executing.
    /// The optional \a identity value is used for the reduction operation and
    /// defaults to a copy of \a var.
    ///
    /// T shall meet the requirements of \a CopyConstructible and
    /// \a MoveAssignable.
    ///
    /// \note In order to produce useful results, modifications to the view
    ///       should be limited to commutative operations closely related to the
    ///       combiner operation.
    ///
    /// \returns This returns a reduction object of unspecified type having a
    ///          value type of \a T. When the return value is used by an
    ///          algorithm, the reference to \a var is used as the live-out
    ///          object, new views are initialized to a copy of identity, and
    ///          views are combined by invoking the copy of combiner, passing it
    ///          the two views to be combined.
    ///
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE constexpr hpx::parallel::detail::reduction_helper<T,
        hpx::parallel::detail::max_of<T>>
    reduction_max(T& var)
    {
        return reduction(var, var, hpx::parallel::detail::max_of<T>());
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE constexpr hpx::parallel::detail::reduction_helper<T,
        hpx::parallel::detail::max_of<T>>
    reduction_max(T& var, T const& identity)
    {
        return reduction(var, identity, hpx::parallel::detail::max_of<T>());
    }
}    // namespace hpx::experimental
