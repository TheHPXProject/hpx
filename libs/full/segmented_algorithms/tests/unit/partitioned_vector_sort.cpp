//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_is_sorted.hpp>
#include <hpx/include/parallel_sort.hpp>
#include <hpx/include/partitioned_vector.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define HPX_SEGMENTED_SORT_TEST_SIZE 64
#define LARGE_SIZE 2048

template <typename T>
T make_sort_value(int i)
{
    if constexpr (std::is_same_v<T, std::string>)
    {
        return std::to_string(i);
    }
    else
    {
        return T(i);
    }
}

// Transfer test data per partition instead of issuing a synchronous remote
// request for every element during setup and verification.
template <typename T>
std::vector<T> copy_values(hpx::partitioned_vector<T> const& values)
{
    std::vector<std::size_t> positions(values.size());
    std::iota(positions.begin(), positions.end(), std::size_t(0));
    return values.get_values(hpx::launch::sync, positions);
}

template <typename T, typename F>
void initialize_values(hpx::partitioned_vector<T>& values, F make_value)
{
    std::vector<std::size_t> positions(values.size());
    std::iota(positions.begin(), positions.end(), std::size_t(0));
    std::vector<T> data(values.size());
    std::transform(
        positions.begin(), positions.end(), data.begin(), make_value);
    values.set_values(hpx::launch::sync, positions, data);
}

template <typename T>
void verify_sorted(
    hpx::partitioned_vector<T> const& values, std::vector<T> expected)
{
    std::sort(expected.begin(), expected.end());
    HPX_TEST(hpx::is_sorted(values.begin(), values.end()));

    std::vector<T> const got = copy_values(values);
    HPX_TEST_EQ(got.size(), expected.size());
    for (std::size_t i = 0; i != got.size(); ++i)
    {
        HPX_TEST_EQ(got[i], expected[i]);
    }
}

template <typename T, typename Comp>
void verify_sorted(hpx::partitioned_vector<T> const& values,
    std::vector<T> expected, Comp comp)
{
    std::sort(expected.begin(), expected.end(), comp);
    HPX_TEST(hpx::is_sorted(values.begin(), values.end(), comp));

    std::vector<T> const got = copy_values(values);
    HPX_TEST_EQ(got.size(), expected.size());
    for (std::size_t i = 0; i != got.size(); ++i)
    {
        HPX_TEST_EQ(got[i], expected[i]);
    }
}

template <typename T>
void initialize_reverse(hpx::partitioned_vector<T>& values)
{
    initialize_values(values, [](int i) {
        return make_sort_value<T>(HPX_SEGMENTED_SORT_TEST_SIZE - i);
    });
}

template <typename T>
void initialize_mixed(hpx::partitioned_vector<T>& values)
{
    initialize_values(values, [](int i) {
        return make_sort_value<T>((i * 17 + 3) % HPX_SEGMENTED_SORT_TEST_SIZE);
    });
}

template <typename T>
void initialize_sorted(hpx::partitioned_vector<T>& values)
{
    initialize_values(values, [](int i) { return make_sort_value<T>(i); });
}

template <typename T>
void initialize_duplicates(hpx::partitioned_vector<T>& values)
{
    initialize_values(values, [](int i) { return make_sort_value<T>(i % 4); });
}

template <typename T>
void initialize_mixed_n(hpx::partitioned_vector<T>& values, int n)
{
    initialize_values(
        values, [n](int i) { return make_sort_value<T>((i * 17 + 3) % n); });
}

struct throwing_less
{
    // Two template parameters so libc++ can pass heterogeneous
    // iterator dereference / proxy types into std::sort.
    template <typename T, typename U>
    bool operator()(T const&, U const&) const
    {
        throw std::runtime_error("segmented sort comparator");
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }
};

template <typename T>
void test_sort_once(hpx::partitioned_vector<T>& values)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(values.begin(), values.end());
    verify_sorted(values, expected);
}

template <typename ExPolicy, typename T>
void test_sort_once(ExPolicy&& policy, hpx::partitioned_vector<T>& values)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(HPX_FORWARD(ExPolicy, policy), values.begin(), values.end());
    verify_sorted(values, expected);
}

template <typename ExPolicy, typename T>
void test_sort_once_async(ExPolicy&& policy, hpx::partitioned_vector<T>& values)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(HPX_FORWARD(ExPolicy, policy), values.begin(), values.end())
        .get();
    verify_sorted(values, expected);
}

