# Copyright (c) 2021 ETH Zurich
# Copyright (c) 2024 The STE||AR Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

configure_extra_options+=" -DCMAKE_BUILD_TYPE=${build_type}"
configure_extra_options+=" -DHPX_WITH_CHECK_MODULE_DEPENDENCIES=ON"

# The stale allocator cache reference of #6540 only showed up in release
# builds, so verify the cache owner in every configuration here.
configure_extra_options+=" -DHPX_ALLOCATOR_SUPPORT_WITH_CACHE_OWNER_VERIFICATION=ON"
if [ "${build_type}" = "Debug" ]; then
    configure_extra_options+=" -DHPX_WITH_PARCELPORT_COUNTERS=ON"
    configure_extra_options+=" -DLCI_DEBUG=ON"
    configure_extra_options+=" -DHPX_WITH_VERIFY_LOCKS=ON"
#    configure_extra_options+=" -DHPX_WITH_VERIFY_LOCKS_BACKTRACE=ON"
fi

# These tests only make sense if hpx is being installed
configure_extra_options+=" -DHPX_WITH_TESTS_EXTERNAL_BUILD=OFF"

# Slurm on rostam has MpiDefault=none, so a bare srun (as issued by hpxrun.py)
# does not provide a PMIx bootstrap and every rank starts as its own
# single-rank MPI job. Distributed tests then hang in finalization waiting for
# peers that never show up. Select PMIx explicitly for all srun invocations.
export SLURM_MPI_TYPE=pmix

ctest_extra_args+=" --verbose "

hostname
module avail
