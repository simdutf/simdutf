import SafetyMargins.ReplacementLengths
import SafetyMargins.RvvExact
/-
rvv UTF-16 → UTF-8 with replacement: `VKSpec'` discharged, end to end.

The rvv kernel `rvv_utf16_to_utf8_with_details`
(src/rvv/rvv_utf16_to.inl.cpp:74-217) is transcribed as a function of the
input, for every `vl` that `vsetvl` may return (`RvvExact.vlOf pick`, any
`pick`). One iteration is `RvvExact.utf16to8Step` (already the transcription
of the body, :83-212, used by `RvvExact.utf16to8_safe`); `loop` runs it
while `n > 0` (:82) and returns at its `SURROGATE` returns (:189-199) or at
the end (:215-216). All three byte-flip variants (`NONE`, `ZVBB`, `V`)
differ only in how the loaded lanes and `src[0]`, `src[1]` are swapped
(:86, :180, :196): the model is on the units after the flip.

* `rvvFull pick`: the `full_result` (error, `input_count = src - srcBeg`,
  `output_count = dst - dstBeg`) and the end of the stores.
* `rvvVK pick`: what `convert_utf16{le,be}_to_utf8_with_errors` returns
  (:231-243), the `full_result` converted to `result`
  (include/simdutf/error.h:117-123: `output_count` on success,
  `input_count` on error), as `ReplacementRest.KR`.
* `rvvVK_spec : ReplacementLengths.VKSpec' (rvvVK pick)`, from
  `rvvFull_spec`: stores end within `L (ys.map w8)`, on success the output
  count is exactly `L (ys.map w8)` and the whole input is consumed, on error
  `input_count < ys.length`, the units before it have no unpaired surrogate,
  the unit at it is unpaired (`Replacement.UnpairedAt`), and `output_count`
  is the standard length of the units before it.
* `rvvLen pickL`: the rvv `utf8_length_from_utf16`
  (src/rvv/rvv_length_from.inl.cpp:86-103), `LengthsRvv.rvvSum`;
  `SLSpec'_rvv` (indeed `SLSpec`, `LengthsRvv.rvv_lengths`).
* End to end: `replSafe_rvv` (`convert_utf16_to_utf8_with_replacement_safe`,
  src/implementation.cpp:2494-2539) and `dSafe_rvv`
  (`convert_utf16_to_utf8_safe_with_details`, src/implementation.cpp:2028-2091),
  for every `pick` of the kernel, every `pickL` of the length function, and
  every ASCII probe of the scalar tail (`cwe`, `crs` meet `FSpec` for any
  probe, so `ProbeOK` is not needed: the rvv kernel has no scalar tail).
* `via_rvvRepl`: `convert_with_replacement_via`
  (src/generic/utf16_to_utf8/utf16_to_utf8_with_replacement.h:13-31) over
  the rvv kernel (src/rvv/implementation.cpp:125-147): every store ends
  inside `utf8_length_from_utf16_with_replacement` and the returned count is
  exactly that length.

No hypothesis is left beyond those `RvvExact` lists for the transcription
(RVV semantics of `vsetvl`, `vcompress`, `vcpop`, unit-stride stores).
-/
namespace SafetyMargins.RvvReplacement16
open SafetyMargins SafetyMargins.Replacement SafetyMargins.ReplacementMore
open SafetyMargins.RvvExact (utf16to8Step utf16to8_fits pairs vlOf isSurV)
open SafetyMargins.IcelakeReal (hiS loS)
set_option autoImplicit false

/-! ## The model -/

/-- The end of the stores of one iteration, relative to its cursor. -/
def stEnd : List (Nat × Nat) → Nat
  | [] => 0
  | p :: r => max (p.1 + p.2) (stEnd r)

