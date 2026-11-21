..
    Copyright (c) 2026 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _using_the_openshmem_parcelport:

==============================
Using the OpenSHMEM parcelport
==============================

.. _info_openshmem:

Basic information
=================

The OpenSHMEM parcelport transports parcels with `OpenSHMEM`_. Every locality
is one OpenSHMEM processing element (PE). Parcels are moved as pages with
one-sided puts into ring buffers allocated in the symmetric heap of the
receiving PE.

The parcelport supports the OpenSHMEM implementation of `Open MPI`_ (OSHMEM).
It requests ``SHMEM_THREAD_MULTIPLE``; if the implementation provides less, all
OpenSHMEM calls are made from a single I/O thread.

.. _`OpenSHMEM`: http://openshmem.org

.. _`Open MPI`: https://www.open-mpi.org

.. _build_openshmem:

Build Open MPI with OpenSHMEM
=============================

|hpx| does not download or build OpenSHMEM; install it first. Open MPI builds
OSHMEM only when it is configured with UCX, which OSHMEM uses to move data:

.. code-block:: shell-session

   $ ./configure --prefix=$MPI_ROOT --with-ucx=$UCX_ROOT --enable-oshmem
   $ make -j && make install

Check that ``$MPI_ROOT/lib/pkgconfig`` contains ``oshmem.pc`` (or
``oshmem-c.pc``) and that ``oshrun`` was installed. |hpx| finds OpenSHMEM only
through these ``pkg-config`` files.

.. _build_openshmem_pp:

Build |hpx| with the OpenSHMEM parcelport
=========================================

Make the ``pkg-config`` files of Open MPI visible and enable the parcelport:

.. code-block:: shell-session

   $ export PKG_CONFIG_PATH=$MPI_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH
   $ cmake -S hpx -B build -DHPX_WITH_PARCELPORT_OPENSHMEM=ON

.. option:: HPX_WITH_PARCELPORT_OPENSHMEM

   Enable the OpenSHMEM parcelport. The default value is ``OFF``. All the
   following variables only take effect when this variable is set to ``ON``.

.. option:: HPX_WITH_PARCELPORT_OPENSHMEM_MTU

   Default of ``hpx.parcel.openshmem.mtu`` (see below). The default value is
   ``65536``.

.. option:: HPX_WITH_PARCELPORT_OPENSHMEM_SLOTS

   Default of ``hpx.parcel.openshmem.slots`` (see below). The default value is
   ``8``.

.. option:: HPX_OPENSHMEM_REQUIRE_UCX

   Fail the configuration if the ``pkg-config`` files of OpenSHMEM don't list
   the UCX libraries. Otherwise |cmake|_ only prints a warning. The default
   value is ``OFF``.

The OpenSHMEM location is stored in the |cmake|_ cache. To switch to another
OpenSHMEM installation, configure a new build directory.

.. _run_openshmem_pp:

Run |hpx| applications with the OpenSHMEM parcelport
====================================================

An |hpx| application built with the OpenSHMEM parcelport is started with
``oshrun``, one PE per locality. Unlike the other parcelports, the OpenSHMEM
parcelport is disabled by default even when |hpx| is built with it. Enable it,
make it the parcelport that carries the parcels and bootstraps the localities,
and switch off the other parcelports of the build: the MPI, LCI and LCW
parcelports would initialize MPI a second time inside the OpenSHMEM job, and the
GASNet parcelport would start its own runtime. The ``key!=value`` form is
accepted also for parcelports that are not part of the build:

.. code-block:: shell-session

   $ oshrun -n 4 ./my_hpx_application \
       --hpx:ini=hpx.parcel.openshmem.enable=1 \
       --hpx:ini=hpx.parcel.openshmem.priority=1000 \
       --hpx:ini=hpx.parcel.bootstrap=openshmem \
       --hpx:ini=hpx.parcel.mpi.enable!=0 \
       --hpx:ini=hpx.parcel.lci.enable!=0 \
       --hpx:ini=hpx.parcel.lcw.enable!=0 \
       --hpx:ini=hpx.parcel.tcp.enable!=0 \
       --hpx:ini=hpx.parcel.gasnet.enable!=0 \
       --hpx:ini=hpx.max_idle_loop_count=0

Pass each setting in its own ``--hpx:ini`` option. Parcels arriving through
OpenSHMEM do not wake idle worker threads; ``hpx.max_idle_loop_count=0`` keeps
them polling, which avoids a delay of about 80 ms per message for applications
with many round trips.

``oshrun`` accepts the options of Open MPI's ``mpirun`` (host lists, mapping and
binding, ``-x`` to forward environment variables). The size of the symmetric
heap of every PE is set with the environment variable ``SMA_SYMMETRIC_SIZE``
(for example ``-x SMA_SYMMETRIC_SIZE=1G``); Open MPI's default is 256 MiB.
The parcelport's ring buffers take about ``2 * slots * mtu`` bytes per PE in the
job from that heap, so large jobs or a large ``mtu`` need a larger heap.

``hpxrun.py`` supports the parcelport with ``-p openshmem -r oshrun``:

.. code-block:: shell-session

   $ hpxrun.py ./my_hpx_application -l 4 -t 8 -p openshmem -r oshrun

This runs ``oshrun -n <localities> --bind-to none --mca osc ucx -x
SMA_SYMMETRIC_SIZE=<size> -x UCX_TLS=<transports>`` with the settings shown
above. ``--bind-to none`` keeps ``oshrun`` from binding each process to a single
core, which would make all worker threads of a locality share it; pass it also
when starting ``oshrun`` by hand. The heap size is taken
from ``SMA_SYMMETRIC_SIZE`` in the environment of ``hpxrun.py`` and defaults to
``1G``. The UCX transports are taken from ``UCX_TLS`` and default to
``sm,self``, the shared-memory transports, which work only within one node: set
``UCX_TLS`` (for example to ``all``) for runs across nodes. Set
``HPXRUN_OSHRUN`` to use another ``oshrun`` executable.

