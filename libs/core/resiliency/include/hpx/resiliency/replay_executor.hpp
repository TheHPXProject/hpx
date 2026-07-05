//  Copyright (c) 2020-2025 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/resiliency/config.hpp>
#include <hpx/resiliency/async_replay_executor.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/synchronization.hpp>

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::resiliency::experimental {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename BaseExecutor, typename Validate>
    class replay_executor
    {
    public:
        static constexpr int num_spread = 4;
        static constexpr int num_tasks = 128;

        using execution_category =
            hpx::traits::executor_execution_category_t<BaseExecutor>;
        using executor_parameters_type =
            hpx::traits::executor_parameters_type_t<BaseExecutor>;

        template <typename Result>
        using future_type =
            hpx::traits::executor_future_t<BaseExecutor, Result>;

        template <typename BaseExecutor_, typename F>
        explicit replay_executor(BaseExecutor_&& exec, std::size_t n, F&& f)
          : exec_(HPX_FORWARD(BaseExecutor_, exec))
          , replay_count_(n)
          , validator_(HPX_FORWARD(F, f))
        {
        }

        bool operator==(replay_executor const& rhs) const noexcept
        {
            return exec_ == rhs.exec_;
        }

        bool operator!=(replay_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        constexpr replay_executor const& context() const noexcept
        {
            return *this;
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return async_replay_validate(exec_, replay_count_, validator_,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            std::size_t size = hpx::util::size(shape);

            using result_type =
                hpx::parallel::execution::detail::bulk_function_result_t<F, S,
                    Ts...>;
            using future_type =
                hpx::traits::executor_future_t<BaseExecutor, result_type>;

            std::vector<future_type> results;
            results.resize(size);

            hpx::latch l(static_cast<std::ptrdiff_t>(size + 1));

            spawn_hierarchical(results, l, 0, size, num_tasks, f,
                hpx::util::begin(shape), ts...);

            l.arrive_and_wait();

            return results;
        }

    protected:
        /// \cond NOINTERNAL
        template <typename Result, typename F, typename Iter, typename... Ts>
        void spawn_sequential(std::vector<hpx::future<Result>>& results,
            hpx::latch& l, std::size_t base, std::size_t size, F&& func,
            Iter it, Ts&&... ts) const
        {
            // spawn tasks sequentially
            HPX_ASSERT(base + size <= results.size());

            for (std::size_t i = 0; i != size; (void) ++i, ++it)
            {
                results[base + i] = async_execute(func, *it, ts...);
            }

            l.count_down(static_cast<std::ptrdiff_t>(size));
        }

        template <typename Result, typename F, typename Iter, typename... Ts>
        void spawn_hierarchical(std::vector<hpx::future<Result>>& results,
            hpx::latch& l, std::size_t base, std::size_t size,
            std::size_t num_tasks, F&& func, Iter it, Ts&&... ts) const
        {
            if (size > num_tasks)
            {
                // spawn hierarchical tasks
                std::size_t chunk_size = (size + num_spread) / num_spread - 1;
                chunk_size = (std::max) (chunk_size, num_tasks);

                while (size > chunk_size)
                {
                    hpx::parallel::execution::post(
                        exec_, [&, base, chunk_size, num_tasks, it] {
                            spawn_hierarchical(results, l, base, chunk_size,
                                num_tasks, func, it, ts...);
                        });

                    base += chunk_size;
                    std::advance(it, chunk_size);
                    size -= chunk_size;
                }
            }

            // spawn remaining tasks sequentially
            spawn_sequential(results, l, base, size, func, it, ts...);
        }
        /// \endcond

    public:
        BaseExecutor const& get_executor() const
        {
            return exec_;
        }
        std::size_t get_replay_count() const
        {
            return replay_count_;
        }
        Validate const& get_validator() const
        {
            return validator_;
        }

        // support scheduling properties via query() for new CPO dispatch
        template <typename Tag, typename... Args>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag>)
        auto query(Tag tag, Args&&... args) const
            -> decltype(replay_executor<BaseExecutor, Validate>(
                std::declval<Tag>()(
                    std::declval<BaseExecutor>(), HPX_FORWARD(Args, args)...),
                std::declval<std::size_t>(), std::declval<Validate>()))
        {
            return replay_executor<BaseExecutor, Validate>(
                tag(exec_, HPX_FORWARD(Args, args)...), replay_count_,
                validator_);
        }

    private:
        BaseExecutor exec_;
        std::size_t replay_count_;
        Validate validator_;
    };

    ///////////////////////////////////////////////////////////////////////////
    // support all properties exposed by the wrapped executor
    HPX_CXX_CORE_EXPORT template <typename Tag, typename BaseExecutor,
        typename Validate, typename Property,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>)>
    auto tag_invoke(Tag tag,
        replay_executor<BaseExecutor, Validate> const& exec, Property&& prop)
        -> decltype(replay_executor<BaseExecutor, Validate>(
            std::declval<Tag>()(
                std::declval<BaseExecutor>(), std::declval<Property>()),
            std::declval<std::size_t>(), std::declval<Validate>()))
    {
        return replay_executor<BaseExecutor, Validate>(
            tag(exec.get_executor(), HPX_FORWARD(Property, prop)),
            exec.get_replay_count(), exec.get_validator());
    }

    HPX_CXX_CORE_EXPORT template <typename Tag, typename BaseExecutor,
        typename Validate,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>)>
    auto tag_invoke(
        Tag tag, replay_executor<BaseExecutor, Validate> const& exec)
        -> decltype(std::declval<Tag>()(std::declval<BaseExecutor>()))
    {
        return tag(exec.get_executor());
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <executor_any BaseExecutor, typename Validate>
    replay_executor<BaseExecutor, std::decay_t<Validate>> make_replay_executor(
        BaseExecutor& exec, std::size_t n, Validate&& validate)
    {
        return replay_executor<BaseExecutor, std::decay_t<Validate>>(
            exec, n, HPX_FORWARD(Validate, validate));
    }

    HPX_CXX_CORE_EXPORT template <executor_any BaseExecutor>
    replay_executor<BaseExecutor, detail::replay_validator>
    make_replay_executor(BaseExecutor& exec, std::size_t n)
    {
        return replay_executor<BaseExecutor, detail::replay_validator>(
            exec, n, detail::replay_validator());
    }
}    // namespace hpx::resiliency::experimental

namespace hpx::execution::experimental {

    template <typename BaseExecutor, typename Validator>
    struct is_two_way_executor<
        hpx::resiliency::experimental::replay_executor<BaseExecutor, Validator>>
      : std::true_type
    {
    };

    template <typename BaseExecutor, typename Validator>
    struct is_bulk_two_way_executor<
        hpx::resiliency::experimental::replay_executor<BaseExecutor, Validator>>
      : std::true_type
    {
    };
}    // namespace hpx::execution::experimental
