//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/async_base/launch_policy.hpp
/// \page hpx::launch
/// \headerfile hpx/future.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_base/detail/policy_holder_query.hpp>
#include <hpx/async_base/scheduling_properties.hpp>
#include <hpx/async_base/traits/is_launch_policy.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstdint>
#include <type_traits>
#include <utility>

namespace hpx {

    HPX_CXX_CORE_EXPORT enum class launch_policy : std::int8_t {
        async = 0x01,
        deferred = 0x02,
        task = 0x04,    // see N3632
        sync = 0x08,
        fork = 0x10,    // same as async, but forces continuation stealing
        apply = 0x20,

        sync_policies = 0x0a,     // sync | deferred
        async_policies = 0x15,    // async | task | fork
        all = 0x3f                // async | deferred | task | sync |
                                  // fork | apply
    };

    /// \cond NOINTERNAL
    namespace detail {

        struct policy_holder_base
        {
            constexpr explicit policy_holder_base(launch_policy const p,
                threads::thread_priority const priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) noexcept
              : policy_(p)
              , priority_(priority)
              , stacksize_(stacksize)
              , hint_(hint)
            {
            }

            constexpr explicit operator bool() const noexcept
            {
                return is_valid();
            }

            [[nodiscard]] constexpr launch_policy get_policy() const noexcept
            {
                return policy_;
            }

            [[nodiscard]] constexpr bool is_valid() const noexcept
            {
                return static_cast<int>(policy_) != 0;
            }

            [[nodiscard]] constexpr threads::thread_priority get_priority()
                const noexcept
            {
                return priority_;
            }

            [[nodiscard]] constexpr threads::thread_stacksize get_stacksize()
                const noexcept
            {
                return stacksize_;
            }

            [[nodiscard]] constexpr threads::thread_schedule_hint get_hint()
                const noexcept
            {
                return hint_;
            }

            void set_priority(threads::thread_priority const priority) noexcept
            {
                priority_ = priority;
            }

            void set_stacksize(
                threads::thread_stacksize const stacksize) noexcept
            {
                stacksize_ = stacksize;
            }

            void set_hint(threads::thread_schedule_hint const hint) noexcept
            {
                hint_ = hint;
            }

        protected:
            launch_policy policy_;
            threads::thread_priority priority_;
            threads::thread_stacksize stacksize_;
            threads::thread_schedule_hint hint_;

        private:
            friend class serialization::access;

            HPX_CORE_EXPORT void load(
                serialization::input_archive& ar, unsigned);
            HPX_CORE_EXPORT void save(
                serialization::output_archive& ar, unsigned) const;

            HPX_SERIALIZATION_SPLIT_MEMBER()
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Derived = void>
        struct policy_holder
          : policy_holder_base
          , hpx::detail::policy_holder_query<Derived>
        {
        private:
            constexpr explicit policy_holder(launch_policy const p,
                threads::thread_priority const priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) noexcept
              : policy_holder_base(p, priority, stacksize, hint)
            {
            }

            constexpr explicit policy_holder(
                policy_holder_base const p) noexcept
              : policy_holder_base(p)
            {
            }

            friend Derived;

            template <typename D>
            friend constexpr policy_holder<D> operator~(
                policy_holder<D> const& p) noexcept;

            template <typename Left, typename Right>
            friend policy_holder<Left> operator&=(policy_holder<Left>& lhs,
                policy_holder<Right> const& rhs) noexcept;

            template <typename Left, typename Right>
            friend policy_holder<Left> operator|=(policy_holder<Left>& lhs,
                policy_holder<Right> const& rhs) noexcept;

            template <typename Left, typename Right>
            friend policy_holder<Left> operator^=(policy_holder<Left>& lhs,
                policy_holder<Right> const& rhs) noexcept;

        public:
            constexpr operator launch_policy() const noexcept
            {
                return static_cast<Derived const*>(this)->get_policy();
            }

            constexpr explicit operator bool() const noexcept
            {
                return static_cast<Derived const*>(this)->is_valid();
            }

            [[nodiscard]] constexpr launch_policy policy() const noexcept
            {
                return static_cast<Derived const*>(this)->get_policy();
            }
            [[nodiscard]] constexpr threads::thread_priority priority()
                const noexcept
            {
                return static_cast<Derived const*>(this)->get_priority();
            }
            [[nodiscard]] constexpr threads::thread_stacksize stacksize()
                const noexcept
            {
                return static_cast<Derived const*>(this)->get_stacksize();
            }
            [[nodiscard]] constexpr threads::thread_schedule_hint hint()
                const noexcept
            {
                return static_cast<Derived const*>(this)->get_hint();
            }
        };

