#!/usr/bin/env python3

from pathlib import Path
import sys
import argparse
import json
import subprocess


def main():
    args = parse_args()

    benchmark = args.build_dir / 'benchmarks' / 'benchmark'
    if not benchmark.is_file():
        raise RuntimeError(f"{benchmark} not found, was simdutf compiled with `-DSIMDUTF_BENCHMARKS=ON`?")

    # load registered tests
    simdutf_tests = [entry for entry in json.loads(execute([benchmark, "--json"])) if entry["simdutf"]]
    maxlen = max(len(entry['name']) for entry in simdutf_tests)
    count = len(simdutf_tests)

    for id, test in enumerate(simdutf_tests, 1):
        name = test["name"]
        encodings = test["encodings"]

        print(f"[{id}/{count}] {name:{maxlen}}... ", end='')
        sys.stdout.flush()
        if len(encodings) > 1:
            print(f"skipping, requires multiple input encodings ({encodings})")
            continue

        report_name = (name + '.txt')
        report_path = args.output_dir / report_name
        if report_path.exists():
            print("already benchmarked")
            continue

        enc = encodings[0]
        cmd = [benchmark, "-P", name, "-F"] + input_files(args.lipsum_dir, enc)
        result = execute(cmd)
        print(f"creating {report_path}")
        report_path.write_bytes(result)


def input_files(lipsum_dir, encoding):
    dir = lipsum_dir / 'wikipedia_mars'
    return list(dir.glob(patterns[encoding]))


# in unicode_ipsum/wikipedia_mars
patterns = {
    'latin1'    : '*.latin1.txt',
    'utf8'      : '*.utf8.txt',
    'utf16le'   : '*.utf16.txt',
    'utf16be'   : '*.utf16be.txt',
    'utf32le'   : '*.utf32.txt',
}


def parse_args():
    p = argparse.ArgumentParser("SIMDUTF tool for benchmarking")
    p.add_argument("--build-dir",
                   required=True,
                   type=existing_dir,
                   metavar="DIR",
                   help="simdutf build directory")
    p.add_argument("--lipsum-dir",
                   required=True,
                   type=existing_dir,
                   metavar="DIR",
                   help="unicode lorem lipsum directory")
    p.add_argument("--output-dir",
                   required=True,
                   type=existing_dir,
                   metavar="DIR",
                   help="output directory")

    args = p.parse_args()

    return args


def existing_dir(s):
    path = Path(s).expanduser().absolute()
    if not path.is_dir():
        raise argparse.ArgumentError("'{path}' is not a directory")

    return path


def execute(params):
    cmd = [str(param) for param in params]
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        print(' '.join(cmd))
        if result.stderr:
            print("stderr:")
            print(result.stderr)
        if result.stdout:
            print("stdout:")
            print(result.stdout)
        print(f"return code: {result.returncode}")
        raise ValueError("command failed")

    return result.stdout


if __name__ == '__main__':
    main()
