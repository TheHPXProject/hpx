//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2012-2023 Hartmut Kaiser
//  Copyright (c) 2009 Oliver Kowalke
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/coroutines/detail/get_stack_pointer.hpp>
#include <hpx/coroutines/detail/swap_context.hpp>
#if defined(HPX_HAVE_COROUTINE_COUNTERS)
#include <hpx/modules/util.hpp>
#endif

// include unistd.h conditionally to check for POSIX version. Not all OSs have
// the unistd header...
#if defined(HPX_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(_POSIX_VERSION) &&                                                 \
    !(defined(__ARM64_ARCH_8__) && defined(__APPLE__))
#include <hpx/coroutines/detail/posix_utility.hpp>

#define HPX_USE_POSIX_STACK_UTILITIES
#endif

#include <boost/context/detail/fcontext.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <utility>

#if !defined(HPX_HAVE_THREADS_GET_STACK_POINTER)
#include <limits>
#endif

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_GENERIC_CONTEXT_USE_SEGMENTED_STACKS)

#define HPX_COROUTINES_SEGMENTS 10

extern "C" {
void* __splitstack_makecontext(
    std::size_t, void* [HPX_COROUTINES_SEGMENTS], std::size_t*);
void __splitstack_releasecontext(void* [HPX_COROUTINES_SEGMENTS]);
void __splitstack_resetcontext(void* [HPX_COROUTINES_SEGMENTS]);
void __splitstack_block_signals_context(
    void* [HPX_COROUTINES_SEGMENTS], int* new_value, int* old_value);
void __splitstack_getcontext(void* [HPX_COROUTINES_SEGMENTS]);
void __splitstack_setcontext(void* [HPX_COROUTINES_SEGMENTS]);
}

#if !defined(SIGSTKSZ)
#define SIGSTKSZ (8 * 1024)
#define UDEF_SIGSTKSZ
#endif