/-- The loop `for (...; n > 0;)` (src/rvv/rvv_utf16_to.inl.cpp:82) with fuel:
`(returned SURROGATE, src - srcBeg, dst - dstBeg, end of stores)`. Each
iteration is `utf16to8Step` on the remaining units; a `SURROGATE` return
(:189-199) happens after the stores of its iteration, with `src` and `dst`
at the failed unit (`It.c`, `It.adv`); the loop ends when `n` reaches 0
(:215-216). -/
def loop (pick : Nat → Nat) (ys : List Nat) : Nat → Nat → Nat → Nat → Bool × Nat × Nat × Nat
  | 0, pos, out, hi => (false, pos, out, hi)
  | f + 1, pos, out, hi =>
    if (ys.drop pos).length = 0 then (false, pos, out, hi)
    else
      let it := utf16to8Step pick () (ys.drop pos)
      let hi1 := max hi (out + stEnd it.st)
      if it.stop then (true, pos + it.c, out + it.adv, hi1)
      else loop pick ys f (pos + it.c) (out + it.adv) hi1

/-- **`rvv_utf16_to_utf8_with_details`** (src/rvv/rvv_utf16_to.inl.cpp:74-217):
the `full_result` (`err` 1 = `SURROGATE`, 0 = `SUCCESS`, `inp`, `outp`) and
the end of its stores (`hi`). Every non-returning iteration consumes a unit
(`step_facts`), so `ys.length + 1` iterations suffice (`rvvFull_spec`). -/
def rvvFull (pick : Nat → Nat) (ys : List Nat) : ReplacementRest.FR :=
  let r := loop pick ys (ys.length + 1) 0 0 0
  ⟨if r.1 then 1 else 0, r.2.1, r.2.2.1, r.2.2.2⟩

/-- **`convert_utf16{le,be}_to_utf8_with_errors`** (src/rvv/rvv_utf16_to.inl.cpp:231-243):
the `full_result` converted to `result` (include/simdutf/error.h:117-123):
`output_count` on success, `input_count` on error. -/
def rvvVK (pick : Nat → Nat) (ys : List Nat) : ReplacementRest.KR :=
  let r := rvvFull pick ys
  if r.err = 0 then ⟨0, r.outp, r.hi⟩ else ⟨r.err, r.inp, r.hi⟩

/-! ## Lemmas on units -/

theorem high_sur (u : Nat) (h : isHigh u = true) : isSur u = true := by
  simp [isHigh, isSur] at *; omega

theorem unp_nosur : ∀ l : List Nat, (∀ x ∈ l, isSur x = false) → unp l = 0
  | [], _ => rfl
  | [u], h => by simp [unp, h u (by simp)]
  | u :: v :: r, h => by
    have hu := h u (by simp)
    have hh : isHigh u = false := by
      cases e : isHigh u
      · rfl
      · rw [high_sur u e] at hu; cases hu
    have ih := unp_nosur (v :: r) (fun x hx => h x (List.mem_cons_of_mem _ hx))
    rw [unp]; simp [hh, hu, ih]

theorem sur_of_V (v : Nat) (h : isSurV v = false) : isSur v = false := by
  simp [isSurV, isSur] at *; omega

theorem pairs_le : ∀ zs : List Nat,
    2 * (pairs zs).1 ≤ zs.length ∧ ((pairs zs).2 = true → 2 * (pairs zs).1 < zs.length)
  | [] => by simp [pairs]
  | [w] => by simp [pairs]
  | w :: w2 :: r => by
    have ih := pairs_le r
    unfold pairs
    by_cases h1 : isSurV w = true
    · by_cases h2 : hiS w = true
      · by_cases h3 : loS w2 = true
        · simp only [h1, h2, h3, Bool.not_true, Bool.false_eq_true, ite_false, List.length_cons]
          exact ⟨by omega, fun h => by have := ih.2 h; omega⟩
        · simp [h1, h2, h3]
      · simp [h1, h2]
    · simp [h1]

theorem pairs_unp : ∀ zs : List Nat, unp (zs.take (2 * (pairs zs).1)) = 0
  | [] => by simp [pairs, unp]
  | [w] => by simp [pairs, unp]
  | w :: w2 :: r => by
    unfold pairs
    by_cases h1 : isSurV w = true
    · by_cases h2 : hiS w = true
      · by_cases h3 : loS w2 = true
        · simp only [h1, h2, h3, Bool.not_true, Bool.false_eq_true, ite_false]
          rw [show 2 * ((pairs r).1 + 1) = 2 * (pairs r).1 + 1 + 1 by omega]
          simp only [List.take_succ_cons]
          have hh : isHigh w = true := by simp [hiS, isHigh] at h2 ⊢; omega
          have hl : isLow w2 = true := by simp [loS, isLow] at h3 ⊢; omega
          rw [unp]; simp [hh, hl, pairs_unp r]
        · simp [h1, h2, h3, unp]
      · simp [h1, h2, unp]
    · simp [h1, unp]

