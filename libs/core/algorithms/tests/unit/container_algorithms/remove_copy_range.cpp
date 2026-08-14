//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iter_sent.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
void test_remove_copy_sent()
{
    using hpx::get;

    std::size_t const size = 100;
    std::vector<std::int16_t> c(size), d(size, -1);
    std::iota(std::begin(c), std::end(c), 1);
    c[99] = 42;    //both c[99] and c[42] are equal to 42

    int value = 42;

    auto pred = [](int const& n) -> bool { return n > 0; };

    hpx::ranges::remove_copy(
        std::begin(c), sentinel<std::int16_t>{50}, std::begin(d), value);
    auto result = std::count_if(std::begin(d), std::end(d), pred);

    HPX_TEST(result == 48);
}

template <typename ExPolicy>
void test_remove_copy_sent(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    std::size_t const size = 100;
    std::vector<std::int16_t> c(size), d(size, -1);
    std::iota(std::begin(c), std::end(c), 1);

    c[99] = 42;    //both c[99] and c[42] are equal to 42

    int value = 42;

    auto pred = [](int const& n) -> bool { return n > 0; };

    hpx::ranges::remove_copy(policy, std::begin(c), sentinel<std::int16_t>{50},
        std::begin(d), value);
    auto result = std::count_if(std::begin(d), std::end(d), pred);

    HPX_TEST(result == 48);
}

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_remove_copy(IteratorTag)
{
    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size() / 2);
    std::size_t middle_idx = std::rand() % (c.size() / 2);
    auto middle = hpx::parallel::detail::next(std::begin(c.base()), middle_idx);
    std::fill(std::begin(c.base()), middle, 1);
    std::fill(middle, std::end(c.base()), 2);

    hpx::ranges::remove_copy(c, std::begin(d), std::size_t(2));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), middle, std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, middle_idx);
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size() / 2);
    std::size_t middle_idx = std::rand() % (c.size() / 2);
    auto middle = hpx::parallel::detail::next(std::begin(c.base()), middle_idx);
    std::fill(std::begin(c.base()), middle, 1);
    std::fill(middle, std::end(c.base()), 2);

    hpx::ranges::remove_copy(policy, c, std::begin(d), std::size_t(2));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), middle, std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, middle_idx);
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_async(ExPolicy p, IteratorTag)
{
    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(c.size() / 2);
    std::size_t middle_idx = std::rand() % (c.size() / 2);
    auto middle = hpx::parallel::detail::next(std::begin(c.base()), middle_idx);
    std::fill(std::begin(c.base()), middle, 1);
    std::fill(middle, std::end(c.base()), 2);

    auto f = hpx::ranges::remove_copy(p, c, std::begin(d), std::size_t(2));
    f.wait();

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), middle, std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, middle_idx);
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_outiter(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(0);
    std::iota(std::begin(c), std::end(c), 0);

    hpx::ranges::remove_copy(
        policy, c, std::back_inserter(d), std::size_t(3000));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::begin(c.base()) + 3000,
        std::begin(d), [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST(std::equal(std::begin(c.base()) + 3001, std::end(c.base()),
        std::begin(d) + 3000, [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_outiter_async(ExPolicy p, IteratorTag)
{
    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c(10007);
    std::vector<std::size_t> d(0);
    std::iota(std::begin(c), std::end(c), 0);

    auto f = hpx::ranges::remove_copy(
        p, c, std::back_inserter(d), std::size_t(3000));
    f.wait();

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c.base()), std::begin(c.base()) + 3000,
        std::begin(d), [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST(std::equal(std::begin(c.base()) + 3001, std::end(c.base()),
        std::begin(d) + 3000, [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename IteratorTag>
void test_remove_copy()
{
    using namespace hpx::execution;
    test_remove_copy(IteratorTag());
    test_remove_copy(seq, IteratorTag());
    test_remove_copy(par, IteratorTag());
    test_remove_copy(par_unseq, IteratorTag());

    test_remove_copy_async(seq(task), IteratorTag());
    test_remove_copy_async(par(task), IteratorTag());

    test_remove_copy_sent();
    test_remove_copy_sent(seq);
    test_remove_copy_sent(par);
    test_remove_copy_sent(par_unseq);
}

void remove_copy_test()
{
    test_remove_copy<std::random_access_iterator_tag>();
    test_remove_copy<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////////
// Projection tests for hpx::ranges::remove_copy
// The projection extracts 'key'; removal is done when INVOKE(proj,*it)==value.
void test_remove_copy_projection()
{
    // Build input: 20 elements, keys cycling 0..4, tags unique
    std::size_t const size = 20;
    std::vector<projected_element> c(size);
    for (std::size_t i = 0; i != size; ++i)
        c[i] = {static_cast<int>(i % 5), static_cast<int>(i)};

    // Remove all elements where key == 2 via projection
    int const remove_key = 2;
    auto proj = [](projected_element const& e) -> int { return e.key; };

    // Expected: elements with key != 2  (4 out of every 5 are kept)
    std::vector<projected_element> expected;
    for (auto const& e : c)
        if (e.key != remove_key)
            expected.push_back(e);

    // No-policy (sequential) - range form
    {
        std::vector<projected_element> dest(size);
        auto res =
            hpx::ranges::remove_copy(c, std::begin(dest), remove_key, proj);
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // seq policy
    {
        std::vector<projected_element> dest(size);
        auto res = hpx::ranges::remove_copy(
            hpx::execution::seq, c, std::begin(dest), remove_key, proj);
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // par policy
    {
        std::vector<projected_element> dest(size);
        auto res = hpx::ranges::remove_copy(
            hpx::execution::par, c, std::begin(dest), remove_key, proj);
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // par_unseq policy
    {
        std::vector<projected_element> dest(size);
        auto res = hpx::ranges::remove_copy(
            hpx::execution::par_unseq, c, std::begin(dest), remove_key, proj);
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // seq(task) - async
    {
        std::vector<projected_element> dest(size);
        auto f =
            hpx::ranges::remove_copy(hpx::execution::seq(hpx::execution::task),
                c, std::begin(dest), remove_key, proj);
        auto res = f.get();
        HPX_TEST(res.in == std::end(c));
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }

    // par(task) - async
    {
        std::vector<projected_element> dest(size);
        auto f =
            hpx::ranges::remove_copy(hpx::execution::par(hpx::execution::task),
                c, std::begin(dest), remove_key, proj);
        auto res = f.get();
        HPX_TEST(test::equal(std::begin(dest), res.out, std::begin(expected),
            std::end(expected)));
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), 0);

    bool caught_exception = false;
    try
    {
        hpx::ranges::remove_copy(policy,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            std::begin(d), std::size_t(3000));
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
void test_remove_copy_exception_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), 0);

    bool caught_exception = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::remove_copy(p,
            hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))),
            std::begin(d), std::size_t(3000));
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
void test_remove_copy_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_remove_copy_exception(seq, IteratorTag());
    test_remove_copy_exception(par, IteratorTag());

    test_remove_copy_exception_async(seq(task), IteratorTag());
    test_remove_copy_exception_async(par(task), IteratorTag());
}

void remove_copy_exception_test()
{
    test_remove_copy_exception<std::random_access_iterator_tag>();
    test_remove_copy_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename IteratorTag>
void test_remove_copy_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), 0);

    bool caught_bad_alloc = false;
    try
    {
        hpx::ranges::remove_copy(policy,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            std::begin(d), std::size_t(3000));
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
void test_remove_copy_bad_alloc_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), 0);

    bool caught_bad_alloc = false;
    bool returned_from_algorithm = false;
    try
    {
        auto f = hpx::ranges::remove_copy(p,
            hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                          []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c))),
            std::begin(d), std::size_t(3000));
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
void test_remove_copy_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_remove_copy_bad_alloc(seq, IteratorTag());
    test_remove_copy_bad_alloc(par, IteratorTag());

    test_remove_copy_bad_alloc_async(seq(task), IteratorTag());
    test_remove_copy_bad_alloc_async(par(task), IteratorTag());
}

void remove_copy_bad_alloc_test()
{
    test_remove_copy_bad_alloc<std::random_access_iterator_tag>();
    test_remove_copy_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    remove_copy_test();
    remove_copy_exception_test();
    remove_copy_bad_alloc_test();
    test_remove_copy_projection();
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
