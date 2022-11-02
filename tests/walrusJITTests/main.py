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


def is_valid_file(parser, arg):
    if not os.path.exists(arg):
        parser.error(COLOR_RED + "The file %s does not exist!" % arg + COLOR_RESET)


def runTests(args):
    executable = args.wabt
    tests = args.testFolder
    expected = args.expectedFolder
    minimal = args.minimal
    test_files = []
    expected_files = []

    if os.path.isfile(tests):
        test_files.append(tests)
    else:
        test_files = [f for f in listdir(tests) if isfile(join(tests, f))]

    if os.path.isfile(expected):
        expected_files.append(expected)
    else:
        expected_files = [f for f in listdir(expected) if isfile(join(expected, f))]

    for i in range(0, len(test_files)):
        if os.path.isabs(test_files[i]) is False:
            test_files[i] = os.path.abspath(tests + "/" + test_files[i])

    for i in range(0, len(expected_files)):
        if os.path.isabs(expected_files[i]) is False:
            expected_files[i] = os.path.abspath(expected + "/" + expected_files[i])

    failing_tests = []

    for file in test_files:
        print(file)
        output = subprocess.run([executable, '--jit', '--verbose', file], universal_newlines=True,
                                capture_output=True)

        if minimal is False or output.returncode != 0:
            print(output.stderr)
            print(output.stdout)
        for expectedFile in expected_files:
            if os.path.basename(expectedFile).split('.')[0] == os.path.basename(file).split('.')[0]:
                content = Path(expectedFile).read_text()
                has_error = False

                for line in difflib.unified_diff(output.stderr.strip().splitlines(), content.strip().splitlines(),
                                                 fromfile='output', tofile='expected', lineterm=''):
                    print(line)
                    has_error = True

                if has_error:
                    failing_tests.append(file)

                break

    if len(failing_tests) != 0:
        print(COLOR_RED + "\nFailing tests:")
        for test in failing_tests:
            print(test)
        print(COLOR_RESET)
    else:
        print(COLOR_GREEN + "Every test ran successfully" + COLOR_RESET)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('--wabt', dest="wabt", required=False, metavar="FILE",
                        default="../../third_party/wabt/cmake-build-debug/wasm-interp", help="WABT executable")
    parser.add_argument('--test', dest="testFolder", required=False, type=str, default="./tests", metavar="FILE",
                        help="Test(s) to run")
    parser.add_argument('--expected', dest="expectedFolder", required=False, type=str, default="./expected",
                        metavar="FILE",
                        help="Output folder of expected test results")
    parser.add_argument('--not-minimal', dest="minimal", required=False, action='store_false',
                        help="Show result of each tests or not")
    parser.set_defaults(minimal=True)

    args = parser.parse_args()

    is_valid_file(parser, args.wabt)
    is_valid_file(parser, args.testFolder)
    is_valid_file(parser, args.expectedFolder)
    runTests(args)


if __name__ == '__main__':
    main()
