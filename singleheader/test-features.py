#!/usr/bin/env python3
from pathlib import Path
from io import StringIO
import os


TMP = Path("/dev/shm/")
if not TMP.exists():
    TMP = Path(os.environ.get('TMP', '.'))


def main():
    compilers = [
        ('default', 'c++', [])
    ]

    crosscompilers = find_crosscompilers()
    if crosscompilers:
        print("Found the following crosscompilers in $PATH:")
        for arch, compiler, opts in crosscompilers:
            print('%-16s: %s' % (arch, compiler))

        compilers.extend(crosscompilers)

    make = create_make(compilers)

    file = Path('Makefile')
    update_file(make, file)


def create_make(compilers):
    f = StringIO()

    def writeln(s):
        f.write(s + '\n')

    writeln("SRC=../src")
    writeln("INC=../include")

    archs = []
    for arch, compiler, opts in compilers:
        targets = []
        for features in feature_combinations:
            parts = [arch] + name_for_features(features)

            target_dir = TMP / '-'.join(parts)
            target_obj = target_dir / 'simdutf.o'
            target_exe = target_dir / 'a.out'

            targets.append((target_obj, target_exe, features))

        archs.append((arch, compiler, opts, targets))

        writeln('')
        writeln(f"{arch.upper()}=\\")
        for (target_obj, target_exe, _) in targets:
            writeln(f'\t{target_exe} {target_obj}\\')

    # first add 'all' target
    writeln('')
    writeln('.PHONY: all')
    writeln('all: %s' % (' '.join(arch for arch, _, _ in compilers)))
    writeln('')
    writeln('.PHONY: all')
    writeln('clean:')
    writeln('\t$(RM) %s' % ' '.join(f"$({arch.upper()})" for arch, _, _ in compilers))

    # add 'help' target
    writeln('')
    writeln(".PHONY: help")
    writeln("help:")
    width = max((len(arch) for arch, _, _ in compilers))
    width = max(width, len("all"), len("clear"))

    target = "all"
    writeln(f'\t@echo "make {target:{width}} --- build all targets"')
    target = "clear"
    writeln(f'\t@echo "make {target:{width}} --- remove all generated files"')
    target = "help"
    writeln(f'\t@echo "make {target:{width}} --- show this help"')
    writeln('\t@echo')
    for arch, compiler, _ in compilers:
        writeln(f'\t@echo "make {arch:{width}} --- build using {compiler}"')

    # add convienent targets for architectures ('default' is the first)
    for arch, _, _ in compilers:
        writeln('')
        writeln(f'.PHONY: {arch}')
        writeln(f'{arch}: $({arch.upper()})')

    # add individual targets
    for arch, compiler, opts, targets in archs:
        compiler_opts   = ' '.join(opts)
        for target_obj, target_exe, features in targets:
            amalgamate_opts = ' '.join(feature2option[feat] for feat in features)

            target_dir = target_obj.parent

            writeln('')
            writeln(f"{target_obj}: amalgamate.py")
            writeln(f"\tmkdir -p {target_dir}")
            writeln(f"\tpython3 amalgamate.py --no-zip --no-readme --source-dir=$(SRC) --include-dir=$(INC) --output {target_dir} {amalgamate_opts}")
            writeln(f"\tcd {target_dir} && {compiler} {compiler_opts} -c simdutf.cpp")
            writeln('')
            writeln(f"{target_exe}: {target_obj}")
            writeln(f"\tcd {target_dir} && {compiler} {compiler_opts} amalgamation_demo.cpp")

    return f.getvalue()


def update_file(contents, path):
    if path.exists():
        if path.read_text() == contents:
            return

        print(f"updating {path}")
    else:
        print(f"creating {path}")

    path.write_text(contents)


def find_crosscompilers():
    return list(find_crosscompilers_aux())


def find_crosscompilers_aux():
    found = set()
    for path in os.environ['PATH'].split(':'):
        path = Path(path)
        gxx = glob_many(path, ['*-g++*', '*-c++*', '*clang++'])
        for item in crosscompilers:
            if isinstance(item, str):
                arch = item
                name = arch
                opts = []
            elif isinstance(item, tuple):
                assert len(item) == 3, item
                arch, name, opts = item
            else:
                assert False, item

            if name in found:
                continue

            for filename in gxx:
                if is_compiler(arch, ['g++', 'c++', 'clang++'], filename):
                    yield (name, filename, opts)
                    found.add(name)
                    break


