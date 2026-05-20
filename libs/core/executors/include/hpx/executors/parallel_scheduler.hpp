// Copyright (c) 2025 Sai Charan Arvapally
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>

#include <hpx/execution/algorithms/detail/sync_wait_domain.hpp>
#include <hpx/executors/parallel_scheduler_backend.hpp>
#include <hpx/executors/thread_pool_scheduler.hpp>
#include <hpx/executors/thread_pool_scheduler_bulk.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace hpx::execution::experimental {

    // Forward declaration for parallel_scheduler_domain
    HPX_CXX_CORE_EXPORT class parallel_scheduler;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT parallel_scheduler
    get_parallel_scheduler();

    // Virtual bulk dispatch infrastructure for P2079R10.
    //
    // transform_sender must return a single concrete type, but we
    // need two execution paths:
    //   - Fast path (default HPX backend): thread_pool_bulk_sender
    //     with work-stealing, NUMA awareness, etc.
    //   - Virtual path (custom backends): routes through
    //     backend->schedule_bulk_chunked/unchunked().
    //
    // Solution: type-erase the operation state behind a virtual
    // base class. Cost: one heap allocation per bulk operation.
    // For bulk work processing thousands of elements, this is
    // negligible.
    namespace detail {

        // Virtual base for type-erased bulk operation states.
        HPX_CXX_CORE_EXPORT struct base_parallel_bulk_op
        {
            virtual ~base_parallel_bulk_op() = default;
            virtual void start() noexcept = 0;
        };

        // Fast path: wraps thread_pool_bulk_sender's connected
        // operation state. Zero overhead beyond the heap allocation.
        HPX_CXX_CORE_EXPORT template <typename FastSender, typename Receiver>
        struct fast_parallel_bulk_op final : base_parallel_bulk_op
        {
            using inner_op_t =
                hpx::execution::experimental::connect_result_t<FastSender,
                    Receiver>;

            inner_op_t inner_;

            fast_parallel_bulk_op(FastSender&& s, Receiver&& r)
              : inner_(hpx::execution::experimental::connect(
                    HPX_MOVE(s), HPX_MOVE(r)))
            {
            }

            void start() noexcept override
            {
                hpx::execution::experimental::start(inner_);
            }
        };

        // Virtual dispatch path: connects child sender to an internal
        // receiver. When the child completes with values, constructs a
        // concrete_proxy in inline aligned storage (no heap allocation) and
        // calls backend->schedule_bulk_chunked() or schedule_bulk_unchunked().
        HPX_CXX_CORE_EXPORT template <typename F, bool IsChunked,
            bool IsParallel, typename ChildSender, typename Receiver>
        struct virtual_parallel_bulk_op final : base_parallel_bulk_op
        {
            std::shared_ptr<parallel_scheduler_backend> backend_;
            std::size_t
                count_;    // Count passed to backend (1 for seq, shape for par)
            std::size_t
                actual_shape_;    // P3804R2: Actual shape for proxy execution
            F f_;
            std::decay_t<Receiver> receiver_;

            // Pre-allocated storage passed to the backend as scratch space.
            alignas(parallel_scheduler_storage_alignment)
                std::byte storage_[parallel_scheduler_storage_size];

            // ---- Nested concrete proxy template -------------------------
            // Lifted out of do_bulk() so that sizeof/alignof are computable
            // for the inline storage below.  Ts... are the decayed value types
            // types forwarded by the child sender.
            template <typename... Ts>
            struct concrete_proxy final
              : parallel_scheduler_bulk_item_receiver_proxy
            {
                virtual_parallel_bulk_op& op_;
                std::tuple<Ts...> values_;

                // Takes values by value so both lvalue and rvalue arguments
                // from the child sender are handled uniformly.
                concrete_proxy(virtual_parallel_bulk_op& o, Ts... ts)
                  : op_(o)
                  , values_(HPX_MOVE(ts)...)
                {
                }

                void execute(
                    std::size_t begin, std::size_t end) noexcept override
                {
                    if constexpr (IsChunked)
                    {
                        if constexpr (IsParallel)
                        {
                            std::apply(
                                [&](auto&... vals) {
                                    op_.f_(begin, end, vals...);
                                },
                                values_);
                        }
                        else
                        {
                            // P3804R2: seq policy -> f(0, shape, args...)
                            std::apply(
                                [&](auto&... vals) {
                                    op_.f_(0, op_.actual_shape_, vals...);
                                },
                                values_);
                        }
                    }
                    else
                    {
                        if constexpr (IsParallel)
                        {
                            for (std::size_t i = begin; i < end; ++i)
                            {
                                std::apply(
                                    [&](auto&... vals) { op_.f_(i, vals...); },
                                    values_);
                            }
                        }
                        else
                        {
                            // P3804R2: seq -> for(i=0; i<shape; ++i) f(i,...)
                            for (std::size_t i = 0; i < op_.actual_shape_; ++i)
                            {
                                std::apply(
                                    [&](auto&... vals) { op_.f_(i, vals...); },
                                    values_);
                            }
                        }
                    }
                }

                void set_value() noexcept override
                {
                    std::apply(
                        [&](auto&&... vals) {
                            hpx::execution::experimental::set_value(
                                HPX_MOVE(op_.receiver_), HPX_MOVE(vals)...);
                        },
                        std::move(values_));
                }

                void set_error(std::exception_ptr ep) noexcept override
                {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(op_.receiver_), HPX_MOVE(ep));
                }

                void set_stopped() noexcept override
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(op_.receiver_));
                }

                bool stop_requested() const noexcept override
                {
                    return get_stop_token(get_env(op_.receiver_))
                        .stop_requested();
                }
            };

            // ---- Proxy type computation ----------------------------------
            // Derive the concrete_proxy specialisation from ChildSender's
            // value completion type.  Bulk chains always have exactly one
            // value completion signature (static_assert below enforces this).
            using value_env_t = env_of_t<std::decay_t<Receiver>>;

            // mk_decayed_tuple<T1,T2,...> = std::tuple<decay_t<T1>,...>
            template <typename... Ts>
            using mk_decayed_tuple = std::tuple<std::decay_t<Ts>...>;

            // std::variant<std::tuple<decay_t<Ts>...>> for each value sig
            using value_variant_t = value_types_of_t<ChildSender, value_env_t,
                mk_decayed_tuple, std::variant>;

            static_assert(std::variant_size_v<value_variant_t> == 1,
                "virtual_parallel_bulk_op: child sender must have exactly "
                "one value completion signature");

            // std::tuple<decay_t<T1>, decay_t<T2>, ...>
            using value_tuple_t =
                std::variant_alternative_t<0, value_variant_t>;

            // concrete_proxy<T1, T2, ...> from std::tuple<T1, T2, ...>
            template <typename Tuple>
            struct proxy_for_tuple;
            template <typename... Ts>
            struct proxy_for_tuple<std::tuple<Ts...>>
            {
                using type = concrete_proxy<Ts...>;
            };
            using proxy_t = typename proxy_for_tuple<value_tuple_t>::type;

            // ---- Inline proxy storage ------------------------------------
            // Eliminates the second heap allocation that make_unique<proxy>
            // would require.  Valid from do_bulk() until the first completion
            // signal is delivered, after which the operation state is
            // released and this destructor runs.
            alignas(proxy_t) std::byte proxy_buf_[sizeof(proxy_t)];
            bool proxy_active_ = false;

            proxy_t& active_proxy() noexcept
            {
                return *std::launder(reinterpret_cast<proxy_t*>(proxy_buf_));
            }

            // ---- Child receiver -----------------------------------------
            struct child_receiver
            {
                using receiver_concept =
                    hpx::execution::experimental::receiver_t;
                virtual_parallel_bulk_op* self_;

                template <typename... Vs>
                void set_value(Vs&&... vs) & noexcept
                {
                    self_->do_bulk(HPX_FORWARD(Vs, vs)...);
                }

                template <typename... Vs>
                void set_value(Vs&&... vs) && noexcept
                {
                    static_cast<child_receiver&>(*this).set_value(
                        HPX_FORWARD(Vs, vs)...);
                }

                void set_error(std::exception_ptr ep) & noexcept
                {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(self_->receiver_), HPX_MOVE(ep));
                }

                void set_error(std::exception_ptr ep) && noexcept
                {
                    static_cast<child_receiver&>(*this).set_error(HPX_MOVE(ep));
                }

                void set_stopped() & noexcept
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(self_->receiver_));
                }

                void set_stopped() && noexcept
                {
                    static_cast<child_receiver&>(*this).set_stopped();
                }

                auto get_env() const noexcept
                {
                    return hpx::execution::experimental::get_env(
                        self_->receiver_);
                }
            };

            // Connected child sender's operation state.
            hpx::execution::experimental::connect_result_t<ChildSender,
                child_receiver>
                child_op_;

            virtual_parallel_bulk_op(
                std::shared_ptr<parallel_scheduler_backend> b,
                std::size_t count, std::size_t shape, F f, ChildSender&& child,
                Receiver&& rcvr)
              : backend_(HPX_MOVE(b))
              , count_(count)
              , actual_shape_(shape)
              , f_(HPX_MOVE(f))
              , receiver_(HPX_FORWARD(Receiver, rcvr))
              , child_op_(hpx::execution::experimental::connect(
                    HPX_FORWARD(ChildSender, child), child_receiver{this}))
            {
            }

            ~virtual_parallel_bulk_op()
            {
                if (proxy_active_)
                    active_proxy().~proxy_t();
            }

            void start() noexcept override
            {
                hpx::execution::experimental::start(child_op_);
            }

            // Called by child_receiver::set_value when the child sender
            // completes. Constructs the proxy via placement new into the
            // inline buffer (no heap allocation) then dispatches to the
            // backend.
            template <typename... Vs>
            void do_bulk(Vs&&... vs) noexcept
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        new (proxy_buf_) proxy_t(*this, HPX_FORWARD(Vs, vs)...);
                        proxy_active_ = true;

                        std::span<std::byte> span(storage_);
                        if constexpr (IsChunked)
                        {
                            backend_->schedule_bulk_chunked(
                                count_, active_proxy(), span);
                        }
                        else
                        {
                            backend_->schedule_bulk_unchunked(
                                count_, active_proxy(), span);
                        }
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver_), HPX_MOVE(ep));
                    });
            }
        };

        // Unified sender returned by parallel_scheduler_domain's
        // transform_sender. Holds either the fast-path
        // thread_pool_bulk_sender or virtual dispatch data.
        HPX_CXX_CORE_EXPORT template <typename FastSender, typename ChildSender,
            typename F, bool IsChunked, bool IsParallel>
        struct parallel_bulk_dispatch_sender
        {
            using sender_concept = sender_t;

            struct fast_path_data
            {
                FastSender sender_;
            };

            struct virtual_path_data
            {
                std::shared_ptr<parallel_scheduler_backend> backend_;
                std::size_t count_;    // P3804R2: 1 for seq, shape for par
                std::size_t actual_shape_;    // P3804R2: Actual shape value
                F f_;
                ChildSender child_;
            };

            std::variant<fast_path_data, virtual_path_data> data_;

            template <typename Self, typename Env>
            static consteval auto get_completion_signatures() noexcept
                -> decltype(hpx::execution::experimental::
                        transform_completion_signatures(
                            hpx::execution::experimental::
                                completion_signatures_of_t<ChildSender, Env>{},
                            hpx::execution::experimental::keep_completion<
                                hpx::execution::experimental::set_value_t>{},
                            hpx::execution::experimental::keep_completion<
                                hpx::execution::experimental::set_error_t>{},
                            hpx::execution::experimental::keep_completion<
                                hpx::execution::experimental::set_stopped_t>{},
                            hpx::execution::experimental::completion_signatures<
                                hpx::execution::experimental::set_error_t(
                                    std::exception_ptr)>{}))
            {
                return {};
            }

            // Unified operation state: holds type-erased op via
            // unique_ptr<base_parallel_bulk_op>.
            template <typename Receiver>
            struct dispatch_op
            {
                std::unique_ptr<base_parallel_bulk_op> impl_;

                explicit dispatch_op(std::unique_ptr<base_parallel_bulk_op> p)
                  : impl_(HPX_MOVE(p))
                {
                }

                dispatch_op(dispatch_op&&) = delete;
                dispatch_op(dispatch_op const&) = delete;
                dispatch_op& operator=(dispatch_op&&) = delete;
                dispatch_op& operator=(dispatch_op const&) = delete;

                void start() noexcept
                {
                    impl_->start();
                }
            };

            // connect: creates the right op state behind the
            // type-erased pointer.
            template <typename Receiver>
            friend dispatch_op<std::decay_t<Receiver>> tag_invoke(
                hpx::execution::experimental::connect_t,
                parallel_bulk_dispatch_sender&& self, Receiver&& rcvr)
            {
                if (auto* fast = std::get_if<fast_path_data>(&self.data_))
                {
                    return dispatch_op<std::decay_t<Receiver>>{
                        std::make_unique<fast_parallel_bulk_op<FastSender,
                            std::decay_t<Receiver>>>(HPX_MOVE(fast->sender_),
                            HPX_FORWARD(Receiver, rcvr))};
                }
                else
                {
                    auto& vp = std::get<virtual_path_data>(self.data_);
                    return dispatch_op<std::decay_t<Receiver>>{
                        std::make_unique<virtual_parallel_bulk_op<F, IsChunked,
                            IsParallel, ChildSender, std::decay_t<Receiver>>>(
                            HPX_MOVE(vp.backend_), vp.count_, vp.actual_shape_,
                            HPX_MOVE(vp.f_), HPX_MOVE(vp.child_),
                            HPX_FORWARD(Receiver, rcvr))};
                }
            }
        };

    }    // namespace detail

    // P2079R10: Domain for parallel_scheduler bulk operations.
    // The existing thread_pool_domain checks __completes_on with
    // thread_pool_policy_scheduler, but parallel_scheduler's sender
    // returns parallel_scheduler as the completion scheduler.
    // This domain bridges the gap by extracting the underlying
    // thread_pool_policy_scheduler and delegating to HPX's optimized
    // thread_pool_bulk_sender.
    HPX_CXX_CORE_EXPORT struct parallel_scheduler_domain
      : hpx::execution::experimental::detail::sync_wait_domain
    {
        template <bulk_chunked_or_unchunked_sender Sender, typename Env>
        auto transform_sender(hpx::execution::experimental::set_value_t,
            Sender&& sndr, Env const& /*env*/) const
        {
            if constexpr (hpx::execution::experimental::stdexec_internal::
                              __completes_on<Sender, parallel_scheduler, Env>)
            {
                // Extract bulk parameters using structured binding
                auto&& [tag, data, child] = sndr;
                auto&& [pol, shape, f] = data;

                // Get the parallel_scheduler from the bulk sender's env.
                // The outer if constexpr(__completes_on<Sender,
                // parallel_scheduler, Env>) guarantees this query succeeds,
                // using the same env_of_t<Sender> that __completes_on checks.
                auto par_sched =
                    hpx::execution::experimental::get_completion_scheduler<
                        hpx::execution::experimental::set_value_t>(
                        hpx::execution::experimental::get_env(sndr));

                // Extract the underlying thread pool scheduler from the
                // backend. For the default HPX backend this returns the
                // concrete thread_pool_policy_scheduler; for custom backends
                // it returns nullptr (bulk goes through virtual dispatch).
                auto const* underlying_ptr =
                    par_sched.get_underlying_scheduler();
                auto const* pu_mask_ptr = par_sched.get_pu_mask();

                // Only bulk_chunked_t uses the chunked f(begin, end, ...)
                // signature. Both bulk_t (P3481R5 high-level) and
                // bulk_unchunked_t use the unchunked f(index, ...) signature
                // that HPX's bulk users pass. Treating bulk_t as chunked here
                // would force f(begin, end, ...) on user lambdas that take a
                // single index, causing a template instantiation failure.
                constexpr bool is_chunked =
                    sender_invokes_algorithm_v<Sender, bulk_chunked_t>;

                // Determine parallelism at compile time from policy type
                // (pol is a __policy_wrapper, use __get() to unwrap)
                constexpr bool is_parallel =
                    !is_sequenced_policy_v<std::decay_t<decltype(pol.__get())>>;

                constexpr bool is_unsequenced = is_unsequenced_bulk_policy_v<
                    std::decay_t<decltype(pol.__get())>>;

                auto iota_shape =
                    hpx::util::counting_shape(decltype(shape){0}, shape);

                // Compute the fast-path sender type (needed even on the
                // virtual path so both branches return the same type).
                using fast_sender_t = hpx::execution::experimental::detail::
                    thread_pool_bulk_sender<hpx::launch,
                        std::decay_t<decltype(child)>,
                        std::decay_t<decltype(iota_shape)>,
                        std::decay_t<decltype(f)>, is_chunked, is_parallel,
                        is_unsequenced>;

                using dispatch_sender_t =
                    detail::parallel_bulk_dispatch_sender<fast_sender_t,
                        std::decay_t<decltype(child)>,
                        std::decay_t<decltype(f)>, is_chunked, is_parallel>;

                // Fast path: default HPX backend with underlying scheduler
                // available. Create optimized thread_pool_bulk_sender
                // with work-stealing, NUMA awareness, etc. Use the same
                // processing-unit mask as thread_pool_domain (pool-derived)
                // rather than the backend's cached full_mask so mask and
                // worker-thread cardinality stay aligned (fixes P2079 / small
                // --hpx:threads counts).
                if (underlying_ptr != nullptr)
                {
                    auto underlying = *underlying_ptr;
                    hpx::threads::mask_type pu_mask = *pu_mask_ptr;

                    auto fast_sender = fast_sender_t(HPX_MOVE(underlying),
                        HPX_FORWARD(decltype(child), child),
                        HPX_MOVE(iota_shape), HPX_FORWARD(decltype(f), f),
                        HPX_MOVE(pu_mask));

                    return dispatch_sender_t{
                        typename dispatch_sender_t::fast_path_data{
                            HPX_MOVE(fast_sender)}};
                }

                // Virtual dispatch path: custom backend without an
                // underlying thread_pool_policy_scheduler. Routes
                // through backend->schedule_bulk_chunked/unchunked().
                //
                // P3804R2: Pass (is_parallel ? shape : 1) to backend.
                // When seq policy, backend receives count=1 and proxy
                // will execute all work in a single call:
                //   - chunked: proxy.execute(0, shape) -> f(0, shape, args...)
                //   - unchunked: proxy.execute(0, shape) ->
                //     for(i=0; i<shape; ++i) f(i, args...)
                return dispatch_sender_t{
                    typename dispatch_sender_t::virtual_path_data{
                        par_sched.get_backend(),
                        static_cast<std::size_t>(is_parallel ? shape : 1),
                        static_cast<std::size_t>(shape),
                        HPX_FORWARD(decltype(f), f),
                        HPX_FORWARD(decltype(child), child)}};
            }
            else
            {
                // P2079R10: bulk operations require the parallel_scheduler
                // in the environment. Add a continues_on transition to the
                // parallel_scheduler before the bulk algorithm.
                static_assert(
                    hpx::execution::experimental::stdexec_internal::
                        __completes_on<Sender, parallel_scheduler, Env>,
                    "Cannot dispatch bulk algorithm to the parallel_scheduler: "
                    "no parallel_scheduler found in the environment. "
                    "Add a continues_on transition to the parallel_scheduler "
                    "before the bulk algorithm.");
            }
        }
    };

    // P2079R10 parallel_scheduler implementation.
    // Stores a shared_ptr<parallel_scheduler_backend> for replaceability.
    // The default backend wraps HPX's thread_pool_policy_scheduler.
    HPX_CXX_CORE_EXPORT class parallel_scheduler
    {
    public:
        parallel_scheduler() = delete;

        parallel_scheduler(parallel_scheduler const& other) noexcept = default;
        parallel_scheduler(parallel_scheduler&& other) noexcept = default;
        parallel_scheduler& operator=(
            parallel_scheduler const&) noexcept = default;
        parallel_scheduler& operator=(parallel_scheduler&&) noexcept = default;

        // P2079R10 6.4: two schedulers compare equal iff BACKEND-OF(lhs)
        // and BACKEND-OF(rhs) refer to the same object, i.e., their
        // shared_ptr targets are identical.  Pointer equality is the only
        // comparison mandated by the standard; equal_to() on the backend
        // interface is an HPX-specific extension that custom backends may
        // implement for their own purposes but is not used here.
        friend bool operator==(parallel_scheduler const& lhs,
            parallel_scheduler const& rhs) noexcept
        {
            return lhs.backend_.get() == rhs.backend_.get();
        }

        friend bool operator!=(parallel_scheduler const& lhs,
            parallel_scheduler const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        // P2079R10: query() member for forward progress guarantee
        // (modern stdexec pattern, preferred over tag_invoke)
        constexpr forward_progress_guarantee query(
            get_forward_progress_guarantee_t) const noexcept
        {
            return forward_progress_guarantee::parallel;
        }

        // Scheduling properties: forward to the wrapped thread_pool_policy_scheduler
        // when present so callers use get_processing_units_mask(sched),
        // get_first_core(sched), processing_units_count(..., sched), etc.,
        // consistent with thread_pool_policy_scheduler.
        friend std::size_t tag_invoke(
            get_first_core_t, parallel_scheduler const& sched) noexcept
        {
            if (auto const* u = sched.get_underlying_scheduler())
                return get_first_core(*u);
            return 0;
        }

        template <hpx::executor_parameters Parameters>
        friend std::size_t tag_invoke(processing_units_count_t, Parameters&&,
            parallel_scheduler const& sched,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0)
        {
            if (auto const* u = sched.get_underlying_scheduler())
                return processing_units_count(
                    null_parameters, *u, hpx::chrono::null_duration, 0);
            return 1;
        }

        friend auto tag_invoke(
            get_processing_units_mask_t, parallel_scheduler const& sched)
        {
            if (auto const* cached = sched.get_pu_mask())
                return *cached;
            if (auto const* u = sched.get_underlying_scheduler())
                return get_processing_units_mask(*u);
            return hpx::threads::create_topology().get_machine_affinity_mask();
        }

        friend auto tag_invoke(
            get_cores_mask_t, parallel_scheduler const& sched)
        {
            if (auto const* u = sched.get_underlying_scheduler())
                return get_cores_mask(*u);
            return hpx::threads::create_topology().get_machine_affinity_mask();
        }

        // P2079R10: operation_state owns the receiver and manages the
        // frontend/backend boundary. On start(), it checks the stop token
        // and then delegates to the backend.
        template <typename Receiver>
        struct operation_state
        {
            // Concrete receiver_proxy that adapts the actual Receiver
            // to the type-erased proxy interface.
            struct concrete_receiver_proxy final
              : parallel_scheduler_receiver_proxy
            {
                std::decay_t<Receiver>& receiver_;

                explicit concrete_receiver_proxy(
                    std::decay_t<Receiver>& rcvr) noexcept
                  : receiver_(rcvr)
                {
                }

                void set_value() noexcept override
                {
                    hpx::execution::experimental::set_value(
                        HPX_MOVE(receiver_));
                }

                void set_error(std::exception_ptr ep) noexcept override
                {
                    hpx::execution::experimental::set_error(
                        HPX_MOVE(receiver_), HPX_MOVE(ep));
                }

                void set_stopped() noexcept override
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(receiver_));
                }

                // P2079R10 4.2: allow backends to poll for cancellation.
                // Forwards the stop token state of the actual receiver.
                bool stop_requested() const noexcept override
                {
                    return get_stop_token(get_env(receiver_)).stop_requested();
                }
            };

            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver_;
            std::shared_ptr<parallel_scheduler_backend> backend_;
            // The proxy must be a member (not a local) because the
            // backend's schedule() posts work asynchronously. The
            // operation_state outlives the completion per the
            // sender/receiver protocol.
            concrete_receiver_proxy proxy_;

            // P2079R10 4.2: pre-allocated storage for the backend.
            alignas(parallel_scheduler_storage_alignment)
                std::byte storage_[parallel_scheduler_storage_size];

            template <typename Receiver_>
            operation_state(Receiver_&& receiver,
                std::shared_ptr<parallel_scheduler_backend> backend)
              : receiver_(HPX_FORWARD(Receiver_, receiver))
              , backend_(HPX_MOVE(backend))
              , proxy_(receiver_)
            {
            }

            operation_state(operation_state&&) = delete;
            operation_state(operation_state const&) = delete;
            operation_state& operator=(operation_state&&) = delete;
            operation_state& operator=(operation_state const&) = delete;

            void start() noexcept
            {
                // P2079R10 4.1: if stop_token is stopped, complete
                // with set_stopped as soon as is practical.
                auto stop_token = get_stop_token(get_env(receiver_));
                if (stop_token.stop_requested())
                {
                    set_stopped(HPX_MOVE(receiver_));
                    return;
                }

                // Delegate to the backend via the member proxy,
                // passing pre-allocated storage per P2079R10 / P3927R2.
                backend_->schedule(proxy_, std::span<std::byte>(storage_));
            }
        };

        // Nested sender type
        template <typename Scheduler>
        struct sender
        {
            Scheduler sched_;

            using sender_concept = sender_t;
            using completion_signatures =
                ::hpx::execution::experimental::completion_signatures<
                    set_value_t(), set_error_t(std::exception_ptr),
                    set_stopped_t()>;

            template <typename Receiver>
            friend operation_state<std::decay_t<Receiver>> tag_invoke(
                connect_t, sender const& s, Receiver&& receiver) noexcept(std::
                    is_nothrow_constructible_v<std::decay_t<Receiver>,
                        Receiver>)
            {
                return {
                    HPX_FORWARD(Receiver, receiver), s.sched_.get_backend()};
            }

            template <typename Receiver>
            friend operation_state<std::decay_t<Receiver>>
            tag_invoke(connect_t, sender&& s, Receiver&& receiver) noexcept(
                std::is_nothrow_constructible_v<std::decay_t<Receiver>,
                    Receiver>)
            {
                return {
                    HPX_FORWARD(Receiver, receiver), s.sched_.get_backend()};
            }

            struct env
            {
                Scheduler const& sched_;

                // P2079R10: expose completion scheduler for set_value_t
                // and set_stopped_t
                auto query(
                    get_completion_scheduler_t<set_value_t>) const noexcept
                {
                    return sched_;
                }

                auto query(
                    get_completion_scheduler_t<set_stopped_t>) const noexcept
                {
                    return sched_;
                }

                // Domain query
                parallel_scheduler_domain query(get_domain_t) const noexcept
                {
                    return {};
                }
            };

            env get_env() const noexcept
            {
                return {sched_};
            }
        };

        // Direct schedule() member for modern stdexec
        sender<parallel_scheduler> schedule() const noexcept
        {
            return {*this};
        }

        // Domain customization for bulk operations
        parallel_scheduler_domain query(get_domain_t) const noexcept
        {
            return {};
        }

        // Required for stdexec domain resolution: when a bulk sender's
        // completing domain is resolved, stdexec queries the completion
        // scheduler with get_completion_domain_t<set_value_t>. Without
        // this, the resolution falls to default_domain and our
        // parallel_scheduler_domain::transform_sender is never called.
        parallel_scheduler_domain query(
            get_completion_domain_t<set_value_t>) const noexcept
        {
            return {};
        }

        // Access the backend (for connect and domain transform).
        std::shared_ptr<parallel_scheduler_backend> const& get_backend()
            const noexcept
        {
            return backend_;
        }

        // HPX-specific: access the underlying thread pool scheduler
        // from the backend (returns nullptr for custom backends).
        thread_pool_policy_scheduler<hpx::launch> const*
        get_underlying_scheduler() const noexcept
        {
            return backend_ ? backend_->get_underlying_scheduler() : nullptr;
        }

        // HPX-specific: access the cached PU mask from the backend
        // (returns nullptr for custom backends).
        hpx::threads::mask_type const* get_pu_mask() const noexcept
        {
            return backend_ ? backend_->get_pu_mask() : nullptr;
        }

    private:
        // P2079R10: Construct from a backend shared_ptr. Private; only
        // get_parallel_scheduler() (and copy/move) may produce instances.
        explicit parallel_scheduler(
            std::shared_ptr<parallel_scheduler_backend> backend) noexcept
          : backend_(HPX_MOVE(backend))
        {
        }

        friend parallel_scheduler get_parallel_scheduler();

        std::shared_ptr<parallel_scheduler_backend> backend_;
    };

    // Stream output operator for parallel_scheduler
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::ostream& operator<<(
        std::ostream& os, parallel_scheduler const&);

}    // namespace hpx::execution::experimental
