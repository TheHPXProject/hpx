//  Copyright (c) 2014-2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <ctime>
#include <iostream>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <hpx/iterator_support/tests/iter_sent.hpp>
#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
void test_minmax_element_sent()
{
    using hpx::get;

    auto c = test::random_repeat(100, std::size_t(50));
    c[50] = std::size_t(101);
    auto ref = std::minmax_element(std::begin(c), std::begin(c) + 50);
    auto r = hpx::ranges::minmax_element(
        std::begin(c), sentinel<size_t>{*(std::begin(c) + 50)});

    HPX_TEST((r.min == ref.first) && (r.max == ref.second));

    auto c1 = std::vector<size_t>{5, 7, 8};
    ref = std::minmax_element(
        std::begin(c1), std::begin(c1) + 2, std::greater<std::size_t>());
    r = hpx::ranges::minmax_element(
        std::begin(c1), sentinel<size_t>{8}, std::greater<std::size_t>());

    HPX_TEST((r.min == ref.first) && (r.max == ref.second));

    auto c2 = std::vector<size_t>{2, 2, 2};
    r = hpx::ranges::minmax_element(std::begin(c2), sentinel<size_t>{2});
    HPX_TEST((r.min == std::begin(c2)) && (r.max == std::begin(c2)));

    auto c3 = std::vector<size_t>{2, 3, 3, 4};
    r = hpx::ranges::minmax_element(std::begin(c3), sentinel<size_t>{3});
    HPX_TEST((*r.min == 2) && (*r.max == 2));
}

template <typename ExPolicy>
void test_minmax_element_sent(ExPolicy policy)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using hpx::get;

    auto c = test::random_repeat(100, std::size_t(50));
    c[50] = std::size_t(101);
    auto ref = std::minmax_element(std::begin(c), std::begin(c) + 50);
    auto r = hpx::ranges::minmax_element(policy, std::begin(c),
        test::sized_sentinel_from_iterator(std::begin(c) + 50));

    HPX_TEST((r.min == ref.first) && (r.max == ref.second));

    auto c1 = std::vector<size_t>{5, 7, 8};
    ref = std::minmax_element(
        std::begin(c1), std::begin(c1) + 2, std::greater<std::size_t>());
    r = hpx::ranges::minmax_element(policy, std::begin(c1),
        test::sized_sentinel_from_iterator(std::begin(c1) + 2),
        std::greater<std::size_t>());

    HPX_TEST((r.min == ref.first) && (r.max == ref.second));

    auto c2 = std::vector<size_t>{2, 2, 2};
    r = hpx::ranges::minmax_element(policy, std::begin(c2),
        test::sized_sentinel_from_iterator(std::begin(c2)));
    HPX_TEST((r.min == std::begin(c2)) && (r.max == std::begin(c2)));

    auto c3 = std::vector<size_t>{2, 3, 3, 4};
    r = hpx::ranges::minmax_element(policy, std::begin(c3),
        test::sized_sentinel_from_iterator(std::begin(c3) + 1));
    HPX_TEST((*r.min == 2) && (*r.max == 2));
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_minmax_element(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c = test::random_repeat(10007, std::size_t(100));

    base_iterator ref_end(std::end(c.base()));

    auto r = hpx::ranges::minmax_element(c, std::less<std::size_t>());
    HPX_TEST(r.min != std::end(c) && r.max != std::end(c));

    std::pair<base_iterator, base_iterator> ref = std::minmax_element(
        std::begin(c.base()), std::end(c.base()), std::less<std::size_t>());
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *r.min);
    HPX_TEST_EQ(*ref.second, *r.max);

    r = hpx::ranges::minmax_element(c);
    HPX_TEST(r.min != std::end(c) && r.max != std::end(c));

    ref = std::minmax_element(std::begin(c.base()), std::end(c.base()));
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *r.min);
    HPX_TEST_EQ(*ref.second, *r.max);
}

