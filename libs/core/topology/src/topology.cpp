//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2008-2009 Chirag Dekate, Anshul Tandon
//  Copyright (c) 2012-2013 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/topology.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/util.hpp>

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <errno.h>

#include <hwloc.h>

#if HWLOC_API_VERSION < 0x00010b00
#define HWLOC_OBJ_NUMANODE HWLOC_OBJ_NODE
#endif

#if defined(__ANDROID__) && defined(ANDROID)
#include <cpu-features.h>
#endif

#if defined(__bgq__)
#include <hwi/include/bqc/A2_inlines.h>
#endif

#if defined(HPX_HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(_POSIX_VERSION)
#include <sys/resource.h>
#include <sys/syscall.h>
#endif

namespace hpx::threads::detail {

    std::size_t hwloc_hardware_concurrency() noexcept
    {
        try
        {
            threads::topology const& top = threads::create_topology();
            return top.get_number_of_pus();
        }
        catch (...)
        {
            return std::thread::hardware_concurrency();
        }
    }

    void write_to_log(char const* valuename, std::size_t value) noexcept
    {
        LTM_(debug).format("topology: {}: {}", valuename, value);    //-V128
    }

    void write_to_log_mask(char const* valuename, mask_cref_type value)
    {
        LTM_(debug).format(
            "topology: {}: {}", valuename, hpx::threads::to_string(value));
    }

    void write_to_log(
        char const* valuename, std::vector<std::size_t> const& values)
    {
        LTM_(debug).format("topology: {}s, size: {}", valuename, values.size());

        std::size_t i = 0;
        for (std::size_t value : values)
        {
            LTM_(debug).format("topology: {}({}): {}", valuename, i++, value);
        }
    }

    void write_to_log_mask(
        char const* valuename, std::vector<mask_type> const& values)
    {
        LTM_(debug).format("topology: {}s, size: {}", valuename, values.size());

        std::size_t i = 0;
        for (mask_cref_type value : values)
        {
            LTM_(debug).format("topology: {}({}): {}", valuename, i++,
                hpx::threads::to_string(value));
        }
    }

    std::size_t get_index(hwloc_obj_t obj)
    {
        // on Windows logical_index is always -1
        if (obj->logical_index == ~0x0u)
            return static_cast<std::size_t>(obj->os_index);

        return static_cast<std::size_t>(obj->logical_index);
    }

    hwloc_obj_t adjust_node_obj(hwloc_obj_t node) noexcept
    {
#if HWLOC_API_VERSION >= 0x00020000
        // www.open-mpi.org/projects/hwloc/doc/hwloc-v2.0.0-letter.pdf:
        // Starting with hwloc v2.0, NUMA nodes are not in the main tree
        // anymore. They are attached under objects as Memory Children
        // on the side of normal children.
        while (hwloc_obj_type_is_memory(node->type))
            node = node->parent;
        HPX_ASSERT(node);
#endif
        return node;
    }

    ///////////////////////////////////////////////////////////////////////////
    // abstract away memory page size
    std::size_t get_memory_page_size_impl()
    {
#if defined(HPX_WINDOWS)
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        return static_cast<std::size_t>(systemInfo.dwPageSize);
#elif defined(HPX_HAVE_UNISTD_H)
        return sysconf(_SC_PAGE_SIZE);
#else
        return 4096;
#endif
    }
}    // namespace hpx::threads::detail

std::size_t hpx::threads::topology::memory_page_size_ =
    hpx::threads::detail::get_memory_page_size_impl();

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    std::ostream& operator<<(
        std::ostream& os, hpx_hwloc_bitmap_wrapper const* bmp)
    {
        char buffer[256];
        hwloc_bitmap_snprintf(buffer, 256, bmp->bmp_);
        os << buffer;
        return os;
    }

    ///////////////////////////////////////////////////////////////////////////
    mask_type topology::get_service_affinity_mask(
        mask_cref_type used_processing_units, error_code& ec) const
    {
        // We bind the service threads to the first NUMA domain. This is useful
        // as the first NUMA domain is likely to have the PCI controllers etc.
        mask_cref_type machine_mask = this->get_numa_node_affinity_mask(0, ec);
        if (ec || !any(machine_mask))
            return {};

        if (&ec != &throws)
            ec = make_success_code();

        mask_type res = ~used_processing_units & machine_mask;

        return (!any(res)) ? machine_mask : res;
    }

    bool topology::reduce_thread_priority([[maybe_unused]] error_code& ec) const
    {
#ifdef HPX_HAVE_NICE_THREADLEVEL
#if defined(__linux__) && !defined(__ANDROID__) && !defined(__bgq__)
        pid_t tid;
        tid = syscall(SYS_gettid);
        if (setpriority(PRIO_PROCESS, tid, 19))
        {
            HPX_THROWS_IF(ec, hpx::error::no_success,
                "topology::reduce_thread_priority",
                "setpriority returned an error");
            return false;
        }
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__)

        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST))
        {
            HPX_THROWS_IF(ec, hpx::error::no_success,
                "topology::reduce_thread_priority",
                "SetThreadPriority returned an error");
            return false;
        }
#elif defined(__bgq__)
        ThreadPriority_Low();
#endif
#endif
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
#if !defined(HPX_HAVE_MAX_CPU_COUNT)
    mask_type topology::empty_mask =
        mask_type(static_cast<std::size_t>(hardware_concurrency()));
#else
    mask_type topology::empty_mask = mask_type();
