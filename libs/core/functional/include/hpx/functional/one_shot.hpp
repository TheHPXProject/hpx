//  Copyright (c) 2011-2012 Thomas Heller
//  Copyright (c) 2013-2016 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/functional/traits/get_function_address.hpp>
#include <hpx/functional/traits/get_function_annotation.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename F>
        class one_shot_wrapper    //-V690
        {
        public:
            // default constructor is needed for serialization
            constexpr one_shot_wrapper() noexcept = default;

            template <typename F_,
                typename = std::enable_if_t<std::is_constructible_v<F, F_>>>
            constexpr explicit one_shot_wrapper(F_&& f)
              : _f(HPX_FORWARD(F_, f))
            {
            }

            constexpr one_shot_wrapper(one_shot_wrapper&& other) noexcept
              : _f(HPX_MOVE(other._f))
#if defined(HPX_DEBUG)
              , _called(other._called)
#endif
            {
#if defined(HPX_DEBUG)
                other._called = true;
#endif
            }

            one_shot_wrapper& operator=(one_shot_wrapper&& other) noexcept
            {
                _f = HPX_MOVE(other._f);
#if defined(HPX_DEBUG)
                _called = other._called;
                other._called = true;
#endif
                return *this;
            }

            one_shot_wrapper(one_shot_wrapper const& other) = delete;
            one_shot_wrapper& operator=(one_shot_wrapper const&) = delete;

            ~one_shot_wrapper() = default;

            void check_call() noexcept
            {
#if defined(HPX_DEBUG)
                HPX_ASSERT(!_called);
                _called = true;
#endif
            }

            template <typename... Ts>
            constexpr HPX_HOST_DEVICE util::invoke_result_t<F, Ts...>
            operator()(Ts&&... vs)
            {
                check_call();

                return HPX_INVOKE(HPX_MOVE(_f), HPX_FORWARD(Ts, vs)...);
            }

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const /*version*/)
            {
                // clang-format off
                ar & _f;
                // clang-format on
            }

            [[nodiscard]] constexpr std::size_t get_function_address()
                const noexcept
            {
                return traits::get_function_address<F>::call(_f);
            }

            [[nodiscard]] constexpr char const* get_function_annotation()
                const noexcept
            {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                return traits::get_function_annotation<F>::call(_f);
#else
                return nullptr;
#endif
            }

            [[nodiscard]] hpx::tracing::annotation_handle
            get_function_annotation_tracing() const
            {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                return traits::get_function_annotation_tracing<F>::call(_f);
#else
                static auto sh =
                    hpx::tracing::create_annotation_handle("one_shot_wrapper");
                return sh;
#endif
            }

        public:    // exposition-only
            F _f;
#if defined(HPX_DEBUG)
            bool _called = false;
#endif
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr detail::one_shot_wrapper<std::decay_t<F>> one_shot(F&& f)
    {
        using result_type = detail::one_shot_wrapper<std::decay_t<F>>;
        return result_type(HPX_FORWARD(F, f));
    }
}    // namespace hpx::util

///////////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F>
    struct get_function_address<util::detail::one_shot_wrapper<F>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            util::detail::one_shot_wrapper<F> const& f) noexcept
        {
            return f.get_function_address();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F>
    struct get_function_annotation<util::detail::one_shot_wrapper<F>>
    {
        [[nodiscard]] static constexpr char const* call(
            util::detail::one_shot_wrapper<F> const& f) noexcept
        {
            return f.get_function_annotation();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename F>
    struct get_function_annotation_tracing<util::detail::one_shot_wrapper<F>>
    {
        [[nodiscard]] static hpx::tracing::annotation_handle call(
            util::detail::one_shot_wrapper<F> const& f) noexcept
        {
            return f.get_function_annotation_tracing();
        }
    };
}    // namespace hpx::traits
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::serialization {

    HPX_CXX_CORE_EXPORT template <typename Archive, typename F>
    void serialize(Archive& ar,
        ::hpx::util::detail::one_shot_wrapper<F>& one_shot_wrapper,
        unsigned int const version = 0)
    {
        one_shot_wrapper.serialize(ar, version);
    }
}    // namespace hpx::serialization
