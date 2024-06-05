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

from pathlib import Path
from os.path import abspath, dirname, join
from markdownTable import markdownTable  # pip install py-markdown-table


TEST_DIR = join(dirname(abspath(__file__)), "ctests")

DIFF_TRESHOLD = 1e-7

expectedValues = {
    "change": 3,
    "factorial": 899999994000000000,
    "fannkuch": 360,
    "fibonacci": 63245986,
    "gregory": 3.14159264,
    "hanoi": 67108863,
    "heapsort": 0,
    "huffman": 0,
    "kNucleotide": 1,
    "mandelbrotFloat": 775014,
    "mandelbrotDouble": 775014,
    "matrixMultiply": 3920.0,
    "miniWalrus": 27449,
    "nbody": -0.16904405,
    "nqueens": 246,
    "prime": 70657,
    "quickSort": 0,
    "redBlack": 13354000,
    "rsa": 0,
    "salesman": 2520,
    "simdMandelbrotFloat": 775014,
    "simdMandelbrotDouble": 775014,
    "simdNbody": -0.16904405,
    "simdMatrixMultiply": 3920.0,
    "ticTacToe": 18995600
}

# https://benchmarksgame-team.pages.debian.net/benchmarksgame/description/simple.html#simple
gameTests = ["mandelbrotFloat", "nbody", "gregory", "fannkuch", "kNucleotide"]

simdTests = ["simdMandelbrotFloat", "simdMandelbrotDouble", "simdNbody", "simdMatrixMultiply"]

errorList = []

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--test-dir", metavar="PATH", help="path to the test files written in c", nargs="?",
                        const=TEST_DIR, default=TEST_DIR)
    parser.add_argument("--only-game", help="only run Game tests", action="store_true")
    parser.add_argument("--only-simd", help="only run SIMD tests", action="store_true")
    parser.add_argument("--run", metavar="TEST", help="only run one benchmark")
    parser.add_argument("--engines", metavar="PATH", help="paths to wasm engines", nargs="+", default=["walrus"])
    parser.add_argument("--report", metavar="PATH", help="path to the report", nargs="?", const="./report.md")
    parser.add_argument("--iterations", metavar="NUMBER", help="how many times run the tests", nargs="?",
                        const="10", default=10, type=int)
    parser.add_argument("--compile-anyway", help="compile the tests even if they are compiled", action="store_true")
    parser.add_argument("--mem", help="measure MAX RSS", action="store_true")
    parser.add_argument("--verbose", help="prints extra informations e.g. paths", action="store_true")
    parser.add_argument("--no-system-emcc", help="Don't use emcc command from the system", action="store_true", default=False)
    parser.add_argument("--no-time", help="Don't measure time", action="store_true", default=False)
    parser.add_argument("--results", choices=["interpreter", "jit", "jit_no_reg_alloc", "jit2interpreter", "jit_no_reg_alloc2jit", "jit_no_reg_alloc2interpreter"], nargs="*", default=["interpreter", "jit", "jit_no_reg_alloc"])
    args = parser.parse_args()

    args.orig_results = args.results.copy()

    if "jit2interpreter" in args.results and not "interpreter" in args.results:
        args.results.append("interpreter")

    if "jit2interpreter" in args.results and not "jit" in args.results:
        args.results.append("jit")

    if "jit_no_reg_alloc2interpreter" in args.results and not "interpreter" in args.results:
        args.results.append("interpreter")

    if "jit_no_reg_alloc2interpreter" in args.results and not "jit_no_reg_alloc" in args.results:
        args.results.append("jit_no_reg_alloc")

    if "jit_no_reg_alloc2jit" in args.results and not "jit" in args.results:
        args.results.append("jit")

    if "jit_no_reg_alloc2jit" in args.results and not "jit_no_reg_alloc" in args.results:
        args.results.append("jit_no_reg_alloc")

    if args.no_time and not args.mem:
        raise Exception("You couldn't use --no-time without --mem")

    return args


def check_programs(engines, verbose):
    if os.system("git --version >/dev/null") != 0:
        raise Exception("Git not found!")

    for engine in engines:
        path = engine.split(" ")[0]
        if not os.path.isfile(path):
            raise Exception(f"{path} not found")

    if (verbose): print("Checks done")


