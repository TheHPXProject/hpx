..
    Copyright (c) 2026 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _using_the_gasnet_parcelport:

===========================
Using the GASNet parcelport
===========================

.. _info_gasnet:

Basic information
=================

The GASNet parcelport transports parcels with `GASNet-EX`_. Every locality
attaches one registered GASNet segment, and parcels are moved as pages with
one-sided remote memory access (RMA) into ring buffers carved out of that
segment. The parcelport issues GASNet calls from several threads at once, so it
requires a GASNet-EX installation built in PAR (thread-multiple) mode.

GASNet-EX supports several networks through its *conduits*. |hpx| is built for
exactly one conduit, selected with ``HPX_WITH_PARCELPORT_GASNET_CONDUIT``. The
``mpi`` conduit runs everywhere an MPI library is available and is started like
an MPI program. The ``ibv`` conduit needs InfiniBand: it does not support RoCE
or iWARP networks.

.. _`GASNet-EX`: https://gasnet.lbl.gov

.. _build_gasnet:

Build GASNet-EX
===============

|hpx| does not download or build GASNet-EX; install it first. The build must
fulfill these requirements, which |cmake|_ checks while configuring |hpx|:

* PAR threading mode (``--enable-par``). SEQ and PARSYNC builds are rejected.
* Position-independent code (``-fPIC`` for all compiler and linker flags):
  GASNet-EX installs static libraries, which are linked into |hpx|'s shared
  libraries.
* The conduit selected for |hpx| (for example ``--enable-mpi``).
* For the ``mpi`` conduit: the MPI library used to build GASNet-EX, and a UCX
  installation built with multithreading support (``--enable-mt``) that
  ``pkg-config`` can find as ``ucx``.

A configuration known to work, with the C and C++ compilers of the system and an
MPI library installed in ``$MPI_ROOT``:

.. code-block:: shell-session

   $ ./configure --prefix=$GASNET_ROOT \
       --with-cflags="-O3 -fPIC" --with-cxxflags="-O3 -fPIC" \
       --with-ldflags="-fPIC" --with-mpi-cflags="-O3 -fPIC" \
       --enable-par --enable-pthreads --enable-segment-large \
       --enable-mpi --with-mpi-cc=$MPI_ROOT/bin/mpicc \
       --enable-smp --enable-udp --enable-ibv --disable-ucx --disable-ofi
   $ make -j && make install

GASNet-EX describes each conduit and threading mode in a ``pkg-config`` file,
for example ``$GASNET_ROOT/lib/pkgconfig/gasnet-mpi-par.pc``. |hpx| finds
GASNet-EX only through these files.

GASNet-EX may be built with a different compiler than |hpx|. Its
``pkg-config`` files then name the runtime library directory of that compiler;
|hpx| drops these entries, so that |hpx| links against the runtime of its own
compiler.

.. _build_gasnet_pp:

Build |hpx| with the GASNet parcelport
======================================

Make the ``pkg-config`` files of GASNet-EX visible, either through
``PKG_CONFIG_PATH`` or by passing the installation prefix, and enable the
parcelport:

.. code-block:: shell-session

   $ export PKG_CONFIG_PATH=$GASNET_ROOT/lib/pkgconfig:$PKG_CONFIG_PATH
   $ cmake -S hpx -B build \
       -DHPX_WITH_PARCELPORT_GASNET=ON \
       -DHPX_WITH_PARCELPORT_GASNET_CONDUIT=mpi \
       -DHPX_WITH_PARCELPORT_MPI=ON -DMPI_HOME=$MPI_ROOT

.. option:: HPX_WITH_PARCELPORT_GASNET

   Enable the GASNet parcelport. The default value is ``OFF``. All the following
   variables only take effect when this variable is set to ``ON``.

.. option:: HPX_WITH_PARCELPORT_GASNET_CONDUIT

   The GASNet-EX conduit |hpx| is built for, one of ``smp``, ``udp``, ``mpi``,
   ``ofi``, ``ucx`` and ``ibv``. The default value is ``udp``. |hpx| uses the
   ``pkg-config`` module ``gasnet-<conduit>-par``. With the ``mpi`` conduit,
   |hpx| also sets up MPI, so ``MPI_HOME`` (or the MPI compiler wrappers on
   ``PATH``) must point to the MPI library GASNet-EX was built with.

.. option:: GASNet_ROOT

   The installation prefix of GASNet-EX. It is added to ``CMAKE_PREFIX_PATH``,
   which ``pkg-config`` searches as well. Setting ``PKG_CONFIG_PATH`` instead
   works as well.

.. _run_gasnet_pp:

Run |hpx| applications with the GASNet parcelport
=================================================

An |hpx| application built with the GASNet parcelport has to be started by the
job launcher of the GASNet-EX conduit: every locality is one GASNet process, and
the localities find each other through the conduit's bootstrap. For the ``mpi``
conduit that launcher is ``mpiexec`` (or ``mpirun``, or ``srun`` with an MPI
library built with Slurm support).

