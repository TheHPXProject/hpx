..
    Copyright (c) 2026 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _thread_scheduler_configuration:

=============================================
Configuring and tuning the thread schedulers
=============================================

The |hpx| thread schedulers have a number of settings that control when idle
worker threads look for work elsewhere, how much work they take, when they go
to sleep, and how thread objects are recycled. This section explains how a
worker thread finds work, what each setting influences, which scheduler uses
which setting, and how the settings depend on each other. The individual
settings are listed in :ref:`ini_hpx` and :ref:`ini_hpx_thread_queue`; the
scheduling policies themselves are described in :ref:`schedulers`.

.. important::

   The defaults are chosen to work well for a wide range of applications. Change
   these settings only after measuring, and measure again afterwards. Many of
   the settings interact; changing one of them can move the point at which
   another one takes effect.

All settings can be changed at runtime without recompiling, either on the
command line (for instance ``--hpx:ini=hpx.thread_queue.max_add_new_count=20``)
or in a configuration file. The settings in :ref:`ini_hpx_thread_queue` and most
``hpx.max_*`` settings can also be set through the environment variable shown
next to them, and have a compile time default defined by a preprocessor
constant (for instance ``HPX_THREAD_QUEUE_MAX_ADD_NEW_COUNT``) that can be
overridden when building |hpx|.

Terminology
===========

Worker thread
   An operating system thread that runs |hpx| threads. Each thread pool has one
   worker thread per processing unit assigned to it.

Task (staged task)
   A unit of work that was created, but for which no |hpx| thread object exists
   yet. New work is placed into the *staged queue* of a worker thread as a task
   description. This makes creating work cheap.

Pending thread
   An |hpx| thread object that is ready to run. Staged tasks are *converted*
   into pending threads in batches; threads that were suspended and are woken up
   go directly to the *pending queue*.

Scheduling loop
   The loop each worker thread runs. One *iteration* either runs one |hpx|
   thread or does not find anything to run (an *empty* iteration).

Idle loop count
   The number of consecutive empty iterations of a worker thread. It is reset
   when the worker thread finds work. Most of the settings below take effect
   once a worker thread has been idle for a given number of iterations.

Stealing
   Taking work from another worker thread's queues. Idle workers steal pending
   threads and staged tasks; the workers they steal from are called *victims*.

.. _thread_scheduler_find_work:

How a worker thread finds work
==============================

The following describes the default scheduler (``local-priority-fifo``, see
:ref:`schedulers`). The other schedulers use a subset of these steps; see
:ref:`thread_scheduler_matrix`.

In each iteration of its scheduling loop, a worker thread tries, in this order:

#. its own high priority queue, its bound queue (|hpx| threads that must run on
   this worker thread), and its own normal queue. If any of these has staged
   tasks but no pending threads, the worker converts them first.
#. the pending queues of other workers (stealing pending threads), if stealing
   is enabled (see :ref:`thread_scheduler_pending_steal`). Workers that have a
   high priority queue try each victim's high priority queue before its normal
   queue. A worker only steals normal priority threads from workers that have
   several of them, unless their owner doesn't get to them.
#. the low priority queue (shared by all worker threads of the pool).

If none of these yields an |hpx| thread, the iteration is empty: the idle loop
count is incremented, and the worker converts its own staged tasks into pending
threads (at most ``hpx.thread_queue.max_add_new_count`` of them at a time). If
it has none and has been idle for long enough, it steals staged tasks from
other workers (see :ref:`thread_scheduler_staged_steal`), converting them into
pending threads in its own queue.

As the idle loop count grows, a worker thread goes through the following
phases (default values in parentheses):

.. list-table::
   :header-rows: 1

   * * Empty iterations
     * What the worker thread does
   * * from the first one
     * Steals pending threads, but only from workers that have at least
       ``hpx.thread_queue.min_tasks_to_steal_pending_early`` (4) of them, or
       that don't get to the few they have (see
       :ref:`thread_scheduler_pending_steal`).
   * * after ``hpx.thread_queue.idle_loop_count_steal_staged`` (30)
     * Also steals staged tasks, but only from workers that have at least
       ``hpx.thread_queue.min_tasks_to_steal_staged_early`` (10) of them, or
       that don't get to the few they have (see
       :ref:`thread_scheduler_staged_steal`).
   * * after ``hpx.max_idle_loop_count`` (200000)
     * Goes to sleep for a while (idle backoff, see
       :ref:`thread_scheduler_idle_backoff`), then starts counting again.

