//  Copyright (c) 2022 Gregor Daiss
//  Copyright (c) 2024-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_sycl/sycl_future.hpp>
#include <hpx/include/post.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::sycl::experimental {
    namespace detail {
        /// type trait to identify basic sycl queue args (as in: no sycl indices / callable kernels)
        /// Required for correct overloading later on
        template <class T>
        struct is_basic_queue_arg
          : std::integral_constant<bool,
                std::is_scalar<T>::value ||
                    std::is_same<T, ::sycl::event>::value ||
                    std::is_same<T, std::vector<::sycl::event> const&>::value>
        {
        };
    }    // namespace detail

    struct sycl_executor
    {
        using future_type = hpx::future<void>;

        // --------------------------------------------------------------------
        /// Create a SYCL executor (based on a sycl queue)
        template <typename sycl_device_selector>
        explicit sycl_executor(sycl_device_selector const& selector)
          : command_queue(selector, ::sycl::property::queue::in_order{})
        {
        }

        // TODO Future work: Add more constructors as needed (in case we need
        // different queue constructors)
        // --------------------------------------------------------------------
        // Queue will be cleaned up by its own destructor
        ~sycl_executor() = default;

        /// Get future for this command_queue (NOTE will be more efficient if
        /// an event is provided -- otherwise a dummy kernel must be submitted
        /// to get an event)
        HPX_FORCEINLINE future_type get_future()
        {
            return detail::get_future(command_queue);
        }

        /// Get future for that becomes ready when the given event completes
        HPX_FORCEINLINE future_type get_future(::sycl::event event)
        {
            return detail::get_future(event);
        }

        /// Default Implementation without the extra intel code_location
        /// parameter. Removes the reference for trivial types to make the
        /// function matching easier (see sycl_stream.cpp test)
        template <typename... Params>
        using queue_function_ptr_t = ::sycl::event (::sycl::queue::*)(
            std::conditional_t<
                std::is_trivial_v<std::remove_reference_t<Params>>,
                std::decay_t<Params>, Params>...);
        /// Invoke queue member function given queue and parameters -- do not
        /// use event to return a hpx::future (One way)
        template <typename... Params>
        void post(queue_function_ptr_t<Params...>&& queue_member_function,
            Params&&... args)
        {
            ::sycl::event e =
                HPX_INVOKE(HPX_FORWARD(queue_function_ptr_t<Params...>,
                               queue_member_function),
                    command_queue, HPX_FORWARD(Params, args)...);
        }
        /// Invoke queue member function given queue and parameters --
        /// hpx::future tied to the sycl event / (two way)
        template <typename... Params>
        hpx::future<void> async_execute(
            queue_function_ptr_t<Params...>&& queue_member_function,
            Params&&... args)
        {
            // launching a sycl member function may throw -- if it does put it
            // into the future
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    ::sycl::event e =
                        HPX_INVOKE(HPX_FORWARD(queue_function_ptr_t<Params...>,
                                       queue_member_function),
                            command_queue, HPX_FORWARD(Params, args)...);

#if defined(__HIPSYCL__)
                    // TODO This overload works better for hipsycl -- checkout
                    // why! Apparently, having a subsequent dummy kernel start
                    // via get_future(), causes hipsycl to clean up the
                    // previous kernel once done. Without this buffers seem to
                    // leak as hipsycl does not clean up the device memory
                    // without some trigger flush.  In any case, the problems
                    // seems to be avoided by always having a following dummy
                    // kernel within the hipsycl dag. Runtime of this is about
                    // 5us, so not too bad. Still keeping this as a ToDo, might
                    // be a hipsycl issue...
                    //
                    // NOTE 1: Alternative workaround: use environment
                    // variable: export HIPSYCL_RT_SCHEDULER=direct This
                    // directly circumvents the problematic scheduler, allowing
                    // us to also use get_future(e)
                    //
                    // NOTE2 : Not a problem with oneapi in either case from
                    // what I can see
                    return get_future();
#else
                    return get_future(e);
#endif
                },
                [&](std::exception_ptr&& ep) {
                    return hpx::make_exceptional_future<void>(HPX_MOVE(ep));
                });
        }

