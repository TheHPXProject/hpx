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
#include <hpx/concurrency/barrier.hpp>
#include <hpx/io_service/io_service_pool_fwd.hpp>
#include <hpx/threading_base/callback_notifier.hpp>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#endif
#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>

// The boost asio support includes termios.h. The termios.h file on ppc64le
// defines these macros, which are also used by blaze, blaze_tensor as Template
// names. Make sure we undefine them before continuing.
#undef VT1
#undef VT2

#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    /// A pool of io_service objects.
    class io_service_pool
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
        bool run(bool join_threads = true, barrier* startup = nullptr);

        /// Run all io_service objects in the pool. If join_threads is true
        /// this will also wait for all threads to complete
        bool run(std::size_t num_threads, bool join_threads = true,
            barrier* startup = nullptr);

        /// \brief Stop all io_service objects in the pool.
        void stop();

        /// \brief Join all io_service threads in the pool.
        void join();

        /// \brief Clear all internal data structures
        void clear();

        /// \brief Wait for all work to be done
        void wait();

        bool stopped();

        /// \brief Get an io_service to use.
        ::asio::io_context& get_io_service(int index = -1);

        /// \brief access underlying thread handle
        std::thread& get_os_thread_handle(std::size_t thread_num);

        /// \brief Get number of threads associated with this I/O service.
        [[nodiscard]] constexpr std::size_t size() const noexcept
        {
            return pool_size_;
        }

        /// \brief Activate the thread \a index for this thread pool
        void thread_run(std::size_t index, barrier* startup = nullptr) const;

        /// \brief Return name of this pool
        [[nodiscard]] constexpr char const* get_name() const noexcept
        {
            return pool_name_;
        }

        void init(std::size_t pool_size);

    protected:
        bool run_locked(
            std::size_t num_threads, bool join_threads, barrier* startup);
        void stop_locked();
        void join_locked();
        void clear_locked();
        void wait_locked();

    private:
        using io_service_ptr = std::unique_ptr<::asio::io_context>;

        using raw_work_type =
            ::asio::executor_work_guard<::asio::io_context::executor_type>;
        using work_type = std::unique_ptr<raw_work_type>;

        static work_type initialize_work(::asio::io_context& io_service);

        std::mutex mtx_;

        /// The pool of io_services.
        std::vector<io_service_ptr> io_services_;
        std::vector<std::thread> threads_;

        /// The work that keeps the io_services running.
        std::vector<work_type> work_;

        /// The next io_service to use for a connection.
        std::size_t next_io_service_;

        /// set to true if stopped
        bool stopped_;

        /// initial number of OS threads to execute in this pool
        std::size_t pool_size_;

        /// call this for each thread start/stop
        threads::policies::callback_notifier const& notifier_;

        char const* pool_name_;
        char const* pool_name_postfix_;

        /// Set to true if waiting for work to finish
        bool waiting_;

        // Barriers for waiting for work to finish on all worker threads
        std::unique_ptr<barrier> wait_barrier_;
        std::unique_ptr<barrier> continue_barrier_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
