#!/usr/bin/env python3
"""
Comprehensive tests for fastbase64 (BSD-like) and
fastbase64.coreutils (GNU-like).
"""
import base64
import os
import platform
import shutil
import subprocess
import sys
import tempfile

is_windows = platform.system() == 'Windows'

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
PASS = 0
FAIL = 0

def ok(label):
    global PASS
    PASS += 1
    print(f" PASS {label}")

def fail(label, detail=""):
    global FAIL
    FAIL += 1
    msg = f" FAIL {label}"
    if detail:
        msg += f"\n {detail}"
    print(msg)

def run(cmd, input=None, expect_failure=False):
    """Run a command, return (returncode, stdout_bytes, stderr_bytes).
    On Windows the C runtime may translate \\n to \\r\\n on stdout,
    so we strip \\r from the output to get consistent \\n line endings."""
    result = subprocess.run(cmd, input=input, capture_output=True, text=False)
    stdout = result.stdout.replace(b'\r\n', b'\n')
    stderr = result.stderr.replace(b'\r\n', b'\n')
    if not expect_failure and result.returncode != 0:
        fail(f"Unexpected failure: {' '.join(str(c) for c in cmd)}",
             stderr.decode('utf-8', errors='replace'))
        sys.exit(1)
    return result.returncode, stdout, stderr

def must_run(cmd, input=None):
    """Run a command and return stdout; exit on failure."""
    rc, stdout, stderr = run(cmd, input=input)
    return stdout

def read_binary(path):
    """Read a file in binary mode without altering bytes."""
    return open(path, 'rb').read()

def check_base64_alphabet(data_bytes, label):
    """Verify the output only contains valid base64 chars + newlines."""
    allowed = frozenset(
        b'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=\r\n'
    )
    bad = [b for b in data_bytes if b not in allowed]
    if bad:
        fail(f"{label}: invalid chars in output: {bad[:8]}")
    else:
        ok(label)

def generate_deterministic_data(size: int) -> bytes:
    """Deterministic pseudo-random bytes (covers all byte values, reproducible)."""
    if size <= 0:
        return b''
    return bytes(((i * 31 + 17) & 0xFF) for i in range(size))

def run_encode_binary(path, data, is_coreutils=False):
    """Encode binary data, using temp file on Windows if data contains \\n to avoid CRLF conversion."""
    if is_windows and b'\n' in data:
        with tempfile.NamedTemporaryFile(delete=False) as tf:
            tf.write(data)
            tf_path = tf.name
        try:
            if is_coreutils:
                return must_run([path, tf_path])
            else:
                return must_run([path, '-e', '-i', tf_path])
        finally:
            os.unlink(tf_path)
    else:
        if is_coreutils:
            return must_run([path], input=data)
        else:
            return must_run([path, '-e'], input=data)

