#!/usr/bin/env python3

# Copyright 2026-present Samsung Electronics Co., Ltd.
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

import sys

from argparse import ArgumentParser
from collections import Counter


def read_leb(data, pos):
    result = shift = 0
    while True:
        byte = data[pos]
        pos += 1
        result |= (byte & 0x7f) << shift
        shift += 7
        if byte < 0x80:
            return result, pos


def read_function_names(path):
    with open(path, 'rb') as f:
        data = f.read()

    names = {}
    pos = 8
    while pos < len(data):
        section = data[pos]
        size, pos = read_leb(data, pos + 1)
        end = pos + size

        length, start = read_leb(data, pos)
        if section == 0 and data[start:start + length] == b'name':
            pos = start + length
            while pos < end:
                subsection = data[pos]
                size, pos = read_leb(data, pos + 1)
                if subsection == 1:
                    count, entry = read_leb(data, pos)
                    for _ in range(count):
                        index, entry = read_leb(data, entry)
                        length, entry = read_leb(data, entry)
                        names[index] = data[entry:entry + length].decode('utf-8', 'replace')
                        entry += length
                pos += size
        pos = end

    return names


def read_profile(path):
    header = {}
    snapshots = []
    functions = {}

    with open(path) as f:
        lines = iter(f.readlines())

    for line in lines:
        fields = line.split()
        if not fields:
            continue

        kind = fields[0]
        if kind == 'snapshot':
            instructions, entries, pages = int(fields[2]), int(fields[3]), int(fields[4])
            for _ in range(entries):
                next(lines)
            while pages > 0:
                count = int(next(lines).split()[3])
                for _ in range(count):
                    next(lines)
                pages -= count
            snapshots.append([instructions, 0, []])
        elif kind == 'stack':
            pushed = [int(slot) for slot in next(lines).split()] if int(fields[2]) else []
            snapshots[-1][1:] = [int(fields[1]), pushed]
        elif kind == 'function':
            functions[int(fields[2])] = int(fields[1])
        elif kind not in ('keyframe', 'exit', 'layout', 'block', 'memory', 'end'):
            header[kind] = fields[1:]

    return header, snapshots, functions


def fold(snapshots, functions, names):
    folded = Counter()
    stack = []

    for instructions, keep, pushed in snapshots:
        stack = stack[:keep] + pushed
        frames = [names.get(functions[slot], 'func[%d]' % functions[slot]) for slot in stack] or ['[unknown]']
        folded[';'.join(frames)] += instructions

    return folded


def main():
    parser = ArgumentParser(description='Convert a walrus profile into folded stacks for flame graph tools')
    parser.add_argument('profile', metavar='PATH',
                        help='profile written by a WALRUS_PROFILER build')
    parser.add_argument('--wasm', metavar='PATH',
                        help='module whose name section gives function names')
    parser.add_argument('-o', '--output', metavar='PATH',
                        help='output file (default: stdout)')
    args = parser.parse_args()

    header, snapshots, functions = read_profile(args.profile)
    if 'stack' not in header.get('features', ['stack']) or int(header['walrus-profile'][0]) < 6:
        print('%s: no call stack records, record with WALRUS_PROFILE_FEATURES including stack' % args.profile)
        sys.exit(1)

    names = read_function_names(args.wasm) if args.wasm else {}
    output = open(args.output, 'w') if args.output else sys.stdout
    for stack, instructions in sorted(fold(snapshots, functions, names).items()):
        output.write('%s %d\n' % (stack, instructions))


if __name__ == '__main__':
    main()
