#!/usr/bin/env python3

# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Verify the production dashboard preserves results before interruption."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / 'lsu' / 'ctest.cmake'
CMAKE = shutil.which('cmake')
FIXTURE = r'''
function(interrupt_at phase)
  if(STOP_PHASE STREQUAL phase)
    message(FATAL_ERROR "fixture interruption during ${phase}")
  endif()
endfunction()
macro(ctest_start)
endmacro()
macro(ctest_update)
endmacro()
function(ctest_submit)
  cmake_parse_arguments(SUBMIT "" "BUILD_ID;RETURN_VALUE" "PARTS" ${ARGN})
  set(${SUBMIT_BUILD_ID} 123 PARENT_SCOPE)
  set(${SUBMIT_RETURN_VALUE} 0 PARENT_SCOPE)
endfunction()
macro(ctest_configure)
  interrupt_at(Configure)
endmacro()
macro(ctest_build)
  interrupt_at(Build)
endmacro()
macro(ctest_test)
  interrupt_at(Test)
endmacro()
set(CTEST_BUILD_CONFIGURATION_NAME fixture-release)
include("${DASHBOARD_SCRIPT}")
'''


@unittest.skipUnless(CMAKE, 'CMake is required')
class SubmissionResultsTest(unittest.TestCase):
    def test_completed_submissions_survive_a_later_interruption(self):
        for phase, completed in (
            ('Configure', ('Update',)),
            ('Build', ('Update', 'Configure')),
            ('Test', ('Update', 'Configure', 'Build')),
            ('', ('Update', 'Configure', 'Build', 'Tests')),
        ):
            with self.subTest(phase=phase):
                with tempfile.TemporaryDirectory(prefix='hpx-submit-test-') as tmp:
                    root = Path(tmp)
                    wrapper = root / 'fixture.cmake'
                    wrapper.write_text(FIXTURE)
                    env = dict(os.environ, git_local_branch='fixture')
                    env.pop('ghprbPullId', None)
                    result = subprocess.run(
                        [CMAKE, '-DSTOP_PHASE=' + phase,
                         '-DDASHBOARD_SCRIPT=' + str(SCRIPT), '-P', str(wrapper)],
                        cwd=root, env=env, text=True, capture_output=True,
                        timeout=15)
                    if phase:
                        self.assertNotEqual(result.returncode, 0)
                        self.assertIn('fixture interruption during ' + phase,
                                      result.stderr)
                    else:
                        self.assertEqual(result.returncode, 0, result.stderr)
                    prefix = 'jenkins-hpx-fixture-release-cdash-'
                    self.assertEqual((root / (prefix + 'build-id.txt')).read_text(),
                                     '123')
                    self.assertEqual(
                        (root / (prefix + 'submission.txt')).read_text(),
                        'CTest submission results:\n' +
                        ''.join(name + ': 0\n' for name in completed))


if __name__ == '__main__':
    unittest.main()
