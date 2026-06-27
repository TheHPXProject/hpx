//  Copyright (c) 2017-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/affinity.hpp>
#include <hpx/modules/concurrency.hpp>

#include <hpx/io_service/io_service_pool.hpp>
#include <hpx/io_service/io_service_thread_pool.hpp>

#include <cstddef>
#include <exception>

namespace hpx::threads::detail {

    io_service_thread_pool::io_service_thread_pool(
        hpx::threads::thread_pool_init_parameters const& init)
      : thread_pool_base(init)
      , threads_(std::make_unique<util::io_service_pool>(
            init.notifier_, init.name_.c_str()))
    {
    }

    hpx::state io_service_thread_pool::get_state() const
    {
        return hpx::state::stopped;
    }

    hpx::state io_service_thread_pool::get_state(
        std::size_t /* num_thread */) const
    {
        return hpx::state::stopped;
    }

    bool io_service_thread_pool::has_reached_state(hpx::state /* s */) const
    {
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    void io_service_thread_pool::create_thread(thread_init_data& /* data */,
        thread_id_ref_type& /* id */, error_code& /* ec */)
    {
    }

    thread_id_ref_type io_service_thread_pool::create_work(
        thread_init_data& /* data */, error_code& /* ec */)
    {
        return invalid_thread_id;
    }

    threads::thread_state io_service_thread_pool::set_state(
        thread_id_type const& /* id */, thread_schedule_state /* new_state */,
        thread_restart_state /* new_state_ex */, thread_priority /* priority */,
        error_code& /* ec */)
    {
        return threads::thread_state(threads::thread_schedule_state::terminated,
            threads::thread_restart_state::unknown);
    }

    threads::thread_id_ref_type io_service_thread_pool::set_state(
        hpx::chrono::steady_time_point const& /* abs_time */,
        thread_id_type const& id, thread_schedule_state /* newstate */,
        thread_restart_state /* newstate_ex */, thread_priority /* priority */,
        error_code& /* ec */)
    {
        return id;
    }

    bool io_service_thread_pool::report_error(
        std::size_t /* num */, std::exception_ptr const& /* e */)
    {
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool io_service_thread_pool::run(
        [[maybe_unused]] std::unique_lock<std::mutex>& l,
        std::size_t num_threads)
    {
        HPX_ASSERT(l.owns_lock());
        util::barrier startup(1);
        return threads_->run(num_threads, false, &startup);
    }

    void io_service_thread_pool::stop(
        std::unique_lock<std::mutex>& /* l */, bool /* blocking = true */)
    {
    }

    void io_service_thread_pool::resume_direct(error_code& /* ec */)
    {
        HPX_ASSERT_MSG(
            false, "Resuming io_service_thread_pool is not supported");
    }

    void io_service_thread_pool::suspend_direct(error_code& /* ec */)
    {
        HPX_ASSERT_MSG(
            false, "Suspending io_service_thread_pool is not supported");
    }

    void io_service_thread_pool::suspend_processing_unit_direct(
        std::size_t /* virt_core */, error_code& /* ec */)
    {
        HPX_ASSERT_MSG(false,
            "Suspending threads on io_service_thread_pool is not "
            "supported");
    }

    void io_service_thread_pool::resume_processing_unit_direct(
        std::size_t /* virt_core */, error_code& /* ec */)
    {
        HPX_ASSERT_MSG(false,
            "Suspending threads on io_service_thread_pool is not "
            "supported");
    }

    std::thread& io_service_thread_pool::get_os_thread_handle(
        std::size_t global_thread_num)
    {
        return threads_->get_os_thread_handle(
            global_thread_num - this->thread_offset_);
    }

    std::size_t io_service_thread_pool::get_os_thread_count() const
    {
        return threads_->size();
    }
}    // namespace hpx::threads::detail