Fast idle mode (see :ref:`thread_scheduler_modes`, always on for the
work-requesting schedulers) shortens this: a worker steals staged tasks from the
first empty iteration, and ``hpx.thread_queue.min_tasks_to_steal_pending_early``
and ``hpx.thread_queue.min_tasks_to_steal_staged_early`` do not apply
(``hpx.thread_queue.min_tasks_to_steal_pending`` and
``hpx.thread_queue.min_tasks_to_steal_staged`` still do). The idle loop count
grows by ``hpx.max_idle_loop_count / 1024`` with each iteration that did not
find work to steal, so a worker thread goes to sleep after about 1024 such
iterations.

Every 2000 (``hpx.max_busy_loop_count``) |hpx| threads it runs, and when it is
idle, a worker thread also does background work, such as polling the network
(see :ref:`thread_scheduler_background_work`).

.. _thread_scheduler_pending_steal:

Stealing pending threads
========================

Settings: ``hpx.thread_queue.min_tasks_to_steal_pending``,
``hpx.thread_queue.min_tasks_to_steal_pending_early``.

A worker steals one pending thread at a time from another worker's queue.
``hpx.thread_queue.min_tasks_to_steal_pending`` is the minimum number of
pending threads a victim must have; the default (``0``) allows stealing from
any worker that has a pending thread. The work-requesting schedulers apply the
same minimum when they answer a steal request.

When a worker converts a batch of staged tasks, all workers that are idle at
that moment find the new pending threads and pop them one by one from the same
queue. This contention can slow all of them down so much that they go idle
together again, convert the next batch together, and so on, in lock step. With
the local priority queue scheduler, a worker therefore steals pending threads
only from workers that have at least
``hpx.thread_queue.min_tasks_to_steal_pending_early`` (4) of them. This only
applies to the normal queues, not to high priority threads, and not in fast
idle mode. Set ``hpx.thread_queue.min_tasks_to_steal_pending_early=0`` to turn
this off.

The threads queued behind a long running |hpx| thread still have to be
stolen. A worker that finds a victim's queue short but not empty 16 times in a
row ignores the minimum for that victim: its owner evidently doesn't get to
these threads (finding the queue empty starts over). A worker that was just
woken up because new work arrived (see :ref:`thread_scheduler_idle_backoff`)
ignores the minimum for its first steal.

.. note::

   Setting ``hpx.thread_queue.min_tasks_to_steal_pending`` to a value larger
   than ``1`` is not recommended. It applies all the time, so up to that many
   minus one pending threads queued behind a long running |hpx| thread can not
   be stolen at all until it finishes. If that long running thread waits for
   one of them without suspending, the application hangs.

.. _thread_scheduler_staged_steal:

Stealing staged tasks
=====================

Settings: ``hpx.thread_queue.idle_loop_count_steal_staged``,
``hpx.thread_queue.min_tasks_to_steal_staged_early``,
``hpx.thread_queue.min_tasks_to_steal_staged``, ``hpx.max_idle_loop_count``.

A worker that has no pending threads and no staged tasks of its own steals
staged tasks from other workers and converts them into pending threads in its
own queue (at most ``hpx.thread_queue.max_add_new_count`` of them at a time).
Staged tasks are cheaper to steal than pending threads and they take load off
the victim's pending queue.

With the local priority queue scheduler, a worker starts stealing staged tasks
after ``hpx.thread_queue.idle_loop_count_steal_staged`` (30) empty iterations
(at most ``hpx.max_idle_loop_count / 2``). It only steals from workers that
have at least ``hpx.thread_queue.min_tasks_to_steal_staged_early`` (10) staged
tasks. A worker with only a few staged tasks will convert them soon itself, and
leaving them there keeps them on the worker they were placed on, close to the
data in its caches. If it doesn't get to them, they are stolen anyway: all
thieves count how often they find a victim's few staged tasks in a row (16
looks divided by the number of staged tasks are needed, finding none starts
over), and the thief that completes the count steals a single task, ignoring
the minimum. A worker that was just woken up because new work arrived (see
:ref:`thread_scheduler_idle_backoff`) ignores the minimum for its first steal.
``hpx.thread_queue.min_tasks_to_steal_staged`` (0) always applies. The local
and shared priority schedulers steal staged tasks right away; the
work-requesting schedulers do not steal staged tasks (see
:ref:`thread_scheduler_matrix`).

