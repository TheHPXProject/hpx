..
    Copyright (c)      2021 ETH Zurich
    Copyright (c) 2007-2023 Louisiana State University

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

===========================
Release procedure for |hpx|
===========================

Below is a step by step procedure for making an |hpx| release. We aim to produce two
releases per year: one in March-April, and one in September-October.

This is a living document and may not be totally current or accurate. It is an
attempt to capture current practices in making an |hpx| release. Please update it
as appropriate.

One way to use this procedure is to print a copy and check off the lines as they
are completed to avoid confusion.

#. Notify developers that a release is imminent.

#. For minor and major releases: create and check out a new branch at an
   appropriate point on ``master`` with the name ``release-major.minor.X``.
   ``major`` and ``minor`` should be the major and minor versions of the
   release. For patch releases: check out the corresponding
   ``release-major.minor.X`` branch.

#. Write release notes in ``docs/sphinx/releases/whats_new_$VERSION.rst``. Keep
   adding merged PRs and closed issues to this until just before the release is
   made. Use ``tools/generate_pr_issue_list.sh`` to generate the lists. Add the
   new release notes to the table of contents in ``docs/sphinx/releases.rst``.

#. Build the docs, and proof-read them. Update any documentation that may have
   changed, and correct any typos. Pay special attention to:

   * ``$HPX_SOURCE/README.rst``

     * Update grant information

   * ``docs/sphinx/releases/whats_new_$VERSION.rst``
   * ``docs/sphinx/about_hpx/people.rst``

     *   Update collaborators
     *   Update grant information

#. This step does not apply to patch releases. For APEX:

   * Change the release branch to be the most current release tag available in
     the APEX ``git_external`` section in the main ``CMakeLists.txt``.
     Please contact the maintainers of the respective packages to generate a new
     release to synchronize with the |hpx| release (`APEX
     <http://github.com/UO-OACISS/xpress-apex>`_).

#. Make sure ``HPX_VERSION_MAJOR/MINOR/SUBMINOR`` in ``CMakeLists.txt`` contain
   the correct values. Change them if needed.

#. Change version references in CITATION.cff. There are two occurrences. Change
   year in the copyright file under ``/libs/core/version/src/version.cpp``.

#. This step does not apply to patch releases. Remove features which have been
   deprecated for at least 2 releases. This involves removing build options
   which enable those features from the main CMakeLists.txt and also deleting
   all related code and tests from the main source tree.

   The general deprecation policy involves a three-step process we have to go
   through in order to introduce a breaking change:

   a. First release cycle: add a build option that allows for explicitly disabling
      any old (now deprecated) code.
   b. Second release cycle: turn this build option OFF by default.
   c. Third release cycle: completely remove the old code.

   The main CMakeLists.txt contains a comment indicating for which version
   the breaking change was introduced first.
   In the case of deprecated features which don't have a replacement yet, we
   keep them around in case (like Vc for example).

#. Update the minimum required versions if necessary (compilers, dependencies,
   etc.) in ``prerequisites.rst``. Update latest tested versions.

#. Verify that the Jenkins setups for the release branch on Rostam
   are running and do not display any errors.

#. Repeat the following steps until satisfied with the release.

   #. Change ``HPX_VERSION_TAG`` in ``CMakeLists.txt`` to ``-rcN``, where ``N``
      is the current iteration of this step. Start with ``-rc1``.

   #. Create a pre-release on GitHub using the script ``tools/roll_release.sh``.
      This script automatically tag with the corresponding release number.
      The script requires that you have the |stellar| Group signing key.

   #. This step is not necessary for patch releases. Notify
      ``hpx-users@stellar-group.org`` of the
      availability of the release candidate. Ask users to test the candidate by
      checking out the release candidate tag.

   #. Allow at least a week for testing of the release candidate.

      * Use ``git merge`` when possible, and fall back to ``git cherry-pick``
        when needed. For patch releases ``git cherry-pick`` is most likely your
        only choice if there have been significant unrelated changes on master
        since the previous release.
      * Go back to the first step when enough patches have been added.
      * If there are no more patches, continue to make the final release.

#. Update any occurrences of the latest stable release to refer to the version
   about to be released. For example, ``quickstart.rst`` contains instructions
   to check out the latest stable tag. Make sure that refers to the new version.

#. Add a new entry to the RPM changelog (``cmake/packaging/rpm/Changelog.txt``)
   with the new version number and a link to the corresponding changelog.

#. Change ``HPX_VERSION_TAG`` in ``CMakeLists.txt`` to an empty string.

#. Add the release date to the caption of the current "What's New" section in
   the docs, and change the value of ``HPX_VERSION_DATE`` in
   ``CMakeLists.txt``.

#. Create a release on GitHub using the script ``tools/roll_release.sh``. This
   script automatically tag the with the corresponding release number. The
   script requires that you have the |stellar| Group signing key.

#. Update the website (`hpx.dev <https://hpx.dev>`_). You can login on
   wordpress through `this page <https://hpx.dev/wp-login.php>`.
   You can update the pages with the following:

   * Update links on the downloads page. Link to the release on GitHub.
   * Documentation links on the docs page (link to generated documentation on
     GitHub Pages). Follow the style of previous releases.
   * A new blog post announcing the release, which links to downloads and the
     "What's New" section in the documentation (see previous releases for
     examples).

#. Merge release branch into master.

#. Post-release cleanup. Create a new pull request against master with the
   following changes:

   #. Modify the release procedure if necessary.

   #. Change ``HPX_VERSION_TAG`` in ``CMakeLists.txt`` back to ``-trunk``.

   #. Increment ``HPX_VERSION_MINOR`` in ``CMakeLists.txt``.

#. Update Vcpkg (``https://github.com/Microsoft/vcpkg``) to pull from latest
   release.

   * Update version number in CONTROL
   * Update tag and SHA512 to that of the new release

#. Update spack (``https://github.com/spack/spack``) with the latest HPX package.

   * Update version number in ``hpx/package.py`` and SHA256 to that of the new
     release

#. Announce the release on hpx-users@stellar-group.org, stellar@cct.lsu.edu,
   allcct@cct.lsu.edu, faculty@csc.lsu.edu, faculty@ece.lsu.edu,
   the |hpx| Slack channel, the STEllAR-GROUP Discord channel,
   our list of external collaborators, isocpp.org, reddit.com, HPC Wire, Inside
   HPC, Heise Online, and a CCT press release.

#. Beer and pizza.
