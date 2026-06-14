//  Copyright (c) 2025 Hartmut Kaiser
//  Copyright (c) 2017 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/chrono.hpp>
#include <hpx/format.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/itt_notify.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
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

    // actual measurement
    std::uint64_t time = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i < test_count; ++i)
    {
        hpx::merge(policy, first1, last1, first2, last2, dest);
    }

    time = hpx::chrono::high_resolution_clock::now() - time;

    return (static_cast<double>(time) * 1e-9) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
using picoseconds = std::chrono::duration<long long, std::pico>;

struct adaptive_chunk_size
{
    template <typename Rep1, typename Period1, typename Rep2, typename Period2>
    explicit constexpr adaptive_chunk_size(
        std::chrono::duration<Rep1, Period1> const time_per_iteration,
        std::chrono::duration<Rep2, Period2> const overhead_time =
            std::chrono::microseconds(1))
      : time_per_iteration_(
            std::chrono::duration_cast<picoseconds>(time_per_iteration))
      , overhead_time_(std::chrono::duration_cast<picoseconds>(overhead_time))
    {
    }

    // calculate number of cores
    template <typename Executor>
    std::size_t processing_units_count(Executor&& exec,
        hpx::chrono::steady_duration const&, std::size_t count) const noexcept
    {
        std::size_t const cores_baseline =
            hpx::execution::experimental::processing_units_count(
                exec, time_per_iteration_, count);

        auto const overall_time =
            static_cast<double>((count + 1) * time_per_iteration_.count());

        constexpr double efficiency_factor = 0.052;    // see paper: 1 / 19
        auto const optimal_num_cores =
            static_cast<std::size_t>(efficiency_factor * overall_time /
                static_cast<double>(overhead_time_.count()));

        std::size_t num_cores = (std::min) (cores_baseline, optimal_num_cores);
        num_cores = (std::max) (num_cores, static_cast<std::size_t>(1));

        return num_cores;
    }

    template <typename Executor>
    std::size_t get_chunk_size(Executor&&, hpx::chrono::steady_duration const&,
        std::size_t const cores, std::size_t const num_iterations) const
    {
        if (cores == 1)
        {
            return num_iterations;
        }

        std::size_t times_cores = 8;
        if (cores == 2)
        {
            times_cores = 4;
        }

        // Return a chunk size that ensures that each core ends up with the same
        // number of chunks the sizes of which are equal (except for the last
        // chunk, which may be smaller by not more than the number of chunks in
        // terms of elements).
        std::size_t const num_chunks = times_cores * cores;
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

    picoseconds time_per_iteration_;
    picoseconds overhead_time_;
};

template <>
struct hpx::execution::experimental::is_executor_parameters<adaptive_chunk_size>
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

struct force_sequential_execution
{
    force_sequential_execution() = default;

    template <typename Executor>
    constexpr std::size_t maximal_number_of_chunks(
        Executor&&, std::size_t const, std::size_t const) const noexcept
    {
        return 1;
    }

