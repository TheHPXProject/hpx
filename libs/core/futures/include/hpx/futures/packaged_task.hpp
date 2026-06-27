//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file packaged_task.hpp
/// \page hpx::packaged_task
/// \headerfile hpx/future.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/futures/detail/future_data.hpp>
#include <hpx/futures/future_fwd.hpp>
#include <hpx/futures/promise.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/threading_base.hpp>

#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx {

    /// The class template hpx::packaged_task wraps any Callable` target
    /// (function, lambda expression, bind expression, or another function
    /// object) so that it can be invoked asynchronously. Its return value or
    /// exception thrown is stored in a shared state which can be accessed
    /// through hpx::future objects. Just like hpx::function, hpx::packaged_task
    /// is a polymorphic, allocator-aware container: the stored callable target
    /// may be allocated on heap or with a provided allocator.
    HPX_CXX_CORE_EXPORT template <typename Sig>
    class packaged_task;

    template <typename R, typename... Ts>
    class packaged_task<R(Ts...)>
    {
        using function_type = hpx::move_only_function<R(Ts...)>;

    public:
        // construction and destruction
        packaged_task() = default;

        template <typename F, typename FD = std::decay_t<F>,
            typename Enable =
                std::enable_if_t<!std::is_same_v<FD, packaged_task> &&
                    is_invocable_r_v<R, FD&, Ts...>>>
        explicit packaged_task(F&& f)
          : function_(HPX_FORWARD(F, f))
          , promise_()
        {
        }

        template <typename Allocator, typename F, typename FD = std::decay_t<F>,
            typename Enable =
                std::enable_if_t<!std::is_same_v<FD, packaged_task> &&
                    is_invocable_r_v<R, FD&, Ts...>>>
        explicit packaged_task(std::allocator_arg_t, Allocator const& a, F&& f)
          : function_(HPX_FORWARD(F, f))
          , promise_(std::allocator_arg, a)
        {
        }

        packaged_task(packaged_task const& rhs) noexcept = delete;
        packaged_task(packaged_task&& rhs) noexcept = default;

        packaged_task& operator=(packaged_task const& rhs) noexcept = delete;
        packaged_task& operator=(packaged_task&& rhs) noexcept = default;

        void swap(packaged_task& rhs) noexcept
        {
            function_.swap(rhs.function_);
            promise_.swap(rhs.promise_);
        }

        void operator()(Ts... ts)
        {
            if (function_.empty())
            {
                HPX_THROW_EXCEPTION(hpx::error::no_state,
                    "packaged_task<Signature>::operator()",
                    "this packaged_task has no valid shared state");
            }

            // synchronous execution of the embedded function (object)
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    hpx::scoped_annotation annotate(function_);
                    if constexpr (std::is_void_v<R>)
                    {
                        function_(HPX_FORWARD(Ts, ts)...);
                        promise_.set_value();
                    }
                    else
                    {
                        promise_.set_value(function_(HPX_FORWARD(Ts, ts)...));
                    }
                },
                [&](std::exception_ptr ep) {
                    promise_.set_exception(HPX_MOVE(ep));
                });
        }

        // result retrieval
        hpx::future<R> get_future(error_code& ec = throws)
        {
            if (function_.empty())
            {
                HPX_THROWS_IF(ec, hpx::error::no_state,
                    "packaged_task<Signature>::get_future",
                    "this packaged_task has no valid shared state");
                return hpx::future<R>();
            }
            return promise_.get_future();
        }

        bool valid() const noexcept
        {
            return !function_.empty() && promise_.valid();
        }

        void reset(error_code& ec = throws)
        {
            if (function_.empty())
            {
                HPX_THROWS_IF(ec, hpx::error::no_state,
                    "packaged_task<Signature>::reset",
                    "this packaged_task has no valid shared state");
                return;
            }
            promise_ = hpx::promise<R>();
        }

        // extension
        void set_exception(std::exception_ptr const& e)
        {
            promise_.set_exception(e);
        }

    private:
        function_type function_;
        hpx::promise<R> promise_;
    };
}    // namespace hpx

namespace std {    //-V1061

    // Requires: Allocator shall be an allocator (17.6.3.5)
    template <typename Sig, typename Allocator>
    struct uses_allocator<hpx::packaged_task<Sig>, Allocator> : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Sig>
    void swap(
        hpx::packaged_task<Sig>& lhs, hpx::packaged_task<Sig>& rhs) noexcept
    {
        lhs.swap(rhs);
    }
}    // namespace std
