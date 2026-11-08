import SafetyMargins.LoadsAll
import SafetyMargins.RvvExact
import SafetyMargins.ReplacementRest

/-!
# The input loads not covered by `Loads.lean`, `LoadsAll.lean` and `Utf16Fix.lean`

Every read below is shown to lie in `[0, n)` for every input length `n`,
from a transcription of the loop tests and guards of the C++ (cited next to
each definition). Four groups:

1. **rvv** (`RLoop`, `rvv_all`, `rload_in`). Every rvv loop of src/rvv/
   (base64 excluded; `utf16fix` is `Utf16Fix.rvv_inBounds`) keeps the
   invariant `cursor + remaining + T = n`, where `T` is the look-ahead the
   loop holds back (`T = 3` in the UTF-8 decoder and the UTF-8 validator, 0
   elsewhere), and draws `vl` with `1 ≤ vl ≤ remaining` (`RvvExact.vlOf`).
   Each read is one of: the `vl` lanes of the `vle` load (`blk`); `src[vl + k]`
   with `k < T` (`ahead`); an index below `vl` reported by `vfirst` or
   `vl - 1` after `src += vl` (`inblk`); `src[k]` in a scalar `while (n)`
   tail after a guard `k < n` (`atRem`). The reparse step of the UTF-8
   decoder and the backward scan of `rvv_count_valid_utf8` are
   `rvv_reparse_in` and `rvv_count_back_in`.
2. **Backward scans** (`walk_errors`, `walk_counts`, `rewind_convert_in`).
   `rewind_and_validate_with_errors`, `rewind_and_validate_with_counts`, the
   rvv validator and `rewind_and_convert_with_errors` step back from a
   cursor `c` over continuation bytes. They stay at or after the start of the
   input because the first byte of the input is checked to be a leading byte
   (or because the scan is bounded by `prior_bytes = c`), not because of
   their step bounds.
3. **Validators.** `buf_block_reader` (`reader_*`), `tail_is_ascii`
   (`tailLoads_in`), `remainder_is_ascii`, `check_remainder`
   (`check_remainder_in`), the fixed-test loops of the generic, arm64, lsx,
   lasx and icelake validators as `LoadsAll.Loop`s (`validators_fit`), and
   the AVX-512 masked heads and tails (`ice_*`): a masked lane is accessed
   only when its mask bit is set, so only the set lanes count.
4. **Replacement drivers** (`msubC_spec`, `culCalls_in`, `rec_in`,
   `tr_calls_in`, `chunk_in`, `crs_read_in`, `latin1_safe_in`): the reads of
   `maximal_subpart` (transcribed), the ranges handed to callees by
   `transcode_utf8_to_utf16_with_replacement`, `convert_using_locations`
   (for **every** list of recorded offsets, including the user-supplied
   `utf8_to_utf16_result` of the public overload), the recording loop of
   `utf16_length_from_utf8_with_replacement`, the UTF-16 → UTF-8 `_safe`
   drivers (`buf[read_len - 1]`) and the scalar `convert_with_replacement_safe`
   (`data[input_count]`, `data[input_count + 1]`).

What is read from the C++: the transcriptions (loop tests, guards, offsets);
that a callee handed `(ptr, len)` reads only `[ptr, ptr + len)` (each callee
is covered here, in `LoadsAll`, or is a scalar loop whose reads are guarded
as `guarded_read` states); RVV semantics (`vsetvl` returns
`1 ≤ vl ≤ AVL`; a unit-stride load of `vl` elements touches `vl` elements);
AVX-512 masked loads suppress faults on masked-off lanes; the vector
validators return `input_count = length` on success (src/generic/utf8_validation/utf8_validator.h:153).

Skipped: the Ice Lake UTF-8 decoders (another file); the Ice Lake
`validate_utf16be*` loops are listed with their LE twins (same tests, a byte
shift on the loaded value). Result: no read outside the input was found.
-/

namespace SafetyMargins.LoadsRest
open SafetyMargins SafetyMargins.LoadsAll

/-! ## 1. rvv -/

/-- One read of an rvv loop iteration, relative to the cursor. -/
inductive RLd where
  /-- `vle*(src, vl)`: lanes `0 .. vl - 1`. -/
  | blk
  /-- `src[vl + k]`, look-ahead. -/
  | ahead (k : Nat)
  /-- One index below `vl` (`vfirst` result, `src[0]`, `src[-1]` after `src += vl`). -/
  | inblk
  /-- `src[k]` inside a scalar `while (n)` after a guard `k < n`. -/
  | atRem (k : Nat)
deriving Repr, DecidableEq

/-- `j` is an index read by `d`, with `vl` lanes and `r` units remaining. -/
def RLd.Reads : RLd → Nat → Nat → Nat → Prop
  | .blk, vl, _, j => j < vl
  | .ahead k, vl, _, j => j = vl + k
  | .inblk, vl, _, j => j < vl
  | .atRem k, _, r, j => j = k ∧ k < r

def RLd.ok (T : Nat) : RLd → Bool
  | .ahead k => decide (k < T)
  | _ => true

/-- An rvv loop: the held-back look-ahead `T` and the reads of one iteration. -/
structure RLoop where
  T : Nat
  loads : List RLd
deriving Repr

def RLoop.ok (l : RLoop) : Bool := l.loads.all (RLd.ok l.T)

