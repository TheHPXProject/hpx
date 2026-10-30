//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/lcos_local/receive_buffer.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iterator>
#include <mutex>
#include <utility>

namespace hpx::lcos::local {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct channel_impl_base
        {
            constexpr channel_impl_base() noexcept
              : count_(0)
            {
            }

            virtual ~channel_impl_base() = default;

            virtual hpx::future<T> get(
                std::size_t generation, bool blocking = false) = 0;
            virtual bool try_get(
                std::size_t generation, hpx::future<T>* f = nullptr) = 0;
            virtual hpx::future<void> set(std::size_t generation, T&& t) = 0;
            virtual std::size_t close(bool force_delete_entries = false) = 0;

            virtual bool requires_delete() noexcept
            {
                return 0 == release();
            }
            virtual void destroy() noexcept
            {
                delete this;
            }

            long use_count() const noexcept
            {
                return count_;
            }
            long addref() noexcept
            {
                return count_.increment();
            }
            long release() noexcept
            {
                return count_.decrement();
            }

        private:
            hpx::util::atomic_count count_;
        };

        // support functions for hpx::intrusive_ptr
        HPX_CXX_CORE_EXPORT template <typename T>
        void intrusive_ptr_add_ref(channel_impl_base<T>* p) noexcept
        {
            p->addref();
        }

        HPX_CXX_CORE_EXPORT template <typename T>
        void intrusive_ptr_release(channel_impl_base<T>* p) noexcept
        {
            if (p->requires_delete())
            {
                // The thread that decrements the reference count to zero must
                // perform an acquire to ensure that it doesn't start
                // destructing the object until all previous writes have
                // drained.
                std::atomic_thread_fence(std::memory_order_acquire);

                p->destroy();
            }
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        class unlimited_channel : public channel_impl_base<T>
        {
            using mutex_type = hpx::spinlock;

        public:
            HPX_NON_COPYABLE(unlimited_channel);

        public:
            constexpr unlimited_channel() noexcept
              : get_generation_(0)
              , set_generation_(0)
              , closed_(false)
            {
            }

        protected:
            hpx::future<T> get(std::size_t generation, bool blocking)
            {
                std::unique_lock<mutex_type> l(mtx_);

                if (buffer_.empty())
                {
                    if (closed_)
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::error::invalid_status,
                                "hpx::lcos::local::channel::get",
                                "this channel is empty and was closed"));
                    }

                    if (blocking && this->use_count() == 1)
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::error::invalid_status,
                                "hpx::lcos::local::channel::get",
                                "this channel is empty and is not accessible "
                                "by any other thread causing a deadlock"));
                    }
                }

                ++get_generation_;
                if (generation == std::size_t(-1))
                {
                    generation = get_generation_;
                }

                if (closed_)
                {
                    // the requested item must be available, otherwise this
                    // would create a deadlock
                    hpx::future<T> f;
                    if (!buffer_.try_receive(generation, &f))
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::error::invalid_status,
                                "hpx::lcos::local::channel::get",
                                "this channel is closed and the requested value"
                                "has not been received yet"));
                    }
                    return f;
                }

                return buffer_.receive(generation);
            }

            bool try_get(std::size_t generation, hpx::future<T>* f = nullptr)
            {
                std::lock_guard<mutex_type> l(mtx_);

                if (buffer_.empty() && closed_)
                {
                    return false;
                }

                ++get_generation_;
                if (generation == std::size_t(-1))
                {
                    generation = get_generation_;
                }

                if (f != nullptr)
                {
                    *f = buffer_.receive(generation);
                }
                return true;
            }

            hpx::future<void> set(std::size_t generation, T&& t)
            {
                std::unique_lock<mutex_type> l(mtx_);
                if (closed_)
                {
                    l.unlock();
                    return hpx::make_exceptional_future<void>(
                        HPX_GET_EXCEPTION(hpx::error::invalid_status,
                            "hpx::lcos::local::channel::set",
                            "attempting to write to a closed channel"));
                }

                ++set_generation_;
                if (generation == std::size_t(-1))
                {
                    generation = set_generation_;
                }

                buffer_.store_received(generation, HPX_MOVE(t), &l);
                return hpx::make_ready_future();
            }

            std::size_t close(bool force_delete_entries = false)
            {
                std::unique_lock<mutex_type> l(mtx_);
                if (closed_)
                {
                    l.unlock();
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "hpx::lcos::local::channel::close",
                        "attempting to close an already closed channel");
                    return 0;
                }

                closed_ = true;

                if (buffer_.empty())
                {
                    return 0;
                }

                std::exception_ptr e;

                {
                    unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    e = HPX_GET_EXCEPTION(hpx::error::future_cancelled,
                        hpx::throwmode::lightweight, "hpx::lcos::local::close",
                        "canceled waiting on this entry");
                }

                // all pending requests that can't be satisfied have to be
                // canceled at this point, force deleting possibly waiting
                // requests
                return buffer_.cancel_waiting(e, force_delete_entries);
            }

        private:
            mutable mutex_type mtx_;
            receive_buffer<T, hpx::no_mutex> buffer_;
            std::size_t get_generation_;
            std::size_t set_generation_;
            bool closed_;
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        class one_element_queue_async
        {
        public:
            HPX_NON_COPYABLE(one_element_queue_async);

        private:
            template <typename T1>
            void set(T1&& val)
            {
                val_ = HPX_FORWARD(T1, val);
                empty_ = false;
                push_active_ = false;
            }

            void set_deferred(T&& val)
            // NVCC with GCC versions less than 10 don't compile push_pt
            // correctly if this is noexcept. hpx::util::result_of (and
            // std::result_of) inside deferred_call does not detect that the
            // call is valid, and compilation fails.
#if !(defined(HPX_CUDA_VERSION) && defined(HPX_GCC_VERSION) &&                 \
    (HPX_GCC_VERSION < 100000))
                noexcept
