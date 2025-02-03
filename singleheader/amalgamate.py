#!/usr/bin/env python3
#
# Creates the amalgamated source files.

import argparse
import sys
import os.path
import subprocess
import os
import pathlib
import re
import shutil
import datetime

if sys.version_info[0] < 3:
    sys.stdout.write("Sorry, requires Python 3.x or better\n")
    sys.exit(1)


SCRIPTPATH = os.path.dirname(os.path.abspath(sys.argv[0]))
PROJECTPATH = os.path.dirname(SCRIPTPATH)


class Context:
    pass


context = Context()


def main():
    (args, enabled_features) = parse_args()

    print(f"SCRIPTPATH={SCRIPTPATH} PROJECTPATH={PROJECTPATH}")
    print("We are about to amalgamate all simdutf files into one source file.")
    print("See https://www.sqlite.org/amalgamation.html and https://en.wikipedia.org/wiki/Single_Compilation_Unit for rationale.")

    context.args             = args
    context.enabled_features = enabled_features
    context.zipname          = 'singleheader.zip'
    context.timestamp        = get_timestamp()
    if enabled_features != known_features:
        context.read_file = filter_features
    else:
        context.read_file = read_file

    print(f"timestamp is {context.timestamp}")

    create_files()
    if args.zip:
        create_zip()

    print("Done with all files generation.")
    print(f"Files have been written to directory: {context.args.output_dir}")

    if args.readme:
        print_instructions()


known_features = {
    'SIMDUTF_FEATURE_DETECT_ENCODING',
    'SIMDUTF_FEATURE_LATIN1',
    'SIMDUTF_FEATURE_ASCII',
    'SIMDUTF_FEATURE_BASE64',
    'SIMDUTF_FEATURE_UTF8',
    'SIMDUTF_FEATURE_UTF16',
    'SIMDUTF_FEATURE_UTF32',
}


def parse_args():
    p = argparse.ArgumentParser("SIMDUTF tool for amalgmation")
    p.add_argument("--source-dir",
                   default=os.path.join(PROJECTPATH, "src"),
                   metavar="SRC",
                   help="Source dir")
    p.add_argument("--include-dir",
                   default=os.path.join(PROJECTPATH, "include"),
                   metavar="INC",
                   help="Include dir")
    p.add_argument("--output-dir",
                   default=os.path.join(SCRIPTPATH),
                   metavar="DIR",
                   help="Output directory")
    p.add_argument("--no-zip",
                   default=True,
                   action='store_false',
                   dest='zip',
                   help="Do not create .zip file")
    p.add_argument("--no-readme",
                   default=True,
                   action='store_false',
                   dest='readme',
                   help="Do not show readme after creating files")
    p.add_argument("--with-utf8",
                   default=None,
                   action='store_true',
                   help="Include UTF-8 support")
    p.add_argument("--with-utf16",
                   default=None,
                   action='store_true',
                   help="Include UTF-16 support")
    p.add_argument("--with-utf32",
                   default=None,
                   action='store_true',
                   help="Include UTF-32 support")
    p.add_argument("--with-base64",
                   default=None,
                   action='store_true',
                   help="Include Base64 support")
    p.add_argument("--with-detect-enc",
                   default=None,
                   action='store_true',
                   help="Include encoding detection support")
    p.add_argument("--with-ascii",
                   default=None,
                   action='store_true',
                   help="Include ASCII support")
    p.add_argument("--with-latin1",
                   default=None,
                   action='store_true',
                   help="Include Latin1 support")
    p.add_argument("--debug-sources",
                   default=False,
                   action='store_true',
                   help="Include exact source location in amalgamated sources")

    args = p.parse_args()

    items = (
        ("AMALGAMATE_SOURCE_PATH", "source_dir"),
        ("AMALGAMATE_INCLUDE_PATH", "include_dir"),
        ("AMALGAMATE_OUTPUT_PATH", "output_dir"),
    )
    for var, attribute in items:
        if var in os.environ:
            val = os.environ[var]
            print(f"using env variable {var}={val}")
            setattr(args, attribute, val)

    enabled_features = set()
    if args.with_utf8:
        enabled_features.add('SIMDUTF_FEATURE_UTF8')
    if args.with_utf16:
        enabled_features.add('SIMDUTF_FEATURE_UTF16')
    if args.with_utf32:
        enabled_features.add('SIMDUTF_FEATURE_UTF32')
    if args.with_base64:
        enabled_features.add('SIMDUTF_FEATURE_BASE64')
    if args.with_detect_enc:
        enabled_features.add('SIMDUTF_FEATURE_DETECT_ENCODING')
    if args.with_ascii:
        enabled_features.add('SIMDUTF_FEATURE_ASCII')
    if args.with_latin1:
        enabled_features.add('SIMDUTF_FEATURE_LATIN1')

    if not enabled_features:
        enabled_features = set(known_features)

    return (args, enabled_features)


