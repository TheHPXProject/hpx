//  Copyright (c) 2007-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "sheneos/interpolator.hpp"

#include <hpx/modules/program_options.hpp>

char const* const shen_symbolic_name = "/sheneos/interpolator_test";

///////////////////////////////////////////////////////////////////////////////
/// This is the test function. It will be invoked on all localities that the
/// benchmark is being run on.
void test_sheneos(std::size_t num_ye_points, std::size_t num_temp_points,
    std::size_t num_rho_points, std::size_t seed)
{
    // Create a client instance connected to the already existing interpolation
    // object.
    sheneos::interpolator shen;
    shen.connect(shen_symbolic_name);

    ///////////////////////////////////////////////////////////////////////////
    // Compute the minimum, maximum and delta values for each dimension.
    double min_ye = 0, max_ye = 0;
    shen.get_dimension(sheneos::dimension::ye, min_ye, max_ye);
    double const delta_ye = (max_ye - min_ye) / num_ye_points;

    double min_temp = 0, max_temp = 0;
    shen.get_dimension(sheneos::dimension::temp, min_temp, max_temp);
    double const delta_temp = (max_temp - min_temp) / num_temp_points;

    double min_rho = 0, max_rho = 0;
    shen.get_dimension(sheneos::dimension::rho, min_rho, max_rho);
    double const delta_rho = (max_rho - min_rho) / num_rho_points;

    ///////////////////////////////////////////////////////////////////////////
    // Generate the data points, spacing them out equally.
    std::vector<double> values_ye(num_ye_points);
    std::vector<std::size_t> sequence_ye(num_ye_points);
    double ye = min_ye;
    for (std::size_t i = 0; i < num_ye_points; ++i)
    {
        values_ye[i] = ye;
        sequence_ye[i] = i;
        ye += delta_ye;
    }

    std::vector<double> values_temp(num_temp_points);
    std::vector<std::size_t> sequence_temp(num_temp_points);
    double temp = min_temp;
    for (std::size_t i = 0; i < num_temp_points; ++i)
    {
        values_temp[i] = temp;
        sequence_temp[i] = i;
        temp += delta_temp;
    }

    std::vector<double> values_rho(num_rho_points);
    std::vector<std::size_t> sequence_rho(num_rho_points);
    double rho = min_rho;
    for (std::size_t i = 0; i < num_rho_points; ++i)
    {
        values_rho[i] = rho;
        sequence_rho[i] = i;
        rho += delta_rho;
    }

    ///////////////////////////////////////////////////////////////////////////
    // We want to avoid invoking the same evaluation sequence on all localities
    // performing the test, so we randomly shuffle the sequences. We combine
    // the shared seed with the locality id to ensure that each locality has
    // a unique, reproducible seed.
    std::srand(static_cast<unsigned int>(seed + hpx::get_locality_id()));
    std::random_shuffle(sequence_ye.begin(), sequence_ye.end());
    std::random_shuffle(sequence_temp.begin(), sequence_temp.end());
    std::random_shuffle(sequence_rho.begin(), sequence_rho.end());

    // Create the three-dimensional future grid.
    std::vector<hpx::future<std::vector<double>>> tests;
    for (std::size_t const& ii : sequence_ye)
    {
        for (std::size_t const& jj : sequence_temp)
        {
            for (std::size_t const& kk : sequence_rho)
            {
                tests.push_back(shen.interpolate_async(
                    values_ye[ii], values_temp[jj], values_rho[kk]));
            }
        }
    }

    hpx::wait_all(tests);
}

HPX_DECLARE_ACTION(test_sheneos, test_action);
HPX_ACTION_USES_MEDIUM_STACK(test_action);
HPX_PLAIN_ACTION(test_sheneos, test_action)

