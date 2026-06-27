//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/futures/future_fwd.hpp>
#include <hpx/futures/traits/future_traits.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/type_support.hpp>

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT struct future_data_void
        {
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Result>
        struct shared_state_ptr_result
        {
            using type = Result;
        };

        template <typename Result>
        struct shared_state_ptr_result<Result&>
        {
            using type = Result&;
        };

        template <>
        struct shared_state_ptr_result<void>
        {
            using type = future_data_void;
        };

        HPX_CXX_CORE_EXPORT template <typename Future>
        using shared_state_ptr_result_t =
            typename shared_state_ptr_result<Future>::type;

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename R, typename Enable = void>
        struct shared_state_ptr
        {
            using result_type = shared_state_ptr_result_t<R>;
            using type =
                hpx::intrusive_ptr<lcos::detail::future_data_base<result_type>>;
        };

        HPX_CXX_CORE_EXPORT template <typename Future>
        using shared_state_ptr_t = typename shared_state_ptr<Future>::type;

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Future, typename Enable = void>
        struct shared_state_ptr_for
          : shared_state_ptr<
                traits::future_traits_t<std::remove_const_t<Future>>>
        {
        };

        template <typename Future>
        struct shared_state_ptr_for<Future&> : shared_state_ptr_for<Future>
        {
        };

        template <typename Future>
        struct shared_state_ptr_for<Future&&> : shared_state_ptr_for<Future>
        {
        };

        template <typename Future>
        struct shared_state_ptr_for<std::vector<Future>>
        {
            using type =
                std::vector<typename shared_state_ptr_for<Future>::type>;
        };

        template <typename Future, std::size_t N>
        struct shared_state_ptr_for<std::array<Future, N>>
        {
            using type =
                std::array<typename shared_state_ptr_for<Future>::type, N>;
        };

        HPX_CXX_CORE_EXPORT template <typename Future>
        using shared_state_ptr_for_t =
            typename shared_state_ptr_for<Future>::type;

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename SharedState, typename Allocator>
        struct shared_state_allocator;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_shared_state : std::false_type
    {
    };

    template <typename R>
    struct is_shared_state<
        hpx::intrusive_ptr<lcos::detail::future_data_base<R>>> : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename R>
    inline constexpr bool is_shared_state_v = is_shared_state<R>::value;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename T, typename Enable = void>
        struct future_access_customization_point;
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    struct future_access : detail::future_access_customization_point<T>
    {
    };

    template <typename R>
    struct future_access<hpx::future<R>>
    {
        template <typename SharedState>
        static hpx::future<R> create(
            hpx::intrusive_ptr<SharedState> const& shared_state)
        {
            return hpx::future<R>(shared_state);
        }

        template <typename T = void>
        static hpx::future<R> create(
            detail::shared_state_ptr_for_t<hpx::future<hpx::future<R>>> const&
                shared_state)
        {
            return hpx::future<hpx::future<R>>(shared_state);
        }

        template <typename SharedState>
        static hpx::future<R> create(
            hpx::intrusive_ptr<SharedState>&& shared_state)
        {
            return hpx::future<R>(HPX_MOVE(shared_state));
        }

        template <typename T = void>
        static hpx::future<R> create(
            detail::shared_state_ptr_for_t<hpx::future<hpx::future<R>>>&&
                shared_state)
        {
            return hpx::future<hpx::future<R>>(HPX_MOVE(shared_state));
        }

        template <typename SharedState>
        static hpx::future<R> create(
            SharedState* shared_state, bool addref = true)
        {
            return hpx::future<R>(
                hpx::intrusive_ptr<SharedState>(shared_state, addref));
        }

        HPX_FORCEINLINE static traits::detail::shared_state_ptr_t<R> const&
        get_shared_state(hpx::future<R> const& f)
        {
            return f.shared_state_;
        }

        HPX_FORCEINLINE static
            typename traits::detail::shared_state_ptr_t<R>::element_type*
            detach_shared_state(hpx::future<R>&& f)
        {
            return f.shared_state_.detach();
        }

        template <typename Destination>
        HPX_FORCEINLINE static void transfer_result(
            hpx::future<R>&& src, Destination& dest)
        {
            if constexpr (std::is_void_v<R>)
            {
                src.get();
                dest.set_value(util::unused);
            }
            else
            {
                dest.set_value(src.get());
            }
        }
    };

    template <typename R>
    struct future_access<hpx::shared_future<R>>
    {
        template <typename SharedState>
        static hpx::shared_future<R> create(
            hpx::intrusive_ptr<SharedState> const& shared_state)
        {
            return hpx::shared_future<R>(shared_state);
        }

        template <typename T = void>
        static hpx::shared_future<R> create(detail::shared_state_ptr_for_t<
            hpx::shared_future<hpx::future<R>>> const& shared_state)
        {
            return hpx::shared_future<hpx::future<R>>(shared_state);
        }

        template <typename SharedState>
        static hpx::shared_future<R> create(
            hpx::intrusive_ptr<SharedState>&& shared_state)
        {
            return hpx::shared_future<R>(HPX_MOVE(shared_state));
        }

        template <typename T = void>
        static hpx::shared_future<R> create(
            detail::shared_state_ptr_for_t<hpx::shared_future<hpx::future<R>>>&&
                shared_state)
        {
            return hpx::shared_future<hpx::future<R>>(HPX_MOVE(shared_state));
        }

        template <typename SharedState>
        static hpx::shared_future<R> create(
            SharedState* shared_state, bool addref = true)
        {
            return hpx::shared_future<R>(
                hpx::intrusive_ptr<SharedState>(shared_state, addref));
        }

        HPX_FORCEINLINE static traits::detail::shared_state_ptr_t<R> const&
        get_shared_state(hpx::shared_future<R> const& f)
        {
            return f.shared_state_;
        }

        HPX_FORCEINLINE static
            typename traits::detail::shared_state_ptr_t<R>::element_type*
            detach_shared_state(hpx::shared_future<R> const& f)
        {
            return f.shared_state_.get();
        }

        template <typename Destination>
        HPX_FORCEINLINE static void transfer_result(
            hpx::shared_future<R>&& src, Destination& dest)
        {
            if constexpr (std::is_void_v<R>)
            {
                src.get();
                dest.set_value(util::unused);
            }
            else
            {
                dest.set_value(src.get());
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename SharedState, typename Allocator>
    struct shared_state_allocator
      : detail::shared_state_allocator<SharedState, Allocator>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename SharedState, typename Allocator>
    using shared_state_allocator_t =
        typename shared_state_allocator<SharedState, Allocator>::type;
}    // namespace hpx::traits
