# Safety margins of the simdutf transcoders

The SIMD kernels write full vectors, and a vector can extend past the output
the iteration accounts for. The caller sizes the output buffer with the
matching length function (`utf8_length_from_utf16`, ...). A loop's *safety
margin* is the extra input it requires before running an iteration, so that
these overhanging stores still land inside that buffer, on every input the
function accepts, including ill-formed input for the validating functions.

This directory derives, in Lean 4, the margin each transcoding loop needs, for
every SIMD kernel: westmere, haswell, icelake, arm64, ppc64, rvv, lsx and lasx.
Base64 is out of scope.

```sh
source "$HOME/.elan/env"
cd formal/safety_margins
lake build
```

`lean-toolchain` pins Lean 4.35. There is no mathlib and no `sorry`.
`native_decide` is used only for finite checks: the lookup tables
(`TableFacts.lean`) and the pair tests on all valid 4-byte characters
(`Exact.pair4`). The other exhaustive checks (the validator on all byte
pairs, `Validator.pair_ok`; on every state of the UTF-8 automaton,
`ValidExact.closure`, `ValidatorSound.sound_closure`; the 16-bit lane facts
of the replacement files) use `decide +kernel`, checked by the Lean kernel.

## Files

| file | content |
|---|---|
| `SafetyMargins/Basic.lean` | the model and the two safety theorems |
| `SafetyMargins/Kernels.lean` | path tables shared by several architectures |
| `SafetyMargins/Utf8.lean` | the dynamic margin of the UTF-8 decoders, and `convert_valid` on valid input |
| `SafetyMargins/Validator.lean` | the UTF-8 check `check_utf8_bytes`, and the character structure of a block that passes it |
| `SafetyMargins/Exact.lean` | exact output on checked blocks, the in-loop store on unit weights, and the ASCII tail |
| `SafetyMargins/ValidLatin1.lean` | `valid_utf8_to_latin1.h` on the valid, Latin-1-representable input its contract allows |
| `SafetyMargins/Utf8Table.lean` | the decoder's per-call facts, derived from the `utf8bigindex` and `shufutf8` tables and the lane arithmetic |
| `SafetyMargins/Tables.lean` | lookup tables extracted from `src/tables/` by `extract_tables.py`; don't edit it by hand |
| `SafetyMargins/TableFacts.lean` | facts about those tables |
| `SafetyMargins/Arch/*.lean` | one file per architecture: every loop, its path table, its margins |
| `SafetyMargins/Loads.lean` | the input loads of every fixed-margin loop whose margin was lowered (§7) |
| `SafetyMargins/Utf16Fix.lean` | every access of every `utf16fix` kernel, for every length (§7) |
| `SafetyMargins/Replacement.lean` | the two conversions with replacement (§7) |
| `SafetyMargins/ValidExact.lean` | the `convert_valid` cursor and tail on valid UTF-8 (§7) |
| `SafetyMargins/LoadsAll.lean` | the input loads of every other fixed-margin loop (§7) |
| `SafetyMargins/ReplacementMore.lean` | `maximal_subpart`, the replacement length functions, the scalar validator (§7) |
| `SafetyMargins/ReplacementRest.lean` | the Ice Lake replacement length, the window boundary, the whole UTF-8 → UTF-16 driver, the `_safe` UTF-16 → UTF-8 entry points (§7) |
| `SafetyMargins/ValidatorSound.lean` | the SIMD UTF-8 validator accepts exactly valid UTF-8; the replacement driver end to end (§7) |
| `SafetyMargins/RvvExact.lean` | every rvv kernel, on its lane values (§7) |
| `SafetyMargins/ValidDynamic.lean` | the dynamic margin of `convert_valid`: the backward scan, stores and loads in the blocks it admits (§4) |
| `SafetyMargins/ValidHybrid.lean` | `convert_valid` as the headers run it, the fixed-margin loop then at most one dynamic block: stores and loads end to end on valid input, and valid inputs on which one less fails (§4) |
| `check_citations.py` | checks that every C++ `file:line` cited in the Lean files exists, and that every cited loop test is a loop |
| `SafetyMargins/IcelakeReal.lean` | the Ice Lake UTF-16 loops on the unit values, which discharge the data hypotheses of `safe_utf16to32_real` and `safe_utf16to8_real` (§7) |
| `SafetyMargins/Scalar.lean` | the scalar code: every `scalar::` converter, `rewind_and_convert_with_errors` and the scalar fallbacks inside the vector loops write at most `L` of what they are given; the charge of `Basic.written` for the tail is met (§8) |
| `SafetyMargins/ReplacementClose.lean` | the contracts of the conversions with replacement discharged for the generic kernels; the `_safe` entry points `convert_utf16_to_utf8_safe` and `convert_latin1_to_utf8_safe` (§8) |
| `SafetyMargins/LoadsRest.lean` | the loads of the rvv kernels, the validators, the backward scans and the replacement drivers (§8) |
| `SafetyMargins/IcelakeMask.lean`, `IcelakeUtf8.lean`, `IcelakeUtf8Fixed.lean` | the icelake UTF-8 → UTF-16 / UTF-32 / Latin-1 decoders at the level of 64-byte blocks and masks: every store within the length function, every load within the input (§8) |
| `SafetyMargins/Lengths*.lean` | every ordinary length function, scalar and on every kernel, returns the per-unit sum on every input, except two over-estimates of `utf8_length_from_utf16` on ill-formed input (§8) |
| `SafetyMargins/ReplacementLengths.lean` | the replacement and `_safe` entry points of the generic kernels with each kernel's own length function: no length hypothesis left (§8) |

## 1. Model (`Basic.lean`)

**Input.** An input is the list of per-unit contributions of the length
function. `L` is their sum, so the buffer holds `L xs` units, and
`L (x ++ y) = L x + L y`. `Pos xs` says every unit contributes at least 1.
That holds for UTF-16, UTF-32 and Latin-1 sources. It fails in two cases: a
UTF-8 continuation byte contributes 0, and so does a low surrogate under
`utf32_length_from_utf16`.

**Loop test.** `end - buf >= B + m` guarantees `need B m false = B + m` units.
`end - buf > B + m` guarantees `need B m true = B + m + 1`.

