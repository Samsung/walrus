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
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import platform
import shlex
import shutil
import subprocess
import time

from pathlib import Path
from os.path import join
from py_markdown_table.markdown_table import markdown_table
import pandas as pd


SCRIPT_DIR = Path(__file__).resolve().parent
TEST_DIR = SCRIPT_DIR / "PolyBenchC-4.2.1"
WASM_DIR = SCRIPT_DIR / "wasm"

MODE_FLAGS = {
    "i": (),
    "j": ("--jit",),
    "n": ("--jit", "--jit-no-reg-alloc"),
}

MODE_NAMES = {
    "i": "INTERPRETER",
    "j": "JIT",
    "n": "JIT_NO_REG_ALLOC",
}

RESULT_CHOICES = ["i", "j", "n", "j2i", "i2j", "n2j", "j2n", "n2i", "i2n"]

engine_map = {}
engine_path = 0
error_list = []


class CustomHelpFormat(argparse.HelpFormatter):

  def __init__(self, *args, **kwargs):
    kwargs["max_help_position"] = 70
    super().__init__(*args, **kwargs)


def parse_args():
  parser = argparse.ArgumentParser(
      usage="%(prog)s [options]",
      description="Compile and run the PolyBench/C benchmarks with Walrus.",
      formatter_class=CustomHelpFormat,
  )
  parser.add_argument(
      "--test-dir",
      metavar="PATH",
      type=Path,
      default=TEST_DIR,
      help=f"path to PolyBench/C (default: {TEST_DIR})",
  )
  parser.add_argument(
      "--wasm-dir",
      metavar="PATH",
      type=Path,
      default=WASM_DIR,
      help=f"directory for compiled wasm files (default: {WASM_DIR})",
  )
  parser.add_argument(
      "--engines",
      metavar="PATH",
      nargs="+",
      help="paths to Walrus executables",
  )
  parser.add_argument(
      "--iterations",
      metavar="NUMBER",
      type=int,
      default=4,
      help="number of times to run each benchmark (default: 4)",
  )
  parser.add_argument(
      "--dataset",
      choices=["MINI", "SMALL", "MEDIUM", "LARGE", "EXTRALARGE"],
      default="MEDIUM",
      help="PolyBench dataset size (default: MEDIUM)",
  )
  parser.add_argument(
      "--results",
      choices=RESULT_CHOICES,
      nargs="*",
      default=list(MODE_FLAGS),
      help="results to show (default: i j n)",
  )
  parser.add_argument("--run", metavar="TEST", help="only run one benchmark")
  parser.add_argument(
      "--report",
      metavar="PATH",
      nargs="?",
      const="./report.md",
      help="write the report as Markdown or CSV",
  )
  parser.add_argument(
      "--compile-anyway",
      action="store_true",
      help="compile benchmarks even when their wasm files already exist",
  )
  parser.add_argument(
      "--verbose",
      action="store_true",
      help="print compile and run commands",
  )
  parser.add_argument(
      "--no-system-emcc",
      action="store_true",
      help="download emsdk instead of using emcc from PATH",
  )
  parser.add_argument(
      "--mem",
      action="store_true",
      help="measure maximum resident set size",
  )
  parser.add_argument(
      "--no-time",
      action="store_true",
      help="only measure memory",
  )
  parser.add_argument(
      "--summary",
      action="store_true",
      help="append mean values to the report",
  )
  parser.add_argument(
      "--engine-path",
      type=int,
      default=-2,
      help="engine display path: -2=number, -1=hide, 0=full, positive=last path components",
  )
  parser.add_argument(
      "--arch",
      help="architecture to use when Walrus must be built",
      nargs="?",
  )

  args = parser.parse_args()
  if args.iterations < 1:
    parser.error("--iterations must be at least 1")
  if args.no_time and not args.mem:
    parser.error("--no-time requires --mem")
  if args.engine_path < -2:
    parser.error("--engine-path must be -2 or greater")

  args.orig_results = args.results.copy()
  for result in args.orig_results:
    if "2" not in result:
      continue
    source, target = result.split("2")
    if source not in args.results:
      args.results.append(source)
    if target not in args.results:
      args.results.append(target)

  return args


def find_program(program, description):
  path = Path(program)
  if path.parent != Path("."):
    path = path.expanduser().resolve()
    if not path.is_file():
      raise FileNotFoundError(f"{description} not found: {path}")
    return str(path)

  resolved = shutil.which(program)
  if resolved is None:
    raise FileNotFoundError(f"{description} not found in PATH: {program}")
  return resolved


