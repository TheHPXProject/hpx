..
    Copyright (c) 2007-2026 Louisiana State University

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

|github_status| |codacy_quality| |OpenSSF| |CFF| |JOSS| |zenodo_doi|

Documentation: `latest <https://docs.hpx.dev/latest/html/index.html>`_,
`development (master) <https://docs.hpx.dev/branches/master/html/index.html>`_, also |deepwiki|

|hpx_logo|

------

HPX is a C++ Standard Library for Concurrency and Parallelism. It implements all
of the corresponding facilities as defined by the C++ Standard. Additionally, in
HPX we implement functionalities proposed as part of the ongoing C++
standardization process. We also extend the C++ Standard APIs to the distributed
case.

The goal of HPX is to create a high quality, freely available, open source
implementation of a new programming model for conventional systems, such as
classic Linux based Beowulf clusters or multi-socket highly parallel SMP nodes.
At the same time, we want to have a very modular and well designed runtime
system architecture which would allow us to port our implementation onto new
computer system architectures. We want to use real-world applications to drive
the development of the runtime system, coining out required functionalities and
converging onto a stable API which will provide a smooth migration path for
developers.

The API exposed by HPX is not only modeled after the interfaces defined by the
C++11/14/17/20/23 ISO standard, it also adheres to the programming guidelines used
by the Boost collection of C++ libraries. We aim to improve the scalability of
today's applications and to expose new levels of parallelism which are necessary
to take advantage of the exascale systems of the future.

What's so special about HPX?
============================

* HPX exposes a uniform, standards-oriented API for ease of programming parallel
  and distributed applications.
* It enables programmers to write fully asynchronous code using hundreds of
  millions of threads.
* HPX provides unified syntax and semantics for local and remote operations.
* HPX makes concurrency manageable with dataflow and future based
  synchronization.
* It implements a rich set of runtime services supporting a broad range of use
  cases.
* HPX exposes a uniform, flexible, and extendable performance counter framework
  which can enable runtime adaptivity
* It is designed to solve problems conventionally considered to be
  scaling-impaired.
* HPX has been designed and developed for systems of any scale, from hand-held
  devices to very large scale systems.
* It is the first fully functional implementation of the ParalleX execution
  model.
* HPX is published under a liberal open-source license and has an open, active,
  and thriving developer community.
* HPX is a member project of the `High-Performance Software Foundation <https://hpsf.io/>`_.

Governance
==========

The HPX project is a meritocratic, consensus-based community project. Anyone
with an interest in the project can join the community, contribute to the
project design and participate in the decision making process.
`This document <http://hpx.stellar-group.org/documents/governance/>`_ describes
how that participation takes place and how to set about earning merit within
the project community.


Documentation
=============

If you plan to use HPX we suggest to start with the latest released version
which can be downloaded `here <https://github.com/TheHPXProject/hpx/releases/>`_.

To quickly get started with HPX on most Linux distributions you can read the
quick start guide `here
<https://docs.hpx.dev/latest/html/quickstart.html>`_.
Detailed instructions on building and installing HPX on various platforms can be
found `here
<https://docs.hpx.dev/latest/html/manual/building_hpx.html>`_.
The full documentation for the latest release of HPX can always be found `here
<https://docs.hpx.dev/latest/html/index.html>`_.

If you would like to work with the cutting edge version of this repository
(``master`` branch) the documentation can be found `here
<https://docs.hpx.dev/branches/master/html/index.html>`_.
We strongly recommend that you follow the current health status of the master
branch by looking at our `continuous integration results website
<https://cdash.rostam.cct.lsu.edu/index.php?project=HPX>`_. While we try to keep the
master branch stable and usable, sometimes new bugs trick their way into the
code base. The Github continuous integration service additionally tracks the
current build status for the master branch: |github_status|.

We use `Codacy <https://www.codacy.com/>`_ to assess the code quality of this
project: |codacy_quality|.

If you can't find what you are looking for in the documentation or you suspect
you've found a bug in HPX we very much encourage and appreciate any issue
reports through the `issue tracker for this Github project
<https://github.com/TheHPXProject/hpx/issues>`_.

If you have any questions feel free to ask it over at `StackOverflow
<https://stackoverflow.com>`_ and tag the question with `hpx
<https://stackoverflow.com/questions/tagged/hpx>`_.

For a full list of support options please see our `Support page
<https://github.com/TheHPXProject/hpx/blob/master/.github/SUPPORT.md>`_.

If you would like to read more about HPX, we recommend the `book <https://link.springer.com/book/10.1007/978-3-031-54369-2>`_ 
"Parallel C++ -- Efficient and Scalable High-Performance Parallel Programming Using HPX" the code examples are available `here <https://github.com/ModernCPPBook/Examples>`_.


Code of conduct
===============

We have adopted a `code of conduct
<https://github.com/TheHPXProject/hpx/blob/master/.github/CODE_OF_CONDUCT.md>`_
for this project. Please refer to this document if you would like to know more
about the expectations for members of our community, with regard to how they
will behave toward each other.

Please find the project's gpg key, which is used to sign HPX releases
`here
<https://pgp.mit.edu/pks/lookup?op=get&search=0xE18AE35E86BB194F>`_.

Citing
======

In publications, please use our paper in The Journal of Open Source
Software as the main citation for HPX: |JOSS|. For referring
to the latest release of HPX please use: |zenodo_doi|.

Acknowledgements
================

We have used a free open-source license of `PVS-Studio, a static analyzer for C++ code
<https://pvs-studio.com/en/pvs-studio/?utm_source=github&utm_medium=organic&utm_campaign=open_source>`_
to check our code base.

Past and current funding and support for HPX is listed `here
<https://hpx.stellar-group.org/funding-acknowledgements>`_.

Copyright © 2026 HPX Project a Series of LF Projects, LLC

.. |github_status| image:: https://github.com/TheHPXProject/hpx/actions/workflows/build-and-test.yml/badge.svg
     :target: https://github.com/TheHPXProject/hpx/actions/workflows/build-and-test.yml
     :alt: HPX master branch build status

.. |zenodo_doi| image:: https://zenodo.org/badge/DOI/10.5281/zenodo.598202.svg
     :target: https://doi.org/10.5281/zenodo.598202
     :alt: Latest software release of HPX

.. |codacy_quality| image:: https://app.codacy.com/project/badge/Grade/aa011ca3557846a5aa96f4cac9322374
     :target: https://app.codacy.com/gh/TheHPXProject/hpx/dashboard
     :alt: HPX Code Quality Assessment

.. |JOSS| image:: https://joss.theoj.org/papers/022e5917b95517dff20cd3742ab95eca/status.svg
    :target: https://joss.theoj.org/papers/022e5917b95517dff20cd3742ab95eca
    :alt: JOSS Paper about HPX

.. |CFF| image:: https://github.com/TheHPXProject/hpx/actions/workflows/cff-validator.yml/badge.svg
    :target: https://github.com/TheHPXProject/hpx/actions/workflows/cff-validator.yml
    :alt: Citations file for HPX

.. |OpenSSF| image:: https://bestpractices.coreinfrastructure.org/projects/6093/badge
    :target: https://bestpractices.coreinfrastructure.org/projects/6093
    :alt: OpenSSF Best Practices

.. |hpx_logo| image:: http://hpx.stellar-group.org/files/hpx_logo.png
    :target: http://hpx.stellar-group.org/
    :alt: HPX Logo

.. |deepwiki| image:: https://deepwiki.com/badge.svg
    :target: https://deepwiki.com/TheHPXProject/hpx
    :alt: Ask DeepWiki