**Iteration.** An iteration (`Iter`) consumes some units and issues stores
(`Place`):
- `rel a w` writes `w` output units at `L` of the first `a` consumed units. A
  kernel whose cursor lags behind `L` only ends earlier, so this is an upper
  bound.
- `tail o` ends at most `o` past `L` of the consumed prefix. `tail 0` is an
  exact (masked, or `vl`-exact) store.
- `ahead` ends at most at `L` of the consumed prefix plus the next unit, which
  the loop test must guarantee (one unit of look-ahead, used by icelake).

A `stop` iteration is an error return. It writes at most `L` of what it
consumed. The scalar fallback (`Kernels.scalar`) has no vector store.

**Path table.** Each kernel loop is described by the list of iterations it
can perform, transcribed from the C++ source cited next to the table.

**Optimal margin.** `optimalMargin B strict paths` is the least `m` with
`a + w ≤ need B m strict` for every `rel a w`, and with
`consumed + o ≤ need B m strict` for every `tail o`, and with
`consumed + 1 ≤ need B m strict` for every `ahead`.

**`safe_of_paths`.** Let `paths.all (Iter.ok N)` hold, and let `choose` be
any function that picks a member of `paths` whenever at least `N` units
remain. Then for every `Pos` input and any number of iterations, the
furthest written index `written N choose n xs` is at most `L xs`.
- `choose` is arbitrary, so the proof does not depend on how the C++ selects
  a path, only on the table listing every path.
- The theorem uses `Pos` because the units not yet consumed must pay for the
  overhang.

**`safe_fits` / `safe_exact` / `safe_of_exact`.** A loop whose stores never
pass `L xs` (in particular, never pass `L` of the consumed prefix, or of the
consumed prefix plus the next unit) is safe for every input, with no `Pos`.
This covers the masked stores (icelake), the `vl`-exact stores (rvv), and the
weight-0 sources: UTF-16 → UTF-32, and the icelake UTF-8 decoders.

Each fixed-margin loop in `Arch/*.lean` has the following theorems:
- `m_*`: `optimalMargin = m*`, by `decide`.
- `cur_*`: the margin in the source suffices.
- `ok_*`: `m*` suffices. It is stated where the source margin was lowered
  to `m*`, and then replaces `cur_*`.
- `wit_*`: when `m* > 0`, an iteration of the table and an input of length
  `need B (m* - 1) strict` whose store ends one unit past `L`. One less
  therefore overflows.

## 2. Tables (`TableFacts.lean`)

A `rel` offset such as `row[0]` is justified by a table fact:
- `pack12_exact`: `pack_1_2_utf8_bytes` stores `16 - popcount`.
- `pack123_exact`: `pack_1_2_3_utf8_bytes` stores the sum of the lane lengths
  on every reachable index.
- `bigindex_exact`: for every non-error row of `utf8bigindex`, the call ends
  on a code-point boundary and produces one unit per code point consumed.
  Error rows consume 12 bytes and the kernels store nothing for them.
  `bigconsumed_le`: no row consumes more than 12 bytes.
- `shuf3_starts`: on the three-code-point rows of `shufutf8`, the code points
  start at mask boundaries, and each lane's top byte is zero or the code
  point's first byte.

What Lean does not check is that the kernel uses the entry as the store
offset.

## 3. UTF-8 sources: the dynamic margin (`Utf8.lean`)

A tail of continuation bytes has `L = 0`, so no fixed margin works. The
validating decoders (generic `utf8_to_utf16.h`, `utf8_to_utf32.h`,
`utf8_to_latin1.h`, `valid_utf8_to_latin1.h`) scan back from the end until
they have seen `K` leading bytes, then loop while
`pos + 64 + safety_margin <= size`.

`tk K xs` is the length of that suffix.

- **`cover`.** Suppose the output cursor lags `L` of the consumed input by
  `D`, and a call inside the block stores `s` units. If `s ≤ D` plus the
  leading bytes the call inspects plus `K`, the store fits.
- **`cover_aligned`.** A call that starts on a leading byte and produces `e`
  units needs `s ≤ e + 1 + K`. The `+ 1` is the leading byte that follows
  the call (`bigindex_exact`).
- **`coverL` / `reach_L`.** The same bound, paying with `L` of the inspected
  window rather than its leading-byte count. A 4-byte lead weighs 2 under
  `utf16_length_from_utf8` and pays 2. `K` is still a count of leading bytes.
- **The lag invariant (`Reach`, `reach_lag`).** `Reach` is the generic
  decoder loop as a transition system: ASCII blocks; entering a non-ASCII
  block, which is rejected when its first byte is a continuation byte
  (`utf8_continuation_mask & 1`); converting calls, which end just before a
  leading byte (`bigindex_exact`); error rows (12 bytes, no output); leaving
  the inner loop at `pos + 52`. `reach_lag` proves that the cursor `o`
  satisfies `o ≤ L` of the consumed input at block heads, and
  `o + [next byte is a continuation] ≤ L` inside a block. So a call that
  starts on a continuation byte (possible only after an error row) has one
  unit of slack, which pays for the unit it produces from that byte.
- **The calls are `Step`s (`Utf8Table.lean`).** `Reach` takes each call as a
  `Step`; these theorems derive that from the extracted tables, for every
  call shape of `convert_masked_utf8_to_*`:
  - `table_step`: a table-driven call that builds the 12-bit
    end-of-code-point mask (`emask`, `bit_emask`), looks up `utf8bigindex`,
    consumes `bigConsumed` and produces one unit per code point (UTF-32,
    Latin-1, and UTF-16 rows below 145). Facts: `bigindex_exact`,
    `bigconsumed_le`.
  - `table_step16`: a UTF-16 row with three code points, where a lane may
    write a surrogate pair. A pair needs a top byte `≥ 0xF0`:
    `pair_needs_f0` for the `composed > 0x3c00000` test of westmere, haswell
    and ppc64, `pair_needs_f0_arm` for the `& 0xf8000000` test of arm64, lsx
    and lasx. `shuf3_starts` checks on the `shufutf8` table that the top byte
    is the first byte of its code point, which `utf16_length_from_utf8` then
    counts twice.
  - `count_step`: the mask fast paths (one unit per code-point end).
  - `step_888`: the three-4-byte-character path of arm64, lsx and lasx,
    which advances only after checking the three leads.
