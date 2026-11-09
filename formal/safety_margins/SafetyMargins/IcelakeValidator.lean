import SafetyMargins.ReplacementRest
import SafetyMargins.ValidatorSound
/-
Ice Lake `validate_utf8_with_counts` meets `ReplacementRest.VSpec2`.

**The model** (`iceV misal`) transcribes
`implementation::validate_utf8_with_counts` (src/icelake/implementation.cpp:315-419)
line by line, for every address `misal` of `buf` (the code reads it only as
`misalignment = misal % 64`, :327): the early return on `len == 0` (:317-319);
for `len >= 1024` and a nonzero misalignment the masked head load at
`buf - misalignment` (lanes below `misalignment` are zero), the rewind from
`buf` if it fails (:333-336), its counts (:337-340), `count = 64 - misalignment`
(:342); the 64-byte loop (:345-395) with the rewind from `count` and the
counters before the block (:353-362), the counts of a non-ASCII block
(:363-365) and the ASCII skip (:366-393: one block, then 256-byte steps
tested by the OR of four loads, then 64-byte steps; none of these blocks goes
through the checker); the masked tail load, zero past `end` (:399-406);
`check_eof` (:407) and the rewind from `count` with the counters before the
tail (:408-417), or success with the final counters (:418). The rewind is
`ValidatorSound.rewindV` (`rewind_and_validate_with_counts`,
include/simdutf/scalar/utf8.h:400-450, and the caller's additions), and the scalar
validator its `vwc4F`, both reused unchanged. The checker
`avx512_utf8_checker` (src/icelake/icelake_utf8_validation.inl.cpp:65-116) is
transcribed on bytes: `check_special_cases` with the three tables as the
64-bit words of the C++ and `_mm512_ternarylogic_epi64` bit by bit,
`check_multibyte_lengths` with the saturating byte arithmetic,
`is_incomplete` with `max_value`, `prev<N>`
(src/icelake/icelake_common.inl.cpp:5-16) as the byte selection of
`_mm512_permutex2var_epi32` and `_mm512_alignr_epi8`; the counters
`utf8_count_continuations` (signed compare) and `utf8_count_4_byte_leads`
(src/icelake/icelake_utf8_common.inl.cpp:761-771); the loads `_mm512_loadu_si512`
and `_mm512_maskz_loadu_epi8` with the masks `~0 << misalignment` and
`~0 >> (64 - (end - ptr))` as 64-bit numbers.

**Where it differs from the generic validator, and why it does not matter.**
* The checker (`next_proj`): its tables are the generic ones (`mask1_eq`, ...),
  its per-byte check is `Validator.err false` (`iceErr_eq`: the
  `adds_epu8(0x7f, ·)`/`ternarylogic` form of `must_be_2_3_continuation` is
  the `|` one), `prev<N>` for `N ≤ 3` is the input shifted by `N` with the end
  of the previous block in front (`prevN_eq`, `decide` over the 64 lanes),
  `is_incomplete` is `ValidatorSound.isInc` of the last three bytes
  (`incV_eq`), and the ASCII test is `allAscii`. So one `check_next_input` is
  `ValidatorSound.nextInput false` on the last three carried bytes, and the
  generic soundness invariant (`GS`, `block_sound`, `gs_end`) applies as is.
* The head block (`inv_zeros`): the zero lanes before `buf` are ASCII and
  leave the automaton between characters, so they feed the first real byte
  the same `prev` bytes as the start of a stream, and the invariant on
  `zeros ++ X` is the invariant on `X`. An error they could "hide" would be a
  byte after a zero, which is checked as the first byte of a stream (a
  leading continuation byte is flagged); a flag they "invent" is harmless,
  since a flag only sends the input to the scalar rewind.
* The ASCII skip (`gs_extend`, `skipFrom_spec`): the skipped blocks are
  ASCII and come after a passing ASCII block, after which `prev_incomplete` is
  clear; an ASCII block then leaves the generic checker unchanged, so skipping
  it is checking it, and it adds nothing to the counters.
* The tail is padded with zeros rather than 0x20 (both ASCII, neither a
  continuation byte nor a four-byte lead), and the loop runs while
  `end - ptr >= 64`, so the tail can be empty and the rewind can start at
  `count = len` (allowed: the contract asks nothing of `p ≥ len`).
* The rewind offset `count` is `64 - misalignment + 64 k` (or `64 k`): not a
  multiple of 64 and possibly 1, 2 or 3. `ValidatorSound.rewind_spec` assumes
  `count % 64 = 0`, so it is re-proved here for every `count ≤ len` whose
  prefix passed (`rewind_good`). The comment of the scan used to say that
  callers pass a block boundary at least 64 bytes into the stream, which
  does not hold for this caller; the backward scan still stays in the
  buffer: it runs only when the first byte is not a continuation byte
  (include/simdutf/scalar/utf8.h:403-405) and stops at the first byte that
  is not one (`ext_le`). The pull request corrects the comment
  (include/simdutf/scalar/utf8.h:422-425).

**The result** (`iceV_spec2`): for every `misal`, `iceV misal` meets
`ReplacementRest.VSpec2`: on success the input is `Valid8`, `utf16_length()`
(`input_count - continuation_count + four_byte_count`,
include/simdutf/error.h:143-146) is `L (xs.map w16)` and `input_count` is the
length; on an error at `p < len` the prefix is valid with `utf16_length()`
units and `dchar` fails at `p`. No hypothesis. The `size_t` arithmetic of
the rewind is done in `Int`, as in `ValidatorSound`. `decide +kernel` checks
the tables (16³ triples, 256 × the values of `check_special_cases`) and the
ASCII test; `decide` checks the 64 lanes of `prev<N>`. No `native_decide`.
-/
namespace SafetyMargins.IcelakeValidator
open SafetyMargins SafetyMargins.Validator SafetyMargins.ValidExact SafetyMargins.ValidatorSound
open SafetyMargins.ReplacementMore (Bytes vchar dchar vwcF vwc VRes VSpec)
open SafetyMargins.Utf8 (w16)
set_option autoImplicit false

/-! ## 1. The byte operations -/

/-- `_mm512_subs_epu8` on one lane (bytes): saturating subtraction. -/
def subsU8 (a b : Nat) : Nat := a - b
/-- `_mm512_adds_epu8` on one lane (bytes): saturating addition. -/
def addsU8 (a b : Nat) : Nat := min (a + b) 255

/-- `_mm512_ternarylogic_epi{32,64}(a, b, c, imm)` on one byte: bit `i` of the
result is bit `4 a_i + 2 b_i + c_i` of `imm`. -/
def tern (a b c imm : Nat) : Nat :=
  (List.range 8).foldl (fun acc i => acc +
    (if imm.testBit ((if a.testBit i then 4 else 0) + (if b.testBit i then 2 else 0) +
      (if c.testBit i then 1 else 0)) then 2 ^ i else 0)) 0

/-- The 16 bytes of the two 64-bit words `_mm512_setr_epi64` puts in each
128-bit lane, little endian. -/
def qbytes (q0 q1 : Nat) : List Nat :=
  (List.range 8).map (fun i => q0 / 2 ^ (8 * i) % 256) ++
  (List.range 8).map (fun i => q1 / 2 ^ (8 * i) % 256)

/-- `mask1`, `mask2`, `mask3` of `check_special_cases`
(src/icelake/icelake_utf8_validation.inl.cpp:5-8, :14-17, :20-23); the four
128-bit lanes repeat the same two words, so `_mm512_shuffle_epi8` with an
index below 16 reads the same table in every lane. -/
def mask1 : List Nat := qbytes 0x0202020202020202 0x4915012180808080
def mask2 : List Nat := qbytes 0xcbcbcb8b8383a3e7 0xcbcbdbcbcbcbcbcb
def mask3 : List Nat := qbytes 0x101010101010101 0x1010101babaaee6

/-- The Ice Lake tables are the generic ones (`Tables.byte1High`, ...). -/
theorem mask1_eq : mask1 = Tables.byte1High.toList := by decide
theorem mask2_eq : mask2 = Tables.byte1Low.toList := by decide
theorem mask3_eq : mask3 = Tables.byte2High.toList := by decide

/-- `check_special_cases` on one byte `c` and `p1 = prev<1>` at that byte
(src/icelake/icelake_utf8_validation.inl.cpp:3-27): the indices are the high
nibble (`_mm512_srli_epi16(·, 4) & 0x0f`) and the low nibble (`& 0x0f`), and
the three looked-up bytes are combined by `_mm512_ternarylogic_epi64(·, ·, ·, 128)`. -/
def iceSC (p1 c : Nat) : Nat :=
  tern (mask1.getD (p1 / 16) 0) (mask2.getD (p1 % 16) 0) (mask3.getD (c / 16) 0) 128

/-- `check_multibyte_lengths` on one byte
(src/icelake/icelake_utf8_validation.inl.cpp:29-48): `is_third_byte`,
`is_fourth_byte` (`_mm512_subs_epu8`), their OR, `_mm512_adds_epu8` with
`0x7f`, and `_mm512_ternarylogic_epi32(·, 0x80, sc, 0b1101010)`. -/
def iceErr (p3 p2 p1 c : Nat) : Nat :=
  tern (addsU8 0x7f (subsU8 p2 (0b11100000 - 1) ||| subsU8 p3 (0b11110000 - 1))) 0x80
    (iceSC p1 c) 0b1101010

/-! ### The Ice Lake per-byte check is `Validator.err false` -/

/-- `iceSC` takes three table bytes: `128` is their AND, on every triple. -/
def scTabOK : Bool :=
  (List.range 16).all fun i => (List.range 16).all fun j => (List.range 16).all fun k =>
    tern (mask1.getD i 0) (mask2.getD j 0) (mask3.getD k 0) 128 ==
      (mask1.getD i 0 &&& mask2.getD j 0 &&& mask3.getD k 0)

theorem sc_tab_ok : scTabOK = true := by decide +kernel

theorem getD_toList (a : Array Nat) (i : Nat) : a.toList.getD i 0 = a[i]! := by
  rw [List.getD_eq_getElem?_getD, Array.getElem?_toList]
  by_cases h : i < a.size
  · simp [h]
  · simp [h]

theorem iceSC_eq (p1 c : Nat) (h1 : p1 < 256) (h2 : c < 256) : iceSC p1 c = sc p1 c := by
  have k := sc_tab_ok
  unfold scTabOK at k
  have k := List.all_eq_true.mp k (p1 / 16) (List.mem_range.mpr (by omega))
  have k := List.all_eq_true.mp k (p1 % 16) (List.mem_range.mpr (by omega))
  have k := List.all_eq_true.mp k (c / 16) (List.mem_range.mpr (by omega))
  unfold iceSC sc
  rw [beq_iff_eq] at k
  rw [k, mask1_eq, mask2_eq, mask3_eq, getD_toList, getD_toList, getD_toList]
  rfl

/-- The values `check_special_cases` can take. -/
def scVals : List Nat :=
  ((List.range 16).flatMap fun i => (List.range 16).flatMap fun j => (List.range 16).map fun k =>
    mask1.getD i 0 &&& mask2.getD j 0 &&& mask3.getD k 0).eraseDups

/-- With `b = 0x80`, `0b1101010` is `(a & 0x80) ^ c`, for every byte `a` and
every value of `check_special_cases`. -/
def ternOK : Bool :=
  (List.range 256).all fun a => scVals.all fun c =>
    tern a 0x80 c 0b1101010 == ((if 0x80 ≤ a then 0x80 else 0) ^^^ c)

theorem tern_ok : ternOK = true := by decide +kernel

theorem sc_mem (p1 c : Nat) (h1 : p1 < 256) (h2 : c < 256) : sc p1 c ∈ scVals := by
  rw [← iceSC_eq p1 c h1 h2]
  have k := sc_tab_ok
  unfold scTabOK at k
  have k := List.all_eq_true.mp k (p1 / 16) (List.mem_range.mpr (by omega))
  have k := List.all_eq_true.mp k (p1 % 16) (List.mem_range.mpr (by omega))
  have k := List.all_eq_true.mp k (c / 16) (List.mem_range.mpr (by omega))
  rw [beq_iff_eq] at k
  unfold iceSC; rw [k]
  unfold scVals
  rw [List.mem_eraseDups]
  simp only [List.mem_flatMap, List.mem_range, List.mem_map]
  exact ⟨p1 / 16, by omega, p1 % 16, by omega, c / 16, by omega, rfl⟩

theorem is34_ge (p3 p2 : Nat) (h3 : p3 < 256) (h2 : p2 < 256) :
    addsU8 0x7f (subsU8 p2 (0b11100000 - 1) ||| subsU8 p3 (0b11110000 - 1)) < 256 ∧
    (0x80 ≤ addsU8 0x7f (subsU8 p2 (0b11100000 - 1) ||| subsU8 p3 (0b11110000 - 1)) ↔
      (0xE0 ≤ p2 ∨ 0xF0 ≤ p3)) := by
  unfold addsU8 subsU8
  simp only [show (0b11100000 : Nat) - 1 = 223 from rfl, show (0b11110000 : Nat) - 1 = 239 from rfl]
  have hor : ∀ a b : Nat, a ||| b = 0 ↔ a = 0 ∧ b = 0 := fun a b => Nat.or_eq_zero_iff
  have hle : (p2 - 223) ||| (p3 - 239) ≤ 63 := by
    have := Nat.or_lt_two_pow (x := p2 - 223) (y := p3 - 239) (n := 6) (by omega) (by omega)
    omega
  have := hor (p2 - 223) (p3 - 239)
  constructor
  · omega
  · constructor
    · intro h
      refine Classical.byContradiction fun hn => ?_
      have : (p2 - 223) ||| (p3 - 239) = 0 := (hor _ _).mpr ⟨by omega, by omega⟩
      omega
    · intro h
      have : (p2 - 223) ||| (p3 - 239) ≠ 0 := fun h0 => by have := (hor _ _).mp h0; omega
      omega

/-- **The Ice Lake per-byte check is the generic one** with `|` in
`must_be_2_3_continuation` (`xm = false`), on all bytes. -/
theorem iceErr_eq (p3 p2 p1 c : Nat) (h3 : p3 < 256) (h2 : p2 < 256) (h1 : p1 < 256)
    (hc : c < 256) : iceErr p3 p2 p1 c = Validator.err false p3 p2 p1 c := by
  obtain ⟨hlt, hiff⟩ := is34_ge p3 p2 h3 h2
  unfold iceErr
  rw [iceSC_eq p1 c h1 hc]
  have k := tern_ok
  unfold ternOK at k
  have k := List.all_eq_true.mp k _ (List.mem_range.mpr hlt)
  have k := List.all_eq_true.mp k _ (sc_mem p1 c h1 hc)
  rw [beq_iff_eq] at k
  rw [k]
  unfold Validator.err must23
  simp only [Bool.false_eq_true, ↓reduceIte]
  by_cases h : 0xE0 ≤ p2 ∨ 0xF0 ≤ p3
  · have : 0x80 ≤ addsU8 0x7f (subsU8 p2 (0b11100000 - 1) ||| subsU8 p3 (0b11110000 - 1)) :=
      hiff.mpr h
    simp only [this, ↓reduceIte]
    rcases h with h | h <;> simp [h]
  · have : ¬ 0x80 ≤ addsU8 0x7f (subsU8 p2 (0b11100000 - 1) ||| subsU8 p3 (0b11110000 - 1)) :=
      fun h' => h (hiff.mp h')
    simp only [this, ↓reduceIte]
    have a : ¬ 0xE0 ≤ p2 := fun x => h (Or.inl x)
    have b : ¬ 0xF0 ≤ p3 := fun x => h (Or.inr x)
    simp [a, b]

