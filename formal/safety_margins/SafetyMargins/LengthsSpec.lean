import SafetyMargins.LengthsRvv
/-
Which model each kernel runs for each length function, and the length
hypotheses of `ReplacementRest` discharged.

| function | fallback | westmere | haswell | icelake | arm64 | ppc64 | lsx | lasx | rvv |
|---|---|---|---|---|---|---|---|---|---|
| `utf8_length_from_utf16le/be` | `scal sc8of16` | `bmU8of16 8` | `bmU8of16 16` | `ice8of16` ≠ | `arm8of16` ≠ | `genU8of16` | `bmU8of16 8` | `bmU8of16 16` | `rvv8of16` |
| `utf32_length_from_utf16le/be`, `count_utf16le/be` | `scal sc32of16` | `genCount16` | `genCount16` | `iceCount16` | `genCount16` | `genCount16` | `bmCount16 8` | `bmCount16 16` | `rvvC16` |
| `utf16_length_from_utf8` | `scal sc16of8` | `bmU16of8 16` | `bmU16of8 32` | `ice16of8` | `genU16of8` | `genU16of8` | `bmU16of8 16` | `bmU16of8 32` | `rvv16of8` |
| `utf32_length_from_utf8` | `scal scCount8` | `genCount8` | `genCount8` | `iceCount8` | `genCount8` | `genCount8` | `genCount8` | `genCount8` | `rvvC8` |
| `count_utf8`, `latin1_length_from_utf8` | `scal scCount8` | `bmCount8 16` | `bmCount8 32` | `iceCount8` | `genCount8` | `genCount8` | `genCount8` | `lasxCount8` | `rvvC8` |
| `utf8_length_from_utf32` | `scal sc8of32` | `genU8of32 false 4` | `genU8of32 false 8` | `genU8of32 false 16` | `arm8of32` | `genU8of32 false 4` | `genU8of32 true 4` | `genU8of32 true 8` | `sc8of32` |
| `utf16_length_from_utf32` | `scal sc16of32` | `x86_16of32 4` | `x86_16of32 8` | `ice16of32` | `gt16of32` | `scal sc16of32` | `gt16of32` | `gt16of32` | `sc16of32` |
| `utf8_length_from_latin1` | `swarLatin1` | `latin1X 16` | `latin1X 32` | `iceLatin1` | `armLatin1` | `ppcLatin1` | `laLatin1` | `laLatin1` | `rvvLatin1` |

Sources of the dispatch: src/fallback/implementation.cpp:418-534,
src/westmere/implementation.cpp:1107-1298, src/haswell/implementation.cpp:1082-1268,
src/icelake/implementation.cpp:1629-2043, src/arm64/implementation.cpp:1083-1275,
src/ppc64/implementation.cpp:717-846, src/lsx/implementation.cpp:1097-1241,
src/lasx/implementation.cpp:1195-1355, src/rvv/rvv_length_from.inl.cpp:1-164.

Every entry returns the per-unit sum of its weight on every input of its
unit type (`lengths_all`), except the two marked ≠:
`utf8_length_from_utf16` on icelake below 33 units (`ice8of16_eq`: the count
with replacement) and on arm64 for `16 k ≥ 32` units ending with a high
surrogate (`arm8of16_eq`: two more). Both are never below the per-unit sum.

`ReplacementRest.SLSpec` (the vector `utf8_length_from_utf16`) holds for
every kernel except icelake and arm64 (`SLSpec_*`, `icelake_not_SLSpec`,
`arm64_not_SLSpec`). `ReplacementRest.ULSpec` (`utf16_length_from_utf8` on
valid UTF-8) holds for every kernel, and the equality holds on every byte
input (`UL_*`).
-/
namespace SafetyMargins.LengthsSpec
open SafetyMargins SafetyMargins.ReplacementMore SafetyMargins.RvvExact SafetyMargins.Lengths
  SafetyMargins.LengthsGeneric SafetyMargins.LengthsArch SafetyMargins.LengthsIcelake
  SafetyMargins.LengthsRvv
set_option autoImplicit false

/-! ## Valid UTF-8 is bytes -/

theorem step_big (s b : Nat) (h : 256 ≤ b) : ValidExact.step s b = 8 := by
  unfold ValidExact.step
  repeat' split
  all_goals omega

