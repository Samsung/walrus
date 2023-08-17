#!/bin/python3

# Copyright 2023-present Samsung Electronics Co., Ltd.
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

import argparse
import math
import os
import subprocess
import sys
import time

from markdownTable import markdownTable  # pip install py-markdown-table

expectedValues = {
    "change": 4,
    "factorial": 30,
    "fannkuch": 120,
    "fibonacci": 1,
    "gregory": 3.141592640,
    "hanoi": 0,
    "heapsort": 0,
    "k_nucleotide": 1,
    "mandelbrot": 2091942736,
    "nbody": -0.169083713,
    "nqueens": 0,
    "prime": 48611,
    "quick_sort": 0,
    "red-black": 4000000,
    "salesman": 840
}
# https://benchmarksgame-team.pages.debian.net/benchmarksgame/description/simple.html#simple
gameTests = ["mandelbrot", "nbody", "gregory", "fannkuch", "k_nucleotide"]


def prepare_arg_pars():
    parser = argparse.ArgumentParser()
    parser.add_argument('--test-dir', metavar='PATH', help='path to the test files written in c', nargs='?',
                        const='./ctests', default='./ctests')
    parser.add_argument('--only-game', help='only run The Benchmarks Game tests', action='store_true')
    parser.add_argument('--run', metavar='TEST', help='only run one benchmark')
    parser.add_argument('--walrus', metavar='PATH', help='path to the engine', nargs='+', default=['walrus'])
    parser.add_argument('--report', metavar='PATH', help='path to the report', nargs='?', const='./report.md')
    parser.add_argument('--iterations', metavar='NUMBER', help='how many times run the tests', nargs='?',
                        const='10', default=10, type=int)
    parser.add_argument('--compile-anyway', help='compile the tests even if they are compiled', action='store_true')
    return parser.parse_args()


def check_programs(walrus):
    if os.system("git --version >/dev/null") != 0:
        print("git not found")
        exit(1)

    for w in walrus:
        path = w.split(" ")[0]
        if os.path.isfile(path) is False:
            print(path + " not found")
            exit(1)

    print("Checks done")


def get_emcc():
    if os.path.exists("./emsdk/.git"):
        os.system("(cd ./emsdk && git fetch -a) >/dev/null")
        os.system("(cd ./emsdk && git reset --hard origin/HEAD) >/dev/null")
        os.system("./emsdk/emsdk install latest >/dev/null")
        os.system("./emsdk/emsdk activate latest >/dev/null")
    else:
        os.system("git clone --depth 1 https://github.com/emscripten-core/emsdk.git ./emsdk >/dev/null")
        os.system("./emsdk/emsdk install latest >/dev/null")
        os.system("./emsdk/emsdk activate latest >/dev/null")

    print("EMCC done")


def compile_tests(path, only_game=False, compile_anyway=False, run=None):
    if not os.path.exists(path):
        print("invalid path for tests")
        exit(1)

    if not os.path.exists(path + "/wasm"):
        os.makedirs(path + "/wasm")

    path = os.path.abspath(path)
    test_list = [file for file in os.listdir(path) if file.endswith('.c')]
    test_list.sort()
    test_names = []

    for file in test_list:
        name = file.split('.')[0]

        if (name not in gameTests and only_game) or (run is not None and name != run):
            continue

        test_names.append(name)

        if not compile_anyway and os.path.exists(path + "/wasm/" + name + ".wasm"):
            print("target files are found; compilation skipped")
            continue

        print("compiling " + name)
        bob_the_stringbuilder = "./emsdk/upstream/emscripten/emcc " + path + "/" + file + " --no-entry -s WASM=1 -s EXPORTED_FUNCTIONS=_runtime -s EXPORTED_RUNTIME_METHODS=ccall,cwrap -o " + path + "/wasm/" + name + ".wasm"
        os.system(bob_the_stringbuilder)

    return test_names

def run_walrus(engine, path, name):
    if not os.path.exists(path):
        print("invalid path for walrus run")
        exit(1)

    result = subprocess.check_output(engine + " --run-export runtime " + path + "/wasm/" + name + ".wasm", shell=True)

    if float(f'{float(result):.9f}') != float(expectedValues[name]):
        print("walrus failed with " + name + ".wasm", file=sys.stderr)
        print("Expected: " + str(expectedValues[name]), file=sys.stderr)
        print("Got: " + str(result), file=sys.stderr)
        return False

    return True


def measure_time(path, name, functon, engine=None):
    start_time = time.perf_counter_ns()

    if engine is None:
        ret_val = functon(path, name)
    else:
        ret_val = functon(engine, path, name)

    end_time = time.perf_counter_ns()

    if ret_val:
        return end_time - start_time
    else:
        return math.nan


def run_tests(path, test_names, walrus, number_of_runs):
    ret_val = []
    for name in test_names:
        print("running " + name)
        measurements_walrus = {}
        for w in walrus:
            measurements_walrus[w] = []

        for i in range(0, number_of_runs):
            print("round " + str(i + 1))
            for w in walrus:
                measurements_walrus[w].append(measure_time(path, name, run_walrus, w))

        result_list = {"test": name}

        min_walrus_first = False
        min_walrus = {}
        for w in walrus:
            min_walrus[w] = min(measurements_walrus[w])
            if min_walrus_first is False:
                min_walrus_first = min_walrus[w]

        for w in walrus:
            result_list[w + " [s]"] = "{:.3f}".format(min_walrus[w] / 1000000000) + " ({:.3f}x)".format((min_walrus[w] / min_walrus_first))

        ret_val.append(result_list)

    return ret_val


def generate_report(data, file=None):
    if file is None:
        file = sys.stdout
    else:
        file = open(file, "w")

    print(markdownTable(data).setParams(row_sep='markdown', quote=False).getMarkdown(), file=file)


def main():
    args = prepare_arg_pars()

    if args.walrus is None:
        print("You need to specify the engine location")
        exit(1)

    check_programs(args.walrus)
    get_emcc()
    test_names = compile_tests(args.test_dir, args.only_game, args.compile_anyway, args.run)
    generate_report(
        run_tests(args.test_dir, test_names, args.walrus, args.iterations),
        args.report)


if __name__ == '__main__':
    main()
