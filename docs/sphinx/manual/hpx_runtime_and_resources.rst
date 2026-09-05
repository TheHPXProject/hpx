..
    Copyright (C) 2020      ETH Zurich
    Copyright (C) 2013      Patricia Grubel
    Copyright (C) 2007-2014 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

===========================
|hpx| runtime and resources
===========================

.. _schedulers:

|hpx| thread scheduling policies
================================

The |hpx| runtime has six thread scheduling policies: local-priority,
static-priority, local, static, local-workrequesting-fifo, and abp-priority. 
These policies can be specified from the command line using the command line
option :option:`--hpx:queuing`. In order to use a particular scheduling policy,
the runtime system must be built with the appropriate scheduler flag turned on
(e.g. ``cmake -DHPX_THREAD_SCHEDULERS=local``, see :ref:`cmake_variables` for
more information).

Priority local scheduling policy (default policy)
-------------------------------------------------

The priority local scheduling policy maintains one queue per operating system
(OS) thread. The OS thread pulls its work from this queue. By default the number
of high priority queues is equal to the number of OS threads; the number of high
priority queues can be specified on the command line using
:option:`--hpx:high-priority-threads`. High priority threads are executed by any
of the OS threads before any other work is executed. When a queue is empty, work
will be taken from high priority queues first. There is one low priority queue
from which threads will be scheduled only when there is no other work.

For this scheduling policy there is an option to turn on NUMA sensitivity using
the command line option :option:`--hpx:numa-sensitive`. When NUMA sensitivity is
turned on, work stealing is done from queues associated with the same NUMA domain
first, only after that work is stolen from other NUMA domains.

This scheduler is enabled at build time by default using the FIFO
(first-in-first-out) queueing policy. This policy can be invoked using
:option:`--hpx:queuing`\ ``local-priority-fifo``. The scheduler can also be
enabled using the LIFO (last-in-first-out) policy. This is not the default
policy and must be invoked using the command line option
:option:`--hpx:queuing`\ ``local-priority-lifo``.

Static priority scheduling policy
---------------------------------

* invoke using: :option:`--hpx:queuing`\ ``static-priority`` (or ``-qs``)

The static scheduling policy maintains one queue per OS thread from which each
OS thread pulls its tasks (user threads). Threads are distributed in a round
robin fashion. There is no thread stealing in this policy.

Local scheduling policy
-----------------------

* invoke using: :option:`--hpx:queuing`\ ``local`` (or ``-ql``)
* flag to turn on for build: ``HPX_THREAD_SCHEDULERS=all`` or
  ``HPX_THREAD_SCHEDULERS=local``

The local scheduling policy maintains one queue per OS thread from which each OS
thread pulls its tasks (user threads).

Static scheduling policy
------------------------

* invoke using: :option:`--hpx:queuing`\ ``static``
* flag to turn on for build: ``HPX_THREAD_SCHEDULERS=all`` or
  ``HPX_THREAD_SCHEDULERS=static``

The static scheduling policy maintains one queue per OS thread from which each
OS thread pulls its tasks (user threads). Threads are distributed in a round
robin fashion. There is no thread stealing in this policy.

Priority ABP scheduling policy
------------------------------

* invoke using: :option:`--hpx:queuing`\ ``abp-priority-fifo``
* flag to turn on for build: ``HPX_THREAD_SCHEDULERS=all`` or
  ``HPX_THREAD_SCHEDULERS=abp-priority``

Priority ABP policy maintains a double ended lock free queue for each OS thread.
By default the number of high priority queues is equal to the number of OS
threads; the number of high priority queues can be specified on the command line
using :option:`--hpx:high-priority-threads`. High priority threads are executed
by the first OS threads before any other work is executed. When a queue is empty
work will be taken from high priority queues first. There is one low priority
queue from which threads will be scheduled only when there is no other work. For
this scheduling policy there is an option to turn on NUMA sensitivity using the
command line option :option:`--hpx:numa-sensitive`. When NUMA sensitivity
is turned on work stealing is done from queues associated with the same NUMA
domain first, only after that work is stolen from other NUMA domains.

This scheduler can be used with two underlying queuing policies (FIFO:
first-in-first-out, and LIFO: last-in-first-out). In order to use the LIFO
policy use the command line option
:option:`--hpx:queuing`\ ``abp-priority-lifo``.