def check_engines(engines):
  checked = []
  for engine in engines:
    command = shlex.split(engine)
    if not command:
      raise ValueError("engine command must not be empty")
    command[0] = find_program(command[0], "wasm engine")
    checked.append(command)
  return checked


def get_emcc(verbose, system_emcc=True):
  emcc_path = None

  if system_emcc and os.system("emcc --version >/dev/null") == 0:
    if verbose:
      print("Emscripten already installed on system")
    emcc_path = "emcc"
    return emcc_path

  if os.getenv("EMSDK"):
    emcc_path = join(os.getenv("EMSDK"), "upstream/emscripten/emcc.py")
    if os.path.exists(emcc_path):
      if verbose:
        print(f"EMCC already installed: {emcc_path}")
      return emcc_path

  if os.path.exists("./emsdk/.git"):
    os.system("(cd ./emsdk && git fetch -a) >/dev/null")
    os.system("(cd ./emsdk && git reset --hard origin/HEAD) >/dev/null")
  else:
    os.system(
        "git clone --depth 1 https://github.com/emscripten-core/emsdk.git ./emsdk >/dev/null"
    )

  os.system("./emsdk/emsdk install latest >/dev/null")
  os.system("./emsdk/emsdk activate latest >/dev/null")

  emcc_path = "./emsdk/upstream/emscripten/emcc"
  if verbose:
    print(f"EMCC install done: {emcc_path}")
  return emcc_path


def engine_display_name(engine):
  if engine not in engine_map:
    if engine_path == -2:
      engine_map[engine] = str(len(engine_map))
    elif engine_path == -1:
      engine_map[engine] = ""
    elif engine_path == 0:
      engine_map[engine] = engine
    else:
      command = shlex.split(engine)
      engine_map[engine] = "/".join(command[0].split("/")[-engine_path:])

  return engine_map[engine]


def compile_tests(
    emcc_path,
    path,
    wasm_dir,
    dataset,
    compile_anyway,
    run,
    verbose,
):
  path = Path(path).resolve()
  wasm_dir = Path(wasm_dir).resolve()

  benchmark_list = path / "utilities" / "benchmark_list"
  utility_source = path / "utilities" / "polybench.c"
  utility_include = path / "utilities"

  if not benchmark_list.is_file():
    raise FileNotFoundError(f"{benchmark_list} not found")
  if not utility_source.is_file():
    raise FileNotFoundError(f"{utility_source} not found")

  wasm_dir.mkdir(parents=True, exist_ok=True)
  source_files = benchmark_list.read_text(encoding="utf-8").split()
  tests = []

  for source_file in source_files:
    source_file = source_file.removeprefix("./")
    source_path = path / source_file
    include_path = source_path.parent
    test_name = source_path.stem
    wasm_path = wasm_dir / f"{test_name}.wasm"

    if run is not None and test_name != run:
      continue
    if not source_path.is_file():
      raise FileNotFoundError(f"{source_path} not found")

    tests.append((test_name, wasm_path))
    if not compile_anyway and wasm_path.exists():
      if verbose:
        print(f"{wasm_path} found; compilation skipped")
      continue

    command = [
        emcc_path,
        str(utility_source),
        str(source_path),
        f"-I{utility_include}",
        f"-I{include_path}",
        "-O2",
        f"-D{dataset}_DATASET",
        "-DPOLYBENCH_NO_FLUSH_CACHE",
        "-s",
        "WASM=1",
        "-o",
        str(wasm_path),
    ]

    if verbose:
      print("compiling:", shlex.join(command))
    subprocess.run(command, check=True)

  if run is not None and not tests:
    raise ValueError(f"unknown PolyBench test: {run}")
  if not tests:
    raise ValueError(f"no PolyBench tests found in {benchmark_list}")

  return tests


def run_command(command):
  result = subprocess.run(command, capture_output=True, text=True)
  if result.returncode != 0:
    output = result.stderr.strip() or result.stdout.strip()
    message = f"command failed ({result.returncode}): {shlex.join(command)}"
    if output:
      message += f"\n{output}"
    raise RuntimeError(message)


def measure_time(command):
  started = time.perf_counter_ns()
  run_command(command)
  return time.perf_counter_ns() - started


