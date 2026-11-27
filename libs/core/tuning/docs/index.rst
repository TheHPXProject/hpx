..
    Copyright (c) 2026 TheHPXProject

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_tuning:

======
tuning
======

This module measures the properties of the machine that the scheduler's idle
settings depend on and derives the settings from them. The right idle settings
depend on the machine more than on the application: how long the shortest
sleep of the operating system lasts, what it costs to wake a sleeping thread,
how fast a core runs the scheduler's spin rounds, and how the cores are
grouped into NUMA domains and sockets.

The measurements and the rules are plain functions that return plain
structures. They neither write files nor start processes, so a tool can call
them and write the settings to an ini file, and the runtime can call them
itself, for instance during startup. All declarations are in the namespace
``hpx::tuning`` and are available through
``#include <hpx/modules/tuning.hpp>``; the module is also exposed as a
C++ module when |hpx| is built with ``HPX_WITH_CXX_MODULES=ON``.

The module has six parts:

1. :ref:`modules_tuning_statistics`: the summary of repeated
   measurements.
2. :ref:`modules_tuning_machine`: the parts of the topology the
   settings depend on.
3. :ref:`modules_tuning_measurements`: the measurements that need no
   |hpx| runtime.
4. :ref:`modules_tuning_rules`: the rules that turn the measurements
   into settings, and the table of settings by scheduler, tuner and pool size.
5. :ref:`modules_tuning_ini`: the settings of one or more areas as an ini
   file.
6. :ref:`modules_tuning_fingerprint`: what the settings were measured on.

.. _modules_tuning_statistics:

Summaries of repeated measurements
==================================

Every measurement is repeated (21 times by default) and reported as a
``summary``:

.. code-block:: c++

    struct summary
    {
        std::size_t count;
        double min, q1, median, q3, p90, max;
    };

    summary summarize(std::vector<double> values);

``summarize`` sorts the values and interpolates the quantiles linearly; all
fields are 0 for an empty series. The rules compare medians; two values whose
interquartile ranges (``q1`` to ``q3``) overlap are not different.

.. _modules_tuning_machine:

The machine
===========

.. code-block:: c++

    struct machine_description
    {
        std::size_t sockets, numa_domains, cores, pus;
        std::vector<std::size_t> pool_sizes;
    };

    machine_description describe_machine(hpx::threads::topology const& topo);

``describe_machine`` reads the counts from |hpx|'s topology
(``hpx::threads::get_topology()``). ``pool_sizes`` lists the pool sizes the
settings are measured for: the whole domains of the machine, one worker per
core, that is the cores of one NUMA domain, of one socket and of the whole
machine, in ascending order and without repetitions (for example 20 and 40 on
a machine with two sockets of 20 cores, 16, 64 and 128 on one with two sockets
of 64 cores in eight NUMA domains). A pool of another size uses the default
settings (see below).

.. _modules_tuning_measurements:

Measurements without a runtime
==============================

These functions run on the calling thread or on threads of the operating
system; none of them needs a running |hpx| runtime. Each takes a
``microbenchmark_options`` whose ``repeats`` is the number of repetitions
(21 by default).

``summary measure_clock_read(microbenchmark_options const& = {})``
    The cost of one ``std::chrono::steady_clock::now()``, in nanoseconds, from
    1000 reads in a row. It shows whether a clock read per backoff step is
    cheap next to a step.

``summary measure_spin_rate(microbenchmark_options const& = {})``
    The rate of the idle backoff's spin rounds on the calling thread, in rounds
    per microsecond: a round is
    ``scheduling_policy::calibration_pauses_per_round`` pause instructions
    (``HPX_SMT_PAUSE``) and a relaxed load, as the scheduler calibrates its
    spin. The rate depends on the core the thread runs on, since the cores of
    a machine may be of different types; pin the thread before calling it.