/-! ## 2. `prev<N>`, `is_incomplete` and the checker -/

/-- A register as 64 bytes, read with `getD`; `pick a b (true, k)` is byte `k`
of `b`, `pick a b (false, k)` byte `k` of `a`. -/
def pick (a b : List Nat) (s : Bool × Nat) : Nat := if s.1 then b.getD s.2 0 else a.getD s.2 0

/-- `movemask` of `prev<N>` (src/icelake/icelake_common.inl.cpp:7-8). -/
def movemask : List Nat := [28, 29, 30, 31, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]

/-- `_mm512_permutex2var_epi32(input, movemask, previous)`, byte `k`: dword
`k / 4` takes dword `i % 16` of `previous` if bit 4 of `i` is set, of `input`
otherwise. -/
def permSel (k : Nat) : Bool × Nat :=
  let i := movemask.getD (k / 4) 0
  (i &&& 16 != 0, 4 * (i % 16) + k % 4)

/-- `_mm512_alignr_epi8(a, b, s)`, byte `j`: in each 128-bit lane, the 32
bytes `a:b` (`b` low) shifted right by `s` bytes. `true` selects `b`. -/
def alignSel (s j : Nat) : Bool × Nat :=
  let t := j % 16 + s
  if t < 16 then (true, 16 * (j / 16) + t) else (false, 16 * (j / 16) + t - 16)

/-- `rotated` of `prev<N>` (src/icelake/icelake_common.inl.cpp:9). -/
def rotated (input previous : List Nat) : List Nat :=
  (List.range 64).map fun k => pick input previous (permSel k)

/-- `prev<N>(input, previous)` (src/icelake/icelake_common.inl.cpp:5-16):
`_mm512_alignr_epi8(input, rotated, 16 - N)`. -/
def prevN (N : Nat) (input previous : List Nat) : List Nat :=
  (List.range 64).map fun j => pick input (rotated input previous) (alignSel (16 - N) j)

/-- Where byte `j` of `prev<N>` comes from, through both instructions. -/
def prevSel (N j : Nat) : Bool × Nat :=
  let a := alignSel (16 - N) j
  if a.1 then permSel a.2 else a

/-- `prev<N>` for `N ≤ 3` is the input shifted by `N` bytes, the first `N`
from the end of `previous`. -/
def prevSelOK : Bool :=
  [1, 2, 3].all fun N => (List.range 64).all fun j =>
    decide ((alignSel (16 - N) j).2 < 64) &&
    prevSel N j == (if N ≤ j then (false, j - N) else (true, 64 + j - N))

theorem prevSel_ok : prevSelOK = true := by decide

theorem getD_range_map (f : Nat → Nat) (n j : Nat) (h : j < n) :
    ((List.range n).map f).getD j 0 = f j := by
  rw [List.getD_eq_getElem?_getD, List.getElem?_map, List.getElem?_range h]
  rfl

/-- The three bytes before byte `j` of a block, the first ones from the end of
the previous block, as `ValidatorSound.scan` carries them. -/
def ext3 (p3 p2 p1 : Nat) (blk : List Nat) (j : Nat) : Nat := ([p3, p2, p1] ++ blk).getD j 0

theorem prevN_eq (N : Nat) (hN : N = 1 ∨ N = 2 ∨ N = 3) (input previous : List Nat) (j : Nat)
    (hj : j < 64) :
    (prevN N input previous).getD j 0 =
      ext3 (previous.getD 61 0) (previous.getD 62 0) (previous.getD 63 0) input (j + 3 - N) := by
  have k := prevSel_ok
  unfold prevSelOK at k
  have k := List.all_eq_true.mp k N (by rcases hN with h | h | h <;> subst h <;> simp)
  have k := List.all_eq_true.mp k j (List.mem_range.mpr hj)
  simp only [Bool.and_eq_true, decide_eq_true_eq, beq_iff_eq] at k
  obtain ⟨hlt, hsel⟩ := k
  unfold prevN
  rw [getD_range_map _ _ _ hj]
  unfold prevSel at hsel
  have hv : pick input (rotated input previous) (alignSel (16 - N) j) =
      pick input previous (prevSel N j) := by
    unfold prevSel
    generalize alignSel (16 - N) j = a at hlt ⊢
    obtain ⟨b, k⟩ := a
    cases b
    · rfl
    · simp only [pick, ↓reduceIte, rotated]
      rw [getD_range_map _ _ _ hlt]
  rw [hv]
  unfold prevSel; rw [hsel]
  unfold ext3
  by_cases hNj : N ≤ j
  · simp only [hNj, ↓reduceIte, pick, Bool.false_eq_true]
    rw [show j + 3 - N = (j - N) + 3 by omega]
    rfl
  · simp only [hNj, ↓reduceIte, pick]
    rcases hN with rfl | rfl | rfl
    · obtain rfl : j = 0 := by omega
      rfl
    · rcases (by omega : j = 0 ∨ j = 1) with rfl | rfl <;> rfl
    · rcases (by omega : j = 0 ∨ j = 1 ∨ j = 2) with rfl | rfl | rfl <;> rfl

