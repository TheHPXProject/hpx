//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/static_reinit.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/runtime_components/console_logging.hpp>
#include <hpx/runtime_components/server/console_logging.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components {

    void fallback_console_logging_locked(
        messages_type const& msgs, std::string fail_msg = "")
    {
        using hpx::get;

        if (!fail_msg.empty())
            fail_msg = "Logging failed due to: " + fail_msg + "\n";

        for (std::size_t i = 0; i != msgs.size(); ++i)
        {
            message_type const& msg = msgs[i];
            switch (get<0>(msg))
            {
            // NOLINTNEXTLINE(bugprone-branch-clone)
            default:
            case logging_destination::hpx:
                LHPX_CONSOLE_(get<1>(msg)) << fail_msg << get<2>(msg);
                break;

            case logging_destination::timing:
                LTIM_CONSOLE_(get<1>(msg)) << fail_msg << get<2>(msg);
                break;

            case logging_destination::agas:
                LAGAS_CONSOLE_(get<1>(msg)) << fail_msg << get<2>(msg);
                break;

            case logging_destination::parcel:
                LPT_CONSOLE_(get<1>(msg)) << fail_msg << get<2>(msg);
                break;

            case logging_destination::app:
                LAPP_CONSOLE_(get<1>(msg)) << fail_msg << get<2>(msg);
                break;

            case logging_destination::debuglog:
                LDEB_CONSOLE_ << fail_msg << get<2>(msg);
                break;
            }
        }
    }

    void console_logging_locked(
        hpx::id_type const& prefix, messages_type const& msgs)
    {
        // If we're not in an HPX thread, we cannot call apply as it may access
        // the AGAS components. We just throw an exception here - there's no
        // thread-manager, so the exception will probably be unhandled. This is
        // desirable in this situation, as we can't trust the logging system to
        // report this error.
        if (HPX_UNLIKELY(!threads::get_self_ptr()))
        {
            // write the message to a local file in any case
            fallback_console_logging_locked(msgs);

            // raise error as this should get called from outside a HPX-thread
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "components::console_logging_locked",
                "console_logging_locked was not called from a HPX-thread");
        }

        try
        {
            hpx::post<server::console_logging_action<>>(prefix, msgs);
        }
        catch (hpx::exception const& e)
        {
            // if this is not the console locality (or any other error occurs)
            // we might be too late for any logging, write to local file
            fallback_console_logging_locked(msgs, e.what());
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    struct HPX_EXPORT pending_logs
    {
        using prefix_mutex_type = hpx::mutex;
        using queue_mutex_type = util::spinlock;

        enum
        {
            max_pending = 128
        };

        pending_logs()
          : prefix_mtx_()
          , prefix_(hpx::invalid_id)
          , queue_mtx_()
          , activated_(false)
          , is_sending_(false)
        {
        }

        void add(message_type const& msg);

        void cleanup();

        void activate()
        {
            activated_.store(true);
        }

    private:
        bool ensure_prefix();
        void send();
        bool is_active();

        prefix_mutex_type prefix_mtx_;
        hpx::id_type prefix_;

        queue_mutex_type queue_mtx_;
        messages_type queue_;

        std::atomic<bool> activated_;
        std::atomic<bool> is_sending_;
    };

    bool pending_logs::is_active()
    {
        return threads::get_self_ptr() &&
            threads::threadmanager_is(hpx::state::running) && activated_.load();
    }

    void pending_logs::add(message_type const& msg)
    {
        if (nullptr == hpx::get_runtime_ptr())
        {
            // This branch will be taken if it's too early or too late in the
            // game. We do local logging only. Any queued messages which may be
            // still left in the queue are logged locally as well.

            // queue up the new message and log it with the rest of it
            messages_type msgs;
            {
                std::lock_guard<queue_mutex_type> l(queue_mtx_);
                queue_.push_back(msg);
                queue_.swap(msgs);
            }

            fallback_console_logging_locked(msgs);
        }
        else if (is_active())
        {
            // This branch will be taken under normal circumstances. We queue
            // up the message and either log it immediately (if we are on the
            // console locality) or defer logging until 'max_pending' messages
            // have been queued. Note: we can invoke send only from within a
            // HPX-thread.
            std::size_t size = 0;

            {
                std::lock_guard<queue_mutex_type> l(queue_mtx_);
                queue_.push_back(msg);
                size = queue_.size();
            }

            // Invoke actual logging immediately if we're on the console or
            // if the number of waiting log messages is too large.
            if (agas::is_console() || size > max_pending)
                send();
        }
        else
        {
            // This branch will be taken if the runtime is up and running, but
            // either the thread manager is not active or this is not invoked
            // on a HPX-thread.

            // Note: is_console can be called outside of a HPX-thread
            if (!agas::is_console())
            {
                // queue it for delivery to the console
                std::lock_guard<queue_mutex_type> l(queue_mtx_);
                queue_.push_back(msg);
            }
            else
            {
                // log it locally on the console
                messages_type msgs;
                msgs.push_back(msg);
                fallback_console_logging_locked(msgs);
            }
        }
    }

    void pending_logs::cleanup()
    {
        if (threads::threadmanager_is(hpx::state::running) &&
            threads::get_self_ptr())
        {
            send();
        }
        else
        {
            messages_type msgs;
            {
                std::lock_guard<queue_mutex_type> l(queue_mtx_);
                if (queue_.empty())
                    return;    // some other thread did the deed
                queue_.swap(msgs);
            }

            fallback_console_logging_locked(msgs);
        }
    }

    bool pending_logs::ensure_prefix()
    {
        // Resolve the console prefix if it's still invalid.
        if (HPX_UNLIKELY(hpx::invalid_id == prefix_))
        {
            std::unique_lock<prefix_mutex_type> l(
                prefix_mtx_, std::try_to_lock);

            if (l.owns_lock() && (hpx::invalid_id == prefix_))
            {
                naming::gid_type raw_prefix;
                {
                    unlock_guard<std::unique_lock<prefix_mutex_type>> ul(l);
                    naming::get_agas_client().get_console_locality(raw_prefix);
                }

                HPX_ASSERT(naming::invalid_gid != raw_prefix);
                if (!prefix_)
                {
                    prefix_ = hpx::id_type(
                        raw_prefix, hpx::id_type::management_type::unmanaged);
                }
                else
                {
                    HPX_ASSERT(prefix_.get_gid() == raw_prefix);
                }
            }

            // Someone else started getting the console prefix.
            else
            {
                return false;
            }
        }
        return true;
    }

    void pending_logs::send()
    {
        // WARNING: Never, ever call this outside of a HPX-thread.
        HPX_ASSERT(threads::get_self_ptr());

        bool expected = false;
        if (!is_sending_.compare_exchange_strong(expected, true))
            return;

        try
        {
            {
                std::lock_guard<queue_mutex_type> l(queue_mtx_);
                if (queue_.empty())
                    return;    // some other thread did the deed
            }

            if (!ensure_prefix())
                return;    // some other thread tries to do logging

            messages_type msgs;
            {
                std::lock_guard<queue_mutex_type> l(queue_mtx_);
                if (queue_.empty())
                    return;    // some other thread did the deed
                queue_.swap(msgs);
            }

            console_logging_locked(prefix_, msgs);
        }
        catch (...)
        {
            is_sending_ = false;
            throw;
        }

        is_sending_ = false;
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        struct pending_logs_tag
        {
        };

        pending_logs& logger()
        {
            util::reinitializable_static<pending_logs, pending_logs_tag> logs;
            return logs.get();
        }
    }    // namespace detail

    void console_logging(
        logging_destination dest, std::size_t level, std::string const& s)
    {
        message_type msg(dest, level, s);
        detail::logger().add(msg);
    }

    void cleanup_logging()
    {
        detail::logger().cleanup();
    }

    void activate_logging()
    {
        detail::logger().activate();
    }
}    // namespace hpx::components
