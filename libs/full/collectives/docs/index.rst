..
    Copyright (c) 2019-2022 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_collectives:

===========
collectives
===========

The collectives module exposes a set of distributed collective operations. Those
can be used to exchange data between participating sites in a coordinated way.
At this point the module exposes the following collective primitives:

* :hpx:func:`hpx::collectives::all_gather`: receives a set of values from all
  participating sites.
* :hpx:func:`hpx::collectives::all_reduce`: performs a reduction on data from
  each participating site to each participating site.
* :hpx:func:`hpx::collectives::all_to_all`: each participating site provides its
  element of the data to collect while all participating sites receive the data
  from every other site.
* :hpx:func:`hpx::collectives::broadcast_to` and
  :hpx:func:`hpx::collectives::broadcast_from`: performs a broadcast operation
  from a root site to all participating sites.
* :hpx:func:`hpx::collectives::exclusive_scan`: performs an exclusive scan operation
  on a set of values received from all call sites operating on the given base name.
* :hpx:func:`hpx::collectives::gather_here` and
  :hpx:func:`hpx::collectives::gather_there`: gathers values from all
  participating sites.
* :hpx:func:`hpx::collectives::inclusive_scan`: performs an inclusive scan operation
  on a set of values received from all call sites operating on the given base name.
* :hpx:func:`hpx::collectives::reduce_here` and
  :hpx:func:`hpx::collectives::reduce_there`: performs a reduction on data from each
  participating site to a root site.
* :hpx:func:`hpx::collectives::scatter_to` and
  :hpx:func:`hpx::collectives::scatter_from`: receives an element of a set of values
  operating on the given base name.

* :hpx:func:`hpx::lcos::broadcast`: performs a given action on all given global
  identifiers.
* :hpx:class:`hpx::distributed::barrier`: distributed barrier.
* :hpx:func:`hpx::lcos::fold`: performs a fold with a given action on all given
  global identifiers.
* :hpx:class:`hpx::distributed::latch`: distributed latch.
* :hpx:func:`hpx::lcos::reduce`: performs a reduction on data from each
  given global identifiers.
* :hpx:class:`hpx::lcos::spmd_block`: performs the same operation on a local
  image while providing handles to the other images.

See the :ref:`API reference <modules_collectives_api>` of the module for more
details.

Hierarchical collectives
------------------------

Hierarchical communicators split a collective into sub-communicators arranged as
a tree. This reduces the number of participants in each individual operation,
but it also means that one user-visible hierarchical call can touch several
internal communicators.

Using a hierarchical communicator
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Create a :hpx:struct:`hpx::collectives::hierarchical_communicator` with
:hpx:func:`hpx::collectives::create_hierarchical_communicator`. Every
participating site has to call it with the same base name, number of sites,
arity, generation, root site, and flat fallback threshold, and with its own site
index. These arguments decide the tree of communicators that each site builds,
so a mismatch can make the collective operations hang:

.. literalinclude:: ../../../../../libs/full/collectives/examples/hierarchical_collectives.cpp
   :language: c++
   :start-after: //[hierarchical_collectives_create
   :end-before: //]

The resulting communicator can be passed to the collective operations in place
of a flat communicator. All of
:hpx:func:`hpx::collectives::all_gather`,
:hpx:func:`hpx::collectives::all_reduce`,
:hpx:func:`hpx::collectives::all_to_all`,
:hpx:func:`hpx::collectives::broadcast_to` and
:hpx:func:`hpx::collectives::broadcast_from`,
:hpx:func:`hpx::collectives::gather_here` and
:hpx:func:`hpx::collectives::gather_there`,
:hpx:func:`hpx::collectives::reduce_here` and
:hpx:func:`hpx::collectives::reduce_there`,
:hpx:func:`hpx::collectives::scatter_to` and
:hpx:func:`hpx::collectives::scatter_from`,
:hpx:func:`hpx::collectives::inclusive_scan`,
:hpx:func:`hpx::collectives::exclusive_scan`, and ``hpx::collectives::barrier``
accept it. The example below runs three different operations on one shared
communicator. It keeps a single generation counter and increments it for every
call, whatever the operation, as described in the next section:

.. literalinclude:: ../../../../../libs/full/collectives/examples/hierarchical_collectives.cpp
   :language: c++
   :start-after: //[hierarchical_collectives_use
   :end-before: //]

The complete program is
:hpx-header:`libs/full/collectives/examples,hierarchical_collectives.cpp`. It
runs ``hpx_main`` on all localities, so start it on several localities, for
example with ``hpxrun.py -l 4 -t 1 bin/hierarchical_collectives -- --arity=2``.

A few rules apply to all hierarchical collectives:

* The arity is the number of children of each node in the tree. It must be at
  least 2 and defaults to 2. A small arity gives a deeper tree with fewer
  participants per sub-communicator, a large arity gives a flatter tree with
  fewer steps. The number of sites does not have to be a power of the arity.
* If the number of sites is smaller than the flat fallback threshold
  (:hpx:struct:`hpx::collectives::flat_fallback_threshold_arg`, 16 by
  default), the communicator collapses to a single flat communicator spanning
  all sites. Pass ``flat_fallback_threshold_arg(0)`` to disable this fallback.
  The communicator is still a single flat communicator if the arity is not
  smaller than the number of sites.
* Any site can be the root of the tree, but all sites have to pass the same
  ``root_site_arg``. Only the root site may call the root-side operations
  (``broadcast_to``, ``gather_here``, ``reduce_here``, and ``scatter_to``).
  The operations that take a ``root_site_arg`` although every site receives
  the result (``all_gather``, ``all_reduce``, the scans, and ``barrier``)
  accept only 0 (the default) or the root of the communicator, and report
  ``bad_parameter`` for any other site.
* The futures returned by the asynchronous hierarchical operations are not
  fully non-blocking: walking the tree of communicators can wait internally
  before the returned future is ready.

Generation model
~~~~~~~~~~~~~~~~

Hierarchical collectives use two internal generations for each user generation.
For a user generation ``k``, the first internal phase uses ``2k - 1`` and the
second uses ``2k``. Single-pass hierarchical operations and the inter-group
exchange of :hpx:func:`hpx::collectives::all_to_all` advance the communicator by
two generations in one step so that shared hierarchical communicators stay
aligned with two-phase collectives.

When a hierarchical communicator instance is shared between collective
operations, callers must pass explicit, strictly consecutive positive
generation numbers. Two-phase hierarchical collectives, including
:hpx:func:`hpx::collectives::all_gather`,
:hpx:func:`hpx::collectives::all_reduce`,
:hpx:func:`hpx::collectives::all_to_all`,
:hpx:func:`hpx::collectives::inclusive_scan`,
:hpx:func:`hpx::collectives::exclusive_scan`, and
:hpx:class:`hpx::distributed::barrier`, reject the default generation because
their phase generations are derived from the explicit user generation.