- **`reach_store` / `reach_okK`.** In a reachable state, a store fits when
  `q.f ≤ cont + cnt` of the inspected window and the path is on an `okK`
  list. The width `q.s` is the transcribed path. `Utf8Table.lean` discharges
  the budget `q.f` for these shapes:
  - `reach_count`: `p` leads in bytes `t + 1 .. t + c`, and `q.f ≤ p + 1`.
    `reach_fff` is `0xfff` (12 leads, 16 stored, `K = 3`), specialized as
    `reach_x86_utf16_fff`, `reach_x86_utf32_fff`, `reach_x86_latin1_fff`.
    `reach_924` is `0x924` (4 leads, 8 stored); `reach_x86_utf16_924` is the
    UTF-16 case.
  - `reach_row`: a unit-weight table row. The budget is `units + 1`,
    including a call that starts on a continuation byte.
  - `reach_arm_888`: arm64 writes 6 units on `0x888` before the `F0` check.
    The mask gives three leads; with `cont` the budget is 4, and `K = 2`
    covers the store (`budget_888`).
  - `reach_lsx_888`: the check returns before the store, and the store is 6
    units, which is `L` of the 12 consumed bytes (`reach_L`, `window_888`).
  - `reach_lasx_888`: the check returns before the store, then `__lsx_vst`
    writes 8 units. `L` of the 13-byte window is at least 7, so `K ≥ 1`
    covers it (`reach_L`). `K = 3` is what the x86 and POWER `0xfff` and
    `0x924` paths require.
- **`okK K paths`.** The list of `(stored, e + 1)` pairs of each kernel's
  `convert_masked_utf8_to_*` meets the condition above. `okK m* = true` and
  `okK (m* - 1) = false` give the optimum on writes.
- **`read_ok` / `read_K1_fails`.** A 16-byte load at `pos + 51` stays in the
  input when `K ≥ 2`. With `K = 1` the loop test does not guarantee it: on 66
  ASCII bytes it admits a block at `pos = 0`. That is a bound on the loop test
  only; on that input the C++ takes the ASCII path, which does not load at
  `pos + 51`. `ValidLatin1.la_read_K1` gives the reachable call: on 66 bytes
  of valid Latin-1-representable UTF-8, `K = 1` admits the block and `K = 2`
  rejects it, and the lsx and lasx UTF-32 and Latin-1 kernels call at byte
  51 (`Reach` and `VReach` derivations, rows `TableFacts.lv_rows`), whose
  16-byte load reads byte 66, past the input. The negative controls with
  `K = 1` on LoongArch show the overread in the C++.

| header | K before | write optimum | now (with reads) | Lean |
|---|---|---|---|---|
| `utf8_to_utf16.h` | 8 | 3 (x86, ppc64, lsx, lasx); 2 (arm64) | 3; arm64 2 | `K_x86_utf16`, `K_la_utf16`, `K_arm_utf16`, `Arm64.hdr_utf8to16` |
| `utf8_to_utf32.h` | 8 | 3 (x86, ppc64, arm64); 1 (lsx, lasx) | 3; lsx, lasx 2 | `K_x86_utf32`, `K_la_utf32`, `Arm64.K_arm_utf32`, `Lsx.hdr_utf8to32`, `Lasx.hdr_utf8to32` |
| `utf8_to_latin1.h` | 16 (`convert`), 8 (`_with_errors`) | 3 (x86, ppc64, arm64); 0 (lsx, lasx) | 3; lsx, lasx 2 | `K_x86_latin1`, `Arm64.K_arm_latin1`, `Lsx.hdr_utf8toLatin1`, `Lasx.hdr_utf8toLatin1` |
| `valid_utf8_to_latin1.h` | 8 | 3 | 3; lsx, lasx 2 | same lists, through `ValidLatin1.vreach_reach`; `Lsx.hdr_valid_utf8toLatin1`, `Lasx.hdr_valid_utf8toLatin1` |

The headers are compiled once per kernel, and each selects its constant for
the build target: `#if defined(SIMDUTF_IS_ARM64)` in `utf8_to_utf16.h`,
`#if defined(SIMDUTF_IS_LSX) || defined(SIMDUTF_IS_LASX)` in
`utf8_to_utf32.h`, `utf8_to_latin1.h` and `valid_utf8_to_latin1.h`. On arm64 and LoongArch only those
kernels compile the headers, so the value is the least that covers that
target's stores and the 16-byte loads (`K ≥ 2`, `read_ok`). The x86 and
POWER kernels keep `K = 3`. `valid_utf8_to_latin1.h` keeps `K = 3` on x86,
POWER and arm64 and uses `K = 2` on LoongArch; its loop has no
`continuation_mask & 1` test, so §3.2 proves it separately on the input its
contract allows.
- `fff_tight` (x86) and `Arm64.arm_fff_tight` give `K = 2` failing on the
  `0xfff` path (x86; arm64 UTF-32 and Latin-1).
- `Arm64.arm_888_tight` gives `K = 1` failing on arm64 UTF-16.
- `la_924_tight` gives `K = 2` failing on LoongArch UTF-16 (lsx and lasx):
  a valid 68-byte input, the calls of the kernel up to byte 51 as a `Reach`
  derivation (the `bigindex` row at byte 44 is `TableFacts.la_924_rows`),
  then the `0x924` call at 51 stores 8 units ending one past `L`. The
  calls take the same paths on lsx and lasx.
- `read_K1_fails` shows that with `K = 1` the loop test no longer guarantees
  the loads, and `ValidLatin1.la_read_K1` gives an input, valid for all three
  headers, on which the kernels call at byte 51 and load past the input with
  `K = 1`. That read is what keeps lsx and lasx at 2; the LoongArch `K = 1`
  controls fault.
- `ValidLatin1.lv_tight_K2` gives `K = 2` failing for
  `valid_utf8_to_latin1.h` on x86, arm64 and POWER, on 67 bytes the contract
  allows: a `VReach` derivation of the kernels' calls to byte 51 (table rows
  of six ASCII bytes, then rows of 12, 7 and 8 bytes) with the output at `L`
  of the consumed bytes, then the `0xfff` path, which stores 16 bytes and
  ends one past `L`. The kernels compare the whole remaining mask with
  `0xfff`, so that path runs only on the last 12 bits of a block. The three kernels have the same store paths
  (`Arm64.arm_latin1_eq_x86`, `Ppc64.ppc_latin1_eq_x86`).

