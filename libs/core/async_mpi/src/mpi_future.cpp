//  Copyright (c) 2019 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_mpi/mpi_exception.hpp>
#include <hpx/async_mpi/mpi_future.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/mpi_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <mpi.h>

namespace hpx { namespace mpi { namespace experimental {

    namespace detail {

        // Holds an MPI_Request and a callback. The callback is intended to be
        // called when the operation tight to the request handle is finished.
        struct request_callback
        {
            MPI_Request request;
            request_callback_function_type callback_function;
        };

        using request_callback_queue_type =
            concurrency::ConcurrentQueue<request_callback>;

        request_callback_queue_type& get_request_callback_queue()
        {
            static request_callback_queue_type request_callback_queue;
            return request_callback_queue;
        }

        using request_callback_vector_type = std::vector<request_callback>;

        request_callback_vector_type& get_request_callback_vector()
        {
            static request_callback_vector_type request_callback_vector;
            return request_callback_vector;
        }

        std::size_t get_num_active_requests_in_vector()
        {
            return std::count_if(detail::get_requests_vector().begin(),
                detail::get_requests_vector().end(),
                [](MPI_Request r) { return r != MPI_REQUEST_NULL; });
        }

        // used internally to add an MPI_Request to the lockfree queue
        // that will be used by the polling routines to check when requests
        // have completed
        void add_to_request_callback_queue(request_callback&& req_callback)
        {
            get_request_callback_queue().enqueue(std::move(req_callback));
            ++(get_mpi_info().requests_queue_size_);

            if constexpr (mpi_debug.is_enabled())
            {
                mpi_debug.debug(debug::str<>("request callback queued"),
                    get_mpi_info(), "request",
                    debug::hex<8>(req_callback.request));
            }
        }

        // used internally to add a request to the main polling vector
        // that is passed to MPI_Testany
        void add_to_request_callback_vector(request_callback&& req_callback)
        {
            get_requests_vector().push_back(req_callback.request);
            get_request_callback_vector().push_back(std::move(req_callback));
            get_mpi_info().requests_vector_size_ =
                static_cast<std::uint32_t>(get_requests_vector().size());

            if constexpr (mpi_debug.is_enabled())
            {
                mpi_debug.debug(
                    debug::str<>("request callback moved from queue to vector"),
                    get_mpi_info(), "request",
                    debug::hex<8>(req_callback.request), "callbacks in vector",
                    debug::dec<3>(get_request_callback_vector().size()),
                    "non null",
                    debug::dec<3>(get_num_active_requests_in_vector()));
            }
        }

        void add_request_callback(
            request_callback_function_type&& callback, MPI_Request request)
        {
            detail::add_to_request_callback_queue(
                request_callback{request, std::move(callback)});
        }

        // mutex needed to protect mpi request vector, note that the
        // mpi poll function takes place inside the main scheduling loop
        // of hpx and not on an hpx worker thread, so we must use std:mutex
        mutex_type& get_vector_mtx()
        {
            static mutex_type vector_mtx;
            return vector_mtx;
        }

        // an MPI error handling type that we can use to intercept
        // MPI errors if we enable the error handler
        MPI_Errhandler hpx_mpi_errhandler = 0;

        // an instance of mpi_info that we store data in
        mpi_info& get_mpi_info()
        {
            static mpi_info mpi_info_;
            return mpi_info_;
        }

        // stream operator to display debug mpi_info
        HPX_CORE_EXPORT std::ostream& operator<<(
            std::ostream& os, mpi_info const&)
        {
            os << "R " << debug::dec<3>(get_mpi_info().rank_) << "/"
               << debug::dec<3>(get_mpi_info().size_) << " requests in vector "
               << debug::dec<3>(get_mpi_info().requests_vector_size_)
               << " queued requests "
               << debug::dec<3>(get_mpi_info().requests_queue_size_);
            return os;
        }

        // function that converts an MPI error into an exception
        void hpx_MPI_Handler(MPI_Comm*, int* errorcode, ...)
        {
            mpi_debug.debug(debug::str<>("hpx_MPI_Handler"));
            HPX_THROW_EXCEPTION(hpx::error::invalid_status, "hpx_MPI_Handler",
                detail::error_message(*errorcode));
        }