template <typename T, typename Comp>
void test_sort_comp(hpx::partitioned_vector<T>& values, Comp comp)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(values.begin(), values.end(), comp);
    verify_sorted(values, expected, comp);
}

template <typename ExPolicy, typename T, typename Comp>
void test_sort_comp(
    ExPolicy&& policy, hpx::partitioned_vector<T>& values, Comp comp)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(
        HPX_FORWARD(ExPolicy, policy), values.begin(), values.end(), comp);
    verify_sorted(values, expected, comp);
}

template <typename ExPolicy, typename T, typename Comp>
void test_sort_comp_async(
    ExPolicy&& policy, hpx::partitioned_vector<T>& values, Comp comp)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(HPX_FORWARD(ExPolicy, policy), values.begin(), values.end(), comp)
        .get();
    verify_sorted(values, expected, comp);
}

bool caught_sort_exception(std::function<void()> const& f)
{
    try
    {
        f();
    }
    catch (hpx::exception_list const&)
    {
        return true;
    }
    catch (std::runtime_error const&)
    {
        return true;
    }
    return false;
}

template <typename T>
void test_sort_throwing(std::vector<hpx::id_type>& localities)
{
    hpx::partitioned_vector<T> values(
        HPX_SEGMENTED_SORT_TEST_SIZE, T{}, hpx::container_layout(localities));
    initialize_mixed(values);

    throwing_less comp;
    HPX_TEST(caught_sort_exception(
        [&]() { hpx::sort(values.begin(), values.end(), comp); }));

    initialize_mixed(values);
    HPX_TEST(caught_sort_exception([&]() {
        hpx::sort(hpx::execution::par, values.begin(), values.end(), comp);
    }));

    initialize_mixed(values);
    HPX_TEST(caught_sort_exception([&]() {
        hpx::sort(hpx::execution::par(hpx::execution::task), values.begin(),
            values.end(), comp)
            .get();
    }));
}

template <typename T>
void test_sort_large(std::vector<hpx::id_type>& localities)
{
    hpx::partitioned_vector<T> values(
        LARGE_SIZE, T{}, hpx::container_layout(localities));
    initialize_mixed_n(values, LARGE_SIZE);
    test_sort_once(hpx::execution::par, values);
}

template <typename T>
void run_cases(std::vector<hpx::id_type>& localities)
{
    hpx::partitioned_vector<T> values(
        HPX_SEGMENTED_SORT_TEST_SIZE, T{}, hpx::container_layout(localities));

    initialize_reverse(values);
    test_sort_once(values);

    initialize_mixed(values);
    test_sort_once(hpx::execution::seq, values);

    initialize_mixed(values);
    test_sort_once(hpx::execution::par, values);

    initialize_mixed(values);
    test_sort_once_async(hpx::execution::seq(hpx::execution::task), values);

    initialize_mixed(values);
    test_sort_once_async(hpx::execution::par(hpx::execution::task), values);

    initialize_sorted(values);
    test_sort_once(values);

    initialize_duplicates(values);
    test_sort_once(values);

    std::greater<T> greater;
    initialize_mixed(values);
    test_sort_comp(values, greater);

    initialize_mixed(values);
    test_sort_comp(hpx::execution::par, values, greater);

    initialize_mixed(values);
    test_sort_comp_async(
        hpx::execution::par(hpx::execution::task), values, greater);

    hpx::partitioned_vector<T> empty(0, T{}, hpx::container_layout(localities));
    test_sort_once(empty);
}

template <typename ExPolicy>
void test_sort_subranges(
    ExPolicy const& policy, std::vector<hpx::id_type> const& localities)
{
    // Eight partitions put multiple partitions on each locality. The
    // [3, 47) subrange spans five runs, including clipped endpoints.
    // [33, 77) starts on a remote partition with two or four localities.
    hpx::partitioned_vector<int> values(
        80, 0, hpx::container_layout(8, localities));
    std::pair<std::ptrdiff_t, std::ptrdiff_t> const ranges[] = {
        {0, 0}, {3, 4}, {2, 7}, {3, 10}, {10, 20}, {3, 47}, {33, 77}, {0, 80}};

    for (auto const& [begin, end] : ranges)
    {
        initialize_mixed_n(values, 80);
        auto expected = copy_values(values);
        std::sort(expected.begin() + begin, expected.begin() + end,
            std::greater<int>{});

        if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
        {
            hpx::sort(policy, values.begin() + begin, values.begin() + end,
                std::greater<int>{})
                .get();
        }
        else
        {
            hpx::sort(policy, values.begin() + begin, values.begin() + end,
                std::greater<int>{});
        }

        // Comparing the whole vector also checks that values outside the
        // requested subrange remain unchanged.
        HPX_TEST(copy_values(values) == expected);
    }
}