### 3.1 The ASCII tail (`Validator.lean`, `Exact.lean`)

`convert_ascii_tail` ends with a 64-unit store that starts up to 63 units
*before* the cursor. An upper bound on the cursor is not enough here: the
cursor must equal `L` of the consumed input. Without the check this fails.
For example, `E0 80 80 80 80` followed by ASCII hits an error row (mask
`0xFF0`: 12 bytes consumed, no output). The proof therefore models the check.

- **The check.** `Validator.sc` and `err` transcribe `check_special_cases`
  (its three lookup tables) and `check_multibyte_lengths`, with both
  versions of `must_be_2_3_continuation` (`|` on x86 and ppc64, `^` on
  arm64, lsx and lasx). `pair_ok` checks all byte pairs.
- **Character structure (`char_shape`, `cont_of_lead`).** In a block that
  passed, a lead of length `n` is followed by exactly `n - 1` continuation
  bytes, a 4-byte lead is at most `0xF4`, and `0xF0` is followed by at
  least `0x90`. `no4`: four consecutive continuation bytes never pass.
- **Exact calls.** Every call in a checked block consumes `c` bytes ending
  just before a lead, never takes an error row, and advances by exactly `L`
  of the consumed bytes (`XStep`):
  - `exact_w01`: UTF-32 and Latin-1, table rows (`well_noerr`).
  - `exact_utf16_145`: UTF-16 rows below 145, which never hold a 4-byte
    character (`well_145`).
  - `exact_utf16_3`: UTF-16 rows with three code points, for both pair
    tests. A lane writes a pair exactly when its character has 4 bytes:
    `well_3` checks how `shufutf8` gathers the lanes, and `pair4` checks
    that the tests fire on every valid 4-byte character.
  - `exact_count`: the mask fast paths.
  - `exact_888`: the arm64/lsx/lasx `0x888` path, whose lead check passes.
- **The tail (`XReach`, `xreach_exact`, `ascii_tail_safe`).** On checked
  blocks the loop keeps the cursor at exactly `L`. `tail_block` covers the
  tail's exact 64-byte stores. `tail_last` shows that its last, backward
  store starts inside the buffer and ends at `L xs`.
- **In-loop stores on unit weights (`xreach_store`, `xreach_store_w01`,
  `xreach_okK`).** A checked block whose weights are 0 or 1 (UTF-32 and
  Latin-1, `w01`) has its cursor at `L` (`xreach_exact`). A call there with
  `s ≤ p + 1 + K` fits. A UTF-16 4-byte lead weighs 2; those stores are
  `reach_count`, `reach_arm_888` and `reach_lasx_888`.
  `valid_utf8_to_latin1.h` is outside this theorem: the loop has no
  `continuation_mask & 1` test, and the tail is scalar `convert_valid`.
  §3.2 covers it.

### 3.2 `valid_utf8_to_latin1.h` (`ValidLatin1.lean`)

The contract of `convert_valid_utf8_to_latin1` requires valid UTF-8 that
Latin-1 can represent: every character is an ASCII byte, or `0xC2`/`0xC3`
followed by one continuation byte (`LV`). The loop does not reject a block
that starts on a continuation byte, and the tail is scalar.

- **`VReach`** is the loop: `Reach` without the test on the first byte of a
  block, with only the loop test (`t + 64 ≤ length`) and with calls that end
  just before a leading byte.
- **`vreach_reach`.** On `LV` input every state of `VReach` is a state of
  `Reach`, because every block head is a leading byte: the input starts with
  one (`LV.first`), an ASCII byte is never followed by a continuation byte
  (`LV.after_ascii`), and every call ends just before a leading byte.
- **`table_conv`.** On `LV` input no table call takes an error row: two
  continuation bytes are never adjacent (`LV.no2`), so the 12-bit mask is well
  shaped and `well_noerr` applies; the call then ends just before a leading
  byte (`row_conv`). The fast paths are `count_step`. The `0x888` path needs
  three adjacent continuation bytes and cannot occur.
- **Stores.** `valid_latin1_okK`, `valid_latin1_row` and `valid_latin1_fff`
  are `reach_okK`, `reach_row` and `reach_x86_latin1_fff` through
  `vreach_reach`, with the same Latin-1 lists and `K = 3`
  (`Utf8.K_x86_latin1`, `Arm64.K_arm_latin1`, `Ppc64.K_ppc_latin1`). The lsx
  and lasx lists need `K = 0`, and the 16-byte loads need `K ≥ 2`
  (`read_ok`), so LoongArch uses `K = 2`: `Lsx.hdr_valid_utf8toLatin1` and
  `Lasx.hdr_valid_utf8toLatin1` give the stores and the loads at `K = 2`.
- **`valid_latin1_tail`.** The scalar tail writes one byte per leading byte,
  `L` of the rest, after a cursor at most `L` of the consumed bytes.

## 4. `convert_valid` on valid UTF-8 (`ValidDynamic.lean`, `ValidHybrid.lean`)

`valid_utf8_to_utf16.h` and `valid_utf8_to_utf32.h` used a fixed
`safety_margin` of 16 bytes. They now run their block loop on a smaller fixed
margin. After the loop, except on arm64 for UTF-16 and on LoongArch for
UTF-32, they scan back once from the end for 3 leading bytes (the dynamic
margin of §3) and convert one more block if it admits it; fewer than 64
bytes then remain, and the scalar tail finishes.

| header | target | fixed margin | extra block |
|---|---|---|---|
| `valid_utf8_to_utf16.h` | westmere, haswell, ppc64, lsx, lasx | 9 | `K = 3` |
| `valid_utf8_to_utf16.h` | arm64 | 3 | none |
| `valid_utf8_to_utf32.h` | westmere, haswell, ppc64, arm64 | 12 | `K = 3` |
| `valid_utf8_to_utf32.h` | lsx, lasx | 4 | none |

