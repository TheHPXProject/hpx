//  Copyright (c) 2026 K S Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks with_stack_size and its shorthands: the stack size is seen through
// get_stack_size, the wrapper nests with with_task_size in both orders (each
// trait looks through the other wrapper), set_stack_size lets the function's
// stack size win, the wrapper forwards arguments, keeps the function's
// address and annotation, and runs through async and post.

#include <hpx/functional.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/testing.hpp>

#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

using hpx::threads::huge_stack;
using hpx::threads::large_stack;
using hpx::threads::medium_stack;
using hpx::threads::nano_task;
using hpx::threads::no_stack;
using hpx::threads::small_stack;
using hpx::threads::task_size;
using hpx::threads::thread_stacksize;
using hpx::threads::with_stack_size;
using hpx::threads::with_task_size;

namespace {

    int plain_function(int i)
    {
        return i + 1;
    }

    // tells which call operator was used
    struct const_or_not
    {
        int operator()()
        {
            return 1;
        }

        int operator()() const
        {
            return 2;
        }
    };

    template <typename F>
    inline constexpr thread_stacksize stacksize_of =
        hpx::traits::get_stack_size_v<F>;

    template <typename F>
    inline constexpr task_size size_of = hpx::traits::get_task_size_v<F>;

    template <typename F>
    thread_stacksize stacksize_of_object(F const& f)
    {
        return hpx::traits::get_stack_size<F>::call(f);
    }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    // Has a name only through the trait, so the name its thread gets must
    // come from stack_sized_function forwarding get_function_annotation.
    struct named_function
    {
        std::string operator()() const
        {
            return hpx::threads::get_thread_description(
                hpx::threads::get_self_id())
                .get_description();
        }
    };
#endif
}    // namespace

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
template <>
struct hpx::traits::get_function_annotation<named_function>
{
    static constexpr char const* call(named_function const&) noexcept
    {
        return "named";
    }
};
#endif

///////////////////////////////////////////////////////////////////////////////
void test_trait()
{
    auto lambda = [](int i) { return i; };
    static_assert(stacksize_of<decltype(lambda)> == thread_stacksize::unknown);
    static_assert(
        stacksize_of<decltype(&plain_function)> == thread_stacksize::unknown);

    auto large = with_stack_size<thread_stacksize::large>(lambda);
    static_assert(stacksize_of<decltype(large)> == thread_stacksize::large);
    static_assert(
        stacksize_of<decltype(with_stack_size<thread_stacksize::nostack>(
            &plain_function))> == thread_stacksize::nostack);

    // the shorthands give the same types as with_stack_size
    static_assert(std::is_same_v<decltype(no_stack(lambda)),
        decltype(with_stack_size<thread_stacksize::nostack>(lambda))>);
    static_assert(std::is_same_v<decltype(small_stack(lambda)),
        decltype(with_stack_size<thread_stacksize::small_>(lambda))>);
    static_assert(std::is_same_v<decltype(medium_stack(lambda)),
        decltype(with_stack_size<thread_stacksize::medium>(lambda))>);
    static_assert(std::is_same_v<decltype(large_stack(&plain_function)),
        decltype(with_stack_size<thread_stacksize::large>(&plain_function))>);
    static_assert(std::is_same_v<decltype(huge_stack(lambda)),
        decltype(with_stack_size<thread_stacksize::huge>(lambda))>);

    // a qualified function has the stack size of the unqualified one
    using large_type = decltype(large);
    static_assert(stacksize_of<large_type const> == thread_stacksize::large);
    static_assert(stacksize_of<large_type&> == thread_stacksize::large);
    static_assert(stacksize_of<large_type const&> == thread_stacksize::large);
    static_assert(stacksize_of<large_type&&> == thread_stacksize::large);
    HPX_TEST(stacksize_of_object(large) == thread_stacksize::large);

    // the stack size is a template argument, so the wrapper is as big as f
    static_assert(sizeof(large) == sizeof(lambda));

    // the second argument is reported when the function has no stack size,
    // and a wrapper's stack size wins over it
    using hpx::traits::get_stack_size_v;
    static_assert(get_stack_size_v<decltype(lambda), thread_stacksize::huge> ==
        thread_stacksize::huge);
    static_assert(get_stack_size_v<decltype(large), thread_stacksize::small_> ==
        thread_stacksize::large);

    // wrapping again replaces the stack size
    static_assert(stacksize_of<decltype(large_stack(no_stack(lambda)))> ==
        thread_stacksize::large);
    static_assert(stacksize_of<decltype(no_stack(large_stack(lambda)))> ==
        thread_stacksize::nostack);

    // the wrapper doesn't give the function a task size
    static_assert(size_of<decltype(no_stack(lambda))> == task_size::unknown);
}

