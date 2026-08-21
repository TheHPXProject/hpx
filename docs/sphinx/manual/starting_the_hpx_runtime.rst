..
    Copyright (C) 2018 Nikunj Gupta
    Copyright (C) 2007-2017 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _starting_hpx:

==========================
Starting the |hpx| runtime
==========================

In order to write an application that uses services from the |hpx| runtime
system, you need to initialize the |hpx| library by inserting certain calls
into the code of your application. Depending on your use case, this can be done
in 3 different ways:

* :ref:`Minimally invasive <minimal>`: Re-use the ``main()`` function as the
  main |hpx| entry point.
* :ref:`Balanced use case <medium>`: Supply your own main |hpx| entry point
  while blocking the main thread.
* :ref:`Most flexibility <flexible>`: Supply your own main |hpx| entry point
  while avoiding blocking the main thread.
* :ref:`Suspend and resume <suspend_resume>`: As above but suspend and resume
  the |hpx| runtime to allow for other runtimes to be used.

.. _minimal:

Re-use the ``main()`` function as the main |hpx| entry point
============================================================

This method is the least intrusive to your code. However, it provides you with
the smallest flexibility in terms of initializing the |hpx| runtime system. The
following code snippet shows what a minimal |hpx| application using this
technique looks like:

.. code-block:: c++

    #include <hpx/hpx_main.hpp>

    int main(int argc, char* argv[])
    {
        return 0;
    }

The only change to your code you have to make is to include the file
``hpx/hpx_main.hpp``. In this case the function ``main()`` will be invoked as
the first |hpx| thread of the application. The runtime system will be
initialized behind the scenes before the function ``main()`` is executed and
will automatically stop after ``main()`` has returned. For this method to work
you must link your application to the |cmake|_ target ``HPX::wrap_main``. This is
done automatically if you are using the provided macros
(:ref:`using_hpx_cmake_macros`) to set up your application, but must be done
explicitly if you are using targets directly (:ref:`using_hpx_cmake_targets`).
All |hpx| API functions can be used from within the ``main()`` function now. If
you cannot or do not want to include ``hpx/hpx_main.hpp`` in ``main.cpp``, you
can instead link against ``HPX::auto_wrap_main``. That target enables the same
runtime startup path without needing the header-triggered opt-in.

.. note::

    The use of ``HPX::auto_wrap_main`` is not supported when using the
    native Windows MSVC toolchain.

.. note::

   The function ``main()`` does not need to expect receiving ``argc`` and
   ``argv`` as shown above, but could expose the signature ``int main()``. This
   is consistent with the usually allowed prototypes for the function ``main()``
   in C++ applications.

All command line arguments specific to |hpx| will still be processed by the
|hpx| runtime system as usual. However, those command line options will be
removed from the list of values passed to ``argc``/\ ``argv`` of the function
``main()``. The list of values passed to ``main()`` will hold only the
commandline options that are not recognized by the |hpx| runtime system (see
the section :ref:`commandline` for more details on what options are recognized
by |hpx|).

.. note::

   In this mode all one-letter shortcuts that are normally
   available on the |hpx| command line are disabled (such as ``-t`` or ``-l`` see
   :ref:`commandline`). This is done to minimize any possible interaction
   between the command line options recognized by the |hpx| runtime system and
   any command line options defined by the application.

The value returned from the function ``main()`` as shown above will be returned
to the operating system as usual.

.. important::

   To achieve this seamless integration, the header file ``hpx/hpx_main.hpp``
   defines a macro::

        #define main hpx_startup::user_main

   which could result in unexpected behavior.

.. important::

   To achieve this seamless integration, we use different implementations for
   different operating systems. In case of Linux or macOS, the code present in
   ``hpx_wrap.cpp`` is put into action. We hook into the system function in case
   of Linux and provide alternate entry point in case of macOS. For other
   operating systems we rely on a macro::

       #define main hpx_startup::user_main

   provided in the header file ``hpx/hpx_main.hpp``. This implementation can
   result in unexpected behavior.

.. caution::
   We make use of an *override* variable ``include_libhpx_wrap`` in the header
   file ``hpx/hpx_main.hpp`` to swiftly choose the function call stack at
   runtime. Therefore, the header file should *only* be included in the main
   executable. Including it in the components will result in multiple definition
   of the variable.

.. _medium:

Supply your own main |hpx| entry point while blocking the main thread
=====================================================================

With this method you need to provide an explicit main-thread function named
``hpx_main`` at global scope. This function will be invoked as the main entry
point of your |hpx| application on the console :term:`locality` only (this
function will be invoked as the first |hpx| thread of your application). All
|hpx| API functions can be used from within this function.

The thread executing the function :hpx:func:`hpx::init` will block waiting for
the runtime system to exit. The value returned from ``hpx_main`` will be
returned from :hpx:func:`hpx::init` after the runtime system has stopped.

The function :hpx:func:`hpx::finalize` has to be called on one of the |hpx|
localities in order to signal that all work has been scheduled and the runtime
system should be stopped after the scheduled work has been executed.

