//  Copyright (c) 2014-2016 Hartmut Kaiser
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
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_minmax_element(IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    iterator end(std::end(c));
    base_iterator ref_end(std::end(c));

    auto r = hpx::minmax_element(
        iterator(std::begin(c)), iterator(end), std::less<std::size_t>());
    HPX_TEST(r.min != end && r.max != end);

    auto ref = std::minmax_element(
        std::begin(c), std::end(c), std::less<std::size_t>());
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *r.min);
    HPX_TEST_EQ(*ref.second, *r.max);

    r = hpx::minmax_element(iterator(std::begin(c)), iterator(std::end(c)));
    HPX_TEST(r.min != end && r.max != end);

    ref = std::minmax_element(std::begin(c), std::end(c));
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *r.min);
    HPX_TEST_EQ(*ref.second, *r.max);

    struct set
    {
        std::size_t identifier;
        std::size_t value;
    };

    typedef typename std::vector<set>::iterator base_set_iterator;
    typedef test::test_iterator<base_set_iterator, IteratorTag> set_iterator;

    std::vector<set> d = { {0, 100}, {1, 100}, {2, 99}, {3, 99}, {4, 100} };

    set_iterator end2(std::end(d));
    base_set_iterator ref_end2(std::end(d));

    auto r2 = hpx::minmax_element(
        set_iterator(std::begin(d)), set_iterator(end2),
        [](set first, set second) {return first.value < second.value;});
    HPX_TEST(r2.min != end2 && r2.max != end2);

    auto ref2 = std::minmax_element(std::begin(d), std::end(d),
        [](set first, set second) {return first.value < second.value;});
    HPX_TEST(ref2.first != ref_end2 && ref2.second != ref_end2);

    HPX_TEST_EQ(ref2.first->identifier, r2.min->identifier);
    HPX_TEST_EQ(ref2.second->identifier, r2.max->identifier);

    d = { {0, 99}, {1, 99}, {2, 100}, {3, 100}, {4, 99} };

    end2 = set_iterator(std::end(d));
    ref_end2 = base_set_iterator(std::end(d));

    r2 = hpx::minmax_element(
        set_iterator(std::begin(d)), set_iterator(std::end(d)),
        [](set first, set second) {return first.value < second.value;});
    HPX_TEST(r2.min != end2 && r2.max != end2);

    ref2 = std::minmax_element(std::begin(d), std::end(d),
        [](set first, set second) {return first.value < second.value;});
    HPX_TEST(ref2.first != ref_end2 && ref2.second != ref_end2);

    HPX_TEST_EQ(ref2.first->identifier, r2.min->identifier);
    HPX_TEST_EQ(ref2.second->identifier, r2.max->identifier);

}

template <typename ExPolicy, typename IteratorTag>
void test_minmax_element(ExPolicy policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    iterator end(std::end(c));
    base_iterator ref_end(std::end(c));

    auto r = hpx::minmax_element(policy, iterator(std::begin(c)), iterator(end),
        std::less<std::size_t>());
    HPX_TEST(r.min != end && r.max != end);

    auto ref = std::minmax_element(
        std::begin(c), std::end(c), std::less<std::size_t>());
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *r.min);
    HPX_TEST_EQ(*ref.second, *r.max);

    r = hpx::minmax_element(
        policy, iterator(std::begin(c)), iterator(std::end(c)));
    HPX_TEST(r.min != end && r.max != end);

    ref = std::minmax_element(std::begin(c), std::end(c));
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *r.min);
    HPX_TEST_EQ(*ref.second, *r.max);

    struct set
    {
        std::size_t identifier;
        std::size_t value;
    };

    typedef typename std::vector<set>::iterator base_set_iterator;
    typedef test::test_iterator<base_set_iterator, IteratorTag> set_iterator;

    std::vector<set> d = { {0, 100}, {1, 100}, {2, 99}, {3, 99}, {4, 100} };

    set_iterator end2(std::end(d));
    base_set_iterator ref_end2(std::end(d));

    auto r2 = hpx::minmax_element(
        policy, set_iterator(std::begin(d)), set_iterator(end2),
        [](set first, set second) {return first.value < second.value;});
    HPX_TEST(r2.min != end2 && r2.max != end2);

    auto ref2 = std::minmax_element(std::begin(d), std::end(d),
        [](set first, set second) {return first.value < second.value;});
    HPX_TEST(ref2.first != ref_end2 && ref2.second != ref_end2);

    HPX_TEST_EQ(ref2.first->identifier, r2.min->identifier);
    HPX_TEST_EQ(ref2.second->identifier, r2.max->identifier);

    d = { {0, 99}, {1, 99}, {2, 100}, {3, 100}, {4, 99} };

    end2 = set_iterator(std::end(d));
    ref_end2 = base_set_iterator(std::end(d));

    r2 = hpx::minmax_element(
        policy, set_iterator(std::begin(d)), set_iterator(std::end(d)),
        [](set first, set second) {return first.value < second.value;});
    HPX_TEST(r2.min != end2 && r2.max != end2);

    ref2 = std::minmax_element(std::begin(d), std::end(d),
        [](set first, set second) {return first.value < second.value;});
    HPX_TEST(ref2.first != ref_end2 && ref2.second != ref_end2);

    HPX_TEST_EQ(ref2.first->identifier, r2.min->identifier);
    HPX_TEST_EQ(ref2.second->identifier, r2.max->identifier);
}

