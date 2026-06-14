//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2016 Marcin Copik
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/execution_policy.hpp
/// \page hpx::execution::seq, hpx::execution::par, hpx::execution::par_unseq, hpx::execution::task, hpx::execution::sequenced_policy, hpx::execution::parallel_policy, hpx::execution::parallel_unsequenced_policy, hpx::execution::sequenced_task_policy, hpx::execution::parallel_task_policy
/// \headerfile hpx/execution.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/execution_policy_fwd.hpp>
#include <hpx/executors/execution_policy_mappings.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/executors/sequenced_executor.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/properties.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

#include <concepts>

namespace hpx::execution {

    namespace detail {

        // forward declare only
        HPX_CXX_CORE_EXPORT template <template <class, class> typename Derived,
            typename Executor, typename Parameters = void,
            typename Category = void>
        struct execution_policy;
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct task_policy_tag final
      : hpx::execution::experimental::to_task_t
    {
    private:
        // we don't want to allow using 'task' as a CPO from user code
        using hpx::execution::experimental::to_task_t::operator();

        template <template <class, class> typename Derived, typename Executor,
            typename Parameters, typename Category>
        friend struct detail::execution_policy;
    };

    HPX_CXX_CORE_EXPORT inline constexpr task_policy_tag task{};

    HPX_CXX_CORE_EXPORT struct non_task_policy_tag final
      : hpx::execution::experimental::to_non_task_t
    {
    private:
        // we don't want to allow using 'non_task' as a CPO from user code
        using hpx::execution::experimental::to_non_task_t::operator();

        template <template <class, class> typename Derived, typename Executor,
            typename Parameters, typename Category>
        friend struct detail::execution_policy;
    };

    HPX_CXX_CORE_EXPORT inline constexpr non_task_policy_tag non_task{};

    namespace experimental {

        template <>
        struct is_execution_policy_mapping<task_policy_tag> : std::true_type
        {
        };

        template <>
        struct is_execution_policy_mapping<non_task_policy_tag> : std::true_type
        {
        };
    }    // namespace experimental

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
        struct has_async_execution_policy : std::false_type
        {
        };

        template <typename T>
        struct has_async_execution_policy<T,
            std::void_t<decltype(std::declval<std::decay_t<T>>()(task))>>
          : std::true_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool has_async_execution_policy_v =
            has_async_execution_policy<T>::value;

        ////////////////////////////////////////////////////////////////////////
        // Base execution policy
        HPX_CXX_CORE_EXPORT template <template <class, class> typename Derived,
            typename Executor, typename Parameters, typename Category>
        struct execution_policy
        {
        private:
            using decayed_executor_type = std::decay_t<Executor>;
            using decayed_parameters_type = std::decay_t<Parameters>;
            using derived_type = Derived<Executor, Parameters>;

            constexpr derived_type& derived() noexcept
            {
                return static_cast<derived_type&>(*this);
            }
            constexpr derived_type const& derived() const noexcept
            {
                return static_cast<derived_type const&>(*this);
            }

        public:
            // The type of the executor associated with this execution policy
            using executor_type = decayed_executor_type;

            // The type of the associated executor parameters object which is
            // associated with this execution policy
            using executor_parameters_type = decayed_parameters_type;

            // The category of the execution agents created by this execution
            // policy.
            using execution_category =
                std::conditional_t<std::is_void_v<Category>,
                    hpx::traits::executor_execution_category_t<executor_type>,
                    Category>;

            // Rebind the type of executor used by this execution policy. The
            // execution category of Executor shall not be weaker than that of
            // this execution policy
            template <typename Executor_, typename Parameters_>
            struct rebind
            {
                using type = Derived<Executor_, Parameters_>;
            };

            constexpr execution_policy() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~execution_policy() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr execution_policy(Executor_&& exec, Parameters_&& params)
              : exec_(HPX_FORWARD(Executor_, exec))
              , params_(HPX_FORWARD(Parameters_, params))
            {
            }

            // Create a new execution policy using the given tag
            //
            // \param tag [in] Specify the type of the execution policy to
            //                 return
            //
            // \returns The new execution policy
            //
            template <typename Tag,
                typename Enable = std::enable_if_t<hpx::execution::
                        experimental::is_execution_policy_mapping_v<Tag>>>
            inline constexpr decltype(auto) operator()(Tag tag) const;

            // Create a new derived execution policy from the given executor
            //
            // \tparam Executor  The type of the executor to associate with this
            //                   execution policy.
            //
            // \param exec       [in] The executor to use for the execution of
            //                   the parallel algorithm the returned execution
            //                   policy is used with.
            //
            // \note Requires: is_executor_v<Executor> is true
            //
            // \returns The new execution policy
            //
            template <typename Executor_>
            constexpr decltype(auto) on(Executor_&& exec) const
            {
                static_assert(
                    hpx::traits::is_executor_any_v<std::decay_t<Executor_>>,
                    "hpx::traits::is_executor_any_v<Executor>");

                return hpx::execution::experimental::create_rebound_policy(
                    derived(), HPX_FORWARD(Executor_, exec), parameters());
            }