/-- **Every read of an rvv iteration is inside the input**: with
`cursor + remaining + T = n` and `vl ≤ remaining`. -/
theorem rload_in (l : RLoop) (hl : l.ok = true) (n i r vl j : Nat)
    (hinv : i + r + l.T = n) (hvl : vl ≤ r) (d : RLd) (hd : d ∈ l.loads)
    (hj : d.Reads vl r j) : i + j < n := by
  have hok := (List.all_eq_true.mp hl) d hd
  cases d with
  | blk => simp only [RLd.Reads] at hj; omega
  | ahead k => simp only [RLd.Reads] at hj; simp [RLd.ok] at hok; omega
  | inblk => simp only [RLd.Reads] at hj; omega
  | atRem k => simp only [RLd.Reads] at hj; omega

/-- The invariant is kept by an advance of `a ≤ remaining` (`len -= vl, src += vl`,
and the `vl - 1` / `+ 1` adjustments of UTF-16 → UTF-32, which advance at most
the `vl` of the iteration). -/
theorem rinv_step (n i r T a : Nat) (h : i + r + T = n) (ha : a ≤ r) :
    (i + a) + (r - a) + T = n := by omega

/-- `vl = vsetvl(remaining)` as `RvvExact` models it (`vlOf`) satisfies the
hypothesis of `rload_in`. -/
theorem rvv_vl (pick : Nat → Nat) (r : Nat) : RvvExact.vlOf pick r ≤ r := RvvExact.vlOf_le pick r

/-- Copies and widenings: src/rvv/rvv_latin1_to.inl.cpp:5-7 (Latin-1 → UTF-8),
:38-40, :49-51, :64-66 (Latin-1 → UTF-16/32); src/rvv/rvv_utf16_to.inl.cpp:6-8,
:51-53, :62-64; src/rvv/rvv_utf32_to.inl.cpp:11-13, :205-207, :277-279;
src/rvv/rvv_length_from.inl.cpp:31-33, :46-48, :76-78, :90-92, :123-125,
:140-142, :156-158; src/rvv/rvv_find.cpp:4-6, :18-20;
src/rvv/rvv_validate.inl.cpp:6-8, :18-20, :33-35, :232-234, :251-253. Each loop
`for (...; len > 0; len -= vl, src += vl ...)` loads `vl` elements at `src`. -/
def rvv_plain : RLoop := { T := 0, loads := [.blk] }

/-- UTF-16 → UTF-8 (src/rvv/rvv_utf16_to.inl.cpp:82-85, `n = len` at :75), and
the scalar pair loop `while (n)` (:179-180 reads `src[0]`; :189 returns when
`n <= 1`, so `src[1]` at :196 has `1 < n`). -/
def rvv_utf16to8 : RLoop := { T := 0, loads := [.blk, .atRem 0, .atRem 1] }

/-- UTF-32 → UTF-8 (src/rvv/rvv_utf32_to.inl.cpp:45-48, `n = len` at :38); the
scalar `while (n)` at :157-158 reads `src[0]`. -/
def rvv_utf32to8 : RLoop := { T := 0, loads := [.blk, .atRem 0] }

/-- UTF-16 → UTF-32 with errors (src/rvv/rvv_utf16_to.inl.cpp:271-273): `src[0]`
(:291), `vl = vsetvl(vl - 1)` (:298) then `src[idx]` with `idx < vl` (:319) and
`src[vl]` (:327), both below the `vl` of the load; the advance is that new `vl`
plus at most one (:355-363), at most the loaded `vl`. -/
def rvv_utf16to32 : RLoop := { T := 0, loads := [.blk, .inblk] }

/-- UTF-8 → Latin-1 (src/rvv/rvv_utf8_to.inl.cpp:277-280, :343-346): the
`last = src[-1]` of the loop increment is lane `vl - 1` of the block just
loaded. UTF-16 validation (src/rvv/rvv_validate.inl.cpp:167-169): `last = src[-1]`
likewise, and `src[idx - 1]` with `0 < idx < vl` (:179). -/
def rvv_lastlane : RLoop := { T := 0, loads := [.blk, .inblk] }

/-- The UTF-8 decoder (src/rvv/rvv_utf8_to.inl.cpp:49-52, `n = len - tail` with
`tail = 3` at :29-30) reads `src[vl + 0]`, `src[vl + 1]`, `src[vl + 2]`
(:56-58); `rvv_count_valid_utf8` (src/rvv/rvv_validate.inl.cpp:90-92, `n = len - 3`
at :87-88) reads the same three (:94-96). -/
def rvv_utf8 : RLoop := { T := 3, loads := [.blk, .ahead 0, .ahead 1, .ahead 2] }

theorem rvv_all :
    [rvv_plain, rvv_utf16to8, rvv_utf32to8, rvv_utf16to32, rvv_lastlane, rvv_utf8].all
      RLoop.ok = true := by decide

/-- With one unit less held back, the UTF-8 look-ahead would leave the input:
`T = 3` is what `src[vl + 2]` needs. -/
theorem rvv_utf8_tight : ({ rvv_utf8 with T := 2 } : RLoop).ok = false := by decide

/-- The prologue of the UTF-8 decoder and validator
(src/rvv/rvv_utf8_to.inl.cpp:22-23, src/rvv/rvv_validate.inl.cpp:69-70):
`while (idx < len && ...) ++idx` reads `src[idx]` only with `idx < len`, and
`scalar::utf8::validate(src, idx)` reads `[0, idx)` with `idx ≤ len`. -/
theorem rvv_prologue_in (len idx j : Nat) (hidx : idx ≤ len) (hj : j < idx) : j < len := by
  omega

