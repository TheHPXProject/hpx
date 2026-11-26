..
    Copyright (c) 2026 TheHPXProject

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_parcel_tuning:

=============
parcel_tuning
=============

This module measures how fast the parcel layer moves data between two
localities in the configurations it can use, and derives the parcel layer's
settings from what it measured. Which parcelport is fastest, and whether a
device conduit pays off below a parcelport, depends on the network and its
libraries more than on the application: on an InfiniBand network the UCX
device conduit makes the TCP parcelport about as fast as the MPI parcelport
for large host data, and gains nothing below the MPI parcelport, which already
moves each chunk over the same network.

As in the ``tuning`` module, whose structures it uses (``summary``,
``tuned_setting``, ``ini_section``), the measurements and the rules are plain
functions; they neither write files nor start processes. The tool
``hpx_tune`` (``tools/tuners``) calls them with ``--parcel``: it starts a
child of two localities for every configuration it measures, and writes the
settings the rules return to an ini file, to be passed to applications with
``--hpx:config``. All declarations are in the namespace ``hpx::tuning`` and
are available through ``#include <hpx/modules/parcel_tuning.hpp>``. The
module exists only where |hpx| is built with networking.

The module has two parts:

- ``transfers.hpp``: ``measure_transfers`` measures the round trips of
  vectors of the given sizes (by default the powers of 2 from 1 KiB to
  64 MiB) to another locality, both as a result returned from there (get) and
  as an argument sent there (set), every round checked, with the number of
  chunks the device conduit moved per round, so that a caller can tell
  whether the data went through the conduit. Before the first size it sends
  data until the device conduit moves it, if the conduit moves host chunks
  at all, as the conduit checks that it reaches a destination first.
- ``parcel_settings.hpp``: the rules. For each parcelport, the measurements
  of host data through the parcelport alone and with the device conduit
  moving the large host chunks give
  ``hpx.parcel.<parcelport>.conduit_host_chunks`` (1 if the conduit is
  faster from some size on, up to the largest size measured; 0 if it isn't
  faster at the largest size) and ``hpx.parcel.conduit_host_threshold`` (the
  largest of those sizes, at least the zero-copy threshold). A size counts as
  faster if its median, get and set together, is lower by more than 5% and
  the interquartile ranges don't overlap. Which configuration is fastest by
  size is reported only: choosing the parcelport is left to the user. A rule
  whose measurements are missing, wrong, or didn't go through the conduit
  writes nothing and says why.

This is the first version of the parcel area of ``hpx_tune``: host data with
the TCP and MPI parcelports, with and without the UCX device conduit. Device
data, the other parcelports and the settings that depend on the application
(message coalescing, connection counts) are not tuned yet.