``hpx.thread_queue.min_tasks_to_steal_staged`` only applies to stealing; a
worker always converts its own staged tasks.

.. _thread_scheduler_conversion:

Converting staged tasks
=======================

Settings: ``hpx.thread_queue.min_add_new_count``,
``hpx.thread_queue.max_add_new_count``, ``hpx.thread_queue.max_thread_count``.

Each conversion turns a batch of staged tasks into pending threads. Its size is
``hpx.thread_queue.max_thread_count`` minus the number of |hpx| threads that
currently exist in that queue, clamped to the range [``min_add_new_count``,
``max_add_new_count``] (both 10 by default). If the queue holds more than
``max_thread_count`` (1000) minus ``min_add_new_count`` threads, nothing is
converted, unless the queue has no pending threads at all: then
``min_add_new_count`` tasks are
converted and the limit of that queue is raised by the same amount, so that
work can always make progress. ``max_thread_count=0`` removes the limit (all
staged tasks are converted at once).

Larger batches let a worker run longer before it has to convert again, and
make stolen work go further. Smaller batches leave more work staged, where
other workers can steal it cheaply. ``min_add_new_count`` is limited to
``max_add_new_count``.

.. _thread_scheduler_idle_backoff:

Idle backoff
============

Settings: ``hpx.max_idle_loop_count``, ``hpx.max_idle_backoff_time``.

After ``hpx.max_idle_loop_count`` (200000) consecutive empty iterations, a
worker thread sleeps until new work arrives for it, but at most for
2\ :sup:`n` microseconds, where n counts the number of times it went to sleep
since it last found work or was woken up by new work, and never longer than ``hpx.max_idle_backoff_time``
(1000 microseconds). Afterwards it starts counting empty iterations from zero
again. This requires ``HPX_WITH_THREAD_MANAGER_IDLE_BACKOFF=ON`` (the default)
and the ``enable_idle_backoff`` scheduler mode (on by default).

Creating new work wakes the sleeping worker threads. A worker thread woken this
way may steal once without the early minima (see
:ref:`thread_scheduler_pending_steal` and :ref:`thread_scheduler_staged_steal`),
so that it finds the new work even if it was placed on a worker that has a few
tasks only.

Lowering ``hpx.max_idle_loop_count`` makes idle worker threads go to sleep
sooner, which saves energy and leaves the processing units to other processes,
but it also makes them wake up later when new work arrives.

.. _thread_scheduler_background_work:

Background work
===============

Settings: ``hpx.max_busy_loop_count``, ``hpx.max_background_threads``.

Worker threads with an index (within their thread pool) smaller than
``hpx.max_background_threads`` (default: all of them) do background work, such
as sending and receiving parcels, if the ``do_background_work`` scheduler mode
is set (the default). They do it when they are idle, and every
``hpx.max_busy_loop_count`` (2000) |hpx| threads they ran. The limit applies to
each thread pool separately. Thread pools created with their own background
work function use all of their worker threads for it. Without networking, at
most one worker thread per pool does the runtime's background work.

.. _thread_scheduler_recycling:

Thread object management
========================

Settings: ``hpx.thread_queue.init_threads_count``,
``hpx.thread_queue.cached_threads_count``,
``hpx.thread_queue.max_terminated_threads``,
``hpx.thread_queue.min_delete_count``, ``hpx.thread_queue.max_delete_count``.

|hpx| thread objects (and their stacks) are expensive to create, so they are
reused:

* Each queue creates ``hpx.thread_queue.init_threads_count`` (10) thread objects
  with small stacks when its worker thread starts.
* Terminated threads are collected and cleaned up in batches. Once a queue holds
  more than ``hpx.thread_queue.min_delete_count`` (10) terminated threads, a
  batch of one tenth of them, but at least ``min_delete_count`` and at most
  ``hpx.thread_queue.max_delete_count`` (1000) is cleaned up. More than
  ``hpx.thread_queue.max_terminated_threads`` (100) terminated threads are
  cleaned up completely.
* Cleaned up thread objects are kept for reuse, up to
  ``hpx.thread_queue.cached_threads_count`` (1000) per stack size and queue;
  the rest are deallocated.

