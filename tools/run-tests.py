#!/usr/bin/env python3

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
import json
import time
import re
import fnmatch

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
JIT_EXCLUDE_FILES = []
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

def _run_wast_tests(engine, files, is_fail):
    fails = 0
    for file in files:
        if jit:
            filename = os.path.basename(file) 
            if filename in JIT_EXCLUDE_FILES:
                continue

        proc = Popen([engine, "--mapdirs", "./test/wasi", "/var", file], stdout=PIPE) if not jit else Popen([engine, "--mapdirs", "./test/wasi", "/var", "--jit", file], stdout=PIPE)
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
    xpass = glob(join(TEST_DIR, '*.wast'))
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total - fail_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("basic tests failed")


@runner('wasm-test-core', default=True)
def run_core_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'wasm-spec', 'core')

    print('Running wasm-test-core tests:')
    xpass = glob(join(TEST_DIR, '**/*.wast'), recursive=True)
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total - fail_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("wasm-test-core failed")


@runner('wasi', default=True)
def run_wasi_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'wasi')

    print('Running wasi tests:')
    xpass = glob(join(TEST_DIR, '*.wast'))
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total - fail_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("basic wasi tests failed")


@runner('jit', default=True)
def run_jit_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'jit')

    print('Running jit tests:')
    xpass = glob(join(TEST_DIR, '*.wast'))
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total - fail_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("basic wasm-test-core failed")

@runner('wasm-test-extended', default=True)
def run_extended_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'extended')

    print('Running wasm-extended tests:')
    xpass = glob(join(TEST_DIR, '**/*.wast'), recursive=True)
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total - fail_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("wasm-test-extended failed")

@runner('regression', default=False)
def run_extended_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'other')


    other_test_run_config = json.load(open(join(TEST_DIR, 'regression/running_config.json')))

    test_list_pass = []
    test_list_fail = []

    for test_case in other_test_run_config['test cases']:
        if test_case['deprecated']:
            continue

        test_paths = sum(
            [glob(join(TEST_DIR, 'regression', f'issue-{id}', '*.wasm'), recursive=False) for id in test_case['ids']],
            [])

        if not test_paths:
            test_paths = sum(
                [glob(join(TEST_DIR, 'regression', f'issue-{id}', f"{test_case['file']}"), recursive=False) for id in
                 test_case['ids']],
                [])

        if ('expected return' not in test_case and test_case['compile_options']['expected return']['ret code'] == 0) \
                or test_case['expected return']['ret code'] == 0:
            test_list_pass.extend(test_paths)
        else:
            test_list_fail.extend(test_paths)

    print('Running other regression tests:')
    result = _run_wast_tests(engine, test_list_pass, False)
    result += _run_wast_tests(engine, test_list_fail, True)

    tests_total = len(test_list_pass) + len(test_list_fail)
    fail_total = result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total - fail_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("other regression failed")

@runner('unit', default=True)
def run_other_unit_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'other', 'unit')

    print('Running other-unit tests:')
    xfail = [filename for filename in glob(join(TEST_DIR, 'linear-memory-wasm/*.wast'), recursive=False) if
             ";; Should report an error." in open(filename).read()]
    xfail.extend([filename for filename in glob(join(TEST_DIR, 'memory64/*exceed*.wat'), recursive=False)])
    xpass = [filename for filename in glob(join(TEST_DIR, '**/*.wa*t'), recursive=True) if filename not in xfail]
    xpass_result = _run_wast_tests(engine, xpass, False)
    xpass_result += _run_wast_tests(engine, xfail, True)

    tests_total = len(xpass) + len(xfail)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total - fail_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("other unit failed")

@runner('standalone', default=True)
def run_extended_tests(engine):
    TEST_DIR = join(PROJECT_SOURCE_DIR, 'test', 'other', 'standalone')

    print('Running other standalone tests:')
    xpass = glob(join(TEST_DIR, '**/*.wasm'), recursive=True)
    xpass_result = _run_wast_tests(engine, xpass, False)

    tests_total = len(xpass)
    fail_total = xpass_result
    print('TOTAL: %d' % (tests_total))
    print('%sPASS : %d%s' % (COLOR_GREEN, tests_total - fail_total, COLOR_RESET))
    print('%sFAIL : %d%s' % (COLOR_RED, fail_total, COLOR_RESET))

    if fail_total > 0:
        raise Exception("other standalone tests failed")

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
    if jit:
        exclude_list_file = join(PROJECT_SOURCE_DIR, 'tools', 'jit_exclude_list.txt')
        with open(exclude_list_file) as f:
            global JIT_EXCLUDE_FILES
            JIT_EXCLUDE_FILES = f.read().replace('\n', ' ').split()


    for suite in args.suite:
        if suite not in RUNNERS:
            parser.error('invalid test suite: %s' % suite)

    success, fail = [], []

    for suite in args.suite:
        print(COLOR_PURPLE + f'running test suite{ " with jit" if jit else ""}: ' + suite + COLOR_RESET)
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
