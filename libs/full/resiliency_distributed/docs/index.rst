..
    Copyright (c) 2020 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_resiliency_distributed:

======================
resiliency_distributed
======================

Software resiliency features of |HPX| were introduced in the
:ref:`resiliency module <modules_resiliency_api>`. This module extends the APIs
to run on distributed-memory systems allowing the user to invoke the failing
task on other localities at runtime. This is useful in cases where a node is
identified to fail more often (e.g., for certain ALU computes) as the task can
now be replayed or replicated among different localities. The API exposed
allows for an easy integration with the local only resiliency APIs as well.

Distributed software resilience APIs have a similar function signature
and lives under the same namespace of ``hpx::resiliency::experimental``.
The difference arises in the formal parameters where distributed APIs takes
the localities as the first argument, and an action as opposed to a function or
a function object. The localities signify the order in which the API will either
schedule (in case of Task Replay) tasks in a round robin fashion or replicate
the tasks onto the list of localities.

The list of APIs exposed by distributed resiliency modules is the same as those
defined in :ref:`local resiliency module <modules_resiliency_api>`.

See the :ref:`API reference <modules_resiliency_distributed_api>` of this module
for more details.
