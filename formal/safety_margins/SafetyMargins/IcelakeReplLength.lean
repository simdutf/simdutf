import SafetyMargins.ReplacementRest
/-
Ice Lake `utf8_length_from_utf16_with_replacement`, the whole `result`.

`ReplacementMore.icelake_identity` proves `L (xs.map iw) = countRepl xs +
2 * prs xs` and says that the three loops of
`icelake_utf8_length_from_utf16_with_replacement`
(src/icelake/icelake_utf8_length_from_utf16.inl.cpp:114-269) count each unit
once and each adjacent (high, low) pair once "is read from the C++, not
proved". That statement is stale: `ReplacementRest` transcribes the function
(`iceLen`, with `iceLoop1`, `iceStepN`, `iceTail` for the three loops, the
straddle words and the masked tail) and proves `iceLen_eq` from the loop
invariants `loop1_inv`, `stepN_inv`, `tail_inv`; `icelake_identity` is used
only there (ReplacementRest.lean, in `iceLen_eq`), as a lemma about lists, not
as a hypothesis. This file

* states the tiling claim of the docstring as a theorem of the transcription
  (`icelake_tiling`): after the three loops, `count + pos` is `L` of `iw` and
  `matches` is `prs`, for every input of 33 units or more, both
  endiannesses;
* transcribes the other field of the `result`, `any_surrogates ? SURROGATE :
  SUCCESS` (:142, :178, :219, :254, :268), which `iceLen` drops, and the
  flag of the scalar function (include/simdutf/scalar/utf16.h:153-181,
  `scAny`), and proves that the Ice Lake function returns exactly the
  scalar `result` (error and count) on every input (`iceRes_eq`), so the
  replacement length on Ice Lake is `countRepl` with no hypothesis left.

Read from the C++: the transcription (cited lines; the loops are those of
`ReplacementRest`).
-/
namespace SafetyMargins.IcelakeReplLength
open SafetyMargins SafetyMargins.Replacement SafetyMargins.ReplacementMore
open SafetyMargins.ReplacementRest
set_option autoImplicit false
set_option linter.unusedSimpArgs false

/-! ## The tiling -/

/-- The state after the three loops (src/icelake/icelake_utf8_length_from_utf16.inl.cpp:147-263), for an input of 33 units or
more. -/
def iceFinal (sw : Bool) (rs : List Nat) : IceSt :=
  iceTail sw rs rs.length (iceStepN sw rs rs.length (iceLoop1 sw rs ((rs.length - 1) / 64)))

theorem iceFinal_inv (sw : Bool) (rs : List Nat) (hu : U16 rs) (hlen : 33 ≤ rs.length) :
    (iceFinal sw rs).cnt8 = CC sw rs rs.length ∧ (iceFinal sw rs).mt = PR sw rs rs.length ∧
    (iceFinal sw rs).pos = rs.length := by
  have hB := Nat.div_mul_le_self (rs.length - 1) 64
  have hB2 := Nat.lt_div_mul_add (a := rs.length - 1) (b := 64) (by decide)
  obtain ⟨i1, i2, i3⟩ := loop1_inv sw rs hu ((rs.length - 1) / 64)
  unfold iceFinal
  generalize iceLoop1 sw rs ((rs.length - 1) / 64) = st1 at *
  obtain ⟨n1, n2, n3⟩ := stepN_inv sw rs hu rs.length st1 (by rw [i2, i1]) (by rw [i3, i1])
  generalize iceStepN sw rs rs.length st1 = st2 at *
  have hp : st2.pos + 1 ≤ rs.length ∧ rs.length ≤ st2.pos + 32 := by
    rw [n3]; split <;> omega
  exact tail_inv sw rs hu rs.length st2 rfl (by omega) hp.1 hp.2 n1 n2