/// Verify compare-split phases with uneven runs and duplicate values.
template <typename ExPolicy>
void test_uneven_merge_split(
    ExPolicy const& policy, std::vector<hpx::id_type> const& localities)
{
    // Eleven physical partitions across four localities exercise both parities
    // of the compare-split network, unequal run sizes, and duplicate values.
    hpx::partitioned_vector<int> values(
        257, 0, hpx::container_layout(11, localities));
    initialize_values(values,
        [](std::size_t i) { return static_cast<int>((i * 37 + 11) % 23); });
    auto const expected = copy_values(values);

    if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
    {
        hpx::sort(policy, values.begin(), values.end()).get();
    }
    else
    {
        hpx::sort(policy, values.begin(), values.end());
    }
    verify_sorted(values, expected);
}

/// Verify that a complete two-run exchange remains chunked and distributed.
void test_two_run_merge_split(std::vector<hpx::id_type> const& localities)
{
    // Put two reversed runs on different localities. Every value crosses the
    // merge partition, exercising the bounded exchange without gathering the
    // complete input on either locality.
    std::vector<hpx::id_type> hosts{
        localities[0], localities[localities.size() > 1 ? 1 : 0]};
    hpx::partitioned_vector<int> values(
        2 * (65536 + 7), 0, hpx::container_layout(2, hosts));
    initialize_values(values, [&](std::size_t i) {
        auto const run_size = values.size() / 2;
        if (i < run_size)
        {
            return static_cast<int>(run_size + i);
        }
        return static_cast<int>(i - run_size);
    });
    auto const expected = copy_values(values);

    hpx::sort(hpx::execution::par, values.begin(), values.end());
    verify_sorted(values, expected);
}

void test_large_transfers(std::vector<hpx::id_type> const& localities)
{
    // Large enough to exercise multiple bounded transfers through the public
    // algorithm without depending on implementation helpers.
    constexpr std::size_t size = 8 * 65536 + 7;
    hpx::partitioned_vector<int> values(
        size, 0, hpx::container_layout(3, localities));
    initialize_values(values,
        [](std::size_t i) { return static_cast<int>((i * 17 + 3) % 97); });
    auto expected = copy_values(values);
    std::sort(expected.begin() + 1, expected.end() - 2);
    hpx::sort(hpx::execution::par(hpx::execution::task), values.begin() + 1,
        values.end() - 2)
        .get();
    HPX_TEST(copy_values(values) == expected);

    // Exercise the distributed merge with nontrivial values and descending
    // order as well as the default integer comparator above.
    hpx::partitioned_vector<std::string> strings(
        2 * 65536 + 7, std::string{}, hpx::container_layout(3, localities));
    initialize_values(strings, [](std::size_t i) {
        return std::to_string((i * 17 + 3) % 97) + std::string(32, 'x');
    });
    auto expected_strings = copy_values(strings);
    std::greater<std::string> greater;
    std::sort(expected_strings.begin(), expected_strings.end(), greater);
    hpx::sort(hpx::execution::seq(hpx::execution::task), strings.begin(),
        strings.end(), greater)
        .get();
    HPX_TEST(copy_values(strings) == expected_strings);
}

struct throwing_merge_less
{
    bool allocation;

    bool operator()(int a, int b) const
    {
        // Each initial run lies within one band. Only a merge compares
        // different bands, after the local sort phase has succeeded.
        constexpr auto band_size = static_cast<int>(
            hpx::parallel::detail::segmented_sort_transfer_chunk_size);
        if (a / band_size != b / band_size)
        {
            if (allocation)
            {
                throw std::bad_alloc();
            }
            throw std::runtime_error("segmented merge comparator");
        }
        return a < b;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & allocation;
    }
};

