//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Checks that get_task_size reads the size of a class type from its
// hpx::task_size annotation, that a size given to the function wins over it,
// that it passes through the wrappers, and that it reaches the hint of the
// thread running the function. A scheduler that counts the sizes it is asked
// to create threads with sees that.

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)

#include <hpx/functional.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

using hpx::threads::task_size;
using hpx::traits::get_task_size;
using hpx::traits::get_task_size_v;

namespace {

    struct[[= hpx::task_size::large]] annotated
    {
        int operator()() const
        {
            return 42;
        }
    };

    struct plain
    {
        int operator()() const
        {
            return 42;
        }
    };

    struct[[= hpx::task_size::small_, = hpx::task_size::small_]] annotated_twice
    {
    };

    struct marker
    {
    };

    // annotations of other types don't count
    struct[[= marker{}, = hpx::task_size::medium, = 42]] other_annotations
    {
    };

    // annotations are not inherited
    struct derived : annotated
    {
    };

    using annotated_alias = annotated;

    // an explicit unknown is the size of the type, not a missing size
    struct[[= hpx::task_size::unknown]] annotated_unknown
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct[[= hpx::task_size::medium]] annotated_template
    {
        int operator()() const
        {
            return 42;
        }
    };

    template <>
    struct annotated_template<char>
    {
    };

    template <>
    struct[[= hpx::task_size::large]] annotated_template<long>
    {
    };

    template <typename T>
    struct annotated_template<T*>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    std::array<std::atomic<int>, 5> created_with{};

    using base_scheduler =
        hpx::threads::policies::local_priority_queue_scheduler<>;

    struct counting_scheduler : base_scheduler
    {
        using base_scheduler::base_scheduler;

        void create_thread(hpx::threads::thread_init_data& data,
            hpx::threads::thread_id_ref_type* id, hpx::error_code& ec) override
        {
            ++created_with[static_cast<std::size_t>(
                data.schedulehint.task_size())];
            base_scheduler::create_thread(data, id, ec);
        }
    };

    // Runs f and checks that it created one thread of the given size and none
    // of another known size. For an unknown size no thread has a known size.
    template <typename F>
    void check_size(F&& f, task_size size)
    {
        task_size const known[] = {task_size::nano, task_size::small_,
            task_size::medium, task_size::large};
        auto const count = [](task_size s) {
            return created_with[static_cast<std::size_t>(s)].load();
        };
        int before[4];
        for (int i = 0; i != 4; ++i)
            before[i] = count(known[i]);

        HPX_FORWARD(F, f)();

        for (int i = 0; i != 4; ++i)
        {
            HPX_TEST_EQ(count(known[i]) - before[i], known[i] == size ? 1 : 0);
        }
    }

    // counts down a latch, so that it can be posted
    struct[[= hpx::task_size::large]] annotated_count_down
    {
        hpx::latch* l;

        void operator()() const
        {
            l->count_down(1);
        }
    };

    struct plain_count_down
    {
        hpx::latch* l;

        void operator()() const
        {
            l->count_down(1);
        }
    };