///////////////////////////////////////////////////////////////////////////////
void test_nesting()
{
    auto lambda = [] { return 1; };

    // both orders give the same task size and stack size
    auto nano_nostack = nano_task(no_stack(lambda));
    auto nostack_nano = no_stack(nano_task(lambda));

    static_assert(size_of<decltype(nano_nostack)> == task_size::nano);
    static_assert(size_of<decltype(nostack_nano)> == task_size::nano);
    static_assert(
        stacksize_of<decltype(nano_nostack)> == thread_stacksize::nostack);
    static_assert(
        stacksize_of<decltype(nostack_nano)> == thread_stacksize::nostack);

    HPX_TEST(stacksize_of_object(nano_nostack) == thread_stacksize::nostack);
    HPX_TEST(stacksize_of_object(nostack_nano) == thread_stacksize::nostack);
    HPX_TEST(hpx::traits::get_task_size<decltype(nostack_nano)>::call(
                 nostack_nano) == task_size::nano);

    // the defaults are passed on to the wrapped function
    using hpx::traits::get_stack_size_v;
    using hpx::traits::get_task_size_v;
    static_assert(get_stack_size_v<decltype(nano_task(lambda)),
                      thread_stacksize::medium> == thread_stacksize::medium);
    static_assert(
        get_task_size_v<decltype(no_stack(lambda)), task_size::large> ==
        task_size::large);

    // the outer annotation of the same kind wins
    static_assert(
        size_of<decltype(with_task_size<task_size::large>(nostack_nano))> ==
        task_size::large);
    static_assert(stacksize_of<decltype(large_stack(nano_nostack))> ==
        thread_stacksize::large);

    HPX_TEST_EQ(nano_nostack(), 1);
    HPX_TEST_EQ(nostack_nano(), 1);

    // annotated_function keeps the stack size
    auto annotated = hpx::annotated_function(no_stack(lambda), "annotated");
    static_assert(
        stacksize_of<decltype(annotated)> == thread_stacksize::nostack);
    HPX_TEST(stacksize_of_object(annotated) == thread_stacksize::nostack);

    // with_task_size_of gives a function the task size of another one, but
    // not its stack size
    auto const other = hpx::threads::detail::with_task_size_of(
        [] { return 2; }, nano_task(no_stack(lambda)));
    HPX_TEST(stacksize_of_object(other) == thread_stacksize::unknown);
    HPX_TEST(hpx::traits::get_task_size<decltype(other)>::call(other) ==
        task_size::nano);

    auto const own = hpx::threads::detail::with_task_size_of(
        large_stack([] { return 3; }), nano_task(lambda));
    HPX_TEST(stacksize_of_object(own) == thread_stacksize::large);
    HPX_TEST_EQ(own(), 3);
}

///////////////////////////////////////////////////////////////////////////////
void test_set_stack_size()
{
    auto lambda = [] {};

    // a function's stack size replaces the given one
    thread_stacksize stacksize = thread_stacksize::small_;
    hpx::threads::detail::set_stack_size(stacksize, no_stack(lambda));
    HPX_TEST(stacksize == thread_stacksize::nostack);

    stacksize = thread_stacksize::current;
    hpx::threads::detail::set_stack_size(stacksize, huge_stack(lambda));
    HPX_TEST(stacksize == thread_stacksize::huge);

    // through the task size wrapper too
    stacksize = thread_stacksize::default_;
    hpx::threads::detail::set_stack_size(
        stacksize, nano_task(large_stack(lambda)));
    HPX_TEST(stacksize == thread_stacksize::large);

    // a function without one keeps the given one, unlike set_task_size
    stacksize = thread_stacksize::medium;
    hpx::threads::detail::set_stack_size(stacksize, lambda);
    HPX_TEST(stacksize == thread_stacksize::medium);

    hpx::threads::detail::set_stack_size(stacksize, nano_task(lambda));
    HPX_TEST(stacksize == thread_stacksize::medium);
}

