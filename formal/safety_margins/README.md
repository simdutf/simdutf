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
(`TableFacts.lean`), the validator on all byte pairs (`Validator.pair_ok`),
and the pair tests on all valid 4-byte characters (`Exact.pair4`).

## Files

| file | content |
|---|---|
| `SafetyMargins/Basic.lean` | the model and the two safety theorems |
| `SafetyMargins/Kernels.lean` | path tables shared by several architectures |
| `SafetyMargins/Utf8.lean` | the dynamic margin of the UTF-8 decoders, and `convert_valid` on valid input |
| `SafetyMargins/Validator.lean` | the UTF-8 check `check_utf8_bytes`, and the character structure of a block that passes it |
| `SafetyMargins/Exact.lean` | exact output on checked blocks, the in-loop store on unit weights, and the ASCII tail |
| `SafetyMargins/Utf8Table.lean` | the decoder's per-call facts, derived from the `utf8bigindex` and `shufutf8` tables and the lane arithmetic |
| `SafetyMargins/Tables.lean` | lookup tables extracted from `src/tables/` by `extract_tables.py`; don't edit it by hand |
| `SafetyMargins/TableFacts.lean` | facts about those tables |
| `SafetyMargins/Arch/*.lean` | one file per architecture: every loop, its path table, its margins |

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
    covers it (`reach_L`). The header's `K = 3` is what `0xfff` and `0x924`
    require.
- **`okK K paths`.** The list of `(stored, e + 1)` pairs of each kernel's
  `convert_masked_utf8_to_*` meets the condition above. `okK m* = true` and
  `okK (m* - 1) = false` give the optimum on writes.
- **`read_ok` / `read_K1_fails`.** A 16-byte load at `pos + 51` needs
  `K ≥ 2`.

| header | K before | write optimum | Lean |
|---|---|---|---|
| `utf8_to_utf16.h` | 8 | 3 (x86, ppc64, lsx, lasx); 2 (arm64) | `K_x86_utf16`, `K_la_utf16`, `K_arm_utf16` |
| `utf8_to_utf32.h` | 8 | 3 (x86, ppc64, arm64); 1 (lsx, lasx) | `K_x86_utf32`, `K_la_utf32`, `Arm64.K_arm_utf32` |
| `utf8_to_latin1.h` | 16 (`convert`), 8 (`_with_errors`) | 3 (x86, ppc64, arm64); 0 (lsx, lasx) | `K_x86_latin1`, `Arm64.K_arm_latin1`, `Lsx.K_utf8toLatin1`, `Lasx.K_lasx_latin1` |
| `valid_utf8_to_latin1.h` | 8 | 3 | same lists |

The headers are shared, so one constant must cover every architecture. With
reads, `K = 3` suffices everywhere, and every header now uses `K = 3`.
- `fff_tight` (x86) and `Arm64.arm_fff_tight` give `K = 2` failing on the
  `0xfff` path.
- `Arm64.arm_888_tight` gives `K = 1` failing on arm64 UTF-16.

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

## 4. `convert_valid` on valid UTF-8 (`Utf8.lean`)

`valid_utf8_to_utf16.h` and `valid_utf8_to_utf32.h` use a fixed
`safety_margin` (16 before, now 9 and 12). On valid input the cursor is exact, and the bytes after
the last call (it ends by `pos + 63`) are at least `1 + m` bytes of complete
characters.
- **UTF-16.** A character of at most 3 bytes gives one unit
  (`bytes_le_3u16`). A deficit of 4 units therefore needs `m ≥ 9`
  (`valid_utf16`). `valid_utf16_tight` shows three U+0800 in 9 bytes give
  only 3 units.
- **UTF-32.** A character of at most 4 bytes gives one unit. A deficit of 4
  therefore needs `m ≥ 12` (`valid_utf32`, `valid_utf32_tight`).