/-- The reparse step of the UTF-8 decoder (src/rvv/rvv_utf8_to.inl.cpp:252-256):
after the loop the cursor is `len - tail` (`n = 0`, `tail = 3`); `while
((src[0] >> 6) == 0b10 && tail < len) --src, ++tail` reads `src[0]` at
`len - tail` with `tail ≤ len` (it reads before testing `tail < len`, and only
reaches `tail = len` after a step from `tail < len`); the scalar tail reads
`[len - tail, len)`. -/
theorem rvv_reparse_in (len tail : Nat) (ht : tail ≤ len)
    (hpos : 0 < tail) : len - tail < len ∧ len - tail + tail = len := by omega

/-! ## 2. Backward scans -/

/-- **`rewind_and_validate_with_errors`** (include/simdutf/scalar/utf8.h:231-251)
and the rvv scan (src/rvv/rvv_validate.inl.cpp:135): read `byte[c - k]` for
`k = 0, 1, ...`, and step again only on a continuation byte. If the first byte
of the input is not a continuation byte (checked at include/simdutf/scalar/utf8.h:234; for
rvv, `scalar::utf8::validate(src, idx)` of the prologue rejects a first
continuation byte), every step `k` reached has `k ≤ c`: the scan never reads
before the input, whatever its step bound (5 there, none for rvv). `byte` is
any function on `Int` indices; nothing is assumed about the bytes before 0. -/
theorem walk_errors (byte : Int → Nat) (cont : Nat → Bool) (h0 : cont (byte 0) = false)
    (c k : Nat) (hreach : ∀ j, j < k → cont (byte ((c : Int) - j)) = true) : k ≤ c := by
  refine Nat.le_of_not_lt fun hk => ?_
  have := hreach c hk
  rw [Int.sub_self] at this
  rw [h0] at this; cases this

/-- **`rewind_and_validate_with_counts`** (include/simdutf/scalar/utf8.h:400-447): when
`buf ≠ start` it steps back first and reads `byte[c - k]`, `k = 1, 2, 3, 4`,
stepping again only on a continuation byte; the first byte is checked at
:403-405. Every step reached has `k ≤ c`. The comment at :419-422 says callers
pass a block boundary at least 64 bytes in; Ice Lake does not
(src/icelake/implementation.cpp:342 sets `count = 64 - misalignment`, which can be 1, 2
or 3, and :355-357 rewinds from it), but this theorem, which needs only `1 ≤ c`,
keeps those reads inside the input. -/
theorem walk_counts (byte : Int → Nat) (cont : Nat → Bool) (h0 : cont (byte 0) = false)
    (c k : Nat) (hc : 1 ≤ c)
    (hreach : ∀ j, 1 ≤ j → j < k → cont (byte ((c : Int) - j)) = true) : k ≤ c := by
  refine Nat.le_of_not_lt fun hk => ?_
  have := hreach c hc hk
  rw [Int.sub_self] at this
  rw [h0] at this; cases this

/-- After the scan, `validate_with_errors(buf, len + extra_len)` /
`validate_with_counts(buf, len + extra_len)` (include/simdutf/scalar/utf8.h:249, :443)
reads `[c - extra, c - extra + (n - c) + extra) = [c - extra, n)`. -/
theorem rewind_validate_range (n c extra : Nat) (hc : c ≤ n) (he : extra ≤ c) :
    c - extra + ((n - c) + extra) = n := by omega

/-- The callers of the rewinds pass `count < length` on an error
(src/generic/utf8_validation/utf8_validator.h:75-81: `count` is the index of a full block, minus
one; :90-96: `count` is the index of the remainder, `< length`, minus one), so
the first read `byte[count]` of `rewind_and_validate_with_errors` is inside. -/
theorem rewind_caller (len idx : Nat) (h : idx < len) : idx - 1 < len := by omega

/-- **`rewind_and_convert_with_errors`**
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:306-321) reads `buf[-i]` for
`i ≤ how_far_back = prior_bytes = pos` (:303-306): index `pos - i ≥ 0`; then
`convert_with_errors(buf - i, len + i)` (:337) reads `[pos - i, pos + len)`.
The callers pass `pos < size` and `len = size - pos`
(src/generic/utf8_to_utf16/utf8_to_utf16.h:324-325, :369-370). -/
theorem rewind_convert_in (pos size i : Nat) (hp : pos < size) (hi : i ≤ pos) :
    (pos : Int) - i ≥ 0 ∧ pos - i + ((size - pos) + i) = size := by omega

/-! ## 3. Validators -/

/-- The scalar validators and decoders read `data[pos + i]`, `i < w`, only after
a guard `pos + w <= len` (`next_pos > len` returns:
include/simdutf/scalar/utf8.h:51-55, :64-71, :83-93 and the same in `validate_with_errors`
and `validate_with_counts`; the 16-byte ASCII probe :26-31; the ASCII loop
`if (++pos == len) return` before `data[pos]` at :44-47;
include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:116-119 for its 4-unit probe and :178-187 for
`data[pos + 1]`; include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:79-85, :101). -/
theorem guarded_read (pos w i len : Nat) (h : pos + w ≤ len) (hi : i < w) : pos + i < len := by
  omega

/-- `buf_block_reader<64>` (src/generic/buf_block_reader.h:80-116): the loop
`while (reader.has_full_block())` (`idx < lenminusstep`, :91) with `idx += 64`. -/
def readerEnd (len : Nat) : Nat → Nat → Nat
  | 0, idx => idx
  | f + 1, idx => if idx < len - 64 then readerEnd len f (idx + 64) else idx

/-- A full block lies in the input (`lenminusstep = len - 64` when `len ≥ 64`,
else 0, :81). -/
theorem reader_full (len idx : Nat) (h : idx < len - 64) : idx + 64 ≤ len := by omega

