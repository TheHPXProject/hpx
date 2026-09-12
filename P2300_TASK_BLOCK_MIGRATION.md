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

## CI Compliance Notes
- ASCII-only comments (no em-dashes, smart quotes) verified clean.
- `hpxinspect` clean: no `<iostream>` in headers.
- Copyright year updated to 2026 where files are modified.
- `clang-format` applied per `.clang-format` in repo root.
- Only umbrella module headers (`hpx/modules/<name>.hpp`) are included from outside their own module (per AGENTS.md rule).