///////////////////////////////////////////////////////////////////////////////
/// This is the test function. It will be invoked on all localities that the
/// benchmark is being run on.
void test_sheneos_one_bulk(std::size_t num_ye_points,
    std::size_t num_temp_points, std::size_t num_rho_points, std::size_t seed)
{
    // Create a client instance connected to the already existing interpolation
    // object.
    sheneos::interpolator shen;
    shen.connect(shen_symbolic_name);

    ///////////////////////////////////////////////////////////////////////////
    // Compute the minimum, maximum and delta values for each dimension.
    double min_ye = 0, max_ye = 0;
    shen.get_dimension(sheneos::dimension::ye, min_ye, max_ye);
    double const delta_ye = (max_ye - min_ye) / num_ye_points;

    double min_temp = 0, max_temp = 0;
    shen.get_dimension(sheneos::dimension::temp, min_temp, max_temp);
    double const delta_temp = (max_temp - min_temp) / num_temp_points;

    double min_rho = 0, max_rho = 0;
    shen.get_dimension(sheneos::dimension::rho, min_rho, max_rho);
    double const delta_rho = (max_rho - min_rho) / num_rho_points;

    ///////////////////////////////////////////////////////////////////////////
    // Generate the data points, spacing them out equally.
    std::vector<double> values_ye(num_ye_points);
    std::vector<std::size_t> sequence_ye(num_ye_points);
    double ye = min_ye;
    for (std::size_t i = 0; i < num_ye_points; ++i)
    {
        values_ye[i] = ye;
        sequence_ye[i] = i;
        ye += delta_ye;
    }

    std::vector<double> values_temp(num_temp_points);
    std::vector<std::size_t> sequence_temp(num_temp_points);
    double temp = min_temp;
    for (std::size_t i = 0; i < num_temp_points; ++i)
    {
        values_temp[i] = temp;
        sequence_temp[i] = i;
        temp += delta_temp;
    }

    std::vector<double> values_rho(num_rho_points);
    std::vector<std::size_t> sequence_rho(num_rho_points);
    double rho = min_rho;
    for (std::size_t i = 0; i < num_rho_points; ++i)
    {
        values_rho[i] = rho;
        sequence_rho[i] = i;
        rho += delta_rho;
    }

    ///////////////////////////////////////////////////////////////////////////
    // We want to avoid invoking the same evaluation sequence on all localities
    // performing the test, so we randomly shuffle the sequences. We combine
    // the shared seed with the locality id to ensure that each locality has
    // a unique, reproducible seed.
    std::srand(static_cast<unsigned int>(seed + hpx::get_locality_id()));
    std::random_shuffle(sequence_ye.begin(), sequence_ye.end());
    std::random_shuffle(sequence_temp.begin(), sequence_temp.end());
    std::random_shuffle(sequence_rho.begin(), sequence_rho.end());

    // build the array of coordinates we want to get the interpolated values for
    std::vector<sheneos::sheneos_coord> values;
    values.reserve(num_ye_points * num_temp_points * num_rho_points);

    std::vector<hpx::future<std::vector<double>>> tests;
    for (std::size_t const& ii : sequence_ye)
    {
        for (std::size_t const& jj : sequence_temp)
        {
            for (std::size_t const& kk : sequence_rho)
            {
                values.push_back(sheneos::sheneos_coord(
                    values_ye[ii], values_temp[jj], values_rho[kk]));
            }
        }
    }

    // Execute bulk operation
    hpx::future<std::vector<double>> bulk_one_tests =
        shen.interpolate_one_bulk_async(
            values, sheneos::server::partition3d::logpress);

    std::vector<double> results = hpx::unwrap(bulk_one_tests);
    (void) results;
}

HPX_DECLARE_ACTION(test_sheneos_one_bulk, test_one_bulk_action);
HPX_ACTION_USES_MEDIUM_STACK(test_one_bulk_action);
HPX_PLAIN_ACTION(test_sheneos_one_bulk, test_one_bulk_action)

///////////////////////////////////////////////////////////////////////////////
/// This is the test function for interpolate_bulk. It will be invoked on all
/// localities that the benchmark is being run on.
void test_sheneos_bulk(std::size_t num_ye_points, std::size_t num_temp_points,
    std::size_t num_rho_points, std::size_t seed)
{
    // Create a client instance connected to the already existing interpolation
    // object.
    sheneos::interpolator shen;
    shen.connect(shen_symbolic_name);

    ///////////////////////////////////////////////////////////////////////////
    // Compute the minimum, maximum and delta values for each dimension.
    double min_ye = 0, max_ye = 0;
    shen.get_dimension(sheneos::dimension::ye, min_ye, max_ye);
    double const delta_ye = (max_ye - min_ye) / num_ye_points;

    double min_temp = 0, max_temp = 0;
    shen.get_dimension(sheneos::dimension::temp, min_temp, max_temp);
    double const delta_temp = (max_temp - min_temp) / num_temp_points;

    double min_rho = 0, max_rho = 0;
    shen.get_dimension(sheneos::dimension::rho, min_rho, max_rho);
    double const delta_rho = (max_rho - min_rho) / num_rho_points;

    ///////////////////////////////////////////////////////////////////////////
    // Generate the data points, spacing them out equally.
    std::vector<double> values_ye(num_ye_points);
    std::vector<std::size_t> sequence_ye(num_ye_points);
    double ye = min_ye;
    for (std::size_t i = 0; i < num_ye_points; ++i)
    {
        values_ye[i] = ye;
        sequence_ye[i] = i;
        ye += delta_ye;
    }

    std::vector<double> values_temp(num_temp_points);
    std::vector<std::size_t> sequence_temp(num_temp_points);
    double temp = min_temp;
    for (std::size_t i = 0; i < num_temp_points; ++i)
    {
        values_temp[i] = temp;
        sequence_temp[i] = i;
        temp += delta_temp;
    }

    std::vector<double> values_rho(num_rho_points);
    std::vector<std::size_t> sequence_rho(num_rho_points);
    double rho = min_rho;
    for (std::size_t i = 0; i < num_rho_points; ++i)
    {
        values_rho[i] = rho;
        sequence_rho[i] = i;
        rho += delta_rho;
    }

    ///////////////////////////////////////////////////////////////////////////
    // We want to avoid invoking the same evaluation sequence on all localities
    // performing the test, so we randomly shuffle the sequences. We combine
    // the shared seed with the locality id to ensure that each locality has
    // a unique, reproducible seed.
    std::srand(static_cast<unsigned int>(seed + hpx::get_locality_id()));
    std::random_shuffle(sequence_ye.begin(), sequence_ye.end());
    std::random_shuffle(sequence_temp.begin(), sequence_temp.end());
    std::random_shuffle(sequence_rho.begin(), sequence_rho.end());

    // build the array of coordinates we want to get the interpolated values for
    std::vector<sheneos::sheneos_coord> values;
    values.reserve(num_ye_points * num_temp_points * num_rho_points);

    std::vector<hpx::future<std::vector<double>>> tests;
    for (std::size_t const& ii : sequence_ye)
    {
        for (std::size_t const& jj : sequence_temp)
        {
            for (std::size_t const& kk : sequence_rho)
            {
                values.push_back(sheneos::sheneos_coord(
                    values_ye[ii], values_temp[jj], values_rho[kk]));
            }
        }
    }

    // Execute bulk operation
    hpx::future<std::vector<std::vector<double>>> bulk_tests =
        shen.interpolate_bulk_async(values);

    std::vector<std::vector<double>> results = hpx::unwrap(bulk_tests);
    (void) results;
}