- **The loop.** `ValidHybrid.head` decides whether a block runs: the fixed
  test in the loop, then once the test of the extra block
  (`pos + 64 + 4 <= size`, the scan, the dynamic test), then nothing. `HX`
  is the loop model `ValidExact.VX` with that test at each block head.
  `hx_vx`: every block it runs passes the fixed or the dynamic test.
  `ValidDynamic.scanLoop` transcribes the backward scan, and
  `safetyMargin_eq` shows that it is `tk K` of the weights plus 1.
- **Stores.** On valid input the cursor is exact and every call starts on a
  leading byte (`ValidExact.vx_inv`, `vx_after_call`). A call that produces
  `p` units stores at most `p + d`: `d = 4` on x86, POWER and LoongArch for
  UTF-16 and on x86, POWER and arm64 for UTF-32; `d = 2` on arm64 for UTF-16
  and on LoongArch for UTF-32. In a block the fixed margin `m` admitted, the
  `1 + m` bytes after the call are complete characters
  (`ValidExact.tail_expand`), one UTF-16 unit per 3 bytes and one code point
  per 4, so the stores need `3 d ≤ m + 3` (`fixed_store16`: 9 and 3) or
  `4 d ≤ m + 4` (`fixed_store32`: 12 and 4). In the extra block, the leading
  byte after the call and the `K` leading bytes after the block pay one unit
  each, so `K ≥ d - 1 = 3` (`ValidDynamic.dyn_store`).
- **Loads.** The 16-byte loads at `pos + 51` need a fixed margin of at least
  3 or `K ≥ 2` (`hyb_load`, `ValidDynamic.dyn_load`).
- **End to end.** `hyb_utf16_safe` and `hyb_utf32_safe`: on valid input,
  with each target's constants, every store (calls, ASCII blocks, the scalar
  tail, `Scalar.valid8to16_le`, `valid8to32_le`) is within `L`, and every
  load inside the input. `constants` checks that each value is the least the
  stores and loads allow.

One less fails on a valid input, through `HX`: with the lowered constant
`head` runs the block at 0; with the header's constants it runs no block
there, the extra block included. A block that a lowered fixed margin runs and
that overflows necessarily fails the dynamic test with `K = 3`
(`dyn_store`), so the extra block cannot hide those failures; the fixed
witnesses state it. The calls of each kernel to byte 51 are those of
`ValidDynamic`.

| control | targets | input | failure |
|---|---|---|---|
| `fix16_x86`: fixed 8 | westmere, haswell, ppc64 UTF-16 | `v16fix`, 72 bytes | `0xfff` at 51: 16 units for 12, one past `L` |
| `k16_x86`: `K = 2` | westmere, haswell, ppc64 UTF-16 | `lvTight`, 67 bytes | same |
| `fix32_x86`: fixed 11 | westmere, haswell, ppc64, arm64 UTF-32 | `v32fix`, 75 bytes | same |
| `k32_x86`: `K = 2` | westmere, haswell, ppc64, arm64 UTF-32 | `lvTight` | same |
| `fix16_la`: fixed 8 | lsx, lasx UTF-16 | `la16fix`, 72 bytes | `0x924` at 51: 8 units for 4 |
| `k16_la`: `K = 2` | lsx, lasx UTF-16 | `la924b`, 68 bytes | same |
| `fix32_la`: fixed 3 | lsx, lasx UTF-32 | `la32fix`, 67 bytes | `0xaaa` at 51: 8 units for 6 |
| `fix16_arm_read`: fixed 2 | arm64 UTF-16 | `laRead`, 66 bytes | the call at 51 loads byte 66 |

The inputs, in hex: `lvTight` is `41`×24, `C3 A9`×7, `41`×5, `C3 A9`×2,
`41`×16, `C3 A9 41 41`; `v16fix` and `v32fix` replace its last 4 bytes by
`E3 81 82`×3 and `F0 9F 98 80`×3. `la924b` is `41`×32, `E3 81 82`×4,
`C3 A9`×2, `41`, `C3 A9`, `E3 81 82`×5, `41 41`; `la16fix` replaces its
last 5 bytes by `E3 81 82`×3. `la32fix` is `41`×32, `C3 A9`×7, `41`×5,
`C3 A9`×6, `F0 9F 98 80`. `laRead` is `41`×32, `C3 A9`×7, `41`×20.
`ValidDynamic` also has the dynamic test alone, for any `K`: `utf16_x86_K2`,
`utf32_x86_K2`, `utf16_la_K2`, and `read_K1` (`K = 1` loads past the input
with the arm64 UTF-16 and LoongArch UTF-32 kernels, targets whose headers
have no dynamic block).

These functions require valid input. On invalid input no fixed margin bounds
them, because a tail of continuation bytes has `L = 0`. The same holds for
the icelake `valid_utf8_to_latin1`, which keeps unpaired continuation bytes.

## 5. Results: fixed-margin loops

`B` is the units per iteration; "strict" means a `>` test. The theorem names
are in `SafetyMargins.Arch.<Arch>`. `safe_latin1to8`, `safe_utf32to8`,
`safe_utf32to8_err` and `safe_utf32to16` are `safe_of_paths` at
`need B m*`. The two icelake `*_real` theorems bound the stores that the
model abstracts; they are stated after the table.

