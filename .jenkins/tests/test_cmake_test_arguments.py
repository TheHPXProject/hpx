# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Check argument forwarding in generated CTest commands without building HPX."""

import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


HELPER = Path(__file__).resolve().parents[2] / 'cmake' / 'HPX_AddTest.cmake'
PROJECT = '''
cmake_minimum_required(VERSION 3.19)
project(TestArgumentForwarding NONE)
enable_testing()
function(hpx_debug)
endfunction()
set(HPX_WITH_NETWORKING OFF)
set(HPX_WITH_PARALLEL_TESTS_BIND_NONE OFF)
set(HPX_WITH_TESTS_MAX_THREADS_PER_LOCALITY 0)
include("${HPX_TEST_HELPER}")
add_hpx_test(tests explicit_args EXECUTABLE "${Python_EXECUTABLE}"
  ARGS --size=128 "value with spaces")
add_hpx_test(tests legacy_args EXECUTABLE "${Python_EXECUTABLE}"
  --size=128 "value with spaces")
add_hpx_test(tests mixed_args EXECUTABLE "${Python_EXECUTABLE}"
  --before ARGS --after "value with spaces")
'''


@unittest.skipUnless(shutil.which('cmake') and shutil.which('ctest'),
                     'CMake and CTest are required')
class CMakeTestArgumentsTest(unittest.TestCase):
    def test_generated_commands_preserve_arguments(self):
        with tempfile.TemporaryDirectory(prefix='hpx-test-arguments-') as name:
            source = Path(name)
            build = source / 'build'
            (source / 'CMakeLists.txt').write_text(PROJECT)
            subprocess.run(
                ['cmake', '-S', str(source), '-B', str(build),
                 '-DHPX_TEST_HELPER=' + str(HELPER),
                 '-DPython_EXECUTABLE=' + sys.executable],
                check=True, capture_output=True, text=True, timeout=30)
            metadata = json.loads(subprocess.check_output(
                ['ctest', '--test-dir', str(build), '--show-only=json-v1'],
                text=True, timeout=30))
            commands = {test['name']: test['command']
                        for test in metadata['tests']}
            expected = {
                'explicit_args': ['--size=128', 'value with spaces'],
                'legacy_args': ['--size=128', 'value with spaces'],
                'mixed_args': ['--before', '--after', 'value with spaces'],
            }
            for test, arguments in expected.items():
                with self.subTest(test=test):
                    command = commands['tests.' + test]
                    self.assertEqual(command[command.index('--') + 1:], arguments)


if __name__ == '__main__':
    unittest.main()
