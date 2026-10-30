//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks with_task_size and its shorthands: the size is seen through
// get_task_size (also through the binders, annotated_function and
// deferred_call), the wrapper forwards arguments, serializes, and runs through
// async, post, an executor, future::then and dataflow.

#include <hpx/execution.hpp>
#include <hpx/functional.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using hpx::threads::large_task;
using hpx::threads::medium_task;
using hpx::threads::nano_task;
using hpx::threads::small_task;
using hpx::threads::task_size;
using hpx::threads::with_task_size;

namespace {

    int plain_function(int i)
    {
        return i + 1;
    }

    // a public member only, so it is serialized without a serialize function
    struct add_n
    {
        int n;

        int operator()(int i) const
        {
            return i + n;
        }
    };

    // can only be called as an lvalue, which is how the wrapper calls f
    struct lvalue_only
    {
        int operator()() &
        {
            return 3;
        }
    };

    struct const_lvalue_only
    {
        int operator()() const&
        {
            return 4;
        }
    };

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
    inline constexpr task_size size_of = hpx::traits::get_task_size_v<F>;

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    // Has a name only through the trait, so the name its thread gets must
    // come from sized_function forwarding get_function_annotation.
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
    static_assert(size_of<decltype(lambda)> == task_size::unknown);
    static_assert(size_of<decltype(&plain_function)> == task_size::unknown);

    auto large = with_task_size<task_size::large>(lambda);
    static_assert(size_of<decltype(large)> == task_size::large);
    static_assert(
        size_of<decltype(with_task_size<task_size::small_>(&plain_function))> ==
        task_size::small_);
    static_assert(
        size_of<decltype(with_task_size<task_size::medium>(lambda))> ==
        task_size::medium);

    // the shorthands give the same types as with_task_size
    static_assert(std::is_same_v<decltype(nano_task(lambda)),
        decltype(with_task_size<task_size::nano>(lambda))>);
    static_assert(std::is_same_v<decltype(small_task(lambda)),
        decltype(with_task_size<task_size::small_>(lambda))>);
    static_assert(std::is_same_v<decltype(medium_task(lambda)),
        decltype(with_task_size<task_size::medium>(lambda))>);
    static_assert(std::is_same_v<decltype(large_task(&plain_function)),
        decltype(with_task_size<task_size::large>(&plain_function))>);
    static_assert(size_of<decltype(small_task(lambda))> == task_size::small_);

    // a qualified function has the size of the unqualified one
    using large_type = decltype(large);
    static_assert(size_of<large_type const> == task_size::large);
    static_assert(size_of<large_type&> == task_size::large);
    static_assert(size_of<large_type const&> == task_size::large);
    static_assert(size_of<large_type&&> == task_size::large);
    static_assert(
        hpx::traits::get_task_size_v<large_type const&, task_size::small_> ==
        task_size::large);
    HPX_TEST(hpx::traits::get_task_size<large_type const>::call(large) ==
        task_size::large);

    // the size is a template argument, so the wrapper is as big as f
    static_assert(sizeof(large) == sizeof(lambda));

    // hpx::async binds its arguments with deferred_call
    static_assert(size_of<decltype(hpx::util::deferred_call(large, 1))> ==
        task_size::large);
    static_assert(size_of<decltype(hpx::util::deferred_call(lambda, 1))> ==
        task_size::unknown);

    // the second argument is reported when the function has no size, and a
    // wrapper's size wins over it
    using hpx::traits::get_task_size_v;
    static_assert(get_task_size_v<decltype(lambda), task_size::large> ==
        task_size::large);
    static_assert(get_task_size_v<decltype(large), task_size::small_> ==
        task_size::large);
    static_assert(get_task_size_v<decltype(hpx::util::deferred_call(lambda, 1)),
                      task_size::medium> == task_size::medium);
    static_assert(get_task_size_v<decltype(hpx::annotated_function(lambda)),
                      task_size::medium> == task_size::medium);

    // each size stands for a hundred times the run time of the one below it,
    // so a hundred nano calls make a small task, a hundred small calls a
    // medium one and a hundred medium calls a large one
    using hpx::bulk_task_size;
    using hpx::task_size_duration;
    using namespace std::chrono_literals;
    static_assert(task_size_duration(task_size::unknown) == 0ns);
    static_assert(task_size_duration(task_size::nano) == 1ns);
    static_assert(task_size_duration(task_size::small_) == 100ns);
    static_assert(task_size_duration(task_size::medium) == 10us);
    static_assert(task_size_duration(task_size::large) == 1ms);