#endif

    topology::topology()
    {
        int err = hwloc_topology_init(&topo);
        if (err != 0)
        {
            HPX_THROW_EXCEPTION(hpx::error::no_success, "topology::topology",
                "Failed to init hwloc topology");
        }

#if HWLOC_API_VERSION >= 0x00020000
#if defined(HPX_TOPOLOGY_HAVE_ADDITIONAL_HWLOC_TESTING)
        // Enable HWLOC filtering that makes it report no cores. This is purely
        // an option allowing to test whether things work properly on systems
        // that may not report cores in the topology at all (e.g. FreeBSD).
        err = hwloc_topology_set_type_filter(
            topo, HWLOC_OBJ_CORE, HWLOC_TYPE_FILTER_KEEP_NONE);
        if (err != 0)
        {
            HPX_THROW_EXCEPTION(hpx::error::no_success, "topology::topology",
                "Failed to set core filter for hwloc topology");
        }
#endif
#endif

        err = hwloc_topology_load(topo);
        if (err != 0)
        {
            HPX_THROW_EXCEPTION(hpx::error::no_success, "topology::topology",
                "Failed to load hwloc topology");
        }

        init_num_of_pus();

        socket_numbers_.reserve(num_of_pus_);
        numa_node_numbers_.reserve(num_of_pus_);
        core_numbers_.reserve(num_of_pus_);

        // Initialize each set of data entirely, as some of the initialization
        // routines rely on access to other pieces of topology data. The
        // compiler will optimize the loops where possible anyways.

        std::size_t num_of_sockets = get_number_of_sockets();
        if (num_of_sockets == 0)
            num_of_sockets = 1;

        for (std::size_t i = 0; i < num_of_pus_; ++i)
        {
            std::size_t socket = init_socket_number(i);
            HPX_ASSERT(socket < num_of_sockets);
            socket_numbers_.push_back(socket);
        }

        std::size_t num_of_nodes = get_number_of_numa_nodes();
        if (num_of_nodes == 0)
            num_of_nodes = 1;

        for (std::size_t i = 0; i < num_of_pus_; ++i)
        {
            std::size_t numa_node = init_numa_node_number(i);
            HPX_ASSERT(numa_node < num_of_nodes);
            numa_node_numbers_.push_back(numa_node);
        }

        std::size_t num_of_cores = get_number_of_cores();
        if (num_of_cores == 0)
            num_of_cores = 1;

        for (std::size_t i = 0; i < num_of_pus_; ++i)
        {
            std::size_t core_number = init_core_number(i);
            HPX_ASSERT(core_number < num_of_cores);
            core_numbers_.push_back(core_number);
        }

        machine_affinity_mask_ = init_machine_affinity_mask();
        socket_affinity_masks_.reserve(num_of_pus_);
        numa_node_affinity_masks_.reserve(num_of_pus_);
        core_affinity_masks_.reserve(num_of_pus_);
        thread_affinity_masks_.reserve(num_of_pus_);

        for (std::size_t i = 0; i < num_of_pus_; ++i)
        {
            socket_affinity_masks_.emplace_back(init_socket_affinity_mask(i));
        }

        for (std::size_t i = 0; i < num_of_pus_; ++i)
        {
            numa_node_affinity_masks_.emplace_back(
                init_numa_node_affinity_mask(i));
        }

        for (std::size_t i = 0; i < num_of_pus_; ++i)
        {
            core_affinity_masks_.emplace_back(init_core_affinity_mask(i));
        }

        for (std::size_t i = 0; i < num_of_pus_; ++i)
        {
            thread_affinity_masks_.emplace_back(init_thread_affinity_mask(i));
        }

        init_numa_node_distances();
    }

    void topology::init_numa_node_distances()
    {
        std::size_t num_nodes = get_number_of_numa_nodes();
        if (num_nodes == 0)
            num_nodes = 1;

        // Initialise with ACPI SLIT defaults: local = 10, remote = 20.
        // These are the standard baseline values used on systems that do not
        // expose a distance matrix (single-socket VMs, MIC, etc.).
        numa_node_distances_.assign(
            num_nodes, std::vector<std::size_t>(num_nodes, 20u));
        for (std::size_t i = 0; i < num_nodes; ++i)
            numa_node_distances_[i][i] = 10u;

#if HWLOC_API_VERSION >= 0x00020000
        // hwloc 2.x: query the latency distance matrix for NUMA nodes.
        unsigned nr = 1;
        hwloc_distances_s* dist = nullptr;
        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            if (hwloc_distances_get_by_type(topo, HWLOC_OBJ_NUMANODE, &nr,
                    &dist, HWLOC_DISTANCES_KIND_MEANS_LATENCY, 0) < 0)
            {
                dist = nullptr;
            }
        }
        if (dist != nullptr)
        {
            for (unsigned i = 0; i < dist->nbobjs; ++i)
            {
                if (dist->objs[i] == nullptr)
                    continue;
                std::size_t const ni =
                    static_cast<std::size_t>(dist->objs[i]->logical_index);
                for (unsigned j = 0; j < dist->nbobjs; ++j)
                {
                    if (dist->objs[j] == nullptr)
                        continue;
                    std::size_t const nj =
                        static_cast<std::size_t>(dist->objs[j]->logical_index);
                    if (ni < num_nodes && nj < num_nodes)
                    {
                        numa_node_distances_[ni][nj] = static_cast<std::size_t>(
                            dist->values[i * dist->nbobjs + j]);
                    }
                }
            }
            std::unique_lock<mutex_type> lk(topo_mtx);
            hwloc_distances_release(topo, dist);
        }