        std::vector<MPI_Request>& get_requests_vector()
        {
            static std::vector<MPI_Request> requests_vector;
            return requests_vector;
        }

#if defined(HPX_DEBUG)
        std::atomic<std::size_t>& get_register_polling_count()
        {
            static std::atomic<std::size_t> register_polling_count{0};
            return register_polling_count;
        }
#endif
    }    // namespace detail

    // return a future object from a user supplied MPI_Request
    hpx::future<void> get_future(MPI_Request request)
    {
        if (request != MPI_REQUEST_NULL)
        {
            HPX_ASSERT_MSG(detail::get_register_polling_count() != 0,
                "MPI event polling has not been enabled on any pool. Make sure "
                "that MPI event polling is enabled on at least one thread "
                "pool.");

            // create a future data shared state with the request Id
            detail::future_data_ptr data(new detail::future_data(
                detail::future_data::init_no_addref{}, request));

            // return a future bound to the shared state
            using traits::future_access;
            return future_access<hpx::future<void>>::create(HPX_MOVE(data));
        }
        return hpx::make_ready_future<void>();
    }

    // set an error handler for communicators that will be called
    // on any error instead of the default behavior of program termination
    void set_error_handler()
    {
        mpi_debug.debug(debug::str<>("set_error_handler"));

        MPI_Comm_create_errhandler(
            detail::hpx_MPI_Handler, &detail::hpx_mpi_errhandler);
        MPI_Comm_set_errhandler(MPI_COMM_WORLD, detail::hpx_mpi_errhandler);
    }