..
    Questions, concerns and notes:

    Are all the work queues FIFO except perhaps the deque ABP?

    What is the low priority thread for priority policies?
    One of the comments says that there are exactly one queue per OS threads
    then an additional number of high-priority-threads queues plus an additional
    low priority queue.

    Is numa-sensitive only for local priority??? I know it says that in the
    documentation and error messages but seems to be available for abp
    priority and periodic priority

    There should be some way of verifying which policy is being used.

    --hpx-high-priority-threads option ********* it seems to me this option
    should be =< number of OS threads but command line accepts any number.
    Okay so I'm confused, in the documentation for command line options it
    states: the number of operating system threads maintaining a high priority
    queue (default: number of OS threads), valid for
    --hpx:queuing=local-priority only examples/spell_check/example_text.txt
    but in hpx_init.cpp the comment states: local scheduler with priority queue
    (one queue for each OS threads plus one separate queue for high priority
    HPX-threads)

    SCHEDULER
    initialization parameters:
    max count per queue (1000) this is for all policies
    number of queues  (OS threads) all except global
    number of high priority queues (selectable on command line  local priority,
    periodic and abp priority policies)
    minimum add thread count (10)  for periodic priority policy the number of
    threads will be incremented in steps of this count

    maximum number of active threads = 1000 is that per queue? I don't understand
    the comment:
    The maximum number of active threads this thread manager should

    // create. This number will be a constraint only as long as the work
    // items queue is not empty. Otherwise the number of active threads
    // will be incremented in steps equal to the \a min_add_new_count
    // specified above.
    enum { max_thread_count = 1000 };

    I see both FIFO and double ended queues in ABP policies?

Work requesting scheduling policies
-----------------------------------

* invoke using: :option:`--hpx:queuing`\ ``local-workrequesting-fifo``,
  using :option:`--hpx:queuing`\ ``local-workrequesting-lifo``,
  or using :option:`--hpx:queuing`\ ``local-workrequesting-mc``

The work-requesting policies rely on a different mechanism of balancing work
between cores (compared to the other policies listed above). Instead of actively
trying to steal work from other cores, requesting work relies on a less
disruptive mechanism. If a core runs out of work, instead of actively looking at
the queues of neighboring cores, in this case a request is posted to another
core. This core now (whenever it is not busy with other work) either responds to
the original core by sending back work or passes the request on to the next
possible core in the system. In general, this scheme avoids contention on the
work queues as those are always accessed by their own cores only.


The |hpx| resource partitioner
==============================

The |hpx| resource partitioner lets you take the execution resources available
on a system---processing units, cores, and numa domains---and assign them to
thread pools. By default |hpx| creates a single thread pool name ``default``.
While this is good for most use cases, the resource partitioner lets you create
multiple thread pools with custom resources and options.

Creating custom thread pools is useful for cases where you have tasks which
absolutely need to run without interference from other tasks. An example of this
is when using |mpi|_ for distribution instead of the built-in mechanisms in
|hpx| (useful in legacy applications). In this case one can create a thread pool
containing a single thread for |mpi| communication. |mpi| tasks will then
always run on the same thread, instead of potentially being stuck in a queue
behind other threads.

Note that |hpx| thread pools are completely independent from each other in the
sense that task stealing will never happen between different thread pools.
However, tasks running on a particular thread pool can schedule tasks on another
thread pool.

.. note::

   It is simpler in some situations to schedule important tasks with high
   priority instead of using a separate thread pool.

.. _using_resource_partitioner:

Using the resource partitioner
------------------------------

The :hpx:class:`hpx::resource::partitioner` is now created during |hpx| runtime
initialization without explicit action needed from the user. To specify some of
the initialization parameters you can use the :hpx:struct:`hpx::init_params`.

.. literalinclude:: ../../libs/full/resource_partitioner/examples/simplest_resource_partitioner_1.cpp
   :start-after: //[body
   :end-before: //body]

The resource partitioner callback is the interface to add thread pools to the
|hpx| runtime and to assign resources to the thread pools.  In order to create
custom thread pools you can specify the resource partitioner callback
:hpx:member:`hpx::init_params::rp_callback` which will be called once the
resource partitioner will be created , see the example below. You can also
specify other parameters, see :hpx:struct:`hpx::init_params`.

To add a thread pool use the
:hpx:member:`hpx::resource::partitioner::create_thread_pool` method. If you
simply want to use the default scheduler and scheduler options, it is enough to
call ``rp.create_thread_pool("my-thread-pool")``.

