//  Copyright (c) 2019-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/coroutines/coroutine_fwd.hpp>
#include <hpx/coroutines/detail/coroutine_self.hpp>
#include <hpx/coroutines/thread_enums.hpp>
#include <hpx/coroutines/thread_id_type.hpp>

#include <cstddef>
#include <limits>
#include <utility>

namespace hpx::threads::coroutines::detail {

    HPX_CXX_CORE_EXPORT class coroutine_stackless_self : public coroutine_self
    {
    public:
        explicit coroutine_stackless_self(stackless_coroutine* pimpl) noexcept
          : coroutine_self(nullptr)
          , pimpl_(pimpl)
        {
            HPX_ASSERT(pimpl_);
        }

        arg_type yield_impl(result_type) override
        {
            // stackless coroutines don't support suspension (leaf / nostack)
            HPX_ASSERT_MSG(false,
                "non-suspending (nostack) leaf task attempted to "
                "suspend/yield");
            return threads::thread_restart_state::abort;
        }

        thread_id_type get_thread_id() const noexcept override
        {
            return pimpl_->get_thread_id();
        }

#if defined(HPX_HAVE_THREAD_PHASE_INFORMATION)
        std::size_t get_thread_phase() const noexcept override
        {
            return pimpl_->get_thread_phase();
        }
#else
        std::size_t get_thread_phase() const noexcept override
        {
            return 0;
        }
#endif

        std::ptrdiff_t get_available_stack_space() const noexcept override
        {
            return (std::numeric_limits<std::ptrdiff_t>::max)();
        }

        std::size_t get_thread_data() const noexcept override
        {
            return pimpl_->get_thread_data();
        }
        std::size_t set_thread_data(std::size_t data) noexcept override
        {
            return pimpl_->set_thread_data(data);
        }

#if defined(HPX_HAVE_LIBCDS)
        std::size_t get_libcds_data() const override
        {
            return pimpl_->get_libcds_data();
        }
        std::size_t set_libcds_data(std::size_t data) override
        {
            return pimpl_->set_libcds_data(data);
        }

        std::size_t get_libcds_hazard_pointer_data() const override
        {
            return pimpl_->get_libcds_hazard_pointer_data();
        }
        std::size_t set_libcds_hazard_pointer_data(std::size_t data) override
        {
            return pimpl_->set_libcds_hazard_pointer_data(data);
        }

        std::size_t get_libcds_dynamic_hazard_pointer_data() const override
        {
            return pimpl_->get_libcds_dynamic_hazard_pointer_data();
        }
        std::size_t set_libcds_dynamic_hazard_pointer_data(
            std::size_t data) override
        {
            return pimpl_->set_libcds_dynamic_hazard_pointer_data(data);
        }
#endif

#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        tss_storage* get_thread_tss_data() override
        {
            return pimpl_->get_thread_tss_data(false);
        }
#else
        tss_storage* get_thread_tss_data() override
        {
            return nullptr;
        }
#endif

#if defined(HPX_HAVE_THREAD_LOCAL_STORAGE)
        tss_storage* get_or_create_thread_tss_data() override
        {
            return pimpl_->get_thread_tss_data(true);
        }
#else
        tss_storage* get_or_create_thread_tss_data() override
        {
            return nullptr;
        }
#endif

        std::size_t& get_continuation_recursion_count() noexcept override
        {
            return pimpl_->get_continuation_recursion_count();
        }

    private:
        stackless_coroutine* pimpl_;
    };
}    // namespace hpx::threads::coroutines::detail
