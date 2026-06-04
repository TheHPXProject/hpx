///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2017 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/format.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "utils.hpp"

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();

///////////////////////////////////////////////////////////////////////////////
struct random_fill
{
    random_fill(std::size_t random_range)
      : gen(seed)
      , dist(0, static_cast<int>(random_range - 1))
    {
    }

    int operator()()
    {
        return dist(gen);
    }

    std::mt19937 gen;
    std::uniform_int_distribution<> dist;
};

///////////////////////////////////////////////////////////////////////////////
struct vector_type
{
    vector_type() = default;
    vector_type(int rand_no)
    {
        vec_.reserve(vec_size_);
        for (std::size_t i = 0u; i < vec_size_; ++i)
            vec_.push_back(rand_no);
    }

    bool operator==(vector_type const& t) const
    {
        return vec_ == t.vec_;
    }

    std::vector<int> vec_;
    static std::size_t const vec_size_{30};
};

struct array_type
{
    array_type() = default;
    array_type(int rand_no)
    {
        for (std::size_t i = 0u; i < arr_size_; ++i)
            arr_[i] = rand_no;
    }

    bool operator==(array_type const& t) const
    {
        return arr_ == t.arr_;
    }

    static std::size_t const arr_size_{30};
    std::array<int, arr_size_> arr_;
};

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag, typename DataType>
void run_benchmark(std::size_t vector_size, int test_count,
    std::size_t random_range, IteratorTag, DataType)
{
    typedef test_container<IteratorTag, DataType> test_container;
    typedef typename test_container::type container;

    container v = test_container::get_container(vector_size);
    container org_v;

    auto first = std::begin(v);
    auto last = std::end(v);

    // initialize data
    using namespace hpx::execution;
    hpx::generate(par, std::begin(v), std::end(v), random_fill(random_range));
    org_v = v;

    auto value = DataType(static_cast<int>(random_range / 2));
    auto pred = [value](DataType const& a) -> bool { return a == value; };

    // auto dest_dist = std::distance(first, std::remove_if(first, last, pred));

    auto org_first = std::begin(org_v);
    auto org_last = std::end(org_v);

    hpx::util::perftests_report("hpx::remove_if", "seq", test_count, [&] {
        // Restore [first, last) with original data.
        hpx::copy(hpx::execution::par, org_first, org_last, first);

        hpx::remove_if(seq, first, last, pred);
    });

    hpx::util::perftests_report("hpx::remove_if", "par", test_count, [&] {
        // Restore [first, last) with original data.
        hpx::copy(hpx::execution::par, org_first, org_last, first);

        hpx::remove_if(par, first, last, pred);
    });

    hpx::util::perftests_report("hpx::remove_if", "par_unseq", test_count, [&] {
        // Restore [first, last) with original data.
        hpx::copy(hpx::execution::par, org_first, org_last, first);

        hpx::remove_if(par_unseq, first, last, pred);
    });

    hpx::util::perftests_print_times();
}

template <typename IteratorTag>
void run_benchmark(std::size_t vector_size, int test_count,
    std::size_t random_range, IteratorTag iterator_tag,
    std::string const& data_type_str)
{
    if (data_type_str == "int")
        run_benchmark(
            vector_size, test_count, random_range, iterator_tag, int());
    else if (data_type_str == "vector")
        run_benchmark(
            vector_size, test_count, random_range, iterator_tag, vector_type());
    else    // array
        run_benchmark(
            vector_size, test_count, random_range, iterator_tag, array_type());
}

void run_benchmark(std::size_t vector_size, int test_count,
    std::size_t random_range, std::string const& iterator_tag_str,
    std::string const& data_type_str)
{
    if (iterator_tag_str == "random")
        run_benchmark(vector_size, test_count, random_range,
            std::random_access_iterator_tag(), data_type_str);
    else if (iterator_tag_str == "bidirectional")
        run_benchmark(vector_size, test_count, random_range,
            std::bidirectional_iterator_tag(), data_type_str);
    else    // forward
        run_benchmark(vector_size, test_count, random_range,
            std::forward_iterator_tag(), data_type_str);
}

///////////////////////////////////////////////////////////////////////////////
std::string correct_iterator_tag_str(std::string const& iterator_tag)
{
    if (iterator_tag != "random" && iterator_tag != "bidirectional" &&
        iterator_tag != "forward")
        return "random";
    else
        return iterator_tag;
}

std::string correct_data_type_str(std::string const& data_type)
{
    if (data_type != "int" && data_type != "vector" && data_type != "array")
        return "int";
    else
        return data_type;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    // pull values from cmd
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    std::size_t random_range = vm["random_range"].as<std::size_t>();
    int test_count = vm["test_count"].as<int>();
    std::string iterator_tag_str =
        correct_iterator_tag_str(vm["iterator_tag"].as<std::string>());
    std::string data_type_str =
        correct_data_type_str(vm["data_type"].as<std::string>());

    [[maybe_unused]] std::size_t const os_threads = hpx::get_os_thread_count();

    hpx::util::perftests_init(vm, "benchmark_remove_if");

    if (random_range < 1)
        random_range = 1;

    run_benchmark(
        vector_size, test_count, random_range, iterator_tag_str, data_type_str);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("vector_size",
        hpx::program_options::value<std::size_t>()->default_value(1000000),
        "size of vector (default: 1000000)")("random_range",
        hpx::program_options::value<std::size_t>()->default_value(6),
        "range of random numbers [0, x) (default: 6)")("iterator_tag",
        hpx::program_options::value<std::string>()->default_value("random"),
        "the kind of iterator tag (random/bidirectional/forward)")("data_type",
        hpx::program_options::value<std::string>()->default_value("int"),
        "the kind of data type (int/vector/array)")("test_count",
        hpx::program_options::value<int>()->default_value(100),
        "number of tests to be averaged (default: 100)")("seed,s",
        hpx::program_options::value<unsigned int>(),
        "the random number generator seed to use for this run");

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