/-- Where `pairs` returns, the unit is unpaired: a low surrogate, or a high
one at the end or before a unit that is not low. -/
theorem pairs_stop : ∀ zs : List Nat, (pairs zs).2 = true →
    ∃ u rest, zs.drop (2 * (pairs zs).1) = u :: rest ∧
      ((isLow u = true ∧ isHigh u = false) ∨ (isHigh u = true ∧ startsLow rest = false))
  | [] => by simp [pairs]
  | [w] => by
    intro h
    simp only [pairs] at h
    refine ⟨w, [], by simp [pairs], ?_⟩
    by_cases hh : isHigh w = true
    · exact Or.inr ⟨hh, rfl⟩
    · left; simp [isSurV, isHigh, isLow] at h hh ⊢; omega
  | w :: w2 :: r => by
    intro h
    unfold pairs at h ⊢
    by_cases h1 : isSurV w = true
    · by_cases h2 : hiS w = true
      · by_cases h3 : loS w2 = true
        · simp only [h1, h2, h3, Bool.not_true, Bool.false_eq_true, ite_false] at h ⊢
          obtain ⟨u, rest, hd, hc⟩ := pairs_stop r h
          refine ⟨u, rest, ?_, hc⟩
          rw [show 2 * ((pairs r).1 + 1) = 2 * (pairs r).1 + 1 + 1 by omega]
          simpa using hd
        · simp only [h1, h2, h3, Bool.not_true, Bool.not_false, Bool.false_eq_true, ite_false,
            ite_true]
          refine ⟨w, w2 :: r, by simp, Or.inr ⟨?_, ?_⟩⟩
          · simp [hiS, isHigh] at h2 ⊢; omega
          · simp [startsLow, loS, isLow] at h3 ⊢; omega
      · simp only [h1, h2, Bool.not_true, Bool.not_false, Bool.false_eq_true, ite_false, ite_true]
        refine ⟨w, w2 :: r, by simp, Or.inl ⟨?_, ?_⟩⟩
        · simp [hiS, isSurV, isLow] at h1 h2 ⊢; omega
        · simp [hiS, isHigh] at h2 ⊢; omega
    · simp [h1] at h

theorem pairs_start (y : Nat) (rest : List Nat) (h : isSurV y = true) :
    (pairs (y :: rest)).2 = true ∨ 1 ≤ (pairs (y :: rest)).1 := by
  cases rest with
  | nil => simp [pairs, h]
  | cons w2 r =>
    unfold pairs
    by_cases h2 : hiS y = true
    · by_cases h3 : loS w2 = true
      · simp [h, h2, h3]
      · simp [h, h2, h3]
    · simp [h, h2]

theorem w8_lt80 (v : Nat) (h : v ≤ 0x7F) : w8 v = 1 := by
  unfold w8 Arch.Icelake.w8; simp; omega

/-! ## One iteration -/

