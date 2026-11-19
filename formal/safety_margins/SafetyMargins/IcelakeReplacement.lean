import SafetyMargins.IcelakeReplacementBlock
import SafetyMargins.ReplacementRest
/-
The contracts of the conversions with replacement for Ice Lake:
`ReplacementRest.BSpec` for `convert_utf8_to_utf16{le,be}_with_errors`
(src/icelake/implementation.cpp:1124-1134) and `ReplacementRest.CVSpec` for
`convert_valid_utf8_to_utf16{le,be}` (src/icelake/implementation.cpp:1136-1158).

The models reuse the transcriptions of `IcelakeUtf8.lean`: the loop `run16`
(src/icelake/icelake_convert_utf8_to_utf16.inl.cpp:16-60) over the block
`block16`, the fourth-continuation test `guard` (lines 63-71) and the stores of
the scalar rewind `rewind`; the result of `rewind_and_convert_with_errors`
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:296-342) is
`ValidatorSound.rewindK`, whose scalar loop is `ReplacementMore.vwcF`. The two
transcriptions of the scalar decoder agree on what they store (`sc8_vwcF`,
from `sc8_vchar`; the code points of `IcelakeMask` with `&`, `<<`, `|` are those
of `ReplacementMore` with `%`, `*`, `+`: `cp2_eq`, `cp3_eq`, `cp4_eq`).

* **`iceK_bspec`.** Success: the window is valid and the count is its UTF-16
  length. The vector loop only consumes valid bytes and stores exactly their
  length (`run16_sound`, from `block_sound`); a failing block leaves the loop
  on a character boundary with the cursor at `L` of the bytes before it; the
  rewind then decodes from there and reports the first character `dchar`
  rejects. If the rewind succeeded, the C++ would return its unit count plus
  the byte offset (src/icelake/icelake_convert_utf8_to_utf16.inl.cpp:79);
  this never happens, since a failing block means invalid input
  (`run16_complete`, from `block_complete`). The stores end at
  `L` of a valid prefix (the error offset or the block start), with no slack
  needed. The fourth-continuation return is `TOO_LONG` at a continuation byte
  after a valid prefix.
* **`iceCV_cvspec`.** On valid input the loop without checks runs as the one
  with them (`run16_false_eq`), so it consumes everything, returns exactly
  `L`, and stores nothing past it.
* **The error position.** The vector loop finds errors block by block (a
  failing block is rejected whole, `run16` stops at its start), but the
  position it reports is computed by the scalar rewind from that start, after
  bytes all proved valid: it is the first byte where `dchar` fails, exactly
  (`iceK_bspec`; examples below). Blocks start on character boundaries and
  carry no state, so the late report seen for the UTF-32 validator does not
  arise here.
* **Fuel.** Every block that lets the loop continue consumes at least one
  byte (`block_sound`; without checks the loop stops on a block that does not
  advance, src/icelake/icelake_convert_utf8_to_utf16.inl.cpp:32-34), so
  `length + 1` blocks suffice and any larger fuel gives the same result (`run16_fuel`); `iceK`, `iceCV` use `length + 1`, and
  `iceKF_bspec`, `iceCVF_cvspec` hold for any fuel above the length.

What is read from the C++: the transcriptions only.
-/
namespace SafetyMargins.IcelakeReplacement
open SafetyMargins SafetyMargins.IcelakeUtf8 SafetyMargins.ValidExact
open SafetyMargins.Utf8 (w16)
set_option autoImplicit false
set_option linter.unusedSimpArgs false
set_option linter.unusedVariables false

/-! ## The scalar decoder: the two transcriptions agree -/

theorem or6 (a b : Nat) (hb : b < 64) : a <<< 6 ||| b = a * 64 + b := by
  rw [← Nat.shiftLeft_add_eq_or_of_lt (i := 6) hb, Nat.shiftLeft_eq]

