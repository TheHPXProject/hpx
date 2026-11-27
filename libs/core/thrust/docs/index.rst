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

Serialization
=============

``hpx/thrust/serialization/host_vector.hpp``,
``hpx/thrust/serialization/universal_vector.hpp`` and
``hpx/thrust/serialization/device_vector.hpp`` make ``thrust::host_vector``,
``thrust::universal_vector`` and ``thrust::device_vector`` serializable, e.g.
as action arguments.

* ``host_vector`` and ``universal_vector`` are serialized from their own
  memory, like a ``std::vector`` (zero-copy chunks included).
* ``device_vector``: data at or above the zero-copy threshold is sent as a
  zero-copy chunk of the device memory itself if the parcelport sends device
  memory (CUDA- or ROCm-aware MPI, see ``HPX_WITH_PARCELPORT_CUDA_SUPPORT``
  and ``HPX_WITH_PARCELPORT_HIP_SUPPORT``), otherwise through pinned host
  staging buffers with asynchronous copies. The receiving side decides on its
  own whether it receives such data directly into device memory, whatever
  memory it was sent from (for the MPI parcelport see
  ``hpx.parcel.mpi.send_device_memory`` and
  ``hpx.parcel.mpi.receive_device_memory``). Smaller data is copied into the
  archive. The data must be complete (the kernels writing it finished) when
  the vector is serialized. A ``device_vector`` whose memory is pinned host
  memory (see below) is sent from and received into its own memory, without
  staging copies.

Several GPUs per Locality
-------------------------

A ``device_vector`` is sent from the GPU its memory is on, whatever GPU is
current. It is received on

#. the GPU the memory of the receiving vector is on, if it has memory (a
   vector deserialized into an existing object),
#. for an argument of an action, the GPU of the target of the component the
   action is invoked on: a component names its target with a ``get_target()``
   member function returning, for instance, an
   ``hpx::cuda::experimental::target``, or with a specialization of
   ``hpx::traits::component_target``,
#. for the result of a remote call, the GPU of the default target
   (``hpx::cuda::experimental::get_default_target()``) of the locality that
   made the call. All threads share the default target: set it before making
   calls, not while calls are in flight,
#. else the GPU current on the thread that decodes the parcel.

.. code-block:: c++

    #include <hpx/include/components.hpp>
    #include <hpx/modules/async_cuda.hpp>
    #include <hpx/modules/thrust.hpp>

    struct partition : hpx::components::component_base<partition>
    {
        partition() = default;
        explicit partition(int device)
          : target_(device)
        {
        }

        // device_vector arguments of the actions of this component are
        // received on this GPU
        hpx::cuda::experimental::target const& get_target() const noexcept
        {
            return target_;
        }

        void set(thrust::device_vector<double> const& data);
        HPX_DEFINE_COMPONENT_ACTION(partition, set)

        hpx::cuda::experimental::target target_;
    };

A target stands for the GPU its ``native_handle().get_device()`` returns if
that is an integer (see ``hpx::traits::target_device``); other targets, for
instance ``hpx::compute::host::target``, stand for none. Results are received
on the default target's GPU only if their type holds device data
(``hpx::traits::has_device_data``, true for ``thrust::device_vector``):
specialize it for other result types holding device vectors, for instance a
``struct`` with a ``device_vector`` member.

Pooled Memory
=============

``hpx/thrust/allocator.hpp`` provides ``hpx::thrust::pooled_allocator<T>``
and ``hpx::thrust::device_vector<T>``, a ``thrust::device_vector`` using it.
Allocating device or pinned host memory takes long, and the network has to
register every new buffer before it moves data from or into it. The allocator
takes memory from pools that keep deallocated memory for reuse, so buffers are
allocated and registered once. Deallocated memory is reused right away: no
device work may use it anymore when its vector is destroyed.

The configuration entry ``hpx.thrust.memory`` (or the environment variable
``HPX_THRUST_MEMORY``) selects the memory:

``device`` (the default)
   Device memory.

``pinned``
   Pinned (page-locked) host memory. The network sends it from and receives
   it into its place, without staging copies, but device kernels work on it
   slower than on device memory, also on integrated GPUs (measured on an
   NVIDIA GB10).

Each pool keeps at most ``hpx.thrust.max_cached`` (``HPX_THRUST_MAX_CACHED``)
bytes for reuse (by default an eighth of the device's memory, at least 256 MiB
of pinned memory).
``hpx::thrust::release_cached_memory()`` frees the kept memory; the pools do
that themselves if an allocation runs out of memory.

.. code-block:: c++

    #include <hpx/thrust/allocator.hpp>

    hpx::thrust::device_vector<double> v(1 << 20);
    // the same as
    thrust::device_vector<double, hpx::thrust::pooled_allocator<double>> w;

Build Requirements
==================

* CUDA Toolkit 12.4.0+ with ``HPX_WITH_CUDA=ON``, or
* ROCm with rocThrust and ``HPX_WITH_HIP=ON``
* ``HPX_WITH_THRUST`` (ON by default with CUDA or HIP)

See Also
========

* :ref:`modules_async_cuda` - CUDA and HIP integration
* :ref:`modules_execution` - |hpx| execution policies