``std::vector<timed_wait> measure_timed_waits(double max_us = 1024.0, microbenchmark_options const& = {})``
    The actual lengths of timed waits of a ``std::condition_variable`` that
    nobody notifies, for 1, 2, 4, ... up to ``max_us`` microseconds requested.
    Each ``timed_wait`` holds the ``requested_us`` and the ``actual_us``
    summary. The shortest of them is the shortest sleep of the machine: on
    Linux about the timer slack of the thread (50 us by default), on Windows a
    millisecond or more. A backoff step shorter than that lasts as long as the
    shortest sleep if it is slept.

``wake_cost measure_wake_cost(microbenchmark_options const& = {})``
    The time from a notification to the notified thread running, in
    microseconds, for a thread asleep on a ``std::condition_variable``
    (``asleep_us``) and for a thread spinning on a flag (``spinning_us``),
    with two milliseconds between the rounds so that the sleeping thread is
    really asleep. The difference of the two medians is the cost of a wake up
    outside the scheduler.

These measurements use the threads and the condition variable of the standard
library on purpose: the scheduler's timed waits are those of
``std::condition_variable``, and a woken worker is a thread of the operating
system. Run them on an otherwise idle machine; a busy machine makes every
number longer.

.. _modules_tuning_rules:

Rules and the table of settings
===============================