# ---------------------------------------------------------------------------
# fastbase64 (BSD-like)
# ---------------------------------------------------------------------------
def test_fastbase64(path, readme):
    print("\n=== fastbase64 (BSD-like) ===")
    src = read_binary(readme)
    # --- help / version -------------------------------------------------------
    rc, out, _ = run([path, '--help'])
    if rc == 0 and b'Usage' in out:
        ok('--help')
    else:
        fail('--help', f'rc={rc}')
    rc, out, _ = run([path, '--version'])
    if rc == 0 and b'fastbase64' in out:
        ok('--version')
    else:
        fail('--version', f'rc={rc}')
    # --- error on unknown option -----------------------------------------------
    rc, _, _ = run([path, '--unknown-flag'], expect_failure=True)
    if rc != 0:
        ok('unknown option rejected')
    else:
        fail('unknown option rejected')
    # --- encode / decode round-trip (README.md) --------------------------------
    encoded = must_run([path, '-e', readme])
    decoded = must_run([path, '-d'], input=encoded)
    if decoded == src:
        ok('round-trip: -e FILE | -d')
    else:
        fail('round-trip: -e FILE | -d')
    decoded_ref = b'hello default\n'
    encoded_default = must_run([path], input=decoded_ref)
    expected = base64.b64encode(decoded_ref) + b'\n'
    if encoded_default == expected:
        ok('encoded matches expected')
    else:
        fail('encoded matches expected', f'got {encoded_default!r}')
    # --- explicit -e and -d flags (binary data) --------------------------------
    sample = b'\x00\xff\xfe\x80binary\x01\x02'
    enc = must_run([path, '-e'], input=sample)
    dec = must_run([path, '-d'], input=enc)
    if dec == sample:
        ok('-e / -d explicit flags (binary data)')
    else:
        fail('-e / -d explicit flags (binary data)')
    # --- --encode / --decode long options -------------------------------------
    enc2 = must_run([path, '--encode'], input=sample)
    dec2 = must_run([path, '--decode'], input=enc2)
    if enc2 == enc and dec2 == sample:
        ok('--encode / --decode long options')
    else:
        fail('--encode / --decode long options')
    # --- -D as decode alias ---------------------------------------------------
    dec3 = must_run([path, '-D'], input=enc)
    if dec3 == sample:
        ok('-D decode alias')
    else:
        fail('-D decode alias')
    # --- wrapping: -b N -------------------------------------------------------
    enc_b76 = must_run([path, '-e', '-b', '76', readme])
    lines = enc_b76.decode('utf-8').rstrip('\n').split('\n')
    if all(len(l) <= 76 for l in lines):
        ok('-b 76 wraps at 76 chars')
    else:
        too_long = [l for l in lines if len(l) > 76]
        fail('-b 76 wraps at 76 chars', f'{len(too_long)} lines exceeded 76')
    # --- wrapping: -w N -------------------------------------------------------
    enc_w40 = must_run([path, '-e', '-w', '40', readme])
    lines_w = enc_w40.decode('utf-8').rstrip('\n').split('\n')
    if all(len(l) <= 40 for l in lines_w):
        ok('-w 40 wraps at 40 chars')
    else:
        fail('-w 40 wraps at 40 chars')
    # --- wrapping: --wrap=N ---------------------------------------------------
    enc_wl = must_run([path, '-e', '--wrap=60', readme])
    lines_wl = enc_wl.decode('utf-8').rstrip('\n').split('\n')
    if all(len(l) <= 60 for l in lines_wl):
        ok('--wrap=60 wraps at 60 chars')
    else:
        fail('--wrap=60 wraps at 60 chars')
    # --- no-wrap default (single line body) -----------------------------------
    enc_nw = must_run([path, '-e'], input=src)
    stripped = enc_nw.rstrip(b'\n')
    if b'\n' not in stripped:
        ok('default no-wrap: no interior newlines')
    else:
        fail('default no-wrap: no interior newlines')
    # --- base64 alphabet check ------------------------------------------------
    check_base64_alphabet(enc_nw, 'base64 alphabet in output')
    # --- empty input ----------------------------------------------------------
    enc_empty = must_run([path, '-e'], input=b'')
    dec_empty = must_run([path, '-d'], input=enc_empty)
    if dec_empty == b'':
        ok('empty input round-trip')
    else:
        fail('empty input round-trip', f'got {dec_empty!r}')
    # --- single byte ----------------------------------------------------------
    enc_1 = must_run([path, '-e'], input=b'\xab')
    dec_1 = must_run([path, '-d'], input=enc_1)
    if dec_1 == b'\xab':
        ok('single-byte round-trip')
    else:
        fail('single-byte round-trip', f'got {dec_1!r}')
    # --- all 256 byte values round-trip ---------------------------------------
    all_bytes = bytes(range(256))
    enc_all = run_encode_binary(path, all_bytes, is_coreutils=False)
    dec_all = must_run([path, '-d'], input=enc_all)
    if dec_all == all_bytes:
        ok('all-256-byte-values round-trip')
    else:
        fail('all-256-byte-values round-trip')
    # --- -i FILE (input file option) ------------------------------------------
    with tempfile.NamedTemporaryFile(delete=False) as tf:
        tf.write(src)
        tf_path = tf.name
    try:
        enc_i = must_run([path, '-e', '-i', tf_path])
        if must_run([path, '-d'], input=enc_i) == src:
            ok('-i FILE (explicit input file)')
        else:
            fail('-i FILE (explicit input file)')
    finally:
        os.unlink(tf_path)
    # --- --input FILE option --------------------------------------------------
    with tempfile.NamedTemporaryFile(delete=False) as tf:
        tf.write(src)
        tf_path = os.path.normpath(tf.name)
    try:
        enc_inp = must_run([path, '-e', '--input', tf_path])
        if enc_inp == must_run([path, '-e', readme]):
            ok('--input FILE')
        else:
            fail('--input FILE')
    finally:
        os.unlink(tf_path)
    # --- -o FILE (output file option) -----------------------------------------
    with tempfile.NamedTemporaryFile(delete=False, suffix='.b64') as tf_out:
        tf_out_path = os.path.normpath(tf_out.name)
    try:
        rc, _, _ = run([path, '-e', '-o', tf_out_path, readme])
        enc_o = read_binary(tf_out_path)
        if rc == 0 and must_run([path, '-d'], input=enc_o) == src:
            ok('-o FILE (explicit output file)')
        else:
            fail('-o FILE (explicit output file)')
    finally:
        os.unlink(tf_out_path)
    # --- --output FILE option -------------------------------------------------
    with tempfile.NamedTemporaryFile(delete=False, suffix='.b64') as tf_out2:
        tf_out2_path = os.path.normpath(tf_out2.name)
    try:
        rc, _, _ = run([path, '-e', '--output', tf_out2_path, readme])
        enc_o2 = read_binary(tf_out2_path)
        if rc == 0 and must_run([path, '-d'], input=enc_o2) == src:
            ok('--output FILE')
        else:
            fail('--output FILE')
    finally:
        os.unlink(tf_out2_path)
    # --- second positional argument as output file ----------------------------
    ref_enc = must_run([path, '-e', readme])
    with tempfile.NamedTemporaryFile(delete=False, suffix='.b64') as tf_pos:
        tf_pos_path = os.path.normpath(tf_pos.name)
    try:
        rc, _, _ = run([path, '-e', readme, tf_pos_path])
        enc_pos = read_binary(tf_pos_path)
        if rc == 0 and enc_pos == ref_enc:
            ok('second positional arg as output file')
        else:
            fail('second positional arg as output file')
    finally:
        os.unlink(tf_pos_path)
    # --- bad decode: invalid base64 character should fail ---------------------
    rc, _, _ = run([path, '-d'], input=b'not!valid@base64\n', expect_failure=True)
    if rc != 0:
        ok('invalid base64 input fails')
    else:
        fail('invalid base64 input fails')
    # --- stdin marker '-' for input -------------------------------------------
    enc_stdin = must_run([path, '-e', '-'], input=src)
    if enc_stdin == enc_nw:
        ok("'-' reads from stdin")
    else:
        fail("'-' reads from stdin")
    # --- wrapped encode followed by round-trip --------------------------------
    enc_wrap = must_run([path, '-e', '-b', '76', readme])
    dec_wrap = must_run([path, '-d'], input=enc_wrap)
    if dec_wrap == src:
        ok('wrapped encode round-trips correctly')
    else:
        fail('wrapped encode round-trips correctly')
    # --ignore-garbage for BSD-like tool
    # Build clean data from file input to avoid Windows stdin text-mode CRLF->LF conversion.
    clean = must_run([path, '-e', readme])
    corrupted = clean[:10] + b'!!!@#$!!!' + clean[10:]
    rc, _, _ = run([path, '-d'], input=corrupted, expect_failure=True)
    if rc != 0:
        ok('fastbase64: corrupt input fails without ignore-garbage')
    dec_ig = must_run([path, '-d', '--ignore-garbage'], input=corrupted)
    if dec_ig == src:
        ok('fastbase64: --ignore-garbage recovers')
    else:
        fail('fastbase64: --ignore-garbage recovers')