`valid_utf16` and `valid_utf32` count units. The store forms
(`overshoot_le`) give `s ≤ L (pre ++ expansion)` once the store ends at
most `d` past `L pre` and the unread tail is the expansion of those
characters. The cursor and the tail are read from the C++.
`length_expand16` and `length_expand32` prove that the expansion has
`bytes ks` bytes.
- `valid_utf16_store`: deficit 4, `m ≥ 9`, tail `expand16`. Shared header.
- `valid_utf32_store`: deficit 4, `m ≥ 12`, tail `expand32`. Shared header.
- `Arm64.valid_utf16_arm_store`: deficit 2, `m ≥ 3`.
- `Lsx.valid_utf32_lsx_store`, `Lasx.valid_utf32_lasx_store`: deficit 2,
  `m ≥ 4`. The shared UTF-32 header stays at 12, and the shared UTF-16
  header stays at 9, because the other architectures overshoot by 4.

| header | before | needed per architecture | now |
|---|---|---|---|
| `valid_utf8_to_utf16.h` | 16 | 9 (`valid_utf16_store`); arm64 3 (`Arm64.valid_utf16_arm_store`) | 9 |
| `valid_utf8_to_utf32.h` | 16 | 12 (`valid_utf32_store`); lsx, lasx 4 (`Lsx.valid_utf32_lsx_store`, `Lasx.valid_utf32_lasx_store`) | 12 |

These functions require valid input. On invalid input no fixed margin bounds
them, because a tail of continuation bytes has `L = 0`.

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
(`ahead_carry`), or 0. The margin stays 1.

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
- **icelake UTF-8 decoders** store exact counts (`Icelake.safe_utf8`).

Every loop now uses its optimal margin.

## 6. What is assumed

- **Path tables.** Lean does not parse C++. Every table is a transcription,
  as is the reading that the kernels index the lookup tables the way
  `TableFacts` and `table_step` describe. A missed path would be a missed
  store. The validator's three nibble tables (`Validator.byte1High`,
  `byte1Low`, `byte2High`) are copied by hand from
  `utf8_lookup4_algorithm.h`; the other tables are extracted by
  `extract_tables.py`.
- **Which call shape each kernel branch is.** That a branch is a table
  lookup, a fast path, or the `0x888` path, and which pair test it uses, is
  read from the C++. So is the shape of its stores: the `okK` pairs, and the
  width `s` in `reach_count`, `reach_row`, `reach_arm_888`, `reach_lasx_888`
  and `xreach_okK`. Those theorems discharge the lead budget.
- **`convert_valid` tails.** `valid_utf16_store`, `valid_utf32_store`,
  `Arm64.valid_utf16_arm_store`, `Lsx.valid_utf32_lsx_store` and
  `Lasx.valid_utf32_lasx_store` assume the cursor equals `L` of the consumed
  prefix and the unread tail is the expansion of complete characters. Both
  are read from the C++. `valid_utf8_to_latin1.h` is not among them: it has
  no `continuation_mask & 1` test, and its tail is scalar.
- **Ice Lake real ends.** `safe_utf16to32_real` assumes the surrogate
  iteration runs only when `16 ≤ L` of the 32 loaded lanes.
  `safe_utf16to8_real` assumes each `.ahead` iteration's extra bytes are the
  spill, the low-surrogate carry, or zero.
- **Control flow around the check.** The ASCII tail is reached only after
  every converted non-ASCII block passed `check_utf8_bytes`: plain `convert`
  returns 0 on `errors()` first, and `convert_with_errors` tests `errors()`
  before converting each block. This is read from the C++.
- **Witnesses.** A `wit_*` is a weight list and one iteration. It shows that
  the arithmetic is tight, not that the C++ reaches that iteration on a
  concrete input. The UTF-32/UTF-16/Latin-1 witnesses and `arm_fff_tight`,
  `arm_888_tight` were confirmed with ASan on arm64 and x86, on buffers of
  exactly `L` units, with the margin lowered by one. The `ahead` loops have
  no witness: margin 1 there is also what the 32-unit load needs.
