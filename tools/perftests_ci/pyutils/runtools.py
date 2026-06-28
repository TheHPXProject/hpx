# -*- coding: utf-8 -*-
'''
Copyright (c) 2020 ETH Zurich

SPDX-License-Identifier: BSL-1.0
Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''

import asyncio
import functools
import io
import time

from pyutils import env, log


async def _run_async(command, log_output, **kwargs):
    process = await asyncio.create_subprocess_exec(
        *command,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
        env=env.env,
        **kwargs)

    async def read_output(stream):
        buffer = io.StringIO()
        async for line in stream:
            line = line.decode()
            buffer.write(line)
            log_output(command[0], line.strip('\n'))
        buffer.seek(0)
        return buffer.read()

    returncode, stdout, stderr = await asyncio.gather(
        process.wait(), read_output(process.stdout),
        read_output(process.stderr))

    if returncode != 0:
        commstr = ' '.join(f'"{c}"' for c in command)
        log.error(
            f'{commstr} finished with exit code {returncode} and message',
            stderr)
        raise RuntimeError(f'{commstr} failed with message "{stderr}"')

    return stdout


def run(command, log_output=None, **kwargs):
    if not command:
        raise ValueError('No command provided')
    if log_output is None:
        log_output = log.debug

    log.debug('Invoking', ' '.join(f'"{c}"' for c in command))
    start = time.time()

    try:
        loop = asyncio.get_event_loop()
    except RuntimeError:
        loop = asyncio.new_event_loop()
    output = loop.run_until_complete(_run_async(command, log_output, **kwargs))

    end = time.time()
    log.debug(f'{command[0]} finished in {end - start:.2f}s')
    return output


@functools.lru_cache()
def _slurm_available():
    try:
        run(['srun', '--version'])
        log.info('Using SLURM')
        return True
    except FileNotFoundError:
        log.info('SLURM not found: invoking commands directly')
        return False


def srun(command, use_mpi_config=False, **kwargs):
    if _slurm_available():
        command = ['srun'] + env.sbatch_options(use_mpi_config) + command

    return run(command, **kwargs)


def salloc(command, use_mpi_config=False, **kwargs):
    if _slurm_available():
        command = ['salloc'] + env.sbatch_options(use_mpi_config) + command

    return run(command, **kwargs)