def measure_memory(command):
  memory_marker = "POLYBENCH_MAX_RSS="
  result = subprocess.run(
      ["/usr/bin/time", "-f", f"{memory_marker}%M", *command],
      stdout=subprocess.PIPE,
      stderr=subprocess.PIPE,
      text=True,
  )
  if result.returncode != 0:
    output = result.stderr.strip() or result.stdout.strip()
    raise RuntimeError(
        f"command failed ({result.returncode}): {shlex.join(command)}\n{output}"
    )

  for line in reversed(result.stderr.splitlines()):
    if line.startswith(memory_marker):
      return int(line.removeprefix(memory_marker))

  raise RuntimeError(f"could not measure memory: {shlex.join(command)}")


def build_engine_modes(engine_args, engines, modes):
  engine_modes = []
  for engine_arg, engine in zip(engine_args, engines):
    for mode in MODE_FLAGS:
      if mode not in modes:
        continue
      flags = MODE_FLAGS[mode] if "walrus" in engine_arg else ()
      engine_modes.append({
          "name": f"{engine_display_name(engine_arg)} {MODE_NAMES[mode]}",
          "command": [*engine, *flags],
      })
  return engine_modes


def average(values, scale, precision):
  if not values or any(value < 0 for value in values):
    return -1
  return f"{sum(values) / len(values) / scale:.{precision}f}"


def run_tests(
    tests,
    engine_args,
    engines,
    iterations,
    mem,
    no_time,
    modes,
    verbose,
):
  time_records = []
  memory_records = []
  engine_modes = build_engine_modes(engine_args, engines, modes)

  for test_name, wasm_path in tests:
    if verbose:
      print(f"running {test_name} (wasm: {wasm_path})")

    time_results = {engine["name"]: [] for engine in engine_modes}
    memory_results = {engine["name"]: [] for engine in engine_modes}

    for iteration in range(iterations):
      if verbose:
        print(f"round {iteration + 1}")

      for engine in engine_modes:
        command = [*engine["command"], str(wasm_path)]
        if verbose:
          print(f"engine {engine['name']}: {shlex.join(command)}")

        try:
          elapsed = measure_time(command) if not no_time else None
          max_rss = measure_memory(command) if mem else None
          if elapsed is not None:
            time_results[engine["name"]].append(elapsed)
          if max_rss is not None:
            memory_results[engine["name"]].append(max_rss)
        except Exception as error:
          message = f"{test_name} {engine['name']}: {error}"
          error_list.append(message)
          print(message)
          if not no_time:
            time_results[engine["name"]].append(-1)
          if mem:
            memory_results[engine["name"]].append(-1)

    if not no_time:
      record = {"test": test_name}
      for engine in engine_modes:
        record[engine["name"]] = average(
            time_results[engine["name"]], 1e9, 3
        )
      time_records.append(record)

    if mem:
      record = {"test": test_name}
      for engine in engine_modes:
        record[engine["name"]] = average(
            memory_results[engine["name"]], 1, 1
        )
      memory_records.append(record)

  return {"time": time_records, "mem": memory_records}


def comparison_key(engine, result):
  source, target = result.split("2")
  return (
      f"{engine_display_name(engine)} "
      f"{MODE_NAMES[target]}/{MODE_NAMES[source]}"
  )


def compare(data, engines, results):
  comparisons = [result for result in results if "2" in result]
  for record in data:
    for engine in engines:
      engine_name = engine_display_name(engine)
      for result in comparisons:
        source, target = result.split("2")
        source_value = float(record[f"{engine_name} {MODE_NAMES[source]}"])
        target_value = float(record[f"{engine_name} {MODE_NAMES[target]}"])
        ratio = -1
        if source_value >= 0 and target_value >= 0 and source_value != 0:
          ratio = target_value / source_value
        record[comparison_key(engine, result)] = (
            f"{source_value:g} ({ratio:.2f}x)"
        )


def order_data(data, engines, results):
  ordered = []
  for test in data:
    record = {"test": test["test"]}
    for engine in engines:
      for result in results:
        if "2" in result:
          key = comparison_key(engine, result)
        else:
          key = f"{engine_display_name(engine)} {MODE_NAMES[result]}"
        record[key] = test[key]
    ordered.append(record)
  return ordered


