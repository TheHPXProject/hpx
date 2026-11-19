..
    Copyright (c) 2025 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_thrust:

======
thrust
======

The thrust module integrates |hpx| parallel algorithms with NVIDIA Thrust
(CUDA) or rocThrust (HIP), enabling GPU acceleration using familiar |hpx|
algorithm syntax.

Execution Policies
==================

``hpx::thrust::thrust_host_policy``
   CPU execution using optimized parallel implementations

``hpx::thrust::thrust_device_policy``
   Synchronous GPU execution

``hpx::thrust::thrust_task_policy``
   Asynchronous GPU execution returning |hpx| futures

Policy Mappings
===============

The |hpx| thrust policies map directly to Thrust execution policies:

.. code-block:: c++

    // Policy mappings
    hpx::thrust::thrust_host_policy    -> thrust::host
    hpx::thrust::thrust_device_policy  -> thrust::device  
    hpx::thrust::thrust_task_policy    -> thrust::par_nosync

Usage
=====

Synchronous Device Execution
-----------------------------

.. code-block:: c++

    #include <hpx/modules/thrust.hpp>
    #include <thrust/device_vector.h>

    hpx::thrust::thrust_device_policy device{};
    thrust::device_vector<int> d_vec(1000, 0);
    
    hpx::fill(device, d_vec.begin(), d_vec.end(), 42);
    int sum = hpx::reduce(device, d_vec.begin(), d_vec.end(), 0);

Asynchronous Execution with Futures
------------------------------------

.. code-block:: c++

    #include <hpx/modules/async_cuda.hpp>
    #include <hpx/modules/thrust.hpp>

    int hpx_main() {
        // Required for async operations
        hpx::cuda::experimental::enable_user_polling polling_guard("default");
        
        hpx::thrust::thrust_task_policy task{};
        thrust::device_vector<int> d_vec(1000, 1);
        
        auto fill_future = hpx::fill(task, d_vec.begin(), d_vec.end(), 42);
        fill_future.get();  // Standard HPX future operations
        
    }

Build Requirements
==================

* CUDA Toolkit 12.4.0+ with ``HPX_WITH_CUDA=ON``, or
* ROCm with rocThrust and ``HPX_WITH_HIP=ON``
* ``HPX_WITH_THRUST`` (ON by default with CUDA or HIP)

See Also
========

* :ref:`modules_async_cuda` - CUDA and HIP integration
* :ref:`modules_execution` - |hpx| execution policies
