///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2017 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/chrono.hpp>
#include <hpx/format.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/schedulers.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include "utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();

///////////////////////////////////////////////////////////////////////////////
template <typename InIter1, typename InIter2, typename OutIter>
double run_merge_benchmark_std(int const test_count, InIter1 first1,
    InIter1 last1, InIter2 first2, InIter2 last2, OutIter dest)
{
    // warmup
    std::merge(first1, last1, first2, last2, dest);

    // actual measurement
    std::uint64_t time = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i < test_count; ++i)
    {
        std::merge(first1, last1, first2, last2, dest);
    }

    time = hpx::chrono::high_resolution_clock::now() - time;

    return (static_cast<double>(time) * 1e-9) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
    typename FwdIter3>
double run_merge_benchmark_hpx(int const test_count, ExPolicy policy,
    FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter2 last2,
    FwdIter3 dest)
{
    // warmup
    hpx::merge(policy, first1, last1, first2, last2, dest);

#if HPX_HAVE_ITTNOTIFY != 0 && !defined(HPX_HAVE_APEX)
    auto local_policy = hpx::execution::experimental::with_annotation(
        policy, "run_merge_benchmark_hpx (child)");
#else
    auto local_policy = policy;
#endif

    // actual measurement
    std::uint64_t time = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i < test_count; ++i)
    {
        hpx::merge(local_policy, first1, last1, first2, last2, dest);
    }

    time = hpx::chrono::high_resolution_clock::now() - time;

    return (static_cast<double>(time) * 1e-9) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
struct compute_chunk_size
{
    explicit constexpr compute_chunk_size(std::size_t times_cores = 4)
      : times_cores_(times_cores)
    {
    }

    //template <typename Executor>
    //friend std::size_t tag_override_invoke(
    //    hpx::execution::experimental::maximal_number_of_chunks_t,
    //    compute_chunk_size& this_, Executor&, std::size_t const cores,
    //    std::size_t const)
    //{
    //    return this_.times_cores_ * cores;
    //}

    template <typename Executor>
    std::size_t get_chunk_size(Executor&&, hpx::chrono::steady_duration const&,
        std::size_t const cores, std::size_t const num_iterations) const
    {
        if (cores == 1)
        {
            return num_iterations;
        }

        // Return a chunk size that ensures that each core ends up with the same
        // number of chunks the sizes of which are equal (except for the last
        // chunk, which may be smaller by not more than the number of chunks in
        // terms of elements).
        std::size_t const num_chunks = times_cores_ * cores;
        std::size_t chunk_size = (num_iterations + num_chunks - 1) / num_chunks;

        // we should not consider more chunks than we have elements
        auto const max_chunks = (std::min) (num_chunks, num_iterations);

        // we should not make chunks smaller than what's determined by the max
        // chunk size
        chunk_size = (std::max) (chunk_size,
            (num_iterations + max_chunks - 1) / max_chunks);

        HPX_ASSERT(chunk_size * num_chunks >= num_iterations);

        return chunk_size;
    }

    std::size_t times_cores_;
};

template <>
struct hpx::execution::experimental::is_executor_parameters<compute_chunk_size>
  : std::true_type
{
};

struct enable_fast_idle_mode
{
    template <typename Executor>
    void mark_begin_execution(Executor&& exec)
    {
        auto const pu_mask =
            hpx::execution::experimental::get_processing_units_mask(exec);
        auto const full_pu_mask =
            hpx::resource::get_partitioner().get_used_pus_mask();

        // Enable fast-idle mode only for PU's that are not used by this
        // algorithm invocation.
        hpx::threads::add_remove_scheduler_mode(
            hpx::threads::policies::scheduler_mode::fast_idle_mode,
            hpx::threads::policies::scheduler_mode::enable_stealing |
                hpx::threads::policies::scheduler_mode::enable_stealing_numa,
            full_pu_mask & ~pu_mask);
    }

