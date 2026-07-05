//  Copyright (c) 2026 Adhithya Ragavan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for the datapar zip_iterator guard fix.
//
// Before the fix, calling mismatch/equal/find_end/search with par_simd and
// plain arithmetic pointers (int*) caused a hard compile error:
//   iterator_datapar_compatible<int*> is true, so the SIMD path was selected.
//   The SIMD path internally constructs zip_iterator<int*,int*> whose
//   value_type is hpx::tuple<int,int> (not arithmetic), causing the EVE
//   backend's store_/addressof to fail with no matching specialization.
//
// After the fix the guard checks iterator_datapar_compatible on the
// zip_iterator directly (always false for tuple value_type), so the
// algorithms correctly fall back to the scalar sequential path.

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <numeric>
#include <string>
#include <vector>

void test_mismatch_raw_ptr()
{
    std::vector<int> a(100), b(100);
    std::iota(a.begin(), a.end(), 0);
    std::iota(b.begin(), b.end(), 0);
    b[42] = -1;

    int* pa = a.data();
    int* pb = b.data();

    auto r = hpx::mismatch(hpx::execution::par_simd, pa, pa + 100, pb);
    HPX_TEST_EQ(r.first - pa, std::ptrdiff_t(42));
    HPX_TEST_EQ(r.second - pb, std::ptrdiff_t(42));

    std::iota(b.begin(), b.end(), 0);
    auto r2 = hpx::mismatch(hpx::execution::par_simd, pa, pa + 100, pb);
    HPX_TEST(r2.first == pa + 100);
}

void test_equal_raw_ptr()
{
    std::vector<int> a(100), b(100);
    std::iota(a.begin(), a.end(), 0);
    std::iota(b.begin(), b.end(), 0);

    int* pa = a.data();
    int* pb = b.data();

    HPX_TEST(hpx::equal(hpx::execution::par_simd, pa, pa + 100, pb));
    b[50] = -1;
    HPX_TEST(!hpx::equal(hpx::execution::par_simd, pa, pa + 100, pb));
}

void test_find_end_raw_ptr()
{
    // haystack: {1,2,3,4,1,2,3,4,1,2}
    // needle:   {1,2,3}
    // occurrences at index 0 and 4; last occurrence starts at index 4
    std::vector<int> h = {1, 2, 3, 4, 1, 2, 3, 4, 1, 2};
    std::vector<int> n = {1, 2, 3};
    int* ph = h.data();
    int* pn = n.data();

    auto* r = hpx::find_end(hpx::execution::par_simd, ph,
        ph + static_cast<std::ptrdiff_t>(h.size()), pn,
        pn + static_cast<std::ptrdiff_t>(n.size()));
    HPX_TEST_EQ(r - ph, std::ptrdiff_t(4));
}

void test_search_raw_ptr()
{
    // haystack: {1,2,3,4,5,6}
    // needle:   {3,4,5}
    // first occurrence starts at index 2
    std::vector<int> h = {1, 2, 3, 4, 5, 6};
    std::vector<int> n = {3, 4, 5};
    int* ph = h.data();
    int* pn = n.data();

    auto* r = hpx::search(hpx::execution::par_simd, ph,
        ph + static_cast<std::ptrdiff_t>(h.size()), pn,
        pn + static_cast<std::ptrdiff_t>(n.size()));
    HPX_TEST_EQ(r - ph, std::ptrdiff_t(2));
}

int hpx_main()
{
    test_mismatch_raw_ptr();
    test_equal_raw_ptr();
    test_find_end_raw_ptr();
    test_search_raw_ptr();
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
