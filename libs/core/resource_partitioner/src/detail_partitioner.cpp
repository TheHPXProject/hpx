//  Copyright (c) 2017 Shoshana Jakobovits
//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/thread_pools.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/resource_partitioner/detail/partitioner.hpp>
#include <hpx/resource_partitioner/partitioner.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace hpx::resource {

    char const* get_scheduler_name(scheduling_policy const policy)
    {
        switch (policy)
        {
        case resource::scheduling_policy::unspecified:
            return "unspecified";
        case resource::scheduling_policy::user_defined:
            return "user supplied";
        case resource::scheduling_policy::local:
            return "local";
        case resource::scheduling_policy::local_priority_fifo:
            return "local_priority_fifo";
        case resource::scheduling_policy::local_priority_fifo_double:
            return "local_priority_fifo_double";
        case resource::scheduling_policy::local_priority_lifo:
            return "local_priority_lifo";
#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
        case resource::scheduling_policy::local_workrequesting_fifo:
            return "local_workrequesting_fifo";
        case resource::scheduling_policy::local_workrequesting_lifo:
            return "local_workrequesting_lifo";
        case resource::scheduling_policy::local_workrequesting_mc:
            return "local_workrequesting_mc";
#else
        case resource::scheduling_policy::local_workrequesting_fifo:
        case resource::scheduling_policy::local_workrequesting_lifo:
        case resource::scheduling_policy::local_workrequesting_mc:
            return "unknown";
#endif
        case resource::scheduling_policy::static_:
            return "static";
        case resource::scheduling_policy::static_priority:
            return "static_priority";
        case resource::scheduling_policy::abp_priority_fifo:
            return "abp_priority_fifo";
        case resource::scheduling_policy::abp_priority_lifo:
            return "abp_priority_lifo";
        case resource::scheduling_policy::shared_priority:
            return "shared_priority";
        default:
            break;
        }
        return "unknown";
    }

    scheduling_policy get_scheduling_policy(char const* scheduler_name)
    {
        if (scheduler_name == nullptr || *scheduler_name == '\0')
        {
            return scheduling_policy::unspecified;
        }

        if (0 == std::string("local").find(scheduler_name))
        {
            return scheduling_policy::local;
        }
        if (0 == std::string("local-priority-fifo").find(scheduler_name))
        {
            return scheduling_policy::local_priority_fifo;
        }
        if (0 == std::string("local-priority-fifo-double").find(scheduler_name))
        {
            return scheduling_policy::local_priority_fifo_double;
        }
        if (0 == std::string("local-priority-lifo").find(scheduler_name))
        {
            return scheduling_policy::local_priority_lifo;
        }
#if defined(HPX_HAVE_WORK_REQUESTING_SCHEDULERS)
        if (0 == std::string("local-workrequesting-fifo").find(scheduler_name))
        {
            return scheduling_policy::local_workrequesting_fifo;
        }
        if (0 == std::string("local-workrequesting-lifo").find(scheduler_name))
        {
            return scheduling_policy::local_workrequesting_lifo;
        }
        if (0 == std::string("local-workrequesting-mc").find(scheduler_name))
        {
            return scheduling_policy::local_workrequesting_mc;
        }
#endif
        if (0 == std::string("static").find(scheduler_name))
        {
            return scheduling_policy::static_;
        }
        if (0 == std::string("static-priority").find(scheduler_name))
        {
            return scheduling_policy::static_priority;
        }
        if (0 == std::string("abp-priority-fifo").find(scheduler_name))
        {
            return scheduling_policy::abp_priority_fifo;
        }
        if (0 == std::string("abp-priority-lifo").find(scheduler_name))
        {
            return scheduling_policy::abp_priority_lifo;
        }
        if (0 == std::string("shared-priority").find(scheduler_name))
        {
            return scheduling_policy::shared_priority;
        }
        return scheduling_policy::unspecified;
    }
}    // namespace hpx::resource

namespace hpx::resource::detail {

