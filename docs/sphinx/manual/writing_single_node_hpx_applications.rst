..
    Copyright (C) 2012 Bryce Adelstein-Lelbach
    Copyright (C) 2007-2016 Hartmut Kaiser
    Copyright (C) 2023 Dimitra Karatza

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _writing_single_node_applications:

================================
Writing single-node applications
================================

Being a C++ Standard Library for Concurrency and Parallelism, |hpx| implements all
of the corresponding facilities as defined by the C++ Standard but also those which
are proposed as part of the ongoing C++ standardization process. This section focuses
on the features available in |hpx| for parallel and concurrent computation on a single
node, although many of the features presented here are also implemented to work in the
distributed case.

.. _synchronization_objects:

Synchronization objects
=======================

The following objects are providing synchronization for |hpx| applications:

#. :ref:`barrier`
#. :ref:`condition_variable`
#. :ref:`latch`
#. :ref:`mutex`
#. :ref:`shared_mutex`
#. :ref:`semaphore`
#. :ref:`guards`

.. _barrier:

Barrier
-------

:ref:`Barriers <public_api_header_hpx_barrier>` are used for synchronizing multiple threads.
They provide a synchronization point, where all threads must wait until they have all reached
the barrier, before they can continue execution. This allows multiple threads to work together
to solve a common task, and ensures that no thread starts working on the next task until all
threads have completed the current task. This ensures that all threads are in the same state
before performing any further operations, leading to a more consistent and accurate computation.

Unlike latches, barriers are reusable: once the participating threads are released from a
barrier's synchronization point, they can re-use the same barrier. It is thus useful for
managing repeated tasks, or phases of a larger task, that are handled by multiple threads.
The code below shows how barriers can be used to synchronize two threads:

.. literalinclude:: ../../examples/quickstart/barrier_docs.cpp
   :language: c++
   :start-after: //[barrier_docs
   :end-before: //]

In this example, two ``hpx::future`` objects are created, each representing a separate thread of
execution. The ``wait`` function of the ``hpx::barrier`` object is called by each thread. The
threads will wait at the barrier until both have reached it. Once both threads have reached
the barrier, they can continue with their next task.

.. _condition_variable:

Condition variable
------------------

A :ref:`condition variable <public_api_header_hpx_condition_variable>` is a synchronization primitive
in |hpx| that allows a thread to wait for a specific condition to be satisfied before continuing
execution. It is typically used in conjunction with a mutex or a lock to protect shared data that is
being modified by multiple threads. Hence, it blocks one or more threads until another thread both
modifies a shared variable (the condition) and notifies the ``condition_variable``. The code below
shows how two threads modifying the shared variable ``data`` can be synchronized using the
``condition_variable``:

.. literalinclude:: ../../examples/quickstart/condition_variable_docs.cpp
   :language: c++
   :start-after: //[condition_variable_docs
   :end-before: //]

The main thread of the code above starts by creating a worker thread and preparing the shared variable ``data``.
Once the data is ready, the main thread acquires a lock on the mutex ``m`` using ``std::lock_guard<hpx::mutex> lk(m)``
and sets the ready flag to true, then signals the worker thread to start processing by calling ``cv.notify_one()``.
The ``cv.wait()`` call in the main thread then blocks until the worker thread signals that processing is
complete by setting the ``processed`` flag.

The worker thread starts by acquiring a lock on the mutex ``m`` to ensure exclusive access to the shared data.
The ``cv.wait()`` call blocks the thread until the ``ready`` flag is set by the main thread. Once this is
true, the worker thread accesses the shared data resource, processes it, and sets the ``processed`` flag
to indicate completion. The mutex is then unlocked using ``lk.unlock()`` and the ``cv.notify_one()`` call
signals the main thread to resume execution. Finally, the new ``data`` is printed by the main thread to the
console.

.. _latch:

Latch
-----

A :ref:`latch <public_api_header_hpx_latch>` is a downward counter which can be used to synchronize threads.
The value of the counter is initialized on creation. Threads may block on the latch until the counter is
decremented to zero. There is no possibility to increase or reset the counter, which makes the latch a
single-use barrier.

In |hpx|, a latch is implemented as a counting semaphore, which can be initialized with a specific count
value and decremented each time a thread reaches the latch. When the count value reaches zero, all
waiting threads are unblocked and allowed to continue execution. The code below shows how latch can
be used to synchronize 16 threads:

.. literalinclude:: ../../examples/quickstart/latch_local.cpp
   :language: c++
   :start-after: //[latch_docs
   :end-before: //]

In the above code, the ``hpx_main`` function creates a latch object ``l`` with a count of ``num_threads + 1``
and ``num_threads`` number of threads using ``hpx::async``. These threads call the ``wait_for_latch``
function and pass the reference to the latch object. In the ``wait_for_latch`` function, the thread calls the
``arrive_and_wait`` method on the latch, which decrements the count of the latch and causes the thread to wait
until the count reaches zero. Finally, the main thread waits for all the threads to arrive at the latch by
calling the ``arrive_and_wait`` method and then waits for all the threads to finish by calling the
``hpx::wait_all`` method.

.. _mutex:

Mutex
-----

A :ref:`mutex <public_api_header_hpx_mutex>` (short for "mutual exclusion") is a synchronization primitive in
|hpx| used to control access to a shared resource, ensuring that only one thread can access it at a time. A
mutex is used to protect data structures from race conditions and other synchronization-related issues. When
a thread acquires a mutex, other threads that try to access the same resource will be blocked until the mutex
is released. The code below shows the basic use of mutexes:

.. literalinclude:: ../../examples/quickstart/mutex_docs.cpp
   :language: c++
   :start-after: //[mutex_docs
   :end-before: //]

In this example, two |hpx| threads created using ``hpx::async`` are acquiring a ``hpx::mutex m``.
``std::scoped_lock sl(m)`` is used to take ownership of the given mutex ``m``. When control leaves
the scope in which the ``scoped_lock`` object was created, the ``scoped_lock`` is destructed and
the mutex is released.

.. attention::

  A common way to acquire and release mutexes is by using the function ``m.lock()`` before accessing
  the shared resource, and ``m.unlock()`` called after the access is complete. However, these functions
  may lead to deadlocks in case of exception(s). That is, if an exception happens when the mutex is locked
  then the code that unlocks the mutex will never be executed, the lock will remain held by the thread
  that acquired it, and other threads will be unable to access the shared resource. This can cause a
  deadlock if the other threads are also waiting to acquire the same lock. For this reason, we suggest
  you use ``std::scoped_lock``, which prevents this issue by releasing the lock when control leaves the
  scope in which the ``scoped_lock`` object was created.