//#if defined(__LIBSYCL_MAJOR_VERSION) && defined(__LIBSYCL_MINOR_VERSION)
#if defined(__INTEL_LLVM_COMPILER) ||                                          \
    (defined(__clang__) && defined(SYCL_IMPLEMENTATION_ONEAPI))
        // To find the correct overload (or any at all actually) we need to add
        // the code_location argument which is the last argument in every queue
        // member function in the intel oneapi sycl implementation.  As far as
        // I can tell it is usually invisible from the user-side since it is
        // using a default argument (code_location::current())

        /// sycl::queue::member_function type with code_location parameter
        template <typename... Params>
        using queue_function_code_loc_ptr_t = ::sycl::event (::sycl::queue::*)(
            std::conditional_t<
                std::is_trivial_v<std::remove_reference_t<Params>>,
                std::decay_t<Params>, Params>...,
            ::sycl::detail::code_location const&);

        /// Invoke member function given queue and parameters. Default
        /// code_location argument added automatically.
        template <typename... Params>
        void post(
            queue_function_code_loc_ptr_t<Params...>&& queue_member_function,
            Params&&... args)
        {
            // for the intel version we need to actually pass the code
            // location.  Within the intel sycl api this is usually a default
            // argument, but for invoke we need to pass it manually
            ::sycl::event e =
                HPX_INVOKE(HPX_FORWARD(queue_function_code_loc_ptr_t<Params...>,
                               queue_member_function),
                    command_queue, HPX_FORWARD(Params, args)...,
                    ::sycl::detail::code_location::current());
        }
        /// Invoke queue member function given queue and parameters. Default
        /// code_location argument added automatically.  / Returns hpx::future
        /// tied to the sycl event returned by the asynchronous queue member
        /// function call (two way)
        template <typename... Params>
        hpx::future<void> async_execute(
            queue_function_code_loc_ptr_t<Params...>&& queue_member_function,
            Params&&... args)
        {
            // launching a sycl member function may throw -- if it does put it
            // into the future
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    ::sycl::event e = HPX_INVOKE(
                        HPX_FORWARD(queue_function_code_loc_ptr_t<Params...>,
                            queue_member_function),
                        command_queue, HPX_FORWARD(Params, args)...,
                        ::sycl::detail::code_location::current());
                    return get_future(e);
                },
                [&](std::exception_ptr&& ep) {
                    return hpx::make_exceptional_future<void>(HPX_MOVE(ep));
                });
        }
#endif

        // OneWay Execution and TwoWay Execution are provided by
        // the post() and async_execute() member functions above,
        // which are detected directly by the CPOs.

        // Property interface:

        /// Return the device used by the underlying SYCL queue
        HPX_FORCEINLINE ::sycl::device get_device() const
        {
            return command_queue.get_device();
        }
        /// Return the context used by the underlying SYCL queue
        HPX_FORCEINLINE ::sycl::context get_context() const
        {
            return command_queue.get_context();
        }
        /// Return the underlying queue for direct access
        HPX_FORCEINLINE ::sycl::queue& get_queue()
        {
            return command_queue;
        }

        // TODO Future work: Check if we want to expose any other (non-event)
        // queue methods
    protected:
        ::sycl::queue command_queue;
    };
}    // namespace hpx::sycl::experimental

namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    template <>
    struct is_one_way_executor<hpx::sycl::experimental::sycl_executor>
      : std::true_type
    {
        // support for fire and forget without returning a waitable/future
    };

    template <>
    struct is_two_way_executor<hpx::sycl::experimental::sycl_executor>
      : std::true_type
    {
        // support for a waitable/future
    };
    /// \endcond
}    // namespace hpx::execution::experimental

namespace hpx {

    // Add overloads for apply and async to help the compiler determine the
    // correct sycl queue member function/overload by passing it the types for
    // subsequent arguments.  Without this, the user would have to specify the
    // exact type for the queue_function_ptr on his/her own which can be
    // annoying, especially when the type depends on a kernel lambda (as it
    // does for example in the submit member function).

    /// hpx::async overload for launching sycl queue member functions with an
    /// sycl executor
#if defined(__INTEL_LLVM_COMPILER) ||                                          \
    (defined(__clang__) && defined(SYCL_IMPLEMENTATION_ONEAPI))
    template <typename Executor, typename... Ts,
        std::enable_if_t<!(std::conjunction_v<sycl::experimental::detail::
                                 is_basic_queue_arg<Ts>...>),
            bool>
            guard = true>
#else
    template <typename Executor, typename... Ts>
#endif
    HPX_FORCEINLINE decltype(auto) async(Executor&& exec,
        hpx::sycl::experimental::sycl_executor::queue_function_ptr_t<Ts...>&& f,
        Ts&&... ts)
    {
        // Make sure we only use this for sycl executors
        static_assert(std::is_same<std::decay_t<Executor>,
            hpx::sycl::experimental::sycl_executor>::value);
        // Use the same async_dispatch than the normal async otherwise
        return detail::async_dispatch<
            typename std::decay<Executor>::type>::call(HPX_FORWARD(Executor,
                                                           exec),
            HPX_FORWARD(
                hpx::sycl::experimental::sycl_executor::queue_function_ptr_t<
                    Ts...>,
                f),
            HPX_FORWARD(Ts, ts)...);
    }

