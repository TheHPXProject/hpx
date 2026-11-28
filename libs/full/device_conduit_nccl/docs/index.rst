..
    Copyright (c) 2026 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_device_conduit_nccl:

===================
device_conduit_nccl
===================

A device conduit (``hpx::parcelset::device_conduit``) based on NCCL, or on
RCCL in HIP builds (the same API): it moves the zero-copy chunks of device
memory of parcels between localities with NCCL point-to-point operations,
below the parcelport, which sends the rest of the parcels. NCCL chooses the
path: NVLink or PCIe peer access within a node, GPUDirect RDMA between nodes,
staging through host memory where neither is available, and several network
ports at once where a node has them.

Enable it with ``HPX_WITH_DEVICE_CONDUIT_NCCL=ON`` (it requires
``HPX_WITH_NETWORKING=ON`` and ``HPX_WITH_CUDA=ON`` or ``HPX_WITH_HIP=ON``;
NCCL or RCCL 2.14 or newer) and select it at runtime with
``hpx.parcel.device_conduit=nccl``. It finds NCCL through ``NCCL_ROOT`` (or
the environment variables ``NCCL_ROOT`` and ``NCCL_HOME``) and RCCL through
``RCCL_ROOT`` or the ROCm installation. NCCL reads its own configuration from
the ``NCCL_`` environment variables (e.g. ``NCCL_DMABUF_ENABLE=1``, which
GPUDirect RDMA with ROCm needs where it is available only through DMA-BUF).

NCCL matches the sends and receives between two ranks by the order they are
issued in, and its communicators are not thread safe. The conduit uses one
communicator of two ranks for each direction between two localities (and
pair of GPUs, see below), created when the first device data goes that way (the receiving locality creates its
identifier and sends it with its first request). The receiving locality
decides the order: it collects the receives of the chunks it has decoded into
numbered rounds, posts them as one group, and asks the sending locality for the
chunks of each round, which it sends in that order. Each transfer therefore
costs a small control message from the receiver to the sender in addition to
NCCL's own latency (tens of microseconds per operation); the conduit pays off
for large transfers on systems where NCCL is faster than the alternatives,
for instance nodes with several network ports.

The communicators are non-blocking: no thread waits in NCCL, the conduit
polls the communicators and the CUDA events of the rounds in its background
work.

A rank of an NCCL communicator belongs to one GPU, so a channel is a
direction between two localities and a pair of GPUs: the GPU of the sent data
and the GPU of the memory it is received into. The sender puts its GPU into
the chunk's descriptor, the receiver chooses its GPU by allocating the memory
(device data in a parcel is received on the GPU of the parcel's destination,
see the ``thrust`` module). Communicators are created for the pairs of GPUs
that are used only; each costs some GPU memory and its setup. Localities on
the same node must not use the same GPU for one channel (NCCL does not allow
two ranks of a communicator on the same GPU). The results of remote calls are
received on the GPU of the default target
(``hpx::cuda::experimental::get_default_target()``, GPU 0 unless set): where
several localities share a node, set it to a different GPU in each.

With RCCL the communicators are blocking instead: RCCL (2.22) hangs or
crashes when the non-blocking initializations of two communicators overlap in
a process, which happens here with a communicator per direction. The calls
that may wait for the other locality, the initialization of a communicator and
issuing a group, run on a thread of the channel, one at a time; the conduit
polls their results in its background work as it polls a non-blocking
communicator with NCCL.

NCCL moves data with kernels that wait on the GPU until the other locality
has started its part of the transfer. Functions that wait for all work on the
GPU, such as ``cudaFree`` and ``cudaDeviceSynchronize``, therefore must not be
called while transfers are in flight: if both localities do, each waits for a
transfer the other cannot start, and both hang. Use memory that is allocated
once, such as the pooled allocator of ``hpx::thrust`` (the default for
``thrust::device_vector`` sent in parcels), or stream-ordered allocation
(``cudaMallocAsync`` and ``cudaFreeAsync``).

The conduit reads sent data on a stream of its own, which nothing orders after
the application's streams: before data is sent, its allocation and the writes
to it must have completed (wait for the stream that produced it or for an
event recorded on it; a device-wide wait is not needed), and the memory must
stay allocated until the transfer has completed (the parcel's data is kept
alive by the parcel until then).

With HIP the rule is broader: HIP runs the streams of a GPU on a few in-order
hardware queues (``GPU_MAX_HW_QUEUES``), and RCCL creates several streams of
its own, so waiting for any stream may wait for an RCCL kernel queued before
it on the same hardware queue. This includes stream-ordered allocation
followed by ``hipStreamSynchronize``, and the work ``hipFreeAsync`` queues.
Allocate the device memory of transfers once and reuse it (``hipMalloc`` does
not wait for the GPU). Reusing memory avoids the waits of allocating and
freeing, but does not make waiting for a stream safe: work on a stream can
still be queued behind an RCCL kernel, and if the waiting thread has to make
progress before the other locality can post its part of that transfer, the
wait deadlocks. Finish the preparation that needs a blocking wait before
transfers that may overlap with it start, or wait on a thread whose progress
the transfers don't depend on.

NCCL requires the processes to launch the operations of all communicators
that share a GPU in one consistent order; otherwise a kernel can wait for a
kernel of its peer that cannot start because its GPU is busy with a kernel of
another communicator, which waits as well (see "Using multiple NCCL
communicators concurrently" in NCCL's documentation; NCCL 2.26 adds an
ordering by the order of the launches in each process, which does not make
the order consistent between processes). The conduit has two communicators
per pair of GPUs (one per direction), and each receiver orders its own
rounds, so this order is not guaranteed. A GPU normally runs the conduit's
kernels of different communicators side by side; the risk grows with the
number of peers per GPU (two communicators for each remote GPU), with MPS or
other limits on a GPU's resources, and with application kernels occupying a
GPU while transfers are in flight. The UCX conduit does not have this
limitation.

The conduit reports a round that has been in flight for longer than
``hpx.parcel.conduit.nccl.stall_timeout`` seconds (default 30, 0 turns the
check off) as a warning in the parcel log: the round, its peer and GPUs, and
the other rounds in flight on the same GPU in the order the process launched
them. Comparing the reports of the two localities shows whether they
launched the rounds of two communicators in different orders. With
``hpx.parcel.conduit.nccl.stall_action=fail`` (default ``report``), the
stalled channel also fails: its communicator is aborted, which releases
NCCL's kernels, and its transfers fail with an error naming the probable
cause, instead of waiting forever (with RCCL the abort waits for the call of
the channel's thread to return). A stall can also come from a peer that is
slow or has stopped, which the report cannot tell apart.

See the :ref:`API reference <modules_device_conduit_nccl_api>` of this module
for more details.
