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
from subprocess import PIPE, Popen

from os.path import abspath, dirname, join

PROGRAMS_SOURCE_DIR = dirname(abspath(__file__))

PROGRAMS_EXECUTABLE_DIR = join(dirname(abspath(__file__)), "executables")

COREMARK_SOURCES = join(PROGRAMS_SOURCE_DIR, "coremark")

clang = False
emcc = False
clang_path = '/opt/wasi-sdk/bin/clang'
emcc_path = '/usr/lib/emscripten/emcc'

def compile_coremark():
    global emcc, emcc_path, clang, clang_path

    if clang:
        if os.system(clang_path + " --version >/dev/null") != 0:
            print("Could not find wasi-clang for building coremark.")
            exit(1)

        proc = Popen([clang_path,
                      '-O3',
                      '-I'+COREMARK_SOURCES+'/posix', '-I'+COREMARK_SOURCES,
                      '-DFLAGS_STR="-O3 -DPERFORMANCE_RUN=1"',
                      '-Wl,--export=main',
                      '-DITERATIONS=400000',
                      '-DSEED_METHOD=SEED_VOLATILE',
                      '-DPERFORMANCE_RUN=1',
                      '-Wl,--allow-undefined',
                      COREMARK_SOURCES + '/core_list_join.c',
                      COREMARK_SOURCES + '/core_main.c',
                      COREMARK_SOURCES + '/core_matrix.c',
                      COREMARK_SOURCES + '/core_state.c',
                      COREMARK_SOURCES + '/core_util.c',
                      COREMARK_SOURCES + '/posix/core_portme.c',
                      '-o'+ PROGRAMS_EXECUTABLE_DIR +'/coremark_clang.wasm'
                      ])

        out, _ = proc.communicate()

        if proc.returncode != 0:
            print("Error with clang compilation! Stopping.")
            exit(1)


    if emcc:
        if os.system(emcc_path + " --version >/dev/null") != 0:
            print("Could not find emcc for building coremark.")
            exit(1)

        proc = Popen([emcc_path,
                      '-O3',
                      '-I'+COREMARK_SOURCES, '-I',COREMARK_SOURCES+'/posix',
                      '-DFLAGS_STR="-O3 -DPERFORMANCE_RUN=1"',
                      '-Wl,--allow-undefined',
                      '-DITERATIONS=400000',
                      '-DSEED_METHOD=SEED_VOLATILE',
                      '-DPERFORMANCE_RUN=1',
                      '-Wl,--export=main',
                      '-sWASM=1',
                      '-sEXPORTED_FUNCTIONS=_main',
                      '-sEXPORTED_RUNTIME_METHODS=ccal,cwrap',
                      COREMARK_SOURCES + '/core_list_join.c',
                      COREMARK_SOURCES + '/core_main.c',
                      COREMARK_SOURCES + '/core_matrix.c',
                      COREMARK_SOURCES + '/core_state.c',
                      COREMARK_SOURCES + '/core_util.c',
                      COREMARK_SOURCES + '/posix/core_portme.c',
                      '-o'+ PROGRAMS_EXECUTABLE_DIR +'/coremark_emcc.wasm'
                      ])

        out, _ = proc.communicate()

        if proc.returncode != 0:
            print("Error with emscripten compilation! Stopping.")
            exit(1)

    return


def parse_args():
    global emcc, emcc_path, clang, clang_path

    parser = argparse.ArgumentParser()
    parser.add_argument("--all", help="compile all programs", action="store_true", default=True)
    parser.add_argument("--coremark", help="compile coremark", action="store_true")
    parser.add_argument("--summary", help="Generate summary", action="store_true", default=False)
    parser.add_argument("--emcc", help="Compile with emscripten. If there is no system emcc then follow the argument with a path to the emcc executeable.", nargs="?", default="")
    parser.add_argument("--clang", help="Compile with clang. If there is no system emcc then follow the argument with a path to the emcc executeable.", nargs="?", default="")
    args = parser.parse_args()

    if args.emcc != "" and args.emcc is not None:
        emcc_path = args.emcc
        emcc = True
    elif args.emcc != "":
        emcc = True

    if args.clang != "" and args.clang is not None:
        clang_path = args.clang
        clang = True
    elif args.clang != "":
        clang = True

    if not clang and not emcc:
        print("Please define which compilers to use with --emcc, --clang!")
        exit(1)

    return args

def main():
    args = parse_args()

    if args.all:
        compile_coremark()

    print("All programs compiled succesfully!")
    return

if __name__ == "__main__":
    main()