HPX_DECLARE_ACTION(test_sheneos_bulk, test_bulk_action);
HPX_ACTION_USES_MEDIUM_STACK(test_bulk_action);
HPX_PLAIN_ACTION(test_sheneos_bulk, test_bulk_action)

///////////////////////////////////////////////////////////////////////////////
void wait_for_task(std::size_t i, hpx::chrono::high_resolution_timer& t)
{
    std::cout << "Finished task " << i << ": " << t.elapsed() << " [s]"
              << std::endl;
}

void wait_for_bulk_one_task(
    std::size_t i, hpx::chrono::high_resolution_timer& t)
{
    std::cout << "Finished bulk-one task " << i << ": " << t.elapsed() << " [s]"
              << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::string const datafilename = vm["file"].as<std::string>();

    std::size_t const num_ye_points = vm["num-ye-points"].as<std::size_t>();
    std::size_t const num_temp_points = vm["num-temp-points"].as<std::size_t>();
    std::size_t const num_rho_points = vm["num-rho-points"].as<std::size_t>();

    std::size_t const num_partitions = vm["num-partitions"].as<std::size_t>();

    std::size_t num_workers = vm["num-workers"].as<std::size_t>();

    std::size_t seed = vm["seed"].as<std::size_t>();
    if (!seed)
        seed = std::size_t(std::time(nullptr));

    std::cout << "Seed: " << seed << std::endl;

    {
        hpx::chrono::high_resolution_timer t;

        // Create a distributed interpolation object with the name
        // shen_symbolic_name. The interpolation object will have
        // num_partitions partitions
        sheneos::interpolator shen(
            datafilename, shen_symbolic_name, num_partitions);

        std::cout << "Created interpolator: " << t.elapsed() << " [s]"
                  << std::endl;

        // Get a list of all localities that support the test action.
        std::vector<hpx::id_type> locality_ids = hpx::find_all_localities();

        t.restart();

        // Kick off the computation asynchronously. On each locality,
        // num_workers test_actions are created.
        std::vector<hpx::future<void>> bulk_tests;
        for (hpx::id_type const& id : locality_ids)
        {
            for (std::size_t i = 0; i < num_workers; ++i)
            {
                bulk_tests.push_back(hpx::async<test_bulk_action>(
                    id, num_ye_points, num_temp_points, num_rho_points, seed));
            }
        }
        hpx::wait_all(bulk_tests);

        std::cout << "Completed bulk tests: " << t.elapsed() << " [s]"
                  << std::endl;

    }    // Ensure that everything is out of scope before shutdown.

    // Shutdown the runtime system.
    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    using hpx::program_options::options_description;
    using hpx::program_options::value;

    // Configure application-specific options.
    options_description cmdline("Usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("file",
        value<std::string>()->default_value(
            "sheneos_220r_180t_50y_extT_analmu_20100322_SVNr28.h5"),
        "name of HDF5 data file containing the Shen EOS tables")(
        "num-ye-points,Y", value<std::size_t>()->default_value(40),
        "number of points to interpolate on the ye axis")("num-temp-points,T",
        value<std::size_t>()->default_value(40),
        "number of points to interpolate on the temp axis")("num-rho-points,R",
        value<std::size_t>()->default_value(40),
        "number of points to interpolate on the rho axis")("num-partitions",
        value<std::size_t>()->default_value(32),
        "number of partitions to create")("num-workers",
        value<std::size_t>()->default_value(1),
        "number of worker/measurement threads to create")("seed",
        value<std::size_t>()->default_value(0),
        "the seed for the pseudo random number generator (if 0, a seed "
        "is chosen based on the current system time)");

    // Initialize and run HPX.
    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::init(argc, argv, init_args);
}

#endif