def get_emcc(verbose, system_emcc=True):
    emcc_path = None 

    if system_emcc and os.system("emcc --version >/dev/null") == 0:
        if (verbose): print("Emscripten already installed on system")
        emcc_path = "emcc"
        return emcc_path

    if os.getenv("EMSDK"):
        emcc_path = join(os.getenv("EMSDK"), "upstream/emscripten/emcc.py")
        if os.path.exists(emcc_path):
            if (verbose): print(f"EMCC already installed: {emcc_path}")
            return emcc_path

    if os.path.exists("./emsdk/.git"):
        os.system("(cd ./emsdk && git fetch -a) >/dev/null")
        os.system("(cd ./emsdk && git reset --hard origin/HEAD) >/dev/null")
    else:
        os.system("git clone --depth 1 https://github.com/emscripten-core/emsdk.git ./emsdk >/dev/null")

    os.system("./emsdk/emsdk install latest >/dev/null")
    os.system("./emsdk/emsdk activate latest >/dev/null")

    emcc_path = "./emsdk/upstream/emscripten/emcc"
    if (verbose): print(f"EMCC install done: {emcc_path}")
    return emcc_path


def compile_tests(emcc_path, path, only_game, only_simd, compile_anyway, run, verbose):
    if os.system(f"{emcc_path} --version >/dev/null") != 0:
        raise Exception(f"Invalid path for emcc: {emcc_path}")

    if not os.path.exists(path):
        raise Exception(f"Invalid path for tests: {path}")

    if not os.path.exists(f"{path}/wasm"):
        os.makedirs(f"{path}/wasm")

    path = os.path.abspath(path)
    test_list = [file for file in os.listdir(path) if file.endswith(".c")]
    test_list.sort()
    test_names = list()

    for file in test_list:
        name = file.split(".")[0]

        if (name not in gameTests and only_game) or \
           (name not in simdTests and only_simd) or \
           (run is not None and name != run):
            continue

        test_names.append(name)

        if not compile_anyway and os.path.exists(f"{path}/wasm/{name}.wasm"):
            if (verbose): print(f"{name}.wasm is found; compilation skipped")
            continue

        flags = "-msimd128" if file.startswith("simd") else ""
        flags += (" -s WASM=1 -s EXPORTED_FUNCTIONS=_runtime"
                  " -s EXPORTED_RUNTIME_METHODS=ccall,cwrap"
                  " -O2"
                  f" -o {path}/wasm/{name}.wasm")
        if (verbose): print(f"compiling {name}")
        command = f"{emcc_path} {path}/{file} {flags}"
        if verbose: print(command)
        os.system(command)

    return test_names


def run_wasm(engine, path, test_name, jit, jit_no_reg_alloc):
    if not os.path.exists(path):
        raise Exception(f"Invalid path for run: {path}")

    tc_path =  f"{path}/wasm/{test_name}.wasm"
    flags = " --jit" if ((jit or jit_no_reg_alloc) and "walrus" in engine) else ""
    flags += " --jit-no-reg-alloc" if jit_no_reg_alloc else ""

    result = subprocess.check_output(f"{engine} {flags} {tc_path}", shell=True)

    diff = abs(float(f"{float(result):.8f}") - float(expectedValues[test_name]))
    if diff > DIFF_TRESHOLD:
        message = (f"Run failed with {test_name}.wasm,"
                   f" expected: {str(expectedValues[test_name])},"
                   f" but got: {str(result)}")
        raise Exception(message)


def measure_time(path, name, function, engine, jit, jit_no_reg_alloc):
    start_time = time.perf_counter_ns()
    function(engine, path, name, jit, jit_no_reg_alloc)
    end_time = time.perf_counter_ns()
    return (end_time - start_time)


def measure_memory(path, name, engine, jit, jit_no_reg_alloc):
    if not os.path.exists(path):
        raise Exception(f"Invalid path for run: {path}")

    mem_tool = "/usr/bin/time -f %M"
    tc_path = f"{path}/wasm/{name}.wasm"
    flags = " --jit" if ((jit or jit_no_reg_alloc) and "walrus" in engine) else ""
    flags += " --jit-no-reg-alloc" if jit_no_reg_alloc else ""
    run_cmd = f"{mem_tool} {engine} {flags} {tc_path}"

    outputs = subprocess.check_output(run_cmd, shell=True, stderr=subprocess.STDOUT, universal_newlines=True)
    results = outputs.split("\n")

    return int(results[1])