        template <>
        struct policy_holder<void>
          : policy_holder_base
          , hpx::detail::policy_holder_query<policy_holder<void>>
        {
        public:
            constexpr explicit policy_holder(launch_policy const p,
                threads::thread_priority const priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) noexcept
              : policy_holder_base(p, priority, stacksize, hint)
            {
            }

            constexpr explicit policy_holder(
                policy_holder_base const p) noexcept
              : policy_holder_base(p)
            {
            }

            constexpr operator launch_policy() const noexcept
            {
                return this->policy_holder_base::get_policy();
            }

            constexpr explicit operator bool() const noexcept
            {
                return this->policy_holder_base::is_valid();
            }

            [[nodiscard]] constexpr launch_policy policy() const noexcept
            {
                return this->policy_holder_base::get_policy();
            }
            [[nodiscard]] constexpr threads::thread_priority priority()
                const noexcept
            {
                return this->policy_holder_base::get_priority();
            }
            [[nodiscard]] constexpr threads::thread_stacksize stacksize()
                const noexcept
            {
                return this->policy_holder_base::get_stacksize();
            }
            [[nodiscard]] constexpr threads::thread_schedule_hint hint()
                const noexcept
            {
                return this->policy_holder_base::get_hint();
            }
        };

        ///////////////////////////////////////////////////////////////////////
        struct async_policy : policy_holder<async_policy>
        {
            constexpr explicit async_policy(
                threads::thread_priority const priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) noexcept
              : policy_holder<async_policy>(
                    launch_policy::async, priority, stacksize, hint)
            {
            }
        };

        struct fork_policy : policy_holder<fork_policy>
        {
            constexpr explicit fork_policy(
                threads::thread_priority const priority =
                    threads::thread_priority::boost,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) noexcept
              : policy_holder<fork_policy>(
                    launch_policy::fork, priority, stacksize, hint)
            {
            }
        };

        struct sync_policy : policy_holder<sync_policy>
        {
            constexpr explicit sync_policy(
                threads::thread_priority const priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) noexcept
              : policy_holder<sync_policy>(
                    launch_policy::sync, priority, stacksize, hint)
            {
            }
        };

        struct deferred_policy : policy_holder<deferred_policy>
        {
            constexpr explicit deferred_policy(
                threads::thread_priority const priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) noexcept
              : policy_holder<deferred_policy>(
                    launch_policy::deferred, priority, stacksize, hint)
            {
            }
        };

        struct apply_policy : policy_holder<apply_policy>
        {
            constexpr explicit apply_policy(
                threads::thread_priority const priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) noexcept
              : policy_holder<apply_policy>(
                    launch_policy::apply, priority, stacksize, hint)
            {
            }
        };

        template <typename Pred>
        struct select_policy : policy_holder<select_policy<Pred>>
        {
            template <typename F,
                typename U = std::enable_if_t<
                    !std::is_same_v<select_policy<Pred>, std::decay_t<F>>>>
            explicit select_policy(F&& f,
                threads::thread_priority priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint hint = {})    // NOLINT
              : policy_holder<select_policy<Pred>>(
                    launch_policy::async, priority, stacksize, hint)
              , pred_(HPX_FORWARD(F, f))
            {
            }

            constexpr launch_policy get_policy() const
                noexcept(noexcept(std::declval<Pred>()()))
            {
                return pred_();
            }

            static constexpr bool is_valid() noexcept
            {
                return true;
            }

        private:
            Pred pred_;
        };

