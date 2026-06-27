//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Adelstein-Lelbach
//
//  Parts of this code were taken from the Boost.Asio library
//  Copyright (c) 2003-2007 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/threading_base.hpp>

#include <hpx/io_service/io_service_pool.hpp>

#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/version.hpp>
#if ASIO_VERSION >= 103400
#include <asio/post.hpp>
#endif

// The asio support includes termios.h. The termios.h file on ppc64le defines
// these macros, which are also used by blaze, blaze_tensor as Template names.
// Make sure we undefine them before continuing.
#undef VT1
#undef VT2

#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::detail { namespace {

    /// A pool of io_service objects.
    class io_service_pool final : public io_service_pool_base
    {
    public:
        io_service_pool(std::size_t pool_size,
            threads::policies::callback_notifier const& notifier,
            char const* pool_name, char const* name_postfix);

        io_service_pool(threads::policies::callback_notifier const& notifier,
            char const* pool_name, char const* name_postfix);

        io_service_pool(io_service_pool const&) = delete;
        io_service_pool(io_service_pool&&) = delete;
        io_service_pool& operator=(io_service_pool const&) = delete;
        io_service_pool& operator=(io_service_pool&&) = delete;

        ~io_service_pool() override;

        /// Run all io_service objects in the pool. If join_threads is true
        /// this will also wait for all threads to complete
        bool run(bool join_threads = true, barrier* startup = nullptr) override;

        /// Run all io_service objects in the pool. If join_threads is true
        /// this will also wait for all threads to complete
        bool run(std::size_t num_threads, bool join_threads = true,
            barrier* startup = nullptr) override;

        /// \brief Stop all io_service objects in the pool.
        void stop() override;

        /// \brief Join all io_service threads in the pool.
        void join() override;

        /// \brief Clear all internal data structures
        void clear() override;

        /// \brief Wait for all work to be done
        void wait() override;

        bool stopped() override;

        /// \brief Get an io_service to use.
        ::asio::io_context& get_io_service(int index = -1) override;

        /// \brief access underlying thread handle
        std::thread& get_os_thread_handle(std::size_t thread_num) override;

        /// \brief Get number of threads associated with this I/O service.
        [[nodiscard]] std::size_t size() const noexcept override
        {
            return pool_size_;
        }

        /// \brief Activate the thread \a index for this thread pool
        void thread_run(
            std::size_t index, barrier* startup = nullptr) const override;

        /// \brief Return name of this pool
        [[nodiscard]] char const* get_name() const noexcept override
        {
            return pool_name_;
        }

        void init(std::size_t pool_size) override;

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

        /// The pool of io_services.
        std::vector<io_service_ptr> io_services_;
        std::vector<std::thread> threads_;

        /// The work that keeps the io_services running.
        std::vector<work_type> work_;

        /// The next io_service to use for a connection.
        std::size_t next_io_service_;

        /// initial number of OS threads to execute in this pool
        std::size_t pool_size_;

        /// call this for each thread start/stop
        threads::policies::callback_notifier const& notifier_;

        char const* pool_name_;
        char const* pool_name_postfix_;

        // Barriers for waiting for work to finish on all worker threads
        std::unique_ptr<barrier> wait_barrier_;
        std::unique_ptr<barrier> continue_barrier_;

        /// set to true if stopped
        bool stopped_;

        /// Set to true if waiting for work to finish
        bool waiting_;

        std::mutex mtx_;
    };

    io_service_pool::io_service_pool(std::size_t const pool_size,
        threads::policies::callback_notifier const& notifier,
        char const* pool_name, char const* name_postfix)
      : next_io_service_(0)
      , pool_size_(0)
      , notifier_(notifier)
      , pool_name_(pool_name)
      , pool_name_postfix_(name_postfix)
      , stopped_(false)
      , waiting_(false)
    {
        LPROGRESS_ << pool_name;
        io_service_pool::init(pool_size);
    }

    void io_service_pool::init(std::size_t const pool_size)
    {
        pool_size_ = pool_size;
        if (pool_size_ == 0)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "io_service_pool::io_service_pool",
                "io_service_pool size is 0");
        }

        wait_barrier_.reset(new barrier(pool_size + 1));
        continue_barrier_.reset(new barrier(pool_size + 1));

        // Give all the io_services work to do so that their run() functions
        // will not exit until they are explicitly stopped.
        for (std::size_t i = 0; i < pool_size_; ++i)
        {
            std::unique_ptr<::asio::io_context> p =
                std::make_unique<::asio::io_context>();
            io_services_.emplace_back(HPX_MOVE(p));
            work_.emplace_back(initialize_work(*io_services_[i]));
        }
    }

    io_service_pool::io_service_pool(
        threads::policies::callback_notifier const& notifier,
        char const* pool_name, char const* name_postfix)
      : next_io_service_(0)
      , pool_size_(0)
      , notifier_(notifier)
      , pool_name_(pool_name)
      , pool_name_postfix_(name_postfix)
      , wait_barrier_(nullptr)
      , continue_barrier_(nullptr)
      , stopped_(false)
      , waiting_(false)
    {
        LPROGRESS_ << pool_name;
    }

    io_service_pool::~io_service_pool()
    {
        std::scoped_lock<std::mutex> l(mtx_);
        stop_locked();
        join_locked();
        clear_locked();
    }

    void io_service_pool::thread_run(
        std::size_t const index, util::barrier* startup) const
    {
        // wait for all threads to start up before starting HPX work
        if (startup != nullptr)
            startup->wait();

        notifier_.on_start_thread(index, index, pool_name_, pool_name_postfix_);

        // use this thread for the given io service
        while (true)
        {
            io_services_[index]->run();    // run io service

            if (waiting_)    //-V779
            {
                wait_barrier_->wait();
                continue_barrier_->wait();
            }
            else
            {
                break;
            }
        }

        notifier_.on_stop_thread(index, index, pool_name_, pool_name_postfix_);
    }

    bool io_service_pool::run(std::size_t const num_threads,
        bool const join_threads, util::barrier* startup)
    {
        std::scoped_lock<std::mutex> l(mtx_);

        // Create a pool of threads to run all io_services.
        if (!threads_.empty())    // should be called only once
        {
            HPX_ASSERT(pool_size_ == io_services_.size());
            HPX_ASSERT(threads_.size() == io_services_.size());
            HPX_ASSERT(work_.size() == io_services_.size());

            if (join_threads)
                join_locked();

            return false;
        }

        // Give all the io_services work to do so that their run() functions
        // will not exit until they are explicitly stopped.
        if (!io_services_.empty())
            clear_locked();

        return run_locked(num_threads, join_threads, startup);
    }

    bool io_service_pool::run(bool const join_threads, util::barrier* startup)
    {
        std::scoped_lock<std::mutex> l(mtx_);

        // Create a pool of threads to run all io_services.
        if (!threads_.empty())    // should be called only once
        {
            HPX_ASSERT(pool_size_ == io_services_.size());
            HPX_ASSERT(threads_.size() == io_services_.size());
            HPX_ASSERT(work_.size() == io_services_.size());

            if (join_threads)
                join_locked();

            return false;
        }

        // Give all the io_services work to do so that their run() functions
        // will not exit until they are explicitly stopped.
        if (!io_services_.empty())
            clear_locked();

        return run_locked(pool_size_, join_threads, startup);
    }

    bool io_service_pool::run_locked(std::size_t const num_threads,
        bool const join_threads, util::barrier* startup)
    {
        if (io_services_.empty())
        {
            pool_size_ = num_threads;

            for (std::size_t i = 0; i < num_threads; ++i)
            {
                std::unique_ptr<::asio::io_context> p =
                    std::make_unique<::asio::io_context>();
                io_services_.emplace_back(HPX_MOVE(p));
                work_.emplace_back(initialize_work(*io_services_[i]));
            }
        }

        for (std::size_t i = 0; i < num_threads; ++i)
        {
            std::thread t(&io_service_pool::thread_run, this, i, startup);
            threads_.emplace_back(HPX_MOVE(t));
        }

        next_io_service_ = 0;
        stopped_ = false;

        HPX_ASSERT(pool_size_ == io_services_.size());
        HPX_ASSERT(threads_.size() == io_services_.size());
        HPX_ASSERT(work_.size() == io_services_.size());

        if (join_threads)
            join_locked();

        return true;
    }

    void io_service_pool::join()
    {
        std::scoped_lock<std::mutex> l(mtx_);
        join_locked();
    }

    void io_service_pool::join_locked()
    {
        // Wait for all threads in the pool to exit.
        for (auto& thread : threads_)
            thread.join();
        threads_.clear();
    }

    void io_service_pool::stop()
    {
        std::scoped_lock<std::mutex> l(mtx_);
        stop_locked();
    }

    void io_service_pool::stop_locked()
    {
        if (!stopped_)
        {
            // Explicitly inform all work to exit.
            work_.clear();

            // Explicitly stop all io_services.
            for (std::size_t i = 0; !stopped_ && i < io_services_.size(); ++i)
                io_services_[i]->stop();

            stopped_ = true;
        }
    }

    void io_service_pool::wait()
    {
        std::scoped_lock<std::mutex> l(mtx_);
        wait_locked();
    }

    void io_service_pool::wait_locked()
    {
        if (!stopped_)
        {
            // Clear work so that the run functions return when all work is done
            waiting_ = true;
            work_.clear();
            wait_barrier_->wait();

            // Add back the work guard and restart the services
            waiting_ = false;
            for (std::size_t i = 0; i < pool_size_; ++i)
            {
                work_.emplace_back(initialize_work(*io_services_[i]));
#if ASIO_VERSION >= 103400
                io_services_[i]->restart();
#else
                io_services_[i]->reset();
#endif
            }

            continue_barrier_->wait();
        }
    }

    void io_service_pool::clear()
    {
        std::scoped_lock<std::mutex> l(mtx_);
        clear_locked();
    }

    void io_service_pool::clear_locked()
    {
        if (stopped_)
        {
            next_io_service_ = 0;
            threads_.clear();
            work_.clear();
            io_services_.clear();
        }
    }

    bool io_service_pool::stopped()
    {
        std::scoped_lock<std::mutex> l(mtx_);
        return stopped_;
    }

    io_service_pool::work_type io_service_pool::initialize_work(
        ::asio::io_context& io_service)
    {
        return std::make_unique<raw_work_type>(io_service.get_executor());
    }

    ::asio::io_context& io_service_pool::get_io_service(int index)
    {
        // use this function for single group io_service pools only
        std::scoped_lock<std::mutex> l(mtx_);

        if (index == -1)
        {
            if (++next_io_service_ == pool_size_)
                next_io_service_ = 0;

            // Use a round-robin scheme to choose the next io_service to use.
            index = static_cast<int>(next_io_service_);
        }
        else
        {
            next_io_service_ = static_cast<std::size_t>(index);
        }

        return *io_services_[static_cast<std::size_t>(index)];    //-V108
    }

    std::thread& io_service_pool::get_os_thread_handle(
        std::size_t const thread_num)
    {
        HPX_ASSERT(thread_num < pool_size_);
        return threads_[thread_num];
    }
}}    // namespace hpx::util::detail

