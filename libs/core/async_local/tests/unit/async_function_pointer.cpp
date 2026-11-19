//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx::async<&f>(...) and hpx::post<&f>(...) run a plain function locally. As
// the function is a template argument, its size can come from
// hpx::traits::task_size_of or from an annotation of the function. A
// scheduler that counts the sizes of the threads it creates sees it.

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_pools.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

using hpx::threads::task_size;
using hpx::traits::task_size_of_v;

///////////////////////////////////////////////////////////////////////////////
int add(int a, int b)
{
    return a + b;
}

int add_small(int a, int b)
{
    return a + b;
}

int add_noexcept(int a, int b) noexcept
{
    return a + b;
}

void count_down(hpx::latch* l)
{
    l->count_down(1);
}

void count_down_small(hpx::latch* l)
{
    l->count_down(1);
}

int overload(int i)
{
    return i;
}

int overload(double d)
{
    return static_cast<int>(d) + 1;
}

using overload_int = int (*)(int);
using overload_double = int (*)(double);

template <>
struct hpx::traits::task_size_of<&add_small>
{
    static constexpr task_size value = task_size::small_;
};

template <>
struct hpx::traits::task_size_of<&count_down_small>
{
    static constexpr task_size value = task_size::small_;
};

template <>
struct hpx::traits::task_size_of<static_cast<overload_int>(&overload)>
{
    static constexpr task_size value = task_size::small_;
};

template <>
struct hpx::traits::task_size_of<static_cast<overload_double>(&overload)>
{
    static constexpr task_size value = task_size::large;
};

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
[[= hpx::task_size::large]] int annotated()
{
    return 42;
}

// a specialization wins over the annotation
[[= hpx::task_size::large]] int annotated_specialized()
{
    return 42;
}

template <>
struct hpx::traits::task_size_of<&annotated_specialized>
{
    static constexpr task_size value = task_size::small_;
};
#endif

///////////////////////////////////////////////////////////////////////////////
namespace {

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
}    // namespace

///////////////////////////////////////////////////////////////////////////////
void test_sizes()
{
    static_assert(task_size_of_v<&add> == task_size::unknown);
    // without a size the function is passed on as it is
    static_assert(
        std::is_same_v<decltype(hpx::detail::sized_function_pointer<&add>()),
            int (*)(int, int)>);
    static_assert(task_size_of_v<&add_small> == task_size::small_);
    static_assert(task_size_of_v<static_cast<overload_int>(&overload)> ==
        task_size::small_);
    static_assert(task_size_of_v<static_cast<overload_double>(&overload)> ==
        task_size::large);

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
    static_assert(task_size_of_v<&annotated> == task_size::large);
    static_assert(task_size_of_v<&annotated_specialized> == task_size::small_);
#endif
}

void test_values()
{
    HPX_TEST_EQ(hpx::async<&add>(1, 2).get(), 3);
    HPX_TEST_EQ(hpx::async<&add_small>(3, 4).get(), 7);
    HPX_TEST_EQ(hpx::async<&add_noexcept>(1, 4).get(), 5);
    HPX_TEST_EQ(hpx::async<static_cast<overload_int>(&overload)>(5).get(), 5);
    HPX_TEST_EQ(
        hpx::async<static_cast<overload_double>(&overload)>(5.0).get(), 6);
    HPX_TEST_EQ(hpx::async<&add_small>(hpx::launch::async, 1, 1).get(), 2);

    hpx::execution::parallel_executor exec;
    HPX_TEST_EQ(hpx::async<&add_small>(exec, 2, 2).get(), 4);

    hpx::latch l(3);
    hpx::post<&count_down>(&l);
    hpx::post<&count_down_small>(exec, &l);
    l.arrive_and_wait();
}

void test_size_reaches_thread()
{
    check_size([] { HPX_TEST_EQ(hpx::async<&add_small>(1, 2).get(), 3); },
        task_size::small_);
    check_size([] { HPX_TEST_EQ(hpx::async<&add>(1, 2).get(), 3); },
        task_size::unknown);

    // overloads, each with its own size
    check_size(
        [] {
            HPX_TEST_EQ(
                hpx::async<static_cast<overload_int>(&overload)>(1).get(), 1);
        },
        task_size::small_);
    check_size(
        [] {
            HPX_TEST_EQ(
                hpx::async<static_cast<overload_double>(&overload)>(1.0).get(),
                2);
        },
        task_size::large);

    // hpx::post
    check_size(
        [] {
            hpx::latch l(2);
            hpx::post<&count_down_small>(&l);
            l.arrive_and_wait();
        },
        task_size::small_);
    check_size(
        [] {
            hpx::latch l(2);
            hpx::post<&count_down>(&l);
            l.arrive_and_wait();
        },
        task_size::unknown);

    // with a launch policy or an executor
    check_size(
        [] {
            HPX_TEST_EQ(
                hpx::async<&add_small>(hpx::launch::async, 1, 2).get(), 3);
        },
        task_size::small_);
    check_size(
        [] {
            hpx::execution::parallel_executor exec;
            HPX_TEST_EQ(hpx::async<&add_small>(exec, 1, 2).get(), 3);
        },
        task_size::small_);
    check_size(
        [] {
            hpx::execution::parallel_executor exec;
            hpx::latch l(2);
            hpx::post<&count_down_small>(exec, &l);
            l.arrive_and_wait();
        },
        task_size::small_);

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
    check_size([] { HPX_TEST_EQ(hpx::async<&annotated>().get(), 42); },
        task_size::large);
    check_size(
        [] { HPX_TEST_EQ(hpx::async<&annotated_specialized>().get(), 42); },
        task_size::small_);
#endif
}

int hpx_main()
{
    test_sizes();
    test_values();
    test_size_reaches_thread();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // the test counts threads, so it runs on one worker
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
