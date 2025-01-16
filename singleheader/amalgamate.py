#!/usr/bin/env python3
#
# Creates the amalgamated source files.
import sys
import os.path
import subprocess
import os
import re
import shutil
import datetime

if sys.version_info[0] < 3:
    sys.stdout.write("Sorry, requires Python 3.x or better\n")
    sys.exit(1)


SCRIPTPATH = os.path.dirname(os.path.abspath(sys.argv[0]))
PROJECTPATH = os.path.dirname(SCRIPTPATH)


def main():
    print(f"SCRIPTPATH={SCRIPTPATH} PROJECTPATH={PROJECTPATH}")
    print("We are about to amalgamate all simdutf files into one source file.")
    print("See https://www.sqlite.org/amalgamation.html and https://en.wikipedia.org/wiki/Single_Compilation_Unit for rationale.")

    timestamp = get_timestamp()
    print(f"timestamp is {timestamp}")

    create_files(timestamp)
    create_zip()

    print("Done with all files generation.")
    print(f"Files have been written to directory: {AMALGAMATE_OUTPUT_PATH}/")

    print_instructions()


if "AMALGAMATE_SOURCE_PATH" not in os.environ:
    AMALGAMATE_SOURCE_PATH = os.path.join(PROJECTPATH, "src")
else:
    AMALGAMATE_SOURCE_PATH = os.environ["AMALGAMATE_SOURCE_PATH"]
if "AMALGAMATE_INCLUDE_PATH" not in os.environ:
    AMALGAMATE_INCLUDE_PATH = os.path.join(PROJECTPATH, "include")
else:
    AMALGAMATE_INCLUDE_PATH = os.environ["AMALGAMATE_INCLUDE_PATH"]
if "AMALGAMATE_OUTPUT_PATH" not in os.environ:
    AMALGAMATE_OUTPUT_PATH = os.path.join(SCRIPTPATH)
else:
    AMALGAMATE_OUTPUT_PATH = os.environ["AMALGAMATE_OUTPUT_PATH"]


found_includes = set()

current_implementation = ''


def doinclude(fid, file, line):
    for directory in [AMALGAMATE_INCLUDE_PATH, AMALGAMATE_SOURCE_PATH]:
        path = os.path.join(directory, file)
        if os.path.exists(path):
            # generic includes are included multiple times
            if re.match('.*generic/.*.h', file):
                dofile(fid, directory, file)
            # begin/end_implementation are also included multiple times
            elif re.match('.*/begin.h', file):
                dofile(fid, directory, file)
            elif re.match('.*/end.h', file):
                dofile(fid, directory, file)
            elif file not in found_includes:
                found_includes.add(file)
                dofile(fid, directory, file)
            else:
                pass

            return

    # If we don't recognize it, just emit the #include
    print(line, file=fid)


def dofile(fid, prepath, filename):
    global current_implementation
    file = os.path.join(prepath, filename)
    RELFILE = os.path.relpath(file, PROJECTPATH)
    # Last lines are always ignored. Files should end by an empty lines.
    print(f"/* begin file {RELFILE} */", file=fid)
    includepattern = re.compile(r'\s*#\s*include "(.*)"')
    redefines_simdutf_implementation = re.compile(r'^#define\s+SIMDUTF_IMPLEMENTATION\s+(.*)')
    undefines_simdutf_implementation = re.compile(r'^#undef\s+SIMDUTF_IMPLEMENTATION\s*$')
    uses_simdutf_implementation = re.compile('SIMDUTF_IMPLEMENTATION([^_a-zA-Z0-9]|$)')
    for line in read_file(file):
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
            doinclude(fid, includedfile, line)
        else:
            # does it contain a redefinition of SIMDUTF_IMPLEMENTATION ?
            s = redefines_simdutf_implementation.search(line)
            if s:
                current_implementation = s.group(1)
                print(f"// redefining SIMDUTF_IMPLEMENTATION to \"{current_implementation}\"\n// {line}", file=fid)
            elif undefines_simdutf_implementation.search(line):
                # Don't include #undef SIMDUTF_IMPLEMENTATION since we're handling it ourselves
                pass
            else:
                # copy the line, with SIMDUTF_IMPLEMENTATION replace to what it is currently defined to
                print(uses_simdutf_implementation.sub(current_implementation+"\\1", line), file=fid)

    print(f"/* end file {RELFILE} */", file=fid)


def read_file(file):
    prev_empty = False
    with open(file, 'r') as f:
        for line in f:
            line = line.rstrip()
            if line or not prev_empty:
                yield line

            prev_empty = not line


def get_timestamp():
    # Get the generation date from git, so the output is reproducible.
    # The %ci specifier gives the unambiguous ISO 8601 format, and
    # does not change with locale and timezone at time of generation.
    # Forcing it to be UTC is difficult, because it needs to be portable
    # between gnu date and busybox date.
    try:
        ret = subprocess.run(['git', 'show', '-s', '--format=%ci', 'HEAD'],
                             stdout=subprocess.PIPE)

        if ret.returncode != 0:
            raise ValueError(f"non-zero exit code {ret.returncode}")

        return ret.stdout.decode('utf-8').strip()
    except (UnicodeDecodeError, FileNotFoundError):
        print("git not found, timestamp based on current time")
        return str(datetime.datetime.now())


def create_files(timestamp):
    os.makedirs(AMALGAMATE_OUTPUT_PATH, exist_ok=True)
    AMAL_H = os.path.join(AMALGAMATE_OUTPUT_PATH, "simdutf.h")
    AMAL_C = os.path.join(AMALGAMATE_OUTPUT_PATH, "simdutf.cpp")

    # this list excludes the "src/generic headers"
    ALLCFILES = ["simdutf.cpp"]

    # order matters
    ALLCHEADERS = ["simdutf.h"]

    print(f"Creating {AMAL_H}")
    with open(AMAL_H, 'w') as f:
        print(f"/* auto-generated on {timestamp}. Do not edit! */", file=f)
        for header in ALLCHEADERS:
            doinclude(f, header, f"ERROR {header} not found")

    print(f"Creating {AMAL_C}")
    with open(AMAL_C, 'w') as f:
        print(f"/* auto-generated on {timestamp}. Do not edit! */", file=f)
        for cpp in ALLCFILES:
            doinclude(f, cpp, f"ERROR {cpp} not found")

    # copy the README and DEMOCPP
    if SCRIPTPATH != AMALGAMATE_OUTPUT_PATH:
        for name in ["amalgamation_demo.cpp", "README.md"]:
            path = os.path.join(SCRIPTPATH, name)
            print(f"Creating {AMALGAMATE_OUTPUT_PATH}/{name}")
            shutil.copy2(path, AMALGAMATE_OUTPUT_PATH)


def create_zip(zipname='singleheader.zip'):
    import zipfile
    path = os.path.join(AMALGAMATE_OUTPUT_PATH, zipname)
    print(f"Creating {path}")
    with zipfile.ZipFile(path, 'w') as zf:
        for name in ["simdutf.cpp", "simdutf.h", "amalgamation_demo.cpp", "README.md"]:
            source = os.path.join(AMALGAMATE_OUTPUT_PATH, name)
            zf.write(source, name)


def print_instructions():
    README = os.path.join(AMALGAMATE_OUTPUT_PATH, "README.md")
    print()
    print("Giving final instructions:")
    print()
    sys.stdout.write(open(README).read())
    print()


if __name__ == '__main__':
    main()