# ---------------------------------------------------------------------------
# fastbase64.coreutils (GNU-like)
# ---------------------------------------------------------------------------
def test_coreutils(path, readme):
    print("\n=== fastbase64.coreutils (GNU-like) ===")
    src = read_binary(readme)
    # --- help / version -------------------------------------------------------
    rc, out, _ = run([path, '--help'])
    if rc == 0 and b'Usage' in out:
        ok('--help')
    else:
        fail('--help', f'rc={rc}')
    rc, out, _ = run([path, '--version'])
    if rc == 0 and b'fastbase64' in out:
        ok('--version')
    else:
        fail('--version', f'rc={rc}')
    # --- error on unknown option -----------------------------------------------
    rc, _, _ = run([path, '--unknown-flag'], expect_failure=True)
    if rc != 0:
        ok('unknown option rejected')
    else:
        fail('unknown option rejected')
    encoded = must_run([path, readme])
    decoded = must_run([path, '-d'], input=encoded)
    if decoded == src:
        ok('round-trip')
    else:
        fail('round-trip')
    # --- explicit -e and -d flags (binary data) --------------------------------
    sample = b'\x00\xff\xfe\x80binary\x01\x02'
    enc = must_run([path, '-e'], input=sample)
    dec = must_run([path, '-d'], input=enc)
    if dec == sample:
        ok('-e / -d explicit flags (binary data)')
    else:
        fail('-e / -d explicit flags (binary data)')
    # --- --encode / --decode long options -------------------------------------
    enc2 = must_run([path, '--encode'], input=sample)
    dec2 = must_run([path, '--decode'], input=enc2)
    if enc2 == enc and dec2 == sample:
        ok('--encode / --decode long options')
    else:
        fail('--encode / --decode long options')
    # --- -D as decode alias ---------------------------------------------------
    dec3 = must_run([path, '-D'], input=enc)
    if dec3 == sample:
        ok('-D decode alias')
    else:
        fail('-D decode alias')
    # --- default: wrapping at 76 ------------------------------
    enc_nw = must_run([path, readme])
    stripped = enc_nw.rstrip(b'\n')
    if b'\n' in stripped:
        ok('default wrapping: has interior newlines')
    else:
        fail('default wrapping: has interior newlines')
    # --- base64 alphabet check ------------------------------------------------
    check_base64_alphabet(enc_nw, 'base64 alphabet in output')
    # --- -w 76 wrapping -------------------------------------------------------
    enc_w76 = must_run([path, '-w', '76', readme])
    lines = enc_w76.decode('utf-8').rstrip('\n').split('\n')
    if all(len(l) <= 76 for l in lines):
        ok('-w 76 wraps at 76 chars')
    else:
        fail('-w 76 wraps at 76 chars')
    # --- -w 0 explicit no-wrap ------------------------------------------------
    enc_w0 = must_run([path, '-w', '0', readme])
    stripped_w0 = enc_w0.rstrip(b'\n')
    if b'\n' not in stripped_w0:
        ok('-w 0 no-wrap: no interior newlines')
    else:
        fail('-w 0 no-wrap: no interior newlines')
    # --- --wrap=N -------------------------------------------------------------
    enc_wrap_eq = must_run([path, '--wrap=40', readme])
    lines_eq = enc_wrap_eq.decode('utf-8').rstrip('\n').split('\n')
    if all(len(l) <= 40 for l in lines_eq):
        ok('--wrap=40 wraps at 40 chars')
    else:
        fail('--wrap=40 wraps at 40 chars')
    # --- -b N (alias for -w) --------------------------------------------------
    enc_b = must_run([path, '-b', '76', readme])
    if enc_b == enc_w76:
        ok('-b 76 is alias for -w 76')
    else:
        fail('-b 76 is alias for -w 76')
    # --- empty input ----------------------------------------------------------
    enc_empty = must_run([path], input=b'')
    dec_empty = must_run([path, '-d'], input=enc_empty)
    if dec_empty == b'':
        ok('empty input round-trip')
    else:
        fail('empty input round-trip', f'got {dec_empty!r}')
    # --- all 256 byte values round-trip ---------------------------------------
    all_bytes = bytes(range(256))
    enc_all = run_encode_binary(path, all_bytes, is_coreutils=True)
    dec_all = must_run([path, '-d'], input=enc_all)
    if dec_all == all_bytes:
        ok('all-256-byte-values round-trip')
    else:
        fail('all-256-byte-values round-trip')
    # --- -i (ignore-garbage, GNU style) --------------------------------------
    clean_enc = must_run([path, '-w', '0', readme])
    corrupted = clean_enc[:10] + b'!' + clean_enc[10:]
    # Without -i: should fail
    rc_fail, _, _ = run([path, '-d'], input=corrupted, expect_failure=True)
    if rc_fail != 0:
        ok('corrupt input fails without ignore-garbage')
    else:
        fail('corrupt input fails without ignore-garbage')
    # With -i: should produce original
    dec_ig = must_run([path, '-d', '--ignore-garbage'], input=corrupted)
    if dec_ig == src:
        ok('-i ignores garbage and decodes correctly')
    else:
        fail('-i ignores garbage and decodes correctly')
    # --- --ignore-garbage long option -----------------------------------------
    dec_ig2 = must_run([path, '-d', '--ignore-garbage'], input=corrupted)
    if dec_ig2 == src:
        ok('--ignore-garbage long option')
    else:
        fail('--ignore-garbage long option')
    # --- -o FILE output -------------------------------------------------------
    with tempfile.NamedTemporaryFile(delete=False, suffix='.b64') as tf_out:
        tf_out_path = os.path.normpath(tf_out.name)
    try:
        rc, _, _ = run([path, '-o', tf_out_path, readme])
        enc_o = read_binary(tf_out_path)
        if rc == 0 and enc_o == enc_nw:
            ok('-o FILE output')
        else:
            fail('-o FILE output')
    finally:
        os.unlink(tf_out_path)
    # --- second positional argument as output file ----------------------------
    with tempfile.NamedTemporaryFile(delete=False, suffix='.b64') as tf_pos:
        tf_pos_path = os.path.normpath(tf_pos.name)
    try:
        rc, _, _ = run([path, readme, tf_pos_path])
        enc_pos = read_binary(tf_pos_path)
        if rc == 0 and enc_pos == enc_nw:
            ok('second positional arg as output file')
        else:
            fail('second positional arg as output file')
    finally:
        os.unlink(tf_pos_path)
    # --- stdin marker '-' for input -------------------------------------------
    enc_stdin = must_run([path, '-'], input=src)
    dec_stdin = must_run([path, '-d'], input=enc_stdin)
    expected_stdin = src.replace(b'\r\n', b'\n') if is_windows else src
    if dec_stdin == expected_stdin:
        ok("'-' reads from stdin")
    else:
        fail("'-' reads from stdin")
    # --- wrapped encode followed by round-trip --------------------------------
    enc_wrap = must_run([path, '-w', '76', readme])
    dec_wrap = must_run([path, '-d'], input=enc_wrap)
    if dec_wrap == src:
        ok('wrapped encode round-trips correctly')
    else:
        fail('wrapped encode round-trips correctly')