///////////////////////////////////////////////////////////////////////////////
void test_forwarding()
{
    auto add = no_stack(&plain_function);
    HPX_TEST_EQ(add(41), 42);

    auto take = large_stack([](std::unique_ptr<int> p) { return p; });
    std::unique_ptr<int> p = take(std::make_unique<int>(7));
    HPX_TEST(p != nullptr);
    HPX_TEST_EQ(*p, 7);

    // a mutable function keeps its state between calls
    auto count = small_stack([n = 0]() mutable { return ++n; });
    count();
    HPX_TEST_EQ(count(), 2);

    // a const wrapper calls f as const
    auto which = medium_stack(const_or_not{});
    auto const& const_which = which;
    HPX_TEST_EQ(which(), 1);
    HPX_TEST_EQ(const_which(), 2);

    // a returned reference stays a reference
    int value = 0;
    auto const get = no_stack([&value]() -> int& { return value; });
    static_assert(std::is_same_v<decltype(get()), int&>);
    get() = 7;
    HPX_TEST_EQ(value, 7);
}

///////////////////////////////////////////////////////////////////////////////
void test_hpx_names()
{
    auto lambda = [] {};
    static_assert(std::is_same_v<decltype(hpx::no_stack(lambda)),
        decltype(hpx::threads::no_stack(lambda))>);
    static_assert(std::is_same_v<decltype(hpx::small_stack(lambda)),
        decltype(hpx::threads::small_stack(lambda))>);
    static_assert(std::is_same_v<decltype(hpx::medium_stack(lambda)),
        decltype(hpx::threads::medium_stack(lambda))>);
    static_assert(std::is_same_v<decltype(hpx::large_stack(lambda)),
        decltype(hpx::threads::large_stack(lambda))>);
    static_assert(std::is_same_v<decltype(hpx::huge_stack(lambda)),
        decltype(hpx::threads::huge_stack(lambda))>);
    static_assert(
        std::is_same_v<decltype(hpx::with_stack_size<thread_stacksize::large>(
                           lambda)),
            decltype(hpx::threads::with_stack_size<thread_stacksize::large>(
                lambda))>);
}

///////////////////////////////////////////////////////////////////////////////
void test_annotation()
{
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    auto f = [] { return 1; };

    auto stack_annotated = large_stack(hpx::annotated_function(f, "outer"));
    auto nested = nano_task(no_stack(hpx::annotated_function(f, "nested")));

    HPX_TEST_EQ(
        std::strcmp(hpx::traits::get_function_annotation<
                        decltype(stack_annotated)>::call(stack_annotated),
            "outer"),
        0);
    HPX_TEST_EQ(
        std::strcmp(
            hpx::traits::get_function_annotation<decltype(nested)>::call(
                nested),
            "nested"),
        0);

    HPX_TEST_EQ(hpx::traits::get_function_address<decltype(no_stack(
                    &plain_function))>::call(no_stack(&plain_function)),
        hpx::traits::get_function_address<decltype(&plain_function)>::call(
            &plain_function));

    HPX_TEST_EQ(
        hpx::async(large_stack(named_function{})).get(), std::string("named"));
#endif
}

///////////////////////////////////////////////////////////////////////////////
void test_launch()
{
    auto f = large_stack([](std::unique_ptr<int> p, int i) { return *p + i; });
    HPX_TEST_EQ(hpx::async(f, std::make_unique<int>(40), 2).get(), 42);

    hpx::latch l(2);
    int result = 0;
    hpx::post(small_stack([&](int i) {
        result = i;
        l.count_down(1);
    }),
        3);
    l.arrive_and_wait();
    HPX_TEST_EQ(result, 3);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_trait();
    test_nesting();
    test_set_stack_size();
    test_forwarding();
    test_hpx_names();
    test_annotation();
    test_launch();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
