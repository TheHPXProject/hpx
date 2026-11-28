..
    Copyright (c) 2025 Dimitra Karatza

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _executors:

=========
Executors
=========

Executors in |hpx| provide a flexible way to control **how, when, and where**
tasks are executed. Instead of manually creating threads or managing thread
pools, you can hand your tasks to an executor, and it takes care of the details
of running them.

This page introduces the concept of executors, the main types available in |hpx|,
and how to create custom executors.

.. _executors_intro:

What is an executor?
====================

An **executor** is an abstraction that separates the *what* from the *how* of
task execution:

* **What**: the work to be performed (e.g. a function or task).
* **How**: whether the task runs synchronously, asynchronously, or in parallel
  across multiple cores.

By using executors, you can switch between execution strategies without
rewriting your algorithms. |hpx| provides a rich set of executors with a unified
API.

.. _executors_main:

Main Executors in |hpx|
=======================

|hpx| provides multiple executors. Below are some of the most commonly used:

Parallel Executor
-----------------

* Default in |hpx|.
* Creates a new HPX thread for every scheduled task.
* Works well for large tasks, but frequent small tasks incur overhead due to
  thread creation/destruction.

.. code-block:: c++

    #include <hpx/execution.hpp>
    #include <hpx/algorithm.hpp>
    #include <vector>

    std::vector<int> data(100, 1);

    hpx::for_each(
        hpx::execution::par,
        data.begin(), data.end(),
        [](int &x) {
            x += 1;
        }
    );

Fork-Join Executor
------------------

* Spawns one thread per CPU core when created.
* Threads are reused across tasks, avoiding repeated creation costs.
* Efficient for workloads with many sequential parallel regions.
* May waste resources if parallel work is limited.

.. code-block:: c++

    #include <hpx/execution.hpp>
    #include <hpx/algorithm.hpp>
    #include <vector>

    std::vector<int> data(100, 1);

    hpx::execution::experimental::fork_join_executor exec;

    hpx::for_each(
        hpx::execution::par.on(exec),
        data.begin(), data.end(),
        [](int &x) {
            x += 1;
        }
    );

Suspending Fork-Join Executor
-----------------------------

* Does the same job as the fork-join executor, but its waiting threads spin
  only for a short time (the spin duration, 20 microseconds by default) and
  then suspend until the next parallel region starts or the current one has
  finished.
* Other work can run on the cores of an idle executor, and idle cores can go
  to sleep.
* Constexpr default constructible; the worker threads are created on first
  use. Copies made on an HPX thread share the worker threads of the original.
* The calling thread takes part in the parallel regions if it runs on one of
  the executor's PUs; otherwise it suspends until the region has finished.
* A region wakes at most one worker thread per index, so a loop that the
  executor parameters (for instance ``task_size_chunk_size``) give few cores
  and chunks wakes only that many. ``sync_execute`` runs a single task on the
  calling thread; the parallel algorithms use it for a loop that gets one
  core and one chunk.

.. code-block:: c++

    #include <hpx/execution.hpp>
    #include <hpx/algorithm.hpp>

    std::vector<int> data(100, 1);

    hpx::execution::experimental::suspending_fork_join_executor exec;

    hpx::for_each(
        hpx::execution::par.on(exec),
        data.begin(), data.end(),
        [](int &x) {
            x += 1;
        }
    );

A ``suspending_fork_join_executor`` can't be used from several threads at the
same time or from inside one of its own parallel regions (nested
parallelism). ``shared_suspending_fork_join_executor`` can:

* All ``shared_suspending_fork_join_executor`` objects with the same
  configuration (priority, stack size, loop schedule and spin duration) share
  one set of worker threads per thread pool. It is created when an |hpx|
  thread of the pool uses such an executor first, and stopped when the
  runtime starts to shut down.
* A parallel region runs on these worker threads if they are free. If they are
  busy, because they run a region for another thread or because the region is
  started from inside one of their regions, the region runs through a
  ``parallel_executor`` instead: one task for each worker thread of the pool,
  each with a contiguous range of the indices. The same happens if the calling
  thread is not an |hpx| thread, and once the worker threads were stopped.
* Exceptions are reported by whichever executor ran the region: the first
  exception from the worker threads, or an ``hpx::exception_list`` from the
  ``parallel_executor``.
* As with ``suspending_fork_join_executor``, asynchronous calls
  (``bulk_async_execute``, ``async_invoke``) run the region before they
  return a ready future.
* Single tasks (``sync_execute``, ``async_execute``, ``then_execute``,
  ``post``) and continuations (``bulk_then_execute``) run as a
  ``parallel_executor`` runs them.
* ``with_processing_units_count`` and ``with_first_core`` limit the regions
  to the given number of worker threads, starting at the given one; the
  other worker threads are not woken. ``with_priority`` and
  ``with_stacksize`` select the shared worker threads with that priority or
  stack size.
* The properties (also ``with_hint`` and ``with_annotation``) apply as well
  to the ``parallel_executor`` that runs the single tasks, the
  continuations and the regions that fall back, and that a task policy uses
  (see below). The worker threads get the priority ``bound`` if the
  priority is ``default_`` (as it is by default), so that they stay on their
  cores, while the tasks of the ``parallel_executor`` can be stolen.
