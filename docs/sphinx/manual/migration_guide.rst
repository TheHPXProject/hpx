..
    Copyright (c) 2021 Dimitra Karatza

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _migration_guide:

===============
Migration guide
===============

The Migration Guide serves as a valuable resource for developers seeking to transition their
parallel computing applications from different APIs (i.e. |openmp|, |tbb|, |mpi|) to |hpx|. |hpx|, an
advanced C++ library, offers a versatile and high-performance platform for parallel and distributed
computing, providing a wide range of features and capabilities. This guide aims to assist developers
in understanding the key differences between different APIs and |hpx|, and it provides step-by-step
instructions for converting code to |hpx| code effectively.

Some general steps that can be used to migrate code to |hpx| code are the following:

1. Install |hpx| using the :ref:`quickstart` guide.

2. Include the |hpx| header files:

   Add the necessary header files for HPX at the beginning of your code, such as:

    .. code-block:: c++

        #include <hpx/init.hpp>

3. Replace your code with |hpx| code using the guide that follows.

4. Use HPX-specific features and APIs:

   |hpx| provides additional features and APIs that can be used to take advantage of the library's
   capabilities. For example, you can use the HPX asynchronous execution to express fine-grained
   tasks and dependencies, or utilize HPX's distributed computing features for distributed memory systems.

5. Compile and run the |hpx| code:

   Compile the converted code with the |hpx| library and run it using the appropriate HPX runtime environment.

.. _openmp:

|openmp|
========

The |openmp| API supports multi-platform shared-memory parallel programming in C/C++. Typically it is used for
loop-level parallelism, but it also supports function-level parallelism. Below are some examples on how to
convert |openmp| to |hpx| code:

|openmp| parallel for loop
--------------------------