def doinclude(file, line):
    for directory in [context.args.include_dir, context.args.source_dir]:
        path = os.path.join(directory, file)
        if os.path.exists(path):
            # generic includes are included multiple times
            if re.match('.*generic/.*.h', file):
                dofile(directory, file)
            # begin/end_implementation are also included multiple times
            elif re.match('.*/begin.h', file):
                dofile(directory, file)
            elif re.match('.*/end.h', file):
                dofile(directory, file)
            elif file not in context.found_includes:
                context.found_includes.add(file)
                dofile(directory, file)
            else:
                pass

            return

    # If we don't recognize it, just emit the #include
    print(line, file=context.fid)


def dofile(prepath, filename):
    file = os.path.join(prepath, filename)
    fid  = context.fid
    RELFILE = os.path.relpath(file, PROJECTPATH)
    # Last lines are always ignored. Files should end by an empty lines.
    print(f"/* begin file {RELFILE} */", file=fid)
    includepattern = re.compile(r'^\s*#\s*include "(.*)"')
    redefines_simdutf_implementation = re.compile(r'^#define\s+SIMDUTF_IMPLEMENTATION\s+(.*)')
    undefines_simdutf_implementation = re.compile(r'^#undef\s+SIMDUTF_IMPLEMENTATION\s*$')
    uses_simdutf_implementation = re.compile('SIMDUTF_IMPLEMENTATION([^_a-zA-Z0-9]|$)')
    for line in context.read_file(file):
        s = includepattern.search(line)
        if s:
            includedfile = s.group(1)
            # include all from simdutf.cpp except simdutf.h
            if includedfile == "simdutf.h" and filename == "simdutf.cpp":
                print(line, file=fid)
                continue

            if includedfile.startswith('../'):
                includedfile = includedfile[2:]
            # we explicitly include simdutf headers, one time each (unless they are generic, in which case multiple times is fine)
            doinclude(includedfile, line)
        else:
            # does it contain a redefinition of SIMDUTF_IMPLEMENTATION ?
            s = redefines_simdutf_implementation.search(line)
            if s:
                context.current_implementation = s.group(1)
                print(f"// redefining SIMDUTF_IMPLEMENTATION to \"{context.current_implementation}\"\n// {line}", file=fid)
            elif undefines_simdutf_implementation.search(line):
                # Don't include #undef SIMDUTF_IMPLEMENTATION since we're handling it ourselves
                pass
            else:
                # copy the line, with SIMDUTF_IMPLEMENTATION replace to what it is currently defined to
                print(uses_simdutf_implementation.sub(context.current_implementation+"\\1", line), file=fid)

    print(f"/* end file {RELFILE} */", file=fid)


def get_timestamp():
    # Get the generation date from git, so the output is reproducible.
    # The %ci specifier gives the unambiguous ISO 8601 format, and
    # does not change with locale and timezone at time of generation.
    # Forcing it to be UTC is difficult, because it needs to be portable
    # between gnu date and busybox date.
    try:
        # avoid git going outside simdutf, which could happen when
        # unpacking a release tarball inside a subdirectory of an unrelated
        # git repository. that would lead to picking up the timestamp of the
        # unrelated git repository.
        simdroot = pathlib.Path(SCRIPTPATH).absolute().parent
        GIT_CEILING_DIRECTORIES = str(simdroot.parent)
        ret = subprocess.run(['git', '-C', SCRIPTPATH, 'show', '-s', '--format=%ci', 'HEAD'],
                             stdout=subprocess.PIPE,
                             env=dict(os.environ, GIT_CEILING_DIRECTORIES=GIT_CEILING_DIRECTORIES))

        if ret.returncode != 0:
            print(f"git called resulted in non-zero exit code {ret.returncode}")
            print("timestamp based on current time")
            return str(datetime.datetime.now())

        return ret.stdout.decode('utf-8').strip()
    except (UnicodeDecodeError, FileNotFoundError):
        print("UnicodeDecodeError or FileNotFoundError, timestamp based on current time")
        return str(datetime.datetime.now())


def create_files():
    outdir    = context.args.output_dir
    timestamp = context.timestamp

    os.makedirs(outdir, exist_ok=True)
    AMAL_H = os.path.join(outdir, "simdutf.h")
    AMAL_C = os.path.join(outdir, "simdutf.cpp")

    context.found_includes = set()
    context.current_implementation = ''

    # this list excludes the "src/generic headers"
    ALLCFILES = ["simdutf.cpp"]

    # order matters
    ALLCHEADERS = ["simdutf.h"]

    print(f"Creating {AMAL_H}")
    with open(AMAL_H, 'w') as f:
        context.fid = f
        print(f"/* auto-generated on {timestamp}. Do not edit! */", file=f)
        for header in ALLCHEADERS:
            doinclude(header, f"ERROR {header} not found")

    print(f"Creating {AMAL_C}")
    with open(AMAL_C, 'w') as f:
        context.fid = f
        print(f"/* auto-generated on {timestamp}. Do not edit! */", file=f)
        for cpp in ALLCFILES:
            doinclude(cpp, f"ERROR {cpp} not found")

    # copy the README and DEMOCPP
    if SCRIPTPATH != outdir:
        for name in ["amalgamation_demo.cpp", "README.md"]:
            path = os.path.join(SCRIPTPATH, name)
            print(f"Creating {outdir}/{name}")
            shutil.copy2(path, outdir)