.. _shared_mutex:

Shared mutex
------------

A :ref:`shared mutex <public_api_header_hpx_shared_mutex>` is a synchronization primitive that can be used
to protect shared data from being simultaneously accessed by multiple threads. In contrast to other mutex
types which facilitate exclusive access, a ``shared_mutex`` has two levels of access:

* `Exclusive access` prevents any other thread from acquiring the mutex, just as with the normal mutex.
  It does not matter if the other thread tries to acquire shared or exclusive access.
* `Shared access` allows multiple threads to acquire the mutex, but all of them only in shared mode.
  Exclusive access is not granted until all of the previous shared holders have returned the mutex
  (typically, as long as an exclusive request is waiting, new shared ones are queued to be granted after
  the exclusive access).

Shared mutexes are especially useful when shared data can be safely read by any number of threads
simultaneously, but a thread may only write the same data when no other thread is reading or writing
at the same time. A typical scenario is a database: The data can be read simultaneously by different
threads with no problem. However, modification of the database is critical: if some threads read data
while another one is writing,  the threads reading may receive inconsistent data. Hence, while a thread
is writing, reading should not be allowed. After writing is complete, reads can occur simultaneously again.
The code below shows how ``shared_mutex`` can be used to synchronize reads and writes:

.. literalinclude:: ../../examples/quickstart/shared_mutex.cpp
   :language: c++
   :start-after: //[shared_mutex_docs
   :end-before: //]

The above code creates ``writers`` and ``readers`` threads, each of which will perform ``cycles`` of operations.
Both the writer and reader threads use the ``hpx::shared_mutex`` object ``stm`` to synchronize access to a shared
resource.

* For the writer threads, a ``unique_lock`` on the shared mutex is acquired before each write operation and is released
  after control leaves the scope in which the ``unique_lock`` object was created.
* For the reader threads, a ``shared_lock`` on the shared mutex is acquired before each read operation and is released
  after control leaves the scope in which the ``shared_lock`` object was created.

Before each operation, both the reader and writer threads sleep for a random time period, which is generated using
a random number generator. The random time period simulates the processing time of the operation.

.. _semaphore:

Semaphore
---------

:ref:`Semaphores <public_api_header_hpx_semaphore>` are a synchronization mechanism used to control concurrent
access to a shared resource. The two types of semaphores are:

* counting semaphore: it has a counter that is bigger than zero. The counter is initialized in the constructor.
  Acquiring the semaphore decreases the counter and releasing the semaphore increases the counter. If a thread
  tries to acquire the semaphore when the counter is zero, the thread will block until another thread increments
  the counter by releasing the semaphore. Unlike ``hpx::mutex``, an ``hpx::counting_semaphore`` is not bound to a
  thread, which means that the acquire and release call of a semaphore can happen on different threads.
* binary semaphore: it is an alias for a ``hpx::counting_semaphore<1>``. In this case, the least maximal value
  is 1. ``hpx::binary_semaphore`` can be used to implement locks.

.. literalinclude:: ../../examples/quickstart/counting_semaphore_docs.cpp
   :language: c++
   :start-after: //[counting_semaphore_docs
   :end-before: //]

In this example, the counting semaphore is initialized to the value of 3. This means that up to 3 threads can
access the critical section (the section of code inside ``the worker()`` function) at the same time. When a thread
enters the critical section, it acquires the semaphore, which decrements the count, while when it exits the
critical section, it releases the semaphore, incrementing thus the count. The ``worker()`` function simulates a
critical section by acquiring the semaphore, sleeping for 1 second and then releasing the semaphore.

In the main function, 5 worker threads are created and started, each trying to enter the critical section.
If the count of the semaphore is already 0, a worker will wait until another worker releases the semaphore
(increasing its value).

.. _guards:

Composable guards
-----------------

Composable guards operate in a manner similar to locks, but are applied only to
asynchronous functions. The guard (or guards) is automatically locked at the
beginning of a specified task and automatically unlocked at the end. Because
guards are never added to an existing task's execution context, the calling of
guards is freely composable and can never deadlock.

To call an application with a single guard, simply declare the guard and call
``run_guarded()`` with a function ``(task)``::

     hpx::lcos::local::guard gu;
     run_guarded(gu,task);

If a single method needs to run with multiple guards, use a guard set::

     std::shared_ptr<hpx::lcos::local::guard> gu1(new hpx::lcos::local::guard());
     std::shared_ptr<hpx::lcos::local::guard> gu2(new hpx::lcos::local::guard());
     gs.add(*gu1);
     gs.add(*gu2);
     run_guarded(gs,task);

Guards use two atomic operations (which are not called repeatedly) to manage
what they do, so overhead should be extremely low.

.. _execution_control:

Execution control
=================

The following objects are providing control of the execution in |hpx| applications:

#. :ref:`future`
#. :ref:`channel`
#. :ref:`task_block`
#. :ref:`task_group`
#. :ref:`thread`

.. _future:

Futures
-------

:ref:`Futures <public_api_header_hpx_future>` are a mechanism to represent the result of a
potentially asynchronous operation. A future is a type that represents a value that will
become available at some point in the future, and it can be used to write asynchronous and
parallel code. Futures can be returned from functions that perform time-consuming operations,
allowing the calling code to continue executing while the function performs its work. The
value of the future is set when the operation completes and can be accessed later. Futures
are used in |hpx| to write asynchronous and parallel code. Below is an example demonstrating
different features of futures:

.. literalinclude:: ../../libs/full/include/tests/unit/api_future.cpp
   :language: c++
   :lines: 7-

The first section of the main function demonstrates how to use futures for asynchronous execution.
The first two lines create two futures, one for void and another for an integer, using the
``hpx::async()`` function. These futures are executed *asynchronously* in separate threads using
the ``hpx::launch::async`` launch policy. The third future is created by *chaining* the second
future using the ``then()`` member function. This future multiplies the result of the second future
by 3.

The next part of the code demonstrates how to use `promises` and `packaged` tasks, which are constructs
used for communicating data between threads. The ``promise`` class is used to store a value that can be
retrieved *later* using a future. The ``packaged_task`` class represents a task that can be executed
*asynchronously*, and its result can be obtained using a future. The last three lines create a
packaged task that returns an integer, obtain its future, execute the task, and check whether the
future is ready or not.