            // Create a new execution policy from the given execution parameters
            //
            // \tparam Parameters  The type of the executor parameters to
            //                     associate with this execution policy.
            //
            // \param params       [in] The executor parameters to use for the
            //                     execution of the parallel algorithm the
            //                     returned execution policy is used with.
            //
            // \note Requires: all parameters are executor_parameters, different
            //                 parameter types can't be duplicated
            //
            // \returns The new execution policy
            //
            template <typename... Parameters_>
            constexpr decltype(auto) with(Parameters_&&... params) const
            {
                return hpx::execution::experimental::create_rebound_policy(
                    derived(), executor(),
                    hpx::execution::experimental::join_executor_parameters(
                        HPX_FORWARD(Parameters_, params)...));
            }

        public:
            // Return the associated executor object.
            executor_type& executor() noexcept
            {
                return exec_;
            }

            // Return the associated executor object.
            constexpr executor_type const& executor() const noexcept
            {
                return exec_;
            }

            // Return the associated executor parameters object.
            executor_parameters_type& parameters() noexcept
            {
                return params_;
            }

            // Return the associated executor parameters object.
            constexpr executor_parameters_type const& parameters()
                const noexcept
            {
                return params_;
            }

            // Scheduling property query implementations forward to the
            // embedded executor and rebound through create_rebound_policy.
            template <scheduling_property Tag, typename Property>
                requires(!std::is_same_v<Tag,
                             hpx::execution::experimental::
                                 with_processing_units_count_t> &&
                    std::invocable<Tag, executor_type, Property>)
            [[nodiscard]] auto query(Tag tag, Property&& prop) const
            {
                return hpx::execution::experimental::create_rebound_policy(
                    derived(), tag(executor(), HPX_FORWARD(Property, prop)),
                    parameters());
            }

            template <scheduling_property Tag>
                requires(std::invocable<Tag, executor_type>)
            [[nodiscard]] auto query(Tag tag) const
            {
                return tag(executor());
            }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            [[nodiscard]] auto query(
                hpx::execution::experimental::with_annotation_t,
                char const* annotation) const
                requires(std::invocable<
                    hpx::execution::experimental::with_annotation_t,
                    executor_type, char const*>)
            {
                auto exec = hpx::execution::experimental::with_annotation(
                    executor(), annotation);

                return hpx::execution::experimental::create_rebound_policy(
                    derived(), HPX_MOVE(exec), parameters());
            }

            [[nodiscard]] auto query(
                hpx::execution::experimental::with_annotation_t,
                std::string annotation) const
                requires(std::invocable<
                    hpx::execution::experimental::with_annotation_t,
                    executor_type, std::string>)
            {
                auto exec = hpx::execution::experimental::with_annotation(
                    executor(), HPX_MOVE(annotation));

                return hpx::execution::experimental::create_rebound_policy(
                    derived(), HPX_MOVE(exec), parameters());
            }

            [[nodiscard]] decltype(auto) query(
                hpx::execution::experimental::get_annotation_t) const
                requires(std::invocable<
                    hpx::execution::experimental::get_annotation_t,
                    executor_type>)
            {
                return hpx::execution::experimental::get_annotation(executor());
            }
#endif

            [[nodiscard]] auto query(
                hpx::execution::experimental::with_processing_units_count_t,
                std::size_t num_cores) const
                requires(std::invocable<
                    hpx::execution::experimental::with_processing_units_count_t,
                    executor_type, std::size_t>)
            {
                using exec_type = executor_type;
                using updated_exec_type = std::decay_t<decltype(hpx::execution::
                        experimental::with_processing_units_count(
                            std::declval<exec_type const&>(), num_cores))>;

                if constexpr (std::is_same_v<updated_exec_type, exec_type>)
                {
                    auto exec = hpx::execution::experimental::
                        with_processing_units_count(executor(), num_cores);

                    return hpx::execution::experimental::create_rebound_policy(
                        derived(), HPX_MOVE(exec), parameters());
                }
                else if constexpr (requires(exec_type e) {
                                       e.num_cores_;
                                       e.pool();
                                   })
                {
                    exec_type exec = executor();
                    if (num_cores == 0)
                    {
                        num_cores = exec.pool()->get_active_os_thread_count();
                    }
                    exec.num_cores_ = num_cores;

                    return hpx::execution::experimental::create_rebound_policy(
                        derived(), HPX_MOVE(exec), parameters());
                }
                else
                {
                    auto exec = hpx::execution::experimental::
                        with_processing_units_count(executor(), num_cores);

                    return hpx::execution::experimental::create_rebound_policy(
                        derived(), HPX_MOVE(exec), parameters());
                }
            }