theorem run_eight : ∀ bs, ValidExact.run 8 bs = 8 := by
  intro bs
  induction bs with
  | nil => rfl
  | cons b r ih =>
    simp only [ValidExact.run, List.foldl_cons] at ih ⊢
    rw [show ValidExact.step 8 b = 8 by unfold ValidExact.step; simp]
    exact ih

theorem valid8_bytes : ∀ bs s, ValidExact.run s bs = 0 → Bytes bs := by
  intro bs
  induction bs with
  | nil => intro _ _ b hb; simp at hb
  | cons b r ih =>
    intro s h c hc
    simp only [ValidExact.run, List.foldl_cons] at h
    by_cases hb : 256 ≤ b
    · rw [step_big s b hb] at h
      have := run_eight r
      unfold ValidExact.run at this; rw [this] at h; cases h
    · simp only [List.mem_cons] at hc
      rcases hc with rfl | hc
      · omega
      · exact ih _ h c hc

/-! ## `utf8_length_from_utf16` and `SLSpec` -/

theorem SLSpec_scalar : ReplacementRest.SLSpec (scal sc8of16) :=
  fun ys hu => ((scalar_lengths ys).1 hu).1
/-- westmere, lsx (`N = 8`); haswell, lasx (`N = 16`). -/
theorem SLSpec_bytemask (N : Nat) : ReplacementRest.SLSpec (bmU8of16 N) :=
  fun ys hu => bmU8of16_eq N ys hu
theorem SLSpec_ppc64 : ReplacementRest.SLSpec genU8of16 := fun ys hu => genU8of16_eq ys hu
theorem SLSpec_rvv (pick : Nat → Nat) : ReplacementRest.SLSpec (fun ys => rvvSum pick rvv8of16 ys.length ys) :=
  fun ys hu => ((rvv_lengths pick ys).2.1 hu).2

/-! ## `utf16_length_from_utf8` and `ULSpec` -/

theorem UL_scalar (v : List Nat) (hb : Bytes v) : scal sc16of8 v = L (v.map w16) :=
  ((scalar_lengths v).2.1 hb).2.1
theorem UL_bytemask (N : Nat) (v : List Nat) (hb : Bytes v) : bmU16of8 N v = L (v.map w16) :=
  bmU16of8_eq N v hb
theorem UL_generic (v : List Nat) (hb : Bytes v) : genU16of8 v = L (v.map w16) := genU16of8_eq v hb
theorem UL_icelake (v : List Nat) (hb : Bytes v) : ice16of8 v = L (v.map w16) := ice16of8_eq v hb
theorem UL_rvv (pick : Nat → Nat) (v : List Nat) (hb : Bytes v) :
    rvvSum pick rvv16of8 v.length v = L (v.map w16) := ((rvv_lengths pick v).1 hb).2.1

theorem ULSpec_of (UL : List Nat → Nat) (h : ∀ v, Bytes v → UL v = L (v.map w16)) :
    ReplacementRest.ULSpec UL := fun v hv => h v (valid8_bytes v 0 hv)

theorem ULSpec_scalar : ReplacementRest.ULSpec (scal sc16of8) := ULSpec_of _ UL_scalar
/-- westmere, lsx (`N = 16`); haswell, lasx (`N = 32`). -/
theorem ULSpec_bytemask (N : Nat) : ReplacementRest.ULSpec (bmU16of8 N) := ULSpec_of _ (UL_bytemask N)
/-- arm64, ppc64. -/
theorem ULSpec_generic : ReplacementRest.ULSpec genU16of8 := ULSpec_of _ UL_generic
theorem ULSpec_icelake : ReplacementRest.ULSpec ice16of8 := ULSpec_of _ UL_icelake
theorem ULSpec_rvv (pick : Nat → Nat) :
    ReplacementRest.ULSpec (fun v => rvvSum pick rvv16of8 v.length v) := ULSpec_of _ (UL_rvv pick)

/-! ## Everything -/