        struct select_policy_generator
        {
            constexpr async_policy operator()(
                threads::thread_priority const priority,
                threads::thread_stacksize const stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint const hint = {}) const noexcept
            {
                return async_policy(priority, stacksize, hint);
            }

            template <typename F>
            select_policy<std::decay_t<F>> operator()(F&& f,
                threads::thread_priority priority =
                    threads::thread_priority::default_,
                threads::thread_stacksize stacksize =
                    threads::thread_stacksize::default_,
                threads::thread_schedule_hint hint = {}) const
            {
                return select_policy<std::decay_t<F>>(
                    HPX_FORWARD(F, f), priority, stacksize, hint);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Left, typename Right>
        constexpr inline policy_holder_base operator&(
            policy_holder<Left> const& lhs,
            policy_holder<Right> const& rhs) noexcept
        {
            return policy_holder_base(
                static_cast<launch_policy>(static_cast<int>(lhs.policy()) &
                    static_cast<int>(rhs.policy())),
                lhs.get_priority(), lhs.get_stacksize(), lhs.get_hint());
        }

        template <typename Left, typename Right>
        constexpr inline policy_holder_base operator|(
            policy_holder<Left> const& lhs,
            policy_holder<Right> const& rhs) noexcept
        {
            return policy_holder_base(
                static_cast<launch_policy>(static_cast<int>(lhs.policy()) |
                    static_cast<int>(rhs.policy())),
                lhs.get_priority(), lhs.get_stacksize(), lhs.get_hint());
        }

        template <typename Left, typename Right>
        constexpr inline policy_holder_base operator^(
            policy_holder<Left> const& lhs,
            policy_holder<Right> const& rhs) noexcept
        {
            return policy_holder_base(
                static_cast<launch_policy>(static_cast<int>(lhs.policy()) ^
                    static_cast<int>(rhs.policy())),
                lhs.get_priority(), lhs.get_stacksize(), lhs.get_hint());
        }

        template <typename Derived>
        constexpr inline policy_holder<Derived> operator~(
            policy_holder<Derived> const& p) noexcept
        {
            return policy_holder<Derived>(
                static_cast<launch_policy>(~static_cast<int>(p.policy())),
                p.get_priority(), p.get_stacksize(), p.get_hint());
        }

        template <typename Left, typename Right>
        inline policy_holder<Left> operator&=(
            policy_holder<Left>& lhs, policy_holder<Right> const& rhs) noexcept
        {
            lhs = policy_holder<Left>(lhs & rhs);
            return lhs;
        }

        template <typename Left, typename Right>
        inline policy_holder<Left> operator|=(
            policy_holder<Left>& lhs, policy_holder<Right> const& rhs) noexcept
        {
            lhs = policy_holder<Left>(lhs | rhs);
            return lhs;
        }

        template <typename Left, typename Right>
        inline policy_holder<Left> operator^=(
            policy_holder<Left>& lhs, policy_holder<Right> const& rhs) noexcept
        {
            lhs = policy_holder<Left>(lhs ^ rhs);
            return lhs;
        }

        template <typename Left, typename Right>
        constexpr inline bool operator==(policy_holder<Left> const& lhs,
            policy_holder<Right> const& rhs) noexcept
        {
            return static_cast<int>(lhs.policy()) ==
                static_cast<int>(rhs.policy());
        }

        template <typename Left, typename Right>
        constexpr inline bool operator!=(policy_holder<Left> const& lhs,
            policy_holder<Right> const& rhs) noexcept
        {
            return !(lhs == rhs);
        }
    }    // namespace detail
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// Launch policies for \a hpx::async etc.
    HPX_CXX_CORE_EXPORT struct launch : detail::policy_holder<>
    {
        ///////////////////////////////////////////////////////////////////////
        /// Default constructor. This creates a launch policy representing all
        /// possible launch modes
        constexpr launch() noexcept
          : detail::policy_holder<>{launch_policy::all}
        {
        }

        /// \cond NOINTERNAL
        template <typename Derived>
        constexpr launch(detail::policy_holder<Derived> const& ph) noexcept
          : detail::policy_holder<>{ph}
        {
        }

        constexpr launch(detail::policy_holder_base const& ph) noexcept
          : detail::policy_holder<>{ph}
        {
        }
        /// \endcond

        /// Create a launch policy representing asynchronous execution
        constexpr launch(detail::async_policy const p) noexcept
          : detail::policy_holder<>{
                launch_policy::async, p.priority(), p.stacksize(), p.hint()}
        {
        }

        /// Create a launch policy representing asynchronous execution. The
        /// new thread is executed in a preferred way
        constexpr launch(detail::fork_policy const p) noexcept
          : detail::policy_holder<>{
                launch_policy::fork, p.priority(), p.stacksize(), p.hint()}
        {
        }

        /// Create a launch policy representing synchronous execution
        constexpr launch(detail::sync_policy const p) noexcept
          : detail::policy_holder<>{
                launch_policy::sync, p.priority(), p.stacksize(), p.hint()}
        {
        }

        /// Create a launch policy representing deferred execution
        constexpr launch(detail::deferred_policy const p) noexcept
          : detail::policy_holder<>{
                launch_policy::deferred, p.priority(), p.stacksize(), p.hint()}
        {
        }

        /// Create a launch policy representing fire and forget execution
        constexpr launch(detail::apply_policy const p) noexcept
          : detail::policy_holder<>{
                launch_policy::apply, p.priority(), p.stacksize(), p.hint()}
        {
        }

        /// Create a launch policy representing fire and forget execution
        template <typename F>
        constexpr launch(detail::select_policy<F> const& p) noexcept
          : detail::policy_holder<>{
                p.policy(), p.priority(), p.stacksize(), p.hint()}
        {
        }

        template <typename Launch,
            typename Enable =
                std::enable_if_t<hpx::traits::is_launch_policy_v<Launch>>>
        constexpr launch(Launch l, threads::thread_priority const priority,
            threads::thread_stacksize const stacksize,
            threads::thread_schedule_hint const hint) noexcept
          : detail::policy_holder<>(l.policy(), priority, stacksize, hint)
        {
        }

        ///////////////////////////////////////////////////////////////////////
        /// \cond NOINTERNAL
        using async_policy = detail::async_policy;
        using fork_policy = detail::fork_policy;
        using sync_policy = detail::sync_policy;
        using deferred_policy = detail::deferred_policy;
        using apply_policy = detail::apply_policy;
        template <typename F>
        using select_policy = detail::select_policy<F>;
        /// \endcond

        ///////////////////////////////////////////////////////////////////////
        /// Predefined launch policy representing asynchronous execution
        HPX_CORE_EXPORT static detail::async_policy const async;

        /// Predefined launch policy representing asynchronous execution.The
        /// new thread is executed in a preferred way
        HPX_CORE_EXPORT static detail::fork_policy const fork;

        /// Predefined launch policy representing synchronous execution
        HPX_CORE_EXPORT static detail::sync_policy const sync;

        /// Predefined launch policy representing deferred execution
        HPX_CORE_EXPORT static detail::deferred_policy const deferred;

        /// Predefined launch policy representing fire and forget execution
        HPX_CORE_EXPORT static detail::apply_policy const apply;

        /// Predefined launch policy representing delayed policy selection
        HPX_CORE_EXPORT static detail::select_policy_generator const select;

        /// \cond NOINTERNAL
        HPX_CORE_EXPORT static detail::policy_holder<> const all;
        HPX_CORE_EXPORT static detail::policy_holder<> const sync_policies;
        HPX_CORE_EXPORT static detail::policy_holder<> const async_policies;
        /// \endcond
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT HPX_FORCEINLINE constexpr bool has_async_policy(
        launch const p) noexcept
    {
        return static_cast<bool>(static_cast<int>(p.get_policy()) &
            static_cast<int>(launch_policy::async_policies));
    }

    HPX_CXX_CORE_EXPORT template <typename F>
    HPX_FORCEINLINE constexpr bool has_async_policy(
        detail::policy_holder<F> const& p) noexcept
    {
        return static_cast<bool>(static_cast<int>(p.policy()) &
            static_cast<int>(launch_policy::async_policies));
    }
    /// \endcond
}    // namespace hpx
