//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/mutex.hpp>

#include <atomic>
#include <cstddef>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

using hpx::execution::experimental::continues_on;
using hpx::execution::experimental::execute;
using hpx::execution::experimental::then;
using hpx::execution::experimental::thread_pool_scheduler;
using hpx::experimental::async_rw_mutex;
using hpx::this_thread::experimental::sync_wait;

unsigned int seed = std::random_device{}();

///////////////////////////////////////////////////////////////////////////////
// Custom type with a more restricted interface in the base class. Used for
// testing that the read-only type of async_rw_mutex can be customized.
class mytype_base
{
protected:
    std::size_t x = 0;

public:
    mytype_base() = default;
    mytype_base(mytype_base&&) = default;
    mytype_base& operator=(mytype_base&&) = default;
    mytype_base(mytype_base const&) = delete;
    mytype_base& operator=(mytype_base const&) = delete;

    std::size_t const& read() const
    {
        return x;
    }
};

class mytype : public mytype_base
{
public:
    mytype() = default;
    mytype(mytype&&) = default;
    mytype& operator=(mytype&&) = default;
    mytype(mytype const&) = delete;
    mytype& operator=(mytype const&) = delete;

    std::size_t& readwrite()
    {
        return x;
    }
};

// Struct with call operators used for checking that the correct types are sent
// from the async_rw_mutex senders.
struct checker
{
    bool expect_readonly;
    std::size_t expected_predecessor_value;
    std::atomic<std::size_t>& count;
    std::size_t count_min;
    std::size_t count_max = count_min;

    // Access types are differently tagged for read-only and read-write access.
    using void_read_access_type =
        typename async_rw_mutex<void>::read_access_type;
    using void_readwrite_access_type =
        typename async_rw_mutex<void>::readwrite_access_type;

    void operator()(void_read_access_type)
    {
        HPX_ASSERT(expect_readonly);
        HPX_TEST_RANGE(++count, count_min, count_max);
    }

    void operator()(void_readwrite_access_type)
    {
        HPX_ASSERT(!expect_readonly);
        HPX_TEST_RANGE(++count, count_min, count_max);
    }

    // Non-void access types must be convertible to (const) references of the
    // types on which the async_rw_mutex is templated.
    using size_t_read_access_type =
        typename async_rw_mutex<std::size_t>::read_access_type;
    using size_t_readwrite_access_type =
        typename async_rw_mutex<std::size_t>::readwrite_access_type;

    static_assert(
        std::is_convertible<size_t_read_access_type, std::size_t const&>::value,
        "The given access type must be convertible to a const reference of "
        "given template type");
    static_assert(
        std::is_convertible<size_t_readwrite_access_type, std::size_t&>::value,
        "The given access type must be convertible to a reference of given "
        "template type");

    void operator()(std::size_t const& x)
    {
        HPX_TEST(expect_readonly);
        HPX_TEST_EQ(x, expected_predecessor_value);
        HPX_TEST_RANGE(++count, count_min, count_max);
    }

    void operator()(std::size_t& x)
    {
        HPX_ASSERT(!expect_readonly);
        HPX_TEST_EQ(x, expected_predecessor_value);
        HPX_TEST_RANGE(++count, count_min, count_max);
        ++x;
    }

    // Non-void access types must be convertible to (const) references of the
    // types on which the async_rw_mutex is templated.
    using mytype_read_access_type =
        typename async_rw_mutex<mytype, mytype_base>::read_access_type;
    using mytype_readwrite_access_type =
        typename async_rw_mutex<mytype, mytype_base>::readwrite_access_type;

    static_assert(
        std::is_convertible<mytype_read_access_type, mytype_base const&>::value,
        "The given access type must be convertible to a const reference of "
        "given template type");
    static_assert(
        std::is_convertible<mytype_readwrite_access_type, mytype&>::value,
        "The given access type must be convertible to a reference of given "
        "template type");

    void operator()(mytype_base const& x)
    {
        HPX_TEST(expect_readonly);
        HPX_TEST_EQ(x.read(), expected_predecessor_value);
        HPX_TEST_RANGE(++count, count_min, count_max);
    }

    void operator()(mytype& x)
    {
        HPX_ASSERT(!expect_readonly);
        HPX_TEST_EQ(x.read(), expected_predecessor_value);
        HPX_TEST_RANGE(++count, count_min, count_max);
        ++(x.readwrite());
    }
};

template <typename Executor, typename Senders>
void submit_senders(Executor&& exec, Senders& senders)
{
    for (auto& sender : senders)
    {
        // Original code uses sync_wait inside an hpx scheduler. Sync_wait completely
        // blocks the thread with std synchronization primitives which causes it to hang
        hpx::execution::experimental::start_detached(
            hpx::execution::experimental::schedule(exec) |
            hpx::execution::experimental::let_value(
                [s = std::move(sender)]() mutable { return std::move(s); }));
    }
}