    /// hpx::apply overload for launching sycl queue member functions with an
    /// sycl executor
#if defined(__INTEL_LLVM_COMPILER) ||                                          \
    (defined(__clang__) && defined(SYCL_IMPLEMENTATION_ONEAPI))
    template <typename Executor, typename... Ts,
        std::enable_if_t<!(std::conjunction_v<sycl::experimental::detail::
                                 is_basic_queue_arg<Ts>...>),
            bool>
            guard = true>
#else
    template <typename Executor, typename... Ts>
#endif
    HPX_FORCEINLINE bool apply(Executor&& exec,
        hpx::sycl::experimental::sycl_executor::queue_function_ptr_t<Ts...>&& f,
        Ts&&... ts)
    {
        // Make sure we only use this for sycl executors
        static_assert(std::is_same<std::decay_t<Executor>,
            hpx::sycl::experimental::sycl_executor>::value);
        // Use the same apply_dispatch than the normal apply otherwise
        return detail::post_dispatch<typename std::decay<Executor>::type>::call(
            HPX_FORWARD(Executor, exec),
            HPX_FORWARD(
                hpx::sycl::experimental::sycl_executor::queue_function_ptr_t<
                    Ts...>,
                f),
            HPX_FORWARD(Ts, ts)...);
    }

#if defined(__INTEL_LLVM_COMPILER) ||                                          \
    (defined(__clang__) && defined(SYCL_IMPLEMENTATION_ONEAPI))
    /// hpx::async overload for launching sycl queue member functions with an
    /// sycl executor and code location ptrs
    template <typename Executor, typename... Ts,
        std::enable_if_t<(std::conjunction_v<sycl::experimental::detail::
                                 is_basic_queue_arg<Ts>...>),
            bool>
            guard = true>
    HPX_FORCEINLINE decltype(auto) async(Executor&& exec,
        hpx::sycl::experimental::sycl_executor::queue_function_code_loc_ptr_t<
            Ts...>&& f,
        Ts&&... ts)
    {
        // Make sure we only use this for sycl executors
        static_assert(std::is_same<std::decay_t<Executor>,
            hpx::sycl::experimental::sycl_executor>::value);
        // Use the same async_dispatch than the normal async otherwise
        return detail::async_dispatch<
            typename std::decay<Executor>::type>::call(HPX_FORWARD(Executor,
                                                           exec),
            HPX_FORWARD(hpx::sycl::experimental::sycl_executor::
                            queue_function_code_loc_ptr_t<Ts...>,
                f),
            HPX_FORWARD(Ts, ts)...);
    }

    /// hpx::apply overload for launching sycl queue member functions with an
    /// sycl executor and code location ptrs
    template <typename Executor, typename... Ts,
        std::enable_if_t<(std::conjunction_v<sycl::experimental::detail::
                                 is_basic_queue_arg<Ts>...>),
            bool>
            guard = true>
    HPX_FORCEINLINE bool apply(Executor&& exec,
        hpx::sycl::experimental::sycl_executor::queue_function_code_loc_ptr_t<
            Ts...>&& f,
        Ts&&... ts)
    {
        // Make sure we only use this for sycl executors
        static_assert(std::is_same_v<std::decay_t<Executor>,
            hpx::sycl::experimental::sycl_executor>);
        // Use the same apply_dispatch than the normal apply otherwise
        return detail::post_dispatch<typename std::decay<Executor>::type>::call(
            HPX_FORWARD(Executor, exec),
            HPX_FORWARD(hpx::sycl::experimental::sycl_executor::
                            queue_function_code_loc_ptr_t<Ts...>,
                f),
            HPX_FORWARD(Ts, ts)...);
    }

    template <typename Executor, typename... Ts, typename T,
        std::enable_if_t<(std::conjunction_v<sycl::experimental::detail::
                                 is_basic_queue_arg<Ts>...>),
            bool>
            guard = true>
    HPX_FORCEINLINE decltype(auto) async(Executor&& exec,
        hpx::sycl::experimental::sycl_executor::queue_function_code_loc_ptr_t<
            Ts..., T>&& f,
        Ts&&... ts, T&& t)
    {
        // Make sure we only use this for sycl executors
        static_assert(std::is_same<std::decay_t<Executor>,
            hpx::sycl::experimental::sycl_executor>::value);
        // Use the same async_dispatch than the normal async otherwise
        return detail::async_dispatch<
            typename std::decay<Executor>::type>::call(HPX_FORWARD(Executor,
                                                           exec),
            std::forward<hpx::sycl::experimental::sycl_executor::
                    queue_function_code_loc_ptr_t<Ts..., T>>(f),
            std::forward<Ts>(ts)..., std::forward<T>(t));
    }

    /// hpx::apply overload for launching sycl queue member functions with an
    /// sycl executor and code location ptrs
    /* template <typename Executor, typename T> */
    template <typename Executor, typename... Ts, typename T,
        std::enable_if_t<(std::conjunction_v<sycl::experimental::detail::
                                 is_basic_queue_arg<Ts>...>),
            bool>
            guard = true>
    HPX_FORCEINLINE bool apply(Executor&& exec,
        hpx::sycl::experimental::sycl_executor::queue_function_code_loc_ptr_t<
            Ts..., T>&& f,
        Ts&&... ts, T&& t)
    {
        // Make sure we only use this for sycl executors
        static_assert(std::is_same_v<std::decay_t<Executor>,
            hpx::sycl::experimental::sycl_executor>);
        // Use the same apply_dispatch than the normal apply otherwise
        return detail::post_dispatch<typename std::decay<Executor>::type>::call(
            HPX_FORWARD(Executor, exec),
            std::forward<hpx::sycl::experimental::sycl_executor::
                    queue_function_code_loc_ptr_t<Ts..., T>>(f),
            std::forward<Ts>(ts)..., std::forward<T>(t));
    }
#endif
}    // namespace hpx