#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads::coroutines {

    // some platforms need special preparation of the main thread
    struct prepare_main_thread
    {
        prepare_main_thread() = default;
    };

    namespace detail::generic_context {

        ///////////////////////////////////////////////////////////////////////
        // This is taken directly from one of the Boost.Context examples
#if !defined(HPX_GENERIC_CONTEXT_USE_SEGMENTED_STACKS)
        struct stack_allocator
        {
            static constexpr std::ptrdiff_t maximum_stacksize() noexcept
            {
                return HPX_HUGE_STACK_SIZE;
            }

            static constexpr std::ptrdiff_t default_stacksize() noexcept
            {
                return HPX_MEDIUM_STACK_SIZE;
            }

            static constexpr std::ptrdiff_t minimum_stacksize() noexcept
            {
                return HPX_SMALL_STACK_SIZE;
            }

            static void* allocate(std::size_t size)
            {
                // Condition excludes MacOS/M1 from using posix mmap
#if defined(HPX_USE_POSIX_STACK_UTILITIES)
                void* limit = posix::alloc_stack(size);
                posix::watermark_stack(limit, size);
#else
                void* limit = std::calloc(size, sizeof(char));
                if (!limit)
                    throw std::bad_alloc();
#endif
                return static_cast<char*>(limit) + size;
            }

            static void deallocate(void* vp, std::size_t size) noexcept
            {
                HPX_ASSERT(vp);
                void* limit = static_cast<char*>(vp) - size;
#if defined(HPX_USE_POSIX_STACK_UTILITIES)
                posix::free_stack(limit, size);
#else
                std::free(limit);
#endif
            }
        };
#else
        struct stack_allocator
        {
            typedef void* segments_context[HPX_COROUTINES_SEGMENTS];

            static std::ptrdiff_t maximum_stacksize() noexcept
            {
                HPX_ASSERT_MSG(false, "segmented stack is unbound");
                return 0;
            }

            static constexpr std::ptrdiff_t default_stacksize() noexcept
            {
                return minimum_stacksize();
            }

            static constexpr std::ptrdiff_t minimum_stacksize() noexcept
            {
                return SIGSTKSZ + sizeof(boost::context::detail::fcontext_t) +
                    15;
            }

            void* allocate(std::size_t size) const
            {
                HPX_ASSERT(default_stacksize() <= size);

                void* limit =
                    __splitstack_makecontext(size, segments_ctx_, &size);
                if (!limit)
                    throw std::bad_alloc();

                int off = 0;
                __splitstack_block_signals_context(segments_ctx_, &off, 0);

                return static_cast<char*>(limit) + size;
            }

            void deallocate(void* vp, std::size_t size) const noexcept
            {
                __splitstack_releasecontext(segments_ctx_);
            }

            segments_context segments_ctx_;
        };
#endif

        // Generic implementation for the context_impl_base class based on
        // Boost.Context.
        template <typename T>
        [[noreturn]] void trampoline(boost::context::detail::transfer_t tr)
        {
            auto const arg = static_cast<
                std::pair<void*, boost::context::detail::fcontext_t*>*>(
                tr.data);

            HPX_ASSERT(arg->second);
            *arg->second = tr.fctx;

            T* fun = static_cast<T*>(arg->first);
            HPX_ASSERT(fun);
            (*fun)();

            std::abort();
        }

        template <typename CoroutineImpl>
        class fcontext_context_impl
        {
        public:
            // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
            HPX_NON_COPYABLE(fcontext_context_impl);

        public:
            using context_impl_base = fcontext_context_impl;

            // Create a context that on restore invokes Functor on
            // a new stack. The stack size can be optionally specified.
            // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
            explicit fcontext_context_impl(std::ptrdiff_t stack_size = -1)
              : cb_(std::make_pair(static_cast<void*>(this), nullptr))
              , funp_(&trampoline<CoroutineImpl>)
              , ctx_(nullptr)
              , alloc_()
              , stack_size_((stack_size == -1) ?
                        alloc_.minimum_stacksize() :
                        static_cast<std::ptrdiff_t>(stack_size))
              , stack_pointer_(nullptr)
            {
            }

            void init()
            {
                if (stack_pointer_ != nullptr)
                    return;

                stack_pointer_ = alloc_.allocate(stack_size_);
                if (stack_pointer_ == nullptr)
                {
                    throw std::runtime_error(
                        "could not allocate memory for stack");
                }
                ctx_ = boost::context::detail::make_fcontext(
                    stack_pointer_, stack_size_, funp_);
            }

            ~fcontext_context_impl()
            {
                if (ctx_ && stack_pointer_)
                {
                    alloc_.deallocate(stack_pointer_, stack_size_);
                }
            }

            // Return the size of the reserved stack address space.
            constexpr std::ptrdiff_t get_stacksize() const noexcept
            {
                return stack_size_;
            }

#if defined(HPX_HAVE_THREADS_GET_STACK_POINTER)
            std::ptrdiff_t get_available_stack_space() const noexcept
            {
                return stack_size_ -
                    (reinterpret_cast<std::size_t>(stack_pointer_) -
                        get_stack_ptr());
            }
#else
            constexpr std::ptrdiff_t get_available_stack_space() const noexcept
            {
                return (std::numeric_limits<std::ptrdiff_t>::max)();
            }
#endif
            void reset_stack(bool direct_execution)
            {
                if (direct_execution)
                    return;

                if (ctx_)
                {
#if defined(HPX_USE_POSIX_STACK_UTILITIES)
                    void* limit =
                        static_cast<char*>(stack_pointer_) - stack_size_;
                    if (posix::reset_stack(limit, stack_size_))
                    {
#if defined(HPX_HAVE_COROUTINE_COUNTERS)
                        increment_stack_unbind_count();
#endif
                    }
#else
                    // nothing we can do here ...
#endif
                }
            }

            void rebind_stack()
            {
                if (ctx_)
                {
#if defined(HPX_HAVE_COROUTINE_COUNTERS)
                    increment_stack_recycle_count();
#endif
                    ctx_ = boost::context::detail::make_fcontext(
                        stack_pointer_, stack_size_, funp_);
                }
            }

#if defined(HPX_HAVE_COROUTINE_COUNTERS)
            using counter_type = std::atomic<std::int64_t>;

        private:
            static counter_type& get_stack_unbind_counter() noexcept
            {
                static counter_type counter(0);
                return counter;
            }

            static std::uint64_t increment_stack_unbind_count() noexcept
            {
                return ++get_stack_unbind_counter();
            }

            static counter_type& get_stack_recycle_counter() noexcept
            {
                static counter_type counter(0);
                return counter;
            }

            static std::uint64_t increment_stack_recycle_count() noexcept
            {
                return ++get_stack_recycle_counter();
            }

        public:
            static std::uint64_t get_stack_unbind_count(bool reset) noexcept
            {
                return util::get_and_reset_value(
                    get_stack_unbind_counter(), reset);
            }

            static std::uint64_t get_stack_recycle_count(bool reset) noexcept
            {
                return util::get_and_reset_value(
                    get_stack_recycle_counter(), reset);
            }
#endif
        private:
            friend void swap_context(fcontext_context_impl& from,
                fcontext_context_impl& to, detail::default_hint)
            {
#if defined(HPX_GENERIC_CONTEXT_USE_SEGMENTED_STACKS)
                __splitstack_getcontext(from.alloc_.segments_ctx_);
                __splitstack_setcontext(to.alloc_.segments_ctx);
#endif
                // switch to other coroutine context
                to.cb_.second = &from.ctx_;
                auto transfer = boost::context::detail::jump_fcontext(
                    to.ctx_, static_cast<void*>(&to.cb_));
                to.ctx_ = transfer.fctx;

#if defined(HPX_GENERIC_CONTEXT_USE_SEGMENTED_STACKS)
                __splitstack_setcontext(from.alloc_.segments_ctx);
#endif
            }

        private:
            std::pair<void*, boost::context::detail::fcontext_t*> cb_;
            void (*funp_)(boost::context::detail::transfer_t);
            boost::context::detail::fcontext_t ctx_;
            stack_allocator alloc_;
            std::ptrdiff_t stack_size_;
            void* stack_pointer_;
        };
    }    // namespace detail::generic_context
}    // namespace hpx::threads::coroutines

#undef HPX_USE_POSIX_STACK_UTILITIES
