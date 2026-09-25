# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2022 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

configuration_slurm_partition="jenkins-compute"
# The build matrix and the coverage lane exclude this node through
# configuration_slurm_exclude in their Slurm configuration files. Change
# those too if the performance tests move to another node.
configuration_slurm_nodelist="medusa08"
