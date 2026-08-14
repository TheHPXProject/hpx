# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2024 Alireza Kheirkhahan
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

module purge
module load cmake
module load gcc/14
module load boost/1.85.0-${build_type,,}
module load hwloc
module load openmpi
module load likwid/5.4.1

export HPXRUN_RUNWRAPPER=srun
export CXX_STD="23"
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:$PKG_CONFIG_PATH

configure_extra_options+=" -DCMAKE_BUILD_TYPE=${build_type}"
configure_extra_options+=" -DHPX_WITH_CXX_STANDARD=${CXX_STD}"
configure_extra_options+=" -DHPX_WITH_MALLOC=system"
configure_extra_options+=" -DHPX_WITH_FETCH_ASIO=ON"
configure_extra_options+=" -DHPX_WITH_COMPILER_WARNINGS=ON"
configure_extra_options+=" -DHPX_WITH_COMPILER_WARNINGS_AS_ERRORS=ON"
configure_extra_options+=" -DHPX_WITH_PARCELPORT_MPI=ON"
configure_extra_options+=" -DHPX_WITH_PARCELPORT_LCI=ON"
configure_extra_options+=" -DHPX_WITH_FETCH_LCI=ON"
configure_extra_options+=" -DHPX_WITH_LCI_BOOTSTRAP_MPI=ON"
configure_extra_options+=" -DHPX_WITH_PARCELPORT_LCW=ON"
configure_extra_options+=" -DHPX_WITH_FETCH_LCW=ON"
configure_extra_options+=" -DCMAKE_C_COMPILER=gcc"
configure_extra_options+=" -DCMAKE_C_FLAGS=-fPIC"
configure_extra_options+=" -DHPX_WITH_DATAPAR_BACKEND=STD_EXPERIMENTAL_SIMD"
configure_extra_options+=" -DHPX_LIKWID_WITH_LIKWID=ON"

# The pwrapi library still needs to be set up properly on rostam
# configure_extra_options+=" -DHPX_WITH_POWER_COUNTER=ON"

configure_extra_options+=" -DHPX_COROUTINES_WITH_THREAD_SCHEDULE_HINT_RUNS_AS_CHILD=ON"

# also enable additional handshaking in MPI parcelport
configure_extra_options+=" -DHPX_WITH_TESTS_COMMAND_LINE=--hpx:ini=hpx.parcel.mpi.ack_handshake!=1"
