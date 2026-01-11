#!/usr/bin/env python3

import subprocess
import glob
import pathlib
import shutil


def main():
    subprocess.run(['./scripts/prepare_doxygen.sh',])
    subprocess.run(['doxygen',])
    pathlib.Path('doc/api/html/doc').mkdir(parents=True, exist_ok=True)
    for file in glob.glob('doc/*png'):
        print(f"copying {file}")
        shutil.copy(file, 'doc/api/html/doc')


if __name__ == "__main__":
    main()