The code then demonstrates how to use the ``hpx::post()`` and ``hpx::sync()`` functions for
*fire-and-forget* and *synchronous* execution, respectively. The ``hpx::post()`` function executes a
given function *asynchronously* and *returns immediately* without waiting for the result. The
``hpx::sync()`` function executes a given function *synchronously* and *waits* for the result before
returning.

Next the code demonstrates the use of `combinators`, which are higher-order functions that combine
two or more futures into a single future. The ``hpx::when_all()`` function is used to combine two futures,
which return double values, into a tuple of futures. The ``then()`` member function is then used to
compute the area of a circle using the values of the two futures. The ``get()`` member function is used to
retrieve the result of the computation.

The last section demonstrates the use of ``hpx::dataflow()``, which is a higher-order function that waits
for all the future or shared_future arguments to be ready before executing the continuation. The
``hpx::make_ready_future()`` function is used to create a future with a given value. The
``hpx::split_future()`` function is used to split a future of a tuple into a tuple of futures. The last
line retrieves the value of the second future in the tuple using ``hpx::get()`` and prints it to the console.

.. _extend_futures:

Extended facilities for futures
...............................

Concurrency is about both decomposing and composing the program from the parts
that work well individually and together. It is in the composition of connected
and multicore components where today's C++ libraries are still lacking.

The functionality of :cppreference-generic:`thread,future` offers a partial solution.
It allows for the separation of the initiation of an operation and the act of waiting
for its result; however, the act of waiting is synchronous. In communication-intensive
code this act of waiting can be unpredictable, inefficient and simply
frustrating. The example below illustrates a possible synchronous wait using
futures::

    #include <future>
    using namespace std;
    int main()
    {
        future<int> f = async([]() { return 123; });
        int result = f.get(); // might block
    }

For this reason, |hpx| implements a set of extensions to
:cppreference-generic:`thread,future` (as proposed by |cpp11_n4107|_). This
proposal introduces the following key asynchronous operations to
:hpx:class:`hpx::future`, :hpx:class:`hpx::shared_future` and
:hpx:func:`hpx::async`,
which enhance and enrich these facilities.

.. list-table:: Facilities extending ``std::future``
   :widths: 20 80

   * * Facility
     * Description
   * * :hpx:member:`hpx::future::then`
     * In asynchronous programming, it is very common for one asynchronous
       operation, on completion, to invoke a second operation and pass data to
       it. The current C++ standard does not allow one to register a
       continuation to a future. With ``then``, instead of waiting for the result,
       a continuation is "attached" to the asynchronous operation, which is
       invoked when the result is ready. Continuations registered using then
       function will help to avoid blocking waits or wasting threads on polling,
       greatly improving the responsiveness and scalability of an application.
   * * unwrapping constructor for :hpx:class:`hpx::future`
     * In some scenarios, you might want to create a future that returns another
       future, resulting in nested futures. Although it is possible to write
       code to unwrap the outer future and retrieve the nested future and its
       result, such code is not easy to write because users must handle exceptions
       and it may cause a blocking call. Unwrapping can allow users to mitigate
       this problem by doing an asynchronous call to unwrap the outermost
       future.
   * * :hpx:member:`hpx::future::is_ready`
     * There are often situations where a ``get()`` call on a future may not be
       a blocking call, or is only a blocking call under certain circumstances.
       This function gives the ability to test for early completion and allows
       us to avoid associating a continuation, which needs to be scheduled with
       some non-trivial overhead and near-certain loss of cache efficiency.
   * * :hpx:func:`hpx::make_ready_future`
     * Some functions may know the value at the point of construction. In these
       cases the value is immediately available, but needs to be returned as a
       future. By using ``hpx::make_ready_future`` a future can be created that
       holds a pre-computed result in its shared state. In the current standard
       it is non-trivial to create a future directly from a value. First a
       promise must be created, then the promise is set, and lastly the future
       is retrieved from the promise. This can now be done with one operation.

The standard also omits the ability to compose multiple futures. This is a
common pattern that is ubiquitous in other asynchronous frameworks and is
absolutely necessary in order to make C++ a powerful asynchronous programming
language. Not including these functions is synonymous to Boolean algebra without
AND/OR.

In addition to the extensions proposed by |cpp11_n4107|_, |hpx| adds functions
allowing users to compose several futures in a more flexible way.

.. list-table:: Facilities for composing ``hpx::future``\ s

   * * Facility
     * Description
   * * :hpx:func:`hpx::when_any`, :hpx:func:`hpx::when_any_n`
     * Asynchronously wait for at least one of multiple future or shared_future
       objects to finish.
   * * :hpx:func:`hpx::wait_any`, :hpx:func:`hpx::wait_any_n`
     * Synchronously wait for at least one of multiple future or shared_future
       objects to finish.
   * * :hpx:func:`hpx::when_all`, :hpx:func:`hpx::when_all_n`
     * Asynchronously wait for all future and shared_future objects to finish.
   * * :hpx:func:`hpx::wait_all`, :hpx:func:`hpx::wait_all_n`
     * Synchronously wait for all future and shared_future objects to finish.
   * * :hpx:func:`hpx::when_some`, :hpx:func:`hpx::when_some_n`
     * Asynchronously wait for multiple future and shared_future objects to
       finish.
   * * :hpx:func:`hpx::wait_some`, :hpx:func:`hpx::wait_some_n`
     * Synchronously wait for multiple future and shared_future objects to
       finish.
   * * :hpx:func:`hpx::when_each`
     * Asynchronously wait for multiple future and shared_future objects to
       finish and call a function for each of the future objects as soon as it
       becomes ready.
   * * :hpx:func:`hpx::wait_each`, :hpx:func:`hpx::wait_each_n`
     * Synchronously wait for multiple future and shared_future objects to
       finish and call a function for each of the future objects as soon as it
       becomes ready.

.. _channel:

Channels
--------

:ref:`Channels <public_api_header_hpx_channel>` combine communication (the exchange of a value)
with synchronization (guaranteeing that two calculations (tasks) are in a known state). A channel
can transport any number of values of a given type from a sender to a receiver:

.. literalinclude:: ../../examples/quickstart/local_channel_docs.cpp
   :language: c++
   :start-after: //[local_channel_minimal
   :end-before: //]

Channels can be handed to another thread (or in case of channel components, to
other localities), thus establishing a communication channel between two
independent places in the program:

.. literalinclude:: ../../examples/quickstart/local_channel_docs.cpp
   :language: c++
   :start-after: //[local_channel_send_receive
   :end-before: //]

