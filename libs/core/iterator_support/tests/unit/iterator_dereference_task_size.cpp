//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <iterator>
#include <tuple>
#include <utility>
#include <vector>

using hpx::threads::task_size;
using hpx::traits::iterator_dereference_task_size;
using hpx::traits::iterator_dereference_task_size_v;

///////////////////////////////////////////////////////////////////////////////
// An iterator that returns the elements it computes, by value.
template <typename Reference>
struct computing_iterator
  : hpx::util::iterator_facade<computing_iterator<Reference>,
        std::remove_cvref_t<Reference>, std::forward_iterator_tag, Reference>
{
    Reference dereference() const;
    void increment() {}
    bool equal(computing_iterator const&) const
    {
        return true;
    }
};

// The same, but it says its dereference is a small task.
struct small_computing_iterator
  : hpx::util::iterator_facade<small_computing_iterator, int,
        std::forward_iterator_tag, int>
{
    static constexpr task_size dereference_task_size = task_size::small_;

    int dereference() const
    {
        return 0;
    }
    void increment() {}
    bool equal(small_computing_iterator const&) const
    {
        return true;
    }
};

// An adaptor that does not replace the dereference of the iterator it wraps.
template <typename Base>
struct forwarding_adaptor
  : hpx::util::iterator_adaptor<forwarding_adaptor<Base>, Base>
{
    using base_adaptor = hpx::util::iterator_adaptor<forwarding_adaptor, Base>;
    using base_adaptor::base_adaptor;
};

// An adaptor that returns something else than the iterator it wraps.
struct replacing_adaptor
  : hpx::util::iterator_adaptor<replacing_adaptor, int*, int,
        std::forward_iterator_tag, int>
{
    int dereference() const
    {
        return *base() * 2;
    }
};

void test_default()
{
    // an iterator that returns a reference reads an existing element
    static_assert(iterator_dereference_task_size_v<int*> == task_size::nano);
    static_assert(
        iterator_dereference_task_size_v<int const*> == task_size::nano);
    static_assert(
        iterator_dereference_task_size_v<std::vector<int>::const_iterator> ==
        task_size::nano);
    static_assert(iterator_dereference_task_size_v<std::move_iterator<int*>> ==
        task_size::nano);

    // also if it returns a tuple of references
    static_assert(iterator_dereference_task_size_v<
                      computing_iterator<std::tuple<int&, int const&>>> ==
        task_size::nano);
    static_assert(
        iterator_dereference_task_size_v<
            computing_iterator<std::pair<int&, double&&>>> == task_size::nano);

    // an iterator that returns a value may do any amount of work
    static_assert(iterator_dereference_task_size_v<computing_iterator<int>> ==
        task_size::unknown);
    static_assert(
        iterator_dereference_task_size_v<
            computing_iterator<std::tuple<int&, int>>> == task_size::unknown);

    // the iterator type can give the size itself
    static_assert(iterator_dereference_task_size_v<small_computing_iterator> ==
        task_size::small_);

    // a qualified iterator type has the size of the unqualified one
    static_assert(
        iterator_dereference_task_size_v<small_computing_iterator const&> ==
        task_size::small_);

    // call gives the size for an iterator
    int i = 0;
    HPX_TEST(iterator_dereference_task_size<int*>::call(&i) == task_size::nano);
}

void test_adaptors()
{
    // an adaptor that forwards the dereference has the size of the iterator
    // it wraps
    static_assert(iterator_dereference_task_size_v<forwarding_adaptor<int*>> ==
        task_size::nano);
    static_assert(iterator_dereference_task_size_v<
                      forwarding_adaptor<hpx::util::counting_iterator<int>>> ==
        task_size::nano);
    static_assert(
        iterator_dereference_task_size_v<
            forwarding_adaptor<computing_iterator<int>>> == task_size::unknown);

    // an adaptor that replaces it does not
    static_assert(iterator_dereference_task_size_v<replacing_adaptor> ==
        task_size::unknown);
}

void test_hpx_iterators()
{
    // counting_iterator returns its counter
    static_assert(
        iterator_dereference_task_size_v<hpx::util::counting_iterator<int>> ==
        task_size::nano);
    static_assert(
        iterator_dereference_task_size_v<hpx::util::counting_iterator<int*>> ==
        task_size::nano);

    // transform_iterator calls its transformer
    auto const plain = [](int* p) { return *p; };
    auto const sized = hpx::threads::nano_task(plain);
    using plain_iterator = hpx::util::transform_iterator<int*, decltype(plain)>;
    using sized_iterator = hpx::util::transform_iterator<int*, decltype(sized)>;
    static_assert(
        iterator_dereference_task_size_v<plain_iterator> == task_size::unknown);
    static_assert(
        iterator_dereference_task_size_v<sized_iterator> == task_size::nano);

    int i = 0;
    HPX_TEST(iterator_dereference_task_size<sized_iterator>::call(
                 sized_iterator(&i, sized)) == task_size::nano);

    // zip_iterator dereferences all of its iterators
    static_assert(
        iterator_dereference_task_size_v<
            hpx::util::zip_iterator<int*, double*>> == task_size::nano);
    static_assert(
        iterator_dereference_task_size_v<
            hpx::util::zip_iterator<int*, hpx::util::counting_iterator<int>>> ==
        task_size::nano);
    static_assert(iterator_dereference_task_size_v<
                      hpx::util::zip_iterator<int*, plain_iterator>> ==
        task_size::unknown);
    static_assert(
        iterator_dereference_task_size_v<
            hpx::util::zip_iterator<int*, small_computing_iterator>> ==
        task_size::small_);
}

void test_combined_task_size()
{
    using hpx::threads::combined_task_size;

    static_assert(combined_task_size(task_size::nano, task_size::nano) ==
        task_size::nano);
    static_assert(combined_task_size(task_size::nano, task_size::medium) ==
        task_size::medium);
    static_assert(combined_task_size(task_size::large, task_size::small_) ==
        task_size::large);
    static_assert(combined_task_size(task_size::unknown, task_size::nano) ==
        task_size::unknown);
    static_assert(combined_task_size(task_size::small_, task_size::unknown) ==
        task_size::unknown);
}

int main()
{
    test_default();
    test_adaptors();
    test_hpx_iterators();
    test_combined_task_size();

    return hpx::util::report_errors();
}
