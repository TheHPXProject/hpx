# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Run the coverage batch.sh with fixture tools in a private directory."""

import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SCRIPT = (Path(__file__).resolve().parents[1] / 'lsu-test-coverage' /
          'batch.sh')
REPORT = '''TN:
SF:libs/core/fixture/src/fixture.cpp
FN:10,_ZN3hpx7fixtureIiE3runEv
FN:10,_ZN3hpx7fixtureIdE3runEv
FNDA:3,_ZN3hpx7fixtureIiE3runEv
FNDA:0,_ZN3hpx7fixtureIdE3runEv
FNF:2
FNH:1
DA:10,3
DA:11,0
LF:2
LH:1
end_of_record
'''
STUB = r'''
import json
import os
from pathlib import Path
import sys

command = Path(sys.argv[0]).name
args = sys.argv[1:]
root = Path(os.environ['FIXTURE_ROOT'])
with (root / 'calls.jsonl').open('a') as log:
    log.write(json.dumps([command, args]) + '\n')

if command == 'cmake':
    sys.exit(0)
if command == 'ctest':
    sys.exit(int(os.environ.get('CTEST_EXIT', 0)))
if command == 'grcov':
    assert '-o' not in args, 'The report must be streamed, not written whole'
    sys.stdout.write(os.environ['GRCOV_REPORT'])
    sys.exit(int(os.environ.get('GRCOV_EXIT', 0)))
if command == 'grep':
    # Stands in for a filter that fails part way through, for example on a
    # full disk, after writing the first record.
    lines = sys.stdin.read().splitlines(keepends=True)
    sys.stdout.write(next(l for l in lines if not l.startswith('FN')))
    sys.exit(int(os.environ['FILTER_EXIT']))
if command == 'curl':
    # Stands in for downloading the Codacy uploader. A failed download writes
    # nothing, like curl --fail.
    code = int(os.environ.get('CURL_EXIT', 0))
    if code:
        sys.exit(code)
    Path(args[args.index('-o') + 1]).write_text(
        'cat "$3" > "${FIXTURE_ROOT}/uploaded.info"\n'
        'exit ' + os.environ.get('CODACY_EXIT', '0') + '\n')
    sys.exit(0)
raise RuntimeError('Unexpected command: ' + command)
'''


class CoverageResultsTest(unittest.TestCase):
    def run_batch(self, expected, report=REPORT, **updates):
        with tempfile.TemporaryDirectory(prefix='hpx-coverage-test-') as name:
            root = Path(name).resolve()
            config = root / '.jenkins' / 'lsu-test-coverage'
            config.mkdir(parents=True)
            (config / 'env-fixture.sh').write_text(':\n')
            bindir = root / 'bin'
            bindir.mkdir()
            stub = bindir / 'stub'
            stub.write_text('#!' + sys.executable + '\n' + STUB)
            stub.chmod(0o700)
            tools = ['cmake', 'ctest', 'curl']
            if 'FILTER_EXIT' in updates:
                tools.append('grep')
            for tool in tools:
                (bindir / tool).symlink_to(stub)
            # batch.sh runs the grcov that entry.sh unpacked next to it.
            (root / 'grcov').symlink_to(stub)
            bash_env = root / 'bash-env'
            bash_env.write_text('ulimit() { return 0; }\n')
            env = {
                'PATH': str(bindir) + ':/usr/bin:/bin',
                'BASH_ENV': str(bash_env), 'FIXTURE_ROOT': str(root),
                'configuration_name': 'fixture', 'GIT_COMMIT': 'sha',
                'CODACY_TOKEN': 'fixture-token', 'GRCOV_REPORT': report,
            }
            env.update({key: str(value) for key, value in updates.items()})
            result = subprocess.run(
                ['/bin/bash', str(SCRIPT)], cwd=root, env=env,
                capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, expected, result.stderr)
            self.assertFalse((root / 'codacy-uploader.sh').exists())
            calls = [json.loads(line) for line in
                     (root / 'calls.jsonl').read_text().splitlines()]
            uploaded = root / 'uploaded.info'
            return (result, calls, (root / 'lcov.info').read_text(),
                    uploaded.read_text() if uploaded.exists() else None,
                    (root / 'jenkins-hpx-fixture-ctest-status.txt')
                    .read_text().strip())

    def test_report_keeps_line_records_only(self):
        _, calls, report, uploaded, status = self.run_batch(0)
        self.assertEqual(status, '0')
        self.assertNotRegex(report, r'(?m)^FN')
        self.assertIn('SF:libs/core/fixture/src/fixture.cpp\n', report)
        self.assertIn('DA:10,3\nDA:11,0\n', report)
        self.assertIn('end_of_record\n', report)
        self.assertEqual(uploaded, report)
        ctest = next(args for name, args in calls if name == 'ctest')
        self.assertEqual(ctest[ctest.index('--parallel') + 1], '4')
        self.assertEqual(ctest[ctest.index('--timeout') + 1], '300')

    def test_grcov_failure_fails_the_run(self):
        result, _, _, uploaded, status = self.run_batch(1, GRCOV_EXIT=3)
        self.assertIn('grcov failed', result.stdout)
        self.assertIsNone(uploaded)
        self.assertEqual(status, '0')

    def test_empty_report_fails_the_run(self):
        result, _, report, uploaded, _ = self.run_batch(
            1, report='FN:1,only\nFNDA:1,only\n')
        self.assertEqual(report, '')
        self.assertIn('no coverage data', result.stdout)
        self.assertIsNone(uploaded)

    def test_filter_failure_fails_the_run(self):
        result, _, report, uploaded, _ = self.run_batch(1, FILTER_EXIT=2)
        self.assertEqual(report, 'TN:\n')
        self.assertIn('filtering the coverage data failed', result.stdout)
        self.assertIsNone(uploaded)

    def test_uploader_download_failure_fails_the_run(self):
        result, calls, _, uploaded, _ = self.run_batch(1, CURL_EXIT=22)
        self.assertIn('downloading the Codacy uploader failed', result.stdout)
        self.assertIsNone(uploaded)
        curl = next(args for name, args in calls if name == 'curl')
        self.assertIn('--fail', curl)

    def test_upload_failure_fails_the_run(self):
        result, _, _, _, _ = self.run_batch(1, CODACY_EXIT=7)
        self.assertIn('uploading coverage to Codacy failed', result.stdout)

    def test_test_failures_take_precedence(self):
        _, _, _, uploaded, status = self.run_batch(8, CTEST_EXIT=8)
        self.assertEqual(status, '8')
        self.assertIsNotNone(uploaded)


if __name__ == '__main__':
    unittest.main()
