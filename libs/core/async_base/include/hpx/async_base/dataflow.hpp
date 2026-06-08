//  Copyright (c) 2007-2025 Hartmut Kaiser
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
#include <hpx/modules/tag_invoke.hpp>

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
        HPX_CXX_CORE_EXPORT inline constexpr struct dataflow_t final
          : hpx::functional::detail::tag_fallback<dataflow_t>
        {
        private:
            template <typename Target, typename... Args>
                requires requires(Target&& t, Args&&... args) {
                    HPX_FORWARD(Target, t).dataflow(HPX_FORWARD(Args, args)...);
                }
            friend constexpr auto tag_invoke(
                dataflow_t, Target&& target, Args&&... args)
            {
                return HPX_FORWARD(Target, target)
                    .dataflow(HPX_FORWARD(Args, args)...);
            }

            template <typename F, typename... Ts,
                HPX_CONCEPT_REQUIRES_(
                    !hpx::traits::is_allocator_v<std::decay_t<F>>)>
            friend constexpr HPX_FORCEINLINE auto tag_fallback_invoke(
                dataflow_t tag, F&& f, Ts&&... ts)
                -> decltype(hpx::functional::tag_invoke(tag,
                    hpx::util::thread_local_caching_allocator<
                        hpx::lockfree::variable_size_stack,
                        hpx::util::internal_allocator<>>{},
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                using allocator_type =
                    hpx::util::thread_local_caching_allocator<
                        hpx::lockfree::variable_size_stack,
                        hpx::util::internal_allocator<>>;
                return hpx::functional::tag_invoke(tag, allocator_type{},
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
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