* A hint with ``thread_sharing_hint::do_not_combine_tasks`` says that the
  iterations depend on each other: each iteration then runs as a task of its
  own on the ``parallel_executor``. The algorithms that need a task for each
  of their parts (the uninitialized algorithms, ``spmd_block``) use the
  ``parallel_executor`` right away.
* Copies never own worker threads, so the executor can be a ``constexpr``
  variable. Its configuration can be serialized, for instance as the
  executor of a policy sent to another locality.

If |hpx| is configured with
``HPX_WITH_SUSPENDING_FORK_JOIN_EXECUTOR_AS_DEFAULT=ON`` (default: ``OFF``),
``shared_suspending_fork_join_executor`` is the executor of the parallel
execution policies ``par`` and ``par_unseq``, instead of
``parallel_executor``. The task policies ``par(task)`` and
``par_unseq(task)`` keep using ``parallel_executor``: they run an algorithm
asynchronously, which the worker threads of a fork-join executor can't do.
For the same reason, mapping a policy with a
``shared_suspending_fork_join_executor`` to a task policy (``to_task``, as in
``par(task)`` or ``par.on(exec)(task)``) gives it a ``parallel_executor``.

.. code-block:: c++

    #include <hpx/execution.hpp>
    #include <hpx/algorithm.hpp>

    std::vector<int> data(100, 1);

    // can be used by any number of threads, also from inside the loop body
    constexpr hpx::execution::experimental::shared_suspending_fork_join_executor
        exec;

    hpx::for_each(
        hpx::execution::par.on(exec),
        data.begin(), data.end(),
        [](int &x) {
            x += 1;
        }
    );

Sequential Executor
-------------------

* Executes all tasks synchronously on the calling thread.
* Useful for debugging (deterministic execution) or when parallelism brings no
  benefit.
* Maintains the same executor-based API while running tasks sequentially.

.. code-block:: c++

    #include <hpx/execution.hpp>
    #include <hpx/algorithm.hpp>
    #include <vector>

    std::vector<int> data(100, 1);

    hpx::execution::sequenced_executor seq_exec;

    hpx::for_each(
        hpx::execution::seq.on(seq_exec),
        data.begin(), data.end(),
        [](int &x) { x += 1; }
    );

.. _executors_real_world:

Executors in real-world applications
====================================

A practical example is the **LULESH** mini-application (a hydrodynamics
benchmark).

.. _parallel_executor:

.. figure:: ../_static/images/lulesh_parallel.png
   :width: 45%
   :align: left

   Parallel executor: creates and destroys threads for each loop.

.. _fork_join_executor:

.. figure:: ../_static/images/lulesh_fork_join.png
   :width: 45%
   :align: right

   Fork-join executor: reuses threads across loops.

* With the **parallel executor**, each parallel loop creates and destroys new
  threads, introducing overhead.
* With the **fork-join executor**, threads are created once and reused across
  loops, reducing overhead and improving performance.
* For bulk leaf regions that must not suspend, construct
  ``fork_join_executor`` with ``thread_stacksize::nostack`` (bodies must not
  ``async`` / ``future::get`` / ``this_thread::suspend``; see
  `#3348 <https://github.com/TheHPXProject/hpx/issues/3348>`_).

In studies, the fork-join executor achieved significant speedups, in some cases
more than twice as fast as traditional OpenMP implementations.

.. _executor_performance_comparison:

.. figure:: ../_static/images/lulesh_speedup_comparison.png
   :width: 80%
   :align: center

   Performance comparison between executors in the LULESH benchmark.

.. _executors_custom:

Custom executors
================

While |hpx| provides a variety of built-in executors, you may sometimes need to
adapt task execution to your own requirements. This is where **custom
executors** come in. By writing a small wrapper around an existing executor, you
can extend its behavior?for example, to add logging, profiling information, or
special scheduling rules?while still taking advantage of the |hpx| executor API.

Custom annotating executor
--------------------------

The following example shows how to implement a simple executor that annotates
tasks with a string label for easier debugging and profiling.

.. note::

    Annotations do not affect how tasks run or what results they produce. Their
    main purpose is to give human-readable names to tasks so that they can be
    identified in profilers, and debuggers.

Full example code
^^^^^^^^^^^^^^^^^

