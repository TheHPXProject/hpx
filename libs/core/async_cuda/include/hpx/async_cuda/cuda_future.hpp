//  Copyright (c) 2023 Gregor Daiss
//  Copyright (c) 2020 John Biddiscombe
//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2016-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_cuda/cuda_event.hpp>
#include <hpx/async_cuda/cuda_exception.hpp>
#include <hpx/async_cuda/custom_gpu_api.hpp>
#include <hpx/async_cuda/detail/cuda_debug.hpp>
#include <hpx/async_cuda/detail/cuda_event_callback.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading_base.hpp>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::cuda::experimental {

    HPX_CXX_CORE_EXPORT using event_mode = std::true_type;
    HPX_CXX_CORE_EXPORT using callback_mode = std::false_type;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // -------------------------------------------------------------
        // cuda future data implementation
        // This version supports 2 modes of operation
        // 1) a callback based future that is made ready
        // by a cuda callback when the stream event occurs
        // 2) an event based callback that must be polled/queried by
        // the runtime to set the future ready state
        HPX_CXX_CORE_EXPORT template <typename Allocator, typename Mode>
        struct future_data;

        HPX_CXX_CORE_EXPORT template <typename Allocator>
        struct future_data<Allocator, event_mode>
          : lcos::detail::future_data_allocator<void, Allocator,
                future_data<Allocator, event_mode>>
        {
            future_data(future_data const&) = delete;
            future_data(future_data&&) = delete;
            future_data& operator=(future_data const&) = delete;
            future_data& operator=(future_data&&) = delete;

            using init_no_addref =
                typename lcos::detail::future_data_allocator<void, Allocator,
                    future_data>::init_no_addref;

            using other_allocator = typename std::allocator_traits<
                Allocator>::template rebind_alloc<future_data>;

            future_data() {}

            future_data(init_no_addref no_addref, other_allocator const& alloc,
                cudaStream_t stream, int device)
              : lcos::detail::future_data_allocator<void, Allocator,
                    future_data>(no_addref, alloc)
            {
                add_event_callback(
                    [fdp = hpx::intrusive_ptr<future_data>(this)](
                        cudaError_t const status) {
                        HPX_ASSERT(status != cudaErrorNotReady);

                        if (status == cudaSuccess)
                        {
                            fdp->set_data(hpx::util::unused);
                        }
                        else
                        {
                            fdp->set_exception(
                                std::make_exception_ptr(cuda_exception(
                                    std::string(
                                        "cuda function returned error code :") +
                                        cudaGetErrorString(status),
                                    status)));
                        }
                    },
                    stream, device);
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Allocator>
        struct future_data<Allocator, callback_mode>
          : lcos::detail::future_data_allocator<void, Allocator,
                future_data<Allocator, callback_mode>>
        {
            HPX_NON_COPYABLE(future_data);

            using init_no_addref =
                typename lcos::detail::future_data_allocator<void, Allocator,
                    future_data>::init_no_addref;

            using other_allocator = typename std::allocator_traits<
                Allocator>::template rebind_alloc<future_data>;

            future_data()
              : rt_(hpx::get_runtime_ptr())
            {
            }

            future_data(init_no_addref no_addref, other_allocator const& alloc,
                cudaStream_t const stream)
              : lcos::detail::future_data_allocator<void, Allocator,
                    future_data>(no_addref, alloc)
              , rt_(hpx::get_runtime_ptr())
            {
                // Hold on to the shared state on behalf of the cuda runtime
                // right away as the callback could be called immediately, and it
                // happens on another thread which might cause a race
                lcos::detail::intrusive_ptr_add_ref(this);

                cudaError_t error =
                    cudaStreamAddCallback(stream, stream_callback, this, 0);
                if (error != cudaSuccess)
                {
                    // callback was not called, release object
                    lcos::detail::intrusive_ptr_release(this);
                    // report error
                    check_cuda_error(error);
                }
                cud_debug.debug(
                    debug::str<>("init_callback"), "event", debug::ptr(this));
            }

            // this is called from the nvidia backend on a non-hpx thread
            // extreme care must be taken to not lock/block or take too long
            // in this callback
            static void CUDART_CB stream_callback(
                cudaStream_t, cudaError_t const error, void* user_data)
            {
                future_data* this_ = static_cast<future_data*>(user_data);

                // continuations run by making the future ready here are
                // handed to HPX threads (see set_on_completed)
                in_stream_callback() = true;
                auto on_exit = hpx::experimental::scope_exit([&] {
                    in_stream_callback() = false;
                    hpx::lcos::detail::intrusive_ptr_release(this_);
                });

                if (error != cudaSuccess)
                {
                    this_->set_exception(
                        HPX_GET_EXCEPTION(hpx::error::kernel_error,
                            "cuda::detail::future_data::stream_callback()",
                            std::string("cudaStreamAddCallback failed: ") +
                                cudaGetErrorString(error)));
                    return;
                }

                this_->set_data(hpx::util::unused);
                cud_debug.debug(debug::str<>("set data callback"), "event",
                    debug::ptr(this_));
            }

            // stream_callback makes this future ready on a thread of the CUDA
            // runtime, which must not run HPX work (and must not call CUDA).
            // Continuations that would run there are handed to an HPX thread
            // instead. Continuations run anywhere else (the future is ready
            // already, or became ready while they were attached) run right
            // away on that thread, as usual.
            void set_on_completed(
                typename lcos::detail::future_data_refcnt_base::
                    completed_callback_type&& f) override
            {
                using base_type = lcos::detail::future_data_allocator<void,
                    Allocator, future_data>;
                using callback_type = std::decay_t<decltype(f)>;

                // nothing to run, or the future is ready already: the base
                // implementation ignores it or runs it right away
                if (!f || this->is_ready())
                {
                    base_type::set_on_completed(HPX_MOVE(f));
                    return;
                }

                // allocated here, so a failure reaches the caller; shared, so
                // it can still run in stream_callback if no HPX thread can be
                // created (nothing may throw into the CUDA runtime)
                auto shared_f = std::make_shared<callback_type>(HPX_MOVE(f));

                base_type::set_on_completed([shared_f]() {
                    if (!in_stream_callback())
                    {
                        auto local = HPX_MOVE(*shared_f);
                        local();
                        return;
                    }

                    bool registered = false;
                    try
                    {
                        // the HPX thread moves the function out, so its
                        // captures are destroyed there, never on the CUDA
                        // runtime's thread (which only releases the empty
                        // function)
                        hpx::threads::thread_init_data data(
                            hpx::threads::make_thread_function_nullary(
                                [shared_f]() {
                                    auto local = HPX_MOVE(*shared_f);
                                    local();
                                }),
                            "cuda_future_on_completed",
                            hpx::threads::thread_priority::boost);

                        hpx::error_code ec(hpx::throwmode::lightweight);
                        hpx::threads::register_work(data, ec);
                        registered = !ec;
                    }
                    catch (...)
                    {
                        // not registered either
                    }
                    if (!registered)
                    {
                        // no new HPX threads anymore (shutdown): run it here
                        (*shared_f)();
                    }
                });
            }

            // whether the calling thread runs stream_callback
            static bool& in_stream_callback() noexcept
            {
                thread_local bool in_callback = false;
                return in_callback;
            }

            hpx::runtime* rt_;
        };

        // -------------------------------------------------------------
        // main API call to get a future from a stream using allocator, and the
        // specified mode
        HPX_CXX_CORE_EXPORT template <typename Allocator, typename Mode>
        hpx::future<void> get_future(
            Allocator const& a, cudaStream_t stream, int device)
        {
            using shared_state = future_data<Allocator, Mode>;

            using other_allocator = typename std::allocator_traits<
                Allocator>::template rebind_alloc<shared_state>;
            using traits = std::allocator_traits<other_allocator>;

            using init_no_addref = typename shared_state::init_no_addref;

            using unique_ptr = std::unique_ptr<shared_state,
                util::allocator_deleter<other_allocator>>;

            other_allocator alloc(a);
            unique_ptr p(traits::allocate(alloc, 1),
                hpx::util::allocator_deleter<other_allocator>{alloc});

            static_assert(std::is_same_v<Mode, event_mode> ||
                    std::is_same_v<Mode, callback_mode>,
                "get_future mode not supported!");

            if constexpr (std::is_same_v<Mode, event_mode>)
            {
                traits::construct(
                    alloc, p.get(), init_no_addref{}, alloc, stream, device);
            }
            else if constexpr (std::is_same_v<Mode, callback_mode>)
            {
                traits::construct(
                    alloc, p.get(), init_no_addref{}, alloc, stream);
            }

            return hpx::traits::future_access<future<void>>::create(
                p.release(), false);
        }

        // -------------------------------------------------------------
        // main API call to get a future from a stream using allocator
        HPX_CXX_CORE_EXPORT template <typename Allocator>
        hpx::future<void> get_future_with_callback(
            Allocator const& a, cudaStream_t const stream)
        {
            // device id 0 will be dropped in callback mode - can be
            // an arbitrary number here
            return get_future<Allocator, callback_mode>(a, stream, 0);
        }

        // -------------------------------------------------------------
        // main API call to get a future from a stream using allocator
        HPX_CXX_CORE_EXPORT template <typename Allocator>
        hpx::future<void> get_future_with_event(
            Allocator const& a, cudaStream_t const stream, int device = -1)
        {
            if (device == -1)
                check_cuda_error(cudaGetDevice(&device));
            return get_future<Allocator, event_mode>(a, stream, device);
        }

        // -------------------------------------------------------------
        // non allocator version of : get future with a callback set
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT hpx::future<void>
            get_future_with_callback(cudaStream_t);

        // -------------------------------------------------------------
        // non allocator version of : get future with an event set
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT hpx::future<void>
        get_future_with_event(cudaStream_t stream, int device = -1);
    }    // namespace detail
}    // namespace hpx::cuda::experimental

HPX_CXX_CORE_EXPORT template <typename Allocator, typename Mode,
    typename NewAllocator>
struct hpx::traits::detail::shared_state_allocator<
    hpx::cuda::experimental::detail::future_data<Allocator, Mode>, NewAllocator>
{
    using type =
        hpx::cuda::experimental::detail::future_data<NewAllocator, Mode>;
};