theorem readerEnd_spec (len : Nat) : ∀ f idx, idx ≤ len → len < idx + 64 * f + 64 →
    readerEnd len f idx ≤ len ∧ len - 64 ≤ readerEnd len f idx ∧
      (readerEnd len f idx = len ∨ readerEnd len f idx < len) := by
  intro f
  induction f with
  | zero => intro idx h1 h2; simp [readerEnd]; omega
  | succ f ih =>
    intro idx h1 h2
    unfold readerEnd
    split
    · exact ih (idx + 64) (by omega) (by omega)
    · omega

/-- **The remainder of `buf_block_reader`.** After the full blocks
(`readerEnd` from 0, enough steps), `idx ≤ len` and `len - idx ≤ 64`:
`get_remainder` (:102-111) copies `[idx, len)` into its 64-byte buffer, and
`check_remainder`'s shift `idx - start` with `start = len - 64` is in `[0, 64)`. -/
theorem reader_rem (len : Nat) :
    readerEnd len (len + 1) 0 ≤ len ∧ len - readerEnd len (len + 1) 0 ≤ 64 :=
  let h := readerEnd_spec len (len + 1) 0 (Nat.zero_le _) (by omega)
  ⟨h.1, by omega⟩

/-- `tail_is_ascii(p, n)` (src/generic/buf_block_reader.h:14-33): the
`(offset, width)` of every load. The loop `for (i = 0; i + 16 < n; i += 16)`
(:17) loads 8 bytes at `i` and `i + 8`. -/
def tailLoads (n : Nat) : List (Nat × Nat) :=
  if 16 ≤ n then
    ((List.range n).filter (fun i => i % 16 == 0 && decide (i + 16 < n))).flatMap
        (fun i => [(i, 8), (i + 8, 8)]) ++ [(n - 16, 8), (n - 8, 8)]
  else if 8 ≤ n then [(0, 8), (n - 8, 8)]
  else if 4 ≤ n then [(0, 4), (n - 4, 4)]
  else (List.range n).map fun i => (i, 1)

/-- **`tail_is_ascii` reads only its `n` bytes**, for every `n`. -/
theorem tailLoads_in (n : Nat) : ∀ p ∈ tailLoads n, p.1 + p.2 ≤ n := by
  intro p hp
  unfold tailLoads at hp
  split at hp
  · simp only [List.mem_append, List.mem_flatMap, List.mem_filter, List.mem_range,
      Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq, List.mem_cons, List.not_mem_nil,
      or_false] at hp
    rcases hp with ⟨i, ⟨_, _, hi⟩, rfl | rfl⟩ | rfl | rfl <;> simp <;> omega
  split at hp
  · simp at hp; rcases hp with rfl | rfl <;> simp <;> omega
  split at hp
  · simp at hp; rcases hp with rfl | rfl <;> simp <;> omega
  · simp at hp; obtain ⟨i, hi, rfl⟩ := hp; simp; omega

/-- `remainder_is_ascii(buf, len, idx)` (:41-48): one 64-byte load at
`len - 64` when `len ≥ 64`, else `tail_is_ascii(buf + idx, len - idx)`. -/
theorem remainder_in (len idx : Nat) (hidx : idx ≤ len) :
    (64 ≤ len → len - 64 + 64 = len) ∧ (∀ p ∈ tailLoads (len - idx), idx + p.1 + p.2 ≤ len) := by
  refine ⟨fun h => by omega, fun p hp => ?_⟩
  have := tailLoads_in _ p hp; omega

/-- `check_remainder` (src/generic/utf8_validation/utf8_validator.h:14-40), with the
register width `W` (16 or 32 bytes): when `length ≥ 64 + W` it loads 64
bytes at `length - 64` and `W` bytes at `length - 64 - W` (:22-30); otherwise,
when `idx ≠ length`, it copies `[idx, length)` into a 64-byte buffer (:35-36),
which fits by `reader_rem`. -/
theorem check_remainder_in (len idx W : Nat) (hidx : idx ≤ len) (hrem : len - idx ≤ 64) :
    (64 + W ≤ len → len - 64 + 64 ≤ len ∧ len - 64 - W + W ≤ len - 64 ∧ len - 64 ≤ idx) ∧
    (idx + (len - idx) = len ∧ len - idx ≤ 64) := by omega

/-! ### Fixed-test validator loops, as `LoadsAll.Loop` -/

/-- Full blocks of `generic_validate_utf8`, `_with_errors`, `_with_counts` and
`generic_validate_ascii*` (src/generic/utf8_validation/utf8_validator.h:49-50, :71-72, :119-120;
src/generic/ascii_validation.h:9-10, :33-34; src/arm64/arm_validate_utf8_with_counts.cpp:59,
whose remainder is `get_remainder` at :84-85): `has_full_block` is
`idx < len - 64` (src/generic/buf_block_reader.h:91), at least 65 bytes. -/
def v_blocks : Loop := { B := 64, m := 0, strict := true, loads := [(0, 64)] }

/-- `validate_utf16_with_errors` (src/generic/validate_utf16.h:69-74, loop
`input + N <= end`): two registers of `N / 2` units, `N` = 16 (westmere,
arm64, ppc64, lsx) or 32 (haswell, lasx). -/
def v_utf16_16 : Loop := { B := 16, m := 0, strict := false, loads := [(0, 8), (8, 8)] }
def v_utf16_32 : Loop := { B := 32, m := 0, strict := false, loads := [(0, 16), (16, 16)] }

/-- `validate_utf16_as_ascii_with_errors` (src/generic/validate_utf16.h:141-143):
`pos < size / 32 * 32`, so 32 units remain (`as_ascii_test`); 32 units loaded.
The scalar tail (:156-158) reads `input[pos]`, `pos < size`. -/
def v_utf16_ascii : Loop := { B := 32, m := 0, strict := false, loads := [(0, 32)] }

