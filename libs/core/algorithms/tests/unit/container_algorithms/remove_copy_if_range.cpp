//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iter_sent.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
// projected_element: two-field struct for testing projection support.
// 'key' is the value inspected by the projection; 'tag' distinguishes
// elements that share the same key, allowing post-hoc identity checks.
struct projected_element
{
    int key;
    int tag;

    bool operator==(projected_element const& o) const noexcept
    {
        return key == o.key && tag == o.tag;
    }
};

////////////////////////////////////////////////////////////////////////////
void test_remove_copy_if_sent()
{
    using hpx::get;

    std::size_t const size = 100;
    std::vector<std::int16_t> c(size), d(size, 1), e(size, 1);
    std::iota(std::begin(c), std::end(c), 1);

    auto pred1 = [](std::int16_t const& a) -> bool { return a % 42 != 0; };
    auto pred2 = [](std::int16_t const& a) -> bool { return a % 42 == 0; };

    hpx::ranges::remove_copy_if(c, std::begin(e), pred1);
    auto result_e = std::count_if(std::begin(e), std::end(e), pred2);

    hpx::ranges::remove_copy_if(
        std::begin(c), sentinel<std::int16_t>{50}, std::begin(d), pred1);
    auto result_d = std::count_if(std::begin(d), std::end(d), pred2);

    HPX_TEST(result_e == 2 && result_d == 1);
}