#endif
    }

    void topology::write_to_log() const
    {
        std::size_t num_of_sockets = get_number_of_sockets();
        if (num_of_sockets == 0)
            num_of_sockets = 1;
        detail::write_to_log("num_sockets", num_of_sockets);

        std::size_t num_of_nodes = get_number_of_numa_nodes();
        if (num_of_nodes == 0)
            num_of_nodes = 1;
        detail::write_to_log("num_of_nodes", num_of_nodes);

        std::size_t num_of_cores = get_number_of_cores();
        if (num_of_cores == 0)
            num_of_cores = 1;
        detail::write_to_log("num_of_cores", num_of_cores);

        detail::write_to_log("num_of_pus", num_of_pus_);

        detail::write_to_log("socket_number", socket_numbers_);
        detail::write_to_log("numa_node_number", numa_node_numbers_);
        detail::write_to_log("core_number", core_numbers_);

        detail::write_to_log_mask(
            "machine_affinity_mask", machine_affinity_mask_);

        detail::write_to_log_mask(
            "socket_affinity_mask", socket_affinity_masks_);
        detail::write_to_log_mask(
            "numa_node_affinity_mask", numa_node_affinity_masks_);
        detail::write_to_log_mask("core_affinity_mask", core_affinity_masks_);
        detail::write_to_log_mask(
            "thread_affinity_mask", thread_affinity_masks_);
    }

    topology::~topology()
    {
        if (topo)
            hwloc_topology_destroy(topo);
    }

    std::size_t topology::get_pu_number(
        std::size_t num_core, std::size_t num_pu, error_code& ec) const
    {
        std::unique_lock<mutex_type> lk(topo_mtx);

        int num_cores = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_CORE);
        bool use_pus = false;

        // If num_cores is smaller 0, we have an error, it should never be zero
        // either to avoid division by zero, we should always have at least one
        // core
        if (num_cores <= 0)
        {
            // on some platforms, hwloc can't report the number of cores (BSD),
            // fall back to report the number of PUs instead
            num_cores = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU);
            if (num_cores <= 0)
            {
                HPX_THROWS_IF(ec, hpx::error::no_success,
                    "topology::hwloc_get_nobjs_by_type",
                    "Failed to get number of cores");
                return static_cast<std::size_t>(-1);
            }
            use_pus = true;
        }
        num_core %= num_cores;    //-V101 //-V104 //-V107

        hwloc_obj_t core_obj;
        if (!use_pus)
        {
            core_obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_CORE, static_cast<unsigned>(num_core));

            num_pu %= core_obj->arity;    //-V101 //-V104
            return static_cast<std::size_t>(
                core_obj->children[num_pu]->logical_index);
        }

        core_obj = hwloc_get_obj_by_type(
            topo, HWLOC_OBJ_PU, static_cast<unsigned>(num_core));

        return static_cast<std::size_t>(core_obj->logical_index);
    }

    ///////////////////////////////////////////////////////////////////////////
    mask_cref_type topology::get_machine_affinity_mask(
        error_code& ec) const noexcept
    {
        if (&ec != &throws)
            ec = make_success_code();

        return machine_affinity_mask_;
    }

    mask_cref_type topology::get_socket_affinity_mask(
        std::size_t num_thread, error_code& ec) const
    {
        if (std::size_t const num_pu = num_thread % num_of_pus_;
            num_pu < socket_affinity_masks_.size())
        {
            if (&ec != &throws)
                ec = make_success_code();

            return socket_affinity_masks_[num_pu];
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "hpx::threads::topology::get_socket_affinity_mask",
            "thread number {1} is out of range", num_thread);
        return empty_mask;
    }

    mask_cref_type topology::get_numa_node_affinity_mask(
        std::size_t num_thread, error_code& ec) const
    {
        if (std::size_t const num_pu = num_thread % num_of_pus_;
            num_pu < numa_node_affinity_masks_.size())
        {
            if (&ec != &throws)
                ec = make_success_code();

            return numa_node_affinity_masks_[num_pu];
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "hpx::threads::topology::get_numa_node_affinity_mask",
            "thread number {1} is out of range", num_thread);
        return empty_mask;
    }

    mask_cref_type topology::get_core_affinity_mask(
        std::size_t num_thread, error_code& ec) const
    {
        if (std::size_t const num_pu = num_thread % num_of_pus_;
            num_pu < core_affinity_masks_.size())
        {
            if (&ec != &throws)
                ec = make_success_code();

            return core_affinity_masks_[num_pu];
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "hpx::threads::topology::get_core_affinity_mask",
            "thread number {1} is out of range", num_thread);
        return empty_mask;
    }

    mask_cref_type topology::get_thread_affinity_mask(
        std::size_t num_thread, error_code& ec) const
    {
        if (std::size_t const num_pu = num_thread % num_of_pus_;
            num_pu < thread_affinity_masks_.size())
        {
            if (&ec != &throws)
                ec = make_success_code();

            return thread_affinity_masks_[num_pu];
        }

        HPX_THROWS_IF(ec, hpx::error::bad_parameter,
            "hpx::threads::topology::get_thread_affinity_mask",
            "thread number {1} is out of range", num_thread);
        return empty_mask;
    }

    ///////////////////////////////////////////////////////////////////////////
    void topology::set_thread_affinity_mask(
        mask_cref_type mask, error_code& ec) const
    {
#if !defined(__APPLE__)
        // setting thread affinities is not supported by OSX
        hwloc_cpuset_t const cpuset = hwloc_bitmap_alloc();

        int const pu_depth = hwloc_get_type_or_below_depth(topo, HWLOC_OBJ_PU);

        auto const size = mask_size(mask);
        for (std::size_t i = 0; i != size; ++i)
        {
            if (test(mask, i))
            {
                hwloc_obj_t const pu_obj = hwloc_get_obj_by_depth(
                    topo, pu_depth, static_cast<unsigned>(i));
                HPX_ASSERT(i == detail::get_index(pu_obj));
                hwloc_bitmap_set(
                    cpuset, static_cast<unsigned int>(pu_obj->os_index));
            }
        }

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            if (hwloc_set_cpubind(
                    topo, cpuset, HWLOC_CPUBIND_STRICT | HWLOC_CPUBIND_THREAD))
            {
                // Strict binding not supported or failed, try weak binding.
                if (hwloc_set_cpubind(topo, cpuset, HWLOC_CPUBIND_THREAD))
                {
                    auto const buffer = std::make_unique<char[]>(1024);

                    hwloc_bitmap_snprintf(buffer.get(), 1024, cpuset);
                    hwloc_bitmap_free(cpuset);

                    HPX_THROWS_IF(ec, hpx::error::kernel_error,
                        "hpx::threads::topology::set_thread_affinity_mask",
                        "failed to set thread affinity mask ({}) for cpuset {}",
                        hpx::threads::to_string(mask), buffer.get());
                    return;
                }
            }
        }
#if defined(__linux) || defined(linux) || defined(__linux__) ||                \
    defined(__FreeBSD__)
        sleep(0);    // Allow the OS to pick up the change.
#endif
        hwloc_bitmap_free(cpuset);
#else
        (void) mask;
#endif    // __APPLE__

        if (&ec != &throws)
            ec = make_success_code();
    }

    ///////////////////////////////////////////////////////////////////////////
    mask_type topology::get_thread_affinity_mask_from_lva(
        void const* lva, error_code& ec) const
    {
        if (&ec != &throws)
            ec = make_success_code();

        hwloc_membind_policy_t policy = ::HWLOC_MEMBIND_DEFAULT;

        {
            hwloc_nodeset_t nodeset = hwloc_bitmap_alloc();
            std::unique_lock<mutex_type> lk(topo_mtx);
#if HWLOC_API_VERSION >= 0x00010b06
            int const ret = hwloc_get_area_membind(
                topo, lva, 1, nodeset, &policy, HWLOC_MEMBIND_BYNODESET);
#else
            int const ret = hwloc_get_area_membind_nodeset(
                topo, lva, 1, nodeset, &policy, 0);
#endif

            if (-1 != ret)
            {
                hwloc_cpuset_t const cpuset = hwloc_bitmap_alloc();
                hwloc_cpuset_from_nodeset(topo, cpuset, nodeset);
                lk.unlock();

                hwloc_bitmap_free(nodeset);

                auto mask = mask_type();
                resize(mask, get_number_of_pus());

                int const pu_depth =
                    hwloc_get_type_or_below_depth(topo, HWLOC_OBJ_PU);
                for (unsigned int i = 0;
                    static_cast<std::size_t>(i) != num_of_pus_; ++i)
                {
                    hwloc_obj_t const pu_obj =
                        hwloc_get_obj_by_depth(topo, pu_depth, i);

                    if (auto const idx =
                            static_cast<unsigned>(pu_obj->os_index);
                        hwloc_bitmap_isset(cpuset, idx) != 0)
                    {
                        set(mask, detail::get_index(pu_obj));
                    }
                }

                hwloc_bitmap_free(cpuset);
                return mask;
            }

            std::string const errstr = std::strerror(errno);

            lk.unlock();
            HPX_THROW_EXCEPTION(hpx::error::no_success,
                "topology::get_thread_affinity_mask_from_lva",
                "failed calling 'hwloc_get_area_membind_nodeset', "
                "reported error: {}",
                errstr);
        }
    }

    std::size_t topology::init_numa_node_number(std::size_t num_thread) const
    {
#if HWLOC_API_VERSION >= 0x00020000
        if (static_cast<std::size_t>(-1) == num_thread)
            return static_cast<std::size_t>(-1);

        std::size_t const num_pu = (num_thread + pu_offset) % num_of_pus_;

        hwloc_obj_t obj;
        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_PU, static_cast<unsigned>(num_pu));
            HPX_ASSERT(num_pu == detail::get_index(obj));
        }

        hwloc_obj_t tmp = nullptr;
        while ((tmp = hwloc_get_next_obj_by_type(
                    topo, HWLOC_OBJ_NUMANODE, tmp)) != nullptr)
        {
            if (hwloc_bitmap_intersects(tmp->cpuset, obj->cpuset))
            {
                // tmp matches, use it
                return static_cast<std::size_t>(tmp->logical_index);
            }
        }
        return 0;
