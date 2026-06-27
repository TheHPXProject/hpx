..
    Copyright (C) 2018 Mikael Simberg

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _using_docker:

============================
Using docker for development
============================

Although it can often be useful to set up a local development environment with
system-provided or self-built dependencies, |docker|_ provides a convenient
alternative to quickly get all the dependencies needed to start development of
|hpx|. Our testing setup on |github|_ uses a docker image to run all tests.

To get started you need to install |docker|_ using whatever means is most
convenient on your system. Once you have |docker|_ installed, you can pull or
directly run the docker image. The image is based on Debian and Clang, and can
be found on |docker_build_env|_. To start a container using the |hpx| build
environment, run:

.. code-block:: shell-session

   $ docker run --interactive --tty stellargroup/build_env:latest bash

You are now in an environment where all the |hpx| build and runtime dependencies
are present. You can install additional packages according to your own needs.
Please see the |docker_docs|_ for more information on using |docker|_.

.. warning::

   All changes made within the container are lost when the container is closed.
   If you want files to persist (e.g., the |hpx| source tree) after closing the
   container, you can bind directories from the host system into the container
   (see |docker_docs_bind_mounts|_).