How parcels are sent
--------------------

The worker threads of a locality drive the network from their background work:
they send queued messages, check for pages that arrived, and decode received
messages. The parcelport starts no threads of its own for this unless
``hpx.parcel.openshmem.worker_progress`` is ``0``. If the OpenSHMEM runtime
provides only ``SHMEM_THREAD_SERIALIZED``, one worker thread at a time does
this, and the others skip it while it does.

Each pair of localities has a ring of pages for each direction, and only one
message is in flight to a destination at a time. A parcel to a destination
that nothing is being sent to goes out immediately as a message of its own.
While a message to a destination is in flight, further parcels to it are
queued, and once the destination is free, all queued parcels are encoded into
one message and sent together. Isolated parcels so get the lowest latency, and
many parcels to one destination are sent in few messages.
``hpx.parcel.openshmem.sendimm=0`` queues every parcel instead, as the other
parcelports do by default.

How parcels are received
------------------------

A worker thread that finds a complete message from a source decodes it and
runs the actions of its parcels on the same thread. Before it does that, it
releases the source, so that the next message from that source can be received
while the actions run. An action may suspend or wait for further messages
without stopping the network. With ``SHMEM_THREAD_SERIALIZED`` the thread
first leaves the section in which it uses the OpenSHMEM runtime, so that other
threads can drive the network while the actions run.

While the runtime starts up, the worker threads don't run yet. A thread of the
parcelport drives the network until the runtime tells the parcelport that its
bootstrap is complete, and the worker threads take over from then on.

Worker threads or I/O threads
-----------------------------

Worker progress gives the lowest latency, since no thread hands the messages
over to another one. With worker progress the network makes progress only while
worker threads run their background work, between two tasks or while they are
idle. A worker thread that runs a long task without suspending does not drive
the network in the meantime. Applications with such tasks should run enough
worker threads, or use I/O threads (``hpx.parcel.openshmem.worker_progress=0``),
each of which needs a core of its own.

Runtime settings
----------------

The following settings configure the parcelport at runtime. ``mtu``, ``slots``
and ``checksum`` are part of the wire protocol and must be the same on all
localities.

.. option:: --hpx:ini=hpx.parcel.openshmem.enable=<0|1>

   Use the OpenSHMEM parcelport. The default value is ``0``.

.. option:: --hpx:ini=hpx.parcel.openshmem.mtu=<bytes>

   Size of a page, the unit in which parcels are transferred. ``0`` selects it
   automatically: as large as 1 MiB, but smaller for large jobs, so that the
   ring buffers stay within 128 MiB of the symmetric heap. The default value
   is ``HPX_WITH_PARCELPORT_OPENSHMEM_MTU``.

.. option:: --hpx:ini=hpx.parcel.openshmem.slots=<n>

   Number of pages in flight between each pair of localities. The default value
   is ``HPX_WITH_PARCELPORT_OPENSHMEM_SLOTS``.

.. option:: --hpx:ini=hpx.parcel.openshmem.checksum=<0|1>

   Verify a checksum over every received page. The default value is ``1``.

.. option:: --hpx:ini=hpx.parcel.openshmem.uniform_check=<0|1>

   Compare ``mtu``, ``slots`` and ``checksum`` across all localities at startup.
   The default value is ``0``.

.. option:: --hpx:ini=hpx.parcel.openshmem.symmetric_memory_size=<bytes>

   The symmetric heap size the job is started with. Startup fails with an
   error if the ring buffers don't fit into it, and locality 0 prints a warning
   if they take more than three quarters of it. The default value is
   ``268435456`` (256 MiB); ``SMA_SYMMETRIC_SIZE`` takes precedence when it is
   set.

.. option:: --hpx:ini=hpx.parcel.openshmem.scan_window=<n>

   Maximum number of source localities checked for new pages in one pass of the
   receiver. ``0`` checks all of them in every pass. The default value is
   ``8``.

.. option:: --hpx:ini=hpx.parcel.openshmem.worker_progress=<0|1>

   ``1`` (the default): the worker threads make progress on the network.
   ``0``: dedicated I/O threads of the parcelport do instead; they poll the
   network continuously, each on its own core.

.. option:: --hpx:ini=hpx.parcel.openshmem.io_pool_size=<n>

   Number of I/O threads with ``worker_progress=0``; each of them serves a part
   of the source localities. The default value is ``2``; if the OpenSHMEM
   runtime provides only ``SHMEM_THREAD_SERIALIZED``, a single one is used.
   Without I/O threads (``worker_progress=1``) the setting has no effect.

.. option:: --hpx:ini=hpx.parcel.openshmem.sendimm=<0|1>

   ``1`` (the default) sends a parcel immediately if nothing is being sent to
   its destination, and queues it otherwise; the queued parcels to a
   destination are sent together in one message once it is free. ``0`` queues
   all parcels.

.. option:: --hpx:ini=hpx.parcel.openshmem.progress_interval=<n>

   Call OpenSHMEM's progress function on every ``n``-th pass only (``1``: every
   pass, ``0``: never). The default value is ``16``.

.. option:: --hpx:ini=hpx.parcel.openshmem.stop_timeout=<seconds>

   How long shutdown waits for the last pages in flight when no transfer makes
   progress any more. Parcels still in flight after the timeout are dropped. The
   default value is ``30``.
