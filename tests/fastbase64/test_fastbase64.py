#!/usr/bin/env python3
"""
Comprehensive tests for fastbase64 (BSD-like, default-decode) and
fastbase64.coreutils (GNU-like, default-encode).
Optionally cross-checks against the system 'base64' utility.
"""

import base64
import os
import shutil
import subprocess
import sys
import tempfile

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

PASS = 0
FAIL = 0

def ok(label):
    global PASS
    PASS += 1
    print(f"  PASS  {label}")

def fail(label, detail=""):
    global FAIL
    FAIL += 1
    msg = f"  FAIL  {label}"
    if detail:
        msg += f"\n        {detail}"
    print(msg)

def run(cmd, input=None, expect_failure=False):
    """Run a command, return (returncode, stdout_bytes, stderr_bytes)."""
    result = subprocess.run(cmd, input=input, capture_output=True, text=False)
    if not expect_failure and result.returncode != 0:
        fail(f"Unexpected failure: {' '.join(str(c) for c in cmd)}",
             result.stderr.decode('utf-8', errors='replace'))
        sys.exit(1)
    return result.returncode, result.stdout, result.stderr

def must_run(cmd, input=None):
    """Run a command and return stdout; exit on failure."""
    rc, stdout, stderr = run(cmd, input=input)
    return stdout

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

# ---------------------------------------------------------------------------
# fastbase64  (BSD-like:  default = decode)
# ---------------------------------------------------------------------------

def test_fastbase64(path, readme):
    print("\n=== fastbase64 (BSD-like, default-decode) ===")
    src = open(readme, 'rb').read()

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

    # --- default action is decode ----------------------------------------------
    encoded_ref = base64.b64encode(b'hello default\n') + b'\n'
    decoded_default = must_run([path], input=encoded_ref)
    if decoded_default == b'hello default\n':
        ok('default action is decode')
    else:
        fail('default action is decode', f'got {decoded_default!r}')

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
    enc_all = must_run([path, '-e'], input=all_bytes)
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
        tf_path = tf.name
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
        tf_out_path = tf_out.name
    try:
        rc, _, _ = run([path, '-e', '-o', tf_out_path, readme])
        enc_o = open(tf_out_path, 'rb').read()
        if rc == 0 and must_run([path, '-d'], input=enc_o) == src:
            ok('-o FILE (explicit output file)')
        else:
            fail('-o FILE (explicit output file)')
    finally:
        os.unlink(tf_out_path)

    # --- --output FILE option -------------------------------------------------
    with tempfile.NamedTemporaryFile(delete=False, suffix='.b64') as tf_out2:
        tf_out2_path = tf_out2.name
    try:
        rc, _, _ = run([path, '-e', '--output', tf_out2_path, readme])
        enc_o2 = open(tf_out2_path, 'rb').read()
        if rc == 0 and must_run([path, '-d'], input=enc_o2) == src:
            ok('--output FILE')
        else:
            fail('--output FILE')
    finally:
        os.unlink(tf_out2_path)

    # --- second positional argument as output file ----------------------------
    ref_enc = must_run([path, '-e', readme])
    with tempfile.NamedTemporaryFile(delete=False, suffix='.b64') as tf_pos:
        tf_pos_path = tf_pos.name
    try:
        rc, _, _ = run([path, '-e', readme, tf_pos_path])
        enc_pos = open(tf_pos_path, 'rb').read()
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

# ---------------------------------------------------------------------------
# fastbase64.coreutils  (GNU-like:  default = encode)
# ---------------------------------------------------------------------------

def test_coreutils(path, readme):
    print("\n=== fastbase64.coreutils (GNU-like, default-encode) ===")
    src = open(readme, 'rb').read()

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

    # --- default action is encode ---------------------------------------------
    encoded = must_run([path, readme])
    decoded = must_run([path, '-d'], input=encoded)
    if decoded == src:
        ok('default action is encode (round-trip)')
    else:
        fail('default action is encode (round-trip)')

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

    # --- default: no wrapping (single line body) ------------------------------
    enc_nw = must_run([path, readme])
    stripped = enc_nw.rstrip(b'\n')
    if b'\n' not in stripped:
        ok('default no-wrap: no interior newlines')
    else:
        fail('default no-wrap: no interior newlines')

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
    if enc_w0.rstrip(b'\n') == enc_nw.rstrip(b'\n'):
        ok('-w 0 same as default (no wrap)')
    else:
        fail('-w 0 same as default (no wrap)')

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
    enc_all = must_run([path], input=all_bytes)
    dec_all = must_run([path, '-d'], input=enc_all)
    if dec_all == all_bytes:
        ok('all-256-byte-values round-trip')
    else:
        fail('all-256-byte-values round-trip')

    # --- -i  (ignore-garbage, GNU style) --------------------------------------
    clean_enc = must_run([path, '-w', '0', readme])
    corrupted = clean_enc[:10] + b'!' + clean_enc[10:]
    # Without -i: should fail
    rc_fail, _, _ = run([path, '-d'], input=corrupted, expect_failure=True)
    if rc_fail != 0:
        ok('corrupt input fails without ignore-garbage')
    else:
        fail('corrupt input fails without ignore-garbage')
    # With -i: should produce original
    dec_ig = must_run([path, '-d', '-i'], input=corrupted)
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
        tf_out_path = tf_out.name
    try:
        rc, _, _ = run([path, '-o', tf_out_path, readme])
        enc_o = open(tf_out_path, 'rb').read()
        if rc == 0 and enc_o == enc_nw:
            ok('-o FILE output')
        else:
            fail('-o FILE output')
    finally:
        os.unlink(tf_out_path)

    # --- second positional argument as output file ----------------------------
    with tempfile.NamedTemporaryFile(delete=False, suffix='.b64') as tf_pos:
        tf_pos_path = tf_pos.name
    try:
        rc, _, _ = run([path, readme, tf_pos_path])
        enc_pos = open(tf_pos_path, 'rb').read()
        if rc == 0 and enc_pos == enc_nw:
            ok('second positional arg as output file')
        else:
            fail('second positional arg as output file')
    finally:
        os.unlink(tf_pos_path)

    # --- stdin marker '-' for input -------------------------------------------
    enc_stdin = must_run([path, '-'], input=src)
    if enc_stdin == enc_nw:
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
# Cross-tool compatibility
# ---------------------------------------------------------------------------

