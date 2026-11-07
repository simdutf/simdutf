import SafetyMargins.Replacement
import SafetyMargins.ValidExact
/-
The hypotheses of `Replacement.lean` read from the C++, as theorems.

* **B3** (`subpart_of_none`): `maximal_subpart`, transcribed (`msub`), restarts
  the decoder: at a byte where the validator fails, `count` is one U+FFFD plus
  `count` after the `maximal_subpart` bytes (`Replacement.Subpart`).
* **B1** (`lenRepl_eq`, `lenRepl_scalar`): the loop of
  `utf16_length_from_utf8_with_replacement` (`lenReplF`, both the scalar
  header and src/utf8_to_utf16_with_replacement.inl.h) computes `count` of the
  input, for the scalar `validate_with_counts` (transcribed, `vwcF`) and for
  any validator meeting `VSpec`.
* **B2** (`validRun_of_valid8`, `lreach_gap`, `lreach_rreach`): valid UTF-8
  (`ValidExact.Valid8`) is a `ValidRun` with `L` of its `w16` units; the gaps
  `convert_using_locations` hands to `convert_valid_prefix` are valid UTF-8,
  and its states are `RReach` states.
* **A3** (`bytemask_eq`, `armLen_eq`): the generic (westmere, haswell) and
  arm64 vector `utf8_length_from_utf16_with_replacement`, transcribed at the
  level of lanes (16-bit lanes with wraparound and the flush every 32767
  iterations; arm64's 8-bit lanes), equal `countRepl` on every input. Ice
  Lake: the identity its algorithm relies on (`icelake_identity`).
* **A1** (`sc8_spec`, `via_scalar`, `compose_tail`): the scalar
  `convert_with_errors` reports the first unpaired surrogate with the
  standard length of the prefix; a vector part followed by it does too, given
  what the vector part converted.

Still read from the C++: that the vector `validate_utf8_with_counts` meets
`VSpec` (B1, and B2 for `convert_using_locations`); that the bulk
`_with_errors` kernel of `transcode_utf8_to_utf16_with_replacement` commits,
on success or before an error, a valid prefix with `L` of its `w16` units, and
reports its error where `dchar` fails (B2, B3 for that path; the
validator's completeness is `ValidExact.checkedAt_of_valid`, its soundness is
not proved); the Ice Lake tiling of units and pairs (A3); what the vector
UTF-16 → UTF-8 parts convert before they stop (A1, hypotheses of
`compose_tail`).

`decide +kernel` is used for finite exhaustive checks only: `masks` (256
bytes), `lanes` and `armLanes` (65536 units).
-/
namespace SafetyMargins.ReplacementMore
open SafetyMargins SafetyMargins.Replacement
open SafetyMargins.Utf8 (w16)
set_option autoImplicit false
set_option linter.unusedSimpArgs false

/-! # B3: `maximal_subpart` and the decoder

The scalar validator (`vchar`), the decoder's acceptance of a character
(`dchar`), the decoder of `Replacement` (`run`, `count`) and
`maximal_subpart` (`msub`). -/

/-- Every element is a byte (the C++ reads `uint8_t`). -/
def Bytes (bs : List Nat) : Prop := ∀ b ∈ bs, b < 256

/-- The class tests of `validate_with_counts` and `maximal_subpart` on a byte
(`(b & 0xC0) == 0x80`, ...), checked on all 256 bytes (`decide +kernel`, a
finite exhaustive check). -/
theorem masks : ∀ b, b < 256 →
    ((b &&& 0xC0 == 0x80) = decide (0x80 ≤ b ∧ b < 0xC0)) ∧
    ((b &&& 0xE0 == 0xC0) = decide (0xC0 ≤ b ∧ b < 0xE0)) ∧
    ((b &&& 0xF0 == 0xE0) = decide (0xE0 ≤ b ∧ b < 0xF0)) ∧
    ((b &&& 0xF8 == 0xF0) = decide (0xF0 ≤ b ∧ b < 0xF8)) := by decide +kernel

/-- Code point of a 2-byte sequence as include/simdutf/scalar/utf8.h:308 computes
it (`x & (2^k - 1)` is `x % 2^k`; `<<` and `|` of disjoint fields are `*` and
`+`). -/
def cp2 (b b1 : Nat) : Nat := (b % 32) * 64 + b1 % 64
/-- Code point of a 3-byte sequence, include/simdutf/scalar/utf8.h:328-330. -/
def cp3 (b b1 b2 : Nat) : Nat := (b % 16) * 4096 + (b1 % 64) * 64 + b2 % 64
/-- Code point of a 4-byte sequence, include/simdutf/scalar/utf8.h:357-359. -/
def cp4 (b b1 b2 b3 : Nat) : Nat :=
  (b % 8) * 262144 + (b1 % 64) * 4096 + (b2 % 64) * 64 + b3 % 64

/-- One iteration of the loop of the scalar `validate_with_counts`
(include/simdutf/scalar/utf8.h:256-380) at the head of `ys`: `some (bytes,
units)` for a character it accepts (`units` is its share of
`utf16_length()`, include/simdutf/error.h:144-146: bytes minus continuation
bytes plus four-byte leads), `none` where it returns an error (at the head).
The ASCII loop (include/simdutf/scalar/utf8.h:290-295) and the 16-byte
ASCII probe (:271-285) accept
ASCII bytes one character each, as here. -/
def vchar : List Nat → Option (Nat × Nat)
  | [] => none
  | b :: r =>
    if b < 0x80 then some (1, 1)
    else if b &&& 0xE0 == 0xC0 then
      match r with
      | b1 :: _ =>
        if !(b1 &&& 0xC0 == 0x80) then none
        else if cp2 b b1 < 0x80 then none
        else some (2, 1)
      | [] => none
    else if b &&& 0xF0 == 0xE0 then
      match r with
      | b1 :: b2 :: _ =>
        if !(b1 &&& 0xC0 == 0x80) then none
        else if !(b2 &&& 0xC0 == 0x80) then none
        else if cp3 b b1 b2 < 0x800 then none
        else if 0xD7FF < cp3 b b1 b2 ∧
            cp3 b b1 b2 < 0xE000 then none
        else some (3, 1)
      | _ => none
    else if b &&& 0xF8 == 0xF0 then
      match r with
      | b1 :: b2 :: b3 :: _ =>
        if !(b1 &&& 0xC0 == 0x80) then none
        else if !(b2 &&& 0xC0 == 0x80) then none
        else if !(b3 &&& 0xC0 == 0x80) then none
        else if cp4 b b1 b2 b3 ≤ 0xFFFF then none
        else if 0x10FFFF < cp4 b b1 b2 b3
          then none
        else some (4, 2)
      | _ => none
    else none

/-- The same acceptance in the terms of the decoder's `lo`/`hi` bounds (`lead`
of `Replacement`, include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:129-160). -/
def dchar : List Nat → Option (Nat × Nat)
  | [] => none
  | b :: r =>
    if b < 0x80 then some (1, 1)
    else if b < 0xC2 then none
    else if b < 0xE0 then
      match r with
      | b1 :: _ => if 0x80 ≤ b1 ∧ b1 ≤ 0xBF then some (2, 1) else none
      | [] => none
    else if b < 0xF0 then
      match r with
      | b1 :: b2 :: _ =>
        if (if b = 0xE0 then 0xA0 else 0x80) ≤ b1 ∧ b1 ≤ (if b = 0xED then 0x9F else 0xBF) ∧
          0x80 ≤ b2 ∧ b2 ≤ 0xBF then some (3, 1) else none
      | _ => none
    else if b < 0xF5 then
      match r with
      | b1 :: b2 :: b3 :: _ =>
        if (if b = 0xF0 then 0x90 else 0x80) ≤ b1 ∧ b1 ≤ (if b = 0xF4 then 0x8F else 0xBF) ∧
          0x80 ≤ b2 ∧ b2 ≤ 0xBF ∧ 0x80 ≤ b3 ∧ b3 ≤ 0xBF then some (4, 2) else none
      | _ => none
    else none