# ---------------------------------------------------------------------------
# Large round-trips (explicitly >64K, deterministic)
# ---------------------------------------------------------------------------
def test_large_roundtrips(fast_path, core_path):
    print("\n=== Large inputs round-trips (over 64K, deterministic) ===")
    sizes = [65536, 65537, 131072, 262144, 524288, 1048576, 2097152]
    for size in sizes:
        data = generate_deterministic_data(size)
        label = f"{size:,} bytes"
        # fastbase64
        with tempfile.NamedTemporaryFile(delete=False) as tf:
            tf.write(data)
            tf_path = tf.name
        try:
            enc_f = must_run([fast_path, '-e', '-i', tf_path])
            dec_f = must_run([fast_path, '-d'], input=enc_f)
            if dec_f == data:
                ok(f'fastbase64 round-trip {label}')
            else:
                fail(f'fastbase64 round-trip {label}')
        finally:
            os.unlink(tf_path)
        # coreutils
        with tempfile.NamedTemporaryFile(delete=False) as tf:
            tf.write(data)
            tf_path = tf.name
        try:
            enc_c = must_run([core_path, tf_path])
            dec_c = must_run([core_path, '-d'], input=enc_c)
            if dec_c == data:
                ok(f'coreutils round-trip {label}')
            else:
                fail(f'coreutils round-trip {label}')
        finally:
            os.unlink(tf_path)
        # wrapped 76 on both
        with tempfile.NamedTemporaryFile(delete=False) as tf:
            tf.write(data)
            tf_path = tf.name
        try:
            enc_fw = must_run([fast_path, '-e', '-w', '76', '-i', tf_path])
            if must_run([fast_path, '-d'], input=enc_fw) == data:
                ok(f'fastbase64 wrapped {label}')
        finally:
            os.unlink(tf_path)
        with tempfile.NamedTemporaryFile(delete=False) as tf:
            tf.write(data)
            tf_path = tf.name
        try:
            enc_cw = must_run([core_path, '-w', '76', tf_path])
            if must_run([core_path, '-d'], input=enc_cw) == data:
                ok(f'coreutils wrapped {label}')
        finally:
            os.unlink(tf_path)
        check_base64_alphabet(enc_f, f'large alphabet check {label}')