    template <typename F>
    void post_and_wait(task_size size)
    {
        check_size(
            [] {
                hpx::latch l(2);
                hpx::post(F{&l});
                l.arrive_and_wait();
            },
            size);
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
void test_types()
{
    static_assert(get_task_size_v<annotated> == task_size::large);
    static_assert(get_task_size_v<plain> == task_size::unknown);
    static_assert(
        get_task_size_v<plain, task_size::medium> == task_size::medium);

    // the annotation wins over the default
    static_assert(
        get_task_size_v<annotated, task_size::medium> == task_size::large);
    static_assert(get_task_size_v<annotated_unknown, task_size::medium> ==
        task_size::unknown);

    // the same value twice is fine
    static_assert(get_task_size_v<annotated_twice> == task_size::small_);

    static_assert(get_task_size_v<other_annotations> == task_size::medium);
    static_assert(get_task_size_v<annotated_alias> == task_size::large);

    // base classes are not scanned
    static_assert(get_task_size_v<derived> == task_size::unknown);

    // only class types are looked at
    auto const lambda = [] { return 42; };
    static_assert(get_task_size_v<decltype(lambda)> == task_size::unknown);
    static_assert(get_task_size_v<int (*)()> == task_size::unknown);
    static_assert(get_task_size_v<int> == task_size::unknown);
    static_assert(
        get_task_size_v<int (*)(), task_size::small_> == task_size::small_);
}

void test_qualified_types()
{
    static_assert(get_task_size_v<annotated const> == task_size::large);
    static_assert(get_task_size_v<annotated&> == task_size::large);
    static_assert(get_task_size_v<annotated const&> == task_size::large);
}

// A specialization of a class template has the annotations of the template it
// is instantiated from, so a partial or explicit specialization doesn't get the
// ones of the primary template and has to be annotated itself.
void test_class_templates()
{
    static_assert(
        get_task_size_v<annotated_template<int>> == task_size::medium);
    static_assert(
        get_task_size_v<annotated_template<int> const&> == task_size::medium);

    static_assert(
        get_task_size_v<annotated_template<char>> == task_size::unknown);
    static_assert(
        get_task_size_v<annotated_template<long>> == task_size::large);
    static_assert(
        get_task_size_v<annotated_template<int*>> == task_size::unknown);
}

void test_call()
{
    annotated const f;
    HPX_TEST(get_task_size<annotated>::call(f) == task_size::large);
    HPX_TEST((get_task_size<annotated, task_size::small_>::call(f) ==
        task_size::large));

    // the size of a call, which the algorithms use
    HPX_TEST((hpx::traits::get_task_size_for<annotated, int>::call(f) ==
        task_size::large));
}

void test_sized_function_wins()
{
    auto const sized = hpx::small_task(annotated{});
    static_assert(get_task_size_v<decltype(sized)> == task_size::small_);
    HPX_TEST(get_task_size<decltype(sized)>::call(sized) == task_size::small_);
}

template <typename F>
void check_wrapper(F const& f, task_size size)
{
    HPX_TEST(get_task_size_v<F> == size);
    HPX_TEST(get_task_size<F>::call(f) == size);
}

void test_wrappers()
{
    check_wrapper(hpx::bind(annotated{}), task_size::large);
    check_wrapper(hpx::bind_front(annotated{}), task_size::large);
    check_wrapper(hpx::util::deferred_call(annotated{}), task_size::large);
    check_wrapper(hpx::annotated_function(annotated{}, "f"), task_size::large);

    // the size is found in a function that is stored, at run time only
    hpx::function<int()> const stored = annotated{};
    HPX_TEST(get_task_size<decltype(stored)>::call(stored) == task_size::large);
}

void test_size_reaches_thread()
{
    // hpx::post
    post_and_wait<annotated_count_down>(task_size::large);
    post_and_wait<plain_count_down>(task_size::unknown);

    // hpx::async
    check_size([] { HPX_TEST_EQ(hpx::async(annotated{}).get(), 42); },
        task_size::large);
}

int hpx_main()
{
    test_types();
    test_qualified_types();
    test_class_templates();
    test_call();
    test_sized_function_wins();
    test_wrappers();
    test_size_reaches_thread();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // the tests count threads, so the test runs on one worker
    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.os_threads=1"};
    init_args.rp_callback = [](auto& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            [](hpx::threads::thread_pool_init_parameters thread_pool_init,
                hpx::threads::policies::thread_queue_init_parameters
                    thread_queue_init)
                -> std::unique_ptr<hpx::threads::thread_pool_base> {
                counting_scheduler::init_parameter_type init(
                    thread_pool_init.num_threads_,
                    thread_pool_init.affinity_data_, std::size_t(-1),
                    thread_queue_init);
                auto scheduler = std::make_unique<counting_scheduler>(init);

                return std::make_unique<hpx::threads::detail::
                        scheduled_thread_pool<counting_scheduler>>(
                    std::move(scheduler), thread_pool_init);
            });
    };

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);
    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