.. code-block:: c++

    #include <hpx/hpx_main.hpp>
    #include <hpx/include/parallel_executors.hpp>
    #include <hpx/include/async.hpp>
    #include <hpx/execution.hpp>
    #include <hpx/modules/async_base.hpp>
    #include <hpx/modules/threading_base.hpp>

    #include <iostream>
    #include <string>
    #include <utility>

    template <typename BaseExecutor>
    struct simple_annotating_executor
    {
        BaseExecutor base_;
        const char* annotation_;

        simple_annotating_executor(BaseExecutor exec, const char* ann)
            : base_(std::move(exec)), annotation_(ann)
        {}

        // Non-blocking one-way executor
        template <typename F, typename... Ts>
        friend void tag_invoke(hpx::parallel::execution::post_t,
                            simple_annotating_executor const& exec,
                            F&& f, Ts&&... ts)
        {
            hpx::post(
                hpx::annotated_function(std::forward<F>(f), exec.annotation_),
                std::forward<Ts>(ts)...);
        }

        // Synchronous execution
        template <typename F, typename... Ts>
        friend auto tag_invoke(hpx::parallel::execution::sync_execute_t,
                            simple_annotating_executor const& exec,
                            F&& f, Ts&&... ts)
        {
            return hpx::parallel::execution::sync_execute(
                exec.base_,
                hpx::annotated_function(std::forward<F>(f), exec.annotation_),
                std::forward<Ts>(ts)...);
        }
    };

    // Example functions
    int compute_square(int x)
    {
        std::cout << "[sync_execute] Running task with annotation\n";
        return x * x;
    }

    void say_hello()
    {
        std::cout << "[post] Running task with annotation\n";
    }

    int main()
    {
        simple_annotating_executor exec(hpx::execution::parallel_executor{}, "my_custom_task");

        // Synchronous execution
        int result = hpx::parallel::execution::sync_execute(exec, &compute_square, 7);
        std::cout << "Result from sync_execute: " << result << "\n";

        // Post a task
        hpx::parallel::execution::post(exec, &say_hello);

        return 0;
    }

Explanation
^^^^^^^^^^^

The first lines pull in the necessary HPX headers for executors, asynchronous
execution, and annotated functions. The key one here is
`hpx/threading_base/annotated_function.hpp`, which provides the facility to tag
tasks with a string label. We then define a simple_annotating_executor
that wraps another executor and associates an annotation string with every task:

.. code-block:: c++

    template <typename BaseExecutor>
    struct simple_annotating_executor
    {
        BaseExecutor base_;
        const char* annotation_;

        simple_annotating_executor(BaseExecutor exec, const char* ann)
            : base_(std::move(exec)), annotation_(ann)
        {}
    };

