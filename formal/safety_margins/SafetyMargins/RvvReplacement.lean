import SafetyMargins.ReplacementLengths
import SafetyMargins.RvvExact
/-
rvv: the contracts of the UTF-8 → UTF-16 conversion with replacement,
discharged.

`ReplacementRest` proves `convert_utf8_to_utf16{le,be}_with_replacement` and
`transcode_utf8_to_utf16_with_replacement` (src/utf8_to_utf16_with_replacement.inl.h)
safe under `BSpec`, `VSpec2`, `CVSpec` and `ULSpec` on the kernels the driver
calls. On rvv these are (src/rvv/implementation.cpp:152-155,
src/rvv/rvv_utf8_to.inl.cpp:380-412):

* `validate_utf8_with_counts` is the scalar `validate_with_counts`
  (`rvvV = scalarV`): **`rvvV_spec2`** is `ReplacementRest.scalarV_spec2`.
* `convert_utf8_to_utf16{le,be}_with_errors` runs the validating
  `rvv_utf8_to_common` (src/rvv/rvv_utf8_to.inl.cpp:2-268) and, when it
  returns 0, the scalar `convert_with_errors` (`rvvK`): **`rvvK_bspec`**.
* `convert_valid_utf8_to_utf16{le,be}` is `rvv_utf8_to_common` without the
  checks (`rvvCV`): **`rvvCV_cvspec`**.
* End to end, with the rvv `utf16_length_from_utf8` (`LengthsSpec.ULSpec_rvv`):
  **`convertRepl_rvv`** (returns `utf16_length_from_utf8_with_replacement`,
  every store inside it) and **`trRun_rvv`**. No hypothesis is left.

Every theorem holds for every `vl` each `vsetvl` returns (`RvvExact.vlOf
pick`, any `pick`), every `M1 = vsetvlmax_e8m1` and every input.

The model (`rvvConv`) is new: `RvvExact.utf8Step` models the loop for its
stores only, with an arbitrary check. Here the check is the C++ one and the
output is the list of the 16-bit values stored, because the reparse step reads
one of them back (`dst[-1]`, :257-262). Reused from `RvvExact`: `win` (the
lanes of `v0..v3`), `c1234` and `helperLane` (the 4-byte path and
`rvv_utf32_store_utf16_m4`), `vlOf`, `back`, `skipCont`, `lastNC`, the lane
count lemmas.

* **The check (§1).** `laneErr` transcribes :81-97 with the rvv tables
  `err1m..err3m`; `laneErr_eq` proves it is `Validator.err false` (the
  per-byte check of src/generic, `|` form of `must_be_2_3_continuation`) for
  every byte pair (`decide +kernel` over 65536 pairs: the tables equal
  `Tables.byte1High..byte2High`, and `TWO_CONTS` never comes with another
  bit). Lane `i` of a block at `s` checks the byte at `s + i + 3`; the loop
  covers bytes `3 ..< len`, the prologue (:20-27) validates the bytes up to
  `idx ≥ 3` with the scalar validator.
* **Soundness (§2).** `R_upto`/`lead_valid`: from the valid prologue
  (completeness of the check, `ValidExact.closure_at`) and the passing
  positions (soundness, `ValidatorSound.sound_at`), every non-continuation
  byte in the checked range is a character boundary. With the scalar tail's
  validation this gives `conv_sound`: a nonzero result means valid input.
* **Values (§3).** On a valid character each path's lane writes `w16` units,
  the last never a high surrogate, a pair starting with one (`UOK`;
  `u12_ok`, `u123_ok`, `hunits_ok`).
* **The scalar code (§4).** `sconv` is the loop of the scalar `convert` and
  `convert_with_errors` over the existing step transcription `Scalar.u8to16`;
  `sconv_eq` proves it equal to `ReplacementMore.vwc` on bytes, so
  `vwcF_spec` describes it.
* **Exactness (§5).** `CInv`: the output is `L` of the bytes consumed and its
  last two units tell the reparse step what the last character wrote;
  `reparse_valid` and `conv_valid`: on valid input the result is
  `L (xs.map w16)` and every store is inside it.
* **Stores on any input (§6, `conv_store`).** The loop's output never exceeds
  `L` of the bytes consumed (`block_le`); the last character start before
  them is a boundary of a valid prefix at most 2 units below; the scalar
  tail adds the units of a valid continuation.

Read from the C++ and the RVV specification, not checked by Lean:
* the transcription (file:line in each docstring);
* RVV semantics, as in `RvvExact`: `1 ≤ vl ≤ AVL`, and `vl = AVL` when
  `AVL ≤ VLMAX` (the `M1` split, :231-246); `vcompress` packs the selected
  lanes in order, `vcpop` counts them, a store of `n` elements writes those
  `n`; `vrgather` reads lane `idx` of the table register (the V extension
  implies VLEN ≥ 128, so the 16 table bytes loaded by `vle64(·, 2)` are lanes
  0-15 of an m1 register, and every index is a nibble); RISC-V is
  little-endian; the byte flip of the big-endian variants is applied at
  every store and undone at the load of `dst[-1]`, so the model keeps the
  native values;
* the scalar code: the step transcription `Scalar.u8to16` (as in
  `Scalar`); `scalar::utf8::validate` (include/simdutf/scalar/utf8.h:13-111),
  used by the prologue, makes the tests of that step without storing, so its
  result is `(sconv ·).1`.

`decide +kernel` is used for finite exhaustive checks only: `tbl_ok` (16
indices), `sc_shape` (65536 byte pairs), `lane_logic` (258 cases),
`err_ascii` (16384 pairs). The examples at the end run the model (`decide`).
-/
namespace SafetyMargins.RvvReplacement
open SafetyMargins SafetyMargins.ReplacementMore
open SafetyMargins.ValidExact (Valid8 run step Bd)
open SafetyMargins.Validator (isCont)
open SafetyMargins.Utf8 (w16)
open SafetyMargins.RvvExact (vlOf win hunits hcnt back skipCont lastNC)
open SafetyMargins.IcelakeReal (hiS)
set_option autoImplicit false
set_option linter.unusedSimpArgs false

/-! # 1. The lane check of `rvv_utf8_to_common` -/

/-- Byte `i` of the 16-byte table `{lo, hi}` loaded by `__riscv_vle64_v_u64m1(·, 2)`
and reinterpreted as bytes (RISC-V is little-endian). -/
def tbl (lo hi i : Nat) : Nat := if i < 8 then lo / 256 ^ i % 256 else hi / 256 ^ (i - 8) % 256

/-- `err1m`, `err2m`, `err3m` (src/rvv/rvv_utf8_to.inl.cpp:33-35). -/
def err1 (i : Nat) : Nat := tbl 0x0202020202020202 0x4915012180808080 i
def err2 (i : Nat) : Nat := tbl 0xCBCBCB8B8383A3E7 0xCBCBDBCBCBCBCBCB i
def err3 (i : Nat) : Nat := tbl 0x0101010101010101 0x01010101BABAAEE6 i

/-- `errs` of one lane (:81-89): `vrgather` of the three tables at the high
nibble of `v2`, the low nibble of `v2` and the high nibble of `v3` (every index
is below 16, inside the table register), and-ed. `v2` holds `p1`, `v3` holds
`c`. -/
def rvvSc (p1 c : Nat) : Nat := err1 (p1 >>> 4) &&& err2 (p1 &&& 0xF) &&& err3 (c >>> 4)

/-- `errm` of one lane (:91-97), lane `i` of a block reading `v0 = p3`,
`v1 = p2`, `v2 = p1`, `v3 = c`: `errs > 0` as `int8`, or
`is_3 | is_4` (`v1 > 0b11100000 - 1`, `v0 > 0b11110000 - 1`) differing from
`errs < 0`. -/
def laneErr (p3 p2 p1 c : Nat) : Bool :=
  let e := rvvSc p1 c
  decide (0 < e ∧ e < 128) || ((decide (0xE0 - 1 < p2) || decide (0xF0 - 1 < p3)) != decide (128 ≤ e))

theorem tbl_ok : ∀ i, i < 16 →
    err1 i = Validator.byte1High[i]! ∧ err2 i = Validator.byte1Low[i]! ∧
      err3 i = Validator.byte2High[i]! := by decide +kernel

theorem rvvSc_eq (p1 c : Nat) (h1 : p1 < 256) (h2 : c < 256) : rvvSc p1 c = Validator.sc p1 c := by
  unfold rvvSc Validator.sc
  have a : p1 >>> 4 = p1 / 16 := by rw [Nat.shiftRight_eq_div_pow]
  have b : p1 &&& 0xF = p1 % 16 := Nat.and_two_pow_sub_one_eq_mod p1 4
  have d : c >>> 4 = c / 16 := by rw [Nat.shiftRight_eq_div_pow]
  rw [a, b, d, (tbl_ok _ (by omega)).1, (tbl_ok _ (by omega)).2.1, (tbl_ok _ (by omega)).2.2]

/-- The bits of `check_special_cases` other than `TWO_CONTS` are never set
with it (checked on all byte pairs). -/
theorem sc_shape : ∀ p1, p1 < 256 → ∀ c, c < 256 → Validator.sc p1 c < 128 ∨ Validator.sc p1 c = 128 := by
  decide +kernel

theorem lane_logic : ∀ e, e < 129 → ∀ m : Bool,
    (decide (0 < e ∧ e < 128) || (m != decide (128 ≤ e))) = ((if m then 0x80 else 0) ^^^ e != 0) := by
  decide +kernel

/-- **The rvv lane check is the generic per-byte check** (`Validator.err`
with the `|` form of `must_be_2_3_continuation`, as on westmere). -/
theorem laneErr_eq (p3 p2 p1 c : Nat) (h1 : p1 < 256) (h2 : c < 256) :
    laneErr p3 p2 p1 c = (Validator.err false p3 p2 p1 c != 0) := by
  unfold laneErr Validator.err Validator.must23
  simp only [Bool.false_eq_true, ↓reduceIte]
  rw [rvvSc_eq p1 c h1 h2]
  have hm : (decide (0xE0 - 1 < p2) || decide (0xF0 - 1 < p3)) =
      (decide (0xE0 ≤ p2) || decide (0xF0 ≤ p3)) := by
    congr 1 <;> simp only [decide_eq_decide] <;> omega
  rw [hm]
  have hs := sc_shape p1 h1 c h2
  exact lane_logic _ (by omega) _

/-- Four ASCII bytes pass the check. -/
theorem err_ascii : ∀ p3, p3 < 128 → ∀ p2, p2 < 128 → ∀ p1, p1 < 128 → ∀ c, c < 128 →
    Validator.err false p3 p2 p1 c = 0 := by
  intro p3 h3 p2 h2 p1 h1 c hc
  unfold Validator.err Validator.must23
  have : (decide (0xE0 ≤ p2) || decide (0xF0 ≤ p3)) = false := by
    simp only [Bool.or_eq_false_iff, decide_eq_false_iff_not]; omega
  rw [this]
  simp only [Bool.false_eq_true, ↓reduceIte, Nat.zero_xor]
  have k : ∀ p1, p1 < 128 → ∀ c, c < 128 → Validator.sc p1 c = 0 := by decide +kernel
  exact k p1 h1 c hc

/-! # 2. Passing checks give valid prefixes

`R ys p3 p2 p1 s0`: the bytes `ys` end with `p3 p2 p1` (zeros before the
start), `s0` is the automaton state before `p1`, and `ValidatorSound.H` holds:
the invariant of `ValidatorSound.scan_sound`, kept here with its state. -/

def R (ys : List Nat) (p3 p2 p1 s0 : Nat) : Prop :=
  ValidatorSound.H s0 (decide (0xF0 ≤ p3)) (decide (0xE0 ≤ p2)) (decide (0xF0 ≤ p2)) p1 = true ∧
    run 0 ys = step s0 p1

theorem R_nil : R [] 0 0 0 0 := ⟨by decide, rfl⟩

theorem R_sound {ys : List Nat} {p3 p2 p1 s0 c : Nat} (h : R ys p3 p2 p1 s0) (hc : c < 256)
    (he : Validator.err false p3 p2 p1 c = 0) : R (ys ++ [c]) p2 p1 c (run 0 ys) := by
  obtain ⟨hH, hrun⟩ := h
  obtain ⟨-, hH'⟩ := ValidatorSound.sound_at false hH hc
    ((ValidatorSound.err_zero_iff false p3 p2 p1 c).mp he)
  rw [hrun]
  exact ⟨hH', by rw [ValidatorSound.run_snoc, hrun]⟩

