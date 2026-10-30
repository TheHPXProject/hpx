..
    Copyright (C)      2013 Thomas Heller

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

=============
Testing |hpx|
=============

To ensure correctness of |hpx|, we ship a large variety of unit and regression
tests. The tests are driven by the |ctest|_ tool and are executed automatically 
on each commit to the |hpx| |hpx_github|_ repository. In addition, it is encouraged 
to run the test suite manually to ensure proper operation on your target system. 
If a test fails for your platform, we highly recommend submitting an issue on our 
|hpx_github_issues|_ tracker with detailed information about the target system.

Running tests manually
======================

Running the tests manually is as easy as typing ``make tests && make test``.
This will build all tests and run them once the tests are built successfully.
After the tests have been built, you can invoke separate tests with the help of
the ``ctest`` command. You can list all available test targets using ``make help
| grep tests``. Please see the |ctest_doc|_ for further details.

Running performance tests
=========================

For each pull request, Jenkins runs the performance tests on Rostam (see
``.jenkins/lsu-perftests``) and posts a comparison with the reference results
on the pull request. To run a performance test yourself, configure |hpx| with
``-DHPX_WITH_TESTS_RUN_BENCHMARKS=ON`` (and ``-DHPX_WITH_NANOBENCH=ON``, as the
CI does) and build the target ``<name>_perftest``, e.g.
``benchmark_merge_perftest``. It runs the benchmark 30 times (or as often as
the ``N_EXECUTIONS`` argument of ``add_hpx_performance_report_test`` says) and
writes the results to ``tools/perftests_ci/results/<name>.json`` in the build
directory. Then build the target ``perftests_plot``, which compares all results
found there with the reference results, and writes the measurements and their
plots to ``tools/perftests_ci/index.html``.

Adding new performance tests
============================

To add a new performance test, wrap the portion of code to benchmark with
``hpx::util::perftests_report``, passing the test name, the executor name, the
number of repetitions and the function to time (can be a lambda). Add the
command line options of this facility with ``hpx::util::perftests_cfg`` in
``main``, call ``hpx::util::perftests_init(vm, "<name>")`` at the beginning of
``hpx_main``, using the name of the test, and call
``hpx::util::perftests_print_times`` at the end to print the results in the
json format used for the comparison. See ``future_overhead_report.cpp`` for an
example.

To add the test to the CI report, register it with
``add_hpx_performance_report_test`` if ``HPX_WITH_TESTS_RUN_BENCHMARKS`` is
enabled (see ``libs/core/algorithms/tests/performance/CMakeLists.txt``); the CI
builds and runs all registered performance tests.
Finally, add the reference results. The timings differ between the nodes the
CI may run on, so each node has its own reference results in
``tools/perftests_ci/perftest/references/<node>.tar.gz``, and the comparison
uses the ones of the node the benchmarks ran on. If a benchmark has no
reference on that node, because it is new or its measurements changed, the CI
makes new references of all benchmarks of the node from the run, so that they
are measured together, and always uploads them, packed, to
``TheHPXProject/hpx-perftest-images`` (``pr/<pull request>/<build>/references``
or ``branch/<branch>/<build>/references``); the report links to them. Copy the
archive to ``tools/perftests_ci/perftest/references`` to use them. To update
a single reference by hand, build the ``<name>_perftest`` target on that node,
copy ``tools/perftests_ci/results/<name>.json`` from the build directory to the
unpacked reference results of the node, and pack them again with
``tools/perftests_ci/pack_references.sh`` (see the comment at the top of the
script).
The CI comments on a pull request only about measurements that got more than
5% faster or slower; the pull request comment ``retest lsu perf full`` starts a
run that reports all measurements.
Every CI run also keeps all of its results packed as references of its node
(``perftests-reports/references/<node>.tar.gz`` in the Jenkins artifacts).

The performance tests also run for the commits of the master branch, always
on the same node (``configuration_slurm_history_node`` in
``.jenkins/lsu-perftests/slurm-constraint-perftests.sh``, a node the build
matrix doesn't use) and always reporting all measurements. Each of these runs is
added to the performance history in
`hpx-performance <https://github.com/TheHPXProject/hpx-performance>`_
(``.jenkins/lsu-perftests/upload_history.sh``): a directory
``<year>/<node>/<commit date>-<commit>-b<build>`` with all results, their
medians, the comparison with the references (without the plots, which
``perftests_plot.py`` can draw again from the results), and the commit (hash,
dates, pull request), when the benchmarks ran, the Jenkins build and the
configuration in ``metadata.json``. ``runs.tsv`` lists all runs, and
``timeline.csv`` the medians of all measurements of all runs, which the
README of the repository shows as a graph per benchmark over time (drawn by
``tools/perftests_ci/perftests_timeline.py`` with every new run).
``tools/perftests_ci/perftests_history.py`` rebuilds ``timeline.csv`` from the
run directories of a clone of the repository, optionally for one node, one
benchmark or a range of dates.

Issue tracker
=============

If you stumble over a bug or missing feature in |hpx|, please
submit an issue to our |hpx_github_issues|_ page. For more information on how to
submit support requests or other means of getting in contact with the developers,
please see the |support|_ page.

Continuous testing
==================

In addition to manual testing, we run automated tests on various platforms. We also 
run tests on all pull requests using both |github|_. You can see the dashboards
|hpx_cdash|_ .
