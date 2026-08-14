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

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/coroutines/coroutine.hpp>
#include <hpx/coroutines/detail/coroutine_impl.hpp>
#include <hpx/coroutines/detail/coroutine_stackful_self.hpp>
#include <hpx/coroutines/detail/coroutine_stackful_self_direct.hpp>

#include <cstddef>
#include <exception>
#include <utility>

namespace hpx::threads::coroutines::detail {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_DEBUG)
    coroutine_impl::~coroutine_impl()
    {
        HPX_ASSERT(!m_fun);    // functor should have been reset by now
    }
#endif

    void coroutine_impl::operator()() noexcept
    {
        using context_state = super_type::context_state;
        using context_exit_status = super_type::context_exit_status;

        auto status = context_exit_status::not_exited;

        // yield value once the thread function has finished executing
        result_type result_last;

        // loop as long this coroutine has been rebound
        do
        {
#if defined(HPX_HAVE_ADDRESS_SANITIZER)
            finish_switch_fiber(nullptr, m_caller);
#endif
            {
                coroutine_self* old_self = coroutine_self::get_self();
                coroutine_stackful_self self(this, old_self);

                coroutine_self::set_self(&self);
                auto on_exit = hpx::experimental::scope_exit(
                    [&] { coroutine_self::set_self(old_self); });

                try
                {
                    result_last = m_fun(*this->args());
                    HPX_ASSERT(
                        result_last.first == thread_schedule_state::terminated);
                    status = context_exit_status::exited_return;
                }
                catch (...)
                {
                    status = context_exit_status::exited_abnormally;
                    this->m_type_info = std::current_exception();
                }

                // Reset early as the destructors may still yield.
                this->reset_tss();
                this->reset(false);

                // return value to other side of the fence
                this->bind_result(result_last);
            }

            this->do_return(status);
        } while (this->m_state == context_state::running);

        // should not get here, never
        HPX_ASSERT(this->m_state == context_state::running);
    }

    // execute the coroutine function directly in the context of the calling
    // thread
    coroutine_impl::result_type coroutine_impl::invoke_directly(
        coroutine_impl::arg_type const arg)
    {
        using context_state = super_type::context_state;
        using context_exit_status = super_type::context_exit_status;

        auto status = context_exit_status::not_exited;

        result_type const result_last(
            thread_schedule_state::unknown, invalid_thread_id);

        this->m_state = context_state::running;

        std::exception_ptr tinfo;
        {
            coroutine_self* old_self = coroutine_self::get_self();
            coroutine_stackful_self_direct self(this, old_self);

            coroutine_self::set_self(&self);
            auto on_exit = hpx::experimental::scope_exit(
                [&] { coroutine_self::set_self(old_self); });

            try
            {
                this->m_result = this->m_fun(arg);
                HPX_ASSERT(
                    this->m_result.first == thread_schedule_state::terminated);
                status = context_exit_status::exited_return;
            }
            catch (...)
            {
                status = context_exit_status::exited_abnormally;
                this->m_type_info = std::current_exception();
            }

            this->reset_tss();
            this->reset(true);

            this->bind_result(result_last);
        }

        HPX_ASSERT(this->m_state == context_state::running);
        this->m_state = context_state::exited;
        this->m_exit_status = status;

        if (status == context_exit_status::exited_abnormally)
        {
            std::rethrow_exception(this->m_type_info);
        }
        return this->m_result;
    }
}    // namespace hpx::threads::coroutines::detail