template <typename ExPolicy, typename IteratorTag>
void test_minmax_element_async(ExPolicy p, IteratorTag)
{
    typedef std::vector<std::size_t>::iterator base_iterator;
    typedef test::test_iterator<base_iterator, IteratorTag> iterator;

    std::vector<std::size_t> c = test::random_repeat(10007, std::size_t(100));

    iterator end(std::end(c));
    base_iterator ref_end(std::end(c));

    auto r = hpx::minmax_element(
        p, iterator(std::begin(c)), iterator(end), std::less<std::size_t>());
    auto rit = r.get();
    HPX_TEST(rit.min != end && rit.max != end);

    auto ref = std::minmax_element(
        std::begin(c), std::end(c), std::less<std::size_t>());
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *rit.min);
    HPX_TEST_EQ(*ref.second, *rit.max);

    r = hpx::minmax_element(p, iterator(std::begin(c)), iterator(std::end(c)));
    rit = r.get();
    HPX_TEST(rit.min != end && rit.max != end);

    ref = std::minmax_element(std::begin(c), std::end(c));
    HPX_TEST(ref.first != ref_end && ref.second != ref_end);

    HPX_TEST_EQ(*ref.first, *rit.min);
    HPX_TEST_EQ(*ref.second, *rit.max);
}

template <typename IteratorTag>
void test_minmax_element()
{
    using namespace hpx::execution;

    test_minmax_element(IteratorTag());
    test_minmax_element(seq, IteratorTag());
    test_minmax_element(par, IteratorTag());
    test_minmax_element(par_unseq, IteratorTag());

    test_minmax_element_async(seq(task), IteratorTag());
    test_minmax_element_async(par(task), IteratorTag());
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
            hpx::minmax_element(decorated_iterator(std::begin(c),
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
            hpx::minmax_element(decorated_iterator(std::begin(c),
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
            hpx::minmax_element(policy,
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
            hpx::minmax_element(policy,
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
            auto f = hpx::minmax_element(p,
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
            auto f = hpx::minmax_element(p,
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
void test_minmax_element_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_minmax_element_exception(IteratorTag());
    test_minmax_element_exception(seq, IteratorTag());
    test_minmax_element_exception(par, IteratorTag());

    test_minmax_element_exception_async(seq(task), IteratorTag());
    test_minmax_element_exception_async(par(task), IteratorTag());
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
            hpx::minmax_element(decorated_iterator(std::begin(c),
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
            hpx::minmax_element(decorated_iterator(std::begin(c),
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
            hpx::minmax_element(policy,
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
            hpx::minmax_element(policy,
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
            auto f = hpx::minmax_element(p,
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
            auto f = hpx::minmax_element(p,
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
void test_minmax_element_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_minmax_element_bad_alloc(IteratorTag());
    test_minmax_element_bad_alloc(seq, IteratorTag());
    test_minmax_element_bad_alloc(par, IteratorTag());

    test_minmax_element_bad_alloc_async(seq(task), IteratorTag());
    test_minmax_element_bad_alloc_async(par(task), IteratorTag());
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
