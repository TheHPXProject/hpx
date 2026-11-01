..
    Copyright (C) 2014 University of Oregon
    Copyright (c) 2026 The STE||AR-Group
    Copyright (c) 2026 Vansh Dobhal

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _profiling_and_tracing:

========================================
Profiling and tracing |hpx| applications
========================================

|hpx| can send events about its tasks, schedulers, futures and parcels to an
external profiler. The profiler shows these events on a timeline, so you can
see which task ran on which worker thread, when a task was suspended, when it
ran again, and which future started a continuation. This page explains how to
build |hpx| with one of the supported profilers and how to read the data they
show.

For the performance counters that |hpx| collects itself, see
:ref:`performance_counters`.

.. _profiling_backends:

Choosing a backend
==================

|hpx| supports three profilers. They are called backends. Pick exactly one of
them when you configure |hpx| with |cmake|_. If more than one of
:option:`HPX_WITH_TRACY`, :option:`HPX_WITH_ITTNOTIFY` and
:option:`HPX_WITH_APEX` is ``ON``, the configuration step stops with an error.

.. list-table::
   :header-rows: 1

   * * Backend
     * CMake option
     * What it records
     * How to view it
   * * |tracy|_
     * :option:`HPX_WITH_TRACY`
     * Task zones and fibers, task lifecycle, futures and continuations, work
       stealing, parcels, locks
     * The Tracy profiler (Linux, Windows, macOS)
   * * |ittnotify|_
     * :option:`HPX_WITH_ITTNOTIFY`
     * Tasks, task lifecycle, work stealing, markers, locks, performance
       counters
     * |vtune|
   * * |apex|_
     * :option:`HPX_WITH_APEX`
     * Task timers, performance counters, parcels
     * APEX output, |tau|_ and other tools (see the |apex_hpx_doc|_)

If no backend is enabled, all tracing hooks in |hpx| are empty functions.

.. _optimizing_with_tracy:

Tracy
=====

|tracy|_ is a profiler with a graphical viewer. A program built with the Tracy
client sends its events over the network to the viewer, which shows them on a
timeline while the program runs. |hpx| shows each of its tasks to Tracy as a
fiber, so you can follow a task across worker threads.

Building |hpx| with Tracy
-------------------------

Configure |hpx| with :option:`HPX_WITH_TRACY`\ ``=ON``. To get readable task
names, also set ``HPX_WITH_THREAD_DEBUG_INFO=ON`` (see
:ref:`tracy_task_names`):

.. code-block:: shell-session

   $ cmake -S hpx -B build -G Ninja \
       -DCMAKE_BUILD_TYPE=RelWithDebInfo \
       -DHPX_WITH_TRACY=ON \
       -DHPX_WITH_THREAD_DEBUG_INFO=ON
   $ cmake --build build

You do not need to install Tracy first. At configure time, CMake downloads the
Tracy client sources from `github.com/wolfpld/tracy
<https://github.com/wolfpld/tracy>`_ and builds them together with |hpx|. The
option ``HPX_WITH_TRACY_TAG`` selects the Tracy version (default ``v0.14.1``).
For builds without network access, point ``FETCHCONTENT_SOURCE_DIR_TRACY`` at a
local Tracy source tree of the same version, and CMake uses it instead of the
download.

|hpx| builds the Tracy client with these settings:

* ``TRACY_ENABLE``: the client is active. Tracy 0.14 turns it off by default.
* ``TRACY_ON_DEMAND``: the program records events only while a profiler is
  connected (see :ref:`tracy_capturing`).
* ``TRACY_FIBERS``: |hpx| tasks are shown as Tracy fibers.
* ``TRACY_STATIC``: the Tracy client is a static library that is linked into
  the |hpx| core library, also when |hpx| is built as shared libraries.

In Debug builds the Tracy client also prints extra diagnostic messages
(``TRACY_VERBOSE``). If your project already defines a ``tracy::tracy`` target
before |hpx| is configured, |hpx| uses that target and does not apply the
settings above.

On Windows, |hpx| sets ``TRACY_DBGHELP_LOCK=HpxDbgHelp`` on the Tracy client,
so DbgHelp calls from Tracy and from the symbol lookup in |hpx| share a single
mutex.

