//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>

#include <hpx/assert.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/lcos_local.hpp>
#include <hpx/include/runtime.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "interpolator.hpp"
#include "partition3d.hpp"
#include "read_values.hpp"

#include <H5public.h>

///////////////////////////////////////////////////////////////////////////////
// Add factory registration functionality.
HPX_REGISTER_COMPONENT_MODULE()    // Create entry point for component factory.

///////////////////////////////////////////////////////////////////////////////
// Register a shutdown function which will be called as a px-thread during
// runtime shutdown.
namespace sheneos {
    ///////////////////////////////////////////////////////////////////////////
    // This function will be registered as a shutdown function for HPX below.
    void shutdown()
    {
        // Because of problems while dynamically loading/unloading the HDF5
        // libraries we need to manually call the HDF5 termination routines.
        ::H5close();
    }

    ///////////////////////////////////////////////////////////////////////////
    bool get_shutdown(
        hpx::shutdown_function_type& shutdown_func, bool& pre_shutdown)
    {
        shutdown_func = shutdown;
        pre_shutdown = false;    // run this as late as possible
        return true;
    }
}    // namespace sheneos
HPX_REGISTER_SHUTDOWN_MODULE(&::sheneos::get_shutdown);

///////////////////////////////////////////////////////////////////////////////
// Interpolation client.
namespace sheneos {
    interpolator::interpolator()
      : num_partitions_per_dim_(0)
      , was_created_(false)
    {
        std::memset(minval_, 0, sizeof(minval_));
        std::memset(maxval_, 0, sizeof(maxval_));
        std::memset(delta_, 0, sizeof(delta_));
        std::memset(num_values_, 0, sizeof(num_values_));
    }

    interpolator::interpolator(hpx::future<hpx::id_type>&& id)
      : base_type(std::move(id))
      , num_partitions_per_dim_(0)
      , was_created_(false)
    {
        std::memset(minval_, 0, sizeof(minval_));
        std::memset(maxval_, 0, sizeof(maxval_));
        std::memset(delta_, 0, sizeof(delta_));
        std::memset(num_values_, 0, sizeof(num_values_));
    }

