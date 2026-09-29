//  Copyright (c) 2026 the-ivii
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
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <new>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define SIZE 64
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
    initialize_values(
        values, [](int i) { return make_sort_value<T>(SIZE - i); });
}

template <typename T>
void initialize_mixed(hpx::partitioned_vector<T>& values)
{
    initialize_values(
        values, [](int i) { return make_sort_value<T>((i * 17 + 3) % SIZE); });
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
    catch (std::exception const&)
    {
        return true;
    }
    catch (...)
    {
        return true;
    }
    return false;
}

template <typename T>
void test_sort_throwing(std::vector<hpx::id_type>& localities)
{
    hpx::partitioned_vector<T> values(
        SIZE, T{}, hpx::container_layout(localities));
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
        SIZE, T{}, hpx::container_layout(localities));

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

// Exercise the block network with exhaustive zero/one inputs, including a
// short final block. A stage must never read/write overlapping blocks.
void test_merge_network()
{
    namespace detail = hpx::parallel::detail;
    for (std::size_t count = 2; count <= 7; ++count)
    {
        for (std::size_t tail : {1, 2, 3})
        {
            std::size_t combinations = tail + 1;
            for (std::size_t i = 1; i < count; ++i)
            {
                combinations *= 4;
            }
            for (std::size_t input = 0; input < combinations; ++input)
            {
                std::vector<std::vector<int>> blocks(count);
                auto digits = input;
                std::size_t zeros = 0;
                for (std::size_t i = 0; i < count; ++i)
                {
                    auto const size = i + 1 == count ? tail : 3;
                    auto const nzero = digits % (size + 1);
                    digits /= size + 1;
                    zeros += nzero;
                    blocks[i].resize(size, 1);
                    std::fill_n(blocks[i].begin(), nzero, 0);
                }
                detail::segmented_sort_merge_stages(
                    count, [&](auto const& pairs) {
                        std::vector<bool> touched(count, false);
                        for (auto [left, right] : pairs)
                        {
                            HPX_TEST(!touched[left] && !touched[right]);
                            touched[left] = touched[right] = true;
                            std::vector<int> merged(
                                blocks[left].size() + blocks[right].size());
                            std::merge(blocks[left].begin(), blocks[left].end(),
                                blocks[right].begin(), blocks[right].end(),
                                merged.begin());
                            auto middle = merged.begin() +
                                static_cast<std::ptrdiff_t>(
                                    blocks[left].size());
                            std::copy(
                                merged.begin(), middle, blocks[left].begin());
                            std::copy(
                                middle, merged.end(), blocks[right].begin());
                        }
                    });
                for (auto const& block : blocks)
                {
                    for (int value : block)
                    {
                        HPX_TEST_EQ(value, zeros != 0 ? 0 : 1);
                        if (zeros != 0)
                        {
                            --zeros;
                        }
                    }
                }
            }
        }
    }
}

struct counted_sort_value
{
    inline static std::atomic<std::size_t> live{0};
    inline static std::atomic<std::size_t> peak{0};
    inline static std::atomic<std::size_t> copies{0};
    int value = 0;

    counted_sort_value()
    {
        auto const current = ++live;
        auto previous = peak.load();
        while (previous < current &&
            !peak.compare_exchange_weak(previous, current))
        {
        }
    }
    counted_sort_value(counted_sort_value const& other)
      : counted_sort_value()
    {
        value = other.value;
        ++copies;
    }
    counted_sort_value(counted_sort_value&& other) noexcept
      : counted_sort_value()
    {
        value = other.value;
    }
    counted_sort_value& operator=(counted_sort_value const&) = default;
    counted_sort_value& operator=(counted_sort_value&&) = default;
    ~counted_sort_value()
    {
        --live;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & value;
    }
};

// HPX component factories must be registered before runtime startup.
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
HPX_REGISTER_PARTITIONED_VECTOR(counted_sort_value)