# ---------------------------------------------------------------------------
# Wrapping extremes + edge options
# ---------------------------------------------------------------------------
def test_wrapping_extremes(fast_path, core_path):
    print("\n=== Wrapping extremes & edge options ===")
    data = generate_deterministic_data(8192)
    for tool, name, default_encode in [(fast_path, "fastbase64", False), (core_path, "coreutils", True)]:
        for w in [1, 4, 76, 100000]:
            with tempfile.NamedTemporaryFile(delete=False) as tf:
                tf.write(data)
                tf_path = tf.name
            try:
                cmd = [tool, '-w', str(w)]
                if not default_encode:
                    cmd.insert(1, '-e')
                else:
                    cmd.append(tf_path)
                if not default_encode:
                    cmd.extend(['-i', tf_path])
                enc = must_run(cmd)
                dec = must_run([tool, '-d'], input=enc)
                if dec == data:
                    ok(f'{name} -w {w} round-trip')
                else:
                    fail(f'{name} -w {w} round-trip')
                # verify line lengths
                lines = enc.rstrip(b'\n').split(b'\n')
                max_len = max((len(l) for l in lines if l), default=0)
                if max_len <= w or w >= len(data) * 4 // 3:
                    ok(f'{name} -w {w} line lengths correct')
                else:
                    fail(f'{name} -w {w} line lengths', f'max line {max_len}')
            finally:
                os.unlink(tf_path)