theorem as_ascii_test (pos size : Nat) (hm : pos % 32 = 0) (h : pos < size / 32 * 32) :
    pos + 32 ≤ size := by omega

/-- `utf32::validate` and `validate_with_errors` (src/generic/validate_utf32.h:25-26,
:68-69, loop `input + N < end`): one register of `N` = 4 or 8 units. -/
def v_utf32_4 : Loop := { B := 4, m := 0, strict := true, loads := [(0, 4)] }
def v_utf32_8 : Loop := { B := 8, m := 0, strict := true, loads := [(0, 8)] }

/-- arm64: src/arm64/arm_validate_utf16.cpp:8-13 and :75-77 (`end - input >= 16`)
and :103-107 (`input + 16 < end`), two registers of 8 units. -/
def v_arm_utf16 : Loop := { B := 16, m := 0, strict := false, loads := [(0, 8), (8, 8)] }
def v_arm_utf16_err : Loop := { B := 16, m := 0, strict := true, loads := [(0, 8), (8, 8)] }

/-- `arm_validate_utf16_as_ascii` (src/arm64/arm_validate_utf16.cpp:75-77) is
`v_arm_utf16` too; its tail is the scalar `validate_as_ascii` on the rest. -/
theorem arm_as_ascii_same : v_arm_utf16.loads = [(0, 8), (8, 8)] := rfl

/-- src/arm64/arm_validate_utf32le.cpp:11-12 and :38-39 (`end - input >= 4`). -/
def v_arm_utf32 : Loop := { B := 4, m := 0, strict := false, loads := [(0, 4)] }

/-- src/lsx/lsx_validate_utf32le.cpp:10-11 and :46-47 (`input + 4 < end`). -/
def v_lsx_utf32 : Loop := { B := 4, m := 0, strict := true, loads := [(0, 4)] }

/-- src/lasx/lasx_validate_utf32le.cpp:18-19 and :63-64 (`input + 8 < end`); the
alignment heads :5-6 and :46-47 (`input < end`) read one unit. -/
def v_lasx_utf32 : Loop := { B := 8, m := 0, strict := true, loads := [(0, 8)] }
def v_lasx_head : Loop := { B := 1, m := 0, strict := false, loads := [(0, 1)] }

/-- Ice Lake, unmasked loads. ASCII: src/icelake/icelake_ascii_validation.inl.cpp:27-35
(`end - buf >= 256`) and :37-38 (`>= 64`); src/icelake/implementation.cpp:454-458 and
:465-466 (same tests, `_with_errors`). -/
def v_ice_256 : Loop :=
  { B := 256, m := 0, strict := false, loads := [(0, 64), (64, 64), (128, 64), (192, 64)] }
def v_ice_64 : Loop := { B := 64, m := 0, strict := false, loads := [(0, 64)] }

/-- UTF-8, src/icelake/implementation.cpp:220-221, :283-284, :345-346 (`end - ptr >= 64`);
the ASCII skip of `validate_utf8_with_counts`: :369-370 (`end - q >= 64`),
:373-377 (`end - q >= 256`), :385-386 (`end - q >= 64`). These are `v_ice_64`
and `v_ice_256`. UTF-16 (units): src/icelake/implementation.cpp:603-605 (`end - buf >= 64`,
loads at 0 and 32), :506-507, :551-552, :653-654, :723-724, :790-791, :870-871
(`end - buf >= 32`). -/
def v_ice16_64 : Loop := { B := 64, m := 0, strict := false, loads := [(0, 32), (32, 32)] }
def v_ice16_32 : Loop := { B := 32, m := 0, strict := false, loads := [(0, 32)] }

/-- UTF-32 (units): src/icelake/icelake_utf32_validation.inl.cpp:39-43 (`end - buf >= 64`,
four registers of 16) and :62-63 (`>= 16`); src/icelake/implementation.cpp:975-979
(`buf + 48 <= end` with `end = buf + len - 16`, :946, so 64 units remain) and
:993-994 (`buf <= end`, 16 units), with the first full block :945-952
(`len >= 16`). -/
def v_ice32_64 : Loop :=
  { B := 64, m := 0, strict := false, loads := [(0, 16), (16, 16), (32, 16), (48, 16)] }
def v_ice32_16 : Loop := { B := 16, m := 0, strict := false, loads := [(0, 16)] }

/-- The shifted `end` of src/icelake/implementation.cpp:946: `buf + 48 <= buf0 + len - 16`
is 64 units remaining, `buf <= buf0 + len - 16` is 16. -/
theorem ice32_end (b0 len b : Nat) (h16 : 16 ≤ len) :
    (b + 48 ≤ b0 + len - 16 → b + 64 ≤ b0 + len) ∧ (b ≤ b0 + len - 16 → b + 16 ≤ b0 + len) := by
  omega

def validatorLoops : List Loop :=
  [v_blocks, v_utf16_16, v_utf16_32, v_utf16_ascii, v_utf32_4, v_utf32_8, v_arm_utf16,
    v_arm_utf16_err, v_arm_utf32, v_lsx_utf32, v_lasx_utf32, v_lasx_head, v_ice_256, v_ice_64,
    v_ice16_64, v_ice16_32, v_ice32_64, v_ice32_16]

/-- **Every load of every fixed-test validator loop fits its loop test.** -/
theorem validators_fit : validatorLoops.all (fun l => l.fits l.N) = true := by decide

theorem validators_reads : ∀ l ∈ validatorLoops, l.Reads l.N := fun l hl =>
  reads_of_fits l l.N ((List.all_eq_true.mp validators_fit) l hl)

/-! ### AVX-512 masked heads and tails (Ice Lake) -/