theorem R_complete {ys : List Nat} {p3 p2 p1 s0 c : Nat} (h : R ys p3 p2 p1 s0) (hc : c < 256)
    (hv : run 0 (ys ++ [c]) ≠ 8) : R (ys ++ [c]) p2 p1 c (run 0 ys) := by
  obtain ⟨hH, hrun⟩ := h
  obtain ⟨hp1, hs0⟩ := ValidatorSound.H_lt hH
  have hv1 := hv
  rw [ValidatorSound.run_snoc, hrun] at hv1
  have hs1 : step s0 p1 ≠ 8 := fun h8 => hv1 (by rw [h8, ValidExact.step_8])
  obtain ⟨-, hg⟩ := ValidExact.closure_at false (ValidatorSound.H_goodA hH hs1) hp1 hc hv1
  have hle := ValidExact.step_le s0 p1
  have hH' := ValidatorSound.H_of_goodA hc (by omega) hv1 hg
  rw [hrun]
  exact ⟨hH', by rw [ValidatorSound.run_snoc, hrun]⟩

/-- Byte `k` before position `j`, zero before the start. -/
def pv (xs : List Nat) (j k : Nat) : Nat := if k ≤ j then xs.getD (j - k) 0 else 0

/-- Every position `3 ≤ j < J` passes the check, with its three bytes before. -/
def ChkTo (xs : List Nat) (J : Nat) : Prop :=
  ∀ j, 3 ≤ j → j < J →
    Validator.err false (xs.getD (j - 3) 0) (xs.getD (j - 2) 0) (xs.getD (j - 1) 0) (xs.getD j 0) = 0

theorem take_succ' (xs : List Nat) (J : Nat) (h : J < xs.length) :
    xs.take (J + 1) = xs.take J ++ [xs.getD J 0] := by
  rw [List.take_add_one, List.getElem?_eq_getElem h]
  simp [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem h]

theorem getD_lt256 {xs : List Nat} (hb : Bytes xs) (j : Nat) : xs.getD j 0 < 256 := by
  rw [List.getD_eq_getElem?_getD]
  cases h : xs[j]? with
  | none => simp
  | some v => simp; exact hb v (List.mem_of_getElem? h)

/-- The invariant up to `J`: from the valid prologue `take idx` (`idx ≥ 3`) on
by completeness, past it by soundness of each passing position. -/
theorem R_upto (xs : List Nat) (hb : Bytes xs) (idx : Nat) (h3 : 3 ≤ idx)
    (hidx : Valid8 (xs.take idx)) (J : Nat) (hc : ChkTo xs J) :
    ∀ j, j ≤ J → j ≤ xs.length →
      ∃ s0, R (xs.take j) (pv xs j 3) (pv xs j 2) (pv xs j 1) s0 ∧
        (1 ≤ j → s0 = run 0 (xs.take (j - 1))) := by
  intro j
  induction j with
  | zero => intro _ _; exact ⟨0, by simpa [pv] using R_nil, by omega⟩
  | succ j ih =>
    intro hj hl
    obtain ⟨s0, hR, -⟩ := ih (by omega) (by omega)
    have ht := take_succ' xs j (by omega)
    have e3 : pv xs (j + 1) 3 = pv xs j 2 := by unfold pv; split <;> split <;> first | omega | (congr 1; omega) | rfl
    have e2 : pv xs (j + 1) 2 = pv xs j 1 := by unfold pv; split <;> split <;> first | omega | (congr 1; omega) | rfl
    have e1 : pv xs (j + 1) 1 = xs.getD j 0 := by unfold pv; simp
    rw [e3, e2, e1, ht]
    refine ⟨run 0 (xs.take j), ?_, fun _ => by simp⟩
    by_cases hji : j + 1 ≤ idx
    · apply R_complete hR (getD_lt256 hb j)
      rw [← ht]
      have := ValidExact.run_take_ne8 hidx (j + 1)
      rwa [List.take_take, show min (j + 1) idx = j + 1 by omega] at this
    · apply R_sound hR (getD_lt256 hb j)
      have := hc j (by omega) (by omega)
      have a : pv xs j 3 = xs.getD (j - 3) 0 := by unfold pv; simp; omega
      have b : pv xs j 2 = xs.getD (j - 2) 0 := by unfold pv; simp; omega
      have c : pv xs j 1 = xs.getD (j - 1) 0 := by unfold pv; simp; omega
      rw [a, b, c]; exact this

/-- **A non-continuation byte inside the checked range is a character
boundary**: the bytes before it are valid UTF-8. -/
theorem lead_valid (xs : List Nat) (hb : Bytes xs) (idx : Nat) (h3 : 3 ≤ idx)
    (hidx : Valid8 (xs.take idx)) (J : Nat) (hc : ChkTo xs J) (q : Nat) (hq : q < J)
    (hql : q < xs.length) (hnc : isCont (xs.getD q 0) = false) : Valid8 (xs.take q) := by
  obtain ⟨s0, ⟨hH, -⟩, hs⟩ := R_upto xs hb idx h3 hidx J hc (q + 1) (by omega) (by omega)
  have e1 : pv xs (q + 1) 1 = xs.getD q 0 := by unfold pv; simp
  rw [e1] at hH
  have := (ValidatorSound.H_last hH).2 hnc
  rw [hs (by omega)] at this
  exact this

/-! # 3. The values each lane writes

The loop writes, per non-continuation byte of the block (a lane), the UTF-16
units of the character it starts. The values matter for one read: the
reparse step tests whether `dst[-1]` is a high surrogate (:257-262). On a
valid character, each path's lane writes `w16` units, the last of which is
not a high surrogate, and a lane writing two units writes a high surrogate
first (`UOK`). -/

/-- The 1/2-byte path lane (:115-125): `b2 & 0x3F`; `m1 = b1 > 0b10111111`;
`b1 & 63` under `m1`; widening multiply by `1 << 6` or 1; widening add of `b2`
under `m1`. -/
def u12 (b n1 : Nat) : Nat :=
  let b2 := n1 &&& 0x3F
  let m1 := decide (0xBF < b)
  let b1 := if m1 then b &&& 63 else b
  let b12 := b1 * (if m1 then 64 else 1)
  if m1 then (b12 + b2) % 65536 else b12