Note how :hpx:member:`hpx::lcos::local::channel::get` without any arguments
returns a future which is ready when a value has been set on the channel. The
launch policy ``hpx::launch::sync`` can be used to make
:hpx:member:`hpx::lcos::local::channel::get` block until a value is set and
return the value directly.

A channel component is created on one :term:`locality` and can be sent to
another :term:`locality` using an action. This example also demonstrates how a
channel can be used as a range of values:

.. literalinclude:: ../../examples/quickstart/channel_docs.cpp
   :language: c++
   :start-after: //[channel
   :end-before: //]


.. _task_block:

Task blocks
-----------

:ref:`Task blocks <public_api_header_hpx_task_block>` in |hpx| provide a way to
structure and organize the execution of tasks in a parallel program, making it
easier to manage dependencies between tasks. A task block actually is a group of
tasks that can be executed in parallel. Tasks in a task block can depend on other
tasks in the same task block. The task block allows the runtime to optimize
the execution of tasks, by scheduling them in an optimal order based on the
dependencies between them.

The ``define_task_block``, ``run`` and the ``wait`` functions implemented based
on |cpp17_n4755|_ are based on the ``task_block`` concept that is a part of the
common subset of the |ppl|_ and the |tbb|_ libraries.

These implementations adopt a simpler syntax than exposed by those libraries---
one that is influenced by language-based concepts, such as spawn and sync from
|cilk_pp|_ and async and finish from |x10|_. They improve on existing practice in
the following ways:

* The exception handling model is simplified and more consistent with normal C++
  exceptions.
* Most violations of strict fork-join parallelism can be enforced at compile
  time (with compiler assistance, in some cases).
* The syntax allows scheduling approaches other than child stealing.

Consider an example of a parallel traversal of a tree, where a user-provided
function compute is applied to each node of the tree, returning the sum of the
results::

    template <typename Func>
    int traverse(node& n, Func && compute)
    {
        int left = 0, right = 0;
        define_task_block(
            [&](task_block<>& tr) {
                if (n.left)
                    tr.run([&] { left = traverse(*n.left, compute); });
                if (n.right)
                    tr.run([&] { right = traverse(*n.right, compute); });
            });

        return compute(n) + left + right;
    }

The example above demonstrates the use of two of the functions,
:hpx:func:`hpx::experimental::define_task_block` and the
:hpx:member:`hpx::experimental::task_block::run` member function of a
:hpx:class:`hpx::experimental::task_block`.

The ``task_block`` function delineates a region in a program code potentially
containing invocations of threads spawned by the ``run`` member function of the
``task_block`` class. The ``run`` function spawns an |hpx| thread, a unit of
work that is allowed to execute in parallel with respect to the caller. Any
parallel tasks spawned by ``run`` within the task block are joined back to a
single thread of execution at the end of the ``define_task_block``. ``run``
takes a user-provided function object ``f`` and starts it asynchronously---i.e.,
it may return before the execution of ``f`` completes. The |hpx| scheduler may
choose to run ``f`` immediately or delay running ``f`` until compute resources
become available.

A ``task_block`` can be constructed only by ``define_task_block`` because it has
no public constructors. Thus, ``run`` can be invoked directly or indirectly
only from a user-provided function passed to ``define_task_block``::

    void g();

    void f(task_block<>& tr)
    {
        tr.run(g);          // OK, invoked from within task_block in h
    }

    void h()
    {
        define_task_block(f);
    }

    int main()
    {
        task_block<> tr;    // Error: no public constructor
        tr.run(g);          // No way to call run outside of a define_task_block
        return 0;
    }

.. _task_block_extensions:

Extensions for task blocks
..........................

Using execution policies with task blocks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

|hpx| implements some extensions for ``task_block`` beyond the actual
standards proposal |cpp17_n4755|_. The main addition is that a ``task_block``
can be invoked with an execution policy as its first argument, very similar to
the parallel algorithms.

An execution policy is an object that expresses the requirements on the
ordering of functions invoked as a consequence of the invocation of a
task block. Enabling passing an execution policy to ``define_task_block``
gives the user control over the amount of parallelism employed by the
created ``task_block``. In the following example the use of an explicit
``par`` execution policy makes the user's intent explicit::

    template <typename Func>
    int traverse(node *n, Func&& compute)
    {
        int left = 0, right = 0;

        define_task_block(
            execution::par,                // execution::parallel_policy
            [&](task_block<>& tb) {
                if (n->left)
                    tb.run([&] { left = traverse(n->left, compute); });
                if (n->right)
                    tb.run([&] { right = traverse(n->right, compute); });
            });

        return compute(n) + left + right;
    }

This also causes the :hpx:class:`hpx::experimental::task_block` object to be a
template in our implementation. The template argument is the type of the
execution policy used to create the task block. The template argument defaults
to :hpx:class:`hpx::execution::parallel_policy`.

|hpx| still supports calling :hpx:func:`hpx::experimental::define_task_block`
without an explicit execution policy. In this case the task block will run using
the :hpx:class:`hpx::execution::parallel_policy`.

|hpx| also adds the ability to access the execution policy that was used to
create a given ``task_block``.

Using executors to run tasks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Often, users want to be able to not only define an execution policy to use by
default for all spawned tasks inside the task block, but also to
customize the execution context for one of the tasks executed by
``task_block::run``. Adding an optionally passed executor instance to that
function enables this use case::

    template <typename Func>
    int traverse(node *n, Func&& compute)
    {
        int left = 0, right = 0;

        define_task_block(
            execution::par,                // execution::parallel_policy
            [&](auto& tb) {
                if (n->left)
                {
                    // use explicitly specified executor to run this task
                    tb.run(my_executor(), [&] { left = traverse(n->left, compute); });
                }
                if (n->right)
                {
                    // use the executor associated with the par execution policy
                    tb.run([&] { right = traverse(n->right, compute); });
                }
            });

        return compute(n) + left + right;
    }

|hpx| still supports calling :hpx:member:`hpx::experimental::task_block::run`
without an explicit executor object. In this case the task will be run using the
executor associated with the execution policy that was used to call
:hpx:func:`hpx::experimental::define_task_block`.

.. _task_group:

Task groups
-----------

A :ref:`task group <public_api_header_hpx_task_group>` in |hpx| is a synchronization primitive
that allows you to execute a group of tasks concurrently and wait for their completion before
continuing. The tasks in an ``hpx::experimental::task_group`` can be added dynamically. This is the |hpx|
implementation of `tbb::task_group` of the |tbb|_ library.

The example below shows that to use a task group, you simply create an ``hpx::task_group`` object
and add tasks to it using the ``run()`` method. Once all the tasks have been added, you can call
the ``wait()`` method to synchronize the tasks and wait for them to complete.

