//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/for_loop_induction.hpp
/// \page hpx::experimental::induction
/// \headerfile hpx/algorithm.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/modules/execution.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

/// \cond NOINTERNAL
namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct induction_helper
    {
        explicit constexpr induction_helper(T var) noexcept
          : var_(var)
          , curr_(var)
        {
        }

        HPX_HOST_DEVICE
        constexpr void init_iteration(
            std::size_t index, std::size_t /*current_thread*/) noexcept
        {
            curr_ = parallel::detail::next(var_, index);
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T const& iteration_value(
            std::size_t /*current_thread*/) const noexcept
        {
            return curr_;
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void next_iteration(
            std::size_t /*current_thread*/) noexcept
        {
            ++curr_;
        }

        HPX_HOST_DEVICE
        static constexpr void exit_iteration(std::size_t /*index*/) noexcept {}

    private:
        std::decay_t<T> var_;
        T curr_;
    };

    template <typename T>
    struct induction_helper<T&>
    {
        explicit constexpr induction_helper(T& var) noexcept
          : live_out_var_(var)
          , var_(var)
          , curr_(var)
        {
        }

        HPX_HOST_DEVICE
        constexpr void init_iteration(
            std::size_t index, std::size_t /*current_thread*/) noexcept
        {
            curr_ = parallel::detail::next(var_, index);
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T const& iteration_value(
            std::size_t /*current_thread*/) const noexcept
        {
            return curr_;
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void next_iteration(
            std::size_t /*current_thread*/) noexcept
        {
            ++curr_;
        }

        HPX_HOST_DEVICE
        constexpr void exit_iteration(std::size_t index) noexcept
        {
            live_out_var_ = parallel::detail::next(live_out_var_, index);
        }

    private:
        T& live_out_var_;
        T var_;
        T curr_;
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct induction_stride_helper
    {
        constexpr induction_stride_helper(T var, std::size_t stride) noexcept
          : var_(var)
          , curr_(var)
          , stride_(stride)
        {
        }

        HPX_HOST_DEVICE
        constexpr void init_iteration(
            std::size_t index, std::size_t /*current_thread*/) noexcept
        {
            curr_ = parallel::detail::next(var_, stride_ * index);
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T const& iteration_value(
            std::size_t /*current_thread*/) const noexcept
        {
            return curr_;
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void next_iteration(
            std::size_t /*current_thread*/) noexcept
        {
            curr_ = parallel::detail::next(curr_, stride_);
        }

        HPX_HOST_DEVICE
        static constexpr void exit_iteration(std::size_t /*index*/) noexcept {}

    private:
        std::decay_t<T> var_;
        T curr_;
        std::size_t stride_;
    };

    template <typename T>
    struct induction_stride_helper<T&>
    {
        constexpr induction_stride_helper(T& var, std::size_t stride) noexcept
          : live_out_var_(var)
          , var_(var)
          , curr_(var)
          , stride_(stride)
        {
        }

        HPX_HOST_DEVICE
        constexpr void init_iteration(
            std::size_t index, std::size_t /*current_thread*/) noexcept
        {
            curr_ = parallel::detail::next(var_, stride_ * index);
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T const& iteration_value(
            std::size_t /*current_thread*/) const noexcept
        {
            return curr_;
        }

        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void next_iteration(
            std::size_t /*current_thread*/) noexcept
        {
            curr_ = parallel::detail::next(curr_, stride_);
        }

        HPX_HOST_DEVICE
        constexpr void exit_iteration(std::size_t index) noexcept
        {
            live_out_var_ =
                parallel::detail::next(live_out_var_, stride_ * index);
        }

    private:
        T& live_out_var_;
        T var_;
        T curr_;
        std::size_t stride_;
    };
}    // namespace hpx::parallel::detail
/// \endcond

namespace hpx::experimental {

    /// The function template returns an induction object of unspecified type
    /// having a value type and encapsulating an initial \a value of that type
    /// and, optionally, a stride.
    ///
    /// For each element in the input range, a looping algorithm over input
    /// sequence \a S computes an induction value from an induction variable and
    /// ordinal position \a p within \a S by the formula i + p * stride if a
    /// stride was specified or i + p otherwise. This induction value is passed
    /// to the element access function.
    ///
    /// If the \a value argument to \a induction is a non-const lvalue, then
    /// that lvalue becomes the live-out object for the returned induction
    /// object. For each induction object that has a live-out object, the
    /// looping algorithm assigns the value of i + n * stride to the live-out
    /// object upon return, where \a n is the number of elements in the input
    /// range.
    ///
    /// \tparam T       The value type to be used by the induction object.
    ///
    /// \param value    [in] The initial value to use for the induction object
    /// \param stride   [in] The (optional) stride to use for the induction
    ///                 object (default: 1)
    ///
    /// \returns This returns an induction object with value type \a T, initial
    ///          \a value, and (if specified) \a stride. If \a T is a lvalue of
    ///          non-const type, \a value is used as the live-out object for the
    ///          induction object; otherwise there is no live-out object.
    ///
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE constexpr hpx::parallel::detail::induction_stride_helper<T>
    induction(T&& value, std::size_t stride)
    {
        return hpx::parallel::detail::induction_stride_helper<T>(
            HPX_FORWARD(T, value), stride);
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_FORCEINLINE constexpr hpx::parallel::detail::induction_helper<T>
    induction(T&& value)
    {
        return hpx::parallel::detail::induction_helper<T>(
            HPX_FORWARD(T, value));
    }
}    // namespace hpx::experimental
/// \endcond
