#!/bin/python3

import argparse
import difflib
import os
import subprocess
from os import listdir
from os.path import isfile, join
from pathlib import Path

COLOR_GREEN = '\033[32m'
COLOR_RED = '\033[31m'
COLOR_RESET = '\033[0m'


def isFileExists(parser, arg):
    if not os.path.exists(arg):
        parser.error(COLOR_RED + "The file %s does not exist!" % arg + COLOR_RESET)


def addFile(test_files, input_file):
    if input_file.endswith(".txt"):
        test_files.append(input_file)


def runTests(args):
    interp = args.interp
    wat2wasm = args.wat2wasm
    tests = args.testFolder
    update = args.update
    verbose = args.verbose
    test_files = []
    expected_files = []

    if os.path.isfile(tests):
        addFile(test_files, tests)
    else:
        for f in listdir(tests):
            if isfile(join(tests, f)):
                addFile(test_files, f)

    for i in range(0, len(test_files)):
        if not os.path.isabs(test_files[i]):
            test_files[i] = os.path.abspath(tests + "/" + test_files[i])

    failing_tests = []

    for text_file in test_files:
        print("Running: %s" % (text_file))

        wasm_file = text_file[0:-4] + ".wasm";

        output = subprocess.run([wat2wasm, '-o', wasm_file, text_file], universal_newlines=True,
                                capture_output=True)

        if output.returncode != 0:
            if verbose:
                print("Error %d when running %s\n" % (output.returncode, wat2wasm))
                print(output.stderr)
                print(output.stdout)
            failing_tests.append(text_file)
            continue

        output = subprocess.run([interp, '--jit', '--jit-verbose', wasm_file], universal_newlines=True,
                                capture_output=True)

        if output.returncode != 0:
            if verbose:
                print("Error %d when running %s\n" % (output.returncode, interp))
                print(output.stderr)
                print(output.stdout)
            failing_tests.append(text_file)
            os.remove(wasm_file)
            continue

        content = ""

        expected_file = text_file[0:-4] + ".expected"

        if update:
            try:
                f = open(expected_file, 'w')
                f.write(output.stdout)
                f.close()
            except:
                print("Cannot write expected file: %s" % expected_file)

            os.remove(wasm_file)
            continue

        try:
           f = open(expected_file, 'r')
           content = f.read()
           f.close()
        except:
            print("----------------------------")
            print("Expected file not found: %s" % expected_file)
            failing_tests.append(text_file)
            os.remove(wasm_file)
            continue

        failed = False
        for line in difflib.unified_diff(output.stdout.splitlines(), content.splitlines(),
                                         fromfile='output', tofile='expected', lineterm='\n'):
            print(line)
            if not failed:
                failed = True
                failing_tests.append(text_file)

        os.remove(wasm_file)

    if update:
        print("\nTests are updated")
        return

    if len(failing_tests) != 0:
        print(COLOR_RED + "\nFailing tests:")
        for test in failing_tests:
            print(test)
        print(COLOR_RESET)
    else:
        print(COLOR_GREEN + "Every test ran successfully" + COLOR_RESET)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('--interp', dest="interp", required=False, metavar="FILE",
                        default="../../third_party/wabt/bin/wasm-interp", help="WABT interpreter")
    parser.add_argument('--wat2wasm', dest="wat2wasm", required=False, metavar="FILE",
                        default="../../third_party/wabt/bin/wat2wasm", help="WABT wat2wasm")
    parser.add_argument('--test', dest="testFolder", required=False, type=str, default="./tests", metavar="FILE",
                        help="Test(s) to run")
    parser.add_argument('--update', dest="update", required=False, action='store_true',
                        help="Update test results")
    parser.add_argument('--verbose', dest="verbose", required=False, action='store_true',
                        help="Show result of each tests")

    args = parser.parse_args()

    isFileExists(parser, args.interp)
    isFileExists(parser, args.wat2wasm)
    isFileExists(parser, args.testFolder)
    runTests(args)


if __name__ == '__main__':
    main()
