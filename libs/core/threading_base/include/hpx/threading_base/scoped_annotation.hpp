//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file scoped_annotation.hpp
/// \page hpx::scoped_annotation
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/thread_description.hpp>
#include <hpx/threading_base/thread_helpers.hpp>
#endif

#include <string>
#include <type_traits>

namespace hpx {

    namespace detail {

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const*
        store_function_annotation(std::string name);
    }    // namespace detail

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_COMPUTE_DEVICE_CODE)
    HPX_CXX_CORE_EXPORT struct [[nodiscard]] scoped_annotation
    {
        scoped_annotation(scoped_annotation const&) = delete;
        scoped_annotation(scoped_annotation&&) = delete;
        scoped_annotation& operator=(scoped_annotation const&) = delete;
        scoped_annotation& operator=(scoped_annotation&&) = delete;

        explicit constexpr scoped_annotation(char const*) noexcept {}

        template <typename F>
        explicit HPX_HOST_DEVICE constexpr scoped_annotation(F&&) noexcept
        {
        }

        // add empty (but non-trivial) destructor to silence warnings
        HPX_HOST_DEVICE ~scoped_annotation() {}
    };
#elif defined(HPX_HAVE_THREAD_DESCRIPTION)
    HPX_CXX_CORE_EXPORT struct [[nodiscard]] scoped_annotation
    {
        scoped_annotation(scoped_annotation const&) = delete;
        scoped_annotation(scoped_annotation&&) = delete;
        scoped_annotation& operator=(scoped_annotation const&) = delete;
        scoped_annotation& operator=(scoped_annotation&&) = delete;

        explicit scoped_annotation(char const* name)
        {
            auto const* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                desc_ = threads::get_thread_id_data(self->get_thread_id())
                            ->set_description(name);

                if (auto timer_data = threads::get_self_timer_data();
                    timer_data.valid())
                {
                    hpx::tracing::update_task_timer(timer_data, name);
                    threads::set_self_timer_data(HPX_MOVE(timer_data));
                }
            }
        }

        explicit scoped_annotation(std::string name)
        {
            auto const* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                char const* name_c_str =
                    detail::store_function_annotation(HPX_MOVE(name));
                desc_ = threads::get_thread_id_data(self->get_thread_id())
                            ->set_description(name_c_str);

                if (auto timer_data = threads::get_self_timer_data();
                    timer_data.valid())
                {
                    hpx::tracing::update_task_timer(timer_data, name_c_str);
                    threads::set_self_timer_data(HPX_MOVE(timer_data));
                }
            }
        }

        template <typename F,
            typename =
                std::enable_if_t<!std::is_same_v<std::decay_t<F>, std::string>>>
        explicit scoped_annotation(F&& f)
        {
            auto const* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                desc_ =
                    threads::get_thread_id_data(self->get_thread_id())
                        ->set_description(hpx::threads::thread_description(f));

                if (auto timer_data = threads::get_self_timer_data();
                    timer_data.valid())
                {
                    hpx::tracing::update_task_timer(
                        timer_data, desc_.get_description());
                    threads::set_self_timer_data(HPX_MOVE(timer_data));
                }
            }
        }

        ~scoped_annotation()
        {
            auto const* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                threads::get_thread_id_data(self->get_thread_id())
                    ->set_description(desc_);
            }
        }

        hpx::threads::thread_description desc_;
    };
#else
    HPX_CXX_CORE_EXPORT struct [[nodiscard]] scoped_annotation
    {
        scoped_annotation(scoped_annotation const&) = delete;
        scoped_annotation(scoped_annotation&&) = delete;
        scoped_annotation& operator=(scoped_annotation const&) = delete;
        scoped_annotation& operator=(scoped_annotation&&) = delete;

        explicit scoped_annotation(char const* name)
        {
            auto const* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                desc_ = threads::get_thread_id_data(self->get_thread_id())
                            ->set_description(name);

                if (auto timer_data = threads::get_self_timer_data();
                    timer_data.valid())
                {
                    hpx::tracing::update_task_timer(timer_data, name);
                    threads::set_self_timer_data(HPX_MOVE(timer_data));
                }
            }
        }

        explicit scoped_annotation(std::string name)
        {
            auto const* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                char const* name_c_str =
                    detail::store_function_annotation(HPX_MOVE(name));
                desc_ = threads::get_thread_id_data(self->get_thread_id())
                            ->set_description(name_c_str);

                if (auto timer_data = threads::get_self_timer_data();
                    timer_data.valid())
                {
                    hpx::tracing::update_task_timer(timer_data, name_c_str);
                    threads::set_self_timer_data(HPX_MOVE(timer_data));
                }
            }
        }

        template <typename F,
            typename =
                std::enable_if_t<!std::is_same_v<std::decay_t<F>, std::string>>>
        explicit scoped_annotation(F&& f)
        {
            auto const* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                desc_ =
                    threads::get_thread_id_data(self->get_thread_id())
                        ->set_description(hpx::threads::thread_description(f));
            }
        }

        ~scoped_annotation()
        {
            auto const* self = hpx::threads::get_self_ptr();
            if (self != nullptr)
            {
                threads::get_thread_id_data(self->get_thread_id())
                    ->set_description(desc_);
            }
        }

        hpx::threads::thread_description desc_;
    };
#endif

#else
    /// \brief scoped_annotation associates a \c name with a section of code
    ///        (scope). It can be used to visualize code execution in profiling
    ///        tools like \a Intel \a VTune, \a Apex \a Profiler, etc. That
    ///        allows analyzing performance to figure out which part(s) of code
    ///        is (are) responsible for performance degradation, etc.
    HPX_CXX_CORE_EXPORT struct [[nodiscard]] scoped_annotation
    {
        scoped_annotation(scoped_annotation const&) = delete;
        scoped_annotation(scoped_annotation&&) = delete;
        scoped_annotation& operator=(scoped_annotation const&) = delete;
        scoped_annotation& operator=(scoped_annotation&&) = delete;

        explicit constexpr scoped_annotation(char const* /*name*/) noexcept {}

        template <typename F>
        explicit HPX_HOST_DEVICE constexpr scoped_annotation(F&& /*f*/) noexcept
        {
        }

        // add empty (but non-trivial) destructor to silence warnings
        HPX_HOST_DEVICE ~scoped_annotation() {}
    };
#endif
}    // namespace hpx
