//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  Parts of this code were taken from the Boost.Asio library
//  Copyright (c) 2003-2007 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <thread>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    namespace detail {

        struct HPX_CORE_EXPORT io_service_pool_base
        {
            virtual ~io_service_pool_base() = default;

            virtual bool run(bool join_threads, barrier* startup) = 0;
            virtual bool run(std::size_t num_threads, bool join_threads,
                barrier* startup) = 0;
            virtual void stop() = 0;
            virtual void join() = 0;
            virtual void clear() = 0;
            virtual void wait() = 0;
            virtual bool stopped() = 0;
            virtual ::asio::io_context& get_io_service(int index = -1) = 0;
            virtual std::thread& get_os_thread_handle(
                std::size_t thread_num) = 0;
            virtual std::size_t size() const noexcept = 0;
            virtual void thread_run(
                std::size_t index, barrier* startup) const = 0;
            virtual char const* get_name() const noexcept = 0;
            virtual void init(std::size_t pool_size) = 0;
        };
    }    // namespace detail

    /// A pool of io_service objects.
    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT io_service_pool
    {
    public:
        /// \brief Construct the io_service pool.
        /// \param pool_size [in] The number of threads to run to serve incoming
        ///                  requests
        /// \param notifier     [in]
        /// \param pool_name    [in]
        /// \param name_postfix [in]
        explicit io_service_pool(std::size_t pool_size = 2,
            threads::policies::callback_notifier const& notifier =
                threads::policies::callback_notifier(),
            char const* pool_name = "", char const* name_postfix = "");

        /// \brief Construct the io_service pool.
        /// \param notifier     [in]
        /// \param pool_name    [in]
        /// \param name_postfix [in]
        explicit io_service_pool(
            threads::policies::callback_notifier const& notifier,
            char const* pool_name = "", char const* name_postfix = "");

        io_service_pool(io_service_pool const&) = delete;
        io_service_pool(io_service_pool&&) = delete;
        io_service_pool& operator=(io_service_pool const&) = delete;
        io_service_pool& operator=(io_service_pool&&) = delete;

        ~io_service_pool();

        /// Run all io_service objects in the pool. If join_threads is true
        /// this will also wait for all threads to complete
        bool run(bool join_threads = true, barrier* startup = nullptr) const;

        /// Run all io_service objects in the pool. If join_threads is true
        /// this will also wait for all threads to complete
        bool run(std::size_t num_threads, bool join_threads = true,
            barrier* startup = nullptr) const;

        /// \brief Stop all io_service objects in the pool.
        void stop() const;

        /// \brief Join all io_service threads in the pool.
        void join() const;

        /// \brief Clear all internal data structures
        void clear() const;

        /// \brief Wait for all work to be done
        void wait() const;

        bool stopped() const;

        /// \brief Get an io_service to use.
        ::asio::io_context& get_io_service(int index = -1) const;

        /// \brief access underlying thread handle
        std::thread& get_os_thread_handle(std::size_t thread_num) const;

        /// \brief Get number of threads associated with this I/O service.
        [[nodiscard]] std::size_t size() const noexcept;

        /// \brief Activate the thread \a index for this thread pool
        void thread_run(std::size_t index, barrier* startup = nullptr) const;

        /// \brief Return name of this pool
        [[nodiscard]] char const* get_name() const noexcept;

        void init(std::size_t pool_size) const;

    private:
        detail::io_service_pool_base* pool_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