template <typename ExPolicy>
void test_merge_exceptions(
    ExPolicy const& policy, std::vector<hpx::id_type> const& localities)
{
    for (bool allocation : {false, true})
    {
        auto const size =
            4 * hpx::parallel::detail::segmented_sort_transfer_chunk_size;
        hpx::partitioned_vector<int> values(
            size, 0, hpx::container_layout(4, localities));
        initialize_values(values, [](std::size_t i) {
            constexpr auto band_size =
                hpx::parallel::detail::segmented_sort_transfer_chunk_size;
            auto const band = i / band_size;
            return static_cast<int>((3 - band) * band_size + i % band_size);
        });
        bool caught = false;
        try
        {
            if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
            {
                hpx::sort(policy, values.begin(), values.end(),
                    throwing_merge_less{allocation})
                    .get();
            }
            else
            {
                hpx::sort(policy, values.begin(), values.end(),
                    throwing_merge_less{allocation});
            }
        }
        catch (std::bad_alloc const&)
        {
            HPX_TEST(allocation);
            caught = true;
        }
        catch (hpx::exception_list const&)
        {
            HPX_TEST(!allocation);
            caught = true;
        }
        HPX_TEST(caught);
        // Reuse immediately: the failed operation must have drained its
        // actions.
        initialize_reverse(values);
        test_sort_once(values);
    }
}

struct collection_gate
{
    hpx::promise<void> release;
    hpx::shared_future<void> ready = release.get_future();
};

struct collection_probe_iterator
{
    hpx::partitioned_vector<int>::iterator base;
    std::shared_ptr<collection_gate> gate;

    bool operator==(collection_probe_iterator const& other) const
    {
        return base == other.base;
    }
};

namespace hpx::traits {
    template <>
    struct segmented_iterator_traits<collection_probe_iterator>
      : segmented_iterator_traits<hpx::partitioned_vector<int>::iterator>
    {
        using base_traits =
            segmented_iterator_traits<hpx::partitioned_vector<int>::iterator>;

        static segment_iterator segment(collection_probe_iterator iter)
        {
            iter.gate->ready.wait();
            return base_traits::segment(iter.base);
        }

        static local_iterator local(collection_probe_iterator iter)
        {
            return base_traits::local(iter.base);
        }

        static std::uint32_t get_locality_id(segment_iterator const&)
        {
            return hpx::naming::invalid_locality_id;
        }
    };
}    // namespace hpx::traits

template <typename ExPolicy>
void test_async_run_collection(ExPolicy const& policy)
{
    hpx::partitioned_vector<int> values(64, 0,
        hpx::container_layout(2, std::vector<hpx::id_type>{hpx::find_here()}));
    initialize_reverse(values);
    auto gate = std::make_shared<collection_gate>();
    hpx::future<void> sorted;
    hpx::promise<void> returned;
    auto return_signal = returned.get_future();
    auto invoke = hpx::async([&]() {
        sorted = hpx::parallel::detail::segmented_sort(policy,
            collection_probe_iterator{values.begin(), gate},
            collection_probe_iterator{values.end(), gate}, std::less<int>{},
            hpx::identity_v, hpx::is_sequenced_execution_policy<ExPolicy>{});
        returned.set_value();
    });
    // Collection is deliberately blocked; obtaining the sort future must
    // not wait for it. Release on timeout too, so the old path fails cleanly.
    auto const status = return_signal.wait_for(std::chrono::seconds(5));
    gate->release.set_value();
    invoke.get();
    sorted.get();
    HPX_TEST(status == hpx::future_status::ready);
    auto got = copy_values(values);
    HPX_TEST(std::is_sorted(got.begin(), got.end()));

    auto runs = hpx::parallel::detail::segmented_sort_runs(
        collection_probe_iterator{values.begin(), gate},
        collection_probe_iterator{values.end(), gate});
    for (auto const& run : runs)
    {
        HPX_TEST(run.locality == hpx::find_here());
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    test_uneven_merge_split(hpx::execution::seq, localities);
    test_uneven_merge_split(hpx::execution::par, localities);
    test_uneven_merge_split(
        hpx::execution::par(hpx::execution::task), localities);
    test_two_run_merge_split(localities);
    test_large_transfers(localities);
    test_async_run_collection(hpx::execution::seq(hpx::execution::task));
    test_async_run_collection(hpx::execution::par(hpx::execution::task));
    test_merge_exceptions(hpx::execution::seq, localities);
    test_merge_exceptions(hpx::execution::par, localities);
    test_merge_exceptions(
        hpx::execution::seq(hpx::execution::task), localities);
    test_merge_exceptions(
        hpx::execution::par(hpx::execution::task), localities);
    run_cases<int>(localities);
    run_cases<std::string>(localities);
    test_sort_large<int>(localities);
    test_sort_throwing<int>(localities);
    test_sort_subranges(hpx::execution::seq, localities);
    test_sort_subranges(hpx::execution::par, localities);
    test_sort_subranges(hpx::execution::seq(hpx::execution::task), localities);
    test_sort_subranges(hpx::execution::par(hpx::execution::task), localities);
    return hpx::util::report_errors();
}
#endif