Getting the Tracy profiler
--------------------------

The profiler (the viewer) is not built with |hpx|. Build it from the same
Tracy version that |hpx| uses (``HPX_WITH_TRACY_TAG``, default ``v0.14.1``).
The network protocol of Tracy can change between releases. If it did, the
profiler cannot connect to the client and reports an incompatible protocol
version.

.. code-block:: shell-session

   $ git clone --branch v0.14.1 https://github.com/wolfpld/tracy.git
   $ cd tracy
   $ cmake -B profiler/build -S profiler -DCMAKE_BUILD_TYPE=Release
   $ cmake --build profiler/build --config Release --parallel
   $ cmake -B capture/build -S capture -DCMAKE_BUILD_TYPE=Release
   $ cmake --build capture/build --config Release --parallel

If you set ``HPX_WITH_TRACY_TAG`` to another tag, clone that tag instead, so
the profiler and |hpx| use the same Tracy version.

This builds ``tracy-profiler``, the graphical viewer, and ``tracy-capture`` and
``tracy-capture-daemon``, two command line tools that record traces to files.
The configure step downloads further libraries, so it needs network access and
``git``. The Tracy manual (``tracy.pdf`` on the `release page
<https://github.com/wolfpld/tracy/releases>`_) lists the libraries that you
need to install first. The release page also has prebuilt profiler binaries.

.. _tracy_task_names:

Naming your tasks
-----------------

Tracy labels each task with its |hpx| thread description. |hpx| stores these
descriptions only when it is built with ``HPX_WITH_THREAD_DEBUG_INFO=ON``.
Without this option every task is called ``<unknown>``. The option also turns
on other per-task debug data (target address, parent reference, thread phase),
which adds work to every task.

When descriptions are stored, a task gets its name from:

* the name given with ``hpx::annotated_function`` (from ``hpx/functional.hpp``)
  when the task is created,
* the name given with ``hpx::execution::experimental::with_annotation`` for
  all tasks created through an executor, for example by a parallel algorithm,
* the name of the action, for tasks that run an action,
* otherwise the description of the task that created it, or ``<unknown>`` if
  there is none.

.. code-block:: c++

   #include <hpx/functional.hpp>
   #include <hpx/future.hpp>

   int compute(int n);

   hpx::future<int> f =
       hpx::async(hpx::annotated_function(&compute, "compute"), 42);

Tasks that run as stackless threads (created with
``hpx::threads::thread_stacksize::nostack``) never suspend. They get no task
zone and no fiber in Tracy. Their lifecycle messages are still recorded.

.. _tracy_capturing:

Capturing a trace
-----------------

#. Start ``tracy-profiler``.
#. Start your application:

   .. code-block:: shell-session

      $ ./bin/work_stealing_smoke --hpx:threads=4

#. In the start window of the profiler, click your program in the list of
   discovered clients. You can also type the address of the machine the
   program runs on (for example ``127.0.0.1``) and click *Connect*.

Because the client runs in on-demand mode, it records events only while a
profiler is connected. Events from before the connection are not part of the
trace. Task zones that started before the connection are not recorded, and
locks that were created before it record no lock events.

To record a short program from its start, type the address and click
*Connect* before you start the program. The profiler keeps trying to connect
until the program is running. Also set ``TRACY_NO_EXIT=1`` for such programs:
the program then does not exit before a profiler has connected.

To record a trace without the graphical viewer, for example on a cluster node,
use ``tracy-capture`` and open the file later in ``tracy-profiler``:

.. code-block:: shell-session

   $ tracy-capture -o trace.tracy -a 127.0.0.1

``-o`` names the output file, ``-a`` gives the address of the program
(``localhost`` if not given) and ``-p`` the port. If the program is not
running yet, ``tracy-capture`` waits for it. Press :kbd:`Ctrl+C` to stop the
capture early.

The Tracy client listens on TCP port 8086. It also announces itself on the
local network with UDP broadcast messages, which the profiler shows as the list
of discovered clients. If the program runs on another machine that the
broadcast does not reach, connect to that machine by address, or forward the
port, for example with ``ssh -L 8086:localhost:8086 <host>``. The environment
variable ``TRACY_PORT`` changes the port the client listens on.