def run_tests(path, test_names, engines, number_of_runs, mem, no_time, jit, jit_no_reg_alloc, interpreter, verbose):
    ret_time_val = list()
    ret_mem_val = list()
    _engines = list()

    for engine in engines:
        if interpreter:
            _engines.append({
                "name": f"{engine} INTERPRETER",
                "path": engine,
                "jit": False,
                "jit_no_reg_alloc": False
            })

        if jit:
            _engines.append({
                "name": f"{engine} JIT",
                "path": engine,
                "jit": True,
                "jit_no_reg_alloc": False
            })

        if jit_no_reg_alloc:
            _engines.append({
                "name": f"{engine} JIT_NO_REG_ALLOC",
                "path": engine,
                "jit": True,
                "jit_no_reg_alloc": True
            })


    for name in test_names:
        tc_path = f"{path}/wasm/{name}.wasm"
        if (verbose): print(f"running {name} (TC path: {tc_path})")
        time_results = dict()
        mem_results = dict()
        for engine in _engines:
            time_results[engine["name"]] = list()
            mem_results[engine["name"]] = list()

        for i in range(number_of_runs):
            if (verbose): print(f"round {i + 1}")
            for engine in _engines:
                if (verbose): print(f"engine: {engine['name']}")
                try:
                    if not no_time:
                        time_results[engine["name"]].append(measure_time(path, name, run_wasm, engine["path"], engine["jit"], engine["jit_no_reg_alloc"]))
                    if (mem):
                        mem_results[engine["name"]].append(measure_memory(path, name, engine["path"], engine["jit"], engine["jit_no_reg_alloc"]))
                except Exception as e:
                    errorList.append(f"{name} {engine['name']} {e}")
                    print(e)
                    if not no_time:
                        time_results[engine["name"]].append(-1)

                    if (mem):
                        mem_results[engine["name"]].append(-1)

                    continue

        if not no_time:
            record = {"test": name}
            for engine in _engines:
                if sum(time_results[engine["name"]]) < 0:
                    record[engine["name"]] = -1
                    continue
                try: 
                    value = (sum(time_results[engine["name"]]) / len(time_results[engine["name"]])) / 1e9
                except ZeroDivisionError:
                    value = -1
                record[engine["name"]] = "{:.3f}".format(value)
            ret_time_val.append(record)

        if (mem):
            record = {"test": name}
            for engine in _engines:
                if sum(mem_results[engine["name"]]) < 0:
                    record[engine["name"]] = -1
                    continue
                try: 
                    value = (sum(mem_results[engine["name"]]) / len(mem_results[engine["name"]]))
                except ZeroDivisionError:
                    value = -1
                record[engine["name"]] = "{:.1f}".format(value)
            ret_mem_val.append(record)

    return {"time" : ret_time_val, "mem" : ret_mem_val}


def generate_report(data, file_name=None):
    if file_name is None:
        print(markdownTable(data).setParams(row_sep="markdown", quote=False).getMarkdown())
        return
    with open(file_name, "w") as file:
        if file_name.endswith(".csv"):
            header = "test"
            engine_names = list()
            if len(data) > 0:
                engine_names = list(data[0].keys())
                engine_names.remove("test")
            for engineName in engine_names:
                header += f";{engineName}"
            header += "\n"
            file.write(header)
            for record in data:
                line = record["test"]
                for engineName in engine_names:
                    line += f";{record[engineName]}"

                line += "\n"
                file.write(line)
            return
        file.write(markdownTable(data).setParams(row_sep="markdown", quote=False).getMarkdown())

