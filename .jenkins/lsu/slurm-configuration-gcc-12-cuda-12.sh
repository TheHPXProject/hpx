# Copyright (c) 2020 ETH Zurich
# Copyright (c) 2024 Alireza Kheirkhahan
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

configuration_slurm_partition="jenkins-cuda"
configuration_slurm_num_nodes="1"

# bahram was taken out of CI testing as a broken node in 2023
configuration_slurm_exclude="bahram"
