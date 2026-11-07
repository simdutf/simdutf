import SafetyMargins.ReplacementMore
/-
Soundness of the SIMD UTF-8 validator, and the vector `validate_utf8_with_counts`.

`ValidExact.checkedAt_of_valid` is the completeness direction for one block:
valid UTF-8 passes `check_utf8_bytes`. Here the whole-input validator of
src/generic/utf8_validation/utf8_lookup4_algorithm.h and
src/generic/utf8_validation/utf8_validator.h is modelled as the C++ runs it,
and both directions are proved for it.

* **The model.** `nextInput` is `check_next_input` (src/generic/utf8_validation/utf8_lookup4_algorithm.h:215-239): an
  ASCII block only ORs `prev_incomplete` into the error (:216-218) and keeps
  the carried `prev_input_block` and `prev_incomplete`, which are then stale;
  any other block runs `check_utf8_bytes` (`Validator.err`, both
  `must_be_2_3_continuation`) on every byte with the three bytes before it,
  the first ones taken from the carried block, and sets `prev_incomplete`
  (`isInc`, `is_incomplete`, :115-154) and `prev_input_block` from its last
  bytes. `loop` is the loop over full blocks of
  `generic_validate_utf8_with_counts` (utf8_validator.h:119-135,
  `has_full_block` is `idx + 64 < len`, src/generic/buf_block_reader.h:90);
  `checkAll` adds the padded last block
  (src/generic/utf8_validation/utf8_validator.h:136-141, `get_remainder` pads with
  0x20, buf_block_reader.h:102-111) and `check_eof` (src/generic/utf8_validation/utf8_lookup4_algorithm.h:207-211).
* **Soundness (`flag_sound`).** If the checker flags nothing, the input is
  `Valid8` (Unicode Table 3-7: no overlong form, no surrogate, nothing above
  U+10FFFF, no truncated or stray continuation, also at the end).
  Completeness (`flag_complete`) and the equivalence (`flag_iff`) follow
  with the same invariant.
* **The finite fact (`sound_closure`, `decide +kernel`).** The check sees
  the byte `c`, the byte `p1` before it and three bits of the two bytes
  before that. `H` relates them to the automaton state *before* `p1`: a byte
  that cannot start a character (`C0`, `C1`, `F5`-`FF`) is caught only at
  the next byte (`check_special_cases` tests the first byte of each pair), so
  the invariant runs one byte behind. `sound_closure` checks, over all
  states, bits, `p1` and `c`, that a byte that passes keeps `H`.
* **`validate_utf8_with_counts` (`vecV`, `vecV_spec`).** On an error the C++
  rewinds at most 4 bytes to a leading byte and runs the scalar
  `validate_with_counts` from there (include/simdutf/scalar/utf8.h:400-448,
  transcribed as `rewindV`, the scalar loop as `vwc4F`), then adds back the
  counts of the blocks before. `vecV_spec` proves `ReplacementMore.VSpec`
  for it: on success the input is valid and `utf16_length()` is `L` of its
  `w16` weights; on an error at `p`, the prefix is valid with that many
  units and `dchar` fails at `p`. So `lenRepl_eq` and the `LReach` theorems
  of `ReplacementMore` hold for the vector validator with no hypothesis
  (`lenRepl_vec`, `lreach_rreach_vec`, `lreach_tail_vec`).
* **The bulk `_with_errors` kernel (`KR`, `KOut`, `kout_spec`).**
  `convert_with_errors` of src/generic/utf8_to_utf16/utf8_to_utf16.h:270-377
  checks each non-ASCII block with `prev_input = 0` (`Exact.CheckedAt`); a
  checked block that starts on a character boundary is valid up to each of
  its leading bytes (`checked_lead`), so the kernel only ever commits a valid
  prefix with `L` of its `w16` units (`kr_inv`). On an error it runs
  `rewind_and_convert_with_errors` (`rewindK`), whose offset is inside the
  window, after a valid prefix, where `dchar` fails (`rewindK_spec`); after a
  failed block the rewind never succeeds (`ValidExact.checkedAt_of_valid`).
* **The driver (`DR`, `dr_rreach`, `transcode_exact`).**
  `transcode_utf8_to_utf16_with_replacement` with these kernels: every
  state is a `Replacement.RReach` state; an error inside the window is an
  error of the input, because the window ends before a byte that is not a
  continuation byte (`dchar_lift`, `boundary_spec`); the returned count is the
  buffer size `utf16_length_from_utf8_with_replacement` computes, and every
  store of the bulk kernel ends 7 units inside the buffer.

Still read from the C++: the transcriptions themselves (as everywhere in this
library); that each `convert_masked_utf8_to_utf16` call of a checked block is
exact and ends by `pos + 63` (`KR.call`: `XStep` and `hd`, which
`Exact.exact_utf16_145`, `exact_utf16_3`, `exact_count`, `exact_888` and
`ValidExact.row_hd` give per call shape); that `utf16_length_from_utf8` and
`convert_valid_prefix` return `L` of the `w16` weights of a valid prefix;
that `arm_validate_utf8_with_counts` sums the same counts (its int8 lane
accumulators are reduced before they overflow). The icelake and rvv
validators and kernels have their own code and are not modelled here; the
fallback kernel is the scalar code (`vwcF`).

The C++ computes the counts in `size_t` and lets the subtractions of the
rewind wrap (utf8.h:392-399: "the unsigned wraparound cancels exactly"); the
model computes them in `Int`, which is the same value whenever the result
lies in `[0, 2^64)`, as it does for any input that fits in memory.

`decide +kernel` is used for finite exhaustive checks only: `sound_closure`
(2 modes × 8 states × 8 bit patterns × 256 × 256 byte pairs), `inc_ok`,
`last_ok` (8 × 8 × 256), `cont_rank` (9 × 256), and the examples. No
`native_decide`.
-/
namespace SafetyMargins.ValidatorSound
open SafetyMargins SafetyMargins.Validator SafetyMargins.ValidExact
open SafetyMargins.ReplacementMore (Bytes vchar dchar vwcF vwc vwcF_spec VRes VSpec)
open SafetyMargins.Utf8 (w16)
set_option autoImplicit false

/-! ## The invariant and its finite closure -/