/-- On bytes, the validator and the decoder accept the same characters. -/
theorem vchar_eq_dchar (ys : List Nat) (h : Bytes ys) : vchar ys = dchar ys := by
  rcases ys with _ | ⟨b, r⟩
  · rfl
  have hb := h b (by simp)
  obtain ⟨_, m2, m3, m4⟩ := masks b hb
  rcases r with _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩
  · simp only [vchar, dchar, m2, m3, m4]
    simp only [Bool.not_eq_true', decide_eq_true_eq, decide_eq_false_iff_not]
    repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
  · obtain ⟨c1, -⟩ := masks b1 (h b1 (by simp only [List.mem_cons, true_or, or_true]))
    simp only [vchar, dchar, m2, m3, m4, c1]
    simp only [Bool.not_eq_true', decide_eq_true_eq, decide_eq_false_iff_not]
    repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
  · obtain ⟨c1, -⟩ := masks b1 (h b1 (by simp only [List.mem_cons, true_or, or_true]))
    obtain ⟨c2, -⟩ := masks b2 (h b2 (by simp only [List.mem_cons, true_or, or_true]))
    simp only [vchar, dchar, m2, m3, m4, c1, c2]
    simp only [Bool.not_eq_true', decide_eq_true_eq, decide_eq_false_iff_not]
    repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
  · obtain ⟨c1, -⟩ := masks b1 (h b1 (by simp only [List.mem_cons, true_or, or_true]))
    obtain ⟨c2, -⟩ := masks b2 (h b2 (by simp only [List.mem_cons, true_or, or_true]))
    obtain ⟨c3, -⟩ := masks b3 (h b3 (by simp only [List.mem_cons, true_or, or_true]))
    simp only [vchar, dchar, m2, m3, m4, c1, c2, c3]
    simp only [Bool.not_eq_true', decide_eq_true_eq, decide_eq_false_iff_not]
    clear m2 m3 m4 c1 c2 c3 h
    by_cases a1 : b < 0x80
    · simp only [a1, ite_true]
    simp only [a1, ite_false]
    by_cases a2 : 0xC0 ≤ b ∧ b < 0xE0
    · simp only [a2, ite_true, and_self, show b < 0xE0 by omega]
      repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
    simp only [a2, ite_false]
    by_cases a3 : 0xE0 ≤ b ∧ b < 0xF0
    · simp only [a3, ite_true, and_self, show ¬ b < 0xC2 by omega, show ¬ b < 0xE0 by omega,
        show b < 0xF0 by omega, ite_false]
      repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
    simp only [a3, ite_false]
    by_cases a4 : 0xF0 ≤ b ∧ b < 0xF8
    · simp only [a4, ite_true, and_self, show ¬ b < 0xC2 by omega, show ¬ b < 0xE0 by omega,
        show ¬ b < 0xF0 by omega, ite_false]
      by_cases a5 : b < 0xF5
      · simp only [a5, ite_true]
        repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
      · simp only [a5, ite_false]
        repeat' (first | rfl | omega | (simp only [cp2, cp3, cp4] at *; omega) | split)
    simp only [a4, ite_false]
    repeat' (first | rfl | omega | split)


/-- The final U+FFFD of a pending sequence (include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:184-186). -/
def pend (s : St) : Nat := if s.needed = 0 then 0 else 1

/-- `count` from any decoder state. -/
def cnt (s : St) (ys : List Nat) : Nat := (run s ys).2 + pend (run s ys).1

theorem count_eq_cnt (ys : List Nat) : count ys = cnt init ys := rfl

theorem cnt_cons (s : St) (b : Nat) (r : List Nat) :
    cnt s (b :: r) = (step s b).2 + cnt (step s b).1 r := by
  simp only [cnt, run]; omega

theorem cnt_nil (s : St) : cnt s [] = pend s := by simp [cnt, run]

theorem step_init (b : Nat) : step init b = lead b := rfl

/-- A byte outside `[lo, hi]` in a pending sequence: one U+FFFD, then the byte
is processed from the initial state (include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:163-171,
`continue` without `i++`). -/
theorem cnt_bad (s : St) (x : Nat) (r : List Nat) (hn : s.needed ≠ 0)
    (hx : x < s.lo ∨ s.hi < x) : cnt s (x :: r) = 1 + cnt init (x :: r) := by
  rw [cnt_cons, cnt_cons, step_init]
  have : step s x = ((lead x).1, 1 + (lead x).2) := by
    simp only [step, hn, ite_false, hx, ite_true]
  rw [this]; simp only; omega

theorem lead_one (b : Nat) (h : b < 0xC2 ∨ 0xF5 ≤ b) : lead b = (init, 1) := by
  unfold lead; repeat' (first | rfl | omega | split)

theorem lead2 (b : Nat) (h1 : 0xC2 ≤ b) (h2 : b < 0xE0) :
    lead b = (⟨1, 0, b % 32, 0x80, 0xBF⟩, 0) := by
  unfold lead; repeat' (first | rfl | omega | split)

theorem lead3 (b : Nat) (h1 : 0xE0 ≤ b) (h2 : b < 0xF0) :
    lead b = (⟨2, 0, b % 16, if b = 0xE0 then 0xA0 else 0x80,
      if b = 0xED then 0x9F else 0xBF⟩, 0) := by
  unfold lead; repeat' (first | rfl | omega | split)

theorem lead4 (b : Nat) (h1 : 0xF0 ≤ b) (h2 : b < 0xF5) :
    lead b = (⟨3, 0, b % 8, if b = 0xF0 then 0x90 else 0x80,
      if b = 0xF4 then 0x8F else 0xBF⟩, 0) := by
  unfold lead; repeat' (first | rfl | omega | split)

theorem step_mid (s : St) (x : Nat) (hn : s.needed ≠ 0) (hlo : s.lo ≤ x) (hhi : x ≤ s.hi)
    (hc : s.seen + 1 ≠ s.needed) :
    step s x = (⟨s.needed, s.seen + 1, s.cp * 64 + x % 64, 0x80, 0xBF⟩, 0) := by
  simp only [step, hn, ite_false, hc, show ¬ (x < s.lo ∨ s.hi < x) by omega]

theorem step_fin (s : St) (x : Nat) (hn : s.needed ≠ 0) (hlo : s.lo ≤ x) (hhi : x ≤ s.hi)
    (hc : s.seen + 1 = s.needed) : step s x = (init, units (s.cp * 64 + x % 64)) := by
  simp only [step, hn, ite_false, hc, ite_true, show ¬ (x < s.lo ∨ s.hi < x) by omega]

theorem units_one (c : Nat) (h : c < 0x10000) : units c = 1 := by
  unfold units; split <;> omega
theorem units_two (c : Nat) (h : 0x10000 ≤ c) : units c = 2 := by
  unfold units; split <;> omega

theorem run2 (b b1 : Nat) (h1 : 0xC2 ≤ b) (h2 : b < 0xE0) (c1 : 0x80 ≤ b1 ∧ b1 ≤ 0xBF) :
    run init [b, b1] = (init, 1) := by
  have e2 := step_fin ⟨1, 0, b % 32, 0x80, 0xBF⟩ b1 (by simp) c1.1 c1.2 rfl
  simp only [run, step_init, lead2 b h1 h2, e2, units_one _ (by omega : (b % 32) * 64 + b1 % 64 < 0x10000)]

theorem run3 (b b1 b2 : Nat) (h1 : 0xE0 ≤ b) (h2 : b < 0xF0)
    (hlo : (if b = 0xE0 then 0xA0 else 0x80) ≤ b1) (hhi : b1 ≤ (if b = 0xED then 0x9F else 0xBF))
    (c2 : 0x80 ≤ b2 ∧ b2 ≤ 0xBF) : run init [b, b1, b2] = (init, 1) := by
  have e2 := step_mid ⟨2, 0, b % 16, if b = 0xE0 then 0xA0 else 0x80,
    if b = 0xED then 0x9F else 0xBF⟩ b1 (by simp) hlo hhi (by simp)
  have e3 := step_fin ⟨2, 0 + 1, b % 16 * 64 + b1 % 64, 0x80, 0xBF⟩ b2 (by simp) c2.1 c2.2 rfl
  simp only [run, step_init, lead3 b h1 h2, e2, e3,
    units_one _ (by omega : (b % 16 * 64 + b1 % 64) * 64 + b2 % 64 < 0x10000)]

theorem run4 (b b1 b2 b3 : Nat) (h1 : 0xF0 ≤ b) (h2 : b < 0xF5)
    (hlo : (if b = 0xF0 then 0x90 else 0x80) ≤ b1) (hhi : b1 ≤ (if b = 0xF4 then 0x8F else 0xBF))
    (c2 : 0x80 ≤ b2 ∧ b2 ≤ 0xBF) (c3 : 0x80 ≤ b3 ∧ b3 ≤ 0xBF) :
    run init [b, b1, b2, b3] = (init, 2) := by
  have e2 := step_mid ⟨3, 0, b % 8, if b = 0xF0 then 0x90 else 0x80,
    if b = 0xF4 then 0x8F else 0xBF⟩ b1 (by simp) hlo hhi (by simp)
  have e3 := step_mid ⟨3, 0 + 1, b % 8 * 64 + b1 % 64, 0x80, 0xBF⟩ b2 (by simp) c2.1 c2.2
    (by simp)
  have e4 := step_fin ⟨3, 0 + 1 + 1, (b % 8 * 64 + b1 % 64) * 64 + b2 % 64, 0x80, 0xBF⟩ b3
    (by simp) c3.1 c3.2 rfl
  have hge : 0x10000 ≤ ((b % 8 * 64 + b1 % 64) * 64 + b2 % 64) * 64 + b3 % 64 := by
    by_cases hb : b = 0xF0
    · subst hb; simp at hlo hhi; omega
    · omega
  simp only [run, step_init, lead4 b h1 h2, e2, e3, e4, units_two _ hge]

/-- A character `dchar` accepts takes the decoder from its initial state back to
it, with the same units. -/
theorem dchar_run (ys : List Nat) (k u : Nat) (h : dchar ys = some (k, u)) :
    run init (ys.take k) = (init, u) ∧ 1 ≤ k ∧ k ≤ ys.length := by
  rcases ys with _ | ⟨b, r⟩
  · simp [dchar] at h
  rcases r with _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩ <;> simp only [dchar] at h <;>
    repeat' (first | (simp only [reduceCtorEq] at h) | split at h)
  all_goals
    simp only [Option.some.injEq, Prod.mk.injEq] at h
    obtain ⟨rfl, rfl⟩ := h
    refine ⟨?_, by simp, by simp⟩
    simp only [List.take]
  all_goals
    first
      | omega
      | (simp [run, step_init, lead_one b (by omega)])
      | rw [run2 b b1 (by omega) (by omega) ⟨by omega, by omega⟩]
      | rw [run3 b b1 b2 (by omega) (by omega) (by split <;> omega) (by split <;> omega)
          ⟨by omega, by omega⟩]
      | rw [run4 b b1 b2 b3 (by omega) (by omega) (by split <;> omega) (by split <;> omega)
          ⟨by omega, by omega⟩ ⟨by omega, by omega⟩]


/-- The `while` of `maximal_subpart` (include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:63-67):
continuation bytes (`(x & 0xC0) == 0x80`), at most `k` of them. -/
def contRun : Nat → List Nat → Nat
  | 0, _ => 0
  | _ + 1, [] => 0
  | k + 1, x :: r => if x &&& 0xC0 == 0x80 then 1 + contRun k r else 0

/-- `maximal_subpart` after the lead (include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:59-68): `n < 2`
or the second byte outside `[lo, hi]` gives 1, else 2 plus the continuation
bytes. -/
def msubTail (need lo hi : Nat) : List Nat → Nat
  | [] => 1
  | b1 :: r => if b1 < lo ∨ hi < b1 then 1 else 2 + contRun (need - 1) r

/-- **`maximal_subpart`** (include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:27-69), transcribed: the
lead classes and `lo`/`hi` of :38-58, then `msubTail`. -/
def msub : List Nat → Nat
  | [] => 1
  | b :: r =>
    if b < 0xC2 then 1
    else if b < 0xE0 then msubTail 1 0x80 0xBF r
    else if b < 0xF0 then
      msubTail 2 (if b = 0xE0 then 0xA0 else 0x80) (if b = 0xED then 0x9F else 0xBF) r
    else if b < 0xF5 then
      msubTail 3 (if b = 0xF0 then 0x90 else 0x80) (if b = 0xF4 then 0x8F else 0xBF) r
    else 1

theorem contRun_noncont (k x : Nat) (r : List Nat) (hx : x < 256) (hc : ¬ (0x80 ≤ x ∧ x ≤ 0xBF)) :
    contRun (k + 1) (x :: r) = 0 := by
  obtain ⟨m, -⟩ := masks x hx
  simp only [contRun, m]
  rw [ite_eq_right (by simp; omega)]

theorem contRun_cont (k x : Nat) (r : List Nat) (hx : x < 256) (hc : 0x80 ≤ x ∧ x ≤ 0xBF) :
    contRun (k + 1) (x :: r) = 1 + contRun k r := by
  obtain ⟨m, -⟩ := masks x hx
  simp only [contRun, m]
  rw [ite_eq_left (by simp; omega)]

theorem count_cons (b : Nat) (r : List Nat) : count (b :: r) = (lead b).2 + cnt (lead b).1 r := by
  rw [count_eq_cnt, cnt_cons, step_init]

theorem pend_ne (s : St) (h : s.needed ≠ 0) : cnt s [] = 1 := by
  simp [cnt_nil, pend, h]

theorem count_nil : count [] = 0 := rfl

/-- **B3.** At a byte where `dchar` (equivalently the scalar validator,
`vchar_eq_dchar`) reports an error, `transcode_with_replacement` stores one
U+FFFD for the `maximal_subpart` bytes and then behaves as from its initial
state: `Replacement.Subpart ys (msub ys)`, with `1 ≤ msub ys ≤ ys.length`. -/
theorem subpart_of_none (ys : List Nat) (hb : Bytes ys) (hne : ys ≠ []) (h : dchar ys = none) :
    count ys = 1 + count (ys.drop (msub ys)) ∧ 1 ≤ msub ys ∧ msub ys ≤ ys.length := by
  rcases ys with _ | ⟨b, r⟩
  · exact absurd rfl hne
  have hb0 : b < 256 := hb b (by simp)
  by_cases a1 : b < 0xC2 ∨ 0xF5 ≤ b
  · have hm : msub (b :: r) = 1 := by
      simp only [msub]; repeat' (first | rfl | omega | split)
    rw [hm, count_cons, lead_one b a1]
    simp [count_eq_cnt]
  by_cases a2 : b < 0xE0
  · have hl := lead2 b (by omega) a2
    rcases r with _ | ⟨b1, r⟩
    · have hm : msub [b] = 1 := by
        simp only [msub, msubTail, ite_eq_right (show ¬ b < 0xC2 by omega), ite_eq_left a2]
      rw [hm, count_cons, hl, pend_ne _ (by simp)]; simp [count_nil]
    · have hc : ¬ (0x80 ≤ b1 ∧ b1 ≤ 0xBF) := by
        intro hc; simp only [dchar, ite_eq_right (show ¬ b < 0x80 by omega),
          ite_eq_right (show ¬ b < 0xC2 by omega), ite_eq_left a2, ite_eq_left hc] at h; cases h
      have hm : msub (b :: b1 :: r) = 1 := by
        simp only [msub, msubTail, ite_eq_right (show ¬ b < 0xC2 by omega), ite_eq_left a2]
        rw [ite_eq_left (by omega)]
      rw [hm, count_cons, hl, cnt_bad _ _ _ (by simp) (by simp; omega)]
      simp [count_eq_cnt]
  have hb1 : ∀ x ∈ r, x < 256 := fun x hx => hb x (by simp [hx])
  by_cases a3 : b < 0xF0
  · have hl := lead3 b (by omega) a3
    have hlo : 0x80 ≤ (if b = 0xE0 then 0xA0 else 0x80) := by split <;> omega
    have hhi : (if b = 0xED then 0x9F else 0xBF) ≤ 0xBF := by split <;> omega
    have hm0 : msub (b :: r) = msubTail 2 (if b = 0xE0 then 0xA0 else 0x80)
        (if b = 0xED then 0x9F else 0xBF) r := by
      simp only [msub, ite_eq_right (show ¬ b < 0xC2 by omega),
        ite_eq_right (show ¬ b < 0xE0 by omega), ite_eq_left a3]
    have hd : ∀ b1 b2 r', r = b1 :: b2 :: r' →
        ¬ ((if b = 0xE0 then 0xA0 else 0x80) ≤ b1 ∧ b1 ≤ (if b = 0xED then 0x9F else 0xBF) ∧
          0x80 ≤ b2 ∧ b2 ≤ 0xBF) := by
      intro b1 b2 r' hr hc
      subst hr
      simp only [dchar, ite_eq_right (show ¬ b < 0x80 by omega),
        ite_eq_right (show ¬ b < 0xC2 by omega), ite_eq_right (show ¬ b < 0xE0 by omega),
        ite_eq_left a3, ite_eq_left hc] at h
      cases h
    rw [hm0, count_cons, hl]
    clear hm0 hl h
    generalize (if b = 0xE0 then 0xA0 else 0x80) = lo at *
    generalize (if b = 0xED then 0x9F else 0xBF) = hi at *
    rcases r with _ | ⟨b1, r⟩
    · simp [msubTail, pend_ne, count_nil]
    by_cases i1 : lo ≤ b1 ∧ b1 ≤ hi
    · have e2 := step_mid ⟨2, 0, b % 16, lo, hi⟩ b1 (by simp) i1.1 i1.2 (by simp)
      rcases r with _ | ⟨b2, r⟩
      · simp only [msubTail, ite_eq_right (show ¬ (b1 < lo ∨ hi < b1) by omega), contRun]
        rw [cnt_cons, e2]
        simp [cnt_nil, pend, count_nil]
      · have h2 : ¬ (0x80 ≤ b2 ∧ b2 ≤ 0xBF) := fun h2 => hd b1 b2 r rfl ⟨i1.1, i1.2, h2⟩
        simp only [msubTail, ite_eq_right (show ¬ (b1 < lo ∨ hi < b1) by omega),
          show 2 - 1 = 0 + 1 from rfl, contRun_noncont 0 b2 r (hb1 b2 (by simp)) h2]
        rw [cnt_cons, e2]
        dsimp only
        rw [cnt_bad _ _ _ (by simp) (by simp; omega)]
        simp [count_eq_cnt]
    · simp only [msubTail, ite_eq_left (show b1 < lo ∨ hi < b1 by omega)]
      rw [cnt_bad _ _ _ (by simp) (by simp; omega)]
      simp [count_eq_cnt]
  have a4 : b < 0xF5 := by omega
  have hl := lead4 b (by omega) a4
  have hlo : 0x80 ≤ (if b = 0xF0 then 0x90 else 0x80) := by split <;> omega
  have hhi : (if b = 0xF4 then 0x8F else 0xBF) ≤ 0xBF := by split <;> omega
  have hm0 : msub (b :: r) = msubTail 3 (if b = 0xF0 then 0x90 else 0x80)
      (if b = 0xF4 then 0x8F else 0xBF) r := by
    simp only [msub, ite_eq_right (show ¬ b < 0xC2 by omega),
      ite_eq_right (show ¬ b < 0xE0 by omega), ite_eq_right a3, ite_eq_left a4]
  have hd : ∀ b1 b2 b3 r', r = b1 :: b2 :: b3 :: r' →
      ¬ ((if b = 0xF0 then 0x90 else 0x80) ≤ b1 ∧ b1 ≤ (if b = 0xF4 then 0x8F else 0xBF) ∧
        0x80 ≤ b2 ∧ b2 ≤ 0xBF ∧ 0x80 ≤ b3 ∧ b3 ≤ 0xBF) := by
    intro b1 b2 b3 r' hr hc
    subst hr
    simp only [dchar, ite_eq_right (show ¬ b < 0x80 by omega),
      ite_eq_right (show ¬ b < 0xC2 by omega), ite_eq_right (show ¬ b < 0xE0 by omega),
      ite_eq_right a3, ite_eq_left a4, ite_eq_left hc] at h
    cases h
  rw [hm0, count_cons, hl]
  clear hm0 hl h
  generalize (if b = 0xF0 then 0x90 else 0x80) = lo at *
  generalize (if b = 0xF4 then 0x8F else 0xBF) = hi at *
  rcases r with _ | ⟨b1, r⟩
  · simp [msubTail, cnt_nil, pend, count_nil]
  by_cases i1 : lo ≤ b1 ∧ b1 ≤ hi
  · have e2 := step_mid ⟨3, 0, b % 8, lo, hi⟩ b1 (by simp) i1.1 i1.2 (by simp)
    rcases r with _ | ⟨b2, r⟩
    · simp only [msubTail, ite_eq_right (show ¬ (b1 < lo ∨ hi < b1) by omega), contRun]
      rw [cnt_cons, e2]
      simp [cnt_nil, pend, count_nil]
    by_cases i2 : 0x80 ≤ b2 ∧ b2 ≤ 0xBF
    · have e3 := step_mid ⟨3, 0 + 1, b % 8 * 64 + b1 % 64, 0x80, 0xBF⟩ b2 (by simp) i2.1 i2.2
        (by simp)
      rcases r with _ | ⟨b3, r⟩
      · simp only [msubTail, ite_eq_right (show ¬ (b1 < lo ∨ hi < b1) by omega),
          show 3 - 1 = 1 + 1 from rfl, contRun_cont 1 b2 [] (hb1 b2 (by simp)) i2, contRun]
        rw [cnt_cons, e2]
        dsimp only
        rw [cnt_cons, e3]
        simp [cnt_nil, pend, count_nil]
      · have h3 : ¬ (0x80 ≤ b3 ∧ b3 ≤ 0xBF) := fun h3 =>
          hd b1 b2 b3 r rfl ⟨i1.1, i1.2, i2.1, i2.2, h3⟩
        simp only [msubTail, ite_eq_right (show ¬ (b1 < lo ∨ hi < b1) by omega),
          show 3 - 1 = 1 + 1 from rfl, contRun_cont 1 b2 _ (hb1 b2 (by simp)) i2,
          contRun_noncont 0 b3 r (hb1 b3 (by simp)) h3]
        rw [cnt_cons, e2]
        dsimp only
        rw [cnt_cons, e3]
        dsimp only
        rw [cnt_bad _ _ _ (by simp) (by simp; omega)]
        simp [count_eq_cnt]
    · simp only [msubTail, ite_eq_right (show ¬ (b1 < lo ∨ hi < b1) by omega),
        show 3 - 1 = 1 + 1 from rfl, contRun_noncont 1 b2 r (hb1 b2 (by simp)) i2]
      rw [cnt_cons, e2]
      dsimp only
      rw [cnt_bad _ _ _ (by simp) (by simp; omega)]
      simp [count_eq_cnt]
  · simp only [msubTail, ite_eq_left (show b1 < lo ∨ hi < b1 by omega)]
    rw [cnt_bad _ _ _ (by simp) (by simp; omega)]
    simp [count_eq_cnt]


/-! # B2: valid UTF-8 is a valid run -/

/-- `utf16_length_from_utf8` weights (`Utf8.w16`) by byte class. -/
theorem w16_val (b : Nat) :
    (0x80 ≤ b ∧ b < 0xC0 → w16 b = 0) ∧ (b < 0x80 ∨ (0xC0 ≤ b ∧ b < 0xF0) → w16 b = 1) ∧
    (0xF0 ≤ b → w16 b = 2) := by
  unfold w16; refine ⟨?_, ?_, ?_⟩ <;> intro h <;> repeat' (first | rfl | omega | split)

theorem ve2 (b b1 : Nat) (h1 : 0xC2 ≤ b) (h2 : b < 0xE0) (c1 : 0x80 ≤ b1 ∧ b1 ≤ 0xBF) :
    ValidExact.run 0 [b, b1] = 0 := by
  show ValidExact.step (ValidExact.step 0 b) b1 = 0
  have e : ValidExact.step 0 b = 1 := by
    unfold ValidExact.step; repeat' (first | rfl | omega | split)
  rw [e, ValidExact.step1, ite_eq_left ⟨c1.1, by omega⟩]

theorem ve3 (b b1 b2 : Nat) (h1 : 0xE0 ≤ b) (h2 : b < 0xF0)
    (hlo : (if b = 0xE0 then 0xA0 else 0x80) ≤ b1) (hhi : b1 ≤ (if b = 0xED then 0x9F else 0xBF))
    (c2 : 0x80 ≤ b2 ∧ b2 ≤ 0xBF) : ValidExact.run 0 [b, b1, b2] = 0 := by
  show ValidExact.step (ValidExact.step (ValidExact.step 0 b) b1) b2 = 0
  have e : ValidExact.step (ValidExact.step 0 b) b1 = 1 := by
    by_cases a : b = 0xE0
    · subst a; simp at hlo hhi
      rw [show ValidExact.step 0 0xE0 = 4 from rfl, ValidExact.step4, ite_eq_left ⟨hlo, by omega⟩]
    by_cases a' : b = 0xED
    · subst a'; simp at hlo hhi
      rw [show ValidExact.step 0 0xED = 5 from rfl, ValidExact.step5, ite_eq_left ⟨hlo, by omega⟩]
    rw [ite_eq_right a] at hlo; rw [ite_eq_right a'] at hhi
    have e0 : ValidExact.step 0 b = 2 := by
      unfold ValidExact.step; repeat' (first | rfl | omega | split)
    rw [e0, ValidExact.step2, ite_eq_left ⟨hlo, by omega⟩]
  rw [e, ValidExact.step1, ite_eq_left ⟨c2.1, by omega⟩]

theorem ve4 (b b1 b2 b3 : Nat) (h1 : 0xF0 ≤ b) (h2 : b < 0xF5)
    (hlo : (if b = 0xF0 then 0x90 else 0x80) ≤ b1) (hhi : b1 ≤ (if b = 0xF4 then 0x8F else 0xBF))
    (c2 : 0x80 ≤ b2 ∧ b2 ≤ 0xBF) (c3 : 0x80 ≤ b3 ∧ b3 ≤ 0xBF) :
    ValidExact.run 0 [b, b1, b2, b3] = 0 := by
  show ValidExact.step (ValidExact.step (ValidExact.step (ValidExact.step 0 b) b1) b2) b3 = 0
  have e : ValidExact.step (ValidExact.step 0 b) b1 = 2 := by
    by_cases a : b = 0xF0
    · subst a; simp at hlo hhi
      rw [show ValidExact.step 0 0xF0 = 6 from rfl, ValidExact.step6, ite_eq_left ⟨hlo, by omega⟩]
    by_cases a' : b = 0xF4
    · subst a'; simp at hlo hhi
      rw [show ValidExact.step 0 0xF4 = 7 from rfl, ValidExact.step7, ite_eq_left ⟨hlo, by omega⟩]
    rw [ite_eq_right a] at hlo; rw [ite_eq_right a'] at hhi
    have e0 : ValidExact.step 0 b = 3 := by
      unfold ValidExact.step; repeat' (first | rfl | omega | split)
    rw [e0, ValidExact.step3, ite_eq_left ⟨hlo, by omega⟩]
  rw [e, ValidExact.step2, ite_eq_left ⟨c2.1, by omega⟩, ValidExact.step1, ite_eq_left ⟨c3.1, by omega⟩]

/-- A character `dchar` accepts is valid UTF-8 (`ValidExact`, Unicode Table
3-7), and its units are `L` of its `w16` weights. -/
theorem dchar_valid8 (ys : List Nat) (k u : Nat) (h : dchar ys = some (k, u)) :
    ValidExact.run 0 (ys.take k) = 0 ∧ u = L ((ys.take k).map w16) := by
  rcases ys with _ | ⟨b, r⟩
  · simp [dchar] at h
  rcases r with _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩ <;> simp only [dchar] at h <;>
    repeat' (first | (simp only [reduceCtorEq] at h) | split at h)
  all_goals
    simp only [Option.some.injEq, Prod.mk.injEq] at h
    obtain ⟨rfl, rfl⟩ := h
    simp only [List.take, List.map, L_cons, L_nil]
  all_goals first
    | omega
    | (refine ⟨?_, ?_⟩
       · first
          | (show ValidExact.step 0 b = 0
             unfold ValidExact.step; repeat' (first | rfl | omega | split))
          | exact ve2 b b1 (by omega) (by omega) ⟨by omega, by omega⟩
          | exact ve3 b b1 b2 (by omega) (by omega) (by split <;> omega) (by split <;> omega)
              ⟨by omega, by omega⟩
          | exact ve4 b b1 b2 b3 (by omega) (by omega) (by split <;> omega) (by split <;> omega)
              ⟨by omega, by omega⟩ ⟨by omega, by omega⟩
       · have := w16_val b
         first
          | have := w16_val b1; have := w16_val b2; have := w16_val b3; omega
          | have := w16_val b1; have := w16_val b2; omega
          | have := w16_val b1; omega
          | omega)


theorem peel (s x c lo hi : Nat) (r : List Nat)
    (hs : ∀ y, ValidExact.step s y = if lo ≤ y ∧ y < hi then c else 8)
    (h : ValidExact.run s (x :: r) = 0) : lo ≤ x ∧ x < hi ∧ ValidExact.run c r = 0 := by
  rw [ValidExact.run_cons, hs] at h
  split at h
  · rename_i hx; exact ⟨hx.1, hx.2, h⟩
  · rw [ValidExact.run_8] at h; cases h

theorem nil_ne (s : Nat) (hs : s ≠ 0) : ValidExact.run s [] ≠ 0 := by
  simp [ValidExact.run]; exact hs

/-- Valid UTF-8 starts with a character `dchar` accepts. -/
theorem dchar_of_valid (b : Nat) (r : List Nat) (h : ValidExact.run 0 (b :: r) = 0) :
    ∃ k u, dchar (b :: r) = some (k, u) ∧ ValidExact.run 0 ((b :: r).drop k) = 0 := by
  rw [ValidExact.run_cons] at h
  rcases ValidExact.step0_cases b with
    ⟨a, e⟩ | ⟨a, a', e⟩ | ⟨a, a', e⟩ | ⟨a, e⟩ | ⟨a, e⟩ | ⟨a, a', e⟩ | ⟨a, e⟩ | ⟨a, a', e⟩ |
    ⟨a, e⟩ | ⟨a, e⟩ <;> rw [e] at h
  · exact ⟨1, 1, by simp [dchar, a], h⟩
  · rw [ValidExact.run_8] at h; cases h
  · rcases r with _ | ⟨x1, r⟩
    · exact absurd h (nil_ne _ (by decide))
    obtain ⟨l1, u1, h⟩ := peel _ _ _ _ _ _ ValidExact.step1 h
    refine ⟨2, 1, ?_, h⟩
    simp only [dchar]; repeat' (first | rfl | omega | split)
  · rcases r with _ | ⟨x1, _ | ⟨x2, r⟩⟩
    · exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step4 h; exact absurd h (nil_ne _ (by decide))
    obtain ⟨l1, u1, h⟩ := peel _ _ _ _ _ _ ValidExact.step4 h
    obtain ⟨l2, u2, h⟩ := peel _ _ _ _ _ _ ValidExact.step1 h
    refine ⟨3, 1, ?_, h⟩
    simp only [dchar]; repeat' (first | rfl | omega | split)
  · rcases r with _ | ⟨x1, _ | ⟨x2, r⟩⟩
    · exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step5 h; exact absurd h (nil_ne _ (by decide))
    obtain ⟨l1, u1, h⟩ := peel _ _ _ _ _ _ ValidExact.step5 h
    obtain ⟨l2, u2, h⟩ := peel _ _ _ _ _ _ ValidExact.step1 h
    refine ⟨3, 1, ?_, h⟩
    simp only [dchar]; repeat' (first | rfl | omega | split)
  · rcases r with _ | ⟨x1, _ | ⟨x2, r⟩⟩
    · exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step2 h; exact absurd h (nil_ne _ (by decide))
    have hne : b ≠ 0xED := fun hb => by subst hb; exact absurd e (by decide)
    obtain ⟨l1, u1, h⟩ := peel _ _ _ _ _ _ ValidExact.step2 h
    obtain ⟨l2, u2, h⟩ := peel _ _ _ _ _ _ ValidExact.step1 h
    refine ⟨3, 1, ?_, h⟩
    simp only [dchar]; repeat' (first | rfl | omega | split)
  · rcases r with _ | ⟨x1, _ | ⟨x2, _ | ⟨x3, r⟩⟩⟩
    · exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step6 h; exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step6 h
      obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step2 h; exact absurd h (nil_ne _ (by decide))
    obtain ⟨l1, u1, h⟩ := peel _ _ _ _ _ _ ValidExact.step6 h
    obtain ⟨l2, u2, h⟩ := peel _ _ _ _ _ _ ValidExact.step2 h
    obtain ⟨l3, u3, h⟩ := peel _ _ _ _ _ _ ValidExact.step1 h
    refine ⟨4, 2, ?_, h⟩
    simp only [dchar]; repeat' (first | rfl | omega | split)
  · rcases r with _ | ⟨x1, _ | ⟨x2, _ | ⟨x3, r⟩⟩⟩
    · exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step3 h; exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step3 h
      obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step2 h; exact absurd h (nil_ne _ (by decide))
    obtain ⟨l1, u1, h⟩ := peel _ _ _ _ _ _ ValidExact.step3 h
    obtain ⟨l2, u2, h⟩ := peel _ _ _ _ _ _ ValidExact.step2 h
    obtain ⟨l3, u3, h⟩ := peel _ _ _ _ _ _ ValidExact.step1 h
    refine ⟨4, 2, ?_, h⟩
    simp only [dchar]; repeat' (first | rfl | omega | split)
  · rcases r with _ | ⟨x1, _ | ⟨x2, _ | ⟨x3, r⟩⟩⟩
    · exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step7 h; exact absurd h (nil_ne _ (by decide))
    · obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step7 h
      obtain ⟨-, -, h⟩ := peel _ _ _ _ _ _ ValidExact.step2 h; exact absurd h (nil_ne _ (by decide))
    obtain ⟨l1, u1, h⟩ := peel _ _ _ _ _ _ ValidExact.step7 h
    obtain ⟨l2, u2, h⟩ := peel _ _ _ _ _ _ ValidExact.step2 h
    obtain ⟨l3, u3, h⟩ := peel _ _ _ _ _ _ ValidExact.step1 h
    refine ⟨4, 2, ?_, h⟩
    simp only [dchar]; repeat' (first | rfl | omega | split)
  · rw [ValidExact.run_8] at h; cases h


theorem L_map_split (ys : List Nat) (k : Nat) :
    L (ys.map w16) = L ((ys.take k).map w16) + L ((ys.drop k).map w16) := by
  rw [← L_append, ← List.map_append, List.take_append_drop]

theorem valid8_run_aux : ∀ n (ys : List Nat), ys.length ≤ n → ValidExact.run 0 ys = 0 →
    run init ys = (init, L (ys.map w16)) := by
  intro n
  induction n with
  | zero =>
    intro ys hl _
    rw [List.eq_nil_of_length_eq_zero (by omega : ys.length = 0)]; rfl
  | succ n ih =>
    intro ys hl hv
    rcases ys with _ | ⟨b, r⟩
    · rfl
    obtain ⟨k, u, hd, hrest⟩ := dchar_of_valid b r hv
    obtain ⟨hr, hk1, hk2⟩ := dchar_run _ k u hd
    obtain ⟨-, hu⟩ := dchar_valid8 _ k u hd
    have ih' := ih ((b :: r).drop k) (by simp at hl ⊢; omega) hrest
    conv => lhs; rw [← List.take_append_drop k (b :: r)]
    rw [run_append, hr]
    simp only [ih', L_map_split (b :: r) k, hu]

/-- **B2, decoder side.** Valid UTF-8 (`ValidExact.Valid8`) is a
`Replacement.ValidRun` with `L` of its `w16` weights units, which is what
`utf16_length_from_utf8` and an exact `convert_valid` give on it. -/
theorem validRun_of_valid8 (v : List Nat) (h : ValidExact.Valid8 v) :
    ValidRun v (L (v.map w16)) :=
  valid8_run_aux v.length v (Nat.le_refl _) h

theorem bytes_drop {ys : List Nat} (h : Bytes ys) (k : Nat) : Bytes (ys.drop k) :=
  fun x hx => h x (List.mem_of_mem_drop hx)

theorem bytes_take {ys : List Nat} (h : Bytes ys) (k : Nat) : Bytes (ys.take k) :=
  fun x hx => h x (List.mem_of_mem_take hx)

/-! # B1: the buffer size is `count` -/

/-- **The scalar `validate_with_counts`** (include/simdutf/scalar/utf8.h:256-380)
with fuel: `(error == SUCCESS, input_count, utf16_length())`. -/
def vwcF : Nat → List Nat → Bool × Nat × Nat
  | 0, _ => (true, 0, 0)
  | _ + 1, [] => (true, 0, 0)
  | f + 1, b :: r =>
    match vchar (b :: r) with
    | none => (false, 0, 0)
    | some (k, u) =>
      ((vwcF f ((b :: r).drop k)).1, k + (vwcF f ((b :: r).drop k)).2.1,
        u + (vwcF f ((b :: r).drop k)).2.2)

def vwc (ys : List Nat) : Bool × Nat × Nat := vwcF ys.length ys

theorem ve_run_split (ys : List Nat) (k : Nat) (h : ValidExact.run 0 (ys.take k) = 0) :
    ValidExact.run 0 ys = ValidExact.run 0 (ys.drop k) := by
  conv => lhs; rw [← List.take_append_drop k ys]
  rw [ValidExact.run_append, h]

/-- The scalar validator is correct: on success the input is valid UTF-8 and
`utf16_length()` is `L` of its `w16`; on error `input_count` is inside the
input, the prefix is valid with `utf16_length()` units, and `dchar` fails at
`input_count`. -/
theorem vwcF_spec : ∀ f (ys : List Nat), Bytes ys → ys.length ≤ f →
    ((vwcF f ys).1 = true → ValidExact.Valid8 ys ∧ (vwcF f ys).2.2 = L (ys.map w16)) ∧
    ((vwcF f ys).1 = false → (vwcF f ys).2.1 < ys.length ∧
      ValidExact.Valid8 (ys.take (vwcF f ys).2.1) ∧
      (vwcF f ys).2.2 = L ((ys.take (vwcF f ys).2.1).map w16) ∧
      dchar (ys.drop (vwcF f ys).2.1) = none) := by
  intro f
  induction f with
  | zero =>
    intro ys _ hl
    rw [List.eq_nil_of_length_eq_zero (by omega : ys.length = 0)]
    exact ⟨fun _ => ⟨rfl, rfl⟩, fun h => by simp [vwcF] at h⟩
  | succ f ih =>
    intro ys hb hl
    rcases ys with _ | ⟨b, r⟩
    · exact ⟨fun _ => ⟨rfl, rfl⟩, fun h => by simp [vwcF] at h⟩
    have hv : vchar (b :: r) = dchar (b :: r) := vchar_eq_dchar _ hb
    cases hd : dchar (b :: r) with
    | none =>
      have e : vwcF (f + 1) (b :: r) = (false, 0, 0) := by simp only [vwcF, hv, hd]
      rw [e]
      exact ⟨fun h => (by cases h), fun _ => ⟨by simp, rfl, rfl, by simpa using hd⟩⟩
    | some ku =>
      obtain ⟨k, u⟩ := ku
      obtain ⟨-, hk1, hk2⟩ := dchar_run _ k u hd
      obtain ⟨hvk, hu⟩ := dchar_valid8 _ k u hd
      have e : vwcF (f + 1) (b :: r) = ((vwcF f ((b :: r).drop k)).1,
          k + (vwcF f ((b :: r).drop k)).2.1, u + (vwcF f ((b :: r).drop k)).2.2) := by
        simp only [vwcF, hv, hd]
      rw [e]
      obtain ⟨i1, i2⟩ := ih ((b :: r).drop k) (bytes_drop hb k) (by simp at hl ⊢; omega)
      refine ⟨fun h => ?_, fun h => ?_⟩
      · obtain ⟨j1, j2⟩ := i1 h
        refine ⟨by unfold ValidExact.Valid8; rw [ve_run_split _ k hvk]; exact j1, ?_⟩
        rw [j2, hu, ← L_map_split]
      · obtain ⟨j0, j1, j2, j3⟩ := i2 h
        generalize (vwcF f ((b :: r).drop k)).2.1 = p at *
        have et : (b :: r).take (k + p) = (b :: r).take k ++ ((b :: r).drop k).take p := by
          rw [List.take_add]
        refine ⟨by simp at j0 ⊢; omega, ?_, ?_, ?_⟩
        · unfold ValidExact.Valid8; rw [et, ValidExact.run_append, hvk]; exact j1
        · rw [et, List.map_append, L_append, hu, j2]
        · rw [← List.drop_drop]; exact j3


/-- What `utf16_length_from_utf8_with_replacement` reads from a validator: `ok`
(`error == SUCCESS`), `verr` (`is_utf8_validation_error`), `p`
(`input_count`), `u` (`utf16_length()`). -/
structure VRes where
  ok : Bool
  verr : Bool
  p : Nat
  u : Nat

/-- The contract a validator must meet for `lenRepl_eq`. The scalar one meets it
(`scalarV_spec`). For the vector `validate_utf8_with_counts`
(src/generic/utf8_validation/utf8_validator.h:114-161 for westmere, haswell,
ppc64, lsx, lasx; src/arm64/arm_validate_utf8_with_counts.cpp:54; icelake
and rvv in their implementation.cpp) it is assumed. -/
def VSpec (V : List Nat → VRes) : Prop :=
  ∀ ys, Bytes ys → ys ≠ [] →
    ((V ys).ok = true → ValidExact.Valid8 ys ∧ (V ys).u = L (ys.map w16)) ∧
    ((V ys).ok = false → (V ys).verr = true → (V ys).p < ys.length →
      ValidExact.Valid8 (ys.take (V ys).p) ∧ (V ys).u = L ((ys.take (V ys).p).map w16) ∧
      dchar (ys.drop (V ys).p) = none)

/-- The scalar validator as a `VRes` (all its errors are validation errors). -/
def scalarV (ys : List Nat) : VRes := ⟨(vwc ys).1, true, (vwc ys).2.1, (vwc ys).2.2⟩

theorem scalarV_spec : VSpec scalarV := by
  intro ys hb _
  obtain ⟨i1, i2⟩ := vwcF_spec ys.length ys hb (Nat.le_refl _)
  exact ⟨i1, fun h _ _ => (i2 h).2⟩

/-- **`utf16_length_from_utf8_with_replacement`**, both copies:
include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:209-256 (with `scalarV`) and
src/utf8_to_utf16_with_replacement.inl.h:238-276 (with the active
`validate_utf8_with_counts`). On the rest `ys`: success adds
`utf16_length()` and returns; an error that is not a validation error or
not inside `ys` adds `count_with_replacement` of `ys` (:250-258); otherwise
it adds `utf16_length() + 1` and resumes after the `maximal_subpart`
(:260-273). The fuel bounds the iterations. -/
def lenReplF (V : List Nat → VRes) : Nat → List Nat → Nat
  | 0, _ => 0
  | f + 1, ys =>
    if ys = [] then 0
    else if (V ys).ok then (V ys).u
    else if (V ys).verr = false ∨ ys.length ≤ (V ys).p then count ys
    else (V ys).u + 1 + lenReplF V f (ys.drop ((V ys).p + msub (ys.drop (V ys).p)))

def lenRepl (V : List Nat → VRes) (ys : List Nat) : Nat := lenReplF V ys.length ys

/-- The loop computes `count` of what is left. -/
theorem lenReplF_eq (V : List Nat → VRes) (hV : VSpec V) : ∀ f (ys : List Nat), Bytes ys →
    ys.length ≤ f → lenReplF V f ys = count ys := by
  intro f
  induction f with
  | zero =>
    intro ys _ hl
    rw [List.eq_nil_of_length_eq_zero (by omega : ys.length = 0)]; rfl
  | succ f ih =>
    intro ys hb hl
    by_cases hne : ys = []
    · subst hne; rfl
    obtain ⟨s1, s2⟩ := hV ys hb hne
    simp only [lenReplF, hne, ite_false]
    by_cases hok : (V ys).ok = true
    · rw [ite_eq_left hok]
      obtain ⟨hv, hu⟩ := s1 hok
      have := validRun_of_valid8 ys hv
      have hc := count_valid_append ys [] _ this
      rw [List.append_nil] at hc
      rw [hc, hu]; rfl
    rw [ite_eq_right hok]
    by_cases hf : (V ys).verr = false ∨ ys.length ≤ (V ys).p
    · rw [ite_eq_left hf]
    rw [ite_eq_right hf]
    have hok' : (V ys).ok = false := by simpa using hok
    have hverr : (V ys).verr = true := by simpa using (not_or.mp hf).1
    have hp : (V ys).p < ys.length := by have := (not_or.mp hf).2; omega
    obtain ⟨hv, hu, hd⟩ := s2 hok' hverr hp
    generalize (V ys).p = p at *
    generalize (V ys).u = u at *
    have hc := count_valid_append (ys.take p) (ys.drop p) _ (validRun_of_valid8 _ hv)
    rw [List.take_append_drop] at hc
    have hne' : ys.drop p ≠ [] := by
      intro h; have := congrArg List.length h; simp at this; omega
    obtain ⟨hs, hm1, hm2⟩ := subpart_of_none (ys.drop p) (bytes_drop hb p) hne' hd
    rw [List.drop_drop] at hs
    rw [ih _ (bytes_drop hb _) (by simp; omega), hc, hs, hu]
    omega

/-- **B1.** For every validator meeting `VSpec`, the buffer size
`utf16_length_from_utf8_with_replacement(...).count` is `count` of the input. -/
theorem lenRepl_eq (V : List Nat → VRes) (hV : VSpec V) (ys : List Nat) (hb : Bytes ys) :
    lenRepl V ys = count ys :=
  lenReplF_eq V hV _ ys hb (Nat.le_refl _)

/-- **B1, scalar validator**, no hypothesis besides bytes. -/
theorem lenRepl_scalar (ys : List Nat) (hb : Bytes ys) : lenRepl scalarV ys = count ys :=
  lenRepl_eq scalarV scalarV_spec ys hb


/-! # B2 and B3 in `RReach` and `convert_using_locations` -/

/-- `Replacement.Subpart` at an error position where `dchar` fails. -/
theorem subpart_at (xs : List Nat) (hb : Bytes xs) (e : Nat) (he : e < xs.length)
    (hd : dchar (xs.drop e) = none) : Subpart (xs.drop e) (msub (xs.drop e)) := by
  have hne : xs.drop e ≠ [] := by
    intro h; have := congrArg List.length h; simp at this; omega
  exact (subpart_of_none _ (bytes_drop hb e) hne hd).1

/-- A `Replacement.RReach.valid` step from a valid run whose units are `L` of
its `w16` weights (what an exact `convert_valid_prefix` returns on valid
input, `ValidExact.vx_tail`, and what `utf16_length_from_utf8` returns,
src/utf8_to_utf16_with_replacement.inl.h:125). -/
theorem rreach_valid8 {xs : List Nat} {pos written w : Nat} (h : RReach xs pos written)
    (hv : ValidExact.Valid8 ((xs.drop pos).take w)) :
    RReach xs (pos + w) (written + L (((xs.drop pos).take w).map w16)) :=
  RReach.valid _ _ _ _ h (validRun_of_valid8 _ hv)

/-- A `Replacement.RReach.error` step from a valid prefix and an error at which
`dchar` fails, with the `maximal_subpart` skip. -/
theorem rreach_error8 {xs : List Nat} {pos written k : Nat} (hb : Bytes xs)
    (h : RReach xs pos written) (hv : ValidExact.Valid8 ((xs.drop pos).take k))
    (hk : pos + k < xs.length) (hd : dchar (xs.drop (pos + k)) = none) :
    RReach xs (pos + k + msub (xs.drop (pos + k)))
      (written + L (((xs.drop pos).take k).map w16) + 1) :=
  RReach.error _ _ _ _ _ h (validRun_of_valid8 _ hv) (subpart_at xs hb _ hk hd)

/-- The positions and counts of the loop of `lenReplF` on the whole input `xs`:
the error offsets it records (`err = pos + input_count`,
src/utf8_to_utf16_with_replacement.inl.h:264-273) and the count so far.
`convert_using_locations` (:194-236) walks the same offsets: it converts the
gap before each with `convert_valid_prefix`, stores one U+FFFD and skips the
`maximal_subpart`. -/
inductive LReach (V : List Nat → VRes) (xs : List Nat) : Nat → Nat → Prop
  | start : LReach V xs 0 0
  | error (pos acc : Nat) (h : LReach V xs pos acc)
      (hne : xs.drop pos ≠ []) (hok : (V (xs.drop pos)).ok = false)
      (hv : (V (xs.drop pos)).verr = true)
      (hp : (V (xs.drop pos)).p < (xs.drop pos).length) :
      LReach V xs (pos + (V (xs.drop pos)).p + msub (xs.drop (pos + (V (xs.drop pos)).p)))
        (acc + (V (xs.drop pos)).u + 1)

/-- **B2, `convert_using_locations`.** Each gap it hands to
`convert_valid_prefix` (:213-216) is valid UTF-8 (the contract of
`convert_valid`), its `utf16_length()` is `L` of its `w16`, and `dchar`
fails at the recorded offset. -/
theorem lreach_gap (V : List Nat → VRes) (hV : VSpec V) (xs : List Nat) (hb : Bytes xs)
    (pos : Nat) (hne : xs.drop pos ≠ []) (hok : (V (xs.drop pos)).ok = false)
    (hv : (V (xs.drop pos)).verr = true) (hp : (V (xs.drop pos)).p < (xs.drop pos).length) :
    ValidExact.Valid8 ((xs.drop pos).take (V (xs.drop pos)).p) ∧
    (V (xs.drop pos)).u = L (((xs.drop pos).take (V (xs.drop pos)).p).map w16) ∧
    dchar (xs.drop (pos + (V (xs.drop pos)).p)) = none := by
  obtain ⟨-, s2⟩ := hV _ (bytes_drop hb pos) hne
  obtain ⟨a, b, c⟩ := s2 hok hv hp
  exact ⟨a, b, by rw [List.drop_drop] at c; exact c⟩

/-- Every state of the walk is a `Replacement.RReach` state, so
`Replacement.rreach_exact_store` applies to `convert_using_locations` with
no hypothesis on `ValidRun` or `Subpart`. -/
theorem lreach_rreach (V : List Nat → VRes) (hV : VSpec V) (xs : List Nat) (hb : Bytes xs)
    (pos acc : Nat) (h : LReach V xs pos acc) : RReach xs pos acc := by
  induction h with
  | start => exact RReach.start
  | error pos acc h hne hok hv hp ih =>
    obtain ⟨g1, g2, g3⟩ := lreach_gap V hV xs hb pos hne hok hv hp
    rw [g2]
    exact rreach_error8 hb ih g1 (by simp at hp ⊢; omega) g3

/-- The last gap (:226-230, `more_errors` false) is valid UTF-8 and its units
bring the count to `count xs`. -/
theorem lreach_tail (V : List Nat → VRes) (hV : VSpec V) (xs : List Nat) (hb : Bytes xs)
    (pos acc : Nat) (h : LReach V xs pos acc) (hne : xs.drop pos ≠ [])
    (hok : (V (xs.drop pos)).ok = true) :
    ValidExact.Valid8 (xs.drop pos) ∧ acc + (V (xs.drop pos)).u = count xs := by
  obtain ⟨s1, -⟩ := hV _ (bytes_drop hb pos) hne
  obtain ⟨hv8, hu⟩ := s1 hok
  refine ⟨hv8, ?_⟩
  have hi := rreach_inv xs pos acc (lreach_rreach V hV xs hb pos acc h)
  have hc := count_valid_append (xs.drop pos) [] _ (validRun_of_valid8 _ hv8)
  rw [List.append_nil] at hc
  rw [hu]; rw [count_nil] at hc; omega

/-! # A3: the vector `utf8_length_from_utf16_with_replacement`

All quantities are on the units after the byte swap of the non-native
endianness (applied to the loaded vectors and to the scalar reads alike).
`size_t` sums are `Nat`. -/

/-- Lane `min(input & 0xff80, 1)`,
src/generic/utf16/utf8_length_from_utf16_bytemask.h:127. -/
def c0 (u : Nat) : Nat := min (u &&& 0xff80) 1
/-- Lane `min(input & 0xf800, 1)` (:130). -/
def c1 (u : Nat) : Nat := min (u &&& 0xf800) 1
/-- Lane `(input & 0xf800) == 0xd800` (:124). -/
def surM (u : Nat) : Bool := u &&& 0xf800 == 0xd800
/-- `vector_u16(is_surrogate)`: 0xFFFF on a surrogate (:134). -/
def vsur (u : Nat) : Nat := if surM u then 0xFFFF else 0

/-- The lane facts, per 16-bit value. -/
def laneOK (u : Nat) : Bool :=
  (c0 u + c1 u + vsur u) % 65536 == w8 u - 1 && surM u == isSur u &&
  (u &&& 0xfc00 == 0xd800) == isHigh u && (u &&& 0xfc00 == 0xdc00) == isLow u

/-- Checked on all 65536 values (`decide +kernel`, a finite exhaustive check). -/
theorem lanes : (List.range 65536).all laneOK = true := by decide +kernel


theorem lane (u : Nat) (h : u < 65536) :
    (c0 u + c1 u + vsur u) % 65536 = w8 u - 1 ∧ surM u = isSur u ∧
    (u &&& 0xfc00 == 0xd800) = isHigh u ∧ (u &&& 0xfc00 == 0xdc00) = isLow u := by
  have := List.all_eq_true.mp lanes u (List.mem_range.mpr h)
  simp only [laneOK, Bool.and_eq_true, beq_iff_eq] at this
  exact ⟨this.1.1.1, this.1.1.2, this.1.2, this.2⟩

/-! ## The telescoping identity -/

/-- `sumTo f k = f 0 + ... + f (k - 1)`. -/
def sumTo (f : Nat → Nat) : Nat → Nat
  | 0 => 0
  | k + 1 => sumTo f k + f k

theorem sumTo_add (f : Nat → Nat) (a : Nat) : ∀ m,
    sumTo f (a + m) = sumTo f a + sumTo (fun i => f (a + i)) m := by
  intro m; induction m with
  | zero => rfl
  | succ m ih => rw [← Nat.add_assoc, sumTo, ih, sumTo]; omega

theorem sumTo_plus (f h : Nat → Nat) : ∀ m,
    sumTo (fun i => f i + h i) m = sumTo f m + sumTo h m := by
  intro m; induction m with
  | zero => rfl
  | succ m ih => simp only [sumTo, ih]; omega

theorem sumTo_congr (f h : Nat → Nat) (m : Nat) (e : ∀ i, i < m → f i = h i) :
    sumTo f m = sumTo h m := by
  induction m with
  | zero => rfl
  | succ m ih => simp only [sumTo, ih (fun i hi => e i (by omega)), e m (by omega)]

theorem sumTo_zero (m : Nat) : sumTo (fun _ => 0) m = 0 := by
  induction m with
  | zero => rfl
  | succ m ih => simp [sumTo, ih]

theorem sumTo_le (f : Nat → Nat) (c m : Nat) (h : ∀ i, i < m → f i ≤ c) : sumTo f m ≤ m * c := by
  induction m with
  | zero => simp [sumTo]
  | succ m ih =>
    have := ih (fun i hi => h i (by omega)); have := h m (by omega)
    simp only [sumTo, Nat.succ_mul]; omega

/-- `illseq` of a lane, `is_high(u) xor is_low(v)`, as 0/1 (:144-150). -/
def illvB (u v : Nat) : Nat := if isHigh u != isLow v then 1 else 0

section
variable (xs : List Nat)

/-- Unit `j` (0 past the end; the C++ reads no unit past the end). -/
def g (j : Nat) : Nat := xs.getD j 0

def illv (j : Nat) : Nat := illvB (g xs j) (g xs (j + 1))

/-- What the vector part has counted after `k` units: `count` (sum of `w8 - 1`,
plus `pos`) plus `mismatched_count` (`in[0]` low, plus `illseq`). -/
def Phi (k : Nat) : Nat :=
  sumTo (fun j => w8 (g xs j) - 1) k + k + (if isLow (g xs 0) then 1 else 0) + sumTo (illv xs) k

/-- What the scalar part must count from `k`, an unpaired low surrogate at `k`
having been counted once by `illseq`. -/
def rest (k : Nat) : Nat :=
  if isLow (g xs k) then 2 + countRepl (xs.drop (k + 1)) else countRepl (xs.drop k)
end

theorem drop_g (xs : List Nat) (k : Nat) (hk : k < xs.length) :
    xs.drop k = g xs k :: xs.drop (k + 1) := by
  rw [List.drop_eq_getElem_cons hk]
  simp [g, List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hk]

theorem g_past (xs : List Nat) (k : Nat) (hk : xs.length ≤ k) : g xs k = 0 := by
  simp [g, List.getD_eq_getElem?_getD, List.getElem?_eq_none hk]

theorem cR_low (u : Nat) (r : List Nat) (h : isLow u = true) : countRepl (u :: r) = 3 + countRepl r := by
  have hh : isHigh u = false := by simp [isHigh, isLow] at *; omega
  cases r with
  | nil => simp [countRepl, hh, h]
  | cons v r => simp [countRepl, hh, h]

theorem cR_pair (u v : Nat) (r : List Nat) (hu : isHigh u = true) (hv : isLow v = true) :
    countRepl (u :: v :: r) = 4 + countRepl r := by simp [countRepl, hu, hv]

theorem cR_high (u : Nat) (r : List Nat) (hu : isHigh u = true)
    (hr : ∀ v r', r = v :: r' → isLow v = false) : countRepl (u :: r) = 3 + countRepl r := by
  cases r with
  | nil => simp [countRepl, hu]
  | cons v r => simp [countRepl, hu, hr v r rfl]

theorem cR_plain (u : Nat) (r : List Nat) (hs : isSur u = false) :
    countRepl (u :: r) = w8 u + countRepl r := by
  have hh : isHigh u = false := by simp [isHigh, isSur] at *; omega
  have hl : isLow u = false := by simp [isLow, isSur] at *; omega
  rw [w8_nonsur u hs]
  cases r with
  | nil => simp [countRepl, hh, hl]
  | cons v r => simp [countRepl, hh, hl]

theorem rest_step (xs : List Nat) (k : Nat) (hk : k < xs.length) :
    rest xs k = w8 (g xs k) + illv xs k + rest xs (k + 1) := by
  have hd := drop_g xs k hk
  have hnext : ∀ v r', xs.drop (k + 1) = v :: r' → v = g xs (k + 1) ∧ r' = xs.drop (k + 2) := by
    intro v r' h
    by_cases hk1 : k + 1 < xs.length
    · rw [drop_g xs (k + 1) hk1] at h; simp at h; exact ⟨h.1.symm, h.2.symm⟩
    · have : xs.drop (k + 1) = [] := List.drop_eq_nil_of_le (by omega)
      rw [this] at h; cases h
  have hpast : xs.length ≤ k + 1 → isLow (g xs (k + 1)) = false := by
    intro h; rw [g_past xs _ h]; decide
  have hin : isLow (g xs (k + 1)) = true → k + 1 < xs.length := by
    intro hl1; refine Nat.lt_of_not_le fun hc => ?_; rw [hpast hc] at hl1; cases hl1
  unfold rest illv illvB
  generalize hu : g xs k = u at *
  by_cases hl : isLow u = true
  · have hh : isHigh u = false := by simp [isHigh, isLow] at *; omega
    have w2 : w8 u = 2 := w8_sur u (low_sur hl)
    simp only [hl, ite_true, hh, w2]
    by_cases hl1 : isLow (g xs (k + 1)) = true
    · have hk1 : k + 1 < xs.length := by
        exact hin hl1
      rw [drop_g xs (k + 1) hk1, cR_low _ _ hl1, hl1]; simp; omega
    · simp only [Bool.not_eq_true] at hl1; simp [hl1]
  · simp only [Bool.not_eq_true] at hl
    rw [hd]
    simp only [hl, Bool.false_eq_true, ite_false]
    by_cases hh : isHigh u = true
    · have w2 : w8 u = 2 := w8_sur u (high_sur hh)
      by_cases hl1 : isLow (g xs (k + 1)) = true
      · have hk1 : k + 1 < xs.length := by
          exact hin hl1
        rw [drop_g xs (k + 1) hk1, cR_pair _ _ _ hh hl1, hh, hl1, w2]; simp; omega
      · simp only [Bool.not_eq_true] at hl1
        rw [cR_high _ _ hh (fun v r' h => by obtain ⟨rfl, -⟩ := hnext v r' h; exact hl1), hh, hl1, w2]
        simp
    · simp only [Bool.not_eq_true] at hh
      have hs : isSur u = false := by simp [isHigh, isLow, isSur] at *; omega
      rw [cR_plain _ _ hs, hh]
      by_cases hl1 : isLow (g xs (k + 1)) = true
      · have hk1 : k + 1 < xs.length := by
          exact hin hl1
        rw [drop_g xs (k + 1) hk1, cR_low _ _ hl1, hl1]; simp; omega
      · simp only [Bool.not_eq_true] at hl1; simp [hl1]

/-- **The telescoping identity** behind both vector functions: for every
`k ≤ length`, `Phi k + rest k = countRepl xs`. -/
theorem tele (xs : List Nat) : ∀ k, k ≤ xs.length → Phi xs k + rest xs k = countRepl xs := by
  intro k
  induction k with
  | zero =>
    intro _
    unfold Phi rest
    simp only [sumTo, List.drop_zero]
    by_cases hl : isLow (g xs 0) = true
    · have h0 : 0 < xs.length := by
        refine Nat.lt_of_not_le fun hc => ?_; rw [g_past xs 0 hc] at hl; cases hl
      have e := drop_g xs 0 h0
      rw [List.drop_zero] at e
      conv => rhs; rw [e, cR_low _ _ hl]
      simp only [hl, ite_true]; omega
    · simp only [Bool.not_eq_true] at hl; simp [hl]
  | succ k ih =>
    intro hk
    have := ih (by omega)
    rw [rest_step xs k (by omega)] at this
    unfold Phi at this ⊢
    simp only [sumTo]
    have := w8_ge (g xs k)
    omega


/-! ## The generic loop, lane by lane -/

/-- 16-bit lane addition. -/
def addw (a b : Nat) : Nat := (a + b) % 65536
def hiM (u : Nat) : Nat := if u &&& 0xfc00 == 0xd800 then 0xFFFF else 0
def loM (v : Nat) : Nat := if v &&& 0xfc00 == 0xdc00 then 0xFFFF else 0
def illM (u v : Nat) : Nat := min (hiM u ^^^ loM v) 1

/-- The state of the loop of `utf8_length_from_utf16_with_replacement`
(src/generic/utf16/utf8_length_from_utf16_bytemask.h:87-194): the lanes of
`v_count` and `v_mismatched_count` (as functions of the lane index),
`count`, `mismatched_count`, `iteration`. -/
structure BM where
  vc : Nat → Nat
  vm : Nat → Nat
  count : Nat
  mism : Nat
  iter : Nat

/-- `max_iterations = 65535 / 2` (:109). -/
def maxIt : Nat := 65535 / 2

/-- Before the loop (:96-115). -/
def bmInit (xs : List Nat) : BM :=
  ⟨fun _ => 0, fun _ => 0, 0, if isLow (g xs 0) then 1 else 0, maxIt⟩

/-- One iteration of the loop (:117-163) at `pos`, `N` lanes: the three 16-bit
additions into `v_count`; the `illseq` additions when the block has a
surrogate or `in[pos + N]` is low; the flush every `max_iterations` (`sum()`
widens: src/simdutf/westmere/simd16-inl.h:134-145,
src/simdutf/haswell/simd16-inl.h:170-184). -/
def bmStep (N : Nat) (xs : List Nat) (st : BM) (pos : Nat) : BM :=
  let vc' := fun i => addw (addw (addw (st.vc i) (c0 (g xs (pos + i))))
    (c1 (g xs (pos + i)))) (vsur (g xs (pos + i)))
  let cond := (List.range N).any (fun i => surM (g xs (pos + i))) || isLow (g xs (pos + N))
  let vm' := fun i =>
    if cond then addw (st.vm i) (illM (g xs (pos + i)) (g xs (pos + i + 1))) else st.vm i
  if st.iter - 1 = 0 then
    ⟨fun _ => 0, fun _ => 0, st.count + sumTo vc' N, st.mism + sumTo vm' N, maxIt⟩
  else ⟨vc', vm', st.count, st.mism, st.iter - 1⟩

/-- `b` iterations, block `b` at `b * N`. -/
def bmLoop (N : Nat) (xs : List Nat) : Nat → BM
  | 0 => bmInit xs
  | b + 1 => bmStep N xs (bmLoop N xs b) (b * N)

/-- After the loop
(src/generic/utf16/utf8_length_from_utf16_bytemask.h:165-193, and
src/arm64/arm_convert_utf16_to_utf8.cpp:750-775), the final flush done: the
low surrogate at `pos`, `count += pos + mismatched_count`, the high
surrogate at `pos - 1`, and the scalar
`utf8_length_from_utf16_with_replacement` (`countRepl`) on the rest. The
`size_t` decrement of `mismatched_count` does not wrap (it is at least 1
there, see `tail_eq`). -/
def tailRepl (xs : List Nat) (P count1 mism1 : Nat) : Nat :=
  let A := isLow (g xs P) && !isHigh (g xs (P - 1))
  let count2 := if A then count1 + 2 else count1
  let mism2 := if A then mism1 - 1 else mism1
  let pos2 := if A then P + 1 else P
  let count3 := count2 + pos2 + mism2
  let hb := isHigh (g xs (pos2 - 1))
  let count4 := if hb then
      (if pos2 = xs.length then count3 + 2 else if isLow (g xs pos2) then count3 + 2 else count3)
    else count3
  let pos4 := if hb = true ∧ pos2 ≠ xs.length ∧ isLow (g xs pos2) = true then pos2 + 1 else pos2
  count4 + countRepl (xs.drop pos4)

/-- **The generic `utf8_length_from_utf16_with_replacement`**
(src/generic/utf16/utf8_length_from_utf16_bytemask.h:87-194; westmere
`N = 8`, haswell `N = 16`, src/westmere/implementation.cpp:1231-1241,
src/haswell/implementation.cpp:1143-1153) on the units after the byte swap.
The loop runs `(size - 1) / N` times
(src/generic/utf16/utf8_length_from_utf16_bytemask.h:117). -/
def bytemask (N : Nat) (xs : List Nat) : Nat :=
  if N + 1 > xs.length then countRepl xs else
  let st := bmLoop N xs ((xs.length - 1) / N)
  tailRepl xs ((xs.length - 1) / N * N)
    (st.count + (if st.iter > 0 then sumTo st.vc N else 0))
    (st.mism + (if st.iter > 0 then sumTo st.vm N else 0))

/-- Every unit is a 16-bit value. -/
def U16 (xs : List Nat) : Prop := ∀ u ∈ xs, u < 65536

theorem g_lt (xs : List Nat) (h : U16 xs) (j : Nat) : g xs j < 65536 := by
  unfold g
  rw [List.getD_eq_getElem?_getD]
  cases e : xs[j]? with
  | none => simp
  | some v => simpa using h v (List.mem_of_getElem? e)

theorem addw3 (a x y z : Nat) : addw (addw (addw a x) y) z = (a + (x + y + z)) % 65536 := by
  simp only [addw, Nat.mod_add_mod]; congr 1; omega

theorem illM_eq (u v : Nat) (hu : u < 65536) (hv : v < 65536) :
    illM u v = if isHigh u != isLow v then 1 else 0 := by
  obtain ⟨-, -, h1, -⟩ := lane u hu
  obtain ⟨-, -, -, h2⟩ := lane v hv
  unfold illM hiM loM
  rw [h1, h2]
  cases isHigh u <;> cases isLow v <;> decide

/-- Loop invariant: since the last flush, each `v_count` lane is at most
`2 * iterations` and each `v_mismatched_count` lane at most `iterations`,
so no 16-bit lane wraps; the totals are the unit sums. -/
def BInv (N : Nat) (xs : List Nat) (st : BM) (k : Nat) : Prop :=
  1 ≤ st.iter ∧ st.iter ≤ maxIt ∧
  (∀ i, i < N → st.vc i ≤ 2 * (maxIt - st.iter) ∧ st.vm i ≤ maxIt - st.iter) ∧
  st.count + sumTo st.vc N = sumTo (fun j => w8 (g xs j) - 1) k ∧
  st.mism + sumTo st.vm N = (if isLow (g xs 0) then 1 else 0) + sumTo (illv xs) k

theorem illv_le (xs : List Nat) (j : Nat) : illv xs j ≤ 1 := by
  unfold illv illvB; split <;> omega

/-- A block with no surrogate and no low surrogate after it has no `illseq`:
skipping it (src/generic/utf16/utf8_length_from_utf16_bytemask.h:135-136)
changes nothing. -/
theorem ill_zero (xs : List Nat) (N pos : Nat)
    (hs : ∀ i, i < N → isSur (g xs (pos + i)) = false) (hl : isLow (g xs (pos + N)) = false) :
    ∀ i, i < N → illv xs (pos + i) = 0 := by
  intro i hi
  have hh : isHigh (g xs (pos + i)) = false := by
    cases e : isHigh (g xs (pos + i))
    · rfl
    · have := hs i hi; rw [high_sur e] at this; cases this
  have hl' : isLow (g xs (pos + i + 1)) = false := by
    by_cases hi1 : i + 1 < N
    · have := hs (i + 1) hi1
      rw [show pos + i + 1 = pos + (i + 1) by omega]
      cases e : isLow (g xs (pos + (i + 1)))
      · rfl
      · rw [low_sur e] at this; cases this
    · rw [show pos + i + 1 = pos + N by omega]; exact hl
  simp [illv, illvB, hh, hl']

/-- The invariant holds after every iteration. -/
theorem bm_inv (N : Nat) (xs : List Nat) (hu : U16 xs) : ∀ b, BInv N xs (bmLoop N xs b) (b * N) := by
  intro b
  induction b with
  | zero =>
    simp only [bmLoop, bmInit, Nat.zero_mul]
    refine ⟨by simp [maxIt], by simp, fun i _ => by simp, ?_, ?_⟩
    · simp [sumTo_zero, sumTo]
    · simp [sumTo_zero, sumTo]
  | succ b ih =>
    obtain ⟨i1, i2, i3, i4, i5⟩ := ih
    generalize hst : bmLoop N xs b = st at *
    have hpos : (b + 1) * N = b * N + N := Nat.succ_mul b N
    simp only [bmLoop, hst]
    generalize b * N = pos at *
    rw [hpos]
    -- the lanes after the block
    have hvc : ∀ i, i < N → addw (addw (addw (st.vc i) (c0 (g xs (pos + i))))
        (c1 (g xs (pos + i)))) (vsur (g xs (pos + i))) = st.vc i + (w8 (g xs (pos + i)) - 1) := by
      intro i hi
      rw [addw3, ← Nat.add_mod_mod, (lane _ (g_lt xs hu _)).1]
      have := (i3 i hi).1
      have : w8 (g xs (pos + i)) ≤ 3 := by unfold w8 Arch.Icelake.w8; repeat' (first | omega | split)
      apply Nat.mod_eq_of_lt
      unfold maxIt at *; omega
    have hvm : ∀ i, i < N →
        (if ((List.range N).any (fun i => surM (g xs (pos + i))) || isLow (g xs (pos + N))) = true
          then addw (st.vm i) (illM (g xs (pos + i)) (g xs (pos + i + 1))) else st.vm i) =
        st.vm i + illv xs (pos + i) := by
      intro i hi
      split
      · rw [illM_eq _ _ (g_lt xs hu _) (g_lt xs hu _)]
        unfold addw illv illvB
        have := (i3 i hi).2
        apply Nat.mod_eq_of_lt
        unfold maxIt at *; split <;> omega
      · rename_i hc
        simp only [Bool.or_eq_true, List.any_eq_true, List.mem_range, not_or, not_exists,
          not_and, Bool.not_eq_true] at hc
        have hs : isSur (g xs (pos + i)) = false := by
          rw [← (lane _ (g_lt xs hu _)).2.1]; exact hc.1 i hi
        have hh : isHigh (g xs (pos + i)) = false := by
          cases e : isHigh (g xs (pos + i))
          · rfl
          · rw [high_sur e] at hs; cases hs
        have hl : isLow (g xs (pos + i + 1)) = false := by
          by_cases hi1 : i + 1 < N
          · have := hc.1 (i + 1) hi1
            rw [(lane _ (g_lt xs hu _)).2.1] at this
            rw [show pos + i + 1 = pos + (i + 1) by omega]
            cases e : isLow (g xs (pos + (i + 1)))
            · rfl
            · rw [low_sur e] at this; cases this
          · rw [show pos + i + 1 = pos + N by omega]; exact hc.2
        simp [illv, illvB, hh, hl]
    have svc := sumTo_congr _ _ N hvc
    have svm := sumTo_congr _ _ N hvm
    rw [sumTo_plus] at svc svm
    have a1 := sumTo_add (fun j => w8 (g xs j) - 1) pos N
    have a2 := sumTo_add (illv xs) pos N
    unfold bmStep
    simp only
    split
    · refine ⟨by simp [maxIt], by simp, fun i _ => by simp [maxIt], ?_, ?_⟩
      · simp only [sumTo_zero]; rw [svc, a1]; omega
      · simp only [sumTo_zero]; rw [svm, a2]; omega
    · unfold BInv; dsimp only
      refine ⟨by omega, by omega, fun i hi => ?_, ?_, ?_⟩
      · rw [hvc i hi, hvm i hi]
        have := i3 i hi
        have : w8 (g xs (pos + i)) ≤ 3 := by unfold w8 Arch.Icelake.w8; repeat' (first | omega | split)
        have := illv_le xs (pos + i)
        have := w8_ge (g xs (pos + i))
        omega
      · rw [svc, a1]; omega
      · rw [svm, a2]; omega


/-- The tail turns the vector totals into `countRepl` of the input. -/
theorem tail_eq (xs : List Nat) (P c m : Nat) (hP1 : 1 ≤ P) (hP2 : P ≤ xs.length - 1)
    (hc : c = sumTo (fun j => w8 (g xs j) - 1) P)
    (hm : m = (if isLow (g xs 0) then 1 else 0) + sumTo (illv xs) P) :
    tailRepl xs P c m = countRepl xs := by
  subst hc hm
  have ht := tele xs P (by omega)
  unfold Phi rest at ht
  unfold tailRepl
  have hsplit : sumTo (illv xs) P = sumTo (illv xs) (P - 1) + illv xs (P - 1) := by
    conv => lhs; rw [show P = (P - 1) + 1 by omega]
    rfl
  by_cases hA : isLow (g xs P) = true ∧ isHigh (g xs (P - 1)) = false
  · have hill : illv xs (P - 1) = 1 := by
      unfold illv illvB; rw [show P - 1 + 1 = P by omega, hA.1, hA.2]; decide
    have hhP : isHigh (g xs (P + 1 - 1)) = false := by
      rw [show P + 1 - 1 = P by omega]
      have := hA.1; simp [isHigh, isLow] at *; omega
    simp only [hA.1, hA.2, Bool.not_false, Bool.and_self, ite_true, hhP, Bool.false_eq_true,
      ite_false, false_and] at ht ⊢
    omega
  · have hA' : (isLow (g xs P) && !isHigh (g xs (P - 1))) = false := by
      cases h1 : isLow (g xs P) <;> cases h2 : isHigh (g xs (P - 1)) <;> simp_all
    simp only [hA', Bool.false_eq_true, ite_false]
    by_cases hh : isHigh (g xs (P - 1)) = true
    · by_cases hl : isLow (g xs P) = true
      · simp only [hh, hl, ite_true, show P ≠ xs.length by omega, ite_false, ne_eq,
          not_false_eq_true, and_self] at ht ⊢
        omega
      · simp only [Bool.not_eq_true] at hl
        simp only [hh, hl, ite_true, show P ≠ xs.length by omega, ite_false, ne_eq,
          Bool.false_eq_true, and_false, not_false_eq_true, and_self] at ht ⊢
        omega
    · have hl : isLow (g xs P) = false := by
        cases h1 : isLow (g xs P)
        · rfl
        · exact absurd ⟨h1, by simpa using hh⟩ hA
      simp only [Bool.not_eq_true] at hh
      simp only [hh, hl, Bool.false_eq_true, ite_false, false_and] at ht ⊢
      omega

/-- **A3, generic (westmere, haswell).** The vector length function equals the
scalar one, `countRepl`, on every input, for every vector width `N ≥ 1`. -/
theorem bytemask_eq (N : Nat) (hN : 0 < N) (xs : List Nat) (hu : U16 xs) :
    bytemask N xs = countRepl xs := by
  unfold bytemask
  split
  · rfl
  rename_i hlen
  obtain ⟨i1, -, -, i4, i5⟩ := bm_inv N xs hu ((xs.length - 1) / N)
  have hB1 : 1 ≤ (xs.length - 1) / N := (Nat.le_div_iff_mul_le hN).mpr (by omega)
  have hP2 : (xs.length - 1) / N * N ≤ xs.length - 1 := Nat.div_mul_le_self _ _
  generalize (xs.length - 1) / N = B at *
  have hP1 : N ≤ B * N := by simpa using Nat.mul_le_mul_right N hB1
  generalize bmLoop N xs B = st at *
  simp only [show st.iter > 0 from by omega, ite_true]
  exact tail_eq xs _ _ _ (by omega) hP2 (by omega) (by omega)


/-! ## arm64 -/

/-- The arm64 lane count on the deinterleaved bytes `msb = u / 256`,
`lsb = u % 256` (src/arm64/arm_convert_utf16_to_utf8.cpp:689-690,
:726-733): `c0 + c1 + is_surrogate` in 8-bit lanes. -/
def armLane (u : Nat) : Nat :=
  (min ((u % 256 &&& 0x80) ||| u / 256) 1 + min (u / 256 &&& 0xF8) 1 +
    (if (u / 256 + 256 - 0xD8) % 256 ≤ 7 then 0xFF else 0)) % 256
/-- `vcleq_u8(vsubq_u8(msb, 0xd8), 7)` (:689-690). -/
def armSur (u : Nat) : Bool := decide ((u / 256 + 256 - 0xD8) % 256 ≤ 7)
/-- `(lb_is_high ^ block_is_low) & 1` (:703-724). -/
def armIll (u v : Nat) : Nat :=
  ((if u / 256 &&& 0xFC == 0xD8 then 0xFF else 0) ^^^
    (if v / 256 &&& 0xFC == 0xDC then 0xFF else 0)) &&& 1

/-- The arm64 lane facts, per 16-bit value. -/
def armOK (u : Nat) : Bool :=
  armLane u == w8 u - 1 && armSur u == isSur u &&
  (u / 256 &&& 0xFC == 0xD8) == isHigh u && (u / 256 &&& 0xFC == 0xDC) == isLow u

/-- Checked on all 65536 values (`decide +kernel`, a finite exhaustive check). -/
theorem armLanes : (List.range 65536).all armOK = true := by decide +kernel

theorem arm_lane (u : Nat) (h : u < 65536) :
    armLane u = w8 u - 1 ∧ armSur u = isSur u ∧
    (u / 256 &&& 0xFC == 0xD8) = isHigh u ∧ (u / 256 &&& 0xFC == 0xDC) = isLow u := by
  have := List.all_eq_true.mp armLanes u (List.mem_range.mpr h)
  simp only [armOK, Bool.and_eq_true, beq_iff_eq] at this
  exact ⟨this.1.1.1, this.1.1.2, this.1.2, this.2⟩

theorem armIll_eq (u v : Nat) (hu : u < 65536) (hv : v < 65536) :
    armIll u v = illvB u v := by
  obtain ⟨-, -, h1, -⟩ := arm_lane u hu
  obtain ⟨-, -, -, h2⟩ := arm_lane v hv
  unfold armIll illvB
  rw [h1, h2]
  cases isHigh u <;> cases isLow v <;> decide

/-- **`arm64_utf8_length_from_utf16_with_replacement`**
(src/arm64/arm_convert_utf16_to_utf8.cpp:644-776): `vaddlvq_u8` sums each
block exactly; the loop `for (; size - pos >= N + 1; pos += N)` runs
`(size - 1) / 16` times (`arm_iters`); the tail is the generic one. -/
def armLen (xs : List Nat) : Nat :=
  if 16 + 1 > xs.length then countRepl xs else
  let B := (xs.length - 1) / 16
  let count1 := sumTo (fun b => sumTo (fun i => armLane (g xs (b * 16 + i))) 16) B
  let unp := (if isLow (g xs 0) then 1 else 0) + sumTo (fun b =>
    if ((List.range 16).any (fun i => armSur (g xs (b * 16 + i))) || isLow (g xs (b * 16 + 16)))
    then sumTo (fun i => armIll (g xs (b * 16 + i)) (g xs (b * 16 + i + 1))) 16 else 0) B
  tailRepl xs (B * 16) count1 unp

/-- The arm64 loop bound is the generic one. -/
theorem arm_iters (n N : Nat) (hN : 0 < N) :
    (∀ b, b < (n - 1) / N → N + 1 ≤ n - b * N) ∧ ¬ (N + 1 ≤ n - (n - 1) / N * N) := by
  have h1 := Nat.div_mul_le_self (n - 1) N
  have h2 := Nat.lt_div_mul_add (a := n - 1) hN
  refine ⟨fun b hb => ?_, by omega⟩
  have : (b + 1) * N ≤ (n - 1) / N * N := Nat.mul_le_mul_right N hb
  rw [Nat.succ_mul] at this; omega

theorem blocks_sum (f : Nat → Nat) (N : Nat) : ∀ B,
    sumTo (fun b => sumTo (fun i => f (b * N + i)) N) B = sumTo f (B * N) := by
  intro B
  induction B with
  | zero => simp [sumTo]
  | succ B ih => rw [sumTo, ih, Nat.succ_mul, sumTo_add]

/-- **A3, arm64.** The vector length function equals `countRepl` on every
input. -/
theorem armLen_eq (xs : List Nat) (hu : U16 xs) : armLen xs = countRepl xs := by
  unfold armLen
  split
  · rfl
  rename_i hlen
  have hB1 : 1 ≤ (xs.length - 1) / 16 := (Nat.le_div_iff_mul_le (by decide)).mpr (by omega)
  have hP2 : (xs.length - 1) / 16 * 16 ≤ xs.length - 1 := Nat.div_mul_le_self _ _
  generalize (xs.length - 1) / 16 = B at *
  simp only
  apply tail_eq xs _ _ _ (by omega) hP2
  · rw [← blocks_sum]
    apply sumTo_congr; intro b _; apply sumTo_congr; intro i _
    exact (arm_lane _ (g_lt xs hu _)).1
  · rw [← blocks_sum]
    refine congrArg (fun z => (if isLow (g xs 0) = true then 1 else 0) + z) ?_
    apply sumTo_congr; intro b _
    split
    · apply sumTo_congr; intro i _
      rw [armIll_eq _ _ (g_lt xs hu _) (g_lt xs hu _)]; rfl
    · rename_i hc
      simp only [Bool.or_eq_true, List.any_eq_true, List.mem_range, not_or, not_exists,
        not_and, Bool.not_eq_true] at hc
      rw [sumTo_congr _ (fun _ => 0) 16 (ill_zero xs 16 (b * 16) (fun i hi => by
        rw [← (arm_lane _ (g_lt xs hu _)).2.1]; exact hc.1 i hi) hc.2), sumTo_zero]

/-! ## Ice Lake -/

/-- Ice Lake per-unit count: `1 + c0 + c1` (`_mm512_test_epi16_mask` with
0xff80 and 0xf800, src/icelake/icelake_utf8_length_from_utf16.inl.cpp:165-172,
:212-215, :246-249, and `count += pos` :265); a surrogate counts 3. -/
def iw (u : Nat) : Nat := 1 + (if u > 0x7F then 1 else 0) + (if u > 0x7FF then 1 else 0)

/-- Adjacent (high, low) pairs: Ice Lake's `matches` (:177-199, :218-230,
:253-262). -/
def prs : List Nat → Nat
  | u :: v :: r => (if isHigh u && isLow v then 1 else 0) + prs (v :: r)
  | _ => 0

theorem iw_sur (u : Nat) (h : isSur u = true) : iw u = 3 := by
  simp [isSur] at h; unfold iw; split <;> split <;> omega

theorem iw_nonsur (u : Nat) (h : isSur u = false) : iw u = w8 u := by
  rw [w8_nonsur u h]; rfl

theorem prs_low (v : Nat) (r : List Nat) (h : isLow v = true) : prs (v :: r) = prs r := by
  have hv : isHigh v = false := by simp [isHigh, isLow] at *; omega
  cases r with
  | nil => rfl
  | cons w r => simp [prs, hv]

/-- **A3, Ice Lake, the identity.** `count - 2 * matches`, with `count` the sum
of `iw` and `matches` the adjacent (high, low) pairs, is `countRepl`. That
the three loops of `icelake_utf8_length_from_utf16_with_replacement`
(:115-269: unrolled `2N` blocks with two straddle words, one `N` block with
one straddle word, a masked tail whose low lanes are zero) count each unit
once and each adjacent pair once is read from the C++, not proved. -/
theorem icelake_identity (xs : List Nat) : L (xs.map iw) = countRepl xs + 2 * prs xs := by
  induction xs using wR.induct with
  | case1 => rfl
  | case2 u =>
    simp only [List.map, L_cons, L_nil, countRepl, prs]
    by_cases hs : isSur u = true
    · rw [iw_sur u hs]
      by_cases hh : isHigh u = true
      · simp [hh]
      · have hl : isLow u = true := by simp [isSur, isHigh, isLow] at *; omega
        simp [hh, hl]
    · simp only [Bool.not_eq_true] at hs
      have hh : isHigh u = false := by simp [isSur, isHigh] at *; omega
      have hl : isLow u = false := by simp [isSur, isLow] at *; omega
      rw [iw_nonsur u hs, w8_nonsur u hs]; simp [hh, hl]
  | case3 u v r hp ih =>
    simp only [Bool.and_eq_true] at hp
    simp only [List.map, L_cons, countRepl, prs, hp.1, hp.2, ite_true, Bool.and_self,
      iw_sur u (high_sur hp.1), iw_sur v (low_sur hp.2), prs_low v r hp.2]
    rw [show L (List.map iw r) = _ from ih]; omega
  | case4 u v r hp ih =>
    simp only [List.map, L_cons] at ih ⊢
    simp only [prs, ite_eq_right hp]
    simp only [countRepl]
    by_cases hh : isHigh u = true
    · have hv : isLow v = false := by
        cases h : isLow v
        · rfl
        · exact absurd (by simp [hh, h]) hp
      simp only [hh, hv, ite_true, Bool.false_eq_true, ite_false, iw_sur u (high_sur hh)]
      omega
    · simp only [Bool.not_eq_true] at hh
      by_cases hl : isLow u = true
      · simp only [hh, hl, ite_true, Bool.false_eq_true, ite_false, iw_sur u (low_sur hl)]
        omega
      · simp only [Bool.not_eq_true] at hl
        have hs : isSur u = false := by simp [isSur, isHigh, isLow] at *; omega
        simp only [hh, hl, Bool.false_eq_true, ite_false, iw_nonsur u hs, w8_nonsur u hs]
        omega


/-! # A1: the UTF-16 → UTF-8 kernel stops at the first unpaired surrogate -/

/-- **The scalar `convert_with_errors`** for UTF-16 → UTF-8
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:99-203, `check_output`
false): `(error == SURROGATE, pos, bytes written)`. The ASCII probe
(:115-135) writes one byte per unit, as `w8`. The surrogate branch fails at
`pos` when there is no next unit, when the unit is a low surrogate, or when
the next unit is not a low surrogate (:178-190). -/
def sc8 : List Nat → Bool × Nat × Nat
  | [] => (false, 0, 0)
  | u :: r =>
    if isSur u = false then
      ((sc8 r).1, 1 + (sc8 r).2.1, w8 u + (sc8 r).2.2)
    else match r with
      | [] => (true, 0, 0)
      | v :: r' =>
        if isHigh u = true ∧ isLow v = true then
          ((sc8 r').1, 2 + (sc8 r').2.1, 4 + (sc8 r').2.2)
        else (true, 0, 0)

/-- `unp` splits where `wR` does. -/
theorem unp_append (a b : List Nat) (h : endsHigh a = false) : unp (a ++ b) = unp a + unp b := by
  have e := wR_append a b (by simp [h])
  have h1 := L_wR (a ++ b); have h2 := L_wR a; have h3 := L_wR b
  rw [e, L_append, List.map_append, L_append] at h1
  omega

theorem endsHigh_append_false (a b : List Nat) (ha : endsHigh a = false)
    (hb : endsHigh b = false) : endsHigh (a ++ b) = false := by
  unfold endsHigh at *
  rw [List.getLast?_append]
  cases h : b.getLast? with
  | none => simpa using ha
  | some x => simp only [h] at hb; simpa using hb

/-- `UnpairedAt` after a prefix that does not end with a high surrogate. -/
theorem unpaired_shift (pre r : List Nat) (p : Nat) (hend : endsHigh pre = false)
    (hu : UnpairedAt r p) : UnpairedAt (pre ++ r) (pre.length + p) := by
  obtain ⟨u, rest, hd, hc⟩ := hu
  refine ⟨u, rest, by rw [List.drop_append, Nat.add_sub_cancel_left,
    List.drop_eq_nil_of_le (by omega), List.nil_append]; exact hd, ?_⟩
  rw [List.take_append, Nat.add_sub_cancel_left, List.take_of_length_le (by omega)]
  rcases hc with ⟨h1, h2⟩ | h
  · exact Or.inl ⟨h1, endsHigh_append_false _ _ hend h2⟩
  · exact Or.inr h

/-- **A1, scalar kernel.** If the scalar `convert_with_errors` reports
`SURROGATE` at `pos`, that unit is unpaired (`UnpairedAt`), no unpaired
surrogate comes before it, and `output_count` is the standard UTF-8 length
of the prefix. On success it consumed everything, with no unpaired
surrogate. -/
theorem sc8_spec : ∀ (n : Nat) (r : List Nat), r.length ≤ n →
    ((sc8 r).1 = true → UnpairedAt r (sc8 r).2.1 ∧ unp (r.take (sc8 r).2.1) = 0 ∧
      (sc8 r).2.2 = L ((r.take (sc8 r).2.1).map w8)) ∧
    ((sc8 r).1 = false → (sc8 r).2.1 = r.length ∧ unp r = 0 ∧ (sc8 r).2.2 = L (r.map w8)) := by
  intro n
  induction n with
  | zero =>
    intro r hr
    rw [List.eq_nil_of_length_eq_zero (by omega : r.length = 0)]
    exact ⟨fun h => by simp [sc8] at h, fun _ => ⟨rfl, rfl, rfl⟩⟩
  | succ n ih =>
    intro r hr
    rcases r with _ | ⟨u, r⟩
    · exact ⟨fun h => by simp [sc8] at h, fun _ => ⟨rfl, rfl, rfl⟩⟩
    by_cases hs : isSur u = false
    · have e : sc8 (u :: r) = ((sc8 r).1, 1 + (sc8 r).2.1, w8 u + (sc8 r).2.2) := by
        cases r <;> simp only [sc8, hs, ite_true]
      obtain ⟨e1, e2, e3⟩ : (sc8 (u :: r)).1 = (sc8 r).1 ∧ (sc8 (u :: r)).2.1 = 1 + (sc8 r).2.1 ∧
          (sc8 (u :: r)).2.2 = w8 u + (sc8 r).2.2 := by rw [e]; exact ⟨rfl, rfl, rfl⟩
      rw [e1, e2, e3]
      obtain ⟨i1, i2⟩ := ih r (by simp at hr; omega)
      have hh : isHigh u = false := by
        cases h : isHigh u
        · rfl
        · rw [high_sur h] at hs; cases hs
      have hend : endsHigh [u] = false := by simp [endsHigh, hh]
      have hu0 : unp [u] = 0 := by simp [unp, hs]
      refine ⟨fun h => ?_, fun h => ?_⟩
      · obtain ⟨j1, j2, j3⟩ := i1 h
        refine ⟨by simpa [Nat.add_comm] using unpaired_shift [u] r _ hend j1, ?_, ?_⟩
        · rw [show 1 + (sc8 r).2.1 = (sc8 r).2.1 + 1 by omega, List.take_succ_cons,
            ← List.singleton_append, unp_append _ _ hend, hu0, j2]
        · rw [show 1 + (sc8 r).2.1 = (sc8 r).2.1 + 1 by omega, List.take_succ_cons,
            List.map_cons, L_cons, j3]
      · obtain ⟨j1, j2, j3⟩ := i2 h
        refine ⟨by simp [j1]; omega, ?_, by simp [j3]⟩
        rw [← List.singleton_append, unp_append _ _ hend, hu0, j2]
    · simp only [Bool.not_eq_false] at hs
      rcases r with _ | ⟨v, r⟩
      · have e : sc8 [u] = (true, 0, 0) := by simp [sc8, hs]
        rw [e]
        refine ⟨fun _ => ⟨?_, rfl, rfl⟩, fun h => by cases h⟩
        refine ⟨u, [], rfl, ?_⟩
        by_cases hh : isHigh u = true
        · exact Or.inr ⟨hh, rfl⟩
        · have : isLow u = true := by simp [isSur, isHigh, isLow] at *; omega
          exact Or.inl ⟨this, rfl⟩
      by_cases hp : isHigh u = true ∧ isLow v = true
      · have e : sc8 (u :: v :: r) = ((sc8 r).1, 2 + (sc8 r).2.1, 4 + (sc8 r).2.2) := by
          simp only [sc8, hs, Bool.true_eq_false, ite_false, hp, and_self, ite_true]
        obtain ⟨e1, e2, e3⟩ : (sc8 (u :: v :: r)).1 = (sc8 r).1 ∧
            (sc8 (u :: v :: r)).2.1 = 2 + (sc8 r).2.1 ∧
            (sc8 (u :: v :: r)).2.2 = 4 + (sc8 r).2.2 := by rw [e]; exact ⟨rfl, rfl, rfl⟩
        rw [e1, e2, e3]
        obtain ⟨i1, i2⟩ := ih r (by simp at hr; omega)
        have hend : endsHigh [u, v] = false := by
          simp [endsHigh]; have := hp.2; simp [isHigh, isLow] at *; omega
        have hu0 : unp [u, v] = 0 := by simp [unp, hp.1, hp.2]
        have w2 : w8 u + w8 v = 4 := by
          rw [w8_sur u (high_sur hp.1), w8_sur v (low_sur hp.2)]
        refine ⟨fun h => ?_, fun h => ?_⟩
        · obtain ⟨j1, j2, j3⟩ := i1 h
          refine ⟨by simpa [Nat.add_comm] using unpaired_shift [u, v] r _ hend j1, ?_, ?_⟩
          · rw [show 2 + (sc8 r).2.1 = (sc8 r).2.1 + 1 + 1 by omega,
              List.take_succ_cons, List.take_succ_cons,
              show u :: v :: List.take (sc8 r).2.1 r = [u, v] ++ List.take (sc8 r).2.1 r from rfl,
              unp_append _ _ hend, hu0, j2]
          · rw [show 2 + (sc8 r).2.1 = (sc8 r).2.1 + 1 + 1 by omega,
              List.take_succ_cons, List.take_succ_cons, List.map_cons, List.map_cons, L_cons,
              L_cons, j3]; omega
        · obtain ⟨j1, j2, j3⟩ := i2 h
          refine ⟨by simp [j1]; omega, ?_, by simp [j3]; omega⟩
          rw [show u :: v :: r = [u, v] ++ r from rfl, unp_append _ _ hend, hu0, j2]
      · have e : sc8 (u :: v :: r) = (true, 0, 0) := by
          simp only [sc8, hs, Bool.true_eq_false, ite_false, hp]
        rw [e]
        refine ⟨fun _ => ⟨?_, rfl, rfl⟩, fun h => by cases h⟩
        refine ⟨u, v :: r, rfl, ?_⟩
        by_cases hh : isHigh u = true
        · have : isLow v = false := by
            cases h : isLow v
            · rfl
            · exact absurd ⟨hh, h⟩ hp
          exact Or.inr ⟨hh, this⟩
        · have : isLow u = true := by simp [isSur, isHigh, isLow] at *; omega
          exact Or.inl ⟨this, rfl⟩


/-- A `Replacement.Via` step from a scalar kernel call, with no hypothesis. -/
theorem via_scalar (xs : List Nat) (pos out : Nat) (h : Via xs pos out)
    (he : (sc8 (xs.drop pos)).1 = true) :
    Via xs (pos + (sc8 (xs.drop pos)).2.1 + 1) (out + (sc8 (xs.drop pos)).2.2 + 3) := by
  obtain ⟨a, b, c⟩ := (sc8_spec _ _ (Nat.le_refl _)).1 he
  exact Via.surrogate pos out _ _ h a b c

/-- **A1, vector part + scalar tail.** The `_with_details` wrappers
(src/westmere/implementation.cpp:748-767, src/haswell/implementation.cpp:720-739,
src/arm64/implementation.cpp:705-723, src/icelake/implementation.cpp:1361-1375)
finish with the scalar `convert_with_errors` on `buf + count`. If the vector
part stopped at `q` having converted a prefix with no unpaired surrogate,
not ending with a high surrogate, and written its standard length, an error
of the scalar tail meets the hypotheses of `Replacement.Via.surrogate`. -/
theorem compose_tail (r : List Nat) (q o : Nat) (hq : q ≤ r.length)
    (hu : unp (r.take q) = 0) (he : endsHigh (r.take q) = false)
    (ho : o = L ((r.take q).map w8)) (hs : (sc8 (r.drop q)).1 = true) :
    UnpairedAt r (q + (sc8 (r.drop q)).2.1) ∧ unp (r.take (q + (sc8 (r.drop q)).2.1)) = 0 ∧
    o + (sc8 (r.drop q)).2.2 = L ((r.take (q + (sc8 (r.drop q)).2.1)).map w8) := by
  obtain ⟨a, b, c⟩ := (sc8_spec _ _ (Nat.le_refl _)).1 hs
  have hl : (r.take q).length = q := by simp; omega
  have et : r.take (q + (sc8 (r.drop q)).2.1) = r.take q ++ (r.drop q).take (sc8 (r.drop q)).2.1 :=
    List.take_add ..
  refine ⟨?_, ?_, ?_⟩
  · have := unpaired_shift (r.take q) (r.drop q) _ he a
    rw [List.take_append_drop, hl] at this; exact this
  · rw [et, unp_append _ _ he, hu, b]
  · rw [et, List.map_append, L_append, ho, c]

/-- **The scalar `convert_with_replacement`** of the fallback kernel
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:211-290,
src/fallback/implementation.cpp:501-511): bytes written. -/
def scRepl : List Nat → Nat
  | [] => 0
  | u :: r =>
    if isSur u = false then w8 u + scRepl r
    else match r with
      | [] => 3
      | v :: r' => if isHigh u = true ∧ isLow v = true then 4 + scRepl r' else 3 + scRepl (v :: r')

/-- It writes exactly `utf8_length_from_utf16_with_replacement` bytes; its
stores are sequential, so all end inside the buffer. -/
theorem scRepl_eq (xs : List Nat) : scRepl xs = countRepl xs := by
  induction xs using wR.induct with
  | case1 => rfl
  | case2 u =>
    by_cases hs : isSur u = false
    · have hh : isHigh u = false := by simp [isSur, isHigh] at *; omega
      have hl : isLow u = false := by simp [isSur, isLow] at *; omega
      simp [scRepl, countRepl, hs, hh, hl, w8_nonsur u hs]
    · simp only [Bool.not_eq_false] at hs
      by_cases hh : isHigh u = true
      · simp [scRepl, countRepl, hs, hh]
      · have hl : isLow u = true := by simp [isSur, isHigh, isLow] at *; omega
        simp [scRepl, countRepl, hs, hh, hl]
  | case3 u v r hp ih =>
    simp only [Bool.and_eq_true] at hp
    simp [scRepl, countRepl, hp.1, hp.2, high_sur hp.1, ih]
  | case4 u v r hp ih =>
    by_cases hs : isSur u = false
    · have hh : isHigh u = false := by simp [isSur, isHigh] at *; omega
      have hl : isLow u = false := by simp [isSur, isLow] at *; omega
      simp [scRepl, countRepl, hs, hh, hl, w8_nonsur u hs, ih]
    · simp only [Bool.not_eq_false] at hs
      by_cases hh : isHigh u = true
      · have hv : isLow v = false := by
          cases h : isLow v
          · rfl
          · exact absurd (by simp [hh, h]) hp
        simp [scRepl, countRepl, hs, hh, hv, ih]
      · have hl : isLow u = true := by simp [isSur, isHigh, isLow] at *; omega
        simp [scRepl, countRepl, hs, hh, hl, ih]

/-! # Examples -/

/-- `F0 90 80` ×10, `F0 90` (`Replacement.tight32`): 11 errors, the buffer is
11 units. -/
example : lenRepl scalarV tight32 = 11 := by decide

example : msub [0xF0, 0x90, 0x41] = 2 ∧ msub [0xE0, 0x80] = 1 ∧ msub [0xED, 0xA0] = 1 ∧
    msub [0xF4, 0x8F, 0xBF] = 3 ∧ msub [0x80] = 1 := by decide

/-- Why `Subpart` needs the error position: at a valid four-byte character
`maximal_subpart` returns 4 and the decoder stores 2 units, not 1. -/
example : msub [0xF0, 0x9F, 0x98, 0x80] = 4 ∧ count [0xF0, 0x9F, 0x98, 0x80] = 2 := by decide

example : bytemask 8 [0x41, 0xD800, 0xDC00, 0xDC00, 0x41, 0x41, 0x41, 0x41, 0xD800, 0x41] =
    countRepl [0x41, 0xD800, 0xDC00, 0xDC00, 0x41, 0x41, 0x41, 0x41, 0xD800, 0x41] := by decide

end SafetyMargins.ReplacementMore
