#!/usr/bin/env python3

import sys
from itertools import chain
from pathlib import Path
from table import Table


HELP = """
$ base64bench_print.py old.txt old.new
"""


def main():
    script = sys.argv[0]
    args = sys.argv[1:]
    n = len(args)
    if "-h" in args or "--help" in args or (n != 2):
        print(HELP)
        return

    if n == 2:
        with open(args[0]) as f:
            old = parse(f.read())
        with open(args[1]) as f:
            new = parse(f.read())

        compare(old, new)


def compare(old, new):
    if old.metainfo != new.metainfo:
        print(old.metainfo)
        print(new.metainfo)
        return

    all = {impl: True for impl in chain(old.measurements.keys(), new.measurements.keys())}

    mi = old.metainfo
    title = '%s of %d bytes from %d input(s)' % (mi.mode, mi.volume, mi.inputs_count)

    table = Table()
    table.set_header(["implementation", "old [GB/s]", "new [GB/s]", "speedup"])
    for impl in all:
        try:
            old_speed = old.measurements[impl]
        except KeyError:
            old_speed = None

        try:
            new_speed = new.measurements[impl]
        except KeyError:
            new_speed = None

        if old_speed is not None and new_speed is not None:
            speedup = '%0.2fx' % (new_speed / old_speed)
            new_speed = '%0.2f' % new_speed
            old_speed = '%0.2f' % old_speed

        if old_speed is None:
            old_speed = '--'
            speedup   = '--'

        if new_speed is None:
            new_speed = '--'
            speedup   = '--'

        table.add_row([impl, old_speed, new_speed, speedup])

    print(table)


class Data:
    def __init__(self, mi):
        self.metainfo = mi
        self.measurements = {}


def parse(string):
    data = Data(parse_metainfo(string))
    for line in string.splitlines():
        line = line.rstrip()
        if not line:
            continue

        if not line.startswith('#'):
            impl, speed = parse_result(line)
            data.measurements[impl] = speed

    return data


def parse_metainfo(string):
    mi = Metainfo()
    mi.system = clip(string, '# current system detected as ', '.')
    mi.volume = int(clip(string, '# volume: ', ' bytes'))
    mi.max_length = int(clip(string, '# max length: ', ' bytes'))
    mi.inputs_count = int(clip(string, '# number of inputs: ', '\n'))

    modes = [
        ('# decode', 'decoding'),
    ]

    for frag, mode in modes:
        if frag in string:
            mi.mode = mode
            break

    return mi


class Metainfo:
    def __init__(self):
        self.system = None
        self.volume = None
        self.max_length = None
        self.inputs_count = None
        self.mode = None

    def __eq__(self, other):
        return vars(self) == vars(other)


def parse_result(line):
    name, sep, values = line.rpartition(':')
    name = name.strip()

    values = values.split()

    return (name, float(values[0]))


def clip(s, sep1, sep2):
    before, sep, after = s.partition(sep1)
    if sep == '':
        return None

    between, sep, after = after.partition(sep2)
    assert sep == sep2

    return between


if __name__ == '__main__':
    main()
