//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/openshmem_base/openshmem_environment.hpp>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace hpx::util {

    openshmem_environment::mutex_type openshmem_environment::mtx_{};

    bool openshmem_environment::enabled_ = false;
    bool openshmem_environment::has_called_init_ = false;

    bool openshmem_environment::check_openshmem_environment(
        runtime_configuration const& cfg)
    {
        return hpx::util::get_entry_as<bool>(
            cfg, "hpx.parcel.openshmem.enable", false);
    }

    void openshmem_environment::init(
        int*, char***, runtime_configuration& cfg)
    {
        if (enabled_)
            return;

        has_called_init_ = false;

        enabled_ = check_openshmem_environment(cfg);
        if (!enabled_)
        {
            cfg.add_entry("hpx.parcel.openshmem.enable", "0");
            return;
        }

        int provided = 0;
        int ret = shmem_init_thread(SHMEM_THREAD_MULTIPLE, &provided);
        if (ret != 0)
        {
            enabled_ = false;
            throw std::runtime_error("Failed to initialize OpenSHMEM");
        }

        if (provided != SHMEM_THREAD_MULTIPLE)
        {
            std::cerr << "Warning: OpenSHMEM did not provide "
                         "SHMEM_THREAD_MULTIPLE. "
                      << "Provided level: " << provided << std::endl;
        }

        has_called_init_ = true;

        cfg.set_num_localities(static_cast<std::uint32_t>(shmem_n_pes()));

        int const this_rank = rank();
        if (this_rank == 0)
        {
            cfg.mode_ = hpx::runtime_mode::console;
        }
        else
        {
            cfg.mode_ = hpx::runtime_mode::worker;
        }

        cfg.add_entry(
            "hpx.parcel.openshmem.rank", std::to_string(this_rank));
        cfg.add_entry(
            "hpx.parcel.openshmem.processorname", get_processor_name());
    }

    void openshmem_environment::finalize() noexcept
    {
        scoped_lock l(mtx_);
        if (enabled_ && has_called_init_)
        {
            has_called_init_ = false;
            shmem_finalize();
        }
    }

    bool openshmem_environment::enabled() noexcept
    {
        return enabled_;
    }

    bool openshmem_environment::has_called_init() noexcept
    {
        return has_called_init_;
    }

    int openshmem_environment::rank() noexcept
    {
        return shmem_my_pe();
    }

    int openshmem_environment::size() noexcept
    {
        return shmem_n_pes();
    }

    std::string openshmem_environment::get_processor_name()
    {
        return std::to_string(shmem_my_pe());
    }
}    // namespace hpx::util

#endif
