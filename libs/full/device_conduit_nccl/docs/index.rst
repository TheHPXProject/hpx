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
``HPX_WITH_CUDA=ON`` or ``HPX_WITH_HIP=ON``) and select it at runtime with
``hpx.parcel.device_conduit=nccl``. It finds NCCL through ``NCCL_ROOT`` (or
the environment variables ``NCCL_ROOT`` and ``NCCL_HOME``) and RCCL through
``RCCL_ROOT`` or the ROCm installation. NCCL reads its own configuration from
the ``NCCL_`` environment variables (e.g. ``NCCL_DMABUF_ENABLE=1``, which
GPUDirect RDMA with ROCm needs where it is available only through DMA-BUF).

NCCL matches the sends and receives between two ranks by the order they are
issued in, and its communicators are not thread safe. The conduit uses one
communicator of two ranks for each direction between two localities, created
when the first device data goes that way (the receiving locality creates its
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
work. A channel uses the GPU of the first chunk it moves: the conduit supports
one GPU per locality, and localities on the same node need different GPUs
(NCCL does not allow two ranks of a communicator on the same GPU).

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

With HIP the rule is broader: HIP runs the streams of a GPU on a few in-order
hardware queues (``GPU_MAX_HW_QUEUES``), and RCCL creates several streams of
its own, so waiting for any stream may wait for an RCCL kernel queued before
it on the same hardware queue. This includes stream-ordered allocation
followed by ``hipStreamSynchronize``, and the work ``hipFreeAsync`` queues.
Allocate the device memory of transfers once and reuse it (``hipMalloc`` does
not wait for the GPU), and synchronize only the streams whose work is needed.

See the :ref:`API reference <modules_device_conduit_nccl_api>` of this module
for more details.