    // Background progress function for MPI async operations
    // Checks for completed MPI_Requests and sets mpi::experimental::future
    // ready when found
    hpx::threads::policies::detail::polling_status poll()
    {
        using hpx::threads::policies::detail::polling_status;

        auto& request_callback_vector = detail::get_request_callback_vector();
        auto& requests_vector = detail::get_requests_vector();

        std::unique_lock<detail::mutex_type> lk(
            detail::get_vector_mtx(), std::try_to_lock);
        if (!lk.owns_lock())
        {
            if constexpr (mpi_debug.is_enabled())
            {
                // for debugging, create a timer
                static auto poll_deb =
                    mpi_debug.make_timer(1, debug::str<>("Poll - lock failed"));
                // output mpi debug info every N seconds
                mpi_debug.timed(poll_deb, detail::get_mpi_info());
            }
            return polling_status::idle;
        }

        if constexpr (mpi_debug.is_enabled())
        {
            // for debugging, create a timer
            static auto poll_deb =
                mpi_debug.make_timer(1, debug::str<>("Poll - lock success"));
            // output mpi debug info every N seconds
            mpi_debug.timed(poll_deb, detail::get_mpi_info());
        }

        {
            // have any requests been made that need to be handled?
            detail::request_callback req_callback;
            while (
                detail::get_request_callback_queue().try_dequeue(req_callback))
            {
                --(detail::get_mpi_info().requests_queue_size_);
                add_to_request_callback_vector(HPX_MOVE(req_callback));
            }
        }

        bool keep_trying = !requests_vector.empty();
        while (keep_trying)
        {
            int index = 0;
            int flag = 0;
            MPI_Status status;

            int result = MPI_Testany(static_cast<int>(requests_vector.size()),
                requests_vector.data(), &index, &flag, &status);

            if constexpr (mpi_debug.is_enabled())
            {
                if (result == MPI_SUCCESS)
                {
                    static auto poll_deb =
                        mpi_debug.make_timer(1, debug::str<>("Poll - success"));

                    mpi_debug.timed(poll_deb, detail::get_mpi_info(),
                        debug::str<>("Success"), "index",
                        debug::dec<>(index == MPI_UNDEFINED ? -1 : index),
                        "flag", debug::dec<>(flag), "status",
                        debug::hex(status.MPI_ERROR));
                }
                else
                {
                    auto poll_deb =
                        mpi_debug.make_timer(1, debug::str<>("Poll - <ERR>"));

                    mpi_debug.error(poll_deb, detail::get_mpi_info(),
                        debug::str<>("Poll <ERR>"), "MPI_ERROR",
                        detail::error_message(status.MPI_ERROR), "status",
                        debug::dec<>(status.MPI_ERROR), "index",
                        debug::dec<>(index), "flag", debug::dec<>(flag));
                }
            }

            // No operation completed
            if (index == MPI_UNDEFINED)
                break;

            keep_trying = flag;
            if constexpr (mpi_debug.is_enabled())
            {
                // One operation completed
                if (keep_trying)
                {
                    mpi_debug.debug(debug::str<>("MPI_Testany(set)"),
                        detail::get_mpi_info(), "request",
                        debug::hex<8>(requests_vector[std::size_t(index)]),
                        "vector size", debug::dec<3>(requests_vector.size()),
                        "non null",
                        debug::dec<3>(
                            detail::get_num_active_requests_in_vector()));
                }
            }
            if (result != MPI_SUCCESS)    // Error and operation not completed
                keep_trying = false;
            if (keep_trying || result != MPI_SUCCESS)
            {
                // Invoke the callback with the status of the completed
                // operation (status of the request is forwarded to MPI_Testany)
                request_callback_vector[std::size_t(index)].callback_function(
                    result);

                // Remove the request from our vector to prevent retesting
                requests_vector[std::size_t(index)] = MPI_REQUEST_NULL;

                // Could store only the callbacks, right now the request
                // is only used for an assert
                request_callback_vector[std::size_t(index)].request =
                    MPI_REQUEST_NULL;
            }
        }

        // if there are more than 25% NULL request handles in our vector,
        // compact them
        if (!requests_vector.empty())
        {
            std::size_t nulls = std::count(requests_vector.begin(),
                requests_vector.end(), MPI_REQUEST_NULL);

            if (nulls > requests_vector.size() / 4)    //-V112
            {
                // compact away any requests that have been replaced by
                // MPI_REQUEST_NULL
                auto end1 = std::remove(requests_vector.begin(),
                    requests_vector.end(), MPI_REQUEST_NULL);
                requests_vector.resize(
                    std::distance(requests_vector.begin(), end1));

                auto end2 = std::remove_if(request_callback_vector.begin(),
                    request_callback_vector.end(),
                    [](detail::request_callback& req_callback) {
                        return req_callback.request == MPI_REQUEST_NULL;
                    });
                request_callback_vector.resize(
                    std::distance(request_callback_vector.begin(), end2));

                if (requests_vector.size() != request_callback_vector.size())
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "hpx::mpi::experimental::poll",
                        "Fatal Error: Mismatch in vectors");
                }

                detail::get_mpi_info().requests_vector_size_ =
                    static_cast<std::uint32_t>(requests_vector.size());

                if constexpr (mpi_debug.is_enabled())
                {
                    mpi_debug.debug(debug::str<>("MPI_REQUEST_NULL"),
                        detail::get_mpi_info(), "nulls ", debug::dec<>(nulls));
                }
            }
        }

        return requests_vector.empty() ? polling_status::idle :
                                         polling_status::busy;
    }

    namespace detail {
        std::size_t get_work_count()
        {
            std::size_t work_count = 0;
            {
                std::unique_lock<detail::mutex_type> lk(
                    detail::get_vector_mtx(), std::try_to_lock);
                if (lk.owns_lock())
                {
                    work_count += get_num_active_requests_in_vector();
                }
            }

            work_count += get_mpi_info().requests_queue_size_;

            return work_count;
        }

        // -------------------------------------------------------------
        void register_polling(hpx::threads::thread_pool_base& pool)
        {
#if defined(HPX_DEBUG)
            ++get_register_polling_count();
#endif
            mpi_debug.debug(debug::str<>("enable polling"));
            auto* sched = pool.get_scheduler();
            sched->set_mpi_polling_functions(
                &hpx::mpi::experimental::poll, &get_work_count);
        }