void test_merge_buffer()
{
    namespace detail = hpx::parallel::detail;
    auto const size = detail::segmented_sort_max_block_size;
    {
        std::vector<counted_sort_value> left(size), right(size);
        for (std::size_t i = 0; i < size; ++i)
        {
            left[i].value = static_cast<int>(2 * i);
            right[i].value = static_cast<int>(2 * i + 1);
        }
        auto pred = [](auto const& a, auto const& b) {
            return a.value < b.value;
        };
        // HPX's merge itself creates temporary values. Compare against a
        // direct merge to isolate copies and storage added by our wrapper.
        std::size_t reference_copies;
        std::size_t reference_peak;
        {
            std::vector<counted_sort_value> reference(2 * size);
            hpx::merge(hpx::execution::seq, left.begin(), left.end(),
                right.begin(), right.end(), reference.begin(), pred);
            reference_copies = counted_sort_value::copies.load();
            reference_peak = counted_sort_value::peak.load();
        }
        counted_sort_value::copies = 0;
        counted_sort_value::peak = counted_sort_value::live.load();
        auto merged = detail::segmented_sort_merge_pair(
            hpx::execution::seq, left, right, pred);
        HPX_TEST_EQ(counted_sort_value::copies.load(), reference_copies);
        HPX_TEST_EQ(counted_sort_value::peak.load(), reference_peak);
        for (std::size_t i = 0; i < merged.size(); ++i)
        {
            HPX_TEST_EQ(merged[i].value, static_cast<int>(i));
        }
    }
    HPX_TEST_EQ(counted_sort_value::live.load(), std::size_t(0));
}

void test_host_placement(std::vector<hpx::id_type> const& localities)
{
    namespace detail = hpx::parallel::detail;
    constexpr auto unit = static_cast<std::ptrdiff_t>(
        detail::segmented_sort_max_block_size / 32 + 1);
    std::vector<int> data(static_cast<std::size_t>(45 * unit));
    using iterator = std::vector<int>::iterator;
    detail::segmented_sort_block<iterator> runs;
    auto const& a = localities.front();
    auto const& b = localities.back();
    // A owns the largest individual run (5 units); B owns ten of 4.
    runs.emplace_back(a, a, data.begin(), data.begin() + 5 * unit);
    for (std::ptrdiff_t i = 5 * unit; i < 45 * unit; i += 4 * unit)
    {
        runs.emplace_back(b, b, data.begin() + i, data.begin() + i + 4 * unit);
    }
    HPX_TEST(detail::segmented_sort_host(runs, {}) == b);
    HPX_TEST_EQ(detail::segmented_sort_block_capacity(runs),
        a == b ? detail::segmented_sort_max_block_size : (data.size() + 1) / 2);

    if (a != b)
    {
        detail::segmented_sort_block<iterator> left{
            {a, a, data.begin(), data.begin() + unit}};
        detail::segmented_sort_block<iterator> right{
            {b, b, data.begin() + unit, data.begin() + 2 * unit}};
        std::map<hpx::id_type, std::size_t> assignments;
        for (std::size_t i = 0; i != 8; ++i)
        {
            auto const host =
                detail::segmented_sort_host(left, right, assignments);
            ++assignments[host];
        }
        HPX_TEST_EQ(assignments[a], std::size_t(4));
        HPX_TEST_EQ(assignments[b], std::size_t(4));
    }

    if (a != b)
    {
        hpx::partitioned_vector<int> values(
            200, 0, hpx::container_layout(20, std::vector<hpx::id_type>{a, b}));
        initialize_values(values, [](int i) { return 200 - i; });
        auto first = values.begin() + 90;
        auto participating = detail::segmented_sort_runs(first, values.end());
        HPX_TEST_EQ(participating.size(), std::size_t(11));
        HPX_TEST(participating.front().locality == a);
        HPX_TEST_EQ(std::count_if(participating.begin(), participating.end(),
                        [&](auto const& run) { return run.locality == b; }),
            std::ptrdiff_t(10));
        HPX_TEST(detail::segmented_sort_host(participating, {}) == b);
        auto expected = copy_values(values);
        std::sort(expected.begin() + 90, expected.end());
        hpx::sort(hpx::execution::par, first, values.end());
        HPX_TEST(copy_values(values) == expected);
    }
}