The post customization schedules a task to run asynchronously. We wrap the
task in `hpx::annotated_function` so that it carries the annotation.
Executors in HPX customize these operations through `tag_invoke` overloads,
which are selected by special tag objects like `post_t`` and `sync_execute_t`.
This is why the executor interface may look different from a normal member
function API.

.. code-block:: c++

    template <typename F, typename... Ts>
    friend void tag_invoke(hpx::parallel::execution::post_t,
                        simple_annotating_executor const& exec,
                        F&& f, Ts&&... ts)
    {
        hpx::post(
            hpx::annotated_function(std::forward<F>(f), exec.annotation_),
            std::forward<Ts>(ts)...);
    }

The `sync_execute` customization runs a task immediately and returns the
result. Again, we wrap the function with an annotation before executing.
The key difference is that `post` schedules a task in a fire-and-forget style
(no result is returned), while `sync_execute` blocks until the task finishes
and gives you the result back.

.. code-block:: c++

    template <typename F, typename... Ts>
    friend auto tag_invoke(hpx::parallel::execution::sync_execute_t,
                        simple_annotating_executor const& exec,
                        F&& f, Ts&&... ts)
    {
        return hpx::parallel::execution::sync_execute(
            exec.base_,
            hpx::annotated_function(std::forward<F>(f), exec.annotation_),
            std::forward<Ts>(ts)...);
    }

Note how we delegate the actual execution to `exec.base_`, the underlying
executor. This makes the custom executor lightweight: it only adds annotations,
while leaving the scheduling strategy to the base executor (here, a
`parallel_executor`).

We define two simple functions to demonstrate both synchronous and asynchronous
execution:
These functions also print to `std::cout`, but this output is not the actual
annotation. Annotations are stored internally by HPX and become visible when you
use debugging or profiling tools.

.. code-block:: c++

    int compute_square(int x)
    {
        std::cout << "[sync_execute] Running task with annotation\n";
        return x * x;
    }

    void say_hello()
    {
        std::cout << "[post] Running task with annotation\n";
    }

Finally, in main we create the executor with a base executor and annotation
string. We then run one task with sync_execute (blocking, returns result)
and one with post (asynchronous, fire-and-forget).
You can also create multiple annotating executors with different strings, so
each task gets its own label. This is especially useful in larger applications
with many different kinds of tasks, where annotations make it much easier to
trace what is happening.

.. code-block:: c++

    int main()
    {
        simple_annotating_executor exec(hpx::execution::parallel_executor{}, "my_custom_task");

        // Synchronous execution
        int result = hpx::parallel::execution::sync_execute(exec, &compute_square, 7);
        std::cout << "Result from sync_execute: " << result << "\n";

        // Post a task
        hpx::parallel::execution::post(exec, &say_hello);

        return 0;
    }

Custom annotating executor with parallel algorithms
---------------------------------------------------

The following example demonstrates how to use a custom annotating executor with
|hpx| parallel algorithms, such as `for_each`.
This allows you to attach annotations to tasks while executing them in parallel.

Full example code
^^^^^^^^^^^^^^^^^

.. code-block:: c++

    #include <hpx/execution.hpp>
    #include <hpx/hpx_main.hpp>
    #include <hpx/include/parallel_algorithm.hpp>
    #include <hpx/include/parallel_executors.hpp>
    #include <hpx/modules/threading_base.hpp>

    #include <iostream>
    #include <utility>
    #include <vector>

    template <typename BaseExecutor>
    struct simple_annotating_executor
    {
        BaseExecutor base_;
        char const* annotation_;

        using execution_category =
            hpx::traits::executor_execution_category_t<BaseExecutor>;

        simple_annotating_executor(BaseExecutor exec, char const* ann)
        : base_(std::move(exec))
        , annotation_(ann)
        {
        }

        // Bulk async_execute (used by parallel algorithms)
        template <typename F, typename Shape, typename... Ts>
        friend auto tag_invoke(hpx::parallel::execution::bulk_async_execute_t,
            simple_annotating_executor const& exec, F&& f, Shape const& shape,
            Ts&&... ts)
        {
            return hpx::parallel::execution::bulk_async_execute(
                exec.base_,
                hpx::annotated_function(std::forward<F>(f), exec.annotation_),
                shape, std::forward<Ts>(ts)...);
        }
    };

    namespace hpx::execution::experimental {

        // The annotating executor exposes the same executor categories as its
        // underlying (wrapped) executor.

        template <typename BaseExecutor>
        struct is_two_way_executor<simple_annotating_executor<BaseExecutor>>
        : is_two_way_executor<BaseExecutor>
        {
        };
    }    // namespace hpx::execution::experimental

    int main()
    {
        using base_executor = hpx::execution::parallel_executor;
        simple_annotating_executor exec(base_executor{}, "for_each_task");

        std::vector<int> data = {1, 2, 3, 4, 5};

        // Use the custom executor with a parallel algorithm
        hpx::for_each(hpx::execution::par.on(exec),    // attach executor
            data.begin(), data.end(), [](int& x) {
                std::cout << "Processing " << x << " on thread "
                        << hpx::get_worker_thread_num() << "\n";
                x *= x;
            });

        std::cout << "Squared values: ";
        for (int v : data)
            std::cout << v << " ";
        std::cout << "\n";

        return 0;
    }

Explanation
^^^^^^^^^^^

Similar as before, the first lines pull in the necessary HPX headers for executors,
asynchronous execution, and annotated functions. The key one here is
`hpx/threading_base/annotated_function.hpp`, which provides the facility to tag
tasks with a string label. We then define a simple_annotating_executor
that wraps another executor and associates an annotation string with every task:

.. code-block:: c++

    template <typename BaseExecutor>
    struct simple_annotating_executor
    {
        BaseExecutor base_;
        const char* annotation_;

        using execution_category =
            hpx::traits::executor_execution_category_t<BaseExecutor>;

        simple_annotating_executor(BaseExecutor exec, const char* ann)
            : base_(std::move(exec)), annotation_(ann)
        {}
    };

Note that we expose the execution category of the custom executor with
`using execution_category = hpx::traits::executor_execution_category_t<BaseExecutor>`.
We inherit the execution category from the underlying executor (`BaseExecutor``), which
ensures that our `simple_annotating_executor` behaves like the base executor in terms of
parallelism and task execution capabilities.

The `bulk_async_execute` customization schedules a set of tasks to run asynchronously.
We wrap each task in `hpx::annotated_function` so that it carries the annotation.
Executors in |hpx| customize these operations through `tag_invoke` overloads, which are
selected by special tag objects like `bulk_async_execute_t`. This is why the executor
interface may look different from a normal member function API - it uses tag dispatch
to integrate seamlessly with the |hpx| parallel algorithms infrastructure.

.. code-block:: c++

    template <typename F, typename Shape, typename... Ts>
    friend auto tag_invoke(hpx::parallel::execution::bulk_async_execute_t,
        simple_annotating_executor const& exec, F&& f, Shape const& shape,
        Ts&&... ts)
    {
        return hpx::parallel::execution::bulk_async_execute(
            exec.base_,
            hpx::annotated_function(std::forward<F>(f), exec.annotation_),
            shape, std::forward<Ts>(ts)...);
    }

Note how we delegate the actual execution to `exec.base_`, the underlying
executor. This makes the custom executor lightweight: it only adds annotations,
while leaving the scheduling strategy to the base executor (here, a
`parallel_executor`).

The `hpx::execution::experimental` namespace contains traits that describe executor capabilities,
such as whether an executor can run tasks one-way, two-way, or never-blocking.
These traits are used internally by |hpx| to verify that an executor is compatible with
a given parallel algorithm or execution policy.

.. code-block:: c++

    namespace hpx::execution::experimental {

        // The annotating executor exposes the same executor categories as its
        // underlying (wrapped) executor.

        template <typename BaseExecutor>
        struct is_never_blocking_one_way_executor<
            simple_annotating_executor<BaseExecutor>>
        : is_never_blocking_one_way_executor<BaseExecutor>
        {
        };

        template <typename BaseExecutor>
        struct is_one_way_executor<simple_annotating_executor<BaseExecutor>>
        : is_one_way_executor<BaseExecutor>
        {
        };

        template <typename BaseExecutor>
        struct is_two_way_executor<simple_annotating_executor<BaseExecutor>>
        : is_two_way_executor<BaseExecutor>
        {
        };
    }