    static_assert(bulk_task_size(task_size::nano, 1) == task_size::nano);
    static_assert(bulk_task_size(task_size::nano, 99) == task_size::nano);
    static_assert(bulk_task_size(task_size::nano, 100) == task_size::small_);
    static_assert(bulk_task_size(task_size::nano, 9999) == task_size::small_);
    static_assert(bulk_task_size(task_size::nano, 10000) == task_size::medium);
    static_assert(bulk_task_size(task_size::nano, 1000000) == task_size::large);
    static_assert(bulk_task_size(task_size::small_, 1) == task_size::small_);
    static_assert(bulk_task_size(task_size::small_, 99) == task_size::small_);
    static_assert(bulk_task_size(task_size::small_, 100) == task_size::medium);
    static_assert(bulk_task_size(task_size::small_, 9999) == task_size::medium);
    static_assert(bulk_task_size(task_size::small_, 10000) == task_size::large);
    static_assert(bulk_task_size(task_size::medium, 1) == task_size::medium);
    static_assert(bulk_task_size(task_size::medium, 99) == task_size::medium);
    static_assert(bulk_task_size(task_size::medium, 100) == task_size::large);
    static_assert(
        bulk_task_size(task_size::medium, std::size_t(-1)) == task_size::large);
    static_assert(bulk_task_size(task_size::large, 1) == task_size::large);
    static_assert(
        bulk_task_size(task_size::unknown, 1000) == task_size::unknown);

    // the binders see through to the function they call
    using hpx::placeholders::_1;
    static_assert(size_of<decltype(hpx::bind(large, _1))> == task_size::large);
    static_assert(
        size_of<decltype(hpx::bind_front(large, 1))> == task_size::large);
    static_assert(
        size_of<decltype(hpx::bind_back(large, 1))> == task_size::large);
    static_assert(
        size_of<decltype(hpx::util::one_shot(large))> == task_size::large);
    static_assert(
        size_of<decltype(hpx::bind(lambda, _1))> == task_size::unknown);
    static_assert(get_task_size_v<decltype(hpx::bind_front(lambda, 1)),
                      task_size::small_> == task_size::small_);
}

///////////////////////////////////////////////////////////////////////////////
void test_forwarding()
{
    auto add = with_task_size<task_size::small_>(&plain_function);
    HPX_TEST_EQ(add(41), 42);

    auto take = with_task_size<task_size::medium>(
        [](std::unique_ptr<int> p) { return p; });
    std::unique_ptr<int> p = take(std::make_unique<int>(7));
    HPX_TEST(p != nullptr);
    HPX_TEST_EQ(*p, 7);

    // a mutable function keeps its state between calls
    auto count =
        with_task_size<task_size::small_>([n = 0]() mutable { return ++n; });
    count();
    HPX_TEST_EQ(count(), 2);

    // lvalue arguments are passed through as references
    int value = 0;
    auto set = with_task_size<task_size::small_>([](int& i) { i = 5; });
    set(value);
    HPX_TEST_EQ(value, 5);

    // a const wrapper calls f as const, through a const reference too
    auto which = small_task(const_or_not{});
    auto const& const_which = which;
    HPX_TEST_EQ(which(), 1);
    HPX_TEST_EQ(const_which(), 2);

    auto const add_const = small_task(add_n{41});
    HPX_TEST_EQ(add_const(1), 42);

    // f is called as an lvalue, so callables that only allow that work; only
    // the non-const call operator can call an operator() &
    auto lvalue = small_task(lvalue_only{});
    HPX_TEST_EQ(lvalue(), 3);

    auto const_lvalue = small_task(const_lvalue_only{});
    auto const& const_lvalue_ref = const_lvalue;
    HPX_TEST_EQ(const_lvalue(), 4);
    HPX_TEST_EQ(const_lvalue_ref(), 4);

    // a returned reference stays a reference
    auto const get = small_task([&value]() -> int& { return value; });
    static_assert(std::is_same_v<decltype(get()), int&>);
    get() = 7;
    HPX_TEST_EQ(value, 7);
}

