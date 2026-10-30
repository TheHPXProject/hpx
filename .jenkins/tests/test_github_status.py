#!/usr/bin/env python3
# Copyright (c) 2026 Anshuman Agrawal
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""Run with python3; entry tests require Bash >= 4 (or HPX_TEST_BASH).

All scripts run in private temporary directories with dummy credentials. Curl
uses only a loopback server; Slurm and sleep are stubbed. Script bodies are
unchanged, but their login-shell shebangs are disabled to preserve the test PATH.
"""

import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import sys
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, HTTPServer

ROOT = Path(__file__).resolve().parents[2]
HELPER = Path('.jenkins/common/set_github_status.sh')
ENTRY = Path('.jenkins/lsu/entry.sh')
BASH = os.environ.get('HPX_TEST_BASH',
                      os.environ.get('BASH', shutil.which('bash')))
CURL = shutil.which('curl')
TOKEN = 'DUMMY_STATUS_TEST_TOKEN'

STUB = r'''import json, os, subprocess, sys
from pathlib import Path
name = Path(sys.argv[0]).name
args = sys.argv[1:]
if name == 'curl':
    Path('curl-args.json').write_text(json.dumps(args))
    if 'TEST_HTTP_URL' in os.environ:
        args[args.index('--url') + 1] = os.environ['TEST_HTTP_URL']
        sys.exit(subprocess.run([os.environ['TEST_CURL']] + args).returncode)
    print('201', end='')
    sys.exit(int(os.environ.get('TEST_CURL_EXIT', '0')))
if name == 'sbatch':
    prefix = 'jenkins-hpx-' + os.environ['configuration_name_with_build_type']
    for suffix, text in [('.out', ''), ('.err', ''),
                         ('-cdash-submission.txt', 'submitted'),
                         ('-cdash-build-id.txt', '123'),
                         ('-ctest-status.txt', os.environ['TEST_BUILD_EXIT'])]:
        Path(prefix + suffix).write_text(text)
    print('12345')
    sys.exit(0)
if name in ('sleep', 'scancel', 'squeue'):
    sys.exit(0)
raise SystemExit('unexpected command: ' + name)
'''


class GithubStatusTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='hpx-github-status-')
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        scripts = [ENTRY] + [path.relative_to(ROOT) for path in
                             (ROOT / '.jenkins/common').glob('*.sh')]
        for relative in scripts:
            target = self.root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text((ROOT / relative).read_text().replace(
                '#!/bin/bash -l', '#!/bin/bash', 1))
            target.chmod(0o700)
        config = self.root / '.jenkins/lsu/slurm-configuration-fixture.sh'
        config.write_text('configuration_slurm_num_nodes=1\n'
                          'configuration_slurm_partition=fixture\n')
        bindir = self.root / 'bin'
        bindir.mkdir()
        timeout = shutil.which('timeout') or shutil.which('gtimeout')
        if timeout:
            (bindir / 'timeout').symlink_to(timeout)
        for command in ('curl', 'sbatch', 'scancel', 'squeue', 'sleep'):
            stub = bindir / command
            stub.write_text('#!' + sys.executable + '\n' + STUB)
            stub.chmod(0o700)
        self.env = {
            'PATH': str(bindir) + ':/usr/bin:/bin',
            'HOME': str(self.root),
            'TEST_CURL': CURL,
            'GITHUB_TOKEN': TOKEN,
            'GIT_COMMIT': 'master-sha',
            'GIT_BRANCH': 'origin/master',
            'configuration_name': 'fixture',
            'build_type': 'Release',
            'TEST_BUILD_EXIT': '0',
        }

    def run_script(self, script, *args):
        result = subprocess.run(
            [BASH, '-x', str(script)] + list(args), cwd=self.root,
            env=self.env, capture_output=True, text=True, timeout=15)
        self.assertNotIn(TOKEN, result.stdout + result.stderr)
        return result

    def run_helper(self):
        return self.run_script(HELPER, TOKEN, 'TheHPXProject/hpx',
                               'master-sha', 'success', 'fixture-release',
                               '123', 'jenkins/lsu')

    def serve(self, statuses):
        requests = []

        class Handler(BaseHTTPRequestHandler):
            def do_POST(self):
                body = self.rfile.read(int(self.headers['Content-Length']))
                requests.append((self.path, json.loads(body)))
                status = statuses[min(len(requests) - 1, len(statuses) - 1)]
                self.send_response(status)
                if 300 <= status < 400:
                    self.send_header('Location', '/redirect-target')
                self.end_headers()

            def log_message(self, *args):
                pass

        server = HTTPServer(('127.0.0.1', 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()

        def stop():
            server.shutdown()
            server.server_close()
            thread.join()

        self.addCleanup(stop)
        self.env['TEST_HTTP_URL'] = (
            'http://127.0.0.1:' + str(server.server_port) + '/status')
        return requests

    def curl_args(self):
        return json.loads((self.root / 'curl-args.json').read_text())

    def test_success_and_request(self):
        requests = self.serve([201])
        result = self.run_helper()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(requests, [('/status', {
            'state': 'success',
            'target_url': 'https://cdash.rostam.cct.lsu.edu/build/123',
            'description': 'Jenkins',
            'context': 'jenkins/lsu/fixture-release',
        })])
        args = self.curl_args()
        self.assertEqual(args[0], '--disable')
        for option, value in (('--connect-timeout', '10'),
                              ('--max-time', '30'), ('--retry', '2'),
                              ('--retry-max-time', '60')):
            self.assertEqual(args[args.index(option) + 1], value)

    def test_redirect_is_rejected_without_following(self):
        requests = self.serve([307])
        result = self.run_helper()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('HTTP 307', result.stderr)
        self.assertEqual(len(requests), 1)

    def test_http_client_error_is_not_retried(self):
        requests = self.serve([401])
        self.assertEqual(self.run_helper().returncode, 22)
        self.assertEqual(len(requests), 1)

    def test_http_server_error_has_bounded_retries(self):
        requests = self.serve([503])
        self.assertEqual(self.run_helper().returncode, 22)
        self.assertEqual(len(requests), 3)

    def test_transient_server_error_recovers(self):
        requests = self.serve([503, 201])
        self.assertEqual(self.run_helper().returncode, 0)
        self.assertEqual(len(requests), 2)

    def test_connection_refusal_propagates(self):
        # Close a reserved loopback port to obtain a refused connection.
        with socket.socket() as sock:
            sock.bind(('127.0.0.1', 0))
            self.env['TEST_HTTP_URL'] = (
                'http://127.0.0.1:' + str(sock.getsockname()[1]) + '/status')
        self.assertEqual(self.run_helper().returncode, 7)

    def test_entry_status_and_build_failures(self):
        version = subprocess.check_output(
            [BASH, '-c', 'echo "${BASH_VERSINFO[0]}"'], text=True)
        if int(version) < 4:
            self.skipTest('entry.sh requires Bash >= 4; set HPX_TEST_BASH')
        for pull_request in (False, True):
            if pull_request:
                self.env.update(
                    ghprbPullId='123', ghprbActualCommit='pr-sha',
                    ghprbPullLink=(
                        'https://github.com/TheHPXProject/hpx/pull/123'))
            for build_exit in (0, 42):
                for curl_exit in (0, 7, 22):
                    with self.subTest(pr=pull_request, build=build_exit,
                                      curl=curl_exit):
                        self.env['TEST_BUILD_EXIT'] = str(build_exit)
                        self.env['TEST_CURL_EXIT'] = str(curl_exit)
                        result = self.run_script(ENTRY)
                        self.assertEqual(result.returncode,
                                         build_exit or curl_exit,
                                         result.stderr)
                        args = self.curl_args()
                        sha = 'pr-sha' if pull_request else 'master-sha'
                        self.assertEqual(args[args.index('--url') + 1],
                                         'https://api.github.com/repos/'
                                         'TheHPXProject/hpx/statuses/' + sha)
                        data = json.loads(args[args.index('--data') + 1])
                        self.assertEqual(data['state'],
                                         'failure' if build_exit else 'success')


if __name__ == '__main__':
    unittest.main(verbosity=2)
