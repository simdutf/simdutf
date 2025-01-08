# Fuzzing

All non deprecated functions part of the public API are fuzzed.

There have been quite a few bugs uncovered by this,
showing the effectiveness of this type of testing.

The fuzzers are doing differential fuzzing where applicable, meaning the output
of the different implementations are compared against each other. Deviations
are not tolerated, unless in some special cases. For instance, if the result of
a conversion is that the input was invalid, the (possibly partially) converted
output is allowed to differ.

Because simdutf does runtime cpu dispatching, you have to run the fuzzers at
different systems to ensure all code is exercised. For instance, the icelake
code is not run if you have an older x86 cpu.

simdutf is participating in the oss-fuzz project. The upstream instructions for
how to run, debug and develop fuzzers are applicable in addition to the
instructions here.

## Useful links
 - [OSS fuzz build logs](https://oss-fuzz-build-logs.storage.googleapis.com/index.html#simdutf)
 - [OSS fuzz bugs reported](https://bugs.chromium.org/p/oss-fuzz/issues/list?sort=-id&q=proj%3Dsimdutf)
 - [fuzz introspector (requires login)](https://storage.googleapis.com/oss-fuzz-introspector/simdutf/inspector-report/20240816/fuzz_report.html)
 - [libfuzzer documentation](https://llvm.org/docs/LibFuzzer.html)
## Building the fuzzers locally

Ensure you have clang installed.

```shell
cd fuzz
./build.sh
mkdir -p corpus/base64
out/base64 corpus/base64
```

## Developing fuzzers in an IDE
Using an IDE is nice because it gives code completion, debug support etc.
Make sure the SIMDUTF_FUZZ_FUZZERS is set to on. This should work regardless
of platform, regardless if libFuzzer is available. There will however be no
instrumentation, to get that CXXFLAGS have to be set. Easiest is to use the
build.sh script outside of the IDE and have two builds, one in the IDE to work
with and the one created by the build.sh script.

## Run files through a non-fuzz enabled build
This is useful for instance to test that a gcc build has no errors
on fuzz input.

Example using a sanitized build:
```shell
export CXX=/usr/lib/ccache/g++-14
cmake -B /tmp/build-gcc -DSIMDUTF_SANITIZE=On -DSIMDUTF_SANITIZE_UNDEFINED=On -DSIMDUTF_FUZZERS=On -S . -GNinja
cmake --build /tmp/build-gcc
/tmp/build-gcc/fuzz/conversion corpus/conversion
```

Or, running through valgrind:
```shell
export CXX=/usr/lib/ccache/g++-14
cmake -B /tmp/build-gcc-for-valgrind -DSIMDUTF_FUZZERS=On -S . -GNinja
cmake --build /tmp/build-gcc-for-valgrind
/tmp/build-gcc-for-valgrind/fuzz/conversion corpus/conversion
```

## Minimizing a crash
This is easiest shown with an example:
```shell
  ./build.sh
  mkdir -p corpus/conversion
  out/conversion corpus/conversion
  # ...crashes...
  # crash-XXXX is created where XXXX is a hash of the crashing input
  ./minimize_and_cleanse.sh out/conversion crash-*
  # you now find cleaned_crash.conversion in the current directory
  # see if it reproduces (it should)
  out/conversion cleaned_crash.conversion
```

Some of the fuzzers support printing out a reproducer, which makes it easy to
convert the fuzz finding into a unit test.

## Continuous fuzzing

There is a script in `random_fuzz.sh` which is useful to do ad-hoc fuzzing.
It selects a random compiler optimization level, sanitizer setup and runs the
fuzzers for a limited time. Then it pulls from git and repeats. This way, it is
easy to take a system and run some fuzzing. It is recommended to run it in tmux
or gnu screen, to avoid having it interrupt in case the network goes down.

The script has so far been tested on
 - debian bookworm (amd64/icelake, aarch64)
 - rocky linux 9 (amd64/icelake)
 - freebsd (aarch64)

We are happy to get some testing on architectures which are not fuzzed already.
If you have a power, riscv or lsx system, it would be great if you can run this
for a few days and report back.

Assuming clang and cmake are available, testing this should be as easy as:
```shell
git clone https://github.com/simdutf/simdutf
simdutf/fuzz/random_fuzz.sh
# let it run as long as you wish for
```

## Fuzzing using qemu

It is useful to use virtualization for fuzzing on architectures when it
is for some reason difficult or impractical to run on matching hardware.

The [qemu project](https://www.qemu.org/) is very capable and already used for
CI in this project.

### Fuzzing on emulated riscv64 with vector extensions

Debian is used, which provides a clang version with sanitizers enabled. There
is a [project providing prebuilt qemu images](https://people.debian.org/~gio/dqib/)
which are easy to use.

This shows how to get the fuzzers running on a debian system (tested on
Debian Trixie running amd64):

 - install qemu: `apt install qemu-system-riscv`
 - download the qemu image: `wget https://gitlab.com/api/v4/projects/giomasce%2Fdqib/jobs/artifacts/master/download?job=convert_riscv64-virt`
 - unpack it: `unzip "download?job=convert_riscv64-virt"`

Some adoption is needed for the provided readme.txt in the unpacked
folder:

 - set the number of cpus and memory
 - remove the `-bios` argument
 - specify vector extensions by adding to the `-cpu` flag

The following commandlines worked successfully (pick one of them):
```
# 128 bit simd
qemu-system-riscv64 -machine 'virt' -cpu 'rv64,v=on,vlen=128,rvv_ta_all_1s=on,rvv_ma_all_1s=on' -m 8G -smp 5 -device virtio-blk-device,drive=hd -drive file=image.qcow2,if=none,id=hd -device virtio-net-device,netdev=net -netdev user,id=net -kernel /usr/lib/u-boot/qemu-riscv64_smode/uboot.elf -object rng-random,filename=/dev/urandom,id=rng -device virtio-rng-device,rng=rng -nographic -append "root=LABEL=rootfs console=ttyS0"
# 256 bit simd
qemu-system-riscv64 -machine 'virt' -cpu 'rv64,v=on,zvbb=on,vlen=256,rvv_ta_all_1s=on,rvv_ma_all_1s=on' -m 8G -smp 5 -device virtio-blk-device,drive=hd -drive file=image.qcow2,if=none,id=hd -device virtio-net-device,netdev=net -netdev user,id=net -kernel /usr/lib/u-boot/qemu-riscv64_smode/uboot.elf -object rng-random,filename=/dev/urandom,id=rng -device virtio-rng-device,rng=rng -nographic -append "root=LABEL=rootfs console=ttyS0"
" 1024 bit simd
qemu-system-riscv64 -machine 'virt' -cpu 'rv64,v=on,zvbb=on,vlen=1024,rvv_ta_all_1s=on,rvv_ma_all_1s=on' -m 8G -smp 5 -device virtio-blk-device,drive=hd -drive file=image.qcow2,if=none,id=hd -device virtio-net-device,netdev=net -netdev user,id=net -kernel /usr/lib/u-boot/qemu-riscv64_smode/uboot.elf -object rng-random,filename=/dev/urandom,id=rng -device virtio-rng-device,rng=rng -nographic -append "root=LABEL=rootfs console=ttyS0"
```

Note: with a recent qemu (9.2.0 works fine), it is possible to append
`,rvv_vl_half_avl=on` to the back of the -cpu option and exercise the code
somewhat differently.

Once the machine has booted, login with root/root. Update the machine with `apt
update && apt dist-upgrade` and reboot it with ctrl-x ctrl-a, then login again.
Install necessary packages with `apt install cmake clang-19 wget unzip`

Inside the machine, follow the instructions in "Continuous fuzzing" above.

The speed of emulation is pretty good. The conversion fuzzer currently reaches
about 3300 executions/second per core on real hardware (Banana Pi BPI-F3) while
emulation with 256 bit SIMD gives roughly 2/3 of that speed.

### Fuzzing on emulated s390x

Use the same approach as described in the riscv64 section above, but use the 390x image instead. Boot with the following line instead of what is proposed in the readme.txt (tune memory and number of cores flags to your liking):

```
wget 'https://gitlab.com/api/v4/projects/giomasce%2Fdqib/jobs/artifacts/master/download?job=convert_s390x-virt'
unzip 'download?job=convert_s390x-virt'
cd dqib_s390x-virt
qemu-system-s390x -machine s390-ccw-virtio -cpu max,zpci=on -m 4G -smp 2 -drive file=image.qcow2 -device virtio-net-ccw,netdev=net -netdev user,id=net,hostfwd=tcp::2222-:22 -kernel kernel -initrd initrd -nographic -append root=LABEL=rootfs console=ttyAMA0
```