///////////////////////////////////////////////////////////////////////////////
void test_hpx_names()
{
    auto lambda = [] {};
    static_assert(std::is_same_v<hpx::task_size, hpx::threads::task_size>);
    static_assert(std::is_same_v<decltype(hpx::small_task(lambda)),
        decltype(hpx::threads::small_task(lambda))>);
    static_assert(std::is_same_v<decltype(hpx::medium_task(lambda)),
        decltype(hpx::threads::medium_task(lambda))>);
    static_assert(std::is_same_v<decltype(hpx::large_task(lambda)),
        decltype(hpx::threads::large_task(lambda))>);
    static_assert(size_of<decltype(hpx::with_task_size<hpx::task_size::medium>(
                      lambda))> == hpx::task_size::medium);
}

///////////////////////////////////////////////////////////////////////////////
void test_hint()
{
    using hpx::threads::thread_schedule_hint;
    using hpx::threads::thread_schedule_hint_mode;

    static_assert(sizeof(thread_schedule_hint) == 4);

    thread_schedule_hint hint;
    HPX_TEST(hint.task_size() == task_size::unknown);
    hint.task_size(task_size::medium);
    HPX_TEST(hint.task_size() == task_size::medium);
    HPX_TEST(hint != thread_schedule_hint());

    // every part keeps its value next to the others, numa (2) included, which
    // needs the mode stored unsigned
    thread_schedule_hint const all(thread_schedule_hint_mode::numa, 5,
        hpx::threads::thread_placement_hint::depth_first,
        hpx::threads::thread_execution_hint::run_as_child,
        hpx::threads::thread_sharing_hint::do_not_combine_tasks,
        task_size::large);
    HPX_TEST(all.mode() == thread_schedule_hint_mode::numa);
    HPX_TEST_EQ(all.hint, 5);
    HPX_TEST(all.task_size() == task_size::large);
    HPX_TEST(all.placement_mode() ==
        hpx::threads::thread_placement_hint::depth_first);
    HPX_TEST(all.runs_as_child_mode() ==
        hpx::threads::thread_execution_hint::run_as_child);
    HPX_TEST(all.sharing_mode() ==
        hpx::threads::thread_sharing_hint::do_not_combine_tasks);

    // a function's size replaces the one in the hint, and a function without
    // one leaves the hint without one
    auto lambda = [] {};
    thread_schedule_hint h(task_size::small_);
    hpx::threads::detail::set_task_size(h, lambda);
    HPX_TEST(h.task_size() == task_size::unknown);
    hpx::threads::detail::set_task_size(h, large_task(lambda));
    HPX_TEST(h.task_size() == task_size::large);
}

///////////////////////////////////////////////////////////////////////////////
template <typename F>
task_size size_of_object(F const& f)
{
    return hpx::traits::get_task_size<F>::call(f);
}

void test_type_erased()
{
    // a type erased function has the size of the function stored in it, which
    // is known at run time only
    hpx::move_only_function<int()> move_only = large_task([] { return 1; });
    static_assert(size_of<decltype(move_only)> == task_size::unknown);
    HPX_TEST(size_of_object(move_only) == task_size::large);

    hpx::function<int()> const f = small_task([] { return 1; });
    HPX_TEST(size_of_object(f) == task_size::small_);
    HPX_TEST_EQ(f(), 1);

    // a function without a size has none, or the default
    hpx::function<int()> const plain = [] { return 1; };
    HPX_TEST(size_of_object(plain) == task_size::unknown);
    HPX_TEST((hpx::traits::get_task_size<hpx::function<int()>,
                  task_size::medium>::call(plain) == task_size::medium));
    HPX_TEST(size_of_object(hpx::function<int()>()) == task_size::unknown);

    // the wrappers see through to it
    using hpx::placeholders::_1;
    HPX_TEST(size_of_object(hpx::util::deferred_call(f)) == task_size::small_);
    HPX_TEST(size_of_object(hpx::bind(f)) == task_size::small_);
    HPX_TEST(size_of_object(hpx::bind_front(f)) == task_size::small_);
    HPX_TEST(size_of_object(hpx::bind_back(f)) == task_size::small_);
    HPX_TEST(size_of_object(hpx::util::one_shot(f)) == task_size::small_);
    HPX_TEST(
        size_of_object(hpx::annotated_function(f, "f")) == task_size::small_);

    // so do type erased functions storing a wrapper
    hpx::function<int()> const bound = hpx::bind_front(f);
    HPX_TEST(size_of_object(bound) == task_size::small_);

    // with_task_size_of gives a function the size of another one
    auto const other =
        hpx::threads::detail::with_task_size_of(f, [] { return 2; });
    HPX_TEST(size_of_object(other) == task_size::small_);
    HPX_TEST_EQ(other(), 2);

    // and set_task_size takes the size from the function object
    hpx::threads::thread_schedule_hint h;
    hpx::threads::detail::set_task_size(h, f);
    HPX_TEST(h.task_size() == task_size::small_);
}