void test_bounded_blocks(std::vector<hpx::id_type> const& localities)
{
    namespace detail = hpx::parallel::detail;
    auto const size = 8 * detail::segmented_sort_max_block_size + 7;
    hpx::partitioned_vector<int> values(
        size, 0, hpx::container_layout(3, localities));
    initialize_values(values,
        [](std::size_t i) { return static_cast<int>((i * 17 + 3) % 97); });
    auto expected = copy_values(values);
    std::sort(expected.begin() + 1, expected.end() - 2);
    auto runs =
        detail::segmented_sort_runs(values.begin() + 1, values.end() - 2);
    auto blocks = detail::segmented_sort_blocks(
        runs, detail::segmented_sort_max_block_size);
    std::size_t total = 0;
    for (std::size_t i = 0; i < blocks.size(); ++i)
    {
        auto const n = detail::segmented_sort_block_size(blocks[i]);
        HPX_TEST_LTE(n, detail::segmented_sort_max_block_size);
        if (i + 1 != blocks.size())
        {
            HPX_TEST_EQ(n, detail::segmented_sort_max_block_size);
        }
        total += n;
        for (auto const& run : blocks[i])
        {
            HPX_TEST(run.locality ==
                hpx::get_colocation_id(hpx::launch::sync, run.id));
            HPX_TEST(run.locality != run.id);
        }
    }
    HPX_TEST_EQ(total, size - 3);
    hpx::sort(hpx::execution::par(hpx::execution::task), values.begin() + 1,
        values.end() - 2)
        .get();
    HPX_TEST(copy_values(values) == expected);

    // Exercise the distributed merge with nontrivial values and descending
    // order as well as the default integer comparator above.
    hpx::partitioned_vector<std::string> strings(
        2 * detail::segmented_sort_max_block_size + 7, std::string{},
        hpx::container_layout(3, localities));
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
        // Each initial block lies within one band. Only a merge compares
        // different bands, after the local sort phase has succeeded.
        constexpr auto band_size = static_cast<int>(
            hpx::parallel::detail::segmented_sort_max_block_size);
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
            4 * hpx::parallel::detail::segmented_sort_max_block_size;
        hpx::partitioned_vector<int> values(
            size, 0, hpx::container_layout(4, localities));
        initialize_values(values, [](int i) { return i; });
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
        // Reuse immediately: the failed stage must have drained its actions.
        initialize_reverse(values);
        test_sort_once(values);
    }
}

// This comparator observes overlap between two initial block sorts. Waiting
// on an HPX future yields the worker, and the timeout also diagnoses the old
// serial scheduler without hanging the test.
struct initial_sort_probe_state
{
    std::array<std::atomic<bool>, 2> entered{};
    std::atomic<std::size_t> started{0};
    hpx::promise<void> both_started;
    hpx::shared_future<void> ready = both_started.get_future();
    std::atomic<bool> timed_out{false};
};

struct initial_sort_probe
{
    inline static std::shared_ptr<initial_sort_probe_state> state;
    static constexpr auto block_size =
        hpx::parallel::detail::segmented_sort_max_block_size;

    bool operator()(int a, int b) const
    {
        auto const block = static_cast<std::size_t>(a) / block_size;
        if (!state->entered[block].exchange(true))
        {
            if (++state->started == 2)
            {
                state->both_started.set_value();
            }
            if (state->ready.wait_for(std::chrono::seconds(5)) !=
                hpx::future_status::ready)
            {
                state->timed_out = true;
            }
        }
        return a < b;
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }
};

