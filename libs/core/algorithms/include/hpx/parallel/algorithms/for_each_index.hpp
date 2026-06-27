//  Copyright (c) 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/for_each_index.hpp
/// \page hpx::experimental::for_each_index
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace experimental {

    /// \brief Calls \a fun with every multidimensional index in the domain of
    ///        \a mapping (i.e., its extents), in sequential order.
    ///
    /// \note Complexity: Applies \a fun exactly the product of all extents of
    ///       \a mapping times.
    ///
    /// If \a fun returns a result, the result is ignored.
    ///
    /// The layout mapping is a hint; implementations are encouraged to choose
    /// an iteration order that performs well for the given mapping.
    ///
    /// \tparam Mapping   A type meeting the layout mapping requirements. Must
    ///                   expose \a index_type, \a extents_type,
    ///                   \c static \a rank(), and \a extents().
    /// \tparam Fun       The type of the function/function object to use
    ///                   (deduced). \a Fun must meet \a CopyConstructible and
    ///                   be invocable with exactly \a Mapping::rank() arguments
    ///                   of type \a Mapping::index_type.
    ///
    /// \param mapping    The layout mapping whose extents define the iteration
    ///                   domain.
    /// \param fun        Callable invoked as \c fun(i0, i1, ...) for every
    ///                   multidimensional index in the domain.
    ///
    template <typename Mapping, typename Fun>
    constexpr void for_each_index(Mapping const& mapping, Fun fun);

    /// \brief Calls \a fun with every multidimensional index in the domain of
    ///        \a mapping (i.e., its extents), executed according to \a policy.
    ///
    /// \note Complexity: Applies \a fun exactly the product of all extents of
    ///       \a mapping times.
    ///
    /// If \a fun returns a result, the result is ignored.
    ///
    /// \tparam ExPolicy  The type of the execution policy to use (deduced).
    /// \tparam Mapping   A type meeting the layout mapping requirements.
    ///                   Must meet \a CopyConstructible.
    /// \tparam Fun       The type of the function/function object to use
    ///                   (deduced). Must meet \a CopyConstructible.
    ///
    /// \param policy     The execution policy to use for scheduling.
    /// \param mapping    The layout mapping whose extents define the iteration
    ///                   domain.
    /// \param fun        Callable invoked as \c fun(i0, i1, ...) for every
    ///                   multidimensional index in the domain.
    ///
    /// \returns  \a hpx::future<void> if the execution policy is
    ///           \a sequenced_task_policy or \a parallel_task_policy,
    ///           \a void otherwise.
    ///
    template <typename ExPolicy, typename Mapping, typename Fun>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy> for_each_index(
        ExPolicy&& policy, Mapping const& mapping, Fun fun);

}}    // namespace hpx::experimental