namespace hpx::util {

    io_service_pool::io_service_pool(std::size_t pool_size,
        threads::policies::callback_notifier const& notifier,
        char const* pool_name, char const* name_postfix)
      : pool_(new detail::io_service_pool(
            pool_size, notifier, pool_name, name_postfix))
    {
    }

    io_service_pool::io_service_pool(
        threads::policies::callback_notifier const& notifier,
        char const* pool_name, char const* name_postfix)
      : pool_(new detail::io_service_pool(notifier, pool_name, name_postfix))
    {
    }

    io_service_pool::~io_service_pool()
    {
        delete pool_;
    }

    bool io_service_pool::run(bool const join_threads, barrier* startup) const
    {
        return pool_->run(join_threads, startup);
    }

    bool io_service_pool::run(std::size_t const num_threads,
        bool const join_threads, barrier* startup) const
    {
        return pool_->run(num_threads, join_threads, startup);
    }

    void io_service_pool::stop() const
    {
        pool_->stop();
    }

    void io_service_pool::join() const
    {
        pool_->join();
    }

    void io_service_pool::clear() const
    {
        pool_->clear();
    }

    void io_service_pool::wait() const
    {
        pool_->wait();
    }

    bool io_service_pool::stopped() const
    {
        return pool_->stopped();
    }

    ::asio::io_context& io_service_pool::get_io_service(int const index) const
    {
        return pool_->get_io_service(index);
    }

    std::thread& io_service_pool::get_os_thread_handle(
        std::size_t const thread_num) const
    {
        return pool_->get_os_thread_handle(thread_num);
    }

    std::size_t io_service_pool::size() const noexcept
    {
        return pool_->size();
    }

    char const* io_service_pool::get_name() const noexcept
    {
        return pool_->get_name();
    }

    void io_service_pool::thread_run(
        std::size_t const index, barrier* startup) const
    {
        pool_->thread_run(index, startup);
    }

    void io_service_pool::init(std::size_t const pool_size) const
    {
        pool_->init(pool_size);
    }
}    // namespace hpx::util