Reading the timeline
--------------------

Worker threads and task zones
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Each OS thread that |hpx| starts appears as a row named after the thread, for
example ``worker-thread#0``. In a run with more than one locality the name
starts with the locality. Each time a task runs on a worker, |hpx| opens a zone
on that row, named with the task description. The value of the zone is the
thread phase, which counts how many times the task has run so far. Without
``HPX_WITH_THREAD_DEBUG_INFO`` the phase is always 0.

The scheduler also opens teal zones named ``hpx::background_work`` while a
worker does background work, such as polling the network. Blue zones mark
points of interest inside |hpx| and come from ``HPX_TRACING_MARK_EVENT``, for
example ``idle_callback`` and ``do_some_work`` in the scheduler,
``fork_join_executor::bulk_sync_execute``, ``do_work_chunk`` in the bulk
executors, and ``send_parcel`` / ``recv_parcel`` / ``parcel_scheduled`` in the
parcel layer.

The zones that |hpx| opens do not record call stacks. For task zones the call
stack would show the same scheduler frames for every task. Messages do record
call stacks (see `Messages`_).

Task fibers
~~~~~~~~~~~

Each task, except stackless (``nostack``) tasks, also runs on a fiber row,
named ``<description>_<id>``, where the id is the address of the |hpx| thread.
You can follow a task on its fiber row when it moves from one worker to
another. The color of a zone on a fiber row depends on the worker it ran on, so
a change of color means that the task moved.

|hpx| reuses thread objects. A later task that reuses the same thread object
appears on the same fiber row, so a fiber row can show more than one task under
one name.

Messages
~~~~~~~~

|hpx| sends short text messages for important events. They appear as marks on
the timeline and in the *Messages* window of the profiler, where you can search
them. On platforms where Tracy can collect call stacks, each message also
records the call stack where it was sent, up to 10 frames deep. In the
messages, ``<id>`` is the address of the |hpx| thread and ``W<n>`` or ``W#<n>``
is the number of a worker thread.

.. list-table::
   :header-rows: 1

   * * Message
     * Color
     * When
   * * ``Task Staged: <description> (Parent: <id>)``
     * grey
     * A task was requested, before its thread object exists.
   * * ``Task Created: <id> - <description> (Parent: <id>)``
     * green
     * The thread object of a task was created.
   * * ``Task Executing (W<n>): <id> - <description>``
     * magenta
     * A task starts or continues running on worker ``<n>``.
   * * ``Task Yielded: <id> - <description>``
     * orange
     * A task gave up its worker but stays ready to run.
   * * ``Task Suspended: <id> - <description> (Reason: <reason>)``
     * red
     * A task waits, for example on a future or a timer.
   * * ``Task Resumed: <id> - <description> (Wake: <reason>)``
     * cyan
     * A suspended task runs again. It is sent from the task itself, after its
       ``Task Executing`` message.
   * * ``Task Completed: <id> - <description>``
     * dark green
     * A task finished.
   * * ``Task Deleted: <id>``
     * purple
     * The thread object of a task was removed.
   * * ``Work Stolen: Thief W#<n> <- Victim W#<m> | Task: <id> (<description>)``
     * amber
     * Worker ``<n>`` took a task from the queue of worker ``<m>``.
   * * ``OS Worker #<n> entering sleep on CV``
     * blue-grey
     * Worker ``<n>`` found no work.

The ``(Parent: <id>)`` part is only present when the parent task is known. The
messages ``Task ...`` and ``Work Stolen`` are controlled by
:option:`HPX_WITH_TRACING_LIFECYCLE_EVENTS`,
:option:`HPX_WITH_TRACING_WORK_STEALING_EVENTS` and the sample rate (see
:ref:`tracy_overhead`).

Futures and continuations
~~~~~~~~~~~~~~~~~~~~~~~~~

These messages connect a future to the continuations it starts. The address in
the message is the address of the shared state of the future, so you can search
for the same address on the producer side and on the consumer side. If a task
is running when the message is sent, the message is also added to the text of
its zone on the fiber row.

