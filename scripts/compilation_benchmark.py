#!/usr/bin/env python3
#
# runs a compilation benchmark

import subprocess
import tempfile
import argparse
import datetime
import statistics


class Sample:
    def __init__(self, t0, t1, t2):
        self._configure = (t1-t0).total_seconds()
        self._compile = (t2-t1).total_seconds()

# benchmarks compilation, returns the time consumption


def measure_once(standard: int):
    builddir = tempfile.TemporaryDirectory()
    t0 = datetime.datetime.now()
    subprocess.run(["cmake", f"-DSIMDUTF_CXX_STANDARD={standard}", "-DSIMDUTF_FAST_TESTS=On",
                   "-B", builddir.name, "-GNinja"], stdout=subprocess.DEVNULL)
    t1 = datetime.datetime.now()
    subprocess.run(["cmake", "--build", builddir.name],
                   stdout=subprocess.DEVNULL)
    t2 = datetime.datetime.now()
    # print(f"configure took {t1-t0}, build took {t2-t1}")
    return Sample(t0, t1, t2)


def benchmark_single(standard: int, repetitions: int = 3):
    d = [measure_once(standard) for i in range(repetitions)]

    m = statistics.median([e._configure for e in d])
    print(f"median configure is {m:.3f}s")

    m = statistics.median([e._compile for e in d])
    print(f"median compile is {m:.3f}s")


def main():
    parser = argparse.ArgumentParser(
        prog='compilation benchmark',
        description='benchmarks compilation of simdutf')
    parser.add_argument('--standard', type=int, default=23)
    parser.add_argument('--repetitions', type=int, default=3)

    args = parser.parse_args()
    benchmark_single(standard=args.standard, repetitions=args.repetitions)


if __name__ == '__main__':
    main()