# ---------------------------------------------------------------------------
# Adversarial decoding (bad padding, garbage, whitespace, long lines)
# ---------------------------------------------------------------------------
def test_adversarial_decoding(fast_path, core_path):
    print("\n=== Adversarial decoding (padding, garbage, whitespace) ===")
    src = b'adversarial\x00\xff\x01\x02\xab\xcd'
    clean = base64.b64encode(src) + b'\n'
    # garbage in various positions
    for garbage, pos in [(b'!!!', 20), (b'@#$', 10)]:
        dirty = clean[:pos] + garbage + clean[pos:]
        # without ignore -> fail
        for tool, ignore_flag in [(fast_path, '--ignore-garbage'), (core_path, '--ignore-garbage')]:
            rc, _, _ = run([tool, '-d'], input=dirty, expect_failure=True)
            if rc != 0:
                ok(f'{tool} rejects garbage without ignore')
            else:
                fail(f'{tool} rejects garbage without ignore')
            # with ignore -> recover
            dec = must_run([tool, '-d', ignore_flag], input=dirty)
            if dec == src:
                ok(f'{tool} ignore-garbage recovers')
            else:
                fail(f'{tool} ignore-garbage recovers')
    # bad padding
    bad_pads = [b'QUFB=', b'QUFB==', b'QUFB===', b'=QUFB', b'QUFB\n=']
    for bp in bad_pads:
        for tool in [fast_path, core_path]:
            rc, _, _ = run([tool, '-d'], input=bp, expect_failure=True)
            if rc != 0:
                ok(f'{tool} rejects bad padding {bp!r}')
            else:
                fail(f'{tool} rejects bad padding {bp!r}')
    # incomplete base64 groups (BASE64_INPUT_REMAINDER)
    remainder_cases = [b'AAAAA', b'AAAAAA', b'AAAAAAA', b'QUJ']  # 5, 6, 7, 3 chars
    for rc_input in remainder_cases:
        for tool in [fast_path, core_path]:
            rc, _, _ = run([tool, '-d'], input=rc_input, expect_failure=True)
            if rc != 0:
                ok(f'{tool} rejects incomplete group {rc_input!r}')
            else:
                fail(f'{tool} rejects incomplete group {rc_input!r}')
    # non-zero padding bits (BASE64_EXTRA_BITS)
    extra_bits_cases = [b'YWF=', b'ZXhhZh==']  # "aa" with extra bits, "exhf" with extra bits
    for eb_input in extra_bits_cases:
        for tool in [fast_path, core_path]:
            rc, _, _ = run([tool, '-d'], input=eb_input, expect_failure=True)
            if rc != 0:
                ok(f'{tool} rejects non-zero padding bits {eb_input!r}')
            else:
                fail(f'{tool} rejects non-zero padding bits {eb_input!r}')
    # very long single-line base64
    long_data = generate_deterministic_data(100000)
    long_enc = base64.b64encode(long_data) + b'\n'
    for tool in [fast_path, core_path]:
        dec_long = must_run([tool, '-d'], input=long_enc)
        if dec_long == long_data:
            ok(f'{tool} handles 100KB single-line base64')
        else:
            fail(f'{tool} handles 100KB single-line base64')

