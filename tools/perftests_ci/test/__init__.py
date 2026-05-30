# -*- coding: utf-8 -*-
'''
Copyright (c) 2020 ETH Zurich

SPDX-License-Identifier: BSL-1.0
Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

test - A package to compile and run test and examples
'''

import os

from pyutils import env, log, runtools
import buildinfo

def _ctest(only=None, exclude=None, verbose=False):
    command = ['ctest', '--output-on-failure']
    if only:
        command += ['-L', only]
    if exclude:
        command += ['-LE', exclude]
    if verbose:
        command.append('-VV')
    return command


def run(run_mpi_tests, verbose_ctest):
    runtools.srun(_ctest(exclude='mpi', verbose=verbose_ctest),
                  log_output=log.info,
                  cwd=buildinfo.binary_dir)
    if run_mpi_tests:
        runtools.salloc(_ctest(only='mpi', verbose=verbose_ctest),
                        log_output=log.info,
                        cwd=buildinfo.binary_dir,
                        use_mpi_config=True)


def run_perftests():
    runtools.srun([os.path.join('tests', 'performance')],
                  log_output=log.info,
                  cwd=buildinfo.binary_dir)


def compile_and_run_examples(build_dir, verbose_ctest):
    import build

    source_dir = os.path.join(buildinfo.install_dir, 'examples')
    build_dir = os.path.abspath(build_dir)
    os.makedirs(build_dir, exist_ok=True)

    env.set_cmake_arg('CMAKE_BUILD_TYPE', buildinfo.build_type.title())

    log.info('Configuring examples')
    build.cmake(source_dir, build_dir)
    log.info('Building examples')
    build.make(build_dir)
    log.info('Successfully built examples')
    runtools.srun(_ctest(verbose=verbose_ctest),
                  log_output=log.info,
                  cwd=build_dir)
    log.info('Successfully executed examples')