| conversion | architecture | B, strict | before | now (optimal) | Lean |
|---|---|---|---|---|---|
| UTF-16 → UTF-8 | westmere, haswell, arm64, ppc64, lsx, lasx | 16 | 12 | 12 | `m_/cur_/wit_utf16to8` |
| UTF-16 → UTF-8 | icelake (32 loaded, 31 consumed) | 31 | 1 | 1 | `m_/cur_utf16to8`, `safe_utf16to8`, `safe_utf16to8_real` |
| UTF-32 → UTF-8 | westmere plain, haswell, icelake, ppc64 | 16 | 12 | 12 | `m_/cur_/wit_utf32to8` |
| UTF-32 → UTF-8 | westmere `_with_errors` | 16 | 12 | 4 | `m_/ok_/wit_utf32to8_err`, `safe_utf32to8_err` |
| UTF-32 → UTF-8 | arm64, lsx | 16, strict | 12 | 3 | `m_/ok_/wit_utf32to8`, `safe_utf32to8` |
| UTF-32 → UTF-8 | lasx | 16, strict | 12 | 11 | `m_/ok_/wit_utf32to8`, `safe_utf32to8` |
| UTF-32 → UTF-16 | westmere plain (inline `>= 16 + 4`) | 16 | 8 | 4 | `m_/ok_/wit_utf32to16`, `safe_utf32to16` |
| UTF-32 → UTF-16 | haswell | 8 | 12 | 0 | `m_/ok_utf32to16`, `safe_utf32to16` |
| UTF-32 → UTF-16 | arm64 | 8 | 4 | 4 | `m_/cur_/wit_utf32to16` |
| UTF-32 → UTF-16 | westmere `_with_errors`, icelake, ppc64, lsx, lasx | 8 or 16 | 0 | 0 | `m_/cur_utf32to16*` |
| Latin-1 → UTF-8 | haswell, lasx | 16 | 12 | 8 | `m_/ok_/wit_latin1to8`, `safe_latin1to8` |
| Latin-1 → UTF-8 | westmere, ppc64 | 16 | 8 | 8 | `m_/cur_/wit_latin1to8` |
| Latin-1 → UTF-8 | arm64 | 16 | 8 | 0 | `m_/ok_latin1to8`, `safe_latin1to8` |
| Latin-1 → UTF-8 | lsx | 16 | 0 | 0 | `m_/cur_latin1to8` |
| Latin-1 → UTF-8 | icelake (`pos + 96 <= len`) | 64 | 64 | 32 | `m_/ok_/wit_latin1to8`, `safe_latin1to8` |
| UTF-32 → Latin-1 | lsx (16-byte store for 8 units) | 8 | 8 (in `>= 16`) | 8 | `m_/cur_/wit_utf32toLatin1` |
| Latin-1 → UTF-16/32, UTF-16/32 → Latin-1, change_endianness, utf16fix, to_well_formed | all others | — | 0 | 0 | `m_/cur_*` |

On icelake UTF-16 → UTF-8, `safe_utf16to8` bounds the model end `.ahead`,
which is `L` of the 32 loaded lanes. `safe_utf16to8_real` bounds the bytes
written past `L` of the 31 consumed units once that extra is `spill` of lane
31 (`ahead_spill`), the 2 bytes of a low surrogate carried out of lane 30
(`ahead_carry`), or 0. The margin stays 1. `wit_utf16to8` shows that 0
overflows: `realWritten8`, with the loop test of margin 0, runs the table's
`.ahead` path on 31 units ending with a high surrogate; a low surrogate past
the input makes the carry case write 2 bytes past `L`. `read_utf16to8`
shows that margin 0 reads past the input on every input that enters the
loop with 31 units, since each iteration loads 32: the read alone sets the
optimum 1.

For the loops with no margin:
- **UTF-16 → UTF-32** (low surrogates weigh 0) is safe on every architecture
  without a margin: `safe_utf16to32` or `utf16to32_safe`, through
  `safe_exact` or `safe_fits`. On icelake, `safe_utf16to32` bounds the model.
  The end of `.ahead` in that model is `L` of the 32 loaded lanes. The
  unmasked store writes 16 units. `safe_utf16to32_real` puts those 16 inside
  that end under the hypothesis `16 ≤ L` of the 32 lanes. `hsur_of_vzero`
  gives the hypothesis from `utf16to32_ahead` when that iteration runs only
  for `V == 0` (`vzero_noAdj`).
- **rvv:** every loop is `vl`-exact (`Rvv.safe_vla`).
- **icelake UTF-8 decoders**: `IcelakeUtf8.icelake_utf8_safe` (§8), from a
  transcription at the level of masks; `Icelake.safe_utf8` is the earlier
  abstract table.

Every loop now uses its optimal margin, for each target.

## 6. What is assumed

- **Path tables.** Lean does not parse C++. Every table is a transcription,
  as is the reading that the kernels index the lookup tables the way
  `TableFacts` and `table_step` describe. A missed path would be a missed
  store. The lookup tables, including the validator's three nibble tables
  from `utf8_lookup4_algorithm.h`, are extracted by `extract_tables.py`.
- **Which call shape each kernel branch is.** That a branch is a table
  lookup, a fast path, or the `0x888` path, and which pair test it uses, is
  read from the C++. So is the shape of its stores: the `okK` pairs, and the
  width `s` in `reach_count`, `reach_row`, `reach_arm_888`, `reach_lasx_888`
  and `xreach_okK`. Those theorems discharge the lead budget.
- **`convert_valid`.** `ValidDynamic` and `ValidHybrid` assume the deficit `s ≤ p + d` of each
  path, read from the C++ as the other call shapes, and that a call ends by
  `pos + 63`. `valid_utf8_to_latin1.h` is covered by §3.2, under its
  contract (`LV` input). The scalar tails are proved in `Scalar.lean` (§8).
- **Ice Lake real ends.** `safe_utf16to32_real` assumes the surrogate
  iteration runs only when `16 ≤ L` of the 32 loaded lanes.
  `safe_utf16to8_real` assumes each `.ahead` iteration's extra bytes are the
  spill, the low-surrogate carry, or zero. `IcelakeReal` proves both from a
  transcription of the loops on the unit values (§7).
- **Control flow around the check.** The ASCII tail is reached only after
  every converted non-ASCII block passed `check_utf8_bytes`: plain `convert`
  returns 0 on `errors()` first, and `convert_with_errors` tests `errors()`
  before converting each block. This is read from the C++.
- **Witnesses.** A `wit_*` is a weight list and one iteration. It shows that
  the arithmetic is tight, not that the C++ reaches that iteration on a
  concrete input. The UTF-32/UTF-16/Latin-1 witnesses and `arm_fff_tight`,
  `arm_888_tight` were confirmed with ASan on arm64 and x86, on buffers of
  exactly `L` units, with the margin lowered by one. The icelake UTF-16 →
  UTF-8 witness (`wit_utf16to8`) relies on the unit after the input being a
  low surrogate, which no test controls; margin 1 there is also what the
  32-unit load needs (`read_utf16to8`).
- **`utf16fix`, loads, with replacement.** Modelled in §7, from
  transcriptions of the C++.

## 7. Loads, `utf16fix`, replacement, exact cursors, Ice Lake lanes