def generate_report(data, summary, file_name=None):
  if summary:
    df = pd.DataFrame.from_records(data)
    for column in df.columns:
      if column == "test":
        continue
      if "/" in column.split(" ")[-1]:
        df[column] = df[column].str.split(" ").str[-1].str[1:-2]
      df[column] = df[column].astype(float)

    df = df.describe().loc[["mean"]].to_dict("records")
    df[0]["test"] = "MEAN"
    separator = [{column: "*" for column in data[0]}]
    data = data + separator + df

  if file_name is None:
    print(
        markdown_table(data).set_params(row_sep="markdown",
                                        quote=False).get_markdown()
    )
    if engine_path == -2:
      print("\n\n# Engines")
      for engine, serial in engine_map.items():
        print(f"{serial}: {engine}")
    return

  file_name = str(file_name)
  with open(file_name, "w") as report:
    if file_name.endswith(".csv"):
      header = "test"
      engine_names = []
      if data:
        engine_names = list(data[0])
        engine_names.remove("test")
      for engine_name in engine_names:
        header += f";{engine_name}"
      report.write(f"{header}\n")

      for record in data:
        line = record["test"]
        for engine_name in engine_names:
          line += f";{record[engine_name]}"
        report.write(f"{line}\n")

      if engine_path == -2:
        report.write("\n\n# Engines\n")
        for engine, serial in engine_map.items():
          report.write(f"{serial};{engine}\n")
      return

    report.write(
        markdown_table(data).set_params(row_sep="markdown",
                                        quote=False).get_markdown()
    )
    if engine_path == -2:
      report.write("\n\nEngines:\n")
      for engine, serial in engine_map.items():
        report.write(f"{serial}: {engine}\n")


def getWalrus(args):
  walrus_dir = Path("./walrus")
  if (walrus_dir / ".git").exists():
    subprocess.run(["git", "fetch", "-a"], cwd=walrus_dir, check=True)
    subprocess.run(
        ["git", "reset", "--hard", "origin/HEAD"],
        cwd=walrus_dir,
        check=True,
    )
  else:
    subprocess.run(
        [
            "git",
            "clone",
            "--depth",
            "1",
            "https://github.com/Samsung/walrus.git",
            str(walrus_dir),
        ],
        check=True,
    )

  subprocess.run(
      ["git", "submodule", "update", "--init"],
      cwd=walrus_dir,
      check=True,
  )

  arch = args.arch or platform.machine()
  if "x86_64" in arch:
    arch = "x64"
  elif "i386" in arch or "x86" in arch:
    arch = "x86"
  elif "aarch64" in arch or "arm64" in arch:
    arch = "aarch64"
  elif "arm" in arch or "arm32" in arch:
    arch = "arm"

  build_dir = walrus_dir / "out/release" / arch
  if build_dir.exists():
    shutil.rmtree(build_dir)

  subprocess.run(
      [
          "cmake",
          "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
          "-H.",
          f"-Bout/release/{arch}",
          f"-DWALRUS_ARCH={arch}",
          "-DWALRUS_HOST=linux",
          "-DWALRUS_MODE=release",
          "-DWALRUS_OUTPUT=shell",
          "-GNinja",
      ],
      cwd=walrus_dir,
      stdout=subprocess.DEVNULL,
      check=True,
  )
  subprocess.run(
      ["ninja", "-C", f"./out/release/{arch}"],
      cwd=walrus_dir,
      check=True,
  )
  return str(build_dir / "walrus")


def main():
  args = parse_args()

  global engine_path
  engine_path = args.engine_path

  if args.engines is None:
    args.engines = [getWalrus(args)]
    print(args.engines)

  if args.verbose:
    print(f"PolyBench directory: {args.test_dir.resolve()}")
    print(f"wasm output directory: {args.wasm_dir.resolve()}")

  emcc_path = get_emcc(args.verbose, not args.no_system_emcc)
  engines = check_engines(args.engines)
  tests = compile_tests(
      emcc_path,
      args.test_dir,
      args.wasm_dir,
      args.dataset,
      args.compile_anyway,
      args.run,
      args.verbose,
  )
  results = run_tests(
      tests,
      args.engines,
      engines,
      args.iterations,
      args.mem,
      args.no_time,
      args.results,
      args.verbose,
  )

  if not args.no_time:
    compare(results["time"], args.engines, args.orig_results)
    results["time"] = order_data(
        results["time"], args.engines, args.orig_results
    )
    if args.report is None:
      print("# Time results\n")
    generate_report(results["time"], args.summary, args.report)

  if args.mem:
    compare(results["mem"], args.engines, args.orig_results)
    results["mem"] = order_data(
        results["mem"], args.engines, args.orig_results
    )
    memory_report = None
    if args.report is not None:
      report_path = Path(args.report)
      memory_report = report_path.with_name(
          f"{report_path.stem}_mem{report_path.suffix}"
      )
    else:
      print("# Memory results\n")
    generate_report(results["mem"], args.summary, memory_report)

  if error_list:
    print("\n# Errors")
    for error in error_list:
      print(error)
    raise SystemExit(1)


if __name__ == "__main__":
  main()