/-- **One iteration of the loop** (`utf16to8Step`) on `n > 0` units: it
consumes at most the input, and a unit unless it returns; the units it
consumes have no unpaired surrogate; its cursor advance is the standard
length of what it consumed; a `SURROGATE` return is at an unpaired unit. -/
theorem step_facts (pick : Nat → Nat) (ys : List Nat) (hne : 0 < ys.length) (hu : U16 ys) :
    (utf16to8Step pick () ys).c ≤ ys.length ∧
    ((utf16to8Step pick () ys).stop = false → 1 ≤ (utf16to8Step pick () ys).c) ∧
    unp (ys.take (utf16to8Step pick () ys).c) = 0 ∧
    (utf16to8Step pick () ys).adv = L ((ys.take (utf16to8Step pick () ys).c).map w8) ∧
    ((utf16to8Step pick () ys).stop = true →
      ∃ u rest, ys.drop (utf16to8Step pick () ys).c = u :: rest ∧
        ((isLow u = true ∧ isHigh u = false) ∨ (isHigh u = true ∧ startsLow rest = false))) := by
  have hvl := RvvExact.vlOf_le pick ys.length
  have hv1 : 1 ≤ vlOf pick ys.length := by unfold vlOf; omega
  unfold utf16to8Step; dsimp only
  generalize hvv : vlOf pick ys.length = vl at hvl hv1
  have hlen : (ys.take vl).length = vl := RvvExact.take_vl_len ys hvl
  split
  · rename_i h1
    have hall : ∀ v ∈ ys.take vl, v ≤ 0x7F := fun v hv => by
      have := List.all_eq_true.mp h1 v hv; simpa using this
    refine ⟨hvl, fun _ => hv1, unp_nosur _ (fun x hx => ?_), ?_, fun h => by cases h⟩
    · have := hall x hx; simp [isSur]; omega
    · rw [RvvExact.L_map_congr _ _ (fun _ => 1) (fun v hv => w8_lt80 v (hall v hv)), RvvExact.L_map_one, hlen]
  split
  · rename_i _ h12
    have hall : ∀ v ∈ ys.take vl, v ≤ 0x7FF := fun v hv => by
      have := List.all_eq_true.mp h12 v hv; simpa using this
    refine ⟨hvl, fun _ => hv1, unp_nosur _ (fun x hx => ?_), ?_, fun h => by cases h⟩
    · have := hall x hx; simp [isSur]; omega
    · apply RvvExact.L_map_congr
      intro v hv
      have hv' := hall v hv
      rw [RvvExact.lane12_eq 0x3F1F v (by decide) (by omega), show w8 v = RvvExact.w8 v from rfl,
        RvvExact.w8_eq]
      simp [RvvExact.ind]; omega
  · generalize hpre : (ys.take vl).takeWhile (fun v => !isSurV v) = pre
    have htp : ys.take pre.length = pre := by
      rw [← hpre]; exact RvvExact.take_takeWhile ys vl _
    have hpl : pre.length ≤ vl := by rw [← hpre]; exact RvvExact.takeWhile_len_le ys vl _
    have hmem : ∀ v ∈ pre, isSurV v = false := fun v hv => by
      have := RvvExact.mem_takeWhile (hpre ▸ hv); simpa using this
    obtain ⟨q1, q2⟩ := pairs_le (ys.drop pre.length)
    simp only [List.length_drop] at q1 q2
    have htk : ys.take (pre.length + 2 * (pairs (ys.drop pre.length)).1)
        = pre ++ (ys.drop pre.length).take (2 * (pairs (ys.drop pre.length)).1) := by
      rw [List.take_add, htp]
    dsimp only
    refine ⟨by omega, fun hs => ?_, ?_, ?_, fun hs => ?_⟩
    · -- a unit is consumed
      by_cases hp0 : pre.length = 0
      · obtain ⟨y, rest, hy⟩ : ∃ y rest, ys = y :: rest := by
          cases ys with
          | nil => simp at hne
          | cons y rest => exact ⟨y, rest, rfl⟩
        have hyS : isSurV y = true := by
          have hpe : pre = [] := List.eq_nil_of_length_eq_zero hp0
          rw [hpe, hy] at hpre
          obtain ⟨vl', rfl⟩ : ∃ vl', vl = vl' + 1 := ⟨vl - 1, by omega⟩
          simp only [List.take_succ_cons, List.takeWhile_cons] at hpre
          split at hpre
          · simp at hpre
          · rename_i hn; simpa using hn
        rw [hp0, List.drop_zero, hy] at hs ⊢
        rcases pairs_start y rest hyS with h | h
        · rw [h] at hs; cases hs
        · omega
      · omega
    · rw [htk]
      exact ReplacementLengths.unp_append _ _ (unp_nosur pre (fun x hx => sur_of_V x (hmem x hx)))
        (pairs_unp _)
    · rw [RvvExact.L_map_take_add, htp, RvvExact.pairs_L]
      congr 1
      apply RvvExact.L_map_congr
      intro v hv
      have hb : v < 65536 := hu v (List.mem_of_mem_take (RvvExact.mem_takeWhile_sub (hpre ▸ hv)))
      rw [RvvExact.lane123_eq v hb, RvvExact.w8_lane v (hmem v hv)]
    · obtain ⟨u, rest, hd, hc⟩ := pairs_stop (ys.drop pre.length) hs
      refine ⟨u, rest, ?_, hc⟩
      rw [← List.drop_drop]; exact hd