.. literalinclude:: ../../examples/quickstart/task_group_docs.cpp
   :language: c++
   :start-after: //[task_group_docs
   :end-before: //]

.. note::

   `task groups` and `task blocks` are both ways to group and synchronize parallel tasks, but
   `task groups` are used to group multiple tasks together as a single unit, while `task blocks`
   are used to execute a loop in parallel, with each iteration of the loop executing in a separate
   task. If the difference is not clear yet, continue reading.

   A `task group` is a construct that allows multiple parallel tasks to be grouped together as a
   single unit. The task group provides a way to synchronize all the tasks in the group before
   continuing with the rest of the program.

   A `task block`, on the other hand, is a parallel loop construct that allows you to execute a
   loop in parallel, with each iteration of the loop executing in a separate task. The loop
   iterations are executed in a block, meaning that the loop body is executed as a single task.

.. _thread:

Threads
-------

A :ref:`thread <public_api_header_hpx_task_thread>` in |hpx| refers to a sequence of instructions
that can be executed concurrently with other such sequences in multithreading environments, while
sharing a same address space. These threads can communicate with each other through various means,
such as futures or shared data structures.

The example below demonstrates how to launch multiple threads and synchronize them using a ``hpx::latch``
object. It also shows how to query the state of threads and wait for futures to complete.

.. literalinclude:: ../../examples/quickstart/enumerate_threads.cpp
   :language: c++
   :start-after: //[threads_docs
   :end-before: //]

In more detail, the ``wait_for_latch()`` function is a simple helper function that waits for a ``hpx::latch``
object to be released. At this point we remind that ``hpx::latch`` is a synchronization primitive that
allows multiple threads to wait for a common event to occur.

In the ``hpx_main()`` function, an ``hpx::latch`` object is created with a count of ``num_threads + 1``,
indicating that ``num_threads`` threads need to arrive at the latch before the latch is released. The loop
that follows launches ``num_threads`` asynchronous operations, each of which calls the ``wait_for_latch``
function. The resulting futures are added to the vector.

After the threads have been launched, ``hpx::this_thread::yield()`` is called to give them a chance to
reach the latch before the program proceeds. Then, the ``hpx::threads::enumerate_threads`` function
prints the state of each suspended thread, while the next call of ``l.arrive_and_wait()`` waits for all
the threads to reach the latch. Finally, ``hpx::wait_all`` is called to wait for all the futures to complete.

.. hint::

   An advantage of using ``hpx::thread`` over other threading libraries is that it is optimized for
   high-performance parallelism, with support for lightweight threads and task scheduling to minimize
   thread overhead and maximize parallelism. Additionally, ``hpx::thread`` integrates seamlessly with
   other features of |hpx| such as futures, promises, and task groups, making it a powerful tool for
   parallel programming.

   Checkout the examples of :ref:`shared_mutex`, :ref:`condition_variable`, :ref:`semaphore`
   to see how |hpx| threads are used in combination with other features.

.. _parallel:

High level parallel facilities
==============================

In preparation for the upcoming C++ Standards, there are currently several proposals
targeting different facilities supporting parallel programming. |hpx| implements
(and extends) some of those proposals. This is well aligned with our strategy to
align the APIs exposed from |hpx| with current and future C++ Standards.

At this point, |hpx| implements several of the C++ Standardization working
papers, most notably |cpp11_n4104|_ (Working Draft, Technical Specification for
C++ Extensions for Parallelism), |cpp17_n4755|_ (Task Blocks), and
|cpp11_n4406|_ (Parallel Algorithms Need Executors).

.. _parallel_algorithms:

Using parallel algorithms
-------------------------

A parallel algorithm is a function template declared in the namespace
``hpx::parallel``.

All parallel algorithms are very similar in semantics to their sequential
counterparts (as defined in the ``namespace std``) with an additional formal
template parameter named ``ExecutionPolicy``. The execution policy is generally
passed as the first argument to any of the parallel algorithms and describes the
manner in which the execution of these algorithms may be parallelized and the
manner in which they apply user-provided function objects.

The applications of function objects in parallel algorithms invoked with an
execution policy object of type :hpx:class:`hpx::execution::sequenced_policy` or
:hpx:class:`hpx::execution::sequenced_task_policy` execute in sequential order. For
:hpx:class:`hpx::execution::sequenced_policy` the execution happens in the calling thread.

The applications of function objects in parallel algorithms invoked with an
execution policy object of type :hpx:class:`hpx::execution::parallel_policy` or
:hpx:class:`hpx::execution::parallel_task_policy` are permitted to execute in an unordered
fashion in unspecified threads, and are indeterminately sequenced within each
thread.

.. important::

   It is the caller's responsibility to ensure correctness, such as making sure that the
   invocation does not introduce data races or deadlocks.

The example below demonstrates how to perform a sequential and parallel :hpx:func:`hpx::for_each` \
loop on a vector of integers.

.. literalinclude:: ../../examples/quickstart/for_each_docs.cpp
   :language: c++
   :start-after: //[for_each_docs
   :end-before: //]

The above code uses ``hpx::for_each`` to print the elements of the vector ``v{1, 2, 3, 4, 5}``.
At first, ``hpx::for_each()`` is called without an execution policy, which means that it applies
the lambda function ``print`` to each element in the vector sequentially. Hence, the elements are
printed in order.

Next, ``hpx::for_each()`` is called with the ``hpx::execution::par`` execution policy,
which applies the lambda function ``print`` to each element in the vector in parallel. Therefore,
the output order of the elements in the vector is not deterministic and may vary from run to run.

Parallel exceptions
-------------------

During the execution of a standard parallel algorithm, if temporary memory
resources are required by any of the algorithms and no memory is available, the
algorithm throws a ``std::bad_alloc`` exception.

During the execution of any of the parallel algorithms, if the application of a
function object terminates with an uncaught exception, the behavior of the
program is determined by the type of execution policy used to invoke the
algorithm:

* If the execution policy object is of type
  :hpx:class:`hpx::execution::parallel_unsequenced_policy`, :hpx:func:`hpx::terminate` shall
  be called.
* If the execution policy object is of type :hpx:class:`hpx::execution::sequenced_policy`,
  :hpx:class:`hpx::execution::sequenced_task_policy`, :hpx:class:`hpx::execution::parallel_policy`, or
  :hpx:class:`hpx::execution::parallel_task_policy`, the execution of the algorithm terminates
  with an :hpx:class:`hpx::exception_list` exception. All uncaught exceptions thrown during the
  application of user-provided function objects shall be contained in the
  :hpx:class:`hpx::exception_list`.