* `is_never_blocking_one_way_executor` indicates whether the executor can schedule tasks in a fire-and-forget style without blocking.
* `is_one_way_executor` indicates support for one-way execution (tasks can be scheduled but no result is returned).
* `is_two_way_executor` indicates support for two-way execution (tasks return a result or a future).

In all cases, the custom executor inherits the capabilities of the base executor, so it integrates seamlessly with |hpx| algorithms.

This design ensures that `simple_annotating_executor` can be used anywhere its underlying executor could be used, while still
adding the annotation functionality. It keeps the custom executor lightweight and fully compatible with the parallel algorithms
infrastructure.

In main, we create the executor with a base executor and an annotation string,
and then use it with a parallel algorithm:

.. code-block:: c++

    int main()
    {
        using base_executor = hpx::execution::parallel_executor;
        simple_annotating_executor exec(base_executor{}, "for_each_task");

        std::vector<int> data = {1, 2, 3, 4, 5};

        // Use the custom executor with a parallel algorithm
        hpx::for_each(hpx::execution::par.on(exec),    // attach executor
            data.begin(), data.end(), [](int& x) {
                std::cout << "Processing " << x << " on thread "
                        << hpx::get_worker_thread_num() << "\n";
                x *= x;
            });

        std::cout << "Squared values: ";
        for (int v : data)
            std::cout << v << " ";
        std::cout << "\n";

        return 0;
    }

First, we create a base executor (`parallel_executor`) and wrap it in our `simple_annotating_executor`,
providing an annotation string "for_each_task". This custom executor will attach the annotation to every
task it schedules, while delegating actual execution to the base executor.

We then use hpx::for_each with a parallel execution policy and attach our custom executor using `par.on(exec)`:
* hpx::execution::par.on(exec) attaches our custom executor to the algorithm.
* for_each internally partitions the work across threads and schedules each task using `bulk_async_execute`.
* Each task is annotated with "for_each_task", visible in debuggers and profilers.
* The results of the parallel computation are stored in the data vector, demonstrating that the algorithm executed successfully in parallel.

This pattern is especially useful in larger applications with many tasks, as annotations make it much easier
to trace and debug the execution of parallel algorithms.

.. _task_sizes:

Task sizes
==========

A task can tell the scheduler how long it is expected to run. The size is a
property of the function a task runs, so it is attached to the function:
``hpx::with_task_size<Size>(f)`` and its shorthands ``hpx::nano_task(f)``,
``hpx::small_task(f)``, ``hpx::medium_task(f)`` and ``hpx::large_task(f)``
return a function that carries one of the sizes of
``hpx::task_size``. ``hpx::traits::get_task_size_v<F>`` reads it from
the type of a function, and is ``task_size::unknown`` for a function without a
size. ``hpx::traits::get_task_size<F>::call(f)`` reads it from the function
object ``f``, which also finds the size of a function stored in an
``hpx::function`` or an ``hpx::move_only_function``, known at run time only.

If the compiler supports C++26 reflection and annotations, a class type can
also give its size with an annotation:

.. code-block:: c++

   struct [[=hpx::task_size::large]] solver_step
   {
       void operator()() const;
   };

   hpx::post(solver_step{});    // runs as a large task

A size given with ``hpx::with_task_size`` wins over the annotation. For a task
posted directly, only an annotation on a class type is read: a function
pointer stays ``task_size::unknown`` even if the function is annotated, and so
does a lambda. An annotation on a function is read by the actions listed
below.
Annotations are not inherited: a class derived from an annotated class has no
size unless it is annotated itself. Two different sizes on one type are a
compile-time error when the size of that type is read, for example when it is
posted.

.. note::

   As a rule of thumb, a nano task runs for about a nanosecond, a small task
   for about 100 nanoseconds, a medium task for about 10 microseconds,
   and a large task for about a millisecond or more: each size stands for a
   hundred times the run time of the one below it, and
   ``hpx::task_size_duration(size)`` returns it as a ``std::chrono``
   duration. Nano is mostly meant for loop bodies, for instance comparing or
   adding builtin types; a task that runs only one of them is never worth a
   thread of its own. A task that calls
   a function many times has a size that follows from the function's size, a
   hundred nano calls make a small task, a hundred small calls a medium one
   and a hundred medium calls a large one;
   ``hpx::bulk_task_size(size, calls)`` computes it. The sizes stand for these
   run times so that the scheduler can make use of them: a size that only
   ordered tasks relative to each other could not be compared with anything
   the scheduler knows, while a known run time can be compared with its own
   costs, for instance the cost of stealing a task (on the order of a
   microsecond) or of waking a worker. This allows decisions like not
   stealing small tasks, or waking a worker only for a large task or for a
   backlog of smaller ones.