#else
        return init_node_number(num_thread, HWLOC_OBJ_NODE);
#endif
    }

    std::size_t topology::init_node_number(
        std::size_t num_thread, hwloc_obj_type_t type) const
    {
        if (static_cast<std::size_t>(-1) == num_thread)
            return static_cast<std::size_t>(-1);

        std::size_t const num_pu = (num_thread + pu_offset) % num_of_pus_;

        {
            hwloc_obj_t obj;

            {
                std::unique_lock<mutex_type> lk(topo_mtx);
                obj = hwloc_get_obj_by_type(
                    topo, HWLOC_OBJ_PU, static_cast<unsigned>(num_pu));
                HPX_ASSERT(num_pu == detail::get_index(obj));
            }

            while (obj)
            {
                if (hwloc_compare_types(obj->type, type) == 0)
                {
                    return detail::get_index(obj);
                }
                obj = obj->parent;
            }
        }

        return 0;
    }

    void topology::extract_node_mask(hwloc_obj_t parent, mask_type& mask) const
    {
        hwloc_obj_t obj;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            obj = hwloc_get_next_child(topo, parent, nullptr);
        }

        while (obj)
        {
            if (hwloc_compare_types(HWLOC_OBJ_PU, obj->type) == 0)
            {
                do
                {
                    set(mask, detail::get_index(obj));    //-V106
                    {
                        std::unique_lock<mutex_type> lk(topo_mtx);
                        obj = hwloc_get_next_child(topo, parent, obj);
                    }
                } while (obj != nullptr &&
                    hwloc_compare_types(HWLOC_OBJ_PU, obj->type) == 0);
                return;
            }

            extract_node_mask(obj, mask);

            std::unique_lock<mutex_type> lk(topo_mtx);
            obj = hwloc_get_next_child(topo, parent, obj);
        }
    }    // }}}

    std::size_t topology::extract_node_count_locked(
        hwloc_obj_t parent, hwloc_obj_type_t type, std::size_t count) const
    {
        if (parent == nullptr)
        {
            return count;
        }

        if (hwloc_compare_types(type, parent->type) == 0)
        {
            return count;
        }

        hwloc_obj_t obj = hwloc_get_next_child(topo, parent, nullptr);

        while (obj)
        {
            if (hwloc_compare_types(type, obj->type) == 0)
            {
                ++count;
            }

            count = extract_node_count_locked(obj, type, count);
            obj = hwloc_get_next_child(topo, parent, obj);
        }

        return count;
    }

    std::size_t topology::extract_node_count(
        hwloc_obj_t parent, hwloc_obj_type_t type, std::size_t count) const
    {
        std::unique_lock<mutex_type> lk(topo_mtx);
        return extract_node_count_locked(parent, type, count);
    }

    std::size_t topology::get_number_of_sockets() const
    {
        int const nobjs = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_SOCKET);
        if (0 > nobjs)
        {
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "hpx::threads::topology::get_number_of_sockets",
                "hwloc_get_nbobjs_by_type failed");
        }
        return static_cast<std::size_t>(nobjs);
    }

    std::size_t topology::get_number_of_numa_nodes() const
    {
        int const nobjs = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_NUMANODE);
        if (0 > nobjs)
        {
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "hpx::threads::topology::get_number_of_numa_nodes",
                "hwloc_get_nbobjs_by_type failed");
        }
        return static_cast<std::size_t>(nobjs);
    }

    std::size_t topology::get_number_of_cores() const
    {
        int nobjs = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_CORE);

        // If num_cores is smaller 0, we have an error
        if (0 > nobjs)
        {
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "hpx::threads::topology::get_number_of_cores",
                "hwloc_get_nbobjs_by_type(HWLOC_OBJ_CORE) failed");
        }
        else if (0 == nobjs)
        {
            // some platforms report zero cores but might still report the
            // number of PUs
            nobjs = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU);
            if (0 > nobjs)
            {
                HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                    "hpx::threads::topology::get_number_of_cores",
                    "hwloc_get_nbobjs_by_type(HWLOC_OBJ_PU) failed");
            }
        }

        // the number of reported cores/pus should never be zero either to
        // avoid division by zero, we should always have at least one core
        if (0 == nobjs)
        {
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "hpx::threads::topology::get_number_of_cores",
                "hwloc_get_nbobjs_by_type reports zero cores/pus");
        }

        return static_cast<std::size_t>(nobjs);
    }

    std::size_t topology::get_number_of_socket_pus(std::size_t num_socket) const
    {
        hwloc_obj_t socket_obj;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            socket_obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_SOCKET, static_cast<unsigned>(num_socket));
        }

        if (socket_obj)
        {
            HPX_ASSERT(num_socket == detail::get_index(socket_obj));
            constexpr std::size_t pu_count = 0;
            return extract_node_count(socket_obj, HWLOC_OBJ_PU, pu_count);
        }

        return num_of_pus_;
    }

    std::size_t topology::get_number_of_numa_node_pus(
        std::size_t numa_node) const
    {
        hwloc_obj_t node_obj;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            node_obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_NODE, static_cast<unsigned>(numa_node));
        }

        if (node_obj)
        {
            HPX_ASSERT(numa_node == detail::get_index(node_obj));
            node_obj = detail::adjust_node_obj(node_obj);

            constexpr std::size_t pu_count = 0;
            return extract_node_count(node_obj, HWLOC_OBJ_PU, pu_count);
        }

        return num_of_pus_;
    }

    std::size_t topology::get_number_of_core_pus_locked(std::size_t core) const
    {
        hwloc_obj_t const core_obj = hwloc_get_obj_by_type(
            topo, HWLOC_OBJ_CORE, static_cast<unsigned>(core));

        if (!use_pus_as_cores_ && core_obj)
        {
            HPX_ASSERT(core == detail::get_index(core_obj));
            constexpr std::size_t pu_count = 0;
            return extract_node_count_locked(core_obj, HWLOC_OBJ_PU, pu_count);
        }

        return static_cast<std::size_t>(1);
    }

    std::size_t topology::get_number_of_core_pus(std::size_t core) const
    {
        std::unique_lock<mutex_type> lk(topo_mtx);
        return get_number_of_core_pus_locked(core);
    }

    std::size_t topology::get_number_of_socket_cores(
        std::size_t num_socket) const
    {
        hwloc_obj_t socket_obj;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            socket_obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_SOCKET, static_cast<unsigned>(num_socket));
        }

        if (socket_obj)
        {
            HPX_ASSERT(num_socket == detail::get_index(socket_obj));
            constexpr std::size_t pu_count = 0;
            return extract_node_count(socket_obj,
                use_pus_as_cores_ ? HWLOC_OBJ_PU : HWLOC_OBJ_CORE, pu_count);
        }

        return get_number_of_cores();
    }

    std::size_t topology::get_number_of_numa_node_cores(
        std::size_t numa_node) const
    {
        hwloc_obj_t node_obj;
        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            node_obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_NODE, static_cast<unsigned>(numa_node));
        }

        if (node_obj)
        {
            HPX_ASSERT(numa_node == detail::get_index(node_obj));
            node_obj = detail::adjust_node_obj(node_obj);

            constexpr std::size_t pu_count = 0;
            return extract_node_count(node_obj,
                use_pus_as_cores_ ? HWLOC_OBJ_PU : HWLOC_OBJ_CORE, pu_count);
        }

        return get_number_of_cores();
    }

    hwloc_bitmap_ptr topology::cpuset_to_nodeset(mask_cref_type mask) const
    {
        hwloc_bitmap_t cpuset = mask_to_bitmap(mask, HWLOC_OBJ_PU);
        hwloc_bitmap_t nodeset = hwloc_bitmap_alloc();
#if HWLOC_API_VERSION >= 0x00020000
        hwloc_cpuset_to_nodeset(topo, cpuset, nodeset);
#else
        hwloc_cpuset_to_nodeset_strict(topo, cpuset, nodeset);
#endif
        hwloc_bitmap_free(cpuset);
        return std::make_shared<hpx::threads::hpx_hwloc_bitmap_wrapper>(
            nodeset);
    }

    namespace detail {

        void print_info(
            std::ostream& os, hwloc_obj_t obj, char const* name, bool comma)
        {
            if (comma)
                os << ", ";
            os << name;

            if (obj->logical_index != ~0x0u)
                os << "L#" << obj->logical_index;
            if (obj->os_index != ~0x0u)
                os << "(P#" << obj->os_index << ")";
        }

        void print_info(std::ostream& os, hwloc_obj_t obj, bool comma = false)
        {
            switch (obj->type)
            {
            case HWLOC_OBJ_PU:
                print_info(os, obj, "PU ", comma);
                break;

            case HWLOC_OBJ_CORE:
                print_info(os, obj, "Core ", comma);
                break;

            case HWLOC_OBJ_SOCKET:
                print_info(os, obj, "Socket ", comma);
                break;

            case HWLOC_OBJ_NODE:
                print_info(os, obj, "NUMANode ", comma);
                break;

            default:
                break;
            }
        }
    }    // namespace detail

    void topology::print_affinity_mask(std::ostream& os, std::size_t num_thread,
        mask_cref_type m, std::string const& pool_name) const
    {
        hpx::util::ios_flags_saver ifs(os);
        bool first = true;

        for (std::size_t i = 0; i != num_of_pus_; ++i)
        {
            hwloc_obj_t obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_PU, static_cast<unsigned>(i));
            if (!obj)
            {
                HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                    "hpx::threads::topology::print_affinity_mask",
                    "object not found");
            }

            if (!test(m, detail::get_index(obj)))    //-V106
                continue;

            if (first)
            {
                first = false;
                os << std::setw(4) << num_thread << ": ";    //-V112 //-V128
            }
            else
            {
                os << "      ";
            }

            detail::print_info(os, obj);

            while (obj->parent)
            {
                detail::print_info(os, obj->parent, true);
                obj = obj->parent;
            }

            os << ", on pool \"" << pool_name << "\"";

            os << std::endl;
        }
    }

    mask_type topology::init_machine_affinity_mask() const
    {
        auto machine_affinity_mask = mask_type();
        resize(machine_affinity_mask, get_number_of_pus());

        hwloc_obj_t machine_obj;
        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            machine_obj = hwloc_get_obj_by_type(topo, HWLOC_OBJ_MACHINE, 0);
        }

        if (machine_obj)
        {
            extract_node_mask(machine_obj, machine_affinity_mask);
            return machine_affinity_mask;
        }

        HPX_THROW_EXCEPTION(hpx::error::kernel_error,
            "hpx::threads::topology::init_machine_affinity_mask",
            "failed to initialize machine affinity mask");
    }

    mask_type topology::init_socket_affinity_mask_from_socket(
        std::size_t num_socket) const
    {
        // If we have only one or no socket, the socket affinity mask
        // spans all processors
        if (static_cast<std::size_t>(-1) == num_socket)
            return machine_affinity_mask_;

        hwloc_obj_t socket_obj;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            socket_obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_SOCKET, static_cast<unsigned>(num_socket));
        }

        if (socket_obj)
        {
            HPX_ASSERT(num_socket == detail::get_index(socket_obj));

            auto socket_affinity_mask = mask_type();
            resize(socket_affinity_mask, get_number_of_pus());

            extract_node_mask(socket_obj, socket_affinity_mask);
            return socket_affinity_mask;
        }

        return machine_affinity_mask_;
    }

    mask_type topology::init_numa_node_affinity_mask_from_numa_node(
        std::size_t numa_node) const
    {
        // If we have only one or no NUMA domain, the NUMA affinity mask
        // spans all processors
        if (static_cast<std::size_t>(-1) == numa_node)
        {
            return machine_affinity_mask_;
        }

        hwloc_obj_t numa_node_obj;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            numa_node_obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_NODE, static_cast<unsigned>(numa_node));
        }

        if (numa_node_obj)
        {
            HPX_ASSERT(numa_node == detail::get_index(numa_node_obj));
            auto node_affinity_mask = mask_type();
            resize(node_affinity_mask, get_number_of_pus());

            numa_node_obj = detail::adjust_node_obj(numa_node_obj);
            extract_node_mask(numa_node_obj, node_affinity_mask);
            return node_affinity_mask;
        }

        return machine_affinity_mask_;
    }

    mask_type topology::init_core_affinity_mask_from_core(
        std::size_t core, mask_cref_type default_mask) const
    {
        if (static_cast<std::size_t>(-1) == core)
        {
            return default_mask;
        }

        hwloc_obj_t core_obj;
        std::size_t const num_core =
            (core + core_offset) % get_number_of_cores();

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            core_obj = hwloc_get_obj_by_type(topo,
                use_pus_as_cores_ ? HWLOC_OBJ_PU : HWLOC_OBJ_CORE,
                static_cast<unsigned>(num_core));
        }

        if (core_obj)
        {
            HPX_ASSERT(num_core == detail::get_index(core_obj));
            mask_type core_affinity_mask = mask_type();
            resize(core_affinity_mask, get_number_of_pus());

            extract_node_mask(core_obj, core_affinity_mask);
            return core_affinity_mask;
        }

        return default_mask;
    }

    mask_type topology::init_thread_affinity_mask(std::size_t num_thread) const
    {
        if (static_cast<std::size_t>(-1) == num_thread)
        {
            return get_core_affinity_mask(num_thread);
        }

        std::size_t const num_pu = (num_thread + pu_offset) % num_of_pus_;
        hwloc_obj_t obj;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_PU, static_cast<unsigned>(num_pu));
        }

        if (!obj)
        {
            return get_core_affinity_mask(num_thread);
        }

        HPX_ASSERT(num_pu == detail::get_index(obj));
        auto mask = mask_type();
        resize(mask, get_number_of_pus());

        set(mask, detail::get_index(obj));    //-V106

        return mask;
    }

    mask_type topology::init_thread_affinity_mask(
        std::size_t num_core, std::size_t num_pu) const
    {
        hwloc_obj_t obj;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            int const num_cores = hwloc_get_nbobjs_by_type(
                topo, use_pus_as_cores_ ? HWLOC_OBJ_PU : HWLOC_OBJ_CORE);

            // If num_cores is smaller 0, we have an error, it should never be
            // zero either to avoid division by zero, we should always have at
            // least one core
            if (num_cores <= 0)
            {
                HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                    "hpx::threads::topology::init_thread_affinity_mask",
                    "hwloc_get_nbobjs_by_type failed");
            }

            num_core =
                (num_core + core_offset) % static_cast<std::size_t>(num_cores);
            obj = hwloc_get_obj_by_type(topo,
                use_pus_as_cores_ ? HWLOC_OBJ_PU : HWLOC_OBJ_CORE,
                static_cast<unsigned>(num_core));
        }

        if (!obj)
            return empty_mask;    //get_core_affinity_mask(num_thread, false);

        HPX_ASSERT(num_core == detail::get_index(obj));

        auto mask = mask_type();
        resize(mask, get_number_of_pus());

        if (use_pus_as_cores_)
        {
            set(mask, detail::get_index(obj));    //-V106
        }
        else
        {
            num_pu %= obj->arity;    //-V101 //-V104
            set(mask, detail::get_index(obj->children[num_pu]));    //-V106
        }

        return mask;
    }

    ///////////////////////////////////////////////////////////////////////////
    void topology::init_num_of_pus()
    {
        num_of_pus_ = 1;
        use_pus_as_cores_ = false;

        {
            std::unique_lock<mutex_type> lk(topo_mtx);

            // on some platforms, hwloc can't report the number of cores (BSD),
            // in this case we use PUs as cores
            if (hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_CORE) <= 0)
            {
                use_pus_as_cores_ = true;
            }

            if (int const num_of_pus =
                    hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU);
                num_of_pus > 0)
            {
                num_of_pus_ = static_cast<std::size_t>(num_of_pus);
            }
        }
    }

    std::size_t topology::get_number_of_pus() const noexcept
    {
        return num_of_pus_;
    }

    ///////////////////////////////////////////////////////////////////////////
    mask_type topology::get_cpubind_mask(error_code& ec) const
    {
        hwloc_cpuset_t const cpuset = hwloc_bitmap_alloc();

        auto mask = mask_type();
        resize(mask, get_number_of_pus());

#if !defined(__APPLE__)
        {
            std::unique_lock<mutex_type> lk(topo_mtx);
            if (hwloc_get_cpubind(topo, cpuset, HWLOC_CPUBIND_THREAD))
            {
                hwloc_bitmap_free(cpuset);
                HPX_THROWS_IF(ec, hpx::error::kernel_error,
                    "hpx::threads::topology::get_cpubind_mask",
                    "hwloc_get_cpubind failed");
                return empty_mask;
            }

            int const pu_depth =
                hwloc_get_type_or_below_depth(topo, HWLOC_OBJ_PU);
            for (unsigned int i = 0; i != num_of_pus_; ++i)    //-V104
            {
                hwloc_obj_t const pu_obj =
                    hwloc_get_obj_by_depth(topo, pu_depth, i);
                auto const idx = static_cast<unsigned>(pu_obj->os_index);
                if (hwloc_bitmap_isset(cpuset, idx) != 0)
                    set(mask, detail::get_index(pu_obj));
            }
        }
#endif    // __APPLE__

        hwloc_bitmap_free(cpuset);

        if (&ec != &throws)
            ec = make_success_code();

        return mask;
    }

    mask_type topology::get_cpubind_mask(
        std::thread& handle, error_code& ec) const
    {
        hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();

        auto mask = mask_type();
        resize(mask, get_number_of_pus());

        {
            std::unique_lock<mutex_type> lk(topo_mtx);
#if defined(HPX_MINGW)
            if (hwloc_get_thread_cpubind(topo,
                    pthread_gethandle(handle.native_handle()), cpuset,
                    HWLOC_CPUBIND_THREAD))
#else
            if (hwloc_get_thread_cpubind(
                    topo, handle.native_handle(), cpuset, HWLOC_CPUBIND_THREAD))
#endif
            {
                hwloc_bitmap_free(cpuset);
                HPX_THROWS_IF(ec, hpx::error::kernel_error,
                    "hpx::threads::topology::get_cpubind_mask",
                    "hwloc_get_cpubind failed");
                return empty_mask;
            }

            int const pu_depth =
                hwloc_get_type_or_below_depth(topo, HWLOC_OBJ_PU);
            for (unsigned int i = 0; i != num_of_pus_; ++i)    //-V104
            {
                hwloc_obj_t const pu_obj =
                    hwloc_get_obj_by_depth(topo, pu_depth, i);
                auto const idx = static_cast<unsigned>(pu_obj->os_index);
                if (hwloc_bitmap_isset(cpuset, idx) != 0)
                    set(mask, detail::get_index(pu_obj));
            }
        }

        hwloc_bitmap_free(cpuset);

        if (&ec != &throws)
            ec = make_success_code();

        return mask;
    }

    ///////////////////////////////////////////////////////////////////////////
    /// This is equivalent to malloc(), except that it tries to allocate
    /// page-aligned memory from the OS.
    void* topology::allocate(std::size_t len) const
    {
        return hwloc_alloc(topo, len);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Allocate some memory on NUMA memory nodes specified by nodeset
    /// as specified by the hwloc hwloc_alloc_membind_nodeset call
    void* topology::allocate_membind(std::size_t len,
        hwloc_bitmap_ptr const& bitmap, hpx_hwloc_membind_policy policy,
        int flags) const
    {
        return
#if HWLOC_API_VERSION >= 0x00010b06
            hwloc_alloc_membind(topo, len, bitmap->get_bmp(),
                static_cast<hwloc_membind_policy_t>(policy),
                flags | HWLOC_MEMBIND_BYNODESET);
#else
            hwloc_alloc_membind_nodeset(topo, len, bitmap->get_bmp(),
                hwloc_membind_policy_t(policy), flags);
#endif
    }

    bool topology::set_area_membind_nodeset([[maybe_unused]] void const* addr,
        [[maybe_unused]] std::size_t len, [[maybe_unused]] void* nodeset) const
    {
#if !defined(__APPLE__)
        hwloc_membind_policy_t policy = ::HWLOC_MEMBIND_BIND;
        auto ns = static_cast<hwloc_nodeset_t>(nodeset);

#if HWLOC_API_VERSION >= 0x00010b06
        int ret = hwloc_set_area_membind(
            topo, addr, len, ns, policy, HWLOC_MEMBIND_BYNODESET);
#else
        int ret =
            hwloc_set_area_membind_nodeset(topo, addr, len, ns, policy, 0);
#endif

        if (ret < 0)
        {
            std::string msg = std::strerror(errno);
            if (errno == ENOSYS)
                msg = "the action is not supported";
            else if (errno == EXDEV)
                msg = "the binding cannot be enforced";

            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "hpx::threads::topology::set_area_membind_nodeset",
                "hwloc_set_area_membind_nodeset failed : {}", msg);
        }
#endif
        return true;
    }

    static hpx_hwloc_bitmap_wrapper& bitmap_storage()
    {
        static thread_local hpx_hwloc_bitmap_wrapper bitmap_storage_(nullptr);

        return bitmap_storage_;
    }

    threads::mask_type topology::get_area_membind_nodeset(
        void const* addr, std::size_t len) const
    {
        hpx_hwloc_bitmap_wrapper& nodeset = bitmap_storage();
        if (!nodeset)
        {
            nodeset.reset(hwloc_bitmap_alloc());
        }

        hwloc_membind_policy_t policy;
        auto ns = reinterpret_cast<hwloc_nodeset_t>(nodeset.get_bmp());

#if HWLOC_API_VERSION >= 0x00010b06
        if (hwloc_get_area_membind(
                topo, addr, len, ns, &policy, HWLOC_MEMBIND_BYNODESET) == -1)
#else
        if (hwloc_get_area_membind_nodeset(topo, addr, len, ns, &policy, 0) ==
            -1)
#endif
        {
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "hpx::threads::topology::get_area_membind_nodeset",
                "hwloc_get_area_membind_nodeset failed");
        }

        return bitmap_to_mask(ns, HWLOC_OBJ_NUMANODE);
    }

    int topology::get_numa_domain([[maybe_unused]] void const* addr) const
    {
#if HWLOC_API_VERSION >= 0x00010b06
        hpx_hwloc_bitmap_wrapper& nodeset = bitmap_storage();
        if (!nodeset)
        {
            nodeset.reset(hwloc_bitmap_alloc());
        }

        //
        auto const ns = reinterpret_cast<hwloc_nodeset_t>(nodeset.get_bmp());

        int const ret = hwloc_get_area_memlocation(
            topo, addr, 1, ns, HWLOC_MEMBIND_BYNODESET);
        if (ret < 0)
        {
#if defined(__FreeBSD__) || defined(__APPLE__)
            // on some platforms this API is not supported (e.g. FreeBSD, macOS)
            return 0;
#else
            std::string msg(strerror(errno));
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "hpx::threads::topology::get_numa_domain",
                "hwloc_get_area_memlocation failed {}", msg);
#endif
        }

        threads::mask_type const mask = bitmap_to_mask(ns, HWLOC_OBJ_NUMANODE);
        return static_cast<int>(threads::find_first(mask));
