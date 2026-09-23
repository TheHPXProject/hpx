..
    Copyright (C) 2021 Dimitra Karatza

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _examples_matrix_multiplication:

===================
Parallel algorithms
===================

This program will perform a matrix multiplication in parallel. The output will look something like this:

.. code-block:: text

   Matrix A is :
   4 9 6
   1 9 8

   Matrix B is :
   4 9
   6 1
   9 8

   Resultant Matrix is :
   124 93
   130 82

Setup
=====

The source code for this example can be found here:
:download:`matrix_multiplication.cpp
<../../examples/quickstart/matrix_multiplication.cpp>`.

To compile this program, go to your |hpx| build directory (see
:ref:`building_hpx` for information on configuring and building |hpx|) and
enter:

.. code-block:: shell-session

   $ make examples.quickstart.matrix_multiplication

To run the program type:

.. code-block:: shell-session

   $ ./bin/matrix_multiplication

or:

.. code-block:: shell-session

   $ ./bin/matrix_multiplication --n 2 --m 3 --k 2 --s 100 --l 0 --u 10

where the first matrix is `n` x `m` and the second `m` x `k`, s is the seed for creating the random values of
the matrices and the range of these values is [l,u]

This should print:

.. code-block:: text

   Matrix A is :
   4 9 6
   1 9 8

   Matrix B is :
   4 9
   6 1
   9 8

   Resultant Matrix is :
   124 93
   130 82

Notice that the numbers may be different because of the random initialization of the matrices.

Walkthrough
===========

Now that you have compiled and run the code, let's look at how the code works.

First, ``main()`` is used to initialize the runtime system and pass the command line arguments to the program.
``hpx::init`` calls ``hpx_main()`` after setting up HPX, which is where our program is implemented.

.. literalinclude:: ../../examples/quickstart/matrix_multiplication.cpp
   :language: c++
   :start-after: //[mul_main
   :end-before: //]

Proceeding to the ``hpx_main()`` function, we can see that matrix multiplication can be done very easily.

.. literalinclude:: ../../examples/quickstart/matrix_multiplication.cpp
   :language: c++
   :start-after: //[mul_hpx_main
   :end-before: //]

First, the dimensions of the matrices are defined. If they were not given as command-line arguments, their default
values are `2` x `3` for the first matrix and `3` x `2` for the second. We use standard vectors to define the matrices
to be multiplied as well as the resultant matrix.

To give some random initial values to our matrices, we use `std::uniform_int_distribution
<https://en.cppreference.com/w/cpp/numeric/random/uniform_int_distribution>`_. Then, ``std::bind()`` is used
along with ``hpx::ranges::generate()`` to yield two matrices A and B, which contain values in the range of [0, 10] or in
the range defined by the user at the command-line arguments. The seed to generate the values can also be defined by the user.

The next step is to perform the matrix multiplication in parallel. This can be done by just using an :hpx:func:`\hpx::experimental::for_loop`
combined with a parallel execution policy ``hpx::execution::par`` as the outer loop of the multiplication. Note that the execution
of :hpx:func:`\hpx::experimental::for_loop` without specifying an execution policy is equivalent to specifying ``hpx::execution::seq``
as the execution policy.

Finally, the matrices A, B that are multiplied as well as the resultant matrix R are printed using the following function.

.. literalinclude:: ../../examples/quickstart/matrix_multiplication.cpp
   :language: c++
   :start-after: //[mul_print_matrix
   :end-before: //]