A rule returns ``tuned_setting``\ s (``hpx/tuning/tuned_setting.hpp``, so that
the rules of other areas don't need the idle rules):

.. code-block:: c++

    struct tuned_setting
    {
        std::string key;       // for instance "hpx.min_idle_backoff_time"
        std::string value;     // as the configuration writes it
        std::string reason;    // the rule and its inputs
        bool is_default;       // whether the value equals HPX's default
    };

The idle rules take the measurements above and the defaults they start from:

.. code-block:: c++

    struct idle_measurements
    {
        std::vector<timed_wait> timed_waits;
        wake_cost wake;
    };

    struct idle_rule_options
    {
        double default_spin_us = 50.0;            // hpx.min_idle_backoff_time
        double default_max_backoff_us = 1000.0;   // hpx.max_idle_backoff_time
    };

    std::vector<tuned_setting> derive_idle_settings(
        idle_measurements const& measured, idle_rule_options const& = {});

``derive_idle_settings`` returns two settings, in whole microseconds:

``hpx.max_idle_backoff_time``
    The longest backoff step: the default, or the shortest sleep if that is
    longer, since a step can't be shorter than a sleep lasts.

``hpx.min_idle_backoff_time``
    The spin: the shortest sleep, at least the cost of a wake up, at most
    ``hpx.max_idle_backoff_time``. A backoff step shorter than the shortest
    sleep lasts as long as that sleep when it is slept, so it is spun instead;
    and spinning about as long as a wake up costs is the competitive rule of
    spinning before blocking. With a coarse timer, a shortest sleep longer
    than the default longest step (on Windows the 15.6 ms tick of the system
    timer), the spin stays the default: a notification wakes a sleeping
    worker after about the cost of a wake up, so the shortest sleep only
    says how long an idle worker stays asleep, and spinning that long would
    keep every idle worker busy. The reason of the setting says so and
    records the wake cost.

Without timed waits measured, both keep their defaults and say so in their
reason. Two helpers give the inputs of the rules: ``shortest_sleep_us``, the
median actual length of the shortest timed wait, and ``wake_cost_us``, the
difference of the medians of ``wake_cost``, at least 0.

The settings depend on the scheduler, on the tuner a scheduler uses and on the
size of the pool, so they are kept as a table:

.. code-block:: c++

    struct tuning_row
    {
        std::string scheduler;    // as --hpx:queuing names it
        std::string tuner;        // as hpx.scheduler_tuner names it, or "none"
        std::size_t pool_size;    // the number of workers of the pool, or
                                  // any_pool_size
        std::vector<tuned_setting> settings;
    };

    inline constexpr std::size_t any_pool_size = 0;

    std::string section_name(tuning_row const& row);
    std::vector<ini_section> to_sections(std::vector<tuning_row> const& rows);
    std::string to_ini(std::vector<tuning_row> const& rows,
        std::vector<std::string> const& comments, int year);

``section_name`` gives the ini section of a row,
``hpx.idle_tuning.<scheduler>.<tuner>.<pool size>``, or
``hpx.idle_tuning.<scheduler>.<tuner>`` for a row of ``any_pool_size``, which
holds for pools of every size, with the tuner ``none`` if the row has none.
``to_sections`` turns the rows into ini sections (below), one
per row, so that they can be written into one file with the sections of other
areas; ``to_ini`` of a table writes those sections alone. For example:

.. code-block:: ini

    # Copyright (c) 2026 TheHPXProject
    #
    # SPDX-License-Identifier: BSL-1.0
    # Distributed under the Boost Software License, Version 1.0. (See accompanying
    # file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

    [hpx.idle_tuning.local-priority-fifo.none.20]
    # shortest sleep 55.2 us (HPX's default)
    max_idle_backoff_time = 1000
    # shortest sleep 55.2 us, wake cost 18.1 us
    min_idle_backoff_time = 55

A pool takes the row of its scheduler, tuner and number of workers, else the
row of its scheduler and tuner for any pool size, else keeps the default
settings. As long as the measured settings don't depend on the pool size,
``hpx_tune`` writes only the rows for any pool size. Such a file is used only
when it is passed explicitly with ``--hpx:config``.

.. _modules_tuning_ini:

Settings as an ini file
=======================

Any area writes its settings as plain sections, and one file holds the
sections of all areas:

.. code-block:: c++

    struct ini_section
    {
        std::string name;                       // for instance "hpx.parcel"
        std::vector<tuned_setting> settings;
    };

    std::string to_ini(std::vector<ini_section> const& sections,
        std::vector<std::string> const& comments, int year);

``to_ini`` writes the license header of |hpx|'s files with the given year, so
that a generated file can be added to the repository as it is, then the given
comment lines (the host, the date, the tool that wrote it), then the sections
in the given order. Each setting is preceded by a comment with its reason (and
whether it equals the default). A key is written relative to its section: a
key that starts with the section's name and a dot loses that part
(``hpx.parcel.tcp.enable`` is ``enable`` in the section ``hpx.parcel.tcp``),
any other loses its leading ``hpx.`` (``hpx.min_idle_backoff_time`` is
``min_idle_backoff_time`` in an idle tuning section).

.. _modules_tuning_fingerprint:

The host fingerprint
====================

A file of tuned settings records what it was measured on, so that a profile
used on another machine can be recognized:

.. code-block:: c++

    struct fingerprint_entry
    {
        std::string key;        // a single word, for instance "cpu_model"
        std::string value;
        bool compared = true;   // false: for information only
    };

    struct host_fingerprint
    {
        std::vector<fingerprint_entry> entries;

        void add(std::string key, std::string value, bool compared = true);
        fingerprint_entry const* find(std::string_view key) const noexcept;
    };

    host_fingerprint describe_host(hpx::threads::topology const& topo);
    std::vector<std::string> to_comments(host_fingerprint const& f);
    ini_section to_section(host_fingerprint const& f);
    std::vector<std::string> mismatches(
        host_fingerprint const& profile, host_fingerprint const& here);

``describe_host`` gives the core entries: ``host_name`` (not compared, as a
profile is for the hosts of a partition), ``cpu_model``, ``architecture``,
``sockets``, ``numa_domains``, ``cores``, ``pus``, ``os_name``,
``os_release``, ``os_version`` (from the topology, that is from hwloc),
``hpx_version``, ``hpx_commit``, ``hpx_build_type``, ``compiler`` and
``stdlib`` (from |hpx|'s version). Other tuning areas append their own
entries with ``add``, their keys starting with the area's name
(``parcel_conduits``). Keys are single words with underscores, since a dot
would read as a further section level in an ini file.

``to_comments`` gives one comment line per entry (``fingerprint: cpu_model =
...``) and ``to_section`` the section ``[hpx.tuning.fingerprint]`` with one
key per entry, so that the runtime can read it back. In that section a ``$``
that is followed by ``{`` or ``[`` is written doubled, so that the ini reader
takes it literally instead of expanding it. ``mismatches`` lists the compared
entries in which a profile's fingerprint and this host's differ, or that only
one of them has.

See the :ref:`API reference <modules_tuning_api>` of this module for
more details.
