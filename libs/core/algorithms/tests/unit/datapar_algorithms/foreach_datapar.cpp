//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datapar.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "../algorithms/foreach_tests.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_for_each()
{
    using namespace hpx::execution;

    test_for_each(simd, IteratorTag());
    test_for_each(par_simd, IteratorTag());

    test_for_each_async(simd(task), IteratorTag());
    test_for_each_async(par_simd(task), IteratorTag());

#if defined(HPX_HAVE_DATAPAR_EMULATION)
    test_for_each(fixed_size_simd<1>, IteratorTag());
    test_for_each(par_fixed_size_simd<1>, IteratorTag());
    test_for_each(fixed_size_simd<2>, IteratorTag());
    test_for_each(par_fixed_size_simd<2>, IteratorTag());

    test_for_each(fixed_size_simd<3>, IteratorTag());
    test_for_each(par_fixed_size_simd<6>, IteratorTag());

    test_for_each_async(fixed_size_simd<1>(task), IteratorTag());
    test_for_each_async(par_fixed_size_simd<1>(task), IteratorTag());
    test_for_each_async(fixed_size_simd<2>(task), IteratorTag());
    test_for_each_async(par_fixed_size_simd<2>(task), IteratorTag());
#endif
}

void for_each_test()
{
    using hpx::datapar::experimental::fixed_size_simd;
    using hpx::parallel::traits::vector_pack_alignment_v;

#if !defined(HPX_HAVE_DATAPAR_SVE)
    static_assert(
        vector_pack_alignment_v<fixed_size_simd<int, 4>> == 4 * sizeof(int));
#endif
#if defined(HPX_HAVE_DATAPAR_EMULATION)
    static_assert(
        vector_pack_alignment_v<fixed_size_simd<int, 3>> == 4 * sizeof(int));
    static_assert(
        vector_pack_alignment_v<fixed_size_simd<int, 6>> == 8 * sizeof(int));
#endif

    test_for_each<std::random_access_iterator_tag>();
    test_for_each<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.contains("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    for_each_test();
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

    // By default, this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