This method of invoking |hpx| has the advantage of the user being able to decide
which version of :hpx:func:`hpx::init` to call. This allows to pass
additional configuration parameters while initializing the |hpx| runtime system.

.. code-block:: c++

   #include <hpx/hpx_init.hpp>

   int hpx_main(int argc, char* argv[])
   {
       // Any HPX application logic goes here...
       return hpx::finalize();
   }

   int main(int argc, char* argv[])
   {
       // Initialize HPX, run hpx_main as the first HPX thread, and
       // wait for hpx::finalize being called.
       return hpx::init(argc, argv);
   }

.. note::

   The function ``hpx_main`` does not need to expect receiving ``argc``/``argv``
   as shown above, but could expose one of the following signatures::

       int hpx_main();
       int hpx_main(int argc, char* argv[]);
       int hpx_main(hpx::program_options::variables_map& vm);

   This is consistent with (and extends) the usually allowed prototypes for the
   function ``main()`` in C++ applications.

The header file to include for this method of using |hpx| is
``hpx/hpx_init.hpp``.

There are many additional overloads of :hpx:func:`hpx::init` available, such as
the ability to provide your own entry-point function instead of ``hpx_main``.
Please refer to the function documentation for more details (see: ``hpx/hpx_init.hpp``).

.. _flexible:

Supply your own main |hpx| entry point while avoiding blocking the main thread
==============================================================================

With this method you need to provide an explicit main thread function named
``hpx_main`` at global scope. This function will be invoked as the main entry
point of your |hpx| application on the console :term:`locality` only (this
function will be invoked as the first |hpx| thread of your application). All
|hpx| API functions can be used from within this function.

The thread executing the function :hpx:func:`hpx::start` will *not* block
waiting for the runtime system to exit, but will return immediately.
The function :hpx:func:`hpx::finalize` has to be called on one of the |hpx|
localities in order to signal that all work has been scheduled and the runtime
system should be stopped after the scheduled work has been executed.

This method of invoking |hpx| is useful for applications where the main thread
is used for special operations, such a GUIs. The function :hpx:func:`hpx::stop`
can be used to wait for the |hpx| runtime system to exit and should at least be
used as the last function called in ``main()``. The value returned from
``hpx_main`` will be returned from :hpx:func:`hpx::stop` after the runtime
system has stopped.

.. code-block:: c++

    #include <hpx/hpx_start.hpp>

    int hpx_main(int argc, char* argv[])
    {
        // Any HPX application logic goes here...
        return hpx::finalize();
    }

    int main(int argc, char* argv[])
    {
        // Initialize HPX, run hpx_main.
        hpx::start(argc, argv);

        // ...Execute other code here...

        // Wait for hpx::finalize being called.
        return hpx::stop();
    }

.. note::

   The function ``hpx_main`` does not need to expect receiving ``argc``/``argv``
   as shown above, but could expose one of the following signatures::

       int hpx_main();
       int hpx_main(int argc, char* argv[]);
       int hpx_main(hpx::program_options::variables_map& vm);

   This is consistent with (and extends) the usually allowed prototypes for the
   function ``main()`` in C++ applications.

The header file to include for this method of using |hpx| is
``hpx/hpx_start.hpp``.

There are many additional overloads of :hpx:func:`hpx::start` available, such as
the option for users to provide their own entry point function instead of ``hpx_main``.
Please refer to the function documentation for more details (see:
``hpx/hpx_start.hpp``).

.. _explicit_startup_function:

Supply your own explicit startup function as the main |hpx| entry point
=======================================================================

There is also a way to specify any function (besides ``hpx_main``) to be used as
the main entry point for your |hpx| application:

.. code-block:: c++

   #include <hpx/hpx_init.hpp>

   int application_entry_point(int argc, char* argv[])
   {
       // Any HPX application logic goes here...
       return hpx::finalize();
   }

   int main(int argc, char* argv[])
   {
       // Initialize HPX, run application_entry_point as the first HPX thread,
       // and wait for hpx::finalize being called.
       return hpx::init(&application_entry_point, argc, argv);
   }

.. note::

   The function supplied to :hpx:func:`hpx::init` must have one of the following
   prototypes:

       int application_entry_point(int argc, char* argv[]);
       int application_entry_point(hpx::program_options::variables_map& vm);

.. note::

   If ``nullptr`` is used as the function argument, |hpx| will not run any
   startup function on this locality.

.. _suspend_resume:

Suspending and resuming the |hpx| runtime
=========================================

In some applications it is required to combine |hpx| with other runtimes. To
support this use case, |hpx| provides two functions: :hpx:func:`hpx::suspend` and
:hpx:func:`hpx::resume`. :hpx:func:`hpx::suspend` is a blocking call which will
wait for all scheduled tasks to finish executing and then put the thread pool OS
threads to sleep. :hpx:func:`hpx::resume` simply wakes up the sleeping threads
so that they are ready to accept new work. :hpx:func:`hpx::suspend` and
:hpx:func:`hpx::resume` can be found in the header ``hpx/hpx_suspend.hpp``.