template <typename ReadWriteT, typename ReadT = ReadWriteT>
void test_single_read_access(async_rw_mutex<ReadWriteT, ReadT> rwm)
{
    std::atomic<bool> called{false};
    sync_wait(rwm.read() | then([&](auto) { called = true; }));
    HPX_TEST(called);
}

template <typename ReadWriteT, typename ReadT = ReadWriteT>
void test_single_readwrite_access(async_rw_mutex<ReadWriteT, ReadT> rwm)
{
    std::atomic<bool> called{false};
    sync_wait(rwm.readwrite() | then([&](auto) { called = true; }));
    HPX_TEST(called);
}

template <typename ReadWriteT, typename ReadT = ReadWriteT>
void test_moved(async_rw_mutex<ReadWriteT, ReadT> rwm)
{
    // The destructor of an empty async_rw_mutex should not attempt to keep any
    // values alive
    auto rwm2 = std::move(rwm);
    std::atomic<bool> called{false};
    sync_wait(rwm2.read() | then([&](auto) { called = true; }));
    HPX_TEST(called);
}

template <typename ReadWriteT, typename ReadT = ReadWriteT>
void test_multiple_accesses(
    async_rw_mutex<ReadWriteT, ReadT> rwm, std::size_t iterations)
{
    thread_pool_scheduler exec{};

    std::atomic<std::size_t> count{0};

    // Read-only and read-write access return senders of different types
    // clang-format off
    using r_sender_type = std::decay_t<decltype(
        rwm.read() | continues_on(exec) | then(checker{true, 0, count, 0}))>;
    using rw_sender_type = std::decay_t<decltype(
        rwm.readwrite() | continues_on(exec) | then(checker{false, 0, count, 0}))>;
    // clang-format on

    std::vector<r_sender_type> r_senders;
    std::vector<rw_sender_type> rw_senders;

    std::mt19937 r(seed);
    std::uniform_int_distribution<std::size_t> d_senders(1, 10);

    std::size_t expected_count = 0;
    std::size_t expected_predecessor_count = 0;

    auto sender_helper = [&](bool readonly) {
        std::size_t const num_senders = d_senders(r);
        std::size_t const min_expected_count = expected_count + 1;
        std::size_t const max_expected_count = expected_count + num_senders;
        expected_count += num_senders;
        for (std::size_t j = 0; j < num_senders; ++j)
        {
            if (readonly)
            {
                r_senders.push_back(rwm.read() | continues_on(exec) |
                    then(checker{readonly, expected_predecessor_count, count,
                        min_expected_count, max_expected_count}));
            }
            else
            {
                rw_senders.push_back(rwm.readwrite() | continues_on(exec) |
                    then(checker{readonly, expected_predecessor_count, count,
                        min_expected_count, max_expected_count}));
                // Only read-write access is allowed to change the value
                ++expected_predecessor_count;
            }
        }
    };

    for (std::size_t i = 0; i < iterations; ++i)
    {
        // Alternate between read-only and read-write access
        sender_helper(true);
        sender_helper(false);
    }

    // Asynchronously submit the senders
    submit_senders(exec, r_senders);
    submit_senders(exec, rw_senders);

    // The destructor does not block, so we block here manually
    sync_wait(rwm.readwrite());
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
    }

    test_single_read_access(async_rw_mutex<void>{});
    test_single_read_access(async_rw_mutex<std::size_t>{0});
    test_single_read_access(async_rw_mutex<mytype, mytype_base>{mytype{}});

    test_single_readwrite_access(async_rw_mutex<void>{});
    test_single_readwrite_access(async_rw_mutex<std::size_t>{0});
    test_single_readwrite_access(async_rw_mutex<mytype, mytype_base>{mytype{}});

    test_moved(async_rw_mutex<void>{});
    test_moved(async_rw_mutex<std::size_t>{0});
    test_moved(async_rw_mutex<mytype, mytype_base>{mytype{}});

    // Keep the randomized stress test meaningful without tripping the CI time
    // budget on slower clang jobs.
    std::size_t iterations = 20;
    test_multiple_accesses(async_rw_mutex<void>{}, iterations);
    test_multiple_accesses(async_rw_mutex<std::size_t>{0}, iterations);
    test_multiple_accesses(
        async_rw_mutex<mytype, mytype_base>{mytype{}}, iterations);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init_params i;
    hpx::program_options::options_description desc_cmdline(
        "usage: " HPX_APPLICATION_STRING " [options]");
    desc_cmdline.add_options()("seed,s",
        hpx::program_options::value<unsigned int>(),
        "the random number generator seed to use for this run");
    i.desc_cmdline = desc_cmdline;

    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, i), 0);
    return hpx::util::report_errors();
}