/-- The 1/2/3-byte path lane (:137-155): as `u12`, with `m3 = b1 > 0b11011111`
selecting `b1 & 15` (:146, `_mu` keeps `t1` elsewhere), and under `m3` the
16-bit shift by 6 and the widening add of `b3 & 0x3F` (:154-155). -/
def u123 (b n1 n2 : Nat) : Nat :=
  let b2 := n1 &&& 0x3F
  let b3 := n2 &&& 0x3F
  let m1 := decide (0xBF < b)
  let m3 := decide (0xDF < b)
  let t1 := if m1 then b &&& 63 else b
  let b1 := if m3 then b &&& 15 else t1
  let b12 := b1 * (if m1 then 64 else 1)
  let b12' := if m1 then (b12 + b2) % 65536 else b12
  if m3 then ((b12' <<< 6) % 65536 + b3) % 65536 else b12'

/-- A lane's units are good for the character its byte starts. -/
def UOK (U : List Nat) (b : Nat) : Prop :=
  U.length = w16 b ∧ hiS (U.getLastD 0) = false ∧ (U.length = 2 → hiS (U.headD 0) = true)

theorem and63 (x : Nat) : x &&& 63 = x % 64 := Nat.and_two_pow_sub_one_eq_mod x 6
theorem and15 (x : Nat) : x &&& 15 = x % 16 := Nat.and_two_pow_sub_one_eq_mod x 4

/-- The shapes of a character `dchar` accepts. -/
theorem dchar_shape (b n1 n2 n3 k u : Nat) (h : dchar [b, n1, n2, n3] = some (k, u)) :
    b < 0x80 ∨ (0xC2 ≤ b ∧ b < 0xE0 ∧ 0x80 ≤ n1 ∧ n1 ≤ 0xBF) ∨
    (0xE0 ≤ b ∧ b < 0xF0 ∧ (b = 0xE0 → 0xA0 ≤ n1) ∧ 0x80 ≤ n1 ∧ (b = 0xED → n1 ≤ 0x9F) ∧
      n1 ≤ 0xBF ∧ 0x80 ≤ n2 ∧ n2 ≤ 0xBF) ∨
    (0xF0 ≤ b ∧ b < 0xF5 ∧ (b = 0xF0 → 0x90 ≤ n1) ∧ 0x80 ≤ n1 ∧ (b = 0xF4 → n1 ≤ 0x8F) ∧
      n1 ≤ 0xBF ∧ 0x80 ≤ n2 ∧ n2 ≤ 0xBF ∧ 0x80 ≤ n3 ∧ n3 ≤ 0xBF) := by
  simp only [dchar] at h
  repeat' (first | (simp only [reduceCtorEq] at h) | split at h)
  all_goals first | omega | (rename_i hh; split at hh <;> split at hh <;> omega) | (split at * <;> omega)

theorem w16_one (b : Nat) (h : b < 0x80 ∨ (0xC0 ≤ b ∧ b < 0xF0)) : w16 b = 1 := by
  unfold w16; split <;> (try split) <;> omega

theorem w16_two (b : Nat) (h : 0xF0 ≤ b) : w16 b = 2 := by
  unfold w16; split <;> (try split) <;> omega

theorem hiS_lt (v : Nat) (h : v < 0xD800) : hiS v = false := by simp [hiS]; omega

theorem u12_ok (b n1 n2 n3 k u : Nat) (h : dchar [b, n1, n2, n3] = some (k, u)) (hb : b < 0xE0) :
    UOK [u12 b n1] b := by
  have hs := dchar_shape b n1 n2 n3 k u h
  refine ⟨?_, ?_, fun h2 => by simp at h2⟩
  · rw [w16_one b (by omega)]; rfl
  · simp only [List.getLastD_cons, List.getLastD_nil]
    apply hiS_lt
    unfold u12; simp only [and63]
    split <;> simp_all <;> omega

theorem u123_ok (b n1 n2 n3 k u : Nat) (h : dchar [b, n1, n2, n3] = some (k, u)) (hb : b < 0xF0) :
    UOK [u123 b n1 n2] b := by
  have hs := dchar_shape b n1 n2 n3 k u h
  refine ⟨?_, ?_, fun h2 => by simp at h2⟩
  · rw [w16_one b (by omega)]; rfl
  · simp only [List.getLastD_cons, List.getLastD_nil]
    unfold u123 hiS; simp only [and63, and15, Nat.shiftLeft_eq]
    by_cases h1 : b < 0x80
    · simp [show ¬ 0xBF < b by omega, show ¬ 0xDF < b by omega]; omega
    by_cases h2 : b < 0xE0
    · simp [show 0xBF < b by omega, show ¬ 0xDF < b by omega]; omega
    · simp [show 0xBF < b by omega, show 0xDF < b by omega]; omega

theorem c1234_val3 (b x2 x3 x4 : Nat) (hb : b / 16 = 14) :
    RvvExact.c1234 b x2 x3 x4 = (b % 16) * 4096 + (x2 % 64) * 64 + x3 % 64 := by
  have hs : RvvExact.rvvShift8 b = 4 := by
    simp only [RvvExact.rvvShift8, Nat.shiftRight_eq_div_pow, hb]; rfl
  unfold RvvExact.c1234
  simp only [hs, and63, Nat.shiftLeft_eq, Nat.shiftRight_eq_div_pow]
  have h4 := Nat.mod_lt x4 (show 64 > 0 by decide)
  omega

theorem c1234_val2 (b x2 x3 x4 : Nat) (hb : b / 16 = 12 ∨ b / 16 = 13) :
    RvvExact.c1234 b x2 x3 x4 = (b % 32) * 64 + x2 % 64 := by
  have hs : RvvExact.rvvShift8 b = 3 := by
    simp only [RvvExact.rvvShift8, Nat.shiftRight_eq_div_pow]; split <;> omega
  unfold RvvExact.c1234
  simp only [hs, and63, Nat.shiftLeft_eq, Nat.shiftRight_eq_div_pow]
  have h3 := Nat.mod_lt x3 (show 64 > 0 by decide)
  have h4 := Nat.mod_lt x4 (show 64 > 0 by decide)
  omega

theorem c1234_val1 (b x2 x3 x4 : Nat) (hb : b < 0x80) : RvvExact.c1234 b x2 x3 x4 = b := by
  have hs : RvvExact.rvvShift8 b = 0 := by
    simp only [RvvExact.rvvShift8, Nat.shiftRight_eq_div_pow]; split <;> omega
  unfold RvvExact.c1234
  simp only [hs, and63, Nat.shiftLeft_eq, Nat.shiftRight_eq_div_pow]
  have h2 := Nat.mod_lt x2 (show 64 > 0 by decide)
  have h3 := Nat.mod_lt x3 (show 64 > 0 by decide)
  have h4 := Nat.mod_lt x4 (show 64 > 0 by decide)
  omega

theorem c1234_val4 (b x2 x3 x4 : Nat) (hb : b / 16 = 15) :
    RvvExact.c1234 b x2 x3 x4 = (b % 8) * 262144 + (x2 % 64) * 4096 + (x3 % 64) * 64 + x4 % 64 := by
  have hs : RvvExact.rvvShift8 b = 5 := by
    simp only [RvvExact.rvvShift8, Nat.shiftRight_eq_div_pow, hb]; rfl
  unfold RvvExact.c1234
  simp only [hs, and63, Nat.shiftLeft_eq, Nat.shiftRight_eq_div_pow]
  omega

/-- The second unit of the surrogate pair is a low surrogate. -/
theorem sur_hi (B : Nat) :
    0xDC00 ≤ ((B &&& 0x3FF03FF) ||| 0xDC00D800) / 65536 % 65536 ∧
    ((B &&& 0x3FF03FF) ||| 0xDC00D800) / 65536 % 65536 ≤ 0xDFFF := by
  have e4 : (65536 : Nat) = 2^16 := by decide
  rw [e4, Nat.or_div_two_pow, Nat.and_div_two_pow]
  have e2 : (0x3FF03FF : Nat) / 2^16 = 2^10 - 1 := by decide
  have e3 : (0xDC00D800 : Nat) / 2^16 = 2^10 * 55 := by decide
  rw [e2, e3, Nat.and_two_pow_sub_one_eq_mod, Nat.or_comm,
    ← Nat.two_pow_add_eq_or_of_lt (Nat.mod_lt _ (by decide))]
  have := Nat.mod_lt (B / 2^16) (show 2^10 > 0 by decide)
  have e5 : (2:Nat)^10 = 1024 := by decide
  rw [e5] at this ⊢
  omega

theorem helper_big (u : Nat) (h1 : 0xFFFF < u) (h2 : u < 0x110000) :
    RvvExact.helperLane u =
      [RvvExact.surOf (u - 0x10000) % 65536, RvvExact.surOf (u - 0x10000) / 65536 % 65536] ∧
    hiS (RvvExact.surOf (u - 0x10000) % 65536) = true ∧
    hiS (RvvExact.surOf (u - 0x10000) / 65536 % 65536) = false := by
  have hm : RvvExact.merged u = RvvExact.surOf (u - 0x10000) := by
    unfold RvvExact.merged
    rw [ite_eq_left_iff.mpr (fun h => absurd h1 h)]
    rw [show (u + 4294967296 - 0x10000) % 4294967296 = u - 0x10000 by omega]
  have lo := RvvExact.surOf_lo (u - 0x10000)
  have hi : 0xDC00 ≤ RvvExact.surOf (u - 0x10000) / 65536 % 65536 ∧
      RvvExact.surOf (u - 0x10000) / 65536 % 65536 ≤ 0xDFFF := sur_hi _
  unfold RvvExact.helperLane
  rw [hm]
  generalize RvvExact.surOf (u - 0x10000) = M at lo hi ⊢
  have hne : (M / 65536 % 65536 != 0) = true := by simp; omega
  rw [hne]
  exact ⟨rfl, by simp [hiS]; omega, by simp [hiS]; omega⟩

theorem uok_pair (x y b : Nat) (hb : w16 b = 2) (l1 : hiS x = true) (l2 : hiS y = false) :
    UOK [x, y] b := ⟨by rw [hb]; rfl, l2, fun _ => l1⟩

theorem helper_uok (v b : Nat) (h1 : 0xFFFF < v) (h2 : v < 0x110000) (hb : 0xF0 ≤ b) :
    UOK (RvvExact.helperLane v) b := by
  obtain ⟨e, l1, l2⟩ := helper_big v h1 h2
  rw [e]
  exact uok_pair _ _ b (w16_two b hb) l1 l2

theorem helper_uok_small (v b : Nat) (h1 : v ≤ 0xFFFF) (hv : hiS v = false)
    (hb : b < 0x80 ∨ (0xC0 ≤ b ∧ b < 0xF0)) : UOK (RvvExact.helperLane v) b := by
  rw [RvvExact.helper_small v h1]
  exact ⟨by rw [w16_one b hb]; rfl, hv, fun h2 => by simp at h2⟩

theorem hunits_ok (b n1 n2 n3 k u : Nat) (h : dchar [b, n1, n2, n3] = some (k, u)) :
    UOK (hunits (b, n1, n2, n3)) b := by
  have hs := dchar_shape b n1 n2 n3 k u h
  unfold hunits
  simp only
  by_cases h4 : 0xF0 ≤ b
  · have hb : b / 16 = 15 := by omega
    rw [c1234_val4 b n1 n2 n3 hb]
    exact helper_uok _ b (by omega) (by omega) h4
  · have hsmall : RvvExact.c1234 b n1 n2 n3 ≤ 0xFFFF :=
      RvvExact.c1234_bmp b n1 n2 n3 (by omega) (by simp [isCont]; omega)
    apply helper_uok_small _ b hsmall _ (by omega)
    by_cases h1 : b < 0x80
    · rw [c1234_val1 _ _ _ _ h1]; exact hiS_lt _ (by omega)
    by_cases h2 : b < 0xE0
    · rw [c1234_val2 _ _ _ _ (by omega)]; exact hiS_lt _ (by omega)
    · rw [c1234_val3 _ _ _ _ (by omega)]; simp only [hiS, decide_eq_false_iff_not]; omega


/-! # 4. The scalar code

`Scalar.u8to16` transcribes one step of `scalar::utf8_to_utf16::convert` and
`convert_with_errors` (the same tests and stores). `sconv` is the loop of
both with what they return: `(SUCCESS?, bytes accepted, units written)`, the
units being `Scalar.run` (`sconvF_run`). `scalar::utf8::validate`
(include/simdutf/scalar/utf8.h:13-111) makes the same tests without the stores,
so its result is `(sconv ·).1`. On bytes, `sconv` is the scalar validator
`vwc` of `ReplacementMore` (`sconv_eq`): a non-ASCII step makes the tests of
`vchar` (`u8to16_eq`; the code points by shifts and ors are `cp2`, `cp3`,
`cp4`), and the 16-byte fast path, taken only on 16 ASCII bytes
(`Scalar.probe16_ascii`), is 16 ASCII steps of `vwc`. -/

theorem shl_or (a y n : Nat) (hy : y < 2 ^ n) : a <<< n ||| y = a * 2 ^ n + y := by
  rw [Nat.shiftLeft_eq, Nat.mul_comm, ← Nat.two_pow_add_eq_or_of_lt hy]

theorem and_m (x n : Nat) : x &&& (2 ^ n - 1) = x % 2 ^ n := Nat.and_two_pow_sub_one_eq_mod x n

theorem cp2_eq (b b1 : Nat) : ((b &&& 0x1F) <<< 6 ||| (b1 &&& 0x3F)) = cp2 b b1 := by
  have e1 : b &&& 0x1F = b % 32 := and_m b 5
  have e2 : b1 &&& 0x3F = b1 % 64 := and_m b1 6
  rw [e1, e2, shl_or _ _ 6 (by have := Nat.mod_lt b1 (show 64 > 0 by decide); simpa using this)]
  simp [cp2]

theorem cp3_eq (b b1 b2 : Nat) :
    ((b &&& 0xF) <<< 12 ||| (b1 &&& 0x3F) <<< 6 ||| (b2 &&& 0x3F)) = cp3 b b1 b2 := by
  have e0 : b &&& 0xF = b % 16 := and_m b 4
  have e1 : b1 &&& 0x3F = b1 % 64 := and_m b1 6
  have e2 : b2 &&& 0x3F = b2 % 64 := and_m b2 6
  rw [e0, e1, e2]
  have h1 := Nat.mod_lt b1 (show 64 > 0 by decide)
  have h2 := Nat.mod_lt b2 (show 64 > 0 by decide)
  rw [shl_or (b % 16) ((b1 % 64) <<< 6) 12 (by rw [Nat.shiftLeft_eq]; simp; omega)]
  rw [show b % 16 * 2 ^ 12 + (b1 % 64) <<< 6 = (b % 16 * 64 + b1 % 64) <<< 6 by
    simp [Nat.shiftLeft_eq]; omega]
  rw [shl_or _ _ 6 (by simpa using h2)]
  simp [cp3]; omega

theorem cp4_eq (b b1 b2 b3 : Nat) :
    ((b &&& 0x7) <<< 18 ||| (b1 &&& 0x3F) <<< 12 ||| (b2 &&& 0x3F) <<< 6 ||| (b3 &&& 0x3F)) =
      cp4 b b1 b2 b3 := by
  have e0 : b &&& 0x7 = b % 8 := and_m b 3
  have e1 : b1 &&& 0x3F = b1 % 64 := and_m b1 6
  have e2 : b2 &&& 0x3F = b2 % 64 := and_m b2 6
  have e3 : b3 &&& 0x3F = b3 % 64 := and_m b3 6
  rw [e0, e1, e2, e3]
  have h1 := Nat.mod_lt b1 (show 64 > 0 by decide)
  have h2 := Nat.mod_lt b2 (show 64 > 0 by decide)
  have h3 := Nat.mod_lt b3 (show 64 > 0 by decide)
  rw [shl_or (b % 8) ((b1 % 64) <<< 12) 18 (by rw [Nat.shiftLeft_eq]; simp; omega)]
  rw [show b % 8 * 2 ^ 18 + (b1 % 64) <<< 12 = (b % 8 * 64 + b1 % 64) <<< 12 by
    simp [Nat.shiftLeft_eq]; omega]
  rw [shl_or (b % 8 * 64 + b1 % 64) ((b2 % 64) <<< 6) 12 (by rw [Nat.shiftLeft_eq]; simp; omega)]
  rw [show (b % 8 * 64 + b1 % 64) * 2 ^ 12 + (b2 % 64) <<< 6 =
      ((b % 8 * 64 + b1 % 64) * 64 + b2 % 64) <<< 6 by simp [Nat.shiftLeft_eq]; omega]
  rw [shl_or _ _ 6 (by simpa using h3)]
  simp [cp4]; omega

theorem iffs (b : Nat) (hb : b < 256) :
    ((b &&& 0xC0 = 0x80) ↔ (0x80 ≤ b ∧ b < 0xC0)) ∧ ((b &&& 0xE0 = 0xC0) ↔ (0xC0 ≤ b ∧ b < 0xE0)) ∧
    ((b &&& 0xF0 = 0xE0) ↔ (0xE0 ≤ b ∧ b < 0xF0)) ∧ ((b &&& 0xF8 = 0xF0) ↔ (0xF0 ≤ b ∧ b < 0xF8)) := by
  obtain ⟨m1, m2, m3, m4⟩ := masks b hb
  refine ⟨?_, ?_, ?_, ?_⟩
  · rw [← beq_iff_eq, m1]; simp
  · rw [← beq_iff_eq, m2]; simp
  · rw [← beq_iff_eq, m3]; simp
  · rw [← beq_iff_eq, m4]; simp

set_option maxHeartbeats 2000000 in
/-- A step without the fast path accepts what `vchar` accepts. -/
theorem u8to16_eq (ys : List Nat) (hb : Bytes ys) (hne : ys ≠ [])
    (hnf : ¬ (16 ≤ ys.length ∧ Scalar.probe16 ys = true)) : Scalar.u8to16 ys = vchar ys := by
  rcases ys with _ | ⟨b, r⟩
  · exact absurd rfl hne
  have hb0 := hb b (by simp)
  obtain ⟨m1, m2, m3, m4⟩ := masks b hb0
  obtain ⟨i1, i2, i3, i4⟩ := iffs b hb0
  unfold Scalar.u8to16
  rw [ite_eq_right hnf]
  simp only [Scalar.g, Scalar.notCont, List.getD_cons_zero, List.getD_cons_succ, i2, i3, i4, cp2_eq, cp3_eq, cp4_eq]
  rcases r with _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩
  · simp only [vchar, m2, m3, m4]
    repeat' (first | rfl | omega | (simp at *; done) | split)
  · have c1 := iffs b1 (hb b1 (by simp))
    obtain ⟨n1, -⟩ := masks b1 (hb b1 (by simp))
    simp only [List.length_cons, List.length_nil, List.getD_cons_zero, List.getD_cons_succ]
    simp only [vchar, m2, m3, m4, n1, c1.1, Bool.not_eq_true', decide_eq_true_eq,
      decide_eq_false_iff_not, ne_eq]
    repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
  · have c1 := iffs b1 (hb b1 (by simp))
    have c2 := iffs b2 (hb b2 (by simp))
    obtain ⟨n1, -⟩ := masks b1 (hb b1 (by simp))
    obtain ⟨n2, -⟩ := masks b2 (hb b2 (by simp))
    simp only [List.length_cons, List.length_nil, List.getD_cons_zero, List.getD_cons_succ]
    simp only [vchar, m2, m3, m4, n1, n2, c1.1, c2.1, Bool.not_eq_true', decide_eq_true_eq,
      decide_eq_false_iff_not, ne_eq]
    repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
  · have c1 := iffs b1 (hb b1 (by simp))
    have c2 := iffs b2 (hb b2 (by simp))
    have c3 := iffs b3 (hb b3 (by simp))
    obtain ⟨n1, -⟩ := masks b1 (hb b1 (by simp))
    obtain ⟨n2, -⟩ := masks b2 (hb b2 (by simp))
    obtain ⟨n3, -⟩ := masks b3 (hb b3 (by simp))
    simp only [List.length_cons, List.length_nil, List.getD_cons_zero, List.getD_cons_succ]
    simp only [vchar, m2, m3, m4, n1, n2, n3, c1.1, c2.1, c3.1, Bool.not_eq_true', decide_eq_true_eq,
      decide_eq_false_iff_not, ne_eq]
    repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)

def sconvF : Nat → List Nat → Bool × Nat × Nat
  | 0, _ => (true, 0, 0)
  | _ + 1, [] => (true, 0, 0)
  | f + 1, x :: r =>
    match Scalar.u8to16 (x :: r) with
    | none => (false, 0, 0)
    | some (c, w) =>
      ((sconvF f ((x :: r).drop c)).1, c + (sconvF f ((x :: r).drop c)).2.1,
        w + (sconvF f ((x :: r).drop c)).2.2)

def sconv (ys : List Nat) : Bool × Nat × Nat := sconvF ys.length ys

/-- Its stores are those of the existing transcription `Scalar.run`. -/
theorem sconvF_run : ∀ f (ys : List Nat), (sconvF f ys).2.2 = Scalar.run Scalar.u8to16 f ys
  | 0, _ => rfl
  | _ + 1, [] => rfl
  | f + 1, x :: r => by
    simp only [sconvF, Scalar.run]
    split <;> rename_i h <;> rw [h]
    simp only [sconvF_run f]

theorem vwcF_ascii : ∀ k (ys : List Nat) (g : Nat), (∀ j, j < k → ys.getD j 0 < 128) → k ≤ ys.length →
    ys.length ≤ g →
    vwcF g ys = ((vwcF (g - k) (ys.drop k)).1, k + (vwcF (g - k) (ys.drop k)).2.1,
      k + (vwcF (g - k) (ys.drop k)).2.2)
  | 0, ys, g, _, _, _ => by simp
  | k + 1, [], g, _, hk, _ => by simp at hk
  | k + 1, x :: r, g, ha, hk, hg => by
    obtain ⟨g', rfl⟩ : ∃ g', g = g' + 1 := ⟨g - 1, by simp at hg; omega⟩
    have hx : x < 128 := by simpa using ha 0 (by omega)
    have hv : vchar (x :: r) = some (1, 1) := by simp [vchar, hx]
    have ih := vwcF_ascii k r g' (fun j hj => by simpa using ha (j + 1) (by omega))
      (by simp at hk; omega) (by simp at hg; omega)
    simp only [vwcF, hv, List.drop_one, List.tail_cons]
    rw [ih]
    simp only [List.drop_succ_cons, show g' + 1 - (k + 1) = g' - k by omega, Prod.mk.injEq]
    refine ⟨trivial, by omega, by omega⟩

theorem vwcF_fuel : ∀ n (ys : List Nat) f g, ys.length ≤ n → ys.length ≤ f → ys.length ≤ g →
    Bytes ys → vwcF f ys = vwcF g ys := by
  intro n
  induction n with
  | zero => intro ys f g h _ _ _; rw [List.eq_nil_of_length_eq_zero (show ys.length = 0 by omega)]; cases f <;> cases g <;> rfl
  | succ n ih =>
    intro ys f g hn hf hg hb
    rcases ys with _ | ⟨x, r⟩
    · cases f <;> cases g <;> rfl
    obtain ⟨f', rfl⟩ : ∃ f', f = f' + 1 := ⟨f - 1, by simp at hf; omega⟩
    obtain ⟨g', rfl⟩ : ∃ g', g = g' + 1 := ⟨g - 1, by simp at hg; omega⟩
    simp only [vwcF]
    split
    · rfl
    · rename_i k u hv
      rw [vchar_eq_dchar _ hb] at hv
      have := (dchar_run _ k u hv).2.1
      rw [ih _ f' g' (by simp at hn ⊢; omega) (by simp at hf ⊢; omega) (by simp at hg ⊢; omega)
        (bytes_drop hb k)]

theorem sconv_eq_aux : ∀ n (ys : List Nat) f g, ys.length ≤ n → ys.length ≤ f → ys.length ≤ g →
    Bytes ys → sconvF f ys = vwcF g ys := by
  intro n
  induction n with
  | zero => intro ys f g h _ _ _; rw [List.eq_nil_of_length_eq_zero (show ys.length = 0 by omega)]; cases f <;> cases g <;> rfl
  | succ n ih =>
    intro ys f g hn hf hg hb
    rcases ys with _ | ⟨x, r⟩
    · cases f <;> cases g <;> rfl
    obtain ⟨f', rfl⟩ : ∃ f', f = f' + 1 := ⟨f - 1, by simp at hf; omega⟩
    by_cases hfast : 16 ≤ (x :: r).length ∧ Scalar.probe16 (x :: r) = true
    · have hu : Scalar.u8to16 (x :: r) = some (16, 16) := by
        unfold Scalar.u8to16; rw [ite_eq_left hfast]
      have ha := Scalar.probe16_ascii (x :: r) hb hfast.1 hfast.2
      rw [vwcF_ascii 16 (x :: r) g ha hfast.1 hg]
      simp only [sconvF, hu]
      rw [ih _ f' (g - 16) (by simp at hn ⊢; omega) (by simp at hf ⊢; omega) (by simp at hg ⊢; omega)
        (bytes_drop hb 16)]
    · have he := u8to16_eq (x :: r) hb (by simp) hfast
      obtain ⟨g', rfl⟩ : ∃ g', g = g' + 1 := ⟨g - 1, by simp at hg; omega⟩
      simp only [sconvF, vwcF, he]
      cases hv : vchar (x :: r) with
      | none => rfl
      | some ku =>
        obtain ⟨k, u⟩ := ku
        simp only
        rw [vchar_eq_dchar _ hb] at hv
        have := (dchar_run _ k u hv).2.1
        rw [ih _ f' g' (by simp at hn ⊢; omega) (by simp at hf ⊢; omega) (by simp at hg ⊢; omega)
          (bytes_drop hb k)]

/-- **The transcribed scalar `convert` is `vwc`** on bytes. -/
theorem sconv_eq (ys : List Nat) (hb : Bytes ys) : sconv ys = vwc ys :=
  sconv_eq_aux ys.length ys _ _ (Nat.le_refl _) (Nat.le_refl _) (Nat.le_refl _) hb

/-! # 5. The model of `rvv_utf8_to_common<uint16_t, bflip, validate>`

src/rvv/rvv_utf8_to.inl.cpp:2-268. A remaining input `ys` is `xs.drop i`. The
output is the list of the 16-bit values stored so far, before the byte flip of
the big-endian variants (the flip is applied at every store and undone at the
one load, :259-260). -/

/-- The lanes of a block of `vl` bytes: each non-continuation byte with the
next three (`m`, `vcompress` of `v0..v3`, :105-111, :137, :166-167). -/
def lanes (ys : List Nat) (vl : Nat) : List (Nat × Nat × Nat × Nat) :=
  ((win ys).take vl).filter (fun q => !isCont q.1)

/-- The values a block stores (:61-249 after the check): the ASCII path is
handled in `kStep`; by the maximum byte of the block, the 1/2-byte path
(:114-133) or the 1/2/3-byte path (:136-163), otherwise the 4-byte path, whose
two halves (lanes `..M1` and `M1..`, :231-246, `M1 = vsetvlmax_e8m1`) are
stored one after the other. -/
def blockVals (M1 : Nat) (ys : List Nat) (vl : Nat) : List Nat :=
  if (ys.take vl).all (fun b => decide (b < 0xE0)) then (lanes ys vl).map (fun q => u12 q.1 q.2.1)
  else if (ys.take vl).all (fun b => decide (b < 0xF0)) then
    (lanes ys vl).map (fun q => u123 q.1 q.2.1 q.2.2.1)
  else (((lanes ys vl).take M1).map hunits).flatten ++ (((lanes ys vl).drop M1).map hunits).flatten

/-- `vfirst(errm, vl) >= 0` (:98) on the `vl + 3` bytes the block reads: some
lane `i < vl` fails `laneErr`. `rvvChk` is true when none does. -/
def rvvChk (w : List Nat) : Bool :=
  (List.range (w.length - 3)).all fun i =>
    !laneErr (w.getD i 0) (w.getD (i + 1) 0) (w.getD (i + 2) 0) (w.getD (i + 3) 0)

/-- One iteration of the loop (:49-249) on the remaining input `ys`
(`n = ys.length - 3`): `vl = vsetvl_e8m2(n)` (:50); the ASCII path when the
block and `next0..2` are ASCII (:56-72), storing the bytes; with `validate`,
`return 0` when the check fails (:80-100, `none`); otherwise `blockVals`.
`some (vl, stored values)`. -/
def kStep (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (ys : List Nat) :
    Option (Nat × List Nat) :=
  let vl := vlOf pick (ys.length - 3)
  if (ys.take (vl + 3)).all (fun b => decide (b < 0x80)) then some (vl, ys.take vl)
  else if validate && !rvvChk (ys.take (vl + 3)) then none
  else some (vl, blockVals M1 ys vl)

/-- The loop `for (; n > 0; n -= vl, src += vl, dst += vlOut)` (:49) from
consumed `i` and output `out`: `(not returned, consumed, output)`. The fuel
is at least the number of iterations in `rvvConv` (each consumes `vl ≥ 1`). -/
def kLoop (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) :
    Nat → Nat → List Nat → Bool × Nat × List Nat
  | 0, i, out => (true, i, out)
  | f + 1, i, out =>
    if (xs.drop i).length ≤ 3 then (true, i, out)
    else match kStep validate pick M1 (xs.drop i) with
      | none => (false, i, out)
      | some (c, v) => kLoop validate pick M1 xs f (i + c) (out ++ v)

/-- The prologue of the validating variant (:20-27): `idx` skips the
continuation bytes after the first three; `idx > 6` or a failed
`scalar::utf8::validate(src, idx)` returns 0. -/
def prologueS (S : List Nat → Bool × Nat × Nat) (xs : List Nat) : Bool :=
  decide (skipCont xs xs.length xs.length 3 ≤ 6) && (S (xs.take (skipCont xs xs.length xs.length 3))).1

/-- **`rvv_utf8_to_common<uint16_t, bflip, validate>`**
(src/rvv/rvv_utf8_to.inl.cpp:2-268): `(returned value, end of the stores)`.
Below 32 bytes, the scalar `convert` (:17-18). Otherwise the prologue (with
`validate`), the loop, and after it (:251-267) the reparse step: `--dst` when
`src[0]` is a continuation byte (:253-254), `src` back to a non-continuation
byte (`back`, :255-256), `--dst` when `dst[-1]` is a high surrogate
(:257-262), and the scalar `convert` of the rest at `dst` (:264-267).
`S` is the scalar code (`(ok, accepted bytes, units written)`): `sconv`, the
transcription (`rvvConv`), or `vwc`, equal to it on bytes (`sconv_eq`), in the
proofs. The scalar `convert` returns the units on success, 0 on error
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:15-138). -/
def rvvConvS (S : List Nat → Bool × Nat × Nat) (validate : Bool) (pick : Nat → Nat) (M1 : Nat)
    (xs : List Nat) : Nat × Nat :=
  if xs.length < 32 then (if (S xs).1 then (S xs).2.2 else 0, (S xs).2.2)
  else if validate && !prologueS S xs then (0, 0)
  else
    let r := kLoop validate pick M1 xs xs.length 0 []
    if r.1 then
      let out1 := if isCont (xs.getD r.2.1 0) then r.2.2.dropLast else r.2.2
      let out2 := if hiS (out1.getLastD 0) then out1.dropLast else out1
      let t := S (xs.drop (back xs r.2.1))
      let ret := if t.1 then t.2.2 else 0
      (if ret = 0 then 0 else out2.length + ret, max r.2.2.length (out2.length + t.2.2))
    else (0, r.2.2.length)

/-- **`convert_utf8_to_utf16{le,be}_with_errors`** (src/rvv/rvv_utf8_to.inl.cpp:380-396): the
validating `rvv_utf8_to_common`; a nonzero result is `SUCCESS` with that
count; otherwise the scalar `convert_with_errors`
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:140-278: `SUCCESS` with
the units, or a validation error, all of whose codes are validation errors,
at the accepted bytes). The stores of both calls count (`se`). -/
def rvvKS (S : List Nat → Bool × Nat × Nat) (pick : Nat → Nat) (M1 : Nat) (win : List Nat) :
    ReplacementRest.BRes :=
  if (rvvConvS S true pick M1 win).1 ≠ 0 then
    ⟨true, false, (rvvConvS S true pick M1 win).1, (rvvConvS S true pick M1 win).2⟩
  else
    ⟨(S win).1, !(S win).1, if (S win).1 then (S win).2.2 else (S win).2.1,
      max (rvvConvS S true pick M1 win).2 (S win).2.2⟩