.. code-block:: c++

   #include <hpx/hpx_start.hpp>
   #include <hpx/hpx_suspend.hpp>

   int main(int argc, char* argv[])
   {

      // Initialize HPX, don't run hpx_main
       hpx::start(nullptr, argc, argv);

       // Schedule a function on the HPX runtime
       hpx::post(&my_function, ...);

       // Wait for all tasks to finish, and suspend the HPX runtime
       hpx::suspend();

       // Execute non-HPX code here

       // Resume the HPX runtime
       hpx::resume();

       // Schedule more work on the HPX runtime

       // hpx::finalize has to be called from the HPX runtime before hpx::stop
       hpx::post([]() { hpx::finalize(); });
       return hpx::stop();
   }

.. note::

   :hpx:func:`hpx::suspend` does not wait for :hpx:func:`hpx::finalize` to be
   called. Only call :hpx:func:`hpx::finalize` when you wish to fully stop the
   |hpx| runtime.

.. warning::

   :hpx:func:`hpx::suspend` only waits for local tasks, i.e. tasks on the
    current locality, to finish executing. When using :hpx:func:`hpx::suspend`
    in a multi-locality scenario the user is responsible for ensuring that any
    work required from other localities has also finished.

|hpx| also supports suspending individual thread pools and threads. For details
on how to do that, see the documentation for :hpx:class:`hpx::threads::thread_pool_base`.

Automatically suspending worker threads
---------------------------------------

The previous method guarantees that the worker threads are suspended when you
ask for it and that they stay suspended. An alternative way to achieve the same
effect is to tweak how quickly |hpx| suspends its worker threads when they run
out of work. The following configuration values make sure that |hpx| idles very
quickly:

.. code-block:: ini

   hpx.max_idle_backoff_time = 1000
   hpx.max_idle_loop_count = 0

They can be set on the command line using
``--hpx:ini=hpx.max_idle_backoff_time=1000`` and
``--hpx:ini=hpx.max_idle_loop_count=0``. See :ref:`launching_and_configuring`
for more details on how to set configuration parameters.

After setting idling parameters the previous example could now be written like
this instead:

.. code-block:: c++

   #include <hpx/hpx_start.hpp>

   int main(int argc, char* argv[])
   {

      // Initialize HPX, don't run hpx_main
       hpx::start(nullptr, argc, argv);

       // Schedule some functions on the HPX runtime
       // NOTE: run_as_hpx_thread blocks until completion.
       hpx::run_as_hpx_thread(&my_function, ...);
       hpx::run_as_hpx_thread(&my_other_function, ...);

       // hpx::finalize has to be called from the HPX runtime before hpx::stop
       hpx::post([]() { hpx::finalize(); });
       return hpx::stop();
   }

In this example each call to :hpx:func:`hpx::run_as_hpx_thread` acts as a
"parallel region".

.. _hpx_main_implementation:

Working of ``hpx_main.hpp``
===========================

In order to initialize |hpx| from ``main()``, we make use of linker tricks.

It is implemented differently for different operating systems. The method of
implementation is as follows:

* :ref:`Linux <hpx_main_implementation_linux>`: Using linker ``--wrap`` option.
* :ref:`Mac OSX <hpx_main_implementation_osx>`: Using the linker ``-e`` option.
* :ref:`Windows <hpx_main_implementation_windows>`: Using ``#define main
  hpx_startup::user_main``

.. _hpx_main_implementation_linux:

Linux implementation
--------------------

We make use of the Linux linker ``ld``\ 's ``--wrap`` option to wrap the
``main()`` function. This way any calls to ``main()`` are redirected to our own
implementation of main. It is here that we check for the existence of
``hpx_main.hpp`` by making use of a shadow variable ``include_libhpx_wrap``. The
value of this variable determines the function stack at runtime.

The implementation can be found in ``libhpx_wrap.a``.

.. important::

   It is necessary that ``hpx_main.hpp`` be not included more than once.
   Multiple inclusions can result in multiple definition of
   ``include_libhpx_wrap``.

.. _hpx_main_implementation_osx:

Mac OSX implementation
----------------------

Here we make use of yet another linker option ``-e`` to change the entry point
to our custom entry function ``initialize_main``. We initialize the |hpx|
runtime system from this function and call main from the initialized system. We
determine the function stack at runtime by making use of the shadow variable
``include_libhpx_wrap``.

The implementation can be found in ``libhpx_wrap.a``.

.. important::

   It is necessary that ``hpx_main.hpp`` be not included more than once.
   Multiple inclusions can result in multiple definition of
   ``include_libhpx_wrap``.

.. _hpx_main_implementation_windows:

Windows implementation
----------------------

We make use of a macro ``#define main hpx_startup::user_main`` to take care of
the initializations.

This implementation could result in unexpected behaviors.
