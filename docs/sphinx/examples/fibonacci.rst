..
    Copyright (C) 2012 Adrian Serio
    Copyright (C) 2012 Vinay C Amatya
    Copyright (C) 2015 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _examples_fibonacci:

===================================
Asynchronous execution with actions
===================================

This example extends the :ref:`previous example <examples_fibonacci_local>` by
introducing :term:`actions<action>`: functions that can be run remotely. In this
example, however, we will still only run the action locally. The mechanism to
execute :term:`actions<action>` stays the same: :hpx:func:`hpx::async`. Later
examples will demonstrate running actions on remote :term:`localities<locality>`
(e.g. :ref:`examples_hello_world`).

Setup
=====

The source code for this example can be found here:
:download:`fibonacci.cpp <../../examples/quickstart/fibonacci.cpp>`.

To compile this program, go to your |hpx| build directory (see
:ref:`building_hpx` for information on configuring and building |hpx|) and
enter:

.. code-block:: shell-session

   $ make examples.quickstart.fibonacci

To run the program type:

.. code-block:: shell-session

   $ ./bin/fibonacci

This should print (time should be approximate):

.. code-block:: text

    fibonacci(10) == 55
    elapsed time: 0.00186288 [s]

This run used the default settings, which calculate the tenth element of the
Fibonacci sequence. To declare which Fibonacci value you want to calculate, use
the ``--n-value`` option. Additionally you can use the :option:`--hpx:threads`
option to declare how many OS-threads you wish to use when running the program.
For instance, running:

.. code-block:: shell-session

   $ ./bin/fibonacci --n-value 20 --hpx:threads 4

Will yield:

.. code-block:: text

   fibonacci(20) == 6765
   elapsed time: 0.233827 [s]

Walkthrough
===========

The code needed to initialize the |hpx| runtime is the same as in the
:ref:`previous example <examples_fibonacci_local>`:

.. literalinclude:: ../../examples/quickstart/fibonacci.cpp
   :language: c++
   :start-after: //[fib_main
   :end-before: //]


The :hpx:func:`hpx::init` function in ``main()`` starts the runtime system, and
invokes ``hpx_main()`` as the first |hpx|-thread. The command line option
``--n-value`` is read in, a timer
(:hpx:class:`hpx::chrono::high_resolution_timer`) is set up to record the time it
takes to do the computation, the ``fibonacci`` :term:`action` is invoked
synchronously, and the answer is printed out.

.. literalinclude:: ../../examples/quickstart/fibonacci.cpp
   :language: c++
   :start-after: //[fib_hpx_main
   :end-before: //]

Upon a closer look we see that we've created a ``std::uint64_t`` to store the
result of invoking our ``fibonacci_action`` ``fib``. This :term:`action` will
launch synchronously (as the work done inside of the :term:`action` will be
asynchronous itself) and return the result of the Fibonacci sequence. But wait,
what is an :term:`action`? And what is this ``fibonacci_action``? For starters,
an :term:`action` is a wrapper for a function. By wrapping functions, |hpx| can
send packets of work to different processing units. These vehicles allow users
to calculate work now, later, or on certain nodes. The first argument to our
:term:`action` is the location where the :term:`action` should be run. In this
case, we just want to run the :term:`action` on the machine that we are
currently on, so we use :hpx:func:`hpx::find_here`. To
further understand this we turn to the code to find where ``fibonacci_action``
was defined:

.. literalinclude:: ../../examples/quickstart/fibonacci.cpp
   :language: c++
   :start-after: //[fib_action
   :end-before: //]

A plain :term:`action` is the most basic form of :term:`action`. Plain
:term:`action`\ s wrap simple global functions which are not associated with any
particular object (we will discuss other types of :term:`action`\ s in
:ref:`examples_accumulator`). In this block of code the function ``fibonacci()``
is declared. After the declaration, the function is wrapped in an :term:`action`
in the declaration :c:macro:`HPX_PLAIN_ACTION`. This function takes two
arguments: the name of the function that is to be wrapped and the name of the
:term:`action` that you are creating.

This picture should now start making sense. The function ``fibonacci()`` is
wrapped in an :term:`action` ``fibonacci_action``, which was run synchronously
but created asynchronous work, then returns a ``std::uint64_t`` representing the
result of the function ``fibonacci()``. Now, let's look at the function
``fibonacci()``:

.. literalinclude:: ../../examples/quickstart/fibonacci.cpp
   :language: c++
   :start-after: //[fib_func
   :end-before: //]

This block of code is much more straightforward and should look familiar from
the :ref:`previous example <examples_fibonacci_local>`. First, ``if (n < 2)``,
meaning n is 0 or 1, then we return 0 or 1 (recall the first element of the
Fibonacci sequence is 0 and the second is 1). If n is larger than 1 we spawn two
tasks using :hpx:func:`hpx::async`. Each of these futures represents an
asynchronous, recursive call to ``fibonacci``. As previously we wait for both
futures to finish computing, get the results, add them together, and return that
value as our result. The recursive call tree will continue until n is equal to 0
or 1, at which point the value can be returned because it is implicitly known.
When this termination condition is reached, the futures can then be added up,
producing the n-th value of the Fibonacci sequence.