///////////////////////////////////////////////////////////////////////////////
void test_serialization()
{
    auto const f = large_task(add_n{41});

    std::vector<char> buffer;
    {
        hpx::serialization::output_archive oarchive(buffer);
        oarchive << f;
    }

    std::decay_t<decltype(f)> loaded{add_n{0}};
    {
        hpx::serialization::input_archive iarchive(buffer);
        iarchive >> loaded;
    }
    HPX_TEST_EQ(loaded(1), 42);
}

///////////////////////////////////////////////////////////////////////////////
void test_launch()
{
    auto f = with_task_size<task_size::large>(
        [](std::unique_ptr<int> p, int i) { return *p + i; });

    HPX_TEST_EQ(hpx::async(f, std::make_unique<int>(40), 2).get(), 42);
    HPX_TEST_EQ(
        hpx::async(hpx::launch::fork, f, std::make_unique<int>(1), 1).get(), 2);

    hpx::latch l(2);
    int result = 0;
    hpx::post(with_task_size<task_size::small_>([&](int i) {
        result = i;
        l.count_down(1);
    }),
        3);
    l.arrive_and_wait();
    HPX_TEST_EQ(result, 3);

    hpx::execution::parallel_executor exec;
    HPX_TEST_EQ(hpx::async(exec, f, std::make_unique<int>(10), 5).get(), 15);
    HPX_TEST_EQ(hpx::parallel::execution::sync_execute(
                    exec, f, std::make_unique<int>(1), 2),
        3);

    HPX_TEST_EQ(hpx::make_ready_future(1)
                    .then(with_task_size<task_size::small_>(
                        [](hpx::future<int> fut) { return fut.get() + 1; }))
                    .get(),
        2);

    HPX_TEST_EQ(
        hpx::dataflow(with_task_size<task_size::large>(
                          [](hpx::future<int> fut) { return fut.get(); }),
            hpx::make_ready_future(3))
            .get(),
        3);
}

///////////////////////////////////////////////////////////////////////////////
void test_composition()
{
    auto f = [] { return 1; };

    auto sized_annotated =
        with_task_size<task_size::large>(hpx::annotated_function(f, "outer"));
    auto annotated_sized =
        hpx::annotated_function(with_task_size<task_size::medium>(f), "inner");

    static_assert(size_of<decltype(sized_annotated)> == task_size::large);
    static_assert(size_of<decltype(annotated_sized)> == task_size::medium);
    static_assert(
        size_of<decltype(hpx::util::deferred_call(sized_annotated))> ==
        task_size::large);

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    using sized_annotated_type = decltype(sized_annotated);
    using annotated_sized_type = decltype(annotated_sized);

    HPX_TEST_EQ(
        std::strcmp(
            hpx::traits::get_function_annotation<sized_annotated_type>::call(
                sized_annotated),
            "outer"),
        0);
    HPX_TEST_EQ(
        std::strcmp(
            hpx::traits::get_function_annotation<annotated_sized_type>::call(
                annotated_sized),
            "inner"),
        0);

    HPX_TEST_EQ(
        hpx::async(with_task_size<task_size::large>(named_function{})).get(),
        std::string("named"));
#endif

    HPX_TEST_EQ(sized_annotated(), 1);
    HPX_TEST_EQ(annotated_sized(), 1);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_trait();
    test_forwarding();
    test_hpx_names();
    test_hint();
    test_type_erased();
    test_serialization();
    test_launch();
    test_composition();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
