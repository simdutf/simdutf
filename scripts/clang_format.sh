#!/bin/sh
#
# clang formats all files in the repository.
# certain files are excluded via .clang-format-ignore files
# supported from clang format 18 upwards.

set -eu

cf=clang-format-18

gitroot="$(git rev-parse --show-toplevel)"

cd "$gitroot"

git ls-files -z | \
    grep -z -E '(\.cpp|\.h)$' |\
    xargs --null -P $(nproc) -n1 $cf -i
