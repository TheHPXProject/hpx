# -*- coding: utf-8 -*-
'''
Copyright (c) 2020 ETH Zurich

SPDX-License-Identifier: BSL-1.0
Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''

import functools
import os
import platform
import re

from pyutils import default_vars as var, log, runtools

env = os.environ.copy()


def load(envfile):
    if not os.path.exists(envfile):
        raise FileNotFoundError(f'Could find environment file "{envfile}"')
    #env['HPX_CMAKE_PYUTILS_ENVFILE'] = os.path.abspath(envfile)

    envdir, envfile = os.path.split(envfile)
    output = runtools.run(
        ['bash', '-l', '-c', f'set -e && source {envfile} && env -0'],
        cwd=envdir).strip('\0')
    env.update(line.split('=', 1) for line in output.split('\0'))

    log.info(f'Loaded environment from {os.path.join(envdir, envfile)}')
    log.debug(f'{"New environment"}',
              '\n'.join(f'{k}={v}' for k, v in sorted(env.items())))


try:
    from pyutils import buildinfo
except ImportError:
    pass
else:
    if buildinfo.envfile is not None:
        load(buildinfo.envfile)


def _items_with_tag(tag):
    return {k[len(tag):]: v for k, v in env.items() if k.startswith(tag)}


def cmake_args():
    args = []
    for k, v in _items_with_tag('PYUTILS_').items():
        if v.strip().upper() in ('ON', 'OFF'):
            k += ':BOOL'
        else:
            k += ':STRING'
        args.append(f'-D{k}={v}')
    for k, v in _items_with_tag('-G').items():
        args.append(f'-G{k}')
    return args


def set_cmake_arg(arg, value):
    if isinstance(value, bool):
        value = 'ON' if value else 'OFF'
    env[arg] = value


def sbatch_options(mpi):
    options = _items_with_tag('GTRUN_SBATCH_')
    if mpi:
        options.update(_items_with_tag('GTRUNMPI_SBATCH_'))

    return [
        '--' + k.lower().replace('_', '-') + ('=' + v if v else '')
        for k, v in options.items()
    ]


def build_command():
    return env.get(var._project_name+'_BUILD_COMMAND', var._default_build_system).split()


def hostname():
    """
    Host name of the current machine.

    Example:
        >>> hostname()
        'keschln-0002'
    """
    return platform.node()


@functools.lru_cache()
def clustername():
    """
    SLURM cluster name of the current machine.

    Example:
        >>> clustername()
        'kesch'
    """
    try:
        output = runtools.run(['scontrol', 'show', 'config'])
        m = re.compile(r'.*ClusterName\s*=\s*(\S*).*',
                       re.MULTILINE | re.DOTALL).match(output)
        if m:
            return m.group(1)
    except FileNotFoundError:
        return hostname()