def compare(data, engines, jit_to_interpreter, jit_no_reg_alloc_to_interpreter, jit_no_reg_alloc_to_jit):
    if not jit_to_interpreter and not jit_no_reg_alloc_to_interpreter and not jit_no_reg_alloc_to_jit:
        return
    for i in range(len(data)):
        test = data[i]["test"]
        for engine in engines:
            if jit_to_interpreter:
                jit = data[i][f"{engine} JIT"]
                interpreter = data[i][f"{engine} INTERPRETER"]
                data[i][f"{engine} INTERPRETER/JIT"] = "{:.2f}".format(-1 if float(interpreter) < 0 or float(jit) < 0 else float(interpreter) / float(jit))

            if jit_no_reg_alloc_to_interpreter:
                jit_no_reg_alloc = data[i][f"{engine} JIT_NO_REG_ALLOC"]
                interpreter = data[i][f"{engine} INTERPRETER"]
                data[i][f"{engine} INTERPRETER/JIT_NO_REG_ALLOC"] = "{:.2f}".format(-1 if float(interpreter) < 0 or float(jit_no_reg_alloc) < 0 else float(interpreter) / float(jit_no_reg_alloc))

            if jit_no_reg_alloc_to_jit:
                jit_no_reg_alloc = data[i][f"{engine} JIT_NO_REG_ALLOC"]
                jit = data[i][f"{engine} JIT"]
                data[i][f"{engine} JIT/JIT_NO_REG_ALLOC"] = "{:.2f}".format(-1 if float(jit) < 0 or float(jit_no_reg_alloc) < 0 else float(jit) / float(jit_no_reg_alloc))

def orderData(data, engines, orig_results):
    orderedData = list()
    for test in data:
        record = {"test": test["test"]}
        for engine in engines:
            for result in orig_results:
                if result == "interpreter":
                    record[engine + " INTERPRETER"] = test[engine + " INTERPRETER"]
                elif result == "jit":
                    record[engine + " JIT"] = test[engine + " JIT"]
                elif result == "jit_no_reg_alloc":
                    record[engine + " JIT_NO_REG_ALLOC"] = test[engine + " JIT_NO_REG_ALLOC"]
                elif result == "jit2interpreter":
                    record[engine + " INTERPRETER/JIT"] = test[engine + " INTERPRETER/JIT"]
                elif result ==  "jit_no_reg_alloc2interpreter":
                    record[engine + " INTERPRETER/JIT_NO_REG_ALLOC"] = test[engine + " INTERPRETER/JIT_NO_REG_ALLOC"]
                elif result == "jit_no_reg_alloc2jit":
                    record[engine + " JIT/JIT_NO_REG_ALLOC"] = test[engine + " JIT/JIT_NO_REG_ALLOC"]
        orderedData.append(record)
    return orderedData
def main():
    args = parse_args()
    if (args.verbose): print(f"Test dir: {TEST_DIR}")

    if args.engines is None:
        print("You need to specify the engine locations", file=sys.stderr)
        exit(1)

    memreport = None
    if args.report is not None:
        memreport = Path(args.report).absolute()
        memreport = memreport.parent / f"{memreport.stem}_mem{memreport.suffix}"
        memreport = str(memreport)

    check_programs(args.engines, args.verbose)
    emcc_path = get_emcc(args.verbose, not args.no_system_emcc)
    test_names = compile_tests(emcc_path, args.test_dir, args.only_game, args.only_simd, args.compile_anyway, args.run, args.verbose)

    result_data = run_tests(args.test_dir, test_names, args.engines, args.iterations, args.mem, args.no_time, "jit" in args.results, "jit_no_reg_alloc" in args.results, "interpreter" in args.results, args.verbose)
    if not args.no_time:
        compare(result_data["time"], args.engines, "jit2interpreter" in args.results, "jit_no_reg_alloc2interpreter" in args.results, "jit_no_reg_alloc2jit" in args.results)
        result_data["time"] = orderData(result_data["time"], args.engines, args.orig_results)
        print("TIME RESULTS")
        generate_report(result_data["time"], args.report)
    if (args.mem):
        compare(result_data["mem"], args.engines, "jit2interpreter" in args.results, "jit_no_reg_alloc2interpreter" in args.results, "jit_no_reg_alloc2jit" in args.results)
        result_data["mem"] = orderData(result_data["mem"], args.engines, args.orig_results)
        generate_report(result_data["mem"], memreport)

    if len(errorList) > 0:
        print(errorList)
        exit(1)
    
if __name__ == "__main__":
    main()
