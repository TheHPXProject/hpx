..
    Copyright (c) 2026 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_device_conduit_ucx:

==================
device_conduit_ucx
==================

A device conduit (``hpx::parcelset::device_conduit``) based on UCX: it moves
the zero-copy chunks of device memory of parcels between localities with UCX
tagged messages, below the parcelport, which sends the rest of the parcels.
UCX chooses the path: CUDA or ROCm IPC within a node, GPUDirect RDMA between
nodes, staging through host memory where neither is available. UCX registers
CUDA memory with the CUDA context current on the calling thread: the conduit
makes the primary context of the memory's GPU current on threads that have
none.

Enable it with ``HPX_WITH_DEVICE_CONDUIT_UCX=ON`` and select it at runtime
with ``hpx.parcel.device_conduit=ucx``. It looks for UCX next to the MPI
library (``MPI_HOME``, if it is set) first, then in ``UCX_ROOT``. One process
can use only one UCX library: if the MPI library is UCX based and its UCX is
installed elsewhere (or ``MPI_HOME`` is not set because MPI was found through
its compiler wrappers), set ``UCX_ROOT`` to that UCX installation. The UCX that
was found is remembered in the |cmake|_ cache: to switch to another one,
configure a new build directory.

The conduit reads its UCX configuration from the ``UCX_`` environment
variables, like the MPI library, and from those with the prefix
``HPX_DEVICE_CONDUIT_UCX_``, which take precedence and apply to the conduit
only. The prefixed variables cover the protocol settings (e.g.
``HPX_DEVICE_CONDUIT_UCX_TLS``, ``HPX_DEVICE_CONDUIT_UCX_RNDV_THRESH``); the
settings of the memory domains and transports are read by UCX from the
``UCX_`` variables only (e.g. ``UCX_ROCM_COPY_DMABUF=yes``, which GPUDirect
RDMA with ROCm needs where it is available only through DMA-BUF). The
configuration entry
``hpx.parcel.conduit.ucx.rndv_thresh`` sets the size from which UCX uses its
rendezvous protocol (default 16384 bytes) unless
``HPX_DEVICE_CONDUIT_UCX_RNDV_THRESH`` is set.

The configuration entry ``hpx.parcel.conduit.ucx.error_handling`` (or
``HPX_PARCEL_CONDUIT_UCX_ERROR_HANDLING``) sets the error handling of the
endpoints, the same on all localities. With ``peer`` (the default), a locality
that fails or leaves fails the transfers to it instead of hanging them, and the
endpoints are closed at shutdown without waiting for the peers. UCX then does
not use its pipelined protocol for staging device memory through host memory,
which it needs where the network cannot access device memory (no GPUDirect
RDMA, e.g. ConnectX-3 adapters): there, transfers between nodes are several
times slower (4.7x for 8 MiB on V100 nodes with ConnectX-3). With ``none``,
they are as fast as with a UCX based MPI library, but a locality leaving while
transfers to or from it are in flight may hang the others, and closing the
endpoints at shutdown waits for the peers. Use ``none`` only where no locality
leaves before the others (no localities connecting and disconnecting
dynamically).

See the :ref:`API reference <modules_device_conduit_ucx_api>` of this module
for more details.