/-- A masked load at `c` whose set lanes are `lo .. hi - 1` accesses
`[c + lo, c + hi)`; it is inside when `c + hi ≤ n`. -/
theorem masked_in (c lo hi n i : Nat) (h : c + hi ≤ n) (hi' : lo ≤ i ∧ i < hi) : c + i < n := by
  omega

/-- The alignment heads load lanes `0 .. adjustment - 1`:
`64 - misalignment` bytes with `len ≥ 64` (src/icelake/icelake_ascii_validation.inl.cpp:17-22),
`(64 - misalignment) / 2` units with `len ≥ 32` (src/icelake/implementation.cpp:494-498,
:537-542, :589-593, :707-712, :776-780, :854-859), `(64 - misalignment) / 4`
units with `len ≥ 16` (src/icelake/icelake_utf32_validation.inl.cpp:26-31), with
`0 < misalignment < 64`. -/
theorem ice_heads (mis len : Nat) (h1 : 0 < mis) (h2 : mis < 64) :
    (64 ≤ len → 64 - mis ≤ len) ∧ (32 ≤ len → (64 - mis) / 2 ≤ len) ∧
    (16 ≤ len → (64 - mis) / 4 ≤ len) := by omega

/-- `validate_utf8` / `_with_errors` heads (src/icelake/implementation.cpp:206-217, :258-274):
a full 64-byte load at the start (`len ≥ 2048`), then lanes 61-63 of
`ptr - 64` with `ptr = buf + adjustment`, `adjustment = 64 - misalignment ≥ 3`
(`misalignment <= 61`, :208): bytes `adjustment - 3 .. adjustment - 1`, at
or after the start. The address `ptr - 64` is before the buffer; the masked-off
lanes are not accessed (fault suppression). -/
theorem ice_prev3 (mis len : Nat) (h1 : 0 < mis) (h2 : mis ≤ 61) (hl : 2048 ≤ len) :
    64 ≤ len ∧ 3 ≤ 64 - mis ∧ (64 - mis) - 3 + 3 ≤ len := by omega

/-- `validate_utf8_with_counts` head (src/icelake/implementation.cpp:326-331): lanes
`misalignment .. 63` of `aligned = ptr - misalignment`, i.e. bytes
`0 .. 63 - misalignment` of the input (`len ≥ 1024`). -/
theorem ice_counts_head (mis len : Nat) (h2 : mis < 64) (hl : 1024 ≤ len) (i : Nat)
    (hi : mis ≤ i ∧ i < 64) : i - mis < len := by omega

/-- The masked tails load lanes `0 .. end - ptr - 1` with `end - ptr` below the
block (src/icelake/implementation.cpp:224-225, :295-296, :399-400, :473-474, :514-516,
:560-562, :675-677, :747-749, :819-821, :899-901, :1017-1019;
src/icelake/icelake_ascii_validation.inl.cpp:42-44;
src/icelake/icelake_utf32_validation.inl.cpp:71-73). -/
theorem ice_tail (p e i : Nat) (hp : p ≤ e) (hi : i < e - p) : p + i < e := by omega

/-! ## 4. Replacement drivers -/

/-! ### `maximal_subpart` -/

section
variable (g : Nat → Nat) (n : Nat)

/-- The `while` of `maximal_subpart`
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:63-67):
`i <= need && i < n && (ptr[i] & 0xC0) == 0x80`, reading `ptr[i]` only when the
first two hold. Result: final `i` and the indices read. -/
def contRun (need : Nat) : Nat → Nat → Nat × List Nat
  | 0, i => (i, [])
  | f + 1, i =>
    if i ≤ need ∧ i < n then
      if g i &&& 0xC0 = 0x80 then ((contRun need f (i + 1)).1, i :: (contRun need f (i + 1)).2)
      else (i, [i])
    else (i, [])

/-- `need` of :38-58 (0 for the classes that return 1 at once). -/
def mNeed (b : Nat) : Nat := if b < 0xE0 then 1 else if b < 0xF0 then 2 else 3
/-- `lo` of :36-58. -/
def mLo (b : Nat) : Nat := if b = 0xE0 then 0xA0 else if b = 0xF0 then 0x90 else 0x80
/-- `hi` of :37-58. -/
def mHi (b : Nat) : Nat := if b = 0xED then 0x9F else if b = 0xF4 then 0x8F else 0xBF

/-- **`maximal_subpart(ptr, n)`** (:33-69), transcribed: `(result, indices read)`.
`ptr[0]` (:34); `C0`-`C1`, continuation, ASCII and `F5`-`FF` return 1 (:38-39,
:56-57); `n < 2 ||` short-circuits the read of `ptr[1]` (:59-61); then the loop. -/
def msubC : Nat × List Nat :=
  if g 0 < 0xC2 ∨ 0xF5 ≤ g 0 then (1, [0])
  else if n < 2 then (1, [0])
  else if g 1 < mLo (g 0) ∨ mHi (g 0) < g 1 then (1, [0, 1])
  else ((contRun g n (mNeed (g 0)) 3 2).1, 0 :: 1 :: (contRun g n (mNeed (g 0)) 3 2).2)
end

theorem contRun_spec (g : Nat → Nat) (n need : Nat) : ∀ f i, i ≤ n →
    i ≤ (contRun g n need f i).1 ∧ (contRun g n need f i).1 ≤ n ∧
      ∀ j ∈ (contRun g n need f i).2, j < n := by
  intro f
  induction f with
  | zero => intro i h; simp [contRun]; omega
  | succ f ih =>
    intro i h
    unfold contRun
    split
    · split
      · have := ih (i + 1) (by omega)
        refine ⟨by omega, this.2.1, fun j hj => ?_⟩
        simp only [List.mem_cons] at hj
        rcases hj with rfl | hj
        · omega
        · exact this.2.2 j hj
      · refine ⟨Nat.le_refl _, h, fun j hj => ?_⟩
        simp at hj; omega
    · simp; omega

