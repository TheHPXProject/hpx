# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Run batch.sh with fixture results and installations in a private directory."""

import json
import os
import shutil
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).resolve().parents[1] / 'lsu' / 'batch.sh'
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

if command == 'ctest':
    option = '-DCTEST_BINARY_DIRECTORY='
    build = Path(next(a[len(option):] for a in args if a.startswith(option)))
    results = build / 'Testing' / 'fixture-tag'
    results.mkdir(parents=True)
    missing = os.environ.get('MISSING_RESULT')
    if missing != 'TAG':
        (results.parent / 'TAG').write_text('fixture-tag\n')
    for name in ('Configure', 'Build', 'Test'):
        if missing == name:
            continue
        if name == 'Test':
            status = os.environ.get('TEST_STATUS', 'passed')
            body = '<Test Status="' + status + '"><Name>fixture</Name></Test>'
        else:
            body = '<Error>fixture</Error>' if os.environ.get(name) else ''
        (results / (name + '.xml')).write_text('<Site>' + body + '</Site>')
    sys.exit(int(os.environ.get('CTEST_EXIT', 0)))

if command == 'cmake':
    step = 'INSTALL' if '--build' in args else 'CONFIGURE'
    code = int(os.environ.get(step + '_EXIT', 0))
    if step == 'CONFIGURE':
        prefix = args[-1].split('=', 1)[1]
        (root / 'prefix').write_text(prefix)
    elif not code:
        prefix = Path((root / 'prefix').read_text())
        prefix.mkdir()
        (prefix / 'installed').write_text('fixture')
    sys.exit(code)

if command in ('ln', 'mv'):
    code = int(os.environ.get(command.upper() + '_EXIT', 0))
    if code:
        sys.exit(code)
    source, destination = map(Path, args[-2:])
    assert source.parent == root / 'install'
    assert destination.parent == root / 'install'
    if command == 'mv':
        assert '-Tf' in args
    os.execv(os.environ['REAL_' + command.upper()], [command] + args)

if command == 'module':
    sys.exit(int(os.environ.get('MODULE_EXIT', 0)))
if command in ('ls', 'head'):
    os.execv(os.environ['REAL_' + command.upper()], [command] + args)
