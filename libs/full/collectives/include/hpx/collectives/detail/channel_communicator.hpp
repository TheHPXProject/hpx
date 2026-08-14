//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/synchronization.hpp>

#include <cstddef>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

namespace hpx::collectives::detail {

    ///////////////////////////////////////////////////////////////////////////
    class channel_communicator_server
      : public hpx::components::component_base<channel_communicator_server>
    {
    private:
        using channel_type =
            lcos::local::one_element_channel<unique_any_nonser>;

    public:
        channel_communicator_server()    //-V730
        {
            HPX_ASSERT(false);    // shouldn't ever be called
        }

        explicit channel_communicator_server(std::size_t num_sites)
          : data_(num_sites)
        {
            HPX_ASSERT(num_sites != 0);
        }

        template <typename T>
        hpx::future<T> get(std::size_t which, std::size_t tag) const
        {
            hpx::future<unique_any_nonser> f;

            {
                std::unique_lock l(data_[which].mtx_);
                [[maybe_unused]] util::ignore_while_checking il(&l);

                channel_type& c = data_[which].channels_[tag];
                f = c.get();
            }

            return f.then(
                hpx::launch::sync, [](hpx::future<unique_any_nonser>&& f) -> T {
                    return hpx::any_cast<T const&>(f.get());
                });
        }

        template <typename T>
        struct get_action
          : hpx::actions::make_action<hpx::future<T> (
                                          channel_communicator_server::*)(
                                          std::size_t, std::size_t) const,
                &channel_communicator_server::template get<T>,
                get_action<T>>::type
        {
        };

        template <typename T>
        void set(std::size_t which, T value, std::size_t tag)
        {
            std::unique_lock l(data_[which].mtx_);
            [[maybe_unused]] util::ignore_while_checking il(&l);

            data_[which].channels_[tag].set(unique_any_nonser(HPX_MOVE(value)));
        }

        template <typename T>
        struct set_action
          : hpx::actions::make_action<void (channel_communicator_server::*)(
                                          std::size_t, T, std::size_t),
                &channel_communicator_server::template set<T>,
                set_action<T>>::type
        {
        };

    private:
        struct locality_data
        {
            hpx::spinlock mtx_;
            std::map<std::size_t, channel_type> channels_;
        };

        mutable std::vector<locality_data> data_;
    };

    ///////////////////////////////////////////////////////////////////////////
    class channel_communicator
    {
    private:
        using client_type = components::client<channel_communicator_server>;

    public:
        HPX_EXPORT channel_communicator(char const* basename,
            std::size_t num_sites, std::size_t this_site, client_type here);

        HPX_EXPORT channel_communicator(hpx::launch::sync_policy,
            char const* basename, std::size_t num_sites, std::size_t this_site,
            client_type here);

        channel_communicator(channel_communicator const& rhs) = delete;
        channel_communicator(channel_communicator&& rhs) noexcept = delete;

        channel_communicator& operator=(
            channel_communicator const& rhs) = delete;
        channel_communicator& operator=(
            channel_communicator&& rhs) noexcept = delete;

        HPX_EXPORT ~channel_communicator();

        template <typename T>
        hpx::future<T> get(std::size_t site, std::size_t tag) const
        {
            // all get operations refer to the channels located on this site
            using action_type =
                channel_communicator_server::template get_action<T>;
            return hpx::sync(action_type(), clients_[this_site_], site, tag);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        hpx::future<void> set(std::size_t site, T&& value, std::size_t tag)
        {
            // all set operations refer to the channel on the target site
            using action_type =
                channel_communicator_server::template set_action<
                    std::decay_t<T>>;
            return hpx::async(action_type(), clients_[site], this_site_,
                HPX_FORWARD(T, value), tag);
        }

        [[nodiscard]] std::pair<std::size_t, std::size_t> get_info()
            const noexcept
        {
            return std::make_pair(clients_.size(), this_site_);
        }

    private:
        std::size_t this_site_;
        std::vector<client_type> clients_;
    };
}    // namespace hpx::collectives::detail

#endif    // COMPUTE_HOST_CODE
