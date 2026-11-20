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
``hpx.parcel.ucx_conduit.rndv_thresh`` sets the size from which UCX uses its
rendezvous protocol (default 16384 bytes) unless
``HPX_DEVICE_CONDUIT_UCX_RNDV_THRESH`` is set.

See the :ref:`API reference <modules_device_conduit_ucx_api>` of this module
for more details.