Parallel for loop
^^^^^^^^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    #pragma omp parallel for
    for (int i = 0; i < n; ++i) {
        // loop body
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>

    hpx::experimental::for_loop(hpx::execution::par, 0, n, [&](int i) {
        // loop body
    });


In the above code, the |openmp| `#pragma omp parallel for` directive is replaced with
:cpp:func:`hpx::experimental::for_loop` from the |hpx| library. The loop body within the lambda
function will be executed in parallel for each iteration.

Private variables
^^^^^^^^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    int x = 0;

    #pragma omp parallel for private(x)
    for (int i = 0; i < n; ++i) {
        // loop body
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>

    hpx::experimental::for_loop(hpx::execution::par, 0, n, [&](int i) {
            int x = 0; // Declare 'x' as a local variable inside the loop body
            // loop body
    });

The variable `x` is declared as a local variable inside the loop body, ensuring that
it is private to each thread.


Shared variables
^^^^^^^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    int x = 0;

    #pragma omp parallel for shared(x)
    for (int i = 0; i < n; ++i) {
        // loop body
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>

    std::atomic<int> x = 0; // Declare 'x' as a shared variable outside the loop

    hpx::experimental::for_loop(hpx::execution::par, 0, n, [&](int i) {
        // loop body
    });

To ensure variable `x` is shared among all threads, you simply have to declare it as an
atomic variable outside the `for_loop`.

Number of threads
^^^^^^^^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    #pragma omp parallel for num_threads(2)
    for (int i = 0; i < n; ++i) {
        // loop body
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>
    #include <hpx/execution.hpp>

    hpx::execution::experimental::num_cores nc(2);

    hpx::experimental::for_loop(hpx::execution::par.with(nc), 0, n, [&](int i) {
        // loop body
    });


To declare the number of threads to be used for the parallel region, you can use
`hpx::execution::experimental::num_cores` and pass the number of cores (`nc`) to
:cpp:func:`hpx::experimental::for_loop` using `hpx::execution::par.with(nc)`.
This example uses 2 threads for the parallel loop.

Reduction
^^^^^^^^^

|openmp| code:

.. code-block:: c++

    int s = 0;

    #pragma omp parallel for reduction(+: s)
    for (int i = 0; i < n; ++i) {
        s += i;
        // loop body
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>
    #include <hpx/execution.hpp>

    int s = 0;

    hpx::experimental::for_loop(hpx::execution::par, 0, n, reduction(s, 0, plus<>()), [&](int i, int& accum) {
        accum += i;
        // loop body
    });


The reduction clause specifies that the variable `s` should be reduced across iterations using the `plus<>` operation.
It initializes `s` to `0` at the beginning of the loop and accumulates the values of `s` from each iteration using the
`+` operator. The lambda function representing the loop body takes two parameters: `i`, which represents the loop index,
and `accum`, which is the reduction variable `s`. The lambda function is executed for each iteration of the loop.
The reduction ensures that the `accum` value is correctly accumulated across different iterations and threads.

Schedule
^^^^^^^^

|openmp| code:

.. code-block:: c++

    int s = 0;

    // static scheduling with chunk size 1000
    #pragma omp parallel for schedule(static, 1000)
    for (int i = 0; i < n; ++i) {
        // loop body
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>
    #include <hpx/execution.hpp>

    hpx::execution::experimental::static_chunk_size cs(1000);

    hpx::experimental::for_loop(hpx::execution::par.with(cs), 0, n, [&](int i) {
        // loop body
    });

To define the scheduling type, you can use the corresponding execution policy from
`hpx::execution::experimental`, define the chunk size (cs, here declared as 1000) and pass
it to the to :cpp:func:`hpx::experimental::for_loop` using `hpx::execution::par.with(cs)`.

Accordingly, other types of scheduling are available and can be used in a similar manner:

.. code-block:: c++

    #include <hpx/execution.hpp>
    hpx::execution::experimental::dynamic_chunk_size cs(1000);

.. code-block:: c++

    #include <hpx/execution.hpp>
    hpx::execution::experimental::guided_chunk_size cs(1000);

.. code-block:: c++

    #include <hpx/execution.hpp>
    hpx::execution::experimental::auto_chunk_size cs(1000);


|openmp| single thread
----------------------

|openmp| code:

.. code-block:: c++

    {   // parallel code
        #pragma omp single
        {
            // single-threaded code
        }
        // more parallel code
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/mutex.hpp>

    hpx::mutex mtx;

    {   // parallel code
        {   // single-threaded code
            std::scoped_lock l(mtx);
        }
        // more parallel code
    }

To make sure that only one thread accesses a specific code within a parallel section
you can use :cpp:class:`hpx::mutex` and `std::scoped_lock` to take ownership of the given
mutex `mtx`. For more information about mutexes please refer to :ref:`mutex`.

|openmp| tasks
--------------

Simple tasks
^^^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    // executed asynchronously by any available thread
    #pragma omp task
    {
        // task code
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>

    auto future = hpx::async([](){
        // task code
    });

or

.. code-block:: c++

    #include <hpx/future.hpp>

    hpx::post([](){
        // task code
    }); // fire and forget

The tasks in |hpx| can be defined simply by using the :cpp:func:`async` function and passing as argument
the code you wish to run asynchronously. Another alternative is to use :cpp:func:`post` which is a
fire-and-forget method.

.. tip::

    If you think you will like to synchronize your tasks later on, we suggest you use
    :cpp:func:`hpx::async` which provides synchronization options, while :cpp:func:`hpx::post`
    explicitly states that there is no return value or way to synchronize with the function
    execution. Synchronization options are listed below.

Task wait
^^^^^^^^^

|openmp| code:

.. code-block:: c++

    #pragma omp task
    {
        // task code
    }

    #pragma omp taskwait
    // code after completion of task

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>

    hpx::async([](){
        // task code
    }).get(); // wait for the task to complete

    // code after completion of task

The `get()` function can be used to ensure that the task created with :cpp:func:`hpx::async`
is completed before the code continues executing beyond that point.

Multiple tasks synchronization
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    #pragma omp task
    {
        // task 1 code
    }

    #pragma omp task
    {
        // task 2 code
    }

    #pragma omp taskwait
    // code after completion of both tasks 1 and 2

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>

    auto future1 = hpx::async([](){
        // task 1 code
    });

    auto future2 = hpx::async([](){
        // task 2 code
    });

    auto future = hpx::when_all(future1, future2).then([](auto&&){
        // code after completion of both tasks 1 and 2
    });


If you would like to synchronize multiple tasks, you can use the :cpp:func:`hpx::when_all` function
to define which futures have to be ready and the `then()` function to declare what should
be executed once these futures are ready.


Dependencies
^^^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    int a = 10;
    int b = 20;
    int c = 0;

    #pragma omp task depend(in: a, b) depend(out: c)
    {
        // task code
        c = 100;
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>

    int a = 10;
    int b = 20;
    int c = 0;

    // Create a future representing 'a'
    auto future_a = hpx::make_ready_future(a);

    // Create a future representing 'b'
    auto future_b = hpx::make_ready_future(b);

    // Create a task that depends on 'a' and 'b' and executes 'task_code'
    auto future_c = hpx::dataflow(
        []() {
            // task code
            return 100;
        },
        future_a, future_b);

    c = future_c.get();

If one of the arguments of :cpp:func:`hpx::dataflow` is a future, then it will wait for the
future to be ready to launch the thread. Hence, to define the dependencies of tasks
you have to create futures representing the variables that create dependencies and pass
them as arguments to :cpp:func:`hpx::dataflow`. `get()` is used to save the result of the future
to the desired variable.


Nested tasks
^^^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    #pragma omp task
    {
        // Outer task code
        #pragma omp task
        {
            // Inner task code
        }
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>

    auto future_outer = hpx::async([](){
        // Outer task code

        hpx::async([](){
            // Inner task code
        });
    });

or

.. code-block:: c++

    #include <hpx/future.hpp>

    auto future_outer = hpx::post([](){ // fire and forget
        // Outer task code

        hpx::post([](){ // fire and forget
            // Inner task code
        });
    });

If you have nested tasks, you can simply use nested :cpp:func:`hpx::async` or
:cpp:func:`hpx::post` calls. The implementation is similar if you want to take
care of synchronization:

|openmp| code:

.. code-block:: c++

    #pragma omp taskwait
    {
        // Outer task code
        #pragma omp taskwait
        {
            // Inner task code
        }
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>

    auto future_outer = hpx::async([]() {
        // Outer task code

        hpx::async([]() {
            // Inner task code
        }).get();    // Wait for the inner task to complete
    });

    future_outer.get();    // Wait for the outer task to complete

Task yield
^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    #pragma omp task
    {
        // code before yielding
        #pragma omp taskyield
        // code after yielding
    }

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>
    #include <hpx/thread.hpp>

    auto future = hpx::async([](){
        // code before yielding
    });

    // yield execution to potentially allow other tasks to run
    hpx::this_thread::yield();

    // code after yielding

After creating a task using :cpp:func:`hpx::async`, :cpp:func:`hpx::this_thread::yield`
can be used to reschedule the execution of threads, allowing other threads to run.

Task group
^^^^^^^^^^

|openmp| code:

.. code-block:: c++

    #pragma omp taskgroup
    {
        #pragma omp task
        {
            // task 1 code
        }

        #pragma omp task
        {
            // task 2 code
        }
    }


|hpx| equivalent:

.. code-block:: c++

    #include <hpx/task_group.hpp>

    // Declare a task group
    hpx::experimental::task_group tg;

    // Run the tasks
    tg.run([](){
        // task 1 code
    });
    tg.run(
        // task 2 code
    });

    // Wait for the task group
    tg.wait();

To create task groups, you can use :cpp:class:`hpx::experimental::task_group`. The function
`run()` can be used to run each task within the task group, while `wait()` can be used to
achieve synchronization. If you do not care about waiting for the task group to complete
its execution, you can simply remove the `wait()` function.

|openmp| sections
-----------------

|openmp| code:

.. code-block:: c++

    #pragma omp sections
    {
        #pragma omp section
        // section 1 code
        #pragma omp section
        // section 2 code
    } // implicit synchronization


|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>

    auto future_section1 = hpx::async([](){
        // section 1 code
    });
    auto future_section2 = hpx::async([](){
        // section 2 code
    );

    // synchronization: wait for both sections to complete
    hpx::wait_all(future_section1, future_section2);

Unlike tasks, there is an implicit synchronization barrier at the end of each `sections`
directive in |openmp|. This synchronization is achieved using :cpp:func:`hpx::wait_all` function.

.. note::

    If the `nowait` clause is used in the `sections` directive, then you can just remove
    the :cpp:func:`hpx::wait_all` function while keeping the rest of the code as it is.

.. _tbb:

|tbb|
=====

|tbb| provides a high-level interface for parallelism and concurrent programming using
standard ISO C++ code. Below are some examples on how to convert |tbb| to |hpx| code:

parallel_for
------------

|tbb| code:

.. code-block:: c++

    auto values = std::vector<double>(10000);

    tbb::parallel_for( tbb::blocked_range<int>(0,values.size()),
                        [&](tbb::blocked_range<int> r)
    {
        for (int i=r.begin(); i<r.end(); ++i)
        {
            // loop body
        }
    });


|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>

    auto values = std::vector<double>(10000);

    hpx::experimental::for_loop(hpx::execution::par, 0, values.size(), [&](int i) {
        // loop body
    });


In the above code, `tbb::parallel_for` is replaced with :cpp:func:`hpx::experimental::for_loop`
from the |hpx| library. The loop body within the lambda function will be executed in
parallel for each iteration.

parallel_for_each
-----------------

|tbb| code:

.. code-block:: c++

    auto values = std::vector<double>(10000);

    tbb::parallel_for_each(values.begin(), values.end(), [&](){
        // loop body
    });


|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>

    auto values = std::vector<double>(10000);

    hpx::for_each(hpx::execution::par, values.begin(), values.end(), [&](){
        // loop body
    });

By utilizing :cpp:func:`hpx::for_each` and specifying a parallel execution policy with
`hpx::execution::par`, it is possible to transform `tbb::parallel_for_each` into its
equivalent counterpart in |hpx|.

parallel_invoke
---------------

|tbb| code:

.. code-block:: c++

    tbb::parallel_invoke(task1, task2, task3);

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/future.hpp>

    hpx::wait_all(hpx::async(task1), hpx::async(task2), hpx::async(task3));

To convert `tbb::parallel_invoke` to |hpx|, we use :cpp:func:`hpx::async` to asynchronously
execute each task, which returns a future representing the result of each task.
We then pass these futures to :cpp:func:`hpx::when_all`, which waits for all the futures
to complete before returning.


parallel_pipeline
-----------------

|tbb| code:

.. code-block:: c++

    tbb::parallel_pipeline(4,
        tbb::make_filter<void, int>(tbb::filter::serial_in_order,
            [](tbb::flow_control& fc) -> int {
                // Generate numbers from 1 to 10
                static int i = 1;
                if (i <= 10) {
                    return i++;
                }
                else {
                    fc.stop();
                    return 0;
                }
            }) &
        tbb::make_filter<int, int>(tbb::filter::parallel,
            [](int num) -> int {
                // Multiply each number by 2
                return num * 2;
            }) &
        tbb::make_filter<int, void>(tbb::filter::serial_in_order,
            [](int num) {
                // Print the results
                std::cout << num << " ";
            })
    );

|hpx| equivalent:

.. code-block:: c++

    #include <iostream>
    #include <vector>
    #include <ranges>
    #include <hpx/algorithm.hpp>

    // generate the values
    auto range = std::views::iota(1) | std::views::take(10);

    // materialize the output vector
    std::vector<int> results(10);

    // in parallel execution of pipeline and transformation
    hpx::ranges::transform(
        hpx::execution::par, range, result.begin(), [](int i) { return 2 * i; });

    // print the modified vector
    for (int i : result)
    {
        std::cout << i << " ";
    }
    std::cout << std::endl;

The line `auto range = std::views::iota(1) | std::views::take(10);` generates a range of values using
the `std::views::iota` function. It starts from the value 1 and generates an infinite sequence of
incrementing values. The `std::views::take(10)` function is then applied to limit the sequence to the
first 10 values. The result is stored in the `range` variable.

.. hint::

    A view is a lightweight object that represents a particular view of a sequence or range. It acts as
    a read-only interface to the original data, providing a way to query and traverse the elements
    without making any copies or modifications.

    Views can be composed and chained together to form complex pipelines of operations. These operations
    are evaluated lazily, meaning that the actual computation is performed only when the result is
    needed or consumed.

Since views perform lazy evaluation, we use `std::vector<int> results(10);` to meterialize the vector
that will store the transformed values. The `hpx::ranges::transform` function is then used to perform
a parallel transformation on the range. The transformed values will be written to the `results` vector.

.. hint::

    Ranges enable loop fusion by combining multiple operations into a single parallel loop, eliminating
    waiting time and reducing overhead. Using ranges, you can express these operations as a pipeline of
    transformations on a sequence of elements. This pipeline is evaluated in a single pass, performing
    all the desired operations in parallel without the need to wait between them.

    In addition, |hpx| enhances the benefits of range fusion by offering parallel execution policies,
    which can be used to optimize the execution of the fused loop across multiple threads.


parallel_reduce
---------------

Reduction
^^^^^^^^^

|tbb| code:

.. code-block:: c++

    auto values = std::vector<double>{1,2,3,4,5,6,7,8,9};

    auto total = tbb::parallel_reduce(
                    tbb::blocked_range<int>(0,values.size()),
                    0.0,
                    [&](tbb::blocked_range<int> r, double running_total)
                    {
                        for (int i=r.begin(); i<r.end(); ++i)
                        {
                            running_total += values[i];
                        }


                        return running_total;
                    },
                    std::plus<double>());

|hpx| equivalent:

.. code-block:: c++

    #include <hpx/numeric.hpp>

    auto values = std::vector<double>{1,2,3,4,5,6,7,8,9};

    auto total = hpx::reduce(
        hpx::execution::par, values.begin(), values.end(), 0, std::plus{});

By utilizing :cpp:func:`hpx::reduce` and specifying a parallel execution policy with
`hpx::execution::par`, it is possible to transform `tbb::parallel_reduce` into its
equivalent counterpart in |hpx|. As demonstrated in the previous example, the management
of intermediate results is seamlessly handled internally by |hpx|, eliminating the need
for explicit consideration.

Transformation & Reduction
^^^^^^^^^^^^^^^^^^^^^^^^^^

|tbb| code:

.. code-block:: c++

    auto values = std::vector<double>{1,2,3,4,5,6,7,8,9};

    auto transform_function(double current_value){
        // transformation code
    }

    auto total = tbb::parallel_reduce(
                    tbb::blocked_range<int>(0,values.size()),
                    0.0,
                    [&](tbb::blocked_range<int> r, double transformed_val)
                    {
                        for (int i=r.begin(); i<r.end(); ++i)
                        {
                            transformed_val += transform_function(values[i]);
                        }
                        return transformed_val;
                    },
                    std::plus<double>());


|hpx| equivalent:

.. code-block:: c++

    #include <hpx/numeric.hpp>

    auto values = std::vector<double>{1,2,3,4,5,6,7,8,9};

    auto transform_function(double current_value)
    {
        // transformation code
    }

    auto total = hpx::transform_reduce(hpx::execution::par, values.begin(),
        values.end(), 0, std::plus{},
        [&](double current_value) { return transform_function(current_value); });

In situations where certain values require transformation before the reduction process,
|hpx| provides a straightforward solution through :cpp:func:`hpx::transform_reduce`. The
`transform_function()` allows for the application of the desired transformation to each value.

parallel_scan
-------------

|tbb| code:

.. code-block:: c++

    tbb::parallel_scan(tbb::blocked_range<size_t>(0, input.size()),
        0,
        [&input, &output](const tbb::blocked_range<size_t>& range, int& partial_sum, bool is_final_scan) {
            for (size_t i = range.begin(); i != range.end(); ++i) {
                partial_sum += input[i];
                if (is_final_scan) {
                    output[i] = partial_sum;
                }
            }
            return partial_sum;
        },
        [](int left_sum, int right_sum) {
            return left_sum + right_sum;
        }
    );


|hpx| equivalent:

.. code-block:: c++

    #include <hpx/numeric.hpp>

    hpx::inclusive_scan(hpx::execution::par, input.begin(), input.end(),
        output.begin(),
        [](const int& left, const int& right) { return left + right; });

:cpp:func:`hpx::inclusive_scan` with `hpx::execution::par` as execution policy
can be used to perform a prefix scan in parallel. The management of intermediate
results is seamlessly handled internally by |hpx|, eliminating the need
for explicit consideration. `input.begin()` and `input.end()` refer to the beginning
and end of the sequence of elements the algorithm will be applied to respectively.
`output.begin()` refers to the beginning of the destination, while the last argument
specifies the function which will be invoked for each of the values of the input sequence.

.. seealso::

Apart from :cpp:func:`hpx::inclusive_scan`, |hpx| provides its users with :cpp:func:`hpx::exclusive_scan`.
The key difference between inclusive scan and exclusive scan lies in the treatment of the current element
during the scan operation. In an inclusive scan, each element in the output sequence includes the
contribution of the corresponding element in the input sequence, while in an exclusive scan, the current
element in the input sequence does not contribute to the corresponding element in the output sequence.


parallel_sort
-------------

|tbb| code:

.. code-block:: c++

    std::vector<int> numbers = {9, 2, 7, 1, 5, 3};

    tbb::parallel_sort(numbers.begin(), numbers.end());


|hpx| equivalent:

.. code-block:: c++

    #include <hpx/algorithm.hpp>

    std::vector<int> numbers = {9, 2, 7, 1, 5, 3};

    hpx::sort(hpx::execution::par, numbers.begin(), numbers.end());

:cpp:func:`hpx::sort` provides an equivalent functionality to `tbb::parallel_sort`.
When given a parallel execution policy with `hpx::execution::par`, the algorithm employs
parallel execution, allowing for efficient sorting across available threads.

task_group
----------

|tbb| code:

.. code-block:: c++

    // Declare a task group
    tbb::task_group tg;

    // Run the tasks
    tg.run(task1);
    tg.run(task2);

    // Wait for the task group
    tg.wait();


|hpx| equivalent:

.. code-block:: c++

    #include <hpx/task_group.hpp>

    // Declare a task group
    hpx::experimental::task_group tg;

    // Run the tasks
    tg.run(task1);
    tg.run(task2);

    // Wait for the task group
    tg.wait();

|hpx| drew inspiration from |tbb| to introduce the :cpp:func:`hpx::experimental::task_group`
feature. Therefore, utilizing :cpp:func:`hpx::experimental::task_group` provides an
equivalent functionality to `tbb::task_group`.

.. _mpi:

|mpi|
=====

|mpi| is a standardized communication protocol and library that allows multiple processes or
nodes in a parallel computing system to exchange data and coordinate their execution.

List of |mpi|-|hpx| functions
-----------------------------

   .. table:: |hpx| equivalent functions of |mpi|

   ========================================  ===================================================================================================================
   |mpi| function                            |hpx| equivalent
   ========================================  ===================================================================================================================
   :ref:`MPI_Allgather`                      :cpp:class:`hpx::collectives::all_gather`
   :ref:`MPI_Allreduce`                      :cpp:class:`hpx::collectives::all_reduce`
   :ref:`MPI_Alltoall`                       :cpp:class:`hpx::collectives::all_to_all`
   :ref:`MPI_Barrier`                        :cpp:class:`hpx::distributed::barrier`
   :ref:`MPI_Bcast`                          :cpp:class:`hpx::collectives::broadcast_to()` and :cpp:class:`hpx::collectives::broadcast_from()` used with :code:`get()`
   :ref:`MPI_Comm_size <MPI_Send_MPI_Recv>`  :cpp:class:`hpx::get_num_localities`
   :ref:`MPI_Comm_rank <MPI_Send_MPI_Recv>`  :cpp:class:`hpx::get_locality_id()`
   :ref:`MPI_Exscan`                         :cpp:class:`hpx::collectives::exclusive_scan()` used with :code:`get()`
   :ref:`MPI_Gather`                         :cpp:class:`hpx::collectives::gather_here()` and :cpp:class:`hpx::collectives::gather_there()` used with :code:`get()`
   :ref:`MPI_Irecv <MPI_Send_MPI_Recv>`      :cpp:class:`hpx::collectives::get()`
   :ref:`MPI_Isend <MPI_Send_MPI_Recv>`      :cpp:class:`hpx::collectives::set()`
   :ref:`MPI_Reduce`                         :cpp:class:`hpx::collectives::reduce_here` and :cpp:class:`hpx::collectives::reduce_there` used with :code:`get()`
   :ref:`MPI_Scan`                           :cpp:class:`hpx::collectives::inclusive_scan()` used with :code:`get()`
   :ref:`MPI_Scatter`                        :cpp:class:`hpx::collectives::scatter_to()` and :cpp:class:`hpx::collectives::scatter_from()`
   :ref:`MPI_Wait <MPI_Send_MPI_Recv>`       :cpp:class:`hpx::collectives::get()` used with a future i.e. :code:`setf.get()`
   ========================================  ===================================================================================================================

.. _MPI_Send_MPI_Recv:

MPI_Send & MPI_Recv
-------------------

Let's assume we have the following simple message passing code where each process sends a
message to the next process in a circular manner. The exchanged message is modified and printed
to the console.

|mpi| code:

.. code-block:: c++

    #include <cstddef>
    #include <cstdint>
    #include <iostream>
    #include <mpi.h>
    #include <vector>

    constexpr int times = 2;

    int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int num_localities;
    MPI_Comm_size(MPI_COMM_WORLD, &num_localities);

    int this_locality;
    MPI_Comm_rank(MPI_COMM_WORLD, &this_locality);

    int next_locality = (this_locality + 1) % num_localities;
    std::vector<int> msg_vec = {0, 1};

    int cnt = 0;
    int msg = msg_vec[this_locality];

    int recv_msg;
    MPI_Request request_send, request_recv;
    MPI_Status status;

    while (cnt < times) {
        cnt += 1;

        MPI_Isend(&msg, 1, MPI_INT, next_locality, cnt, MPI_COMM_WORLD,
                &request_send);
        MPI_Irecv(&recv_msg, 1, MPI_INT, next_locality, cnt, MPI_COMM_WORLD,
                &request_recv);

        MPI_Wait(&request_send, &status);
        MPI_Wait(&request_recv, &status);

        std::cout << "Time: " << cnt << ", Locality " << this_locality
                << " received msg: " << recv_msg << "\n";

        recv_msg += 10;
        msg = recv_msg;
    }

    MPI_Finalize();
    return 0;
    }

|hpx| equivalent:

.. literalinclude:: ../../libs/full/collectives/examples/channel_communicator.cpp
   :start-after: //[doc
   :end-before: //doc]

To perform message passing between different processes in |hpx| we can use a channel communicator.
To understand this example, let's focus on the `hpx_main()` function:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.
- `create_channel_communicator` function is used to create a channel to serve the communication.
  This function takes several arguments, including the launch policy (`hpx::launch::sync`), the
  name of the communicator (`channel_communicator_name`), the number of localities, and the ID
  of the current locality.
- The communication follows a ring pattern, where each process (or locality) sends a message to
  its neighbor in a circular manner. This means that the messages circulate around the localities,
  ensuring that the communication wraps around when reaching the end of the locality sequence.
  To achieve this, the `next_locality` variable is calculated as the ID of the next locality in
  the ring.
- The initial values for the communication are set (`msg_vec`, `cnt`, `msg`).
- The `set()` function is called to send the message to the next locality in the ring. The message
  is sent asynchronously and is associated with a tag (`cnt`).
- The `get()` function is called to receive a message from the next locality. It is also associated
  with the same tag as the `set()` operation.
- The `setf.get()` call blocks until the message sending operation is complete.
- A continuation is set up using the function `then()` to handle the received message.
  Inside the continuation:

  - The received message value (`rec_msg`) is retrieved using `f.get()`.

  - The received message is printed to the console and then modified by adding 10.

  - The `set()` and `get()` operations are repeated to send and receive the modified message to
    the next locality.

  - The `setf.get()` call blocks until the new message sending operation is complete.
- The `done_msg.get()` call blocks until the continuation is complete for the current loop iteration.

Having said that, we conclude to the following table:

.. _MPI_Gather:

MPI_Gather
----------

The following code gathers data from all processes to the root process and verifies
the gathered data in the root process.

|mpi| code:

.. code-block:: c++

    #include <iostream>
    #include <mpi.h>
    #include <numeric>
    #include <vector>

    int main(int argc, char *argv[]) {
        MPI_Init(&argc, &argv);

        int num_localities, this_locality;
        MPI_Comm_size(MPI_COMM_WORLD, &num_localities);
        MPI_Comm_rank(MPI_COMM_WORLD, &this_locality);

        std::vector<int> local_data; // Data to be gathered

        if (this_locality == 0) {
            local_data.resize(num_localities); // Resize the vector on the root process
        }

        // Each process calculates its local data value
        int my_data = 42 + this_locality;

        for (std::uint32_t i = 0; i != 10; ++i) {

            // Gather data from all processes to the root process (process 0)
            MPI_Gather(&my_data, 1, MPI_INT, local_data.data(), 1, MPI_INT, 0,
                    MPI_COMM_WORLD);

            // Only the root process (process 0) will print the gathered data
            if (this_locality == 0) {
            std::cout << "Gathered data on the root: ";
            for (int i = 0; i < num_localities; ++i) {
                std::cout << local_data[i] << " ";
            }
            std::cout << std::endl;
            }
        }
        std::cout << std::endl;

        MPI_Finalize();
        return 0;
    }


|hpx| equivalent:

.. code-block:: c++

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t this_locality = hpx::get_locality_id();

    // test functionality based on immediate local result value
    auto gather_direct_client = create_communicator(gather_direct_basename,
        num_sites_arg(num_localities), this_site_arg(this_locality));

    for (std::uint32_t i = 0; i != 10; ++i)
    {
        if (this_locality == 0)
        {
            hpx::future<std::vector<std::uint32_t>> overall_result =
                gather_here(gather_direct_client, std::uint32_t(42));

            std::vector<std::uint32_t> sol = overall_result.get();
            std::cout << "Gathered data on the root:";

            for (std::size_t j = 0; j != sol.size(); ++j)
            {
                HPX_TEST(j + 42 == sol[j]);
                std::cout << " " << sol[j];
            }
            std::cout << std::endl;
        }
        else
        {
            hpx::future<void> overall_result =
                gather_there(gather_direct_client, this_locality + 42);
            overall_result.get();
        }

    }

This code will print 10 times the following message:

.. code-block:: c++

    Gathered data on the root: 42 43

|hpx| uses two functions to implement the functionality of `MPI_Gather`: `gather_here` and
`gather_there`. `gather_here` is gathering data from all localities to the locality
with ID 0 (root locality). `gather_there` allows non-root localities to participate in the
gather operation by sending data to the root locality. In more detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `create_communicator()` is used to create a communicator called
  `gather_direct_client`.

- If the current locality is the root (its ID is equal to 0):

  - The `gather_here` function is used to perform the gather operation. It collects data from all
    other localities into the `overall_result` future object. The function arguments provide the necessary
    information, such as the base name for the gather operation (`gather_direct_basename`), the value
    to be gathered (`value`), the number of localities (`num_localities`), the current locality ID
    (`this_locality`), and the generation number (related to the gather operation).

  - The `get()` member function of the `overall_result` future is used to retrieve the gathered data.

  - The next `for` loop is used to verify the correctness of the gathered data (`sol`). `HPX_TEST`
    is a macro provided by the |hpx| testing utilities to perform similar testing with the Standard
    C++ macro `assert`.

- If the current locality is not the root:

  - The `gather_there` function is used to participate in the gather operation initiated by
    the root locality. It sends the data (in this case, the value `this_locality + 42`) to the root
    locality, indicating that it should be included in the gathering.

  - The `get()` member function of the `overall_result` future is used to wait for the gather operation
    to complete for this locality.

.. _MPI_Scatter:

MPI_Scatter
-----------

The following code gathers data from all processes to the root process and verifies
the gathered data in the root process.

|mpi| code:

.. code-block:: c++

    #include <iostream>
    #include <mpi.h>
    #include <vector>

    int main(int argc, char *argv[]) {
        MPI_Init(&argc, &argv);

        int num_localities, this_locality;
        MPI_Comm_size(MPI_COMM_WORLD, &num_localities);
        MPI_Comm_rank(MPI_COMM_WORLD, &this_locality);

        int num_localities = num_localities;
        std::vector<int> data(num_localities);

        if (this_locality == 0) {
            // Fill the data vector on the root locality (locality 0)
            for (int i = 0; i < num_localities; ++i) {
            data[i] = 42 + i;
            }
        }

        int local_data; // Variable to store the received data

        // Scatter data from the root locality to all other localities
        MPI_Scatter(&data[0], 1, MPI_INT, &local_data, 1, MPI_INT, 0, MPI_COMM_WORLD);

        // Now, each locality has its own local_data

        // Print the local_data on each locality
        std::cout << "Locality " << this_locality << " received " << local_data
                    << std::endl;

        MPI_Finalize();
        return 0;
    }

|hpx| equivalent:

.. code-block:: c++

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(std::uint32_t(2), num_localities);

    std::uint32_t this_locality = hpx::get_locality_id();

    auto scatter_direct_client =
        hpx::collectives::create_communicator(scatter_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));

    // test functionality based on immediate local result value
    for (std::uint32_t i = 0; i != 10; ++i)
    {
        if (this_locality == 0)
        {
            std::vector<std::uint32_t> data(num_localities);
            std::iota(data.begin(), data.end(), 42 + i);

            hpx::future<std::uint32_t> result =
                scatter_to(scatter_direct_client, std::move(data));

            HPX_TEST_EQ(i + 42 + this_locality, result.get());
        }
        else
        {
            hpx::future<std::uint32_t> result =
                scatter_from<std::uint32_t>(scatter_direct_client);

            HPX_TEST_EQ(i + 42 + this_locality, result.get());

            std::cout << "Locality " << this_locality << " received "
                      << i + 42 + this_locality << std::endl;
        }
    }

For num_localities = 2 and since we run for 10 iterations this code will print
the following message:

.. code-block:: c++

    Locality 1 received 43
    Locality 1 received 44
    Locality 1 received 45
    Locality 1 received 46
    Locality 1 received 47
    Locality 1 received 48
    Locality 1 received 49
    Locality 1 received 50
    Locality 1 received 51
    Locality 1 received 52

|hpx| uses two functions to implement the functionality of `MPI_Scatter`: `hpx::scatter_to` and
`hpx::scatter_from`. `hpx::scatter_to` is distributing the data from the locality with ID 0
(root locality) to all other localities. `hpx::scatter_from` allows non-root localities to receive
the data from the root locality. In more detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `hpx::collectives::create_communicator()` is used to create a communicator called
  `scatter_direct_client`.

- If the current locality is the root (its ID is equal to 0):

  - The data vector is filled with values ranging from `42 + i` to `42 + i + num_localities - 1`.

  - The `hpx::scatter_to` function is used to perform the scatter operation using the communicator
    `scatter_direct_client`. This scatters the data vector to other localities and
    returns a future representing the result.

  - `HPX_TEST_EQ` is a macro provided by the |hpx| testing utilities to test the distributed values.

- If the current locality is not the root:

  - The `hpx::scatter_from` function is used to collect the data by the root locality.

  - `HPX_TEST_EQ` is a macro provided by the |hpx| testing utilities to test the collected values.

.. _MPI_Allgather:

MPI_Allgather
-------------

The following code gathers data from all processes and sends the data to all
processes.

|mpi| code:

.. code-block:: c++

    #include <cstdint>
    #include <iostream>
    #include <mpi.h>
    #include <vector>

    int main(int argc, char **argv) {
        MPI_Init(&argc, &argv);

        int rank, size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        // Get the number of MPI processes
        int num_localities = size;

        // Get the MPI process rank
        int here = rank;

        std::uint32_t value = here;

        std::vector<std::uint32_t> r(num_localities);

        // Perform an all-gather operation to gather values from all processes.
        MPI_Allgather(&value, 1, MPI_UINT32_T, r.data(), 1, MPI_UINT32_T,
                        MPI_COMM_WORLD);

        // Print the result.
        std::cout << "Locality " << here << " has values:";
        for (size_t j = 0; j < r.size(); ++j) {
            std::cout << " " << r[j];
        }
        std::cout << std::endl;

        MPI_Finalize();
        return 0;
    }

|hpx| equivalent:

.. code-block:: c++

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t here = hpx::get_locality_id();

    // test functionality based on immediate local result value
    auto all_gather_direct_client =
        create_communicator(all_gather_direct_basename,
            num_sites_arg(num_localities), this_site_arg(here));

    std::uint32_t value = here;

    hpx::future<std::vector<std::uint32_t>> overall_result =
        all_gather(all_gather_direct_client, value);

    std::vector<std::uint32_t> r = overall_result.get();

    std::cout << "Locality " << here << " has values:";
    for (std::size_t j = 0; j != r.size(); ++j)
    {
        std::cout << " " << j;
    }
    std::cout << std::endl;

For num_localities = 2 this code will print the following message:

.. code-block:: c++

    Locality 0 has values: 0 1
    Locality 1 has values: 0 1

|hpx| uses the function `all_gather` to implement the functionality of `MPI_Allgather`. In more
detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `hpx::collectives::create_communicator()` is used to create a communicator called
  `all_gather_direct_client`.

- The values that the localities exchange with each other are equal to each locality's ID.

- The gather operation is performed using `all_gather`. The result is stored in an `hpx::future`
  object called `overall_result`, which represents a future result that can be retrieved later when
  needed.

- The `get()` function waits until the result is available and then stores it in the vector called `r`.

.. _MPI_Allreduce:

MPI_Allreduce
-------------

The following code combines values from all processes and distributes the result back to all processes.

|mpi| code:

.. code-block:: c++

    #include <cstdint>
    #include <iostream>
    #include <mpi.h>

    int main(int argc, char **argv) {
        MPI_Init(&argc, &argv);

        int rank, size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        // Get the number of MPI processes
        int num_localities = size;

        // Get the MPI process rank
        int here = rank;

        // Create a communicator for the all reduce operation.
        MPI_Comm all_reduce_direct_client;
        MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &all_reduce_direct_client);

        // Perform the all reduce operation to calculate the sum of 'here' values.
        std::uint32_t value = here;
        std::uint32_t res = 0;
        MPI_Allreduce(&value, &res, 1, MPI_UINT32_T, MPI_SUM,
                        all_reduce_direct_client);

        std::cout << "Locality " << rank << " has value: " << res << std::endl;

        MPI_Finalize();
        return 0;
    }

|hpx| equivalent:

.. code-block:: c++

    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t const here = hpx::get_locality_id();

    auto const all_reduce_direct_client =
        create_communicator(all_reduce_direct_basename,
            num_sites_arg(num_localities), this_site_arg(here));

    std::uint32_t value = here;

    hpx::future<std::uint32_t> overall_result =
        all_reduce(all_reduce_direct_client, value, std::plus<std::uint32_t>{});

    std::uint32_t res = overall_result.get();
    std::cout << "Locality " << here << " has value: " << res << std::endl;

For num_localities = 2 this code will print the following message:

.. code-block:: c++

    Locality 0 has value: 1
    Locality 1 has value: 1

|hpx| uses the function `all_reduce` to implement the functionality of `MPI_Allreduce`. In more
detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `hpx::collectives::create_communicator()` is used to create a communicator called
  `all_reduce_direct_client`.

- The value of each locality is equal to its ID.

- The reduce operation is performed using `all_reduce`. The result is stored in an `hpx::future`
  object called `overall_result`, which represents a future result that can be retrieved later when
  needed.

- The `get()` function waits until the result is available and then stores it in the variable `res`.

.. _MPI_Alltoall:

MPI_Alltoall
-------------

The following code gathers data from and scatters data to all processes.

|mpi| code:

.. code-block:: c++

    #include <algorithm>
    #include <cstdint>
    #include <iostream>
    #include <mpi.h>
    #include <vector>

    int main(int argc, char **argv) {
        MPI_Init(&argc, &argv);

        int rank, size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        // Get the number of MPI processes
        int num_localities = size;

        // Get the MPI process rank
        int this_locality = rank;

        // Create a communicator for all-to-all operation.
        MPI_Comm all_to_all_direct_client;
        MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &all_to_all_direct_client);

        std::vector<std::uint32_t> values(num_localities);
        std::fill(values.begin(), values.end(), this_locality);

        // Create vectors to store received values.
        std::vector<std::uint32_t> r(num_localities);

        // Perform an all-to-all operation to exchange values with other localities.
        MPI_Alltoall(values.data(), 1, MPI_UINT32_T, r.data(), 1, MPI_UINT32_T,
                    all_to_all_direct_client);

        // Print the results.
        std::cout << "Locality " << this_locality << " has values:";
        for (std::size_t j = 0; j != r.size(); ++j) {
            std::cout << " " << r[j];
        }
        std::cout << std::endl;

        MPI_Finalize();
        return 0;
    }


|hpx| equivalent:

.. code-block:: c++

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t this_locality = hpx::get_locality_id();

    auto all_to_all_direct_client =
        create_communicator(all_to_all_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));

    std::vector<std::uint32_t> values(num_localities);
    std::fill(values.begin(), values.end(), this_locality);

    hpx::future<std::vector<std::uint32_t>> overall_result =
        all_to_all(all_to_all_direct_client, std::move(values));

    std::vector<std::uint32_t> r = overall_result.get();
    std::cout << "Locality " << this_locality << " has values:";

    for (std::size_t j = 0; j != r.size(); ++j)
    {
        std::cout << " " << r[j];
    }
    std::cout << std::endl;

For num_localities = 2 this code will print the following message:

.. code-block:: c++

    Locality 0 has values: 0 1
    Locality 1 has values: 0 1

|hpx| uses the function `all_to_all` to implement the functionality of `MPI_Alltoall`. In more
detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `hpx::collectives::create_communicator()` is used to create a communicator called
  `all_to_all_direct_client`.

- The value each locality sends is equal to its ID.

- The all-to-all operation is performed using `all_to_all`. The result is stored in an `hpx::future`
  object called `overall_result`, which represents a future result that can be retrieved later when
  needed.

- The `get()` function waits until the result is available and then stores it in the variable `r`.

.. _MPI_Barrier:

MPI_Barrier
-----------

The following code shows how barrier is used to synchronize multiple processes.

|mpi| code:

.. code-block:: c++

    #include <cstdlib>
    #include <iostream>
    #include <mpi.h>

    int main(int argc, char **argv) {
        MPI_Init(&argc, &argv);

        std::size_t iterations = 5;

        int rank, size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        for (std::size_t i = 0; i != iterations; ++i) {
            MPI_Barrier(MPI_COMM_WORLD);
            if (rank == 0) {
            std::cout << "Iteration " << i << " completed." << std::endl;
            }
        }

        MPI_Finalize();
        return 0;
    }

|hpx| equivalent:

.. code-block:: c++

    std::size_t iterations = 5;
    std::uint32_t this_locality = hpx::get_locality_id();

    char const* const barrier_test_name = "/test/barrier/multiple";

    hpx::distributed::barrier b(barrier_test_name);
    for (std::size_t i = 0; i != iterations; ++i)
    {
        b.wait();
        if (this_locality == 0)
        {
            std::cout << "Iteration " << i << " completed." << std::endl;
        }
    }

This code will print the following message:

.. code-block:: c++

    Iteration 0 completed.
    Iteration 1 completed.
    Iteration 2 completed.
    Iteration 3 completed.
    Iteration 4 completed.

|hpx| uses the function `barrier` to implement the functionality of `MPI_Barrier`. In more
detail:

- After defining the number of iterations, we use
  `hpx::get_locality_id()` to get the ID of the current locality.

- `char const* const barrier_test_name = "/test/barrier/multiple"`: This line defines a constant
  character array as the name of the barrier. This name is used to identify the barrier across
  different localities. All participating threads that use this name will synchronize
  at this barrier.

- Using `hpx::distributed::barrier b(barrier_test_name)`, we create an instance of the distributed
  barrier with the previously defined name. This barrier will be used to synchronize the execution
  of threads across different localities.

- Running for all the desired iterations, we use `b.wait()` to synchronize the threads.
  Each thread waits until all other threads also reach this point before any of them can proceed
  further.

.. _MPI_Bcast:

MPI_Bcast
---------

The following code broadcasts data from one process to all other processes.

|mpi| code:

.. code-block:: c++

    #include <iostream>
    #include <mpi.h>

    int main(int argc, char *argv[]) {
        MPI_Init(&argc, &argv);

        int num_localities;
        MPI_Comm_size(MPI_COMM_WORLD, &num_localities);

        int here;
        MPI_Comm_rank(MPI_COMM_WORLD, &here);

        int value;

        for (int i = 0; i < 5; ++i) {
            if (here == 0) {
                value = i + 42;
            }

            // Broadcast the value from process 0 to all other processes
            MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD);

            if (here != 0) {
                std::cout << "Locality " << here << " received " << value << std::endl;
            }

        }

        MPI_Finalize();
        return 0;
    }


|hpx| equivalent:

.. code-block:: c++

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);

    std::uint32_t here = hpx::get_locality_id();

    auto broadcast_direct_client =
        create_communicator(broadcast_direct_basename,
            num_sites_arg(num_localities), this_site_arg(here));

    // test functionality based on immediate local result value
    for (std::uint32_t i = 0; i != 5; ++i)
    {
        if (here == 0)
        {
            hpx::future<std::uint32_t> result =
                broadcast_to(broadcast_direct_client, i + 42);

            result.get();
        }
        else
        {
            hpx::future<std::uint32_t> result =
                hpx::collectives::broadcast_from<std::uint32_t>(
                    broadcast_direct_client);

            uint32_t r = result.get();

            std::cout << "Locality " << here << " received " << r << std::endl;
        }
    }

For num_localities = 2 this code will print the following message:

.. code-block:: c++

    Locality 1 received 42
    Locality 1 received 43
    Locality 1 received 44
    Locality 1 received 45
    Locality 1 received 46

|hpx| uses two functions to implement the functionality of `MPI_Bcast`: `broadcast_to` and
`broadcast_from`. `broadcast_to` is broadcasting the data from the root locality to all
other localities. `broadcast_from` allows non-root localities to collect the data sent by
the root locality. In more detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `create_communicator()` is used to create a communicator called
  `broadcast_direct_client`.

- If the current locality is the root (its ID is equal to 0):

  - The `broadcast_to` function is used to perform the broadcast operation using the communicator
    `broadcast_direct_client`. This sends the data to other localities and
    returns a future representing the result.

  - The `get()` member function of the `result` future is used to wait for and retrieve the result.

- If the current locality is not the root:

  - The `broadcast_from` function is used to collect the data by the root locality.

  - The `get()` member function of the `result` future is used to wait for the result.

.. _MPI_Exscan:

MPI_Exscan
----------

The following code computes the exclusive scan (partial reductions) of data on a
collection of processes.

|mpi| code:

.. code-block:: c++

    #include <iostream>
    #include <mpi.h>
    #include <numeric>
    #include <vector>

    int main(int argc, char *argv[]) {
        MPI_Init(&argc, &argv);

        int num_localities;
        MPI_Comm_size(MPI_COMM_WORLD, &num_localities);

        int here;
        MPI_Comm_rank(MPI_COMM_WORLD, &here);

        // Calculate the value for this locality (here)
        int value = here;

        // Perform an exclusive scan
        std::vector<int> result(num_localities);
        MPI_Exscan(&value, &result[0], 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        if (here != 0) {
            int r = result[here - 1]; // Result is in the previous rank's slot

            std::cout << "Locality " << here << " has value " << r << std::endl;
        }

        MPI_Finalize();
        return 0;
    }


|hpx| equivalent:

.. code-block:: c++

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t here = hpx::get_locality_id();

    auto exclusive_scan_client = create_communicator(exclusive_scan_basename,
        num_sites_arg(num_localities), this_site_arg(here));

    // test functionality based on immediate local result value
    std::uint32_t value = here;

    hpx::future<std::uint32_t> overall_result = exclusive_scan(
        exclusive_scan_client, value, std::plus<std::uint32_t>{});

    uint32_t r = overall_result.get();

    if (here != 0)
    {
        std::cout << "Locality " << here << " has value " << r << std::endl;
    }

For num_localities = 2 this code will print the following message:

.. code-block:: c++

    Locality 1 has value 0

|hpx| uses the function `exclusive_scan` to implement `MPI_Exscan`. In more detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `create_communicator()` is used to create a communicator called
  `exclusive_scan_client`.

- The `exclusive_scan` function is used to perform the exclusive scan operation
  using the communicator `exclusive_scan_client`. `std::plus<std::uint32_t>{}`
  specifies the binary associative operator to use for the scan. In this case,
  it's addition for summing values.

- The `get()` member function of the `overall_result` future is used to wait for the result.

.. _MPI_Scan:

MPI_Scan
--------

The following code Computes the inclusive scan (partial reductions) of data on a collection
of processes.

|mpi| code:

.. code-block:: c++

    #include <iostream>
    #include <mpi.h>
    #include <numeric>
    #include <vector>

    int main(int argc, char *argv[]) {
        MPI_Init(&argc, &argv);

        int num_localities;
        MPI_Comm_size(MPI_COMM_WORLD, &num_localities);

        int here;
        MPI_Comm_rank(MPI_COMM_WORLD, &here);

        // Calculate the value for this locality (here)
        int value = here;

        std::vector<int> result(num_localities);

        MPI_Scan(&value, &result[0], 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        std::cout << "Locality " << here << " has value " << result[0] << std::endl;

        MPI_Finalize();
        return 0;
    }


|hpx| equivalent:

.. code-block:: c++

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t here = hpx::get_locality_id();

    auto inclusive_scan_client = create_communicator(inclusive_scan_basename,
        num_sites_arg(num_localities), this_site_arg(here));

    std::uint32_t value = here;

    hpx::future<std::uint32_t> overall_result = inclusive_scan(
        inclusive_scan_client, value, std::plus<std::uint32_t>{});

    uint32_t r = overall_result.get();

    std::cout << "Locality " << here << " has value " << r << std::endl;

For num_localities = 2 this code will print the following message:

.. code-block:: c++

    Locality 0 has value 0
    Locality 1 has value 1

|hpx| uses the function `inclusive_scan` to implement `MPI_Scan`. In more detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `create_communicator()` is used to create a communicator called
  `inclusive_scan_client`.

- The `inclusive_scan` function is used to perform the exclusive scan operation
  using the communicator `inclusive_scan_client`. `std::plus<std::uint32_t>{}`
  specifies the binary associative operator to use for the scan. In this case,
  it's addition for summing values.

- The `get()` member function of the `overall_result` future is used to wait for the result.

.. _MPI_Reduce:

MPI_Reduce
----------

The following code performs a global reduce operation across all processes.

|mpi| code:

.. code-block:: c++

    #include <iostream>
    #include <mpi.h>

    int main(int argc, char *argv[]) {
        MPI_Init(&argc, &argv);

        int num_processes;
        MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

        int this_rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &this_rank);

        int value = this_rank;

        int result = 0;

        // Perform the reduction operation
        MPI_Reduce(&value, &result, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

        // Print the result for the root process (process 0)
        if (this_rank == 0) {
            std::cout << "Locality " << this_rank << " has value " << result
                    << std::endl;
        }

        MPI_Finalize();
        return 0;
    }

|hpx| equivalent:

.. code-block:: c++

    std::uint32_t num_localities = hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t this_locality = hpx::get_locality_id();

    auto reduce_direct_client = create_communicator(reduce_direct_basename,
        num_sites_arg(num_localities), this_site_arg(this_locality));

    std::uint32_t value = hpx::get_locality_id();

    if (this_locality == 0)
    {
        hpx::future<std::uint32_t> overall_result = reduce_here(
            reduce_direct_client, value, std::plus<std::uint32_t>{});

        uint32_t r = overall_result.get();

        std::cout << "Locality " << this_locality << " has value " << r
                  << std::endl;
    }
    else
    {
        hpx::future<void> overall_result =
            reduce_there(reduce_direct_client, std::move(value));
        overall_result.get();
    }

This code will print the following message:

.. code-block:: c++

    Locality 0 has value 1

|hpx| uses two functions to implement the functionality of `MPI_Reduce`: `reduce_here` and
`reduce_there`. `reduce_here` is gathering data from all localities to the locality
with ID 0 (root locality) and then performs the defined reduction operation. `reduce_there`
allows non-root localities to participate in the reduction operation by sending data to the
root locality. In more detail:

- `hpx::get_num_localities(hpx::launch::sync)` retrieves the number of localities, while
  `hpx::get_locality_id()` returns the ID of the current locality.

- The function `create_communicator()` is used to create a communicator called
  `reduce_direct_client`.

- If the current locality is the root (its ID is equal to 0):

  - The `reduce_here` function initiates a reduction operation with addition (`std::plus`) as the
    reduction operator. The result is stored in `overall_result`.

  - The `get()` member function of the `overall_result` future is used to wait for the result.

- If the current locality is not the root:

  - The `reduce_there` initiates a remote reduction operation.

  - The `get()` member function of the `overall_result` future is used to wait for the remote
    reduction operation to complete. This is done to ensure synchronization among localities.

.. _reflection_action_migration:

Migrating to C++26 reflection-based actions
============================================

Starting with HPX 2.0, C++26 static reflection (P2996) can be used to define
distributed actions without the verbose macro boilerplate previously required.
This section describes how to migrate existing action definitions to the new
reflection-based API when building with ``HPX_WITH_CXX26_REFLECTION=ON``.

Build requirements
------------------

Reflection-based actions require a C++26-capable compiler with reflection
support enabled:

- GCC 16.0.1 trunk or later (``stellargroup/gcc_trunk_build_env:1``)
  with ``-freflection -std=c++26``
- Clang 22 with P2996 support (``stellargroup/clang_p2996_build_env:1``)
  with ``-freflection-latest -stdlib=libc++``

Reflection support is auto-detected by HPX's CMake build system via
``hpx_check_for_cxx26_reflection()`` in
``cmake/HPX_PerformCxxFeatureTests.cmake``. No manual flag is required
when using a supported compiler — ``HPX_HAVE_CXX26_REFLECTION`` is set
automatically.

Existing code requires no changes
----------------------------------

The most important point: **existing code using** ``HPX_PLAIN_ACTION``,
``HPX_DEFINE_COMPONENT_ACTION``, and related macros requires zero source
changes. When ``HPX_WITH_CXX26_REFLECTION=ON``, these macros automatically
use the reflection-based implementation internally. No
``HPX_REGISTER_ACTION`` calls are needed — registration is automatic.

.. code-block:: c++

    // This works unchanged with or without reflection enabled.
    // When HPX_WITH_CXX26_REFLECTION=ON, it automatically uses
    // reflect_action<^^some_function> internally.
    HPX_PLAIN_ACTION(some_function, some_action)

Explicit migration: plain actions
----------------------------------

For new code or explicit migration, use ``HPX_ACTION`` or
``reflect_action`` directly:

.. code-block:: c++

    // Before: three-step macro sequence
    HPX_PLAIN_ACTION(app::compute, compute_action)
    HPX_REGISTER_ACTION_DECLARATION(compute_action)
    HPX_REGISTER_ACTION(compute_action)

    // After: single line, no registration required
    HPX_ACTION(app::compute, compute_action)

    // Or equivalently, using the template directly:
    using compute_action = hpx::actions::reflect_action<^^app::compute>;

Explicit migration: component actions
--------------------------------------

.. code-block:: c++

    // Before: three-step macro sequence
    HPX_DEFINE_COMPONENT_ACTION(server, compute, compute_action)
    HPX_REGISTER_ACTION_DECLARATION(compute_action)
    HPX_REGISTER_ACTION(compute_action)

    // After: single line
    HPX_COMPONENT_ACTION(server, compute, compute_action)

    // Or equivalently:
    using compute_action =
        hpx::actions::reflect_component_action<^^server::compute>;

Explicit migration: direct plain actions
----------------------------------------

.. code-block:: c++

    // Before: three-step macro sequence
    HPX_PLAIN_DIRECT_ACTION(app::compute, compute_action)
    HPX_REGISTER_ACTION_DECLARATION(compute_action)
    HPX_REGISTER_ACTION(compute_action)

    // After: single line
    HPX_DIRECT_ACTION(app::compute, compute_action)

    // Or equivalently, using the template directly:
    using compute_action =
        hpx::actions::reflect_direct_action<^^app::compute>;

Explicit migration: direct component actions
---------------------------------------------

Direct actions execute without spawning a new HPX thread — locally they
run inline on the calling thread, and remotely on the target locality's
existing HPX thread. The reflection-based API preserves this behavior:

.. code-block:: c++

    // Before: three-step macro sequence
    HPX_DEFINE_COMPONENT_DIRECT_ACTION(server, compute, compute_action)
    HPX_REGISTER_ACTION_DECLARATION(compute_action)
    HPX_REGISTER_ACTION(compute_action)

    // After: single line
    HPX_COMPONENT_DIRECT_ACTION(server, compute, compute_action)

    // Or equivalently, using the template directly:
    using compute_action =
        hpx::actions::reflect_component_direct_action<^^server::compute>;

Overloaded functions
--------------------

Reflection cannot disambiguate overloaded functions. If your action
function is overloaded, keep the existing ``HPX_PLAIN_ACTION`` macro:

.. code-block:: c++

    // Not supported with reflection (ambiguous):
    // using my_action = hpx::actions::reflect_action<^^overloaded_func>;

    // Use the existing macro instead:
    HPX_PLAIN_ACTION(static_cast<int(*)(int)>(overloaded_func), my_action)

Properties available at compile time
--------------------------------------

Reflection-based action types expose additional compile-time properties:

.. code-block:: c++

    using my_action = hpx::actions::reflect_action<^^app::compute>;

    // Number of parameters
    static_assert(my_action::arity == 2);

    // Function pointer type and value
    using fptr = my_action::func_ptr_type;
    constexpr auto fp = my_action::func_ptr;