/-! ## The loop -/

theorem stEnd_le (out B : Nat) : ∀ st : List (Nat × Nat), out ≤ B →
    (∀ p ∈ st, out + p.1 + p.2 ≤ B) → out + stEnd st ≤ B
  | [], h, _ => by simpa [stEnd]
  | p :: r, h, hp => by
    have h1 := hp p (by simp)
    have h2 := stEnd_le out B r h (fun q hq => hp q (List.mem_cons_of_mem _ hq))
    simp only [stEnd]; omega

theorem L_take_le' (ys : List Nat) (i : Nat) : L ((ys.take i).map w8) ≤ L (ys.map w8) :=
  RvvExact.L_map_take_le w8 ys i

/-- **The loop invariant**: the cursor is the standard length of the units
consumed, which contain no unpaired surrogate; the stores end within the
standard length of the input; a `SURROGATE` return is at an unpaired unit
inside the input; with enough fuel a success consumes the whole input. -/
theorem loop_spec (pick : Nat → Nat) (ys : List Nat) (hu : U16 ys) :
    ∀ f pos out hi, out = L ((ys.take pos).map w8) → hi ≤ L (ys.map w8) → pos ≤ ys.length →
      unp (ys.take pos) = 0 →
      (loop pick ys f pos out hi).2.2.2 ≤ L (ys.map w8) ∧
      (loop pick ys f pos out hi).2.2.1 = L ((ys.take (loop pick ys f pos out hi).2.1).map w8) ∧
      (loop pick ys f pos out hi).2.1 ≤ ys.length ∧
      unp (ys.take (loop pick ys f pos out hi).2.1) = 0 ∧
      ((loop pick ys f pos out hi).1 = true →
        (loop pick ys f pos out hi).2.1 < ys.length ∧
        UnpairedAt ys (loop pick ys f pos out hi).2.1) ∧
      (ys.length - pos < f → (loop pick ys f pos out hi).1 = false →
        (loop pick ys f pos out hi).2.1 = ys.length) := by
  intro f
  induction f with
  | zero =>
    intro pos out hi h1 h2 h3 h4
    simp only [loop]
    exact ⟨h2, h1, h3, h4, (fun h => by cases h), (fun h => by omega)⟩
  | succ f ih =>
    intro pos out hi h1 h2 h3 h4
    unfold loop
    by_cases hz : (ys.drop pos).length = 0
    · rw [ite_eq_left hz]
      dsimp only
      simp only [List.length_drop] at hz
      exact ⟨h2, h1, h3, h4, (fun h => by cases h), (fun _ _ => by omega)⟩
    rw [ite_eq_right hz]
    have hne : 0 < (ys.drop pos).length := by omega
    have hud : U16 (ys.drop pos) := ReplacementRest.U16_drop hu pos
    obtain ⟨s1, s2, s3, s4, s5⟩ := step_facts pick (ys.drop pos) hne hud
    have hfit := (utf16to8_fits pick () (ys.drop pos) hud hne).1
    change ∀ p ∈ (utf16to8Step pick () (ys.drop pos)).st,
      p.1 + p.2 ≤ 0 + L (((ys.drop pos).take (utf16to8Step pick () (ys.drop pos)).c).map w8) at hfit
    generalize utf16to8Step pick () (ys.drop pos) = it at s1 s2 s3 s4 s5 hfit
    simp only [List.length_drop] at s1
    have hLc := RvvExact.L_map_take_add w8 ys pos it.c
    have hLe := L_take_le' ys (pos + it.c)
    have hhi : out + stEnd it.st ≤ L (ys.map w8) := by
      apply stEnd_le
      · have := L_take_le' ys pos; omega
      · intro p hp; have := hfit p hp; simp only [Nat.zero_add] at this; omega
    have hunp : unp (ys.take (pos + it.c)) = 0 := by
      rw [List.take_add]; exact ReplacementLengths.unp_append _ _ h4 s3
    dsimp only
    by_cases hst : it.stop = true
    · rw [ite_eq_left hst]
      dsimp only
      obtain ⟨u, rest, hd, hc⟩ := s5 hst
      have hlt : pos + it.c < ys.length := by
        have : (ys.drop (pos + it.c)).length = rest.length + 1 := by
          rw [← List.drop_drop, hd]; simp
        simp at this; omega
      refine ⟨by omega, by omega, by omega, hunp, fun _ => ⟨hlt, ?_⟩,
        fun _ h => by cases h⟩
      refine ⟨u, rest, by rw [← List.drop_drop]; exact hd, ?_⟩
      rcases hc with ⟨hl, hh⟩ | hc
      · left; refine ⟨hl, ?_⟩
        cases e : endsHigh (ys.take (pos + it.c))
        · rfl
        · have := ReplacementLengths.endsHigh_unp _ e; omega
      · exact Or.inr hc
    · rw [ite_eq_right hst]
      have hs' : it.stop = false := by simpa using hst
      have hc1 := s2 hs'
      have r := ih (pos + it.c) (out + it.adv) (max hi (out + stEnd it.st)) (by omega)
        (by omega) (by omega) hunp
      exact ⟨r.1, r.2.1, r.2.2.1, r.2.2.2.1, r.2.2.2.2.1, fun _ => r.2.2.2.2.2 (by omega)⟩