def test_cross_compatibility(fast_path, core_path, readme):
    print("\n=== Cross-tool compatibility ===")
    src = open(readme, 'rb').read()

    # fastbase64 encode -> coreutils decode
    enc = must_run([fast_path, '-e', readme])
    dec = must_run([core_path, '-d'], input=enc)
    if dec == src:
        ok('fastbase64 -e -> coreutils -d')
    else:
        fail('fastbase64 -e -> coreutils -d')

    # coreutils encode -> fastbase64 decode
    enc2 = must_run([core_path, readme])
    dec2 = must_run([fast_path, '-d'], input=enc2)
    if dec2 == src:
        ok('coreutils encode -> fastbase64 -d')
    else:
        fail('coreutils encode -> fastbase64 -d')

    # coreutils -w 76 -> fastbase64 decode (handles line-wrapped input)
    enc3 = must_run([core_path, '-w', '76', readme])
    dec3 = must_run([fast_path, '-d'], input=enc3)
    if dec3 == src:
        ok('coreutils -w 76 -> fastbase64 -d')
    else:
        fail('coreutils -w 76 -> fastbase64 -d')

    # fastbase64 -b 76 -> coreutils decode
    enc4 = must_run([fast_path, '-e', '-b', '76', readme])
    dec4 = must_run([core_path, '-d'], input=enc4)
    if dec4 == src:
        ok('fastbase64 -b 76 -> coreutils -d')
    else:
        fail('fastbase64 -b 76 -> coreutils -d')

    # Both tools produce byte-identical output for the same input
    enc_fast = must_run([fast_path, '-e', readme])
    enc_core = must_run([core_path, readme])
    if enc_fast == enc_core:
        ok('both tools produce identical base64 output')
    else:
        fail('both tools produce identical base64 output',
             f'fast={enc_fast[:40]!r} core={enc_core[:40]!r}')

    # Cross-check against Python's base64 module
    py_enc = base64.b64encode(src) + b'\n'
    if enc_fast == py_enc:
        ok('output matches Python base64.b64encode')
    else:
        fail('output matches Python base64.b64encode',
             f'tool={enc_fast[:40]!r} py={py_enc[:40]!r}')

# ---------------------------------------------------------------------------
# System base64 compatibility (optional)
# ---------------------------------------------------------------------------

def test_system_base64(fast_path, core_path, readme):
    """Cross-check against the system 'base64' utility if available."""
    sys_b64 = shutil.which('base64')
    if sys_b64 is None:
        print("\n=== System base64 compatibility (SKIPPED: not found) ===")
        return
    print(f"\n=== System base64 compatibility ({sys_b64}) ===")
    src = open(readme, 'rb').read()

    # Determine whether the system base64 is BSD or GNU
    probe = subprocess.run([sys_b64, '--version'], capture_output=True, text=False)
    is_gnu = probe.returncode == 0 and b'GNU' in (probe.stdout + probe.stderr)

    # System encode -> both tools decode
    if is_gnu:
        sys_enc = must_run([sys_b64, readme])
    else:
        # BSD base64 uses -i for input file
        sys_enc = must_run([sys_b64, '-i', readme])

    dec_core = must_run([core_path, '-d'], input=sys_enc)
    if dec_core == src:
        ok('system base64 encode -> coreutils -d')
    else:
        fail('system base64 encode -> coreutils -d')

    dec_fast = must_run([fast_path, '-d'], input=sys_enc)
    if dec_fast == src:
        ok('system base64 encode -> fastbase64 -d')
    else:
        fail('system base64 encode -> fastbase64 -d')

    # coreutils encode -> system decode
    core_enc = must_run([core_path, readme])
    if is_gnu:
        sys_dec_core = must_run([sys_b64, '-d'], input=core_enc)
    else:
        sys_dec_core = must_run([sys_b64, '-D'], input=core_enc)

    if sys_dec_core == src:
        ok('coreutils encode -> system base64 -d')
    else:
        fail('coreutils encode -> system base64 -d')

    # fastbase64 encode -> system decode
    fast_enc = must_run([fast_path, '-e', readme])
    if is_gnu:
        sys_dec_fast = must_run([sys_b64, '-d'], input=fast_enc)
    else:
        sys_dec_fast = must_run([sys_b64, '-D'], input=fast_enc)

    if sys_dec_fast == src:
        ok('fastbase64 -e -> system base64 -d')
    else:
        fail('fastbase64 -e -> system base64 -d')

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
    test_cross_compatibility(fast_path, core_path, readme_path)
    test_system_base64(fast_path, core_path, readme_path)

    print(f"\n{'='*50}")
    print(f"Results: {PASS} passed, {FAIL} failed")
    if FAIL > 0:
        sys.exit(1)
    print("All tests passed!")