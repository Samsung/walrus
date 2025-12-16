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
import os
from subprocess import Popen
from os.path import abspath, dirname, join

PROGRAMS_SOURCE_DIR = dirname(abspath(__file__))

PROGRAMS_EXECUTABLE_DIR = join(dirname(abspath(__file__)), "executables")

COREMARK_SOURCES = join(PROGRAMS_SOURCE_DIR, "coremark")

clang = False
emcc = False
clang_path = "wasm32-wasip1-clang"
emcc_path = "emcc"


def compile_dhrystone(compilers):
    for compiler in compilers:
        if "emcc" in compiler:
            type = "emcc"
        else:
            type = "wasi-clang"

        proc = Popen(
            [
                compiler,
                "dhrystone/dhry_1.c",
                "dhrystone/dhry_2.c",
                "-I",
                "dhrystone/include",
                "-O2",
                "-o",
                "executables/dhrystone_" + type + ".wasm",
            ]
        )

        out, _ = proc.communicate()
        if proc.returncode != 0:
            print("Error with coremark " + type + " compilation! Stopping.")
            exit(1)

    return


def compile_smallpt(compilers):
    if not os.path.isfile("forward.cpp"):
        os.system("wget http://kevinbeason.com/smallpt/forward.cpp")
        os.system("patch -u forward.cpp -i smallpt_wasm.patch")

    for compiler in compilers:
        if "emcc" in compiler:
            type = "emcc"
        else:
            type = "wasi-clang"
            compiler = compiler + "++"

        proc = Popen(
            [
                compiler,
                "forward.cpp",
                "-O2",
                "-o",
                "executables/smallpt_" + type + ".wasm",
            ]
        )

        out, _ = proc.communicate()
        if proc.returncode != 0:
            print("Error with smallpt " + type + " compilation! Stopping.")
            exit(1)

    return


def compile_coremark(compilers):
    for compiler in compilers:
        if "emcc" in compiler:
            type = "emcc"
        else:
            type = "wasi-clang"

        proc = Popen(
            [
                compiler,
                "-I" + COREMARK_SOURCES + "/posix",
                "-I" + COREMARK_SOURCES,
                '-DFLAGS_STR="-O3 -DPERFORMANCE_RUN=1"',
                "-Wl,--export=main",
                "-DITERATIONS=400000",
                "-DSEED_METHOD=SEED_VOLATILE",
                "-DPERFORMANCE_RUN=1",
                "-Wl,--allow-undefined",
                COREMARK_SOURCES + "/core_list_join.c",
                COREMARK_SOURCES + "/core_main.c",
                COREMARK_SOURCES + "/core_matrix.c",
                COREMARK_SOURCES + "/core_state.c",
                COREMARK_SOURCES + "/core_util.c",
                COREMARK_SOURCES + "/posix/core_portme.c",
                "-o",
                "executables/coremark_" + type + ".wasm",
            ]
        )

        out, _ = proc.communicate()
        if proc.returncode != 0:
            print("Error with coremark " + type + " compilation! Stopping.")
            exit(1)

    return


def parse_args():
    global emcc, emcc_path, clang, clang_path

    parser = argparse.ArgumentParser(
        description="script to compile wasm benchmarks, by default it will compile all programs"
    )
    parser.add_argument("--coremark", help="compile coremark", action="store_true")
    parser.add_argument("--smallpt", help="compile smallpt", action="store_true")
    parser.add_argument("--dhrystone", help="compile dhrystone", action="store_true")
    parser.add_argument(
        "--emcc",
        help="compile with emscripten",
        action="store_true",
    )
    parser.add_argument(
        "--emcc-path", help="path to the emcc executable", nargs="?", default=""
    )
    parser.add_argument(
        "--clang",
        help="compile with clang",
        action="store_true",
    )
    parser.add_argument(
        "--clang-path", help="path to the wasi-clang executable", nargs="?", default=""
    )
    args = parser.parse_args()

    if args.emcc is True:
        emcc = True

    if args.clang is True:
        clang = True

    if args.emcc_path:
        emcc_path = args.emcc_path

    if args.clang_path:
        clang_path = args.clang_path

    if not emcc and not clang:
        print("Please specify a compiler with either --emcc or --clang!")
        exit(1)

    return args


def main():
    global emcc, emcc_path, clang, clang_path
    args = parse_args()

    compilers = []
    tests = []

    if emcc:
        if os.system(emcc_path + " --version >/dev/null") != 0:
            print("Could not find emscripten executable")
            exit(1)
        compilers.append(emcc_path)

    if clang:
        if os.system(clang_path + " --version >/dev/null") != 0:
            print("Could not find wasi-clang executable")
            exit(1)
        compilers.append(clang_path)

    if args.coremark:
        tests.append(compile_coremark)
    if args.smallpt:
        tests.append(compile_smallpt)
    if args.dhrystone:
        tests.append(compile_dhrystone)

    if not tests:
        tests.append(compile_coremark)
        tests.append(compile_smallpt)
        tests.append(compile_dhrystone)

    for test in tests:
        test(compilers)

    print("All programs compiled succesfully!")
    return


if __name__ == "__main__":
    main()
