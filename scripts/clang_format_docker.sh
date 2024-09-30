#!/bin/sh
#
# clang formats all files in the repository, using
# a specific docker image.

set -eu

gitroot="$(git rev-parse --show-toplevel)"

cd "$gitroot"

docker run -v"$(pwd):/src" \
       --workdir /src \
       -u "$(id -u $USER):$(id -g $USER)" \
       ghcr.io/pauldreik/clang-format-18:latest \
       scripts/clang_format.sh