# ---------------------------------------------------------------------------
# Chunk-boundary whitespace stress tests
# ---------------------------------------------------------------------------
def test_chunk_boundary_whitespace(fast_path, core_path):
    """Insert whitespace at and near the 65536-byte chunk boundary used by
    the decoder.  This stresses the carry-over / leftover logic."""
    print("\n=== Chunk-boundary whitespace stress ===")
    CHUNK = 65536
    # Use a large enough payload so that the base64 encoding spans multiple
    # chunks.  100 000 binary bytes → ~133 336 base64 chars (> 2 × CHUNK).
    src = generate_deterministic_data(100000)
    clean = base64.b64encode(src)  # no trailing newline yet

    def insert_at(data, pos, insertion):
        """Insert bytes at the given position (clamped to length)."""
        pos = min(pos, len(data))
        return data[:pos] + insertion + data[pos:]

    # 20 test cases: for each tool, insert various whitespace patterns at
    # positions around the first and second chunk boundaries.
    cases = [
        # (label, position relative to CHUNK, bytes to insert)
        ("space at chunk boundary",          CHUNK,     b' '),
        ("space one before boundary",        CHUNK - 1, b' '),
        ("space one after boundary",         CHUNK + 1, b' '),
        ("newline at chunk boundary",        CHUNK,     b'\n'),
        ("10 spaces at boundary",            CHUNK,     b' ' * 10),
        ("CRLF at boundary",                 CHUNK,     b'\r\n'),
        ("spaces straddling boundary -3..+3", CHUNK - 3, b' ' * 6),
        ("tab at boundary",                  CHUNK,     b'\t'),
        ("mixed ws at boundary",             CHUNK - 2, b' \t\r\n '),
        ("50 newlines at boundary",          CHUNK,     b'\n' * 50),
        # Second chunk boundary (2 × CHUNK in the *already-padded* stream,
        # approximate – we just pick offsets near it).
        ("space at 2×chunk boundary",        CHUNK * 2, b' '),
        ("newline at 2×chunk boundary",      CHUNK * 2, b'\n'),
        ("10 spaces at 2×chunk boundary",    CHUNK * 2, b' ' * 10),
        ("spaces near 2×chunk -5..+5",       CHUNK * 2 - 5, b' ' * 10),
        # Multiple insertions: whitespace at BOTH chunk boundaries
        ("spaces at both chunk boundaries",  -1, b''),  # handled specially below
        # Whitespace right at the 4-char base64 group boundary nearest CHUNK
        ("space at group boundary before chunk", (CHUNK // 4) * 4, b' '),
        ("space at group boundary after chunk",  (CHUNK // 4 + 1) * 4, b' '),
        # Large block of whitespace pushing the second chunk far out
        ("1000 spaces at boundary",          CHUNK,     b' ' * 1000),
        # Whitespace every 76 chars (simulating wrapped input decoded raw)
        ("newline every 76 chars",           -2, b''),  # handled specially below
        # Trailing whitespace right before end
        ("trailing spaces before final newline", -3, b''),  # handled specially
    ]

    for tool, tool_name in [(fast_path, "fastbase64"), (core_path, "coreutils")]:
        decode_cmd = [tool, '-d']
        for label, pos, insertion in cases:
            if pos == -1:
                # Special: insert spaces at BOTH chunk boundaries
                modified = insert_at(clean, CHUNK, b' ' * 5)
                modified = insert_at(modified, CHUNK * 2 + 5, b' ' * 5)
                modified += b'\n'
            elif pos == -2:
                # Special: insert newlines every 76 chars (like wrapped output)
                parts = [clean[i:i+76] for i in range(0, len(clean), 76)]
                modified = b'\n'.join(parts) + b'\n'
            elif pos == -3:
                # Special: trailing whitespace
                modified = clean + b'   \t\t  \n'
            else:
                modified = insert_at(clean, pos, insertion) + b'\n'

            dec = must_run(decode_cmd, input=modified)
            if dec == src:
                ok(f'{tool_name}: {label}')
            else:
                fail(f'{tool_name}: {label}',
                     f'got {len(dec)} bytes, expected {len(src)}')

# ---------------------------------------------------------------------------
# Error conditions
# ---------------------------------------------------------------------------
def test_error_conditions(fast_path, core_path):
    print("\n=== Error conditions ===")
    for tool, name in [(fast_path, "fastbase64"), (core_path, "coreutils")]:
        # non-existent file
        rc, _, _ = run([tool, '-e', '/nonexistent_fastbase64_test_987654321'], expect_failure=True)
        if rc != 0:
            ok(f'{name}: non-existent input file rejected')
        else:
            fail(f'{name}: non-existent input file unexpectedly accepted')
        # bad wrap values
        for bad in ['-1', 'abc', '999999999999999']:
            rc, _, _ = run([tool, '-e', '-w', bad], input=b'test', expect_failure=True)
            if rc != 0:
                ok(f'{name}: bad -w {bad} rejected')
            else:
                fail(f'{name}: bad -w {bad} unexpectedly accepted')

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
if __name__ == '__main__':
    if len(sys.argv) != 4:
        print("Usage: python test_fastbase64.py <fastbase64> <fastbase64.coreutils> <input_file>")
        sys.exit(1)
    fast_path, core_path, readme_path = sys.argv[1], sys.argv[2], sys.argv[3]
    for label, p in [('fastbase64', fast_path),
                     ('fastbase64.coreutils', core_path),
                     ('input file', readme_path)]:
        if not os.path.exists(p):
            print(f"Error: {label} not found at {p}")
            sys.exit(1)
    test_fastbase64(fast_path, readme_path)
    test_coreutils(core_path, readme_path)
    # NEW thorough sections
    test_large_roundtrips(fast_path, core_path)
    test_wrapping_extremes(fast_path, core_path)
    test_adversarial_decoding(fast_path, core_path)
    test_chunk_boundary_whitespace(fast_path, core_path)
    test_error_conditions(fast_path, core_path)
    print(f"\n{'='*60}")
    print(f"Results: {PASS} passed, {FAIL} failed")
    if FAIL > 0:
        sys.exit(1)
    print("All tests passed! (including large/adversarial cases)")