/-- **The kernel's `full_result`**: stores within `utf8_length_from_utf16`
of the input; on success the whole input consumed and `output_count` its
standard length; on `SURROGATE` (`err = 1`, the only error) `input_count`
is an unpaired unit of the input, none before it, and `output_count` is the
standard length of the units before it. -/
theorem rvvFull_spec (pick : Nat → Nat) (ys : List Nat) (hu : U16 ys) :
    (rvvFull pick ys).hi ≤ L (ys.map w8) ∧
    ((rvvFull pick ys).err = 0 ∨ (rvvFull pick ys).err = 1) ∧
    ((rvvFull pick ys).err = 0 →
      (rvvFull pick ys).inp = ys.length ∧ (rvvFull pick ys).outp = L (ys.map w8) ∧
      unp ys = 0) ∧
    ((rvvFull pick ys).err ≠ 0 →
      (rvvFull pick ys).inp < ys.length ∧ UnpairedAt ys (rvvFull pick ys).inp ∧
      unp (ys.take (rvvFull pick ys).inp) = 0 ∧
      (rvvFull pick ys).outp = L ((ys.take (rvvFull pick ys).inp).map w8)) := by
  have h := loop_spec pick ys hu (ys.length + 1) 0 0 0 (by simp) (by omega) (by omega) (by simp [unp])
  unfold rvvFull
  generalize loop pick ys (ys.length + 1) 0 0 0 = r at h
  obtain ⟨stp, pos, out, hi⟩ := r
  obtain ⟨h1, h2, h3, h4, h5, h6⟩ := h
  simp only at h1 h2 h3 h4 h5 h6 ⊢
  cases stp
  · have hp := h6 (by omega) rfl
    rw [hp, List.take_length] at h2 h4
    exact ⟨h1, Or.inl rfl, fun _ => ⟨hp, h2, h4⟩, fun h => absurd rfl h⟩
  · obtain ⟨a, b⟩ := h5 rfl
    exact ⟨h1, Or.inr rfl, fun h => absurd h Nat.one_ne_zero, fun _ => ⟨a, b, h4, h2⟩⟩

/-! ## `VKSpec'` -/

/-- **`VKSpec'` holds for the rvv kernel, for every `vl`.** -/
theorem rvvVK_spec (pick : Nat → Nat) : ReplacementLengths.VKSpec' (rvvVK pick) := by
  refine ⟨fun ys hu => ?_, fun ys hu he => ?_⟩
  · obtain ⟨h1, -, h3, h4⟩ := rvvFull_spec pick ys hu
    unfold rvvVK
    by_cases h0 : (rvvFull pick ys).err = 0
    · rw [ite_eq_left h0]
      exact ⟨h1, fun _ => Nat.le_of_eq (h3 h0).2.1, fun h => absurd rfl h⟩
    · rw [ite_eq_right h0]
      exact ⟨h1, fun h => absurd h h0, fun _ => (h4 h0).1⟩
  · obtain ⟨-, -, -, h4⟩ := rvvFull_spec pick ys hu
    unfold rvvVK at he ⊢
    by_cases h0 : (rvvFull pick ys).err = 0
    · simp [h0] at he
    · simp only [h0, ite_false]
      exact (h4 h0).2.2.1

