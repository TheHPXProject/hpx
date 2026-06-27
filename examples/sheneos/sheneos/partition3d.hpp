//  Copyright (c) 2007-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/future.hpp>
#include <hpx/hpx.hpp>
#include <hpx/include/client.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "server/partition3d.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace sheneos {
    class partition3d
      : public hpx::components::client_base<partition3d,
            sheneos::server::partition3d>
    {
    private:
        typedef hpx::components::client_base<partition3d,
            sheneos::server::partition3d>
            base_type;

    public:
        partition3d() {}

        /// Create a new partition instance locally and initialize it
        /// synchronously.
        partition3d(std::string const& datafilename, dimension const& dimx,
            dimension const& dimy, dimension const& dimz)
          : base_type(hpx::new_<server::partition3d>(hpx::find_here()))
        {
            init(datafilename, dimx, dimy, dimz);
        }

        /// Create a new partition instance on a specific locality and
        /// initialize it synchronously.
        ///
        /// \param gid [in] The locality where the partition should be created.
        partition3d(hpx::id_type const& id, std::string const& datafilename,
            dimension const& dimx, dimension const& dimy, dimension const& dimz)
          : base_type(hpx::new_<server::partition3d>(id))
        {
            init(datafilename, dimx, dimy, dimz);
        }

        /// Connect to an existing partition instance.
        partition3d(hpx::id_type const& id)
          : base_type(id)
        {
        }
        partition3d(hpx::future<hpx::id_type>&& id)
          : base_type(std::move(id))
        {
        }

        /// Initialize this partition asynchronously.
        hpx::future<void> init_async(std::string const& datafilename,
            dimension const& dimx, dimension const& dimy, dimension const& dimz)
        {
            using init_action = server::partition3d::init_action;
            return hpx::async(
                init_action(), this->get_id(), datafilename, dimx, dimy, dimz);
        }

        /// Initialize this partition synchronously.
        void init(std::string const& datafilename, dimension const& dimx,
            dimension const& dimy, dimension const& dimz)
        {
            init_async(datafilename, dimx, dimy, dimz).get();
        }

        /// Asynchronously perform an interpolation on this partition.
        ///
        /// \param ye        [in] Electron fraction.
        /// \param temp      [in] Temperature.
        /// \param rho       [in] Rest mass density of the plasma.
        /// \param eosvalues [in] The EOS values to interpolate. Must be
        ///                  in the range of this partition.
        hpx::future<std::vector<double>> interpolate_async(
            double ye, double temp, double rho, std::uint32_t eosvalues) const
        {
            using interpolate_action = server::partition3d::interpolate_action;
            return hpx::async(
                interpolate_action(), this->get_id(), ye, temp, rho, eosvalues);
        }

        /// Synchronously perform an interpolation on this partition.
        ///
        /// \param ye        [in] Electron fraction.
        /// \param temp      [in] Temperature.
        /// \param rho       [in] Rest mass density of the plasma.
        /// \param eosvalues [in] The EOS values to interpolate. Must be
        ///                  in the range of this partition.
        std::vector<double> interpolate(
            double ye, double temp, double rho, std::uint32_t eosvalues) const
        {
            return interpolate_async(ye, temp, rho, eosvalues).get();
        }

        /// Asynchronously perform an interpolation of one given field on this
        /// partition.
        ///
        /// \param ye        [in] Electron fraction.
        /// \param temp      [in] Temperature.
        /// \param rho       [in] Rest mass density of the plasma.
        /// \param eosvalue  [in] The EOS value to interpolate. Must be
        ///                  in the range of the given partition.
        hpx::future<double> interpolate_one_async(
            double ye, double temp, double rho, std::uint32_t eosvalue) const
        {
            using interpolate_one_action =
                server::partition3d::interpolate_one_action;
            return hpx::async(interpolate_one_action(), this->get_id(), ye,
                temp, rho, eosvalue);
        }

        /// Synchronously perform an interpolation of one given field on this
        /// partition.
        ///
        /// \param ye        [in] Electron fraction.
        /// \param temp      [in] Temperature.
        /// \param rho       [in] Rest mass density of the plasma.
        /// \param eosvalue  [in] The EOS value to interpolate. Must be
        ///                  in the range of the given partition.
        double interpolate_one(
            double ye, double temp, double rho, std::uint32_t eosvalue) const
        {
            return interpolate_one_async(ye, temp, rho, eosvalue).get();
        }

        /// Asynchronously perform an interpolation of one given field on this
        /// partition.
        ///
        /// \param cords     [in] triples of electron fractions, temperatures,
        ///                  and rest mass densities of the plasma.
        /// \param eosvalue  [in] The EOS value to interpolate. Must be
        ///                  in the range of the given partition.
        hpx::future<std::vector<double>> interpolate_one_bulk_async(
            std::vector<sheneos_coord> const& coords,
            std::uint32_t eosvalue) const
        {
            using interpolate_one_bulk_action =
                server::partition3d::interpolate_one_bulk_action;
            return hpx::async(interpolate_one_bulk_action(), this->get_id(),
                coords, eosvalue);
        }

        /// Synchronously perform an interpolation of one given field on this
        /// partition.
        ///
        /// \param cords     [in] triples of electron fractions, temperatures,
        ///                  and rest mass densities of the plasma.
        /// \param eosvalue  [in] The EOS value to interpolate. Must be
        ///                  in the range of the given partition.
        std::vector<double> interpolate_one_bulk(
            std::vector<sheneos_coord> const& coords,
            std::uint32_t eosvalue) const
        {
            return interpolate_one_bulk_async(coords, eosvalue).get();
        }

        /// Asynchronously perform an interpolation of one given field on this
        /// partition.
        ///
        /// \param cords     [in] triples of electron fractions, temperatures,
        ///                  and rest mass densities of the plasma.
        /// \param eosvalue  [in] The EOS value to interpolate. Must be
        ///                  in the range of the given partition.
        hpx::future<std::vector<std::vector<double>>> interpolate_bulk_async(
            std::vector<sheneos_coord> const& coords,
            std::uint32_t eosvalue) const
        {
            using interpolate_bulk_action =
                server::partition3d::interpolate_bulk_action;
            return hpx::async(
                interpolate_bulk_action(), this->get_id(), coords, eosvalue);
        }

        /// Synchronously perform an interpolation of one given field on this
        /// partition.
        ///
        /// \param cords     [in] triples of electron fractions, temperatures,
        ///                  and rest mass densities of the plasma.
        /// \param eosvalue  [in] The EOS value to interpolate. Must be
        ///                  in the range of the given partition.
        std::vector<std::vector<double>> interpolate_bulk(
            std::vector<sheneos_coord> const& coords,
            std::uint32_t eosvalue) const
        {
            return interpolate_bulk_async(coords, eosvalue).get();
        }
    };
}    // namespace sheneos

#endif
