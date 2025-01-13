#!/bin/sh
#
# clang formats all files in the repository, using
# a specific docker image.

set -eu

# Check if Docker is installed
if ! command -v docker 2>&1 >/dev/null
then
    echo "Error: Docker not found. Please install Docker to use this script."
    echo "You can get Docker from:"
    echo "  - Linux: https://docs.docker.com/engine/install/"
    echo "  - macOS: Use Homebrew with 'brew install --cask docker'"
    echo "  - Windows: Download from Docker Desktop website"
    exit 1
fi

# Check if Git is installed
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

docker run -v"$(pwd):/src" \
       --workdir /src \
       -u "$(id -u $USER):$(id -g $USER)" \
       ghcr.io/pauldreik/clang-format-18:latest \
       scripts/clang_format.sh
