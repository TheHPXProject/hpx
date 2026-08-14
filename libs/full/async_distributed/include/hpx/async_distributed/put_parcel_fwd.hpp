//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>

#include <memory>
#include <type_traits>

namespace hpx::parcelset {

    namespace detail {

        HPX_CXX_EXPORT struct create_parcel
        {
            template <typename... Args>
            static parcelset::parcel call(naming::gid_type&& dest,
                naming::address&& addr, Args&&... args);

            static inline parcelset::parcel call_with_action(
                naming::gid_type&& dest, naming::address&& addr,
                std::unique_ptr<actions::base_action>&& action);
        };

        HPX_CXX_EXPORT template <typename PutParcel>
        struct put_parcel_cont
        {
            std::decay_t<PutParcel> pp;
            hpx::id_type dest;
            naming::address addr;
            std::unique_ptr<actions::base_action> action;

            void operator()(hpx::future<naming::gid_type> f);
        };

        HPX_CXX_EXPORT template <typename PutParcel>
        void put_parcel_impl(PutParcel&& pp, hpx::id_type const& dest,
            naming::address&& addr,
            std::unique_ptr<actions::base_action>&& action);
    }    // namespace detail

    HPX_CXX_EXPORT template <typename... Args>
    void put_parcel(
        hpx::id_type const& dest, naming::address&& addr, Args&&... args);

    HPX_CXX_EXPORT template <typename Callback, typename... Args>
    void put_parcel_cb(Callback&& cb, hpx::id_type const& dest,
        naming::address&& addr, Args&&... args);
}    // namespace hpx::parcelset

#endif