/-- `max_value` of `is_incomplete`
(src/icelake/icelake_utf8_validation.inl.cpp:58-61). -/
def maxValue : List Nat :=
  qbytes 0xffffffffffffffff 0xffffffffffffffff ++ qbytes 0xffffffffffffffff 0xffffffffffffffff ++
  qbytes 0xffffffffffffffff 0xffffffffffffffff ++ qbytes 0xffffffffffffffff 0xbfdfefffffffffff

theorem maxValue_eq : maxValue = List.replicate 61 0xff ++ [0xef, 0xdf, 0xbf] := by decide

/-- `is_incomplete(input)` is nonzero
(src/icelake/icelake_utf8_validation.inl.cpp:54-63): some lane of
`_mm512_subs_epu8(input, max_value)`. -/
def incV (input : List Nat) : Bool :=
  (List.range 64).any fun j => subsU8 (input.getD j 0) (maxValue.getD j 0) != 0

/-- `_mm512_test_epi8_mask(input, v_80) == 0` (src/icelake/icelake_utf8_validation.inl.cpp:100-102). -/
def asciiV (input : List Nat) : Bool := (List.range 64).all fun j => input.getD j 0 &&& 0x80 == 0

/-- `check_utf8_bytes(input, prev_input)` (src/icelake/icelake_utf8_validation.inl.cpp:78-87):
whether some lane of `check_multibyte_lengths(input, prev_input,
check_special_cases(input, prev<1>))` is nonzero. -/
def blockErr (input previous : List Nat) : Bool :=
  (List.range 64).any fun j =>
    iceErr ((prevN 3 input previous).getD j 0) ((prevN 2 input previous).getD j 0)
      ((prevN 1 input previous).getD j 0) (input.getD j 0) != 0

/-- `avx512_utf8_checker` (src/icelake/icelake_utf8_validation.inl.cpp:65-116):
`error` and `prev_incomplete` matter only through "some lane is nonzero"
(`errors()`, :113-115; `check_eof` ORs `prev_incomplete` into `error`, :92-96),
so they are kept as booleans; `prev_input_block` is kept whole. -/
structure ICS where
  err : Bool
  prev : List Nat
  inc : Bool

/-- `avx512_utf8_checker checker{}` (src/icelake/implementation.cpp:320): everything zero. -/
def ics0 : ICS := ⟨false, List.replicate 64 0, false⟩

/-- `check_next_input` (src/icelake/icelake_utf8_validation.inl.cpp:99-111),
with its return value (ASCII). -/
def iceNext (s : ICS) (input : List Nat) : ICS × Bool :=
  if asciiV input then ({ s with err := s.err || s.inc }, true)
  else (⟨s.err || blockErr input s.prev, input, incV input⟩, false)

/-- `check_eof` (src/icelake/icelake_utf8_validation.inl.cpp:92-96). -/
def eof (s : ICS) : ICS := { s with err := s.err || s.inc }

/-! ### The Ice Lake checker is the generic one on the last three bytes -/

/-- The generic checker state the Ice Lake one stands for: the last three
bytes of `prev_input_block` (`prev<N>` for `N ≤ 3` reads no others,
`prevN_eq`). -/
def proj (s : ICS) : CS := ⟨s.prev.getD 61 0, s.prev.getD 62 0, s.prev.getD 63 0, s.inc, s.err⟩

theorem all_range_getD (f : Nat → Bool) : ∀ l : List Nat,
    (List.range l.length).all (fun j => f (l.getD j 0)) = l.all f
  | [] => rfl
  | b :: r => by
    rw [List.length_cons, List.range_succ_eq_map, List.all_cons, List.all_map, List.all_cons]
    congr 1
    rw [← all_range_getD f r]
    rfl

theorem any_range_getD (f : Nat → Bool) (l : List Nat) :
    (List.range l.length).any (fun j => f (l.getD j 0)) = l.any f := by
  have := all_range_getD (fun b => !f b) l
  rw [← Bool.not_inj_iff] at this
  simpa [List.not_all_eq_any_not] using this

def asciiOK : Bool := (List.range 256).all fun b => (b &&& 0x80 == 0) == decide (b < 0x80)
theorem ascii_ok : asciiOK = true := by decide +kernel

theorem maxValue_ff : ((List.range 61).all fun j => maxValue.getD j 0 == 0xff) = true := by decide

theorem all_congr_mem {l : List Nat} {f g : Nat → Bool} (h : ∀ x ∈ l, f x = g x) :
    l.all f = l.all g := by
  induction l with
  | nil => rfl
  | cons a r ih =>
    simp only [List.all_cons]
    rw [h a (by simp), ih (fun x hx => h x (by simp [hx]))]

theorem any_congr_mem {l : List Nat} {f g : Nat → Bool} (h : ∀ x ∈ l, f x = g x) :
    l.any f = l.any g := by
  induction l with
  | nil => rfl
  | cons a r ih =>
    simp only [List.any_cons]
    rw [h a (by simp), ih (fun x hx => h x (by simp [hx]))]

theorem asciiV_eq (input : List Nat) (hl : input.length = 64) (hb : ∀ b ∈ input, b < 256) :
    asciiV input = allAscii input := by
  unfold asciiV allAscii
  rw [← hl, all_range_getD (fun b => b &&& 0x80 == 0) input]
  apply all_congr_mem
  intro b hm
  have k := List.all_eq_true.mp ascii_ok b (List.mem_range.mpr (hb b hm))
  simpa using k

theorem getD_lt (l : List Nat) (hb : ∀ b ∈ l, b < 256) (j : Nat) : l.getD j 0 < 256 := by
  rw [List.getD_eq_getElem?_getD]
  by_cases h : j < l.length
  · rw [List.getElem?_eq_getElem h]; exact hb _ (List.getElem_mem h)
  · rw [List.getElem?_eq_none (by omega)]; decide

theorem incV_eq (input : List Nat) (hb : ∀ b ∈ input, b < 256) :
    incV input = isInc (input.getD 61 0) (input.getD 62 0) (input.getD 63 0) := by
  unfold incV
  rw [show (64 : Nat) = 61 + 3 from rfl, List.range_add, List.any_append]
  have h1 : (List.range 61).any (fun j => subsU8 (input.getD j 0) (maxValue.getD j 0) != 0) = false := by
    rw [List.any_eq_false]
    intro j hj
    have hj := List.mem_range.mp hj
    have hm : maxValue.getD j 0 = 0xff := by
      have := List.all_eq_true.mp maxValue_ff j (List.mem_range.mpr hj)
      simpa using this
    have := getD_lt input hb j
    simp only [hm, subsU8, bne_iff_ne, ne_eq, Decidable.not_not]
    omega
  rw [h1, Bool.false_or]
  simp only [show List.map (fun x => 61 + x) (List.range 3) = [61, 62, 63] from rfl, List.any_cons,
    List.any_nil, Bool.or_false, show maxValue.getD 61 0 = 0xef from by decide,
    show maxValue.getD 62 0 = 0xdf from by decide, show maxValue.getD 63 0 = 0xbf from by decide,
    isInc, subsU8]
  rw [Bool.eq_iff_iff]
  simp only [Bool.or_eq_true, bne_iff_ne, ne_eq, decide_eq_true_eq]
  omega

/-- `scan` flags exactly the bytes whose check, on the byte and the three
before it, is nonzero. -/
theorem scan_any (xm : Bool) : ∀ (blk : List Nat) (p3 p2 p1 : Nat),
    (scan xm p3 p2 p1 blk).1 = (List.range blk.length).any fun j =>
      Validator.err xm (ext3 p3 p2 p1 blk j) (ext3 p3 p2 p1 blk (j + 1))
        (ext3 p3 p2 p1 blk (j + 2)) (ext3 p3 p2 p1 blk (j + 3)) != 0
  | [], _, _, _ => rfl
  | c :: r, p3, p2, p1 => by
    rw [List.length_cons, List.range_succ_eq_map, List.any_cons, List.any_map]
    simp only [scan]
    rw [scan_any xm r p2 p1 c]
    rfl

theorem scan_last (xm : Bool) : ∀ (blk : List Nat) (p3 p2 p1 : Nat),
    (scan xm p3 p2 p1 blk).2 = (ext3 p3 p2 p1 blk blk.length, ext3 p3 p2 p1 blk (blk.length + 1),
      ext3 p3 p2 p1 blk (blk.length + 2))
  | [], _, _, _ => rfl
  | c :: r, p3, p2, p1 => by
    simp only [scan]
    rw [scan_last xm r p2 p1 c]
    rfl

theorem ext3_lt (p3 p2 p1 : Nat) (blk : List Nat) (h3 : p3 < 256) (h2 : p2 < 256) (h1 : p1 < 256)
    (hb : ∀ b ∈ blk, b < 256) (j : Nat) : ext3 p3 p2 p1 blk j < 256 := by
  unfold ext3
  apply getD_lt
  intro b hm
  simp only [List.cons_append, List.nil_append, List.mem_cons] at hm
  rcases hm with rfl | rfl | rfl | hm
  · exact h3
  · exact h2
  · exact h1
  · exact hb b hm

/-- **`check_utf8_bytes` of Ice Lake is `scan` with `xm = false`.** -/
theorem blockErr_eq (input previous : List Nat) (hl : input.length = 64)
    (hb : ∀ b ∈ input, b < 256) (hp : ∀ b ∈ previous, b < 256) :
    blockErr input previous =
      (scan false (previous.getD 61 0) (previous.getD 62 0) (previous.getD 63 0) input).1 := by
  rw [scan_any]
  unfold blockErr
  rw [hl]
  apply any_congr_mem
  intro j hj
  have hj := List.mem_range.mp hj
  rw [prevN_eq 3 (by simp) _ _ _ hj, prevN_eq 2 (by simp) _ _ _ hj, prevN_eq 1 (by simp) _ _ _ hj]
  have e : input.getD j 0 = ext3 (previous.getD 61 0) (previous.getD 62 0) (previous.getD 63 0)
      input (j + 3) := rfl
  rw [e]
  have L := ext3_lt _ _ _ input (getD_lt _ hp 61) (getD_lt _ hp 62) (getD_lt _ hp 63) hb
  rw [iceErr_eq _ _ _ _ (L _) (L _) (L _) (L _)]
  simp only [Nat.add_sub_cancel]
  rfl