    ///////////////////////////////////////////////////////////////////////////
    [[noreturn]] static void throw_runtime_error(
        std::string const& func, std::string const& message)
    {
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, func, message);
    }

    [[noreturn]] static void throw_invalid_argument(
        std::string const& func, std::string const& message)
    {
        HPX_THROW_EXCEPTION(hpx::error::bad_parameter, func, message);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::size_t init_pool_data::num_threads_overall = 0;

    init_pool_data::init_pool_data(std::string const& name,
        scheduling_policy sched, hpx::threads::policies::scheduler_mode mode,
        background_work_function func)
      : pool_name_(name)
      , scheduling_policy_(sched)
      , num_threads_(0)
      , mode_(mode)
      , background_work_(HPX_MOVE(func))
    {
        if (name.empty())
        {
            throw_invalid_argument("init_pool_data::init_pool_data",
                "cannot instantiate a thread_pool with empty string as a "
                "name.");
        }
    }

    init_pool_data::init_pool_data(std::string const& name,
        scheduler_function create_func,
        hpx::threads::policies::scheduler_mode mode,
        background_work_function func)
      : pool_name_(name)
      , scheduling_policy_(scheduling_policy::user_defined)
      , num_threads_(0)
      , mode_(mode)
      , create_function_(HPX_MOVE(create_func))
      , background_work_(HPX_MOVE(func))
    {
        if (name.empty())
        {
            throw_invalid_argument("init_pool_data::init_pool_data",
                "cannot instantiate a thread_pool with empty string "
                "as a name.");
        }
    }

    // mechanism for adding resources
    // num threads = number of threads desired on a PU. defaults to 1.
    // note: if num_threads > 1 => over-subscription
    void init_pool_data::add_resource(
        std::size_t pu_index, bool exclusive, std::size_t num_threads)
    {
        if (pu_index >=
            static_cast<std::size_t>(hpx::threads::hardware_concurrency()))
        {
            throw_invalid_argument("init_pool_data::add_resource",
                "init_pool_data::add_resource: processing unit index "
                "out of bounds. The total available number of "
                "processing units on this machine is " +
                    std::to_string(hpx::threads::hardware_concurrency()));
        }

        // Increment thread_num count (for pool-count and global count)
        num_threads_ += num_threads;
        num_threads_overall += num_threads;

        // Add pu mask to internal data structure
        threads::mask_type pu_mask = threads::mask_type();
        threads::resize(
            pu_mask, static_cast<std::size_t>(threads::hardware_concurrency()));
        threads::set(pu_mask, pu_index);

        // Add one mask for each OS-thread
        for (std::size_t i = 0; i != num_threads; i++)
        {
            assigned_pus_.push_back(pu_mask);
            assigned_pu_nums_.emplace_back(pu_index, exclusive, false);
        }
    }

    void init_pool_data::print_pool(std::ostream& os) const
    {
        os << "[pool \"" << pool_name_ << "\"] with scheduler \""
           << get_scheduler_name(scheduling_policy_)
           << "\" is running on PUs : \n";

        for (threads::mask_cref_type assigned_pu : assigned_pus_)
        {
            os << hpx::threads::to_string(assigned_pu) << '\n';
        }
    }

    void init_pool_data::assign_pu(std::size_t virt_core)
    {
        HPX_ASSERT(virt_core <= assigned_pu_nums_.size());
        HPX_ASSERT(!hpx::get<2>(assigned_pu_nums_[virt_core]));

        hpx::get<2>(assigned_pu_nums_[virt_core]) =    //-V688 //-V808
            true;                                      // -V601
    }

    void init_pool_data::unassign_pu(std::size_t virt_core)
    {
        HPX_ASSERT(virt_core <= assigned_pu_nums_.size());
        HPX_ASSERT(hpx::get<2>(assigned_pu_nums_[virt_core]));

        hpx::get<2>(assigned_pu_nums_[virt_core])    //-V688 //-V808
            = false;                                 // -V601
    }

    bool init_pool_data::pu_is_exclusive(std::size_t virt_core) const
    {
        HPX_ASSERT(virt_core <= assigned_pu_nums_.size());
        HPX_ASSERT(hpx::get<2>(assigned_pu_nums_[virt_core]));

        return hpx::get<1>(assigned_pu_nums_[virt_core]);
    }

    bool init_pool_data::pu_is_assigned(std::size_t virt_core) const
    {
        HPX_ASSERT(virt_core <= assigned_pu_nums_.size());
        HPX_ASSERT(hpx::get<2>(assigned_pu_nums_[virt_core]));

        return hpx::get<2>(assigned_pu_nums_[virt_core]);
    }

    // 'shift' all thread assignments up by the first_core offset
    void init_pool_data::assign_first_core(std::size_t first_core)
    {
        for (std::size_t i = 0; i != num_threads_; ++i)
        {
            std::size_t& pu_num = hpx::get<0>(assigned_pu_nums_[i]);
            pu_num = (pu_num + first_core) %
                static_cast<std::size_t>(threads::hardware_concurrency());

            threads::reset(assigned_pus_[i]);
            threads::set(assigned_pus_[i], pu_num);
        }
    }

    hpx::threads::mask_type init_pool_data::get_pu_mask() const
    {
        threads::mask_type pu_mask = threads::mask_type();
        threads::resize(
            pu_mask, static_cast<std::size_t>(threads::hardware_concurrency()));

        for (std::size_t i = 0; i != num_threads_; ++i)
        {
            pu_mask |= assigned_pus_[i];
        }

        return pu_mask;
    }

    ////////////////////////////////////////////////////////////////////////
    partitioner::partitioner()
      : first_core_(static_cast<std::size_t>(-1))
      , pus_needed_(static_cast<std::size_t>(-1))
      , mtx_("resource_partitioner")
      , mode_(partitioner_mode::default_)
      , topo_(threads::create_topology())
      , default_scheduler_mode_(threads::policies::scheduler_mode::default_)
    {
        // allow only one partitioner instance
        if (++instance_number_counter_ > 1)
        {
            throw_runtime_error("partitioner::partitioner",
                "Cannot instantiate more than one resource partitioner");
        }

#if defined(HPX_HAVE_MAX_CPU_COUNT)
        if (HPX_HAVE_MAX_CPU_COUNT < topo_.get_number_of_pus())
        {
            throw_runtime_error("partitioner::partitioner",
                hpx::util::format(
                    "Currently, HPX_HAVE_MAX_CPU_COUNT is set to {1} "
                    "while your system has {2} processing units. Please "
                    "reconfigure HPX with -DHPX_WITH_MAX_CPU_COUNT={2} (or "
                    "higher) to increase the maximal CPU count supported by "
                    "HPX.",
                    HPX_HAVE_MAX_CPU_COUNT, topo_.get_number_of_pus()));
        }
#endif

        std::string const default_scheduler_mode_str =
            rtcfg_.get_entry("hpx.default_scheduler_mode", std::string());
        if (!default_scheduler_mode_str.empty())
        {
            default_scheduler_mode_ =
                static_cast<threads::policies::scheduler_mode>(
                    hpx::util::from_string<std::size_t>(
                        default_scheduler_mode_str));
            HPX_ASSERT_MSG(
                (default_scheduler_mode_ &
                    ~threads::policies::scheduler_mode::all_flags) == 0,
                "hpx.default_scheduler_mode contains unknown scheduler "
                "modes");
        }

        // Create the default pool
        initial_thread_pools_.emplace_back(
            "default", scheduling_policy::unspecified, default_scheduler_mode_);
    }

    partitioner::~partitioner()
    {
        --instance_number_counter_;
        detail::init_pool_data::num_threads_overall = 0;
    }

    bool partitioner::pu_exposed(std::size_t pu_num) const
    {
        threads::mask_type pu_mask = threads::mask_type();
        threads::resize(
            pu_mask, static_cast<std::size_t>(threads::hardware_concurrency()));
        threads::set(pu_mask, pu_num);

        threads::topology const& topo = get_topology();
        threads::mask_type const comp =
            affinity_data_.get_used_pus_mask(topo, pu_num);
        return threads::any(comp & pu_mask);
    }

    void partitioner::fill_topology_vectors()
    {
        threads::topology const& topo = get_topology();

        std::size_t pid = 0;
        std::size_t num_numa_nodes = topo.get_number_of_numa_nodes();
        if (num_numa_nodes == 0)
            num_numa_nodes = topo.get_number_of_sockets();
        numa_domains_.reserve(num_numa_nodes);

        // loop on the numa-domains
        for (std::size_t i = 0; i != num_numa_nodes; ++i)
        {
            numa_domains_.emplace_back(i);             // add a numa domain
            numa_domain& nd = numa_domains_.back();    // numa-domain just added

            std::size_t const numa_node_cores =
                topo.get_number_of_numa_node_cores(i);
            nd.cores_.reserve(numa_node_cores);

            bool numa_domain_contains_exposed_cores = false;

            // loop on the cores
            for (std::size_t j = 0; j != numa_node_cores; ++j)
            {
                nd.cores_.emplace_back(j, &nd);
                core& c = nd.cores_.back();

                std::size_t const core_pus = topo.get_number_of_core_pus(j);
                c.pus_.reserve(core_pus);

                bool core_contains_exposed_pus = false;

                // loop on the processing units
                for (std::size_t k = 0; k != core_pus; ++k)
                {
                    if (pu_exposed(pid))
                    {
                        c.pus_.emplace_back(pid, &c,
                            affinity_data_.get_thread_occupancy(topo, pid));
                        pu const& p = c.pus_.back();

                        if (p.thread_occupancy_ == 0)
                        {
                            throw_runtime_error(
                                "partitioner::fill_topology_vectors",
                                "PU #" + std::to_string(pid) +
                                    " has thread occupancy 0");
                        }
                        core_contains_exposed_pus = true;
                    }
                    ++pid;
                }

                if (core_contains_exposed_pus)
                {
                    numa_domain_contains_exposed_cores = true;
                }
                else
                {
                    nd.cores_.pop_back();
                }
            }

            if (!numa_domain_contains_exposed_cores)
            {
                numa_domains_.pop_back();
            }
        }
    }

    std::size_t partitioner::assign_cores(std::size_t first_core)
    {
        std::lock_guard<mutex_type> l(mtx_);

        // adjust first_core, if needed
        if (first_core_ != first_core)
        {
            std::size_t offset = first_core;
            std::size_t const num_pus_core =
                get_topology().get_number_of_core_pus(offset);

            if (first_core_ != static_cast<std::size_t>(-1))
            {
                offset -= first_core_;
            }

            if (offset != 0)
            {
                offset *= num_pus_core;
                for (auto& d : initial_thread_pools_)
                {
                    d.assign_first_core(offset);
                }
            }
            first_core_ = first_core;
            reconfigure_affinities_locked();
        }

        return threads_needed();
    }

    std::size_t partitioner::threads_needed() noexcept
    {
        if (pus_needed_ == static_cast<std::size_t>(-1))
        {
            pus_needed_ = affinity_data_.get_num_pus_needed();
            HPX_ASSERT(pus_needed_ != static_cast<std::size_t>(-1));
        }
        return pus_needed_;
    }

    // This function is called in hpx_init, before the instantiation of the
    // runtime It takes care of configuring some internal parameters of the
    // resource partitioner related to the pools
    // -1 assigns all free resources to the default pool
    // -2 checks whether there are empty pools
    void partitioner::setup_pools()
    {
        // Assign all free resources to the default pool
        bool first = true;
        for (hpx::resource::numa_domain& d : numa_domains_)
        {
            for (hpx::resource::core& c : d.cores_)
            {
                for (hpx::resource::pu& p : c.pus_)
                {
                    if (p.thread_occupancy_count_ == 0)
                    {
                        // The default pool resources are assigned non-
                        // exclusively if dynamic pools are enabled. Also, by
                        // default, the first PU is always exclusive (to avoid
                        // deadlocks).
                        add_resource(p, get_default_pool_name(),
                            first ||
                                !as_bool(mode_ &
                                    partitioner_mode::allow_dynamic_pools));
                        first = false;
                    }
                }
            }
        }

        std::unique_lock<mutex_type> l(mtx_);

        // @TODO allow empty pools
        if (get_pool_data(l, get_default_pool_name()).num_threads_ == 0)
        {
            l.unlock();
            throw_runtime_error("partitioner::setup_pools",
                "Default pool " + get_default_pool_name() +
                    " has no threads assigned. Please rerun with "
                    "--hpx:threads=X and check the pool thread assignment");
        }

        // Check whether any of the pools defined up to now are empty
        if (check_empty_pools())
        {
            l.unlock();
            print_init_pool_data(std::cout);
            throw_runtime_error("partitioner::setup_pools",
                "Pools empty of resources are not allowed. Please re-run this "
                "application with allow-empty-pool-policy (not implemented "
                "yet)");
        }
        //! FIXME add allow-empty-pools policy. Wait, does this even make sense??
    }

    // This function is called in hpx_init, before the instantiation of the
    // runtime It takes care of configuring some internal parameters of the
    // resource partitioner related to the pools' schedulers
    void partitioner::setup_schedulers()
    {
        // select the default scheduler
        std::string const default_scheduler_str =
            rtcfg_.get_entry("hpx.scheduler", std::string());

        scheduling_policy default_scheduler =
            get_scheduling_policy(default_scheduler_str.c_str());
        if (default_scheduler == scheduling_policy::unspecified)
        {
            throw hpx::detail::command_line_error(
                std::string(
                    "Bad value for command line option --hpx:queuing: ") +
                default_scheduler_str);
        }

        // set this scheduler on the pools that do not have a specified scheduler yet
        std::lock_guard<mutex_type> l(mtx_);
        std::size_t const num_pools = initial_thread_pools_.size();
        for (std::size_t i = 0; i != num_pools; ++i)
        {
            if (initial_thread_pools_[i].scheduling_policy_ ==
                scheduling_policy::unspecified)
            {
                initial_thread_pools_[i].scheduling_policy_ = default_scheduler;
            }
        }
    }

    // This function is called in hpx_init, before the instantiation of the
    // runtime. It takes care of configuring some internal parameters of the
    // resource partitioner related to the affinity bindings
    //
    // If we use the resource partitioner, OS-thread numbering gets slightly
    // complicated: The affinity_masks_ data member of affinity_data considers
    // OS-threads to be numbered in order of occupation of the consecutive
    // processing units, while the thread manager will consider them to be
    // ordered according to their assignment to pools (first all threads
    // belonging to the default pool, then all threads belonging to the first
    // pool created, etc.) and instantiate them according to this system. We
    // need to re-write affinity_data_ with the masks in the correct order at
    // this stage.
    void partitioner::reconfigure_affinities()
    {
        std::lock_guard<mutex_type> l(mtx_);
        reconfigure_affinities_locked();
    }

    void partitioner::reconfigure_affinities_locked()
    {
        std::vector<std::size_t> new_pu_nums;
        std::vector<threads::mask_type> new_affinity_masks;

        new_pu_nums.reserve(initial_thread_pools_.size());
        new_affinity_masks.reserve(initial_thread_pools_.size());

        {
            for (auto& itp : initial_thread_pools_)
            {
                for (auto const& mask : itp.assigned_pus_)
                {
                    new_affinity_masks.push_back(mask);
                }
                for (auto const& pu_num : itp.assigned_pu_nums_)
                {
                    new_pu_nums.push_back(hpx::get<0>(pu_num));
                }
            }
        }

        affinity_data_.set_num_threads(new_pu_nums.size());
        affinity_data_.set_pu_nums(HPX_MOVE(new_pu_nums));
        affinity_data_.set_affinity_masks(HPX_MOVE(new_affinity_masks));
    }

    // Returns true if any of the pools defined by the user is empty of
    // resources called in set_default_pool()
    bool partitioner::check_empty_pools() const
    {
        std::size_t const num_thread_pools = initial_thread_pools_.size();

        for (std::size_t i = 0; i != num_thread_pools; i++)
        {
            if (initial_thread_pools_[i].assigned_pus_.empty())
            {
                return true;
            }
            for (auto const& assigned_pus :
                initial_thread_pools_[i].assigned_pus_)
            {
                if (!threads::any(assigned_pus))
                {
                    return true;
                }
            }
        }

        return false;
    }

    // create a new thread_pool
    void partitioner::create_thread_pool(std::string const& pool_name,
        scheduling_policy sched, hpx::threads::policies::scheduler_mode mode,
        background_work_function func)
    {
        if (pool_name.empty())
        {
            throw std::invalid_argument(
                "partitioner::create_thread_pool: "
                "cannot instantiate a initial_thread_pool with empty string "
                "as a name.");
        }

        std::unique_lock<mutex_type> l(mtx_);

        if (pool_name == get_default_pool_name())
        {
            initial_thread_pools_[0] = detail::init_pool_data(
                get_default_pool_name(), sched, mode, HPX_MOVE(func));
            return;
        }

        //! if there already exists a pool with this name
        std::size_t const num_thread_pools = initial_thread_pools_.size();
        for (std::size_t i = 1; i < num_thread_pools; i++)
        {
            if (pool_name == initial_thread_pools_[i].pool_name_)
            {
                l.unlock();
                throw std::invalid_argument(
                    "partitioner::create_thread_pool: "
                    "there already exists a pool named '" +
                    pool_name + "'.\n");
            }
        }

        initial_thread_pools_.emplace_back(
            pool_name, sched, mode, HPX_MOVE(func));
    }

    // create a new thread_pool
    void partitioner::create_thread_pool(std::string const& pool_name,
        scheduler_function scheduler_creation, background_work_function func)
    {
        if (pool_name.empty())
        {
            throw std::invalid_argument(
                "partitioner::create_thread_pool: "
                "cannot instantiate a initial_thread_pool with empty string "
                "as a name.");
        }

        std::unique_lock<mutex_type> l(mtx_);

        if (pool_name == get_default_pool_name())
        {
            initial_thread_pools_[0] = detail::init_pool_data(
                get_default_pool_name(), HPX_MOVE(scheduler_creation),
                default_scheduler_mode_, HPX_MOVE(func));
            return;
        }

        //! if there already exists a pool with this name
        std::size_t const num_thread_pools = initial_thread_pools_.size();
        for (std::size_t i = 1; i != num_thread_pools; ++i)
        {
            if (pool_name == initial_thread_pools_[i].pool_name_)
            {
                l.unlock();
                throw std::invalid_argument(
                    "partitioner::create_thread_pool: "
                    "there already exists a pool named '" +
                    pool_name + "'.\n");
            }
        }

        initial_thread_pools_.emplace_back(pool_name,
            HPX_MOVE(scheduler_creation), default_scheduler_mode_,
            HPX_MOVE(func));
    }

    // ----------------------------------------------------------------------
    // Add processing units to pools via pu/core/domain api
    // ----------------------------------------------------------------------
    void partitioner::add_resource(pu const& p, std::string const& pool_name,
        bool exclusive, std::size_t num_threads)
    {
        std::unique_lock<mutex_type> l(mtx_);

        if (!exclusive &&
            !as_bool(mode_ & partitioner_mode::allow_dynamic_pools))
        {
            l.unlock();
            throw std::invalid_argument(
                "partitioner::add_resource: dynamic pools have not been "
                "enabled for this partitioner");
        }

        if (as_bool(mode_ & partitioner_mode::allow_oversubscription))
        {
            // increment occupancy counter
            get_pool_data(l, pool_name)
                .add_resource(p.id_, exclusive, num_threads);
            ++p.thread_occupancy_count_;
            return;
        }

        // check occupancy counter and increment it
        if (p.thread_occupancy_count_ == 0)
        {
            get_pool_data(l, pool_name)
                .add_resource(p.id_, exclusive, num_threads);
            ++p.thread_occupancy_count_;

            // Make sure the total number of requested threads does not exceed
            // the number of threads requested on the command line
            std::size_t const num_os_threads =
                util::get_entry_as<std::size_t>(rtcfg_, "hpx.os_threads", 0);
            HPX_ASSERT(num_os_threads != 0);

            if (detail::init_pool_data::num_threads_overall > num_os_threads)
            {
                l.unlock();
                throw std::runtime_error("partitioner::add_resource: "
                                         "Creation of " +
                    std::to_string(
                        detail::init_pool_data::num_threads_overall) +
                    " threads requested by the resource partitioner, but "
                    "only " +
                    std::to_string(num_os_threads) +
                    " provided on the command-line.");
            }
        }
        else
        {
            l.unlock();
            throw std::runtime_error("partitioner::add_resource: "
                                     "PU #" +
                std::to_string(p.id_) + " can be assigned only " +
                std::to_string(p.thread_occupancy_) +
                " threads according to affinity bindings.");
        }
    }

    void partitioner::add_resource(
        std::vector<pu> const& pv, std::string const& pool_name, bool exclusive)
    {
        for (pu const& p : pv)
        {
            add_resource(p, pool_name, exclusive);
        }
    }

    void partitioner::add_resource(
        core const& c, std::string const& pool_name, bool exclusive)
    {
        add_resource(c.pus_, pool_name, exclusive);
    }

    void partitioner::add_resource(std::vector<core> const& cv,
        std::string const& pool_name, bool exclusive)
    {
        for (core const& c : cv)
        {
            add_resource(c.pus_, pool_name, exclusive);
        }
    }

    void partitioner::add_resource(
        numa_domain const& nd, std::string const& pool_name, bool exclusive)
    {
        add_resource(nd.cores_, pool_name, exclusive);
    }

    void partitioner::add_resource(std::vector<numa_domain> const& ndv,
        std::string const& pool_name, bool exclusive)
    {
        for (numa_domain const& d : ndv)
        {
            add_resource(d, pool_name, exclusive);
        }
    }

    void partitioner::set_scheduler(
        scheduling_policy sched, std::string const& pool_name)
    {
        std::unique_lock<mutex_type> l(mtx_);
        get_pool_data(l, pool_name).scheduling_policy_ = sched;
    }

    void partitioner::configure_pools()
    {
        setup_pools();
        setup_schedulers();
        reconfigure_affinities();

        is_initialized_ = true;
    }

    ////////////////////////////////////////////////////////////////////////
    // this function is called in the constructor of thread_pool
    // returns a scheduler (moved) that thread pool should have as a data member
    scheduling_policy partitioner::which_scheduler(std::string const& pool_name)
    {
        std::unique_lock<mutex_type> l(mtx_);

        // look up which scheduler is needed
        scheduling_policy const sched_type =
            get_pool_data(l, pool_name).scheduling_policy_;
        if (sched_type == scheduling_policy::unspecified)
        {
            l.unlock();
            throw std::invalid_argument(
                "partitioner::which_scheduler: Thread pool " + pool_name +
                " cannot be instantiated with unspecified scheduler type.");
        }
        return sched_type;
    }

    threads::topology& partitioner::get_topology() const noexcept
    {
        return topo_;
    }

    std::size_t partitioner::get_num_threads() const
    {
        std::size_t num_threads = 0;

        {
            std::unique_lock<mutex_type> l(mtx_);
            std::size_t const num_thread_pools = initial_thread_pools_.size();
            for (size_t i = 0; i != num_thread_pools; ++i)
            {
                num_threads += get_pool_data(l, i).num_threads_;
            }
        }

        // the number of allocated threads should be the same as the number of
        // threads to create (if no over-subscription is allowed)
        HPX_ASSERT(as_bool(mode_ & partitioner_mode::allow_oversubscription) ||
            num_threads ==
                util::get_entry_as<std::size_t>(
                    rtcfg_, "hpx.os_threads", static_cast<std::size_t>(-1)));

        return num_threads;
    }

    std::size_t partitioner::get_num_pools() const noexcept
    {
        std::lock_guard<mutex_type> l(mtx_);
        return initial_thread_pools_.size();
    }

    std::size_t partitioner::get_num_threads(std::size_t pool_index) const
    {
        std::unique_lock<mutex_type> l(mtx_);
        return get_pool_data(l, pool_index).num_threads_;
    }

    std::size_t partitioner::get_num_threads(std::string const& pool_name) const
    {
        std::unique_lock<mutex_type> l(mtx_);
        return get_pool_data(l, pool_name).num_threads_;
    }

    hpx::threads::policies::scheduler_mode partitioner::get_scheduler_mode(
        std::size_t pool_index) const
    {
        std::unique_lock<mutex_type> l(mtx_);
        return get_pool_data(l, pool_index).mode_;
    }

    background_work_function partitioner::get_background_work(
        std::size_t pool_index) const
    {
        std::unique_lock<mutex_type> l(mtx_);
        return get_pool_data(l, pool_index).background_work_;
    }

    detail::init_pool_data const& partitioner::get_pool_data(
        std::unique_lock<mutex_type>& l, std::size_t pool_index) const
    {
        if (pool_index >= initial_thread_pools_.size())
        {
            l.unlock();
            throw_invalid_argument("partitioner::get_pool_data",
                "pool index " + std::to_string(pool_index) +
                    " too large: the resource partitioner owns only " +
                    std::to_string(initial_thread_pools_.size()) +
                    " thread pools.");
        }
        return initial_thread_pools_[pool_index];
    }

    std::string const& partitioner::get_pool_name(std::size_t index) const
    {
        if (index >= initial_thread_pools_.size())
        {
            throw_invalid_argument("partitioner::get_pool_name: ",
                "pool " + std::to_string(index) +
                    " (zero-based index) requested out of bounds. The "
                    "partitioner owns only " +
                    std::to_string(initial_thread_pools_.size()) + " pools");
        }
        return initial_thread_pools_[index].pool_name_;
    }

    std::size_t partitioner::get_pu_num(std::size_t global_thread_num) const
    {
        // protect against stand-alone use of schedulers
        if (is_initialized_)
        {
            return affinity_data_.get_pu_num(global_thread_num);
        }
        return global_thread_num;
    }

    std::size_t partitioner::get_thread_occupancy(std::size_t pu_num) const
    {
        return affinity_data_.get_thread_occupancy(topo_, pu_num);
    }

    threads::mask_type partitioner::get_used_pus_mask(std::size_t pu_num) const
    {
        if (is_initialized_)
        {
            return affinity_data_.get_used_pus_mask(topo_, pu_num);
        }

        auto mask = hpx::threads::mask_type();
        hpx::threads::resize(mask,
            static_cast<std::size_t>(hpx::threads::hardware_concurrency()));
        threads::set(mask, pu_num);
        return mask;
    }

    threads::mask_type partitioner::get_pu_mask(
        std::size_t global_thread_num) const
    {
        if (is_initialized_)
        {
            return affinity_data_.get_pu_mask(topo_, global_thread_num);
        }

        auto mask = hpx::threads::mask_type();
        hpx::threads::resize(mask,
            static_cast<std::size_t>(hpx::threads::hardware_concurrency()));
        threads::set(mask, global_thread_num);
        return mask;
    }

    threads::mask_type partitioner::get_pool_pus_mask(
        std::string const& pool_name) const
    {
        std::unique_lock<mutex_type> l(mtx_);
        return get_pool_data(l, pool_name).get_pu_mask();
    }

    void partitioner::init(resource::partitioner_mode rpmode,
        hpx::util::section const& rtcfg,
        hpx::threads::policies::detail::affinity_data const& affinity_data)
    {
        mode_ = rpmode;
        rtcfg_ = rtcfg;
        affinity_data_ = affinity_data;

        fill_topology_vectors();

        pus_needed_ = assign_cores(0);
    }

    scheduler_function partitioner::get_pool_creator(std::size_t index) const
    {
        std::unique_lock<mutex_type> l(mtx_);
        if (index >= initial_thread_pools_.size())
        {
            l.unlock();
            throw std::invalid_argument(
                "partitioner::get_pool_creator: pool requested out of bounds.");
        }
        return get_pool_data(l, index).create_function_;
    }

    ///////////////////////////////////////////////////////////////////////////
    void partitioner::assign_pu(
        std::string const& pool_name, std::size_t virt_core)
    {
        std::unique_lock<mutex_type> l(mtx_);
        detail::init_pool_data& data = get_pool_data(l, pool_name);
        data.assign_pu(virt_core);
    }

    void partitioner::unassign_pu(
        std::string const& pool_name, std::size_t virt_core)
    {
        std::unique_lock<mutex_type> l(mtx_, std::defer_lock);
        if (l.try_lock())
        {
            detail::init_pool_data& data = get_pool_data(l, pool_name);
            data.unassign_pu(virt_core);
        }
    }

    std::size_t partitioner::shrink_pool(std::string const& pool_name,
        hpx::function<void(std::size_t)> const& remove_pu)
    {
        if (!as_bool(mode_ & partitioner_mode::allow_dynamic_pools))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "partitioner::shrink_pool",
                "dynamic pools have not been enabled for the partitioner");
        }

        std::vector<std::size_t> pu_nums_to_remove;
        bool has_non_exclusive_pus = false;

        {
            std::unique_lock<mutex_type> l(mtx_);
            detail::init_pool_data const& data = get_pool_data(l, pool_name);

            pu_nums_to_remove.reserve(data.num_threads_);

            for (std::size_t i = 0; i != data.num_threads_; ++i)
            {
                if (!data.pu_is_exclusive(i))
                {
                    has_non_exclusive_pus = true;
                    if (data.pu_is_assigned(i))
                    {
                        pu_nums_to_remove.push_back(i);
                    }
                }
            }
        }

        if (!has_non_exclusive_pus)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "partitioner::shrink_pool",
                "pool '{}' has no non-exclusive pus associated", pool_name);
        }

        for (std::size_t const pu_num : pu_nums_to_remove)
        {
            remove_pu(pu_num);
        }

        return pu_nums_to_remove.size();
    }

    std::size_t partitioner::expand_pool(std::string const& pool_name,
        hpx::function<void(std::size_t)> const& add_pu)
    {
        if (!as_bool(mode_ & partitioner_mode::allow_dynamic_pools))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "partitioner::expand_pool",
                "dynamic pools have not been enabled for the partitioner");
        }

        std::vector<std::size_t> pu_nums_to_add;
        bool has_non_exclusive_pus = false;

        {
            std::unique_lock<mutex_type> l(mtx_);
            detail::init_pool_data const& data = get_pool_data(l, pool_name);

            pu_nums_to_add.reserve(data.num_threads_);

            for (std::size_t i = 0; i != data.num_threads_; ++i)
            {
                if (!data.pu_is_exclusive(i))
                {
                    has_non_exclusive_pus = true;
                    if (!data.pu_is_assigned(i))
                    {
                        pu_nums_to_add.push_back(i);
                    }
                }
            }
        }

        if (!has_non_exclusive_pus)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "partitioner::expand_pool",
                "pool '{}' has no non-exclusive pus associated", pool_name);
        }

        for (std::size_t const pu_num : pu_nums_to_add)
        {
            add_pu(pu_num);
        }

        return pu_nums_to_add.size();
    }

    ////////////////////////////////////////////////////////////////////////
    std::size_t partitioner::get_pool_index(std::string const& pool_name) const
    {
        // the default pool is always index 0, it may be renamed but the user
        // can always ask for "default"
        if (pool_name == "default")
        {
            return 0;
        }

        {
            std::lock_guard<mutex_type> l(mtx_);
            std::size_t const num_pools = initial_thread_pools_.size();
            for (std::size_t i = 0; i < num_pools; i++)
            {
                if (initial_thread_pools_[i].pool_name_ == pool_name)
                {
                    return i;
                }
            }
        }

        throw_invalid_argument("partitioner::get_pool_index",
            "the resource partitioner does not own a thread pool named '" +
                pool_name + "'");
    }

    // has to be private bc pointers become invalid after data member
    // thread_pools_ is resized we don't want to allow the user to use it
    detail::init_pool_data const& partitioner::get_pool_data(
        std::unique_lock<mutex_type>& l, std::string const& pool_name) const
    {
        auto const pool = std::find_if(initial_thread_pools_.begin(),
            initial_thread_pools_.end(),
            [&pool_name](detail::init_pool_data const& itp) -> bool {
                return (itp.pool_name_ == pool_name);
            });

        if (pool != initial_thread_pools_.end())
        {
            return *pool;
        }

        l.unlock();
        throw_invalid_argument("partitioner::get_pool_data",
            "the resource partitioner does not own a thread pool named '" +
                pool_name + "'");
    }

    detail::init_pool_data& partitioner::get_pool_data(
        std::unique_lock<mutex_type>& l, std::string const& pool_name)
    {
        auto const pool = std::find_if(initial_thread_pools_.begin(),
            initial_thread_pools_.end(),
            [&pool_name](detail::init_pool_data const& itp) -> bool {
                return (itp.pool_name_ == pool_name);
            });

        if (pool != initial_thread_pools_.end())
        {
            return *pool;
        }

        l.unlock();
        throw_invalid_argument("partitioner::get_pool_data",
            "the resource partitioner does not own a thread pool named '" +
                pool_name + "'");
    }

    void partitioner::print_init_pool_data(std::ostream& os) const
    {
        std::lock_guard<mutex_type> l(mtx_);

        //! make this prettier
        os << "the resource partitioner owns "
           << static_cast<std::uint64_t>(initial_thread_pools_.size())
           << " pool(s) : \n";    // -V128

        for (auto const& itp : initial_thread_pools_)
        {
            itp.print_pool(os);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    std::atomic<int> partitioner::instance_number_counter_(-1);
}    // namespace hpx::resource::detail