raise RuntimeError('Unexpected command: ' + command)
'''


class BuildResultsTest(unittest.TestCase):
    def run_batch(self, expected, *, existing=False, collision=False, **updates):
        with tempfile.TemporaryDirectory(prefix='hpx-batch-test-') as directory:
            root = Path(directory).resolve()
            config = root / '.jenkins' / 'lsu'
            config.mkdir(parents=True)
            (config / 'env-common.sh').write_text(
                'ctest_extra_args=""\nconfigure_extra_options=""\n')
            (config / 'env-fixture.sh').write_text(':\n')
            script = root / 'batch.sh'
            # The only body substitution redirects installation into the fixture.
            script.write_text(SCRIPT.read_text().replace(
                '/work/jenkins/install', str(root / 'install')))
            bindir = root / 'bin'
            bindir.mkdir()
            stub = bindir / 'stub'
            stub.write_text('#!' + sys.executable + '\n' + STUB)
            stub.chmod(0o700)
            for name in ('ctest', 'cmake', 'ln', 'mv', 'module', 'ls', 'head'):
                (bindir / name).symlink_to(stub)
            bash_env = root / 'bash-env'
            bash_env.write_text('ulimit() { return 0; }\n')
            install = root / 'install'
            install.mkdir()
            previous = install / 'hpx-fixture-release-sha-previous-job'
            previous.mkdir()
            (previous / 'installed').write_text('previous')
            published = install / 'hpx-fixture-release'
            if existing:
                published.symlink_to(previous)
            if collision:
                (install / 'hpx-fixture-release-sha-123').mkdir()
            env = {
                'PATH': str(bindir) + ':/usr/bin:/bin',
                'BASH_ENV': str(bash_env), 'FIXTURE_ROOT': str(root),
                'configuration_name': 'fixture',
                'configuration_name_with_build_type': 'fixture-release',
                'GIT_COMMIT': 'sha', 'SLURM_JOB_ID': '123', 'install_hpx': '1',
            }
            for command in ('ln', 'mv', 'ls', 'head'):
                executable = (shutil.which('g' + command) or
                              shutil.which(command))
                env['REAL_' + command.upper()] = executable
            env.update({key: str(value) for key, value in updates.items()})
            result = subprocess.run(
                ['/bin/bash', str(script)], cwd=root, env=env,
                capture_output=True, text=True, timeout=10)
            self.assertEqual(result.returncode, expected, result.stderr)
            status = root / 'jenkins-hpx-fixture-release-ctest-status.txt'
            self.assertEqual(status.read_text().strip(), str(expected))
            calls = [json.loads(line) for line in
                     (root / 'calls.jsonl').read_text().splitlines()]
            commands = [name for name, args in calls]
            if expected or updates.get('install_hpx', '1') != '1':
                self.assertEqual(published.is_symlink(), existing)
                if existing:
                    self.assertEqual(published.resolve(), previous)
                if expected and not updates.get('MV_EXIT'):
                    self.assertNotIn('mv', commands)
            else:
                self.assertEqual(published.resolve().parent, install)
                self.assertNotEqual(published.resolve(), previous)
                self.assertEqual((published / 'installed').read_text(), 'fixture')
                self.assertLess(commands.index('cmake'), commands.index('ln'))
                self.assertLess(commands.index('module'), commands.index('ln'))
            self.assertEqual((previous / 'installed').read_text(), 'previous')
            self.assertEqual(list(install.glob('*.tmp-*')), [])
            return commands

    def test_install_disabled(self):
        for value in ('0', '', 'yes'):
            with self.subTest(value=value):
                commands = self.run_batch(0, install_hpx=value)
                self.assertNotIn('cmake', commands)
                self.assertNotIn('ln', commands)

    def test_install_success_and_same_commit_retry(self):
        for existing in (False, True):
            with self.subTest(existing=existing):
                self.run_batch(0, existing=existing)

    def test_ctest_exit_preserved(self):
        for code, status in ((42, 'passed'), (8, 'notrun'), (255, 'failed')):
            with self.subTest(code=code):
                commands = self.run_batch(
                    code, CTEST_EXIT=code, TEST_STATUS=status, existing=True)
                self.assertNotIn('cmake', commands)

    def test_xml_failure_without_ctest_exit(self):
        failures = ({'TEST_STATUS': 'failed'}, {'TEST_STATUS': 'notrun'},
                    {'Configure': 'error'}, {'Build': 'error'})
        for failure in failures:
            with self.subTest(failure=failure):
                commands = self.run_batch(1, existing=True, **failure)
                self.assertNotIn('cmake', commands)

    def test_missing_results(self):
        for name in ('TAG', 'Configure', 'Build', 'Test'):
            with self.subTest(name=name):
                commands = self.run_batch(1, MISSING_RESULT=name, existing=True)
                self.assertNotIn('cmake', commands)

    def test_install_step_failure(self):
        for step in ('CONFIGURE', 'INSTALL', 'MODULE', 'LN', 'MV'):
            for existing in (False, True):
                with self.subTest(step=step, existing=existing):
                    commands = self.run_batch(
                        13, existing=existing, **{step + '_EXIT': 13})
                    if step == 'CONFIGURE':
                        self.assertEqual(commands.count('cmake'), 1)
                    if step in ('CONFIGURE', 'INSTALL', 'MODULE'):
                        self.assertNotIn('ln', commands)

    def test_existing_install_collision(self):
        commands = self.run_batch(1, collision=True, existing=True)
        self.assertNotIn('cmake', commands)


if __name__ == '__main__':
    unittest.main()
