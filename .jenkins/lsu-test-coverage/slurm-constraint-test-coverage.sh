# Copyright (c) 2023 Panos Syskakis
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

configuration_slurm_partition="jenkins-compute"

# medusa08 runs the performance tests, keep the coverage build off it
configuration_slurm_exclude="medusa08"