Then, to add resources to the thread pool you can use the
:hpx:member:`hpx::resource::partitioner::add_resource` method. The resource
partitioner exposes the hardware topology retrieved using |hwloc|_ and lets you
iterate through the topology to add the wanted processing units to the thread
pool. Below is an example of adding all processing units from the first NUMA
domain to a custom thread pool, unless there is only one NUMA domain in which
case we leave the first processing unit for the default thread pool:

.. literalinclude:: ../../libs/full/resource_partitioner/examples/simplest_resource_partitioner_2.cpp
   :start-after: //[body
   :end-before: //body]

.. note::

   Whatever processing units are not assigned to a thread pool by the time
   :hpx:func:`hpx::init` is called will be added to the default thread pool. It
   is also possible to explicitly add processing units to the default thread
   pool, and to create the default thread pool manually (in order to e.g. set
   the scheduler type).

.. tip::

   The command line option :option:`--hpx:print-bind` is useful for checking
   that the thread pools have been set up the way you expect.

Difference between the old and new version
------------------------------------------

In the old version, you had to create an instance of the
:hpx:class:`resource_partitioner` with ``argc`` and ``argv``.

.. code-block:: c++

    int main(int argc, char** argv)
    {
        hpx::resource::partitioner rp(argc, argv);
        hpx::init();
    }

From |hpx| 1.5.0 onwards, you just pass ``argc`` and ``argv`` to ``hpx::init()``
or ``hpx::start()`` for the binding options to be parsed by the resource
partitioner.

.. code-block:: c++

    int main(int argc, char** argv)
    {
        hpx::init_params init_args;
        hpx::init(argc, argv, init_args);
    }

In the old version, when creating a custom thread pool, you just called the
utilities on the resource partitioner instantiated previously.

.. code-block:: c++

    int main(int argc, char** argv)
    {
        hpx::resource::partitioner rp(argc, argv);

        rp.create_thread_pool("my-thread-pool");

        bool one_numa_domain = rp.numa_domains().size() == 1;
        bool skipped_first_pu = false;

        hpx::resource::numa_domain const& d = rp.numa_domains()[0];

        for (const hpx::resource::core& c : d.cores())
        {
            for (const hpx::resource::pu& p : c.pus())
            {
                if (one_numa_domain && !skipped_first_pu)
                {
                    skipped_first_pu = true;
                    continue;
                }

                rp.add_resource(p, "my-thread-pool");
            }
        }

        hpx::init();
    }

You now specify the resource partitioner callback which will tie the resources
to the resource partitioner created during runtime initialization.

.. code-block:: c++

    void init_resource_partitioner_handler(hpx::resource::partitioner& rp)
    {
        rp.create_thread_pool("my-thread-pool");

        bool one_numa_domain = rp.numa_domains().size() == 1;
        bool skipped_first_pu = false;

        hpx::resource::numa_domain const& d = rp.numa_domains()[0];

        for (const hpx::resource::core& c : d.cores())
        {
            for (const hpx::resource::pu& p : c.pus())
            {
                if (one_numa_domain && !skipped_first_pu)
                {
                    skipped_first_pu = true;
                    continue;
                }

                rp.add_resource(p, "my-thread-pool");
            }
        }
    }

    int main(int argc, char* argv[])
    {
        hpx::init_params init_args;
        init_args.rp_callback = &init_resource_partitioner_handler;

        hpx::init(argc, argv, init_args);
    }

Advanced usage
--------------

It is possible to customize the built in schedulers by passing scheduler options
to :hpx:member:`hpx::resource::partitioner::create_thread_pool`. It is also possible
to create and use custom schedulers.

.. note::

   It is not recommended to create your own scheduler. The |hpx| developers use
   this to experiment with new scheduler designs before making them available to
   users via the standard mechanisms of choosing a scheduler (command line
   options). If you would like to experiment with a custom scheduler the
   resource partitioner example ``shared_priority_queue_scheduler.cpp`` contains
   a fully implemented scheduler with logging, etc. to make exploration easier.

To choose a scheduler and custom mode for a thread pool, pass additional options
when creating the thread pool like this::

    rp.create_thread_pool("my-thread-pool",
        hpx::resource::policies::local_priority_lifo,
        hpx::policies::scheduler_mode(
            hpx::policies::scheduler_mode::default |
            hpx::policies::scheduler_mode::enable_elasticity));

The available schedulers are documented here:
:hpx:enum:`hpx::resource::scheduling_policy`, and the available scheduler modes
here: :hpx:enum:`hpx::threads::policies::scheduler_mode`. Also see the examples
folder for examples of advanced resource partitioner usage:
``simple_resource_partitioner.cpp`` and
``oversubscribing_resource_partitioner.cpp``.