    template <typename Executor>
    constexpr std::size_t processing_units_count(Executor&&,
        hpx::chrono::steady_duration const&, std::size_t const) const noexcept
    {
        return 1;
    }
};

template <>
struct hpx::execution::experimental::is_executor_parameters<
    force_sequential_execution> : std::true_type
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

using data_type = char;

constexpr std::size_t max_chunks = 64;

template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
    typename FwdIter3>
void run_merge_benchmark_sweep(std::string label, int const test_count,
    ExPolicy policy, FwdIter1 first1, FwdIter1 last1, FwdIter2 first2,
    FwdIter2 last2, FwdIter3 dest)
{
    std::size_t const all_cores = hpx::get_num_worker_threads();

    std::vector<std::size_t> num_cores;
    {
        for (std::size_t cores = 1; cores < all_cores; cores *= 2)
        {
            num_cores.push_back(cores);
        }
        num_cores.push_back(all_cores);
    }

    // measure sequential execution when forcing one core and one chunk
    hpx::execution::experimental::chunking_parameters force_sequential_params;
    hpx::execution::experimental::collect_chunking_parameters
        collect_single_core_params(force_sequential_params);

    force_sequential_execution fse;
    double const force_sequential_time = run_merge_benchmark_hpx(test_count,
        policy.with(fse, collect_single_core_params), first1, last1, first2,
        last2, dest);

    double overhead_time = 0.0;
    double const time_per_iteration = force_sequential_time /
        static_cast<double>((std::max) (std::distance(first1, last1),
            std::distance(first2, last2)));

    bool first_line = true;
    for (auto const cores : num_cores)
    {
        hpx::execution::experimental::num_cores nc(cores);

        std::map<std::size_t,
            std::pair<double,
                hpx::execution::experimental::chunking_parameters>>
            chunking_params;

        for (std::size_t chunks = cores; chunks < max_chunks * 2; chunks *= 2)
        {
            hpx::execution::experimental::chunking_parameters params = {};
            hpx::execution::experimental::collect_chunking_parameters
                collect_params(params);

            hpx::execution::experimental::max_num_chunks mnc(chunks);
            double const time = run_merge_benchmark_hpx(test_count,
                policy.with(nc, mnc, collect_params), first1, last1, first2,
                last2, dest);

            chunking_params.emplace(chunks, std::make_pair(time, params));
        }

        // calculate overhead time as the difference between the sequential time
        // and the parallel time with one core and one chunk
        if (cores == 1)
        {
            overhead_time = (chunking_params[1].first - force_sequential_time) /
                static_cast<double>(all_cores);
        }

        // now run with adaptive_chunk_size to compare against sweep results
        hpx::execution::experimental::chunking_parameters adaptive_params = {};
        double adaptive_time;

        {
            hpx::execution::experimental::collect_chunking_parameters
                collect_params(adaptive_params);

            picoseconds time_per_iteration_ps(
                static_cast<std::int64_t>(time_per_iteration * 1e12));
            picoseconds overhead_time_ps(
                static_cast<std::int64_t>(overhead_time * 1e12));

            adaptive_chunk_size acs(time_per_iteration_ps, overhead_time_ps);

            adaptive_time = run_merge_benchmark_hpx(test_count,
                policy.with(acs, collect_params), first1, last1, first2, last2,
                dest);
        }

        if (first_line)
        {
            first_line = false;

            std::cout << "iteration time:      " << time_per_iteration << "\n";
            std::cout << "overhead:            " << overhead_time << "\n";
            std::cout << "adaptive chunk size: " << adaptive_params.chunk_size
                      << ", chunks: " << adaptive_params.num_chunks
                      << ", cores: " << adaptive_params.num_cores
                      << ", time: " << adaptive_time << "\n";

            std::cout << label << "\t";
            for (std::size_t chunks = 1; chunks != max_chunks * 2; chunks *= 2)
            {
                if (auto it = chunking_params.find(chunks);
                    it != chunking_params.end())
                {
                    std::size_t const chunk_size =
                        sizeof(typename std::iterator_traits<
                            FwdIter1>::value_type) *
                        it->second.second.chunk_size;

                    hpx::util::format_to(std::cout, "{1:8}{2}", chunk_size,
                        chunks != max_chunks ? "\t" : "\n");
                }
            }
        }

        std::cout << cores << "\t";
        std::size_t const log_cores =
            static_cast<std::size_t>(std::log2(cores));
        for (std::size_t chunks = 1; chunks <= log_cores; ++chunks)
        {
            std::cout << "       -\t";
        }

        for (std::size_t chunks = cores; chunks < max_chunks * 2; chunks *= 2)
        {
            if (auto it = chunking_params.find(chunks);
                it != chunking_params.end())
            {
                hpx::util::format_to(std::cout, "{1}{2}", it->second.first,
                    chunks != max_chunks ? "\t" : "\n");
            }
        }
    }
    std::cout << "\n";
}

///////////////////////////////////////////////////////////////////////////////
template <typename Policy, typename IteratorTag, typename Allocator>
void run_benchmark(Policy policy, std::size_t vector_size1,
    std::size_t vector_size2, int test_count, IteratorTag,
    Allocator const& alloc, std::string const& type, int entropy)
{
    std::cout << "* Preparing Benchmark Data... (" << type << ")\n";

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

    std::cout << "* Running Benchmark Sweep... (" << type << ")\n";

    std::cout << "--- run_merge_benchmark_std ---\n";
    double const time_std =
        run_merge_benchmark_std(test_count, first1, last1, first2, last2, dest);

    hpx::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "--- run_merge_benchmark_seq ---\n";
    double const time_seq = run_merge_benchmark_hpx(
        test_count, seq, first1, last1, first2, last2, dest);

    std::cout << "\n-------------- Benchmark Result --------------\n";
    auto const fmt = "merge ({1}) : {2}(sec)\n";
    hpx::util::format_to(std::cout, fmt, "std", time_std);
    hpx::util::format_to(std::cout, fmt, "seq", time_seq);

    hpx::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "--- run_merge_benchmark_sweep_par ---\n";
    policy = hpx::execution::experimental::with_priority(
        policy, hpx::threads::thread_priority::initially_bound);

    run_merge_benchmark_sweep(
        "par", test_count, policy, first1, last1, first2, last2, dest);

    hpx::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "--- run_merge_benchmark_par_sweep_fork_join ---\n";
    {
        hpx::execution::experimental::fork_join_executor exec;
        run_merge_benchmark_sweep("fj_par", test_count, par.on(exec), first1,
            last1, first2, last2, dest);
    }

    std::cout << "----------------------------------------------\n";
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    HPX_ITT_PAUSE();

    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    // pull values from cmd
    std::size_t const vector_size = vm["vector_size"].as<std::size_t>();
    double const vector_ratio = vm["vector_ratio"].as<double>();
    int const test_count = vm["test_count"].as<int>();
    int const entropy = vm["entropy"].as<int>();

    std::size_t const os_threads = hpx::get_os_thread_count();

    std::size_t const vector_size1 = static_cast<std::size_t>(
        static_cast<double>(vector_size) * vector_ratio);
    std::size_t const vector_size2 = vector_size - vector_size1;

    std::cout << "-------------- Benchmark Config --------------\n";
    std::cout << "seed         : " << seed << "\n";
    std::cout << "entropy      : " << entropy << "\n";
    std::cout << "vector_size1 : " << vector_size1 << "\n";
    std::cout << "vector_size2 : " << vector_size2 << "\n";
    std::cout << "test_count   : " << test_count << "\n";
    std::cout << "os threads   : " << os_threads << "\n";
    std::cout << "----------------------------------------------\n\n";

    {
        using allocator_type = std::allocator<data_type>;
        allocator_type alloc;

        run_benchmark(hpx::execution::par, vector_size1, vector_size2,
            test_count, std::random_access_iterator_tag(), alloc, "std::vector",
            entropy);

        auto const stackless_policy =
            hpx::execution::experimental::with_stacksize(
                hpx::execution::par, hpx::threads::thread_stacksize::nostack);

        run_benchmark(stackless_policy, vector_size1, vector_size2, test_count,
            std::random_access_iterator_tag(), alloc, "std::vector (stackless)",
            entropy);

        enable_fast_idle_mode efim;
        run_benchmark(stackless_policy.with(efim), vector_size1, vector_size2,
            test_count, std::random_access_iterator_tag(), alloc,
            "std::vector (stackless, fast-idle mode)", entropy);
    }

    {
        auto policy = hpx::execution::par;
        using allocator_type =
            hpx::compute::host::detail::policy_allocator<data_type,
                decltype(policy)>;
        allocator_type const alloc(policy);

        run_benchmark(policy, vector_size1, vector_size2, test_count,
            std::random_access_iterator_tag(), alloc, "hpx::compute::vector",
            entropy);

        auto const stackless_policy =
            hpx::execution::experimental::with_stacksize(
                policy, hpx::threads::thread_stacksize::nostack);

        run_benchmark(stackless_policy, vector_size1, vector_size2, test_count,
            std::random_access_iterator_tag(), alloc,
            "hpx::compute::vector (stackless)", entropy);

        enable_fast_idle_mode efim;
        run_benchmark(stackless_policy.with(efim), vector_size1, vector_size2,
            test_count, std::random_access_iterator_tag(), alloc,
            "hpx::compute::vector (stackless, fast-idle mode)", entropy);
    }

    return hpx::local::finalize();
}

int main(int const argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    constexpr std::size_t vector_size = 268435;

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

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
