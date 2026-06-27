//  Copyright (c) 2014 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_max_element(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    iterator end(std::end(c));
    base_iterator ref_end(std::end(c));

    iterator r = hpx::max_element(
        iterator(std::begin(c)), iterator(end), std::less<std::size_t>());
    HPX_TEST(r != end);

    base_iterator ref =
        std::max_element(std::begin(c), std::end(c), std::less<std::size_t>());
    HPX_TEST(ref != ref_end);
    HPX_TEST_EQ(*ref, *r);
    HPX_TEST_EQ(std::distance(std::begin(c), ref),
        std::distance(iterator(std::begin(c)), r));

    r = hpx::max_element(iterator(std::begin(c)), iterator(std::end(c)));
    HPX_TEST(r != end);

    ref = std::max_element(std::begin(c), std::end(c));
    HPX_TEST(ref != ref_end);
    HPX_TEST_EQ(*ref, *r);
    HPX_TEST_EQ(std::distance(std::begin(c), ref),
        std::distance(iterator(std::begin(c)), r));
}

template <typename ExPolicy, typename IteratorTag>
void test_max_element(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    iterator end(std::end(c));
    base_iterator ref_end(std::end(c));

    iterator r = hpx::max_element(policy, iterator(std::begin(c)),
        iterator(end), std::less<std::size_t>());
    HPX_TEST(r != end);

    base_iterator ref =
        std::max_element(std::begin(c), std::end(c), std::less<std::size_t>());
    HPX_TEST(ref != ref_end);
    HPX_TEST_EQ(*ref, *r);
    HPX_TEST_EQ(std::distance(std::begin(c), ref),
        std::distance(iterator(std::begin(c)), r));

    r = hpx::max_element(
        policy, iterator(std::begin(c)), iterator(std::end(c)));
    HPX_TEST(r != end);

    ref = std::max_element(std::begin(c), std::end(c));
    HPX_TEST(ref != ref_end);
    HPX_TEST_EQ(*ref, *r);
    HPX_TEST_EQ(std::distance(std::begin(c), ref),
        std::distance(iterator(std::begin(c)), r));
}

template <typename ExPolicy, typename IteratorTag>
void test_max_element_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    iterator end(std::end(c));
    base_iterator ref_end(std::end(c));

    hpx::future<iterator> r = hpx::max_element(
        p, iterator(std::begin(c)), iterator(end), std::less<std::size_t>());
    iterator rit = r.get();
    HPX_TEST(rit != end);

    base_iterator ref =
        std::max_element(std::begin(c), std::end(c), std::less<std::size_t>());
    HPX_TEST(ref != ref_end);
    HPX_TEST_EQ(*ref, *rit);
    HPX_TEST_EQ(std::distance(std::begin(c), ref),
        std::distance(iterator(std::begin(c)), rit));

    r = hpx::max_element(p, iterator(std::begin(c)), iterator(std::end(c)));
    rit = r.get();
    HPX_TEST(rit != end);

    ref = std::max_element(std::begin(c), std::end(c));
    HPX_TEST(ref != ref_end);
    HPX_TEST_EQ(*ref, *rit);
    HPX_TEST_EQ(std::distance(std::begin(c), ref),
        std::distance(iterator(std::begin(c)), rit));
}

template <typename IteratorTag>
void test_max_element()
{
    using namespace hpx::execution;

    test_max_element(IteratorTag());
    test_max_element(seq, IteratorTag());
    test_max_element(par, IteratorTag());
    test_max_element(par_unseq, IteratorTag());

    test_max_element_async(seq(task), IteratorTag());
    test_max_element_async(par(task), IteratorTag());
}

void max_element_test()
{
    test_max_element<std::random_access_iterator_tag>();
    test_max_element<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_max_element_exception(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    {
        bool caught_exception = false;
        try
        {
            hpx::max_element(decorated_iterator(std::begin(c),
                                 []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)), std::less<std::size_t>());

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
            hpx::max_element(decorated_iterator(std::begin(c),
                                 []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)));

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
void test_max_element_exception(ExPolicy policy, IteratorTag)
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
            hpx::max_element(policy,
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)), std::less<std::size_t>());

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
            hpx::max_element(policy,
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)));

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
void test_max_element_exception_async(ExPolicy p, IteratorTag)
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
            hpx::future<decorated_iterator> f = hpx::max_element(p,
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)), std::less<std::size_t>());

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
            hpx::future<decorated_iterator> f = hpx::max_element(p,
                decorated_iterator(
                    std::begin(c), []() { throw std::runtime_error("test"); }),
                decorated_iterator(std::end(c)));

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
void test_max_element_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_max_element_exception(IteratorTag());
    test_max_element_exception(seq, IteratorTag());
    test_max_element_exception(par, IteratorTag());

    test_max_element_exception_async(seq(task), IteratorTag());
    test_max_element_exception_async(par(task), IteratorTag());
}

void max_element_exception_test()
{
    test_max_element_exception<std::random_access_iterator_tag>();
    test_max_element_exception<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_max_element_bad_alloc(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::decorated_iterator<base_iterator, IteratorTag>
        decorated_iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    {
        bool caught_exception = false;
        try
        {
            hpx::max_element(decorated_iterator(std::begin(c),
                                 []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)), std::less<std::size_t>());

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
            hpx::max_element(decorated_iterator(std::begin(c),
                                 []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)));

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
void test_max_element_bad_alloc(ExPolicy policy, IteratorTag)
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
            hpx::max_element(policy,
                decorated_iterator(
                    std::begin(c), []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)), std::less<std::size_t>());

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
            hpx::max_element(policy,
                decorated_iterator(
                    std::begin(c), []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)));

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
void test_max_element_bad_alloc_async(ExPolicy p, IteratorTag)
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
            hpx::future<decorated_iterator> f = hpx::max_element(p,
                decorated_iterator(
                    std::begin(c), []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)), std::less<std::size_t>());

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
            hpx::future<decorated_iterator> f = hpx::max_element(p,
                decorated_iterator(
                    std::begin(c), []() { throw std::bad_alloc(); }),
                decorated_iterator(std::end(c)));

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
void test_max_element_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_max_element_bad_alloc(IteratorTag());
    test_max_element_bad_alloc(seq, IteratorTag());
    test_max_element_bad_alloc(par, IteratorTag());

    test_max_element_bad_alloc_async(seq(task), IteratorTag());
    test_max_element_bad_alloc_async(par(task), IteratorTag());
}

void max_element_bad_alloc_test()
{
    test_max_element_bad_alloc<std::random_access_iterator_tag>();
    test_max_element_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    max_element_test();
    max_element_exception_test();
    max_element_bad_alloc_test();

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