        // -------------------------------------------------------------
        void unregister_polling(hpx::threads::thread_pool_base& pool)
        {
#if defined(HPX_DEBUG)
            {
                std::unique_lock<hpx::mpi::experimental::detail::mutex_type> lk(
                    detail::get_vector_mtx());
                bool requests_queue_empty =
                    get_request_callback_queue().size_approx() == 0;
                bool requests_vector_empty =
                    get_num_active_requests_in_vector() == 0;
                lk.unlock();
                HPX_ASSERT_MSG(requests_queue_empty,
                    "MPI request polling was disabled while there are "
                    "unprocessed MPI requests. Make sure MPI request polling "
                    "is not disabled too early.");
                HPX_ASSERT_MSG(requests_vector_empty,
                    "MPI request polling was disabled while there are active "
                    "MPI futures. Make sure MPI request polling is not "
                    "disabled too early.");
            }
#endif
            if constexpr (mpi_debug.is_enabled())
                mpi_debug.debug(debug::str<>("disable polling"));
            auto* sched = pool.get_scheduler();
            sched->clear_mpi_polling_function();
        }
    }    // namespace detail

    // initialize the hpx::mpi background request handler
    // All ranks should call this function,
    // but only one thread per rank needs to do so
    void init(
        bool init_mpi, std::string const& pool_name, bool init_errorhandler)
    {
        if (init_mpi)
        {
            int required = MPI_THREAD_MULTIPLE;
            int provided;
            int was_initialized = 0;
            MPI_Initialized(&was_initialized);
            hpx::util::mpi_environment::init(
                nullptr, nullptr, required, required, provided);
            detail::get_mpi_info().mpi_initialized_ = !was_initialized;
            if (provided != required)
            {
                mpi_debug.error(debug::str<>("hpx::mpi::experimental::init"),
                    "init failed");
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "hpx::mpi::experimental::init",
                    "the MPI installation doesn't allow multiple threads");
            }
            MPI_Comm_rank(MPI_COMM_WORLD, &detail::get_mpi_info().rank_);
            MPI_Comm_size(MPI_COMM_WORLD, &detail::get_mpi_info().size_);
        }
        else
        {
            // Check if MPI_Init has been called previously
            if (detail::get_mpi_info().size_ == -1)
            {
                int is_initialized = 0;
                MPI_Initialized(&is_initialized);
                if (is_initialized)
                {
                    MPI_Comm_rank(
                        MPI_COMM_WORLD, &detail::get_mpi_info().rank_);
                    MPI_Comm_size(
                        MPI_COMM_WORLD, &detail::get_mpi_info().size_);
                }
            }
        }

        mpi_debug.debug(debug::str<>("hpx::mpi::experimental::init"),
            detail::get_mpi_info());

        if (init_errorhandler)
        {
            set_error_handler();
            detail::get_mpi_info().error_handler_initialized_ = true;
        }

        // install polling loop on requested thread pool
        if (pool_name.empty())
        {
            detail::register_polling(hpx::resource::get_thread_pool(0));
        }
        else
        {
            detail::register_polling(hpx::resource::get_thread_pool(pool_name));
        }
    }

    // -----------------------------------------------------------------

    void finalize(std::string const& pool_name)
    {
        if (detail::get_mpi_info().error_handler_initialized_)
        {
            HPX_ASSERT(detail::hpx_mpi_errhandler != 0);
            detail::get_mpi_info().error_handler_initialized_ = false;
            MPI_Errhandler_free(&detail::hpx_mpi_errhandler);
            detail::hpx_mpi_errhandler = 0;
        }

        // clean up only if we initialized mpi, MPI might still be in use
        // elsewhere (e.g. by the MPI parcelport) otherwise
        if (detail::get_mpi_info().mpi_initialized_)
        {
            detail::get_mpi_info().mpi_initialized_ = false;
            hpx::util::mpi_environment::finalize();
        }

        mpi_debug.debug(debug::str<>("Clearing mode"), detail::get_mpi_info(),
            "disable_user_polling");

        if (pool_name.empty())
        {
            detail::unregister_polling(hpx::resource::get_thread_pool(0));
        }
        else
        {
            detail::unregister_polling(
                hpx::resource::get_thread_pool(pool_name));
        }
    }
}}}    // namespace hpx::mpi::experimental
