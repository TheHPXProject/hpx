..
    Copyright (C) 2018 Mikael Simberg

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _documentation:

=============
Documentation
=============

This documentation is built using |sphinx|_, and an automatically generated API
reference using |doxygen|_ and |breathe|_.

We always welcome suggestions on how to improve our documentation, as well as
pull requests with corrections and additions.

Prerequisites
=============

To build the |hpx| documentation, you need recent versions of the following
packages:

- ``python3`` (3.9 or later)
- ``sphinx`` 7.2 or later (Python package)
- ``sphinx-book-theme`` (Python package)
- ``breathe`` 4.36.0 or later (Python package)
- ``doxygen``
- ``sphinxcontrib-bibtex``
- ``sphinx-copybutton``

If the |python|_ dependencies are not available through your system package
manager, you can install them using the Python package manager ``pip``.
We recommend using a virtual environment:

.. code-block:: bash

   mkdir -p build
   python3 -m venv build/.venv
   source build/.venv/bin/activate
   pip install \
      "sphinx>=7.2" "breathe>=4.36.0" \
      sphinx-book-theme sphinxcontrib-bibtex \
      sphinx-copybutton

You may need to set the following CMake variables to make sure CMake can
find the required dependencies.

.. option:: Doxygen_ROOT:PATH

   Specifies where to look for the installation of the |doxygen|_ tool.

.. option:: Sphinx_ROOT:PATH

   Specifies where to look for the installation of the |sphinx|_ tool.

.. option:: Breathe_APIDOC_ROOT:PATH

   Specifies where to look for the installation of the |breathe|_ tool.


Building documentation
======================

Enable building of the documentation by setting ``HPX_WITH_DOCUMENTATION=ON``
during |cmake|_ configuration. To build the documentation, build the ``docs``
target using your build tool. The default output format is HTML documentation.
You can choose alternative output formats (single-page HTML, PDF, and man) with
the ``HPX_WITH_DOCUMENTATION_OUTPUT_FORMATS`` CMake option.

.. note::

   If you add new source files to the Sphinx documentation, you have to run
   CMake again to have the files included in the build.


Style guide
===========

The documentation is written using reStructuredText. These are the conventions
used for formatting the documentation:

* Use, at most, 80 characters per line.
* Top-level headings use over- and underlines with ``=``.
* Sub-headings use only underlines with characters in decreasing level of
  importance: ``=``, ``-`` and ``.``.
* Use sentence case in headings.
* Refer to common terminology using ``:term:`Component```.
* Indent content of directives (``.. directive::``) by three spaces.
* For C++ code samples at the end of paragraphs, use ``::`` and indent the code
  sample by 4 spaces.

  * For other languages (or if you don't want a colon at the end of the
    paragraph), use ``.. code-block:: language`` and indent by three spaces as
    with other directives.
* Use ``.. list-table::`` to wrap tables with a lot of text in cells.

API documentation
=================

The source code is documented using Doxygen. If you add new API documentation
either to existing or new source files, make sure that you add the documented
source files to the ``doxygen_dependencies`` variable in
``docs/CMakeLists.txt``.

Adding new API documentation
============================

When you add new public APIs to |hpx|, follow these guidelines to ensure the
documentation is correctly generated and easy to use.

Documenting headers
-------------------

At the top of a new header file, include the standard Doxygen documentation block.
For example, if the new feature is called `my_new_feature`, you should:

* List the name of the new file under ``\file``.
* Specify the page name with ``\page`` (e.g., ``hpx::my_new_feature``).
* Reference the existing header that will include your new file under ``\headerfile``.

.. code-block:: cpp

   /// \file my_new_feature.hpp
   /// \page hpx::my_new_feature
   /// \headerfile hpx/existing_header_of_new_feature.hpp

Adding to the public API reference
----------------------------------

For the documentation to appear in the generated API reference, add your new API to
``docs/sphinx/api/public_api.rst`` under the appropriate table (usually "Functions") for
the relevant header (for example, ``hpx/existing_header_of_new_feature.hpp``).

Including examples
------------------

Do **not** inline examples in the API reference. If you want to provide an example
of how to use the new API, there are two recommended options:

1. **Documentation page option**
   Edit an existing relevant manual page to explain the new functionality and include a code example.
   Use this option if you want to provide additional context about how the API works.

2. **Standalone examples option**
   Create a separate example under ``docs/sphinx/examples/``. Examples in this directory are
   typically more comprehensive and executable, making them suitable for richer or more
   complex scenarios.