            template <executor_parameters Params>
                requires(std::invocable<hpx::execution::experimental::
                                            with_processing_units_count_t,
                             executor_type, std::size_t> &&
                    std::invocable<
                        hpx::execution::experimental::processing_units_count_t,
                        std::decay_t<Params>, executor_type,
                        hpx::chrono::steady_duration const&, std::size_t>)
            [[nodiscard]] auto query(
                hpx::execution::experimental::with_processing_units_count_t,
                Params&& params) const
            {
                auto exec =
                    hpx::execution::experimental::with_processing_units_count(
                        executor(),
                        hpx::execution::experimental::processing_units_count(
                            HPX_FORWARD(Params, params), executor(),
                            hpx::chrono::null_duration, 0));

                return hpx::execution::experimental::create_rebound_policy(
                    derived(), HPX_MOVE(exec), parameters());
            }

        private:
            friend struct hpx::execution::experimental::create_rebound_policy_t;
            friend class hpx::serialization::access;

            template <typename Archive>
            void serialize(Archive& ar, unsigned int const)
            {
                // clang-format off
                ar & exec_ & params_;
                // clang-format on
            }

        private:
            executor_type exec_;
            executor_parameters_type params_;
        };

        ///////////////////////////////////////////////////////////////////////
        // Extension: The class sequenced_task_policy_shim is an execution
        // policy type used as a unique type to disambiguate parallel algorithm
        // overloading based on combining an underlying \a sequenced_task_policy
        // and an executor and indicate that a parallel algorithm's execution
        // may not be parallelized  (has to run sequentially).
        //
        // The algorithm returns a future representing the result of the
        // corresponding algorithm when invoked with the sequenced_policy.
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
        struct sequenced_task_policy_shim
          : execution_policy<sequenced_task_policy_shim, Executor, Parameters>
        {
        private:
            using base_type = execution_policy<sequenced_task_policy_shim,
                Executor, Parameters>;

        public:
            /// \cond NOINTERNAL
            constexpr sequenced_task_policy_shim() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~sequenced_task_policy_shim() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr sequenced_task_policy_shim(
                Executor_&& exec, Parameters_&& params)
              : base_type(HPX_FORWARD(Executor_, exec),
                    HPX_FORWARD(Parameters_, params))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    !std::is_same_v<
                        sequenced_task_policy_shim<Executor_, Parameters_>,
                        sequenced_task_policy_shim> &&
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            explicit constexpr sequenced_task_policy_shim(
                sequenced_task_policy_shim<Executor_, Parameters_> const& rhs)
              : base_type(sequenced_task_policy_shim(
                    rhs.executor(), rhs.parameters()))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            sequenced_task_policy_shim& operator=(
                sequenced_task_policy_shim<Executor_, Parameters_> const& rhs)
            {
                base_type::operator=(sequenced_task_policy_shim(
                    rhs.executor(), rhs.parameters()));
                return *this;
            }
            /// \endcond

            // Policy mapping member functions
            constexpr auto to_non_task() const;
            constexpr auto to_par() const;
            constexpr auto to_unseq() const;
#if defined(HPX_HAVE_DATAPAR)
            constexpr auto to_simd() const;
#endif
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Extension: The class sequenced_task_policy is an execution policy type
    /// used as a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may not be parallelized
    /// (has to run sequentially).
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the sequenced_policy.
    HPX_CXX_CORE_EXPORT using sequenced_task_policy =
        detail::sequenced_task_policy_shim<sequenced_executor,
            hpx::traits::executor_parameters_type_t<sequenced_executor>>;

    namespace detail {

        // The class sequenced_policy is an execution policy type used as a
        // unique type to disambiguate parallel algorithm overloading and
        // require that a parallel algorithm's execution may not be
        // parallelized.
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
        struct sequenced_policy_shim
          : execution_policy<sequenced_policy_shim, Executor, Parameters>
        {
        private:
            using base_type =
                execution_policy<sequenced_policy_shim, Executor, Parameters>;

        public:
            /// \cond NOINTERNAL
            constexpr sequenced_policy_shim() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~sequenced_policy_shim() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr sequenced_policy_shim(
                Executor_&& exec, Parameters_&& params)
              : base_type(HPX_FORWARD(Executor_, exec),
                    HPX_FORWARD(Parameters_, params))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    !std::is_same_v<
                        sequenced_policy_shim<Executor_, Parameters_>,
                        sequenced_policy_shim> &&
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            explicit constexpr sequenced_policy_shim(
                sequenced_policy_shim<Executor_, Parameters_> const& rhs)
              : base_type(
                    sequenced_policy_shim(rhs.executor(), rhs.parameters()))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            sequenced_policy_shim& operator=(
                sequenced_policy_shim<Executor_, Parameters_> const& rhs)
            {
                base_type::operator=(
                    sequenced_policy_shim(rhs.executor(), rhs.parameters()));
                return *this;
            }
            /// \endcond

            // Policy mapping member functions
            constexpr auto to_task() const;
            constexpr auto to_par() const;
            constexpr auto to_unseq() const;
#if defined(HPX_HAVE_DATAPAR)
            constexpr auto to_simd() const;
#endif
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The class sequenced_policy is an execution policy type used as a unique
    /// type to disambiguate parallel algorithm overloading and require that a
    /// parallel algorithm's execution may not be parallelized.
    HPX_CXX_CORE_EXPORT using sequenced_policy =
        detail::sequenced_policy_shim<sequenced_executor,
            hpx::traits::executor_parameters_type_t<sequenced_executor>>;

    /// Default sequential execution policy object.
    HPX_CXX_CORE_EXPORT inline constexpr sequenced_policy seq{};

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // Extension: The class parallel_task_policy_shim is an execution policy
        // type used as a unique type to disambiguate parallel algorithm
        // overloading based on combining an underlying \a parallel_task_policy
        // and an executor and indicate that a parallel algorithm's execution
        // may be parallelized.
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
        struct parallel_task_policy_shim
          : execution_policy<parallel_task_policy_shim, Executor, Parameters>
        {
        private:
            using base_type = execution_policy<parallel_task_policy_shim,
                Executor, Parameters>;

        public:
            /// \cond NOINTERNAL
            constexpr parallel_task_policy_shim() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~parallel_task_policy_shim() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr parallel_task_policy_shim(
                Executor_&& exec, Parameters_&& params)
              : base_type(HPX_FORWARD(Executor_, exec),
                    HPX_FORWARD(Parameters_, params))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    !std::is_same_v<
                        parallel_task_policy_shim<Executor_, Parameters_>,
                        parallel_task_policy_shim> &&
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            explicit constexpr parallel_task_policy_shim(
                parallel_task_policy_shim<Executor_, Parameters_> const& rhs)
              : base_type(
                    parallel_task_policy_shim(rhs.executor(), rhs.parameters()))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            parallel_task_policy_shim& operator=(
                parallel_task_policy_shim<Executor_, Parameters_> const& rhs)
            {
                base_type::operator=(parallel_task_policy_shim(
                    rhs.executor(), rhs.parameters()));
                return *this;
            }
            /// \endcond

            // Policy mapping member functions
            constexpr auto to_non_task() const;
            constexpr auto to_non_par() const;
            constexpr auto to_unseq() const;
#if defined(HPX_HAVE_DATAPAR)
            constexpr auto to_simd() const;
#endif
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Extension: The class parallel_task_policy is an execution policy type
    /// used as a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may be parallelized.
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the parallel_policy.
    HPX_CXX_CORE_EXPORT using parallel_task_policy =
        detail::parallel_task_policy_shim<parallel_executor,
            hpx::traits::executor_parameters_type_t<parallel_executor>>;

    namespace detail {

        // The class parallel_policy_shim is an execution policy type used as a
        // unique type to disambiguate parallel algorithm overloading and
        // indicate that a parallel algorithm's execution may be parallelized.
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
        struct parallel_policy_shim
          : execution_policy<parallel_policy_shim, Executor, Parameters>
        {
        private:
            using base_type =
                execution_policy<parallel_policy_shim, Executor, Parameters>;

        public:
            /// \cond NOINTERNAL
            constexpr parallel_policy_shim() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~parallel_policy_shim() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr parallel_policy_shim(
                Executor_&& exec, Parameters_&& params)
              : base_type(HPX_FORWARD(Executor_, exec),
                    HPX_FORWARD(Parameters_, params))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    !std::is_same_v<
                        parallel_policy_shim<Executor_, Parameters_>,
                        parallel_policy_shim> &&
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            explicit constexpr parallel_policy_shim(
                parallel_policy_shim<Executor_, Parameters_> const& rhs)
              : base_type(
                    parallel_policy_shim(rhs.executor(), rhs.parameters()))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            parallel_policy_shim& operator=(
                parallel_policy_shim<Executor_, Parameters_> const& rhs)
            {
                base_type::operator=(
                    parallel_policy_shim(rhs.executor(), rhs.parameters()));
                return *this;
            }
            /// \endcond

            // Policy mapping member functions
            constexpr auto to_task() const;
            constexpr auto to_non_par() const;
            constexpr auto to_unseq() const;
#if defined(HPX_HAVE_DATAPAR)
            constexpr auto to_simd() const;
#endif

            /// \cond NOINTERNAL
            // Forward execution operations to wrapped executor
            // (member functions, not tag_invoke)
            template <typename F, typename... Ts>
            decltype(auto) async_execute(F&& f, Ts&&... ts) const
            {
                return hpx::parallel::execution::async_execute(this->executor(),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename F, typename Shape, typename... Ts>
            decltype(auto) bulk_sync_execute(
                F&& f, Shape const& shape, Ts&&... ts) const
            {
                return hpx::parallel::execution::bulk_sync_execute(
                    this->executor(), HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Ts, ts)...);
            }
            /// \endcond
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The class parallel_policy is an execution policy type used as a unique
    /// type to disambiguate parallel algorithm overloading and indicate that a
    /// parallel algorithm's execution may be parallelized.
    HPX_CXX_CORE_EXPORT using parallel_policy =
        detail::parallel_policy_shim<parallel_executor,
            hpx::traits::executor_parameters_type_t<parallel_executor>>;

    /// Default parallel execution policy object.
    HPX_CXX_CORE_EXPORT inline constexpr parallel_policy par{};

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // The class parallel_unsequenced_task_policy_shim is an execution
        // policy type used as a unique type to disambiguate parallel algorithm
        // overloading and indicate that a parallel algorithm's execution may be
        // parallelized and vectorized.
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
        struct parallel_unsequenced_task_policy_shim
          : execution_policy<parallel_unsequenced_task_policy_shim, Executor,
                Parameters>
        {
        private:
            using base_type =
                execution_policy<parallel_unsequenced_task_policy_shim,
                    Executor, Parameters>;

        public:
            /// \cond NOINTERNAL
            constexpr parallel_unsequenced_task_policy_shim() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~parallel_unsequenced_task_policy_shim() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr parallel_unsequenced_task_policy_shim(
                Executor_&& exec, Parameters_&& params)
              : base_type(HPX_FORWARD(Executor_, exec),
                    HPX_FORWARD(Parameters_, params))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    !std::is_same_v<parallel_unsequenced_task_policy_shim<
                                        Executor_, Parameters_>,
                        parallel_unsequenced_task_policy_shim> &&
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            explicit constexpr parallel_unsequenced_task_policy_shim(
                parallel_unsequenced_task_policy_shim<Executor_,
                    Parameters_> const& rhs)
              : base_type(parallel_unsequenced_task_policy_shim(
                    rhs.executor(), rhs.parameters()))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            parallel_unsequenced_task_policy_shim& operator=(
                parallel_unsequenced_task_policy_shim<Executor_,
                    Parameters_> const& rhs)
            {
                base_type::operator=(parallel_unsequenced_task_policy_shim(
                    rhs.executor(), rhs.parameters()));
                return *this;
            }
            /// \endcond

            // Policy mapping member functions
            constexpr auto to_non_task() const;
            constexpr auto to_non_par() const;
            constexpr auto to_non_unseq() const;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The class parallel_unsequenced_task_policy is an execution policy type
    /// used as a unique type to disambiguate parallel algorithm overloading
    /// and indicate that a parallel algorithm's execution may be parallelized
    /// and vectorized.
    HPX_CXX_CORE_EXPORT using parallel_unsequenced_task_policy =
        detail::parallel_unsequenced_task_policy_shim<parallel_executor,
            hpx::traits::executor_parameters_type_t<parallel_executor>>;

    namespace detail {

        // The class parallel_unsequenced_policy_shim is an execution policy type
        // used as a unique type to disambiguate parallel algorithm overloading
        // and indicate that a parallel algorithm's execution may be parallelized.
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
        struct parallel_unsequenced_policy_shim
          : execution_policy<parallel_unsequenced_policy_shim, Executor,
                Parameters>
        {
        private:
            using base_type = execution_policy<parallel_unsequenced_policy_shim,
                Executor, Parameters>;

        public:
            /// \cond NOINTERNAL
            constexpr parallel_unsequenced_policy_shim() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~parallel_unsequenced_policy_shim() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr parallel_unsequenced_policy_shim(
                Executor_&& exec, Parameters_&& params)
              : base_type(HPX_FORWARD(Executor_, exec),
                    HPX_FORWARD(Parameters_, params))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    !std::is_same_v<parallel_unsequenced_policy_shim<Executor_,
                                        Parameters_>,
                        parallel_unsequenced_policy_shim> &&
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            explicit constexpr parallel_unsequenced_policy_shim(
                parallel_unsequenced_policy_shim<Executor_, Parameters_> const&
                    rhs)
              : base_type(parallel_unsequenced_policy_shim(
                    rhs.executor(), rhs.parameters()))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            parallel_unsequenced_policy_shim& operator=(
                parallel_unsequenced_policy_shim<Executor_, Parameters_> const&
                    rhs)
            {
                base_type::operator=(parallel_unsequenced_policy_shim(
                    rhs.executor(), rhs.parameters()));
                return *this;
            }
            /// \endcond

            // Policy mapping member functions
            constexpr auto to_task() const;
            constexpr auto to_non_task() const;
            constexpr auto to_non_par() const;
            constexpr auto to_non_unseq() const;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The class parallel_unsequenced_policy is an execution policy type used
    /// as a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may be parallelized and
    /// vectorized.
    HPX_CXX_CORE_EXPORT using parallel_unsequenced_policy =
        detail::parallel_unsequenced_policy_shim<parallel_executor,
            hpx::traits::executor_parameters_type_t<parallel_executor>>;

    /// Default vector execution policy object.
    HPX_CXX_CORE_EXPORT inline constexpr parallel_unsequenced_policy
        par_unseq{};

    namespace detail {

        // Extension: The class unsequenced_task_policy_shim is an execution
        // policy type used as a unique type to disambiguate parallel algorithm
        // overloading based on combining an underlying
        // \a unsequenced_task_policy and an executor and indicate that a
        // parallel algorithm's execution may be vectorized.
        //
        // The algorithm returns a future representing the result of the
        // corresponding algorithm when invoked with the unsequenced_policy.
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
        struct unsequenced_task_policy_shim
          : execution_policy<unsequenced_task_policy_shim, Executor, Parameters>
        {
        private:
            using base_type = execution_policy<unsequenced_task_policy_shim,
                Executor, Parameters>;

        public:
            /// \cond NOINTERNAL
            constexpr unsequenced_task_policy_shim() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~unsequenced_task_policy_shim() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr unsequenced_task_policy_shim(
                Executor_&& exec, Parameters_&& params)
              : base_type(HPX_FORWARD(Executor_, exec),
                    HPX_FORWARD(Parameters_, params))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    !std::is_same_v<
                        unsequenced_task_policy_shim<Executor_, Parameters_>,
                        unsequenced_task_policy_shim> &&
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            explicit constexpr unsequenced_task_policy_shim(
                unsequenced_task_policy_shim<Executor_, Parameters_> const& rhs)
              : base_type(unsequenced_task_policy_shim(
                    rhs.executor(), rhs.parameters()))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            unsequenced_task_policy_shim& operator=(
                unsequenced_task_policy_shim<Executor_, Parameters_> const& rhs)
            {
                base_type::operator=(unsequenced_task_policy_shim(
                    rhs.executor(), rhs.parameters()));
                return *this;
            }
            /// \endcond

            // Policy mapping member functions
            constexpr auto to_non_task() const;
            constexpr auto to_par() const;
            constexpr auto to_non_unseq() const;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The class unsequenced_task_policy is an execution policy type used as a
    /// unique type to disambiguate parallel algorithm overloading and indicate
    /// that a parallel algorithm's execution may be vectorized.
    HPX_CXX_CORE_EXPORT using unsequenced_task_policy =
        detail::unsequenced_task_policy_shim<sequenced_executor,
            hpx::traits::executor_parameters_type_t<sequenced_executor>>;

    namespace detail {

        // The class unsequenced_policy is an execution policy type used as a
        // unique type to disambiguate parallel algorithm overloading and
        // require that a parallel algorithm's execution may be vectorized.
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
        struct unsequenced_policy_shim
          : execution_policy<unsequenced_policy_shim, Executor, Parameters>
        {
        private:
            using base_type =
                execution_policy<unsequenced_policy_shim, Executor, Parameters>;

        public:
            /// \cond NOINTERNAL
            constexpr unsequenced_policy_shim() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
            constexpr ~unsequenced_policy_shim() {}
#endif

            template <typename Executor_, typename Parameters_>
            constexpr unsequenced_policy_shim(
                Executor_&& exec, Parameters_&& params)
              : base_type(HPX_FORWARD(Executor_, exec),
                    HPX_FORWARD(Parameters_, params))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    !std::is_same_v<
                        unsequenced_policy_shim<Executor_, Parameters_>,
                        unsequenced_policy_shim> &&
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            explicit constexpr unsequenced_policy_shim(
                unsequenced_policy_shim<Executor_, Parameters_> const& rhs)
              : base_type(
                    unsequenced_policy_shim(rhs.executor(), rhs.parameters()))
            {
            }

            template <typename Executor_, typename Parameters_,
                typename = std::enable_if_t<
                    std::is_convertible_v<Executor_, Executor> &&
                    std::is_convertible_v<Parameters_, Parameters>>>
            unsequenced_policy_shim& operator=(
                unsequenced_policy_shim<Executor_, Parameters_> const& rhs)
            {
                base_type::operator=(
                    unsequenced_policy_shim(rhs.executor(), rhs.parameters()));
                return *this;
            }
            /// \endcond

            // Policy mapping member functions
            constexpr auto to_task() const;
            constexpr auto to_par() const;
            constexpr auto to_non_unseq() const;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The class unsequenced_policy is an execution policy type used as a
    /// unique type to disambiguate parallel algorithm overloading and indicate
    /// that a parallel algorithm's execution may be vectorized.
    HPX_CXX_CORE_EXPORT using unsequenced_policy =
        detail::unsequenced_policy_shim<sequenced_executor,
            hpx::traits::executor_parameters_type_t<sequenced_executor>>;

    /// Default vector execution policy object.
    HPX_CXX_CORE_EXPORT inline constexpr unsequenced_policy unseq{};

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // Policy mapping member function implementations
        ///////////////////////////////////////////////////////////////////////
        template <typename Executor, typename Parameters>
        constexpr auto sequenced_policy_shim<Executor, Parameters>::to_task()
            const
        {
            return sequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_task, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto sequenced_policy_shim<Executor, Parameters>::to_par()
            const
        {
            return parallel_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_par, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto sequenced_policy_shim<Executor, Parameters>::to_unseq()
            const
        {
            return unsequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_unseq, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        sequenced_task_policy_shim<Executor, Parameters>::to_non_task() const
        {
            return sequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_task,
                    this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        sequenced_task_policy_shim<Executor, Parameters>::to_par() const
        {
            return parallel_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_par, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        sequenced_task_policy_shim<Executor, Parameters>::to_unseq() const
        {
            return unsequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_unseq, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        parallel_task_policy_shim<Executor, Parameters>::to_non_task() const
        {
            return parallel_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_task,
                    this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        parallel_task_policy_shim<Executor, Parameters>::to_non_par() const
        {
            return sequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_par, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        parallel_task_policy_shim<Executor, Parameters>::to_unseq() const
        {
            return parallel_unsequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_unseq, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_policy_shim<Executor, Parameters>::to_task()
            const
        {
            return parallel_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_task, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_policy_shim<Executor, Parameters>::to_non_par()
            const
        {
            return sequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_par, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_policy_shim<Executor, Parameters>::to_unseq()
            const
        {
            return parallel_unsequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_unseq, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_unsequenced_task_policy_shim<Executor,
            Parameters>::to_non_task() const
        {
            return parallel_unsequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_task,
                    this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_unsequenced_task_policy_shim<Executor,
            Parameters>::to_non_par() const
        {
            return unsequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_par, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_unsequenced_task_policy_shim<Executor,
            Parameters>::to_non_unseq() const
        {
            return parallel_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_unseq,
                    this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        parallel_unsequenced_policy_shim<Executor, Parameters>::to_task() const
        {
            return parallel_unsequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_task, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_unsequenced_policy_shim<Executor,
            Parameters>::to_non_task() const
        {
            return parallel_unsequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_task,
                    this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_unsequenced_policy_shim<Executor,
            Parameters>::to_non_par() const
        {
            return unsequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_par, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_unsequenced_policy_shim<Executor,
            Parameters>::to_non_unseq() const
        {
            return parallel_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_unseq,
                    this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        unsequenced_task_policy_shim<Executor, Parameters>::to_non_task() const
        {
            return unsequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_task,
                    this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        unsequenced_task_policy_shim<Executor, Parameters>::to_par() const
        {
            return parallel_unsequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_par, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        unsequenced_task_policy_shim<Executor, Parameters>::to_non_unseq() const
        {
            return sequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_unseq,
                    this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto unsequenced_policy_shim<Executor, Parameters>::to_task()
            const
        {
            return unsequenced_task_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_task, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto unsequenced_policy_shim<Executor, Parameters>::to_par()
            const
        {
            return parallel_unsequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_par, this->executor()))
                .with(this->parameters());
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        unsequenced_policy_shim<Executor, Parameters>::to_non_unseq() const
        {
            return sequenced_policy()
                .on(hpx::experimental::prefer(
                    hpx::execution::experimental::to_non_unseq,
                    this->executor()))
                .with(this->parameters());
        }

        ///////////////////////////////////////////////////////////////////////
        template <template <class, class> typename Derived, typename Executor,
            typename Parameters, typename Category>
        template <typename Tag, typename Enable>
        constexpr decltype(auto)
        execution_policy<Derived, Executor, Parameters, Category>::operator()(
            Tag tag) const
        {
            return tag(derived());
        }
    }    // namespace detail
}    // namespace hpx::execution

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    // Allow to detect execution policies which were created as a result of a
    // rebind operation. This information can be used to inhibit the
    // construction of a generic execution_policy from any of the rebound
    // policies.
    template <typename Executor, typename Parameters>
    struct is_rebound_execution_policy<
        hpx::execution::detail::sequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_rebound_execution_policy<hpx::execution::detail::
            sequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_rebound_execution_policy<
        hpx::execution::detail::parallel_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_rebound_execution_policy<
        hpx::execution::detail::parallel_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_rebound_execution_policy<
        hpx::execution::detail::unsequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_rebound_execution_policy<hpx::execution::detail::
            unsequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_rebound_execution_policy<hpx::execution::detail::
            parallel_unsequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_rebound_execution_policy<hpx::execution::detail::
            parallel_unsequenced_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    ////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <typename Executor, typename Parameters>
    struct is_execution_policy<
        hpx::execution::detail::parallel_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<hpx::execution::detail::
            parallel_unsequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<
        hpx::execution::detail::unsequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<
        hpx::execution::detail::sequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    // extension
    template <typename Executor, typename Parameters>
    struct is_execution_policy<hpx::execution::detail::
            sequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<
        hpx::execution::detail::parallel_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<hpx::execution::detail::
            unsequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_execution_policy<hpx::execution::detail::
            parallel_unsequenced_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <typename Executor, typename Parameters>
    struct is_parallel_execution_policy<
        hpx::execution::detail::parallel_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_parallel_execution_policy<hpx::execution::detail::
            parallel_unsequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_parallel_execution_policy<
        hpx::execution::detail::parallel_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_parallel_execution_policy<hpx::execution::detail::
            parallel_unsequenced_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <typename Executor, typename Parameters>
    struct is_sequenced_execution_policy<hpx::execution::detail::
            sequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_sequenced_execution_policy<
        hpx::execution::detail::sequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_sequenced_execution_policy<
        hpx::execution::detail::unsequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_sequenced_execution_policy<hpx::execution::detail::
            unsequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <typename Executor, typename Parameters>
    struct is_async_execution_policy<hpx::execution::detail::
            sequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_async_execution_policy<
        hpx::execution::detail::parallel_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_async_execution_policy<hpx::execution::detail::
            unsequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_async_execution_policy<hpx::execution::detail::
            parallel_unsequenced_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <typename Executor, typename Parameters>
    struct is_unsequenced_execution_policy<
        hpx::execution::detail::unsequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_unsequenced_execution_policy<hpx::execution::detail::
            parallel_unsequenced_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_unsequenced_execution_policy<hpx::execution::detail::
            unsequenced_task_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct is_unsequenced_execution_policy<hpx::execution::detail::
            parallel_unsequenced_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };
    /// \endcond

    /// \endcond
}    // namespace hpx::detail

///////////////////////////////////////////////////////////////////////////////
namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    // Make policy shims satisfy executor traits based on their wrapped executor
    template <typename Executor, typename Parameters>
    struct is_two_way_executor<
        hpx::execution::detail::parallel_policy_shim<Executor, Parameters>>
      : is_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_two_way_executor<hpx::execution::detail::
            parallel_unsequenced_policy_shim<Executor, Parameters>>
      : is_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_two_way_executor<
        hpx::execution::detail::parallel_task_policy_shim<Executor, Parameters>>
      : is_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_two_way_executor<hpx::execution::detail::
            parallel_unsequenced_task_policy_shim<Executor, Parameters>>
      : is_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_two_way_executor<hpx::execution::detail::
            sequenced_task_policy_shim<Executor, Parameters>>
      : is_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_two_way_executor<
        hpx::execution::detail::sequenced_policy_shim<Executor, Parameters>>
      : is_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_two_way_executor<
        hpx::execution::detail::unsequenced_policy_shim<Executor, Parameters>>
      : is_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_two_way_executor<hpx::execution::detail::
            unsequenced_task_policy_shim<Executor, Parameters>>
      : is_two_way_executor<Executor>
    {
    };

    // Also add bulk_two_way_executor specializations
    template <typename Executor, typename Parameters>
    struct is_bulk_two_way_executor<
        hpx::execution::detail::parallel_policy_shim<Executor, Parameters>>
      : is_bulk_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_bulk_two_way_executor<hpx::execution::detail::
            parallel_unsequenced_policy_shim<Executor, Parameters>>
      : is_bulk_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_bulk_two_way_executor<
        hpx::execution::detail::parallel_task_policy_shim<Executor, Parameters>>
      : is_bulk_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_bulk_two_way_executor<hpx::execution::detail::
            parallel_unsequenced_task_policy_shim<Executor, Parameters>>
      : is_bulk_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_bulk_two_way_executor<hpx::execution::detail::
            sequenced_task_policy_shim<Executor, Parameters>>
      : is_bulk_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_bulk_two_way_executor<
        hpx::execution::detail::sequenced_policy_shim<Executor, Parameters>>
      : is_bulk_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_bulk_two_way_executor<
        hpx::execution::detail::unsequenced_policy_shim<Executor, Parameters>>
      : is_bulk_two_way_executor<Executor>
    {
    };

    template <typename Executor, typename Parameters>
    struct is_bulk_two_way_executor<hpx::execution::detail::
            unsequenced_task_policy_shim<Executor, Parameters>>
      : is_bulk_two_way_executor<Executor>
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
