//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <utility>

#if defined(HPX_HAVE_ADDRESS_SANITIZER)
#include <sanitizer/asan_interface.h>
#endif

namespace {
    using hpx::threads::invalid_thread_id;
    using hpx::threads::thread_schedule_state;
    using hpx::threads::coroutines::coroutine;
    using hpx::threads::coroutines::detail::coroutine_self;

    constexpr std::ptrdiff_t coroutine_stack_size = std::ptrdiff_t{256} * 1024;

    void yield()
    {
        coroutine_self::get_self()->yield(
            {thread_schedule_state::pending, invalid_thread_id});
    }

    HPX_NOINLINE void use_stack(unsigned int depth)
    {
        // Keep the writes observable in optimized builds. Run this test both
        // with real stack frames and with ASan's fake-stack detection enabled.
        unsigned char volatile data[2048];
        for (std::size_t i = 0; i != sizeof(data); ++i)
            data[i] = static_cast<unsigned char>(depth + i);

        if (depth != 0)
            use_stack(depth - 1);
        else
            yield();

        for (std::size_t i = 0; i != sizeof(data); ++i)
            HPX_TEST_EQ(data[i], static_cast<unsigned char>(depth + i));
    }

    struct yield_on_delete
    {
        void operator()(unsigned int* count) const
        {
            // This runs in m_fun.reset(), after the coroutine body returns.
            if (deep)
                use_stack(4);
            else
                yield();
            ++*count;
        }

        bool deep;
    };

    void test_recycling()
    {
        unsigned int destroyed = 0;
        auto make_function = [&](bool deep) {
            return [&, deep,
                       cleanup = std::unique_ptr<unsigned int, yield_on_delete>(
                           &destroyed, yield_on_delete{deep})](
                       coroutine::arg_type) {
                if (deep)
                    use_stack(8);
                else
                    yield();
                // Throw after resuming to exercise __asan_handle_no_return.
                // Keep alternate iterations shallow throughout cleanup.
                if (deep)
                {
                    bool caught = false;
                    try
                    {
                        throw std::runtime_error("resumed coroutine");
                    }
                    catch (std::runtime_error const&)
                    {
                        caught = true;
                    }
                    HPX_TEST(caught);
                }
                return coroutine::result_type{
                    thread_schedule_state::terminated, invalid_thread_id};
            };
        };

        coroutine c(
            make_function(true), invalid_thread_id, coroutine_stack_size);
        c.init();
#if defined(HPX_HAVE_ADDRESS_SANITIZER)
        void const* stack_bottom = nullptr;
        std::size_t stack_size = 0;
        auto const caller_fake_stack = __asan_get_current_fake_stack();
#endif
        for (unsigned int iteration = 0; iteration != 32; ++iteration)
        {
            if (iteration != 0)
                c.rebind(make_function(iteration % 2 == 0), invalid_thread_id);

            // The body yields once and the function's destructor once.
            for (unsigned int step = 0; step != 3; ++step)
            {
                auto const result = c();
                HPX_TEST_EQ(result.first,
                    step == 2 ? thread_schedule_state::terminated :
                                thread_schedule_state::pending);
#if defined(HPX_HAVE_ADDRESS_SANITIZER)
                // Windows discovers the fiber bounds on the first switch.
                if (iteration == 0 && step == 0)
                {
                    stack_bottom = c.impl()->asan_stack_bottom;
                    stack_size = c.impl()->asan_stack_size;
                }
                HPX_TEST_EQ(c.impl()->asan_stack_bottom, stack_bottom);
                HPX_TEST_EQ(c.impl()->asan_stack_size, stack_size);
                HPX_TEST_EQ(__asan_get_current_fake_stack(), caller_fake_stack);
#endif
            }
            HPX_TEST_EQ(destroyed, iteration + 1);
        }
    }

    void test_exception_and_direct_execution()
    {
        coroutine c(
            [](coroutine::arg_type) -> coroutine::result_type {
                use_stack(8);
                throw std::runtime_error("coroutine exit");
            },
            invalid_thread_id, coroutine_stack_size);
        HPX_TEST_EQ(c().first, thread_schedule_state::pending);
        bool caught = false;
        try
        {
            c();
        }
        catch (std::runtime_error const&)
        {
            caught = true;
        }
        HPX_TEST(caught);

        auto make_function = [] {
            return [](coroutine::arg_type) {
                return coroutine::result_type{
                    thread_schedule_state::terminated, invalid_thread_id};
            };
        };
        // Direct execution needs an enclosing coroutine. It must neither
        // advise that caller's stack nor require its own allocated stack.
        coroutine outer(
            [&](coroutine::arg_type) {
                auto* caller_self = coroutine_self::get_self();
                coroutine direct(
                    make_function(), invalid_thread_id, coroutine_stack_size);
                direct.invoke_directly();
                HPX_TEST(direct.impl()->exited());
                HPX_TEST_EQ(coroutine_self::get_self(), caller_self);

                direct.rebind(
                    [](coroutine::arg_type) -> coroutine::result_type {
                        throw std::runtime_error("direct coroutine exit");
                    },
                    invalid_thread_id);
                bool direct_caught = false;
                try
                {
                    direct.invoke_directly();
                }
                catch (std::runtime_error const&)
                {
                    direct_caught = true;
                }
                HPX_TEST(direct_caught);
                HPX_TEST(direct.impl()->exited());
                HPX_TEST_EQ(coroutine_self::get_self(), caller_self);

                c.rebind(make_function(), invalid_thread_id);
                c.invoke_directly();
                HPX_TEST(c.impl()->exited());
                HPX_TEST_EQ(coroutine_self::get_self(), caller_self);
                c.rebind(make_function(), invalid_thread_id);
                HPX_TEST_EQ(c().first, thread_schedule_state::terminated);
                return coroutine::result_type{
                    thread_schedule_state::terminated, invalid_thread_id};
            },
            invalid_thread_id, coroutine_stack_size);
        HPX_TEST_EQ(outer().first, thread_schedule_state::terminated);
    }
}    // namespace

int main()
{
#if defined(__linux) || defined(linux) || defined(__linux__) ||                \
    defined(__FreeBSD__)
    namespace posix = hpx::threads::coroutines::detail::posix;
    int const old_mode = posix::unbind_on_reset;
    bool const old_guard_pages = posix::use_guard_pages;
    for (bool guard_pages : {false, true})
    {
        posix::use_guard_pages = guard_pages;
        for (int mode = 0; mode != 3; ++mode)
        {
            posix::unbind_on_reset = mode;
            test_recycling();
            test_exception_and_direct_execution();
        }
    }
    posix::use_guard_pages = old_guard_pages;
    posix::unbind_on_reset = old_mode;
#else
    test_recycling();
    test_exception_and_direct_execution();
#endif
    return hpx::util::report_errors();
}