/-! ## The length function and the `_safe` drivers -/

/-- The rvv `utf8_length_from_utf16` (src/rvv/rvv_length_from.inl.cpp:86-103,
reached through src/implementation.cpp:2422-2428 and
src/rvv/rvv_length_from.inl.cpp:105-116), `vl = vsetvl_e16m8(len)` chosen by `pickL`. -/
def rvvLen (pickL : Nat → Nat) (ys : List Nat) : Nat :=
  LengthsRvv.rvvSum pickL LengthsRvv.rvv8of16 ys.length ys

theorem SLSpec_rvv (pickL : Nat → Nat) : ReplacementRest.SLSpec (rvvLen pickL) :=
  fun ys hu => ((LengthsRvv.rvv_lengths pickL ys).2.1 hu).2

theorem SLSpec'_rvv (pickL : Nat → Nat) : ReplacementLengths.SLSpec' (rvvLen pickL) :=
  ReplacementLengths.SLSpec'_of _ (SLSpec_rvv pickL)

/-- **`convert_utf16_to_utf8_with_replacement_safe`
(src/implementation.cpp:2494-2539) on rvv, from the entry point, on every
input of 16-bit units, every `utf8_len`, every `vl` of the kernel (`pick`)
and of the length function (`pickL`), every ASCII probe of the scalar
tail**: every store ends inside `utf8_len` and so does `output_count`. No
hypothesis. -/
theorem replSafe_rvv (pick pickL : Nat → Nat) (probe : List Nat → Nat → Bool)
    (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.tLoop (ReplacementRest.dSafe (rvvVK pick) (rvvLen pickL)
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).hi ≤ cap ∧
    (ReplacementRest.tLoop (ReplacementRest.dSafe (rvvVK pick) (rvvLen pickL)
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).outp ≤ cap := by
  have h := ReplacementRest.tLoop_safe _ _
    (ReplacementLengths.dSafe_spec' _ _ _ (rvvVK_spec pick) (SLSpec'_rvv pickL)
      (ReplacementRest.cwe_spec probe))
    (ReplacementRest.crs_spec _ (ReplacementRest.cwe_spec probe)) xs hu cap (xs.length + 1) 0 0 0
    (by omega) (by omega) (by omega)
  exact ⟨h.1, h.2.1⟩

/-- **`convert_utf16_to_utf8_safe_with_details`** (src/implementation.cpp:2028-2091)
on rvv: stores and `output_count` within `utf8_len`, and the full `FSpec`. No
hypothesis. -/
theorem dSafe_rvv (pick pickL : Nat → Nat) (probe : List Nat → Nat → Bool) :
    ReplacementRest.FSpec (ReplacementRest.dSafe (rvvVK pick) (rvvLen pickL)
      (ReplacementRest.cwe probe)) :=
  ReplacementLengths.dSafe_spec' _ _ _ (rvvVK_spec pick) (SLSpec'_rvv pickL)
    (ReplacementRest.cwe_spec probe)

/-! ## `convert_with_replacement_via` over the rvv kernel -/

section
variable (K : List Nat → ReplacementRest.FR) (xs : List Nat)

/-- **`convert_with_replacement_via`**
(src/generic/utf16_to_utf8/utf16_to_utf8_with_replacement.h:13-31), the
loop at :18 (`while (pos < len)`) with fuel, over a `full_result`
kernel `K`: add `output_count` (:20), stop unless `SURROGATE` (:21-23),
skip the unit (:24), write U+FFFD (3 bytes, :25-28), return the bytes
written (:30). `(returned, end of stores)`. -/
def viaLoop : Nat → Nat → Nat → Nat → Nat × Nat
  | 0, _, out, hi => (out, hi)
  | f + 1, pos, out, hi =>
    if pos < xs.length then
      let r := K (xs.drop pos)
      let out1 := out + r.outp
      let hi1 := max hi (out + r.hi)
      if r.err ≠ 1 then (out1, hi1)
      else viaLoop f (pos + r.inp + 1) (out1 + 3) (max hi1 (out1 + 3))
    else (out, hi)
end

