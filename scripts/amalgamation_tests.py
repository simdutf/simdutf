#!/usr/bin/env python3
#
# runs a test to ensure amalagamation works

import subprocess
import tempfile
import argparse
import datetime
import itertools
import math
import random


def test_one(args, features):
    print(f"testing with these features enabled: {', '.join(features)}")
    builddir = tempfile.TemporaryDirectory(delete=not args.keep_tmpdirs)
    t0 = datetime.datetime.now()
    subprocess.run(
        [
            "python3",
            "singleheader/amalgamate.py",
            "--source-dir",
            "src",
            "--include-dir",
            "include",
            "--output-dir",
            builddir.name,
            "--no-zip",
            "--no-readme",
        ]
        + [f"--with-{feature}" for feature in features],
        stdout=subprocess.DEVNULL,
        check=True,
    )
    print(f"amalgamated in {builddir.name}")
    if args.warnings:
        warnings = [
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Wno-error=unused-function",
            "-Wno-error=unused-const-variable",
        ]
    else:
        warnings = []
    subprocess.run(
        [args.compiler, "-c", "simdutf.cpp", "-I", ".", f"-std=c++{args.standard}"]
        + warnings,
        check=True,
        cwd=builddir.name,
    )
    print(f"compiled in {builddir.name}")


allfeatures = ["utf8", "utf16", "utf32", "base64", "detect-enc", "ascii", "latin1"]


def test_combinations(args, include_count):
    total = math.comb(len(allfeatures), include_count)
    print(f"testing all {total} possible selections of {include_count} features")
    for i, combo in enumerate(itertools.combinations(allfeatures, include_count)):
        test_one(args, combo)
        print(f"tested {i + 1} of {total}")


def generate_subset():
    s = allfeatures
    for subset in itertools.chain.from_iterable(
        itertools.combinations(s, r) for r in range(len(s) + 1)
    ):
        yield subset


def test_random(args, N):
    subsets = [s for s in generate_subset()]
    random.shuffle(subsets)
    if N > 0 and N < len(subsets):
        subsets = subsets[0:N]
    total = len(subsets)
    for i, subset in enumerate(subsets):
        print(f"testing {i + 1} of {total} subsets: {subset}")
        test_one(args, subset)


def main():
    parser = argparse.ArgumentParser(
        prog="amalgamation test", description="ensures that amalgamation works"
    )
    parser.add_argument("--standard", type=int, default=23)
    parser.add_argument("--keep-tmpdirs", action="store_true")
    parser.add_argument("--compiler", type=str, default="c++")
    parser.add_argument("--warnings", action="store_true")
    parser.add_argument(
        "--test-implicit-all",
        action="store_true",
        help="test giving no features, meaning all features are implicitly on",
    )
    parser.add_argument(
        "--test-explicit-all", action="store_true", help="test giving all features"
    )
    parser.add_argument(
        "--test-singles", action="store_true", help="test one feature at a time"
    )
    parser.add_argument(
        "--test-random",
        type=int,
        default=-1,
        help="tests up to the given amount of random subsets. 0 means all. negative means don't do this.",
    )
    parser.add_argument(
        "--test-pairs", action="store_true", help="tests all pairs of features"
    )
    parser.add_argument(
        "--test-triplets", action="store_true", help="tests all triplets of features"
    )
    parser.add_argument(
        "--test-all-but-one",
        action="store_true",
        help="tests all possible feature sets with one disabled",
    )

    args = parser.parse_args()

    if args.test_implicit_all:
        print("test all enabled, implicitly")
        test_one(args, [])

    if args.test_explicit_all:
        print("test all enabled, explicitly")
        test_one(args, allfeatures)

    if args.test_singles:
        print("test one single feature at a time")
        test_combinations(args, 1)

    if args.test_random >= 0:
        print("test random")
        test_random(args, args.test_random)

    if args.test_pairs:
        print("test all pairs")
        test_combinations(args, 2)

    if args.test_triplets:
        print("test all triplets")
        test_combinations(args, 3)

    if args.test_all_but_one:
        print("test all but one single feature at a time")
        test_combinations(args, len(allfeatures) - 1)

    print("all seems to have gone well. bye bye!")


if __name__ == "__main__":
    main()