Larger values use more memory, but reduce allocations in applications that
create many short |hpx| threads. ``min_delete_count`` is limited to
``max_delete_count``.

.. _thread_scheduler_numa:

NUMA domains
============

Settings: ``hpx.numa_sensitive`` (:option:`--hpx:numa-sensitive`).

Idle workers of the local priority queue scheduler steal from the workers on
the same core first, then from the other workers in the same NUMA domain. The
local scheduler also steals within its own NUMA domain first. Whether and how
they steal across NUMA domains depends on ``hpx.numa_sensitive``:

* ``0``: all workers may steal from the other NUMA domains, the nearest ones
  first.
* ``1`` (the default, also the value of a bare :option:`--hpx:numa-sensitive`):
  only the first worker of each NUMA domain steals from other NUMA domains,
  and only from the nearest ones.
* ``2``: work is never stolen across NUMA domains.

The shared priority scheduler only distinguishes ``2`` (no stealing across NUMA
domains) from the other values. The work-requesting schedulers ignore this
setting.

.. _thread_scheduler_modes:

Scheduler modes
===============

Scheduler modes are flags that switch features of a scheduler on or off for a
thread pool. They can be set through the resource partitioner
(:hpx:member:`hpx::resource::partitioner::create_thread_pool`), through
``hpx.default_scheduler_mode`` (a number, the bitwise or of the values below,
used for the default pool), and at runtime with
``hpx::threads::add_scheduler_mode``, ``hpx::threads::remove_scheduler_mode``,
and ``hpx::threads::set_scheduler_mode``. See
:hpx:enum:`hpx::threads::policies::scheduler_mode`. Some schedulers enforce
modes: the work-requesting schedulers always keep ``fast_idle_mode``,
``enable_stealing`` and ``enable_stealing_numa`` set, and the static schedulers
always keep ``enable_stealing`` and ``enable_stealing_numa`` cleared. Changing
these modes has no effect there.

.. list-table::
   :header-rows: 1

   * * Mode
     * Default
     * Effect
   * * ``do_background_work`` (``0x1``)
     * on
     * Worker threads do background work (see
       :ref:`thread_scheduler_background_work`). A worker thread reads this
       mode when it starts; changing it later does not start or stop its
       background work.
   * * ``reduce_thread_priority`` (``0x2``)
     * on
     * Worker threads lower their operating system thread priority when they
       start.
   * * ``delay_exit`` (``0x4``)
     * on
     * During shutdown, worker threads keep looking for work for a while before
       they exit.
   * * ``fast_idle_mode`` (``0x8``)
     * off (always on for the work-requesting schedulers)
     * Idle workers steal staged tasks right away, without the early minima,
       and go to sleep sooner (see :ref:`thread_scheduler_find_work`).
   * * ``enable_elasticity`` (``0x10``)
     * off
     * New work is not placed on suspended worker threads. Required for
       suspending and resuming worker threads of a pool.
   * * ``enable_stealing`` (``0x20``)
     * on
     * Idle workers steal work from other workers (the local scheduler always
       steals).
   * * ``enable_stealing_numa`` (``0x40``)
     * on
     * Idle workers also steal across NUMA domains; cleared if
       ``hpx.numa_sensitive`` is ``2`` (see :ref:`thread_scheduler_numa`).
   * * ``assign_work_round_robin`` (``0x80``), ``assign_work_thread_parent``
       (``0x100``), ``steal_high_priority_first`` (``0x200``),
       ``steal_after_local`` (``0x400``)
     * on, off, off, on
     * Used by the shared priority scheduler only: place new work round robin
       or on the creating worker thread, and the order in which it steals.
   * * ``enable_idle_backoff`` (``0x800``)
     * on
     * Idle worker threads sleep (see :ref:`thread_scheduler_idle_backoff`).
   * * ``do_background_work_only`` (``0x1000``)
     * off
     * The static scheduler only does background work (used by some
       parcelports for their own thread pools).

.. _thread_scheduler_matrix:

Which scheduler uses which setting
==================================

