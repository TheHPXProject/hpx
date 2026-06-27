//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file component_commandline.hpp
/// \page hpx::components::client
/// \headerfile hpx/components.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/components/client_base.hpp>
#include <hpx/modules/futures.hpp>

#include <utility>

namespace hpx::components {

    /// @brief The client class is a wrapper that manages a distributed component.
    /// It extends \c client_base with specific \c Component and \c Data types.
    ///
    /// @tparam Component The type of the component.
    /// @tparam Data The type of the data associated with the client (default is void).
    HPX_CXX_EXPORT template <typename Component, typename Data = void>
    class client : public client_base<client<Component, Data>, Component, Data>
    {
        using base_type = client_base<client, Component, Data>;
        using future_type = typename base_type::future_type;

    public:
        // construction
        client() = default;

        explicit client(hpx::id_type const& id)
          : base_type(id)
        {
        }
        explicit client(hpx::id_type&& id)
          : base_type(HPX_MOVE(id))
        {
        }

        explicit client(future_type const& f) noexcept
          : base_type(f)
        {
        }
        explicit client(future_type&& f) noexcept
          : base_type(HPX_MOVE(f))
        {
        }
        client(future<hpx::id_type>&& f) noexcept
          : base_type(HPX_MOVE(f))
        {
        }
        client(future<client>&& c)
          : base_type(HPX_MOVE(c))
        {
        }

        client(client const& rhs) noexcept = default;
        client(client&& rhs) noexcept = default;

        ~client() = default;

        // copy assignment and move assignment
        client& operator=(hpx::id_type const& id)
        {
            base_type::operator=(id);
            return *this;
        }
        client& operator=(hpx::id_type&& id)
        {
            base_type::operator=(HPX_MOVE(id));
            return *this;
        }

        client& operator=(future_type const& f) noexcept
        {
            base_type::operator=(f);
            return *this;
        }
        client& operator=(future_type&& f) noexcept
        {
            base_type::operator=(HPX_MOVE(f));
            return *this;
        }
        client& operator=(future<hpx::id_type>&& f) noexcept
        {
            base_type::operator=(HPX_MOVE(f));
            return *this;
        }

        client& operator=(client const& rhs) noexcept = default;
        client& operator=(client&& rhs) noexcept = default;
    };
}    // namespace hpx::components
