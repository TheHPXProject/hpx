..
    Copyright (c) 2019 The STE||AR-Group

    SPDX-License-Identifier: BSL-1.0
    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

.. _modules_resiliency:

==========
resiliency
==========

In |hpx|, a program failure is a manifestation of a failing task. This module
exposes several APIs that allow users to manage failing tasks in a convenient way by
either replaying a failed task or by replicating a specific task.

Task replay is analogous to the Checkpoint/Restart mechanism found in conventional
execution models. The key difference being localized fault detection. When the
runtime detects an error, it replays the failing task as opposed to completely
rolling back the entire program to the previous checkpoint.

Task replication is designed to provide reliability enhancements by replicating
a set of tasks and evaluating their results to determine a consensus among them.
This technique is most effective in situations where there are few tasks in the
critical path of the DAG which leaves the system underutilized or where hardware
or software failures may result in an incorrect result instead of an error.
However, the drawback of this method is the additional computational cost incurred
by repeating a task multiple times.


The following API functions are exposed:

- :hpx:func:`hpx::resiliency::experimental::async_replay`: This version of task replay will
  catch user-defined exceptions and automatically reschedule the task N times
  before throwing an :hpx:struct:`hpx::resiliency::experimental::abort_replay_exception` if no
  task is able to complete execution without an exception.

- :hpx:func:`hpx::resiliency::experimental::async_replay_validate`: This version of replay
  adds an argument to async replay which receives a user-provided validation
  function to test the result of the task against. If the task's output is
  validated, the result is returned. If the output fails the check or an
  exception is thrown, the task is replayed until no errors are encountered or
  the number of specified retries has been exceeded.

- :hpx:func:`hpx::resiliency::experimental::async_replicate`: This is the most basic
  implementation of the task replication. The API returns the first result that
  runs without detecting any errors.

- :hpx:func:`hpx::resiliency::experimental::async_replicate_validate`: This API additionally
  takes a validation function which evaluates the return values produced by the
  threads. The first task to compute a valid result is returned.

- :hpx:func:`hpx::resiliency::experimental::async_replicate_vote`: This API adds a vote
  function to the basic replicate function. Many hardware or software failures
  are silent errors which do not interrupt program flow. In order to detect
  errors of this kind, it is necessary to run the task several times and compare
  the values returned by every version of the task. In order to determine which
  return value is "correct", the API allows the user to provide a custom
  consensus function to properly form a consensus. This voting function then
  returns the "correct"" answer.

- :hpx:func:`hpx::resiliency::experimental::async_replicate_vote_validate`: This combines the
  features of the previously discussed replicate set. Replicate vote validate
  allows a user to provide a validation function to filter results.
  Additionally, as described in replicate vote, the user can provide a "voting
  function" which returns the consensus formed by the voting logic.

- :hpx:func:`hpx::resiliency::experimental::dataflow_replay`: This version of dataflow replay
  will catch user-defined exceptions and automatically reschedules the task N
  times before throwing an :hpx:struct:`hpx::resiliency::experimental::abort_replay_exception`
  if no task is able to complete execution without an exception. Any arguments
  for the executed task that are futures will cause the task invocation to be
  delayed until all of those futures have become ready.

- :hpx:func:`hpx::resiliency::experimental::dataflow_replay_validate` : This version of replay
  adds an argument to dataflow replay which receives a user-provided validation
  function to test the result of the task against. If the task's output is
  validated, the result is returned. If the output fails the check or an
  exception is thrown, the task is replayed until no errors are encountered or
  the number of specified retries have been exceeded. Any arguments for the
  executed task that are futures will cause the task invocation to be delayed
  until all of those futures have become ready.

- :hpx:func:`hpx::resiliency::experimental::dataflow_replicate`: This is the most basic
  implementation of the task replication. The API returns the first result that
  runs without detecting any errors. Any arguments for the executed task that
  are futures will cause the task invocation to be delayed until all of those
  futures have become ready.

- :hpx:func:`hpx::resiliency::experimental::dataflow_replicate_validate`: This API
  additionally takes a validation function which evaluates the return values
  produced by the threads. The first task to compute a valid result is returned.
  Any arguments for the executed task that are futures will cause the task
  invocation to be delayed until all of those futures have become ready.

- :hpx:func:`hpx::resiliency::experimental::dataflow_replicate_vote`: This API adds a vote
  function to the basic replicate function. Many hardware or software failures
  are silent errors which do not interrupt program flow. In order to detect
  errors of this kind, it is necessary to run the task several times and compare
  the values returned by every version of the task. In order to determine which
  return value is "correct", the API allows the user to provide a custom
  consensus function to properly form a consensus. This voting function then
  returns the "correct" answer. Any arguments for the executed task that are
  futures will cause the task invocation to be delayed until all of those
  futures have become ready.

- :hpx:func:`hpx::resiliency::experimental::dataflow_replicate_vote_validate`: This combines
  the features of the previously discussed replicate set. Replicate vote
  validate allows a user to provide a validation function to filter results.
  Additionally, as described in replicate vote, the user can provide a "voting
  function" which returns the consensus formed by the voting logic. Any
  arguments for the executed task that are futures will cause the task
  invocation to be delayed until all of those futures have become ready.

See the :ref:`API reference <modules_resiliency_api>` of the module for more
details.