    interpolator::~interpolator()
    {
        if (was_created_)
        {
            // Unregister the config data.
            typedef config_data_type::get_action act;
            config_data data = act()(get_id());

            hpx::agas::unregister_name(hpx::launch::sync, data.symbolic_name_);

            // Unregister all symbolic names.
            for (std::size_t i = 0; i < partitions_.size(); ++i)
            {
                hpx::agas::unregister_name(
                    hpx::launch::sync, data.symbolic_name_ + std::to_string(i));
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    interpolator::interpolator(std::string const& datafilename,
        std::string const& symbolic_name_base, std::size_t num_instances)
      : base_type(hpx::new_<config_data_type>(hpx::find_here(),
            config_data(datafilename, symbolic_name_base, num_instances)))
      , num_partitions_per_dim_(0)
      , was_created_(false)
    {
        hpx::future<void> f = register_as(symbolic_name_base);

        std::memset(minval_, 0, sizeof(minval_));
        std::memset(maxval_, 0, sizeof(maxval_));
        std::memset(delta_, 0, sizeof(delta_));
        std::memset(num_values_, 0, sizeof(num_values_));

        // Initialize the partitions and store the mappings.
        fill_partitions(datafilename, symbolic_name_base, num_instances);

        f.get();
        was_created_ = true;
    }

    ///////////////////////////////////////////////////////////////////////////
    void interpolator::connect(std::string symbolic_name_base)
    {
        // Connect to the config object.
        this->connect_to(symbolic_name_base);

        typedef config_data_type::get_action act;
        config_data data = act()(get_id());

        num_partitions_per_dim_ = static_cast<std::size_t>(
            std::exp(std::log(double(data.num_instances_)) / 3));

        if (data.symbolic_name_[data.symbolic_name_.size() - 1] != '/')
            data.symbolic_name_ += "/";

        // Connect to the partitions.
        partitions_.resize(data.num_instances_);
        for (std::size_t i = 0; i < data.num_instances_; ++i)
        {
            partitions_[i].connect_to(data.symbolic_name_ + std::to_string(i));
        }

        // Read required data from given file.
        std::lock_guard<mutex_type> l(mtx_);

        num_values_[dimension::ye] = extract_data_range(data.datafile_name_,
            "ye", minval_[dimension::ye], maxval_[dimension::ye],
            delta_[dimension::ye]);
        num_values_[dimension::temp] = extract_data_range(data.datafile_name_,
            "logtemp", minval_[dimension::temp], maxval_[dimension::temp],
            delta_[dimension::temp]);
        num_values_[dimension::rho] = extract_data_range(data.datafile_name_,
            "logrho", minval_[dimension::rho], maxval_[dimension::rho],
            delta_[dimension::rho]);
    }

    ///////////////////////////////////////////////////////////////////////////
    void interpolator::fill_partitions(std::string const& datafilename,
        std::string symbolic_name_base, std::size_t num_instances)
    {
        auto locs = hpx::find_all_localities();
        hpx::future<std::vector<partition3d>> result =
            hpx::new_<partition3d[]>(hpx::default_layout(locs), num_instances);

        // Read required data from file.
        {
            std::lock_guard<mutex_type> l(mtx_);
            num_values_[dimension::ye] =
                extract_data_range(datafilename, "ye", minval_[dimension::ye],
                    maxval_[dimension::ye], delta_[dimension::ye]);
            num_values_[dimension::temp] = extract_data_range(datafilename,
                "logtemp", minval_[dimension::temp], maxval_[dimension::temp],
                delta_[dimension::temp]);
            num_values_[dimension::rho] = extract_data_range(datafilename,
                "logrho", minval_[dimension::rho], maxval_[dimension::rho],
                delta_[dimension::rho]);
        }

        // Initialize all attached partition objects.
        num_partitions_per_dim_ =
            static_cast<std::size_t>(std::cbrt(double(locs.size())));

        std::size_t partition_size_x =
            num_values_[dimension::ye] / num_partitions_per_dim_;
        std::size_t last_partition_size_x = num_values_[dimension::ye] -
            partition_size_x * (num_partitions_per_dim_ - 1);

        std::size_t partition_size_y =
            num_values_[dimension::temp] / num_partitions_per_dim_;
        std::size_t last_partition_size_y = num_values_[dimension::temp] -
            partition_size_y * (num_partitions_per_dim_ - 1);

        std::size_t partition_size_z =
            num_values_[dimension::rho] / num_partitions_per_dim_;
        std::size_t last_partition_size_z = num_values_[dimension::rho] -
            partition_size_z * (num_partitions_per_dim_ - 1);

        dimension dim_x(num_values_[dimension::ye]);
        dimension dim_y(num_values_[dimension::temp]);
        dimension dim_z(num_values_[dimension::rho]);

        std::vector<hpx::future<void>> lazy_sync;
        lazy_sync.reserve(2 * num_instances);

        for (std::size_t x = 0; x != num_partitions_per_dim_; ++x)
        {
            dim_x.offset_ = partition_size_x * x;
            if (x == num_partitions_per_dim_ - 1)
                dim_x.count_ = last_partition_size_x;
            else
                dim_x.count_ = partition_size_x;

            for (std::size_t y = 0; y != num_partitions_per_dim_; ++y)
            {
                dim_y.offset_ = partition_size_y * y;
                if (y == num_partitions_per_dim_ - 1)
                    dim_y.count_ = last_partition_size_y;
                else
                    dim_y.count_ = partition_size_y;

                for (std::size_t z = 0; z != num_partitions_per_dim_; ++z)
                {
                    dim_z.offset_ = partition_size_z * z;
                    if (z == num_partitions_per_dim_ - 1)
                        dim_z.count_ = last_partition_size_z;
                    else
                        dim_z.count_ = partition_size_z;

                    std::size_t index = x +
                        (y + z * num_partitions_per_dim_) *
                            num_partitions_per_dim_;
                    HPX_ASSERT(index < partitions_.size());

                    lazy_sync.push_back(partitions_[index].init_async(
                        datafilename, dim_x, dim_y, dim_z));
                }
            }
        }

        if (symbolic_name_base[symbolic_name_base.size() - 1] != '/')
            symbolic_name_base += "/";

        // Register symbolic names of all involved components.
        partitions_ = result.get();

        std::size_t i = 0;
        for (partition3d& p : partitions_)
        {
            lazy_sync.push_back(
                p.register_as(symbolic_name_base + std::to_string(i++)));
        }

        // Wait for initialization to finish.
        hpx::wait_all(lazy_sync);
    }

    ///////////////////////////////////////////////////////////////////////////
    partition3d const& interpolator::get_partition(
        double ye, double temp, double rho) const
    {
        std::size_t x = get_partition_index(dimension::ye, ye);
        std::size_t y = get_partition_index(dimension::temp, std::log10(temp));
        std::size_t z = get_partition_index(dimension::rho, std::log10(rho));

        std::size_t index =
            x + (y + z * num_partitions_per_dim_) * num_partitions_per_dim_;
        HPX_ASSERT(index < partitions_.size());

        return partitions_[index];
    }

    std::size_t interpolator::get_partition_index(
        std::size_t d, double value) const
    {
        std::size_t partition_size = num_values_[d] / num_partitions_per_dim_;
        std::size_t partition_index = static_cast<std::size_t>(
            (value - minval_[d]) / (delta_[d] * partition_size));
        if (partition_index == num_partitions_per_dim_)
            --partition_index;
        HPX_ASSERT(partition_index < num_partitions_per_dim_);
        return partition_index;
    }

    void interpolator::get_dimension(
        dimension::type what, double& min, double& max) const
    {
        switch (what)
        {
        case dimension::ye:
            min = minval_[dimension::ye];
            max = maxval_[dimension::ye];
            break;

        case dimension::temp:
        case dimension::rho:
            min = std::pow(10., minval_[what]);
            max = std::pow(10., maxval_[what]);
            break;

        default:
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "sheneos::interpolator::get_dimension",
                "value of parameter 'what' is not valid");
            break;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // context data and callback function for asynchronous bulk operations
    struct context_data
    {
        std::vector<std::size_t> indices_;
        std::vector<sheneos_coord> coords_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // callback function object which will be called whenever an asynchronous
    // bulk operation has been completed
    struct on_completed_bulk_one
    {
        typedef std::map<hpx::id_type, context_data> partitions_type;

        on_completed_bulk_one(std::shared_ptr<partitions_type> parts,
            context_data const& data, std::vector<double>& overall_result)
          : data_(data)
          , overall_result_(overall_result)
          , partitions_(parts)
        {
        }

        void operator()(hpx::future<std::vector<double>> f)
        {
            std::vector<double> result = f.get();
            std::vector<std::size_t> const& indices = data_.get().indices_;

            if (result.size() != indices.size())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "interpolator::on_completed_bulk_one",
                    "inconsistent sizes of result and index arrays");
            }

            std::vector<double>& overall_result = overall_result_.get();
            for (std::size_t i = 0; i < indices.size(); ++i)
                overall_result[indices[i]] = result[i];
        }

        std::reference_wrapper<context_data const> data_;
        std::reference_wrapper<std::vector<double>> overall_result_;
        std::shared_ptr<partitions_type> partitions_;
    };

    struct bulk_one_context
    {
        typedef std::map<hpx::id_type, context_data> partitions_type;

        bulk_one_context(std::shared_ptr<partitions_type> parts, std::size_t s,
            std::uint32_t eos)
          : partitions(parts)
          , size(s)
          , eosvalue(eos)
        {
        }

        std::vector<double> operator()() const
        {
            // create the overall result vector
            std::vector<double> overall_result;
            overall_result.resize(size);

            // asynchronously invoke the interpolation on the different partitions
            std::vector<hpx::future<void>> lazy_results;
            lazy_results.reserve(partitions->size());

            typedef partitions_type::value_type value_type;
            for (value_type& p : *partitions)
            {
                typedef sheneos::server::partition3d::
                    interpolate_one_bulk_action action_type;

                context_data& d = p.second;
                lazy_results.push_back(hpx::async<action_type>(
                    p.first, std::move(d.coords_), eosvalue)
                        .then(on_completed_bulk_one(
                            partitions, d, overall_result)));
            }

            // wait for all asynchronous operations to complete
            hpx::wait_all(lazy_results);

            return overall_result;
        }

        std::shared_ptr<partitions_type> partitions;
        std::size_t size;
        std::uint32_t eosvalue;
    };

    hpx::future<std::vector<double>> interpolator::interpolate_one_bulk_async(
        std::vector<sheneos_coord> const& coords, std::uint32_t eosvalue) const
    {
        typedef std::map<hpx::id_type, context_data> partitions_type;

        std::shared_ptr<partitions_type> partitions(
            std::make_shared<partitions_type>());

        partitions_type& parts = *partitions;

        std::size_t index = 0;
        for (auto const& c : coords)
        {
            context_data& d = parts[get_partition(c).get_id()];

            d.indices_.push_back(index++);
            d.coords_.push_back(c);
        }

        return hpx::async(
            bulk_one_context(partitions, coords.size(), eosvalue));
    }

    ///////////////////////////////////////////////////////////////////////////
    // callback function object which will be called whenever an asynchronous
    // bulk operation has been completed
    struct on_completed_bulk
    {
        typedef std::map<hpx::id_type, context_data> partitions_type;

        on_completed_bulk(std::shared_ptr<partitions_type> parts,
            context_data const& data,
            std::vector<std::vector<double>>& overall_results)
          : data_(data)
          , overall_results_(overall_results)
          , partitions_(parts)
        {
        }

        void operator()(hpx::future<std::vector<std::vector<double>>> f)
        {
            std::vector<std::vector<double>> result = f.get();
            std::vector<std::size_t> const& indices = data_.get().indices_;

            if (result.size() != indices.size())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "interpolator::on_completed_bulk",
                    "inconsistent sizes of result and index arrays");
            }

            std::vector<std::vector<double>>& overall_results =
                overall_results_.get();
            for (std::size_t i = 0; i < indices.size(); ++i)
                overall_results[indices[i]] = result[i];
        }

        std::reference_wrapper<context_data const> data_;
        std::reference_wrapper<std::vector<std::vector<double>>>
            overall_results_;
        std::shared_ptr<partitions_type> partitions_;
    };

    ///////////////////////////////////////////////////////////////////////////
    struct bulk_context
    {
        typedef std::map<hpx::id_type, context_data> partitions_type;

        bulk_context(std::shared_ptr<partitions_type> parts, std::size_t s,
            std::uint32_t eos)
          : partitions(parts)
          , size(s)
          , eosvalues(eos)
        {
        }

        std::vector<std::vector<double>> operator()() const
        {
            namespace naming = hpx::naming;
            namespace lcos = hpx::lcos;

            // create the overall result vector
            std::vector<std::vector<double>> overall_results;
            overall_results.resize(size);

            // asynchronously invoke the interpolation on the different partitions
            std::vector<hpx::future<void>> lazy_results;
            lazy_results.reserve(partitions->size());

            typedef std::map<hpx::id_type, context_data>::value_type value_type;
            for (value_type& p : *partitions)
            {
                typedef sheneos::server::partition3d::interpolate_bulk_action
                    action_type;

                context_data& d = p.second;
                lazy_results.push_back(hpx::async<action_type>(
                    p.first, std::move(d.coords_), eosvalues)
                        .then(
                            on_completed_bulk(partitions, d, overall_results)));
            }

            // wait for all asynchronous operations to complete
            hpx::wait_all(lazy_results);

            return overall_results;
        }

        std::shared_ptr<partitions_type> partitions;
        std::size_t size;
        std::uint32_t eosvalues;
    };

    hpx::future<std::vector<std::vector<double>>>
    interpolator::interpolate_bulk_async(
        std::vector<sheneos_coord> const& coords, std::uint32_t eosvalues) const
    {
        typedef std::map<hpx::id_type, context_data> partitions_type;
        std::shared_ptr<partitions_type> partitions(
            std::make_shared<partitions_type>());

        partitions_type& parts = *partitions;

        std::size_t index = 0;
        for (auto const& c : coords)
        {
            context_data& d = parts[get_partition(c).get_id()];

            d.indices_.push_back(index++);
            d.coords_.push_back(c);
        }

        return hpx::async(bulk_context(partitions, coords.size(), eosvalues));
    }
}    // namespace sheneos
