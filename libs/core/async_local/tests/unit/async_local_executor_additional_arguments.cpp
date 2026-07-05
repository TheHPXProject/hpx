//  Copyright (c)      2020 ETH Zurich
//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
struct additional_argument
{
};

struct additional_argument_executor
{
    template <typename F, typename... Ts,
        typename Enable =
            std::enable_if_t<!std::is_member_function_pointer_v<F>>>
    decltype(auto) async_execute(F&& f, Ts&&... ts) const
    {
        return hpx::async(
            std::forward<F>(f), additional_argument{}, std::forward<Ts>(ts)...);
    }

    template <typename F, typename T, typename... Ts,
        typename Enable =
            std::enable_if_t<std::is_member_function_pointer_v<F>>>
    decltype(auto) async_execute(F&& f, T&& t, Ts&&... ts) const
    {
        return hpx::async(std::forward<F>(f), std::forward<T>(t),
            additional_argument{}, std::forward<Ts>(ts)...);
    }
};

namespace hpx::execution::experimental {

    template <>
    struct is_two_way_executor<additional_argument_executor> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

///////////////////////////////////////////////////////////////////////////////
std::int32_t increment(additional_argument, std::int32_t i)
{
    return i + 1;
}

std::int32_t increment_with_future(
    additional_argument, hpx::shared_future<std::int32_t> fi)
{
    return fi.get() + 1;
}

///////////////////////////////////////////////////////////////////////////////
struct mult2
{
    std::int32_t operator()(additional_argument, std::int32_t i) const
    {
        return i * 2;
    }
};

///////////////////////////////////////////////////////////////////////////////
struct decrement
{
    std::int32_t call(additional_argument, std::int32_t i) const
    {
        return i - 1;
    }
};

///////////////////////////////////////////////////////////////////////////////
void do_nothing(additional_argument, std::int32_t) {}

struct do_nothing_obj
{
    void operator()(additional_argument, std::int32_t) const {}
};

struct do_nothing_member
{
    void call(additional_argument, std::int32_t) const {}
};

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
void test_async_with_executor(Executor& exec)
{
    {
        hpx::future<std::int32_t> f1 = hpx::async(exec, &increment, 42);
        HPX_TEST_EQ(f1.get(), 43);

        hpx::future<void> f2 = hpx::async(exec, &do_nothing, 42);
        f2.get();
    }

    {
        hpx::promise<std::int32_t> p;
        hpx::shared_future<std::int32_t> f = p.get_future();

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, &increment_with_future, f);
        hpx::future<std::int32_t> f2 =
            hpx::async(exec, &increment_with_future, f);

        p.set_value(42);
        HPX_TEST_EQ(f1.get(), 43);
        HPX_TEST_EQ(f2.get(), 43);
    }

    {
        using hpx::placeholders::_1;
        using hpx::placeholders::_2;

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, hpx::bind_back(&increment, 42));
        HPX_TEST_EQ(f1.get(), 43);

        hpx::future<std::int32_t> f2 =
            hpx::async(exec, hpx::bind(&increment, _1, _2), 42);
        HPX_TEST_EQ(f2.get(), 43);
    }

    {
        hpx::future<std::int32_t> f1 = hpx::async(exec, increment, 42);
        HPX_TEST_EQ(f1.get(), 43);

        hpx::future<void> f2 = hpx::async(exec, do_nothing, 42);
        f2.get();
    }

    {
        mult2 mult;

        hpx::future<std::int32_t> f1 = hpx::async(exec, mult, 42);
        HPX_TEST_EQ(f1.get(), 84);
    }

    {
        mult2 mult;

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, hpx::bind_back(mult, 42));
        HPX_TEST_EQ(f1.get(), 84);

        using hpx::placeholders::_1;
        using hpx::placeholders::_2;

        hpx::future<std::int32_t> f2 =
            hpx::async(exec, hpx::bind(mult, _1, _2), 42);
        HPX_TEST_EQ(f2.get(), 84);

        do_nothing_obj do_nothing_f;
        hpx::future<void> f3 =
            hpx::async(exec, hpx::bind(do_nothing_f, _1, _2), 42);
        f3.get();
    }

    {
        decrement dec;

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, &decrement::call, dec, 42);
        HPX_TEST_EQ(f1.get(), 41);

        do_nothing_member dnm;
        hpx::future<void> f2 =
            hpx::async(exec, &do_nothing_member::call, dnm, 42);
        f2.get();
    }

    {
        decrement dec;

        using hpx::placeholders::_1;
        using hpx::placeholders::_2;

        hpx::future<std::int32_t> f1 =
            hpx::async(exec, hpx::bind(&decrement::call, dec, _1, 42));
        HPX_TEST_EQ(f1.get(), 41);

        hpx::future<std::int32_t> f2 =
            hpx::async(exec, hpx::bind(&decrement::call, dec, _1, _2), 42);
        HPX_TEST_EQ(f2.get(), 41);

        do_nothing_member dnm;
        hpx::future<void> f3 = hpx::async(
            exec, hpx::bind(&do_nothing_member::call, dnm, _1, _2), 42);
        f3.get();
    }
}

int hpx_main()
{
    {
        additional_argument_executor exec;
        test_async_with_executor(exec);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
