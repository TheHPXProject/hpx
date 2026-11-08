<!-- Copyright (c) 2026 Hartmut Kaiser                                            -->
<!--                                                                              -->
<!-- SPDX-License-Identifier: BSL-1.0                                             -->
<!-- Distributed under the Boost Software License, Version 1.0. (See accompanying -->
<!-- file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)        -->

# Backfilling the performance history

The HPX-performance-history job adds a run to the performance history
([TheHPXProject/hpx-performance](https://github.com/TheHPXProject/hpx-performance))
for every master merge (`upload_history.sh`). When it didn't run for some
merges, the scripts here add their runs by hand, on a node held with Slurm,
the way the job would have: the same lane steps (`batch.sh`, without the CDash
submits), at a path of the same length as the Jenkins workspace, and the same
upload, into a local clone that is pushed when the runs are ready.

| script               | does                                                                                          |
|----------------------|-----------------------------------------------------------------------------------------------|
| `missing_commits.sh` | lists the master commits after a commit that aren't in the history yet                        |
| `backfill.sh`        | runs the benchmarks of each commit in `commits.txt` on the held node and commits the run locally |
| `after.sh`           | waits for a process to exit (and for a held job to run), then starts `backfill.sh`            |
| `push.sh`            | pushes the runs committed locally to the history                                              |
| `skip_commits.txt`   | the master commits whose runs are left out on purpose (`backfill_history.sh` skips them)      |

A run takes about 17 minutes on medusa00 (build and benchmarks).

## In the Jenkins job

Every master build of the HPX-performance-history job adds the missing runs
itself (`../backfill_history.sh`, called by `entry.sh` after the build has added
its own run and set its status):

- A commit is missing if it is a first parent of master, has no run in
  `runs.tsv`, isn't in `skip_commits.txt` and was merged more than an hour ago
  (its build may not be in Slurm yet).
- A commit gets one run. All runs share the history node, so the allocation
  serializes them; right before it starts the benchmarks of a commit,
  `backfill.sh` looks at the history on GitHub again and skips the commit if
  another build has added its run meanwhile, and `upload_history.sh` doesn't
  add a master build's run if a backfill added that commit's run while the
  build waited for the node. Nothing is done if another build is already
  adding the missing runs (a lock in the work directory).
- If anything is missing up to the built commit, the build asks for the
  history node, for half an hour per commit and half an hour more for the
  most commits a build adds (`PERFTEST_BACKFILL_MAX`, 6), and waits for it as
  for its own run. The request may wait in the queue for hours, so once it is
  granted, the build finds out again what is missing, now up to the tip of
  master (fetched from `PERFTEST_HPX_REPO`), and releases the node at once if
  nothing is. Otherwise it runs `backfill.sh` in that allocation and pushes
  the runs with the build's token; a push rejected because another build
  pushed meanwhile is tried again on top of it, as in step 4 below.
- The work directory (`PERFTEST_BACKFILL_DIR`) and the tree the commits are
  built in sit next to the workspace and are kept between builds. The tree's
  path has as many characters as the workspace's (its name ends in `-bf`
  instead of the last three characters), so this build's own reports stay
  untouched for archiving.
- The build line of such a run's README says which build added it
  (`PERFTEST_BACKFILL_BY`). A failure is reported in the log, it doesn't change
  the status of the build.

The steps below are for adding runs by hand.

## What is different from a Jenkins run

- The directory of a run ends in `-b0` (there is no Jenkins build number), and
  the build line of its README says that it ran outside Jenkins.
- The benchmarks, `batch.sh` and `ctest.cmake` are those of each commit, but
  the reports and the graphs are drawn by `perftests_plot.py`,
  `perftests_timeline.py` and `upload_history.sh` of the HPX tree the scripts
  are run from, so that all added runs are drawn the same way. Run the
  scripts from a tree at (or after) the newest commit to be added.
- Nothing is submitted to CDash, and nothing is pushed until `push.sh` runs.

## Setup

On rostam's login node (the upload needs the network, nothing is built there),
in an up to date HPX clone:

```bash
git -C <hpx> fetch origin
export PERFTEST_BACKFILL_DIR=$HOME/perfhist          # the work directory
export PERFTEST_BACKFILL_SRC=/work/$USER/workspace/HPX-performance-history
```

`PERFTEST_BACKFILL_SRC` is where the commits are checked out and built. Its
path must have as many characters as the Jenkins workspace,
`/work/jenkins/workspace/HPX-performance-history` (47): the length of the
source and build paths ends up in the binaries (`__FILE__` strings) and moves
the code, so that runs built at a path of another length aren't comparable
with those of Jenkins. `backfill.sh` stops if the length differs (pad the
directory name if your user name is longer or shorter than `jenkins`), and
adds the tree as a worktree of the HPX clone if it doesn't exist.

Other settings, all optional:

| variable                     | default                                              |
|------------------------------|------------------------------------------------------|
| `PERFTEST_BACKFILL_CPUS`     | 40, the cores of the lane steps                      |
| `PERFTEST_HISTORY_REPO`      | `https://github.com/TheHPXProject/hpx-performance.git`, cloned into `$PERFTEST_BACKFILL_DIR/upload/data` on the first run |
| `PERFTEST_BACKFILL_PUSH_URL` | `git@github.com:TheHPXProject/hpx-performance.git`   |
| `PERFTEST_PYTHON_ENV`        | the Python environment of the performance tests, as in `upload_history.sh` |

## 1. List the commits

```bash
mkdir -p $PERFTEST_BACKFILL_DIR
<hpx>/.jenkins/lsu-perftests/backfill/missing_commits.sh [<since>] \
    > $PERFTEST_BACKFILL_DIR/commits.txt
```

`commits.txt` holds full commit hashes, one per line, oldest first. Without
`<since>`, `missing_commits.sh` starts after the earliest commit of master
that has a run in `runs.tsv` and lists every commit after it without a run,
which needs the clone (after the first run of `backfill.sh`). A `<since>`
must be a commit before the gaps to fill: `runs.tsv` (in hpx-performance)
lists the runs in the order they were added, not by commit, so its last
commit can have older gaps. Check the list: the first parents of master are
its merges, plus any commits pushed to master directly.

To add more commits later, append them to `commits.txt`: commits already in
the history are skipped.

## 2. Hold the node

```bash
salloc --no-shell -p medusa -w medusa00 --exclusive -N1 -n1 -c 40 -t 7:00:00 -J hold
squeue -u $USER
```

Hold it exclusively, so nothing else runs while the benchmarks do; seven
hours fit about 20 commits. Ask before holding a node that others use.

## 3. Run

```bash
cd $PERFTEST_BACKFILL_DIR
setsid nohup <hpx>/.jenkins/lsu-perftests/backfill/backfill.sh <job id> \
    >> backfill.log 2>&1 < /dev/null &
tail -f backfill.log
```

Each commit logs its checkout, the lane steps, the srun's exit code and the
CTest status, and `added, <commit> Add the performance test results of ...`
once it is committed in `upload/data`. The output of every run (the lane's
output, the reports and the upload log) is kept in `runs/<n>-<commit>/`. A
commit whose run wasn't recorded is not added and the next one starts; look
at its `runs/` directory.

`backfill.sh` stops before a run when less than 30 minutes are left in the
job. To continue in another held job without watching, queue the next hold
and start `after.sh` with the PID of `backfill.sh`:

```bash
setsid nohup <hpx>/.jenkins/lsu-perftests/backfill/after.sh <pid> <next job id> \
    >> backfill.log 2>&1 < /dev/null &
```

`after.sh` also serves to start a backfill when something else that uses the
node is done (give that process's PID).

## 4. Push

```bash
<hpx>/.jenkins/lsu-perftests/backfill/push.sh
```

It lists the local runs that aren't in the history yet and pushes all of them,
never forced; push after each run or at the end. If the push is rejected
because the history moved on (a Jenkins run was added meanwhile), move the
local clone aside, which makes the next `backfill.sh` clone it again, and run
`backfill.sh` again with the same `commits.txt`: it adds the runs whose
reports are kept in `runs/` again from them, without running their benchmarks,
and runs only the commits that have none. To run the benchmarks of a commit
again, remove its `runs/<n>-<commit>/` first.

```bash
mv $PERFTEST_BACKFILL_DIR/upload/data $PERFTEST_BACKFILL_DIR/upload/data.rejected
```

When all runs are in, cancel the hold (`scancel <job id>`).
