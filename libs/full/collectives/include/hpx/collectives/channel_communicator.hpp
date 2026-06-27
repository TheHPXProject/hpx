//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file create_communicator.hpp

#pragma once

#include <hpx/config.hpp>

#if defined(DOXYGEN)

/// Top level HPX namespace
namespace hpx { namespace collectives {
    // clang-format off

    /// A handle identifying the communication channel to use for get/set
    /// operations
    class channel_communicator{};

    /// Create a new communicator object usable with peer-to-peer
    /// channel-based operations
    ///
    /// This functions creates a new communicator object that can be called in
    /// order to pre-allocate a communicator object usable with multiple
    /// invocations of channel-based peer-to-peer operations.
    ///
    /// \param basename     The base name identifying the collective operation
    /// \param num_sites    The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \returns    This function returns a future to a new communicator object
    ///             usable with the collective operation.
    ///
    /// \note       The caller must ensure that the communicator object
    ///             returned by this function is kept alive (i.e., does not go
    ///             out of scope) until all participating sites have
    ///             successfully connected. Destroying the communicator before
    ///             all sites have reached the creation point may cause the
    ///             remaining sites to hang indefinitely. Consider using a
    ///             barrier or similar synchronization mechanism after creating
    ///             the communicator to ensure all sites are connected before
    ///             allowing it to be destroyed.
    ///
    hpx::future<channel_communicator> create_channel_communicator(
        char const* basename,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg());

    /// Create a new communicator object usable with peer-to-peer
    /// channel-based operations
    ///
    /// This functions creates a new communicator object that can be called in
    /// order to pre-allocate a communicator object usable with multiple
    /// invocations of channel-based peer-to-peer operations.
    ///
    /// \param basename     The base name identifying the collective operation
    /// \param num_sites    The number of participating sites (default: all
    ///                     localities).
    /// \param this_site    The sequence number of this invocation (usually
    ///                     the locality id). This value is optional and
    ///                     defaults to whatever hpx::get_locality_id() returns.
    ///
    /// \returns    This function returns a new communicator object usable
    ///             with the collective operation.
    ///
    /// \note       The caller must ensure that the communicator object
    ///             returned by this function is kept alive (i.e., does not go
    ///             out of scope) until all participating sites have
    ///             successfully connected. Destroying the communicator before
    ///             all sites have reached the creation point may cause the
    ///             remaining sites to hang indefinitely. Consider using a
    ///             barrier or similar synchronization mechanism after creating
    ///             the communicator to ensure all sites are connected before
    ///             allowing it to be destroyed.
    ///
    channel_communicator create_channel_communicator(
        hpx::launch::sync_policy, char const* basename,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg());

    /// Send a value to the given site
    ///
    /// This function sends a value to the given site based on the given
    /// communicator.
    ///
    /// \param comm     The channel communicator object to use for the data
    ///                 transfer
    /// \param site     The destination site
    /// \param value    The value to send
    /// \param tag      The (optional) tag identifying the concrete operation
    ///
    /// \returns    This function returns a future<void> that becomes ready
    ///             once the data transfer operation has finished.
    ///
    template <typename T>
    hpx::future<void> set(channel_communicator comm,
        that_site_arg site, T&& value, tag_arg tag = tag_arg());

    /// Send a value to the given site
    ///
    /// This function receives a value from the given site based on the given
    /// communicator.
    ///
    /// \param comm     The channel communicator object to use for the data transfer
    /// \param site     The source site
    ///
    /// \returns    This function returns a future<T> that becomes ready once the
    ///             data transfer operation has finished. The future will hold the
    ///             received value.
    ///
    template <typename T>
    hpx::future<T> get(channel_communicator comm, that_site_arg site,
        tag_arg tag = tag_arg());
    // clang-format on
}}    // namespace hpx::collectives

#else

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/futures.hpp>

#include <hpx/collectives/argument_types.hpp>
#include <hpx/collectives/detail/channel_communicator.hpp>

#include <cstddef>
#include <memory>
#include <utility>

namespace hpx::collectives {

    // forward declarations
    HPX_CXX_EXPORT class channel_communicator;

    HPX_CXX_EXPORT template <typename T>
    hpx::future<T> get(
        channel_communicator, that_site_arg, tag_arg = tag_arg());

