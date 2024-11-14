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

from os.path import abspath, dirname, join

PROGRAMS_SOURCE_DIR = dirname(abspath(__file__))

def compile_coremark():
    return

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--all", help="compile all programs", action="store_true", default=True)
    parser.add_argument("--coremark", help="compile coremark", action="store_true")
    parser.add_argument("--summary", help="Generate summary", action="store_true", default=False)
    args = parser.parse_args()

    args.orig_results = args.results.copy()
    return args

def main():
    args = parse_args()
    
    if args.all:
        compile_coremark()
    
    print("All programs compiled succesfully!")
    return

if __name__ == "__main__":
    main()
