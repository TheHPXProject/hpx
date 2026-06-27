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

We run performance tests on Piz Daint for each pull request using Jenkins. To
run those performance tests locally or on Piz Daint, a script is provided under
``tools/perftests_ci/local_run.sh`` (to be run in the build directory specifying
the |hpx| source directory as the argument to the script, default is
``$HOME/projects/hpx_perftests_ci``.

Adding new performance tests
============================

To add a new performance test, you need to wrap the portion of code to benchmark
with ``hpx::util::perftests_report``, passing the test name, the executor name
and the function to time (can be a lambda). This facility is used to output the
time results in a json format (format needed to compare the results and plot
them).  To effectively print them at the end of your test, call
``hpx::util::perftests_print_times``. To see an example of use, see
``future_overhead_report.cpp``.  Finally, you can add the test to the CI report
editing the ``hpx_targets`` variable for the executable name and the
``hpx_test_options`` variable for the corresponding options to use for the run
in the performance test script ``.jenkins/cscs-perftests/launch_perftests.sh``.
And then run the ``tools/perftests_ci/local_run.sh`` script to get a reference
json run (use the name of the test) to be added in the
``tools/perftests_ci/perftest/references/daint_default`` directory.

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