.. list-table::
   :header-rows: 1

   * * Message
     * Color
     * When
   * * ``Future Fulfilled: <future>``
     * green
     * A value was stored in a future (producer side).
   * * ``Future Exception Set: <future>``
     * red
     * An exception was stored in a future (producer side).
   * * ``Handle On Completed Fired``
     * yellow
     * The continuations of a ready future are started.
   * * ``Continuation Run: <future>``
     * white
     * A continuation (for example from ``future::then``) starts (consumer
       side).

The plot ``Active Continuations`` shows how many continuations are running.
These events are controlled by :option:`HPX_WITH_TRACING_CAUSAL_EVENTS` and are
never sampled.

Other data
~~~~~~~~~~

* **Frames**: calls to ``hpx::tracing::frame_mark`` in your code split the
  timeline into frames (see :ref:`tracy_own_code`).
* **Locks**: ``hpx::mutex`` and ``hpx::spinlock`` are reported to Tracy as
  locks, which the profiler draws on the timeline. Only locks that were created
  while a profiler was connected record lock events.
* **Application information**: the |hpx| version and build configuration are
  sent to Tracy as application information, which the profiler keeps with the
  trace.

Clicking on a zone opens the zone information window, with the timing of the
zone, its text and the messages sent while it was open.

Profiling distributed runs
--------------------------

Each |hpx| locality is a separate process with its own Tracy client, so you
record one trace per locality. The zones ``send_parcel``, ``recv_parcel`` and
``parcel_scheduled`` always appear. To also see which parcel they belong to,
configure |hpx| with :option:`HPX_WITH_PARCEL_PROFILING`\ ``=ON`` (it is
``OFF`` by default unless |apex| is enabled). |hpx| then gives every parcel an
id that travels with it, and sends these messages:

.. list-table::
   :header-rows: 1

   * * Message
     * Color
     * When
   * * ``Parcel Send: id=<msb>:<lsb> size=<bytes> to=L#<locality>``
     * blue
     * A parcel was sent.
   * * ``Parcel Recv: id=<msb>:<lsb> from=L#<locality>``
     * green
     * A parcel arrived and its header was read.
   * * ``Parcel Scheduled: id=<msb>:<lsb> from=L#<locality> thread=0x<thread>``
     * purple
     * The action of a parcel was put on the scheduler later than the receive,
       for example for zero-copy receives, for messages with more than one
       parcel, or for parcels that were routed through AGAS. ``<thread>`` is
       the parent of the |hpx| thread that sent the parcel, or 0 if |hpx| did
       not record it.

To follow a parcel, search for its id in the *Messages* window of the sending
and of the receiving trace. Always use both halves of the id: the lower half
alone starts again on every locality. ``L#unknown`` means that the locality was
not known yet, for example for parcels sent during startup.

When several localities run on the same machine, every Tracy client needs its
own port. If ``TRACY_PORT`` is not set and port 8086 is taken, a client tries
the next ports (8087, 8088, ... up to 8105). Each locality then appears as its
own entry in the list of discovered clients, with its port. Connect one
profiler window, or one ``tracy-capture`` with ``-p``, to each of them.

``tracy-capture-daemon -o <directory>`` records all clients it finds through
their broadcast messages, each to its own file in ``<directory>``.
``tracy-merge`` (built from the ``merge`` folder of Tracy) combines several
trace files into one. The merged trace keeps zones, messages and plots, but not
call stacks or lock events, and the times of the input traces are not
synchronized. See the Tracy manual for details.

.. _tracy_overhead:

Reducing the overhead
---------------------

When no profiler is connected, most tracing hooks only check whether a
profiler is connected and return. Creating or destroying an ``hpx::mutex`` or
``hpx::spinlock`` still registers it with Tracy. While a profiler is connected,
|hpx| sends a few messages for every task, and each message records a call
stack. For programs with many small tasks this can slow down the run. There are
two ways to reduce it.