template <typename ExPolicy, typename IteratorTag>
void test_minmax_element(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;

    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c = test::random_repeat(10007, std::size_t(100));

    base_iterator ref_end(std::end(c.base()));

    auto r = hpx::ranges::minmax_element(policy, c, std::less<std::size_t>());
    HPX_TEST(r.min != std::end(c) && r.max != std::end(c));

    std::pair<base_iterator, base_iterator> ref = std::minmax_element(
        std::begin(c.base()), std::end(c.base()), std::less<std::size_t>());
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *r.min);
    HPX_TEST_EQ(*ref.second, *r.max);

    r = hpx::ranges::minmax_element(policy, c);
    HPX_TEST(r.min != std::end(c) && r.max != std::end(c));

    ref = std::minmax_element(std::begin(c.base()), std::end(c.base()));
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *r.min);
    HPX_TEST_EQ(*ref.second, *r.max);
}

template <typename ExPolicy, typename IteratorTag>
void test_minmax_element_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;

    typedef test::test_container<std::vector<std::size_t>, IteratorTag>
        test_vector;

    test_vector c = test::random_repeat(10007, std::size_t(100));

    base_iterator ref_end(std::end(c.base()));

    auto r = hpx::ranges::minmax_element(p, c, std::less<std::size_t>());
    auto rit = r.get();
    HPX_TEST(rit.min != std::end(c) && rit.max != std::end(c));

    std::pair<base_iterator, base_iterator> ref = std::minmax_element(
        std::begin(c.base()), std::end(c.base()), std::less<std::size_t>());
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *rit.min);
    HPX_TEST_EQ(*ref.second, *rit.max);

    r = hpx::ranges::minmax_element(p, c);
    rit = r.get();
    HPX_TEST(rit.min != std::end(c) && rit.max != std::end(c));

    ref = std::minmax_element(std::begin(c.base()), std::end(c.base()));
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *rit.min);
    HPX_TEST_EQ(*ref.second, *rit.max);
}

template <typename IteratorTag>
void test_minmax_element()
{
    using namespace hpx::execution;

    test_minmax_element(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_minmax_element(seq, IteratorTag());
        test_minmax_element(par, IteratorTag());
        test_minmax_element(par_unseq, IteratorTag());

        test_minmax_element_async(seq(task), IteratorTag());
        test_minmax_element_async(par(task), IteratorTag());
    }

    test_minmax_element_sent();
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_minmax_element_sent(seq);
        test_minmax_element_sent(par);
        test_minmax_element_sent(par_unseq);
    }
}