def glob_many(rootdir, patterns):
    tmp = []
    for pat in patterns:
        tmp.extend([file.name for file in rootdir.glob(pat)])

    tmp.sort()
    return tmp


def is_compiler(arch, compilers, name):
    # we're looking for "arch-foo-bar-g++" or "arch-foo-bar-g++-version"
    tmp = name.split('-')
    if tmp[0] != arch:
        return False

    if tmp[-1] in compilers:
        return True

    if len(tmp) >= 3 and tmp[-2] in compilers and is_number(tmp[-1]):
        return True


def is_number(s):
    try:
        _ = int(s)
        return True
    except ValueError:
        return False


def name_for_features(features):
    return [feature2stem[feat] for feat in features]


SIMDUTF_FEATURE_DETECT_ENCODING = 'SIMDUTF_FEATURE_DETECT_ENCODING'
SIMDUTF_FEATURE_LATIN1          = 'SIMDUTF_FEATURE_LATIN1'
SIMDUTF_FEATURE_ASCII           = 'SIMDUTF_FEATURE_ASCII'
SIMDUTF_FEATURE_BASE64          = 'SIMDUTF_FEATURE_BASE64'
SIMDUTF_FEATURE_UTF8            = 'SIMDUTF_FEATURE_UTF8'
SIMDUTF_FEATURE_UTF16           = 'SIMDUTF_FEATURE_UTF16'
SIMDUTF_FEATURE_UTF32           = 'SIMDUTF_FEATURE_UTF32'


feature2stem = {
    SIMDUTF_FEATURE_DETECT_ENCODING : 'de',
    SIMDUTF_FEATURE_LATIN1          : 'lat1',
    SIMDUTF_FEATURE_ASCII           : 'ascii',
    SIMDUTF_FEATURE_BASE64          : 'base64',
    SIMDUTF_FEATURE_UTF8            : 'utf8',
    SIMDUTF_FEATURE_UTF16           : 'utf16',
    SIMDUTF_FEATURE_UTF32           : 'utf32',
}

feature2option = {
    SIMDUTF_FEATURE_DETECT_ENCODING : '--with-detect-enc',
    SIMDUTF_FEATURE_LATIN1          : '--with-latin1',
    SIMDUTF_FEATURE_ASCII           : '--with-ascii',
    SIMDUTF_FEATURE_BASE64          : '--with-base64',
    SIMDUTF_FEATURE_UTF8            : '--with-utf8',
    SIMDUTF_FEATURE_UTF16           : '--with-utf16',
    SIMDUTF_FEATURE_UTF32           : '--with-utf32',
}

feature_combinations = [
    [SIMDUTF_FEATURE_DETECT_ENCODING],
    [SIMDUTF_FEATURE_ASCII],
    [SIMDUTF_FEATURE_UTF8],
    [SIMDUTF_FEATURE_UTF16],
    [SIMDUTF_FEATURE_UTF32],
    [SIMDUTF_FEATURE_BASE64],
    [SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_LATIN1],
    [SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_LATIN1],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_LATIN1],
    [SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_LATIN1, SIMDUTF_FEATURE_DETECT_ENCODING],
    [SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_LATIN1, SIMDUTF_FEATURE_DETECT_ENCODING],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_LATIN1, SIMDUTF_FEATURE_DETECT_ENCODING],
    [SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_LATIN1, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII],
    [SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_LATIN1, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_LATIN1, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII],
    [SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_UTF16],
    [SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_UTF32],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_UTF8],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_UTF16],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_UTF8],
    [SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_LATIN1],
    [SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_LATIN1],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_LATIN1],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_LATIN1],
    [SIMDUTF_FEATURE_UTF32, SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_LATIN1],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII, SIMDUTF_FEATURE_UTF8],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII, SIMDUTF_FEATURE_UTF16],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII, SIMDUTF_FEATURE_UTF32],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII, SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_UTF16],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII, SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_UTF32],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII, SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_UTF32],
    [SIMDUTF_FEATURE_BASE64, SIMDUTF_FEATURE_DETECT_ENCODING, SIMDUTF_FEATURE_ASCII, SIMDUTF_FEATURE_UTF8, SIMDUTF_FEATURE_UTF16, SIMDUTF_FEATURE_UTF32],
]

crosscompilers = [
    'aarch64',
    'powerpc64',
    'loongarch64',
    ('loongarch64', 'loongarch64lasx', ["-mlsx", "-mlasx"]),
    ('riscv64', 'riscv64', ["-march=rv64gv"]),
]

if __name__ == '__main__':
    main()