For example, the number of invocations of the user-provided function object in
for_each is unspecified. When :hpx:class:`hpx::for_each` is executed sequentially, only one
exception will be contained in the :hpx:class:`hpx::exception_list` object.

These guarantees imply that, unless the algorithm has failed to allocate memory
and terminated with ``std::bad_alloc``, all exceptions thrown during the
execution of the algorithm are communicated to the caller. It is unspecified
whether an algorithm implementation will "forge ahead" after encountering and
capturing a user exception.

The algorithm may terminate with the ``std::bad_alloc`` exception even if one or
more user-provided function objects have terminated with an exception. For
example, this can happen when an algorithm fails to allocate memory while
creating or adding elements to the :hpx:class:`hpx::exception_list` object.

Parallel algorithms
-------------------

|hpx| provides implementations of the following parallel algorithms:

.. list-table:: Non-modifying parallel algorithms of header :ref:`public_api_header_hpx_algorithm`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::adjacent_find`
     * Computes the differences between adjacent elements in a range.
     * :cppreference-algorithm:`adjacent_find`
   * * :hpx:func:`hpx::all_of`
     * Checks if a predicate is ``true`` for all of the elements in a range.
     * :cppreference-algorithm:`all_any_none_of`
   * * :hpx:func:`hpx::any_of`
     * Checks if a predicate is ``true`` for any of the elements in a range.
     * :cppreference-algorithm:`all_any_none_of`
   * * :hpx:func:`hpx::count`
     * Returns the number of elements equal to a given value.
     * :cppreference-algorithm:`count`
   * * :hpx:func:`hpx::count_if`
     * Returns the number of elements satisfying a specific criteria.
     * :cppreference-algorithm:`count_if`
   * * :hpx:func:`hpx::equal`
     * Determines if two sets of elements are the same.
     * :cppreference-algorithm:`equal`
   * * :hpx:func:`hpx::find`
     * Finds the first element equal to a given value.
     * :cppreference-algorithm:`find`
   * * :hpx:func:`hpx::find_end`
     * Finds the last sequence of elements in a certain range.
     * :cppreference-algorithm:`find_end`
   * * :hpx:func:`hpx::find_first_of`
     * Searches for any one of a set of elements.
     * :cppreference-algorithm:`find_first_of`
   * * :hpx:func:`hpx::find_if`
     * Finds the first element satisfying a specific criteria.
     * :cppreference-algorithm:`find_if`
   * * :hpx:func:`hpx::find_if_not`
     * Finds the first element not satisfying a specific criteria.
     * :cppreference-algorithm:`find_if_not`
   * * :hpx:func:`hpx::for_each`
     * Applies a function to a range of elements.
     * :cppreference-algorithm:`for_each`
   * * :hpx:func:`hpx::for_each_n`
     * Applies a function to a number of elements.
     * :cppreference-algorithm:`for_each_n`
   * * :hpx:func:`hpx::lexicographical_compare`
     * Checks if a range of values is lexicographically less than another range of values.
     * :cppreference-algorithm:`lexicographical_compare`
   * * :hpx:func:`hpx::mismatch`
     * Finds the first position where two ranges differ.
     * :cppreference-algorithm:`mismatch`
   * * :hpx:func:`hpx::none_of`
     * Checks if a predicate is ``true`` for none of the elements in a range.
     * :cppreference-algorithm:`all_any_none_of`
   * * :hpx:func:`hpx::search`
     * Searches for a range of elements.
     * :cppreference-algorithm:`search`
   * * :hpx:func:`hpx::search_n`
     * Searches for a number consecutive copies of an element in a range.
     * :cppreference-algorithm:`search_n`

|

.. list-table:: Modifying parallel algorithms of header :ref:`public_api_header_hpx_algorithm`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::copy`
     * Copies a range of elements to a new location.
     * :cppreference-algorithm:`exclusive_scan`
   * * :hpx:func:`hpx::copy_n`
     * Copies a number of elements to a new location.
     * :cppreference-algorithm:`copy_n`
   * * :hpx:func:`hpx::copy_if`
     * Copies the elements from a range to a new location for which the given predicate is ``true``
     * :cppreference-algorithm:`copy`
   * * :hpx:func:`hpx::move`
     * Moves a range of elements to a new location.
     * :cppreference-algorithm:`move`
   * * :hpx:func:`hpx::fill`
     * Assigns a range of elements a certain value.
     * :cppreference-algorithm:`fill`
   * * :hpx:func:`hpx::fill_n`
     * Assigns a value to a number of elements.
     * :cppreference-algorithm:`fill_n`
   * * :hpx:func:`hpx::generate`
     * Saves the result of a function in a range.
     * :cppreference-algorithm:`generate`
   * * :hpx:func:`hpx::generate_n`
     * Saves the result of N applications of a function.
     * :cppreference-algorithm:`generate_n`
   * * :hpx:func:`hpx::experimental::reduce_by_key`
     * Performs an inclusive scan on consecutive elements with matching keys,
       with a reduction to output only the final sum for each key. The key
       sequence ``{1,1,1,2,3,3,3,3,1}`` and value sequence
       ``{2,3,4,5,6,7,8,9,10}`` would be reduced to ``keys={1,2,3,1}``,
       ``values={9,5,30,10}``.
     *
   * * :hpx:func:`hpx::remove`
     * Removes the elements from a range that are equal to the given value.
     * :cppreference-algorithm:`remove`
   * * :hpx:func:`hpx::remove_if`
     * Removes the elements from a range that are equal to the given predicate is ``false``
     * :cppreference-algorithm:`remove`
   * * :hpx:func:`hpx::remove_copy`
     * Copies the elements from a range to a new location that are not equal to the given value.
     * :cppreference-algorithm:`remove_copy`
   * * :hpx:func:`hpx::remove_copy_if`
     * Copies the elements from a range to a new location for which the given predicate is ``false``
     * :cppreference-algorithm:`remove_copy`
   * * :hpx:func:`hpx::replace`
     * Replaces all values satisfying specific criteria with another value.
     * :cppreference-algorithm:`replace`
   * * :hpx:func:`hpx::replace_if`
     * Replaces all values satisfying specific criteria with another value.
     * :cppreference-algorithm:`replace`
   * * :hpx:func:`hpx::replace_copy`
     * Copies a range, replacing elements satisfying specific criteria with another value.
     * :cppreference-algorithm:`replace_copy`
   * * :hpx:func:`hpx::replace_copy_if`
     * Copies a range, replacing elements satisfying specific criteria with another value.
     * :cppreference-algorithm:`replace_copy`
   * * :hpx:func:`hpx::reverse`
     * Reverses the order elements in a range.
     * :cppreference-algorithm:`reverse`
   * * :hpx:func:`hpx::reverse_copy`
     * Creates a copy of a range that is reversed.
     * :cppreference-algorithm:`reverse_copy`
   * * :hpx:func:`hpx::rotate`
     * Rotates the order of elements in a range.
     * :cppreference-algorithm:`rotate`
   * * :hpx:func:`hpx::rotate_copy`
     * Copies and rotates a range of elements.
     * :cppreference-algorithm:`rotate_copy`
   * * :hpx:func:`hpx::shift_left`
     * Shifts the elements in the range left by n positions.
     * :cppreference-algorithm:`shift_left`
   * * :hpx:func:`hpx::shift_right`
     * Shifts the elements in the range right by n positions.
     * :cppreference-algorithm:`shift_right`
   * * :hpx:func:`hpx::swap_ranges`
     * Swaps two ranges of elements.
     * :cppreference-algorithm:`swap_ranges`
   * * :hpx:func:`hpx::transform`
     * Applies a function to a range of elements.
     * :cppreference-algorithm:`transform`
   * * :hpx:func:`hpx::unique`
     * Eliminates all but the first element from every consecutive group of equivalent elements from a range.
     * :cppreference-algorithm:`unique`
   * * :hpx:func:`hpx::unique_copy`
     * Copies the elements from one range to another in such a way that there are no consecutive equal elements.
     * :cppreference-algorithm:`unique_copy`