/-- **One block of the Ice Lake checker is one block of the generic one**
(`ValidatorSound.nextInput false`) on the last three carried bytes, and
`check_next_input` returns `is_ascii`. -/
theorem next_proj (s : ICS) (input : List Nat) (hl : input.length = 64)
    (hb : ∀ b ∈ input, b < 256) (hp : ∀ b ∈ s.prev, b < 256) :
    proj (iceNext s input).1 = nextInput false (proj s) input ∧
    (iceNext s input).2 = allAscii input ∧ (∀ b ∈ (iceNext s input).1.prev, b < 256) := by
  unfold iceNext nextInput
  rw [asciiV_eq input hl hb]
  by_cases ha : allAscii input = true
  · simp only [ha, ↓reduceIte]
    exact ⟨rfl, trivial, hp⟩
  · simp only [ha, Bool.false_eq_true, ↓reduceIte]
    refine ⟨?_, trivial, hb⟩
    rw [blockErr_eq input s.prev hl hb hp, incV_eq input hb]
    have e := scan_last false input (s.prev.getD 61 0) (s.prev.getD 62 0) (s.prev.getD 63 0)
    rw [hl] at e
    unfold proj
    simp only
    rw [e]
    rfl

/-! ## 3. The rewind at any offset

`ValidatorSound.rewindV` transcribes `rewind_and_validate_with_counts` and the
caller's additions. Its proof (`ValidatorSound.rewind_spec`) assumes the
offset is a multiple of 64 and at least 64 when nonzero, as the generic
callers pass. The Ice Lake `count` after the misaligned
head is `64 - misalignment + 64 k`, which can be 1, 2 or 3. The backward
scan (include/simdutf/scalar/utf8.h:426-442) still stays inside the buffer: it is reached only when the
first byte is not a continuation byte (:403-405), and it stops at the first
byte that is not one (`ext_le`). Here the rewind is proved at every offset
`count ≤ len` whose prefix the checker passed. -/

theorem ext_le (bs : List Nat) (idx : Nat) (hc0 : isCont (bs.getD 0 0) = false) (h1 : 1 ≤ idx) :
    ext bs idx ≤ idx := by
  unfold ext
  split
  · exact h1
  rename_i a
  have i2 : 2 ≤ idx := by
    refine Classical.byContradiction fun h => a ?_
    rw [show idx - 1 = 0 by omega]; exact hc0
  split
  · exact i2
  rename_i b
  have i3 : 3 ≤ idx := by
    refine Classical.byContradiction fun h => b ?_
    rw [show idx - 2 = 0 by omega]; exact hc0
  split
  · exact i3
  rename_i c
  refine Classical.byContradiction fun h => c ?_
  rw [show idx - 3 = 0 by omega]; exact hc0

/-- After a passing prefix the first byte is not a continuation byte. -/
theorem inv_head_cont {xs : List Nat} {p3 p2 p1 b : Nat} {r : List Nat} (h : Inv xs p3 p2 p1)
    (hx : xs = b :: r) (hc : isCont b = true) : False := by
  obtain ⟨s0, hH, hrun, hcase⟩ := h
  have h8 : run 0 xs = 8 := by rw [hx, run_cons, step0_cont b hc, run_8]
  rw [h8] at hrun
  have hH8 : step s0 p1 = 8 := hrun.symm
  unfold H at hH
  simp only [hH8, ↓reduceIte] at hH
  simp only [Bool.and_eq_true, beq_iff_eq] at hH
  have hs00 : s0 = 0 := hH.2.1.1.1.1
  have hbad : badLead p1 = true := hH.2.1.1.1.2
  subst hs00
  rcases hcase with h0 | ⟨ys, hys, hr⟩ | ⟨ys, a, -, -, hr⟩
  · rw [hx] at h0; cases h0
  · rcases ys with _ | ⟨y, ys⟩
    · rw [hx] at hys
      simp only [List.nil_append, List.cons.injEq] at hys
      rw [← hys.1] at hbad
      simp [badLead, isCont] at hbad hc
      omega
    · rw [hx] at hys
      simp only [List.cons_append, List.cons.injEq] at hys
      rw [← hys.1, run_cons, step0_cont b hc, run_8] at hr
      cases hr
  · rw [h8] at hr; cases hr

theorem ext_spec' (bs : List Nat) (idx : Nat) (h1 : 1 ≤ idx) (hlt : idx ≤ bs.length)
    (hc0 : isCont (bs.getD 0 0) = false)
    {p3 p2 p1 : Nat} (hI : Inv (bs.take idx) p3 p2 p1) :
    1 ≤ ext bs idx ∧ ext bs idx ≤ 4 ∧ ext bs idx ≤ idx ∧ run 0 (bs.take (idx - ext bs idx)) = 0 := by
  have hle := ext_le bs idx hc0 h1
  have e1 := take_succ_getD bs (idx - 1) (by omega)
  rw [show idx - 1 + 1 = idx by omega] at e1
  obtain ⟨A, B, C⟩ := inv_last hI e1
  revert hle
  unfold ext
  intro hle
  split
  · rename_i h1; simp only [h1, ↓reduceIte] at hle; exact ⟨Nat.le_refl _, by omega, hle, B h1⟩
  split
  · rename_i h1 h2
    simp only [h1, h2, Bool.true_eq_false, ↓reduceIte] at hle
    refine ⟨by omega, by omega, hle, lead_zero bs (idx - 2) (by omega) ?_ h2⟩
    rw [show idx - 2 + 1 = idx - 1 by omega]; exact A
  split
  · rename_i h1 h2 h3
    simp only [h1, h2, h3, Bool.true_eq_false, ↓reduceIte] at hle
    refine ⟨by omega, by omega, hle, lead_zero bs (idx - 3) (by omega) ?_ h3⟩
    rw [show idx - 3 + 1 = idx - 2 by omega]; exact prefix_ne8 bs (by omega) A
  · rename_i h1 h2 h3
    simp only [h1, h2, h3, Bool.true_eq_false, ↓reduceIte] at hle
    simp only [Bool.not_eq_false] at h1 h2 h3
    refine ⟨by omega, Nat.le_refl _, hle, lead_zero bs (idx - 4) (by omega) ?_ ?_⟩
    · rw [show idx - 4 + 1 = idx - 3 by omega]; exact prefix_ne8 bs (by omega) A
    · cases h4 : isCont (bs.getD (idx - 4) 0)
      · rfl
      · exfalso
        apply C h1
        have t1 := take_succ_getD bs (idx - 4) (by omega)
        have t2 := take_succ_getD bs (idx - 3) (by omega)
        have t3 := take_succ_getD bs (idx - 2) (by omega)
        rw [show idx - 4 + 1 = idx - 3 by omega] at t1
        rw [show idx - 3 + 1 = idx - 2 by omega] at t2
        rw [show idx - 2 + 1 = idx - 1 by omega] at t3
        rw [e1, t3, t2, t1]
        simp only [List.append_assoc, List.cons_append, List.nil_append]
        rw [run_append]
        exact four_conts _ _ _ _ _ h4 h3 h2 h1

/-- What `VSpec` and `VSpec2` ask of one returned value. -/
def Good (bs : List Nat) (v : VRes) : Prop :=
  (v.ok = true → Valid8 bs ∧ v.u = L (bs.map w16) ∧ v.p = bs.length) ∧
  (v.ok = false → v.verr = true → v.p < bs.length →
    Valid8 (bs.take v.p) ∧ v.u = L ((bs.take v.p).map w16) ∧ dchar (bs.drop v.p) = none)

