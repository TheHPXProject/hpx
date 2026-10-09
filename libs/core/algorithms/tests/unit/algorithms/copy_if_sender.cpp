//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_utils.hpp"

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

struct scan_lifecycle_state
{
    std::atomic<std::size_t> begin{0};
    std::atomic<std::size_t> end_scheduling{0};
    std::atomic<std::size_t> end{0};
    std::atomic<std::size_t> writes_after_end_scheduling{0};
};

struct scan_lifecycle_parameters
{
    std::shared_ptr<scan_lifecycle_state> state;

    template <typename Executor>
    void mark_begin_execution(Executor&&) const noexcept
    {
        ++state->begin;
    }

    template <typename Executor>
    void mark_end_of_scheduling(Executor&&) const noexcept
    {
        ++state->end_scheduling;
    }

    template <typename Executor>
    void mark_end_execution(Executor&&) const noexcept
    {
        ++state->end;
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    scan_lifecycle_parameters> : std::true_type
{
};

struct scan_output_value
{
    std::shared_ptr<scan_lifecycle_state> state;
    int value = -1;

    scan_output_value& operator=(int new_value) noexcept
    {
        if (state->end_scheduling.load() >= state->begin.load())
        {
            ++state->writes_after_end_scheduling;
        }
        value = new_value;
        return *this;
    }
};

enum class scan_failure_phase
{
    first,
    prefix,
    final
};

auto make_scan_partitioner_sender(scan_failure_phase phase)
{
    using namespace hpx::execution;

    static std::vector<int> input(128, 1);
    auto exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = par(task).with(ex::static_chunk_size(8)).on(exec);
    using policy_type = std::decay_t<decltype(policy)>;
    using partitioner = hpx::parallel::util::scan_partitioner<policy_type,
        std::size_t, std::size_t>;

    return partitioner::call(
        policy, input.begin(), input.size(), std::size_t(0),
        [phase](auto, std::size_t size) -> std::size_t {
            if (phase == scan_failure_phase::first)
            {
                throw std::runtime_error("first scan phase");
            }
            return size;
        },
        [phase](std::size_t lhs, std::size_t rhs) -> std::size_t {
            if (phase == scan_failure_phase::prefix)
            {
                throw std::runtime_error("scan prefix combination");
            }
            return lhs + rhs;
        },
        [phase](auto, std::size_t, std::size_t) {
            if (phase == scan_failure_phase::final)
            {
                throw std::runtime_error("final scan phase");
            }
        },
        [](std::vector<std::size_t>&& results,
            std::vector<hpx::future<void>>&&) { return results.back(); });
}

void test_scan_partitioner_exceptions()
{
    for (scan_failure_phase phase : {scan_failure_phase::first,
             scan_failure_phase::prefix, scan_failure_phase::final})
    {
        auto sender = make_scan_partitioner_sender(phase);
        bool caught = false;
        try
        {
            tt::sync_wait(HPX_MOVE(sender));
        }
        catch (hpx::exception_list const& errors)
        {
            HPX_TEST_NEQ(errors.size(), std::size_t(0));
            caught = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught);
    }
}

void test_copy_if_sender_lifecycle()
{
    using namespace hpx::execution;

    auto exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    std::vector<int> input{1, 2, 3, 4, 5, 6};
    std::vector<int> output(input.size(), -1);

    // Constructing and destroying an unstarted sender has no lifecycle
    // effects.
    {
        auto state = std::make_shared<scan_lifecycle_state>();
        auto policy = par(task).with(scan_lifecycle_parameters{state}).on(exec);
        {
            auto sender = hpx::copy_if(policy, input.begin(), input.end(),
                output.begin(), [](int value) { return value % 2 == 0; });
            HPX_TEST_EQ(state->begin.load(), std::size_t(0));
            HPX_TEST_EQ(state->end_scheduling.load(), std::size_t(0));
            HPX_TEST_EQ(state->end.load(), std::size_t(0));
        }
        HPX_TEST_EQ(state->begin.load(), std::size_t(0));
        HPX_TEST_EQ(state->end_scheduling.load(), std::size_t(0));
        HPX_TEST_EQ(state->end.load(), std::size_t(0));
    }

    // Each copy creates independent scan state when it is started.
    auto state = std::make_shared<scan_lifecycle_state>();
    std::vector<scan_output_value> observed_output(
        input.size(), scan_output_value{state});
    auto policy = par(task).with(scan_lifecycle_parameters{state}).on(exec);
    auto sender = hpx::copy_if(policy, input.begin(), input.end(),
        observed_output.begin(), [](int value) { return value % 2 == 0; });
    static_assert(std::is_copy_constructible_v<decltype(sender)>);
    std::vector<int> const expected{2, 4, 6};

    for (std::size_t invocation = 1; invocation != 3; ++invocation)
    {
        auto operation = sender;
        auto result = tt::sync_wait(HPX_MOVE(operation));
        HPX_TEST(result.has_value());
        HPX_TEST_EQ(state->begin.load(), invocation);
        HPX_TEST_EQ(state->end_scheduling.load(), invocation);
        HPX_TEST_EQ(state->end.load(), invocation);
        HPX_TEST_EQ(state->writes_after_end_scheduling.load(), std::size_t(0));
        HPX_TEST(
            std::equal(observed_output.begin(), observed_output.begin() + 3,
                expected.begin(), [](scan_output_value const& lhs, int rhs) {
                    return lhs.value == rhs;
                }));
        for (auto& value : observed_output)
        {
            value.value = -1;
        }
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_copy_if_scheduler(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(!hpx::is_async_execution_policy_v<ExPolicy>);

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using scheduler_type = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> input{1, 2, 3, 4, 5, 6};
    std::vector<int> output(input.size(), -1);
    auto exec = ex::explicit_scheduler_executor(scheduler_type(ln_policy));

    auto result = hpx::copy_if(ex_policy.on(exec), iterator(input.begin()),
        iterator(input.end()), output.begin(),
        [](int value) { return value % 2 == 0; });
    static_assert(std::is_same_v<decltype(result), base_iterator>);

    HPX_TEST(result == output.begin() + 3);
    std::vector<int> const expected{2, 4, 6};
    HPX_TEST(std::equal(expected.begin(), expected.end(), output.begin()));
    HPX_TEST(std::all_of(
        result, output.end(), [](int value) { return value == -1; }));
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_copy_if_sender_case(LnPolicy ln_policy, ExPolicy&& ex_policy,
    IteratorTag, std::vector<int> input, std::vector<int> const& expected)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>);

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using scheduler_type = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> output(input.size(), -1);
    auto exec = ex::explicit_scheduler_executor(scheduler_type(ln_policy));

    auto sender =
        ex::just(iterator(input.begin()), iterator(input.end()), output.begin(),
            [](int value) { return value % 2 == 0; }) |
        hpx::copy_if(ex_policy.on(exec));
    static_assert(ex::is_sender_v<decltype(sender)>);

    auto result = tt::sync_wait(std::move(sender));
    HPX_TEST(result.has_value());
    if (!result.has_value())
    {
        return;
    }

    auto const output_end = hpx::get<0>(*result);
    HPX_TEST(output_end ==
        output.begin() + static_cast<std::ptrdiff_t>(expected.size()));
    HPX_TEST(std::equal(expected.begin(), expected.end(), output.begin()));
    HPX_TEST(std::all_of(
        output_end, output.end(), [](int value) { return value == -1; }));
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_copy_if_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    test_copy_if_sender_case(
        ln_policy, ex_policy, IteratorTag{}, {1, 2, 3, 4, 5, 6}, {2, 4, 6});
    test_copy_if_sender_case(ln_policy, ex_policy, IteratorTag{}, {}, {});
    test_copy_if_sender_case(
        ln_policy, ex_policy, IteratorTag{}, {2, 4, 6}, {2, 4, 6});
    test_copy_if_sender_case(
        ln_policy, ex_policy, IteratorTag{}, {1, 3, 5}, {});
}

template <typename Exception, typename LnPolicy, typename ExPolicy,
    typename IteratorTag>
void test_copy_if_sender_exception(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>);

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;
    using scheduler_type = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<int> input{1, 2, 3, 4};
    std::vector<int> output(input.size());
    auto exec = ex::explicit_scheduler_executor(scheduler_type(ln_policy));

    bool caught_expected_exception = false;
    try
    {
        tt::sync_wait(
            ex::just(iterator(input.begin()), iterator(input.end()),
                output.begin(),
                [](int) -> bool {
                    if constexpr (std::is_same_v<Exception, std::runtime_error>)
                    {
                        throw std::runtime_error("test");
                    }
                    else
                    {
                        throw std::bad_alloc();
                    }
                }) |
            hpx::copy_if(ex_policy.on(exec)));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& errors)
    {
        if constexpr (std::is_same_v<Exception, std::runtime_error>)
        {
            test::test_num_exceptions<ExPolicy, IteratorTag>::call(
                ex_policy, errors);

            bool all_exceptions_expected = errors.begin() != errors.end();
            for (std::exception_ptr const& error : errors)
            {
                bool is_expected_exception = false;
                try
                {
                    std::rethrow_exception(error);
                }
                catch (std::runtime_error const&)
                {
                    is_expected_exception = true;
                }
                catch (...)
                {
                    is_expected_exception = false;
                }
                all_exceptions_expected =
                    all_exceptions_expected && is_expected_exception;
            }
            HPX_TEST(all_exceptions_expected);
            caught_expected_exception = all_exceptions_expected;
        }
        else
        {
            HPX_TEST(false);
        }
    }
    catch (Exception const&)
    {
        if constexpr (std::is_same_v<Exception, std::runtime_error>)
        {
            HPX_TEST(false);
        }
        else
        {
            caught_expected_exception = true;
        }
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_expected_exception);
}

template <typename IteratorTag>
void copy_if_sender_test()
{
    using namespace hpx::execution;

    test_copy_if_scheduler(hpx::launch::sync, seq, IteratorTag{});
    test_copy_if_scheduler(hpx::launch::sync, unseq, IteratorTag{});
    test_copy_if_scheduler(hpx::launch::async, par, IteratorTag{});
    test_copy_if_scheduler(hpx::launch::async, par_unseq, IteratorTag{});

    test_copy_if_sender(hpx::launch::sync, seq(task), IteratorTag{});
    test_copy_if_sender(hpx::launch::sync, unseq(task), IteratorTag{});
    test_copy_if_sender(hpx::launch::async, par(task), IteratorTag{});
    test_copy_if_sender(hpx::launch::async, par_unseq(task), IteratorTag{});

    test_copy_if_sender_exception<std::runtime_error>(
        hpx::launch::sync, seq(task), IteratorTag{});
    test_copy_if_sender_exception<std::runtime_error>(
        hpx::launch::async, par(task), IteratorTag{});
    test_copy_if_sender_exception<std::bad_alloc>(
        hpx::launch::sync, seq(task), IteratorTag{});
    test_copy_if_sender_exception<std::bad_alloc>(
        hpx::launch::async, par(task), IteratorTag{});
}

int hpx_main()
{
    test_copy_if_sender_lifecycle();
    test_scan_partitioner_exceptions();
    copy_if_sender_test<std::forward_iterator_tag>();
    copy_if_sender_test<std::random_access_iterator_tag>();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