def create_zip():
    import zipfile
    outdir = context.args.output_dir

    path = os.path.join(outdir, context.zipname)
    print(f"Creating {path}")
    with zipfile.ZipFile(path, 'w') as zf:
        for name in ["simdutf.cpp", "simdutf.h", "amalgamation_demo.cpp", "README.md"]:
            source = os.path.join(outdir, name)
            zf.write(source, name)


def print_instructions():
    README = os.path.join(context.args.output_dir, "README.md")
    print()
    print("Giving final instructions:")
    print()
    sys.stdout.write(open(README).read())
    print()


def read_file(file):
    with open(file, 'r') as f:
        for line in f:
            yield line.rstrip()


def filter_features(file):
    """
    Design:

    * Feature macros SIMDUTF_FEATURE_foo must not be nested.
    * All #endifs must contain a comment with the repeated condition.
    """
    current_features = None
    start_if_line = None
    enabled = True
    prev_line = ''

    root_header = file.endswith("/implementation.h")

    with open(file, 'r') as f:
        lines = [line.rstrip() for line in f.readlines()]
        for (lineno, line) in enumerate(lines, 1):
            if line is None:
                continue

            if root_header and line.startswith('#define SIMDUTF_FEATURE'):
                # '#define SIMDUTF_FEATURE_FOO 1'
                tmp = line.split()
                assert len(tmp) == 3, line
                assert tmp[2] == '1'
                flag = tmp[1]

                if flag in context.enabled_features:
                    yield line
                else:
                    yield f'#define {flag} 0'

            elif line.startswith('#if SIMDUTF_FEATURE'):
                if start_if_line is not None:
                    raise ValueError(f"{file}:{lineno}: feature block already opened at line {start_if_line}")

                prefix_len = len('#if ')
                if line.endswith('\\'):
                    nextline = lines[lineno]
                    lines[lineno] = None

                    expr = line[prefix_len:-1] + nextline
                else:
                    expr = line[prefix_len:]

                current_features = get_features(file, lineno, expr)
                start_if_line = lineno
                enabled = current_features.evaluate(context.enabled_features)
            elif line.startswith('#endif // SIMDUTF_FEATURE'):
                if start_if_line is None:
                    raise ValueError(f"{file}:{lineno}: feature block not opened, orphan #endif found")

                prefix_len = len('#endif // ')
                expr = line[prefix_len:]
                if lineno < len(lines):
                    nextline = lines[lineno]
                    if nextline.startswith('       // '):
                        expr += nextline[prefix_len:]
                        lines[lineno] = None

                features = get_features(line, lineno, expr)
                if str(features) != str(current_features):
                    raise ValueError(f"{file}:{lineno}: feature #endif condition different than opening #if at {start_if_line}")

                enabled = True
                start_if_line = None
                current_features = None
            elif enabled:
                if not prev_line.endswith('\\'):
                    yield f"// {file}:{lineno}"

                if line or (not line and prev_line):
                    yield line

                prev_line = line


def get_features(file, lineno, line):
    try:
        return parse_condition(line)
    except Exception as e:
        raise ValueError(f"{file}:{lineno}: {e}")


class Token:
    def __init__(self, name):
        if name not in known_features:
            raise ValueError(f"unknown feature name '{name}'")

        self.name = name

    def evaluate(self, enabled_features):
        return self.name in enabled_features

    def __str__(self):
        return self.name


class And:
    def __init__(self, a, b):
        self.a = a
        self.b = b

    def evaluate(self, enabled_features):
        a = self.a.evaluate(enabled_features)
        b = self.b.evaluate(enabled_features)

        return a and b

    def __str__(self):
        return '(%s && %s)' % (self.a, self.b)


class Or:
    def __init__(self, a, b):
        self.a = a
        self.b = b

    def evaluate(self, enabled_features):
        a = self.a.evaluate(enabled_features)
        b = self.b.evaluate(enabled_features)

        return a or b

    def __str__(self):
        return '(%s || %s)' % (self.a, self.b)


def parse_condition(s):
    tokens = [t for t in re.split('( |\\(|\\)|&&|\\|\\|)', s) if t not in ('', ' ')]
    stack  = []

    expr = None
    for token in tokens:
        if token == '&&':
            stack.append(token)
        elif token == '||':
            stack.append(token)
        elif token == '(':
            stack.append(expr)
            expr = None
        elif token == ')':
            prev = stack.pop()
            op   = stack.pop()
            if op == '&&':
                expr = And(prev, expr)
            elif op == '||':
                expr = Or(prev, expr)
            else:
                assert False, op
        else:
            e = Token(token)
            if expr is None:
                expr = e
            else:
                assert stack, "empty expression stack"
                op = stack.pop()
                if op == '&&':
                    expr = And(expr, e)
                elif op == '||':
                    expr = Or(expr, e)
                else:
                    assert False, op

    return expr


if __name__ == '__main__':
    main()