- **Loads (`Loads.lean`).** For each fixed-margin loop whose margin was
  lowered (UTF-32 → UTF-8 on arm64, lsx, lasx and westmere `_with_errors`;
  UTF-32 → UTF-16 on westmere and haswell; Latin-1 → UTF-8 on haswell, lasx,
  arm64 and icelake), every vector load of an iteration, as (offset, width),
  with its file:line. `*_tied` states, under the same `need B m strict`, the
  store theorem, the load bound and the bound of the scalar fallback
  (`forward = end - buf - 1`). `lower_fits`: one less margin still fits the
  loads, so the stores set those optima; `zero_margin_tight`: where the
  margin is 0, the load ends exactly at `B`.
- **`utf16fix` (`Utf16Fix.lean`).** Every load and store of every kernel
  (westmere, haswell, icelake, arm64, lsx, lasx, rvv, scalar), including
  `in[-1]`, `out[-1]` and the final overlapping block, lies in `[0, n)` for
  every `n` (`*_inBounds`).
- **With replacement (`Replacement.lean`).** UTF-16 → UTF-8: the driver
  passes the whole rest of the input to the kernel, which stops at the first
  unpaired surrogate; `via_inv` and `via_paths` bound every store by the
  replacement length (`countRepl`, a transcription of
  `utf8_length_from_utf16_with_replacement`). UTF-8 → UTF-16: every store
  path overshoots its output by at most 4 units (`gap_all`, tight:
  `gap_tight`); a byte-level transcription of the scalar decoder with
  replacement gives at least `⌈len/3⌉` units (`count_ge`), so the 32
  held-back bytes give at least 11 (`count_ge_11`, attained by
  `tight32_count`), and `bulk_store` bounds every store of the vector part.
  The hypotheses of this file are discharged below
  (`ReplacementMore`, `ReplacementRest`, `ValidatorSound`).
- **`convert_valid` (`ValidExact.lean`).** Valid UTF-8 is the automaton of
  Unicode Table 3-7. A 64-byte block of valid UTF-8 that starts on a
  character boundary passes `check_utf8_bytes` (`checkedAt_of_valid`, through
  the exhaustive `closure`), so every call of `Exact.lean` is exact; by
  induction over the loop (`vx_inv`) the cursor is `L` of the consumed bytes
  at a character boundary; `ValidDynamic` and `ValidHybrid` use it for the
  store bounds of §4.
- **Ice Lake (`IcelakeReal.lean`).** Both UTF-16 loops as functions of the 32
  lane values and the carry (both endiannesses). `utf16to32_hsur` and
  `utf16to8_hreal` are the data hypotheses of `safe_utf16to32_real` and
  `safe_utf16to8_real`, proved for every lane vector;
  `safe_utf16to32_kernel` and `safe_utf16to8_kernel` bound the whole loops
  with no data hypothesis.
- **All other loads (`LoadsAll.lean`).** Every other fixed-margin loop of
  the path tables (76 loop definitions over westmere, haswell, icelake,
  arm64, ppc64, lsx and lasx): every vector load, every clamped scalar read
  and every read before the cursor fits the units its loop test guarantees
  (`all_fit`). Two loops read past what they consume, and the read sets
  their margin: the icelake UTF-16 → UTF-8 loop (`read_utf16to8`) and the
  icelake fixed-length UTF-8 decoders, whose `+ 4` is tight
  (`icelake_utf8_fixed_tight`). In the 8-unit UTF-16 → UTF-32 loops only the
  `end - buf - 1` clamp keeps the scalar fallback in the input
  (`clamp_needed`).
- **The rest of the replacement conversions.** `ReplacementMore`:
  `maximal_subpart` (`subpart_of_none`), the scalar and vector length
  functions with replacement equal the models (`lenRepl_eq`,
  `bytemask_eq`, `armLen_eq`), the scalar kernel reports the first unpaired
  surrogate (`sc8_spec`). `ReplacementRest`: the icelake length function
  (`iceLen_eq`), the window of the UTF-8 → UTF-16 driver ends before a
  leading byte, so an error inside it is an error of the whole input
  (`window_error`), the whole entry point stays in the buffer
  (`convertRepl_safe`, `tr_safe`), and the `_safe` UTF-16 → UTF-8 entry
  points never write past their capacity (`replSafe_safe`, `crs_safe`).
  `ValidatorSound`: the SIMD validator, over whole inputs with the carry
  between blocks and `check_eof`, flags nothing exactly on valid UTF-8
  (`flag_iff`); the vector `validate_utf8_with_counts` meets the
  specification the driver needs (`vecV_spec`), and the UTF-8 → UTF-16
  driver with replacement is exact end to end (`transcode_exact`).
- **rvv (`RvvExact.lean`).** Every rvv kernel (base64 excluded), on the lane
  values and any `vl` with `1 ≤ vl ≤ n`: no store ends past the length its
  function gives (`*_safe`). The reparse step of the UTF-8 decoders needs
  two leading bytes among the consumed bytes (`utf8_tail`); valid input and
  the validating prologue guarantee it. On input that breaks the contract of
  `convert_valid_utf8_to_utf16`/`utf32`, it can move before the output and
  read `dst[-1]` outside it.
