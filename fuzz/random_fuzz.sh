#!/bin/sh
#
# this is a helper script that repeatedly:
# - pulls the latest changes from upstream using git pull (assumes you are on an appropriate branch)
# - selects random compiler optimization
# - selects random selection of sanitizers
# - selects a random clang compiler
# - builds the fuzzers
# - runs each fuzzer for a limited time, in random order
#
# it is intended to run in a screen/tmux session
# continuously. every once in a while, see if it has
# stopped (indicating compilation failure or a fuzzing finding)

set -eux

src=$(dirname "$0")/..
src=$(readlink -f "$src")

TMPWORKDIR=$(mktemp -d)

# store the corpus dir somewhere "permanent" and not inside the checked out
# git dir since that tends to get cleaned
corpusdir=/var/tmp/$(whoami)/simdutf/corpus/

selectrandomoptlevel() {
    echo "g 0 1 2 3 z s" |tr ' ' '\n' |sort --random-sort |head -n1
}

selectrandomclang() {
    # on a debian/ubuntu system with ccache installed,
    # looking for compilers in /usr/lib/ccache/ is the best place.
    #
    # on rocky linux, clang++ and clang++-$VER are in /usr/bin
    #
    # on the freebsd system I have access to, the compilers are in /usr/local/bin
    # and named clang++$VER (note the lack of dash before $VER!) as well
    # as /usr/bin/clang++ . There is also a clang++-devel in /usr/local/bin
    compilerlist=$TMPWORKDIR/clangcandidates
    echo clang++ >$compilerlist
    echo clang++-devel >>$compilerlist
    for ver in $(seq 15 40); do
	echo clang++-$ver >>$compilerlist
	echo clang++$ver >>$compilerlist
    done
    echo '#include <stddef.h> // for size_t (rather than std::size_t)
          #include <stdint.h> // for uint8_t (rather than std::uint8_t)
          extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size){return 0;}
          ' > $TMPWORKDIR/testprog.cpp
    for candidate in $(sort --random-sort $compilerlist); do
	# only include candidates which exist and support fuzzer and sanitizers
	if $candidate -std=c++20 -fsanitize=fuzzer-no-link,address,undefined -o $TMPWORKDIR/testprog.o -c $TMPWORKDIR/testprog.cpp  >/dev/null 2>&1 && \
		$candidate -fsanitize=fuzzer,address,undefined -o $TMPWORKDIR/testprog $TMPWORKDIR/testprog.o >/dev/null 2>&1; then
	    if [ -e /usr/lib/ccache/$candidate ]; then
		echo /usr/lib/ccache/$candidate
	    elif [ -e /usr/local/libexec/ccache/$candidate ]; then
                echo /usr/local/libexec/ccache/$candidate
	    else
		echo $candidate
	    fi
	    return
	fi
    done	
}

selectsanitizerflags() {
    echo \
	"-fsanitize=fuzzer-no-link,address,undefined -fsanitize-trap=undefined
-fsanitize=fuzzer-no-link,address
-fsanitize=fuzzer-no-link,undefined -fsanitize-trap=undefined
-fsanitize=fuzzer-no-link" | sort --random-sort |head -n1
}

# on bsd, put the fuzzing in low priority. on linux, use nice.
if which idprio >/dev/null 2>&1 ; then
    # idprio is by default a privileged operation, try it out to see if we can use it.
    if idprio 5 true >/dev/null 2>&1; then
	BENICE="idprio 5"
    else
	# nope. use nice instead.
	BENICE="nice -n19"
    fi
elif which nice >/dev/null 2>&1 ; then
    BENICE="nice -n19"
else
    BENICE=""
fi

# to get vector support on riscv64, we need to specify the arch
MARCHFLAGS=
case $(uname -m) in
    x86_64)
    # debian and rocky linux amd64
    ;;
    amd64)
    # freebsd amd64
    ;;
    aarch64)
    # debian arm64
    ;;
    arm64)
    # freebsd arm64
    ;;
    arm)
    # freebsd armv7
    ;;
    riscv64)
	MARCHFLAGS="-march=rv64gcv"
	;;
    *)
	;;
esac

while true ; do

    echo "pulling the latest changes"
    cd "$src"
    git pull
    
    export CXX=$(selectrandomclang)
    optlevel=$(selectrandomoptlevel)
    export CXXFLAGS="$(selectsanitizerflags) -g -O${optlevel} ${MARCHFLAGS}"
    export LIB_FUZZING_ENGINE="-fsanitize=fuzzer"
    export OUT=fuzz/out
    export WORK=fuzz/work
    mkdir -p $OUT $WORK
    
    rm -rf build/
    # we will run in oss-fuzz mode, so we can set CXXFLAGS from here.
    # do that by pointing out one directory above the repo root
    export SRC=$(pwd)/..
    $BENICE fuzz/build.sh

    fuzzers="base64 conversion misc roundtrip"
    for fuzzer in $(echo $fuzzers|tr ' ' '\n' |sort --random-sort); do
	if [ ! -d  "$corpusdir/$fuzzer" ] ; then
	    # populate with the backup corpus from oss-fuzz
	    mkdir -p $TMPWORKDIR/ossfuzzcorpus/$fuzzer
	    cd $TMPWORKDIR/ossfuzzcorpus/$fuzzer
	    # in case this fails, keep going.
	    if wget "https://storage.googleapis.com/simdutf-backup.clusterfuzz-external.appspot.com/corpus/libFuzzer/simdutf_${fuzzer}/public.zip" ;then
		mkdir -p "$corpusdir/$fuzzer"
		unzip -q public.zip -d "$corpusdir/$fuzzer"
		rm public.zip
	    fi
	    cd $src
	fi
	mkdir -p "$corpusdir/$fuzzer"
	$BENICE fuzz/out/$fuzzer -timeout=100 -max_total_time=3600 -jobs=$(nproc) -workers=$(nproc) "$corpusdir/$fuzzer"
    done
done