#endif
            {
                val_ = HPX_MOVE(val);
                empty_ = false;
                push_active_ = false;
            }

            T get() noexcept
            {
                empty_ = true;
                pop_active_ = false;
                return HPX_MOVE(val_);
            }

            template <typename T1>
            hpx::packaged_task<void()> push_pt(T1&& val)
            {
                return hpx::packaged_task<void()>(
                    util::deferred_call(&one_element_queue_async::set_deferred,
                        this, HPX_FORWARD(T1, val)));
            }

            hpx::packaged_task<T()> pop_pt()
            {
                return hpx::packaged_task<T()>([this]() -> T { return get(); });
            }

        public:
            one_element_queue_async() noexcept
              : empty_(true)
              , push_active_(false)
              , pop_active_(false)
            {
            }

            // 26110: Caller failing to hold lock 'l' before calling function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif
            template <typename T1, typename Lock>
            hpx::future<void> push(T1&& val, Lock& l)
            {
                HPX_ASSERT_OWNS_LOCK(l);
                if (!empty_)
                {
                    if (push_active_)
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<void>(
                            HPX_GET_EXCEPTION(hpx::error::invalid_status,
                                "hpx::lcos::local::detail::"
                                "one_element_queue_async::push",
                                "attempting to write to a busy queue"));
                    }

                    push_ = push_pt(HPX_FORWARD(T1, val));
                    push_active_ = true;
                    return push_.get_future();
                }

                set(HPX_FORWARD(T1, val));
                if (pop_active_)
                {
                    // avoid lock-being-held errors
                    util::ignore_while_checking<Lock> il(&l);
                    HPX_UNUSED(il);

                    pop_();    // trigger waiting pop
                }
                return hpx::make_ready_future();
            }

            template <typename Lock>
            std::size_t cancel(std::exception_ptr const& e, Lock& l)
            {
                HPX_ASSERT_OWNS_LOCK(l);

                std::size_t count = 0;
                if (pop_active_)
                {
                    pop_.set_exception(e);
                    pop_active_ = false;
                    ++count;
                }
                if (push_active_)
                {
                    push_.set_exception(e);
                    push_active_ = false;
                    ++count;
                }
                return count;
            }

            template <typename Lock>
            hpx::future<T> pop(Lock& l)
            {
                HPX_ASSERT_OWNS_LOCK(l);
                if (empty_)
                {
                    if (pop_active_)
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::error::invalid_status,
                                "hpx::lcos::local::detail::"
                                "one_element_queue_async::pop",
                                "attempting to read from an empty queue"));
                    }

                    pop_ = pop_pt();
                    pop_active_ = true;
                    return pop_.get_future();
                }

                T val = get();
                if (push_active_)
                {
                    // avoid lock-being-held errors
                    util::ignore_while_checking<Lock> il(&l);
                    HPX_UNUSED(il);

                    push_();    // trigger waiting push
                }
                return hpx::make_ready_future(HPX_MOVE(val));
            }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

            template <typename Lock>
            bool is_empty(Lock& l) const noexcept
            {
                HPX_ASSERT_OWNS_LOCK(l);
                return empty_;
            }

            template <typename Lock>
            bool has_pending_request(Lock& l) const noexcept
            {
                HPX_ASSERT_OWNS_LOCK(l);
                return push_active_;
            }

            template <typename Lock>
            bool has_pending_pop_request(Lock& l) const noexcept
            {
                HPX_ASSERT_OWNS_LOCK(l);
                return pop_active_;
            }

        private:
            T val_;
            hpx::packaged_task<void()> push_;
            hpx::packaged_task<T()> pop_;
            bool empty_;
            bool push_active_;
            bool pop_active_;
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        class one_element_channel : public channel_impl_base<T>
        {
            using mutex_type = hpx::spinlock;

        public:
            HPX_NON_COPYABLE(one_element_channel);

        public:
            one_element_channel() noexcept
              : closed_(false)
            {
            }

        protected:
            hpx::future<T> get(std::size_t, bool blocking)
            {
                std::unique_lock<mutex_type> l(mtx_);

                if (buffer_.is_empty(l) && !buffer_.has_pending_request(l))
                {
                    if (closed_)
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::error::invalid_status,
                                "hpx::lcos::local::channel::get",
                                "this channel is empty and was closed"));
                    }

                    if (blocking && this->use_count() == 1)
                    {
                        l.unlock();
                        return hpx::make_exceptional_future<T>(
                            HPX_GET_EXCEPTION(hpx::error::invalid_status,
                                "hpx::lcos::local::channel::get",
                                "this channel is empty and is not accessible "
                                "by any other thread causing a deadlock"));
                    }
                }

                hpx::future<T> f = buffer_.pop(l);
                if (closed_ && !f.is_ready())
                {
                    // the requested item must be available, otherwise this
                    // would create a deadlock
                    l.unlock();
                    return hpx::make_exceptional_future<T>(
                        HPX_GET_EXCEPTION(hpx::error::invalid_status,
                            "hpx::lcos::local::channel::get",
                            "this channel is closed and the requested value"
                            "has not been received yet"));
                }

                return f;
            }

            bool try_get(std::size_t, hpx::future<T>* f = nullptr)
            {
                std::unique_lock<mutex_type> l(mtx_);

                if (buffer_.is_empty(l) && !buffer_.has_pending_request(l) &&
                    closed_)
                {
                    return false;
                }

                if (f != nullptr)
                {
                    *f = buffer_.pop(l);
                }
                return true;
            }

            hpx::future<void> set(std::size_t, T&& t)
            {
                std::unique_lock<mutex_type> l(mtx_);

                if (closed_)
                {
                    l.unlock();
                    return hpx::make_exceptional_future<void>(
                        HPX_GET_EXCEPTION(hpx::error::invalid_status,
                            "hpx::lcos::local::channel::set",
                            "attempting to write to a closed channel"));
                }

                return buffer_.push(HPX_MOVE(t), l);
            }

            std::size_t close(bool /*force_delete_entries*/ = false)
            {
                std::unique_lock<mutex_type> l(mtx_);

                if (closed_)
                {
                    l.unlock();
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "hpx::lcos::local::channel::close",
                        "attempting to close an already closed channel");
                    return 0;
                }

                closed_ = true;

                if ((buffer_.is_empty(l) || !buffer_.has_pending_request(l)) &&
                    !buffer_.has_pending_pop_request(l))
                {
                    return 0;
                }

                // all pending requests which can't be satisfied have to be
                // canceled at this point
                std::exception_ptr e;
                {
                    unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    e = std::exception_ptr(HPX_GET_EXCEPTION(
                        hpx::error::future_cancelled, "hpx::lcos::local::close",
                        "canceled waiting on this entry"));
                }

                return buffer_.cancel(HPX_MOVE(e), l);
            }

            void set_exception(std::exception_ptr e)
            {
                std::unique_lock<mutex_type> l(mtx_);
                closed_ = true;

                buffer_.cancel(e, l);
            }

        private:
            mutable mutex_type mtx_;
            one_element_queue_async<T> buffer_;
            bool closed_;
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename T>
        class channel_base;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T = void>
    class channel;

    HPX_CXX_CORE_EXPORT template <typename T = void>
    class one_element_channel;

    HPX_CXX_CORE_EXPORT template <typename T = void>
    class receive_channel;

    HPX_CXX_CORE_EXPORT template <typename T = void>
    class send_channel;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    class channel_iterator
      : public hpx::util::iterator_facade<channel_iterator<T>, T const,
            std::input_iterator_tag>
    {
        using base_type = hpx::util::iterator_facade<channel_iterator<T>,
            T const, std::input_iterator_tag>;

    public:
        channel_iterator()
          : channel_(nullptr)
          , data_(T(), false)
        {
        }

        inline explicit channel_iterator(detail::channel_base<T> const* c);
        inline explicit channel_iterator(receive_channel<T> const* c);

    private:
        std::pair<T, bool> get_checked() const
        {
            hpx::future<T> f;
            if (channel_->try_get(std::size_t(-1), &f))
            {
                return std::make_pair(f.get(), true);
            }
            return std::make_pair(T(), false);
        }

        friend class hpx::util::iterator_core_access;

        bool equal(channel_iterator const& rhs) const noexcept
        {
            return (channel_ == rhs.channel_ &&
                       data_.second == rhs.data_.second) ||
                (!data_.second && rhs.channel_ == nullptr) ||
                (channel_ == nullptr && !rhs.data_.second);
        }

        void increment()
        {
            if (channel_)
                data_ = get_checked();
        }

        typename base_type::reference dereference() const noexcept
        {
            HPX_ASSERT(data_.second);
            return data_.first;
        }

    private:
        hpx::intrusive_ptr<detail::channel_impl_base<T>> channel_;
        std::pair<T, bool> data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    class channel_async_iterator
      : public hpx::util::iterator_facade<channel_async_iterator<T>,
            hpx::future<T>, std::input_iterator_tag, hpx::future<T>>
    {
        using base_type = hpx::util::iterator_facade<channel_async_iterator<T>,
            hpx::future<T>, std::input_iterator_tag, hpx::future<T>>;

    public:
        channel_async_iterator()
          : channel_(nullptr)
          , data_(hpx::future<T>(), false)
        {
        }

        inline explicit channel_async_iterator(
            detail::channel_base<T> const* c);

    private:
        std::pair<hpx::future<T>, bool> get_checked() const
        {
            hpx::future<T> f;
            if (channel_->try_get(std::size_t(-1), &f))
            {
                return std::make_pair(HPX_MOVE(f), true);
            }
            return std::make_pair(hpx::future<T>(), false);
        }

        friend class hpx::util::iterator_core_access;

        bool equal(channel_async_iterator const& rhs) const noexcept
        {
            return (channel_ == rhs.channel_ &&
                       data_.second == rhs.data_.second) ||
                (!data_.second && rhs.channel_ == nullptr) ||
                (channel_ == nullptr && !rhs.data_.second);
        }

        void increment()
        {
            if (channel_)
                data_ = get_checked();
        }

        typename base_type::reference dereference() const noexcept
        {
            HPX_ASSERT(data_.second);
            return HPX_MOVE(data_.first);
        }

    private:
        hpx::intrusive_ptr<detail::channel_impl_base<T>> channel_;
        mutable std::pair<hpx::future<T>, bool> data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        class channel_async_range
        {
        public:
            explicit channel_async_range(channel_base<T> const& c) noexcept
              : channel_(c)
            {
            }

            ///////////////////////////////////////////////////////////////////
            channel_async_iterator<T> begin() const
            {
                return channel_async_iterator<T>(&channel_);
            }
            channel_async_iterator<T> end() const
            {
                return channel_async_iterator<T>();
            }

        private:
            channel_base<T> const& channel_;
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        class channel_base
        {
        protected:
            explicit channel_base(channel_impl_base<T>* impl) noexcept
              : channel_(impl)
            {
            }

        public:
            ///////////////////////////////////////////////////////////////////
            hpx::future<T> get(launch::async_policy,
                std::size_t generation = std::size_t(-1)) const
            {
                return channel_->get(generation);
            }
            hpx::future<T> get(std::size_t generation = std::size_t(-1)) const
            {
                return get(launch::async, generation);
            }

            T get(launch::sync_policy, std::size_t generation = std::size_t(-1),
                error_code& ec = throws) const
            {
                return channel_->get(generation, true).get(ec);
            }
            T get(launch::sync_policy, error_code& ec,
                std::size_t generation = std::size_t(-1)) const
            {
                return channel_->get(generation, true).get(ec);
            }

            ///////////////////////////////////////////////////////////////////
            void set(T val, std::size_t generation = std::size_t(-1))
            {
                channel_->set(generation, HPX_MOVE(val)).get();
            }
            void set(launch::sync_policy, T val,
                std::size_t generation = std::size_t(-1))
            {
                channel_->set(generation, HPX_MOVE(val)).get();
            }
            hpx::future<void> set(launch::async_policy, T val,
                std::size_t generation = std::size_t(-1))
            {
                return channel_->set(generation, HPX_MOVE(val));
            }

            std::size_t close(bool force_delete_entries = false)
            {
                return channel_->close(force_delete_entries);
            }

            ///////////////////////////////////////////////////////////////////
            channel_iterator<T> begin() const
            {
                return channel_iterator<T>(this);
            }
            channel_iterator<T> end() const
            {
                return channel_iterator<T>();
            }

            channel_base const& range() const noexcept
            {
                return *this;
            }
            channel_base const& range(launch::sync_policy) const noexcept
            {
                return *this;
            }
            channel_async_range<T> range(launch::async_policy) const
            {
                return channel_async_range<T>(*this);
            }

            ///////////////////////////////////////////////////////////////////
            channel_impl_base<T>* get_channel_impl() const noexcept
            {
                return channel_.get();
            }

        protected:
            hpx::intrusive_ptr<channel_impl_base<T>> channel_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // channel with unlimited buffer
    HPX_CXX_CORE_EXPORT template <typename T>
    class channel : protected detail::channel_base<T>
    {
        using base_type = detail::channel_base<T>;

    private:
        friend class channel_iterator<T>;
        friend class receive_channel<T>;
        friend class send_channel<T>;

    public:
        using value_type = T;

        channel()
          : base_type(new detail::unlimited_channel<T>())
        {
        }

        using base_type::begin;
        using base_type::close;
        using base_type::end;
        using base_type::get;
        using base_type::range;
        using base_type::set;
    };

    // channel with a one-element buffer
    HPX_CXX_CORE_EXPORT template <typename T>
    class one_element_channel : protected detail::channel_base<T>
    {
        using base_type = detail::channel_base<T>;

    private:
        friend class channel_iterator<T>;
        friend class receive_channel<T>;
        friend class send_channel<T>;

    public:
        using value_type = T;

        one_element_channel()
          : base_type(new detail::one_element_channel<T>())
        {
        }

        using base_type::begin;
        using base_type::close;
        using base_type::end;
        using base_type::get;
        using base_type::range;
        using base_type::set;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    class receive_channel : protected detail::channel_base<T>
    {
        using base_type = detail::channel_base<T>;

    private:
        friend class channel_iterator<T>;
        friend class send_channel<T>;

    public:
        receive_channel(channel<T> const& c)
          : base_type(c.get_channel_impl())
        {
        }
        receive_channel(one_element_channel<T> const& c)
          : base_type(c.get_channel_impl())
        {
        }

        using base_type::begin;
        using base_type::end;
        using base_type::get;
        using base_type::range;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    class send_channel : private detail::channel_base<T>
    {
        using base_type = detail::channel_base<T>;

    public:
        send_channel(channel<T> const& c)
          : base_type(c.get_channel_impl())
        {
        }
        send_channel(one_element_channel<T> const& c)
          : base_type(c.get_channel_impl())
        {
        }

        using base_type::close;
        using base_type::set;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    inline channel_iterator<T>::channel_iterator(
        detail::channel_base<T> const* c)
      : channel_(c ? c->get_channel_impl() : nullptr)
      , data_(c ? get_checked() : std::make_pair(T(), false))
    {
    }

    template <typename T>
    inline channel_iterator<T>::channel_iterator(receive_channel<T> const* c)
      : channel_(c ? c->get_channel_impl() : nullptr)
      , data_(c ? get_checked() : std::make_pair(T(), false))
    {
    }

    template <typename T>
    inline channel_async_iterator<T>::channel_async_iterator(
        detail::channel_base<T> const* c)
      : channel_(c ? c->get_channel_impl() : nullptr)
      , data_(c ? get_checked() : std::make_pair(hpx::future<T>(), false))
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    // forward declare specializations
    template <>
    class channel<void>;

    template <>
    class receive_channel<void>;

    template <>
    class send_channel<void>;

    ///////////////////////////////////////////////////////////////////////////
    template <>
    class channel_iterator<void>
      : public hpx::util::iterator_facade<channel_iterator<void>,
            util::unused_type const, std::input_iterator_tag>
    {
        using base_type = hpx::util::iterator_facade<channel_iterator<void>,
            util::unused_type const, std::input_iterator_tag>;

    public:
        channel_iterator() noexcept
          : channel_(nullptr)
          , data_(false)
        {
        }

        inline explicit channel_iterator(detail::channel_base<void> const* c);
        inline explicit channel_iterator(receive_channel<void> const* c);

    private:
        bool get_checked()
        {
            hpx::future<util::unused_type> f;
            if (channel_->try_get(std::size_t(-1), &f))
            {
                f.get();
                return true;
            }
            return false;
        }

        friend class hpx::util::iterator_core_access;

        bool equal(channel_iterator const& rhs) const noexcept
        {
            return (channel_ == rhs.channel_ && data_ == rhs.data_) ||
                (!data_ && rhs.channel_ == nullptr) ||
                (channel_ == nullptr && !rhs.data_);
        }

        void increment()
        {
            if (channel_)
                data_ = get_checked();
        }

        base_type::reference dereference() const noexcept
        {
            HPX_ASSERT(data_);
            return util::unused;
        }

    private:
        hpx::intrusive_ptr<detail::channel_impl_base<util::unused_type>>
            channel_;
        bool data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <>
        class channel_base<void>
        {
        public:
            explicit channel_base(
                detail::channel_impl_base<util::unused_type>* impl) noexcept
              : channel_(impl)
            {
            }

            ///////////////////////////////////////////////////////////////////////
            hpx::future<void> get(launch::async_policy,
                std::size_t generation = std::size_t(-1)) const
            {
                return channel_->get(generation);
            }
            hpx::future<void> get(
                std::size_t generation = std::size_t(-1)) const
            {
                return get(launch::async, generation);
            }
            void get(launch::sync_policy,
                std::size_t generation = std::size_t(-1),
                error_code& ec = throws) const
            {
                channel_->get(generation, true).get(ec);
            }
            void get(launch::sync_policy, error_code& ec,
                std::size_t generation = std::size_t(-1)) const
            {
                channel_->get(generation, true).get(ec);
            }

            ///////////////////////////////////////////////////////////////////////
            void set(std::size_t generation = std::size_t(-1))
            {
                channel_->set(generation, hpx::util::unused_type()).get();
            }
            void set(
                launch::sync_policy, std::size_t generation = std::size_t(-1))
            {
                channel_->set(generation, hpx::util::unused_type()).get();
            }
            hpx::future<void> set(
                launch::async_policy, std::size_t generation = std::size_t(-1))
            {
                return channel_->set(generation, hpx::util::unused_type());
            }

            std::size_t close(bool force_delete_entries = false)
            {
                return channel_->close(force_delete_entries);
            }

            ///////////////////////////////////////////////////////////////////
            channel_iterator<void> begin() const
            {
                return channel_iterator<void>(this);
            }
            channel_iterator<void> end() const
            {
                return channel_iterator<void>();
            }

            channel_base const& range() const noexcept
            {
                return *this;
            }
            channel_base const& range(launch::sync_policy) const noexcept
            {
                return *this;
            }
            channel_async_range<void> range(launch::async_policy) const
            {
                return channel_async_range<void>(*this);
            }

            ///////////////////////////////////////////////////////////////////
            constexpr channel_impl_base<util::unused_type>* get_channel_impl()
                const noexcept
            {
                return channel_.get();
            }

        protected:
            hpx::intrusive_ptr<channel_impl_base<util::unused_type>> channel_;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <>
    class channel<void> : protected detail::channel_base<void>
    {
        using base_type = detail::channel_base<void>;

    private:
        friend class channel_iterator<void>;
        friend class receive_channel<void>;
        friend class send_channel<void>;

    public:
        using value_type = void;

        channel()
          : base_type(new detail::unlimited_channel<util::unused_type>())
        {
        }

        using base_type::begin;
        using base_type::close;
        using base_type::end;
        using base_type::get;
        using base_type::range;
        using base_type::set;
    };

    template <>
    class one_element_channel<void> : protected detail::channel_base<void>
    {
        using base_type = detail::channel_base<void>;

    private:
        friend class channel_iterator<void>;
        friend class receive_channel<void>;
        friend class send_channel<void>;

    public:
        using value_type = void;

        one_element_channel()
          : base_type(new detail::one_element_channel<util::unused_type>())
        {
        }

        using base_type::begin;
        using base_type::close;
        using base_type::end;
        using base_type::get;
        using base_type::range;
        using base_type::set;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <>
    class receive_channel<void> : protected detail::channel_base<void>
    {
        using base_type = detail::channel_base<void>;

    private:
        friend class channel_iterator<void>;
        friend class send_channel<void>;

    public:
        receive_channel(channel<void> const& c)
          : base_type(c.get_channel_impl())
        {
        }
        receive_channel(one_element_channel<void> const& c)
          : base_type(c.get_channel_impl())
        {
        }

        using base_type::begin;
        using base_type::end;
        using base_type::get;
        using base_type::range;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <>
    class send_channel<void> : private detail::channel_base<void>
    {
        using base_type = detail::channel_base<void>;

    public:
        send_channel(channel<void> const& c)
          : base_type(c.get_channel_impl())
        {
        }
        send_channel(one_element_channel<void> const& c)
          : base_type(c.get_channel_impl())
        {
        }

        using base_type::close;
        using base_type::set;
    };

    ///////////////////////////////////////////////////////////////////////////
    inline channel_iterator<void>::channel_iterator(
        detail::channel_base<void> const* c)
      : channel_(c ? c->get_channel_impl() : nullptr)
      , data_(c ? get_checked() : false)
    {
    }

    inline channel_iterator<void>::channel_iterator(
        receive_channel<void> const* c)
      : channel_(c ? c->get_channel_impl() : nullptr)
      , data_(c ? get_checked() : false)
    {
    }
}    // namespace hpx::lcos::local

namespace hpx {

    HPX_CXX_CORE_EXPORT using hpx::lcos::local::channel;
}
