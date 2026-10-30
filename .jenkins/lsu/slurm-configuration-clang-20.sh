# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2025 Alireza Kheirkhahan
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

configuration_slurm_partition="jenkins-compute"
configuration_slurm_num_nodes="1"

# medusa08 runs the performance tests, keep the build matrix off it
configuration_slurm_exclude="medusa08"
