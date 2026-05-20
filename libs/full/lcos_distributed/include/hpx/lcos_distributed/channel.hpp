//  Copyright (c) 2016-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/lcos_distributed/server/channel.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/runtime_components.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::lcos {

    ///////////////////////////////////////////////////////////////////////////
    template <typename T = void>
    class channel;
    template <typename T = void>
    class receive_channel;
    template <typename T = void>
    class send_channel;

    ///////////////////////////////////////////////////////////////////////////
    template <typename T, typename Channel>
    class channel_iterator
      : public hpx::util::iterator_facade<channel_iterator<T, Channel>, T const,
            std::input_iterator_tag>
    {
        using base_type =
            hpx::util::iterator_facade<channel_iterator<T, Channel>, T const,
                std::input_iterator_tag>;

    public:
        channel_iterator()
          : channel_(nullptr)
          , data_(T(), false)
        {
        }

        explicit channel_iterator(Channel const& c)
          : channel_(&c)
          , data_(T(), false)
        {
            data_ = get_checked();
        }

    private:
        std::pair<T, bool> get_checked() const
        {
            hpx::future<T> f = channel_->get();
            f.wait();
            if (!f.has_exception())
            {
                return std::make_pair(f.get(), true);
            }
            return std::make_pair(T(), false);
        }

        friend class hpx::util::iterator_core_access;

        bool equal(channel_iterator const& rhs) const
        {
            return (data_.second == rhs.data_.second &&
                       (channel_ == rhs.channel_ ||
                           (channel_ != nullptr && rhs.channel_ != nullptr &&
                               channel_->get_id() ==
                                   rhs.channel_->get_id()))) ||
                (!data_.second && rhs.channel_ == nullptr) ||
                (channel_ == nullptr && !rhs.data_.second);
        }

        void increment()
        {
            if (channel_)
                data_ = get_checked();
        }

        typename base_type::reference dereference() const
        {
            HPX_ASSERT(data_.second);
            return data_.first;
        }

    private:
        Channel const* channel_;
        std::pair<T, bool> data_;
    };

    template <typename Channel>
    class channel_iterator<void, Channel>
      : public hpx::util::iterator_facade<channel_iterator<void, Channel>,
            util::unused_type const, std::input_iterator_tag>
    {
        using base_type =
            hpx::util::iterator_facade<channel_iterator<void, Channel>,
                util::unused_type const, std::input_iterator_tag>;

    public:
        channel_iterator()
          : channel_(nullptr)
          , data_(false)
        {
        }

        explicit channel_iterator(Channel const& c)
          : channel_(&c)
          , data_(get_checked())
        {
        }

    private:
        bool get_checked()
        {
            hpx::future<void> f = channel_->get();
            f.wait();
            return !f.has_exception();
        }

        friend class hpx::util::iterator_core_access;

        bool equal(channel_iterator const& rhs) const
        {
            return (data_ == rhs.data_ &&
                       (channel_ == rhs.channel_ ||
                           (channel_ != nullptr && rhs.channel_ != nullptr &&
                               channel_->get_id() ==
                                   rhs.channel_->get_id()))) ||
                (!data_ && rhs.channel_ == nullptr) ||
                (channel_ == nullptr && !rhs.data_);
        }

        void increment()
        {
            if (channel_)
                data_ = get_checked();
        }

        typename base_type::reference dereference() const
        {
            HPX_ASSERT(data_);
            return util::unused;
        }

    private:
        Channel const* channel_;
        bool data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    class channel
      : public components::client_base<channel<T>, lcos::server::channel<T>>
    {
        using base_type =
            components::client_base<channel<T>, lcos::server::channel<T>>;

        static constexpr std::size_t default_generation = std::size_t(-1);

    public:
        // FIXME: this typedef is currently needed for traits::is_future_range
        // to work properly. This typedef is to workaround that defect in the
        // trait implementation which expects the value_type typedef once begin
        // and end members are present.
        using value_type = T;

        channel() = default;

        // create a new instance of a channel component
        explicit channel(hpx::id_type const& loc)
          : base_type(hpx::new_<lcos::server::channel<T>>(loc))
        {
        }

        explicit channel(hpx::future<hpx::id_type>&& id)
          : base_type(HPX_MOVE(id))
        {
        }

        explicit channel(hpx::shared_future<hpx::id_type>&& id)
          : base_type(HPX_MOVE(id))
        {
        }

        explicit channel(hpx::shared_future<hpx::id_type> const& id)
          : base_type(id)
        {
        }

        ///////////////////////////////////////////////////////////////////////
        hpx::future<T> get(launch::async_policy,
            std::size_t generation = default_generation) const
        {
            using action_type =
                typename lcos::server::channel<T>::get_generation_action;
            return hpx::async(action_type(), this->get_id(), generation);
        }
        hpx::future<T> get(std::size_t generation = default_generation) const
        {
            return get(launch::async, generation);
        }
        T get(launch::sync_policy, std::size_t generation = default_generation,
            hpx::error_code& ec = hpx::throws) const
        {
            return get(generation).get(ec);
        }
        T get(launch::sync_policy, hpx::error_code& ec,
            std::size_t generation = default_generation) const
        {
            return get(launch::sync, generation, ec);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename U, typename U2 = T>
        std::enable_if_t<!std::is_void<U2>::value, bool> set(
            launch::apply_policy, U val,
            std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<T>::set_generation_action;
            return hpx::post(
                action_type(), this->get_id(), HPX_MOVE(val), generation);
        }
        template <typename U, typename U2 = T>
        std::enable_if_t<!std::is_void<U2>::value, hpx::future<void>> set(
            launch::async_policy, U val,
            std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<T>::set_generation_action;
            return hpx::async(
                action_type(), this->get_id(), HPX_MOVE(val), generation);
        }
        template <typename U, typename U2 = T>
        std::enable_if_t<!std::is_void<U2>::value> set(launch::sync_policy,
            U val, std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<T>::set_generation_action;
            action_type()(this->get_id(), HPX_MOVE(val), generation);
        }
        template <typename U, typename U2 = T>
        std::enable_if_t<!std::is_void<U2>::value &&
            !traits::is_launch_policy<U>::value>
        set(U val, std::size_t generation = default_generation)
        {
            set(launch::sync, HPX_MOVE(val), generation);
        }

        template <typename U = T>
        std::enable_if_t<std::is_void<U>::value, bool> set(
            launch::apply_policy, std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<void>::set_generation_action;
            hpx::util::unused_type unused;
            return hpx::post(
                action_type(), this->get_id(), HPX_MOVE(unused), generation);
        }
        template <typename U = T>
        std::enable_if_t<std::is_void<U>::value, hpx::future<void>> set(
            launch::async_policy, std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<void>::set_generation_action;
            hpx::util::unused_type unused;
            return hpx::async(
                action_type(), this->get_id(), HPX_MOVE(unused), generation);
        }
        template <typename U = T>
        std::enable_if_t<std::is_void<U>::value> set(
            launch::sync_policy, std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<void>::set_generation_action;
            hpx::util::unused_type unused;
            action_type()(this->get_id(), HPX_MOVE(unused), generation);
        }
        template <typename U = T>
        std::enable_if_t<std::is_void<U>::value> set(
            std::size_t generation = default_generation)
        {
            set(launch::sync, generation);
        }

        ///////////////////////////////////////////////////////////////////////
        void close(launch::apply_policy, bool force_delete_entries = false)
        {
            using action_type = typename lcos::server::channel<T>::close_action;
            hpx::post(action_type(), this->get_id(), force_delete_entries);
        }
        hpx::future<std::size_t> close(
            launch::async_policy, bool force_delete_entries = false)
        {
            using action_type = typename lcos::server::channel<T>::close_action;
            return hpx::async(
                action_type(), this->get_id(), force_delete_entries);
        }
        std::size_t close(
            launch::sync_policy, bool force_delete_entries = false)
        {
            using action_type = typename lcos::server::channel<T>::close_action;
            return action_type()(this->get_id(), force_delete_entries);
        }
        std::size_t close(bool force_delete_entries = false)
        {
            return close(launch::sync, force_delete_entries);
        }

        ///////////////////////////////////////////////////////////////////////
        channel_iterator<T, channel<T>> begin() const
        {
            return channel_iterator<T, channel<T>>(*this);
        }
        channel_iterator<T, channel<T>> end() const
        {
            return channel_iterator<T, channel<T>>();
        }

        channel_iterator<T, channel<T>> rbegin() const
        {
            return channel_iterator<T, channel<T>>(*this);
        }
        channel_iterator<T, channel<T>> rend() const
        {
            return channel_iterator<T, channel<T>>();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    class receive_channel
      : public components::client_base<receive_channel<T>,
            lcos::server::channel<T>>
    {
        using base_type = components::client_base<receive_channel<T>,
            lcos::server::channel<T>>;

        static constexpr std::size_t default_generation = std::size_t(-1);

    public:
        // FIXME: this typedef is currently needed for traits::is_future_range
        // to work properly. This typedef is to workaround that defect in the
        // trait implementation which expects the value_type typedef once begin
        // and end members are present.
        using value_type = T;

        receive_channel() = default;

        receive_channel(channel<T> const& c)
          : base_type(c.get_id())
        {
        }

        explicit receive_channel(hpx::future<hpx::id_type>&& id)
          : base_type(HPX_MOVE(id))
        {
        }

        explicit receive_channel(hpx::shared_future<hpx::id_type>&& id)
          : base_type(HPX_MOVE(id))
        {
        }

        explicit receive_channel(hpx::shared_future<hpx::id_type> const& id)
          : base_type(id)
        {
        }

        ///////////////////////////////////////////////////////////////////////
        hpx::future<T> get(launch::async_policy,
            std::size_t generation = default_generation) const
        {
            using action_type =
                typename lcos::server::channel<T>::get_generation_action;
            return hpx::async(action_type(), this->get_id(), generation);
        }
        hpx::future<T> get(std::size_t generation = default_generation) const
        {
            return get(launch::async, generation);
        }
        T get(launch::sync_policy, std::size_t generation = default_generation,
            hpx::error_code& ec = hpx::throws) const
        {
            return get(generation).get(ec);
        }
        T get(launch::sync_policy, hpx::error_code& ec,
            std::size_t generation = default_generation) const
        {
            return get(launch::sync, generation, ec);
        }

        ///////////////////////////////////////////////////////////////////////
        channel_iterator<T, channel<T>> begin() const
        {
            return channel_iterator<T, channel<T>>(*this);
        }
        channel_iterator<T, channel<T>> end() const
        {
            return channel_iterator<T, channel<T>>();
        }

        channel_iterator<T, channel<T>> rbegin() const
        {
            return channel_iterator<T, channel<T>>(*this);
        }
        channel_iterator<T, channel<T>> rend() const
        {
            return channel_iterator<T, channel<T>>();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    class send_channel
      : public components::client_base<send_channel<T>,
            lcos::server::channel<T>>
    {
        using base_type =
            components::client_base<send_channel<T>, lcos::server::channel<T>>;

        static constexpr std::size_t default_generation = std::size_t(-1);

    public:
        // FIXME: this typedef is currently needed for traits::is_future_range
        // to work properly. This typedef is to workaround that defect in the
        // trait implementation which expects the value_type typedef once begin
        // and end members are present.
        using value_type = T;

        send_channel() = default;

        send_channel(channel<T> const& c)
          : base_type(c.get_id())
        {
        }

        explicit send_channel(hpx::future<hpx::id_type>&& id)
          : base_type(HPX_MOVE(id))
        {
        }

        explicit send_channel(hpx::shared_future<hpx::id_type>&& id)
          : base_type(HPX_MOVE(id))
        {
        }

        explicit send_channel(hpx::shared_future<hpx::id_type> const& id)
          : base_type(id)
        {
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename U, typename U2 = T>
        std::enable_if_t<!std::is_void<U2>::value, bool> set(
            launch::apply_policy, U val,
            std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<T>::set_generation_action;
            return hpx::post(
                action_type(), this->get_id(), HPX_MOVE(val), generation);
        }
        template <typename U, typename U2 = T>
        std::enable_if_t<!std::is_void<U2>::value, hpx::future<void>> set(
            launch::async_policy, U val,
            std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<T>::set_generation_action;
            return hpx::async(
                action_type(), this->get_id(), HPX_MOVE(val), generation);
        }
        template <typename U, typename U2 = T>
        std::enable_if_t<!std::is_void<U2>::value> set(launch::sync_policy,
            U val, std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<T>::set_generation_action;
            action_type()(this->get_id(), HPX_MOVE(val), generation);
        }
        template <typename U, typename U2 = T>
        std::enable_if_t<!std::is_void<U2>::value &&
            !traits::is_launch_policy<U>::value>
        set(U val, std::size_t generation = default_generation)
        {
            set(launch::sync, HPX_MOVE(val), generation);
        }

        template <typename U = T>
        std::enable_if_t<std::is_void<U>::value, bool> set(
            launch::apply_policy, std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<void>::set_generation_action;
            hpx::util::unused_type unused;
            return hpx::post(
                action_type(), this->get_id(), HPX_MOVE(unused), generation);
        }
        template <typename U = T>
        std::enable_if_t<std::is_void<U>::value, hpx::future<void>> set(
            launch::async_policy, std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<void>::set_generation_action;
            hpx::util::unused_type unused;
            return hpx::async(
                action_type(), this->get_id(), HPX_MOVE(unused), generation);
        }
        template <typename U = T>
        std::enable_if_t<std::is_void<U>::value> set(
            launch::sync_policy, std::size_t generation = default_generation)
        {
            using action_type =
                typename lcos::server::channel<void>::set_generation_action;
            hpx::util::unused_type unused;
            action_type()(this->get_id(), HPX_MOVE(unused), generation);
        }
        template <typename U = T>
        std::enable_if_t<std::is_void<U>::value> set(
            std::size_t generation = default_generation)
        {
            set(launch::sync, generation);
        }

        ///////////////////////////////////////////////////////////////////////
        void close(launch::apply_policy, bool force_delete_entries = false)
        {
            using action_type = typename lcos::server::channel<T>::close_action;
            hpx::post(action_type(), this->get_id(), force_delete_entries);
        }
        hpx::future<std::size_t> close(
            launch::async_policy, bool force_delete_entries = false)
        {
            using action_type = typename lcos::server::channel<T>::close_action;
            return hpx::async(
                action_type(), this->get_id(), force_delete_entries);
        }
        std::size_t close(
            launch::sync_policy, bool force_delete_entries = false)
        {
            using action_type = typename lcos::server::channel<T>::close_action;
            return action_type()(this->get_id(), force_delete_entries);
        }
        std::size_t close(bool force_delete_entries = false)
        {
            return close(launch::sync, force_delete_entries);
        }
    };
}    // namespace hpx::lcos

namespace hpx::distributed {

    using hpx::lcos::channel;
}    // namespace hpx::distributed

#endif
