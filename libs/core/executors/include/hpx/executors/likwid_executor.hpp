//  Copyright (c) 2022 Srinivas Yadav
//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_MODULE_LIKWID)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/likwid.hpp>

#include <string>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename BaseExecutor>
    class likwid_executor
    {
    private:
        template <typename F>
        struct hook_wrapper
        {
            template <typename... Ts>
            decltype(auto) operator()(Ts&&... ts)
            {
                hpx::likwid::region r(exec_.name_.c_str());
                return hpx::invoke(f_, HPX_FORWARD(Ts, ts)...);
            }

            likwid_executor const& exec_;
            F f_;
        };

    public:
        using execution_category =
            hpx::traits::executor_execution_category_t<BaseExecutor>;
        using executor_parameters_type =
            hpx::traits::executor_parameters_type_t<BaseExecutor>;

        explicit likwid_executor(BaseExecutor const& exec)
          : exec_(&exec)
          , name_("likwid_executor")
        {
        }

        likwid_executor(BaseExecutor const& exec, std::string name)
          : exec_(&exec)
          , name_(HPX_MOVE(name))
        {
        }

        bool operator==(likwid_executor const& rhs) const noexcept
        {
            return *exec_ == *rhs.exec_;
        }

        bool operator!=(likwid_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        constexpr likwid_executor const& context() const noexcept
        {
            return *this;
        }

        // OneWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) sync_execute(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::sync_execute(base_executor(),
                hook_wrapper<F>{*this, HPX_FORWARD(F, f)},
                HPX_FORWARD(Ts, ts)...);
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::async_execute(base_executor(),
                hook_wrapper<F>{*this, HPX_FORWARD(F, f)},
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
        decltype(auto) then_execute(
            F&& f, Future&& predecessor, Ts&&... ts) const
        {
            return hpx::parallel::execution::then_execute(base_executor(),
                hook_wrapper<F>{*this, HPX_FORWARD(F, f)},
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        // NonBlockingOneWayExecutor (adapted) interface
        template <typename F, typename... Ts>
        void post(F&& f, Ts&&... ts) const
        {
            hpx::parallel::execution::post(base_executor(),
                hook_wrapper<F>{*this, HPX_FORWARD(F, f)},
                HPX_FORWARD(Ts, ts)...);
        }

        // BulkOneWayExecutor interface
        template <typename F, typename S, typename... Ts>
        decltype(auto) bulk_sync_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return hpx::parallel::execution::bulk_sync_execute(base_executor(),
                hook_wrapper<F>{*this, HPX_FORWARD(F, f)}, shape,
                HPX_FORWARD(Ts, ts)...);
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return hpx::parallel::execution::bulk_async_execute(base_executor(),
                hook_wrapper<F>{*this, HPX_FORWARD(F, f)}, shape,
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename Future, typename... Ts>
        decltype(auto) bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts) const
        {
            return hpx::parallel::execution::bulk_then_execute(base_executor(),
                hook_wrapper<F>{*this, HPX_FORWARD(F, f)}, shape,
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        constexpr auto query(hpx::execution::experimental::with_annotation_t,
            char const* name) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.name_ = name;
            return exec_with_annotation;
        }

        auto query(hpx::execution::experimental::with_annotation_t,
            std::string name) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.name_ = HPX_MOVE(name);
            return exec_with_annotation;
        }

        constexpr char const* query(
            hpx::execution::experimental::get_annotation_t) const noexcept
        {
            return name_.c_str();
        }

    private:
        constexpr BaseExecutor const& base_executor() const noexcept
        {
            return *exec_;
        }

    private:
        BaseExecutor const* exec_;
        std::string name_;
    };
}    // namespace hpx::execution::experimental

///////////////////////////////////////////////////////////////////////////////
// simple forwarding implementations of executor traits
namespace hpx::execution::experimental {

    HPX_CXX_CORE_EXPORT template <typename BaseExecutor>
    struct is_one_way_executor<
        hpx::execution::experimental::likwid_executor<BaseExecutor>>
      : is_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename BaseExecutor>
    struct is_never_blocking_one_way_executor<
        hpx::execution::experimental::likwid_executor<BaseExecutor>>
      : is_never_blocking_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename BaseExecutor>
    struct is_two_way_executor<
        hpx::execution::experimental::likwid_executor<BaseExecutor>>
      : is_two_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename BaseExecutor>
    struct is_bulk_one_way_executor<
        hpx::execution::experimental::likwid_executor<BaseExecutor>>
      : is_bulk_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename BaseExecutor>
    struct is_bulk_two_way_executor<
        hpx::execution::experimental::likwid_executor<BaseExecutor>>
      : is_bulk_two_way_executor<std::decay_t<BaseExecutor>>
    {
    };
}    // namespace hpx::execution::experimental

#endif