#else

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL

    // Sequential recursive core.
    //
    // Accumulates one index per dimension each level of recursion, then calls
    // fun(i0, i1, ..., i_{rank-1}) when all indices are collected.
    //
    // Template parameter RankIdx is the *next* dimension to iterate (0-based).
    // When RankIdx == Mapping::rank() we have the full index tuple.
    template <std::size_t RankIdx, typename ExPolicy, typename Mapping,
        typename Fun, typename... Indices>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void
    for_each_index_seq_impl_right(Mapping const& m, Fun& f, Indices... idx)
    {
        if constexpr (RankIdx == Mapping::rank())
        {
            hpx::invoke(f, idx...);
        }
        else if constexpr (RankIdx == Mapping::rank() - 1)
        {
            using index_type = typename Mapping::index_type;
            std::size_t const bound =
                static_cast<std::size_t>(m.extents().extent(RankIdx));

            hpx::util::counting_iterator<std::size_t> first(std::size_t{0});
            hpx::parallel::util::loop_n<std::decay_t<ExPolicy>>(
                first, bound, [&](auto const& it) {
                    hpx::invoke(f, idx..., static_cast<index_type>(*it));
                });
        }
        else
        {
            using index_type = typename Mapping::index_type;
            std::size_t const bound =
                static_cast<std::size_t>(m.extents().extent(RankIdx));
            for (std::size_t i = std::size_t{0}; i < bound; ++i)
            {
                for_each_index_seq_impl_right<RankIdx + 1, ExPolicy>(
                    m, f, idx..., static_cast<index_type>(i));
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Function object used by the parallel partitioner.
    //
    // The partitioner splits the integer range [0, extent(0)) across threads.
    // Each call receives a sub-range of i0 values; for each i0 we recurse
    // sequentially over the remaining dimensions.
    //
    // The Mapping and Fun are stored by value so the function object is
    // thread-safe and can be copied to GPU device memory if needed.
    template <typename ExPolicy, typename Mapping, typename Fun>
    struct for_each_index_iteration_right
    {
        using fun_type = std::decay_t<Fun>;
        using index_type = typename Mapping::index_type;

        Mapping mapping_;
        fun_type fun_;

        template <typename M, typename F>
        HPX_HOST_DEVICE for_each_index_iteration_right(M&& m, F&& f)
          : mapping_(HPX_FORWARD(M, m))
          , fun_(HPX_FORWARD(F, f))
        {
        }

#if !defined(__NVCC__) && !defined(__CUDACC__)
        for_each_index_iteration_right(
            for_each_index_iteration_right const&) = default;
        for_each_index_iteration_right(
            for_each_index_iteration_right&&) = default;
#else
        HPX_HOST_DEVICE for_each_index_iteration_right(
            for_each_index_iteration_right const& rhs)
          : mapping_(rhs.mapping_)
          , fun_(rhs.fun_)
        {
        }
        HPX_HOST_DEVICE for_each_index_iteration_right(
            for_each_index_iteration_right&& rhs)
          : mapping_(HPX_MOVE(rhs.mapping_))
          , fun_(HPX_MOVE(rhs.fun_))
        {
        }
#endif
        for_each_index_iteration_right& operator=(
            for_each_index_iteration_right const&) = default;
        for_each_index_iteration_right& operator=(
            for_each_index_iteration_right&&) = default;
        ~for_each_index_iteration_right() = default;

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
            Iter part_begin, std::size_t part_size)
        {
            hpx::parallel::util::loop_n<std::decay_t<ExPolicy>>(
                part_begin, part_size, [&](auto const& it) {
                    auto i0 = static_cast<index_type>(*it);
                    for_each_index_seq_impl_right<1, ExPolicy>(
                        mapping_, fun_, i0);
                });
        }

        // Overload for partitioners that pass a chunk index.
        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
            Iter part_begin, std::size_t part_size, std::size_t)
        {
            (*this)(part_begin, part_size);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Layout-aware (column-major / layout_left) optimized implementations.

    // Accumulates indices from right-to-left.
    // RankIdx decreases from Mapping::rank()-2 down to 0.
    template <std::size_t RankIdx, typename ExPolicy, typename Mapping,
        typename Fun, typename... Indices>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void for_each_index_seq_impl_left(
        Mapping const& m, Fun& f, Indices... idx)
    {
        if constexpr (RankIdx == 0)
        {
            using index_type = typename Mapping::index_type;
            std::size_t const bound =
                static_cast<std::size_t>(m.extents().extent(0));

            hpx::util::counting_iterator<std::size_t> first(std::size_t{0});
            hpx::parallel::util::loop_n<std::decay_t<ExPolicy>>(
                first, bound, [&](auto const& it) {
                    hpx::invoke(f, static_cast<index_type>(*it), idx...);
                });
        }
        else
        {
            using index_type = typename Mapping::index_type;
            std::size_t const bound =
                static_cast<std::size_t>(m.extents().extent(RankIdx));
            for (std::size_t i = std::size_t{0}; i < bound; ++i)
            {
                for_each_index_seq_impl_left<RankIdx - 1, ExPolicy>(
                    m, f, static_cast<index_type>(i), idx...);
            }
        }
    }

    template <typename ExPolicy, typename Mapping, typename Fun>
    struct for_each_index_iteration_left
    {
        using fun_type = std::decay_t<Fun>;
        using index_type = typename Mapping::index_type;

        Mapping mapping_;
        fun_type fun_;

        template <typename M, typename F>
        HPX_HOST_DEVICE for_each_index_iteration_left(M&& m, F&& f)
          : mapping_(HPX_FORWARD(M, m))
          , fun_(HPX_FORWARD(F, f))
        {
        }

#if !defined(__NVCC__) && !defined(__CUDACC__)
        for_each_index_iteration_left(
            for_each_index_iteration_left const&) = default;
        for_each_index_iteration_left(
            for_each_index_iteration_left&&) = default;
#else
        HPX_HOST_DEVICE for_each_index_iteration_left(
            for_each_index_iteration_left const& rhs)
          : mapping_(rhs.mapping_)
          , fun_(rhs.fun_)
        {
        }
        HPX_HOST_DEVICE for_each_index_iteration_left(
            for_each_index_iteration_left&& rhs)
          : mapping_(HPX_MOVE(rhs.mapping_))
          , fun_(HPX_MOVE(rhs.fun_))
        {
        }
#endif
        for_each_index_iteration_left& operator=(
            for_each_index_iteration_left const&) = default;
        for_each_index_iteration_left& operator=(
            for_each_index_iteration_left&&) = default;
        ~for_each_index_iteration_left() = default;

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
            Iter part_begin, std::size_t part_size)
        {
            static_assert(Mapping::rank() >= 2,
                "for_each_index_iteration_left requires rank >= 2");
            hpx::parallel::util::loop_n<std::decay_t<ExPolicy>>(
                part_begin, part_size, [&](auto const& it) {
                    auto i_last = static_cast<index_type>(*it);
                    for_each_index_seq_impl_left<Mapping::rank() - 2, ExPolicy>(
                        mapping_, fun_, i_last);
                });
        }

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
            Iter part_begin, std::size_t part_size, std::size_t)
        {
            (*this)(part_begin, part_size);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Algorithm struct - bridges sequential / parallel dispatch via the
    // hpx::parallel::detail::algorithm CRTP base.
    //
    // sequential() is marked constexpr (non-parallel overload of
    // for_each_index is constexpr per the proposal).
    // parallel()   uses util::partitioner to distribute work.
    HPX_CXX_CORE_EXPORT struct for_each_index_algo
      : public algorithm<for_each_index_algo>
    {
        constexpr for_each_index_algo() noexcept
          : for_each_index_algo::algorithm("for_each_index")
        {
        }

        // Sequential path.
        template <typename ExPolicy, typename Mapping, typename Fun>
        HPX_HOST_DEVICE static constexpr hpx::util::unused_type sequential(
            ExPolicy&&, Mapping const& m, Fun&& f)
        {
            for_each_index_seq_impl_right<0, std::decay_t<ExPolicy>>(m, f);
            return {};
        }

        // Parallel path.
        //
        // Dispatch is layout-aware: column-major (layout_left) mappings
        // parallelize the last extent and recurse inward; row-major
        // (layout_right) and all other layouts parallelize the first extent.
        template <typename ExPolicy, typename Mapping, typename Fun>
        static decltype(auto) parallel(
            ExPolicy&& policy, Mapping const& m, Fun&& f)
        {
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            // rank-0: exactly one invocation - execute sequentially.
            if constexpr (Mapping::rank() == 0)
            {
                hpx::invoke(f);
                return util::detail::algorithm_result<ExPolicy>::get();
            }
            else
            {
                if constexpr (Mapping::rank() >= 2)
                {
                    bool prefer_left = false;
                    if constexpr (requires { m.stride(0); })
                    {
                        if (m.stride(0) == 1 &&
                            m.stride(Mapping::rank() - 1) > 1)
                        {
                            prefer_left = true;
                        }
                    }

                    if (prefer_left)
                    {
                        std::size_t const count = static_cast<std::size_t>(
                            m.extents().extent(Mapping::rank() - 1));

                        if constexpr (!has_scheduler_executor)
                        {
                            if (count == 0)
                            {
                                return util::detail::algorithm_result<
                                    ExPolicy>::get();
                            }
                        }

                        using iter_t =
                            hpx::util::counting_iterator<std::size_t>;
                        iter_t first{std::size_t{0}};

                        auto iter_fun = for_each_index_iteration_left<
                            std::decay_t<ExPolicy>, Mapping, std::decay_t<Fun>>{
                            m, HPX_FORWARD(Fun, f)};

                        if constexpr (hpx::is_async_execution_policy_v<
                                          ExPolicy> ||
                            has_scheduler_executor)
                        {
                            return util::call_with_algorithm_result<ExPolicy>(
                                HPX_FORWARD(ExPolicy, policy), first, count,
                                HPX_MOVE(iter_fun),
                                hpx::util::empty_function{});
                        }
                        else
                        {
                            util::partitioner<ExPolicy>::call(
                                HPX_FORWARD(ExPolicy, policy), first, count,
                                HPX_MOVE(iter_fun),
                                hpx::util::empty_function{});
                            return util::detail::algorithm_result<
                                ExPolicy>::get();
                        }
                    }
                }

                // Default layout_right (row-major) optimized path
                std::size_t const count =
                    static_cast<std::size_t>(m.extents().extent(0));

                if constexpr (!has_scheduler_executor)
                {
                    if (count == 0)
                    {
                        return util::detail::algorithm_result<ExPolicy>::get();
                    }
                }

                // Integer iterator over [0, extent(0)) for the partitioner.
                using iter_t = hpx::util::counting_iterator<std::size_t>;
                iter_t first{std::size_t{0}};

                auto iter_fun =
                    for_each_index_iteration_right<std::decay_t<ExPolicy>,
                        Mapping, std::decay_t<Fun>>{m, HPX_FORWARD(Fun, f)};

                if constexpr (hpx::is_async_execution_policy_v<ExPolicy> ||
                    has_scheduler_executor)
                {
                    return util::call_with_algorithm_result<ExPolicy>(
                        HPX_FORWARD(ExPolicy, policy), first, count,
                        HPX_MOVE(iter_fun), hpx::util::empty_function{});
                }
                else
                {
                    util::partitioner<ExPolicy>::call(
                        HPX_FORWARD(ExPolicy, policy), first, count,
                        HPX_MOVE(iter_fun), hpx::util::empty_function{});
                    return util::detail::algorithm_result<ExPolicy>::get();
                }
            }
        }
    };

    /// \endcond
}    // namespace hpx::parallel::detail

///////////////////////////////////////////////////////////////////////////////
namespace hpx::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // Concept helper: detect minimum layout-mapping interface.
    //
    // We check only the members we actually use so that user-defined mapping
    // types that omit optional members can still participate.
    namespace detail {

        /// \cond NOINTERNAL
        template <typename M, typename = void>
        struct is_layout_mapping_impl : std::false_type
        {
        };

        // A type M satisfies the concept when it exposes:
        //   - M::index_type   (member type)
        //   - M::extents_type (member type)
        //   - M::rank()       (static constexpr function returning std::size_t)
        //   - m.extents()     (const member function)
        template <typename M>
        struct is_layout_mapping_impl<M,
            std::void_t<typename M::index_type, typename M::extents_type,
                decltype(M::rank()),
                decltype(std::declval<M const&>().extents()),
                decltype(std::declval<M const&>().extents().extent(
                    std::size_t{}))>> : std::true_type
        {
        };
        /// \endcond

    }    // namespace detail

    /// True iff \a M meets the minimum layout mapping requirements used by
    /// \a for_each_index.
    template <typename M>
    inline constexpr bool is_layout_mapping_v =
        detail::is_layout_mapping_impl<std::decay_t<M>>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct for_each_index_t final
      : hpx::detail::tag_parallel_algorithm<for_each_index_t>
    {
    private:
        // ------------------------------------------------------------------ //
        // Sequential overload (no execution policy) - constexpr, returns void.
        // ------------------------------------------------------------------ //
        template <typename Mapping, typename Fun>
        // clang-format off
        requires (
            is_layout_mapping_v<Mapping> &&
            std::is_copy_constructible_v<std::decay_t<Fun>>
        )
        // clang-format on
        friend constexpr void tag_fallback_invoke(
            hpx::experimental::for_each_index_t, Mapping const& mapping,
            Fun fun)
        {
            hpx::parallel::detail::for_each_index_seq_impl_right<0,
                hpx::execution::sequenced_policy>(mapping, fun);
        }

        // ------------------------------------------------------------------ //
        // Parallel overload (with execution policy) - void or future<void>.
        // ------------------------------------------------------------------ //
        template <typename ExPolicy, typename Mapping, typename Fun>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            is_layout_mapping_v<Mapping> &&
            std::is_copy_constructible_v<std::decay_t<Mapping>> &&
            std::is_copy_constructible_v<std::decay_t<Fun>>
        )
        // clang-format on
        friend hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
        tag_fallback_invoke(hpx::experimental::for_each_index_t,
            ExPolicy&& policy, Mapping const& mapping, Fun fun)
        {
            return hpx::parallel::detail::for_each_index_algo().call(
                HPX_FORWARD(ExPolicy, policy), mapping, HPX_MOVE(fun));
        }
    } for_each_index{};

}    // namespace hpx::experimental

#endif
