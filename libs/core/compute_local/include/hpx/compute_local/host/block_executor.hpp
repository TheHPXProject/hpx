//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2024-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/compute_local/host/target.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::compute::host {

    /// The block executor can be used to build NUMA aware programs.
    /// It will distribute work evenly across the passed targets
    ///
    /// \tparam Executor The underlying executor to use
    HPX_CXX_CORE_EXPORT template <
        typename Executor =
            hpx::execution::experimental::restricted_thread_pool_executor>
    struct block_executor
    {
    public:
        using executor_parameters_type =
            hpx::execution::experimental::default_parameters;

        explicit block_executor(std::vector<host::target> const& targets,
            threads::thread_priority priority = threads::thread_priority::high,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {})
          : targets_(targets)
          , current_(0)
          , priority_(priority)
          , stacksize_(stacksize)
          , schedulehint_(schedulehint)
        {
            init_executors();
        }

        explicit block_executor(std::vector<host::target>&& targets)
          : targets_(HPX_MOVE(targets))
          , current_(0)
        {
            init_executors();
        }

        block_executor(block_executor const& other)
          : targets_(other.targets_)
          , current_(0)
          , executors_(other.executors_)
        {
        }

        block_executor(block_executor&& other) noexcept
          : targets_(HPX_MOVE(other.targets_))
          , current_(other.current_.load())
          , executors_(HPX_MOVE(other.executors_))
        {
        }

        block_executor& operator=(block_executor const& other)
        {
            if (&other != this)
            {
                targets_ = other.targets_;
                current_ = 0;
                executors_ = other.executors_;
            }
            return *this;
        }

        block_executor& operator=(block_executor&& other) noexcept
        {
            if (&other != this)
            {
                targets_ = HPX_MOVE(other.targets_);
                current_ = other.current_.load();
                executors_ = HPX_MOVE(other.executors_);
            }
            return *this;
        }

        /// \cond NOINTERNAL
        bool operator==(block_executor const& rhs) const noexcept
        {
            return std::equal(
                targets_.begin(), targets_.end(), rhs.targets_.begin());
        }

        bool operator!=(block_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        std::vector<host::target> const& context() const noexcept
        {
            return targets_;
        }
        /// \endcond

        template <typename F, typename... Ts>
        decltype(auto) post(F&& f, Ts&&... ts) const
        {
            hpx::parallel::execution::post(
                get_next_executor(), HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::async_execute(
                get_next_executor(), HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        decltype(auto) sync_execute(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::sync_execute(
                get_next_executor(), HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Shape, typename... Ts>
        decltype(auto) bulk_async_execute_impl(
            F&& f, Shape const& shape, Ts&&... ts) const
        {
            using result_type =
                parallel::execution::detail::bulk_function_result_t<F, Shape,
                    Ts...>;

            std::size_t cnt = util::size(shape);
            auto begin = util::begin(shape);
            std::size_t const num_executors = executors_.size();

            using bulk_async_result_type =
                decltype(std::declval<Executor&>().bulk_async_execute(
                    std::declval<F>(), util::iterator_range(begin, begin),
                    std::declval<Ts>()...));

            // clang-format off

            // bulk_async_execute either returns a future<result_type> or a
            // vector<future<result_type>> or a future<vector<result_type>>
            using collected_result_type = util::select_t<
                hpx::traits::is_future<bulk_async_result_type>,
                    std::conditional_t<
                        std::is_same_v<
                            bulk_async_result_type,
                            hpx::future<result_type>>,
                        hpx::future<result_type>,
                        hpx::future<std::vector<result_type>>>,
                util::else_t, bulk_async_result_type>;
            // clang-format on

            std::vector<collected_result_type> results;
            results.reserve(cnt);

            try
            {
                for (std::size_t i = 0; i != executors_.size(); ++i)
                {
                    std::size_t part_begin_offset = (i * cnt) / num_executors;
                    std::size_t part_end_offset =
                        ((i + 1) * cnt) / num_executors;
                    auto part_begin = begin;
                    auto part_end = begin;
                    std::advance(part_begin, part_begin_offset);
                    std::advance(part_end, part_end_offset);

                    auto futures = hpx::parallel::execution::bulk_async_execute(
                        executors_[i], f,
                        util::iterator_range(part_begin, part_end), ts...);

                    if constexpr (hpx::traits::is_future_v<decltype(futures)>)
                    {
                        results.push_back(HPX_MOVE(futures));
                    }
                    else
                    {
                        results.insert(results.end(),
                            std::make_move_iterator(futures.begin()),
                            std::make_move_iterator(futures.end()));
                    }
                }
            }
            catch (std::bad_alloc const&)
            {
                throw;
            }
            catch (...)
            {
                results.clear();
                results.emplace_back(
                    hpx::make_exceptional_future<collected_result_type>(
                        std::current_exception()));
            }
            return results;
        }

        template <typename F, typename Shape, typename... Ts>
        decltype(auto) bulk_async_execute(
            F&& f, Shape const& shape, Ts&&... ts) const
        {
            return bulk_async_execute_impl(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Shape, typename... Ts>
        decltype(auto) bulk_sync_execute_impl(
            F&& f, Shape const& shape, Ts&&... ts) const
        {
            using result_type =
                hpx::parallel::execution::detail::bulk_execute_result_t<F,
                    Shape, Ts...>;

            std::size_t cnt = util::size(shape);
            std::size_t const num_executors = executors_.size();

            try
            {
                if constexpr (!std::is_void_v<result_type>)
                {
                    std::vector<result_type> results;
                    results.reserve(cnt);

                    auto begin = util::begin(shape);
                    for (std::size_t i = 0; i != num_executors; ++i)
                    {
                        std::size_t part_begin_offset =
                            (i * cnt) / num_executors;
                        std::size_t part_end_offset =
                            ((i + 1) * cnt) / num_executors;
                        auto part_begin = std::next(begin, part_begin_offset);
                        auto part_end = std::next(begin, part_end_offset);
                        auto part_results =
                            hpx::parallel::execution::bulk_sync_execute(
                                executors_[i], f,
                                util::iterator_range(part_begin, part_end),
                                HPX_FORWARD(Ts, ts)...);
                        results.emplace(results.end(),
                            std::make_move_iterator(part_results.begin()),
                            std::make_move_iterator(part_results.end()));
                    }

                    return results;
                }
                else
                {
                    auto begin = util::begin(shape);
                    for (std::size_t i = 0; i != num_executors; ++i)
                    {
                        std::size_t part_begin_offset =
                            (i * cnt) / num_executors;
                        std::size_t part_end_offset =
                            ((i + 1) * cnt) / num_executors;
                        auto part_begin = std::next(begin, part_begin_offset);
                        auto part_end = std::next(begin, part_end_offset);
                        hpx::parallel::execution::bulk_sync_execute(
                            executors_[i], f,
                            util::iterator_range(part_begin, part_end),
                            HPX_FORWARD(Ts, ts)...);
                    }
                }
            }
            catch (std::bad_alloc const&)
            {
                throw;
            }
            catch (...)
            {
                throw exception_list(std::current_exception());
            }
        }

        template <typename F, typename Shape, typename... Ts>
        decltype(auto) bulk_sync_execute(
            F&& f, Shape const& shape, Ts&&... ts) const
        {
            return bulk_sync_execute_impl(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        std::vector<host::target> const& targets() const noexcept
        {
            return targets_;
        }

    private:
        // this function is conceptually const (current_ is mutable)
        auto get_next_executor() const
        {
            return executors_[current_++ % executors_.size()];
        }

        void init_executors()
        {
            executors_.reserve(targets_.size());
            for (auto const& tgt : targets_)
            {
                auto num_pus = tgt.num_pus();
                executors_.emplace_back(num_pus.first, num_pus.second,
                    priority_, stacksize_, schedulehint_);
            }
        }

        std::vector<host::target> targets_;
        mutable std::atomic<std::size_t> current_;
        std::vector<Executor> executors_;
        threads::thread_priority priority_ = threads::thread_priority::high;
        threads::thread_stacksize stacksize_ =
            threads::thread_stacksize::default_;
        threads::thread_schedule_hint schedulehint_ = {};
    };
}    // namespace hpx::compute::host

namespace hpx::execution::experimental {

    template <typename Executor>
    struct executor_execution_category<compute::host::block_executor<Executor>>
    {
        using type = hpx::execution::parallel_execution_tag;
    };

    template <typename Executor>
    struct is_never_blocking_one_way_executor<
        compute::host::block_executor<Executor>>
      : is_never_blocking_one_way_executor<Executor>
    {
    };

    template <typename Executor>
    struct is_one_way_executor<compute::host::block_executor<Executor>>
      : is_one_way_executor<Executor>
    {
    };

    template <typename Executor>
    struct is_two_way_executor<compute::host::block_executor<Executor>>
      : is_two_way_executor<Executor>
    {
    };

    template <typename Executor>
    struct is_bulk_one_way_executor<compute::host::block_executor<Executor>>
      : is_bulk_one_way_executor<Executor>
    {
    };

    template <typename Executor>
    struct is_bulk_two_way_executor<compute::host::block_executor<Executor>>
      : is_bulk_two_way_executor<Executor>
    {
    };
}    // namespace hpx::execution::experimental
