//  Copyright (c) 2026 K S Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks with_annotations: the function gets both a task size and a stack
// size, it is the same as nesting with_task_size and with_stack_size, wrapping
// it again replaces only the annotation given, set_task_size and
// set_stack_size read both, the function's address and annotation are kept,
// and it runs through async and post.

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

using hpx::threads::task_size;
using hpx::threads::thread_stacksize;
using hpx::threads::with_annotations;

namespace {

    int plain_function(int i)
    {
        return i + 1;
    }

    template <typename F>
    inline constexpr task_size size_of = hpx::traits::get_task_size_v<F>;

    template <typename F>
    inline constexpr thread_stacksize stack_size_of =
        hpx::traits::get_stack_size_v<F>;

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    // Has a name only through the trait, so the name its thread gets must
    // come from the wrappers forwarding get_function_annotation.
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
void test_both_sizes()
{
    auto lambda = [](int i) { return i; };
    auto f =
        with_annotations<task_size::nano, thread_stacksize::nostack>(lambda);

    static_assert(size_of<decltype(f)> == task_size::nano);
    static_assert(stack_size_of<decltype(f)> == thread_stacksize::nostack);
    HPX_TEST(
        hpx::traits::get_task_size<decltype(f)>::call(f) == task_size::nano);
    HPX_TEST(hpx::traits::get_stack_size<decltype(f)>::call(f) ==
        thread_stacksize::nostack);

    // a qualified function has the sizes of the unqualified one
    static_assert(size_of<decltype(f) const&> == task_size::nano);
    static_assert(
        stack_size_of<decltype(f) const&> == thread_stacksize::nostack);

    // the same as nesting the two wrappers, so no storage is added
    static_assert(std::is_same_v<decltype(f),
        decltype(hpx::threads::nano_task(hpx::threads::no_stack(lambda)))>);
    static_assert(std::is_same_v<decltype(with_annotations<task_size::large,
                                     thread_stacksize::huge>(&plain_function)),
        decltype(hpx::threads::with_task_size<task_size::large>(
            hpx::threads::with_stack_size<thread_stacksize::huge>(
                &plain_function)))>);
    static_assert(sizeof(f) == sizeof(lambda));

    HPX_TEST_EQ(f(41), 41);
    auto add = with_annotations<task_size::small_, thread_stacksize::large>(
        &plain_function);
    HPX_TEST_EQ(add(41), 42);
}

///////////////////////////////////////////////////////////////////////////////
void test_replacing()
{
    auto lambda = [] { return 1; };
    auto f =
        with_annotations<task_size::large, thread_stacksize::nostack>(lambda);

    // wrapping again replaces only the annotation given
    auto size_replaced = hpx::threads::nano_task(f);
    static_assert(size_of<decltype(size_replaced)> == task_size::nano);
    static_assert(
        stack_size_of<decltype(size_replaced)> == thread_stacksize::nostack);

    auto stack_replaced = hpx::threads::large_stack(f);
    static_assert(size_of<decltype(stack_replaced)> == task_size::large);
    static_assert(
        stack_size_of<decltype(stack_replaced)> == thread_stacksize::large);

    // and the combined form replaces both
    auto both_replaced =
        with_annotations<task_size::small_, thread_stacksize::medium>(f);
    static_assert(size_of<decltype(both_replaced)> == task_size::small_);
    static_assert(
        stack_size_of<decltype(both_replaced)> == thread_stacksize::medium);

    HPX_TEST_EQ(size_replaced(), 1);
    HPX_TEST_EQ(stack_replaced(), 1);
    HPX_TEST_EQ(both_replaced(), 1);

    // set_task_size and set_stack_size take the sizes from the function
    hpx::threads::thread_schedule_hint hint;
    hpx::threads::detail::set_task_size(hint, f);
    HPX_TEST(hint.task_size() == task_size::large);

    thread_stacksize stack_size = thread_stacksize::small_;
    hpx::threads::detail::set_stack_size(stack_size, f);
    HPX_TEST(stack_size == thread_stacksize::nostack);
}

///////////////////////////////////////////////////////////////////////////////
void test_hpx_names()
{
    auto lambda = [] {};
    static_assert(
        std::is_same_v<decltype(hpx::with_annotations<task_size::small_,
                           thread_stacksize::large>(lambda)),
            decltype(hpx::threads::with_annotations<task_size::small_,
                thread_stacksize::large>(lambda))>);
}

///////////////////////////////////////////////////////////////////////////////
void test_annotation()
{
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    auto annotated =
        with_annotations<task_size::small_, thread_stacksize::large>(
            hpx::annotated_function([] { return 1; }, "annotated"));
    HPX_TEST_EQ(
        std::strcmp(
            hpx::traits::get_function_annotation<decltype(annotated)>::call(
                annotated),
            "annotated"),
        0);
    HPX_TEST_EQ(annotated(), 1);

    auto addressed =
        with_annotations<task_size::nano, thread_stacksize::nostack>(
            &plain_function);
    HPX_TEST_EQ(
        hpx::traits::get_function_address<decltype(addressed)>::call(addressed),
        hpx::traits::get_function_address<decltype(&plain_function)>::call(
            &plain_function));

    HPX_TEST_EQ(
        hpx::async(with_annotations<task_size::small_, thread_stacksize::large>(
                       named_function{}))
            .get(),
        std::string("named"));
#endif
}

///////////////////////////////////////////////////////////////////////////////
void test_launch()
{
    auto f = with_annotations<task_size::large, thread_stacksize::large>(
        [](std::unique_ptr<int> p, int i) { return *p + i; });
    HPX_TEST_EQ(hpx::async(f, std::make_unique<int>(40), 2).get(), 42);

    hpx::latch l(2);
    int result = 0;
    hpx::post(with_annotations<task_size::small_, thread_stacksize::medium>(
                  [&](int i) {
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
    test_both_sizes();
    test_replacing();
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
