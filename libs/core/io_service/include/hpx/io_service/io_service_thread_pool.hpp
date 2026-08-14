//  Copyright (c) 2017-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/threading_base.hpp>

#include <hpx/io_service/io_service_pool_fwd.hpp>

#include <cstddef>
#include <exception>
#include <iosfwd>
#include <memory>
#include <mutex>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::threads::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT io_service_thread_pool
      : public thread_pool_base
    {
    public:
        explicit io_service_thread_pool(
            threads::thread_pool_init_parameters const& init);

        void print_pool(std::ostream&) const override {}

        ///////////////////////////////////////////////////////////////////////
        [[nodiscard]] hpx::state get_state() const override;
        [[nodiscard]] hpx::state get_state(
            std::size_t num_thread) const override;
        [[nodiscard]] bool has_reached_state(hpx::state s) const override;

        ///////////////////////////////////////////////////////////////////////
        void create_thread(thread_init_data& data, thread_id_ref_type& id,
            error_code& ec) override;

        thread_id_ref_type create_work(
            thread_init_data& data, error_code& ec) override;

        thread_state set_state(thread_id_type const& id,
            thread_schedule_state new_state, thread_restart_state new_state_ex,
            thread_priority priority, error_code& ec) override;

        thread_id_ref_type set_state(
            hpx::chrono::steady_time_point const& abs_time,
            thread_id_type const& id, thread_schedule_state newstate,
            thread_restart_state newstate_ex, thread_priority priority,
            error_code& ec) override;

        bool report_error(
            std::size_t num, std::exception_ptr const& e) override;

        ///////////////////////////////////////////////////////////////////////
        bool run(
            std::unique_lock<std::mutex>& l, std::size_t num_threads) override;

        ///////////////////////////////////////////////////////////////////////
        void stop(
            std::unique_lock<std::mutex>& l, bool blocking = true) override;

        ///////////////////////////////////////////////////////////////////////
        void resume_direct(error_code& ec = throws) override;

        void suspend_direct(error_code& ec = throws) override;

        ///////////////////////////////////////////////////////////////////////
        void suspend_processing_unit_direct(
            std::size_t virt_core, error_code& ec = throws) override;

        void resume_processing_unit_direct(
            std::size_t virt_core, error_code& ec = throws) override;

        ///////////////////////////////////////////////////////////////////////
        std::thread& get_os_thread_handle(
            std::size_t global_thread_num) override;

        [[nodiscard]] std::size_t get_os_thread_count() const override;

    private:
        std::unique_ptr<util::io_service_pool> threads_;
    };
}    // namespace hpx::threads::detail

#include <hpx/config/warnings_suffix.hpp>