#else
        return 0;
#endif
    }

    // Free memory that was previously allocated by allocate
    void topology::deallocate(void* addr, std::size_t len) const noexcept
    {
        hwloc_free(topo, addr, len);
    }

    ////////////////////////////////////////////////////////////////////////////
    hwloc_obj_t topology::get_pu_obj(std::size_t num_pu) const
    {
        hwloc_obj_t const pu_obj = hwloc_get_obj_by_type(
            topo, HWLOC_OBJ_CORE, static_cast<unsigned>(num_pu));

        if (pu_obj == nullptr)
        {
            HPX_THROW_EXCEPTION(hpx::error::no_success,
                "topology::get_core_obj",
                "Couldn't find required object representing the given core in "
                "topology");
        }

        return pu_obj;
    }

    namespace {
        template <typename F>
        void iterate(hwloc_bitmap_t cpuset, F&& f)
        {
            for (auto id = hwloc_bitmap_first(cpuset);
                static_cast<unsigned>(id) != static_cast<unsigned>(-1);
                id = hwloc_bitmap_next(cpuset, id))
            {
                if (hwloc_bitmap_isset(cpuset, id))
                {
                    f(id);
                }
            }
        }
    }    // namespace

    // Return the size of the cache associated with the given cpuset.
    std::size_t topology::get_cache_size(mask_cref_type mask, int level) const
    {
        if (level < 1 || level > 5)
        {
            return 0;
        }

        std::unique_lock<mutex_type> lk(topo_mtx);

        unsigned cpuset_size = 0;
        hwloc_bitmap_t cpuset =
            mask_to_bitmap(mask, HWLOC_OBJ_PU, &cpuset_size);
        std::size_t cache_size = 0;

#if HWLOC_API_VERSION >= 0x00020000
        hwloc_obj_type_t type = HWLOC_OBJ_L1CACHE;
        switch (level)
        {
        case 2:
            type = HWLOC_OBJ_L2CACHE;
            break;

        case 3:
            type = HWLOC_OBJ_L3CACHE;
            break;

        case 4:
            type = HWLOC_OBJ_L4CACHE;
            break;

        case 5:
            type = HWLOC_OBJ_L5CACHE;
            break;

        default:
            break;
        }
#endif

        iterate(cpuset, [&](auto num_pu) {
            hwloc_obj_t pu_obj = hwloc_get_obj_by_type(
                topo, HWLOC_OBJ_PU, static_cast<unsigned>(num_pu));

#if HWLOC_API_VERSION >= 0x00020000
            if (pu_obj == nullptr)
                return;

            hwloc_obj_t const cache_obj =
                hwloc_get_ancestor_obj_by_type(topo, type, pu_obj);
            if (cache_obj == nullptr)
                return;

            cache_size +=
                static_cast<std::size_t>(cache_obj->attr->cache.size) /
                cpuset_size;
#else
            // traverse up until found the requested cache level
            int levels = 0;
            for (hwloc_obj_t obj = pu_obj; obj != nullptr && levels < level;
                 obj = obj->parent)
            {
                if (obj->type != HWLOC_OBJ_CACHE || ++levels != level)
                    continue;

                cache_size += std::size_t(obj->attr->cache.size) /
                    cpuset_size;
            }
#endif
        });

        hwloc_bitmap_free(cpuset);
        return cache_size;
    }

    ///////////////////////////////////////////////////////////////////////////
    hwloc_bitmap_t topology::mask_to_bitmap(
        mask_cref_type mask, hwloc_obj_type_t htype, unsigned* count) const
    {
        hwloc_bitmap_t const bitmap = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(bitmap);

        int const depth = hwloc_get_type_or_below_depth(topo, htype);

        unsigned count_bits = 0;
        auto const size = mask_size(mask);
        for (std::size_t i = 0; i != size; ++i)
        {
            if (test(mask, i))
            {
                hwloc_obj_t const hw_obj = hwloc_get_obj_by_depth(
                    topo, depth, static_cast<unsigned>(i));
                HPX_ASSERT(i == detail::get_index(hw_obj));
                hwloc_bitmap_set(
                    bitmap, static_cast<unsigned int>(hw_obj->os_index));
                ++count_bits;
            }
        }

        if (count != nullptr)
        {
            *count = count_bits;
        }
        return bitmap;
    }

    ///////////////////////////////////////////////////////////////////////////
    mask_type topology::bitmap_to_mask(
        hwloc_bitmap_t bitmap, hwloc_obj_type_t htype) const
    {
        auto mask = mask_type();
        resize(mask, get_number_of_pus());
        std::size_t const num = hwloc_get_nbobjs_by_type(topo, htype);

        int const pu_depth = hwloc_get_type_or_below_depth(topo, htype);
        for (unsigned int i = 0; static_cast<std::size_t>(i) != num;
            ++i)    //-V104
        {
            hwloc_obj_t const pu_obj =
                hwloc_get_obj_by_depth(topo, pu_depth, i);
            auto const idx = static_cast<unsigned>(pu_obj->os_index);
            if (hwloc_bitmap_isset(bitmap, idx) != 0)
                set(mask, detail::get_index(pu_obj));
        }
        return mask;
    }

    ///////////////////////////////////////////////////////////////////////////
    void topology::print_mask_vector(
        std::ostream& os, std::vector<mask_type> const& v)
    {
        std::size_t const s = v.size();
        if (s == 0)
        {
            os << "(empty)\n";
            return;
        }

        for (std::size_t i = 0; i != s; i++)
        {
            os << hpx::threads::to_string(v[i]) << "\n";
        }
        os << "\n";
    }

    void topology::print_vector(
        std::ostream& os, std::vector<std::size_t> const& v)
    {
        std::size_t const s = v.size();
        if (s == 0)
        {
            os << "(empty)\n";
            return;
        }

        os << static_cast<std::uint64_t>(v[0]);
        for (std::size_t i = 1; i != s; i++)
        {
            os << ", " << std::dec << static_cast<std::uint64_t>(v[i]);
        }
        os << "\n";
    }

    void topology::print_hwloc(std::ostream& os) const
    {
        os << "[HWLOC topology info] number of ...\n"
           << std::dec << "number of sockets     : "
           << static_cast<std::uint64_t>(get_number_of_sockets()) << "\n"
           << "number of numa nodes  : "
           << static_cast<std::uint64_t>(get_number_of_numa_nodes()) << "\n"
           << "number of cores       : "
           << static_cast<std::uint64_t>(get_number_of_cores()) << "\n"
           << "number of PUs         : "
           << static_cast<std::uint64_t>(get_number_of_pus()) << "\n"
           << "hardware concurrency  : " << hpx::threads::hardware_concurrency()
           << "\n"
           << std::endl;

        //! -------------------------------------- topology (affinity masks)
        os << "[HWLOC topology info] affinity masks :\n"
           << "machine               : \n"
           << hpx::threads::to_string(machine_affinity_mask_) << "\n";

        os << "socket                : \n";
        print_mask_vector(os, socket_affinity_masks_);
        os << "numa node             : \n";
        print_mask_vector(os, numa_node_affinity_masks_);
        os << "core                  : \n";
        print_mask_vector(os, core_affinity_masks_);
        os << "PUs (/threads)        : \n";
        print_mask_vector(os, thread_affinity_masks_);

        //! -------------------------------------- topology (numbers)
        os << "[HWLOC topology info] resource numbers :\n";
        os << "socket                : \n";
        print_vector(os, socket_numbers_);
        os << "numa node             : \n";
        print_vector(os, numa_node_numbers_);
        os << "core                  : \n";
        print_vector(os, core_numbers_);
        //os << "PUs (/threads)        : \n";
        //print_vector(os, pu_numbers_);
    }

    ///////////////////////////////////////////////////////////////////////////
    topology& create_topology()
    {
        static topology topo;
        return topo;
    }

    ///////////////////////////////////////////////////////////////////////////
    struct hardware_concurrency_tag
    {
    };

    struct hw_concurrency
    {
        hw_concurrency()
#if defined(__ANDROID__) || defined(ANDROID)
          : num_of_cores_(::android_getCpuCount())
#else
          : num_of_cores_(detail::hwloc_hardware_concurrency())
#endif
        {
            if (num_of_cores_ == 0)
                num_of_cores_ = 1;
        }

        std::size_t num_of_cores_;
    };

    unsigned int hardware_concurrency() noexcept
    {
        try
        {
            static hw_concurrency hwc;
            return static_cast<unsigned int>(hwc.num_of_cores_);
        }
        catch (...)
        {
            return std::thread::hardware_concurrency();
        }
    }
}    // namespace hpx::threads
