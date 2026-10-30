..
    Copyright (c) 2020-2021 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_parcelport_mpi:

==============
parcelport_mpi
==============

This module implements a parcelport that uses |mpi|_ for the communication
between localities. If MPI is not initialized yet when the runtime starts, the
parcelport initializes it and finalizes it when the runtime stops. In this
case the application must neither initialize nor finalize MPI itself. See
:ref:`modules_mpi` for how to use MPI in an application together with |hpx|.

See the :ref:`API reference <modules_parcelport_mpi_api>` of this module for more
details.

