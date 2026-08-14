//  Copyright (c) 2025 Jiakun Yan
//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c)      2020 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCW)

#include <hpx/modules/parcelset_base.hpp>

#include <hpx/parcelport_lcw/config.hpp>
#include <hpx/modules/lcw_base.hpp>
#include <hpx/parcelport_lcw/backlog_queue.hpp>
#include <hpx/parcelport_lcw/completion_manager/completion_manager_queue.hpp>
#include <hpx/parcelport_lcw/locality.hpp>
#include <hpx/parcelport_lcw/parcelport_lcw.hpp>
#include <hpx/parcelport_lcw/receiver_base.hpp>
#include <hpx/parcelport_lcw/sender_base.hpp>
#include <hpx/parcelport_lcw/sender_connection_base.hpp>
#include <hpx/parcelport_lcw/sendrecv/receiver_sendrecv.hpp>
#include <hpx/parcelport_lcw/sendrecv/sender_sendrecv.hpp>

#include <hpx/assert.hpp>

#include <asio/io_context.hpp>
#include <asio/version.hpp>
#if ASIO_VERSION >= 103400
#include <asio/post.hpp>
#endif

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace hpx::parcelset::policies::lcw {

    parcelset::locality parcelport::here()
    {
        return parcelset::locality(locality(util::lcw_environment::enabled() ?
                util::lcw_environment::rank() :
                -1));
    }

    std::size_t parcelport::max_connections(
        util::runtime_configuration const& ini)
    {
        return hpx::util::get_entry_as<std::size_t>(
            ini, "hpx.parcel.lcw.max_connections", HPX_PARCEL_MAX_CONNECTIONS);
    }

    parcelport::parcelport(util::runtime_configuration const& ini,
        threads::policies::callback_notifier const& notifier)
      : parcelport::base_type(ini, here(), notifier)
      , stopped_(false)
    {
        if (!util::lcw_environment::enabled())
            return;
        if (!parcelset::policies::lcw::config_t::is_initialized)
        {
            fprintf(stderr,
                "init_config hasn't been called! Something is wrong!\n");
            exit(1);
        }
        setup(ini);
    }

    parcelport::~parcelport()
    {
        if (!util::lcw_environment::enabled())
            return;
        cleanup();
        util::lcw_environment::finalize();
    }

    void parcelport::initialized()
    {
        if (util::lcw_environment::enabled() &&
            config_t::progress_type != config_t::progress_type_t::pthread &&
            config_t::progress_type !=
                config_t::progress_type_t::pthread_worker)
        {
            join_prg_thread_if_running();
        }
        is_initialized = true;
    }

    // Start the handling of connections.
    bool parcelport::do_run()
    {
        receiver_p->run();
        sender_p->run();
        for (std::size_t i = 0; i != io_service_pool_.size(); ++i)
        {
#if ASIO_VERSION >= 103400
            ::asio::post(io_service_pool_.get_io_service(static_cast<int>(i)),
                hpx::bind(&parcelport::io_service_work, this));
#else
            io_service_pool_.get_io_service(static_cast<int>(i))
                .post(hpx::bind(&parcelport::io_service_work, this));
#endif
        }
        return true;
    }

    // Stop the handling of connections.
    void parcelport::do_stop()
    {
        while (do_background_work(0, parcelport_background_mode::all))
        {
            if (threads::get_self_ptr())
                hpx::this_thread::suspend(
                    hpx::threads::thread_schedule_state::pending,
                    "lcw::parcelport::do_stop");
        }
        stopped_ = true;
    }

    /// Return the name of this locality
    std::string parcelport::get_locality_name() const
    {
        // hostname
        return util::lcw_environment::get_processor_name();
    }

    std::shared_ptr<sender_connection_base> parcelport::create_connection(
        parcelset::locality const& l, error_code&)
    {
        int dest_rank = l.get<locality>().rank();
        return sender_p->create_connection(dest_rank, this);
    }

    parcelset::locality parcelport::agas_locality(
        util::runtime_configuration const&) const
    {
        return parcelset::locality(
            locality(util::lcw_environment::enabled() ? 0 : -1));
    }

    parcelset::locality parcelport::create_locality() const
    {
        return parcelset::locality(locality());
    }

    void parcelport::send_early_parcel(
        hpx::parcelset::locality const& dest, parcel p)
    {
        base_type::send_early_parcel(dest, HPX_MOVE(p));
    }

    bool parcelport::do_background_work(
        std::size_t num_thread, parcelport_background_mode mode)
    {
        static thread_local bool devices_to_progress_initialized = false;
        static thread_local std::vector<device_t*> devices_to_progress;
        if (!devices_to_progress_initialized)
        {
            devices_to_progress_initialized = true;
            if (config_t::progress_type == config_t::progress_type_t::rp &&
                hpx::threads::get_self_id() != hpx::threads::invalid_thread_id)
            {
                if (hpx::this_thread::get_pool() ==
                    &hpx::resource::get_thread_pool("lcw-progress-pool"))
                {
                    int prg_thread_id =
                        static_cast<int>(hpx::get_local_worker_thread_num());
                    HPX_ASSERT(prg_thread_id < config_t::progress_thread_num);
                    for (int i = prg_thread_id * config_t::ndevices /
                            config_t::progress_thread_num;
                        i < (prg_thread_id + 1) * config_t::ndevices /
                            config_t::progress_thread_num;
                        ++i)
                    {
                        devices_to_progress.push_back(&devices[i]);
                    }
                }
            }
        }

        bool has_work = false;
        if (!devices_to_progress.empty())
        {
            // magic number
            int const max_idle_loop_count = 1000;
            int idle_loop_count = 0;
            while (idle_loop_count < max_idle_loop_count)
            {
                for (auto device_p : devices_to_progress)
                {
                    if (util::lcw_environment::do_progress(device_p->device))
                    {
                        has_work = true;
                        idle_loop_count = 0;
                    }
                }
                ++idle_loop_count;
            }
        }
        else
        {
            has_work = base_type::do_background_work(num_thread, mode);
        }
        return has_work;
    }

    bool parcelport::background_work(
        std::size_t num_thread, parcelport_background_mode mode)
    {
        if (stopped_)
            return false;

        bool has_work = false;
        if (mode & parcelport_background_mode::send)
        {
            has_work = sender_p->background_work(num_thread);
            if (config_t::progress_type == config_t::progress_type_t::worker ||
                config_t::progress_type ==
                    config_t::progress_type_t::pthread_worker)
                do_progress_local();
            if (config_t::enable_lcw_backlog_queue)
                // try to send pending messages
                has_work =
                    backlog_queue::background_work(
                        get_tls_device().completion_manager_p->send.get(),
                        num_thread) ||
                    has_work;
        }
        if (mode & parcelport_background_mode::receive)
        {
            has_work = receiver_p->background_work() || has_work;
            if (config_t::progress_type == config_t::progress_type_t::worker ||
                config_t::progress_type ==
                    config_t::progress_type_t::pthread_worker)
                do_progress_local();
        }
        return has_work;
    }

    bool parcelport::can_send_immediate()
    {
        return config_t::enable_send_immediate;
    }

    bool parcelport::send_immediate(parcelset::parcelport* pp,
        parcelset::locality const& dest, sender_base::parcel_buffer_type buffer,
        sender_base::callback_fn_type&& callbackFn)
    {
        return sender_p->send_immediate(pp, dest,
            HPX_FORWARD(sender_base::parcel_buffer_type, buffer),
            HPX_FORWARD(sender_base::callback_fn_type, callbackFn));
    }

    void parcelport::io_service_work()
    {
        std::size_t k = 0;
        // We only execute work on the IO service while HPX is starting
        // and stop io service work when worker threads start to execute the
        // background function
        while (hpx::is_starting() && !is_initialized)
        {
            bool has_work = sender_p->background_work(0);
            has_work = receiver_p->background_work() || has_work;
            if (config_t::progress_type == config_t::progress_type_t::worker ||
                config_t::progress_type ==
                    config_t::progress_type_t::pthread_worker)
                while (do_progress_local())
                    continue;
            if (has_work)
            {
                k = 0;
            }
            else
            {
                ++k;
                util::detail::yield_k(k,
                    "hpx::parcelset::policies::lcw::parcelport::"
                    "io_service_work");
            }
        }
    }

    std::atomic<bool> parcelport::prg_thread_flag = false;
    void parcelport::progress_thread_fn(std::vector<device_t> const& devices)
    {
        while (prg_thread_flag)
        {
            for (auto& device : devices)
            {
                util::lcw_environment::do_progress(device.device);
            }
        }
    }

    void set_affinity(pthread_t pthread_handler, size_t target)
    {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(target, &cpuset);
        int rv =
            pthread_setaffinity_np(pthread_handler, sizeof(cpuset), &cpuset);
        if (rv != 0)
        {
            fprintf(stderr, "ERROR %d thread affinity didn't work.\n", rv);
            exit(1);
        }
    }

    void parcelport::setup(util::runtime_configuration const& rtcfg)
    {
        HPX_UNUSED(rtcfg);

        // Create completion managers
        completion_managers.resize(config_t::ncomps);
        for (auto& completion_manager : completion_managers)
        {
            completion_manager.recv_new =
                std::make_shared<completion_manager_queue>();
            completion_manager.send =
                std::make_shared<completion_manager_queue>();
            completion_manager.recv_followup =
                std::make_shared<completion_manager_queue>();
        }

        // Create device
        devices.resize(config_t::ndevices);
        for (int i = 0; i < config_t::ndevices; ++i)
        {
            auto& device = devices[i];
            // Create the LCW device
            int comp_idx = i * config_t::ncomps / config_t::ndevices;
            device.completion_manager_p = &completion_managers[comp_idx];
            device.idx = i;
            device.device = ::lcw::alloc_device(
                static_cast<int64_t>(get_zero_copy_serialization_threshold()),
                device.completion_manager_p->recv_new->get_completion_object());
        }

        // Create progress threads
        HPX_ASSERT(prg_thread_flag == false);
        HPX_ASSERT(prg_thread_p == nullptr);
        prg_thread_flag = true;
        prg_thread_p =
            std::make_unique<std::thread>(progress_thread_fn, devices);

        // Create the sender and receiver
        sender_p = std::make_shared<sender_sendrecv>(this);
        receiver_p = std::make_shared<receiver_sendrecv>(this);
    }

    void parcelport::cleanup()
    {
        join_prg_thread_if_running();
        // Free devices
        for (auto& device : devices)
        {
            ::lcw::free_device(device.device);
            completion_managers.clear();
        }
    }

    void parcelport::join_prg_thread_if_running()
    {
        if (prg_thread_p)
        {
            prg_thread_flag = false;
            if (prg_thread_p)
            {
                prg_thread_p->join();
                prg_thread_p.reset();
            }
        }
    }

    bool parcelport::do_progress_local()
    {
        bool ret = false;
        switch (config_t::progress_strategy)
        {
        case config_t::progress_strategy_t::local:
        {
            auto device = get_tls_device();
            ret = util::lcw_environment::do_progress(device.device) || ret;
            break;
        }
        case config_t::progress_strategy_t::global:
        {
            for (auto& device : devices)
            {
                ret = util::lcw_environment::do_progress(device.device) || ret;
            }
            break;
        }
        case config_t::progress_strategy_t::random:
        {
            static thread_local unsigned int tls_rand_seed = rand();
            auto device = devices[rand_r(&tls_rand_seed) % devices.size()];
            ret = util::lcw_environment::do_progress(device.device) || ret;
            break;
        }
        default:
            throw std::runtime_error("Unknown progress strategy");
        }
        return ret;
    }

    parcelport::device_t& parcelport::get_tls_device()
    {
        static thread_local std::size_t tls_device_idx = -1;

        if (HPX_UNLIKELY(!is_initialized ||
                hpx::threads::get_self_id() == hpx::threads::invalid_thread_id))
        {
            static thread_local unsigned int tls_rand_seed = rand();
            return devices[rand_r(&tls_rand_seed) % devices.size()];
        }
        if (tls_device_idx == std::size_t(-1))
        {
            // initialize TLS device
            // hpx::threads::topology& topo = hpx::threads::create_topology();
            auto& rp = hpx::resource::get_partitioner();

            std::size_t num_thread =
                hpx::get_worker_thread_num();    // current worker
            std::size_t total_thread_num = rp.get_num_threads();
            HPX_ASSERT(num_thread < total_thread_num);
            std::size_t nthreads_per_device =
                (total_thread_num + config_t::ndevices - 1) /
                config_t::ndevices;

            tls_device_idx = num_thread / nthreads_per_device;
            util::lcw_environment::log(
                util::lcw_environment::log_level_t::debug, "device",
                "Rank %d thread %lu/%lu gets device %lu\n", ::lcw::get_rank(),
                num_thread, total_thread_num, tls_device_idx);
        }
        return devices[tls_device_idx];
    }
}    // namespace hpx::parcelset::policies::lcw

HPX_REGISTER_PARCELPORT(hpx::parcelset::policies::lcw::parcelport, lcw)

#endif