void minmax_element_test()
{
    test_minmax_element<std::random_access_iterator_tag>();
    test_minmax_element<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_minmax_element_exception(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    {
        bool caught_exception = false;
        try
        {
            hpx::ranges::minmax_element(
                hpx::util::iterator_range(
                    decorated_iterator(std::begin(c),
                        []() { throw std::runtime_error("test"); }),
                    decorated_iterator(std::end(c))),
                std::less<std::size_t>());

            HPX_TEST(false);
        }
        catch (hpx::exception_list const& e)
        {
            caught_exception = true;
            test::test_num_exceptions<hpx::execution::sequenced_policy,
                IteratorTag>::call(hpx::execution::seq, e);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }

    {
        bool caught_exception = false;
        try
        {
            hpx::ranges::minmax_element(hpx::util::iterator_range(
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c))));

            HPX_TEST(false);
        }
        catch (hpx::exception_list const& e)
        {
            caught_exception = true;
            test::test_num_exceptions<hpx::execution::sequenced_policy,
                IteratorTag>::call(hpx::execution::seq, e);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_minmax_element_exception(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    {
        bool caught_exception = false;
        try
        {
            hpx::ranges::minmax_element(policy,
                hpx::util::iterator_range(
                    decorated_iterator(std::begin(c),
                        []() { throw std::runtime_error("test"); }),
                    decorated_iterator(std::end(c))),
                std::less<std::size_t>());

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

    {
        bool caught_exception = false;
        try
        {
            hpx::ranges::minmax_element(policy,
                hpx::util::iterator_range(
                    decorated_iterator(std::begin(c),
                        []() { throw std::runtime_error("test"); }),
                    decorated_iterator(std::end(c))));

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
}

template <typename ExPolicy, typename IteratorTag>
void test_minmax_element_exception_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    {
        bool returned_from_algorithm = false;
        bool caught_exception = false;

        try
        {
            auto f = hpx::ranges::minmax_element(p,
                hpx::util::iterator_range(
                    decorated_iterator(std::begin(c),
                        []() { throw std::runtime_error("test"); }),
                    decorated_iterator(std::end(c))),
                std::less<std::size_t>());

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

    {
        bool caught_exception = false;
        bool returned_from_algorithm = false;

        try
        {
            auto f = hpx::ranges::minmax_element(p,
                hpx::util::iterator_range(
                    decorated_iterator(std::begin(c),
                        []() { throw std::runtime_error("test"); }),
                    decorated_iterator(std::end(c))));

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
}

template <typename IteratorTag>
void test_minmax_element_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_minmax_element_exception(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_minmax_element_exception(seq, IteratorTag());
        test_minmax_element_exception(par, IteratorTag());

        test_minmax_element_exception_async(seq(task), IteratorTag());
        test_minmax_element_exception_async(par(task), IteratorTag());
    }
}

void minmax_element_exception_test()
{
    test_minmax_element_exception<std::random_access_iterator_tag>();
    test_minmax_element_exception<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_minmax_element_bad_alloc(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    {
        bool caught_exception = false;
        try
        {
            hpx::ranges::minmax_element(
                hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                              []() { throw std::bad_alloc(); }),
                    decorated_iterator(std::end(c))),
                std::less<std::size_t>());

            HPX_TEST(false);
        }
        catch (std::bad_alloc const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }

    {
        bool caught_exception = false;
        try
        {
            hpx::ranges::minmax_element(
                hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                              []() { throw std::bad_alloc(); }),
                    decorated_iterator(std::end(c))));

            HPX_TEST(false);
        }
        catch (std::bad_alloc const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_minmax_element_bad_alloc(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    {
        bool caught_exception = false;
        try
        {
            hpx::ranges::minmax_element(policy,
                hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                              []() { throw std::bad_alloc(); }),
                    decorated_iterator(std::end(c))),
                std::less<std::size_t>());

            HPX_TEST(false);
        }
        catch (std::bad_alloc const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }

    {
        bool caught_exception = false;
        try
        {
            hpx::ranges::minmax_element(policy,
                hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                              []() { throw std::bad_alloc(); }),
                    decorated_iterator(std::end(c))));

            HPX_TEST(false);
        }
        catch (std::bad_alloc const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }
}

template <typename ExPolicy, typename IteratorTag>
void test_minmax_element_bad_alloc_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    {
        bool returned_from_algorithm = false;
        bool caught_exception = false;

        try
        {
            auto f = hpx::ranges::minmax_element(p,
                hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                              []() { throw std::bad_alloc(); }),
                    decorated_iterator(std::end(c))),
                std::less<std::size_t>());

            returned_from_algorithm = true;

            f.get();

            HPX_TEST(false);
        }
        catch (std::bad_alloc const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }

        HPX_TEST(caught_exception);
        HPX_TEST(returned_from_algorithm);
    }

    {
        bool caught_exception = false;
        bool returned_from_algorithm = false;

        try
        {
            auto f = hpx::ranges::minmax_element(p,
                hpx::util::iterator_range(decorated_iterator(std::begin(c),
                                              []() { throw std::bad_alloc(); }),
                    decorated_iterator(std::end(c))));

            returned_from_algorithm = true;

            f.get();

            HPX_TEST(false);
        }
        catch (std::bad_alloc const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }

        HPX_TEST(caught_exception);
        HPX_TEST(returned_from_algorithm);
    }
}

template <typename IteratorTag>
void test_minmax_element_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_minmax_element_bad_alloc(IteratorTag());
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_minmax_element_bad_alloc(seq, IteratorTag());
        test_minmax_element_bad_alloc(par, IteratorTag());

        test_minmax_element_bad_alloc_async(seq(task), IteratorTag());
        test_minmax_element_bad_alloc_async(par(task), IteratorTag());
    }
}

void minmax_element_bad_alloc_test()
{
    test_minmax_element_bad_alloc<std::random_access_iterator_tag>();
    test_minmax_element_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    minmax_element_test();
    minmax_element_exception_test();
    minmax_element_bad_alloc_test();

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
