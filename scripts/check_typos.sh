#!/bin/sh

#
# checks for typos. run from the root dir.

set -eu

# this exits with nonzero status if it finds something, which terminates the script
codespell \
    --skip="./benchmarks/competition,./build,./fuzz/work"  \
    -L vie,persan,fo,ans,larg,indx,shft,carryin,statics,caf

echo "no typos detected!"
