..
    Copyright (C) 2026 Arpit Khandelwal

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _examples_termination_detection:

====================
Termination detection
====================

This example demonstrates how to use |hpx|'s termination detection API to
ensure all asynchronous work has completed before shutting down the runtime.
This is particularly useful when you need to guarantee that all posted tasks
have finished execution before the application exits.

Overview
========

The termination detection API provides a way to wait for all local |hpx|-threads
to complete their work. This is essential in scenarios where you spawn
asynchronous tasks using :hpx:func:`hpx::post` or :hpx:func:`hpx::async` and
need to ensure they all finish before the runtime shuts down.

The API supports several use cases:

* **Basic usage**: Wait indefinitely for all tasks to complete
* **Timeout**: Wait for a specified duration
* **Deadline**: Wait until a specific time point
* **Cancellation**: Support for cancellation tokens to interrupt the wait

Setup
=====

The source code for this example can be found here:
:download:`termination_detection.cpp <../../examples/quickstart/termination_detection.cpp>`.

To compile this program, go to your |hpx| build directory (see
:ref:`building_hpx` for information on configuring and building |hpx|) and
enter:

.. code-block:: shell-session

   $ make examples.quickstart.termination_detection

To run the program type:

.. code-block:: shell-session

   $ ./bin/termination_detection

This will execute all four examples demonstrating different usage patterns of
the termination detection API.

Walkthrough
===========

The example demonstrates four different usage patterns of the termination
detection API. Let's examine each one:

Basic usage
-----------

The simplest form waits indefinitely for all local threads to complete:

.. literalinclude:: ../../examples/quickstart/termination_detection.cpp
   :language: c++
   :start-after: // Example 1: Basic usage
   :end-before: // Example 2: Timeout

This is useful when you want to ensure all work is done before shutting down,
and you don't have time constraints.

Timeout usage
-------------

You can specify a maximum duration to wait:

.. literalinclude:: ../../examples/quickstart/termination_detection.cpp
   :language: c++
   :start-after: // Example 2: Timeout
   :end-before: // Example 3: Deadline

The function returns ``true`` if all threads completed within the timeout, or
``false`` if the timeout elapsed. This is useful when you want to give tasks a
reasonable amount of time to complete but don't want to wait indefinitely.

Deadline usage
--------------

Similar to timeout, but you specify an absolute time point:

.. literalinclude:: ../../examples/quickstart/termination_detection.cpp
   :language: c++
   :start-after: // Example 3: Deadline
   :end-before: // Example 4: Cancellation

This is useful when you have a specific deadline by which all work must be
completed.

Cancellation support
--------------------

The most flexible form supports cancellation tokens:

.. literalinclude:: ../../examples/quickstart/termination_detection.cpp
   :language: c++
   :start-after: // Example 4: Cancellation
   :end-before: //]

This allows external control over the wait operation. You can request
cancellation from another thread, which is useful in scenarios like graceful
shutdown with user interruption support.

API Reference
=============

The termination detection API is available in the ``hpx::local`` namespace:

.. cpp:function:: void hpx::local::termination_detection()

   Wait indefinitely for all local |hpx|-threads to complete.

.. cpp:function:: bool hpx::local::termination_detection(hpx::chrono::steady_duration const& timeout)

   Wait for all local |hpx|-threads to complete, with a timeout.

   :param timeout: Maximum duration to wait
   :returns: ``true`` if all threads completed, ``false`` if timeout elapsed

.. cpp:function:: bool hpx::local::termination_detection(hpx::chrono::steady_time_point const& deadline)

   Wait for all local |hpx|-threads to complete, until a deadline.

   :param deadline: Absolute time point to wait until
   :returns: ``true`` if all threads completed, ``false`` if deadline passed

.. cpp:function:: bool hpx::local::termination_detection(hpx::stop_token stop_token, hpx::chrono::steady_duration const& timeout)

   Wait for all local |hpx|-threads to complete, with cancellation support.

   :param stop_token: Token that can be used to request cancellation
   :param timeout: Maximum duration to wait (defaults to maximum duration)
   :returns: ``true`` if all threads completed, ``false`` if timeout elapsed or stop was requested

Use cases
=========

The termination detection API is particularly useful in the following scenarios:

* **Graceful shutdown**: Ensure all background tasks complete before exiting
* **Testing**: Verify that all spawned tasks have finished in unit tests
* **Batch processing**: Wait for all work items in a batch to complete
* **Resource cleanup**: Ensure all tasks using shared resources have finished before cleanup

See also
========

* :hpx:func:`hpx::post` - Fire-and-forget asynchronous execution
* :hpx:func:`hpx::async` - Asynchronous execution with a future
* :hpx:func:`hpx::finalize` - Shut down the |hpx| runtime
