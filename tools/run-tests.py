#!/usr/bin/env python

# Copyright 2018-present Samsung Electronics Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import print_function

import os
import traceback
import sys
import time
import re, fnmatch

from argparse import ArgumentParser
from difflib import unified_diff
from glob import glob
from os.path import abspath, basename, dirname, join, relpath
from shutil import copy
from subprocess import PIPE, Popen


PROJECT_SOURCE_DIR = dirname(dirname(abspath(__file__)))
DEFAULT_WALRUS = join(PROJECT_SOURCE_DIR, 'walrus')


COLOR_RED = '\033[31m'
COLOR_GREEN = '\033[32m'
COLOR_YELLOW = '\033[33m'
COLOR_BLUE = '\033[34m'
COLOR_PURPLE = '\033[35m'
COLOR_RESET = '\033[0m'


RUNNERS = {}
DEFAULT_RUNNERS = []
jit = False


class runner(object):

    def __init__(self, suite, default=False):
        self.suite = suite
        self.default = default

    def __call__(self, fn):
        RUNNERS[self.suite] = fn
        if self.default:
            DEFAULT_RUNNERS.append(self.suite)
        return fn


def run(args, cwd=None, env=None, stdout=None, checkresult=True, report=False):
    if cwd:
        print(COLOR_BLUE + 'cd ' + cwd + ' && \\' + COLOR_RESET)
    if env:
        for var, val in sorted(env.items()):
            print(COLOR_BLUE + var + '=' + val + ' \\' + COLOR_RESET)
    print(COLOR_BLUE + ' '.join(args) + COLOR_RESET)

    if env is not None:
        full_env = dict(os.environ)
        full_env.update(env)
        env = full_env

    proc = Popen(args, cwd=cwd, env=env, stdout=PIPE if report else stdout)

    counter = 0

    while report:
        nextline = proc.stdout.readline()
        if nextline == '' and proc.poll() is not None:
            break
        stdout.write(nextline)
        stdout.flush()
        if counter % 250 == 0:
            print('Ran %d tests..' % (counter))
        counter += 1

    out, _ = proc.communicate()

    if out:
        print(out)

    if checkresult and proc.returncode:
        raise Exception('command `%s` exited with %s' % (' '.join(args), proc.returncode))
    return out


def readfile(filename):
    with open(filename, 'r') as f:
        return f.readlines()

def _run_wast_tests(engine, files, is_fail):
    fails = 0
    for file in files:
        proc = Popen([engine, file], stdout=PIPE) if not jit else Popen([engine, '--jit', file], stdout=PIPE)
        out, _ = proc.communicate()

        if is_fail and proc.returncode or not is_fail and not proc.returncode:
            print('%sOK: %s%s' % (COLOR_GREEN, file, COLOR_RESET))
        else:
            print('%sFAIL(%d): %s%s' % (COLOR_RED, proc.returncode, file, COLOR_RESET))
            print(out)

            fails += 1

    return fails


@runner('basic-tests', default=True)
def run_basic_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'basic')

    print('Running basic tests:')
    xpass = glob(join(TEST_DIR, '*'))
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("basic tests failed")

@runner('wasm-test-core', default=True)
def run_basic_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'wasm-spec', 'core')

    print('Running wasm-test-core tests:')
    xpass = glob(join(TEST_DIR, '*.wast'))
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("basic wasm-test-core failed")


@runner('jit', default=True)
def run_basic_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'jit')

    print('Running jit tests:')
    xpass = glob(join(TEST_DIR, '*.wast'))
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("basic wasm-test-core failed")

def main():
    parser = ArgumentParser(description='Walrus Test Suite Runner')
    parser.add_argument('--engine', metavar='PATH', default=DEFAULT_WALRUS,
                        help='path to the engine to be tested (default: %(default)s)')
    parser.add_argument('suite', metavar='SUITE', nargs='*', default=sorted(DEFAULT_RUNNERS),
                        help='test suite to run (%s; default: %s)' % (', '.join(sorted(RUNNERS.keys())), ' '.join(sorted(DEFAULT_RUNNERS))))
    parser.add_argument('--jit', action='store_true', help='test with JIT')
    args = parser.parse_args()
    global jit
    jit = args.jit

    for suite in args.suite:
        if suite not in RUNNERS:
            parser.error('invalid test suite: %s' % suite)

    success, fail = [], []

    for suite in args.suite:
        print(COLOR_PURPLE + 'running test suite: ' + suite + COLOR_RESET)
        try:
            RUNNERS[suite](args.engine)
            success += [suite]
        except Exception as e:
            print('\n'.join(COLOR_YELLOW + line + COLOR_RESET for line in traceback.format_exc().splitlines()))
            fail += [suite]

    if success:
        print(COLOR_GREEN + sys.argv[0] + ': success: ' + ', '.join(success) + COLOR_RESET)
    sys.exit(COLOR_RED + sys.argv[0] + ': fail: ' + ', '.join(fail) + COLOR_RESET if fail else None)


if __name__ == '__main__':
    main()