void test_initial_sort_concurrency()
{
    namespace detail = hpx::parallel::detail;
    constexpr auto size = initial_sort_probe::block_size;
    // Four physical pieces produce two buffered logical blocks on this
    // locality, together using the complete four-block scratch budget.
    hpx::partitioned_vector<int> values(2 * size, 0,
        hpx::container_layout(4, std::vector<hpx::id_type>{hpx::find_here()}));
    initialize_values(values, [](std::size_t i) {
        return static_cast<int>((i / size) * size + size - 1 - i % size);
    });
    auto runs = detail::segmented_sort_runs(values.begin(), values.end());
    auto blocks = detail::segmented_sort_blocks(runs, size);
    initial_sort_probe::state = std::make_shared<initial_sort_probe_state>();
    detail::segmented_sort_stage(hpx::execution::par, blocks, {{0, 0}, {1, 1}},
        initial_sort_probe{}, hpx::identity_v, std::false_type{});
    HPX_TEST(!initial_sort_probe::state->timed_out.load());
    initial_sort_probe::state.reset();
    auto got = copy_values(values);
    HPX_TEST(std::is_sorted(got.begin(), got.end()));
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
}

struct counted_value_less
{
    bool operator()(
        counted_sort_value const& a, counted_sort_value const& b) const
    {
        return a.value < b.value;
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }
};

void test_ordered_endpoints()
{
    namespace detail = hpx::parallel::detail;
    hpx::partitioned_vector<counted_sort_value> values(4096,
        counted_sort_value{},
        hpx::container_layout(2, std::vector<hpx::id_type>{hpx::find_here()}));
    initialize_values(values, [](int i) {
        counted_sort_value value;
        value.value = i;
        return value;
    });
    auto runs = detail::segmented_sort_runs(values.begin(), values.end());
    auto blocks = detail::segmented_sort_blocks(runs, 2048);
    counted_sort_value::copies = 0;
    using local_iterator = hpx::traits::segmented_iterator_traits<
        decltype(values.begin())>::local_iterator;
    detail::segmented_sort_compare_split<local_iterator>::sequential(
        hpx::execution::seq, blocks[0], blocks[1], counted_value_less{},
        hpx::identity_v);
    HPX_TEST_EQ(counted_sort_value::copies.load(), std::size_t(2));
    auto got = copy_values(values);
    for (std::size_t i = 0; i < got.size(); ++i)
    {
        HPX_TEST_EQ(got[i].value, static_cast<int>(i));
    }
}

void test_transfer_batches(std::vector<hpx::id_type> const& localities)
{
    namespace detail = hpx::parallel::detail;
    auto const partitions = 4 * localities.size();
    hpx::partitioned_vector<int> values(
        partitions * 9, 0, hpx::container_layout(partitions, localities));
    initialize_values(values, [](int i) { return i; });
    auto runs = detail::segmented_sort_runs(values.begin(), values.end());
    decltype(runs) block;
    std::vector<int> expected;
    // Interleave ownership so four localities cross the eight-action batch
    // boundary. The helper must preserve logical order and disjoint writes.
    for (std::size_t piece = 0; piece < 4; ++piece)
    {
        for (std::size_t locality = 0; locality < localities.size(); ++locality)
        {
            auto const index = 4 * locality + piece;
            block.push_back(runs[index]);
            for (std::size_t i = 0; i < 9; ++i)
            {
                expected.push_back(static_cast<int>(9 * index + i));
            }
        }
    }
    auto fetched =
        detail::segmented_sort_fetch_block(hpx::execution::par, block);
    HPX_TEST(fetched == expected);
    for (auto& value : fetched)
    {
        value = -value - 1;
    }
    auto end = detail::segmented_sort_store_block(
        hpx::execution::par, block, fetched.begin());
    HPX_TEST(end == fetched.end());
    auto got = copy_values(values);
    for (std::size_t i = 0; i < got.size(); ++i)
    {
        HPX_TEST_EQ(got[i], -static_cast<int>(i) - 1);
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    test_merge_network();
    test_merge_buffer();
    test_host_placement(localities);
    test_bounded_blocks(localities);
    if (hpx::get_os_thread_count() > 1)
    {
        test_initial_sort_concurrency();
    }
    test_ordered_endpoints();
    test_transfer_batches(localities);
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
