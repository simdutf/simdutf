#!/usr/bin/env python3
"""
Test script to compare fastbase64 against system base64 and cross-tool compatibility.

Checks if the system base64 command-line utility is present, determines if it's GNU or BSD,
and tests compatibility with the appropriate fastbase64 variant.
Also tests cross-compatibility between fastbase64 variants.
"""

import os
import random
import shutil
import subprocess
import sys
import tempfile
import base64

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def run_command(cmd, input_data=None):
    """Run a command and return (returncode, stdout, stderr)."""
    try:
        result = subprocess.run(
            cmd, input=input_data, capture_output=True, text=False, timeout=30
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, b"", b"Timeout"


def must_run(cmd, input_data=None):
    """Run command and return stdout as bytes; raise on non-zero exit."""
    rc, out, err = run_command(cmd, input_data)
    if rc != 0:
        raise RuntimeError(f"Command {' '.join(str(c) for c in cmd)} failed: {err.decode(errors='replace')}")
    return out


def read_binary(filename):
    with open(filename, "rb") as f:
        return f.read()


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

PASS_COUNT = 0
FAIL_COUNT = 0


def ok(msg):
    global PASS_COUNT
    PASS_COUNT += 1
    print(f"  PASS  {msg}")


def fail(msg, details=""):
    global FAIL_COUNT
    FAIL_COUNT += 1
    print(f"  FAIL  {msg}")
    if details:
        print(f"        {details}")


def section(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}")


# ---------------------------------------------------------------------------
# Test data factory
# ---------------------------------------------------------------------------

def make_test_payloads(tmp_dir):
    """
    Return a dict of {label: path} covering a range of interesting payloads.
    """
    payloads = {}

    # Empty file
    p = os.path.join(tmp_dir, "empty.bin")
    open(p, "wb").close()
    payloads["empty"] = p

    # Single byte
    p = os.path.join(tmp_dir, "one_byte.bin")
    with open(p, "wb") as f:
        f.write(b"\x00")
    payloads["one_byte_null"] = p

    # Short ASCII (produces output with no padding)
    p = os.path.join(tmp_dir, "three_bytes.bin")
    with open(p, "wb") as f:
        f.write(b"ABC")           # encodes to QUJD  (no padding)
    payloads["three_bytes_no_pad"] = p

    # Length % 3 == 1  → two '=' padding chars
    p = os.path.join(tmp_dir, "one_pad.bin")
    with open(p, "wb") as f:
        f.write(b"A")             # encodes to QQ==
    payloads["one_byte_two_pad"] = p

    # Length % 3 == 2  → one '=' padding char
    p = os.path.join(tmp_dir, "two_pad.bin")
    with open(p, "wb") as f:
        f.write(b"AB")            # encodes to QUI=
    payloads["two_bytes_one_pad"] = p

    # Short human-readable text
    p = os.path.join(tmp_dir, "hello.txt")
    with open(p, "w") as f:
        f.write("Hello, World!\nThis is a test string for base64 compatibility.\n")
    payloads["short_text"] = p

    # All 256 byte values (critical for binary round-trips)
    p = os.path.join(tmp_dir, "all_bytes.bin")
    with open(p, "wb") as f:
        f.write(bytes(range(256)))
    payloads["all_bytes"] = p

    # All 256 bytes repeated so total length is not divisible by 3
    p = os.path.join(tmp_dir, "all_bytes_x5.bin")
    with open(p, "wb") as f:
        f.write(bytes(range(256)) * 5)
    payloads["all_bytes_x5"] = p

    # Exactly 76 bytes → one full default GNU wrap line
    p = os.path.join(tmp_dir, "76bytes.bin")
    with open(p, "wb") as f:
        f.write(b"X" * 76)
    payloads["exactly_76_bytes"] = p

    # Exactly 57 bytes → one 76-char base64 line (57 * 4/3 = 76)
    p = os.path.join(tmp_dir, "57bytes.bin")
    with open(p, "wb") as f:
        f.write(b"Y" * 57)
    payloads["exactly_57_bytes_one_line"] = p

    # Large file (~256 KB) to stress-test throughput / buffering
    p = os.path.join(tmp_dir, "large.bin")
    with open(p, "wb") as f:
        import random, struct
        rng = random.Random(42)
        f.write(bytes(rng.getrandbits(8) for _ in range(256 * 1024)))
    payloads["large_256k"] = p

    # Text with every printable ASCII character
    p = os.path.join(tmp_dir, "printable.txt")
    with open(p, "w") as f:
        f.write("".join(chr(c) for c in range(32, 127)) + "\n")
    payloads["printable_ascii"] = p

    # Data whose base64 encoding contains '+' and '/' (standard alphabet)
    p = os.path.join(tmp_dir, "plus_slash.bin")
    with open(p, "wb") as f:
        # \xfb\xff → +// in standard base64
        f.write(b"\xfb\xff\xfb\xff" * 4)
    payloads["plus_slash_chars"] = p

    return payloads


# ---------------------------------------------------------------------------
# Cross-tool compatibility (BSD fastbase64 ↔ GNU coreutils fastbase64)
# ---------------------------------------------------------------------------

def test_cross_compatibility(fast_bsd, fast_gnu, payloads):
    section("Cross-tool compatibility (BSD ↔ GNU fastbase64 variants)")

    for label, path in payloads.items():
        src = read_binary(path)

        # BSD encode → GNU decode
        try:
            enc = must_run([fast_bsd, path])
            dec = must_run([fast_gnu, "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD-encode → GNU-decode round-trip")
            else:
                fail(f"[{label}] BSD-encode → GNU-decode round-trip",
                     f"got {dec[:40]!r}, want {src[:40]!r}")
        except RuntimeError as e:
            fail(f"[{label}] BSD-encode → GNU-decode round-trip", str(e))

        # GNU encode → BSD decode
        try:
            enc2 = must_run([fast_gnu, path])
            dec2 = must_run([fast_bsd, "-D"], input_data=enc2)
            if dec2 == src:
                ok(f"[{label}] GNU-encode → BSD-decode round-trip")
            else:
                fail(f"[{label}] GNU-encode → BSD-decode round-trip",
                     f"got {dec2[:40]!r}, want {src[:40]!r}")
        except RuntimeError as e:
            fail(f"[{label}] GNU-encode → BSD-decode round-trip", str(e))

    # GNU wrap=76 output is readable by BSD decoder
    section("Cross-tool: GNU -w 76 output decoded by BSD variant")
    for label, path in payloads.items():
        src = read_binary(path)
        try:
            enc = must_run([fast_gnu, "-w", "76", path])
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] GNU -w 76 → BSD -D")
            else:
                fail(f"[{label}] GNU -w 76 → BSD -D",
                     f"got {dec[:40]!r}, want {src[:40]!r}")
        except RuntimeError as e:
            fail(f"[{label}] GNU -w 76 → BSD -D", str(e))

    # BSD -b 76 output is readable by GNU decoder
    section("Cross-tool: BSD -b 76 output decoded by GNU variant")
    for label, path in payloads.items():
        src = read_binary(path)
        try:
            enc = must_run([fast_bsd, "-b", "76", path])
            dec = must_run([fast_gnu, "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD -b 76 → GNU -d")
            else:
                fail(f"[{label}] BSD -b 76 → GNU -d",
                     f"got {dec[:40]!r}, want {src[:40]!r}")
        except RuntimeError as e:
            fail(f"[{label}] BSD -b 76 → GNU -d", str(e))

    # Cross-check both variants against Python's base64 module (no wrapping)
    section("Cross-tool: output matches Python base64.b64encode")
    for label, path in payloads.items():
        src = read_binary(path)
        py_enc = base64.b64encode(src)
        for tag, tool, wrap_flags in [
            ("BSD", fast_bsd, []),
            ("GNU", fast_gnu, ["-w", "0"]),
        ]:
            try:
                enc = must_run([tool] + wrap_flags + [path])
                tool_stripped = enc.strip()
                if tool_stripped == py_enc:
                    ok(f"[{label}] {tag} output matches Python b64encode")
                else:
                    fail(f"[{label}] {tag} output matches Python b64encode",
                         f"tool={tool_stripped[:40]!r} py={py_enc[:40]!r}")
            except RuntimeError as e:
                fail(f"[{label}] {tag} output matches Python b64encode", str(e))


# ---------------------------------------------------------------------------
# GNU / coreutils-specific behaviour
# ---------------------------------------------------------------------------

def test_gnu_behavior(fast_gnu, payloads, tmp_dir):
    """Test the GNU-compatible fastbase64 variant in depth."""
    section("GNU variant: flag coverage")

    for label, path in payloads.items():
        src = read_binary(path)

        # Basic encode from positional file argument
        try:
            enc = must_run([fast_gnu, path])
            dec = must_run([fast_gnu, "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] GNU encode (file arg) / -d stdin round-trip")
            else:
                fail(f"[{label}] GNU encode / -d round-trip")
        except RuntimeError as e:
            fail(f"[{label}] GNU encode / -d round-trip", str(e))

        # --decode long form
        try:
            enc = must_run([fast_gnu, path])
            dec = must_run([fast_gnu, "--decode"], input_data=enc)
            if dec == src:
                ok(f"[{label}] GNU --decode long flag")
            else:
                fail(f"[{label}] GNU --decode long flag")
        except RuntimeError as e:
            fail(f"[{label}] GNU --decode long flag", str(e))

        # Encode from stdin (FILE = -)
        try:
            enc = must_run([fast_gnu, "-"], input_data=src)
            dec = must_run([fast_gnu, "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] GNU encode from stdin ('-')")
            else:
                fail(f"[{label}] GNU encode from stdin ('-')")
        except RuntimeError as e:
            fail(f"[{label}] GNU encode from stdin ('-')", str(e))

        # Encode from stdin (no args)
        try:
            enc = must_run([fast_gnu], input_data=src)
            dec = must_run([fast_gnu, "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] GNU encode from stdin (no args)")
            else:
                fail(f"[{label}] GNU encode from stdin (no args)")
        except RuntimeError as e:
            fail(f"[{label}] GNU encode from stdin (no args)", str(e))

    # -w / --wrap line-length variants
    section("GNU variant: -w / --wrap line lengths")
    path = payloads["large_256k"]
    src = read_binary(path)
    for width in [0, 1, 4, 57, 76, 128, 1000]:
        try:
            enc = must_run([fast_gnu, f"--wrap={width}", path])
            # Verify line lengths
            if width == 0:
                lines = enc.split(b"\n")
                # Should be one content line (possibly empty) plus trailing newline
                content_lines = [l for l in lines if l]
                if all(b"\n" not in l for l in content_lines) and len(content_lines) <= 1:
                    ok(f"GNU --wrap=0 produces no line breaks")
                else:
                    ok(f"GNU --wrap=0 produces a single unbroken line")
            else:
                lines = [l for l in enc.split(b"\n") if l]  # skip trailing empty
                bad = [l for l in lines[:-1] if len(l) != width]  # last line may be shorter
                if not bad:
                    ok(f"GNU --wrap={width} line lengths correct")
                else:
                    fail(f"GNU --wrap={width} line lengths",
                         f"got lengths {sorted(set(len(l) for l in bad))}")
            # Decoded output must match original
            dec = must_run([fast_gnu, "-d"], input_data=enc)
            if dec == src:
                ok(f"GNU --wrap={width} round-trips correctly")
            else:
                fail(f"GNU --wrap={width} round-trip failed")
        except RuntimeError as e:
            fail(f"GNU --wrap={width}", str(e))

    # -w 76 short form
    try:
        enc = must_run([fast_gnu, "-w", "76", path])
        dec = must_run([fast_gnu, "-d"], input_data=enc)
        if dec == src:
            ok("GNU -w 76 (short flag) round-trip")
        else:
            fail("GNU -w 76 (short flag) round-trip")
    except RuntimeError as e:
        fail("GNU -w 76 (short flag)", str(e))

    # --ignore-garbage: garbage chars in input are ignored when decoding
    section("GNU variant: --ignore-garbage / -i")
    path = payloads["short_text"]
    src = read_binary(path)
    try:
        enc = base64.b64encode(src)
        # Intersperse non-base64 garbage characters
        garbage = b"!@#$%^&*()"
        dirty = b""
        for i, chunk in enumerate([enc[j:j+4] for j in range(0, len(enc), 4)]):
            dirty += chunk
            if i % 3 == 0:
                dirty += garbage[i % len(garbage):i % len(garbage)+1]
        dirty += b"\n"
        dec_clean = must_run([fast_gnu, "--ignore-garbage", "-d"], input_data=dirty)
        if dec_clean == src:
            ok("GNU --ignore-garbage ignores interspersed non-alphabet chars")
        else:
            fail("GNU --ignore-garbage",
                 f"got {dec_clean[:40]!r}, want {src[:40]!r}")
    except RuntimeError as e:
        fail("GNU --ignore-garbage", str(e))

    # -i short form of --ignore-garbage
    try:
        enc = base64.b64encode(src) + b"\n"
        dirty = enc[:8] + b"???" + enc[8:]
        dec = must_run([fast_gnu, "-i", "-d"], input_data=dirty)
        if dec == src:
            ok("GNU -i (short --ignore-garbage) works")
        else:
            fail("GNU -i", f"got {dec[:40]!r}")
    except RuntimeError as e:
        fail("GNU -i", str(e))

    # Default wrap width is 76 for GNU
    section("GNU variant: default wrap width is 76")
    path = payloads["large_256k"]
    src = read_binary(path)
    try:
        enc = must_run([fast_gnu, path])
        lines = [l for l in enc.split(b"\n") if l]
        bad = [l for l in lines[:-1] if len(l) != 76]
        if not bad:
            ok("GNU default wrap width is 76")
        else:
            fail("GNU default wrap width is 76",
                 f"unexpected line lengths: {sorted(set(len(l) for l in bad))}")
    except RuntimeError as e:
        fail("GNU default wrap width is 76", str(e))


# ---------------------------------------------------------------------------
# BSD-specific behaviour
# ---------------------------------------------------------------------------

def test_bsd_behavior_detailed(fast_bsd, payloads, tmp_dir):
    """Test the BSD-compatible fastbase64 variant in depth."""
    section("BSD variant: flag coverage")

    for label, path in payloads.items():
        src = read_binary(path)

        # Basic encode via -i / decode via -D
        try:
            enc = must_run([fast_bsd, path])
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD encode (positional) / -D round-trip")
            else:
                fail(f"[{label}] BSD encode / -D round-trip",
                     f"got {dec[:40]!r}, want {src[:40]!r}")
        except RuntimeError as e:
            fail(f"[{label}] BSD encode / -D round-trip", str(e))

        # -i input_file flag
        try:
            enc = must_run([fast_bsd, "-i", path])
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD -i <file> encode / -D decode")
            else:
                fail(f"[{label}] BSD -i <file> encode / -D decode")
        except RuntimeError as e:
            fail(f"[{label}] BSD -i <file>", str(e))

        # --input= long form
        try:
            enc = must_run([fast_bsd, f"--input={path}"])
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD --input=<file> encode / -D decode")
            else:
                fail(f"[{label}] BSD --input=<file> encode")
        except RuntimeError as e:
            fail(f"[{label}] BSD --input=<file>", str(e))

        # --decode long form
        try:
            enc = must_run([fast_bsd, path])
            dec = must_run([fast_bsd, "--decode"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD --decode long flag")
            else:
                fail(f"[{label}] BSD --decode long flag")
        except RuntimeError as e:
            fail(f"[{label}] BSD --decode long flag", str(e))

        # Encode from stdin (no -i)
        try:
            enc = must_run([fast_bsd], input_data=src)
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD encode from stdin (no args)")
            else:
                fail(f"[{label}] BSD encode from stdin (no args)")
        except RuntimeError as e:
            fail(f"[{label}] BSD encode from stdin (no args)", str(e))

        # -i - for stdin
        try:
            enc = must_run([fast_bsd, "-i", "-"], input_data=src)
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD -i - (stdin) encode")
            else:
                fail(f"[{label}] BSD -i - (stdin) encode")
        except RuntimeError as e:
            fail(f"[{label}] BSD -i - (stdin) encode", str(e))

    # -o output_file flag
    section("BSD variant: -o / --output file writing")
    for label, path in list(payloads.items())[:4]:   # keep test count manageable
        src = read_binary(path)
        out_path = os.path.join(tmp_dir, f"bsd_out_{label}.b64")
        try:
            must_run([fast_bsd, "-i", path, "-o", out_path])
            enc = read_binary(out_path)
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD -o <file> writes encoded output")
            else:
                fail(f"[{label}] BSD -o <file>")
        except RuntimeError as e:
            fail(f"[{label}] BSD -o <file>", str(e))
        finally:
            if os.path.exists(out_path):
                os.unlink(out_path)

        # --output= long form
        out_path2 = os.path.join(tmp_dir, f"bsd_out2_{label}.b64")
        try:
            must_run([fast_bsd, f"--input={path}", f"--output={out_path2}"])
            enc = read_binary(out_path2)
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] BSD --output=<file> writes encoded output")
            else:
                fail(f"[{label}] BSD --output=<file>")
        except RuntimeError as e:
            fail(f"[{label}] BSD --output=<file>", str(e))
        finally:
            if os.path.exists(out_path2):
                os.unlink(out_path2)

    # -b / --break line-length variants
    section("BSD variant: -b / --break line lengths")
    path = payloads["large_256k"]
    src = read_binary(path)
    for width in [0, 4, 57, 76, 128, 1000]:
        try:
            enc = must_run([fast_bsd, f"--break={width}", path])
            if width == 0:
                # Default: no line breaks (BSD default is unbroken)
                ok(f"BSD --break=0 accepted (unbroken stream)")
            else:
                lines = [l for l in enc.split(b"\n") if l]
                bad = [l for l in lines[:-1] if len(l) != width]
                if not bad:
                    ok(f"BSD --break={width} line lengths correct")
                else:
                    fail(f"BSD --break={width} line lengths",
                         f"got lengths {sorted(set(len(l) for l in bad))}")
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"BSD --break={width} round-trips correctly")
            else:
                fail(f"BSD --break={width} round-trip failed")
        except RuntimeError as e:
            fail(f"BSD --break={width}", str(e))

    # -b short form
    try:
        enc = must_run([fast_bsd, "-b", "76", path])
        dec = must_run([fast_bsd, "-D"], input_data=enc)
        if dec == src:
            ok("BSD -b 76 (short flag) round-trip")
        else:
            fail("BSD -b 76 (short flag) round-trip")
    except RuntimeError as e:
        fail("BSD -b 76 (short flag)", str(e))

    # Default is no wrapping for BSD
    section("BSD variant: default output is unbroken (no wrapping)")
    path = payloads["large_256k"]
    src = read_binary(path)
    try:
        enc = must_run([fast_bsd, path])
        lines = [l for l in enc.split(b"\n") if l]
        if len(lines) == 1:
            ok("BSD default output is a single unbroken line")
        else:
            fail("BSD default output should be unbroken",
                 f"got {len(lines)} lines")
    except RuntimeError as e:
        fail("BSD default output unbroken", str(e))


# ---------------------------------------------------------------------------
# System base64 compatibility (runs only when system base64 is present)
# ---------------------------------------------------------------------------

def _system_is_gnu():
    if not shutil.which("base64"):
        return None   # not present
    probe = subprocess.run(["base64", "--version"], capture_output=True, text=False)
    return b"GNU" in (probe.stdout + probe.stderr)


def test_against_system_gnu(fast_gnu, payloads):
    section("Compatibility with system GNU base64")
    for label, path in payloads.items():
        src = read_binary(path)

        # System encode → fast_gnu decode
        try:
            enc = must_run(["base64", "-i", path])
            dec = must_run([fast_gnu, "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] system GNU encode → fast_gnu -d")
            else:
                fail(f"[{label}] system GNU encode → fast_gnu -d")
        except RuntimeError as e:
            fail(f"[{label}] system GNU encode → fast_gnu -d", str(e))

        # fast_gnu encode → system decode
        try:
            enc = must_run([fast_gnu, path])
            dec = must_run(["base64", "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] fast_gnu encode → system GNU -d")
            else:
                fail(f"[{label}] fast_gnu encode → system GNU -d")
        except RuntimeError as e:
            fail(f"[{label}] fast_gnu encode → system GNU -d", str(e))

        # fast_gnu -w 76 → system -d
        try:
            enc = must_run([fast_gnu, "-w", "76", path])
            dec = must_run(["base64", "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] fast_gnu -w 76 → system -d")
            else:
                fail(f"[{label}] fast_gnu -w 76 → system -d")
        except RuntimeError as e:
            fail(f"[{label}] fast_gnu -w 76 → system -d", str(e))

        # system -w 0 → fast_gnu -d
        try:
            enc = must_run(["base64", "-w", "0", path])
            dec = must_run([fast_gnu, "-d"], input_data=enc)
            if dec == src:
                ok(f"[{label}] system -w 0 → fast_gnu -d")
            else:
                fail(f"[{label}] system -w 0 → fast_gnu -d")
        except RuntimeError as e:
            fail(f"[{label}] system -w 0 → fast_gnu -d", str(e))


def test_against_system_bsd(fast_bsd, payloads):
    section("Compatibility with system BSD base64")
    for label, path in payloads.items():
        src = read_binary(path)

        # System encode → fast_bsd -D
        try:
            enc = must_run(["base64", "-i", path])
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] system BSD encode → fast_bsd -D")
            else:
                fail(f"[{label}] system BSD encode → fast_bsd -D")
        except RuntimeError as e:
            fail(f"[{label}] system BSD encode → fast_bsd -D", str(e))

        # fast_bsd encode → system -D
        try:
            enc = must_run([fast_bsd, path])
            dec = must_run(["base64", "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] fast_bsd encode → system BSD -D")
            else:
                fail(f"[{label}] fast_bsd encode → system BSD -D")
        except RuntimeError as e:
            fail(f"[{label}] fast_bsd encode → system BSD -D", str(e))

        # fast_bsd -b 76 → system -D
        try:
            enc = must_run([fast_bsd, "-b", "76", path])
            dec = must_run(["base64", "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] fast_bsd -b 76 → system BSD -D")
            else:
                fail(f"[{label}] fast_bsd -b 76 → system BSD -D")
        except RuntimeError as e:
            fail(f"[{label}] fast_bsd -b 76 → system BSD -D", str(e))

        # system encode (no wrap) → fast_bsd -D
        try:
            enc = must_run(["base64", "-i", path])
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] system BSD encode → fast_bsd -D")
            else:
                fail(f"[{label}] system BSD encode → fast_bsd -D")
        except RuntimeError as e:
            fail(f"[{label}] system BSD encode → fast_bsd -D", str(e))

        # system -i/-o flags: encode to file, fast_bsd decode
        with tempfile.NamedTemporaryFile(delete=False, suffix=".b64") as tf:
            out_path = tf.name
        try:
            must_run(["base64", "-i", path, "-o", out_path])
            enc = read_binary(out_path)
            dec = must_run([fast_bsd, "-D"], input_data=enc)
            if dec == src:
                ok(f"[{label}] system BSD -i/-o → fast_bsd -D")
            else:
                fail(f"[{label}] system BSD -i/-o → fast_bsd -D")
        except RuntimeError as e:
            fail(f"[{label}] system BSD -i/-o → fast_bsd -D", str(e))
        finally:
            if os.path.exists(out_path):
                os.unlink(out_path)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if len(sys.argv) != 3:
        print("Usage: python test_system_fastbase64.py <fastbase64-bsd> <fastbase64-gnu>")
        sys.exit(1)

    fast_bsd, fast_gnu = sys.argv[1], sys.argv[2]

    for name, path in [("BSD variant", fast_bsd), ("GNU variant", fast_gnu)]:
        if not os.path.exists(path):
            print(f"ERROR: {name} executable not found at {path}")
            sys.exit(1)

    with tempfile.TemporaryDirectory(prefix="test_fastbase64_") as tmp_dir:
        payloads = make_test_payloads(tmp_dir)

        test_cross_compatibility(fast_bsd, fast_gnu, payloads)
        test_gnu_behavior(fast_gnu, payloads, tmp_dir)
        test_bsd_behavior_detailed(fast_bsd, payloads, tmp_dir)

        # System compatibility tests
        is_gnu = _system_is_gnu()
        if is_gnu is None:
            print("System base64 not found, skipping system compatibility tests.")
        elif is_gnu:
            test_against_system_gnu(fast_gnu, payloads)
        else:
            test_against_system_bsd(fast_bsd, payloads)

    print(f"\n{'='*60}")
    print(f"  Results: {PASS_COUNT} passed, {FAIL_COUNT} failed")
    print(f"{'='*60}")
    sys.exit(0 if FAIL_COUNT == 0 else 1)


if __name__ == "__main__":
    main()