/-- **`maximal_subpart` reads only `ptr[0 .. n - 1]` and returns `1 ≤ skip ≤ n`**,
for every `n ≥ 1` (its precondition, :27) and every byte values. -/
theorem msubC_spec (g : Nat → Nat) (n : Nat) (hn : 1 ≤ n) :
    1 ≤ (msubC g n).1 ∧ (msubC g n).1 ≤ n ∧ ∀ j ∈ (msubC g n).2, j < n := by
  unfold msubC
  split
  · simp; omega
  split
  · simp; omega
  split
  · simp; omega
  · rename_i _ h2 _
    have := contRun_spec g n (mNeed (g 0)) 3 2 (by omega)
    refine ⟨by omega, this.2.1, fun j hj => ?_⟩
    simp only [List.mem_cons] at hj
    rcases hj with rfl | rfl | hj
    · omega
    · omega
    · exact this.2.2 j hj

/-! ### `convert_using_locations`, for every list of offsets -/

/-- The ranges `(start, length)` that `convert_using_locations`
(src/utf8_to_utf16_with_replacement.inl.h:194-236) hands to its callees, for an
arbitrary list of offsets `es` (the C++ caps it at 16, :200-203; the cap does
not matter here) and `sk e`, the result of `maximal_subpart(input + e, length - e)`:
* `err < pos || err >= length` (:208): `transcode(input + pos, length - pos)` (:209-211);
* `err > pos`: `convert_valid_prefix(input + pos, err - pos)` (:213-216);
* `maximal_subpart(input + err, length - err)` (:219-220);
* after the loop, `pos > length` returns (:223-225); else
  `convert_valid_prefix` or `transcode` on `(pos, length - pos)` (:226-235). -/
def culCalls (len : Nat) (sk : Nat → Nat) : List Nat → Nat → List (Nat × Nat)
  | [], pos => if len < pos then [] else [(pos, len - pos)]
  | err :: es, pos =>
    if err < pos ∨ len ≤ err then [(pos, len - pos)]
    else (if pos < err then [(pos, err - pos)] else []) ++ [(err, len - err)] ++
      culCalls len sk es (err + sk err)

/-- **Every range `convert_using_locations` hands out lies in the input, for
every list of recorded offsets**, given that `maximal_subpart` returns
`1 ≤ skip ≤ length - err` (`msubC_spec`). So the public overload taking a
caller-supplied `utf8_to_utf16_result` reads inside the input whatever the
offsets: out-of-order or out-of-range offsets fall to `transcode` on the rest. -/
theorem culCalls_in (len : Nat) (sk : Nat → Nat) (hsk : ∀ e, e < len → sk e ≤ len - e) :
    ∀ es pos, pos ≤ len → ∀ p ∈ culCalls len sk es pos, p.1 + p.2 ≤ len := by
  intro es
  induction es with
  | nil =>
    intro pos hp p hm
    simp only [culCalls] at hm
    split at hm
    · simp at hm
    · simp at hm; subst hm; simp; omega
  | cons err es ih =>
    intro pos hp p hm
    simp only [culCalls] at hm
    split at hm
    · simp at hm; subst hm; simp; omega
    · rename_i h
      simp only [List.mem_append] at hm
      rcases hm with (hm | hm) | hm
      · split at hm
        · simp at hm; subst hm; simp; omega
        · simp at hm
      · simp at hm; subst hm; simp; omega
      · have := hsk err (by omega)
        exact ih _ (by omega) p hm

/-! ### The recording loop and `transcode_utf8_to_utf16_with_replacement` -/

/-- The recording loop of `utf16_length_from_utf8_with_replacement`
(src/utf8_to_utf16_with_replacement.inl.h:243-273), one iteration at `pos < length`: the
validator on `(pos, length - pos)` (:244-245); `count_with_replacement` on the
same range (:252-253); unless it returned, `input_count < length - pos`
(:250-251), and `maximal_subpart(input + err, length - err)` with
`err = pos + input_count < length` (:264-266); then `pos = err + skip ≤ length`. -/
theorem rec_in (len pos ic sk : Nat) (hp : pos < len) (hic : ic < len - pos)
    (hsk : sk ≤ len - (pos + ic)) :
    pos + (len - pos) = len ∧ pos + ic < len ∧ pos + ic + (len - (pos + ic)) = len ∧
      pos + ic + sk ≤ len := by omega

/-- `utf8_boundary_before(input + pos, limit)` (src/utf8_to_utf16_with_replacement.inl.h:55-65)
reads `input[limit]`, then `input[i]` for `i` from `limit` down to 1 (the loop
`while (i > 0 && ...)` tests `i > 0` first). The caller passes
`limit = remaining - 32` with `remaining > 96` (:109-112), so every read is
below `remaining`; the window it returns is at most `limit`
(`Replacement.boundaryBefore_le`). -/
theorem boundary_in (rem i : Nat) (hr : 32 + 64 < rem) (hi : i ≤ rem - 32) : i < rem := by omega

