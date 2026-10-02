//  Copyright (c) 2006, Giovanni P. Deretta
//  Copyright (c) 2007-2024 Hartmut Kaiser
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
#include <hpx/assert.hpp>
#include <hpx/coroutines/coroutine_fwd.hpp>
#include <hpx/coroutines/detail/context_base.hpp>
#include <hpx/coroutines/thread_enums.hpp>
#include <hpx/coroutines/thread_id_type.hpp>
#include <hpx/modules/functional.hpp>

#include <cstddef>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads::coroutines::detail {

    ///////////////////////////////////////////////////////////////////////////
    // This type augments the context_base type with the type of the stored
    // functor.
    HPX_CXX_CORE_EXPORT class coroutine_impl
      : public context_base<coroutine_impl>
    {
    public:
        coroutine_impl(coroutine_impl const&) = delete;
        coroutine_impl(coroutine_impl&&) = delete;
        coroutine_impl& operator=(coroutine_impl const&) = delete;
        coroutine_impl& operator=(coroutine_impl&&) = delete;

    public:
        using super_type = context_base;
        using thread_id_type = context_base::thread_id_type;

        using result_type = std::pair<thread_schedule_state, thread_id_type>;
        using arg_type = thread_restart_state;

        using functor_type = hpx::move_only_function<result_type(arg_type)>;

        coroutine_impl(functor_type&& f, thread_id_type id,
            std::ptrdiff_t stack_size) noexcept
          : context_base(stack_size, HPX_MOVE(id))
          , m_result(thread_schedule_state::unknown, invalid_thread_id)
          , m_arg(nullptr)
          , m_fun(HPX_MOVE(f))
        {
        }

#if defined(HPX_DEBUG)
        HPX_CORE_EXPORT ~coroutine_impl();
#endif

        // execute the coroutine using normal context switching
        HPX_CORE_EXPORT void operator()() noexcept;

        // execute the coroutine function directly in the context of the calling
        // thread
        HPX_CORE_EXPORT result_type invoke_directly(arg_type arg);

    public:
        void bind_result(result_type const& res) noexcept
        {
            HPX_ASSERT(m_result.first != thread_schedule_state::terminated);
            m_result = res;
        }

        result_type result() const noexcept
        {
            return m_result;
        }
        arg_type* args() const noexcept
        {
            HPX_ASSERT(m_arg);
            return m_arg;
        }

        void bind_args(arg_type* arg) noexcept
        {
            m_arg = arg;
        }

#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
        constexpr std::size_t get_thread_phase() const noexcept
        {
            return this->phase();
        }
#endif

        void init()
        {
            this->super_type::init();
        }

        void reset()
        {
            // First reset the function and arguments
            m_result =
                result_type(thread_schedule_state::unknown, invalid_thread_id);
            m_arg = nullptr;
            m_fun.reset();

            // Then reset the id as it may be used by the destructors of the
            // thread function above. The caller recycles the stack after exit.
            this->super_type::reset();
        }

        void rebind(functor_type&& f, thread_id_type id)
        {
            HPX_ASSERT(m_result.first == thread_schedule_state::unknown ||
                m_result.first == thread_schedule_state::terminated);
            this->rebind_stack();    // count how often a coroutines object was reused
            m_result =
                result_type(thread_schedule_state::unknown, invalid_thread_id);
            m_arg = nullptr;
            m_fun = HPX_MOVE(f);
            this->super_type::rebind_base(HPX_MOVE(id));
        }

    private:
        result_type m_result;
        arg_type* m_arg;
        functor_type m_fun;
    };
}    // namespace hpx::threads::coroutines::detail

#include <hpx/config/warnings_suffix.hpp>
