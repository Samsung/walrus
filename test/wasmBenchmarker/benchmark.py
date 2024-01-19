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
    "gregory": 3.14159264,
    "hanoi": 0,
    "heapsort": 0,
    "huffman": 0,
    "kNucleotide": 1,
    "mandelbrotFloat": 775007,
    "mandelbrotDouble": 775007,
    "matrixMultiply": 3920.0,
    "nbody": -0.16910574,
    "nqueens": 0,
    "prime": 48611,
    "quickSort": 0,
    "redBlack": 4000000,
    "rsa": 0,
    "salesman": 840,
    "simdMandelbrotFloat": 775007,
    "simdMandelbrotDouble": 775007,
    "simdNbody": -0.16910574,
    "simdMatrixMultiply": 3920.0,
    "ticTacToe": 4748900
}
# https://benchmarksgame-team.pages.debian.net/benchmarksgame/description/simple.html#simple
gameTests = ["mandelbrotFloat", "nbody", "gregory", "fannkuch", "kNucleotide"]

simdTests = ["simdMandelbrotFloat", "simdMandelbrotDouble", "simdNbody", "simdMatrixMultiply"]

def prepare_arg_pars():
    parser = argparse.ArgumentParser()
    parser.add_argument('--test-dir', metavar='PATH', help='path to the test files written in c', nargs='?',
                        const=TEST_DIR, default=TEST_DIR)
    parser.add_argument('--only-game', help='only run Game tests', action='store_true')
    parser.add_argument('--only-simd', help='only run SIMD tests', action='store_true')
    parser.add_argument('--run', metavar='TEST', help='only run one benchmark')
    parser.add_argument('--engines', metavar='PATH', help='paths to wasm engines', nargs='+', default=['walrus'])
    parser.add_argument('--report', metavar='PATH', help='path to the report', nargs='?', const='./report.md')
    parser.add_argument('--iterations', metavar='NUMBER', help='how many times run the tests', nargs='?',
                        const='10', default=10, type=int)
    parser.add_argument('--compile-anyway', help='compile the tests even if they are compiled', action='store_true')
    parser.add_argument('--mem', help='measure MAX RSS', action='store_true')
    parser.add_argument('--jit', help='use JIT version of Walrus', action='store_true')
    parser.add_argument('--verbose', help='prints extra informations e.g. paths', action='store_true')
    return parser.parse_args()


def check_programs(engines, verbose):
    if os.system("git --version >/dev/null") != 0:
        print("git not found")
        exit(1)

    for e in engines:
        path = e.split(" ")[0]
        if os.path.isfile(path) is False:
            print(path + " not found")
            exit(1)

    if (verbose): print("Checks done")


def get_emcc(verbose):
    emcc_path = None 
    if os.getenv('EMSDK'):
        emcc_path = join(os.getenv('EMSDK'), 'upstream/emscripten/emcc.py')
        if os.path.exists(emcc_path):
            if (verbose): print("EMCC already installed: " + emcc_path)
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
    if (verbose): print("EMCC install done: " + emcc_path)
    return emcc_path


def compile_tests(emcc_path, path, only_game=False, only_simd=False, compile_anyway=False, run=None, verbose=False):
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
           (name not in simdTests and only_simd) or \
           (run is not None and name != run):
            continue

        test_names.append(name)

        if not compile_anyway and os.path.exists(path + "/wasm/" + name + ".wasm"):
            if (verbose): print(name + ".wasm is found; compilation skipped")
            continue

        extraFlags = "-msimd128" if file.startswith("simd") else ""

        if (verbose): print("compiling " + name)
        bob_the_stringbuilder = emcc_path + " " + path + "/" + file + " " + extraFlags + " -s WASM=1 -s EXPORTED_FUNCTIONS=_runtime -s EXPORTED_RUNTIME_METHODS=ccall,cwrap -o " + path + "/wasm/" + name + ".wasm"
        if verbose: print(bob_the_stringbuilder)
        os.system(bob_the_stringbuilder)

    return test_names