Event classes can be compiled out one by one:
:option:`HPX_WITH_TRACING_LIFECYCLE_EVENTS`,
:option:`HPX_WITH_TRACING_CAUSAL_EVENTS` and
:option:`HPX_WITH_TRACING_WORK_STEALING_EVENTS`. All three are ``ON`` by
default. Turning one ``OFF`` makes its hooks empty, so neither the connection
check nor the event itself runs. The arguments at each call site are still
evaluated. The lifecycle and work stealing options apply to the Tracy and ITT
backends, the causal option to the Tracy backend only.

Instead of turning lifecycle events off, you can sample them.
:option:`HPX_WITH_TRACING_SAMPLE_RATE` sets the default rate: with a rate of N,
only one task in N sends lifecycle and work stealing messages. The default is
``1`` (every task). You can set a different rate when you start the program,
with the ``hpx.tracing.sample_rate`` setting, for example
``--hpx:ini=hpx.tracing.sample_rate=10`` (see
:ref:`the hpx.tracing section <ini_hpx_tracing>`). Values
below 1 count as 1. Causal events and ``Task Staged`` are never sampled.
``Task Staged`` is sent before the task exists, so there is nothing to sample
yet.

.. _tracy_own_code:

Instrumenting your own code
---------------------------

Include ``hpx/modules/tracing.hpp`` to use the tracing API in your own code.
The calls compile with every backend. With |apex|, or if no backend is
enabled, they do nothing.

.. code-block:: c++

   #include <hpx/modules/tracing.hpp>

   for (int step = 0; step != num_steps; ++step)
   {
       // starts a new frame named "step" in Tracy
       hpx::tracing::frame_mark("step");

       {
           // a zone named "exchange_halo" until the end of this scope
           HPX_TRACING_MARK_EVENT("exchange_halo");
           exchange_halo();
       }

       compute();
   }

* ``hpx::tracing::frame_mark(name)`` marks a frame boundary. With Tracy, the
  profiler shows the frames above the timeline. With ITT, it emits a marker.
* ``HPX_TRACING_MARK_EVENT(name)`` opens a zone named ``name`` that ends at the
  end of the current scope (Tracy), or an ITT event that starts there and ends
  at the end of the scope (ITT).
* ``HPX_TRACING_PAUSE()`` and ``HPX_TRACING_RESUME()`` pause and resume the
  data collection of an ITT tool. They do nothing with the other backends.

Use a string literal for ``name`` in both calls. Tracy uses the address of the
string to tell frames apart, and for ``HPX_TRACING_MARK_EVENT`` the ITT backend
reads the name only the first time a call site runs.

The examples ``work_stealing_smoke``, ``causal_chain_smoke``,
``causal_exception_smoke`` and ``background_work_smoke`` in
``examples/quickstart`` use this API and describe in their comments what to
look for in Tracy.

.. _profiling_with_itt:

Intel ITT API and VTune
=======================

The |ittnotify|_ is a C API that a program uses to describe its work to an
analysis tool such as |vtune|. When |hpx| is built with
:option:`HPX_WITH_ITTNOTIFY`\ ``=ON``, it reports its tasks and other events
through this API.

Building |hpx| with the ITT API
-------------------------------

To build |hpx|, you need the ITT API header ``ittnotify.h`` and the
``ittnotify`` library. You can build them from the open-source ``ittapi``
project:

.. code-block:: shell-session

   $ git clone https://github.com/intel/ittapi.git
   $ cmake -S ittapi -B ittapi/build -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_INSTALL_PREFIX=$HOME/ittapi-install
   $ cmake --build ittapi/build --target install
   $ cmake -S hpx -B build -G Ninja \
       -DHPX_WITH_ITTNOTIFY=ON \
       -DAmplifier_ROOT=$HOME/ittapi-install
   $ cmake --build build

``Amplifier_ROOT`` (see :ref:`cmake_additional_tools`) tells CMake where to
find the ITT API: CMake looks for ``include/ittnotify.h`` and for the library
in ``lib`` or ``lib64`` below this folder. The name is historical, any
installation of the ITT API works. |vtune| also ships the ITT API. To use that
copy, set ``Amplifier_ROOT`` to the folder of the |vtune| installation that
contains ``include/ittnotify.h``.