/-- **The ranges of one iteration of the driver** (:98-186) at `pos < length`,
`rem = length - pos`: the bulk kernel on `(pos, window)` with
`window ≤ rem - 32` (:114-115); `utf16_length_from_utf8(input + pos, vb)` with
`vb < window` (:122-125); `maximal_subpart(input + pos + vb, rem - vb)`
(:132-133); the validator on `(pos, rem)` (:151-152); `convert_valid_prefix`
on `(pos, ic)` where `ic < rem` on an error (:153-158) and `ic = rem` on
success (:161-163); `maximal_subpart(input + pos + ic, rem - ic)` (:170-172).
Each next `pos` is at most `length` (`skip ≤` its range, `msubC_spec`), and
`finish_with_scalar` reads `(pos, length - pos)` (:67-86). That the validators
return the length on success is read from them
(src/generic/utf8_validation/utf8_validator.h:153). -/
theorem tr_calls_in (len pos window vb ic sk : Nat) (hp : pos < len)
    (hw : window ≤ len - pos - 32) (hvb : vb < window) (hic : ic ≤ len - pos)
    (hsk1 : sk ≤ len - pos - vb) :
    pos + window ≤ len ∧ pos + vb ≤ len ∧ pos + vb + (len - pos - vb) = len ∧
      pos + (len - pos) = len ∧ pos + ic ≤ len ∧ pos + vb + sk ≤ len := by omega

/-! ### UTF-16 → UTF-8 and Latin-1 → UTF-8 `_safe` drivers -/

/-- One iteration of `convert_utf16_to_utf8_safe` (src/implementation.cpp:1979-2008),
`convert_utf16_to_utf8_safe_with_details` (:2041-2076) and
`convert_latin1_to_utf8_safe` (:2697-2711), on `len` units left:
`read_len = min(len, utf8_len / 3)` (or `/ 2`); continue only when
`read_len > 16`; `buf[read_len - 1]` is read when `read_len < len`
(:1988-1991, :2048-2053); the kernel gets `(buf, rl)` with `rl ≤ read_len`;
then `buf += rl, len -= rl`. On error the details variant reads
`utf8_length_from_utf16(buf, count)` with `count < rl` (:2061-2063). -/
theorem chunk_in (len cap rl cnt : Nat) (h16 : 16 < min len (cap / 3))
    (hrl : rl ≤ min len (cap / 3)) (hcnt : cnt ≤ rl) :
    min len (cap / 3) - 1 < len ∧ rl ≤ len ∧ cnt ≤ len ∧ rl + (len - rl) = len := by
  have := Nat.min_le_left len (cap / 3)
  omega

/-- The same for Latin-1 (`utf8_len >> 1`, src/implementation.cpp:2699). -/
theorem chunk_latin1_in (len cap : Nat) : min len (cap / 2) ≤ len := Nat.min_le_left _ _

/-- **`convert_with_replacement_safe`**
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:308-353) reads
`data[input_count]` on `OUTPUT_BUFFER_TOO_SMALL` (:324-326) and
`data[input_count + 1]` only when `input_count + 1 != len` (:329-334). The
kernel `convert_with_errors<…, true>` reports that error at a unit of its
input (`ReplacementRest.FSpec`, met by `cwe_spec`), so both reads are inside. -/
theorem crs_read_in (SK : List Nat → Nat → ReplacementRest.FR) (hSK : ReplacementRest.FSpec SK)
    (xs : List Nat) (hu : ReplacementMore.U16 xs) (ic cap : Nat) (hic : ic < xs.length)
    (h2 : (SK (xs.drop ic) cap).err = 2) :
    ic + (SK (xs.drop ic) cap).inp < xs.length ∧
      (ic + (SK (xs.drop ic) cap).inp + 1 ≠ xs.length →
        ic + (SK (xs.drop ic) cap).inp + 1 < xs.length) := by
  obtain ⟨-, -, -, h4⟩ := hSK (xs.drop ic) cap (ReplacementRest.U16_drop hu ic)
  have := h4 (Or.inr h2)
  simp only [List.length_drop] at this
  omega

/-- The `_with_replacement_safe` driver (src/implementation.cpp:2499-2536) and the
scalar one advance `input_count` by `r.input_count ≤` the length handed in
(`FSpec`), then by one past a `SURROGATE` reported at a unit `< len`; each call
gets `(input + input_count, length - input_count)` with
`input_count ≤ length`. -/
theorem repl_safe_step (len ic r : Nat) (hr : r ≤ len - ic) (hic : ic ≤ len) :
    ic + r ≤ len ∧ (ic + r < len → ic + r + 1 ≤ len) := by omega

/-- `scalar::latin1_to_utf8::convert_safe`
(include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:71-117): the 16-byte probe
when `pos + 16 <= len` (:79-85), `data[pos]` under `pos < len` (:77, :101). -/
theorem latin1_safe_in (pos len : Nat) (h : pos + 16 ≤ len) : ∀ i, i < 16 → pos + i < len := by
  intro i hi; omega

/-! ## Non-vacuity -/

/-- `maximal_subpart` on `E2 82` at the end of the input (`n = 2`) reads two
bytes and skips both; on `F0 80` it skips one (`80` is below `lo = 90`). -/
example : msubC (fun i => [0xE2, 0x82].getD i 0) 2 = (2, [0, 1]) ∧
    msubC (fun i => [0xF0, 0x80, 0x80].getD i 0) 3 = (1, [0, 1]) := by decide

/-- Offsets out of order fall back to `transcode` on the rest. -/
example : culCalls 10 (fun _ => 1) [5, 3] 0 = [(0, 5), (5, 5), (6, 4)] := by decide

/-- The reader on 130 bytes stops at 128 (two full blocks, 2 bytes left); on
128 bytes at 64 (one full block, a full 64-byte remainder). -/
example : readerEnd 130 131 0 = 128 ∧ readerEnd 128 129 0 = 64 ∧ readerEnd 0 1 0 = 0 := by decide

end SafetyMargins.LoadsRest