Make the GASNet parcelport the one that carries the parcels and bootstraps the
localities, and switch off the other parcelports of the build: the MPI, LCI and
LCW parcelports would initialize MPI in every GASNet process, and the OpenSHMEM
parcelport would start its own runtime. The ``key!=value`` form is accepted also
for parcelports that are not part of the build:

.. code-block:: shell-session

   $ mpiexec -n 4 ./my_hpx_application \
       --hpx:ini=hpx.parcel.gasnet.priority=1000 \
       --hpx:ini=hpx.parcel.gasnet.enable=1 \
       --hpx:ini=hpx.parcel.bootstrap=gasnet \
       --hpx:ini=hpx.parcel.mpi.enable!=0 \
       --hpx:ini=hpx.parcel.lci.enable!=0 \
       --hpx:ini=hpx.parcel.lcw.enable!=0 \
       --hpx:ini=hpx.parcel.tcp.enable!=0 \
       --hpx:ini=hpx.parcel.openshmem.enable!=0 \
       --hpx:ini=hpx.max_idle_loop_count=0

Pass each setting in its own ``--hpx:ini`` option. Parcels arriving through
GASNet do not wake idle worker threads; ``hpx.max_idle_loop_count=0`` keeps
them polling, which avoids a delay of about 80 ms per message for applications
with many round trips.

The GASNet parcelport is enabled by default when |hpx| is built with it. To run
an application of such a build with another parcelport, switch it off with
``--hpx:ini=hpx.parcel.gasnet.enable=0``; otherwise every process starts a
GASNet job of its own.

``hpxrun.py`` adds all these settings with ``-p gasnet`` (and switches GASNet
off for the other parcelports). The run wrapper ``-r gasnet`` starts the
application with ``mpiexec -n <localities>``, which fits the ``mpi`` conduit and
conduits started through GASNet's MPI spawner. The run wrapper ``-r
gasnet-smp`` is meant for the ``smp`` conduit: it starts the application once
with ``GASNET_PSHM_NODES=<localities>``, and GASNet-EX forks the processes on
the local node:

.. code-block:: shell-session

   $ hpxrun.py ./my_hpx_application -l 4 -t 8 -p gasnet -r gasnet

Both run wrappers set ``GASNET_QUIET=yes``. Other conduits and spawners are
started with the launchers that GASNet-EX installs in ``$GASNET_ROOT/bin`` (for
example ``gasnetrun_ibv``), as described in the README of each conduit; pass the
settings above to the application in that case.

Launch one locality per GASNet process: the parcelport does not support more
than one |hpx| locality in a process. The number of worker threads of a
locality is set as usual with ``--hpx:threads``.

The following settings configure the parcelport at runtime. ``mtu``, ``slots``
and ``checksum`` are part of the wire protocol and must be the same on all
localities; at startup the parcelport compares them across all localities
unless ``hpx.parcel.gasnet.uniform_check`` is ``0``.

.. option:: --hpx:ini=hpx.parcel.gasnet.mtu=<bytes>

   Size of a page, the unit in which parcels are transferred. The default value
   ``0`` selects it automatically: as large as 1 MiB, but smaller for large jobs,
   so that the receive and send rings stay within about 256 MiB of the
   registered segment.

.. option:: --hpx:ini=hpx.parcel.gasnet.slots=<n>

   Number of pages in flight between each pair of localities. The default value
   is ``8``.

.. option:: --hpx:ini=hpx.parcel.gasnet.checksum=<0|1>

   Verify a checksum over every received page. The default value is ``1``.

.. option:: --hpx:ini=hpx.parcel.gasnet.uniform_check=<0|1>

   Compare ``mtu``, ``slots`` and ``checksum`` across all localities at startup.
   The default value is ``1``.

.. option:: --hpx:ini=hpx.parcel.gasnet.scan_window=<n>

   Maximum number of source localities checked for new pages in one pass of the
   receiver. ``0`` checks all of them in every pass. The default value is
   ``8``.

.. option:: --hpx:ini=hpx.parcel.gasnet.worker_progress=<n>

   Which threads make progress on the network: ``0`` (the default) only the
   parcelport's I/O threads, ``1`` all worker threads, ``n`` > 1 the worker
   threads ``0`` to ``n-1``. Progress from worker threads is known to hang some
   runs with several threads per locality.

.. option:: --hpx:ini=hpx.parcel.gasnet.progress_interval=<n>

   Poll GASNet-EX for network progress on every ``n``-th pass of the
   parcelport's background work (``1``: every pass). The default value is
   ``1``.

.. option:: --hpx:ini=hpx.parcel.gasnet.stop_timeout=<seconds>

   How long shutdown waits for the last pages in flight when no transfer makes
   progress any more. Parcels still in flight after the timeout are dropped. The
   default value is ``30``.

The parcelport sets ``GASNET_NETWORKDEPTH=64`` unless the variable is already
set. All other GASNet-EX environment variables (for example
``GASNET_MAX_SEGSIZE`` or ``GASNET_IBV_PORTS``) are passed to GASNet-EX
unchanged.