/-- A byte that cannot start a character and is not a continuation byte:
`C0`, `C1`, `F5`-`FF`. `check_special_cases` catches it only at the next byte
(src/generic/utf8_validation/utf8_lookup4_algorithm.h:204-206: "check_special_cases only checks for bytes too large in the
first of two bytes"). -/
def badLead (b : Nat) : Bool := decide (0xC0 ≤ b ∧ b < 0xC2) || decide (0xF5 ≤ b ∧ b < 256)

/-- What a passing check says, one byte behind: `s0` is the automaton state
before `p1`; `f3`, `e2`, `f2` are `p3 ≥ 0xF0`, `p2 ≥ 0xE0`, `p2 ≥ 0xF0`. Either
`p1` is a bad lead after a complete character, or the state after `p1` is one
`ValidExact.goodA` allows. -/
def H (s0 : Nat) (f3 e2 f2 : Bool) (p1 : Nat) : Bool :=
  decide (p1 < 256) && decide (s0 < 8) &&
  (if step s0 p1 = 8 then s0 == 0 && badLead p1 && !f3 && !e2 && !f2
   else goodA (step s0 p1) f3 e2 f2 p1)

/-- From every `H` configuration, a byte `c` that passes the check (`sc` agrees
with `must23`) makes the automaton accept `p1`, and `H` holds one byte on. -/
def soundOK : Bool :=
  bools.all fun xm => (List.range 8).all fun s => bools.all fun f3 => bools.all fun e2 =>
    bools.all fun f2 => (List.range 256).all fun p1 => !H s f3 e2 f2 p1 ||
      (List.range 256).all fun c => !(sc p1 c == (if m23 xm f3 e2 then 0x80 else 0)) ||
        (step s p1 != 8 && H (step s p1) f2 (decide (0xE0 ≤ p1)) (decide (0xF0 ≤ p1)) c)

set_option maxRecDepth 100000 in
theorem sound_closure : soundOK = true := by decide +kernel

theorem H_lt {s0 : Nat} {f3 e2 f2 : Bool} {p1 : Nat} (h : H s0 f3 e2 f2 p1 = true) :
    p1 < 256 ∧ s0 < 8 := by
  unfold H at h
  simp only [Bool.and_eq_true, decide_eq_true_eq] at h
  exact ⟨h.1.1, h.1.2⟩

theorem sound_at (xm : Bool) {s0 : Nat} {f3 e2 f2 : Bool} {p1 c : Nat}
    (hH : H s0 f3 e2 f2 p1 = true) (hc : c < 256)
    (hp : sc p1 c = if m23 xm f3 e2 then 0x80 else 0) :
    step s0 p1 ≠ 8 ∧ H (step s0 p1) f2 (decide (0xE0 ≤ p1)) (decide (0xF0 ≤ p1)) c = true := by
  have h := sound_closure
  obtain ⟨hp1, hs0⟩ := H_lt hH
  unfold soundOK at h
  have h := List.all_eq_true.mp h xm (mem_bools xm)
  have h := List.all_eq_true.mp h s0 (List.mem_range.mpr hs0)
  have h := List.all_eq_true.mp h f3 (mem_bools f3)
  have h := List.all_eq_true.mp h e2 (mem_bools e2)
  have h := List.all_eq_true.mp h f2 (mem_bools f2)
  have h := List.all_eq_true.mp h p1 (List.mem_range.mpr hp1)
  rw [hH] at h
  simp only [Bool.not_true, Bool.false_or] at h
  have h := List.all_eq_true.mp h c (List.mem_range.mpr hc)
  rw [hp] at h
  simp only [beq_self_eq_true, Bool.not_true, Bool.false_or, Bool.and_eq_true, bne_iff_ne,
    ne_eq] at h
  exact h

/-- With `prev_incomplete` clear (`p3 ≤ 0xEF`, `p2 ≤ 0xDF`, `p1 ≤ 0xBF`), the
automaton is between characters after `p1`. -/
def incOK : Bool :=
  (List.range 8).all fun s => bools.all fun f2 => (List.range 256).all fun p1 =>
    !H s false false f2 p1 || !decide (p1 < 0xC0) || step s p1 == 0

theorem inc_ok : incOK = true := by decide +kernel

theorem H_inc {s0 : Nat} {f2 : Bool} {p1 : Nat} (h : H s0 false false f2 p1 = true)
    (hp : p1 < 0xC0) : step s0 p1 = 0 := by
  obtain ⟨hp1, hs0⟩ := H_lt h
  have k := inc_ok
  unfold incOK at k
  have k := List.all_eq_true.mp k s0 (List.mem_range.mpr hs0)
  have k := List.all_eq_true.mp k f2 (mem_bools f2)
  have k := List.all_eq_true.mp k p1 (List.mem_range.mpr hp1)
  rw [h] at k
  simpa [hp] using k

/-- The last byte: a continuation byte is accepted, before a non-continuation
byte the automaton is between characters. -/
def lastOK : Bool :=
  (List.range 8).all fun s => bools.all fun f3 => bools.all fun e2 => bools.all fun f2 =>
    (List.range 256).all fun p1 => !H s f3 e2 f2 p1 ||
      ((!isCont p1 || step s p1 != 8) && (isCont p1 || s == 0))

theorem last_ok : lastOK = true := by decide +kernel

theorem H_last {s0 : Nat} {f3 e2 f2 : Bool} {p1 : Nat} (h : H s0 f3 e2 f2 p1 = true) :
    (isCont p1 = true → step s0 p1 ≠ 8) ∧ (isCont p1 = false → s0 = 0) := by
  obtain ⟨hp1, hs0⟩ := H_lt h
  have k := last_ok
  unfold lastOK at k
  have k := List.all_eq_true.mp k s0 (List.mem_range.mpr hs0)
  have k := List.all_eq_true.mp k f3 (mem_bools f3)
  have k := List.all_eq_true.mp k e2 (mem_bools e2)
  have k := List.all_eq_true.mp k f2 (mem_bools f2)
  have k := List.all_eq_true.mp k p1 (List.mem_range.mpr hp1)
  rw [h] at k
  simp only [Bool.not_true, Bool.false_or, Bool.and_eq_true, Bool.or_eq_true,
    Bool.not_eq_true', bne_iff_ne, ne_eq, beq_iff_eq] at k
  refine ⟨fun hc => ?_, fun hc => ?_⟩
  · rcases k.1 with k | k
    · rw [hc] at k; cases k
    · exact k
  · rcases k.2 with k | k
    · rw [hc] at k; cases k
    · exact k

theorem H_goodA {s0 : Nat} {f3 e2 f2 : Bool} {p1 : Nat} (h : H s0 f3 e2 f2 p1 = true)
    (hs : step s0 p1 ≠ 8) : goodA (step s0 p1) f3 e2 f2 p1 = true := by
  unfold H at h
  simp only [hs, ↓reduceIte, Bool.and_eq_true] at h
  exact h.2

theorem H_of_goodA {s0 : Nat} {f3 e2 f2 : Bool} {p1 : Nat} (hp : p1 < 256) (hs0 : s0 < 8)
    (hs : step s0 p1 ≠ 8) (h : goodA (step s0 p1) f3 e2 f2 p1 = true) :
    H s0 f3 e2 f2 p1 = true := by
  unfold H
  simp only [hs, ↓reduceIte, Bool.and_eq_true, decide_eq_true_eq]
  exact ⟨⟨hp, hs0⟩, h⟩

theorem goodA0_inc {f3 e2 f2 : Bool} {p1 : Nat} (h : goodA 0 f3 e2 f2 p1 = true) :
    f3 = false ∧ e2 = false ∧ p1 < 0xC0 := by
  unfold goodA at h
  simp only [↓reduceIte, Bool.and_eq_true, Bool.not_eq_true', Bool.or_eq_true,
    decide_eq_true_eq] at h
  refine ⟨h.1.1.1, h.1.1.2, ?_⟩
  rcases h.2 with h | h
  · omega
  · simp only [isCont, Bool.and_eq_true, decide_eq_true_eq] at h; omega

/-! ## The checker -/

/-- The state of `utf8_checker` (src/generic/utf8_validation/utf8_lookup4_algorithm.h:182-189) that matters: the last three
bytes of `prev_input_block` (`prev<k>` for `k ≤ 3` reads no further back),
`prev_incomplete` (nonzero or not) and `error` (nonzero or not). -/
structure CS where
  p3 : Nat
  p2 : Nat
  p1 : Nat
  inc : Bool
  err : Bool

/-- `utf8_checker c{}`: everything zero. -/
def cs0 : CS := ⟨0, 0, 0, false, false⟩

/-- `is_incomplete` (src/generic/utf8_validation/utf8_lookup4_algorithm.h:115-154): some byte of the last chunk exceeds the
`max_array` entry for its lane; only the last three entries are below 255:
`0b11110000 - 1`, `0b11100000 - 1`, `0b11000000 - 1`. -/
def isInc (p3 p2 p1 : Nat) : Bool :=
  decide (0xEF < p3) || decide (0xDF < p2) || decide (0xBF < p1)

/-- `check_utf8_bytes` (src/generic/utf8_validation/utf8_lookup4_algorithm.h:194-202) over the bytes of a block, with the
three bytes before each (the first from the carried bytes): whether some
byte has nonzero `check_multibyte_lengths` (`Validator.err`), and the last
three bytes. -/
def scan (xm : Bool) : Nat → Nat → Nat → List Nat → Bool × Nat × Nat × Nat
  | p3, p2, p1, [] => (false, p3, p2, p1)
  | p3, p2, p1, c :: r =>
    let t := scan xm p2 p1 c r
    (Validator.err xm p3 p2 p1 c != 0 || t.1, t.2)

/-- `is_ascii(input)`. -/
def allAscii (blk : List Nat) : Bool := blk.all fun b => decide (b < 0x80)

/-- `check_next_input` (src/generic/utf8_validation/utf8_lookup4_algorithm.h:215-239; the `_with_counts` copy is
:280-308). An ASCII block only ORs `prev_incomplete` into `error` and leaves
`prev_input_block` and `prev_incomplete` as they were. -/
def nextInput (xm : Bool) (s : CS) (blk : List Nat) : CS :=
  if allAscii blk then { s with err := s.err || s.inc }
  else
    let t := scan xm s.p3 s.p2 s.p1 blk
    ⟨t.2.1, t.2.2.1, t.2.2.2, isInc t.2.1 t.2.2.1 t.2.2.2, s.err || t.1⟩

/-- Continuation bytes, `(int8_t)b < -65 + 1` (`utf8_counters`, src/generic/utf8_validation/utf8_lookup4_algorithm.h:167-180). -/
def conts (xs : List Nat) : Nat := xs.countP fun b => decide (0x80 ≤ b ∧ b < 0xC0)
/-- Four-byte leads, `b >= 0b11110000`. -/
def fours (xs : List Nat) : Nat := xs.countP fun b => decide (0xF0 ≤ b)

/-- What `check_next_input_with_counts` adds to the counters: nothing on an
ASCII block (src/generic/utf8_validation/utf8_lookup4_algorithm.h:281-283). -/
def bc (blk : List Nat) : Nat := if allAscii blk then 0 else conts blk
def bf (blk : List Nat) : Nat := if allAscii blk then 0 else fours blk

/-- The loop over full blocks of `generic_validate_utf8_with_counts`
(utf8_validator.h:119-135). Returns whether a block was flagged, the offset
`count` of the last block read, the checker state before it, and the counters
before it (`c.continuation_count() - last_counts.continuations`, :128-130).
On an error the C++ goes on with these only. -/
def loop (xm : Bool) (bs : List Nat) (idx : Nat) (s : CS) (C F : Nat) :
    Bool × Nat × CS × Nat × Nat :=
  if _h : idx + 64 < bs.length then
    let blk := (bs.drop idx).take 64
    if (nextInput xm s blk).err then (true, idx, s, C, F)
    else loop xm bs (idx + 64) (nextInput xm s blk) (C + bc blk) (F + bf blk)
  else (false, idx, s, C, F)
termination_by bs.length - idx

/-- `reader.get_remainder(block)` into `uint8_t block[64]{}`
(utf8_validator.h:136-137, buf_block_reader.h:102-111): nothing is copied when
`len == idx` (the block stays zero), otherwise 0x20 padding. -/
def remBlock (bs : List Nat) (idx : Nat) : List Nat :=
  if bs.length ≤ idx then List.replicate 64 0
  else bs.drop idx ++ List.replicate (64 - (bs.length - idx)) 0x20

/-- The whole check of `generic_validate_utf8_with_counts`: the full blocks,
then the padded last block and `check_eof`
(src/generic/utf8_validation/utf8_validator.h:136-142). Returns whether the
checker flags an error (the C++ then rewinds, :122 or :142), the offset
`count`, and the counters (before the flagged block, or in total). -/
def checkAll (xm : Bool) (bs : List Nat) : Bool × Nat × Nat × Nat :=
  let r := loop xm bs 0 cs0 0 0
  if r.1 then (true, r.2.1, r.2.2.2.1, r.2.2.2.2)
  else
    let R := remBlock bs r.2.1
    let s2 := nextInput xm r.2.2.1 R
    if s2.err || s2.inc then (true, r.2.1, r.2.2.2.1, r.2.2.2.2)
    else (false, r.2.1, r.2.2.2.1 + bc R, r.2.2.2.2 + bf R)

/-! ## The stream invariant -/

/-- After the bytes `xs`, with carried bytes `p3 p2 p1`: `H` holds for some
state `s0` before `p1`, the automaton state after `xs` is `step s0 p1`, and
either nothing was read, or `p1` is the last byte read and `s0` the state
before it, or the carried bytes are stale (an ASCII block was read last)
and the automaton is between characters. -/
def Inv (xs : List Nat) (p3 p2 p1 : Nat) : Prop :=
  ∃ s0, H s0 (decide (0xF0 ≤ p3)) (decide (0xE0 ≤ p2)) (decide (0xF0 ≤ p2)) p1 = true ∧
    run 0 xs = step s0 p1 ∧
    (xs = [] ∨ (∃ ys, xs = ys ++ [p1] ∧ run 0 ys = s0) ∨
     (∃ ys a, xs = ys ++ [a] ∧ a < 0x80 ∧ run 0 xs = 0))

theorem inv_nil : Inv [] 0 0 0 := ⟨0, by decide, rfl, Or.inl rfl⟩

theorem run_snoc (s : Nat) (xs : List Nat) (c : Nat) : run s (xs ++ [c]) = step (run s xs) c := by
  rw [run_append]; rfl

theorem run_prefix_ne8 {s : Nat} {a b : List Nat} (h : run s (a ++ b) ≠ 8) : run s a ≠ 8 := by
  intro h8; rw [run_append, h8, run_8] at h; exact h rfl

theorem err_zero_iff (xm : Bool) (p3 p2 p1 c : Nat) :
    Validator.err xm p3 p2 p1 c = 0 ↔
      sc p1 c = if m23 xm (decide (0xF0 ≤ p3)) (decide (0xE0 ≤ p2)) then 0x80 else 0 := by
  unfold Validator.err
  rw [must23_m23]
  constructor
  · intro h; exact (xor_zero h).symm
  · intro h; rw [h]; exact Nat.xor_self _

theorem step0_ascii (b : Nat) (h : b < 0x80) : step 0 b = 0 := by
  rcases step0_cases b with h' | h' | h' | h' | h' | h' | h' | h' | h' | h' <;> omega

theorem step_ascii_ne (s b : Nat) (hb : b < 0x80) (hs : step s b ≠ 8) : s = 0 := by
  refine Classical.byContradiction fun h0 => ?_
  exact hs (step_noncont s b h0 (by simp [isCont]; omega))

theorem run_ascii (blk : List Nat) (h : allAscii blk = true) : run 0 blk = 0 := by
  induction blk with
  | nil => rfl
  | cons b r ih =>
    simp only [allAscii, List.all_cons, Bool.and_eq_true, decide_eq_true_eq] at h
    rw [run_cons, step0_ascii b h.1]
    exact ih h.2

/-- ASCII bytes after a state that is not between characters fail. -/
theorem run_ascii_zero : ∀ (pad : List Nat) (s : Nat), (∀ b ∈ pad, b < 0x80) → run s pad = 0 → s = 0
  | [], s, _, h => h
  | b :: r, s, hp, h => by
    rw [run_cons] at h
    by_cases hs : s = 0
    · exact hs
    · rw [step_noncont s b hs (by simp [isCont]; have := hp b (by simp); omega), run_8] at h
      cases h

/-- **Soundness, one block of `check_utf8_bytes`.** -/
theorem scan_sound (xm : Bool) : ∀ (blk xs : List Nat) (p3 p2 p1 : Nat), Inv xs p3 p2 p1 →
    (∀ c ∈ blk, c < 256) → (scan xm p3 p2 p1 blk).1 = false →
    Inv (xs ++ blk) (scan xm p3 p2 p1 blk).2.1 (scan xm p3 p2 p1 blk).2.2.1
      (scan xm p3 p2 p1 blk).2.2.2 := by
  intro blk
  induction blk with
  | nil => intro xs p3 p2 p1 h _ _; simpa [scan] using h
  | cons c r ih =>
    intro xs p3 p2 p1 h hb hs
    simp only [scan, Bool.or_eq_false_iff, bne_eq_false_iff_eq] at hs ⊢
    obtain ⟨s0, hH, hrun, -⟩ := h
    have hc : c < 256 := hb c (by simp)
    obtain ⟨h8, hH'⟩ := sound_at xm hH hc ((err_zero_iff xm p3 p2 p1 c).mp hs.1)
    have h1 : Inv (xs ++ [c]) p2 p1 c :=
      ⟨step s0 p1, hH', by rw [run_snoc, hrun], Or.inr (Or.inl ⟨xs, rfl, hrun⟩)⟩
    have := ih (xs ++ [c]) p2 p1 c h1 (fun x hx => hb x (by simp [hx])) hs.2
    simpa using this

/-- **Completeness, one block**: valid bytes pass, and the invariant goes on. -/
theorem scan_complete (xm : Bool) : ∀ (blk xs : List Nat) (p3 p2 p1 : Nat), Inv xs p3 p2 p1 →
    (∀ c ∈ blk, c < 256) → run 0 (xs ++ blk) ≠ 8 →
    (scan xm p3 p2 p1 blk).1 = false ∧
    Inv (xs ++ blk) (scan xm p3 p2 p1 blk).2.1 (scan xm p3 p2 p1 blk).2.2.1
      (scan xm p3 p2 p1 blk).2.2.2 := by
  intro blk
  induction blk with
  | nil => intro xs p3 p2 p1 h _ _; simpa [scan] using h
  | cons c r ih =>
    intro xs p3 p2 p1 h hb hv
    obtain ⟨s0, hH, hrun, -⟩ := h
    have hc : c < 256 := hb c (by simp)
    obtain ⟨hp1, hs0⟩ := H_lt hH
    have hv1 : run 0 (xs ++ [c]) ≠ 8 := run_prefix_ne8 (b := r) (by simpa using hv)
    rw [run_snoc, hrun] at hv1
    have hs1 : step s0 p1 ≠ 8 := fun h8 => hv1 (by rw [h8, step_8])
    obtain ⟨hsc, hg⟩ := closure_at xm (H_goodA hH hs1) hp1 hc hv1
    have hle := step_le s0 p1
    have hH' := H_of_goodA hc (by omega) hv1 hg
    have h1 : Inv (xs ++ [c]) p2 p1 c :=
      ⟨step s0 p1, hH', by rw [run_snoc, hrun], Or.inr (Or.inl ⟨xs, rfl, hrun⟩)⟩
    have hz : Validator.err xm p3 p2 p1 c = 0 := (err_zero_iff xm p3 p2 p1 c).mpr hsc
    obtain ⟨i1, i2⟩ := ih (xs ++ [c]) p2 p1 c h1 (fun x hx => hb x (by simp [hx])) (by simpa using hv)
    simp only [scan, hz, bne_self_eq_false, i1, Bool.or_self, true_and]
    simpa using i2

/-- The checker state is consistent with the bytes read. -/
def GS (xs : List Nat) (s : CS) : Prop :=
  s.err = false ∧ s.inc = isInc s.p3 s.p2 s.p1 ∧ Inv xs s.p3 s.p2 s.p1

theorem gs0 : GS [] cs0 := ⟨rfl, rfl, inv_nil⟩

theorem isInc_false {p3 p2 p1 : Nat} (h : isInc p3 p2 p1 = false) :
    decide (0xF0 ≤ p3) = false ∧ decide (0xE0 ≤ p2) = false ∧ p1 < 0xC0 := by
  simp only [isInc, Bool.or_eq_false_iff, decide_eq_false_iff_not] at h
  simp only [decide_eq_false_iff_not]
  omega

/-- With `prev_incomplete` clear, the automaton is between characters. -/
theorem gs_end {xs : List Nat} {s : CS} (h : GS xs s) (hi : s.inc = false) : run 0 xs = 0 := by
  obtain ⟨-, hinc, s0, hH, hrun, -⟩ := h
  rw [hinc] at hi
  obtain ⟨a, b, c⟩ := isInc_false hi
  rw [a, b] at hH
  rw [hrun, H_inc hH c]

/-- Between characters, `prev_incomplete` is clear. -/
theorem gs_inc {xs : List Nat} {s : CS} (h : GS xs s) (h0 : run 0 xs = 0) : s.inc = false := by
  obtain ⟨-, hinc, s0, hH, hrun, -⟩ := h
  rw [hrun] at h0
  have g := H_goodA hH (by rw [h0]; decide)
  rw [h0] at g
  obtain ⟨a, b, c⟩ := goodA0_inc g
  rw [hinc]
  simp only [isInc, Bool.or_eq_false_iff, decide_eq_false_iff_not]
  simp only [decide_eq_false_iff_not] at a b
  omega

theorem inv_ascii {xs : List Nat} {p3 p2 p1 : Nat} (h : Inv xs p3 p2 p1) (h0 : run 0 xs = 0)
    (blk : List Nat) (hne : blk ≠ []) (ha : allAscii blk = true) : Inv (xs ++ blk) p3 p2 p1 := by
  obtain ⟨s0, hH, hrun, -⟩ := h
  have hr : run 0 (xs ++ blk) = 0 := by rw [run_append, h0, run_ascii blk ha]
  refine ⟨s0, hH, by rw [hr, ← hrun, h0], Or.inr (Or.inr ?_)⟩
  obtain ⟨ys, a, rfl⟩ : ∃ ys a, blk = ys ++ [a] := by
    rcases List.eq_nil_or_concat blk with h | ⟨ys, a, h⟩
    · exact absurd h hne
    · exact ⟨ys, a, by rw [h, List.concat_eq_append]⟩
  refine ⟨xs ++ ys, a, by simp, ?_, hr⟩
  simp only [allAscii, List.all_append, List.all_cons, List.all_nil, Bool.and_true,
    Bool.and_eq_true, decide_eq_true_eq] at ha
  exact ha.2

/-- **Soundness, one block of `check_next_input`.** -/
theorem block_sound (xm : Bool) {xs : List Nat} {s : CS} (h : GS xs s) (blk : List Nat)
    (hne : blk ≠ []) (hb : ∀ c ∈ blk, c < 256) (he : (nextInput xm s blk).err = false) :
    GS (xs ++ blk) (nextInput xm s blk) := by
  obtain ⟨herr, hinc, hinv⟩ := h
  unfold nextInput at he ⊢
  split
  · rename_i ha
    rw [ite_eq_left ha] at he
    simp only [herr, Bool.false_or] at he
    refine ⟨by simp [herr, he], hinc, inv_ascii hinv (gs_end ⟨herr, hinc, hinv⟩ he) blk hne ha⟩
  · rename_i ha
    rw [ite_eq_right ha] at he
    simp only [herr, Bool.false_or] at he ⊢
    exact ⟨he, rfl, scan_sound xm blk xs _ _ _ hinv hb he⟩

/-- **Completeness, one block of `check_next_input`.** -/
theorem block_complete (xm : Bool) {xs : List Nat} {s : CS} (h : GS xs s) (blk : List Nat)
    (hne : blk ≠ []) (hb : ∀ c ∈ blk, c < 256) (hv : run 0 (xs ++ blk) ≠ 8) :
    (nextInput xm s blk).err = false ∧ GS (xs ++ blk) (nextInput xm s blk) := by
  obtain ⟨herr, hinc, hinv⟩ := h
  unfold nextInput
  split
  · rename_i ha
    have h0 : run 0 xs = 0 := by
      obtain ⟨b, r, rfl⟩ := List.exists_cons_of_ne_nil hne
      rw [run_append, run_cons] at hv
      simp only [allAscii, List.all_cons, Bool.and_eq_true, decide_eq_true_eq] at ha
      exact step_ascii_ne _ b ha.1 (fun h8 => hv (by rw [h8, run_8]))
    have hi := gs_inc ⟨herr, hinc, hinv⟩ h0
    exact ⟨by simp [herr, hi], by simp [herr, hi], hinc, inv_ascii hinv h0 blk hne ha⟩
  · obtain ⟨i1, i2⟩ := scan_complete xm blk xs _ _ _ hinv hb hv
    exact ⟨by simp [herr, i1], by simp [herr, i1], rfl, i2⟩

/-! ## The whole input -/

theorem bc_eq (blk : List Nat) : bc blk = conts blk := by
  unfold bc; split
  · rename_i ha
    symm; unfold conts; rw [List.countP_eq_zero]
    intro b hb
    simp only [allAscii, List.all_eq_true, decide_eq_true_eq] at ha
    have := ha b hb; simp; omega
  · rfl

theorem bf_eq (blk : List Nat) : bf blk = fours blk := by
  unfold bf; split
  · rename_i ha
    symm; unfold fours; rw [List.countP_eq_zero]
    intro b hb
    simp only [allAscii, List.all_eq_true, decide_eq_true_eq] at ha
    have := ha b hb; simp; omega
  · rfl

theorem conts_append (a b : List Nat) : conts (a ++ b) = conts a + conts b := List.countP_append
theorem fours_append (a b : List Nat) : fours (a ++ b) = fours a + fours b := List.countP_append

/-- The loop keeps the checker consistent with the bytes read, at offsets that
are multiples of 64; it stops at an error (the flagged block is at `idx`) or
when no full block is left. -/
theorem loop_spec (xm : Bool) (bs : List Nat) (hb : Bytes bs) :
    ∀ n idx s C F, bs.length - idx ≤ n → GS (bs.take idx) s → C = conts (bs.take idx) →
      F = fours (bs.take idx) → idx % 64 = 0 → (idx = 0 ∨ idx < bs.length) →
      (loop xm bs idx s C F).2.1 % 64 = 0 ∧
      ((loop xm bs idx s C F).2.1 = 0 ∨ (loop xm bs idx s C F).2.1 < bs.length) ∧
      GS (bs.take (loop xm bs idx s C F).2.1) (loop xm bs idx s C F).2.2.1 ∧
      (loop xm bs idx s C F).2.2.2.1 = conts (bs.take (loop xm bs idx s C F).2.1) ∧
      (loop xm bs idx s C F).2.2.2.2 = fours (bs.take (loop xm bs idx s C F).2.1) ∧
      ((loop xm bs idx s C F).1 = true → (loop xm bs idx s C F).2.1 + 64 < bs.length ∧
        (nextInput xm (loop xm bs idx s C F).2.2.1
          ((bs.drop (loop xm bs idx s C F).2.1).take 64)).err = true) ∧
      ((loop xm bs idx s C F).1 = false → bs.length ≤ (loop xm bs idx s C F).2.1 + 64) := by
  intro n
  induction n with
  | zero =>
    intro idx s C F hn hg hC hF hm hi
    rw [loop, dite_eq_right_iff.mpr (fun h => absurd h (by omega))]
    exact ⟨hm, hi, hg, hC, hF, ⟨fun h => by simp at h, fun _ => by simp; omega⟩⟩
  | succ n ih =>
    intro idx s C F hn hg hC hF hm hi
    rw [loop]
    by_cases h : idx + 64 < bs.length
    · rw [dite_eq_left_of_eq_true (eq_true h)]
      by_cases he : (nextInput xm s ((bs.drop idx).take 64)).err = true
      · simp only [he, ↓reduceIte]
        exact ⟨hm, hi, hg, hC, hF, ⟨fun _ => ⟨h, trivial⟩, fun h' => by simp at h'⟩⟩
      · have he' : (nextInput xm s ((bs.drop idx).take 64)).err = false := by simpa using he
        simp only [he', Bool.false_eq_true, ↓reduceIte]
        have hblk : ((bs.drop idx).take 64).length = 64 := by simp; omega
        have hne : (bs.drop idx).take 64 ≠ [] := by
          intro h0; rw [h0] at hblk; simp at hblk
        have htake : bs.take (idx + 64) = bs.take idx ++ (bs.drop idx).take 64 := List.take_add
        have hg' := block_sound xm hg _ hne
          (fun c hc => hb c (List.mem_of_mem_drop (List.mem_of_mem_take hc))) he'
        rw [← htake] at hg'
        exact ih (idx + 64) _ _ _ (by omega) hg'
          (by rw [htake, conts_append, hC, bc_eq]) (by rw [htake, fours_append, hF, bf_eq])
          (by omega) (Or.inr (by omega))
    · rw [dite_eq_right_iff.mpr (fun h' => absurd h' h)]
      exact ⟨hm, hi, hg, hC, hF, ⟨fun h => by simp at h, fun _ => by simp; omega⟩⟩

/-- The padded last block is the rest of the input followed by ASCII bytes
that are neither continuation bytes nor four-byte leads. -/
theorem rem_facts (bs : List Nat) (idx : Nat) :
    ∃ pad, remBlock bs idx = bs.drop idx ++ pad ∧ (∀ b ∈ pad, b < 0x80) ∧
      conts pad = 0 ∧ fours pad = 0 ∧ remBlock bs idx ≠ [] := by
  unfold remBlock
  split
  · rename_i h
    refine ⟨List.replicate 64 0, by rw [List.drop_eq_nil_of_le h]; rfl, ?_, ?_, ?_, by simp⟩
    · intro b hb; rw [List.eq_of_mem_replicate hb]; decide
    · unfold conts; rw [List.countP_eq_zero]; intro b hb; rw [List.eq_of_mem_replicate hb]; decide
    · unfold fours; rw [List.countP_eq_zero]; intro b hb; rw [List.eq_of_mem_replicate hb]; decide
  · rename_i h
    refine ⟨List.replicate (64 - (bs.length - idx)) 0x20, rfl, ?_, ?_, ?_, ?_⟩
    · intro b hb; rw [List.eq_of_mem_replicate hb]; decide
    · unfold conts; rw [List.countP_eq_zero]; intro b hb; rw [List.eq_of_mem_replicate hb]; decide
    · unfold fours; rw [List.countP_eq_zero]; intro b hb; rw [List.eq_of_mem_replicate hb]; decide
    · intro h0
      have := congrArg List.length h0
      simp at this; omega

theorem rem_bytes (bs : List Nat) (hb : Bytes bs) (idx : Nat) : ∀ c ∈ remBlock bs idx, c < 256 := by
  obtain ⟨pad, he, hp, -⟩ := rem_facts bs idx
  rw [he]; intro c hc
  rcases List.mem_append.mp hc with h | h
  · exact hb c (List.mem_of_mem_drop h)
  · have := hp c h; omega

/-- **Soundness of the validator.** If the checker of
`generic_validate_utf8_with_counts` flags nothing over the whole input (full
blocks, the padded last block, `check_eof`), the input is valid UTF-8. -/
theorem flag_sound (xm : Bool) (bs : List Nat) (hb : Bytes bs)
    (h : (checkAll xm bs).1 = false) : Valid8 bs := by
  have spec := loop_spec xm bs hb bs.length 0 cs0 0 0 (by omega) gs0 rfl rfl rfl (Or.inl rfl)
  unfold checkAll at h
  generalize loop xm bs 0 cs0 0 0 = r at h spec
  obtain ⟨e, idx, s, C, F⟩ := r
  obtain ⟨-, -, hg, -, -, -, -⟩ := spec
  cases e
  · simp only [Bool.false_eq_true, ↓reduceIte] at h
    split at h
    · cases h
    · rename_i hf
      simp only [Bool.or_eq_true, not_or, Bool.not_eq_true] at hf
      obtain ⟨pad, hR, hp, -, -, hne⟩ := rem_facts bs idx
      have hg2 := block_sound xm hg (remBlock bs idx) hne (rem_bytes bs hb idx) hf.1
      have h0 := gs_end hg2 hf.2
      rw [hR, ← List.append_assoc, List.take_append_drop, run_append] at h0
      exact run_ascii_zero pad _ hp h0
  · simp at h

/-- **Completeness of the validator.** Valid UTF-8 is never flagged (the
whole-input form of `ValidExact.checkedAt_of_valid`, with the carried bytes,
stale after ASCII blocks, and `prev_incomplete`). -/
theorem flag_complete (xm : Bool) (bs : List Nat) (hb : Bytes bs) (hv : Valid8 bs) :
    (checkAll xm bs).1 = false := by
  have spec := loop_spec xm bs hb bs.length 0 cs0 0 0 (by omega) gs0 rfl rfl rfl (Or.inl rfl)
  unfold checkAll
  generalize loop xm bs 0 cs0 0 0 = r at spec
  obtain ⟨e, idx, s, C, F⟩ := r
  obtain ⟨-, -, hg, -, -, herr, -⟩ := spec
  cases e
  · simp only [Bool.false_eq_true, ↓reduceIte]
    obtain ⟨pad, hR, hp, -, -, hne⟩ := rem_facts bs idx
    have hrun : run 0 (bs.take idx ++ remBlock bs idx) = 0 := by
      rw [hR, ← List.append_assoc, List.take_append_drop, run_append, hv]
      clear hR hne
      induction pad with
      | nil => rfl
      | cons b r ih =>
        rw [run_cons, step0_ascii b (hp b (by simp))]
        exact ih (fun x hx => hp x (by simp [hx]))
    obtain ⟨h1, h2⟩ := block_complete xm hg (remBlock bs idx) hne (rem_bytes bs hb idx)
      (by rw [hrun]; decide)
    have h3 := gs_inc h2 hrun
    simp [h1, h3]
  · exfalso
    obtain ⟨hlt, he⟩ := herr rfl
    simp only at hlt he
    have hblk : ((bs.drop idx).take 64).length = 64 := by simp; omega
    have hne : (bs.drop idx).take 64 ≠ [] := by
      intro h0; rw [h0] at hblk; simp at hblk
    have h8 := run_take_ne8 hv (idx + 64)
    rw [List.take_add] at h8
    have := (block_complete xm hg _ hne
      (fun c hc => hb c (List.mem_of_mem_drop (List.mem_of_mem_take hc))) h8).1
    rw [he] at this; cases this

/-- **The validator decides UTF-8**: it flags nothing exactly on valid input,
for both `must_be_2_3_continuation` (`xm = false`: westmere, haswell, ppc64;
`xm = true`: arm64, lsx, lasx). -/
theorem flag_iff (xm : Bool) (bs : List Nat) (hb : Bytes bs) :
    (checkAll xm bs).1 = false ↔ Valid8 bs :=
  ⟨flag_sound xm bs hb, flag_complete xm bs hb⟩

/-! ## The error position: `validate_utf8_with_counts` -/

/-- The scalar `validate_with_counts` (include/simdutf/scalar/utf8.h:256-379)
with all four fields: `(error == SUCCESS, pos, continuations, four_byte)`. A
character of `k` bytes adds `k - 1` continuations and, for `k = 4`, one
four-byte lead (:312, :337, :366-367). `ReplacementMore.vwcF` is the same loop
with `utf16_length()` in place of the two counters (`vwc4F_eq`). -/
def vwc4F : Nat → List Nat → Bool × Nat × Nat × Nat
  | 0, _ => (true, 0, 0, 0)
  | _ + 1, [] => (true, 0, 0, 0)
  | f + 1, b :: r =>
    match vchar (b :: r) with
    | none => (false, 0, 0, 0)
    | some (k, _) =>
      ((vwc4F f ((b :: r).drop k)).1, k + (vwc4F f ((b :: r).drop k)).2.1,
        (k - 1) + (vwc4F f ((b :: r).drop k)).2.2.1,
        (if k = 4 then 1 else 0) + (vwc4F f ((b :: r).drop k)).2.2.2)

theorem dchar_ku (ys : List Nat) (k u : Nat) (h : dchar ys = some (k, u)) :
    u + (k - 1) = k + (if k = 4 then 1 else 0) := by
  rcases ys with _ | ⟨b, r⟩
  · simp [dchar] at h
  rcases r with _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩ <;> simp only [dchar] at h <;>
    repeat' (first | (simp only [reduceCtorEq] at h) | split at h)
  all_goals
    simp only [Option.some.injEq, Prod.mk.injEq] at h
    obtain ⟨rfl, rfl⟩ := h
    decide

theorem vwc4F_eq : ∀ f (ys : List Nat), Bytes ys →
    (vwc4F f ys).1 = (vwcF f ys).1 ∧ (vwc4F f ys).2.1 = (vwcF f ys).2.1 ∧
    (vwcF f ys).2.2 + (vwc4F f ys).2.2.1 = (vwc4F f ys).2.1 + (vwc4F f ys).2.2.2 := by
  intro f
  induction f with
  | zero => intro ys _; simp [vwc4F, vwcF]
  | succ f ih =>
    intro ys hb
    rcases ys with _ | ⟨b, r⟩
    · simp [vwc4F, vwcF]
    cases hv : vchar (b :: r) with
    | none => simp only [vwc4F, vwcF, hv]; simp
    | some ku =>
      obtain ⟨k, u⟩ := ku
      have hd : dchar (b :: r) = some (k, u) := by
        rw [← ReplacementMore.vchar_eq_dchar _ hb]; exact hv
      have hku := dchar_ku _ k u hd
      obtain ⟨i1, i2, i3⟩ := ih ((b :: r).drop k) (ReplacementMore.bytes_drop hb k)
      simp only [vwc4F, vwcF, hv]
      refine ⟨i1, by rw [i2], ?_⟩
      generalize (if k = 4 then 1 else 0) = t at hku ⊢
      omega

/-- The scalar validator with counts is correct (`ReplacementMore.vwcF_spec`
through `vwc4F_eq`), with `utf16_length()` as `pos - continuations + four_byte`. -/
theorem vwc4F_spec (ys : List Nat) (hb : Bytes ys) :
    let r := vwc4F ys.length ys
    ((r.1 = true → Valid8 ys ∧ (r.2.1 : Int) - r.2.2.1 + r.2.2.2 = L (ys.map w16)) ∧
    (r.1 = false → r.2.1 < ys.length ∧ Valid8 (ys.take r.2.1) ∧
      (r.2.1 : Int) - r.2.2.1 + r.2.2.2 = L ((ys.take r.2.1).map w16) ∧
      dchar (ys.drop r.2.1) = none)) := by
  obtain ⟨e1, e2, e3⟩ := vwc4F_eq ys.length ys hb
  obtain ⟨s1, s2⟩ := vwcF_spec ys.length ys hb (Nat.le_refl _)
  refine ⟨fun h => ?_, fun h => ?_⟩
  · rw [e1] at h
    obtain ⟨a, b⟩ := s1 h
    refine ⟨a, ?_⟩
    rw [← b]; omega
  · rw [e1] at h
    obtain ⟨a, b, c, d⟩ := s2 h
    rw [e2]
    refine ⟨a, b, ?_, d⟩
    rw [← c]; omega

/-- `rewind_and_validate_with_counts` steps back over at most 4 bytes: to the
first byte before `buf` that is not a continuation byte, or 4 bytes
(include/simdutf/scalar/utf8.h:423-441; `(b & 0xC0) != 0x80` is `!isCont b`
on bytes, `ReplacementMore.masks`). -/
def ext (bs : List Nat) (idx : Nat) : Nat :=
  if isCont (bs.getD (idx - 1) 0) = false then 1
  else if isCont (bs.getD (idx - 2) 0) = false then 2
  else if isCont (bs.getD (idx - 3) 0) = false then 3 else 4

/-- `rewind_and_validate_with_counts(input, input + count, length - count)`
(include/simdutf/scalar/utf8.h:400-448) followed by the caller's additions
(utf8_validator.h:124-131, :143-151): `C` and `F` are the counters of the
blocks before `count`. A first byte that is a continuation byte gives
`TOO_LONG` at 0 (include/simdutf/scalar/utf8.h:403-405); at `count = 0` the scalar validator runs on the
whole input (:408-410); otherwise it runs from `count - ext`, and the counts
of the bytes stepped over are subtracted (:423-447). Every error of the
scalar validator is a validation error (`verr`). The `size_t` arithmetic is
done in `Int` (see the header). -/
def rewindV (bs : List Nat) (count C F : Nat) : VRes :=
  if isCont (bs.getD 0 0) then
    let ic : Int := 0 + count
    let cc : Int := 0 + C
    let fc : Int := 0 + F
    ⟨false, true, ic.toNat, (ic - cc + fc).toNat⟩
  else
    let e := if count = 0 then 0 else ext bs count
    let j := count - e
    let rb := (bs.drop j).take e
    let r := vwc4F (bs.drop j).length (bs.drop j)
    let ic : Int := (r.2.1 : Int) - e + count
    let cc : Int := (r.2.2.1 : Int) - conts rb + C
    let fc : Int := (r.2.2.2 : Int) - fours rb + F
    ⟨r.1, !r.1, ic.toNat, (ic - cc + fc).toNat⟩

/-- **`generic_validate_utf8_with_counts`** (src/generic/utf8_validation/utf8_validator.h:113-162;
westmere, haswell, ppc64 with `xm = false`, lsx and lasx with `xm = true`),
as `ReplacementMore.VRes`: `ok`, `verr`, `input_count`, `utf16_length()`. On
success `utf16_length()` is `length - continuations + four_byte` over all
blocks (:153-154, error.h:144-146). `arm_validate_utf8_with_counts`
(src/arm64/arm_validate_utf8_with_counts.cpp:54-107) runs the same
`utf8_checker` (`xm = true`) over the same blocks and adds the same counts
(its `neon_counter` sums the same two masks of the non-ASCII blocks, reduced
every 124 chunks, before an int8 lane can overflow; the failing block is not
counted, :62-73, :95-102), so it is the same function. -/
def vecV (xm : Bool) (bs : List Nat) : VRes :=
  let r := checkAll xm bs
  if r.1 then rewindV bs r.2.1 r.2.2.1 r.2.2.2
  else
    let ic : Int := bs.length
    ⟨true, false, bs.length, (ic - r.2.2.1 + r.2.2.2).toNat⟩

/-! ### Facts used by the rewind -/

theorem L_w16N : ∀ xs : List Nat, L (xs.map w16) + conts xs = xs.length + fours xs
  | [] => by simp [conts, fours]
  | b :: r => by
    have ih := L_w16N r
    unfold conts fours at *
    rw [List.map_cons, L_cons, List.countP_cons, List.countP_cons, List.length_cons]
    by_cases ha : 0x80 ≤ b <;> by_cases hb : b < 0xC0 <;> by_cases hc : 0xF0 ≤ b <;>
      simp [w16, ha, hb, hc] at ih ⊢ <;> omega

theorem L_w16 (xs : List Nat) : (L (xs.map w16) : Int) = xs.length - conts xs + fours xs := by
  have := L_w16N xs; omega

theorem take_succ_getD (bs : List Nat) (q : Nat) (hq : q < bs.length) :
    bs.take (q + 1) = bs.take q ++ [bs.getD q 0] := by
  rw [List.take_add_one, List.getElem?_eq_getElem hq, List.getD_eq_getElem?_getD,
    List.getElem?_eq_getElem hq]
  rfl

theorem prefix_ne8 (bs : List Nat) {q q' : Nat} (hq : q ≤ q') (h : run 0 (bs.take q') ≠ 8) :
    run 0 (bs.take q) ≠ 8 := by
  have e : bs.take q' = bs.take q ++ (bs.drop q).take (q' - q) := by
    rw [← List.take_add]; congr 1; omega
  rw [e] at h
  exact run_prefix_ne8 h

theorem lead_zero (bs : List Nat) (q : Nat) (hq : q < bs.length)
    (h : run 0 (bs.take (q + 1)) ≠ 8) (hc : isCont (bs.getD q 0) = false) :
    run 0 (bs.take q) = 0 := by
  rw [take_succ_getD bs q hq, run_snoc] at h
  refine Classical.byContradiction fun h0 => ?_
  exact h (step_noncont _ _ h0 hc)

/-- The last byte read, from the invariant. -/
theorem inv_last {xs ys : List Nat} {a p3 p2 p1 : Nat} (h : Inv xs p3 p2 p1)
    (hx : xs = ys ++ [a]) :
    run 0 ys ≠ 8 ∧ (isCont a = false → run 0 ys = 0) ∧ (isCont a = true → run 0 xs ≠ 8) := by
  obtain ⟨s0, hH, hrun, hc⟩ := h
  obtain ⟨hp1, hs0⟩ := H_lt hH
  obtain ⟨l1, l2⟩ := H_last hH
  rcases hc with h0 | ⟨ys', hx', hr'⟩ | ⟨ys', a', hx', ha', hr'⟩
  · rw [h0] at hx; simp at hx
  · rw [hx] at hx'
    obtain ⟨rfl, h2⟩ := List.append_inj' hx' rfl
    simp only [List.cons.injEq, and_true] at h2
    subst h2
    refine ⟨by omega, fun h => by rw [hr']; exact l2 h, fun h => by rw [hrun]; exact l1 h⟩
  · rw [hx] at hx'
    obtain ⟨rfl, h2⟩ := List.append_inj' hx' rfl
    simp only [List.cons.injEq, and_true] at h2
    subst h2
    rw [hx, run_snoc] at hr'
    have := step_ascii_ne _ _ ha' (by rw [hr']; decide)
    refine ⟨by omega, fun _ => this, fun _ => by rw [hx, run_snoc, hr']; decide⟩

/-- A rank that every continuation byte lowers (or the automaton fails). -/
def rk (s : Nat) : Nat :=
  if s = 0 then 1 else if s = 1 then 2 else if s = 2 ∨ s = 4 ∨ s = 5 then 3
  else if s = 3 ∨ s = 6 ∨ s = 7 then 4 else 0

def contRankOK : Bool :=
  (List.range 9).all fun s => (List.range 256).all fun x =>
    !isCont x || step s x == 8 || decide (rk (step s x) + 1 ≤ rk s)

theorem cont_rank : contRankOK = true := by decide +kernel

theorem step_ge9 (s x : Nat) (h : 9 ≤ s) : step s x = 8 := by
  unfold step
  repeat (first | omega | split)

theorem cont_step (s x : Nat) (hx : isCont x = true) :
    step s x = 8 ∨ rk (step s x) + 1 ≤ rk s := by
  by_cases hs : s < 9
  · have hx' : x < 256 := by simp [isCont] at hx; omega
    have k := cont_rank
    unfold contRankOK at k
    have k := List.all_eq_true.mp k s (List.mem_range.mpr hs)
    have k := List.all_eq_true.mp k x (List.mem_range.mpr hx')
    simp only [hx, Bool.not_true, Bool.false_or, Bool.or_eq_true, beq_iff_eq,
      decide_eq_true_eq] at k
    exact k
  · exact Or.inl (step_ge9 s x (by omega))

theorem rk_le (s : Nat) : rk s ≤ 4 := by
  unfold rk; repeat (first | omega | split)

theorem rk_zero (s : Nat) (h8 : s ≤ 8) (h : rk s = 0) : s = 8 := by
  unfold rk at h; repeat (first | omega | split at h)

/-- Four continuation bytes in a row never pass the automaton. -/
theorem four_conts (s a b c d : Nat) (ha : isCont a = true) (hb : isCont b = true)
    (hc : isCont c = true) (hd : isCont d = true) : run s [a, b, c, d] = 8 := by
  show step (step (step (step s a) b) c) d = 8
  rcases cont_step s a ha with h1 | h1
  · rw [h1, step_8, step_8, step_8]
  rcases cont_step _ b hb with h2 | h2
  · rw [h2, step_8, step_8]
  rcases cont_step _ c hc with h3 | h3
  · rw [h3, step_8]
  rcases cont_step _ d hd with h4 | h4
  · exact h4
  have := rk_le s
  exact rk_zero _ (step_le _ _) (by omega)

/-- **Where the rewind lands.** After the blocks before `idx` passed, the
byte `idx - ext` is a leading byte (four continuation bytes cannot precede
`idx`) and the bytes before it are complete characters. -/
theorem ext_spec (bs : List Nat) (idx : Nat) (h64 : 64 ≤ idx) (hlt : idx ≤ bs.length)
    {p3 p2 p1 : Nat} (hI : Inv (bs.take idx) p3 p2 p1) :
    1 ≤ ext bs idx ∧ ext bs idx ≤ 4 ∧ run 0 (bs.take (idx - ext bs idx)) = 0 := by
  have e1 := take_succ_getD bs (idx - 1) (by omega)
  rw [show idx - 1 + 1 = idx by omega] at e1
  obtain ⟨A, B, C⟩ := inv_last hI e1
  unfold ext
  split
  · rename_i h1; exact ⟨Nat.le_refl _, by omega, B h1⟩
  split
  · rename_i h1 h2
    refine ⟨by omega, by omega, lead_zero bs (idx - 2) (by omega) ?_ h2⟩
    rw [show idx - 2 + 1 = idx - 1 by omega]; exact A
  split
  · rename_i h1 h2 h3
    refine ⟨by omega, by omega, lead_zero bs (idx - 3) (by omega) ?_ h3⟩
    rw [show idx - 3 + 1 = idx - 2 by omega]; exact prefix_ne8 bs (by omega) A
  · rename_i h1 h2 h3
    simp only [Bool.not_eq_false] at h1 h2 h3
    refine ⟨by omega, Nat.le_refl _, lead_zero bs (idx - 4) (by omega) ?_ ?_⟩
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

/-- **The rewind meets `VSpec`** at the offset of a flagged block, given that
the blocks before it passed. -/
theorem rewind_spec (bs : List Nat) (hb : Bytes bs) (hne : bs ≠ []) (idx : Nat)
    (hm : idx % 64 = 0) (hlt : idx < bs.length) {p3 p2 p1 : Nat}
    (hI : Inv (bs.take idx) p3 p2 p1) :
    ((rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).ok = true →
      Valid8 bs ∧ (rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).u =
        L (bs.map w16)) ∧
    ((rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).ok = false →
      (rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).verr = true →
      (rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).p < bs.length →
      Valid8 (bs.take (rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).p) ∧
      (rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).u =
        L ((bs.take (rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).p).map w16) ∧
      dchar (bs.drop (rewindV bs idx (conts (bs.take idx)) (fours (bs.take idx))).p) = none) := by
  -- the index of the leading byte the scalar validator starts from
  have hj : ∃ e, (if idx = 0 then 0 else ext bs idx) = e ∧ e ≤ idx ∧ e ≤ 4 ∧
      run 0 (bs.take (idx - e)) = 0 := by
    by_cases h0 : idx = 0
    · exact ⟨0, by simp [h0], by omega, by omega, by simp [h0]; rfl⟩
    · have h64 : 64 ≤ idx := by omega
      obtain ⟨a, b, c⟩ := ext_spec bs idx h64 (by omega) hI
      exact ⟨ext bs idx, by simp [h0], by omega, b, c⟩
  obtain ⟨e, he, hei, he4, hrun⟩ := hj
  unfold rewindV
  split
  · -- `*start` is a continuation byte: only possible at `idx = 0`
    rename_i hc
    have h0 : idx = 0 := by
      refine Classical.byContradiction fun h0 => ?_
      have e1 := take_succ_getD bs (idx - 1) (by omega)
      rw [show idx - 1 + 1 = idx by omega] at e1
      obtain ⟨A, -, -⟩ := inv_last hI e1
      have := prefix_ne8 bs (q := 1) (by omega) A
      rw [show (1 : Nat) = 0 + 1 from rfl, take_succ_getD bs 0 (by omega)] at this
      simp only [List.take_zero, List.nil_append] at this
      exact this (step0_cont _ hc)
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
  · simp only [he]
    -- the scalar validator from `j = idx - e`
    have hbj := ReplacementMore.bytes_drop hb (idx - e)
    have spec := vwc4F_spec (bs.drop (idx - e)) hbj
    generalize vwc4F (bs.drop (idx - e)).length (bs.drop (idx - e)) = r at spec
    obtain ⟨ok, sp, sc', sf⟩ := r
    obtain ⟨s1, s2⟩ := spec
    simp only at s1 s2 ⊢
    -- the counts of the blocks before `idx`, split at `j`
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
      refine ⟨?_, ?_⟩
      · unfold Valid8
        rw [← List.take_append_drop (idx - e) bs, run_append, hrun]; exact v
      · have : L (bs.map w16) = L ((bs.take (idx - e)).map w16) + L ((bs.drop (idx - e)).map w16) := by
          rw [← L_append, ← List.map_append, List.take_append_drop]
        rw [hl, this]; omega
    · obtain ⟨hsp, v, hl, hd⟩ := s2 hok
      simp only [Int.toNat_natCast]
      have hsplit : bs.take (idx - e + sp) = bs.take (idx - e) ++ (bs.drop (idx - e)).take sp :=
        List.take_add
      refine ⟨?_, ?_, ?_⟩
      · unfold Valid8; rw [hsplit, run_append, hrun]; exact v
      · rw [hl, hsplit, List.map_append, L_append]; omega
      · rw [← List.drop_drop]; exact hd

/-- What `checkAll` reports: at a flagged block, the offset, the counters
before it and a consistent checker state; otherwise the counters of the whole
input. -/
theorem checkAll_facts (xm : Bool) (bs : List Nat) (hb : Bytes bs) :
    ((checkAll xm bs).1 = true → (checkAll xm bs).2.1 % 64 = 0 ∧
      ((checkAll xm bs).2.1 = 0 ∨ (checkAll xm bs).2.1 < bs.length) ∧
      (checkAll xm bs).2.2.1 = conts (bs.take (checkAll xm bs).2.1) ∧
      (checkAll xm bs).2.2.2 = fours (bs.take (checkAll xm bs).2.1) ∧
      ∃ s, GS (bs.take (checkAll xm bs).2.1) s) ∧
    ((checkAll xm bs).1 = false → (checkAll xm bs).2.2.1 = conts bs ∧
      (checkAll xm bs).2.2.2 = fours bs) := by
  have spec := loop_spec xm bs hb bs.length 0 cs0 0 0 (by omega) gs0 rfl rfl rfl (Or.inl rfl)
  unfold checkAll
  generalize loop xm bs 0 cs0 0 0 = r at spec
  obtain ⟨e, idx, s, C, F⟩ := r
  obtain ⟨hm, hi, hg, hC, hF, -, -⟩ := spec
  simp only at hm hi hg hC hF
  cases e
  · simp only [Bool.false_eq_true, ↓reduceIte]
    split
    · exact ⟨fun _ => ⟨hm, hi, hC, hF, s, hg⟩, fun h => absurd h (by simp)⟩
    · refine ⟨fun h => absurd h (by simp), fun _ => ?_⟩
      obtain ⟨pad, hR, -, hcp, hfp, -⟩ := rem_facts bs idx
      rw [bc_eq, bf_eq, hR, conts_append, fours_append, hcp, hfp, hC, hF]
      constructor <;> simp only [Nat.add_zero, ← conts_append, ← fours_append, List.take_append_drop]
  · simp only [↓reduceIte]
    exact ⟨fun _ => ⟨hm, hi, hC, hF, s, hg⟩, fun h => absurd h (by simp)⟩

/-- **The vector `validate_utf8_with_counts` meets `ReplacementMore.VSpec`**, for
both `must_be_2_3_continuation`: on success the input is valid UTF-8 and
`utf16_length()` is `L` of its `w16` weights; on an error at `p < length`,
the bytes before `p` are valid with `utf16_length()` units and the decoder's
`dchar` fails at `p`. -/
theorem vecV_spec (xm : Bool) : VSpec (vecV xm) := by
  intro ys hb hne
  obtain ⟨f1, f2⟩ := checkAll_facts xm ys hb
  unfold vecV
  by_cases hf : (checkAll xm ys).1 = true
  · simp only [hf, ↓reduceIte]
    obtain ⟨hm, hi, hC, hF, s, hg⟩ := f1 hf
    have hlt : (checkAll xm ys).2.1 < ys.length := by
      rcases hi with h | h
      · rw [h]; exact List.length_pos_iff.mpr hne
      · exact h
    rw [hC, hF]
    exact rewind_spec ys hb hne _ hm hlt hg.2.2
  · have hf' : (checkAll xm ys).1 = false := by simpa using hf
    simp only [hf', Bool.false_eq_true, ↓reduceIte]
    obtain ⟨hC, hF⟩ := f2 hf'
    refine ⟨fun _ => ⟨flag_sound xm ys hb hf', ?_⟩, fun h => by cases h⟩
    rw [hC, hF, ← L_w16, Int.toNat_natCast]

/-! ### Consequences in `ReplacementMore` -/

/-- **B1 for the vector validator**: `utf16_length_from_utf8_with_replacement`
(src/utf8_to_utf16_with_replacement.inl.h:238-276) with the vector
`validate_utf8_with_counts` computes `count` of the input. -/
theorem lenRepl_vec (xm : Bool) (ys : List Nat) (hb : Bytes ys) :
    ReplacementMore.lenRepl (vecV xm) ys = Replacement.count ys :=
  ReplacementMore.lenRepl_eq _ (vecV_spec xm) ys hb

/-- **B2 for the vector validator**: every state of `convert_using_locations`
on the recorded offsets is a `Replacement.RReach` state. -/
theorem lreach_rreach_vec (xm : Bool) (xs : List Nat) (hb : Bytes xs) (pos acc : Nat)
    (h : ReplacementMore.LReach (vecV xm) xs pos acc) : Replacement.RReach xs pos acc :=
  ReplacementMore.lreach_rreach _ (vecV_spec xm) xs hb pos acc h

/-- The last gap of `convert_using_locations` is valid UTF-8 and brings the
count to `count xs`. -/
theorem lreach_tail_vec (xm : Bool) (xs : List Nat) (hb : Bytes xs) (pos acc : Nat)
    (h : ReplacementMore.LReach (vecV xm) xs pos acc) (hne : xs.drop pos ≠ [])
    (hok : (vecV xm (xs.drop pos)).ok = true) :
    Valid8 (xs.drop pos) ∧ acc + (vecV xm (xs.drop pos)).u = Replacement.count xs :=
  ReplacementMore.lreach_tail _ (vecV_spec xm) xs hb pos acc h hne hok

/-! ## The bulk `_with_errors` kernel

`validating_transcoder::convert_with_errors`
(src/generic/utf8_to_utf16/utf8_to_utf16.h:270-377), the `_with_errors`
kernel of westmere, haswell, ppc64, arm64, lsx and lasx
(their implementation.cpp, `convert_utf8_to_utf16{le,be}_with_errors`), which
`transcode_utf8_to_utf16_with_replacement` calls on a window of at least 64
bytes (src/utf8_to_utf16_with_replacement.inl.h:110-115). Each non-ASCII
block is checked with `prev_input = 0`
(src/generic/utf8_to_utf16/utf8_to_utf16.h:307-316): that is
`Exact.CheckedAt`. -/

/-- One passing byte keeps the invariant (the step of `scan_sound`). -/
theorem inv_step (xm : Bool) {xs : List Nat} {p3 p2 p1 c : Nat} (h : Inv xs p3 p2 p1)
    (hc : c < 256) (hp : sc p1 c = if must23 xm p3 p2 then 0x80 else 0) :
    Inv (xs ++ [c]) p2 p1 c := by
  obtain ⟨s0, hH, hrun, -⟩ := h
  rw [must23_m23] at hp
  obtain ⟨-, hH'⟩ := sound_at xm hH hc hp
  exact ⟨step s0 p1, hH', by rw [run_snoc, hrun], Or.inr (Or.inl ⟨xs, rfl, hrun⟩)⟩

theorem getD_drop' (bs : List Nat) (pos k : Nat) : (bs.drop pos).getD k 0 = bs.getD (pos + k) 0 := by
  simp [List.getD_eq_getElem?_getD, List.getElem?_drop]

/-- The invariant along a checked block (`Exact.blk`: three zero bytes, then
the block). -/
theorem blk_inv (xm : Bool) (bs : List Nat) (pos : Nat) (h : Exact.CheckedAt xm bs pos) :
    ∀ k, k ≤ 64 → Inv ((bs.drop pos).take k) (Exact.blk bs pos k) (Exact.blk bs pos (k + 1))
      (Exact.blk bs pos (k + 2))
  | 0, _ => by simpa [Exact.blk] using inv_nil
  | k + 1, hk => by
    have ih := blk_inv xm bs pos h k (by omega)
    obtain ⟨hb, hlen, hc⟩ := h
    have hq := (hc.at (q := k + 3) (by omega) (by omega)).ok
    simp only [show k + 3 - 1 = k + 2 by omega, show k + 3 - 3 = k by omega,
      show k + 3 - 2 = k + 1 by omega] at hq
    have hg : Exact.blk bs pos (k + 3) = bs.getD (pos + k) 0 := by
      unfold Exact.blk; simp only [show ¬ k + 3 < 3 by omega, ↓reduceIte]
      congr 1
    have e : (bs.drop pos).take (k + 1) = (bs.drop pos).take k ++ [bs.getD (pos + k) 0] := by
      rw [take_succ_getD _ k (by simp; omega), getD_drop']
    rw [e, show k + 1 + 1 = k + 2 by omega, show k + 1 + 2 = k + 3 by omega, hg]
    rw [hg] at hq
    exact inv_step xm ih (Exact.getD_lt bs hb _) hq

/-- **A checked block is sound up to its last leading byte.** If the block at
a character boundary `pos` passes `check_utf8_bytes` (with `prev_input = 0`),
the bytes before any leading byte of the block are valid UTF-8. -/
theorem checked_lead (xm : Bool) (bs : List Nat) (pos q : Nat) (h : Exact.CheckedAt xm bs pos)
    (h0 : run 0 (bs.take pos) = 0) (hq : q < 64) (hl : isCont (bs.getD (pos + q) 0) = false) :
    run 0 (bs.take (pos + q)) = 0 := by
  have I := blk_inv xm bs pos h (q + 1) (by omega)
  have e : (bs.drop pos).take (q + 1) = (bs.drop pos).take q ++ [bs.getD (pos + q) 0] := by
    rw [take_succ_getD _ q (by have := h.2.1; simp; omega), getD_drop']
  obtain ⟨-, B, -⟩ := inv_last I e
  rw [List.take_add, run_append, h0]
  exact B hl

/-- States `(in block, pos, t, o)` of `convert_with_errors` on `bs`, as
`Exact.XReach` but on bytes and with the tests of the C++:
* `ascii`: 64-byte ASCII blocks (src/generic/utf8_to_utf16/utf8_to_utf16.h:296-299)
  and the ASCII tail (`convert_ascii_tail`, :140-171, :357-360), `a` ASCII
  bytes, `a` units;
* `enter`: a non-ASCII block is converted only if it passed the check and its
  first byte is not a continuation byte (:318, `errors() ||
  (utf8_continuation_mask & 1)`);
* `call`: one `convert_masked_utf8_to_utf16` (:346-349), exact (`XStep`, which
  `Exact.exact_utf16_145`, `exact_utf16_3`, `exact_count` and `exact_888`
  give for the calls of a checked block) and ending by `pos + 63` (`hd`, as
  in `ValidExact`). -/
inductive KR (xm : Bool) (bs : List Nat) : Bool → Nat → Nat → Nat → Prop
  | start : KR xm bs false 0 0 0
  | ascii (t o a : Nat) (h : KR xm bs false t t o) (hle : t + a ≤ bs.length)
      (hascii : ∀ i, i < a → bs.getD (t + i) 0 < 0x80) : KR xm bs false (t + a) (t + a) (o + a)
  | enter (t o : Nat) (h : KR xm bs false t t o) (hchk : Exact.CheckedAt xm bs t)
      (hlead : isCont (bs.getD t 0) = false) : KR xm bs true t t o
  | call (pos t o c p : Nat) (h : KR xm bs true pos t o) (hs : Exact.XStep (bs.map w16) t c p)
      (hd : t + c + 1 ≤ pos + 64) : KR xm bs true pos (t + c) (o + p)
  | leave (pos t o : Nat) (h : KR xm bs true pos t o) : KR xm bs false t t o

theorem take_map_w16 (bs : List Nat) (t : Nat) : (bs.map w16).take t = (bs.take t).map w16 :=
  List.map_take.symm

/-- **The kernel commits a valid prefix.** At every state the output is `L`
of the `w16` weights of the bytes consumed, and those bytes are valid UTF-8. -/
theorem kr_inv (xm : Bool) (bs : List Nat) (hb : Bytes bs) :
    ∀ b pos t o, KR xm bs b pos t o →
      o = L ((bs.map w16).take t) ∧ run 0 (bs.take t) = 0 ∧ t ≤ bs.length ∧
      (b = true → Exact.CheckedAt xm bs pos ∧ run 0 (bs.take pos) = 0 ∧ pos ≤ t ∧
        t < pos + 64) := by
  intro b pos t o h
  induction h with
  | start => exact ⟨by simp, rfl, by omega, fun h => absurd h (by simp)⟩
  | ascii t o a h hle hascii ih =>
    obtain ⟨i1, i2, i3, -⟩ := ih
    refine ⟨?_, ?_, hle, fun h => absurd h (by simp)⟩
    · rw [Utf8.L_take_add, Exact.L_ones (bs.map w16) t a (fun i hi => by
        rw [Exact.getD_map w16 bs (t + i) (by omega)]
        have := hascii i hi; unfold w16; split <;> (try split) <;> omega), ← i1]
    · rw [List.take_add, run_append, i2]
      apply run_ascii
      simp only [allAscii, List.all_eq_true, decide_eq_true_eq]
      intro x hx
      obtain ⟨i, hi, rfl⟩ := List.getElem_of_mem hx
      have hi' : i < a := by simp at hi; omega
      have := hascii i hi'
      rw [List.getElem_take, List.getElem_drop]
      rw [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem (by simp at hi; omega)] at this
      exact this
  | enter t o h hchk hlead ih =>
    obtain ⟨i1, i2, i3, -⟩ := ih
    exact ⟨i1, i2, i3, fun _ => ⟨hchk, i2, Nat.le_refl _, by omega⟩⟩
  | call pos t o c p h hs hd ih =>
    obtain ⟨i1, i2, i3, i4⟩ := ih
    obtain ⟨hchk, h0, hpt, htp⟩ := i4 rfl
    obtain ⟨hc, hnext, hp⟩ := hs
    have hin : t + c < bs.length := by
      refine Classical.byContradiction fun hge => ?_
      apply hnext
      rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by simp; omega)]
      rfl
    have hl : isCont (bs.getD (t + c) 0) = false := by
      rw [Exact.getD_map w16 bs _ hin] at hnext
      cases hcb : isCont (bs.getD (t + c) 0)
      · rfl
      · exact absurd ((Exact.w16_contZero _ (Exact.getD_lt bs hb _)).mpr hcb) hnext
    have hv := checked_lead xm bs pos (t + c - pos) hchk h0 (by omega)
      (by rw [show pos + (t + c - pos) = t + c by omega]; exact hl)
    rw [show pos + (t + c - pos) = t + c by omega] at hv
    refine ⟨by rw [Utf8.L_take_add, ← i1, hp], hv, by omega, fun _ => ⟨hchk, h0, by omega, by omega⟩⟩
  | leave pos t o h ih =>
    obtain ⟨i1, i2, i3, -⟩ := ih
    exact ⟨i1, i2, i3, fun h => absurd h (by simp)⟩

/-- The backward search of `rewind_and_convert_with_errors`
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:306-321): the least
`i` in `[i0, i0 + n]` with `buf[-i]` not a continuation byte. -/
def findBack (bs : List Nat) (t : Nat) : Nat → Nat → Option Nat
  | i, 0 => if isCont (bs.getD (t - i) 0) = false then some i else none
  | i, n + 1 => if isCont (bs.getD (t - i) 0) = false then some i else findBack bs t (i + 1) n

theorem findBack_spec (bs : List Nat) (t : Nat) : ∀ n i,
    (∀ k, findBack bs t i n = some k → i ≤ k ∧ isCont (bs.getD (t - k) 0) = false ∧
      ∀ m, i ≤ m → m < k → isCont (bs.getD (t - m) 0) = true) ∧
    (findBack bs t i n = none → ∀ m, i ≤ m → m ≤ i + n → isCont (bs.getD (t - m) 0) = true) := by
  intro n
  induction n with
  | zero =>
    intro i
    unfold findBack
    split
    · rename_i h
      refine ⟨fun k hk => ?_, fun hk => absurd hk (by simp)⟩
      cases hk; exact ⟨Nat.le_refl _, h, fun m h1 h2 => by omega⟩
    · rename_i h
      refine ⟨fun k hk => absurd hk (by simp), fun _ m h1 h2 => ?_⟩
      rw [show m = i by omega]; simpa using h
  | succ n ih =>
    intro i
    obtain ⟨j1, j2⟩ := ih (i + 1)
    unfold findBack
    split
    · rename_i h
      refine ⟨fun k hk => ?_, fun hk => absurd hk (by simp)⟩
      cases hk; exact ⟨Nat.le_refl _, h, fun m h1 h2 => by omega⟩
    · rename_i h
      have h' : isCont (bs.getD (t - i) 0) = true := by simpa using h
      refine ⟨fun k hk => ?_, fun hk m h1 h2 => ?_⟩
      · obtain ⟨a, b, c⟩ := j1 k hk
        refine ⟨by omega, b, fun m h1 h2 => ?_⟩
        by_cases hm : m = i
        · rw [hm]; exact h'
        · exact c m (by omega) h2
      · by_cases hm : m = i
        · rw [hm]; exact h'
        · exact j2 hk m (by omega) (by omega)

/-- `rewind_and_convert_with_errors(pos, in + pos, size - pos, out)` with the
caller's `res.count += pos` on an error
(src/generic/utf8_to_utf16/utf8_to_utf16.h:326, :371): `(success, units
written)` or `(false, absolute error offset)`. In
include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:296-342, no leading
byte back to the start gives `TOO_LONG` at `0 - pos` (:330-336); an ASCII
leading byte found after going back gives `TOO_LONG` at `1 - i` (:312-316);
otherwise the scalar `convert_with_errors` (:140-277, the checks of `vchar` with one unit per
character of at most 3 bytes and two for 4 bytes, so `vwcF`) runs from the
leading byte. -/
def rewindK (bs : List Nat) (t : Nat) : Bool × Nat :=
  match findBack bs t 0 t with
  | none => (false, 0)
  | some i =>
    if 0 < i ∧ bs.getD (t - i) 0 < 0x80 then (false, t - i + 1)
    else
      if (vwcF (bs.drop (t - i)).length (bs.drop (t - i))).1 then
        (true, (vwcF (bs.drop (t - i)).length (bs.drop (t - i))).2.2)
      else (false, t - i + (vwcF (bs.drop (t - i)).length (bs.drop (t - i))).2.1)

theorem cont_dchar (r : List Nat) (b : Nat) (h : isCont b = true) : dchar (b :: r) = none := by
  simp only [isCont, Bool.and_eq_true, decide_eq_true_eq] at h
  simp [dchar, show ¬ b < 0x80 by omega, show b < 0xC2 by omega]

/-- **The rewind at a character boundary.** On success the rest is valid and
the units are its `L`; on an error at `e`, `e` is inside the input, the
bytes before it are valid, and `dchar` fails at `e`. -/
theorem rewindK_spec (bs : List Nat) (hb : Bytes bs) (t : Nat) (ht : t < bs.length)
    (h0 : run 0 (bs.take t) = 0) :
    ((rewindK bs t).1 = true → Valid8 bs ∧ isCont (bs.getD t 0) = false ∧
      (rewindK bs t).2 = L ((bs.drop t).map w16)) ∧
    ((rewindK bs t).1 = false → (rewindK bs t).2 < bs.length ∧
      Valid8 (bs.take (rewindK bs t).2) ∧ dchar (bs.drop (rewindK bs t).2) = none) := by
  obtain ⟨f1, f2⟩ := findBack_spec bs t t 0
  have hgt : ∀ x, bs.drop x = bs.getD x 0 :: bs.drop (x + 1) ∨ bs.length ≤ x := by
    intro x
    by_cases hx : x < bs.length
    · left; rw [List.drop_eq_getElem_cons hx, List.getD_eq_getElem?_getD,
        List.getElem?_eq_getElem hx]; rfl
    · right; omega
  -- the scalar run from a boundary `j`
  have scal : ∀ j, j ≤ t → run 0 (bs.take j) = 0 →
      ((vwcF (bs.drop j).length (bs.drop j)).1 = true → Valid8 bs ∧
        (vwcF (bs.drop j).length (bs.drop j)).2.2 = L ((bs.drop j).map w16)) ∧
      ((vwcF (bs.drop j).length (bs.drop j)).1 = false →
        j + (vwcF (bs.drop j).length (bs.drop j)).2.1 < bs.length ∧
        Valid8 (bs.take (j + (vwcF (bs.drop j).length (bs.drop j)).2.1)) ∧
        dchar (bs.drop (j + (vwcF (bs.drop j).length (bs.drop j)).2.1)) = none) := by
    intro j hj hr
    obtain ⟨s1, s2⟩ := vwcF_spec (bs.drop j).length (bs.drop j)
      (ReplacementMore.bytes_drop hb j) (Nat.le_refl _)
    refine ⟨fun h => ?_, fun h => ?_⟩
    · obtain ⟨v, hl⟩ := s1 h
      refine ⟨?_, hl⟩
      unfold Valid8; rw [← List.take_append_drop j bs, run_append, hr]; exact v
    · obtain ⟨a, v, -, d⟩ := s2 h
      have hl : (bs.drop j).length = bs.length - j := List.length_drop
      refine ⟨by omega, ?_, by rw [← List.drop_drop]; exact d⟩
      unfold Valid8; rw [List.take_add, run_append, hr]; exact v
  unfold rewindK
  split
  · -- no leading byte: the first byte is a continuation byte, so `t = 0`
    rename_i hn
    have h0' := f2 hn 0 (Nat.le_refl _) (by omega)
    have ht0 : t = 0 := by
      refine Classical.byContradiction fun hne => ?_
      have := f2 hn t (by omega) (by omega)
      simp only [Nat.sub_self] at this
      have hv : run 0 (bs.take 1) ≠ 8 := by
        intro h8
        have := run_take_ne8 h0 1
        rw [List.take_take, show min 1 t = 1 by omega] at this
        exact this h8
      rw [show (1 : Nat) = 0 + 1 from rfl, take_succ_getD bs 0 (by omega)] at hv
      exact hv (step0_cont _ this)
    subst ht0
    simp only [Nat.sub_zero] at h0'
    refine ⟨fun h => absurd h (by simp), fun _ => ⟨by omega, rfl, ?_⟩⟩
    rcases hgt 0 with e | e
    · rw [e]; exact cont_dchar _ _ h0'
    · omega
  · rename_i i hi
    obtain ⟨-, hl, hc⟩ := f1 i hi
    by_cases ht' : isCont (bs.getD t 0) = false
    · -- a leading byte at `t`: the scalar runs from `t`
      have hi0 : i = 0 := by
        refine Classical.byContradiction fun hne => ?_
        have := hc 0 (Nat.le_refl _) (by omega)
        rw [Nat.sub_zero, ht'] at this; cases this
      subst hi0
      simp only [Nat.lt_irrefl, false_and, ↓reduceIte, Nat.sub_zero]
      obtain ⟨s1, s2⟩ := scal t (Nat.le_refl _) h0
      split
      · rename_i hok
        exact ⟨fun _ => ⟨(s1 hok).1, ht', (s1 hok).2⟩, fun h => absurd h (by simp)⟩
      · rename_i hok
        simp only [Bool.not_eq_true] at hok
        exact ⟨fun h => absurd h (by simp), fun _ => s2 hok⟩
    · -- a continuation byte at `t`, after the valid prefix
      have htc : isCont (bs.getD t 0) = true := by simpa using ht'
      have hipos : 0 < i := by
        refine Classical.byContradiction fun h0i => ?_
        rw [show i = 0 by omega, Nat.sub_zero, htc] at hl; cases hl
      have hit : i ≤ t := by
        refine Classical.byContradiction fun hgt' => ?_
        have := hc t (by omega) (by omega)
        rw [Nat.sub_self] at this
        rw [show t - i = 0 by omega, this] at hl; cases hl
      -- `t - i` is a leading byte inside the valid prefix
      have hvj : run 0 (bs.take (t - i)) = 0 := by
        have hne8 := run_take_ne8 h0 (t - i + 1)
        rw [List.take_take, show min (t - i + 1) t = t - i + 1 by omega] at hne8
        exact lead_zero bs (t - i) (by omega) hne8 hl
      split
      · -- an ASCII byte: `TOO_LONG` at the continuation byte after it
        rename_i hasc
        refine ⟨fun h => absurd h (by simp), fun _ => ⟨by omega, ?_, ?_⟩⟩
        · have := run_take_ne8 h0 (t - i + 1)
          rw [List.take_take, show min (t - i + 1) t = t - i + 1 by omega] at this
          unfold Valid8
          rw [take_succ_getD bs (t - i) (by omega), run_snoc, hvj, step0_ascii _ hasc.2]
        · have hc1 := hc (i - 1) (by omega) (by omega)
          rw [show t - (i - 1) = t - i + 1 by omega] at hc1
          rcases hgt (t - i + 1) with e | e
          · rw [e]; exact cont_dchar _ _ hc1
          · omega
      · obtain ⟨s1, s2⟩ := scal (t - i) (by omega) hvj
        split
        · -- success is impossible: `t` is a boundary of valid input
          rename_i _ hok
          exfalso
          have hv := (s1 hok).1
          have := bd_lead hv h0 ht
          rw [htc] at this; cases this
        · rename_i _ hok
          simp only [Bool.not_eq_true] at hok
          exact ⟨fun h => absurd h (by simp), fun _ => s2 hok⟩

/-- How `convert_with_errors` returns
(src/generic/utf8_to_utf16/utf8_to_utf16.h:270-377):
* `done`: the blocks and the ASCII tail consumed everything (:361-363);
* `blockErr`: a non-ASCII block failed the check or starts with a
  continuation byte (:318-327): the rewind, then `res.count += pos` whatever
  the outcome;
* `tail`: the rest after the ASCII tail goes to the rewind (:364-376): on an
  error the offset, on success the units so far plus the rewind's. -/
inductive KOut (xm : Bool) (bs : List Nat) : Bool → Nat → Prop
  | done (t o : Nat) (h : KR xm bs false t t o) (ht : t = bs.length) : KOut xm bs true o
  | blockErr (t o : Nat) (h : KR xm bs false t t o) (hlen : t + 64 ≤ bs.length)
      (hbad : ¬ Exact.CheckedAt xm bs t ∨ isCont (bs.getD t 0) = true) :
      KOut xm bs (rewindK bs t).1
        (if (rewindK bs t).1 then (rewindK bs t).2 + t else (rewindK bs t).2)
  | tail (t o : Nat) (h : KR xm bs false t t o) (ht : t < bs.length) :
      KOut xm bs (rewindK bs t).1
        (if (rewindK bs t).1 then o + (rewindK bs t).2 else (rewindK bs t).2)

/-- **The bulk `_with_errors` kernel on its window**: on success the window is
valid UTF-8 and the count is `L` of its `w16` weights; on an error the count
is an offset inside the window, the bytes before it are valid, and `dchar`
fails there. This is what `ReplacementMore` read from the C++ for the bulk
path. (After a failed block the rewind never succeeds, by
`ValidExact.checkedAt_of_valid`.) -/
theorem kout_spec (xm : Bool) (bs : List Nat) (hb : Bytes bs) (ok : Bool) (n : Nat)
    (h : KOut xm bs ok n) :
    (ok = true → Valid8 bs ∧ n = L (bs.map w16)) ∧
    (ok = false → n < bs.length ∧ Valid8 (bs.take n) ∧ dchar (bs.drop n) = none) := by
  cases h with
  | done t o h ht =>
    obtain ⟨i1, i2, -, -⟩ := kr_inv xm bs hb _ _ _ _ h
    subst ht
    rw [List.take_of_length_le (by simp)] at i1 i2
    exact ⟨fun _ => ⟨i2, i1⟩, fun h => absurd h (by simp)⟩
  | blockErr t o h hlen hbad =>
    obtain ⟨-, i2, -, -⟩ := kr_inv xm bs hb _ _ _ _ h
    obtain ⟨s1, s2⟩ := rewindK_spec bs hb t (by omega) i2
    refine ⟨fun hok => ?_, fun hok => ?_⟩
    · exfalso
      obtain ⟨hv, hl, -⟩ := s1 hok
      rcases hbad with hbad | hbad
      · exact hbad (checkedAt_of_valid xm hv i2 hlen)
      · rw [hl] at hbad; cases hbad
    · rw [hok]; simp only [Bool.false_eq_true, ↓reduceIte]; exact s2 hok
  | tail t o h ht =>
    obtain ⟨i1, i2, -, -⟩ := kr_inv xm bs hb _ _ _ _ h
    obtain ⟨s1, s2⟩ := rewindK_spec bs hb t ht i2
    refine ⟨fun hok => ?_, fun hok => ?_⟩
    · obtain ⟨hv, -, hl⟩ := s1 hok
      rw [hok]; simp only [↓reduceIte]
      refine ⟨hv, ?_⟩
      rw [hl, i1, take_map_w16, ← L_append, ← List.map_append, List.take_append_drop]
    · rw [hok]; simp only [Bool.false_eq_true, ↓reduceIte]; exact s2 hok

/-! ## `transcode_utf8_to_utf16_with_replacement`, end to end

The driver (src/utf8_to_utf16_with_replacement.inl.h:88-187) with the bulk
kernel (`KOut`) and the vector validator (`vecV`) in place of the hypotheses
of `Replacement.RReach`. The bulk kernel runs on a window that ends just
before a byte that is not a continuation byte (`utf8_boundary_before`,
:55-65); so an error the kernel reports inside the window is an error of
the whole input (`dchar_lift`): a character cannot straddle the window's end. -/

/-- `dchar` reads at most four bytes. -/
theorem dchar_take4 (ys : List Nat) : dchar ys = dchar (ys.take 4) := by
  rcases ys with _ | ⟨b, _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩⟩ <;> rfl

/-- A character `dchar` accepts is still accepted when the input is cut after
it. -/
theorem dchar_take (ys : List Nat) (n k u : Nat) (h : dchar ys = some (k, u)) (hk : k ≤ n) :
    dchar (ys.take n) = some (k, u) := by
  have hk1 := (ReplacementMore.dchar_run _ _ _ h).2.1
  by_cases h4 : 4 ≤ n
  · rw [dchar_take4, List.take_take, Nat.min_eq_left h4, ← dchar_take4]; exact h
  rcases ys with _ | ⟨b, r⟩
  · simp [dchar] at h
  rcases (by omega : n = 1 ∨ n = 2 ∨ n = 3) with rfl | rfl | rfl <;>
  rcases r with _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩ <;> simp only [List.take] at * <;>
    simp only [dchar] at h ⊢ <;> repeat' (first | (simp only [reduceCtorEq] at h) | split at h)
  all_goals (simp only [Option.some.injEq, Prod.mk.injEq] at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals (first | omega | ((repeat' split) <;> first | rfl | omega | simp_all))

/-- The bytes after the first of a character `dchar` accepts are
continuation bytes. -/
theorem dchar_cont (ys : List Nat) (k u : Nat) (h : dchar ys = some (k, u)) :
    ∀ m, 1 ≤ m → m < k → 0x80 ≤ ys.getD m 0 ∧ ys.getD m 0 < 0xC0 := by
  rcases ys with _ | ⟨b, r⟩
  · simp [dchar] at h
  rcases r with _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩ <;> simp only [dchar] at h <;>
    repeat' (first | (simp only [reduceCtorEq] at h) | split at h)
  all_goals (simp only [Option.some.injEq, Prod.mk.injEq] at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals intro m h1 h2
  all_goals first
    | omega
    | (rcases (by omega : m = 1 ∨ m = 2 ∨ m = 3) with rfl | rfl | rfl <;>
        simp only [List.getD_cons_succ, List.getD_cons_zero] <;>
        (repeat' split at *) <;> omega)

/-- **An error inside the window is an error of the input.** If the byte
after the window is not a continuation byte, `dchar` fails at `n` on the
input as it does on the window. -/
theorem dchar_lift (R : List Nat) (w n : Nat) (hn : n < w)
    (hnext : isCont (R.getD w 0) = false) (h : dchar ((R.take w).drop n) = none) :
    dchar (R.drop n) = none := by
  cases hd : dchar (R.drop n) with
  | none => rfl
  | some ku =>
    exfalso
    obtain ⟨k, u⟩ := ku
    by_cases hk : n + k ≤ w
    · rw [List.drop_take, dchar_take _ (w - n) k u hd (by omega)] at h
      cases h
    · have hk1 := (ReplacementMore.dchar_run _ _ _ hd).2.1
      have := dchar_cont _ k u hd (w - n) (by omega) (by omega)
      rw [getD_drop', show n + (w - n) = w by omega] at this
      simp only [isCont, Bool.and_eq_false_iff, decide_eq_false_iff_not] at hnext
      omega

/-- `utf8_boundary_before` stops at 0 or before a byte that is not a
continuation byte. -/
theorem boundary_spec (bs : List Nat) : ∀ l, Replacement.boundaryBefore bs l = 0 ∨
    isCont (bs.getD (Replacement.boundaryBefore bs l) 0) = false := by
  intro l
  induction l with
  | zero => left; rfl
  | succ i ih =>
    unfold Replacement.boundaryBefore
    split
    · exact ih
    · rename_i h
      right
      simp only [Replacement.isCont, decide_eq_true_eq] at h
      simp only [isCont, Bool.and_eq_false_iff, decide_eq_false_iff_not]
      omega

/-- The window of the bulk call (:109-112). -/
def win (xs : List Nat) (pos : Nat) : Nat :=
  Replacement.boundaryBefore (xs.drop pos) (xs.length - pos - 32)

/-- States `(pos, written)` of `transcode_utf8_to_utf16_with_replacement<true>`
(:88-187), with the kernels as they run:
* `bulkOk`: the bulk kernel converts its window (:113-121);
* `bulkErr`: it reports a validation error at `n < window` (:122-143); the
  valid prefix counts `utf16_length_from_utf8` units (:125, `L` of its `w16`
  weights), then one U+FFFD and `maximal_subpart` (`msub`);
* `valErr`: the vector `validate_utf8_with_counts` reports a validation error
  at `p` inside the rest (:150-185); `convert_valid_prefix` writes its
  `utf16_length()` units (exact on valid input, `ValidExact`).
The exits are `finish_with_scalar` (`count` of the rest, :67-86), the
validator's success (:166-169) and `pos ≥ length`. `short_prefixes` only
chooses between paths and is left free. -/
inductive DR (xm : Bool) (xs : List Nat) : Nat → Nat → Prop
  | start : DR xm xs 0 0
  | bulkOk (pos written n : Nat) (h : DR xm xs pos written) (hr : 32 + 64 < xs.length - pos)
      (hw : 64 ≤ win xs pos) (hk : KOut xm ((xs.drop pos).take (win xs pos)) true n) :
      DR xm xs (pos + win xs pos) (written + n)
  | bulkErr (pos written n : Nat) (h : DR xm xs pos written) (hr : 32 + 64 < xs.length - pos)
      (hw : 64 ≤ win xs pos) (hk : KOut xm ((xs.drop pos).take (win xs pos)) false n)
      (hn : n < win xs pos) :
      DR xm xs (pos + n + ReplacementMore.msub (xs.drop (pos + n)))
        (written + L (((xs.drop pos).take n).map w16) + 1)
  | valErr (pos written : Nat) (h : DR xm xs pos written) (hne : xs.drop pos ≠ [])
      (hok : (vecV xm (xs.drop pos)).ok = false) (hv : (vecV xm (xs.drop pos)).verr = true)
      (hp : (vecV xm (xs.drop pos)).p < (xs.drop pos).length) :
      DR xm xs (pos + (vecV xm (xs.drop pos)).p +
          ReplacementMore.msub (xs.drop (pos + (vecV xm (xs.drop pos)).p)))
        (written + (vecV xm (xs.drop pos)).u + 1)

theorem win_le (xs : List Nat) (pos : Nat) (hr : 32 + 64 < xs.length - pos) :
    pos + win xs pos + 32 ≤ xs.length := Replacement.held_back xs pos hr

/-- **Every state of the driver is a `Replacement.RReach` state**, with no
hypothesis on the validator or the bulk kernel. -/
theorem dr_rreach (xm : Bool) (xs : List Nat) (hb : Bytes xs) :
    ∀ pos written, DR xm xs pos written → Replacement.RReach xs pos written := by
  intro pos written h
  induction h with
  | start => exact Replacement.RReach.start
  | bulkOk pos written n h hr _hw hk ih =>
    have hwl := win_le xs pos hr
    have hbw : Bytes ((xs.drop pos).take (win xs pos)) :=
      ReplacementMore.bytes_take (ReplacementMore.bytes_drop hb pos) _
    have hn' := (kout_spec xm _ hbw true n hk).1 rfl
    rw [hn'.2]
    exact ReplacementMore.rreach_valid8 ih hn'.1
  | bulkErr pos written n h hr hw hk hn ih =>
    have hwl := win_le xs pos hr
    have hbw : Bytes ((xs.drop pos).take (win xs pos)) :=
      ReplacementMore.bytes_take (ReplacementMore.bytes_drop hb pos) _
    obtain ⟨hlt, hv, hd⟩ := (kout_spec xm _ hbw false n hk).2 rfl
    rw [List.take_take, Nat.min_eq_left (by omega)] at hv
    have hnext : isCont ((xs.drop pos).getD (win xs pos) 0) = false := by
      rcases boundary_spec (xs.drop pos) (xs.length - pos - 32) with h0 | h0
      · unfold win at hw; omega
      · exact h0
    have hd' := dchar_lift (xs.drop pos) (win xs pos) n hn hnext hd
    rw [List.drop_drop] at hd'
    exact ReplacementMore.rreach_error8 hb ih hv (by omega) hd'
  | valErr pos written h hne hok hv hp ih =>
    obtain ⟨g1, g2, g3⟩ := ReplacementMore.lreach_gap (vecV xm) (vecV_spec xm) xs hb pos hne hok hv hp
    rw [g2]
    exact ReplacementMore.rreach_error8 hb ih g1 (by simp at hp ⊢; omega) g3

/-- **`transcode_utf8_to_utf16_with_replacement`, end to end.** At every state,
the units written plus `count` of the rest is the buffer size that
`utf16_length_from_utf8_with_replacement` computes with the vector validator
(`lenRepl_vec`); so `finish_with_scalar` (:67-86) and the validator's
success exit (:166-169) both return exactly that size, and every store of the
bulk kernel, at any state of `KR` on its window and ending at most 4 units
past its committed output (`Replacement.gap_all`), ends 7 units inside the
buffer (`Replacement.bulk_store`). No hypothesis on the validator or on the
kernel's error reporting is left. -/
theorem transcode_exact (xm : Bool) (xs : List Nat) (hb : Bytes xs) (pos written : Nat)
    (h : DR xm xs pos written) :
    written + Replacement.count (xs.drop pos) = ReplacementMore.lenRepl (vecV xm) xs ∧
    (xs.drop pos ≠ [] → (vecV xm (xs.drop pos)).ok = true →
      written + (vecV xm (xs.drop pos)).u = ReplacementMore.lenRepl (vecV xm) xs) ∧
    (32 + 64 < xs.length - pos → ∀ (b : Bool) (p t o se : Nat),
      KR xm ((xs.drop pos).take (win xs pos)) b p t o → se ≤ o + 4 →
      written + se + 7 ≤ ReplacementMore.lenRepl (vecV xm) xs) := by
  have hr := dr_rreach xm xs hb pos written h
  have hi := Replacement.rreach_inv xs pos written hr
  rw [lenRepl_vec xm xs hb]
  refine ⟨hi, fun hne hok => ?_, fun hr' b p t o se hk hse => ?_⟩
  · obtain ⟨hv, hu⟩ := ((vecV_spec xm) _ (ReplacementMore.bytes_drop hb pos) hne).1 hok
    have hc := Replacement.count_valid_append (xs.drop pos) [] _
      (ReplacementMore.validRun_of_valid8 _ hv)
    rw [List.append_nil, ReplacementMore.count_nil] at hc
    rw [hu]; omega
  · have hwl := win_le xs pos hr'
    have hbw : Bytes ((xs.drop pos).take (win xs pos)) :=
      ReplacementMore.bytes_take (ReplacementMore.bytes_drop hb pos) _
    obtain ⟨i1, i2, i3, -⟩ := kr_inv xm _ hbw _ _ _ _ hk
    have hlen : ((xs.drop pos).take (win xs pos)).length = win xs pos := by simp; omega
    rw [hlen] at i3
    rw [List.take_take, Nat.min_eq_left i3] at i2
    have hv := ReplacementMore.validRun_of_valid8 _ (show Valid8 ((xs.drop pos).take t) from i2)
    rw [take_map_w16, List.take_take, Nat.min_eq_left i3] at i1
    rw [← i1] at hv
    exact Replacement.bulk_store xs pos written (win xs pos) t o se hr hwl i3 hv hse

/-! ## Examples -/

/-- `F5` as the last byte of a full block is caught at the next block's first
byte (one byte behind); the vector validator reports it at 63, where the
scalar one does. -/
example : (checkAll false (List.replicate 63 0x41 ++ [0xF5] ++ List.replicate 70 0x41)).1 = true ∧
    (vecV false (List.replicate 63 0x41 ++ [0xF5] ++ List.replicate 70 0x41)).p = 63 ∧
    (ReplacementMore.vwc (List.replicate 63 0x41 ++ [0xF5] ++ List.replicate 70 0x41)).2.1 = 63 := by
  decide +kernel

/-- A character across the block boundary passes, with the carried bytes. -/
example : (checkAll true (List.replicate 63 0x41 ++ [0xE2, 0x82, 0xAC] ++ List.replicate 64 0x41)).1 =
    false := by decide +kernel

/-- After an ASCII block the carried bytes are stale; a continuation byte that
starts the next block is still caught (at 128). -/
example : (vecV false (List.replicate 63 0x41 ++ [0xC3, 0xA9] ++ List.replicate 63 0x41 ++
    [0xA9] ++ List.replicate 70 0x41)).p = 128 := by decide +kernel

/-- A truncated character at the end (no padding byte after a 64-byte last
block) is caught by `check_eof`. -/
example : (checkAll false (List.replicate 63 0x41 ++ [0xE2])).1 = true := by decide +kernel

end SafetyMargins.ValidatorSound
