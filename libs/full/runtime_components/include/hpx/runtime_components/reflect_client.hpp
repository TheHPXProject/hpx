//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_HAVE_CXX26_REFLECTION)

#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/runtime_components/new.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <meta>
#include <utility>
#include <vector>

namespace hpx::components {

    namespace detail {

        /// Returns true for user-defined public non-static member functions.
        consteval bool is_client_member_fn(std::meta::info m) noexcept
        {
            return std::meta::is_public(m) && std::meta::is_function(m) &&
                !std::meta::is_constructor(m) && !std::meta::is_destructor(m) &&
                !std::meta::is_static_member(m) &&
                !std::meta::is_special_member_function(m) &&
                !std::meta::is_conversion_function(m);
        }

        consteval std::size_t count_client_member_fns(
            std::meta::info cls) noexcept
        {
            std::size_t n = 0;
            for (auto m : std::meta::members_of(
                     cls, std::meta::access_context::unchecked()))
                if (is_client_member_fn(m))
                    ++n;
            return n;
        }

        template <std::meta::info Cls>
        consteval auto get_client_member_fns() noexcept
        {
            constexpr std::size_t N = count_client_member_fns(Cls);
            std::array<std::meta::info, N> result{};
            std::size_t i = 0;
            for (auto m : std::meta::members_of(
                     Cls, std::meta::access_context::unchecked()))
                if (is_client_member_fn(m))
                    result[i++] = m;
            return result;
        }

        /// Helper alias to build std::function<future<R>(Ps...)>
        template <typename Ret, typename... Ps>
        using make_async_fn = std::function<Ret(Ps...)>;

        /// Returns the std::function type for async dispatch of a server fn.
        consteval std::meta::info build_async_fn_type(std::meta::info fn)
        {
            auto fut_ret = std::meta::substitute(^^hpx::future,
                {
                    std::meta::return_type_of(fn)});
            std::vector<std::meta::info> args = {fut_ret};
            for (auto p : std::meta::parameters_of(fn))
                args.push_back(std::meta::type_of(p));
            return std::meta::substitute(^^detail::make_async_fn, args);
        }

        /// Build the data member spec list for define_aggregate.
        /// Takes the server function array and returns a vector of
        /// data_member_spec entries (one per server fn + one for id_).
        template <std::meta::info Server>
        consteval ::std::vector<::std::meta::info> create_client_data_members()
        {
            ::std::vector<::std::meta::info> mems;
            for (auto m : get_client_member_fns<Server>())
                mems.push_back(
                    ::std::meta::data_member_spec(build_async_fn_type(m),
                        {.name = ::std::meta::identifier_of(m)}));
            mems.push_back(::std::meta::data_member_spec(^^::hpx::id_type,
                {
                    .name = "id_"}));
            return mems;
        }

        struct fn_pair
        {
            std::meta::info server_fn;
            std::meta::info client_dm;
        };

        /// Count data members of Client (excluding id_).
        template <typename Client>
        consteval std::size_t reflect_npairs() noexcept
        {
            std::size_t n = 0;
            for (auto m : std::meta::members_of(
                     ^^Client, std::meta::access_context::unchecked()))
                if (std::meta::has_identifier(m) &&
                    std::meta::identifier_of(m) != "id_" &&
                    !std::meta::is_function(m))
                    ++n;
            return n;
        }

        /// Build fn_pair array pairing server fns with client data members.
        template <typename Client, std::size_t N, std::size_t M>
        consteval auto reflect_get_pairs(
            std::array<std::meta::info, M> const& server_fns) noexcept
        {
            std::array<fn_pair, N> r{};
            std::size_t i = 0;
            for (auto cm : std::meta::members_of(
                     ^^Client, std::meta::access_context::unchecked()))
            {
                if (!std::meta::has_identifier(cm))
                    continue;
                if (std::meta::identifier_of(cm) == "id_")
                    continue;
                if (std::meta::is_function(cm))
                    continue;
                for (auto sm : server_fns)
                    if (std::meta::identifier_of(sm) ==
                        std::meta::identifier_of(cm))
                    {
                        r[i++] = {sm, cm};
                        break;
                    }
            }
            return r;
        }

        /// Async dispatcher for a single server member function.
        template <std::meta::info Sfn, std::meta::info Cdm>
        auto reflect_dispatch(hpx::id_type id)
        {
            return [id](auto&&... args) {
                return hpx::async<hpx::actions::reflect_component_action<Sfn>>(
                    id, std::forward<decltype(args)>(args)...);
            };
        }

        /// Initialize all dispatchers in a generated client.
        template <typename Client, std::size_t N>
        Client reflect_make_client(
            hpx::id_type id, std::array<fn_pair, N> const& pairs)
        {
            Client c;
            c.id_ = id;
            template for (constexpr auto p : pairs)
                c.[:p.client_dm:] = reflect_dispatch<p.server_fn, p.client_dm>(
                                      id);
            return c;
        }

    }    // namespace detail

    /// \brief Reflection-generated client type for a component server.
    ///
    /// Forward-declared here; the actual definition is injected by
    /// HPX_CLIENT(Server, reflect_client<Server>) using consteval{} +
    /// define_aggregate at namespace scope.
    template <typename Server>
    struct reflect_client;

    /// \brief Instantiate a Server component and return a reflect_client.
    ///
    /// Requires HPX_CLIENT(Server) at namespace scope first.
    /// Constructor arguments are forwarded to hpx::new_<Server>.
    ///
    /// \code
    ///   HPX_CLIENT(compute_server)
    ///   auto c = hpx::components::make_client<compute_server>(locality);
    ///   auto f = c.add(42);   // async, returns future<int>
    /// \endcode
    template <typename Server, typename... CtorArgs>
    reflect_client<Server> make_client(
        hpx::id_type locality, CtorArgs&&... ctor_args)
    {
        constexpr auto sfns = detail::get_client_member_fns<^^Server>();
        constexpr auto pairs = detail::reflect_get_pairs<reflect_client<Server>,
            detail::reflect_npairs<reflect_client<Server>>(), sfns.size()>(
            sfns);
        hpx::id_type id =
            hpx::new_<Server>(locality, std::forward<CtorArgs>(ctor_args)...)
                .get();
        return detail::reflect_make_client<reflect_client<Server>>(id, pairs);
    }

}    // namespace hpx::components

#endif    // !HPX_COMPUTE_DEVICE_CODE && HPX_HAVE_CXX26_REFLECTION