/-- **`convert_valid_utf8_to_utf16{le,be}`** (src/rvv/rvv_utf8_to.inl.cpp:398-412): `rvv_utf8_to_common`
without validation. -/
def rvvCV (pick : Nat → Nat) (M1 : Nat) (v : List Nat) : Nat × Nat := rvvConvS sconv false pick M1 v

/-- The kernels with the transcribed scalar code. -/
def rvvConv : Bool → (Nat → Nat) → Nat → List Nat → Nat × Nat := rvvConvS sconv
def rvvK : (Nat → Nat) → Nat → List Nat → ReplacementRest.BRes := rvvKS sconv

/-- **`validate_utf8_with_counts`** on rvv (src/rvv/implementation.cpp:152-155): the scalar
`validate_with_counts`, whatever `vl`. -/
def rvvV (_pick : Nat → Nat) : List Nat → VRes := scalarV

/-! ## The loop -/

theorem vlOf_pos (pick : Nat → Nat) (n : Nat) (h : 1 ≤ n) : 1 ≤ vlOf pick n := by
  unfold vlOf; omega

theorem kStep_c (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (ys : List Nat) (c : Nat)
    (v : List Nat) (hT : 3 < ys.length) (h : kStep validate pick M1 ys = some (c, v)) :
    c = vlOf pick (ys.length - 3) ∧ 1 ≤ c ∧ c + 3 ≤ ys.length := by
  have h1 := vlOf_pos pick (ys.length - 3) (by omega)
  have h2 := RvvExact.vlOf_le pick (ys.length - 3)
  unfold kStep at h; dsimp only at h
  repeat' split at h
  all_goals (simp only [Option.some.injEq, Prod.mk.injEq, reduceCtorEq] at h)
  all_goals (obtain ⟨rfl, -⟩ := h; omega)

theorem kLoop_inv (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (xs : List Nat)
    (P : Nat → List Nat → Prop)
    (hstep : ∀ i out c v, P i out → 3 < (xs.drop i).length →
      kStep validate pick M1 (xs.drop i) = some (c, v) → P (i + c) (out ++ v)) :
    ∀ f i out, P i out →
      P (kLoop validate pick M1 xs f i out).2.1 (kLoop validate pick M1 xs f i out).2.2 := by
  intro f
  induction f with
  | zero => intro i out h; exact h
  | succ f ih =>
    intro i out h
    unfold kLoop
    split
    · exact h
    · rename_i hT
      split
      · exact h
      · rename_i c v hk
        exact ih _ _ (hstep i out c v h (by omega) hk)

/-- The loop leaves exactly 3 bytes when it does not return early. -/
theorem kLoop_exit (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) :
    ∀ f i out, i + 3 ≤ xs.length → xs.length ≤ i + f + 3 →
      (kLoop validate pick M1 xs f i out).1 = true →
      (kLoop validate pick M1 xs f i out).2.1 = xs.length - 3 := by
  intro f
  induction f with
  | zero => intro i out h1 h2 _; simp [kLoop]; omega
  | succ f ih =>
    intro i out h1 h2 h
    unfold kLoop at h ⊢
    split
    · simp; rename_i hT; simp at hT; omega
    · rename_i hT
      simp only [hT, ↓reduceIte] at h
      split
      · rename_i hk; rw [hk] at h; simp at h
      · rename_i c v hk
        rw [hk] at h
        have := kStep_c validate pick M1 _ c v (by omega) hk
        simp only [List.length_drop] at this
        exact ih _ _ (by omega) (by omega) h

/-- Without validation the loop never returns early. -/
theorem kLoop_novalid (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) :
    ∀ f i out, (kLoop false pick M1 xs f i out).1 = true := by
  intro f
  induction f with
  | zero => intro i out; rfl
  | succ f ih =>
    intro i out
    unfold kLoop
    split
    · rfl
    · split
      · rename_i hk; unfold kStep at hk; dsimp only at hk
        split at hk
        · cases hk
        · simp at hk
      · exact ih _ _

/-! ## One block -/

/-- The units of a lane on each path: ASCII (0), 1/2-byte (1), 1/2/3-byte
(2), 4-byte (3). -/
def laneU : Nat → Nat × Nat × Nat × Nat → List Nat
  | 0, q => [q.1]
  | 1, q => [u12 q.1 q.2.1]
  | 2, q => [u123 q.1 q.2.1 q.2.2.1]
  | _, q => hunits q

/-- The bytes each path sees. -/
def PathOK : Nat → Nat → Prop
  | 0, b => b < 0x80
  | 1, b => b < 0xE0
  | 2, b => b < 0xF0
  | _, _ => True

theorem flatten_single {α : Type} (f : α → Nat) : ∀ l : List α, (l.map (fun q => [f q])).flatten = l.map f
  | [] => rfl
  | a :: l => by simp [flatten_single f l]

theorem take_all_lt {ys : List Nat} {k m B : Nat} (h : (ys.take k).all (fun b => decide (b < B)) = true)
    (hm : m ≤ k) : (ys.take m).all (fun b => decide (b < B)) = true := by
  rw [List.all_eq_true] at h ⊢
  intro x hx
  apply h x
  have : ys.take m = (ys.take k).take m := by rw [List.take_take]; congr 1; omega
  rw [this] at hx; exact List.mem_of_mem_take hx

theorem lanes_path {ys : List Nat} {vl B : Nat} (h : (ys.take vl).all (fun b => decide (b < B)) = true)
    (q : Nat × Nat × Nat × Nat) (hq : q ∈ lanes ys vl) : q.1 < B := by
  have := List.all_eq_true.mp h q.1 (RvvExact.lanes_mem hq)
  simpa using this

/-- What a non-returning iteration stores: the concatenation of its lanes'
units, on one path. -/
theorem kStep_shape (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (ys : List Nat) (c : Nat)
    (v : List Nat) (hT : 3 < ys.length) (h : kStep validate pick M1 ys = some (c, v)) :
    ∃ path, v = ((lanes ys c).map (laneU path)).flatten ∧ ∀ q ∈ lanes ys c, PathOK path q.1 := by
  have hc := kStep_c validate pick M1 ys c v hT h
  unfold kStep at h; dsimp only at h
  rw [← hc.1] at h
  split at h
  · rename_i hasc
    simp only [Option.some.injEq, Prod.mk.injEq] at h
    obtain ⟨-, rfl⟩ := h
    refine ⟨0, ?_, fun q hq => lanes_path (take_all_lt hasc (by omega)) q hq⟩
    have hall : ∀ q ∈ (win ys).take c, (!isCont q.1) = true := by
      intro q hq
      have h1 : q.1 ∈ ((win ys).take c).map Prod.fst := List.mem_map_of_mem hq
      rw [List.map_take, RvvExact.win_fst] at h1
      have h2 : q.1 ∈ ys.take (c + 3) := by
        have : ys.take c = (ys.take (c + 3)).take c := by rw [List.take_take]; congr 1; omega
        rw [this] at h1; exact List.mem_of_mem_take h1
      have := List.all_eq_true.mp hasc _ h2
      simp [isCont] at this ⊢; omega
    have hl : lanes ys c = (win ys).take c := List.filter_eq_self.mpr hall
    show ys.take c = ((lanes ys c).map (fun q => [q.1])).flatten
    rw [flatten_single, hl, List.map_take, RvvExact.win_fst]
  · split at h
    · cases h
    · simp only [Option.some.injEq, Prod.mk.injEq] at h
      obtain ⟨-, rfl⟩ := h
      unfold blockVals
      split
      · rename_i h1
        exact ⟨1, by rw [← flatten_single]; rfl, fun q hq => lanes_path h1 q hq⟩
      · split
        · rename_i _ h2
          exact ⟨2, by rw [← flatten_single]; rfl, fun q hq => lanes_path h2 q hq⟩
        · refine ⟨3, ?_, fun _ _ => trivial⟩
          rw [← List.flatten_append, ← List.map_append, List.take_append_drop]; rfl

theorem w16_nc (b : Nat) (h : isCont b = false) : 1 ≤ w16 b := by
  simp [isCont] at h; unfold w16; split <;> (try split) <;> omega

theorem wU8_true : RvvExact.wU8 true = w16 := rfl

theorem laneU_le (path : Nat) (q : Nat × Nat × Nat × Nat) (hnc : isCont q.1 = false) :
    (laneU path q).length ≤ w16 q.1 := by
  have := w16_nc q.1 hnc
  match path with
  | 0 => simp [laneU]; omega
  | 1 => simp [laneU]; omega
  | 2 => simp [laneU]; omega
  | n + 3 => exact RvvExact.hcnt_le q hnc

theorem laneU_ok (path : Nat) (q : Nat × Nat × Nat × Nat) (hp : PathOK path q.1)
    (hd : ∃ k u, dchar [q.1, q.2.1, q.2.2.1, q.2.2.2] = some (k, u)) : UOK (laneU path q) q.1 := by
  obtain ⟨k, u, hd⟩ := hd
  obtain ⟨b, n1, n2, n3⟩ := q
  match path with
  | 0 =>
    simp only [PathOK] at hp
    exact ⟨by rw [w16_one b (by omega)]; rfl, hiS_lt _ (by simp [laneU]; omega),
      fun h => by simp [laneU] at h⟩
  | 1 => exact u12_ok b n1 n2 n3 k u hd hp
  | 2 => exact u123_ok b n1 n2 n3 k u hd hp
  | n + 3 => exact hunits_ok b n1 n2 n3 k u hd

theorem flatten_len {α : Type} (f : α → List Nat) :
    ∀ l : List α, ((l.map f).flatten).length = L (l.map (fun q => (f q).length))
  | [] => rfl
  | a :: l => by simp [flatten_len f l]

theorem lanes_w16 (ys : List Nat) (vl : Nat) :
    L ((lanes ys vl).map (fun q => w16 q.1)) = L ((ys.take vl).map w16) := by
  have := RvvExact.lanes_L true ys vl
  rw [wU8_true] at this
  exact this

theorem lanes_nc' {ys : List Nat} {vl : Nat} {q : Nat × Nat × Nat × Nat} (h : q ∈ lanes ys vl) :
    isCont q.1 = false := RvvExact.lanes_nc h

/-- A block stores at most `L` of its bytes. -/
theorem block_le (path : Nat) (ys : List Nat) (vl : Nat) :
    (((lanes ys vl).map (laneU path)).flatten).length ≤ L ((ys.take vl).map w16) := by
  rw [flatten_len, ← lanes_w16]
  exact RvvExact.L_map_le _ _ _ (fun q hq => laneU_le path q (lanes_nc' hq))

/-- On good lanes, exactly `L` of its bytes. -/
theorem block_eq (path : Nat) (ys : List Nat) (vl : Nat)
    (hok : ∀ q ∈ lanes ys vl, UOK (laneU path q) q.1) :
    (((lanes ys vl).map (laneU path)).flatten).length = L ((ys.take vl).map w16) := by
  rw [flatten_len, ← lanes_w16]
  exact RvvExact.L_map_congr _ _ _ (fun q hq => (hok q hq).1)

/-! ## The last units -/

theorem getLastD_append_ne (o U : List Nat) (h : U ≠ []) : (o ++ U).getLastD 0 = U.getLastD 0 := by
  simp only [List.getLastD_eq_getLast?, List.getLast?_append]
  cases hU : U.getLast? with
  | none => simp at hU; exact absurd hU h
  | some x => simp

theorem uok_shape (U : List Nat) (b : Nat) (hnc : isCont b = false) (h : UOK U b) :
    (∃ u, U = [u] ∧ w16 b = 1 ∧ hiS u = false) ∨
    (∃ x y, U = [x, y] ∧ w16 b = 2 ∧ hiS x = true ∧ hiS y = false) := by
  obtain ⟨h1, h2, h3⟩ := h
  have := w16_nc b hnc
  have hle : w16 b ≤ 2 := by unfold w16; split <;> (try split) <;> omega
  match U, h1, h2, h3 with
  | [u], h1, h2, _ => left; exact ⟨u, rfl, by simp at h1; omega, by simpa using h2⟩
  | [x, y], h1, h2, h3 => right; exact ⟨x, y, rfl, by simp at h1; omega, by simpa using h3 rfl, by simpa using h2⟩
  | [], h1, _, _ => simp at h1; omega
  | _ :: _ :: _ :: _, h1, _, _ => simp at h1; omega

/-- **Appending good lanes**: the last unit is never a high surrogate; the
unit before it is one exactly when the last lane wrote a pair. -/
theorem tails (U : Nat × Nat × Nat × Nat → List Nat) :
    ∀ (ls : List (Nat × Nat × Nat × Nat)) (o : List Nat),
      (∀ q ∈ ls, isCont q.1 = false ∧ UOK (U q) q.1) → hiS (o.getLastD 0) = false →
      hiS ((o ++ (ls.map U).flatten).getLastD 0) = false ∧
      (∀ q, ls.getLast? = some q →
        (w16 q.1 = 2 → hiS ((o ++ (ls.map U).flatten).dropLast.getLastD 0) = true) ∧
        (w16 q.1 = 1 → hiS ((o ++ (ls.map U).flatten).dropLast.getLastD 0) = false))
  | [], o, _, ho => by simpa using ho
  | q :: rest, o, hq, ho => by
    have hq0 := hq q (by simp)
    have ho' : hiS ((o ++ U q).getLastD 0) = false := by
      rcases uok_shape (U q) q.1 hq0.1 hq0.2 with ⟨u, e, -, hu⟩ | ⟨x, y, e, -, -, hy⟩
      · rw [e, getLastD_append_ne _ _ (by simp)]; simpa using hu
      · rw [e, getLastD_append_ne _ _ (by simp)]; simpa using hy
    have ih := tails U rest (o ++ U q) (fun q' h' => hq q' (by simp [h'])) ho'
    have e : o ++ ((q :: rest).map U).flatten = (o ++ U q) ++ (rest.map U).flatten := by simp
    rw [e]
    refine ⟨ih.1, fun q' hl => ?_⟩
    cases rest with
    | nil =>
      simp at hl; subst hl
      simp only [List.map_nil, List.flatten_nil, List.append_nil]
      rcases uok_shape (U q) q.1 hq0.1 hq0.2 with ⟨u, e, hw, hu⟩ | ⟨x, y, e, hw, hx, hy⟩
      · rw [e, List.dropLast_concat]
        exact ⟨fun h => by omega, fun _ => ho⟩
      · rw [e, show o ++ [x, y] = (o ++ [x]) ++ [y] by simp, List.dropLast_concat]
        refine ⟨fun _ => ?_, fun h => by omega⟩
        rw [getLastD_append_ne _ _ (by simp)]; simpa using hx
    | cons r rs =>
      have hl' : (r :: rs).getLast? = some q' := by
        rw [List.getLast?_cons_cons] at hl; exact hl
      exact ih.2 q' hl'

/-! ## Lanes of valid input -/

theorem win_get : ∀ (ys : List Nat) (j : Nat), j < ys.length →
    (win ys)[j]? = some (ys.getD j 0, ys.getD (j + 1) 0, ys.getD (j + 2) 0, ys.getD (j + 3) 0)
  | [], j, h => by simp at h
  | b :: r, 0, _ => by simp [win]
  | b :: r, j + 1, h => by
    simp only [win, List.getElem?_cons_succ]
    rw [win_get r j (by simp at h; omega)]
    simp

theorem lanes_pos {ys : List Nat} {vl : Nat} {q : Nat × Nat × Nat × Nat} (hq : q ∈ lanes ys vl) :
    ∃ j, j < vl ∧ j < ys.length ∧
      q = (ys.getD j 0, ys.getD (j + 1) 0, ys.getD (j + 2) 0, ys.getD (j + 3) 0) := by
  have h1 := (List.mem_filter.mp hq).1
  obtain ⟨n, hn⟩ := List.mem_iff_getElem?.mp h1
  rw [List.getElem?_take] at hn
  split at hn
  · rename_i hlt
    have hl : n < (win ys).length := by
      cases h : (win ys)[n]? with
      | none => rw [h] at hn; cases hn
      | some _ => exact (List.getElem?_eq_some_iff.mp h).1
    have hwl : (win ys).length = ys.length := by
      have := congrArg List.length (RvvExact.win_fst ys); simpa using this
    rw [win_get ys n (by omega)] at hn
    exact ⟨n, hlt, by omega, (Option.some.inj hn).symm⟩
  · cases hn

theorem take4 (xs : List Nat) (p : Nat) (h : p + 4 ≤ xs.length) :
    (xs.drop p).take 4 = [xs.getD p 0, xs.getD (p + 1) 0, xs.getD (p + 2) 0, xs.getD (p + 3) 0] := by
  have e : ∀ k, k < xs.length → xs[k]? = some (xs.getD k 0) := by
    intro k hk; simp [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hk]
  apply List.ext_getElem?
  intro n
  rw [List.getElem?_take, List.getElem?_drop]
  rcases (show n = 0 ∨ n = 1 ∨ n = 2 ∨ n = 3 ∨ 4 ≤ n by omega) with rfl | rfl | rfl | rfl | hn
  · rw [e _ (by omega)]; simp
  · rw [e _ (by omega)]; simp
  · rw [e _ (by omega)]; simp
  · rw [e _ (by omega)]; simp
  · rw [ite_eq_right (by omega)]; simp; omega

/-- In valid input, every lane is a character `dchar` accepts. -/
theorem lane_dchar (xs : List Nat) (hv : Valid8 xs) (p : Nat) (hp : p + 4 ≤ xs.length)
    (hnc : isCont (xs.getD p 0) = false) :
    ∃ k u, dchar [xs.getD p 0, xs.getD (p + 1) 0, xs.getD (p + 2) 0, xs.getD (p + 3) 0] = some (k, u) := by
  have hbd := ValidExact.lead_bd hv (by omega) hnc
  have hvd := ValidExact.valid_drop hv hbd
  rw [← take4 xs p hp, ← ValidatorSound.dchar_take4]
  have e : xs.drop p = xs.getD p 0 :: xs.drop (p + 1) := by
    rw [List.drop_eq_getElem_cons (by omega)]
    simp [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem (show p < xs.length by omega)]
  rw [e] at hvd ⊢
  obtain ⟨k, u, hd, -⟩ := dchar_of_valid _ _ hvd
  exact ⟨k, u, hd⟩

/-! ## The count on valid input -/

/-- The loop invariant on valid input: the output is `L` of the bytes
consumed, its last unit is not a high surrogate, and the unit before it is
one exactly when the last character started so far is a 4-byte one. -/
def CInv (xs : List Nat) (i : Nat) (out : List Nat) : Prop :=
  out.length = L ((xs.take i).map w16) ∧ hiS (out.getLastD 0) = false ∧
    ∀ b, lastNC (xs.take i) = some b →
      (w16 b = 2 → hiS (out.dropLast.getLastD 0) = true) ∧
      (w16 b = 1 → hiS (out.dropLast.getLastD 0) = false)

theorem cinv_step (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (xs : List Nat)
    (hv : Valid8 xs) (i : Nat) (out : List Nat) (c : Nat) (v : List Nat) (h : CInv xs i out)
    (hT : 3 < (xs.drop i).length) (hk : kStep validate pick M1 (xs.drop i) = some (c, v)) :
    CInv xs (i + c) (out ++ v) := by
  obtain ⟨-, -, hc3⟩ := kStep_c validate pick M1 _ c v hT hk
  obtain ⟨path, rfl, hpath⟩ := kStep_shape validate pick M1 _ c v hT hk
  simp only [List.length_drop] at hc3
  have hok : ∀ q ∈ lanes (xs.drop i) c, isCont q.1 = false ∧ UOK (laneU path q) q.1 := by
    intro q hq
    refine ⟨lanes_nc' hq, laneU_ok path q (hpath q hq) ?_⟩
    obtain ⟨j, hj, -, rfl⟩ := lanes_pos hq
    have hnc := lanes_nc' hq
    simp only [ValidatorSound.getD_drop'] at hnc ⊢
    have := lane_dchar xs hv (i + j) (by omega) hnc
    simpa [Nat.add_assoc] using this
  obtain ⟨h1, h2, h3⟩ := h
  have ht := tails (laneU path) (lanes (xs.drop i) c) out hok h2
  refine ⟨?_, ht.1, ?_⟩
  · rw [List.length_append, h1, block_eq path _ c (fun q hq => (hok q hq).2),
      RvvExact.L_map_take_add]
  · intro b hb
    rw [List.take_add, RvvExact.lastNC_append] at hb
    have hf := RvvExact.lanes_fst (xs.drop i) c
    have hlast : lastNC ((xs.drop i).take c) = (lanes (xs.drop i) c).getLast?.map Prod.fst := by
      rw [lastNC, ← hf, List.getLast?_map]; rfl
    rw [hlast] at hb
    cases hq : (lanes (xs.drop i) c).getLast? with
    | none =>
      rw [hq] at hb
      have : lanes (xs.drop i) c = [] := List.getLast?_eq_none_iff.mp hq
      rw [this]; simp only [List.map_nil, List.flatten_nil, List.append_nil]
      exact h3 b (by simpa using hb)
    | some q =>
      rw [hq] at hb
      simp at hb; subst hb
      exact ht.2 q hq

/-! ## The scalar code -/

theorem vwc_valid (ys : List Nat) (hb : Bytes ys) (hv : Valid8 ys) :
    (vwc ys).1 = true ∧ (vwc ys).2.2 = L (ys.map w16) := by
  obtain ⟨s1, s2⟩ := vwcF_spec ys.length ys hb (Nat.le_refl _)
  cases hok : (vwc ys).1
  · exfalso
    obtain ⟨hlt, hvp, -, hd⟩ := s2 hok
    generalize (vwcF ys.length ys).2.1 = e at hlt hvp hd
    have hbd : Bd ys e := hvp
    have hvd := ValidExact.valid_drop hv hbd
    have e1 : ys.drop e = ys.getD e 0 :: ys.drop (e + 1) := by
      rw [List.drop_eq_getElem_cons hlt]
      simp [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hlt]
    rw [e1] at hvd hd
    obtain ⟨k, u, hd', -⟩ := dchar_of_valid _ _ hvd
    rw [hd] at hd'; cases hd'
  · exact ⟨rfl, (s1 hok).2⟩

theorem vwc_store (ys : List Nat) (hb : Bytes ys) :
    ∃ e, e ≤ ys.length ∧ Valid8 (ys.take e) ∧ (vwc ys).2.2 = L ((ys.take e).map w16) := by
  obtain ⟨s1, s2⟩ := vwcF_spec ys.length ys hb (Nat.le_refl _)
  cases hok : (vwc ys).1
  · obtain ⟨hlt, hvp, hu, -⟩ := s2 hok
    exact ⟨_, Nat.le_of_lt hlt, hvp, hu⟩
  · obtain ⟨hv, hu⟩ := s1 hok
    exact ⟨ys.length, Nat.le_refl _, by rw [List.take_length]; exact hv, by rw [List.take_length]; exact hu⟩

theorem vwc_valid_ok (ys : List Nat) (hb : Bytes ys) (h : (vwc ys).1 = true) : Valid8 ys :=
  ((vwcF_spec ys.length ys hb (Nat.le_refl _)).1 h).1

/-! ## The reparse step on valid input -/

theorem getD_cons_drop (xs : List Nat) (q : Nat) (hq : q < xs.length) :
    xs.drop q = xs.getD q 0 :: xs.drop (q + 1) := by
  rw [List.drop_eq_getElem_cons hq]
  simp [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hq]

/-- **The reparse step is exact on valid input**
(src/rvv/rvv_utf8_to.inl.cpp:252-262): after the loop,
the cursor goes back to `L` of the bytes before the last character start
`back xs (len - 3)`. A 2- or 3-byte character there wrote one unit, and the
unit before it is not a high surrogate; a 4-byte one wrote a pair, whose
first unit the second test finds. -/
theorem reparse_valid (xs : List Nat) (hv : Valid8 xs) (hlen : 32 ≤ xs.length) (out : List Nat)
    (h : CInv xs (xs.length - 3) out) :
    (if hiS ((if isCont (xs.getD (xs.length - 3) 0) then out.dropLast else out).getLastD 0) then
        (if isCont (xs.getD (xs.length - 3) 0) then out.dropLast else out).dropLast
      else (if isCont (xs.getD (xs.length - 3) 0) then out.dropLast else out)).length =
      L ((xs.take (back xs (xs.length - 3))).map w16) ∧
    isCont (xs.getD (back xs (xs.length - 3)) 0) = false ∧ back xs (xs.length - 3) ≤ xs.length - 3 := by
  generalize hP : xs.length - 3 = P at h ⊢
  have hPl : P < xs.length := by omega
  obtain ⟨b1, b2, b3, b4⟩ := RvvExact.back_spec w16 ValidExact.w16_cont xs P hPl
  generalize hq : back xs P = q at b1 b2 b3 b4 ⊢
  have hnc : isCont (xs.getD q 0) = false := by
    rcases b3 with h0 | h0
    · subst h0; exact ValidExact.bd_lead hv rfl (by omega)
    · exact h0
  have hlast := b2 hnc
  obtain ⟨h1, h2, h3⟩ := h
  have hW1 := w16_nc _ hnc
  have hW2 : w16 (xs.getD q 0) ≤ 2 := by unfold w16; split <;> (try split) <;> omega
  refine ⟨?_, hnc, b4⟩
  by_cases hc : isCont (xs.getD P 0) = true
  · rw [RvvExact.lastNC_take_succ xs P hPl] at hlast
    simp only [hc, ↓reduceIte] at hlast
    rw [RvvExact.L_take_succ w16 xs P hPl, ValidExact.w16_cont _ hc] at b1
    obtain ⟨k2, k1⟩ := h3 _ hlast
    simp only [hc, ↓reduceIte]
    by_cases hw : w16 (xs.getD q 0) = 2
    · rw [ite_eq_left (k2 hw), List.length_dropLast, List.length_dropLast]; omega
    · have hw1 : w16 (xs.getD q 0) = 1 := by omega
      rw [ite_eq_right (by rw [k1 hw1]; simp), List.length_dropLast]; omega
  · have hc' : isCont (xs.getD P 0) = false := by simpa using hc
    have hqP : q = P := by
      rw [← hq]
      obtain ⟨p', rfl⟩ : ∃ p', P = p' + 1 := ⟨P - 1, by omega⟩
      simp only [back, hc', Bool.false_eq_true, ↓reduceIte]
    subst hqP
    simp only [hc', Bool.false_eq_true, ↓reduceIte, h2]
    exact h1

theorem cinv_zero (xs : List Nat) : CInv xs 0 [] := ⟨rfl, by decide, by simp [lastNC]⟩

/-- **On valid input, `rvv_utf8_to_common` is exact**: when the loop runs to
its end (always without `validate`), it returns `L` of the `w16` weights and
stores inside it. -/
theorem conv_valid (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) (hb : Bytes xs)
    (hv : Valid8 xs)
    (hrun : 32 ≤ xs.length → (validate && !prologueS vwc xs) = false ∧
      (kLoop validate pick M1 xs xs.length 0 []).1 = true) :
    (rvvConvS vwc validate pick M1 xs).1 = L (xs.map w16) ∧ (rvvConvS vwc validate pick M1 xs).2 ≤ L (xs.map w16) := by
  unfold rvvConvS
  by_cases hl : xs.length < 32
  · obtain ⟨o1, o2⟩ := vwc_valid xs hb hv
    simp only [hl, ↓reduceIte, o1, o2]; exact ⟨trivial, Nat.le_refl _⟩
  obtain ⟨hp, hk⟩ := hrun (by omega)
  simp only [hl, ↓reduceIte, hp, Bool.false_eq_true]
  have hex := kLoop_exit validate pick M1 xs xs.length 0 [] (by omega) (by omega) hk
  have hinv := kLoop_inv validate pick M1 xs (CInv xs)
    (fun i out c v h hT hk => cinv_step validate pick M1 xs hv i out c v h hT hk)
    xs.length 0 [] (cinv_zero xs)
  generalize kLoop validate pick M1 xs xs.length 0 [] = r at hk hex hinv
  obtain ⟨ok, i, out⟩ := r
  simp only at hk hex hinv ⊢
  subst hk hex
  obtain ⟨e1, hnc, hle⟩ := reparse_valid xs hv (by omega) out hinv
  simp only [↓reduceIte]
  generalize back xs (xs.length - 3) = q at e1 hnc hle
  have hbd := ValidExact.lead_bd hv (show q < xs.length by omega) hnc
  obtain ⟨t1, t2⟩ := vwc_valid _ (bytes_drop hb q) (ValidExact.valid_drop hv hbd)
  have hpos : 1 ≤ L ((xs.drop q).map w16) := by
    rw [getD_cons_drop xs q (by omega)]; simp only [List.map_cons, L_cons]
    have := w16_nc _ hnc; omega
  have hsplit := L_map_split xs q
  have hout : out.length ≤ L (xs.map w16) := by
    rw [hinv.1]; exact RvvExact.L_map_take_le w16 xs _
  rw [t1, t2]
  simp only [↓reduceIte, show ¬ L ((xs.drop q).map w16) = 0 by omega]
  rw [e1]
  refine ⟨by omega, ?_⟩
  simp only [Nat.max_le]; omega

/-! ## The validating kernel on any input -/

theorem getD_take' (ys : List Nat) (n k : Nat) (h : k < n) : (ys.take n).getD k 0 = ys.getD k 0 := by
  simp only [List.getD_eq_getElem?_getD, List.getElem?_take, ite_eq_left h]

/-- A block the validating loop gets past passes the check at each of its
lanes' positions (the ASCII path: four ASCII bytes pass). -/
theorem kStep_chk (pick : Nat → Nat) (M1 : Nat) (ys : List Nat) (hb : Bytes ys) (c : Nat)
    (v : List Nat) (hT : 3 < ys.length) (h : kStep true pick M1 ys = some (c, v)) :
    ∀ m, m < c → Validator.err false (ys.getD m 0) (ys.getD (m + 1) 0) (ys.getD (m + 2) 0)
      (ys.getD (m + 3) 0) = 0 := by
  obtain ⟨hc, -, hc3⟩ := kStep_c true pick M1 ys c v hT h
  intro m hm
  unfold kStep at h; dsimp only at h
  rw [← hc] at h
  have hg : ∀ k, k < c + 3 → (ys.take (c + 3)).getD k 0 = ys.getD k 0 :=
    fun k hk => getD_take' ys (c + 3) k hk
  split at h
  · rename_i hasc
    have ha : ∀ k, k < c + 3 → ys.getD k 0 < 128 := by
      intro k hk
      rw [← hg k hk]
      have hmem : (ys.take (c + 3)).getD k 0 ∈ ys.take (c + 3) := by
        rw [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem (by simp; omega),
          Option.getD_some]
        exact List.getElem_mem _
      have := List.all_eq_true.mp hasc _ hmem
      simpa using this
    exact err_ascii _ (ha m (by omega)) _ (ha _ (by omega)) _ (ha _ (by omega)) _ (ha _ (by omega))
  · split at h
    · cases h
    · rename_i hchk
      simp only [Bool.true_and, Bool.not_eq_true', Bool.not_eq_false] at hchk
      unfold rvvChk at hchk
      have hlen : (ys.take (c + 3)).length - 3 = c := by simp; omega
      rw [hlen] at hchk
      have := List.all_eq_true.mp hchk m (List.mem_range.mpr hm)
      rw [hg m (by omega), hg _ (by omega), hg _ (by omega), hg _ (by omega)] at this
      rw [laneErr_eq _ _ _ _ (getD_lt256 hb _) (getD_lt256 hb _)] at this
      simpa using this

/-- The invariant of the validating loop: the checked range, and the output
within `L` of the bytes consumed. -/
def SInv (xs : List Nat) (i : Nat) (out : List Nat) : Prop :=
  i + 3 ≤ xs.length ∧ ChkTo xs (i + 3) ∧ out.length ≤ L ((xs.take i).map w16)

theorem sinv_step (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) (hb : Bytes xs) (i : Nat)
    (out : List Nat) (c : Nat) (v : List Nat) (h : SInv xs i out) (hT : 3 < (xs.drop i).length)
    (hk : kStep true pick M1 (xs.drop i) = some (c, v)) : SInv xs (i + c) (out ++ v) := by
  obtain ⟨-, -, hc3⟩ := kStep_c true pick M1 _ c v hT hk
  simp only [List.length_drop] at hc3
  have hchk := kStep_chk pick M1 _ (bytes_drop hb i) c v hT hk
  obtain ⟨path, hv, -⟩ := kStep_shape true pick M1 _ c v hT hk
  obtain ⟨h1, h2, h3⟩ := h
  refine ⟨by omega, fun j hj1 hj2 => ?_, ?_⟩
  · by_cases hj : j < i + 3
    · exact h2 j hj1 hj
    · have := hchk (j - 3 - i) (by omega)
      simp only [ValidatorSound.getD_drop'] at this
      rw [show i + (j - 3 - i) = j - 3 by omega, show i + (j - 3 - i + 1) = j - 2 by omega,
        show i + (j - 3 - i + 2) = j - 1 by omega, show i + (j - 3 - i + 3) = j by omega] at this
      exact this
  · rw [List.length_append, RvvExact.L_map_take_add, hv]
    have := block_le path (xs.drop i) c
    omega

theorem skip_le (xs : List Nat) (len : Nat) : ∀ f idx, idx ≤ len → skipCont xs len f idx ≤ len := by
  intro f
  induction f with
  | zero => intro idx h; simpa [skipCont] using h
  | succ f ih =>
    intro idx h
    unfold skipCont
    split
    · rename_i hc; exact ih _ (by omega)
    · exact h

theorem prologue_spec (xs : List Nat) (hb : Bytes xs) (h : prologueS vwc xs = true) :
    3 ≤ skipCont xs xs.length xs.length 3 ∧ Valid8 (xs.take (skipCont xs xs.length xs.length 3)) := by
  unfold prologueS at h
  simp only [Bool.and_eq_true, decide_eq_true_eq] at h
  exact ⟨(RvvExact.skip_spec xs xs.length xs.length 3).1,
    vwc_valid_ok _ (bytes_take hb _) h.2⟩

theorem sinv_loop (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) (hb : Bytes xs) (hlen : 32 ≤ xs.length) :
    SInv xs (kLoop true pick M1 xs xs.length 0 []).2.1 (kLoop true pick M1 xs xs.length 0 []).2.2 :=
  kLoop_inv true pick M1 xs (SInv xs)
    (fun i out c v h hT hk => sinv_step pick M1 xs hb i out c v h hT hk) xs.length 0 []
    ⟨by omega, fun j h1 h2 => by omega, by simp⟩

/-- A valid prefix ending at most 2 units below `L` of the bytes up to `i`. -/
theorem back_valid (xs : List Nat) (hb : Bytes xs) (idx : Nat) (h3 : 3 ≤ idx)
    (hidx : Valid8 (xs.take idx)) (J i : Nat) (hc : ChkTo xs J) (hi : i < J) (hil : i < xs.length) :
    Valid8 (xs.take (back xs i)) ∧ back xs i ≤ i ∧
      L ((xs.take (i + 1)).map w16) ≤ L ((xs.take (back xs i)).map w16) + 2 := by
  obtain ⟨b1, -, b3, b4⟩ := RvvExact.back_spec w16 ValidExact.w16_cont xs i hil
  have hW : w16 (xs.getD (back xs i) 0) ≤ 2 := by unfold w16; split <;> (try split) <;> omega
  refine ⟨?_, b4, by omega⟩
  rcases b3 with h0 | h0
  · rw [h0]; rfl
  · exact lead_valid xs hb idx h3 hidx J hc _ (by omega) (by omega) h0

/-- **Soundness**: a nonzero result of the validating `rvv_utf8_to_common`
means valid input. -/
theorem conv_sound (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) (hb : Bytes xs)
    (h : (rvvConvS vwc true pick M1 xs).1 ≠ 0) :
    Valid8 xs ∧ (32 ≤ xs.length → (true && !prologueS vwc xs) = false ∧
      (kLoop true pick M1 xs xs.length 0 []).1 = true) := by
  unfold rvvConvS at h
  by_cases hl : xs.length < 32
  · simp only [hl, ↓reduceIte] at h
    split at h
    · rename_i hok; exact ⟨vwc_valid_ok xs hb hok, fun h' => by omega⟩
    · exact absurd rfl h
  simp only [hl, ↓reduceIte] at h
  by_cases hp : prologueS vwc xs = true
  · simp only [hp, Bool.not_true, Bool.and_false, Bool.false_eq_true, ↓reduceIte] at h
    obtain ⟨h3, hidx⟩ := prologue_spec xs hb hp
    have hs := sinv_loop pick M1 xs hb (by omega)
    have hex := kLoop_exit true pick M1 xs xs.length 0 [] (by omega) (by omega)
    generalize kLoop true pick M1 xs xs.length 0 [] = r at h hs hex ⊢
    obtain ⟨ok, i, out⟩ := r
    simp only at h hs hex ⊢
    cases ok
    · simp at h
    · simp only [↓reduceIte] at h
      have hi := hex rfl
      subst hi
      obtain ⟨-, hc, -⟩ := hs
      refine ⟨?_, fun _ => ⟨by simp [hp], rfl⟩⟩
      split at h
      · rename_i hok
        obtain ⟨hvp, -, -⟩ := back_valid xs hb _ h3 hidx _ (xs.length - 3) hc (by omega) (by omega)
        have hvd := vwc_valid_ok _ (bytes_drop hb _) hok
        unfold Valid8 at hvp hvd ⊢
        rw [← List.take_append_drop (back xs (xs.length - 3)) xs, ValidExact.run_append, hvp, hvd]
      · simp at h
  · simp only [Bool.not_eq_true] at hp
    simp [hp] at h

theorem out2_le (a b : Bool) (out : List Nat) :
    (if a then (if b then out.dropLast else out).dropLast else (if b then out.dropLast else out)).length
      ≤ out.length := by
  cases a <;> cases b <;> simp <;> omega

theorem valid_take_add (xs : List Nat) (q e : Nat) (h1 : Valid8 (xs.take q))
    (h2 : Valid8 ((xs.drop q).take e)) : Valid8 (xs.take (q + e)) := by
  unfold Valid8 at *
  rw [List.take_add, ValidExact.run_append, h1, h2]

/-- **Stores of the validating `rvv_utf8_to_common`, on any input**: every
store ends at most 2 units past `L` of a valid prefix (the loop's blocks end
at `L` of the bytes consumed, which the last character start splits into a
valid prefix and at most 2 units; the scalar tail adds the units of a valid
continuation). -/
theorem conv_store (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) (hb : Bytes xs) :
    ∃ c, c ≤ xs.length ∧ Valid8 (xs.take c) ∧
      (rvvConvS vwc true pick M1 xs).2 ≤ L ((xs.take c).map w16) + 2 := by
  unfold rvvConvS
  by_cases hl : xs.length < 32
  · simp only [hl, ↓reduceIte]
    obtain ⟨e, he, hv, hu⟩ := vwc_store xs hb
    exact ⟨e, he, hv, by omega⟩
  simp only [hl, ↓reduceIte]
  by_cases hp : prologueS vwc xs = true
  · simp only [hp, Bool.not_true, Bool.and_false, Bool.false_eq_true, ↓reduceIte]
    obtain ⟨h3, hidx⟩ := prologue_spec xs hb hp
    have hs := sinv_loop pick M1 xs hb (by omega)
    have hex := kLoop_exit true pick M1 xs xs.length 0 [] (by omega) (by omega)
    generalize kLoop true pick M1 xs xs.length 0 [] = r at hs hex ⊢
    obtain ⟨ok, i, out⟩ := r
    simp only at hs hex ⊢
    obtain ⟨hi3, hc, hout⟩ := hs
    obtain ⟨bv, bl, bL⟩ := back_valid xs hb _ h3 hidx (i + 3) i hc (by omega) (by omega)
    have hmono := RvvExact.L_take_mono w16 xs (show i ≤ i + 1 by omega)
    cases ok
    · simp only [Bool.false_eq_true, ↓reduceIte]
      exact ⟨back xs i, by omega, bv, by omega⟩
    · simp only [↓reduceIte]
      obtain ⟨e, he, hv_e, hu⟩ := vwc_store _ (bytes_drop hb (back xs i))
      have hle := out2_le (hiS ((if isCont (xs.getD i 0) then out.dropLast else out).getLastD 0))
        (isCont (xs.getD i 0)) out
      refine ⟨back xs i + e, by simp at he; omega, valid_take_add xs _ e bv hv_e, ?_⟩
      rw [List.take_add, List.map_append, L_append, ← hu]
      generalize (if hiS ((if isCont (xs.getD i 0) then out.dropLast else out).getLastD 0) then
        (if isCont (xs.getD i 0) then out.dropLast else out).dropLast
        else (if isCont (xs.getD i 0) then out.dropLast else out)) = o2 at hle ⊢
      rw [Nat.max_le]
      constructor <;> omega
  · simp only [Bool.not_eq_true] at hp
    simp only [hp, Bool.not_false, Bool.and_self, ↓reduceIte]
    exact ⟨0, Nat.zero_le _, rfl, Nat.zero_le _⟩

/-! # 6. The contracts -/

/-- **`ReplacementRest.VSpec2` for the rvv `validate_utf8_with_counts`**, for
every `vl`: it is the scalar one. -/
theorem rvvV_spec2 (pick : Nat → Nat) : ReplacementRest.VSpec2 (rvvV pick) :=
  ReplacementRest.scalarV_spec2

theorem conv_transfer (validate : Bool) (pick : Nat → Nat) (M1 : Nat) (xs : List Nat) (hb : Bytes xs) :
    rvvConvS sconv validate pick M1 xs = rvvConvS vwc validate pick M1 xs := by
  have hS : ∀ n, sconv (xs.drop n) = vwc (xs.drop n) := fun n => sconv_eq _ (bytes_drop hb n)
  unfold rvvConvS prologueS
  rw [sconv_eq xs hb, sconv_eq _ (bytes_take hb _)]
  simp only [hS]

theorem k_transfer (pick : Nat → Nat) (M1 : Nat) (win : List Nat) (hb : Bytes win) :
    rvvKS sconv pick M1 win = rvvKS vwc pick M1 win := by
  unfold rvvKS
  rw [conv_transfer true pick M1 win hb, sconv_eq win hb]

/-- **`ReplacementRest.CVSpec` for the rvv `convert_valid_utf8_to_utf16{le,be}`**,
for every `vl` (every `pick`) and every `M1`. -/
theorem rvvCV_cvspec (pick : Nat → Nat) (M1 : Nat) : ReplacementRest.CVSpec (rvvCV pick M1) := by
  intro v hv
  have hb := LengthsSpec.valid8_bytes v 0 hv
  unfold rvvCV
  rw [conv_transfer false pick M1 v hb]
  exact conv_valid false pick M1 v hb hv (fun _ => ⟨rfl, kLoop_novalid pick M1 v _ _ _⟩)

theorem rvvKW_bspec (pick : Nat → Nat) (M1 : Nat) : ReplacementRest.BSpec (rvvKS vwc pick M1) := by
  intro win hb _
  obtain ⟨c1, hc1, hv1, hs1⟩ := conv_store pick M1 win hb
  obtain ⟨e, he, hv_e, hu⟩ := vwc_store win hb
  obtain ⟨s1, s2⟩ := vwcF_spec win.length win hb (Nat.le_refl _)
  unfold rvvKS
  by_cases h : (rvvConvS vwc true pick M1 win).1 ≠ 0
  · simp only [h, ne_eq, not_false_eq_true, ↓reduceIte]
    obtain ⟨hv, hrun⟩ := conv_sound pick M1 win hb h
    refine ⟨fun _ => ⟨hv, (conv_valid true pick M1 win hb hv hrun).1⟩, fun hf => (by cases hf),
      ⟨c1, hc1, hv1, by omega⟩⟩
  · simp only [h, ↓reduceIte]
    refine ⟨fun hok => ?_, fun hok _ _ => ?_, ?_⟩
    · obtain ⟨hv, hl⟩ := s1 hok
      simp only [hok, ↓reduceIte]
      exact ⟨hv, hl⟩
    · obtain ⟨-, hvp, -, hd⟩ := s2 hok
      simp only [hok, Bool.false_eq_true, ↓reduceIte]
      exact ⟨hvp, hd⟩
    · by_cases hm : (rvvConvS vwc true pick M1 win).2 ≤ (vwc win).2.2
      · exact ⟨e, he, hv_e, by rw [Nat.max_eq_right hm, hu]; omega⟩
      · exact ⟨c1, hc1, hv1, by rw [Nat.max_eq_left (by omega)]; omega⟩

/-- **`ReplacementRest.BSpec` for the rvv `convert_utf8_to_utf16{le,be}_with_errors`**,
for every `vl` (every `pick`) and every `M1`: success means a valid window
and its exact UTF-16 length (`conv_sound`, `conv_valid`, or the scalar
fallback); an error is the scalar fallback's, after a valid prefix where
`dchar` fails; every store, of both calls, ends at most 2 units past `L` of a
valid prefix (`conv_store`, `vwc_store`). -/
theorem rvvK_bspec (pick : Nat → Nat) (M1 : Nat) : ReplacementRest.BSpec (rvvK pick M1) := by
  intro win hb hne
  have := rvvKW_bspec pick M1 win hb hne
  have e : rvvK pick M1 win = rvvKS vwc pick M1 win := k_transfer pick M1 win hb
  rw [e]
  exact this

/-! # 7. UTF-8 → UTF-16 with replacement on rvv, end to end -/

/-- The rvv `utf16_length_from_utf8` (`LengthsRvv`, `vl` from `pickL`). -/
def rvvUL (pickL : Nat → Nat) (v : List Nat) : Nat := LengthsRvv.rvvSum pickL LengthsRvv.rvv16of8 v.length v

/-- **`convert_utf8_to_utf16{le,be}_with_replacement` on rvv**
(src/utf8_to_utf16_with_replacement.inl.h:278-323 with the rvv kernels), from the entry point, on every
input, for every `vl` each `vsetvl` returns (`pick` for the conversion
kernels, `pickL` for the length) and every `M1`: it returns
`utf16_length_from_utf8_with_replacement` (the rvv `validate_utf8_with_counts`
is the scalar one) and every store ends inside it. No hypothesis. -/
theorem convertRepl_rvv (pick pickL : Nat → Nat) (M1 : Nat) (xs : List Nat) (hb : Bytes xs) :
    (ReplacementRest.convertRepl (rvvK pick M1) (rvvV pick) (rvvCV pick M1) (rvvUL pickL) xs).1 =
      lenRepl (rvvV pick) xs ∧
    (ReplacementRest.convertRepl (rvvK pick M1) (rvvV pick) (rvvCV pick M1) (rvvUL pickL) xs).2 ≤
      lenRepl (rvvV pick) xs := by
  rw [show lenRepl (rvvV pick) xs = Replacement.count xs from lenRepl_scalar xs hb]
  exact ReplacementRest.convertRepl_safe _ _ _ _ (rvvK_bspec pick M1) (rvvV_spec2 pick)
    (rvvCV_cvspec pick M1) (LengthsSpec.ULSpec_rvv pickL) xs hb

/-- **`transcode_utf8_to_utf16_with_replacement` on rvv**: returns `count xs`
and every store ends inside it. No hypothesis. -/
theorem trRun_rvv (pick pickL : Nat → Nat) (M1 : Nat) (xs : List Nat) (hb : Bytes xs) :
    ∃ w h, ReplacementRest.trRun (rvvK pick M1) (rvvV pick) (rvvCV pick M1) (rvvUL pickL) xs =
        some (w, h) ∧ w = Replacement.count xs ∧ h ≤ Replacement.count xs :=
  ReplacementRest.trRun_safe _ _ _ _ (rvvK_bspec pick M1) (rvvV_spec2 pick)
    (rvvCV_cvspec pick M1) (LengthsSpec.ULSpec_rvv pickL) xs hb

/-! ## The model runs

36 ASCII bytes and U+1F600: one block of 37 bytes (`vl = n`) or 37 blocks of
one byte; the 4-byte path writes the pair, the reparse step takes both units
back (`src[0]` is the continuation byte `0x9F`, then `dst[-1]` is the high
surrogate), and the scalar tail writes them again: 38 units. A
3-byte, a 4-byte and a 2-byte character with `vl = 5` and `M1 = 2`. A
surrogate (`ED A0 80`) fails the check, and the scalar fallback reports it at
byte 36; a stray continuation byte after a 4-byte character is reported at
byte 37, with the stores ending at the 35 units of the valid prefix. -/

example : rvvConv true id 16 (List.replicate 36 0x41 ++ [0xF0, 0x9F, 0x98, 0x80]) = (38, 38) := by
  decide
example : rvvConv true (fun _ => 1) 16 (List.replicate 36 0x41 ++ [0xF0, 0x9F, 0x98, 0x80]) =
    (38, 38) := by decide
example : rvvConv true (fun _ => 5) 2
    (List.replicate 30 0x41 ++ [0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80, 0x41, 0xC3, 0xA9]) =
    (35, 35) := by decide
example : rvvCV (fun _ => 5) 2
    (List.replicate 30 0x41 ++ [0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80, 0x41, 0xC3, 0xA9]) =
    (35, 35) := by decide
example : (fun r : ReplacementRest.BRes => (r.ok, r.verr, r.cnt, r.se))
    (rvvK id 16 (List.replicate 36 0x41 ++ [0xED, 0xA0, 0x80, 0x41])) = (false, true, 36, 36) := by
  decide
example : (fun r : ReplacementRest.BRes => (r.ok, r.verr, r.cnt, r.se))
    (rvvK id 16 (List.replicate 33 0x41 ++ [0xF0, 0x9F, 0x98, 0x80, 0x80, 0x41, 0x41])) =
    (false, true, 37, 35) := by decide

end SafetyMargins.RvvReplacement