theorem sh12 (a : Nat) : a <<< 12 = (a <<< 6) <<< 6 := Nat.shiftLeft_add a 6 6
theorem sh18 (a : Nat) : a <<< 18 = ((a <<< 6) <<< 6) <<< 6 := by
  rw [← sh12, ← Nat.shiftLeft_add]

theorem m63 (y : Nat) : y &&& 0x3F = y % 64 := Nat.and_two_pow_sub_one_eq_mod y 6

theorem cp2_eq (x y : Nat) : IcelakeUtf8.cp2 x y = ReplacementMore.cp2 x y := by
  unfold IcelakeUtf8.cp2 ReplacementMore.cp2
  rw [show x &&& 0x1F = x % 32 from Nat.and_two_pow_sub_one_eq_mod x 5, m63, or6 _ _ (Nat.mod_lt _ (by decide))]

theorem cp3_eq (x y z : Nat) : IcelakeUtf8.cp3 x y z = ReplacementMore.cp3 x y z := by
  unfold IcelakeUtf8.cp3 ReplacementMore.cp3
  rw [show x &&& 0x0F = x % 16 from Nat.and_two_pow_sub_one_eq_mod x 4, m63, m63, sh12,
    ← Nat.shiftLeft_or_distrib, or6 _ _ (Nat.mod_lt _ (by decide)), or6 _ _ (Nat.mod_lt _ (by decide))]
  omega

theorem cp4_eq (x y z t : Nat) : IcelakeUtf8.cp4 x y z t = ReplacementMore.cp4 x y z t := by
  unfold IcelakeUtf8.cp4 ReplacementMore.cp4
  rw [show x &&& 0x07 = x % 8 from Nat.and_two_pow_sub_one_eq_mod x 3, m63, m63, m63, sh18, sh12,
    ← Nat.shiftLeft_or_distrib, ← Nat.shiftLeft_or_distrib, or6 _ _ (Nat.mod_lt _ (by decide)),
    ← Nat.shiftLeft_or_distrib, or6 _ _ (Nat.mod_lt _ (by decide)), or6 _ _ (Nat.mod_lt _ (by decide))]
  omega
open SafetyMargins.ReplacementMore (Bytes dchar vchar vwcF vwcF_spec vchar_eq_dchar dchar_run bytes_drop)

