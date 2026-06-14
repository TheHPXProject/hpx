//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file dataflow.hpp
/// \page hpx::dataflow
/// \headerfile hpx/future.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {

    /// The function template \a dataflow runs the function f asynchronously
    /// (potentially in a separate thread which might be a part of a thread
    /// pool) and returns a \c hpx::future that will eventually hold the result
    /// of that function call. Its behavior is similar to \c hpx::async with the
    /// exception that if one of the arguments is a \a future, then
    /// \c hpx::dataflow will wait for the \a future to be ready to launch the
    /// thread. Hence, the operation is delayed until all the arguments are
    /// ready.
    template <typename F, typename... Ts>
    decltype(auto) dataflow(F&& f, Ts&&... ts);
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/concurrency.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace detail {

        // This CPO must live in hpx::detail for now as the dataflow function
        // supports being invoked in two forms: dataflow<Action>(...) and
        // dataflow(Action{}, ...). The only way to support both syntaxes (for
        // the time being, we will deprecate dataflow<Action>()) is to have a
        // real function based API that dispatches to the CPO. Once
        // dataflow<Action>(...) has been removed, this CPO can be moved to
        // namespace hpx.
        // Customization struct specialized in the heavy header
        // (hpx/executors/dataflow.hpp)
        template <typename F, typename Enable = void>
        struct dataflow_dispatch_impl;

        HPX_CXX_CORE_EXPORT inline constexpr struct dataflow_t final
        {
            // Primary: target has .dataflow() member
            template <typename Target, typename... Args>
                requires requires(Target&& t, Args&&... args) {
                    HPX_FORWARD(Target, t).dataflow(HPX_FORWARD(Args, args)...);
                }
            constexpr auto operator()(Target&& target, Args&&... args) const
            {
                return HPX_FORWARD(Target, target)
                    .dataflow(HPX_FORWARD(Args, args)...);
            }

            // Non-member dispatch (handles both allocator and non-allocator)
            template <typename F, typename... Ts>
                requires(!requires(F&& f, Ts&&... ts) {
                    HPX_FORWARD(F, f).dataflow(HPX_FORWARD(Ts, ts)...);
                })
            constexpr HPX_FORCEINLINE decltype(auto) operator()(
                F&& f, Ts&&... ts) const
            {
                if constexpr (hpx::traits::is_allocator_v<std::decay_t<F>>)
                {
                    return dispatch_with_allocator(
                        HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
                }
                else
                {
                    using allocator_type =
                        hpx::util::thread_local_caching_allocator<
                            hpx::lockfree::variable_size_stack,
                            hpx::util::internal_allocator<>>;
                    return dataflow_dispatch_impl<std::decay_t<F>>::call(
                        allocator_type{}, HPX_FORWARD(F, f),
                        HPX_FORWARD(Ts, ts)...);
                }
            }

        private:
            template <typename Allocator, typename Func, typename... Rest>
            constexpr HPX_FORCEINLINE decltype(auto) dispatch_with_allocator(
                Allocator&& alloc, Func&& func, Rest&&... rest) const
            {
                return dataflow_dispatch_impl<std::decay_t<Func>>::call(
                    HPX_FORWARD(Allocator, alloc), HPX_FORWARD(Func, func),
                    HPX_FORWARD(Rest, rest)...);
            }
        } dataflow{};
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    HPX_FORCEINLINE decltype(auto) dataflow(F&& f, Ts&&... ts)
    {
        return hpx::detail::dataflow(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
    }

    template <typename Allocator, typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::dataflow_alloc is deprecated, use hpx::dataflow instead")
    decltype(auto) dataflow_alloc(Allocator const& alloc, Ts&&... ts)
    {
        return hpx::detail::dataflow(alloc, HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx

#endif