.. list-table::
   :header-rows: 1

   * * Setting
     * local-priority, abp-priority
     * local
     * local-workrequesting
     * static, static-priority
     * shared-priority
   * * ``min_tasks_to_steal_pending``
     * yes
     * yes
     * yes
     * no (no stealing)
     * no
   * * ``min_tasks_to_steal_pending_early``
     * yes
     * no
     * no
     * no
     * no
   * * ``min_tasks_to_steal_staged``
     * yes
     * yes
     * no
     * no
     * no
   * * ``idle_loop_count_steal_staged``, ``min_tasks_to_steal_staged_early``
     * yes
     * no (steals staged tasks right away)
     * no (does not steal staged tasks)
     * no
     * no
   * * ``max_thread_count``, ``min_add_new_count``, ``max_add_new_count``
     * yes
     * yes
     * yes
     * yes
     * no
   * * ``init_threads_count``, ``cached_threads_count``
     * yes
     * yes
     * yes
     * yes
     * no
   * * ``min_delete_count``, ``max_delete_count``, ``max_terminated_threads``
     * yes
     * yes
     * yes
     * yes
     * yes (different batch sizes)
   * * ``hpx.max_idle_loop_count``, ``hpx.max_busy_loop_count``,
       ``hpx.max_idle_backoff_time``, ``hpx.max_background_threads``
     * yes
     * yes
     * yes
     * yes
     * yes
   * * ``hpx.numa_sensitive``
     * yes
     * yes
     * no
     * no (no stealing)
     * partly (``2`` or not)

Dependencies between settings
=============================

* ``hpx.thread_queue.idle_loop_count_steal_staged`` is limited to
  ``hpx.max_idle_loop_count / 2``.
* ``hpx.thread_queue.min_tasks_to_steal_staged_early`` and
  ``hpx.thread_queue.min_tasks_to_steal_staged`` both apply; the larger of the
  two counts. When the thieves find that a victim doesn't get to its few
  staged tasks, or a worker was just woken up, only
  ``min_tasks_to_steal_staged`` applies.
* Likewise, ``hpx.thread_queue.min_tasks_to_steal_pending_early`` and
  ``hpx.thread_queue.min_tasks_to_steal_pending`` both apply, and only the
  former is lifted for a victim that doesn't get to its few pending threads.
* Fast idle mode lifts all the early stealing restrictions and makes workers
  reach ``hpx.max_idle_loop_count`` (and go to sleep) after about 1024 empty
  iterations.
* ``hpx.max_idle_loop_count`` determines when worker threads go to sleep, and
  limits ``hpx.thread_queue.idle_loop_count_steal_staged``.
* The conversion batch size is limited by ``hpx.thread_queue.max_thread_count``
  as well as by ``min_add_new_count`` and ``max_add_new_count``;
  ``min_add_new_count`` is limited to ``max_add_new_count``.
* ``hpx.thread_queue.min_delete_count`` is limited to
  ``hpx.thread_queue.max_delete_count``.

Tuning hints
============

The following table lists symptoms and the settings to look at first. The
performance counters show what the scheduler is doing (see
:ref:`performance_counters`), for instance ``/threads/idle-rate`` and, if |hpx|
was built with ``HPX_WITH_THREAD_STEALING_COUNTS=ON``,
``/threads/count/stolen-from-pending`` and
``/threads/count/stolen-from-staged``.

.. list-table::
   :header-rows: 1

   * * Symptom
     * Settings to look at
   * * Many short tasks created on a few worker threads, the other workers are
       idle or the throughput varies a lot between runs.
     * ``min_tasks_to_steal_pending_early`` (contention on the creating
       worker's queue), ``idle_loop_count_steal_staged``, ``max_add_new_count``
       (larger batches per steal).
   * * Work placed on specific worker threads (for instance with an executor
       that uses thread placement hints) ends up on other workers, cache
       locality suffers.
     * ``min_tasks_to_steal_staged_early`` (raise it),
       ``idle_loop_count_steal_staged`` (raise it).
   * * Threads woken up while one worker runs a long task are delayed.
     * ``min_tasks_to_steal_pending`` (keep it at 0),
       ``min_tasks_to_steal_pending_early`` (lower it).
   * * Idle worker threads use too much CPU time.
     * ``hpx.max_idle_loop_count`` (lower it), ``hpx.max_idle_backoff_time``
       (raise it), fast idle mode.
   * * Idle worker threads react slowly to new work.
     * ``hpx.max_idle_backoff_time`` (lower it), ``hpx.max_idle_loop_count``
       (raise it).
   * * Applications creating very many short |hpx| threads spend a lot of time
       allocating thread objects.
     * ``cached_threads_count``, ``init_threads_count``,
       ``max_terminated_threads``.