set_option maxHeartbeats 2000000 in
theorem sc8_vchar (n b : Nat) (r : List Nat) :
    sc8 .u16 (n + 1) (b :: r) =
      match vchar (b :: r) with
      | none => 0
      | some (k, u) => u + sc8 .u16 n ((b :: r).drop k) := by
  cases hv : vchar (b :: r) with
  | none =>
    simp only
    rcases r with _ | ⟨y, _ | ⟨z, _ | ⟨t, r⟩⟩⟩ <;>
      simp only [vchar] at hv <;>
      (repeat' split at hv) <;> simp only [reduceCtorEq] at hv <;>
      simp only [sc8, ct, reduceCtorEq, ite_false, ite_true, u4, cp2_eq, cp3_eq, cp4_eq] <;>
      (repeat' split) <;> (try simp_all) <;> omega
  | some ku =>
    obtain ⟨k, u⟩ := ku
    simp only
    rcases r with _ | ⟨y, _ | ⟨z, _ | ⟨t, r⟩⟩⟩ <;>
      simp only [vchar] at hv <;>
      (repeat' split at hv) <;> simp only [reduceCtorEq, Option.some.injEq, Prod.mk.injEq] at hv <;>
      (try obtain ⟨rfl, rfl⟩ := hv) <;>
      simp only [sc8, ct, reduceCtorEq, ite_false, ite_true, u4, List.drop, cp2_eq, cp3_eq, cp4_eq] <;>
      (repeat' split) <;> (try simp_all) <;> omega

theorem sc8_vwcF : ∀ f (ys : List Nat), Bytes ys → ys.length ≤ f → ∀ g, ys.length ≤ g →
    sc8 .u16 f ys = (vwcF g ys).2.2 := by
  intro f
  induction f with
  | zero =>
    intro ys _ hl g _
    rw [List.eq_nil_of_length_eq_zero (by omega : ys.length = 0)]
    cases g <;> rfl
  | succ f ih =>
    intro ys hb hl g hg
    rcases ys with _ | ⟨b, r⟩
    · cases g <;> rfl
    obtain ⟨g', rfl⟩ : ∃ g', g = g' + 1 := ⟨g - 1, by simp at hg; omega⟩
    rw [sc8_vchar]
    simp only [vwcF]
    have hv : vchar (b :: r) = dchar (b :: r) := vchar_eq_dchar _ hb
    cases hd : vchar (b :: r) with
    | none => rfl
    | some ku =>
      obtain ⟨k, u⟩ := ku
      rw [hv] at hd
      obtain ⟨-, hk1, hk2⟩ := dchar_run _ k u hd
      simp only
      rw [ih _ (bytes_drop hb k) (by simp at hl ⊢; omega) g' (by simp at hg ⊢; omega)]

/-! ## The model of `convert_utf8_to_utf16{le,be}_with_errors` -/

/-- **`convert_utf8_to_utf16{le,be}_with_errors` on Ice Lake**, as
`ReplacementRest` reads it (src/icelake/implementation.cpp:1124-1134,
src/icelake/icelake_convert_utf8_to_utf16.inl.cpp:42-84), with `f` blocks of
fuel. The loop is `IcelakeUtf8.run16 true` (lines 49-60). On success the count
is `out - init_out` (line 82). On a failing block at `in`: the fourth-
continuation test (`IcelakeUtf8.guard`, lines 63-71) returns `TOO_LONG` at
`pos`; otherwise `rewind_and_convert_with_errors(pos, in, final_in - in, out)`
(lines 76-78) is `ValidatorSound.rewindK` for the result (a success count is
units, to which line 79 adds `pos`, as `KOut.blockErr`) and
`IcelakeUtf8.rewind` for its stores at the cursor. Every error code these
paths return (`TOO_LONG`, `TOO_SHORT`, `OVERLONG`, `SURROGATE`, `TOO_LARGE`,
`HEADER_BITS`, include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:185-273,
:315, :335) is a validation error (src/utf8_to_utf16_with_replacement.inl.h:6-18),
so `verr` is `!ok`. `se` is the end of the stores. -/
def iceKF (f : Nat) (win : List Nat) : ReplacementRest.BRes :=
  let r := run16 true win f 0 0
  if r.ok then ⟨true, false, r.out, r.wEnd⟩
  else if guard win r.pos then ⟨false, true, r.pos, r.wEnd⟩
  else
    let rk := ValidatorSound.rewindK win r.pos
    ⟨rk.1, !rk.1, if rk.1 then rk.2 + r.pos else rk.2,
      max r.wEnd (r.out + rewind .u16 win r.pos r.pos)⟩

/-- The kernel with enough fuel: every block that returns `true` consumes at
least one byte (`block_sound`), so `length + 1` blocks end the loop
(`run16_fuel`: any larger fuel gives the same result). -/
def iceK (win : List Nat) : ReplacementRest.BRes := iceKF (win.length + 1) win

/-- `convert_valid_utf8_to_utf16{le,be}` (src/icelake/implementation.cpp:1136-1158):
`(returned count, end of stores)`; the count is 0 when the loop returns a null
output (icelake_convert_utf8_to_utf16.inl.cpp:32-38). -/
def iceCVF (f : Nat) (v : List Nat) : Nat × Nat :=
  let r := run16 false v f 0 0
  (if r.ok then r.out else 0, r.wEnd)

def iceCV (v : List Nat) : Nat × Nat := iceCVF (v.length + 1) v

/-! ## The loop -/

section Loop
variable {xs : List Nat}

theorem byte_view (hb : Bytes xs) (pos p : Nat) : byte (view xs pos) p < 256 := by
  unfold byte
  rw [List.getD_eq_getElem?_getD]
  cases h : (view xs pos)[p]? with
  | none => simp
  | some v =>
    simp only [Option.getD_some]
    have hm := List.mem_of_getElem? h
    simp only [view] at hm
    exact hb v (List.mem_of_mem_drop (List.mem_of_mem_take hm))

theorem view_pos {pos : Nat} (h : pos < xs.length) : 0 < (view xs pos).length := by
  rw [view_length]; omega

theorem Lw_mono {a b : Nat} (h : a ≤ b) : L ((xs.take a).map w16) ≤ L ((xs.take b).map w16) :=
  L_take_mono xs w16 h

/-- **The validating loop is sound.** From a character boundary with the
cursor at `L` of the bytes before it, the loop stays on boundaries with the
cursor at `L` of what it consumed (each passing block is valid and stores
exactly its `L`, `block_sound`), stores nothing past the cursor, and with
enough fuel it stops only at the end of the input or at a failing block. -/
theorem run16_sound (hb : Bytes xs) : ∀ f pos out, Bd xs pos → out = L ((xs.take pos).map w16) →
    pos ≤ xs.length →
    pos ≤ (run16 true xs f pos out).pos ∧ Bd xs (run16 true xs f pos out).pos ∧
    (run16 true xs f pos out).out = L ((xs.take (run16 true xs f pos out).pos).map w16) ∧
    (run16 true xs f pos out).wEnd = (run16 true xs f pos out).out ∧
    (run16 true xs f pos out).pos ≤ xs.length ∧
    (xs.length < f + pos → (run16 true xs f pos out).ok = true →
      (run16 true xs f pos out).pos = xs.length) := by
  intro f
  induction f with
  | zero =>
    intro pos out hbd ho hp
    simp only [run16]
    exact ⟨Nat.le_refl _, hbd, ho, trivial, hp, fun h => by omega⟩
  | succ f ih =>
    intro pos out hbd ho hp
    unfold run16
    by_cases hlt : pos < xs.length
    · rw [ite_eq_left hlt]
      have hl := view_length (xs := xs) pos
      cases hbk : block16 true (view xs pos) with
      | none =>
        simp only
        exact ⟨Nat.le_refl _, hbd, ho, trivial, hp, fun _ h => by cases h⟩
      | some cn =>
        obtain ⟨c, n⟩ := cn
        obtain ⟨hc, -, hv, hn, h1⟩ := block_sound (by omega) (byte_view hb pos) hbk
        have hc1 := h1 (view_pos hlt)
        simp only [Bool.true_eq_false, false_and, ite_false]
        have hbd' : Bd xs (pos + c) := by
          unfold Bd; rw [List.take_add, run_append, hbd, ← view_take _ _ (by omega)]; exact hv
        have ho' : out + n = L ((xs.take (pos + c)).map w16) := by
          rw [Lt_add, ← view_take _ _ (by omega), ← hn, ho]
        obtain ⟨r1, r2, r3, r4, r5, r6⟩ := ih (pos + c) (out + n) hbd' ho' (by omega)
        have hm := Lw_mono (xs := xs) r1
        refine ⟨by omega, r2, r3, ?_, r5, fun hf hok => r6 (by omega) hok⟩
        rw [r4]; omega
    · rw [ite_eq_right hlt]
      simp only
      exact ⟨Nat.le_refl _, hbd, ho, trivial, hp, fun _ _ => by omega⟩

/-- One passing block from a boundary: the next block starts on a boundary,
the cursor advances by `L` of the consumed bytes, and at least one byte is
consumed. -/
theorem block_next (hb : Bytes xs) {pos c n : Nat} (hbd : Bd xs pos) (hlt : pos < xs.length)
    (h : block16 true (view xs pos) = some (c, n)) :
    Bd xs (pos + c) ∧ n = L (((xs.drop pos).take c).map w16) ∧ 1 ≤ c ∧ pos + c ≤ xs.length := by
  have hl := view_length (xs := xs) pos
  obtain ⟨hc, -, hv, hn, h1⟩ := block_sound (by omega) (byte_view hb pos) h
  refine ⟨?_, by rw [hn, view_take _ _ (by omega)], h1 (view_pos hlt), by omega⟩
  unfold Bd; rw [List.take_add, run_append, hbd, ← view_take _ _ (by omega)]; exact hv

/-- **On valid UTF-8 no block fails** (`block_complete`). -/
theorem run16_complete (hv : Valid8 xs) : ∀ f pos out, Bd xs pos → pos ≤ xs.length →
    (run16 true xs f pos out).ok = true := by
  have hb : Bytes xs := valid_lt256 hv
  intro f
  induction f with
  | zero => intro pos out _ _; rfl
  | succ f ih =>
    intro pos out hbd hp
    unfold run16
    by_cases hlt : pos < xs.length
    · rw [ite_eq_left hlt]
      have hl := view_length (xs := xs) pos
      obtain ⟨hv1, hv2⟩ := view_valid hv hbd
      have he := block_complete (blk := view xs pos) (by omega) hv1 hv2
      obtain ⟨c, n, hf⟩ := block16_false_some (view xs pos)
      have ht : block16 true (view xs pos) = some (c, n) := he.trans hf
      rw [ht]
      obtain ⟨hbd', -, -, hle⟩ := block_next hb hbd hlt ht
      simp only [Bool.true_eq_false, false_and, ite_false]
      exact ih (pos + c) (out + n) hbd' hle
    · rw [ite_eq_right hlt]

/-- On valid UTF-8, `convert_valid` (checks compiled out) runs exactly as the
validating loop: every block gives the same result (`block_complete`) and
consumes at least one byte, so the no-progress stop never fires. -/
theorem run16_false_eq (hv : Valid8 xs) : ∀ f pos out, Bd xs pos → pos ≤ xs.length →
    run16 false xs f pos out = run16 true xs f pos out := by
  have hb : Bytes xs := valid_lt256 hv
  intro f
  induction f with
  | zero => intro pos out _ _; rfl
  | succ f ih =>
    intro pos out hbd hp
    unfold run16
    by_cases hlt : pos < xs.length
    · rw [ite_eq_left hlt, ite_eq_left hlt]
      have hl := view_length (xs := xs) pos
      obtain ⟨hv1, hv2⟩ := view_valid hv hbd
      have he := block_complete (blk := view xs pos) (by omega) hv1 hv2
      obtain ⟨c, n, hf⟩ := block16_false_some (view xs pos)
      have ht : block16 true (view xs pos) = some (c, n) := he.trans hf
      rw [ht, hf]
      obtain ⟨hbd', -, h1, hle⟩ := block_next hb hbd hlt ht
      simp only [Bool.true_eq_false, false_and, ite_false, show c ≠ 0 by omega, and_false]
      rw [ih (pos + c) (out + n) hbd' hle]
    · rw [ite_eq_right hlt, ite_eq_right hlt]

/-- **The fuel.** Each block that lets the loop go on consumes at least one
byte (with the checks, `block_sound`; without, the loop stops otherwise,
icelake_convert_utf8_to_utf16.inl.cpp:32-34), so any fuel above the bytes left
gives the same result: the C++ loop's. -/
theorem run16_fuel (v : Bool) (hb : Bytes xs) : ∀ f g pos out, xs.length < f + pos →
    xs.length < g + pos → run16 v xs f pos out = run16 v xs g pos out := by
  intro f
  induction f with
  | zero =>
    intro g pos out hf hg
    cases g with
    | zero => rfl
    | succ g => unfold run16; rw [ite_eq_right (by omega)]
  | succ f ih =>
    intro g pos out hf hg
    cases g with
    | zero => unfold run16; rw [ite_eq_right (by omega)]
    | succ g =>
      unfold run16
      by_cases hlt : pos < xs.length
      · rw [ite_eq_left hlt, ite_eq_left hlt]
        cases hbk : block16 v (view xs pos) with
        | none => rfl
        | some cn =>
          obtain ⟨c, n⟩ := cn
          simp only
          by_cases hs : v = false ∧ c = 0
          · rw [ite_eq_left hs, ite_eq_left hs]
          · rw [ite_eq_right hs, ite_eq_right hs]
            have hc1 : 1 ≤ c := by
              cases v with
              | false => simp at hs; omega
              | true =>
                have hl := view_length (xs := xs) pos
                exact (block_sound (by omega) (byte_view hb pos) hbk).2.2.2.2 (view_pos hlt)
            rw [ih g (pos + c) (out + n) (by omega) (by omega)]
      · rw [ite_eq_right hlt, ite_eq_right hlt]

end Loop

/-! ## The error path -/

section Err
variable {xs : List Nat}

/-- The rewind from a block start allowed by `Good` (the guard having not
fired): it decodes from `pos`, or stores nothing (as in `rewind16_safe`). -/
theorem rewind_cases {pos : Nat} (hg : Good xs pos) (hp : pos < xs.length)
    (hgu : guard xs pos = false) :
    (Validator.isCont (xs.getD pos 0) = false ∧
      rewind .u16 xs pos pos = sc8 .u16 xs.length (xs.drop pos)) ∨
    rewind .u16 xs pos pos = 0 := by
  unfold rewind
  by_cases hc : Validator.isCont (xs.getD pos 0) = false
  · left
    refine ⟨hc, ?_⟩
    have : rwSearch xs pos 0 pos = some 0 := by
      unfold rwSearch; rw [ite_eq_left (by rw [Nat.sub_zero]; exact hc)]
    rw [this]; simp
  · right
    have hc' : Validator.isCont (xs.getD pos 0) = true := by simpa using hc
    rcases hg with h0 | hl | h | ⟨h1, ha⟩ | ⟨h64, a1, a2, a3⟩
    · subst h0
      have : rwSearch xs 0 0 0 = none := by
        unfold rwSearch; rw [ite_eq_right (by rw [Nat.sub_zero, hc']; decide)]
      rw [this]
    · omega
    · exact absurd h hc
    · obtain ⟨r, hr⟩ : ∃ r, pos = r + 1 := ⟨pos - 1, by omega⟩
      have hna : Validator.isCont (xs.getD (pos - 1) 0) = false := by
        unfold Validator.isCont; simp only [Bool.and_eq_false_iff, decide_eq_false_iff_not]; omega
      have : rwSearch xs pos 0 pos = some 1 := by
        subst hr
        rw [rwSearch_succ, Nat.sub_zero, ite_eq_right (by rw [hc']; decide)]
        exact rwSearch_eq _ _ _ hna r
      rw [this]
      simp only [show 0 < 1 from Nat.one_pos, true_and, ha, ite_true]
    · have : IcelakeUtf8.guard xs pos = true := by
        unfold IcelakeUtf8.guard
        simp only [Bool.and_eq_true, decide_eq_true_eq]
        exact ⟨⟨⟨⟨⟨hp, hc'⟩, h64⟩, a1⟩, a2⟩, a3⟩
      rw [this] at hgu; exact absurd hgu (by decide)

theorem rewindK_lead {t : Nat} (hc : Validator.isCont (xs.getD t 0) = false) :
    ValidatorSound.rewindK xs t =
      if (vwcF (xs.drop t).length (xs.drop t)).1 then (true, (vwcF (xs.drop t).length (xs.drop t)).2.2)
      else (false, t + (vwcF (xs.drop t).length (xs.drop t)).2.1) := by
  have : ValidatorSound.findBack xs t 0 t = some 0 := by
    rcases t with _ | n <;> simp only [ValidatorSound.findBack, Nat.sub_zero, hc, ite_true]
  unfold ValidatorSound.rewindK
  rw [this]
  simp

end Err

/-! ## The contracts -/

/-- **`BSpec` for Ice Lake `convert_utf8_to_utf16{le,be}_with_errors`**, for
any fuel above the window length. -/
theorem iceKF_bspec (F : List Nat → Nat) (hF : ∀ w, w.length < F w) :
    ReplacementRest.BSpec (fun w => iceKF (F w) w) := by
  intro win hb hne
  have hS := run16_sound (xs := win) hb (F win) 0 0 (by simp [Bd, run]) (by simp [L]) (Nat.zero_le _)
  have hT := run16_true (xs := win) (F win) 0 0 (Or.inl rfl) (by simp [L]) (Nat.zero_le _)
  have hC : Valid8 win → (run16 true win (F win) 0 0).ok = true :=
    fun hv => run16_complete hv (F win) 0 0 (by simp [Bd, run]) (Nat.zero_le _)
  obtain ⟨-, s2, s3, s4, s5, s6⟩ := hS
  obtain ⟨-, -, hG, hT4, -⟩ := hT
  generalize hr : run16 true win (F win) 0 0 = r at s2 s3 s4 s5 s6 hG hT4 hC
  have hFw := hF win
  simp only [iceKF, hr]
  by_cases hok : r.ok = true
  · have hpos := s6 (by omega) hok
    rw [hpos] at s2 s3
    unfold Bd at s2
    rw [List.take_of_length_le (Nat.le_refl _)] at s2 s3
    rw [ite_eq_left hok]
    dsimp only
    refine ⟨fun _ => ⟨s2, s3⟩, fun h => by simp at h, ⟨win.length, Nat.le_refl _, ?_, ?_⟩⟩
    · rw [List.take_of_length_le (Nat.le_refl _)]; exact s2
    · rw [List.take_of_length_le (Nat.le_refl _), s4, s3]; omega
  have hok' : r.ok = false := by simpa using hok
  have hpos := hT4 hok'
  simp only [hok', Bool.false_eq_true, ite_false]
  by_cases hg : IcelakeUtf8.guard win r.pos = true
  · rw [ite_eq_left hg]
    dsimp only
    refine ⟨fun h => by simp at h, fun _ _ _ => ⟨s2, ?_⟩, ⟨r.pos, s5, s2, by rw [s4, s3]; omega⟩⟩
    have hc : Validator.isCont (win.getD r.pos 0) = true := by
      unfold IcelakeUtf8.guard at hg; simp only [Bool.and_eq_true] at hg; exact hg.1.1.1.1.2
    rw [List.drop_eq_getElem_cons hpos]
    have : win[r.pos] = win.getD r.pos 0 := by simp [List.getD_eq_getElem?_getD, hpos]
    rw [this]
    exact ValidatorSound.cont_dchar _ _ hc
  have hg' : IcelakeUtf8.guard win r.pos = false := by simpa using hg
  simp only [hg', Bool.false_eq_true, ite_false]
  obtain ⟨k1, k2⟩ := ValidatorSound.rewindK_spec win hb r.pos hpos s2
  cases hk : (ValidatorSound.rewindK win r.pos).1
  · obtain ⟨k3, k4, k5⟩ := k2 hk
    simp only [Bool.false_eq_true, ite_false, Bool.not_false]
    refine ⟨fun h => by simp at h, fun _ _ _ => ⟨k4, k5⟩, ?_⟩
    rcases rewind_cases hG hpos hg' with ⟨hc, hrw⟩ | hrw
    · -- the scalar decoder runs from `pos` and stops at the error
      have hd := rewindK_lead (xs := win) hc
      have hbd : Bytes (win.drop r.pos) := bytes_drop hb r.pos
      obtain ⟨-, v2⟩ := vwcF_spec (win.drop r.pos).length (win.drop r.pos) hbd (Nat.le_refl _)
      cases hv : (vwcF (win.drop r.pos).length (win.drop r.pos)).1
      · obtain ⟨-, -, hu, -⟩ := v2 hv
        rw [hv] at hd
        simp only [Bool.false_eq_true, ite_false] at hd
        rw [hd] at k3 k4
        refine ⟨_, Nat.le_of_lt k3, k4, ?_⟩
        rw [hrw, sc8_vwcF _ _ hbd (by simp) _ (Nat.le_refl _), hu, Lt_add, s4, s3]
        omega
      · rw [hv] at hd; simp only [ite_true] at hd; rw [hd] at hk; cases hk
    · refine ⟨r.pos, s5, s2, ?_⟩
      rw [hrw, s4, s3]; omega
  · exfalso
    have := hC (k1 hk).1
    rw [hok'] at this; cases this

/-- **`BSpec` holds for Ice Lake.** -/
theorem iceK_bspec : ReplacementRest.BSpec iceK :=
  iceKF_bspec (fun w => w.length + 1) (fun w => by omega)

/-- **`CVSpec` for Ice Lake `convert_valid_utf8_to_utf16{le,be}`**, for any
fuel above the input length. -/
theorem iceCVF_cvspec (F : List Nat → Nat) (hF : ∀ w, w.length < F w) :
    ReplacementRest.CVSpec (fun v => iceCVF (F v) v) := by
  intro v hv
  have hb : Bytes v := valid_lt256 hv
  have hbd0 : Bd v 0 := by simp [Bd, run]
  have he := run16_false_eq hv (F v) 0 0 hbd0 (Nat.zero_le _)
  have hok := run16_complete hv (F v) 0 0 hbd0 (Nat.zero_le _)
  obtain ⟨-, -, s3, s4, -, s6⟩ := run16_sound (xs := v) hb (F v) 0 0 hbd0 (by simp [L]) (Nat.zero_le _)
  have hpos := s6 (by have := hF v; omega) hok
  rw [hpos, List.take_of_length_le (Nat.le_refl _)] at s3
  simp only [iceCVF, he, hok, ite_true]
  exact ⟨s3, by rw [s4, s3]; exact Nat.le_refl _⟩

/-- **`CVSpec` holds for Ice Lake.** -/
theorem iceCV_cvspec : ReplacementRest.CVSpec iceCV :=
  iceCVF_cvspec (fun w => w.length + 1) (fun w => by omega)


/-! ## Examples -/

/-- A surrogate (`ED A0 80`) after an ASCII byte and a `€`: the vector loop
fails the whole block at byte 0 (range check), the rewind decodes from there
and reports the surrogate at byte 4. -/
example : (run16 true [0x41, 0xE2, 0x82, 0xAC, 0xED, 0xA0, 0x80] 8 0 0).pos = 0 ∧
    (iceK [0x41, 0xE2, 0x82, 0xAC, 0xED, 0xA0, 0x80]).ok = false ∧
    (iceK [0x41, 0xE2, 0x82, 0xAC, 0xED, 0xA0, 0x80]).cnt = 4 := by
  decide +kernel

/-- An overlong lead in the second block: the first block (64 ASCII bytes)
passes, the error is at byte 70. -/
example : (iceK (List.replicate 70 0x41 ++ [0xC0, 0x80])).ok = false ∧
    (iceK (List.replicate 70 0x41 ++ [0xC0, 0x80])).cnt = 70 := by
  decide +kernel

/-- Valid input (`é`, `€`, U+1F600): success with the UTF-16 length; the
`convert_valid` model agrees. -/
example : (iceK [0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80]).ok = true ∧
    (iceK [0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80]).cnt = 4 ∧
    iceCV [0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80] = (4, 4) := by
  decide +kernel

end SafetyMargins.IcelakeReplacement
