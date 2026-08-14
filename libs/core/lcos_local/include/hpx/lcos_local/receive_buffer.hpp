//  Copyright (c) 2014-2024 Hartmut Kaiser
//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/synchronization.hpp>

#include <cstddef>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace hpx::lcos::local {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Mutex = hpx::spinlock>
    struct receive_buffer
    {
    protected:
        using mutex_type = Mutex;
        using buffer_promise_type = hpx::promise<T>;

        struct entry_data
        {
            HPX_NON_COPYABLE(entry_data);

            entry_data() noexcept
              : can_be_deleted_(false)
              , value_set_(false)
            {
            }

            hpx::future<T> get_future()
            {
                return promise_.get_future();
            }

            template <typename Val>
            void set_value(Val&& val)
            {
                value_set_ = true;
                promise_.set_value(HPX_FORWARD(Val, val));
            }

            bool cancel(std::exception_ptr const& e)
            {
                HPX_ASSERT(can_be_deleted_);
                if (!value_set_)
                {
                    promise_.set_exception(e);
                    return true;
                }
                return false;
            }

            buffer_promise_type promise_;
            bool can_be_deleted_;
            bool value_set_;
        };

        using buffer_map_type =
            std::map<std::size_t, std::shared_ptr<entry_data>>;
        using iterator = typename buffer_map_type::iterator;

    public:
        receive_buffer() = default;

        ~receive_buffer()
        {
            HPX_ASSERT(buffer_map_.empty());
        }

        receive_buffer(receive_buffer&& other) noexcept
          : mtx_()
          , buffer_map_(HPX_MOVE(other.buffer_map_))
        {
        }

        receive_buffer& operator=(receive_buffer&& other) noexcept
        {
            if (this != &other)
            {
                mtx_ = mutex_type();
                buffer_map_ = HPX_MOVE(other.buffer_map_);
            }
            return *this;
        }

        hpx::future<T> receive(std::size_t step)
        {
            std::lock_guard<mutex_type> l(mtx_);

            iterator it = get_buffer_entry(step);
            HPX_ASSERT(it != buffer_map_.end());

            // if the value was already set we delete the entry after retrieving
            // the future
            auto& elem = it->second;
            if (elem->can_be_deleted_)
            {
                auto on_exit = hpx::experimental::scope_exit(
                    [this, &it] { buffer_map_.erase(it); });
                return it->second->get_future();
            }

            // otherwise mark the entry as to be deleted once the value was set
            elem->can_be_deleted_ = true;
            return elem->get_future();
        }

        bool try_receive(std::size_t step, hpx::future<T>* f = nullptr)
        {
            std::lock_guard<mutex_type> l(mtx_);

            iterator it = buffer_map_.find(step);
            if (it == buffer_map_.end())
                return false;

            // if the value was already set we delete the entry after
            // retrieving the future
            auto& elem = it->second;
            if (elem->can_be_deleted_)
            {
                if (f != nullptr)
                {
                    auto on_exit = hpx::experimental::scope_exit(
                        [this, &it] { buffer_map_.erase(it); });
                    *f = elem->get_future();
                }
                return true;
            }

            // otherwise mark the entry as to be deleted once the value was set
            if (f != nullptr)
            {
                elem->can_be_deleted_ = true;
                *f = elem->get_future();
            }
            return true;
        }

        // 26110: Caller failing to hold lock 'l' before calling function
        // 26117: Releasing unheld lock '*lock' in function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110 26117)
#endif
        template <typename Lock = hpx::no_mutex>
        void store_received(std::size_t step, T&& val, Lock* lock = nullptr)
        {
            std::shared_ptr<entry_data> entry;

            {
                std::unique_lock<mutex_type> l(mtx_);

                iterator it = get_buffer_entry(step);
                HPX_ASSERT_LOCKED(l, it != buffer_map_.end());

                entry = it->second;

                if (!entry->can_be_deleted_)
                {
                    // if the future was not retrieved yet mark the entry as to
                    // be deleted after it was retrieved
                    entry->can_be_deleted_ = true;
                }
                else
                {
                    // if the future was already retrieved we can delete the
                    // entry now
                    buffer_map_.erase(it);
                }
            }

            if (lock)
                lock->unlock();

            // set value in promise, but only after the lock went out of scope
            entry->set_value(HPX_MOVE(val));
        }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

        bool empty() const noexcept
        {
            return buffer_map_.empty();
        }

        // return the number of deleted buffer entries
        std::size_t cancel_waiting(
            std::exception_ptr const& e, bool force_delete_entries = false)
        {
            std::lock_guard<mutex_type> l(mtx_);

            std::size_t count = 0;
            iterator end = buffer_map_.end();
            for (iterator it = buffer_map_.begin(); it != end; /**/)
            {
                iterator to_delete = it++;
                if (to_delete->second->cancel(e) || force_delete_entries)
                {
                    buffer_map_.erase(to_delete);
                    ++count;
                }
            }
            return count;
        }

    protected:
        iterator get_buffer_entry(std::size_t step)
        {
            iterator it = buffer_map_.find(step);
            if (it == buffer_map_.end())
            {
                std::pair<iterator, bool> res = buffer_map_.insert(
                    std::make_pair(step, std::make_shared<entry_data>()));
                if (!res.second)
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "base_receive_buffer::get_buffer_entry",
                        "couldn't insert a new entry into the receive buffer");
                }
                return res.first;
            }
            return it;
        }

    private:
        mutable mutex_type mtx_;
        buffer_map_type buffer_map_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Mutex>
    struct receive_buffer<void, Mutex>
    {
    protected:
        using mutex_type = Mutex;
        using buffer_promise_type = hpx::promise<void>;

        struct entry_data
        {
            HPX_NON_COPYABLE(entry_data);

            entry_data() noexcept
              : can_be_deleted_(false)
              , value_set_(false)
            {
            }

            hpx::future<void> get_future()
            {
                return promise_.get_future();
            }

            void set_value()
            {
                value_set_ = true;
                promise_.set_value();
            }

            bool cancel(std::exception_ptr const& e)
            {
                HPX_ASSERT(can_be_deleted_);
                if (!value_set_)
                {
                    promise_.set_exception(e);
                    return true;
                }
                return false;
            }

            buffer_promise_type promise_;
            bool can_be_deleted_;
            bool value_set_;
        };

        using buffer_map_type =
            std::map<std::size_t, std::shared_ptr<entry_data>>;
        using iterator = typename buffer_map_type::iterator;

    public:
        receive_buffer() = default;

        ~receive_buffer()
        {
            HPX_ASSERT(buffer_map_.empty());
        }

        receive_buffer(receive_buffer&& other) noexcept
          : buffer_map_(HPX_MOVE(other.buffer_map_))
        {
        }

        receive_buffer& operator=(receive_buffer&& other) noexcept
        {
            if (this != &other)
            {
                buffer_map_ = HPX_MOVE(other.buffer_map_);
            }
            return *this;
        }

        hpx::future<void> receive(std::size_t step)
        {
            std::lock_guard<mutex_type> l(mtx_);

            iterator it = get_buffer_entry(step);
            HPX_ASSERT(it != buffer_map_.end());

            // if the value was already set we delete the entry after
            // retrieving the future
            auto& elem = it->second;
            if (elem->can_be_deleted_)
            {
                auto on_exit = hpx::experimental::scope_exit(
                    [this, &it] { buffer_map_.erase(it); });
                return elem->get_future();
            }

            // otherwise mark the entry as to be deleted once the value was set
            elem->can_be_deleted_ = true;
            return elem->get_future();
        }

        bool try_receive(std::size_t step, hpx::future<void>* f = nullptr)
        {
            std::lock_guard<mutex_type> l(mtx_);

            iterator it = buffer_map_.find(step);
            if (it == buffer_map_.end())
                return false;

            // if the value was already set we delete the entry after
            // retrieving the future
            auto& elem = it->second;
            if (elem->can_be_deleted_)
            {
                if (f != nullptr)
                {
                    auto on_exit = hpx::experimental::scope_exit(
                        [this, &it] { buffer_map_.erase(it); });
                    *f = elem->get_future();
                }
                return true;
            }

            // otherwise mark the entry as to be deleted once the value was set
            if (f != nullptr)
            {
                elem->can_be_deleted_ = true;
                *f = elem->get_future();
            }
            return true;
        }

        template <typename Lock = hpx::no_mutex>
        void store_received(std::size_t step, Lock* lock = nullptr)
        {
            std::shared_ptr<entry_data> entry;

            {
                std::lock_guard<mutex_type> l(mtx_);

                iterator it = get_buffer_entry(step);
                HPX_ASSERT(it != buffer_map_.end());

                entry = it->second;

                if (!entry->can_be_deleted_)
                {
                    // if the future was not retrieved yet mark the entry as to
                    // be deleted after it was retrieved
                    entry->can_be_deleted_ = true;
                }
                else
                {
                    // if the future was already retrieved we can delete the
                    // entry now
                    buffer_map_.erase(it);
                }
            }

            if (lock)
                lock->unlock();

            // set value in promise, but only after the lock went out of scope
            entry->set_value();
        }

        bool empty() const noexcept
        {
            return buffer_map_.empty();
        }

        // return the number of deleted buffer entries
        std::size_t cancel_waiting(
            std::exception_ptr const& e, bool force_delete_entries = false)
        {
            std::lock_guard<mutex_type> l(mtx_);

            std::size_t count = 0;
            iterator end = buffer_map_.end();
            for (iterator it = buffer_map_.begin(); it != end; /**/)
            {
                iterator to_delete = it++;
                if (to_delete->second->cancel(e) || force_delete_entries)
                {
                    buffer_map_.erase(to_delete);
                    ++count;
                }
            }
            return count;
        }

    protected:
        iterator get_buffer_entry(std::size_t step)
        {
            iterator it = buffer_map_.find(step);
            if (it == buffer_map_.end())
            {
                std::pair<iterator, bool> res =
                    buffer_map_.emplace(step, std::make_shared<entry_data>());
                if (!res.second)
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "base_receive_buffer::get_buffer_entry",
                        "couldn't insert a new entry into the receive buffer");
                }
                return res.first;
            }
            return it;
        }

    private:
        mutable mutex_type mtx_;
        buffer_map_type buffer_map_;
    };
}    // namespace hpx::lcos::local