.. code-block:: c++

   // a long running task
   hpx::future<double> r = hpx::async(hpx::large_task(&simulate), input);

   // a short continuation
   auto next = r.then(hpx::small_task([](auto&& r) { return r.get() * 2; }));

The size reaches the scheduler with the thread that runs the function:
``hpx::post``, ``hpx::async``, ``.then()``, dataflow, ``hpx::thread``,
``hpx::run_as_hpx_thread`` and the ``post``,
``sync_execute``, ``async_execute`` and ``then_execute`` of
``hpx::execution::parallel_executor`` (and of the executors built on it)
store it in the ``thread_schedule_hint`` of the thread they create for the
function.

The tasks of bulk operations run the function many times, so their size is
derived from the function's size with ``hpx::bulk_task_size``:
``hpx::execution::parallel_executor`` and the bulk operations of
``hpx::execution::experimental::thread_pool_scheduler`` (which the scheduler
executors use as well) give each of their worker tasks the size of the share
of the elements it runs. For ``bulk_chunked``, which passes a range of
elements to each call, the function's size is still the size of its work for
one element, not for a whole range. Hierarchical spawning
(``hpx::execution::parallel_executor_spawn_hierarchically``) creates a task
for each element instead: these tasks carry the function's size, and the
tasks that only spawn them are small. The parallel algorithms run their loop
body in chunks, and the function they give to the executor has the size of
a chunk, derived from the size of the loop body and the number of iterations
in a chunk. A loop body without a size gives tasks without a size.

The size of the loop body can also decide how a parallel algorithm divides
its iterations: with the executor parameters
``hpx::execution::experimental::task_size_chunk_size``, the run time of one
iteration is the one the loop body's size stands for, and the loop runs on as
many cores, in as many chunks, as that run time is worth. A short loop of
nano or small iterations then runs as a single chunk on the calling thread
instead of being spread over all cores:

.. code-block:: c++

   hpx::for_each(hpx::execution::par.with(
                     hpx::execution::experimental::task_size_chunk_size()),
       v.begin(), v.end(), hpx::nano_task([](int& i) { ++i; }));

By default, ``task_size_chunk_size`` makes chunks of at least 2.5
microseconds and gives each core 8 of them. Values given to its
constructor replace these defaults for one loop, the configuration entries
``hpx.task_size_chunk_size.min_chunk_time_us`` and
``hpx.task_size_chunk_size.chunks_per_core`` replace them for all loops (see
:ref:`ini_hpx_task_size_chunk_size`). The benchmark
``task_size_chunk_size_sweep_test`` measures loops of different sizes with
many pairs of these values and prints the pair that suits a machine best, in
the form of these configuration entries.

The default minimal run time suits ``parallel_executor``, which starts a task
and a future for each chunk. An executor that starts chunks with less
overhead can ask for a minimal run time of its own
(``hpx::execution::experimental::get_min_chunk_time``), which replaces the
default for its loops (a value given to the constructor still wins). The
executors of |hpx| ask for the value configured with
``hpx.task_size_chunk_size.<executor>.min_chunk_time_us``, or for one number
of worker threads with
``hpx.task_size_chunk_size.<executor>.<pool size>.min_chunk_time_us``, where
``<executor>`` is ``parallel`` for ``parallel_executor``, ``fork_join`` for
``fork_join_executor``, ``suspending_fork_join`` for
``suspending_fork_join_executor`` and ``shared_suspending_fork_join`` for
``shared_suspending_fork_join_executor``:

.. code-block:: ini

   [hpx.task_size_chunk_size.fork_join]
   min_chunk_time_us = 0.5

   [hpx.task_size_chunk_size.fork_join.32]
   min_chunk_time_us = 0.8

Without such an entry, the executor's loops use the default. These entries
are not predefined, so on the command line (``--hpx:ini``) they need a ``!``
after the key, for instance
``--hpx:ini=hpx.task_size_chunk_size.fork_join.min_chunk_time_us!=0.5``.

If |hpx| is configured with ``HPX_WITH_TASK_SIZE_DEFAULT_PARAMETERS=ON``, the
default executor parameters (``default_parameters``, used by ``par`` and by
the executors unless other parameters are given) are derived from
``task_size_chunk_size``. A loop whose body has a task size is then chunked as
described here without passing ``task_size_chunk_size`` explicitly, all other
loops are chunked as before.

To tune these values for a machine, build and run the benchmark. It is built
when |hpx| is configured with ``HPX_WITH_TESTS=ON`` and
``HPX_WITH_TESTS_BENCHMARKS=ON`` (the default):

.. code-block:: shell-session

   $ cmake --build . --target task_size_chunk_size_sweep_test
   $ ./bin/task_size_chunk_size_sweep_test

It uses all cores by default, as ``--hpx:threads`` does, and takes about half
a minute. It prints a line of comma separated values for each loop and pair of
values, then a summary for each pair, and ends with the two configuration
entries of the pair closest to the best values for each loop, for instance:

.. code-block:: ini

   hpx.task_size_chunk_size.min_chunk_time_us=2.5
   hpx.task_size_chunk_size.chunks_per_core=8