theorem viaLoop_spec (pick : Nat → Nat) (xs : List Nat) (hu : U16 xs) :
    ∀ f pos out hi, Via xs pos out → hi ≤ L (wR xs) → pos ≤ xs.length →
      (viaLoop (rvvFull pick) xs f pos out hi).2 ≤ L (wR xs) ∧
      (xs.length - pos < f → (viaLoop (rvvFull pick) xs f pos out hi).1 = L (wR xs)) := by
  intro f
  induction f with
  | zero => intro pos out hi _ h2 _; simp only [viaLoop]; exact ⟨h2, fun h => by omega⟩
  | succ f ih =>
    intro pos out hi hv h2 h3
    unfold viaLoop
    have hinv := via_inv xs pos out hv
    by_cases hp : pos < xs.length
    · rw [ite_eq_left hp]
      obtain ⟨k1, k2, k3, k4⟩ := rvvFull_spec pick (xs.drop pos) (ReplacementRest.U16_drop hu pos)
      have hst := std_le_repl (xs.drop pos)
      dsimp only
      by_cases he : (rvvFull pick (xs.drop pos)).err = 1
      · rw [ite_eq_right (by simpa using he)]
        obtain ⟨a1, a2, a3, a4⟩ := k4 (by omega)
        simp only [List.length_drop] at a1
        have hv' := Via.surrogate pos out _ _ hv a2 a3 a4
        have hrs := via_repl_store xs _ _ hv'
        have r := ih _ _ (max (max hi (out + (rvvFull pick (xs.drop pos)).hi))
          (out + (rvvFull pick (xs.drop pos)).outp + 3)) hv' (by omega) (by omega)
        exact ⟨r.1, fun _ => r.2 (by omega)⟩
      · rw [ite_eq_left (by simpa using he)]
        have he0 : (rvvFull pick (xs.drop pos)).err = 0 := by omega
        obtain ⟨b1, b2, hunp⟩ := k3 he0
        simp only [List.length_drop] at b1
        have := std_eq_repl _ hunp
        refine ⟨by omega, fun _ => ?_⟩
        rw [b2]; omega
    · rw [ite_eq_right hp]
      have : xs.drop pos = [] := List.drop_eq_nil_of_le (by omega)
      rw [this] at hinv
      simp [wR] at hinv
      exact ⟨h2, fun _ => by omega⟩

/-- **`convert_utf16{le,be}_to_utf8_with_replacement` on rvv**
(src/rvv/implementation.cpp:125-147, `convert_with_replacement_via` over
`rvv_utf16_to_utf8_with_details`): every store (kernel calls and U+FFFD)
ends inside `utf8_length_from_utf16_with_replacement` (`countRepl`), and
the returned count is exactly that length, for every `vl`. -/
theorem via_rvvRepl (pick : Nat → Nat) (xs : List Nat) (hu : U16 xs) :
    (viaLoop (rvvFull pick) xs (xs.length + 1) 0 0 0).2 ≤ countRepl xs ∧
    (viaLoop (rvvFull pick) xs (xs.length + 1) 0 0 0).1 = countRepl xs := by
  rw [countRepl_eq]
  have := viaLoop_spec pick xs hu (xs.length + 1) 0 0 0 Via.start (by omega) (by omega)
  exact ⟨this.1, this.2 (by omega)⟩

/-! ## Examples -/

/-- `A`, a pair, `é`, a lone high surrogate, `A`, with `vl = 2`: the first
iteration stores `A` and the pair (the scalar pair loop
src/rvv/rvv_utf16_to.inl.cpp:179-212 runs past the vector block and stops at
`é`), the second `é`, then returns `SURROGATE` at unit 4 with 7 bytes
written; `result` carries 4. The driver with replacement returns
`countRepl` = 11. -/
example :
    let xs := [0x41, 0xD83D, 0xDE00, 0xE9, 0xD800, 0x41]
    let r := rvvFull (fun _ => 2) xs
    (r.err, r.inp, r.outp, r.hi) = (1, 4, 7, 7) ∧ (rvvVK (fun _ => 2) xs).cnt = 4 ∧
      viaLoop (rvvFull (fun _ => 2)) xs (xs.length + 1) 0 0 0 = (11, 11) ∧ countRepl xs = 11 := by
  decide

end SafetyMargins.RvvReplacement16