    HPX_CXX_EXPORT template <typename T>
    T get(hpx::launch::sync_policy, channel_communicator, that_site_arg,
        tag_arg = tag_arg());

    HPX_CXX_EXPORT template <typename T>
    hpx::future<void> set(
        channel_communicator, that_site_arg, T&&, tag_arg = tag_arg());

    HPX_CXX_EXPORT template <typename T>
    void set(hpx::launch::sync_policy, channel_communicator, that_site_arg, T&&,
        tag_arg = tag_arg());

    HPX_CXX_EXPORT class channel_communicator
    {
    private:
        friend HPX_EXPORT hpx::future<channel_communicator>
        create_channel_communicator(char const* basename,
            num_sites_arg num_sites, this_site_arg this_site);

        friend HPX_EXPORT channel_communicator create_channel_communicator(
            hpx::launch::sync_policy, char const* basename,
            num_sites_arg num_sites, this_site_arg this_site);

        template <typename T>
        friend hpx::future<T> get(channel_communicator, that_site_arg, tag_arg);

        template <typename T>
        friend T get(hpx::launch::sync_policy, channel_communicator,
            that_site_arg, tag_arg);

        template <typename T>
        friend hpx::future<void> set(
            channel_communicator, that_site_arg, T&&, tag_arg);

        template <typename T>
        friend void set(hpx::launch::sync_policy, channel_communicator,
            that_site_arg, T&&, tag_arg);

    private:
        HPX_EXPORT channel_communicator(char const* basename,
            num_sites_arg num_sites, this_site_arg this_site,
            components::client<detail::channel_communicator_server>&& here);

        HPX_EXPORT channel_communicator(hpx::launch::sync_policy,
            char const* basename, num_sites_arg num_sites,
            this_site_arg this_site,
            components::client<detail::channel_communicator_server>&& here);

    public:
        HPX_EXPORT channel_communicator();
        HPX_EXPORT ~channel_communicator();

        channel_communicator(channel_communicator const& rhs) = default;
        channel_communicator& operator=(
            channel_communicator const& rhs) = default;

        channel_communicator(channel_communicator&& rhs) noexcept = default;
        channel_communicator& operator=(
            channel_communicator&& rhs) noexcept = default;

        HPX_EXPORT void free();

        explicit operator bool() const noexcept
        {
            return !!comm_;
        }

        HPX_EXPORT std::pair<num_sites_arg, this_site_arg> get_info()
            const noexcept;

    private:
        std::shared_ptr<detail::channel_communicator> comm_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT hpx::future<channel_communicator>
    create_channel_communicator(char const* basename,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg());

    HPX_CXX_EXPORT HPX_EXPORT channel_communicator create_channel_communicator(
        hpx::launch::sync_policy, char const* basename,
        num_sites_arg num_sites = num_sites_arg(),
        this_site_arg this_site = this_site_arg());

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    hpx::future<T> get(
        channel_communicator const comm, that_site_arg site, tag_arg tag)
    {
        return comm.comm_->get<T>(site.argument_, tag.argument_);
    }

    HPX_CXX_EXPORT template <typename T>
    T get(hpx::launch::sync_policy, channel_communicator const comm,
        that_site_arg site, tag_arg tag)
    {
        return comm.comm_->get<T>(site.argument_, tag.argument_).get();
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T>
    hpx::future<void> set(channel_communicator const comm, that_site_arg site,
        T&& value, tag_arg tag)
    {
        return comm.comm_->set(
            site.argument_, HPX_FORWARD(T, value), tag.argument_);
    }

    HPX_CXX_EXPORT template <typename T>
    void set(hpx::launch::sync_policy, channel_communicator const comm,
        that_site_arg site, T&& value, tag_arg tag)
    {
        return comm.comm_
            ->set(site.argument_, HPX_FORWARD(T, value), tag.argument_)
            .get();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Predefined p2p communicator (refers to all localities)
    HPX_CXX_EXPORT HPX_EXPORT channel_communicator
    get_world_channel_communicator();

    namespace detail {

        HPX_EXPORT void create_world_channel_communicator();
        HPX_EXPORT void reset_world_channel_communicator();
    }    // namespace detail
}    // namespace hpx::collectives

#endif    // !HPX_COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
