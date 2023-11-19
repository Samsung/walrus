#!/usr/bin/env python3

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

from os.path import abspath, dirname, join
from markdownTable import markdownTable  # pip install py-markdown-table

TEST_DIR = join(dirname(abspath(__file__)), 'ctests')

expectedValues = {
    "change": 4,
    "factorial": 30,
    "fannkuch": 120,
    "fibonacci": 1,
    "gregory": 3.141592640,
    "hanoi": 0,
    "heapsort": 0,
    "huffman": 0,
    "k_nucleotide": 1,
    "mandelbrotFloat": 775007,
    "mandelbrotDouble": 775007,
    "matrix_multiply": 3920.0,
    "nbody": -0.1691057,
    "nqueens": 0,
    "prime": 48611,
    "quick_sort": 0,
    "red-black": 4000000,
    "rsa": 0,
    "salesman": 840,
    "simdMandelbrotFloat": 775007,
    "simdMandelbrotDouble": 775007,
    "simdNbody": -0.1691057,
    "simd_matrix_multiply": 3920.0,
    "ticTacToe": 4748900
}
# https://benchmarksgame-team.pages.debian.net/benchmarksgame/description/simple.html#simple
gameTests = ["mandelbrotFloat", "nbody", "gregory", "fannkuch", "k_nucleotide"]

simdTests = ["simdMandelbrotFloat", "simdMandelbrotDouble", "simdNbody", "simd_matrix_multiply"]

def prepare_arg_pars():
    parser = argparse.ArgumentParser()
    parser.add_argument('--test-dir', metavar='PATH', help='path to the test files written in c', nargs='?',
                        const=TEST_DIR, default=TEST_DIR)
    parser.add_argument('--only-game', help='only run The Benchmarks Game tests', action='store_true')
    parser.add_argument('--run', metavar='TEST', help='only run one benchmark')
    parser.add_argument('--engines', metavar='PATH', help='paths to wasm engines', nargs='+', default=['walrus'])
    parser.add_argument('--report', metavar='PATH', help='path to the report', nargs='?', const='./report.md')
    parser.add_argument('--iterations', metavar='NUMBER', help='how many times run the tests', nargs='?',
                        const='10', default=10, type=int)
    parser.add_argument('--compile-anyway', help='compile the tests even if they are compiled', action='store_true')
    parser.add_argument('--enable-simd', help='run SIMD tests too', action='store_true')
    parser.add_argument('--jit', help='use JIT version of Walrus', action='store_true')
    return parser.parse_args()


def check_programs(engines):
    if os.system("git --version >/dev/null") != 0:
        print("git not found")
        exit(1)

    for e in engines:
        path = e.split(" ")[0]
        if os.path.isfile(path) is False:
            print(path + " not found")
            exit(1)

    print("Checks done")


def get_emcc():
    emcc_path = None 
    if os.getenv('EMSDK'):
        emcc_path = join(os.getenv('EMSDK'), 'upstream/emscripten/emcc.py')
        if os.path.exists(emcc_path):
            print("EMCC already installed: " + emcc_path)
            return emcc_path

    if os.path.exists("./emsdk/.git"):
        os.system("(cd ./emsdk && git fetch -a) >/dev/null")
        os.system("(cd ./emsdk && git reset --hard origin/HEAD) >/dev/null")
        os.system("./emsdk/emsdk install latest >/dev/null")
        os.system("./emsdk/emsdk activate latest >/dev/null")
    else:
        os.system("git clone --depth 1 https://github.com/emscripten-core/emsdk.git ./emsdk >/dev/null")
        os.system("./emsdk/emsdk install latest >/dev/null")
        os.system("./emsdk/emsdk activate latest >/dev/null")

    emcc_path = "./emsdk/upstream/emscripten/emcc"
    print("EMCC install done: " + emcc_path)
    return emcc_path


def compile_tests(emcc_path, path, only_game=False, compile_anyway=False, simd=False, run=None):
    if not os.path.exists(emcc_path):
        print("invalid path for emcc: " + emcc_path)
        exit(1)

    if not os.path.exists(path):
        print("invalid path for tests: " + path)
        exit(1)

    if not os.path.exists(path + "/wasm"):
        os.makedirs(path + "/wasm")

    path = os.path.abspath(path)
    test_list = [file for file in os.listdir(path) if file.endswith('.c')]
    test_list.sort()
    test_names = []

    for file in test_list:
        name = file.split('.')[0]

        if (name not in gameTests and only_game) or \
           (run is not None and name != run) or     \
           (name in simdTests and not simd):
            continue

        test_names.append(name)

        if not compile_anyway and os.path.exists(path + "/wasm/" + name + ".wasm"):
            print("target files are found; compilation skipped")
            continue

        extraFlags = "-msimd128" if file.startswith("simd") else ""

        print("compiling " + name)
        bob_the_stringbuilder = emcc_path + " " + path + "/" + file + " " + extraFlags + " --no-entry -s WASM=1 -s EXPORTED_FUNCTIONS=_runtime -s EXPORTED_RUNTIME_METHODS=ccall,cwrap -o " + path + "/wasm/" + name + ".wasm"
        print(bob_the_stringbuilder)
        os.system(bob_the_stringbuilder)

    return test_names

def run_wasm(engine, path, name, jit=False):
    if not os.path.exists(path):
        print("invalid path for run")
        exit(1)

    tc_path = path + "/wasm/" + name + ".wasm"
    print("TC path: " + tc_path)
    flags = "--run-export runtime" + (" --jit" if jit else "")

    result = subprocess.check_output(engine + " " + flags + " " + tc_path, shell=True)

    if float(f'{float(result):.9f}') != float(expectedValues[name]):
        print("run failed with " + name + ".wasm", file=sys.stderr)
        print("Expected: " + str(expectedValues[name]), file=sys.stderr)
        print("Got: " + str(result), file=sys.stderr)
        return False

    return True


def measure_time(path, name, function, engine=None, jit=False):
    start_time = time.perf_counter_ns()

    if engine is None:
        ret_val = function(None, path, name, jit)
    else:
        ret_val = function(engine, path, name, jit)

    end_time = time.perf_counter_ns()

    if ret_val:
        return end_time - start_time
    else:
        return math.nan


def run_tests(path, test_names, engines, number_of_runs, jit=False):
    ret_val = []
    for name in test_names:
        print("running " + name)
        measurements_engines = {}
        for e in engines:
            measurements_engines[e] = []

        for i in range(0, number_of_runs):
            print("round " + str(i + 1))
            for e in engines:
                measurements_engines[e].append(measure_time(path, name, run_wasm, e, jit))

        result_list = {"test": name}

        compare_result = False
        results = {}
        for e in engines:
            results[e] = sum(measurements_engines[e])/len(measurements_engines[e])
            if compare_result is False:
                compare_result = results[e]

        for e in engines:
            result_list[e + " [s]"] = "{:.3f}".format(results[e] / 1000000000) + " ({:.3f}x)".format((results[e] / compare_result))

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
    print(TEST_DIR)

    if args.engines is None:
        print("You need to specify the engine locations")
        exit(1)

    check_programs(args.engines)
    emcc_path = get_emcc()
    test_names = compile_tests(emcc_path, args.test_dir, args.only_game, args.compile_anyway, args.enable_simd, args.run)
    generate_report(
        run_tests(args.test_dir, test_names, args.engines, args.iterations, args.jit),
        args.report)


if __name__ == '__main__':
    main()