|

.. list-table:: Set operations on sorted sequences of header :ref:`public_api_header_hpx_algorithm`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::merge`
     * Merges two sorted ranges.
     * :cppreference-algorithm:`merge`
   * * :hpx:func:`hpx::inplace_merge`
     * Merges two ordered ranges in-place.
     * :cppreference-algorithm:`inplace_merge`
   * * :hpx:func:`hpx::includes`
     * Returns true if one set is a subset of another.
     * :cppreference-algorithm:`includes`
   * * :hpx:func:`hpx::set_difference`
     * Computes the difference between two sets.
     * :cppreference-algorithm:`set_difference`
   * * :hpx:func:`hpx::set_intersection`
     * Computes the intersection of two sets.
     * :cppreference-algorithm:`set_intersection`
   * * :hpx:func:`hpx::set_symmetric_difference`
     * Computes the symmetric difference between two sets.
     * :cppreference-algorithm:`set_symmetric_difference`
   * * :hpx:func:`hpx::set_union`
     * Computes the union of two sets.
     * :cppreference-algorithm:`set_union`

|

.. list-table:: Heap operations of header :ref:`public_api_header_hpx_algorithm`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::is_heap`
     * Returns ``true`` if the range is max heap.
     * :cppreference-algorithm:`is_heap`
   * * :hpx:func:`hpx::is_heap_until`
     * Returns the first element that breaks a max heap.
     * :cppreference-algorithm:`is_heap_until`
   * * :hpx:func:`hpx::make_heap`
     * Constructs a max heap in the range [first, last).
     * :cppreference-algorithm:`make_heap`

|

.. list-table:: Minimum/maximum operations of header :ref:`public_api_header_hpx_algorithm`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::max_element`
     * Returns the largest element in a range.
     * :cppreference-algorithm:`max_element`
   * * :hpx:func:`hpx::min_element`
     * Returns the smallest element in a range.
     * :cppreference-algorithm:`min_element`
   * * :hpx:func:`hpx::minmax_element`
     * Returns the smallest and the largest element in a range.
     * :cppreference-algorithm:`minmax_element`

|

.. list-table:: Partitioning Operations of header :ref:`public_api_header_hpx_algorithm`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::nth_element`
     * Partially sorts the given range making sure that it is partitioned by the given element
     * :cppreference-algorithm:`nth_element`
   * * :hpx:func:`hpx::is_partitioned`
     * Returns ``true`` if each true element for a predicate precedes the false elements in a range.
     * :cppreference-algorithm:`is_partitioned`
   * * :hpx:func:`hpx::partition`
     * Divides elements into two groups without preserving their relative order.
     * :cppreference-algorithm:`partition`
   * * :hpx:func:`hpx::partition_copy`
     * Copies a range dividing the elements into two groups.
     * :cppreference-algorithm:`partition_copy`
   * * :hpx:func:`hpx::stable_partition`
     * Divides elements into two groups while preserving their relative order.
     * :cppreference-algorithm:`stable_partition`

|

.. list-table:: Sorting Operations of header :ref:`public_api_header_hpx_algorithm`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::is_sorted`
     * Returns ``true`` if each element in a range is sorted.
     * :cppreference-algorithm:`is_sorted`
   * * :hpx:func:`hpx::is_sorted_until`
     * Returns the first unsorted element.
     * :cppreference-algorithm:`is_sorted_until`
   * * :hpx:func:`hpx::sort`
     * Sorts the elements in a range.
     * :cppreference-algorithm:`sort`
   * * :hpx:func:`hpx::stable_sort`
     * Sorts the elements in a range, maintain sequence of equal elements.
     * :cppreference-algorithm:`stable_sort`
   * * :hpx:func:`hpx::partial_sort`
     * Sorts the first elements in a range.
     * :cppreference-algorithm:`partial_sort`
   * * :hpx:func:`hpx::partial_sort_copy`
     * Sorts the first elements in a range, storing the result in another range.
     * :cppreference-algorithm:`partial_sort_copy`
   * * :hpx:func:`hpx::experimental::sort_by_key`
     * Sorts one range of data using keys supplied in another range.
     *

|

.. list-table:: Numeric Parallel Algorithms of header :ref:`public_api_header_hpx_numeric`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::adjacent_difference`
     * Calculates the difference between each element in an input range and the preceding element.
     * :cppreference-algorithm:`adjacent_difference`
   * * :hpx:func:`hpx::exclusive_scan`
     * Does an exclusive parallel scan over a range of elements.
     * :cppreference-algorithm:`exclusive_scan`
   * * :hpx:func:`hpx::inclusive_scan`
     * Does an inclusive parallel scan over a range of elements.
     * :cppreference-algorithm:`inclusive_scan`
   * * :hpx:func:`hpx::reduce`
     * Sums up a range of elements.
     * :cppreference-algorithm:`reduce`
   * * :hpx:func:`hpx::transform_exclusive_scan`
     * Does an exclusive parallel scan over a range of elements after applying a function.
     * :cppreference-algorithm:`transform_exclusive_scan`
   * * :hpx:func:`hpx::transform_inclusive_scan`
     * Does an inclusive parallel scan over a range of elements after applying a function.
     * :cppreference-algorithm:`transform_inclusive_scan`
   * * :hpx:func:`hpx::transform_reduce`
     * Sums up a range of elements after applying a function. Also, accumulates the inner products of two input ranges.
     * :cppreference-algorithm:`transform_reduce`