The ITT backend always stores thread descriptions, so you do not need
``HPX_WITH_THREAD_DEBUG_INFO`` to get task names. Name your tasks as described
in :ref:`tracy_task_names`.

Enabling the events at runtime
------------------------------

An |hpx| program built with the ITT API sends no events unless you turn them
on at runtime with the ``hpx.use_itt_notify`` setting:

.. code-block:: shell-session

   $ ./bin/my_app --hpx:ini=hpx.use_itt_notify=1

Setting the environment variable ``HPX_HAVE_ITTNOTIFY=1`` has the same effect.
The events go to the collector library of the tool that runs the program, for
example |vtune|:

.. code-block:: shell-session

   $ vtune -collect threading -- ./bin/my_app --hpx:ini=hpx.use_itt_notify=1

``threading`` is one of the analysis types of |vtune|. ``vtune -help collect``
lists all of them.

Without |vtune|, you can check the events with the reference collector of
``ittapi``. It writes the task, thread name, counter creation and pause and
resume calls to a log file. It does not log markers, lock events or counter
values. Configure ``ittapi`` with ``-DITT_API_REFERENCE_COLLECTOR=ON``, build
it again, and run, where ``<ittapi>`` is the folder you cloned:

.. code-block:: shell-session

   $ export INTEL_LIBITTNOTIFY64=<ittapi>/build/bin/libittnotify_refcol.so
   $ export INTEL_LIBITTNOTIFY_LOG_DIR=$PWD/itt_logs
   $ mkdir -p itt_logs
   $ ./bin/my_app --hpx:ini=hpx.use_itt_notify=1

What the ITT backend records
----------------------------

Tasks and markers use the ITT domain ``hpx``.

* **Tasks**: each time a task runs on a worker, |hpx| records an ITT task with
  the task description as its name. Background work is recorded as
  ``hpx::background``.
* **Task lifecycle**: every |hpx| task gets an ITT id. Its ITT task begins when
  it starts or continues running and ends when it yields, suspends or
  completes. A requested task is recorded as a marker. Controlled by
  :option:`HPX_WITH_TRACING_LIFECYCLE_EVENTS`. The sample rate does not apply,
  every task is recorded.
* **Work stealing**: a ``work_stolen`` marker with the metadata ``thief``,
  ``victim`` and ``task``. Controlled by
  :option:`HPX_WITH_TRACING_WORK_STEALING_EVENTS`.
* **Markers**: ``os_thread_sleep`` when a worker finds no work, and a marker
  for each call to ``hpx::tracing::frame_mark``.
* **Events**: ``HPX_TRACING_MARK_EVENT`` records an ITT event that lasts until
  the end of the scope.
* **Locks**: ``hpx::mutex`` and ``hpx::spinlock`` are reported through the ITT
  synchronization API.
* **Counters**: performance counters printed with ``--hpx:print-counter`` are
  reported as ITT counters.
* **Thread names**: the names of the OS threads that |hpx| starts.

The ITT backend does not record futures and continuations, and it does not
record parcel events.

.. _profiling_with_apex:

APEX
====

|hpx| provides integration with |apex|_, which is a framework for application
profiling using task timers and various performance counters
:footcite:t:`2015:apex`. |tau|_ is an optional dependency when using |apex|.

To build |hpx| with |apex|_, add :option:`HPX_WITH_APEX`\ ``=ON`` to your
|cmake|_ configuration, and either:

* set ``HPX_WITH_FETCH_APEX=ON`` to let CMake download |apex| and build it
  together with |hpx|. ``HPX_WITH_APEX_TAG`` selects the version (default
  ``hpx_v1.11.0``) and ``HPX_WITH_APEX_REPOSITORY`` the repository, or
* set ``Apex_ROOT`` to an existing |apex| installation.

Optionally, add ``Tau_ROOT=$PATH_TO_TAU`` as well. With |apex| enabled,
:option:`HPX_WITH_PARCEL_PROFILING` is ``ON`` by default. Please see the
|apex_hpx_doc|_ for detailed instructions on using |apex|_ with |hpx|.

References
==========

.. footbibliography::
