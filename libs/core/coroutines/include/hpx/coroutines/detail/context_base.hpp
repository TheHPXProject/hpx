//  Copyright (c) 2006, Giovanni P. Deretta
//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  This code may be used under either of the following two licences:
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE. OF SUCH DAMAGE.
//
//  Or:
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

// This needs to be first for building on Macs
#include <hpx/coroutines/detail/context_impl.hpp>

#include <hpx/assert.hpp>
#include <hpx/coroutines/detail/swap_context.hpp>    //for swap hints
#include <hpx/coroutines/detail/tss.hpp>
#include <hpx/coroutines/thread_id_type.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <utility>

namespace hpx::threads::coroutines::detail {

    /////////////////////////////////////////////////////////////////////////////
    inline constexpr std::ptrdiff_t default_stack_size = -1;

    class coroutine_impl;

    template <typename CoroutineImpl>
    class context_base : public default_context_impl<CoroutineImpl>
    {
        using base_type = default_context_impl<CoroutineImpl>;

    public:
        using deleter_type = void(context_base const*);
        using thread_id_type = hpx::threads::thread_id;

    private:
        context_base(std::ptrdiff_t stack_size, thread_id_type id)
          : base_type(stack_size)
          , m_caller()
          , m_state(context_state::ready)
          , m_exit_state(context_exit_state::not_requested)
          , m_exit_status(context_exit_status::not_exited)
#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
          , m_phase(0)
#endif
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
          , m_thread_data(nullptr)
#else
          , m_thread_data(0)
#endif
#if defined(HPX_HAVE_LIBCDS)
          , libcds_data_(0)
          , libcds_hazard_pointer_data_(0)
          , libcds_dynamic_hazard_pointer_data_(0)
#endif
          , m_type_info()
          , m_thread_id(HPX_MOVE(id))
          , continuation_recursion_count_(0)
        {
        }

        context_base(context_base const&) = delete;
        context_base(context_base&&) = delete;

        friend class coroutine_impl;

    public:
        context_base& operator=(context_base const&) = delete;
        context_base& operator=(context_base&&) = delete;

        void reset_tss() const
        {
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
            delete_tss_storage(m_thread_data);
#else
            m_thread_data = 0;
#endif
#if defined(HPX_HAVE_LIBCDS)
            libcds_data_ = 0;
            libcds_hazard_pointer_data_ = 0;
            libcds_dynamic_hazard_pointer_data_ = 0;
#endif
        }

        void reset()
        {
#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
            m_phase = 0;
#endif
            m_thread_id.reset();
        }

#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
        constexpr std::size_t phase() const noexcept
        {
            return m_phase;
        }
#endif

        constexpr thread_id_type get_thread_id() const noexcept
        {
            return m_thread_id;
        }

        /*
         * Returns true if the context is runnable.
         */
        constexpr bool is_ready() const noexcept
        {
            return m_state == context_state::ready;
        }

        constexpr bool running() const noexcept
        {
            return m_state == context_state::running;
        }

        constexpr bool exited() const noexcept
        {
            return m_state == context_state::exited;
        }

        void init()
        {
            base_type::init();
        }

        // Resume coroutine.
        // Pre:  The coroutine must be ready.
        // Post: The coroutine relinquished control. It might be ready
        //       or exited.
        // Throws:- 'abnormal_exit' if the coroutine was exited by another
        //          uncaught exception.
        // Note, it guarantees that the coroutine is resumed. Can throw only
        // on return.
        void invoke()
        {
            base_type::init();
            HPX_ASSERT(is_ready());
            do_invoke();

            if (m_exit_status != context_exit_status::not_exited)
            {
                if (m_exit_status == context_exit_status::exited_return)
                    return;
                if (m_exit_status == context_exit_status::exited_abnormally)
                {
                    HPX_ASSERT(m_type_info);
                    std::rethrow_exception(m_type_info);
                }
                HPX_ASSERT_MSG(false, "unknown exit status");
            }
        }

        // Put coroutine in ready state and relinquish control
        // to caller until resumed again.
        // Pre:  Coroutine is running.
        //       Exit not pending.
        //       Operations not pending.
        // Post: Coroutine is running.
        // Throws: exit_exception, if exit is pending *after* it has been
        //         resumed.
        void yield()
        {
            //prevent infinite loops
            HPX_ASSERT(m_exit_state < context_exit_state::signaled);
            HPX_ASSERT(running());

            m_state = context_state::ready;
#if defined(HPX_HAVE_ADDRESS_SANITIZER)
            this->start_yield_fiber(&this->asan_fake_stack, m_caller);
#endif
            do_yield();

#if defined(HPX_HAVE_ADDRESS_SANITIZER)
            this->finish_yield_fiber(this->asan_fake_stack);
#endif
            m_exit_status = context_exit_status::not_exited;

            HPX_ASSERT(running());
        }