|

.. list-table:: Dynamic Memory Management of header :ref:`public_api_header_hpx_memory`
   :widths: 25 55 20

   * * Name
     * Description
     * C++ standard
   * * :hpx:func:`hpx::destroy`
     * Destroys a range of objects.
     * :cppreference-memory:`destroy`
   * * :hpx:func:`hpx::destroy_n`
     * Destroys a range of objects.
     * :cppreference-memory:`destroy_n`
   * * :hpx:func:`hpx::uninitialized_copy`
     * Copies a range of objects to an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_copy`
   * * :hpx:func:`hpx::uninitialized_copy_n`
     * Copies a number of objects to an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_copy_n`
   * * :hpx:func:`hpx::uninitialized_default_construct`
     * Copies a range of objects to an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_default_construct`
   * * :hpx:func:`hpx::uninitialized_default_construct_n`
     * Copies a number of objects to an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_default_construct_n`
   * * :hpx:func:`hpx::uninitialized_fill`
     * Copies an object to an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_fill`
   * * :hpx:func:`hpx::uninitialized_fill_n`
     * Copies an object to an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_fill_n`
   * * :hpx:func:`hpx::uninitialized_move`
     * Moves a range of objects to an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_move`
   * * :hpx:func:`hpx::uninitialized_move_n`
     * Moves a number of objects to an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_move_n`
   * * :hpx:func:`hpx::uninitialized_value_construct`
     * Constructs objects in an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_value_construct`
   * * :hpx:func:`hpx::uninitialized_value_construct_n`
     * Constructs objects in an uninitialized area of memory.
     * :cppreference-memory:`uninitialized_value_construct_n`

|

.. list-table:: Index-based for-loops of header :ref:`public_api_header_hpx_algorithm`

   * * Name
     * Description
   * * :hpx:func:`hpx::experimental::for_loop`
     * Implements loop functionality over a range specified by integral or iterator bounds.
   * * :hpx:func:`hpx::experimental::for_loop_strided`
     * Implements loop functionality over a range specified by integral or iterator bounds.
   * * :hpx:func:`hpx::experimental::for_loop_n`
     * Implements loop functionality over a range specified by integral or iterator bounds.
   * * :hpx:func:`hpx::experimental::for_loop_n_strided`
     * Implements loop functionality over a range specified by integral or iterator bounds.

.. _executor_parameters:

Executor parameters and executor parameter traits
-------------------------------------------------

|hpx| introduces the notion of execution parameters and execution parameter
traits. At this point, the only parameter that can be customized is the size of
the chunks of work executed on a single |hpx| thread (such as the number of loop
iterations combined to run as a single task).

An executor parameter object is responsible for exposing the calculation of the
size of the chunks scheduled. It abstracts the (potentially platform-specific)
algorithms of determining those chunk sizes.

The way executor parameters are implemented is aligned with the way executors
are implemented. All functionalities of concrete executor parameter types are
exposed and accessible through a corresponding customization point, e.g.
``get_chunk_size()``.

With ``executor_parameter_traits``, clients access all types of executor
parameters uniformly, e.g.::

    std::size_t chunk_size =
        hpx::execution::experimental::get_chunk_size(my_parameter, my_executor,
            num_cores, num_tasks);

This call synchronously retrieves the size of a single chunk of loop iterations
(or similar) to combine for execution on a single |hpx| thread if the overall
number of cores ``num_cores`` and tasks to schedule is given by ``num_tasks``.
The lambda function exposes a means of test-probing the execution of a single
iteration for performance measurement purposes. The execution parameter type
might dynamically determine the execution time of one or more tasks in order
to calculate the chunk size; see
:hpx:class:`hpx::execution::experimental::auto_chunk_size` for an example of
this executor parameter type.

Other functions in the interface exist to discover whether an executor parameter
type should be invoked once (i.e., it returns a static chunk size; see
:hpx:class:`hpx::execution::experimental::static_chunk_size`) or whether it
should be invoked
for each scheduled chunk of work (i.e., it returns a variable chunk size; for an
example, see :hpx:class:`hpx::execution::experimental::guided_chunk_size`).

Although this interface appears to require executor parameter type authors to
implement all different basic operations, none are required. In
practice, all operations have sensible defaults. However, some executor
parameter types will naturally specialize all operations for maximum efficiency.

|hpx|  implements the following executor parameter types:

* :hpx:class:`hpx::execution::experimental::auto_chunk_size`: Loop iterations
  are divided into pieces and then assigned to threads. The number of loop
  iterations combined is
  determined based on measurements of how long the execution of 1% of the
  overall number of iterations takes. This executor parameter type makes sure
  that as many loop iterations are combined as necessary to run for the amount
  of time specified.
* :hpx:class:`hpx::execution::experimental::static_chunk_size`: Loop iterations
  are divided
  into pieces of a given size and then assigned to threads. If the size is not
  specified, the iterations are, if possible, evenly divided contiguously among
  the threads. This executor parameters type is equivalent to OpenMP's STATIC
  scheduling directive.
* :hpx:class:`hpx::execution::experimental::dynamic_chunk_size`: Loop iterations
  are divided
  into pieces of a given size and then dynamically scheduled among the cores;
  when a core finishes one chunk, it is dynamically assigned another. If the
  size is not specified, the default chunk size is 1. This executor parameter
  type is equivalent to OpenMP's DYNAMIC scheduling directive.
* :hpx:class:`hpx::execution::experimental::guided_chunk_size`: Iterations are
  dynamically
  assigned to cores in blocks as cores request them until no blocks remain to be
  assigned. This is similar to ``dynamic_chunk_size`` except that the block size
  decreases each time a number of loop iterations is given to a thread. The size
  of the initial block is proportional to ``number_of_iterations /
  number_of_cores``. Subsequent blocks are proportional to
  ``number_of_iterations_remaining / number_of_cores``. The optional chunk size
  parameter defines the minimum block size. The default minimal chunk size is 1.
  This executor parameter type is equivalent to OpenMP's GUIDED scheduling
  directive.