    template <typename Executor>
    void mark_end_execution(Executor&& exec)
    {
        auto const pu_mask =
            hpx::execution::experimental::get_processing_units_mask(exec);
        auto const full_pu_mask =
            hpx::resource::get_partitioner().get_used_pus_mask();

        hpx::threads::add_remove_scheduler_mode(
            hpx::threads::policies::scheduler_mode::enable_stealing |
                hpx::threads::policies::scheduler_mode::enable_stealing_numa,
            hpx::threads::policies::scheduler_mode::fast_idle_mode,
            full_pu_mask & ~pu_mask);
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    enable_fast_idle_mode> : std::true_type
{
};

///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct random_to_item_t
{
    double m_min;
    double m_max;

    random_to_item_t(T min, T max)
      : m_min(static_cast<double>(min))
      , m_max(static_cast<double>(max))
    {
    }

    T operator()(double random_value) const
    {
        if constexpr (std::is_floating_point_v<T>)
        {
            return static_cast<T>((m_max - m_min) * random_value + m_min);
        }
        else
        {
            return static_cast<T>(
                std::floor((m_max - m_min + 1) * random_value + m_min));
        }
    }
};

using data_type = int;

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename Allocator>
void run_benchmark(std::size_t vector_size1, std::size_t vector_size2,
    int test_count, IteratorTag, Allocator const& alloc,
    std::string const& type, int entropy, int num_chunks)
{
    std::cout << "* Preparing Benchmark... (" << type << ")" << std::endl;

    // initialize data
    using namespace hpx::execution;

    std::vector<double> uniform_distribution(vector_size1 + vector_size2);

    std::default_random_engine re(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    hpx::generate(par, std::begin(uniform_distribution),
        std::end(uniform_distribution), [&] { return dist(re); });

    using test_container = test_container<IteratorTag, data_type, Allocator>;
    using container = typename test_container::type;
    using T = typename container::value_type;

    container src =
        test_container::get_container(vector_size1 + vector_size2, alloc);

    T const min = (std::numeric_limits<T>::min)();
    T const max = (std::numeric_limits<T>::max)();

    hpx::transform(par, std::begin(uniform_distribution),
        std::end(uniform_distribution), src.data(),
        random_to_item_t<T>(min, max));

    // entropy: 0 -> 1, 4 -> 0.201
    for (int i = 0; i < entropy; ++i, ++seed)
    {
        hpx::generate(par, std::begin(uniform_distribution),
            std::end(uniform_distribution), [&] { return dist(re); });

        container tmp_vec(src.size(), alloc);
        hpx::transform(par, std::begin(uniform_distribution),
            std::end(uniform_distribution), tmp_vec.data(),
            random_to_item_t<T>(min, max));

        hpx::transform(par, src.data(), src.data() + src.size(), tmp_vec.data(),
            src.data(), std::bit_and{});
    }

    hpx::sort(par, src.begin(), src.begin() + vector_size1);
    hpx::sort(par, src.begin() + vector_size1, src.end());

    auto first1 = std::begin(src);
    auto last1 = std::begin(src) + vector_size1;
    auto first2 = std::begin(src) + vector_size1;
    auto last2 = std::end(src);

    container result =
        test_container::get_container(vector_size1 + vector_size2, alloc);

    auto dest = std::begin(result);

    std::cout << "* Running Benchmark... (" << type << ")" << std::endl;
    std::cout << "--- run_merge_benchmark_std ---" << std::endl;

    hpx::util::perftests_report("hpx::merge", "seq", test_count,
        [&] { hpx::merge(seq, first1, last1, first2, last2, dest); });

    hpx::util::perftests_report("hpx::merge", "par", test_count,
        [&] { hpx::merge(par, first1, last1, first2, last2, dest); });

    hpx::util::perftests_report("hpx::merge", "par_unseq", test_count,
        [&] { hpx::merge(par_unseq, first1, last1, first2, last2, dest); });

    hpx::util::perftests_print_times();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    HPX_TRACING_PAUSE();

    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    // pull values from cmd
    std::size_t const vector_size = vm["vector_size"].as<std::size_t>();
    double const vector_ratio = vm["vector_ratio"].as<double>();
    int const test_count = vm["test_count"].as<int>();
    int const entropy = vm["entropy"].as<int>();
    int const num_chunks = vm["num_chunks"].as<int>();

    std::size_t const os_threads = hpx::get_os_thread_count();
    HPX_UNUSED(os_threads);

    std::size_t const vector_size1 = static_cast<std::size_t>(
        static_cast<double>(vector_size) * vector_ratio);
    std::size_t const vector_size2 = vector_size - vector_size1;

    // std::cout << "-------------- Benchmark Config --------------" << std::endl;
    // std::cout << "seed         : " << seed << std::endl;
    // std::cout << "vector_size1 : " << vector_size1 << std::endl;
    // std::cout << "vector_size2 : " << vector_size2 << std::endl;
    // std::cout << "random_range : " << random_range << std::endl;
    // std::cout << "iterator_tag : " << iterator_tag_str << std::endl;
    // std::cout << "test_count   : " << test_count << std::endl;
    // std::cout << "os threads   : " << os_threads << std::endl;
    // std::cout << "----------------------------------------------\n"
    //           << std::endl;

    hpx::util::perftests_init(vm, "benchmark_merge");

    if (iterator_tag_str == "random")
        run_benchmark(vector_size1, vector_size2, test_count, random_range,
            std::random_access_iterator_tag());
    //else if (iterator_tag_str == "bidirectional")
    //    run_benchmark(vector_size1, vector_size2, test_count, random_range,
    //        std::bidirectional_iterator_tag());
    //else // forward
    //    run_benchmark(vector_size1, vector_size2, test_count, random_range,
    //        std::forward_iterator_tag());

    //     run_benchmark(vector_size1, vector_size2, test_count,
    //         std::random_access_iterator_tag(), alloc, "std::vector", entropy,
    //         num_chunks);
    // }

    // {
    //     auto policy = hpx::execution::par;
    //     using allocator_type =
    //         hpx::compute::host::detail::policy_allocator<data_type,
    //             decltype(policy)>;
    //     allocator_type alloc(policy);
    // 
    //     run_benchmark(vector_size1, vector_size2, test_count,
    //         std::random_access_iterator_tag(), alloc, "hpx::compute::vector",
    //         entropy, num_chunks);
    // }

    return hpx::local::finalize();
}

int main(int const argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

#if defined(HPX_DEBUG)
    constexpr std::size_t vector_size = 268435;
#else
    constexpr std::size_t vector_size = 268435456;
#endif

    std::string const vector_size_help =
        "sum of sizes of two vectors (default: " + std::to_string(vector_size) +
        ")";

    // clang-format off
    desc_commandline.add_options()
        ("vector_size", value<std::size_t>()->default_value(vector_size),
         vector_size_help.c_str())
        ("vector_ratio", value<double>()->default_value(0.7),
         "ratio of two vector sizes (default: 0.7)")
        ("entropy", value<int>()->default_value(1),
         "entropy value: 0 -> 1, 4 -> 0.201 (default: 0)")
        ("num_chunks", value<int>()->default_value(8),
         "number of chunks (times number of cores) (default: 8)")
        ("test_count", value<int>()->default_value(10),
         "number of tests to be averaged (default: 10)")
        ("seed,s", value<unsigned int>(),
         "the random number generator seed to use for this run")
    ;
    // clang-format on

    // initialize program
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::util::perftests_cfg(desc_commandline);

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