        // Nothrow.
        ~context_base() noexcept
        {
            HPX_ASSERT(!running());
#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
            HPX_ASSERT(exited() || (is_ready() && m_phase == 0));
#else
            HPX_ASSERT(exited() || is_ready());
#endif
            m_thread_id.reset();
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
            delete_tss_storage(m_thread_data);
#else
            m_thread_data = 0;
#endif
#if defined(HPX_HAVE_LIBCDS)
            libcds_data_ = 0;
            libcds_hazard_pointer_data_ = 0;
            libcds_dynamic_hazard_pointer_data_ = 0;
#endif
        }

#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        std::size_t get_thread_data() const
        {
            if (!m_thread_data)
                return 0;
            return get_tss_thread_data(m_thread_data);
        }
#else
        constexpr std::size_t get_thread_data() const noexcept
        {
            return m_thread_data;
        }
#endif

#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        std::size_t set_thread_data(std::size_t data) const
        {
            return set_tss_thread_data(m_thread_data, data);
        }
#else
        std::size_t set_thread_data(std::size_t data) const noexcept
        {
            std::size_t const olddata = m_thread_data;
            m_thread_data = data;
            return olddata;
        }
#endif

#if defined(HPX_HAVE_LIBCDS)
        std::size_t get_libcds_data() const
        {
            return libcds_data_;
        }

        std::size_t set_libcds_data(std::size_t data)
        {
            std::swap(data, libcds_data_);
            return data;
        }

        std::size_t get_libcds_hazard_pointer_data() const
        {
            return libcds_hazard_pointer_data_;
        }

        std::size_t set_libcds_hazard_pointer_data(std::size_t data)
        {
            std::swap(data, libcds_hazard_pointer_data_);
            return data;
        }

        std::size_t get_libcds_dynamic_hazard_pointer_data() const
        {
            return libcds_dynamic_hazard_pointer_data_;
        }

        std::size_t set_libcds_dynamic_hazard_pointer_data(std::size_t data)
        {
            std::swap(data, libcds_dynamic_hazard_pointer_data_);
            return data;
        }
#endif

#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        tss_storage* get_thread_tss_data(bool create_if_needed) const
        {
            if (!m_thread_data && create_if_needed)
                m_thread_data = create_tss_storage();
            return m_thread_data;
        }
#endif

        std::size_t& get_continuation_recursion_count() noexcept
        {
            return continuation_recursion_count_;
        }

    public:
        // global coroutine state
        enum class context_state : std::uint8_t
        {
            running = 0,    // context running.
            ready,          // context at yield point.
            exited          // context is finished.
        };

    protected:
        // exit request state
        enum class context_exit_state : std::uint8_t
        {
            not_requested = 0,    // exit not requested.
            pending,              // exit requested.
            signaled              // exit request delivered.
        };

        // exit status
        enum class context_exit_status : std::uint8_t
        {
            not_exited = 0,
            exited_return,       // process exited by return.
            exited_abnormally    // process exited uncleanly.
        };

        void rebind_base(thread_id_type id)
        {
            HPX_ASSERT(!running());

            m_thread_id = HPX_MOVE(id);
            m_state = context_state::ready;
            m_exit_state = context_exit_state::not_requested;
            m_exit_status = context_exit_status::not_exited;
#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
            HPX_ASSERT(m_phase == 0);
#endif
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
            HPX_ASSERT(m_thread_data == nullptr);
#else
            HPX_ASSERT(m_thread_data == 0);
#endif
#if defined(HPX_HAVE_LIBCDS)
            HPX_ASSERT(libcds_data_ == 0);
            HPX_ASSERT(libcds_hazard_pointer_data_ == 0);
            HPX_ASSERT(libcds_dynamic_hazard_pointer_data_ == 0);
#endif
            // NOLINTNEXTLINE(bugprone-throw-keyword-missing)
            m_type_info = std::exception_ptr();
        }

        // Nothrow.
        void do_return(context_exit_status status) noexcept
        {
            HPX_ASSERT(status != context_exit_status::not_exited);
            HPX_ASSERT(m_state == context_state::running);

            m_state = context_state::exited;
            m_exit_status = status;
#if defined(HPX_HAVE_ADDRESS_SANITIZER)
            this->start_yield_fiber(&this->asan_fake_stack, m_caller);
#endif

            do_yield();
        }

    protected:
        // Nothrow.
        void do_yield() noexcept
        {
            swap_context(*this, m_caller, detail::yield_hint());
        }

        // Nothrow.
        void do_invoke() noexcept
        {
            HPX_ASSERT(is_ready());
#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
            ++m_phase;
#endif
            m_state = context_state::running;

#if defined(HPX_HAVE_ADDRESS_SANITIZER)
            this->start_switch_fiber(&this->asan_fake_stack);
#endif

            swap_context(m_caller, *this, detail::invoke_hint());

#if defined(HPX_HAVE_ADDRESS_SANITIZER)
            this->finish_switch_fiber(this->asan_fake_stack, m_caller);
#endif
        }

        using ctx_type = typename base_type::context_impl_base;
        ctx_type m_caller;

        context_state m_state;
        context_exit_state m_exit_state;
        context_exit_status m_exit_status;
#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
        std::size_t m_phase;
#endif
#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        mutable detail::tss_storage* m_thread_data;
#else
        mutable std::size_t m_thread_data;
#endif
#if defined(HPX_HAVE_LIBCDS)
        mutable std::size_t libcds_data_;
        mutable std::size_t libcds_hazard_pointer_data_;
        mutable std::size_t libcds_dynamic_hazard_pointer_data_;
#endif

        // This is used to generate a meaningful exception trace.
        std::exception_ptr m_type_info;
        thread_id_type m_thread_id;

        std::size_t continuation_recursion_count_;
    };
}    // namespace hpx::threads::coroutines::detail
