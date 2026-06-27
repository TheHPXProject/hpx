//  Copyright (c) 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Tests for hpx::experimental::for_each_index (P4150R0).
//
// A minimal layout-mapping stub is used so the tests compile on any C++
// standard mode without requiring C++23 <mdspan>.

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Minimal layout-mapping stub.
//
// Satisfies the concept checked by hpx::experimental::is_layout_mapping_v:
//   - index_type  member typedef
//   - extents_type member typedef
//   - static rank()
//   - extents() const
//
// The extents object itself only needs .extent(r).

template <std::size_t Rank>
struct simple_extents
{
    explicit simple_extents(std::size_t const (&dims)[Rank]) noexcept
    {
        for (std::size_t r = 0; r < Rank; ++r)
            dims_[r] = dims[r];
    }

    [[nodiscard]] constexpr std::size_t extent(std::size_t r) const noexcept
    {
        return dims_[r];
    }

    std::size_t dims_[Rank == 0 ? 1 : Rank]{};
};

// Specialisation for rank-0 (zero dimensions, one element).
template <>
struct simple_extents<0>
{
    [[nodiscard]] constexpr std::size_t extent(std::size_t) const noexcept
    {
        return 0;
    }
};

template <std::size_t Rank, typename IndexType = int>
struct simple_mapping
{
    using index_type = IndexType;
    using extents_type = simple_extents<Rank>;

    explicit simple_mapping(std::size_t const (&dims)[Rank == 0 ? 1 : Rank])
      : ext_(dims)
    {
    }

    [[nodiscard]] static constexpr std::size_t rank() noexcept
    {
        return Rank;
    }

    [[nodiscard]] constexpr extents_type const& extents() const noexcept
    {
        return ext_;
    }

    extents_type ext_;
};

// Rank-0 specialisation: no extent array needed.
template <typename IndexType>
struct simple_mapping<0, IndexType>
{
    using index_type = IndexType;
    using extents_type = simple_extents<0>;

    [[nodiscard]] static constexpr std::size_t rank() noexcept
    {
        return 0;
    }

    [[nodiscard]] constexpr extents_type const& extents() const noexcept
    {
        return ext_;
    }

    extents_type ext_;
};

///////////////////////////////////////////////////////////////////////////////
// Column-major (layout_left) mapping stub.
//
// Exposes stride() so that for_each_index can detect the layout and switch
// to the _left parallel path.  stride(0) == 1 and strides increase toward
// the last dimension, matching Fortran/column-major memory order.

template <std::size_t Rank, typename IndexType = int>
struct simple_mapping_left
{
    using index_type = IndexType;
    using extents_type = simple_extents<Rank>;

    explicit simple_mapping_left(
        std::size_t const (&dims)[Rank == 0 ? 1 : Rank])
      : ext_(dims)
    {
        // Compute column-major (Fortran) strides: stride[0]=1,
        // stride[r] = stride[r-1] * extent(r-1).
        strides_[0] = 1;
        for (std::size_t r = 1; r < Rank; ++r)
            strides_[r] = strides_[r - 1] * ext_.extent(r - 1);
    }

    [[nodiscard]] static constexpr std::size_t rank() noexcept
    {
        return Rank;
    }

    [[nodiscard]] constexpr extents_type const& extents() const noexcept
    {
        return ext_;
    }

    [[nodiscard]] constexpr std::size_t stride(std::size_t r) const noexcept
    {
        return strides_[r];
    }

    extents_type ext_;
    std::size_t strides_[Rank == 0 ? 1 : Rank]{};
};

// rank-0: the domain has exactly one element (the empty index tuple).
// fun() is called exactly once.
void test_rank0_seq()
{
    simple_mapping<0> m;

    int count = 0;
    hpx::experimental::for_each_index(m, [&count]() { ++count; });

    HPX_TEST_EQ(count, 1);
}

void test_rank0_par()
{
    simple_mapping<0> m;

    std::atomic<int> count{0};
    hpx::experimental::for_each_index(
        hpx::execution::par, m, [&count]() { ++count; });

    HPX_TEST_EQ(count.load(), 1);
}

// rank-1 sequential: visits every index in [0, N).
void test_rank1_seq(std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(100, 10000);
    std::size_t const N = dist(gen);
    std::size_t dims[1] = {N};
    simple_mapping<1> m(dims);

    std::vector<int> visited(N, 0);
    hpx::experimental::for_each_index(
        m, [&visited](int i) { visited[i] += 1; });

    for (std::size_t i = 0; i < N; ++i)
    {
        HPX_TEST_EQ(visited[i], 1);
    }
}

// rank-1 parallel: visits every index exactly once (atomic counter).
template <typename ExPolicy>
void test_rank1_par(ExPolicy&& policy, std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(100, 10000);
    std::size_t const N = dist(gen);
    std::size_t dims[1] = {N};
    simple_mapping<1> m(dims);

    std::atomic<int> count{0};
    hpx::experimental::for_each_index(
        HPX_FORWARD(ExPolicy, policy), m, [&count](int /*i*/) { ++count; });

    HPX_TEST_EQ(static_cast<std::size_t>(count.load()), N);
}