- **Differential tests.** The models of `Replacement`, `IcelakeReal` and
  `ValidExact` agree with the C++ on 200,000 random inputs on arm64, haswell
  and icelake (the article's `bench/difftest`).

What remains read from the C++ after §7: the transcriptions themselves
(path tables, call shapes, load offsets, lane arithmetic), the RVV
semantics of `vsetvl`, `vcompress` and `vcpop`. §8 proves the scalar code,
the length functions, `neon_counter`, the contracts of the replacement
drivers on the generic kernels, and the remaining loads.

## 8. The scalar code, the remaining contracts, the remaining loads

- **Scalar code (`Scalar.lean`).** `Basic.written` charges the code that
  finishes a conversion `L` of the units it is given (fuel `0`, and an
  iteration with fewer than `N` units). `run` is a scalar loop as a step
  function, and `run_le`: if every step writes at most `L` of what it
  consumes, the loop writes at most `L` of its input, for any fuel. Each
  converter of `include/simdutf/scalar/` is transcribed with its fast path:
  - UTF-8 sources (`utf8to16_le`, `utf8to32_le`, `utf8toL_le`,
    `valid8to16_le`, `valid8to32_le`, `valid8toL_le`), on every byte string,
    the `convert_valid` ones included. The 16- and 8-byte ASCII tests on the
    64-bit words that `memcpy` reads hold only on ASCII bytes
    (`probe16_ascii`, `probe8_ascii`, through `le64_and_gen`).
  - UTF-16 sources (`utf16to8_le`, `utf16to32_le`, `utf16toL_le`), on every
    input and for any fast-path test; `convert_valid` (`valid16to8_le`,
    `valid16to32_le`) on valid UTF-16, its contract, in both byte orders: the
    4-unit test, with the rotation of the other byte order, holds only on
    units below 0x80 (`probeU16_ascii`). On invalid input `convert_valid`
    can write more (`valid_16to8_over`).
  - UTF-32 and Latin-1 sources (`utf32to8_le`, `utf32to16_le`,
    `utf32toL_le`, `latin1to8_le`, `oneEach_run`), on every input, with the
    32-bit masks as arithmetic (`and_high`).
  - `rewind_and_convert_with_errors` (`rewind_le`, `rewind_total`,
    `rewind_rest`): it writes at most `L` of the bytes from the leading byte
    it steps back to, and at most `L` of the bytes from the cursor when the
    byte there is not a continuation byte or the byte before it is ASCII.
  - The scalar fallbacks inside the vector loops (`runK`,
    `fallback_scalar`): the 54 loops listed in the file are iterations of
    `Kernels.scalar k` writing at most `L` of what they consume
    (`fbA_scalar`, `fbB_scalar`, `fbC_scalar`, `fbD_scalar`, `fbL_scalar`).
  - `writtenT_le`, `safe_of_paths_tail`: with a tail within `L`, the whole
    conversion stays within `Basic.written`, hence within `L`.
- **The contracts of the replacement drivers (`ReplacementClose.lean`).**
  `BSpec`, `CVSpec`, `VKSpec` and `VSpec2` are proved for the kernels built
  from src/generic (westmere, haswell, arm64, ppc64, lsx, lasx), from the
  loop models. End to end: `convertRepl_generic`, `trRun_generic`,
  `replSafe_generic`, `dSafe_generic`, and the two entry points not modelled
  before, `pSafe_generic` (`convert_utf16_to_utf8_safe`) and `latin1Safe`
  (`convert_latin1_to_utf8_safe`). Icelake and rvv are not discharged for
  these contracts.
- **Loads (`LoadsRest.lean`).** The rvv loops (`rvv_all`, with the
  look-ahead of the UTF-8 loops tight, `rvv_utf8_tight`), the backward scans
  (`walk_errors`, `walk_counts`, `rewind_convert_in`), the validators
  (`validators_fit`, the AVX-512 masked heads and tails) and the replacement
  drivers (`culCalls_in` for every list of offsets, `tr_calls_in`,
  `chunk_in`, `latin1_safe_in`) read only inside the input.
- **Length functions (`Lengths.lean`, `LengthsGeneric.lean`,
  `LengthsArch.lean`, `LengthsIcelake.lean`, `LengthsRvv.lean`,
  `LengthsNeon.lean`, `LengthsSpec.lean`).** The documentation calls the
  result of a length function on ill-formed input implementation defined.
  Every ordinary length function, scalar, generic and per-kernel (westmere,
  haswell, icelake, arm64, ppc64, lsx, lasx, rvv for every `vl`), returns the
  per-unit sum `L (xs.map w)` on every input (`LengthsSpec.lengths_all`,
  `Lengths.scalar_lengths`, `LengthsRvv.rvv_lengths`), with the lane counters
  at their real widths and their flushes (no overflow), and arm64's
  `neon_counter` sums the generic counts (`LengthsNeon.neon_counts`). Two
  exceptions, both over-estimates on ill-formed input (`exceptions_ge`):
  arm64 `utf8_length_from_utf16` adds 2 when the length is a multiple of 16,
  at least 32, and the last unit is a high surrogate (`arm8of16_eq`,
  src/arm64/arm_convert_utf16_to_utf8.cpp:631-633); icelake below 33 units
  counts an unpaired surrogate as 3 (`ice8of16_eq`,
  src/icelake/icelake_utf8_length_from_utf16.inl.cpp:7-12). A larger buffer is
  safe, so the store theorems hold for buffers sized by any of them.
- **No length hypothesis (`ReplacementLengths.lean`).** `SLSpec` is false
  for arm64 and icelake; the drivers need it only on prefixes with no
  unpaired surrogate (`SLSpec'`), which holds for every kernel, since the
  vector kernels report the first unpaired surrogate (`VKSpec'`).
  `replSafe_kernels`, `dSafe_kernels`, `convertRepl_kernels` and
  `trRun_kernels` instantiate each generic kernel with its own length
  function.
- **Ice Lake UTF-8 decoders (`IcelakeMask.lean`, `IcelakeUtf8.lean`,
  `IcelakeUtf8Fixed.lean`).** `process_block_utf8_to_utf16`, the fixed-length
  UTF-32 decoders and both Latin-1 kernels transcribed on 64-byte blocks and
  64-bit masks (popcount, lzcnt, pdep, pext), with their drivers, scalar
  tails and the rewind. `icelake_utf8_safe` collects the results:
  `convert` and `_with_errors` of the three conversions keep every store
  within `L` of the input and every load inside it on every input
  (`conv16_safe`, `conv16err_safe`, `conv32_safe`, `conv32err_safe`,
  `convL1_safe`, `convL1err_safe`); `convert_valid` on valid UTF-8
  (`conv16valid_safe`; `conv32valid_safe` holds on every input) and on
  valid Latin-1-representable UTF-8 (`convL1valid_safe`; valid UTF-8 alone
  is not enough, `latin1_valid_needs_LV`). The block check is the structure
  check of the kernel (`block16_core`, `pot`), and a valid block passes it
  (`struct_of_valid`); the UTF-32 and Latin-1 proofs hold for every verdict
  of the AVX-512 validator, so they need no proof of it. The `TOO_LONG`
  test before the UTF-16 rewind is needed: without it the rewind would
  decode a 4-byte character twice and write 2 units past the buffer
  (`guard_needed`). `LoadsAll.icelake_utf8_block_tied` and
  `icelake_utf8_fixed_tied` are superseded by these load bounds.
