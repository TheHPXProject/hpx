//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lcos_local.hpp>
#include <hpx/modules/preprocessor.hpp>

#include <hpx/lcos_distributed/macros.hpp>

#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::server {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T,
        typename RemoteType = traits::promise_remote_result_t<T>>
    class channel;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename RemoteType>
    class channel
      : public lcos::base_lco_with_value<T, RemoteType,
            traits::detail::component_tag>
      , public components::component_base<channel<T, RemoteType>>
    {
    public:
        using base_type_holder = lcos::base_lco_with_value<T, RemoteType,
            traits::detail::component_tag>;

    private:
        using base_type = components::component_base<channel>;
        using result_type =
            std::conditional_t<std::is_void<T>::value, util::unused_type, T>;

    public:
        channel() = default;

        // disambiguate base classes
        using base_type::finalize;
        using wrapping_type = typename base_type::wrapping_type;

        static components::component_type get_component_type()
        {
            return components::get_component_type<channel>();
        }
        static void set_component_type(components::component_type type)
        {
            components::set_component_type<channel>(type);
        }

        naming::address get_current_address() const
        {
            return naming::address(
                naming::get_gid_from_locality_id(agas::get_locality_id()),
                components::get_component_type<channel>(),
                const_cast<channel*>(this));
        }

        // standard LCO action implementations

        // Push a value to the channel.
        void set_value(RemoteType&& result)
        {
            channel_.set(HPX_MOVE(result));
        }

        // Close the channel
        void set_exception(std::exception_ptr const& /*e*/)
        {
            channel_.close();
        }

        // Retrieve the next value from the channel
        result_type get_value()
        {
            return channel_.get(launch::sync);
        }
        result_type get_value(error_code& ec)
        {
            return channel_.get(launch::sync, ec);
        }

        // Additional functionality exposed by the channel component
        hpx::future<T> get_generation(std::size_t generation)
        {
            return channel_.get(generation);
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(channel, get_generation)

        void set_generation(RemoteType&& value, std::size_t generation)
        {
            channel_.set(HPX_MOVE(value), generation);
        }
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(channel, set_generation)

        std::size_t close(bool force_delete_entries)
        {
            return channel_.close(force_delete_entries);
        }
        HPX_DEFINE_COMPONENT_ACTION(channel, close)

    private:
        lcos::local::channel<result_type> channel_;
    };
}    // namespace hpx::lcos::server
