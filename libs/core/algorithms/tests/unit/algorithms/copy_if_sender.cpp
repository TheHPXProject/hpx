//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "test_utils.hpp"

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

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
            caught_expected_exception = true;
            test::test_num_exceptions<ExPolicy, IteratorTag>::call(
                ex_policy, errors);
        }
        else
        {
            HPX_TEST(false);
        }
    }
    catch (Exception const&)
    {
        caught_expected_exception = true;
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