def run_check_wasm(engine, path, name, jit=False, verbose=False):
    if not os.path.exists(path):
        print("invalid path for run")
        exit(1)

    tc_path = path + "/wasm/" + name + ".wasm"
    flags = " --jit" if (jit and "walrus" in engine) else ""

    result = subprocess.check_output(engine + " " + flags + " " + tc_path, shell=True)

    if abs(float(f'{float(result):.8f}') - float(expectedValues[name])) > 0.0000001:
        print("run failed with " + name + ".wasm", file=sys.stderr)
        print("Expected: " + str(expectedValues[name]), file=sys.stderr)
        print("Got: " + str(result), file=sys.stderr)
        exit()

    return True


def measure_time(path, name, function, engine, jit=False, verbose=False):
    start_time = time.perf_counter_ns()
    ret_val = function(engine, path, name, jit, verbose)
    end_time = time.perf_counter_ns()

    if ret_val:
        return end_time - start_time
    else:
        return math.nan

def measure_memory(path, name, engine, jit=False):
    if not os.path.exists(path):
        print("invalid path for run")
        exit(1)

    mem_tool = "/usr/bin/time -f %M "
    tc_path = path + "/wasm/" + name + ".wasm"
    flags = " --jit" if (jit and "walrus" in engine) else ""
    run_cmd = mem_tool + engine + " " + flags + " " + tc_path

    outputs = subprocess.check_output(run_cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True)
    results = outputs.split('\n')

    return int(results[1])

def run_tests(path, test_names, engines, number_of_runs, mem=False, jit=False, verbose=False):
    ret_time_val = []
    ret_mem_val = []
    for name in test_names:
        tc_path = path + "/wasm/" + name + ".wasm"
        if (verbose): print("running " + name + " (TC path: "+ tc_path  + ")")
        time_results = {}
        mem_results = {}
        for e in engines:
            time_results[e] = []
            mem_results[e] = []
        for i in range(0, number_of_runs):
            if (verbose): print("round " + str(i + 1))
            for e in engines:
                time_results[e].append(measure_time(path, name, run_check_wasm, e, jit))
                if (mem):
                    mem_results[e].append(measure_memory(path, name, e, jit))

        record = {"test": name}
        for e in engines:
            record[e] = "{:.3f}".format((sum(time_results[e]) / len(time_results[e])) / 1e9)
        ret_time_val.append(record)

        if (mem):
            record = {"test": name}
            for e in engines:
                record[e] = "{:.1f}".format(sum(mem_results[e]) / len(mem_results[e]))
            ret_mem_val.append(record)

    return (ret_time_val, ret_mem_val)


def generate_report(data, fileName=None):
    if fileName is None:
        print(markdownTable(data).setParams(row_sep='markdown', quote=False).getMarkdown())
        return
    with open(fileName, 'w') as file:
        if fileName.endswith(".csv"):
            header = "test"
            engineNames = list()
            if len(data) > 0:
                engineNames = list(data[0].keys())
                engineNames.remove("test")
            for engineName in engineNames:
                header += ";" + engineName
            print(header, file=file)
            for record in data:
                line = record["test"]
                for engineName in engineNames:
                    line += ";" + record[engineName]
                print(line, file=file)
            return
        print(markdownTable(data).setParams(row_sep='markdown', quote=False).getMarkdown(), file=file)


def main():
    args = prepare_arg_pars()
    if (args.verbose): print("Test dir: " + TEST_DIR)

    if args.engines is None:
        print("You need to specify the engine locations")
        exit(1)

    check_programs(args.engines, args.verbose)
    emcc_path = get_emcc(args.verbose)
    test_names = compile_tests(emcc_path, args.test_dir, args.only_game, args.only_simd, args.compile_anyway, args.run, args.verbose)

    result_data = run_tests(args.test_dir, test_names, args.engines, args.iterations, args.mem, args.jit, args.verbose)
    generate_report(result_data[0], args.report)
    if (args.mem):
        generate_report(result_data[1], args.report)


if __name__ == '__main__':
    main()
