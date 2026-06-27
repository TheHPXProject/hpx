//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/assert.hpp>
#include <hpx/collectives/barrier.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace hpx::traits::communication {

    // This is explicitly instantiated to ensure that the id is stable across
    // shared libraries.
    char const* communicator_data<barrier_tag>::name() noexcept
    {
        static char const* name = "barrier";
        return name;
    }
}    // namespace hpx::traits::communication

namespace hpx::distributed {

    namespace {

        std::size_t read_cut_off()
        {
            return hpx::util::from_string<std::size_t>(hpx::get_config_entry(
                "hpx.lcos.collectives.cut_off", static_cast<std::size_t>(-1)));
        }

        std::size_t read_arity()
        {
            return hpx::util::from_string<std::size_t>(
                hpx::get_config_entry("hpx.lcos.collectives.arity", 32));
        }
    }    // namespace

    barrier::barrier() = default;

    void barrier::create_communicator(bool const force_flat)
    {
        using hpx::collectives::arity_arg;
        using hpx::collectives::generation_arg;
        using hpx::collectives::num_sites_arg;
        using hpx::collectives::root_site_arg;
        using hpx::collectives::this_site_arg;

        if (force_flat || num_ < cut_off_)
        {
            comm_ = hpx::collectives::create_communicator(hpx::launch::sync,
                base_name_.c_str(), num_sites_arg(num_), this_site_arg(rank_),
                generation_arg(), root_site_arg(0));
        }
        else
        {
            auto hier = hpx::collectives::create_hierarchical_communicator(
                base_name_.c_str(), num_sites_arg(num_), this_site_arg(rank_),
                arity_arg(read_arity()), generation_arg(), root_site_arg(0));

            // Wait for each sub-communicator's future to become ready so the
            // ctor blocks as documented.
            for (std::size_t i = 0; i != hier.size(); ++i)
            {
                hier.get(i).wait();
            }

            comm_ = std::move(hier);
        }
    }

    barrier::barrier(std::string const& base_name)
      : base_name_(base_name)
      , num_(static_cast<std::size_t>(
            hpx::get_num_localities(hpx::launch::sync)))
      , rank_(static_cast<std::size_t>(hpx::get_locality_id()))
      , cut_off_(read_cut_off())
    {
        create_communicator(false);
    }

    barrier::barrier(std::string const& base_name, std::size_t const num)
      : base_name_(base_name)
      , num_(num)
      , rank_(static_cast<std::size_t>(hpx::get_locality_id()))
      , cut_off_(read_cut_off())
    {
        create_communicator(false);
    }

    barrier::barrier(std::string const& base_name, std::size_t const num,
        std::size_t const rank)
      : base_name_(base_name)
      , num_(num)
      , rank_(rank)
      , cut_off_(read_cut_off())
    {
        create_communicator(false);
    }

    barrier::barrier(std::string const& base_name,
        std::vector<std::size_t> const& ranks, std::size_t const rank)
      : base_name_(base_name)
      , num_(ranks.size())
      , cut_off_(read_cut_off())
    {
        auto const rank_it = std::find(ranks.begin(), ranks.end(), rank);
        HPX_ASSERT(rank_it != ranks.end());
        rank_ = static_cast<std::size_t>(std::distance(ranks.begin(), rank_it));

        create_communicator(false);
    }

    barrier::barrier(std::string const& base_name, std::size_t const num,
        std::size_t const rank, force_flat_tag)
      : base_name_(base_name)
      , num_(num)
      , rank_(rank)
      , cut_off_(read_cut_off())
    {
        create_communicator(true);
    }

    barrier::barrier(barrier&& other) noexcept
      : base_name_(std::move(other.base_name_))
      , num_(other.num_)
      , rank_(other.rank_)
      , cut_off_(other.cut_off_)
      , generation_(other.generation_.load(std::memory_order_relaxed))
      , comm_(std::move(other.comm_))
    {
        other.comm_ = hpx::collectives::communicator{};
    }

    barrier& barrier::operator=(barrier&& other) noexcept
    {
        if (this != &other)
        {
            release();
            base_name_ = std::move(other.base_name_);
            num_ = other.num_;
            rank_ = other.rank_;
            cut_off_ = other.cut_off_;
            generation_.store(other.generation_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            comm_ = std::move(other.comm_);
            other.comm_ = hpx::collectives::communicator{};
        }
        return *this;
    }

    barrier::~barrier()
    {
        release();
    }

    hpx::future<void> barrier::wait(hpx::launch::async_policy) const
    {
        auto const gen = ++generation_;
        return std::visit(
            [&](auto& c) -> hpx::future<void> {
                using T = std::decay_t<decltype(c)>;
                if constexpr (std::is_same_v<T, hpx::collectives::communicator>)
                {
                    if (!c.valid())
                    {
                        return hpx::make_exceptional_future<void>(
                            HPX_GET_EXCEPTION(hpx::error::invalid_status,
                                "hpx::distributed::barrier::wait",
                                "wait() called on a released, detached, or "
                                "moved-from barrier instance"));
                    }
                }

                return hpx::collectives::barrier(c,
                    hpx::collectives::this_site_arg(rank_),
                    hpx::collectives::generation_arg(gen));
            },
            comm_);
    }

    void barrier::wait() const
    {
        wait(hpx::launch::async).get();
    }

    void barrier::release()
    {
        if (auto const* c = std::get_if<hpx::collectives::communicator>(&comm_);
            c && !c->valid())
        {
            return;
        }

        if (hpx::get_runtime_ptr() != nullptr &&
            hpx::threads::threadmanager_is(hpx::state::running) &&
            !hpx::is_stopped_or_shutting_down())
        {
            // make sure this runs as an HPX thread
            if (hpx::threads::get_self_ptr() == nullptr)
            {
                hpx::run_as_hpx_thread(&barrier::release, this);
                return;
            }

            // release() is called from ~barrier(), so we must not let
            // exceptions escape (throwing from a dtor during stack unwinding
            // triggers std::terminate).
            try
            {
                wait(hpx::launch::async).get();
            }
            catch (...)    // NOLINT(bugprone-empty-catch)
            {
            }
        }

        // Auto-unregister happens synchronously via the communicator's
        // shared-state dtor.
        comm_ = hpx::collectives::communicator{};
    }

    void barrier::detach()
    {
        comm_ = hpx::collectives::communicator{};
    }

    barrier barrier::create_global_barrier()
    {
        runtime& rt = get_runtime();
        util::runtime_configuration const& cfg = rt.get_config();
        auto const num = static_cast<std::size_t>(cfg.get_num_localities());
        auto const rank = static_cast<std::size_t>(hpx::get_locality_id());
        return barrier("/0/hpx/global_barrier", num, rank, force_flat_tag::tag);
    }

    barrier& barrier::get_global_barrier()
    {
        static barrier b;
        return b;
    }

    void barrier::synchronize()
    {
        barrier& b = get_global_barrier();
        HPX_ASSERT(
            !std::holds_alternative<hpx::collectives::communicator>(b.comm_) ||
            std::get<hpx::collectives::communicator>(b.comm_).valid());
        b.wait();
    }
}    // namespace hpx::distributed

#endif    // !HPX_COMPUTE_DEVICE_CODE
