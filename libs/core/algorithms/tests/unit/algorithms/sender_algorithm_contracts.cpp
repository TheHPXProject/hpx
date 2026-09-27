//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <atomic>
#include <memory>
#include <new>
#include <stdexcept>

struct throwing_operation
{
    bool allocation;

    template <typename... Ts>
    int operator()(Ts const&...) const
    {
        if (allocation)
            throw std::bad_alloc();
        throw std::runtime_error("sender algorithm test");
    }
};

struct delayed_throwing_compare
{
    bool allocation;
    std::size_t throw_after = 256;
    std::shared_ptr<std::atomic<std::size_t>> calls =
        std::make_shared<std::atomic<std::size_t>>(0);

    bool operator()(int left, int right) const
    {
        if (calls->fetch_add(1, std::memory_order_relaxed) >= throw_after)
        {
            if (allocation)
                throw std::bad_alloc();
            throw std::runtime_error("delayed comparator test");
        }
        return left < right;
    }
};

template <typename Factory>
void test_errors(char const* name, Factory factory)
{
    for (bool allocation : {false, true})
    {
        bool caught = false;
        // Creating the sender must not execute the throwing operation.
        auto sender = factory(throwing_operation{allocation});
        try
        {
            sender_test::wait(std::move(sender));
        }
        catch (std::bad_alloc const&)
        {
            HPX_TEST(allocation);
            caught = true;
        }
        catch (hpx::exception_list const& errors)
        {
            HPX_TEST(!allocation);
            HPX_TEST_NEQ(errors.size(), std::size_t(0));
            caught = true;
        }
        catch (std::exception const& error)
        {
            HPX_TEST_MSG(
                false, (std::string(name) + ": " + error.what()).c_str());
        }
        catch (...)
        {
            HPX_TEST_MSG(false, name);
        }
        HPX_TEST(caught);
    }
}

template <typename Policy>
void test(Policy policy)
{
    auto data = sender_test::values(131073);
    std::vector<int> output(data.size());
    auto first = data.begin();
    auto last = data.end();
    auto middle = first + data.size() / 2;
    test_errors(
        "sort", [&](auto op) { return hpx::sort(policy, first, last, op); });
    test_errors("stable_sort",
        [&](auto op) { return hpx::stable_sort(policy, first, last, op); });
    test_errors("nth_element", [&](auto op) {
        return hpx::nth_element(policy, first, middle, last, op);
    });
    test_errors("partial_sort", [&](auto op) {
        return hpx::partial_sort(policy, first, middle, last, op);
    });
    test_errors("partial_sort_copy", [&](auto op) {
        return hpx::partial_sort_copy(
            policy, first, last, output.begin(), output.end(), op);
    });
    test_errors("partition",
        [&](auto op) { return hpx::partition(policy, first, last, op); });
    test_errors("stable_partition", [&](auto op) {
        return hpx::stable_partition(policy, first, last, op);
    });
    test_errors("copy_if", [&](auto op) {
        return hpx::copy_if(policy, first, last, output.begin(), op);
    });
    test_errors("remove_copy_if", [&](auto op) {
        return hpx::remove_copy_if(policy, first, last, output.begin(), op);
    });
    test_errors("unique_copy", [&](auto op) {
        return hpx::unique_copy(policy, first, last, output.begin(), op);
    });
    test_errors("exclusive_scan", [&](auto op) {
        return hpx::exclusive_scan(policy, first, last, output.begin(), 0, op);
    });
    test_errors("transform_inclusive_scan", [&](auto op) {
        return hpx::transform_inclusive_scan(
            policy, first, last, output.begin(), std::plus<int>{}, op);
    });
    // Trigger exceptions after the initial sortedness checks, inside tasks.
    auto reset = [&] {
        std::generate(
            first, last, [i = 0]() mutable { return (i++ * 37) % 1009; });
    };
    test_errors("sort delayed", [&](auto op) {
        reset();
        return hpx::sort(
            policy, first, last, delayed_throwing_compare{op.allocation});
    });
    test_errors("sort recursive", [&](auto op) {
        reset();
        return hpx::sort(policy, first, last,
            delayed_throwing_compare{op.allocation, data.size() * 2});
    });
    test_errors("stable_sort delayed", [&](auto op) {
        reset();
        return hpx::stable_sort(
            policy, first, last, delayed_throwing_compare{op.allocation});
    });
    test_errors("inplace_merge delayed", [&](auto op) {
        std::iota(first, middle, 100000);
        std::iota(middle, last, 0);
        return hpx::inplace_merge(policy, first, middle, last,
            delayed_throwing_compare{op.allocation});
    });
    if constexpr (!hpx::is_sequenced_execution_policy_v<Policy>)
    {
        auto one_chunk =
            policy.with(sender_test::ex::static_chunk_size(data.size()));
        test_errors("stable_partition single chunk", [&](auto op) {
            return hpx::stable_partition(one_chunk, first, last, op);
        });
    }
}

int hpx_main()
{
    namespace ex = sender_test::ex;
    using namespace hpx::execution;
    auto exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto sync_exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::sync));
    test(seq(task).on(sync_exec));
    test(par(task).on(exec));

    // Non-task scheduler policies complete synchronously. Ordinary task
    // policies must retain their future<void> interface.
    auto data = sender_test::values(1007);
    auto first = data.begin();
    auto last = data.end();
    static_assert(std::is_same_v<decltype(hpx::sort(par(task), first, last)),
        hpx::future<void>>);
    static_assert(
        std::is_same_v<decltype(hpx::stable_sort(par(task), first, last)),
            hpx::future<void>>);
    static_assert(std::is_same_v<decltype(hpx::nth_element(
                                     par(task), first, first, last)),
        hpx::future<void>>);
    hpx::sort(par.on(exec), first, last);
    HPX_TEST(std::is_sorted(first, last));
    hpx::stable_sort(par.on(exec), first, last);
    hpx::nth_element(par.on(exec), first, first, last);
    hpx::sort(par(task), first, last).get();
    hpx::stable_sort(par(task), first, last).get();
    hpx::nth_element(par(task), first, first, last).get();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
