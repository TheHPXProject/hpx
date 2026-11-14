//  Copyright (c) 2026 K S Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks hpx::traits::get_stack_size: a function has no stack size unless the
// trait is specialized for it, the default is reported instead, and a
// qualified function has the stack size of the unqualified one. Also checks
// that thread_stacksize keeps its values in the thread_support module.

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_support.hpp>

#include <cstdint>
#include <type_traits>

using hpx::threads::thread_stacksize;
using hpx::traits::get_stack_size;
using hpx::traits::get_stack_size_v;

namespace {

    int plain_function(int i)
    {
        return i + 1;
    }

    struct function_object
    {
        int operator()(int i) const
        {
            return i;
        }
    };

    // a function that knows it needs a large stack
    struct deep_recursion
    {
        void operator()() const {}
    };
}    // namespace

template <thread_stacksize Default>
struct hpx::traits::get_stack_size<deep_recursion, Default>
{
    static constexpr thread_stacksize value = thread_stacksize::large;

    [[nodiscard]] static constexpr thread_stacksize call(
        deep_recursion const&) noexcept
    {
        return value;
    }
};

///////////////////////////////////////////////////////////////////////////////
void test_enum()
{
    // the values are the ones thread_stacksize had in the coroutines module
    static_assert(
        std::is_same_v<std::underlying_type_t<thread_stacksize>, std::int8_t>);
    static_assert(static_cast<int>(thread_stacksize::unknown) == -1);
    static_assert(static_cast<int>(thread_stacksize::small_) == 1);
    static_assert(static_cast<int>(thread_stacksize::medium) == 2);
    static_assert(static_cast<int>(thread_stacksize::large) == 3);
    static_assert(static_cast<int>(thread_stacksize::huge) == 4);
    static_assert(static_cast<int>(thread_stacksize::nostack) == 5);
    static_assert(static_cast<int>(thread_stacksize::current) == 6);
    static_assert(thread_stacksize::default_ == thread_stacksize::small_);
    static_assert(thread_stacksize::minimal == thread_stacksize::small_);
    static_assert(thread_stacksize::maximal == thread_stacksize::huge);
}

///////////////////////////////////////////////////////////////////////////////
void test_unknown()
{
    // a function has no stack size of its own
    auto lambda = [](int i) { return i; };
    static_assert(
        get_stack_size_v<decltype(lambda)> == thread_stacksize::unknown);
    static_assert(get_stack_size_v<decltype(&plain_function)> ==
        thread_stacksize::unknown);
    static_assert(
        get_stack_size_v<function_object> == thread_stacksize::unknown);

    HPX_TEST(get_stack_size<decltype(lambda)>::call(lambda) ==
        thread_stacksize::unknown);
    HPX_TEST(get_stack_size<function_object>::call(function_object{}) ==
        thread_stacksize::unknown);
    HPX_TEST(get_stack_size<decltype(&plain_function)>::call(&plain_function) ==
        thread_stacksize::unknown);

    // the second argument is reported instead
    static_assert(get_stack_size_v<decltype(lambda), thread_stacksize::huge> ==
        thread_stacksize::huge);
    static_assert(
        get_stack_size_v<function_object, thread_stacksize::nostack> ==
        thread_stacksize::nostack);
    HPX_TEST((get_stack_size<decltype(lambda), thread_stacksize::medium>::call(
                  lambda) == thread_stacksize::medium));
}

///////////////////////////////////////////////////////////////////////////////
void test_specialization()
{
    // a specialization gives a function its stack size, which wins over the
    // second argument
    static_assert(get_stack_size_v<deep_recursion> == thread_stacksize::large);
    static_assert(get_stack_size_v<deep_recursion, thread_stacksize::small_> ==
        thread_stacksize::large);
    HPX_TEST(get_stack_size<deep_recursion>::call(deep_recursion{}) ==
        thread_stacksize::large);

    // a qualified function has the stack size of the unqualified one
    static_assert(
        get_stack_size_v<deep_recursion const> == thread_stacksize::large);
    static_assert(get_stack_size_v<deep_recursion&> == thread_stacksize::large);
    static_assert(
        get_stack_size_v<deep_recursion const&> == thread_stacksize::large);
    static_assert(
        get_stack_size_v<deep_recursion&&> == thread_stacksize::large);
    static_assert(
        get_stack_size_v<function_object const&, thread_stacksize::huge> ==
        thread_stacksize::huge);

    deep_recursion const f;
    HPX_TEST(get_stack_size<deep_recursion const&>::call(f) ==
        thread_stacksize::large);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    test_enum();
    test_unknown();
    test_specialization();

    return hpx::util::report_errors();
}
