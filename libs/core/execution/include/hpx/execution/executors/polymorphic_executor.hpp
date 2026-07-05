//  Copyright (c) 2017-2026 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/polymorphic_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution/detail/future_exec.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::execution {

    namespace detail {

        struct shape_iter_impl_base
        {
            virtual ~shape_iter_impl_base() = default;
            virtual void copy(shape_iter_impl_base const&) = 0;
            virtual shape_iter_impl_base* clone() const = 0;
            virtual void increment() = 0;
            virtual std::size_t dereference() const = 0;
            virtual bool equal_to(shape_iter_impl_base const&) const = 0;
        };

        template <typename Iterator>
        struct shape_iter_impl : shape_iter_impl_base
        {
            explicit shape_iter_impl(Iterator it)
              : iterator_(it)
            {
            }

            void copy(shape_iter_impl_base const& from) override
            {
                iterator_ = static_cast<shape_iter_impl const&>(from).iterator_;
            }

            shape_iter_impl_base* clone() const override
            {
                return new shape_iter_impl(iterator_);
            }

            void increment() override
            {
                ++iterator_;
            }

            std::size_t dereference() const override
            {
                return *iterator_;
            }

            bool equal_to(shape_iter_impl_base const& other) const override
            {
                return iterator_ ==
                    static_cast<shape_iter_impl const&>(other).iterator_;
            }

        private:
            Iterator iterator_;
        };

        struct shape_iter
        {
            using iterator_category = std::input_iterator_tag;
            using value_type = std::size_t;
            using difference_type = std::ptrdiff_t;
            using pointer = std::size_t*;
            using reference = std::size_t&;

            shape_iter() = default;
            template <typename Iterator>
            explicit shape_iter(Iterator it)
              : impl_(new shape_iter_impl<Iterator>(it))
            {
            }

            // we need to deep-copy the embedded iterator
            shape_iter(shape_iter const& rhs)
              : impl_(rhs.impl_->clone())
            {
            }

            shape_iter& operator=(shape_iter const& rhs)
            {
                if (this != &rhs)
                    impl_->copy(*rhs.impl_);
                return *this;
            }

            shape_iter(shape_iter&& rhs) = default;
            shape_iter& operator=(shape_iter&& rhs) = default;

            std::size_t operator*() const
            {
                return impl_->dereference();
            }

            shape_iter& operator++()
            {
                impl_->increment();
                return *this;
            }

            shape_iter operator++(int)
            {
                auto copy = *this;
                ++*this;
                return copy;
            }

            friend bool operator==(shape_iter const& lhs, shape_iter const& rhs)
            {
                return lhs.impl_->equal_to(*rhs.impl_);
            }

            friend bool operator!=(shape_iter const& lhs, shape_iter const& rhs)
            {
                return !(lhs == rhs);
            }

        protected:
            std::unique_ptr<shape_iter_impl_base> impl_{};
        };

        struct range_proxy
        {
            using value_type = std::size_t;

            template <typename Shape>
            explicit range_proxy(Shape const& s)
              : begin_(s.begin())
              , end_(s.end())
              , size_(std::distance(s.begin(), s.end()))
            {
            }

            shape_iter begin() const
            {
                return begin_;
            }

            shape_iter end() const
            {
                return end_;
            }

            constexpr std::ptrdiff_t size() const noexcept
            {
                return size_;
            }

        private:
            shape_iter begin_;
            shape_iter end_;
            std::ptrdiff_t size_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        struct construct_vtable
        {
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename VTable, typename T>
        struct polymorphic_executor_vtables
        {
            static constexpr VTable instance =
                VTable(detail::construct_vtable<T>());
        };

        template <typename VTable, typename T>
        inline constexpr VTable
            polymorphic_executor_vtables<VTable, T>::instance;

        template <typename VTable, typename T>
        constexpr VTable const* get_polymorphic_executor_vtable() noexcept
        {
            static_assert(
                !std::is_reference_v<T>, "T shall have no ref-qualifiers");

            return &polymorphic_executor_vtables<VTable, T>::instance;
        }

        ///////////////////////////////////////////////////////////////////////
        struct empty_polymorphic_executor
        {
        };    // must be trivial and empty

        [[noreturn]] HPX_CORE_EXPORT void throw_bad_polymorphic_executor();

        template <typename R>
        [[noreturn]] inline R throw_bad_polymorphic_executor()
        {
            throw_bad_polymorphic_executor();
#if defined(HPX_INTEL_VERSION)
            return {};
#endif
        }

        ///////////////////////////////////////////////////////////////////////
        struct vtable_base
        {
            template <typename T>
            static T& get(void* obj) noexcept
            {
                return *reinterpret_cast<T*>(obj);
            }

            template <typename T>
            static T const& get(void const* obj) noexcept
            {
                return *reinterpret_cast<T const*>(obj);
            }

            template <typename T>
            static void* allocate(void* storage, std::size_t const storage_size)
            {
                if (sizeof(T) > storage_size)
                {
                    using storage_t =
                        hpx::aligned_storage_t<sizeof(T), alignof(T)>;
                    return new storage_t;
                }
                return storage;
            }

            template <typename T>
            static void _deallocate(void* obj, std::size_t const storage_size,
                bool const destroy) noexcept
            {
                if (destroy)
                {
                    get<T>(obj).~T();
                }

                if (sizeof(T) > storage_size)
                {
                    using storage_t =
                        hpx::aligned_storage_t<sizeof(T), alignof(T)>;
                    delete static_cast<storage_t*>(obj);
                }
            }
            void (*deallocate)(void*, std::size_t storage_size, bool) noexcept;

            template <typename T>
            static void* _copy(void* storage, std::size_t const storage_size,
                void const* src, bool const destroy)
            {
                if (destroy)
                {
                    get<T>(storage).~T();
                }

                void* buffer = allocate<T>(storage, storage_size);
                return hpx::construct_at(static_cast<T*>(buffer), get<T>(src));
            }
            void* (*copy)(void*, std::size_t, void const*, bool);

            template <typename T>
            constexpr explicit vtable_base(construct_vtable<T>) noexcept
              : deallocate(&vtable_base::template _deallocate<T>)
              , copy(&vtable_base::template _copy<T>)
            {
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // NonBlockingOneWayExecutor interface
        template <typename Sig>
        struct never_blocking_oneway_vtable;

        template <typename R, typename... Ts>
        struct never_blocking_oneway_vtable<R(Ts...)>
        {
            using post_function_type = hpx::move_only_function<R(Ts...)>;

            // post
            template <typename T>
            static void _post(void* exec, post_function_type&& f, Ts&&... ts)
            {
                execution::post(vtable_base::get<T>(exec), HPX_MOVE(f),
                    HPX_FORWARD(Ts, ts)...);
            }
            void (*post)(void*, post_function_type&&, Ts&&...);

            template <typename T>
            constexpr explicit never_blocking_oneway_vtable(
                construct_vtable<T>) noexcept
              : post(&never_blocking_oneway_vtable::template _post<T>)
            {
            }

            static void _empty_post(void*, post_function_type&&, Ts&&...)
            {
                throw_bad_polymorphic_executor();
            }

            constexpr explicit never_blocking_oneway_vtable(
                construct_vtable<empty_polymorphic_executor>) noexcept
              : post(&never_blocking_oneway_vtable::_empty_post)
            {
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // OneWayExecutor interface
        template <typename Sig>
        struct oneway_vtable;

        template <typename R, typename... Ts>
        struct oneway_vtable<R(Ts...)>
        {
            using sync_execute_function_type =
                hpx::move_only_function<R(Ts...)>;

            // sync_execute
            template <typename T>
            static R _sync_execute(
                void* exec, sync_execute_function_type&& f, Ts&&... ts)
            {
                return execution::sync_execute(vtable_base::get<T>(exec),
                    HPX_MOVE(f), HPX_FORWARD(Ts, ts)...);
            }
            R (*sync_execute)(void*, sync_execute_function_type&&, Ts&&...);

            template <typename T>
            constexpr explicit oneway_vtable(construct_vtable<T>) noexcept
              : sync_execute(&oneway_vtable::template _sync_execute<T>)
            {
            }

            static R _empty_sync_execute(
                void*, sync_execute_function_type&&, Ts&&...)
            {
                throw_bad_polymorphic_executor<R>();
                return {};
            }

            constexpr explicit oneway_vtable(
                construct_vtable<empty_polymorphic_executor>) noexcept
              : sync_execute(&oneway_vtable::_empty_sync_execute)
            {
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // TwoWayExecutor interface
        template <typename Sig>
        struct twoway_vtable;

        template <typename R, typename... Ts>
        struct twoway_vtable<R(Ts...)>
        {
            using async_execute_function_type =
                hpx::move_only_function<R(Ts...)>;
            using then_execute_function_type = hpx::move_only_function<R(
                hpx::shared_future<void> const&, Ts...)>;

            // async_execute
            template <typename T>
            static hpx::future<R> _async_execute(
                void* exec, async_execute_function_type&& f, Ts&&... ts)
            {
                return execution::async_execute(vtable_base::get<T>(exec),
                    HPX_MOVE(f), HPX_FORWARD(Ts, ts)...);
            }
            hpx::future<R> (*async_execute)(
                void*, async_execute_function_type&& f, Ts&&...);

            // then_execute
            template <typename T>
            static hpx::future<R> _then_execute(void* exec,
                then_execute_function_type&& f,
                hpx::shared_future<void> const& predecessor, Ts&&... ts)
            {
                return execution::then_execute(vtable_base::get<T>(exec),
                    HPX_MOVE(f), predecessor, HPX_FORWARD(Ts, ts)...);
            }
            hpx::future<R> (*then_execute)(void*,
                then_execute_function_type&& f, hpx::shared_future<void> const&,
                Ts&&...);

            template <typename T>
            constexpr explicit twoway_vtable(construct_vtable<T>) noexcept
              : async_execute(&twoway_vtable::template _async_execute<T>)
              , then_execute(&twoway_vtable::template _then_execute<T>)
            {
            }

            static hpx::future<R> _empty_async_execute(void* /* exec */,
                async_execute_function_type&& /* f */, Ts&&... /* ts */)
            {
                throw_bad_polymorphic_executor<R>();
                return {};
            }
            static hpx::future<R> _empty_then_execute(void*,
                then_execute_function_type&&, hpx::shared_future<void> const&,
                Ts&&...)
            {
                throw_bad_polymorphic_executor<R>();
                return {};
            }

            constexpr explicit twoway_vtable(
                construct_vtable<empty_polymorphic_executor>) noexcept
              : async_execute(&twoway_vtable::_empty_async_execute)
              , then_execute(&twoway_vtable::_empty_then_execute)
            {
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // BulkOneWayExecutor interface
        template <typename Sig>
        struct bulk_oneway_vtable;

        template <typename R, typename... Ts>
        struct bulk_oneway_vtable<R(Ts...)>
        {
            using bulk_sync_execute_function_type =
                hpx::function<R(std::size_t, Ts...)>;

            // bulk_sync_execute
            template <typename T>
            static std::vector<R> _bulk_sync_execute(void* exec,
                bulk_sync_execute_function_type&& f, range_proxy const& shape,
                Ts&&... ts)
            {
                return execution::bulk_sync_execute(vtable_base::get<T>(exec),
                    HPX_MOVE(f), shape, HPX_FORWARD(Ts, ts)...);
            }
            std::vector<R> (*bulk_sync_execute)(void*,
                bulk_sync_execute_function_type&&, range_proxy const& shape,
                Ts&&...);

            template <typename T>
            constexpr explicit bulk_oneway_vtable(construct_vtable<T>) noexcept
              : bulk_sync_execute(
                    &bulk_oneway_vtable::template _bulk_sync_execute<T>)
            {
            }

            static std::vector<R> _empty_bulk_sync_execute(void*,
                bulk_sync_execute_function_type&&, range_proxy const&, Ts&&...)
            {
                throw_bad_polymorphic_executor<R>();
                return {};
            }

            constexpr explicit bulk_oneway_vtable(
                construct_vtable<empty_polymorphic_executor>) noexcept
              : bulk_sync_execute(&bulk_oneway_vtable::_empty_bulk_sync_execute)
            {
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // BulkTwoWayExecutor interface
        template <typename Sig>
        struct bulk_twoway_vtable;

        template <typename R, typename... Ts>
        struct bulk_twoway_vtable<R(Ts...)>
        {
            using bulk_async_execute_function_type =
                hpx::function<R(std::size_t, Ts...)>;
            using bulk_then_execute_function_type = hpx::function<R(
                std::size_t, hpx::shared_future<void> const&, Ts...)>;

            // bulk_async_execute
            template <typename T>
            static hpx::future<std::vector<R>> _bulk_async_execute(void* exec,
                bulk_async_execute_function_type&& f, range_proxy const& shape,
                Ts&&... ts)
            {
                auto result =
                    execution::bulk_async_execute(vtable_base::get<T>(exec),
                        HPX_MOVE(f), shape, HPX_FORWARD(Ts, ts)...);

                // bulk_async_execute returns either a future<vector<R>> or a
                // vector<future<R>>
                if constexpr (hpx::traits::is_future_v<decltype(result)>)
                {
                    return result;
                }
                else
                {
                    return hpx::make_future<std::vector<R>>(
                        hpx::when_all(HPX_MOVE(result)), [](auto&& inner) {
                            return hpx::unwrap(
                                HPX_FORWARD(decltype(inner), inner));
                        });
                }
            }
            hpx::future<std::vector<R>> (*bulk_async_execute)(void*,
                bulk_async_execute_function_type&&, range_proxy const& shape,
                Ts&&...);

            // bulk_then_execute
            template <typename T>
            static hpx::future<std::vector<R>> _bulk_then_execute(void* exec,
                bulk_then_execute_function_type&& f, range_proxy const& shape,
                hpx::shared_future<void> const& predecessor, Ts&&... ts)
            {
                return execution::bulk_then_execute(vtable_base::get<T>(exec),
                    HPX_MOVE(f), shape, predecessor, HPX_FORWARD(Ts, ts)...);
            }
            hpx::future<std::vector<R>> (*bulk_then_execute)(void*,
                bulk_then_execute_function_type&&, range_proxy const& shape,
                hpx::shared_future<void> const&, Ts&&...);

            template <typename T>
            constexpr explicit bulk_twoway_vtable(construct_vtable<T>) noexcept
              : bulk_async_execute(
                    &bulk_twoway_vtable::template _bulk_async_execute<T>)
              , bulk_then_execute(
                    &bulk_twoway_vtable::template _bulk_then_execute<T>)
            {
            }

            static hpx::future<std::vector<R>> _empty_bulk_async_execute(void*,
                bulk_async_execute_function_type&&, range_proxy const&, Ts&&...)
            {
                throw_bad_polymorphic_executor<R>();
                return {};
            }

            static hpx::future<std::vector<R>> _empty_bulk_then_execute(void*,
                bulk_then_execute_function_type&&, range_proxy const&,
                hpx::shared_future<void> const&, Ts&&...)
            {
                throw_bad_polymorphic_executor<R>();
                return {};
            }

            constexpr explicit bulk_twoway_vtable(
                construct_vtable<empty_polymorphic_executor>) noexcept
              : bulk_async_execute(
                    &bulk_twoway_vtable::_empty_bulk_async_execute)
              , bulk_then_execute(&bulk_twoway_vtable::_empty_bulk_then_execute)
            {
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Sig>
        struct polymorphic_executor_vtable
          : vtable_base
          , never_blocking_oneway_vtable<Sig>
          , oneway_vtable<Sig>
          , twoway_vtable<Sig>
          , bulk_oneway_vtable<Sig>
          , bulk_twoway_vtable<Sig>
        {
            template <typename T>
            constexpr explicit polymorphic_executor_vtable(
                construct_vtable<T>) noexcept
              : vtable_base(construct_vtable<T>())
              , never_blocking_oneway_vtable<Sig>(construct_vtable<T>())
              , oneway_vtable<Sig>(construct_vtable<T>())
              , twoway_vtable<Sig>(construct_vtable<T>())
              , bulk_oneway_vtable<Sig>(construct_vtable<T>())
              , bulk_twoway_vtable<Sig>(construct_vtable<T>())
            {
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // make sure the empty table instance is initialized in time, even
        // during early startup
        template <typename Sig>
        struct polymorphic_executor_vtable;

        // NOTE: nvcc (at least CUDA 9.2 and 10.1) fails with an internal
        // compiler error ("there was an error in verifying the lgenfe output!")
        // with this enabled, so we explicitly use the fallback.
#if !defined(HPX_HAVE_GPU_SUPPORT)
        template <typename Sig>
        constexpr polymorphic_executor_vtable<Sig> const*
        get_empty_polymorphic_executor_vtable() noexcept
        {
            return &polymorphic_executor_vtables<
                polymorphic_executor_vtable<Sig>,
                empty_polymorphic_executor>::instance;
        }
#else
        template <typename Sig>
        polymorphic_executor_vtable<Sig> const*
        get_empty_polymorphic_executor_vtable() noexcept
        {
            static polymorphic_executor_vtable<Sig> const empty_vtable =
                polymorphic_executor_vtable<Sig>(
                    detail::construct_vtable<empty_polymorphic_executor>());
            return &empty_vtable;
        }
#endif

        ///////////////////////////////////////////////////////////////////////
        inline constexpr std::size_t polymorphic_executor_storage_size =
            3 * sizeof(void*);

        class HPX_CORE_EXPORT polymorphic_executor_base
        {
            using vtable = vtable_base;

        public:
            constexpr explicit polymorphic_executor_base(
                vtable const* empty_vptr) noexcept
              : vptr(empty_vptr)
              , object(nullptr)
              , storage_init()
            {
            }

            polymorphic_executor_base(polymorphic_executor_base const& other,
                vtable const* empty_vtable);
            polymorphic_executor_base(polymorphic_executor_base&& other,
                vtable const* empty_vtable) noexcept;
            ~polymorphic_executor_base();

            void op_assign(polymorphic_executor_base const& other,
                vtable const* empty_vtable);
            void op_assign(polymorphic_executor_base&& other,
                vtable const* empty_vtable) noexcept;

            void destroy() noexcept;
            void reset(vtable const* empty_vptr) noexcept;
            void swap(polymorphic_executor_base& exec) noexcept;

            bool empty() const noexcept
            {
                return object == nullptr;
            }

            explicit operator bool() const noexcept
            {
                return !empty();
            }

        protected:
            vtable const* vptr;
            void* object;
            union
            {
                char storage_init;
                mutable unsigned char
                    storage[polymorphic_executor_storage_size];
            };
        };

        ///////////////////////////////////////////////////////////////////////////
        inline bool is_empty_polymorphic_executor_impl(
            polymorphic_executor_base const* exec) noexcept
        {
            return exec->empty();
        }

        inline constexpr bool is_empty_polymorphic_executor_impl(...) noexcept
        {
            return false;
        }

        template <typename Exec>
        constexpr bool is_empty_polymorphic_executor(Exec const& exec) noexcept
        {
            return detail::is_empty_polymorphic_executor_impl(&exec);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Sig>
    class polymorphic_executor;

    template <typename R, typename... Ts>
    class polymorphic_executor<R(Ts...)> : detail::polymorphic_executor_base
    {
        using base_type = detail::polymorphic_executor_base;
        using vtable = detail::polymorphic_executor_vtable<R(Ts...)>;

    public:
        constexpr polymorphic_executor() noexcept
          : base_type(get_empty_vtable())
        {
        }

        polymorphic_executor(polymorphic_executor const& other)
          : base_type(other, get_empty_vtable())
        {
        }

        polymorphic_executor(polymorphic_executor&& other) noexcept
          : base_type(HPX_MOVE(other), get_empty_vtable())
        {
        }

        polymorphic_executor& operator=(polymorphic_executor const& other)
        {
            base_type::op_assign(other, get_empty_vtable());
            return *this;
        }

        polymorphic_executor& operator=(polymorphic_executor&& other) noexcept
        {
            base_type::op_assign(HPX_MOVE(other), get_empty_vtable());
            return *this;
        }

        template <typename Exec, typename PE = std::decay_t<Exec>,
            typename Enable =
                std::enable_if_t<!std::is_same_v<PE, polymorphic_executor>>>
        polymorphic_executor(Exec&& exec)
          : base_type(get_empty_vtable())
        {
            assign(HPX_FORWARD(Exec, exec));
        }

        template <typename Exec, typename PE = std::decay_t<Exec>,
            typename Enable =
                std::enable_if_t<!std::is_same_v<PE, polymorphic_executor>>>
        polymorphic_executor& operator=(Exec&& exec)
        {
            assign(HPX_FORWARD(Exec, exec));
            return *this;
        }

    private:
        void assign(std::nullptr_t) noexcept
        {
            base_type::reset(get_empty_vtable());
        }

        template <typename Exec>
        void assign(Exec&& exec)
        {
            using T = std::decay_t<Exec>;
            static_assert(std::is_constructible_v<T, T const&>,
                "Exec shall be CopyConstructible");

            if (!detail::is_empty_polymorphic_executor(exec))
            {
                vtable const* exec_vptr = get_vtable<T>();
                void* buffer = nullptr;
                if (vptr == exec_vptr)
                {
                    // reuse object storage
                    HPX_ASSERT(object != nullptr);
                    buffer = object;
                    vtable::template get<T>(object).~T();
                }
                else
                {
                    destroy();
                    vptr = exec_vptr;
                    buffer = vtable::template allocate<T>(
                        storage, detail::polymorphic_executor_storage_size);
                }
                object = hpx::construct_at(
                    static_cast<T*>(buffer), HPX_FORWARD(Exec, exec));
            }
            else
            {
                base_type::reset(get_empty_vtable());
            }
        }

    public:
        void reset() noexcept
        {
            base_type::reset(get_empty_vtable());
        }

        using base_type::empty;
        using base_type::swap;
        using base_type::operator bool;

        ///////////////////////////////////////////////////////////////////////
        // actual executor interface

        // This executor always exposes future<R>
        template <typename>
        using future_type = hpx::future<R>;

    public:
        // NonBlockingOneWayExecutor interface
        template <typename F>
        HPX_FORCEINLINE void post(F&& f, Ts... ts) const
        {
            using function_type = typename vtable::post_function_type;

            vtable const* vptr_ = static_cast<vtable const*>(base_type::vptr);
            vptr_->post(object, function_type(HPX_FORWARD(F, f)),
                HPX_FORWARD(Ts, ts)...);
        }

        // OneWayExecutor interface
        template <typename F>
        HPX_FORCEINLINE R sync_execute(F&& f, Ts... ts) const
        {
            using function_type = typename vtable::sync_execute_function_type;

            vtable const* vptr_ = static_cast<vtable const*>(base_type::vptr);
            return vptr_->sync_execute(object, function_type(HPX_FORWARD(F, f)),
                HPX_FORWARD(Ts, ts)...);
        }

        // TwoWayExecutor interface
        template <typename F>
        HPX_FORCEINLINE hpx::future<R> async_execute(F&& f, Ts... ts) const
        {
            using function_type = typename vtable::async_execute_function_type;

            vtable const* vptr_ = static_cast<vtable const*>(base_type::vptr);
            return vptr_->async_execute(object,
                function_type(HPX_FORWARD(F, f)), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future>
        HPX_FORCEINLINE hpx::future<R> then_execute(
            F&& f, Future&& predecessor, Ts&&... ts) const
        {
            using function_type = typename vtable::then_execute_function_type;

            vtable const* vptr_ = static_cast<vtable const*>(base_type::vptr);
            return vptr_->then_execute(object, function_type(HPX_FORWARD(F, f)),
                hpx::make_shared_future(HPX_FORWARD(Future, predecessor)),
                HPX_FORWARD(Ts, ts)...);
        }

        // BulkOneWayExecutor interface
        template <typename F, typename Shape>
            requires(!std::is_integral_v<Shape>)
        HPX_FORCEINLINE std::vector<R> bulk_sync_execute(
            F&& f, Shape const& s, Ts&&... ts) const
        {
            using function_type =
                typename vtable::bulk_sync_execute_function_type;

            detail::range_proxy shape(s);
            vtable const* vptr_ = static_cast<vtable const*>(base_type::vptr);
            return vptr_->bulk_sync_execute(object,
                function_type(HPX_FORWARD(F, f)), shape,
                HPX_FORWARD(Ts, ts)...);
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename Shape>
            requires(!std::is_integral_v<Shape>)
        HPX_FORCEINLINE hpx::future<std::vector<R>> bulk_async_execute(
            F&& f, Shape const& s, Ts&&... ts) const
        {
            using function_type =
                typename vtable::bulk_async_execute_function_type;

            detail::range_proxy shape(s);
            vtable const* vptr_ = static_cast<vtable const*>(base_type::vptr);
            return vptr_->bulk_async_execute(object,
                function_type(HPX_FORWARD(F, f)), shape,
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Shape>
            requires(!std::is_integral_v<Shape>)
        HPX_FORCEINLINE hpx::future<std::vector<R>> bulk_then_execute(F&& f,
            Shape const& s, hpx::shared_future<void> const& predecessor,
            Ts&&... ts) const
        {
            using function_type =
                typename vtable::bulk_then_execute_function_type;

            detail::range_proxy shape(s);
            vtable const* vptr_ = static_cast<vtable const*>(base_type::vptr);
            return vptr_->bulk_then_execute(object,
                function_type(HPX_FORWARD(F, f)), shape, predecessor,
                HPX_FORWARD(Ts, ts)...);
        }

    private:
        static constexpr vtable const* get_empty_vtable() noexcept
        {
            return detail::get_empty_polymorphic_executor_vtable<R(Ts...)>();
        }

        template <typename T>
        static constexpr vtable const* get_vtable() noexcept
        {
            return detail::get_polymorphic_executor_vtable<vtable, T>();
        }

    protected:
        using base_type::object;
        using base_type::storage;
        using base_type::vptr;
    };
}    // namespace hpx::parallel::execution

namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    template <typename Sig>
    struct is_never_blocking_one_way_executor<
        parallel::execution::polymorphic_executor<Sig>> : std::true_type
    {
    };

    template <typename Sig>
    struct is_one_way_executor<parallel::execution::polymorphic_executor<Sig>>
      : std::true_type
    {
    };

    template <typename Sig>
    struct is_two_way_executor<parallel::execution::polymorphic_executor<Sig>>
      : std::true_type
    {
    };

    template <typename Sig>
    struct is_bulk_one_way_executor<
        parallel::execution::polymorphic_executor<Sig>> : std::true_type
    {
    };

    template <typename Sig>
    struct is_bulk_two_way_executor<
        parallel::execution::polymorphic_executor<Sig>> : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
