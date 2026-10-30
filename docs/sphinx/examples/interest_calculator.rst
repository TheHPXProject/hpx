..
    Copyright (C) 2012 Adrian Serio
    Copyright (C) 2012 Vinay C Amatya
    Copyright (C) 2015 Hartmut Kaiser

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _examples_interest_calculator:

========
Dataflow
========

|hpx| provides its users with several different tools to simply express parallel
concepts. One of these tools is a :term:`local control object` (:term:`LCO`)
called dataflow. An :term:`LCO` is a type of component that can spawn a new
thread when triggered. They are also distinguished from other components by a
standard interface that allow users to understand and use them easily.
A Dataflow, being an :term:`LCO`, is triggered when the values it depends on
become available. For instance, if you have a calculation X that depends on the
results of three other calculations, you could set up a dataflow that would begin
the calculation X as soon as the other three calculations have returned their
values. Dataflows are set up to depend on other dataflows. It is this property
that makes dataflow a powerful parallelization tool. If you understand the
dependencies of your calculation, you can devise a simple algorithm that sets
up a dependency tree to be executed. In this example, we calculate compound
interest. To calculate compound interest, one must calculate the interest made
in each compound period, and then add that interest back to the principal before
calculating the interest made in the next period. A practical person would, of
course, use the formula for compound interest:

.. math::

   F = P(1 + i) ^ n

where :math:`F` is the future value, :math:`P` is the principal value, :math:`i`
is the interest rate, and :math:`n` is the number of compound periods.

However, for the sake of this example, we have chosen to manually calculate the
future value by iterating:

.. math::

   I = Pi

and

.. math::

   P = P + I

Setup
=====

The source code for this example can be found here:
:download:`interest_calculator.cpp
<../../examples/quickstart/interest_calculator.cpp>`.

To compile this program, go to your |hpx| build directory (see
:ref:`building_hpx` for information on configuring and building |hpx|) and
enter:

.. code-block:: shell-session

   $ make examples.quickstart.interest_calculator

To run the program type:

.. code-block:: shell-session

   $ ./bin/interest_calculator --principal 100 --rate 5 --cp 6 --time 36
   Final amount: 134.01
   Amount made: 34.0096

Walkthrough
===========

Let us begin with main. Here we can see that we again are using
|boost_program_options| to set our command line variables (see
:ref:`examples_fibonacci` for more details). These options set the principal,
rate, compound period, and time. It is important to note that the units of time
for ``cp`` and ``time`` must be the same.

.. literalinclude:: ../../examples/quickstart/interest_calculator.cpp
   :language: c++
   :start-after: //[interest_main
   :end-before: //]

Next we look at hpx_main.

.. literalinclude:: ../../examples/quickstart/interest_calculator.cpp
   :language: c++
   :start-after: //[interest_hpx_main
   :end-before: //]


Here we find our command line variables read in, the rate is converted from a
percent to a decimal, the number of calculation iterations is determined, and
then our shared_futures are set up. Notice that we first place our principal and
rate into shares futures by passing the variables ``init_principal`` and
``init_rate`` using :hpx:func:`hpx::make_ready_future`.

In this way :hpx:class:`hpx::shared_future`\ ``<double>`` ``principal``
and ``rate`` will be initialized to ``init_principal`` and ``init_rate`` when
:hpx:func:`hpx::make_ready_future`\ ``<double>`` returns a future containing
those initial values. These shared futures then enter the for loop and are
passed to ``interest``. Next ``principal`` and ``interest`` are passed to the
reassignment of ``principal`` using a :hpx:func:`hpx::dataflow`. A dataflow
will first wait for its arguments to be ready before launching any callbacks, so
``add`` in this case will not begin until both ``principal`` and ``interest``
are ready. This loop continues for each compound period that must be calculated.
To see how ``interest`` and ``principal`` are calculated in the loop, let us look
at ``calc_action`` and ``add_action``:

.. literalinclude:: ../../examples/quickstart/interest_calculator.cpp
   :language: c++
   :start-after: //[interest_calc_add_action
   :end-before: //]

After the shared future dependencies have been defined in hpx_main, we see the
following statement:

.. code-block:: c++

   double result = principal.get();

This statement calls :hpx:member:`hpx::future::get` on the shared future
principal which had its value calculated by our for loop. The program will wait
here until the entire dataflow tree has been calculated and the value assigned
to result. The program then prints out the final value of the investment and the
amount of interest made by subtracting the final value of the investment from
the initial value of the investment.
