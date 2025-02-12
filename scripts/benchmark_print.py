#!/usr/bin/env python3

import sys
import warnings
from pathlib import Path
from table import Table


class Input:
    def __init__(self):
        self.procedure = None
        self.input_size = None
        self.iterations = None
        self.dataset = None

    @property
    def dataset_name(self):
        if self.dataset:
            return self.dataset.stem
        else:
            return 'none'

    def __str__(self):
        return '<Input: %s, size: %d, iterations: %s, dataset: %s>' % \
                (self.procedure, self.input_size, self.iterations, self.dataset)

    __repr__ = __str__


class Result:
    def __init__(self):
        self.instruction_per_byte = None
        self.instruction_per_char = None
        self.instruction_per_cycle = None
        self.cycle_per_byte = None
        self.cycle_per_char = None
        self.bytes_per_char = None
        self.speed_gigabytes = None
        self.speed_gigachars = None
        self.freq = None
        self.time = None

    def update_from_dict(self, D):
        while D:
            unit, value = D.popitem()
            field = unit2field[unit]
            if field is not None:
                setattr(self, field, value)


unit2field = {
    'ins/byte'      : 'instruction_per_byte',
    'ins/char'      : 'instruction_per_char',
    'ins/cycle'     : 'instruction_per_cycle',
    'cycle/byte'    : 'cycle_per_byte',
    'cycle/char'    : 'cycle_per_byte',
    'byte/char'     : 'byte_per_char',
    'GHz'           : 'freq',
    'GB/s'          : 'speed_gigabytes',
    'Gc/s'          : 'speed_gigachars',
    'ns'            : 'time',
    '%'             : None,
}


def parse(file):
    result = []
    for line in file:
        for item in parse_line(line):
            if isinstance(item, Input):
                result.append((item, Result()))
            else:
                assert isinstance(item, dict)
                result[-1][1].update_from_dict(item)

    return result


def parse_line(line):
    if 'input size' in line:
        yield parse_input(normalize_line(line))
    elif 'ins/byte' in line or 'ins/char' in line:
        yield parse_results(normalize_line(line))


def normalize_line(line):
    line = line.replace(',', ' ')
    line = line.replace(':', ' ')
    line = line.replace('(', ' ')
    line = line.replace(')', ' ')

    return line.split()


def parse_input(fields):
    input = Input()
    input.procedure = fields.pop(0)

    assert fields.pop(0) == 'input'
    assert fields.pop(0) == 'size'
    input.input_size = int(fields.pop(0))

    assert fields.pop(0) == 'iterations'
    input.iterations = int(fields.pop(0))

    try:
        assert fields.pop(0) == 'dataset'
        input.dataset = Path(fields.pop(0))
    except IndexError:
        pass

    return input


def try_float(s):
    try:
        return float(s)
    except ValueError:
        return


def parse_results(fields):
    """Consumes pairs "number unit" from the list"""
    D = {}

    while fields:
        value = try_float(fields.pop(0));
        if value is not None and fields:
            D[fields.pop(0)] = value

    return D


def print_speed_comparison(data):
    procedures = set()
    datasets = set()
    results = {}

    for input, result in data:
        procedures.add(input.procedure)
        datasets.add(input.dataset_name)
        results[(input.procedure, input.dataset_name)] = result

    datasets = list(sorted(datasets))
    procedures = list(sorted(procedures))

    def by_procedure():
        table = Table()
        table.set_header(['procedure'] + datasets)

        for procedure in procedures:
            row = []
            row.append(procedure)
            for dataset in datasets:
                try:
                    result = results[(procedure, dataset)]
                    row.append('%0.3f GB/s' % (result.speed_gbs))
                except KeyError:
                    row.append('--')

            table.add_row(row)

        return table

    def by_dataset():
        table = Table()
        table.set_header(['dataset'] + procedures)

        for dataset in datasets:
            row = []
            row.append(dataset)
            for procedure in procedures:
                try:
                    result = results[(procedure, dataset)]
                    row.append('%0.3f GB/s' % (result.speed_gigabytes))
                except KeyError:
                    row.append('--')

            table.add_row(row)

        return table

    if len(procedures) >= len(datasets):
        print(by_procedure())
    else:
        print(by_dataset())


def main():
    if len(sys.argv) < 2:
        print("No input files")
        print("Provide output from the benchmark utility")
        return

    for path in sys.argv[1:]:
        with open(path, 'rt') as f:
            data = parse(f)
            print_speed_comparison(data)


if __name__ == '__main__':
    main()
