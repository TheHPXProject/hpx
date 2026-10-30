# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# The medusa partition has all medusa nodes, jenkins-compute only medusa02 to
# medusa10, which the build matrix keeps busy
configuration_slurm_partition="medusa"
# The nodes the performance tests of pull requests may run on. entry.sh waits
# until one of them is idle, the ones with reference results first, and runs
# there. The build matrix and the coverage lane exclude the first one through
# configuration_slurm_exclude in their Slurm configuration files. Change those
# too if the performance tests move to another node. Each node needs its own
# reference results, see tools/perftests_ci/pack_references.sh.
configuration_slurm_nodes="medusa08 medusa00 medusa01 medusa02 medusa03
    medusa04 medusa05 medusa06 medusa07 medusa09 medusa10 medusa11 medusa12
    medusa13 medusa14 medusa15"
# The runs of master commits (the performance history) are compared with each
# other, so they all run on this node, waiting for it if it is busy. It is not
# in jenkins-compute, so the build matrix never runs on it.
configuration_slurm_history_node="medusa00"