/-- **The tiling, proved.** The three loops of
`icelake_utf8_length_from_utf16_with_replacement` count every unit once
(`count + pos` is the sum of `iw`, :173-176, :216-217, :251-252, :265) and
every adjacent (high, low) pair once (`matches` is `prs`: the pairs inside a
vector, :191-194, :226-227, :261-262, and across vectors through the straddle
words, :196-199, :229-230), on every input of 33 units or more, both
endiannesses (`unitOf sw`). This is the claim the docstring of
`ReplacementMore.icelake_identity` leaves to the C++. -/
theorem icelake_tiling (sw : Bool) (rs : List Nat) (hu : U16 rs) (hlen : 33 ≤ rs.length) :
    (iceFinal sw rs).cnt8 + (iceFinal sw rs).pos = L ((rs.map (unitOf sw)).map iw) ∧
    (iceFinal sw rs).mt = prs (rs.map (unitOf sw)) := by
  obtain ⟨t1, t2, t3⟩ := iceFinal_inv sw rs hu hlen
  have hg : ∀ j, g (rs.map (unitOf sw)) j = gx sw rs j := fun j => g_map rs _ (unitOf_zero sw) j
  rw [t1, t2, t3, L_map_iw, prs_sum]
  simp only [hg, List.length_map]
  exact ⟨rfl, rfl⟩

/-! ## `any_surrogates` -/

/-- Some unit among the first `k` (after the flip) is a surrogate. -/
def AS (sw : Bool) (rs : List Nat) (k : Nat) : Bool := (List.range k).any (fun j => isSur (gx sw rs j))

theorem AS_add (sw : Bool) (rs : List Nat) (p m : Nat) :
    AS sw rs (p + m) = (AS sw rs p || (List.range m).any (fun i => isSur (gx sw rs (p + i)))) := by
  unfold AS
  rw [List.range_add, List.any_append, List.any_map]
  rfl

/-- `is_surrogate` tested as a condition (:177, :218, :253): some lane is a
surrogate. -/
theorem anyM_surM (v : Nat → Nat) (hv : ∀ i, i < 32 → v i < 65536) :
    anyM (surM v) = (List.range 32).any (fun i => isSur (v i)) := by
  unfold anyM
  apply Bool.eq_iff_iff.mpr
  simp only [List.any_eq_true, List.mem_range]
  constructor
  · rintro ⟨i, hi, h⟩
    refine ⟨i, hi, ?_⟩
    simp only [ReplacementRest.surM, eqM, (lf _ (hv i hi)).2.2.1] at h; exact h
  · rintro ⟨i, hi, h⟩
    refine ⟨i, hi, ?_⟩
    simp only [ReplacementRest.surM, eqM, (lf _ (hv i hi)).2.2.1]; exact h

theorem blk_any (sw : Bool) (rs : List Nat) (hu : U16 rs) (p : Nat) :
    anyM (surM (blk sw rs p)) = (List.range 32).any (fun i => isSur (gx sw rs (p + i))) := by
  rw [anyM_surM _ (blk_lt sw rs hu p)]; rfl