/-- **Every length function of every kernel, on every input of its unit type**
(the rvv ones in `rvv_lengths`, for every `vl`). -/
theorem lengths_all (xs : List Nat) :
    (U16 xs →
      -- utf8_length_from_utf16
      bmU8of16 8 xs = L (xs.map w8) ∧ bmU8of16 16 xs = L (xs.map w8) ∧ genU8of16 xs = L (xs.map w8) ∧
      ice8of16 xs = (if 32 + 1 > xs.length then Replacement.countRepl xs else L (xs.map w8)) ∧
      arm8of16 xs = L (xs.map w8) + (if 17 ≤ xs.length ∧ xs.length % 16 = 0 ∧
        Replacement.isHigh (g xs (xs.length - 1)) = true then 2 else 0) ∧
      -- utf32_length_from_utf16, count_utf16
      genCount16 xs = L (xs.map w32) ∧ bmCount16 8 xs = L (xs.map w32) ∧ bmCount16 16 xs = L (xs.map w32) ∧
      iceCount16 xs = L (xs.map w32)) ∧
    (Bytes xs →
      -- utf16_length_from_utf8
      bmU16of8 16 xs = L (xs.map w16) ∧ bmU16of8 32 xs = L (xs.map w16) ∧ genU16of8 xs = L (xs.map w16) ∧
      ice16of8 xs = L (xs.map w16) ∧
      -- utf32_length_from_utf8, count_utf8, latin1_length_from_utf8
      genCount8 xs = L (xs.map w01) ∧ bmCount8 16 xs = L (xs.map w01) ∧ bmCount8 32 xs = L (xs.map w01) ∧
      iceCount8 xs = L (xs.map w01) ∧ (∀ a, lasxCount8 a xs = L (xs.map w01)) ∧
      -- utf8_length_from_latin1
      swarLatin1 xs = L (xs.map wL8) ∧ latin1X 16 xs = L (xs.map wL8) ∧ latin1X 32 xs = L (xs.map wL8) ∧
      iceLatin1 xs = L (xs.map wL8) ∧ armLatin1 xs = L (xs.map wL8) ∧ ppcLatin1 xs = L (xs.map wL8) ∧
      laLatin1 xs = L (xs.map wL8)) ∧
    (U32 xs →
      -- utf8_length_from_utf32
      genU8of32 false 4 xs = L (xs.map w8of32) ∧ genU8of32 false 8 xs = L (xs.map w8of32) ∧
      genU8of32 false 16 xs = L (xs.map w8of32) ∧ genU8of32 true 4 xs = L (xs.map w8of32) ∧
      genU8of32 true 8 xs = L (xs.map w8of32) ∧ arm8of32 xs = L (xs.map w8of32) ∧
      -- utf16_length_from_utf32
      x86_16of32 4 xs = L (xs.map w16of32) ∧ x86_16of32 8 xs = L (xs.map w16of32) ∧
      ice16of32 xs = L (xs.map w16of32) ∧ gt16of32 xs = L (xs.map w16of32)) := by
  refine ⟨fun hu => ⟨bmU8of16_eq 8 xs hu, bmU8of16_eq 16 xs hu, genU8of16_eq xs hu, ice8of16_eq xs hu,
      arm8of16_eq xs hu, genCount16_eq xs hu, bmCount16_eq 8 xs hu, bmCount16_eq 16 xs hu,
      iceCount16_eq xs hu⟩,
    fun hb => ⟨bmU16of8_eq 16 xs hb, bmU16of8_eq 32 xs hb, genU16of8_eq xs hb, ice16of8_eq xs hb,
      genCount8_eq xs hb, bmCount8_eq 16 xs hb, bmCount8_eq 32 xs hb, iceCount8_eq xs hb,
      fun a => lasxCount8_eq a xs hb, swarLatin1_eq xs hb, latin1X_eq 16 xs hb, latin1X_eq 32 xs hb,
      iceLatin1_eq xs hb, armLatin1_eq xs hb, ppcLatin1_eq xs hb, laLatin1_eq xs hb⟩,
    fun hu => ⟨genU8of32_eq false 4 xs hu, genU8of32_eq false 8 xs hu, genU8of32_eq false 16 xs hu,
      genU8of32_eq true 4 xs hu, genU8of32_eq true 8 xs hu, arm8of32_eq xs hu,
      x86_16of32_eq 4 xs hu, x86_16of32_eq 8 xs hu, ice16of32_eq xs hu, gt16of32_eq xs hu⟩⟩

/-- The two exceptions are over-estimates: never below the per-unit sum. -/
theorem exceptions_ge (xs : List Nat) (hu : U16 xs) :
    L (xs.map w8) ≤ ice8of16 xs ∧ L (xs.map w8) ≤ arm8of16 xs := by
  refine ⟨?_, by rw [arm8of16_eq xs hu]; omega⟩
  rw [ice8of16_eq xs hu]
  split
  · rw [Replacement.countRepl_eq]; exact Replacement.std_le_repl xs
  · exact Nat.le_refl _

end SafetyMargins.LengthsSpec