template <typename ExPolicy>
void test_remove_copy_if_sent(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    std::size_t const size = 100;
    std::vector<std::int16_t> c(size), d(size, 1), e(size, 1);
    std::iota(std::begin(c), std::end(c), 1);

    auto pred1 = [](std::int16_t const& a) -> bool { return a % 42 != 0; };
    auto pred2 = [](std::int16_t const& a) -> bool { return a % 42 == 0; };

    hpx::ranges::remove_copy_if(c, std::begin(e), pred1);
    auto result_e = std::count_if(std::begin(e), std::end(e), pred2);

    hpx::ranges::remove_copy_if(policy, std::begin(c),
        sentinel<std::int16_t>{50}, std::begin(d), pred1);
    auto result_d = std::count_if(std::begin(d), std::end(d), pred2);

    HPX_TEST(result_e == 2 && result_d == 1);
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_remove_copy_if(IteratorTag)
{
    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<int> d(c.size());
    std::size_t middle_idx = std::rand() % (c.size() / 2);
    auto middle = hpx::parallel::detail::next(std::begin(c.base()), middle_idx);
    std::iota(
        std::begin(c.base()), middle, static_cast<int>(std::rand() % c.size()));
    std::fill(middle, std::end(c.base()), -1);

    hpx::ranges::remove_copy_if(c, std::begin(d), [](int i) { return i < 0; });

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), middle, std::begin(d),
        [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c.base()), std::begin(d) + middle_idx,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<int> d(c.size());
    std::size_t middle_idx = std::rand() % (c.size() / 2);
    auto middle = hpx::parallel::detail::next(std::begin(c.base()), middle_idx);
    std::iota(
        std::begin(c.base()), middle, static_cast<int>(std::rand() % c.size()));
    std::fill(middle, std::end(c.base()), -1);

    hpx::ranges::remove_copy_if(
        policy, c, std::begin(d), [](int i) { return i < 0; });

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), middle, std::begin(d),
        [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c.base()), std::begin(d) + middle_idx,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_async(ExPolicy p, IteratorTag)
{
    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<int> d(c.size());
    std::size_t middle_idx = std::rand() % (c.size() / 2);
    auto middle = hpx::parallel::detail::next(std::begin(c.base()), middle_idx);
    std::iota(
        std::begin(c.base()), middle, static_cast<int>(std::rand() % c.size()));
    std::fill(middle, std::end(c.base()), -1);

    auto f = hpx::ranges::remove_copy_if(
        p, c, std::begin(d), [](int i) { return i < 0; });
    f.wait();

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), middle, std::begin(d),
        [&count](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));

    HPX_TEST(std::equal(middle, std::end(c.base()), std::begin(d) + middle_idx,
        [&count](int v1, int v2) -> bool {
            HPX_TEST_NEQ(v1, v2);
            ++count;
            return v1 != v2;
        }));

    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_outiter(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<int> d(0);
    std::size_t middle_idx = std::rand() % (c.size() / 2);
    auto middle = hpx::parallel::detail::next(std::begin(c.base()), middle_idx);
    std::iota(
        std::begin(c.base()), middle, static_cast<int>(std::rand() % c.size()));
    std::fill(middle, std::end(c.base()), -1);

    hpx::ranges::remove_copy_if(
        policy, c, std::back_inserter(d), [](int i) { return i < 0; });

    HPX_TEST(std::equal(std::begin(c.base()), middle, std::begin(d),
        [](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            return v1 == v2;
        }));

    HPX_TEST_EQ(middle_idx, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_outiter_async(ExPolicy p, IteratorTag)
{
    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<int> d(0);
    std::size_t middle_idx = std::rand() % (c.size() / 2);
    auto middle = hpx::parallel::detail::next(std::begin(c.base()), middle_idx);
    std::iota(
        std::begin(c.base()), middle, static_cast<int>(std::rand() % c.size()));
    std::fill(middle, std::end(c.base()), -1);

    auto f = hpx::ranges::remove_copy_if(
        p, c, std::back_inserter(d), [](int i) { return i < 0; });
    f.wait();

    HPX_TEST(std::equal(std::begin(c.base()), middle, std::begin(d),
        [](int v1, int v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            return v1 == v2;
        }));

    HPX_TEST_EQ(middle_idx, d.size());
}

template <typename IteratorTag>
void test_remove_copy_if()
{
    using namespace hpx::execution;

    test_remove_copy_if(IteratorTag());
    test_remove_copy_if(seq, IteratorTag());
    test_remove_copy_if(par, IteratorTag());
    test_remove_copy_if(par_unseq, IteratorTag());

    test_remove_copy_if_async(seq(task), IteratorTag());
    test_remove_copy_if_async(par(task), IteratorTag());

    test_remove_copy_if_sent();
    test_remove_copy_if_sent(seq);
    test_remove_copy_if_sent(par);
    test_remove_copy_if_sent(par_unseq);
}

void remove_copy_if_test()
{
    test_remove_copy_if<std::random_access_iterator_tag>();
    test_remove_copy_if<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
// Projection tests for hpx::ranges::remove_copy_if
// The projection extracts 'key'; predicate removes elements with odd key.
void test_remove_copy_if_projection()
{
    // Build input: 20 elements, keys cycling 0..4, tags unique
    std::size_t const size = 20;
    std::vector<projected_element> c(size);
    for (std::size_t i = 0; i != size; ++i)
        c[i] = {static_cast<int>(i % 5), static_cast<int>(i)};

    auto proj = [](projected_element const& e) -> int { return e.key; };
    // Remove elements where projected key is odd
    auto pred = [](int k) -> bool { return k % 2 != 0; };

    // Expected: only elements with even key are kept
    std::vector<projected_element> expected;
    for (auto const& e : c)
        if (e.key % 2 == 0)
            expected.push_back(e);

    // No-policy (sequential) - range form
    {
        std::vector<projected_element> dest(size);
        auto res = hpx::ranges::remove_copy_if(c, std::begin(dest), pred, proj);
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // seq policy
    {
        std::vector<projected_element> dest(size);
        auto res = hpx::ranges::remove_copy_if(
            hpx::execution::seq, c, std::begin(dest), pred, proj);
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // par policy
    {
        std::vector<projected_element> dest(size);
        auto res = hpx::ranges::remove_copy_if(
            hpx::execution::par, c, std::begin(dest), pred, proj);
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // par_unseq policy
    {
        std::vector<projected_element> dest(size);
        auto res = hpx::ranges::remove_copy_if(
            hpx::execution::par_unseq, c, std::begin(dest), pred, proj);
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // seq(task) - async
    {
        std::vector<projected_element> dest(size);
        auto f = hpx::ranges::remove_copy_if(
            hpx::execution::seq(hpx::execution::task), c, std::begin(dest),
            pred, proj);
        auto res = f.get();
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // par(task) - async
    {
        std::vector<projected_element> dest(size);
        auto f = hpx::ranges::remove_copy_if(
            hpx::execution::par(hpx::execution::task), c, std::begin(dest),
            pred, proj);
        auto res = f.get();
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    try
    {
        hpx::ranges::remove_copy_if(
            policy, c, std::begin(d), [](std::size_t v) {
                return throw std::runtime_error("test"), v == 0;
            });
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(policy, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_exception_async(ExPolicy p, IteratorTag)
{
    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f =
            hpx::ranges::remove_copy_if(p, c, std::begin(d), [](std::size_t v) {
                return throw std::runtime_error("test"), v == 0;
            });

        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(p, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST(returned_from_algorithm);
}

template <typename IteratorTag>
void test_remove_copy_if_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_remove_copy_if_exception(seq, IteratorTag());
    test_remove_copy_if_exception(par, IteratorTag());

    test_remove_copy_if_exception_async(seq(task), IteratorTag());
    test_remove_copy_if_exception_async(par(task), IteratorTag());
}

void remove_copy_if_exception_test()
{
    test_remove_copy_if_exception<std::random_access_iterator_tag>();
    test_remove_copy_if_exception<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::remove_copy_if(policy, c, std::begin(d),
            [](std::size_t v) { return throw std::bad_alloc(), v; });

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_if_bad_alloc_async(ExPolicy p, IteratorTag)
{
    typedef test::test_container<std::vector<int>, IteratorTag> test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::remove_copy_if(p, c, std::begin(d),
            [](std::size_t v) { return throw std::bad_alloc(), v; });

        returned_from_algorithm = true;
        f.get();

        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
    HPX_TEST(returned_from_algorithm);
}

template <typename IteratorTag>
void test_remove_copy_if_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_remove_copy_if_bad_alloc(seq, IteratorTag());
    test_remove_copy_if_bad_alloc(par, IteratorTag());

    test_remove_copy_if_bad_alloc_async(seq(task), IteratorTag());
    test_remove_copy_if_bad_alloc_async(par(task), IteratorTag());
}

void remove_copy_if_bad_alloc_test()
{
    test_remove_copy_if_bad_alloc<std::random_access_iterator_tag>();
    test_remove_copy_if_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    remove_copy_if_test();
    remove_copy_if_exception_test();
    remove_copy_if_bad_alloc_test();
    test_remove_copy_if_projection();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
