// (C) Copyright Dave Abrahams and Thomas Becker 2003.
//
//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <list>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Tests for https://svn.boost.org/trac/boost/ticket/1517
int to_value(std::list<int>::const_iterator v)
{
    return *v;
}

void category_test()
{
    std::list<int> rng1;
    std::string rng2;

    auto zipiter = hpx::util::zip_iterator(hpx::make_tuple(
        // BidirectionalInput
        hpx::util::transform_iterator(rng1.begin(), &to_value),
        // RandomAccess
        rng2.begin()));
    (void) zipiter;
}
//

/// Deduces to the result of tuple_cat when it's invoked with the given
/// parameters Ts.
template <typename... Ts>
using tuple_cat_result_of_t = decltype(hpx::tuple_cat(std::declval<Ts>()...));

int main(void)
{
    category_test();

    //     size_t num_successful_tests = 0;
    //     size_t num_failed_tests = 0;

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator construction and dereferencing
    //
    /////////////////////////////////////////////////////////////////////////////

    std::vector<double> vect1(3);
    vect1[0] = 42.;
    vect1[1] = 43.;
    vect1[2] = 44.;

    std::set<int> intset;
    intset.insert(52);
    intset.insert(53);
    intset.insert(54);
    //

    typedef hpx::util::zip_iterator<std::set<int>::iterator,
        std::vector<double>::iterator>
        zit_mixed;

    zit_mixed zip_it_mixed =
        zit_mixed(hpx::make_tuple(intset.begin(), vect1.begin()));

    hpx::tuple<int, double> val_tuple(*zip_it_mixed);

    hpx::tuple<int const&, double&> ref_tuple(*zip_it_mixed);

    double dblOldVal = hpx::get<1>(ref_tuple);
    hpx::get<1>(ref_tuple) -= 41.;

    HPX_TEST(52 == hpx::get<0>(val_tuple) && 42. == hpx::get<1>(val_tuple) &&
        52 == hpx::get<0>(ref_tuple) && 1. == hpx::get<1>(ref_tuple) &&
        1. == *vect1.begin());

    // Undo change to vect1
    hpx::get<1>(ref_tuple) = dblOldVal;

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator with 12 components
    //
    /////////////////////////////////////////////////////////////////////////////

    // Declare 12 containers
    //
    std::list<int> li1;
    li1.push_back(1);
    std::set<int> se1;
    se1.insert(2);
    std::vector<int> ve1;
    ve1.push_back(3);
    //
    std::list<int> li2;
    li2.push_back(4);
    std::set<int> se2;
    se2.insert(5);
    std::vector<int> ve2;
    ve2.push_back(6);
    //
    std::list<int> li3;
    li3.push_back(7);
    std::set<int> se3;
    se3.insert(8);
    std::vector<int> ve3;
    ve3.push_back(9);
    //
    std::list<int> li4;
    li4.push_back(10);
    std::set<int> se4;
    se4.insert(11);
    std::vector<int> ve4;
    ve4.push_back(12);

    // typedefs for cons lists of iterators.
    typedef tuple_cat_result_of_t<hpx::tuple<std::set<int>::iterator>,
        hpx::tuple<std::vector<int>::iterator, std::list<int>::iterator,
            std::set<int>::iterator, std::vector<int>::iterator,
            std::list<int>::iterator, std::set<int>::iterator,
            std::vector<int>::iterator, std::list<int>::iterator,
            std::set<int>::iterator, std::vector<int>::const_iterator>>
        cons_11_its_type;
    //
    typedef tuple_cat_result_of_t<hpx::tuple<std::list<int>::const_iterator>,
        cons_11_its_type>
        cons_12_its_type;

    // typedefs for cons lists for dereferencing the zip iterator
    // made from the cons list above.
    typedef tuple_cat_result_of_t<hpx::tuple<int const&>,
        hpx::tuple<int&, int&, int const&, int&, int&, int const&, int&, int&,
            int const&, int const&>>
        cons_11_refs_type;
    //
    typedef tuple_cat_result_of_t<hpx::tuple<int const&>, cons_11_refs_type>
        cons_12_refs_type;

    // typedef for zip iterator with 12 elements
    typedef hpx::util::zip_iterator<cons_12_its_type> zip_it_12_type;

    // Declare a 12-element zip iterator.
    zip_it_12_type zip_it_12(li1.begin(), se1.begin(), ve1.begin(), li2.begin(),
        se2.begin(), ve2.begin(), li3.begin(), se3.begin(), ve3.begin(),
        li4.begin(), se4.begin(), ve4.begin());

    // Dereference, mess with the result a little.
    cons_12_refs_type zip_it_12_dereferenced(*zip_it_12);
    hpx::get<9>(zip_it_12_dereferenced) = 42;

    // Make a copy and move it a little to force some instantiations.
    zip_it_12_type zip_it_12_copy(zip_it_12);
    ++zip_it_12_copy;

    HPX_TEST(hpx::get<11>(zip_it_12.get_iterator_tuple()) == ve4.begin() &&
        hpx::get<11>(zip_it_12_copy.get_iterator_tuple()) == ve4.end() &&
        1 == hpx::get<0>(zip_it_12_dereferenced) &&
        12 == hpx::get<11>(zip_it_12_dereferenced) && 42 == *(li4.begin()));

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator incrementing and dereferencing
    //
    /////////////////////////////////////////////////////////////////////////////

    std::vector<double> vect2(3);
    vect2[0] = 2.2;
    vect2[1] = 3.3;
    vect2[2] = 4.4;

    hpx::util::zip_iterator<hpx::tuple<std::vector<double>::const_iterator,
        std::vector<double>::const_iterator>>
        zip_it_begin(hpx::make_tuple(vect1.begin(), vect2.begin()));

    hpx::util::zip_iterator<hpx::tuple<std::vector<double>::const_iterator,
        std::vector<double>::const_iterator>>
        zip_it_run(hpx::make_tuple(vect1.begin(), vect2.begin()));

    hpx::util::zip_iterator<hpx::tuple<std::vector<double>::const_iterator,
        std::vector<double>::const_iterator>>
        zip_it_end(hpx::make_tuple(vect1.end(), vect2.end()));

    // NOLINTBEGIN(bugprone-inc-dec-in-conditions)
    HPX_TEST(zip_it_run == zip_it_begin && 42. == hpx::get<0>(*zip_it_run) &&
        2.2 == hpx::get<1>(*zip_it_run) &&
        43. == hpx::get<0>(*(++zip_it_run)) &&
        3.3 == hpx::get<1>(*zip_it_run) &&
        44. == hpx::get<0>(*(++zip_it_run)) &&
        4.4 == hpx::get<1>(*zip_it_run) && zip_it_end == ++zip_it_run);
    // NOLINTEND(bugprone-inc-dec-in-conditions)

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator decrementing and dereferencing
    //
    /////////////////////////////////////////////////////////////////////////////

    // NOLINTBEGIN(bugprone-inc-dec-in-conditions)
    HPX_TEST(zip_it_run == zip_it_end && zip_it_end == zip_it_run-- &&
        44. == hpx::get<0>(*zip_it_run) && 4.4 == hpx::get<1>(*zip_it_run) &&
        43. == hpx::get<0>(*(--zip_it_run)) &&
        3.3 == hpx::get<1>(*zip_it_run) &&
        42. == hpx::get<0>(*(--zip_it_run)) &&
        2.2 == hpx::get<1>(*zip_it_run) && zip_it_begin == zip_it_run);
    // NOLINTEND(bugprone-inc-dec-in-conditions)

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator copy construction and equality
    //
    /////////////////////////////////////////////////////////////////////////////

    hpx::util::zip_iterator<hpx::tuple<std::vector<double>::const_iterator,
        std::vector<double>::const_iterator>>
        zip_it_run_copy(zip_it_run);

    HPX_TEST(zip_it_run == zip_it_run && zip_it_run == zip_it_run_copy);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator inequality
    //
    /////////////////////////////////////////////////////////////////////////////

    // NOLINTBEGIN(bugprone-inc-dec-in-conditions)
    HPX_TEST(
        !(zip_it_run != zip_it_run_copy) && zip_it_run != ++zip_it_run_copy);
    // NOLINTEND(bugprone-inc-dec-in-conditions)

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator less than
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run_copy == zip_it_run + 1
    //
    // NOLINTBEGIN(bugprone-inc-dec-in-conditions)
    HPX_TEST(zip_it_run < zip_it_run_copy &&
        !(zip_it_run < --zip_it_run_copy) && zip_it_run == zip_it_run_copy);
    // NOLINTEND(bugprone-inc-dec-in-conditions)

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator less than or equal
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run_copy == zip_it_run
    //
    ++zip_it_run;
    zip_it_run_copy += 2;

    // NOLINTBEGIN(bugprone-inc-dec-in-conditions)
    HPX_TEST(zip_it_run <= zip_it_run_copy && zip_it_run <= --zip_it_run_copy &&
        !(zip_it_run <= --zip_it_run_copy) && zip_it_run <= zip_it_run);
    // NOLINTEND(bugprone-inc-dec-in-conditions)

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator greater than
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run_copy == zip_it_run - 1
    //
    // NOLINTBEGIN(bugprone-inc-dec-in-conditions)
    HPX_TEST(zip_it_run > zip_it_run_copy &&
        !(zip_it_run > ++zip_it_run_copy) && zip_it_run == zip_it_run_copy);
    // NOLINTEND(bugprone-inc-dec-in-conditions)

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator greater than or equal
    //
    /////////////////////////////////////////////////////////////////////////////

    ++zip_it_run;

    // Note: zip_it_run == zip_it_run_copy + 1
    //
    // NOLINTBEGIN(bugprone-inc-dec-in-conditions)
    HPX_TEST(zip_it_run >= zip_it_run_copy && --zip_it_run >= zip_it_run_copy &&
        !(zip_it_run >= ++zip_it_run_copy));
    // NOLINTEND(bugprone-inc-dec-in-conditions)

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator + int
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run == zip_it_run_copy - 1
    //
    zip_it_run = zip_it_run + 2;
    ++zip_it_run_copy;

    HPX_TEST(zip_it_run == zip_it_run_copy && zip_it_run == zip_it_begin + 3);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator - int
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run == zip_it_run_copy, and both are at end position
    //
    zip_it_run = zip_it_run - 2;
    --zip_it_run_copy;
    --zip_it_run_copy;

    HPX_TEST(zip_it_run == zip_it_run_copy && (zip_it_run - 1) == zip_it_begin);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator +=
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run == zip_it_run_copy, and both are at begin + 1
    //
    zip_it_run += 2;
    HPX_TEST(zip_it_run == zip_it_begin + 3);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator -=
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run is at end position, zip_it_run_copy is at
    // begin plus one.
    //
    zip_it_run -= 2;
    HPX_TEST(zip_it_run == zip_it_run_copy);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator getting member iterators
    //
    /////////////////////////////////////////////////////////////////////////////

    // Note: zip_it_run and zip_it_run_copy are both at
    // begin plus one.
    //
    HPX_TEST(
        hpx::get<0>(zip_it_run.get_iterator_tuple()) == vect1.begin() + 1 &&
        hpx::get<1>(zip_it_run.get_iterator_tuple()) == vect2.begin() + 1);

    /////////////////////////////////////////////////////////////////////////////
    //
    // Making zip iterators
    //
    /////////////////////////////////////////////////////////////////////////////

    std::vector<hpx::tuple<double, double>> vect_of_tuples(3);

    std::copy(
        hpx::util::zip_iterator(hpx::make_tuple(vect1.begin(), vect2.begin())),
        hpx::util::zip_iterator(hpx::make_tuple(vect1.end(), vect2.end())),
        vect_of_tuples.begin());

    HPX_TEST(42. == hpx::get<0>(*vect_of_tuples.begin()) &&
        2.2 == hpx::get<1>(*vect_of_tuples.begin()) &&
        43. == hpx::get<0>(*(vect_of_tuples.begin() + 1)) &&
        3.3 == hpx::get<1>(*(vect_of_tuples.begin() + 1)) &&
        44. == hpx::get<0>(*(vect_of_tuples.begin() + 2)) &&
        4.4 == hpx::get<1>(*(vect_of_tuples.begin() + 2)));

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator non-const --> const conversion
    //
    /////////////////////////////////////////////////////////////////////////////

    hpx::util::zip_iterator<hpx::tuple<std::set<int>::const_iterator,
        std::vector<double>::const_iterator>>
        zip_it_const(hpx::make_tuple(intset.begin(), vect2.begin()));
    //
    hpx::util::zip_iterator<hpx::tuple<std::set<int>::iterator,
        std::vector<double>::const_iterator>>
        zip_it_half_const(hpx::make_tuple(intset.begin(), vect2.begin()));
    //
    hpx::util::zip_iterator<
        hpx::tuple<std::set<int>::iterator, std::vector<double>::iterator>>
        zip_it_non_const(hpx::make_tuple(intset.begin(), vect2.begin()));

    zip_it_half_const = ++zip_it_non_const;
    zip_it_const = zip_it_half_const;
    ++zip_it_const;

    // Error: can't convert from const to non-const
    //  zip_it_non_const = ++zip_it_const;

    HPX_TEST(54 == hpx::get<0>(*zip_it_const) &&
        4.4 == hpx::get<1>(*zip_it_const) &&
        53 == hpx::get<0>(*zip_it_half_const) &&
        3.3 == hpx::get<1>(*zip_it_half_const));

    /////////////////////////////////////////////////////////////////////////////
    //
    // Zip iterator categories
    //
    /////////////////////////////////////////////////////////////////////////////

    // The big iterator of the previous test has vector, list, and set iterators.
    // Therefore, it must be bidirectional, but not random access.
    bool bBigItIsBidirectionalIterator = std::is_convertible<
        hpx::util::zip_iterator_category<zip_it_12_type>::type,
        std::bidirectional_iterator_tag>::value;

    bool bBigItIsRandomAccessIterator = std::is_convertible<
        hpx::util::zip_iterator_category<zip_it_12_type>::type,
        std::random_access_iterator_tag>::value;

    // A combining iterator with all vector iterators must have random access
    // traversal.
    //
    typedef hpx::util::zip_iterator<
        hpx::tuple<std::vector<double>::const_iterator,
            std::vector<double>::const_iterator>>
        all_vects_type;

    bool bAllVectsIsRandomAccessIterator = std::is_convertible<
        hpx::util::zip_iterator_category<all_vects_type>::type,
        std::random_access_iterator_tag>::value;

    // The big test.
    HPX_TEST(bBigItIsBidirectionalIterator && !bBigItIsRandomAccessIterator &&
        bAllVectsIsRandomAccessIterator);

    /////////////////////////////////////////////////////////////////////////////
    //
    // hpx::get<I> on operator[] of a zip_iterator (operator_brackets_proxy)
    // Regression test for #7557: projections rely on hpx::get<I> being
    // applicable to the result of iterator indexing (as done by
    // libc++'s std::sort implementation).
    //
    /////////////////////////////////////////////////////////////////////////////

    std::vector<int> keys = {4, 2, 7, 1};
    std::vector<char> vals = {'d', 'b', 'g', 'a'};

    auto zip_begin =
        hpx::util::zip_iterator(hpx::make_tuple(keys.begin(), vals.begin()));
    [[maybe_unused]] auto zip_end =
        hpx::util::zip_iterator(hpx::make_tuple(keys.end(), vals.end()));

    // hpx::get<I> through the proxy returned by operator[] (non-const)
    HPX_TEST(4 == hpx::get<0>(zip_begin[0]));
    HPX_TEST('d' == hpx::get<1>(zip_begin[0]));
    HPX_TEST(2 == hpx::get<0>(zip_begin[1]));
    HPX_TEST('b' == hpx::get<1>(zip_begin[1]));

    // same through a const zip_iterator (const proxy)
    auto const& zip_begin_const = zip_begin;
    HPX_TEST(7 == hpx::get<0>(zip_begin_const[2]));
    HPX_TEST('g' == hpx::get<1>(zip_begin_const[2]));

    // rvalue proxies: the generic hpx::get<I>(Tuple&&) overloads delegate to
    // the lvalue members and forward the result (which is always a reference
    // into the underlying sequence, never into the proxy or the temporary it
    // converts to), so no separate rvalue members are needed
    auto proxy_key = zip_begin[3];
    auto proxy_val = zip_begin[3];
    HPX_TEST(1 == hpx::get<0>(std::move(proxy_key)));
    HPX_TEST('a' == hpx::get<1>(std::move(proxy_val)));

    // const proxies: the const& member and the generic hpx::get<I>(Tuple
    // const&&) overload delegate to the const& member
    auto const& proxy_const = zip_begin[3];
    HPX_TEST(1 == hpx::get<0>(proxy_const));
    HPX_TEST(1 == hpx::get<0>(std::move(proxy_const)));

    // writing through the proxy must still work
    zip_begin[0] = hpx::make_tuple(9, 'z');
    HPX_TEST(9 == keys[0] && 'z' == vals[0]);

    // and the proxy must still convert to the underlying tuple of references
    hpx::tuple<int&, char&> t = zip_begin[1];
    HPX_TEST(2 == hpx::get<0>(t) && 'b' == hpx::get<1>(t));

    // verify that the proxy itself is not tuple-like for non-tuple-like
    // references (e.g. std::vector<bool>::iterator) - hpx::get must not apply
    {
        std::vector<bool> flags = {true, false, true};
        auto zip_flags = hpx::util::zip_iterator(
            hpx::make_tuple(flags.begin(), vals.begin()));

        static_assert(
            !hpx::traits::is_tuple_like_v<std::decay_t<decltype(zip_flags[0])>>,
            "operator_brackets_proxy should not be tuple-like when the "
            "underlying reference is not tuple-like");

        // the proxy is transparent to hpx::get<I> only if all elements of
        // the underlying reference tuple are lvalue references (otherwise
        // the elements would refer into the temporary the proxy converts
        // to and would dangle)
        static_assert(
            hpx::util::detail::all_lvalue_references_v<hpx::tuple<int&, char&>>,
            "a tuple of lvalue references is all-lvalue-references");
        static_assert(!hpx::util::detail::all_lvalue_references_v<
                          hpx::tuple<std::vector<bool>::reference, char&>>,
            "a tuple containing a proxy element is not all-lvalue-references");

        // the proxy converts to its reference which is a tuple here since
        // zip_iterator always exposes a tuple of references; the point of the
        // test above is that hpx::get works only through that conversion, not
        // directly on the proxy
        auto t_flags =
            static_cast<hpx::tuple<std::vector<bool>::reference, char&>>(
                zip_flags[0]);
        HPX_TEST(hpx::get<0>(t_flags) == true);
        HPX_TEST(
            hpx::get<1>(t_flags) == 'z');    // vals[0] was set to 'z' above
    }

    ///////////////////////////////////////////////////////////////////////////
    // tuple_size: the proxy is genuinely tuple-like when its reference is
    // (same gate as the tuple_element specialization above)
    ///////////////////////////////////////////////////////////////////////////
    static_assert(
        hpx::traits::is_tuple_like_v<std::decay_t<decltype(zip_begin[0])>>,
        "operator_brackets_proxy is tuple-like when the underlying reference "
        "is tuple-like and all of its elements are lvalue references");
    static_assert(hpx::tuple_size_v<std::decay_t<decltype(zip_begin[0])>> == 2,
        "the proxy has the same number of elements as its reference");

    // and hpx::get<I> keeps working through it
    HPX_TEST(9 == hpx::get<0>(zip_begin[0]));
    HPX_TEST('z' == hpx::get<1>(zip_begin[0]));

    ///////////////////////////////////////////////////////////////////////////
    // the traits when T is a reference itself: hpx::tuple_size is not defined
    // for reference types, so a reference is never tuple-like and the proxy
    // specializations (which require is_tuple_like_v) can never engage for
    // one. all_lvalue_references still gives the semantically right answer:
    // the elements reached through a reference to a tuple of lvalue
    // references are themselves lvalue references.
    ///////////////////////////////////////////////////////////////////////////
    static_assert(!hpx::traits::is_tuple_like_v<hpx::tuple<int&, char&>&>,
        "a reference type is not tuple-like");
    static_assert(
        hpx::util::detail::all_lvalue_references_v<hpx::tuple<int&, char&>&>,
        "elements reached through a reference to a tuple of lvalue references "
        "are lvalue references");
    static_assert(hpx::util::detail::all_lvalue_references_v<
                      hpx::tuple<int&, char&> const&>,
        "const references to such tuples behave the same");

    // mixed tuples must be detected no matter where the non-reference element
    // sits (std::conjunction stops instantiating the recursion as soon as one
    // element fails the check)
    static_assert(
        !hpx::util::detail::all_lvalue_references_v<hpx::tuple<int&, char>>,
        "a tuple with a non-reference element is not all-lvalue-references");
    static_assert(
        !hpx::util::detail::all_lvalue_references_v<hpx::tuple<char, int&>>,
        "the position of the non-reference element does not matter");

    return hpx::util::report_errors();
}
