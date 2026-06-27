//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::functional {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT struct extract_locality
    {
        hpx::id_type operator()(
            hpx::id_type const& locality_id, hpx::id_type const& id) const
        {
            if (locality_id == hpx::invalid_id)
            {
                HPX_THROW_EXCEPTION(hpx::error::no_success,
                    "extract_locality::operator()",
                    "could not resolve colocated locality for id({1})", id);
            }
            return locality_id;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT template <typename Bound, typename Continuation>
        struct post_continuation_impl
        {
            using bound_type = std::decay_t<Bound>;
            using continuation_type = std::decay_t<Continuation>;

            post_continuation_impl() = default;
            ~post_continuation_impl() = default;

            template <typename Bound_, typename Continuation_>
            explicit post_continuation_impl(Bound_&& bound, Continuation_&& c)
              : bound_(HPX_FORWARD(Bound_, bound))
              , cont_(HPX_FORWARD(Continuation_, c))
            {
            }

            post_continuation_impl(post_continuation_impl const& o) = default;
            post_continuation_impl& operator=(
                post_continuation_impl const& o) = default;

            post_continuation_impl(post_continuation_impl&& o) = default;
            post_continuation_impl& operator=(
                post_continuation_impl&& o) = default;

            template <typename T>
            util::invoke_result_t<bound_type, hpx::id_type, T> operator()(
                hpx::id_type lco, T&& t)
            {
                using result_type =
                    invoke_result_t<bound_type, hpx::id_type, T>;

                bound_.post_c(HPX_MOVE(cont_), lco, HPX_FORWARD(T, t));
                return result_type();
            }

        private:
            // serialization support
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const)
            {
                // clang-format off
                ar & bound_ & cont_;
                // clang-format on
            }

            bound_type bound_;
            continuation_type cont_;
        };

        template <typename Bound>
        struct post_continuation_impl<Bound, hpx::util::unused_type>
        {
            using bound_type = std::decay_t<Bound>;

            post_continuation_impl() = default;
            ~post_continuation_impl() = default;

            template <typename Bound_,
                typename Enable = std::enable_if_t<!std::is_same_v<
                    std::decay_t<Bound_>, post_continuation_impl>>>
            explicit post_continuation_impl(Bound_&& bound)
              : bound_(HPX_FORWARD(Bound_, bound))
            {
            }

            post_continuation_impl(post_continuation_impl const& o) = default;
            post_continuation_impl& operator=(
                post_continuation_impl const& o) = default;

            post_continuation_impl(post_continuation_impl&& o) = default;
            post_continuation_impl& operator=(
                post_continuation_impl&& o) = default;

            template <typename T>
            typename util::invoke_result<bound_type, hpx::id_type, T>::type
            operator()(hpx::id_type lco, T&& t)
            {
                using result_type = typename util::invoke_result<bound_type,
                    hpx::id_type, T>::type;

                bound_.post(lco, HPX_FORWARD(T, t));
                return result_type();
            }

        private:
            // serialization support
            friend class hpx::serialization::access;

            template <typename Archive>
            HPX_FORCEINLINE void serialize(Archive& ar, unsigned int const)
            {
                // clang-format off
                ar & bound_;
                // clang-format on
            }

            bound_type bound_;
        };
    }    // namespace detail

    template <typename Bound>
    functional::detail::post_continuation_impl<Bound, hpx::util::unused_type>
    post_continuation(Bound&& bound)
    {
        return functional::detail::post_continuation_impl<Bound,
            hpx::util::unused_type>(HPX_FORWARD(Bound, bound));
    }

    template <typename Bound, typename Continuation>
    functional::detail::post_continuation_impl<Bound, Continuation>
    post_continuation(Bound&& bound, Continuation&& c)
    {
        return functional::detail::post_continuation_impl<Bound, Continuation>(
            HPX_FORWARD(Bound, bound), HPX_FORWARD(Continuation, c));
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT template <typename Bound, typename Continuation>
        struct async_continuation_impl
        {
            using bound_type = std::decay_t<Bound>;
            using continuation_type = std::decay_t<Continuation>;

            async_continuation_impl() = default;
            ~async_continuation_impl() = default;

            template <typename Bound_, typename Continuation_>
            explicit async_continuation_impl(Bound_&& bound, Continuation_&& c)
              : bound_(HPX_FORWARD(Bound_, bound))
              , cont_(HPX_FORWARD(Continuation_, c))
            {
            }

            async_continuation_impl(async_continuation_impl const& o) = default;
            async_continuation_impl& operator=(
                async_continuation_impl const& o) = default;

            async_continuation_impl(async_continuation_impl&& o) = default;
            async_continuation_impl& operator=(
                async_continuation_impl&& o) = default;

            template <typename T>
            util::invoke_result_t<bound_type, hpx::id_type, T> operator()(
                hpx::id_type lco, T&& t)
            {
                using result_type =
                    util::invoke_result_t<bound_type, hpx::id_type, T>;

                bound_.post_c(HPX_MOVE(cont_), lco, HPX_FORWARD(T, t));
                return result_type();
            }

        private:
            // serialization support
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const)
            {
                // clang-format off
                ar & bound_ & cont_;
                // clang-format on
            }

            bound_type bound_;
            continuation_type cont_;
        };

        template <typename Bound>
        struct async_continuation_impl<Bound, hpx::util::unused_type>
        {
            using bound_type = std::decay_t<Bound>;

            async_continuation_impl() = default;
            ~async_continuation_impl() = default;

            template <typename Bound_,
                typename Enable = std::enable_if_t<!std::is_same_v<
                    std::decay_t<Bound_>, async_continuation_impl>>>
            explicit async_continuation_impl(Bound_&& bound)
              : bound_(HPX_FORWARD(Bound_, bound))
            {
            }

            async_continuation_impl(async_continuation_impl const& o) = default;
            async_continuation_impl& operator=(
                async_continuation_impl const& o) = default;

            async_continuation_impl(async_continuation_impl&& o) = default;
            async_continuation_impl& operator=(
                async_continuation_impl&& o) = default;

            template <typename T>
            util::invoke_result_t<bound_type, hpx::id_type, T> operator()(
                hpx::id_type lco, T&& t)
            {
                using result_type =
                    util::invoke_result_t<bound_type, hpx::id_type, T>;

                bound_.post_c(lco, lco, HPX_FORWARD(T, t));
                return result_type();
            }

        private:
            // serialization support
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const)
            {
                // clang-format off
                ar & bound_;
                // clang-format on
            }

            bound_type bound_;
        };
    }    // namespace detail

    template <typename Bound>
    functional::detail::async_continuation_impl<Bound, hpx::util::unused_type>
    async_continuation(Bound&& bound)
    {
        return functional::detail::async_continuation_impl<Bound,
            hpx::util::unused_type>(HPX_FORWARD(Bound, bound));
    }

    template <typename Bound, typename Continuation>
    functional::detail::async_continuation_impl<Bound, Continuation>
    async_continuation(Bound&& bound, Continuation&& c)
    {
        return functional::detail::async_continuation_impl<Bound, Continuation>(
            HPX_FORWARD(Bound, bound), HPX_FORWARD(Continuation, c));
    }
}    // namespace hpx::util::functional