theorem loop1_any (sw : Bool) (rs : List Nat) (hu : U16 rs) (b : Nat) :
    (iceLoop1 sw rs b).anyS = AS sw rs (64 * b) := by
  induction b with
  | zero => rfl
  | succ b ih =>
    have hpos := (loop1_inv sw rs hu b).1
    simp only [iceLoop1]
    generalize iceLoop1 sw rs b = st at *
    have e : 64 * (b + 1) = st.pos + 32 + 32 := by omega
    have ih' : st.anyS = AS sw rs st.pos := by rw [hpos]; exact ih
    rw [e, AS_add, AS_add, ← ih']
    have h1 := blk_any sw rs hu st.pos
    have h2 := blk_any sw rs hu (st.pos + 32)
    unfold iceStep2N
    simp only
    split
    · rename_i h
      rw [h1, h2] at h
      simp only [Bool.or_eq_true] at h
      simp only [Nat.add_assoc] at h ⊢
      rcases h with h | h <;> simp [h]
    · rename_i h
      rw [h1, h2] at h
      simp only [Bool.or_eq_true, not_or, Bool.not_eq_true] at h
      simp only [Nat.add_assoc] at h ⊢
      simp [h.1, h.2]

theorem stepN_any (sw : Bool) (rs : List Nat) (hu : U16 rs) (size : Nat) (st : IceSt)
    (h : st.anyS = AS sw rs st.pos) :
    (iceStepN sw rs size st).anyS = AS sw rs (iceStepN sw rs size st).pos := by
  have h1 := blk_any sw rs hu st.pos
  unfold iceStepN
  split
  · simp only
    split
    · rename_i hs
      simp only [AS_add]; rw [← h1, hs]; simp
    · rename_i hs
      simp only [Bool.not_eq_true] at hs
      simp only [AS_add]; rw [← h1, hs, h]; simp
  · exact h

theorem tail_any (sw : Bool) (rs : List Nat) (hu : U16 rs) (size : Nat) (st : IceSt)
    (h32 : 32 ≤ size) (hp1 : st.pos + 1 ≤ size) (hp2 : size ≤ st.pos + 32)
    (h : st.anyS = AS sw rs st.pos) :
    (iceTail sw rs size st).anyS = AS sw rs size := by
  generalize hov : 32 - (size - st.pos) = ov
  have hov32 : ov < 32 := by omega
  have hlt : ∀ i, i < 32 → tailLanes sw rs st.pos ov i < 65536 := by
    intro i hi; rw [tail_lanes sw rs st.pos ov hov32 i hi]; split
    · exact gx_lt sw rs hu _
    · decide
  have hany : anyM (surM (tailLanes sw rs st.pos ov)) =
      (List.range (size - st.pos)).any (fun k => isSur (gx sw rs (st.pos + k))) := by
    rw [anyM_surM _ hlt]
    apply Bool.eq_iff_iff.mpr
    simp only [List.any_eq_true, List.mem_range]
    constructor
    · rintro ⟨i, hi, hs⟩
      rw [tail_lanes sw rs st.pos ov hov32 i hi] at hs
      split at hs
      · refine ⟨i - ov, by omega, ?_⟩
        rw [show st.pos + (i - ov) = st.pos - ov + i by omega]; exact hs
      · simp [isSur] at hs
    · rintro ⟨k, hk, hs⟩
      refine ⟨ov + k, by omega, ?_⟩
      rw [tail_lanes sw rs st.pos ov hov32 _ (by omega), ite_eq_left (by omega),
        show st.pos - ov + (ov + k) = st.pos + k by omega]
      exact hs
  have e : size = st.pos + (size - st.pos) := by omega
  unfold iceTail
  simp only [hov]
  split
  · rename_i hs
    rw [e, AS_add, ← hany, hs]; simp
  · rename_i hs
    simp only [Bool.not_eq_true] at hs
    rw [e, AS_add, ← hany, hs, h]; simp

theorem AS_length (sw : Bool) (rs : List Nat) :
    AS sw rs rs.length = (rs.map (unitOf sw)).any isSur := by
  unfold AS
  apply Bool.eq_iff_iff.mpr
  simp only [List.any_eq_true, List.mem_range, List.mem_map]
  constructor
  · rintro ⟨j, hj, hs⟩
    refine ⟨unitOf sw (rs[j]), ⟨rs[j], List.getElem_mem hj, rfl⟩, ?_⟩
    simpa [gx, g, List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hj] using hs
  · rintro ⟨_, ⟨x, hx, rfl⟩, hs⟩
    obtain ⟨j, hj, rfl⟩ := List.getElem_of_mem hx
    exact ⟨j, hj, by simpa [gx, g, List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hj] using hs⟩

/-- The `any_surrogates` flag of the scalar
`utf8_length_from_utf16_with_replacement` (include/simdutf/scalar/utf16.h:153-181):
set by a high surrogate (:158-159, its low partner skipped, :161-163) or by
a low surrogate (:168-169), never cleared. -/
def scAny : Bool → List Nat → Bool
  | a, [] => a
  | a, u :: r =>
    if isHigh u then
      match r with
      | [] => true
      | v :: r' => if isLow v then scAny true r' else scAny true (v :: r')
    else if isLow u then scAny true r
    else scAny a r

theorem scAny_eq : ∀ (xs : List Nat) (a : Bool), scAny a xs = (a || xs.any isSur) := by
  intro xs
  induction xs using wR.induct with
  | case1 => intro a; simp [scAny]
  | case2 u =>
    intro a
    simp only [scAny, List.any_cons, List.any_nil, Bool.or_false]
    by_cases hh : isHigh u = true
    · simp [hh, high_sur hh]
    · by_cases hl : isLow u = true
      · simp [hh, hl, low_sur hl]
      · have : isSur u = false := by simp [isSur, isHigh, isLow] at *; omega
        simp [hh, hl, this]
  | case3 u v r hp ih =>
    intro a
    simp only [Bool.and_eq_true] at hp
    simp [scAny, hp.1, hp.2, ih, high_sur hp.1]
  | case4 u v r hp ih =>
    intro a
    by_cases hh : isHigh u = true
    · have hv : isLow v = false := by
        cases h : isLow v
        · rfl
        · exact absurd (by simp [hh, h]) hp
      simp only [scAny, hh, ite_true, hv, Bool.false_eq_true, ite_false, ih, List.any_cons, high_sur hh]
      simp
    · by_cases hl : isLow u = true
      · simp only [scAny, hh, hl, ite_true, Bool.false_eq_true, ite_false, ih, List.any_cons, low_sur hl]
        simp
      · have : isSur u = false := by simp [isSur, isHigh, isLow] at *; omega
        simp only [scAny, hh, hl, Bool.false_eq_true, ite_false, ih, List.any_cons, this]
        simp

/-- `result.error` as a Boolean (`SURROGATE` when true, `SUCCESS` when false,
include/simdutf/scalar/utf16.h:179) and `result.count`. -/
abbrev Res := Bool × Nat

/-- The scalar `utf8_length_from_utf16_with_replacement` (include/simdutf/scalar/utf16.h:153-181)
on the flipped units: `countRepl` (transcribed in `Replacement`) and `scAny`. -/
def scRes (xs : List Nat) : Res := (scAny false xs, countRepl xs)

/-- **`icelake_utf8_length_from_utf16_with_replacement`** (src/icelake/icelake_utf8_length_from_utf16.inl.cpp:114-269), the whole
`result`: short inputs return the scalar result (:125-128); otherwise
`any_surrogates` (:142, set at :178, :219, :254) and `count` (:265, :267). -/
def iceRes (sw : Bool) (rs : List Nat) : Res :=
  if 32 + 1 > rs.length then scRes (rs.map (unitOf sw)) else
  let st := iceFinal sw rs
  (st.anyS, st.cnt8 + st.pos - 2 * st.mt)

theorem iceRes_count (sw : Bool) (rs : List Nat) : (iceRes sw rs).2 = iceLen sw rs := by
  unfold iceRes iceLen iceFinal scRes; split <;> rfl

/-- **The Ice Lake replacement length, no hypothesis.** On every input of
16-bit units, both endiannesses, the Ice Lake function returns the scalar
`result`: the same error code and `count = countRepl` of the units. -/
theorem iceRes_eq (sw : Bool) (rs : List Nat) (hu : U16 rs) :
    iceRes sw rs = scRes (rs.map (unitOf sw)) := by
  have hc := iceLen_eq sw rs hu
  rw [← iceRes_count] at hc
  by_cases hlen : 32 + 1 > rs.length
  · unfold iceRes; rw [ite_eq_left hlen]
  · have hfl : (iceRes sw rs).1 = scAny false (rs.map (unitOf sw)) := by
      unfold iceRes; rw [ite_eq_right hlen]
      simp only
      rw [scAny_eq, Bool.false_or, ← AS_length]
      have hB := Nat.div_mul_le_self (rs.length - 1) 64
      have hB2 := Nat.lt_div_mul_add (a := rs.length - 1) (b := 64) (by decide)
      have i1 := (loop1_inv sw rs hu ((rs.length - 1) / 64)).1
      have i4 := loop1_any sw rs hu ((rs.length - 1) / 64)
      have i2 := (loop1_inv sw rs hu ((rs.length - 1) / 64)).2
      unfold iceFinal
      generalize iceLoop1 sw rs ((rs.length - 1) / 64) = st1 at *
      have n4 := stepN_any sw rs hu rs.length st1 (by rw [i4, i1])
      have n3 := (stepN_inv sw rs hu rs.length st1 (by rw [i2.1, i1]) (by rw [i2.2, i1])).2.2
      generalize iceStepN sw rs rs.length st1 = st2 at *
      have hp : st2.pos + 1 ≤ rs.length ∧ rs.length ≤ st2.pos + 32 := by
        rw [n3]; split <;> omega
      exact tail_any sw rs hu rs.length st2 (by omega) hp.1 hp.2 n4
    unfold scRes
    exact Prod.ext hfl hc

/-- A 40-unit input with a pair straddling two vectors, an unpaired low
surrogate in the masked tail, big-endian on a little-endian host. -/
example :
    let xs := List.replicate 31 0x41 ++ [0xD83D, 0xDE00] ++ List.replicate 6 0xE9 ++ [0xDC00]
    iceRes true (xs.map bswap) = (true, countRepl xs) ∧ countRepl xs = 31 + 4 + 12 + 3 := by
  decide +kernel

end SafetyMargins.IcelakeReplLength