/-- **The rewind meets the contract at every offset `idx ≤ len`** whose
prefix the checker passed (`ValidatorSound.rewind_spec` without `idx % 64 = 0`
and `64 ≤ idx`), and on success `input_count` is the length. -/
theorem rewind_good (bs : List Nat) (hb : Bytes bs) (hne : bs ≠ []) (idx : Nat)
    (hlt : idx ≤ bs.length) {p3 p2 p1 : Nat} (hI : Inv (bs.take idx) p3 p2 p1) :
    Good bs (rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))) := by
  unfold rewindV
  split
  · -- `*start` is a continuation byte: only possible at `idx = 0`
    rename_i hc
    have h0 : idx = 0 := by
      refine Classical.byContradiction fun h0 => ?_
      obtain ⟨b, r, rfl⟩ := List.exists_cons_of_ne_nil hne
      obtain ⟨k, rfl⟩ : ∃ k, idx = k + 1 := ⟨idx - 1, by omega⟩
      exact inv_head_cont (b := b) (r := r.take k) hI (by simp) (by simpa using hc)
    subst h0
    obtain ⟨b, r, rfl⟩ := List.exists_cons_of_ne_nil hne
    simp only [List.getD_cons_zero, isCont, Bool.and_eq_true, decide_eq_true_eq] at hc
    simp only [List.take_zero, conts, fours, List.countP_nil]
    constructor
    · intro h; cases h
    · intro _ _ _
      refine ⟨?_, ?_, ?_⟩
      · simp [Valid8, run]
      · simp
      · simp [dchar, show ¬ b < 0x80 by omega, show b < 0xC2 by omega]
  · rename_i hc0
    have hc0 : isCont (bs.getD 0 0) = false := by simpa using hc0
    have hj : ∃ e, (if idx = 0 then 0 else ext bs idx) = e ∧ e ≤ idx ∧ e ≤ 4 ∧
        run 0 (bs.take (idx - e)) = 0 := by
      by_cases h0 : idx = 0
      · exact ⟨0, by simp [h0], by omega, by omega, by simp [h0]; rfl⟩
      · obtain ⟨-, b, c, d⟩ := ext_spec' bs idx (by omega) hlt hc0 hI
        exact ⟨ext bs idx, by simp [h0], c, b, d⟩
    obtain ⟨e, he, hei, he4, hrun⟩ := hj
    simp only [he]
    have hbj := ReplacementMore.bytes_drop hb (idx - e)
    have spec := vwc4F_spec (bs.drop (idx - e)) hbj
    have hlen : (vwc4F (bs.drop (idx - e)).length (bs.drop (idx - e))).1 = true →
        (vwc4F (bs.drop (idx - e)).length (bs.drop (idx - e))).2.1 = (bs.drop (idx - e)).length := by
      obtain ⟨e1, e2, -⟩ := vwc4F_eq (bs.drop (idx - e)).length (bs.drop (idx - e)) hbj
      intro h
      rw [e2]; rw [e1] at h
      exact ReplacementRest.vwcF_len _ _ hbj (Nat.le_refl _) h
    generalize vwc4F (bs.drop (idx - e)).length (bs.drop (idx - e)) = r at spec hlen
    obtain ⟨ok, sp, sc', sf⟩ := r
    obtain ⟨s1, s2⟩ := spec
    simp only at s1 s2 hlen ⊢
    have htk : bs.take idx = bs.take (idx - e) ++ (bs.drop (idx - e)).take e := by
      rw [← List.take_add]; congr 1; omega
    have hC : conts (bs.take idx) = conts (bs.take (idx - e)) + conts ((bs.drop (idx - e)).take e) := by
      rw [htk, conts_append]
    have hF : fours (bs.take idx) = fours (bs.take (idx - e)) + fours ((bs.drop (idx - e)).take e) := by
      rw [htk, fours_append]
    have hLj := L_w16 (bs.take (idx - e))
    have hlj : (bs.take (idx - e)).length = idx - e := by simp; omega
    rw [hlj] at hLj
    have hic : ((sp : Int) - (e : Int) + (idx : Int)) = ((idx - e + sp : Nat) : Int) := by omega
    have hu : ((sp : Int) - e + idx) - ((sc' : Int) - conts ((bs.drop (idx - e)).take e) +
        conts (bs.take idx)) + ((sf : Int) - fours ((bs.drop (idx - e)).take e) + fours (bs.take idx)) =
        (L ((bs.take (idx - e)).map w16) : Int) + ((sp : Int) - sc' + sf) := by
      rw [hC, hF, hLj]; push_cast; omega
    rw [hu, hic]
    refine ⟨fun hok => ?_, fun hok _ _ => ?_⟩
    · obtain ⟨v, hl⟩ := s1 hok
      refine ⟨?_, ?_, ?_⟩
      · unfold Valid8
        rw [← List.take_append_drop (idx - e) bs, run_append, hrun]; exact v
      · have : L (bs.map w16) = L ((bs.take (idx - e)).map w16) + L ((bs.drop (idx - e)).map w16) := by
          rw [← L_append, ← List.map_append, List.take_append_drop]
        rw [hl, this]; simp only; omega
      · simp only [Int.toNat_natCast]
        rw [hlen hok, List.length_drop]; omega
    · obtain ⟨hsp, v, hl, hd⟩ := s2 hok
      simp only [Int.toNat_natCast]
      have hsplit : bs.take (idx - e + sp) = bs.take (idx - e) ++ (bs.drop (idx - e)).take sp :=
        List.take_add
      refine ⟨?_, ?_, ?_⟩
      · unfold Valid8; rw [hsplit, run_append, hrun]; exact v
      · rw [hl, hsplit, List.map_append, L_append]; omega
      · rw [← List.drop_drop]; exact hd

/-! ## 4. Loads and counts -/

/-- `_mm512_loadu_si512(buf + p)`: 64 bytes. -/
def loadu (bs : List Nat) (p : Nat) : List Nat := (List.range 64).map fun j => bs.getD (p + j) 0

/-- `_mm512_maskz_loadu_epi8(k, buf + base)`: lane `j` is read if bit `j` of
`k` is set, and is zero otherwise (a masked-out lane is not read, so `base`
may lie before the buffer). -/
def maskzLoad (k : Nat) (bs : List Nat) (base : Int) : List Nat :=
  (List.range 64).map fun j => if k.testBit j then bs.getD (base + j).toNat 0 else 0

/-- `~UINT64_C(0) << misalignment` (src/icelake/implementation.cpp:330). -/
def headMask (m : Nat) : Nat := ((2 ^ 64 - 1) <<< m) % 2 ^ 64
/-- `~UINT64_C(0) >> (64 - (end - ptr))` (src/icelake/implementation.cpp:401). -/
def tailMask (r : Nat) : Nat := (2 ^ 64 - 1) >>> (64 - r)

theorem headMask_bit (m j : Nat) (hj : j < 64) : (headMask m).testBit j = decide (m ≤ j) := by
  unfold headMask
  rw [Nat.testBit_mod_two_pow, Nat.testBit_shiftLeft, Nat.testBit_two_pow_sub_one]
  by_cases h : m ≤ j <;> simp [h, hj] <;> omega

theorem tailMask_bit (r j : Nat) (hr : r ≤ 64) : (tailMask r).testBit j = decide (j < r) := by
  unfold tailMask
  rw [Nat.testBit_shiftRight, Nat.testBit_two_pow_sub_one]
  by_cases h : j < r <;> simp [h] <;> omega

theorem loadu_eq (bs : List Nat) (p : Nat) (h : p + 64 ≤ bs.length) :
    loadu bs p = (bs.drop p).take 64 := by
  apply List.ext_getElem?
  intro j
  unfold loadu
  rw [List.getElem?_map, List.getElem?_take, List.getElem?_drop]
  by_cases hj : j < 64
  · rw [List.getElem?_range hj]
    simp only [hj, ↓reduceIte, Option.map_some, List.getD_eq_getElem?_getD]
    rw [List.getElem?_eq_getElem (by omega)]
    rfl
  · rw [List.getElem?_eq_none (by simp; omega)]
    simp [hj]

theorem head_eq (bs : List Nat) (m : Nat) (h0 : 0 < m) (h64 : m < 64) (h : 64 - m ≤ bs.length) :
    maskzLoad (headMask m) bs (-(m : Int)) = List.replicate m 0 ++ bs.take (64 - m) := by
  apply List.ext_getElem?
  intro j
  unfold maskzLoad
  rw [List.getElem?_map]
  by_cases hj : j < 64
  · rw [List.getElem?_range hj]
    simp only [Option.map_some]
    rw [headMask_bit m j hj]
    by_cases hm : m ≤ j
    · rw [List.getElem?_append_right (by simp; omega)]
      simp only [hm, decide_true, ↓reduceIte, List.length_replicate, List.getElem?_take,
        List.getD_eq_getElem?_getD]
      rw [show (-(m : Int) + (j : Int)).toNat = j - m by omega]
      simp only [show j - m < 64 - m by omega, ↓reduceIte]
      rw [List.getElem?_eq_getElem (by omega)]
      rfl
    · rw [List.getElem?_append_left (by simp; omega)]
      simp [hm, show j < m by omega]
  · rw [List.getElem?_eq_none (by simp; omega)]
    rw [List.getElem?_eq_none (by simp; omega)]
    rfl

theorem tail_eq (bs : List Nat) (p : Nat) (h1 : p < bs.length) (h2 : bs.length - p < 64) :
    maskzLoad (tailMask (bs.length - p)) bs p =
      bs.drop p ++ List.replicate (64 - (bs.length - p)) 0 := by
  apply List.ext_getElem?
  intro j
  unfold maskzLoad
  rw [List.getElem?_map]
  by_cases hj : j < 64
  · rw [List.getElem?_range hj]
    simp only [Option.map_some]
    rw [tailMask_bit (bs.length - p) j (by omega)]
    by_cases hm : j < bs.length - p
    · rw [List.getElem?_append_left (by simp; omega)]
      simp only [hm, decide_true, ↓reduceIte, List.getElem?_drop, List.getD_eq_getElem?_getD]
      rw [show ((p : Int) + (j : Int)).toNat = p + j by omega]
      rw [List.getElem?_eq_getElem (by omega)]
      rfl
    · rw [List.getElem?_append_right (by simp; omega)]
      simp [hm, List.getElem?_replicate]
      omega
  · rw [List.getElem?_eq_none (by simp; omega)]
    rw [List.getElem?_eq_none (by simp; omega)]
    rfl

/-- `(int8_t)b`. -/
def s8 (b : Nat) : Int := if b < 128 then b else (b : Int) - 256

/-- `utf8_count_continuations` (src/icelake/icelake_utf8_common.inl.cpp:761-765):
`count_ones` of `_mm512_cmplt_epi8_mask(input, 0xc0c0c0c0)` (signed bytes). -/
def iceConts (input : List Nat) : Nat :=
  (List.range 64).countP fun j => decide (s8 (input.getD j 0) < s8 0xc0)

/-- `utf8_count_4_byte_leads` (src/icelake/icelake_utf8_common.inl.cpp:767-771):
`count_ones` of `_mm512_cmpge_epu8_mask(input, 0xf0f0f0f0)`. -/
def iceFours (input : List Nat) : Nat :=
  (List.range 64).countP fun j => decide (0xf0 ≤ input.getD j 0)

theorem countP_range_getD (f : Nat → Bool) : ∀ l : List Nat,
    (List.range l.length).countP (fun j => f (l.getD j 0)) = l.countP f
  | [] => rfl
  | b :: r => by
    rw [List.length_cons, List.range_succ_eq_map, List.countP_cons, List.countP_map,
      List.countP_cons]
    rw [← countP_range_getD f r]
    rfl

theorem iceConts_eq (input : List Nat) (hl : input.length = 64) (hb : ∀ b ∈ input, b < 256) :
    iceConts input = conts input := by
  unfold iceConts conts
  rw [← hl, countP_range_getD (fun b => decide (s8 b < s8 0xc0)) input]
  apply List.countP_congr
  intro x hx
  have := hb x hx
  simp only [s8, decide_eq_true_eq, show ¬ ((0xc0 : Nat) < 128) from by decide, ↓reduceIte]
  split <;> omega

theorem iceFours_eq (input : List Nat) (hl : input.length = 64) :
    iceFours input = fours input := by
  unfold iceFours fours
  rw [← hl, countP_range_getD (fun b => decide (0xf0 ≤ b)) input]

/-! ## 5. The model of `validate_utf8_with_counts` -/

/-- `_mm512_or_si512(_mm512_or_si512(b0, b1), _mm512_or_si512(b2, b3))`
(src/icelake/implementation.cpp:378-379). -/
def or4 (b0 b1 b2 b3 : List Nat) : List Nat :=
  (List.range 64).map fun j => (b0.getD j 0 ||| b1.getD j 0) ||| (b2.getD j 0 ||| b3.getD j 0)

/-- The last loop of the ASCII skip (src/icelake/implementation.cpp:385-389). -/
def skip64 (bs : List Nat) (q : Nat) : Nat :=
  if h : 64 ≤ bs.length - q ∧ asciiV (loadu bs q) = true then skip64 bs (q + 64) else q
termination_by bs.length - q
decreasing_by omega

/-- The 256-byte loop of the ASCII skip (src/icelake/implementation.cpp:373-384): four
loads, their OR, and `break` on a byte with the high bit set. -/
def skip256 (bs : List Nat) (q : Nat) : Nat :=
  if h : 256 ≤ bs.length - q then
    if asciiV (or4 (loadu bs q) (loadu bs (q + 64)) (loadu bs (q + 128)) (loadu bs (q + 192))) = false
    then q
    else skip256 bs (q + 256)
  else q
termination_by bs.length - q
decreasing_by omega

/-- The ASCII skip after an ASCII block at `ptr` (src/icelake/implementation.cpp:367-390):
the new `q`. -/
def skipFrom (bs : List Nat) (ptr : Nat) : Nat :=
  if 64 ≤ bs.length - (ptr + 64) ∧ asciiV (loadu bs (ptr + 64)) = true then
    skip64 bs (skip256 bs (ptr + 64 + 64))
  else ptr + 64

theorem skip64_ge (bs : List Nat) (q : Nat) : q ≤ skip64 bs q := by
  induction q using skip64.induct bs with
  | case1 q h ih => rw [skip64]; split <;> first | omega | contradiction
  | case2 q h => rw [skip64]; split <;> first | omega | contradiction

theorem skip256_ge (bs : List Nat) (q : Nat) : q ≤ skip256 bs q := by
  induction q using skip256.induct bs with
  | case1 q h h' => rw [skip256]; split <;> (try split) <;> first | omega | simp_all
  | case2 q h h' ih => rw [skip256]; split <;> (try split) <;> first | omega | simp_all
  | case3 q h => rw [skip256]; split <;> (try split) <;> first | omega | simp_all

theorem skipFrom_ge (bs : List Nat) (ptr : Nat) : ptr + 64 ≤ skipFrom bs ptr := by
  unfold skipFrom
  split
  · have := skip256_ge bs (ptr + 64 + 64)
    have := skip64_ge bs (skip256 bs (ptr + 64 + 64))
    omega
  · omega

/-- The main loop (src/icelake/implementation.cpp:345-395) from `ptr` with `count`, the
checker `s` and the counters `continuations`, `four_byte_leads`. Returns
the result of an early `return` (:353-362) if there is one, and otherwise
the state at the loop exit. -/
def mainLoop (bs : List Nat) (ptr count : Nat) (s : ICS) (C F : Nat) :
    Option VRes × Nat × Nat × ICS × Nat × Nat :=
  if h : 64 ≤ bs.length - ptr then
    let utf8 := loadu bs ptr
    let r := iceNext s utf8
    if r.1.err then (some (rewindV bs count C F), ptr, count, r.1, C, F)
    else if !r.2 then mainLoop bs (ptr + 64) (count + 64) r.1 (C + iceConts utf8) (F + iceFours utf8)
    else mainLoop bs (skipFrom bs ptr) (count + (skipFrom bs ptr - (ptr + 64)) + 64) r.1 C F
  else (none, ptr, count, s, C, F)
termination_by bs.length - ptr
decreasing_by all_goals (have := skipFrom_ge bs ptr; omega)

/-- After the loop (src/icelake/implementation.cpp:396-417): the masked tail block,
`check_eof`, and the rewind from `count` with the counters before the tail,
or success with the final counters. -/
def finish (bs : List Nat) (ptr count : Nat) (s : ICS) (C F : Nat) : VRes :=
  let t : ICS × Nat × Nat :=
    if bs.length ≠ ptr then
      let utf8 := maskzLoad (tailMask (bs.length - ptr)) bs ptr
      let r := iceNext s utf8
      (r.1, if !r.2 then C + iceConts utf8 else C, if !r.2 then F + iceFours utf8 else F)
    else (s, C, F)
  if (eof t.1).err then rewindV bs count C F
  else ⟨true, false, bs.length, ((bs.length : Int) - t.2.1 + t.2.2).toNat⟩

/-- The loop and what follows it. -/
def body (bs : List Nat) (ptr count : Nat) (s : ICS) (C F : Nat) : VRes :=
  let r := mainLoop bs ptr count s C F
  match r.1 with
  | some v => v
  | none => finish bs r.2.1 r.2.2.1 r.2.2.2.1 r.2.2.2.2.1 r.2.2.2.2.2

/-- **Ice Lake `implementation::validate_utf8_with_counts`**
(src/icelake/implementation.cpp:315-419) as `ReplacementMore.VRes`: `ok`
(`error == SUCCESS`), `verr` (every error comes from the scalar rewind, all of
whose errors are validation errors), `input_count`, `utf16_length()`.
`misal` is the address of `buf`; the code depends on it only through
`misalignment = misal % 64` (:327). With `len >= 1024` and a nonzero
misalignment (:326-344) the first block is the masked load at
`buf - misalignment`, zero below `misalignment`; an error there rewinds from
`buf` with no additions (:333-336). The pointer `ptr` and `count` are both
offsets from `buf`. `utf16_length()` is
`input_count - continuation_count + four_byte_count`
(include/simdutf/error.h:143-146). -/
def iceV (misal : Nat) (bs : List Nat) : VRes :=
  if bs.length = 0 then ⟨true, false, bs.length, 0⟩
  else if 1024 ≤ bs.length ∧ misal % 64 ≠ 0 then
    let head := maskzLoad (headMask (misal % 64)) bs (-((misal % 64 : Nat) : Int))
    let r := iceNext ics0 head
    if r.1.err then rewindV bs 0 0 0
    else body bs (64 - misal % 64) (64 - misal % 64) r.1
      (if !r.2 then 0 + iceConts head else 0) (if !r.2 then 0 + iceFours head else 0)
  else body bs 0 0 ics0 0 0

/-! ## 6. The ASCII skip reads only ASCII bytes -/

theorem dite_t {P : Prop} [Decidable P] {α : Sort _} (h : P) (a : P → α) (b : ¬P → α) :
    dite P a b = a h := by simp [h]
theorem dite_f {P : Prop} [Decidable P] {α : Sort _} (h : ¬P) (a : P → α) (b : ¬P → α) :
    dite P a b = b h := by simp [h]
theorem ite_t {P : Prop} [Decidable P] {α : Sort _} (h : P) (a b : α) : ite P a b = a := by simp [h]
theorem ite_f {P : Prop} [Decidable P] {α : Sort _} (h : ¬P) (a b : α) : ite P a b = b := by simp [h]

theorem take_drop_split (bs : List Nat) {a b c : Nat} (hab : a ≤ b) (hbc : b ≤ c) :
    (bs.drop a).take (c - a) = (bs.drop a).take (b - a) ++ (bs.drop b).take (c - b) := by
  rw [show c - a = (b - a) + (c - b) by omega, List.take_add, List.drop_drop]
  congr 3; omega

theorem allAscii_append (x y : List Nat) : allAscii (x ++ y) = (allAscii x && allAscii y) := by
  simp [allAscii, List.all_append]

theorem allAscii_nil : allAscii [] = true := rfl

theorem bytes_take_drop (bs : List Nat) (hb : Bytes bs) (p n : Nat) :
    ∀ b ∈ (bs.drop p).take n, b < 256 :=
  fun b h => hb b (List.mem_of_mem_drop (List.mem_of_mem_take h))

theorem loadu_ascii (bs : List Nat) (hb : Bytes bs) (q : Nat) (h : q + 64 ≤ bs.length)
    (ha : asciiV (loadu bs q) = true) : allAscii ((bs.drop q).take 64) = true := by
  rw [loadu_eq bs q h] at ha
  rw [← asciiV_eq _ (by simp; omega) (bytes_take_drop bs hb q 64)]
  exact ha

theorem or_and80 (x y : Nat) (h : (x ||| y) &&& 0x80 = 0) : x &&& 0x80 = 0 ∧ y &&& 0x80 = 0 := by
  rw [Nat.and_or_distrib_right] at h
  exact Nat.or_eq_zero_iff.mp h

theorem or4_ascii (b0 b1 b2 b3 : List Nat) (h : asciiV (or4 b0 b1 b2 b3) = true) :
    asciiV b0 = true ∧ asciiV b1 = true ∧ asciiV b2 = true ∧ asciiV b3 = true := by
  unfold asciiV at *
  simp only [List.all_eq_true, List.mem_range, beq_iff_eq] at h ⊢
  have k : ∀ j, j < 64 → (b0.getD j 0 &&& 0x80 = 0 ∧ b1.getD j 0 &&& 0x80 = 0) ∧
      (b2.getD j 0 &&& 0x80 = 0 ∧ b3.getD j 0 &&& 0x80 = 0) := by
    intro j hj
    have := h j hj
    unfold or4 at this
    rw [getD_range_map _ _ _ hj] at this
    obtain ⟨a, b⟩ := or_and80 _ _ this
    exact ⟨or_and80 _ _ a, or_and80 _ _ b⟩
  exact ⟨fun j hj => (k j hj).1.1, fun j hj => (k j hj).1.2, fun j hj => (k j hj).2.1,
    fun j hj => (k j hj).2.2⟩

theorem skip64_spec (bs : List Nat) (hb : Bytes bs) (q : Nat) (hq : q ≤ bs.length) :
    q ≤ skip64 bs q ∧ skip64 bs q ≤ bs.length ∧
      allAscii ((bs.drop q).take (skip64 bs q - q)) = true := by
  induction q using skip64.induct bs with
  | case1 q h ih =>
    rw [skip64, dite_t h]
    obtain ⟨i1, i2, i3⟩ := ih (by omega)
    refine ⟨by omega, i2, ?_⟩
    rw [take_drop_split bs (b := q + 64) (by omega) i1, allAscii_append, i3, Bool.and_true,
      show q + 64 - q = 64 by omega]
    exact loadu_ascii bs hb q (by omega) h.2
  | case2 q h =>
    rw [skip64, dite_f h]
    simp [allAscii_nil, hq]

theorem skip256_spec (bs : List Nat) (hb : Bytes bs) (q : Nat) (hq : q ≤ bs.length) :
    q ≤ skip256 bs q ∧ skip256 bs q ≤ bs.length ∧
      allAscii ((bs.drop q).take (skip256 bs q - q)) = true := by
  induction q using skip256.induct bs with
  | case1 q h h' =>
    rw [skip256, dite_t h, ite_t h']
    simp [allAscii_nil, hq]
  | case2 q h h' ih =>
    rw [skip256, dite_t h, ite_f h']
    obtain ⟨i1, i2, i3⟩ := ih (by omega)
    refine ⟨by omega, i2, ?_⟩
    have ha : asciiV (or4 (loadu bs q) (loadu bs (q + 64)) (loadu bs (q + 128))
        (loadu bs (q + 192))) = true := by simpa using h'
    obtain ⟨a0, a1, a2, a3⟩ := or4_ascii _ _ _ _ ha
    rw [take_drop_split bs (b := q + 256) (by omega) i1, allAscii_append, i3, Bool.and_true,
      take_drop_split bs (a := q) (b := q + 64) (c := q + 256) (by omega) (by omega),
      take_drop_split bs (a := q + 64) (b := q + 128) (c := q + 256) (by omega) (by omega),
      take_drop_split bs (a := q + 128) (b := q + 192) (c := q + 256) (by omega) (by omega)]
    simp only [show q + 64 - q = 64 by omega, show q + 128 - (q + 64) = 64 by omega,
      show q + 192 - (q + 128) = 64 by omega, show q + 256 - (q + 192) = 64 by omega,
      allAscii_append, loadu_ascii bs hb q (by omega) a0,
      loadu_ascii bs hb (q + 64) (by omega) a1, loadu_ascii bs hb (q + 128) (by omega) a2,
      loadu_ascii bs hb (q + 192) (by omega) a3, Bool.and_self]
  | case3 q h =>
    rw [skip256, dite_f h]
    simp [allAscii_nil, hq]

theorem skipFrom_spec (bs : List Nat) (hb : Bytes bs) (ptr : Nat) (hp : ptr + 64 ≤ bs.length) :
    ptr + 64 ≤ skipFrom bs ptr ∧ skipFrom bs ptr ≤ bs.length ∧
      allAscii ((bs.drop (ptr + 64)).take (skipFrom bs ptr - (ptr + 64))) = true := by
  unfold skipFrom
  split
  · rename_i h
    obtain ⟨a1, a2, a3⟩ := skip256_spec bs hb (ptr + 64 + 64) (by omega)
    obtain ⟨b1, b2, b3⟩ := skip64_spec bs hb _ a2
    refine ⟨by omega, b2, ?_⟩
    rw [take_drop_split bs (b := ptr + 64 + 64) (by omega) (by omega), allAscii_append,
      take_drop_split bs (a := ptr + 64 + 64) (b := skip256 bs (ptr + 64 + 64)) (by omega) b1,
      allAscii_append, a3, b3, show ptr + 64 + 64 - (ptr + 64) = 64 by omega,
      loadu_ascii bs hb (ptr + 64) (by omega) h.2]
    rfl
  · simp [allAscii_nil]; omega

/-! ## 7. The contract -/

/-- ASCII bytes in front of the stream change nothing the invariant says
about the bytes after them: they leave the automaton between characters. -/
theorem inv_zeros {Z X : List Nat} {p3 p2 p1 : Nat} (h : Inv (Z ++ X) p3 p2 p1)
    (hZ : allAscii Z = true) (hX : X ≠ []) : Inv X p3 p2 p1 := by
  have hz : run 0 Z = 0 := run_ascii Z hZ
  have hr : ∀ Y, run 0 (Z ++ Y) = run 0 Y := by intro Y; rw [run_append, hz]
  obtain ⟨s0, hH, hrun, hc⟩ := h
  refine ⟨s0, hH, by rw [← hr]; exact hrun, ?_⟩
  obtain ⟨X', x, rfl⟩ : ∃ X' x, X = X' ++ [x] := by
    rcases List.eq_nil_or_concat X with h | ⟨ys, a, h⟩
    · exact absurd h hX
    · exact ⟨ys, a, by rw [h, List.concat_eq_append]⟩
  rcases hc with h0 | ⟨ys, hys, hr'⟩ | ⟨ys, a, hys, ha, hr'⟩
  · simp at h0
  · rw [← List.append_assoc] at hys
    obtain ⟨e1, e2⟩ := List.append_inj' hys rfl
    simp only [List.cons.injEq, and_true] at e2
    subst e2
    refine Or.inr (Or.inl ⟨X', rfl, ?_⟩)
    rw [← hr, e1, hr']
  · rw [← List.append_assoc] at hys
    obtain ⟨e1, e2⟩ := List.append_inj' hys rfl
    simp only [List.cons.injEq, and_true] at e2
    subst e2
    refine Or.inr (Or.inr ⟨X', x, rfl, ha, ?_⟩)
    rw [← hr, ← List.append_assoc, e1]
    rw [← List.append_assoc, e1] at hr'
    exact hr'

theorem gs_zeros {Z X : List Nat} {t : CS} (h : GS (Z ++ X) t) (hZ : allAscii Z = true)
    (hX : X ≠ []) : GS X t :=
  ⟨h.1, h.2.1, inv_zeros h.2.2 hZ hX⟩

theorem ascii_counts (blk : List Nat) (ha : allAscii blk = true) : conts blk = 0 ∧ fours blk = 0 := by
  constructor
  · rw [← bc_eq]; unfold bc; simp [ha]
  · rw [← bf_eq]; unfold bf; simp [ha]

/-- Skipping ASCII bytes without the checker is checking them: after a
passing ASCII block `prev_incomplete` is clear, and an ASCII block then
changes nothing. -/
theorem gs_extend (bs : List Nat) (hb : Bytes bs) (t : CS) (p q : Nat) (hg : GS (bs.take p) t)
    (hi : t.inc = false) (hpq : p ≤ q) (hq : q ≤ bs.length)
    (ha : allAscii ((bs.drop p).take (q - p)) = true) :
    GS (bs.take q) t ∧ conts (bs.take q) = conts (bs.take p) ∧
      fours (bs.take q) = fours (bs.take p) := by
  have htk : bs.take q = bs.take p ++ (bs.drop p).take (q - p) := by
    rw [← List.take_add]; congr 1; omega
  obtain ⟨c0, f0⟩ := ascii_counts _ ha
  rw [htk, conts_append, fours_append, c0, f0]
  refine ⟨?_, rfl, rfl⟩
  by_cases hpq' : p = q
  · subst hpq'; simpa using hg
  · have hne : (bs.drop p).take (q - p) ≠ [] := by
      intro h0; have := congrArg List.length h0; simp at this; omega
    have e : nextInput false t ((bs.drop p).take (q - p)) = t := by
      obtain ⟨p3, p2, p1, inc, err⟩ := t
      have herr : err = false := hg.1
      simp only at hi
      subst hi herr
      simp [nextInput, ha]
    have := block_sound false hg _ hne (bytes_take_drop bs hb p _) (by rw [e]; exact hg.1)
    rwa [e] at this

/-- The loop invariant: `count` is the offset of `ptr`, the checker passed
the bytes before it, the counters count them. -/
def J (bs : List Nat) (ptr count : Nat) (s : ICS) (C F : Nat) : Prop :=
  count = ptr ∧ ptr ≤ bs.length ∧ GS (bs.take ptr) (proj s) ∧ (∀ b ∈ s.prev, b < 256) ∧
    C = conts (bs.take ptr) ∧ F = fours (bs.take ptr)

theorem loadu_facts (bs : List Nat) (hb : Bytes bs) (p : Nat) (h : p + 64 ≤ bs.length) :
    (loadu bs p).length = 64 ∧ (∀ b ∈ loadu bs p, b < 256) ∧
      bs.take p ++ loadu bs p = bs.take (p + 64) := by
  rw [loadu_eq bs p h]
  exact ⟨by simp; omega, bytes_take_drop bs hb p 64, List.take_add.symm⟩

theorem mainLoop_spec (bs : List Nat) (hb : Bytes bs) (hne : bs ≠ []) :
    ∀ n ptr count s C F, bs.length - ptr ≤ n → J bs ptr count s C F →
      (∀ v, (mainLoop bs ptr count s C F).1 = some v → Good bs v) ∧
      ((mainLoop bs ptr count s C F).1 = none →
        J bs (mainLoop bs ptr count s C F).2.1 (mainLoop bs ptr count s C F).2.2.1
          (mainLoop bs ptr count s C F).2.2.2.1 (mainLoop bs ptr count s C F).2.2.2.2.1
          (mainLoop bs ptr count s C F).2.2.2.2.2 ∧
        bs.length - (mainLoop bs ptr count s C F).2.1 < 64) := by
  intro n
  induction n with
  | zero =>
    intro ptr count s C F hn hJ
    rw [mainLoop, dite_f (by omega)]
    exact ⟨(fun v h => by cases h), fun _ => ⟨hJ, by simp; omega⟩⟩
  | succ n ih =>
    intro ptr count s C F hn hJ
    obtain ⟨hc, hp, hg, hpb, hC, hF⟩ := hJ
    rw [mainLoop]
    by_cases h : 64 ≤ bs.length - ptr
    · rw [dite_t h]
      obtain ⟨hl, hbu, htk⟩ := loadu_facts bs hb ptr (by omega)
      obtain ⟨np, na, nb⟩ := next_proj s (loadu bs ptr) hl hbu hpb
      have hne' : loadu bs ptr ≠ [] := by intro h0; rw [h0] at hl; simp at hl
      dsimp only
      generalize iceNext s (loadu bs ptr) = r at np na nb ⊢
      by_cases he : r.1.err = true
      · rw [ite_t he]
        refine ⟨fun v hv => ?_, fun h => by simp at h⟩
        simp only [Option.some.injEq] at hv
        subst hv
        rw [hc, hC, hF]
        exact rewind_good bs hb hne ptr hp hg.2.2
      · have he' : r.1.err = false := by simpa using he
        rw [ite_f he]
        have hg1 : GS (bs.take (ptr + 64)) (proj r.1) := by
          have := block_sound false hg (loadu bs ptr) hne' hbu (by rw [← np]; exact he')
          rwa [htk, ← np] at this
        by_cases ha : r.2 = true
        · rw [ite_f (by simp [ha])]
          have hasc : allAscii (loadu bs ptr) = true := by rw [← na]; exact ha
          have hinc : (proj r.1).inc = false := by
            have e1 : (proj r.1).err = false := he'
            rw [np] at e1 ⊢
            unfold nextInput at e1 ⊢
            rw [ite_t hasc] at e1 ⊢
            simp only [Bool.or_eq_false_iff] at e1 ⊢
            exact e1.2
          obtain ⟨q1, q2, q3⟩ := skipFrom_spec bs hb ptr (by omega)
          obtain ⟨g1, g2, g3⟩ := gs_extend bs hb _ (ptr + 64) (skipFrom bs ptr) hg1 hinc q1 q2 q3
          obtain ⟨c0, f0⟩ := ascii_counts _ hasc
          apply ih _ _ _ _ _ (by omega)
          refine ⟨by omega, q2, g1, nb, ?_, ?_⟩
          · rw [g2, ← htk, conts_append, c0, hC]; rfl
          · rw [g3, ← htk, fours_append, f0, hF]; rfl
        · have ha' : r.2 = false := by simpa using ha
          rw [ite_t (by simp [ha'])]
          have hasc : allAscii (loadu bs ptr) = false := by rw [← na]; exact ha'
          apply ih _ _ _ _ _ (by omega)
          refine ⟨by omega, by omega, hg1, nb, ?_, ?_⟩
          · rw [← htk, conts_append, hC, iceConts_eq _ hl hbu]
          · rw [← htk, fours_append, hF, iceFours_eq _ hl]
    · rw [dite_f h]
      exact ⟨(fun v h => by cases h), fun _ => ⟨⟨hc, hp, hg, hpb, hC, hF⟩, by simp; omega⟩⟩

theorem zeros_ascii (k : Nat) : allAscii (List.replicate k 0) = true := by
  simp [allAscii, List.all_replicate]

theorem conts_zeros (k : Nat) : conts (List.replicate k 0) = 0 ∧ fours (List.replicate k 0) = 0 :=
  ascii_counts _ (zeros_ascii k)

theorem finish_good (bs : List Nat) (hb : Bytes bs) (hne : bs ≠ []) (ptr count : Nat) (s : ICS)
    (C F : Nat) (hJ : J bs ptr count s C F) (hr : bs.length - ptr < 64) :
    Good bs (finish bs ptr count s C F) := by
  obtain ⟨hc, hp, hg, hpb, hC, hF⟩ := hJ
  unfold finish
  by_cases hpe : bs.length ≠ ptr
  · rw [ite_t hpe]
    have hT := tail_eq bs ptr (by omega) hr
    have hl : (maskzLoad (tailMask (bs.length - ptr)) bs ptr).length = 64 := by
      rw [hT]; simp; omega
    have hbu : ∀ b ∈ maskzLoad (tailMask (bs.length - ptr)) bs ptr, b < 256 := by
      rw [hT]; intro b hm
      rcases List.mem_append.mp hm with h | h
      · exact hb b (List.mem_of_mem_drop h)
      · rw [List.eq_of_mem_replicate h]; decide
    obtain ⟨np, na, -⟩ := next_proj s _ hl hbu hpb
    have hne' : maskzLoad (tailMask (bs.length - ptr)) bs ptr ≠ [] := by
      intro h0; rw [h0] at hl; simp at hl
    dsimp only
    generalize iceNext s (maskzLoad (tailMask (bs.length - ptr)) bs ptr) = r at np na ⊢
    by_cases he : (eof r.1).err = true
    · rw [ite_t he]
      rw [hc, hC, hF]
      exact rewind_good bs hb hne ptr hp hg.2.2
    · rw [ite_f he]
      have he' : r.1.err = false ∧ r.1.inc = false := by
        simpa [eof] using he
      have hg1 := block_sound false hg _ hne' hbu (by rw [← np]; exact he'.1)
      rw [← np] at hg1
      have h0 := gs_end hg1 he'.2
      rw [hT, ← List.append_assoc, List.take_append_drop, run_append] at h0
      have hv : Valid8 bs := run_ascii_zero _ _ (fun b hb' => by
        rw [List.eq_of_mem_replicate hb']; decide) h0
      have hcnt : (if !r.2 then C + iceConts (maskzLoad (tailMask (bs.length - ptr)) bs ptr) else C)
            = conts bs ∧
          (if !r.2 then F + iceFours (maskzLoad (tailMask (bs.length - ptr)) bs ptr) else F)
            = fours bs := by
        rw [iceConts_eq _ hl hbu, iceFours_eq _ hl]
        have hsplit : conts bs = C + conts (maskzLoad (tailMask (bs.length - ptr)) bs ptr) ∧
            fours bs = F + fours (maskzLoad (tailMask (bs.length - ptr)) bs ptr) := by
          rw [hT, conts_append, fours_append, (conts_zeros _).1, (conts_zeros _).2, hC, hF,
            Nat.add_zero, Nat.add_zero, ← conts_append, ← fours_append, List.take_append_drop]
          exact ⟨rfl, rfl⟩
        rw [hsplit.1, hsplit.2]
        cases hr2 : r.2
        · simp
        · have := ascii_counts _ (by rw [← na]; exact hr2)
          simp [this.1, this.2]
      rw [hcnt.1, hcnt.2]
      refine ⟨fun _ => ⟨hv, ?_, rfl⟩, fun h => by cases h⟩
      simp only
      rw [← L_w16, Int.toNat_natCast]
  · have hpe' : bs.length = ptr := by simpa using hpe
    rw [ite_f hpe]
    by_cases he : (eof s).err = true
    · rw [ite_t he]
      rw [hc, hC, hF]
      exact rewind_good bs hb hne ptr hp hg.2.2
    · rw [ite_f he]
      have he' : s.err = false ∧ s.inc = false := by simpa [eof] using he
      have h0 := gs_end hg he'.2
      rw [← hpe', List.take_length] at h0 hC hF
      refine ⟨fun _ => ⟨h0, ?_, rfl⟩, fun h => by cases h⟩
      simp only
      rw [hC, hF, ← L_w16, Int.toNat_natCast]

theorem body_good (bs : List Nat) (hb : Bytes bs) (hne : bs ≠ []) (ptr count : Nat) (s : ICS)
    (C F : Nat) (hJ : J bs ptr count s C F) : Good bs (body bs ptr count s C F) := by
  obtain ⟨m1, m2⟩ := mainLoop_spec bs hb hne _ ptr count s C F (Nat.le_refl _) hJ
  unfold body
  dsimp only
  generalize mainLoop bs ptr count s C F = r at m1 m2 ⊢
  obtain ⟨o, ptr', count', s', C', F'⟩ := r
  cases o with
  | some v => exact m1 v rfl
  | none =>
    obtain ⟨j, hr⟩ := m2 rfl
    exact finish_good bs hb hne _ _ _ _ _ j hr

theorem proj_ics0 : proj ics0 = cs0 := rfl

/-- **Every return of Ice Lake `validate_utf8_with_counts`**, for every
address of `buf`, meets the contract. -/
theorem iceV_good (misal : Nat) (bs : List Nat) (hb : Bytes bs) (hne : bs ≠ []) :
    Good bs (iceV misal bs) := by
  have hlen : bs.length ≠ 0 := by simpa using hne
  unfold iceV
  rw [ite_f hlen]
  by_cases hh : 1024 ≤ bs.length ∧ misal % 64 ≠ 0
  · rw [ite_t hh]
    have hm : misal % 64 < 64 := Nat.mod_lt _ (by decide)
    have hH := head_eq bs (misal % 64) (by omega) hm (by omega)
    have hl : (maskzLoad (headMask (misal % 64)) bs (-((misal % 64 : Nat) : Int))).length = 64 := by
      rw [hH]; simp; omega
    have hbu : ∀ b ∈ maskzLoad (headMask (misal % 64)) bs (-((misal % 64 : Nat) : Int)), b < 256 := by
      rw [hH]; intro b hmem
      rcases List.mem_append.mp hmem with h | h
      · rw [List.eq_of_mem_replicate h]; decide
      · exact hb b (List.mem_of_mem_take h)
    obtain ⟨np, na, nb⟩ := next_proj ics0 _ hl hbu (fun b h => by
      rw [show ics0.prev = List.replicate 64 0 from rfl] at h
      rw [List.eq_of_mem_replicate h]; decide)
    rw [proj_ics0] at np
    have hne' : maskzLoad (headMask (misal % 64)) bs (-((misal % 64 : Nat) : Int)) ≠ [] := by
      intro h0; rw [h0] at hl; simp at hl
    dsimp only
    generalize iceNext ics0 (maskzLoad (headMask (misal % 64)) bs (-((misal % 64 : Nat) : Int))) = r
      at np na nb ⊢
    by_cases he : r.1.err = true
    · rw [ite_t he]
      have := rewind_good bs hb hne 0 (by omega) (p3 := 0) (p2 := 0) (p1 := 0)
        (by rw [List.take_zero]; exact inv_nil)
      simpa [conts, fours] using this
    · have he' : r.1.err = false := by simpa using he
      rw [ite_f he]
      have hg1 := block_sound false gs0 _ hne' hbu (by rw [← np]; exact he')
      rw [← np, List.nil_append, hH] at hg1
      have hX : bs.take (64 - misal % 64) ≠ [] := by
        apply List.ne_nil_of_length_pos
        rw [List.length_take]; omega
      have hg2 := gs_zeros hg1 (zeros_ascii _) hX
      have hcnt : conts (maskzLoad (headMask (misal % 64)) bs (-((misal % 64 : Nat) : Int))) =
            conts (bs.take (64 - misal % 64)) ∧
          fours (maskzLoad (headMask (misal % 64)) bs (-((misal % 64 : Nat) : Int))) =
            fours (bs.take (64 - misal % 64)) := by
        rw [hH, conts_append, fours_append, (conts_zeros _).1, (conts_zeros _).2]
        exact ⟨by omega, by omega⟩
      apply body_good bs hb hne
      refine ⟨rfl, by omega, hg2, nb, ?_, ?_⟩
      · rw [iceConts_eq _ hl hbu, ← hcnt.1]
        cases hr2 : r.2
        · simp
        · have := ascii_counts _ (by rw [← na]; exact hr2)
          rw [this.1]; rfl
      · rw [iceFours_eq _ hl, ← hcnt.2]
        cases hr2 : r.2
        · simp
        · have := ascii_counts _ (by rw [← na]; exact hr2)
          rw [this.2]; rfl
  · rw [ite_f hh]
    apply body_good bs hb hne
    refine ⟨rfl, by omega, ?_, fun b h => ?_, rfl, rfl⟩
    · rw [List.take_zero, proj_ics0]; exact gs0
    · rw [show ics0.prev = List.replicate 64 0 from rfl] at h
      rw [List.eq_of_mem_replicate h]; decide

/-- **Ice Lake `validate_utf8_with_counts` meets `ReplacementRest.VSpec2`**,
for every address of the input (every misalignment): on success the input is
valid UTF-8, `utf16_length()` is `L` of its `w16` weights and `input_count`
is its length; on a validation error at `p < len` the bytes before `p` are
valid with `utf16_length()` units and `dchar` fails at `p`. -/
theorem iceV_spec2 (misal : Nat) : ReplacementRest.VSpec2 (iceV misal) := by
  refine ⟨fun ys hb hne => ?_, fun ys hb hne h => ((iceV_good misal ys hb hne).1 h).2.2⟩
  obtain ⟨g1, g2⟩ := iceV_good misal ys hb hne
  exact ⟨fun h => ⟨(g1 h).1, (g1 h).2.1⟩, g2⟩

end SafetyMargins.IcelakeValidator