// rank-2 sequential: every (i, j) pair is visited exactly once.
void test_rank2_seq(std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(10, 200);
    std::size_t const R = dist(gen);
    std::size_t const C = dist(gen);
    std::size_t dims[2] = {R, C};
    simple_mapping<2> m(dims);

    // Flat bitmap: visited[i * C + j] is incremented for each (i, j).
    std::vector<int> visited(R * C, 0);
    hpx::experimental::for_each_index(
        m, [&visited, C](int i, int j) { visited[i * C + j] += 1; });

    for (std::size_t i = 0; i < R; ++i)
        for (std::size_t j = 0; j < C; ++j)
            HPX_TEST_EQ(visited[i * C + j], 1);
}

// rank-2 parallel: atomic sum equals R * C.
template <typename ExPolicy>
void test_rank2_par(ExPolicy&& policy, std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(10, 200);
    std::size_t const R = dist(gen);
    std::size_t const C = dist(gen);
    std::size_t dims[2] = {R, C};
    simple_mapping<2> m(dims);

    std::atomic<int> count{0};
    hpx::experimental::for_each_index(HPX_FORWARD(ExPolicy, policy), m,
        [&count](int /*i*/, int /*j*/) { ++count; });

    HPX_TEST_EQ(static_cast<std::size_t>(count.load()), R * C);
}

// rank-3 sequential: every (i, j, k) triple is visited exactly once.
void test_rank3_seq(std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(2, 20);
    std::size_t const D0 = dist(gen);
    std::size_t const D1 = dist(gen);
    std::size_t const D2 = dist(gen);
    std::size_t dims[3] = {D0, D1, D2};
    simple_mapping<3> m(dims);

    std::vector<int> visited(D0 * D1 * D2, 0);
    hpx::experimental::for_each_index(
        m, [&visited, D1, D2](int i, int j, int k) {
            visited[i * D1 * D2 + j * D2 + k] += 1;
        });

    for (std::size_t idx = 0; idx < D0 * D1 * D2; ++idx)
        HPX_TEST_EQ(visited[idx], 1);
}

// rank-3 parallel.
template <typename ExPolicy>
void test_rank3_par(ExPolicy&& policy, std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(2, 20);
    std::size_t const D0 = dist(gen);
    std::size_t const D1 = dist(gen);
    std::size_t const D2 = dist(gen);
    std::size_t dims[3] = {D0, D1, D2};
    simple_mapping<3> m(dims);

    std::atomic<int> count{0};
    hpx::experimental::for_each_index(HPX_FORWARD(ExPolicy, policy), m,
        [&count](int /*i*/, int /*j*/, int /*k*/) { ++count; });

    HPX_TEST_EQ(static_cast<std::size_t>(count.load()), D0 * D1 * D2);
}

// Async (task) overloads: verify future<void> is returned.
template <typename ExPolicy>
void test_rank2_async(ExPolicy&& policy, std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(10, 50);
    std::size_t const R = dist(gen);
    std::size_t const C = dist(gen);
    std::size_t dims[2] = {R, C};
    simple_mapping<2> m(dims);

    std::atomic<int> count{0};
    auto f = hpx::experimental::for_each_index(HPX_FORWARD(ExPolicy, policy), m,
        [&count](int /*i*/, int /*j*/) { ++count; });
    f.wait();

    HPX_TEST_EQ(static_cast<std::size_t>(count.load()), R * C);
}

// Empty extent: fun must NOT be called at all.
void test_empty_extent_seq(std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(1, 100);
    std::size_t dims[2] = {0, dist(gen)};
    simple_mapping<2> m(dims);

    int count = 0;
    hpx::experimental::for_each_index(
        m, [&count](int /*i*/, int /*j*/) { ++count; });

    HPX_TEST_EQ(count, 0);
}

template <typename ExPolicy>
void test_empty_extent_par(ExPolicy&& policy, std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(1, 100);
    std::size_t dims[2] = {dist(gen), 0};
    simple_mapping<2> m(dims);

    std::atomic<int> count{0};
    hpx::experimental::for_each_index(HPX_FORWARD(ExPolicy, policy), m,
        [&count](int /*i*/, int /*j*/) { ++count; });

    HPX_TEST_EQ(count.load(), 0);
}

// Result of fun is ignored (callable returns int).
void test_result_ignored(std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(2, 5);
    std::size_t dims[2] = {dist(gen), dist(gen)};
    simple_mapping<2> m(dims);

    // Should compile and run without error even though fun returns int.
    hpx::experimental::for_each_index(
        m, [](int /*i*/, int /*j*/) -> int { return 42; });

    HPX_TEST(true);    // reached here => ok
}