These lines can be added to an ini file (see :ref:`ini_format`), or passed
with ``--hpx:ini``. ``--max_work_us`` limits the longest loop (in
microseconds of work, 10000 by default), ``--time_per_case_ms`` the time each
loop is repeated for (300 by default), and ``--min_reps`` and ``--max_reps``
the number of repetitions of each loop (5 and 300).

A size given to an executor, a launch policy or a scheduler through their
``thread_schedule_hint`` has no effect: the hint of a thread created for a
function always gets that function's size, which is ``task_size::unknown`` if
the function has none. The exceptions are
``hpx::execution::experimental::scheduler_executor`` and
``hpx::execution::experimental::explicit_scheduler_executor``: they create
their threads through a scheduler before they see the function, so these
threads have ``task_size::unknown`` even if the function has a size.

Wrappers forward the size of the function they wrap. The ones |hpx| provides
(``hpx::bind``, ``hpx::bind_front``, ``hpx::bind_back``,
``hpx::util::deferred_call``, ``hpx::annotated_function`` and the wrappers
the parallel algorithms create around their loop bodies, for instance) do. A
wrapper of your own can do the same by specializing
``hpx::traits::get_task_size``, the same way wrappers forward annotations with
``hpx::traits::get_function_annotation``. It derives from the specialization
for the wrapped function, which gives it ``value``, and forwards ``call``:

.. code-block:: c++

   template <typename F>
   struct my_wrapper
   {
       F f;

       template <typename... Ts>
       decltype(auto) operator()(Ts&&... ts)
       {
           return f(std::forward<Ts>(ts)...);
       }
   };

   template <typename F, hpx::task_size Size>
   struct hpx::traits::get_task_size<my_wrapper<F>, Size>
     : hpx::traits::get_task_size<F, Size>
   {
       static constexpr hpx::task_size call(
           my_wrapper<F> const& w) noexcept
       {
           return hpx::traits::get_task_size<F, Size>::call(w.f);
       }
   };

An action binds a single function, so its size is the size of that function.
``HPX_ACTION_HAS_TASK_SIZE(action, size)`` gives an action a size, and an
action that binds a function given a size with ``hpx::with_task_size`` has
that size unless ``HPX_ACTION_HAS_TASK_SIZE`` overrides it. The size is known
from the type of the action on every locality, so it is not sent along with
the action.

Like ``HPX_ACTION_HAS_PRIORITY``, ``HPX_ACTION_HAS_TASK_SIZE`` has to be
visible wherever the action is registered or invoked, as each locality reads
the size when it creates the thread for the action. Put it next to
``HPX_REGISTER_ACTION_DECLARATION``, in a header. A macro that is visible in
some source files only leaves the action without a size in the others, which
the compiler does not diagnose.

.. code-block:: c++

   // in a header
   int simulate_step(int);
   HPX_DEFINE_PLAIN_ACTION(simulate_step, simulate_step_action);
   HPX_REGISTER_ACTION_DECLARATION(simulate_step_action)
   HPX_ACTION_HAS_TASK_SIZE(simulate_step_action, hpx::task_size::large)

   int get_value();
   struct get_value_action
     : hpx::actions::make_action_t<decltype(hpx::small_task(&get_value)),
           hpx::small_task(&get_value), get_value_action>
   {
   };
   HPX_REGISTER_ACTION_DECLARATION(get_value_action)

   // in one source file
   HPX_REGISTER_ACTION(simulate_step_action)
   HPX_REGISTER_ACTION(get_value_action)

If the compiler supports C++26 reflection and annotations, an annotation on
the function gives its action a size as well:

.. code-block:: c++

   [[=hpx::task_size::small_]] void update_counter(int);
   HPX_PLAIN_ACTION(update_counter, update_counter_action)

   hpx::post<update_counter_action>(locality, 1);    // runs as a small task

This works for the actions of ``HPX_PLAIN_ACTION``,
``HPX_DEFINE_PLAIN_ACTION``, ``HPX_DEFINE_COMPONENT_ACTION`` and
``HPX_COMPONENT_ACTION``, for ``hpx::async<^^f>`` and the other functions that
take a reflected function, and for actions made with
``hpx::actions::make_action_t`` from a function or member function pointer,
which includes ``hpx::async<&f>`` and ``hpx::async<&C::f>``.
``HPX_ACTION_HAS_TASK_SIZE`` and ``hpx::with_task_size`` win over the
annotation. ``task_size::unknown`` counts as no annotation. Two different
sizes on one function are a compile-time error when the size of an action that
binds it is read. An action made from a pointer to a specialization of a member
function template does not read annotations. Give an explicit specialization of
an annotated free function template the same annotation as the template. With
Clang it also gets the annotation of the template, so a different size is a
compile-time error. Each source file reads the annotation from the declarations
it sees, so put it on the declaration in the header.

A plain function can also be passed to ``hpx::async`` and ``hpx::post`` as a
template argument. It then runs locally, as with ``hpx::async(&f, ...)``, and
its size comes from ``hpx::traits::task_size_of`` instead of a wrapper:

.. code-block:: c++

   void update_counter(int);

   template <>
   struct hpx::traits::task_size_of<&update_counter>
   {
       static constexpr hpx::task_size value = hpx::task_size::small_;
   };

   hpx::post<&update_counter>(1);    // runs as a small task
   hpx::future<void> f = hpx::async<&update_counter>(2);

A launch policy or an executor can come first, as for ``hpx::async``:
``hpx::async<&f>(hpx::launch::async, ...)``, ``hpx::async<&f>(exec, ...)`` and
``hpx::post<&f>(exec, ...)``. An overloaded function is picked with
``static_cast``, for instance ``hpx::async<static_cast<int (*)(int)>(&f)>(1)``.
If the compiler supports C++26 reflection and annotations, a function that
``task_size_of`` is not specialized for has the size of its annotation. Put the
specialization next to the declaration of the function, so that every source
file that calls it sees it.

A first argument that is a launch policy, an executor, an ``hpx::id_type``, a
distribution policy or a client is never passed to ``f``. With an
``hpx::id_type`` or a distribution policy, ``hpx::async<&f>`` calls ``f`` as an
action on that target (see :ref:`action_type_definition`). The action gets its
size as other actions do, ``task_size_of`` is used for local calls only.
``hpx::post<&f>`` with one of these, and either of them with a client, does not
compile. After a launch policy, an ``hpx::id_type`` or a distribution policy
also makes ``hpx::async<&f>`` call ``f`` as an action. After an executor, every
argument is passed to ``f``, a remote target too.

.. _parallel_scheduler:

P2079 parallel scheduler
========================

C++26 (`P2079 <https://wg21.link/p2079>`_)
exposes ``std::execution::get_parallel_scheduler()``. |hpx| implements this as
``hpx::execution::experimental::get_parallel_scheduler()``.

Earlier drafts of the same paper (P2079R2) used the names ``system_context`` and
``system_scheduler``. Those public types were dropped. The default backend in
|hpx| reuses the default thread pool's worker threads instead of spawning a
separate thread pool for the scheduler.

.. literalinclude:: ../../libs/core/executors/examples/parallel_scheduler.cpp
   :language: c++
   :start-after: //[get_parallel_scheduler_default
   :end-before: //get_parallel_scheduler_default]

To bind the scheduler to a named HPX pool created with the resource
partitioner, pass that pool to ``get_parallel_scheduler``. There is no
need to write a custom ``parallel_scheduler_backend`` class:

.. literalinclude:: ../../libs/core/executors/examples/parallel_scheduler.cpp
   :language: c++
   :start-after: //[get_parallel_scheduler_named_pool
   :end-before: //get_parallel_scheduler_named_pool]

``get_thread_pool`` throws ``hpx::exception`` for an unknown pool name.
The pool must outlive the scheduler; HPX does not extend the pool's
lifetime or diagnose a dangling pool.

Replacing the default backend
-----------------------------

Most applications should use the default scheduler or the named-pool overload
above. A replacement backend is intended for integrations that need
``get_parallel_scheduler()`` to submit work to another execution service, or
that need custom resource management, scheduling, or instrumentation that an
HPX thread pool does not provide.

A replacement derives from ``parallel_scheduler_backend`` and implements
``schedule``, ``schedule_bulk_chunked``, and ``schedule_bulk_unchunked``. Each
function receives a type-erased receiver and preallocated scratch storage. The
backend schedules the requested work and eventually completes the receiver
exactly once with ``set_value``, ``set_error``, or ``set_stopped``. A bulk
backend calls the receiver's ``execute`` member for the assigned index ranges
before delivering the terminal completion.

Install an existing backend object with ``set_parallel_scheduler_backend``:

.. code-block:: c++

   namespace ex = hpx::execution::experimental;

   auto previous = ex::query_parallel_scheduler_backend();
   ex::set_parallel_scheduler_backend(
       std::make_shared<my_parallel_scheduler_backend>());

   auto scheduler = ex::get_parallel_scheduler();

   // Restore the process-wide backend after all operations using it finish.
   ex::set_parallel_scheduler_backend(std::move(previous));

The replacement is process-wide and affects subsequent calls to
``get_parallel_scheduler()``. Schedulers obtained earlier retain shared
ownership of their original backend. Do not replace a backend while operations
using it are in flight.

Libraries that select a backend during initialization can instead call
``set_parallel_scheduler_backend_factory`` before the first
``get_parallel_scheduler()`` or ``query_parallel_scheduler_backend()`` call.
The factory is invoked lazily and its backend is shared by subsequent default
schedulers. Changing the factory after that backend has been created does not
replace the active backend; use ``set_parallel_scheduler_backend`` for an
immediate replacement.

Replacement backends receive a ``parallel_scheduler_receiver_proxy`` for each
operation. Its ``try_query<P>(query)`` member exposes supported properties from
the connected receiver's environment. In particular,
``try_query<inplace_stop_token>(get_stop_token)`` returns the receiver's stop
token when its environment provides that type. Unsupported query and result
type combinations return ``std::nullopt``.
