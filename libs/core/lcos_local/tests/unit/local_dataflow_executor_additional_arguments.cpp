//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2015-2024 Hartmut Kaiser
//  Copyright (c) 2013 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>
#include <hpx/thread.hpp>
#include <hpx/unwrap.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using hpx::program_options::options_description;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::bind;
using hpx::dataflow;

using hpx::async;
using hpx::future;
using hpx::shared_future;

using hpx::make_ready_future;

using hpx::local::finalize;
using hpx::local::init;

using hpx::unwrapping;
using hpx::util::report_errors;

///////////////////////////////////////////////////////////////////////////////
struct additional_argument
{
};

struct additional_argument_executor
{
    template <typename F, typename... Ts>
    decltype(auto) async_execute(F&& f, Ts&&... ts) const
    {
        return hpx::async(
            std::forward<F>(f), additional_argument{}, std::forward<Ts>(ts)...);
    }

    template <typename A, typename... Ts>
    decltype(auto) post(hpx::lcos::detail::dataflow_finalization<A>&& f,
        hpx::tuple<Ts...>&& t) const
    {
        additional_argument a;
        hpx::post(f, hpx::tuple_cat(hpx::tie(a), std::move(t)));
    }
};

namespace hpx::execution::experimental {
    template <>
    struct is_one_way_executor<additional_argument_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<additional_argument_executor> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

std::atomic<std::uint32_t> void_f_count;
std::atomic<std::uint32_t> int_f_count;

void void_f(additional_argument)
{
    ++void_f_count;
}
int int_f()
{
    ++int_f_count;
    return 42;
}

std::atomic<std::uint32_t> void_f1_count;
std::atomic<std::uint32_t> int_f1_count;

void void_f1(additional_argument, int)
{
    ++void_f1_count;
}
int int_f1(additional_argument, int i)
{
    ++int_f1_count;
    return i + 42;
}

std::atomic<std::uint32_t> int_f2_count;
int int_f2(additional_argument, int l, int r)
{
    ++int_f2_count;
    return l + r;
}

std::atomic<std::uint32_t> int_f_vector_count;

int int_f_vector(additional_argument, std::vector<int> const& vf)
{
    int sum = 0;
    for (int f : vf)
    {
        sum += f;
    }
    return sum;
}

template <typename Executor>
void function_pointers(Executor& exec)
{
    void_f_count.store(0);
    int_f_count.store(0);
    void_f1_count.store(0);
    int_f1_count.store(0);
    int_f2_count.store(0);

    future<void> f1 = dataflow(exec, unwrapping(&void_f1), async(&int_f));
    future<int> f2 = dataflow(exec, unwrapping(&int_f1),
        dataflow(exec, unwrapping(&int_f1), make_ready_future(42)));
    future<int> f3 = dataflow(exec, unwrapping(&int_f2),
        dataflow(exec, unwrapping(&int_f1), make_ready_future(42)),
        dataflow(exec, unwrapping(&int_f1), make_ready_future(37)));

    int_f_vector_count.store(0);
    std::vector<future<int>> vf;
    for (std::size_t i = 0; i < 10; ++i)
    {
        vf.push_back(
            dataflow(exec, unwrapping(&int_f1), make_ready_future(42)));
    }
    future<int> f4 = dataflow(exec, unwrapping(&int_f_vector), std::move(vf));

    future<int> f5 = dataflow(exec, unwrapping(&int_f1),
        dataflow(exec, unwrapping(&int_f1), make_ready_future(42)),
        dataflow(exec, unwrapping(&void_f), make_ready_future()));

    f1.wait();
    HPX_TEST_EQ(f2.get(), 126);
    HPX_TEST_EQ(f3.get(), 163);
    HPX_TEST_EQ(f4.get(), 10 * 84);
    HPX_TEST_EQ(f5.get(), 126);
    HPX_TEST_EQ(void_f_count, 1u);
    HPX_TEST_EQ(int_f_count, 1u);
    HPX_TEST_EQ(void_f1_count, 1u);
    HPX_TEST_EQ(int_f1_count, 16u);
    HPX_TEST_EQ(int_f2_count, 1u);
}

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::uint32_t> future_void_f1_count;
std::atomic<std::uint32_t> future_void_f2_count;

void future_void_f1(additional_argument, future<void> f1)
{
    HPX_TEST(f1.is_ready());
    ++future_void_f1_count;
}

void future_void_sf1(shared_future<void> f1)
{
    HPX_TEST(f1.is_ready());
    ++future_void_f1_count;
}

void future_void_f2(additional_argument, future<void> f1, future<void> f2)
{
    HPX_TEST(f1.is_ready());
    HPX_TEST(f2.is_ready());
    ++future_void_f2_count;
}

std::atomic<std::uint32_t> future_int_f1_count;
std::atomic<std::uint32_t> future_int_f2_count;

int future_int_f1(additional_argument, future<void> f1)
{
    HPX_TEST(f1.is_ready());
    ++future_int_f1_count;
    return 1;
}

int future_int_f2(additional_argument, future<int> f1, future<int> f2)
{
    HPX_TEST(f1.is_ready());
    HPX_TEST(f2.is_ready());
    ++future_int_f2_count;
    return f1.get() + f2.get();
}

std::atomic<std::uint32_t> future_int_f_vector_count;

int future_int_f_vector(additional_argument, std::vector<future<int>>& vf)
{
    int sum = 0;
    for (future<int>& f : vf)
    {
        HPX_TEST(f.is_ready());
        sum += f.get();
    }
    ++future_int_f_vector_count;
    return sum;
}

template <typename Executor>
void future_function_pointers(Executor& exec)
{
    future_void_f1_count.store(0);
    future_void_f2_count.store(0);
    future_int_f1_count.store(0);
    future_int_f2_count.store(0);

    future<void> f1 = dataflow(exec, &future_void_f1,
        async(&future_void_sf1, shared_future<void>(make_ready_future())));

    f1.wait();

    HPX_TEST_EQ(future_void_f1_count, 2u);
    future_void_f1_count.store(0);

    future<void> f2 = dataflow(exec, &future_void_f2,
        async(&future_void_sf1, shared_future<void>(make_ready_future())),
        async(&future_void_sf1, shared_future<void>(make_ready_future())));

    f2.wait();
    HPX_TEST_EQ(future_void_f1_count, 2u);
    HPX_TEST_EQ(future_void_f2_count, 1u);

    future_void_f1_count.store(0);
    future_void_f2_count.store(0);
    future_int_f1_count.store(0);
    future_int_f2_count.store(0);

    future<int> f3 = dataflow(exec, &future_int_f1, make_ready_future());

    HPX_TEST_EQ(f3.get(), 1);
    HPX_TEST_EQ(future_int_f1_count, 1u);
    future_int_f1_count.store(0);

    future<int> f4 = dataflow(exec, &future_int_f2,
        dataflow(exec, &future_int_f1, make_ready_future()),
        dataflow(exec, &future_int_f1, make_ready_future()));

    HPX_TEST_EQ(f4.get(), 2);
    HPX_TEST_EQ(future_int_f1_count, 2u);
    HPX_TEST_EQ(future_int_f2_count, 1u);
    future_int_f1_count.store(0);
    future_int_f2_count.store(0);

    future_int_f_vector_count.store(0);
    std::vector<future<int>> vf;
    for (std::size_t i = 0; i < 10; ++i)
    {
        vf.push_back(dataflow(exec, &future_int_f1, make_ready_future()));
    }
    future<int> f5 = dataflow(exec, &future_int_f_vector, std::ref(vf));

    HPX_TEST_EQ(f5.get(), 10);
    HPX_TEST_EQ(future_int_f1_count, 10u);
    HPX_TEST_EQ(future_int_f_vector_count, 1u);
}

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::uint32_t> void_f4_count;
std::atomic<std::uint32_t> int_f4_count;

void void_f4(additional_argument, int)
{
    ++void_f4_count;
}
int int_f4(additional_argument, int i)
{
    ++int_f4_count;
    return i + 42;
}

std::atomic<std::uint32_t> void_f5_count;
std::atomic<std::uint32_t> int_f5_count;

void void_f5(additional_argument, int, hpx::future<int>)
{
    ++void_f5_count;
}
int int_f5(additional_argument, int i, hpx::future<int> j)
{
    ++int_f5_count;
    return i + j.get() + 42;
}

template <typename Executor>
void plain_arguments(Executor& exec)
{
    void_f4_count.store(0);
    int_f4_count.store(0);

    {
        future<void> f1 = dataflow(exec, &void_f4, 42);
        future<int> f2 = dataflow(exec, &int_f4, 42);

        f1.wait();
        HPX_TEST_EQ(void_f4_count, 1u);

        HPX_TEST_EQ(f2.get(), 84);
        HPX_TEST_EQ(int_f4_count, 1u);
    }

    void_f5_count.store(0);
    int_f5_count.store(0);

    {
        future<void> f1 = dataflow(exec, &void_f5, 42, async(&int_f));
        future<int> f2 = dataflow(exec, &int_f5, 42, async(&int_f));

        f1.wait();
        HPX_TEST_EQ(void_f5_count, 1u);

        HPX_TEST_EQ(f2.get(), 126);
        HPX_TEST_EQ(int_f5_count, 1u);
    }
}

template <typename Executor>
void plain_deferred_arguments(Executor& exec)
{
    void_f5_count.store(0);
    int_f5_count.store(0);

    {
        future<void> f1 =
            dataflow(exec, &void_f5, 42, async(hpx::launch::deferred, &int_f));
        future<int> f2 =
            dataflow(exec, &int_f5, 42, async(hpx::launch::deferred, &int_f));

        f1.wait();
        HPX_TEST_EQ(void_f5_count, 1u);

        HPX_TEST_EQ(f2.get(), 126);
        HPX_TEST_EQ(int_f5_count, 1u);
    }
}

inline constexpr struct void_f_wrapper
{
    // This should not be instantiated by the dataflow internals for launch
    // policies, since that would not add an additional argument and compilation
    // would fail.
    template <typename... Ts>
    auto operator()(Ts&&... ts)
    {
        return void_f(std::forward<Ts>(ts)...);
    }
} void_f_wrapper_instance{};

template <typename Executor>
void function_wrapper(Executor& exec)
{
    hpx::dataflow(exec, void_f_wrapper{});
    hpx::dataflow(exec, void_f_wrapper_instance);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map&)
{
    {
        additional_argument_executor exec;
        function_pointers(exec);
        future_function_pointers(exec);
        plain_arguments(exec);
        plain_deferred_arguments(exec);
        function_wrapper(exec);
    }

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // We force this test to use several threads by default.
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");
    return report_errors();
}
