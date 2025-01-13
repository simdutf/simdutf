#!/bin/sh
#
# clang formats all files in the repository.
# certain files are excluded via .clang-format-ignore files
# supported from clang format 18 upwards.

set -eu

cf=clang-format-18

# Check if clang-format is installed
if ! command -v $cf 2>&1 >/dev/null
then
    echo "Error: $cf not found. Please install Clang version 18."
    echo "We recommend using the clang_format_docker.sh script."
    exit 1
fi

if ! command -v git 2>&1 >/dev/null
then
    echo "Error: Git not found. Please install Git to use this script."
    echo "You can get Git from:"
    echo "  - Linux: Most distributions include Git; use your package manager (e.g., 'sudo apt-get install git')"
    echo "  - macOS: Use Homebrew with 'brew install git'"
    echo "  - Windows: Download from the official Git website"
    exit 1
fi

gitroot="$(git rev-parse --show-toplevel)"

cd "$gitroot"

git ls-files -z | \
    grep -z -E '(\.cpp|\.h)$' |\
    xargs --null -P $(nproc) -n1 $cf -i
