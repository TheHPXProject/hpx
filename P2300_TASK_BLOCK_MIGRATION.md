# P2300 Task Block & Task Group Migration

## Goal
Modernize HPX's `task_group` and `define_task_block` algorithms to natively
support C++26 P2300 `stdexec` schedulers and C++20 `requires` clauses for HPX
V2.0. This mirrors the pattern established in the `define_spmd_block` migration
(PR #7512).

## Architecture Pattern
- **Single-allocation shared state struct** consolidating latch, atomic counters,
  and exception lists (same pattern as `detail::spmd_shared_state` in
  `spmd_block.hpp`).
- **Sender storage** via `std::vector<ex::any_sender<>>` for dynamically spawned
  tasks within a `task_group`.
- **Lazy sender construction** using `ex::schedule(sched) | ex::then(f)` instead
  of eager `execution::post`.
- **Sender-based join** via `ex::when_all_vector(std::move(senders_))`.
- **C++20 `requires` clauses** replacing all `std::enable_if_t` on new code paths,
  and progressively on existing code paths.

---

## Files Modified

| File | Phase | Description |
|------|-------|-------------|
| `libs/core/algorithms/include/hpx/parallel/task_group.hpp` | 1 | Add P2300 scheduler `run()` overload, `wait_as_sender()`, sender storage |
| `libs/core/algorithms/src/task_group.cpp` | 1 | No changes needed (new code is header-only template) |
| `libs/core/algorithms/include/hpx/parallel/task_block.hpp` | 2 | Add scheduler overloads for `define_task_block`, `task_block::run`, `define_task_block_restore_thread` |
| `libs/core/algorithms/include/hpx/parallel/run_on_all.hpp` | 2 | Add P2300 scheduler `run_on_all` via `ex::bulk`, disambiguate `requires` |

---

## Phase 1: Overhaul `task_group.hpp` -- COMPLETE

### Summary of Changes
1. Added `#include <hpx/modules/execution.hpp>` and `<vector>` for P2300
   sender types.
2. Added `std::vector<ex::any_sender<>> senders_` member to `task_group` for
   sender-based task accumulation.
3. Added `run(Scheduler&& sched, F&& f, Ts&&... ts)` overload constrained with
   `requires(is_scheduler_v<...>)` that builds lazy senders via
   `ex::schedule(sched) | ex::then(...)`.
4. Added `wait_as_sender()` method returning the combined sender graph via
   `ex::when_all_vector`.
5. Updated the default `run(F&&, Ts&&...)` overload's `requires` clause to
   exclude schedulers (`!is_scheduler_v<...>`), preventing ambiguity.
6. Preserved all existing legacy `run(Executor, F, Ts...)` and `wait()` paths
   unchanged.
7. Updated copyright year to 2026.
8. Ran `clang-format` with project `.clang-format`.

### Design Notes
- The callable and arguments are captured in a `shared_ptr<tuple<...>>` so
  they survive the lifetime of the `run()` call and can be shared across the
  sender graph safely.
- Exceptions inside the sender `then()` callback are caught via
  `try_catch_exception_ptr` and forwarded to `add_exception()`, maintaining
  the same error aggregation semantics as the legacy path.
- The `task_group_shared_state` consolidation struct was deferred -- the
  existing members (`latch_`, `errors_`, `has_arrived_`) are already well-
  encapsulated. This can be revisited if needed during Phase 2.

---

## Phase 2: Update `task_block.hpp` and `run_on_all.hpp` -- COMPLETE

### `task_block.hpp` Changes
1. **Copyright** updated to 2007-2026.
2. **`task_block::run(Scheduler&&, F&&, Ts&&...)` overload** -- New member
   function constrained with `requires(is_scheduler_v<...>)` that delegates
   to the underlying `task_group::run(Scheduler&&, ...)` sender path.
   Performs the same `task_block_not_active` thread-ID validation as the
   legacy `run()` overloads.
3. **`define_task_block_impl::operator()(Scheduler&&, F&&)`** -- New scheduler
   overload in the detail implementation struct.  Constructs a
   `task_block<>` with the default `parallel_policy`, invokes `f(trh, sched)`
   so the user can call `trh.run(sched, callable)` inside the block, then
   joins via `ex::sync_wait(trh.tasks_.wait_as_sender())` followed by the
   legacy `wait_for_completion()` to cover mixed usage.
4. **`define_task_block(Scheduler&&, F&&)`** free function -- Public API
   constrained with `requires(is_scheduler_v<...>)`.
5. **`define_task_block_restore_thread(Scheduler&&, F&&)`** free function --
   Same-thread guarantee variant for the scheduler path.
6. **Disambiguation** -- All existing `requires` clauses on `ExPolicy`-based
   `define_task_block` and `define_task_block_restore_thread` overloads
   (including deprecated `hpx::parallel` wrappers) now also exclude
   schedulers via `!is_scheduler_v<...>`.
7. Ran `clang-format`.

### `run_on_all.hpp` Changes
1. **Copyright** updated to 2025-2026.
2. **`run_on_all(Scheduler&&, F&&, Reductions&&...)`** -- New P2300 overload
   constrained with `requires(is_scheduler_v<...>)`.  Uses
   `ex::schedule(sched) | ex::bulk(cores, task)` to build a lazy sender graph
   for parallel execution, then `ex::sync_wait()` to block until all bulk
   items complete.  Reduction init/exit semantics are identical to the legacy
   path.  Core count is obtained via `processing_units_count(par)` to avoid
   adding a `runtime_local` module dependency.
3. **Disambiguation** -- Existing `requires` clauses on `ExPolicy`-based and
   default `run_on_all` overloads now exclude schedulers.
4. Ran `clang-format`.

### Scheduler-based `define_task_block` API

```cpp
// User-facing API:
hpx::experimental::define_task_block(sched, [](auto& tr, auto& sched) {
    tr.run(sched, [] { /* work */ });
    tr.run(sched, [] { /* more work */ });
});
// All tasks join via wait_as_sender() + sync_wait() when f returns.
```

---

## Phase 3: Write CTest Regression Tests -- COMPLETE

### Summary of Tests Implemented in `task_block_scheduler.cpp`
1. **`test_task_group_scheduler_basic`**: Basic task spawning via P2300 scheduler (`task_group::run(sched, f)` + `wait_as_sender()` + `sync_wait()`).
2. **`test_task_group_scheduler_fib`**: Recursive Fibonacci (`fib_sched`) computing 10th Fibonacci number across sender-driven task groups.
3. **`test_task_group_scheduler_reuse`**: Reuse of `task_group` across multiple `run()` / `wait_as_sender()` cycles.
4. **`test_task_group_scheduler_exception`**: Exception propagation into `hpx::exception_list`, verified with multiple throwing tasks caught at `g.wait()`.
5. **`test_task_group_scheduler_move_only`**: Move-only argument passing (`move_only_payload`) with perfect forwarding through `std::apply`.
6. **`test_task_group_scheduler_composability`**: Sender composability piping `g.wait_as_sender()` into `ex::then(...)`.
7. **`test_define_task_block_scheduler`**: `define_task_block(sched, f)` with both 1-argument `f(tr)` and 2-argument `f(tr, s)` callable forms.
8. **`test_define_task_block_scheduler_exception`**: Exception aggregation and propagation through `define_task_block` throwing `hpx::exception_list`.
9. **`test_define_task_block_restore_thread_scheduler`**: Thread affinity preservation with `define_task_block_restore_thread(sched, f)`.
10. **`test_task_group_mixed_executor_scheduler`**: Mixed execution with concurrent legacy executor (`parallel_executor`) and P2300 scheduler (`thread_pool_scheduler`) on the same `task_group`.
11. **`test_run_on_all_scheduler`**: P2300 scheduler `run_on_all` with single and multiple reduction objects (`reduction_plus`).

### Verification Results
All 7 CTest unit test targets under `tests.unit.modules.algorithms.block` pass 100%:
- `spmd_block`: PASSED
- `task_block`: PASSED
- `task_block_executor`: PASSED
- `task_block_par`: PASSED
- `task_group`: PASSED
- `task_block_scheduler`: PASSED
- `run_on_all`: PASSED

---

## Phase 4: CI/CD & Code Review -- IN PROGRESS

### Status
The Pull Request has been officially opened targeting the HPX V2.0 release on the `feat/modernize-task-block` branch. CodeRabbit automated review completed and identified 4 architectural/lifetime edge cases, all of which have been resolved and verified with dedicated regression tests.

### CodeRabbit Review Findings & Resolutions
1. **Thread Counts & Work Stealing in `run_on_all`**:
   - *Issue*: `run_on_all` with scheduler queried `processing_units_count(par)` instead of the supplied scheduler, and `ex::bulk` without work-stealing restrictions allowed worker threads to steal chunks, breaking the guarantee that each worker thread participates exactly once.
   - *Fix*: Updated `processing_units_count(sched)` to query the scheduler's PU count. Applied `ex::with_priority(sched, hpx::threads::thread_priority::bound)` and `ex::with_hint(..., hint)` configured with `thread_sharing_hint::do_not_share_function` to enforce strict 1:1 PU binding and eliminate thread stealing across worker queues.
   - *Test*: Added `test_run_on_all_scheduler_workers` verifying exact 1:1 execution mapping across all workers using `hpx::get_worker_thread_num()`.

2. **Shared State Lifetime (`task_group_shared_state`)**:
   - *Issue*: `wait_as_sender()` captured `this`, leading to a dangling pointer if the enclosing `task_group` went out of scope before the sender was connected and started.
   - *Fix*: Extracted `exception_list`, `latch_`, `senders_` vector, and thread synchronization into `struct task_group_shared_state` managed by `std::shared_ptr<task_group_shared_state>`. Continuations and sender adaptors capture this shared state by value (`state = state_`), giving the sender independent ownership.
   - *Test*: Added `test_task_group_lifetime_safety` verifying execution after `task_group` destruction.

3. **Legacy `wait()` Integration with Schedulers**:
   - *Issue*: Calling legacy `task_group::wait()` (or `task_block::wait()`) when P2300 senders were accumulated did not join or sync-wait those senders alongside the legacy latch.
   - *Fix*: Updated `task_group::wait()` to detect if `state_->has_senders()` is true, and if so, safely drain and `sync_wait` the senders via `wait_as_sender()`, catching errors and synchronizing with `latch_.arrive_and_wait()`.
   - *Test*: Added `test_task_group_scheduler_legacy_wait` verifying synchronous joining of scheduler-based tasks through legacy `wait()`.

4. **Recursive Task Draining in P2300**:
   - *Issue*: Dynamic tasks spawned recursively on the same `task_group` during sender execution were orphaned because `senders_` was previously moved once at `wait_as_sender()` call time.
   - *Fix*: Implemented lazy recursive sender draining via `detail::drain_task_group_senders(state)`:
     - `wait_as_sender()` returns an `ex::just() | ex::let_value(...)` chain that starts draining lazily only when connected/started.
     - `drain_task_group_senders` drains the current batch of senders from the thread-safe shared state. If non-empty, it joins them with `ex::when_all_vector(std::move(senders))` and chains into `ex::let_value([state]() { return drain_task_group_senders(state); })`.
     - Recursion continues until no more senders are appended, at which point an empty drain returns `ex::just()`.
     - Fixed `when_all_vector`'s `connect &` overload with `requires(std::is_copy_constructible_v<Sender>)` to properly construct `operation_state` with an rvalue copy of senders.
     - The terminal continuation checks `state->errors_` and re-throws any aggregated exceptions into the P2300 receiver pipeline.
   - *Test*: Added `test_task_group_recursive_spawning` and `test_task_group_recursive_spawning_legacy_wait` verifying nested multi-tier task spawning on the same `task_group`.

5. **Latch Destruction Segfault in P2300 Senders**:
   - *Issue*: In `task_group_shared_state`, `latch_` was initialized to 1 for legacy coordination. Calling `wait_as_sender()` never decremented the latch, resulting in `Assertion 'counter_ == 0' failed: HPX(assertion_failure)` upon `~latch()` destruction in CI.
   - *Fix*: Added an atomic exchange on `has_arrived_` followed by `state->latch_.count_down(1)` in the final `ex::then` continuation of `wait_as_sender()`. Additionally, added an RAII cleanup check in `~task_group_shared_state()` to guarantee `counter_ == 0` even if tasks are cancelled or aborted prior to completion. In `task_group::wait()`, added coordinated waiting if already arrived.

6. **Serialization Drainage Check in `task_group`**:
   - *Issue*: `task_group::serialize` only checked `!state_->latch_.is_ready()`, allowing active P2300 tasks to escape validation if senders had not finished draining.
   - *Fix*: Expanded check to `if (!state_->latch_.is_ready() || !state_->senders_drained_.load(std::memory_order_acquire))` in `libs/core/algorithms/src/task_group.cpp`.

7. **Lvalue Scheduler Forwarding in `task_block.hpp`**:
   - *Issue*: In `detail::define_task_block_impl::operator()`, `f(trh, HPX_FORWARD(Scheduler, sched))` forwarded an rvalue, whereas `is_invocable_v` checked `decltype((sched))` (an lvalue `Scheduler&`). This could cause compilation errors if the user's callable accepted `auto&`.
   - *Fix*: Changed invocation to `f(trh, sched);` to consistently pass an lvalue.

8. **Move-Only Receiver Forwarding in `when_all_vector.hpp`**:
   - *Issue*: In `connect(Receiver&& receiver) &`, `receiver` was passed by copy to `operation_state<Receiver>`, breaking move-only receivers.
   - *Fix*: Wrapped with `HPX_FORWARD(Receiver, receiver)` to properly forward move-only receivers.

---

## CI Compliance Notes
- ASCII-only comments (no em-dashes, smart quotes) verified clean.
- `hpxinspect` clean: no `<iostream>` in headers.
- Copyright year updated to 2026 where files are modified.
- `clang-format` applied per `.clang-format` in repo root.
- Only umbrella module headers (`hpx/modules/<name>.hpp`) are included from outside their own module (per AGENTS.md rule).
- 100% test pass rate across all block unit tests (`spmd_block`, `task_block`, `task_block_executor`, `task_block_par`, `task_group`, `task_block_scheduler`, `run_on_all`).