///////////////////////////////////////////////////////////////////////////////
// Test-runner functions (group sequential then parallel variants together).
///////////////////////////////////////////////////////////////////////////////
void for_each_index_test_seq(std::mt19937& gen)
{
    test_rank0_seq();
    test_rank1_seq(gen);
    test_rank2_seq(gen);
    test_rank3_seq(gen);
    test_empty_extent_seq(gen);
    test_result_ignored(gen);
}

void for_each_index_test_par(std::mt19937& gen)
{
    using namespace hpx::execution;

    test_rank0_par();

    test_rank1_par(par, gen);
    test_rank1_par(par_unseq, gen);

    test_rank2_par(par, gen);
    test_rank2_par(par_unseq, gen);

    test_rank3_par(par, gen);
    test_rank3_par(par_unseq, gen);

    test_empty_extent_par(par, gen);
    test_empty_extent_par(par_unseq, gen);
}

void for_each_index_test_async(std::mt19937& gen)
{
    using namespace hpx::execution;

    test_rank2_async(seq(task), gen);
    test_rank2_async(par(task), gen);
}

///////////////////////////////////////////////////////////////////////////////
// Column-major (layout_left) tests.
//
// Uses simple_mapping_left which exposes stride() with stride(0)==1.
// This triggers the layout_left parallel dispatch path.
///////////////////////////////////////////////////////////////////////////////

// rank-2 sequential, column-major: every (i, j) pair visited exactly once.
void test_rank2_left_seq(std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(10, 200);
    std::size_t const R = dist(gen);
    std::size_t const C = dist(gen);
    std::size_t dims[2] = {R, C};
    simple_mapping_left<2> m(dims);

    std::vector<int> visited(R * C, 0);
    hpx::experimental::for_each_index(
        m, [&visited, C](int i, int j) { visited[i * C + j] += 1; });

    for (std::size_t i = 0; i < R; ++i)
        for (std::size_t j = 0; j < C; ++j)
            HPX_TEST_EQ(visited[i * C + j], 1);
}

// rank-2 parallel, column-major: atomic sum equals R * C.
template <typename ExPolicy>
void test_rank2_left_par(ExPolicy&& policy, std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(10, 200);
    std::size_t const R = dist(gen);
    std::size_t const C = dist(gen);
    std::size_t dims[2] = {R, C};
    simple_mapping_left<2> m(dims);

    std::atomic<int> count{0};
    hpx::experimental::for_each_index(HPX_FORWARD(ExPolicy, policy), m,
        [&count](int /*i*/, int /*j*/) { ++count; });

    HPX_TEST_EQ(static_cast<std::size_t>(count.load()), R * C);
}

// rank-3 sequential, column-major: every (i, j, k) triple visited once.
void test_rank3_left_seq(std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(2, 20);
    std::size_t const D0 = dist(gen);
    std::size_t const D1 = dist(gen);
    std::size_t const D2 = dist(gen);
    std::size_t dims[3] = {D0, D1, D2};
    simple_mapping_left<3> m(dims);

    std::vector<int> visited(D0 * D1 * D2, 0);
    hpx::experimental::for_each_index(
        m, [&visited, D1, D2](int i, int j, int k) {
            visited[i * D1 * D2 + j * D2 + k] += 1;
        });

    for (std::size_t idx = 0; idx < D0 * D1 * D2; ++idx)
        HPX_TEST_EQ(visited[idx], 1);
}

// rank-3 parallel, column-major: atomic sum equals D0 * D1 * D2.
template <typename ExPolicy>
void test_rank3_left_par(ExPolicy&& policy, std::mt19937& gen)
{
    std::uniform_int_distribution<std::size_t> dist(2, 20);
    std::size_t const D0 = dist(gen);
    std::size_t const D1 = dist(gen);
    std::size_t const D2 = dist(gen);
    std::size_t dims[3] = {D0, D1, D2};
    simple_mapping_left<3> m(dims);

    std::atomic<int> count{0};
    hpx::experimental::for_each_index(HPX_FORWARD(ExPolicy, policy), m,
        [&count](int /*i*/, int /*j*/, int /*k*/) { ++count; });

    HPX_TEST_EQ(static_cast<std::size_t>(count.load()), D0 * D1 * D2);
}

void for_each_index_test_left(std::mt19937& gen)
{
    using namespace hpx::execution;

    test_rank2_left_seq(gen);
    test_rank3_left_seq(gen);

    test_rank2_left_par(par, gen);
    test_rank2_left_par(par_unseq, gen);

    test_rank3_left_par(par, gen);
    test_rank3_left_par(par_unseq, gen);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::mt19937 gen(seed);

    for_each_index_test_seq(gen);
    for_each_index_test_par(gen);
    for_each_index_test_async(gen);
    for_each_index_test_left(gen);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // Run on all available cores.
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
