import SafetyMargins.ReplacementClose
import SafetyMargins.LengthsSpec
/-
The length hypotheses of the conversions with replacement, removed.

`ReplacementRest.SLSpec` (the vector `utf8_length_from_utf16` is the
per-unit sum on every input) is false on arm64 and icelake
(`LengthsSpec.arm64_not_SLSpec`, `icelake_not_SLSpec`): arm64 adds 2 when
the input has `16 k ≥ 32` units and ends with a high surrogate
(`LengthsArch.arm8of16_eq`), icelake returns the count with replacement below
33 units (`LengthsIcelake.ice8of16_eq`). The drivers only apply it to the
units before a `SURROGATE` the kernel reported
(src/implementation.cpp:2061-2067), and those contain no unpaired surrogate:

* **`SLSpec'`**: the length is the per-unit sum on inputs with no unpaired
  surrogate (`Replacement.unp ys = 0`). Such an input does not end with a
  high surrogate (`endsHigh_unp`), and its count with replacement is its
  standard length (`Replacement.std_eq_repl`), so every kernel meets it:
  `SLSpec'_arm64`, `SLSpec'_icelake`, `SLSpec'_bytemask`, `SLSpec'_ppc64`,
  `SLSpec'_scalar`.
* **`VKSpec'`**: `VKSpec`, and on an error the units before `count` have no
  unpaired surrogate. `vkspec'_of_model` proves it for the model `vk` of
  `ReplacementClose`, given what the C++ does on each path (`CleanChoice`):
  an iteration that does not stop consumes no unpaired surrogate (the vector
  paths run only on registers without surrogates, e.g.
  src/westmere/sse_convert_utf16_to_utf8.cpp:349-360, and the scalar fallback
  writes only valid pairs, :471-494), a stopping one reports its first
  failed surrogate (:476-487, `ek`); the scalar tail `cwfLoop` is transcribed,
  and its ASCII probe is sound (`ProbeOK`, the 64-bit test of
  include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:116-124).
* `dLoop_safe'`, `dSafe_spec'`: `ReplacementRest.dLoop_safe` with these two.

End to end, with the length function each kernel runs
(`LengthsSpec` table): `replSafe_kernels` and `dSafe_kernels`
(`convert_utf16_to_utf8_with_replacement_safe`,
`convert_utf16_to_utf8_safe_with_details`, westmere, haswell, arm64, ppc64,
lsx, lasx), and `convertRepl_kernels`, `trRun_kernels` (UTF-8 → UTF-16 with
replacement, the same six, `ULSpec` from `LengthsSpec`). No length
hypothesis is left; what remains are the model hypotheses (each kernel
function returns results of its model, as in `ReplacementClose`).
-/
namespace SafetyMargins.ReplacementLengths
open SafetyMargins SafetyMargins.Replacement SafetyMargins.ReplacementMore
  SafetyMargins.ReplacementClose
open SafetyMargins.Utf8 (w16 MPath)
set_option autoImplicit false

/-! ## Unpaired surrogates -/

theorem endsHigh_unp : ∀ a : List Nat, endsHigh a = true → 1 ≤ unp a := by
  intro a
  induction a using wR.induct with
  | case1 => intro h; simp [endsHigh] at h
  | case2 u =>
    intro h
    simp only [endsHigh, List.getLast?_singleton] at h
    simp [unp, high_sur h]
  | case3 u v r hp ih =>
    intro h
    simp only [Bool.and_eq_true] at hp
    rw [unp, ite_eq_left (by simp [hp.1, hp.2])]
    cases r with
    | nil =>
      simp only [endsHigh, List.getLast?_cons_cons, List.getLast?_singleton] at h
      rw [high_not_low h] at hp; cases hp.2
    | cons w r' =>
      exact ih (by simpa [endsHigh, List.getLast?_cons_cons] using h)
  | case4 u v r hp ih =>
    intro h
    rw [unp, ite_eq_right hp]
    have := ih (by simpa [endsHigh, List.getLast?_cons_cons] using h)
    omega

theorem unp_append (a b : List Nat) (ha : unp a = 0) (hb : unp b = 0) : unp (a ++ b) = 0 := by
  have he : endsHigh a = false := by
    cases h : endsHigh a
    · rfl
    · have := endsHigh_unp a h; omega
  have hw := wR_append a b (by simp [he])
  have e1 := L_wR (a ++ b)
  rw [hw, L_append, L_wR, L_wR, List.map_append, L_append] at e1
  omega

/-- A list with no unpaired surrogate does not end with a high surrogate. -/
theorem last_not_high (xs : List Nat) (h : unp xs = 0) (hne : xs ≠ []) :
    isHigh (g xs (xs.length - 1)) = false := by
  cases hh : isHigh (g xs (xs.length - 1))
  · rfl
  · have : endsHigh xs = true := by
      unfold endsHigh
      rw [List.getLast?_eq_some_getLast hne]
      have e : xs.getLast hne = g xs (xs.length - 1) := by
        simp [g, List.getLast_eq_getElem, List.getD_eq_getElem?_getD,
          List.getElem?_eq_getElem (show xs.length - 1 < xs.length by
            have := List.length_pos_iff.mpr hne; omega)]
      rw [e]; exact hh
    have := endsHigh_unp xs this; omega

theorem unp_single (u : Nat) (h : isSur u = false) : unp [u] = 0 := by simp [unp, h]

theorem unp_pair (u v : Nat) (hu : isHigh u = true) (hv : isLow v = true) : unp [u, v] = 0 := by
  simp [unp, hu, hv]

/-! ## `SLSpec'` holds for every kernel -/

/-- `utf8_length_from_utf16` is the per-unit sum on inputs with no unpaired
surrogate: all the drivers need. -/
def SLSpec' (SL : List Nat → Nat) : Prop :=
  ∀ ys, U16 ys → unp ys = 0 → SL ys = L (ys.map w8)

theorem SLSpec'_of (SL : List Nat → Nat) (h : ReplacementRest.SLSpec SL) : SLSpec' SL :=
  fun ys hu _ => h ys hu

/-- arm64 (`arm8of16`). -/
theorem SLSpec'_arm64 : SLSpec' LengthsArch.arm8of16 := by
  intro ys hu h0
  rw [LengthsArch.arm8of16_eq ys hu]
  by_cases hne : ys = []
  · subst hne; simp
  rw [last_not_high ys h0 hne]; simp

/-- icelake (`ice8of16`). -/
theorem SLSpec'_icelake : SLSpec' LengthsIcelake.ice8of16 := by
  intro ys hu h0
  rw [LengthsIcelake.ice8of16_eq ys hu]
  split
  · rw [countRepl_eq, std_eq_repl _ h0]
  · rfl

theorem SLSpec'_bytemask (N : Nat) : SLSpec' (LengthsGeneric.bmU8of16 N) :=
  SLSpec'_of _ (LengthsSpec.SLSpec_bytemask N)
theorem SLSpec'_ppc64 : SLSpec' LengthsGeneric.genU8of16 := SLSpec'_of _ LengthsSpec.SLSpec_ppc64
theorem SLSpec'_scalar : SLSpec' (Lengths.scal Lengths.sc8of16) := SLSpec'_of _ LengthsSpec.SLSpec_scalar

/-! ## `VKSpec'` for the model -/

/-- `VKSpec`, and on an error the units before `count` have no unpaired
surrogate. -/
def VKSpec' (VK : List Nat → ReplacementRest.KR) : Prop :=
  ReplacementRest.VKSpec VK ∧ ∀ ys, U16 ys → (VK ys).e ≠ 0 → unp (ys.take (VK ys).cnt) = 0

/-- The ASCII probe of the scalar tail is sound, on 16-bit units (the only
inputs it sees). `probeOK_real` proves it for the C++ probe. -/
def ProbeOK (probe : List Nat → Nat → Bool) : Prop :=
  ∀ zs p, U16 zs → probe zs p = true → p + 4 ≤ zs.length → ∀ k, k < 4 → g zs (p + k) < 0x80

theorem take_succ_g (xs : List Nat) (pos : Nat) (h : pos < xs.length) :
    xs.take (pos + 1) = xs.take pos ++ [g xs pos] := by
  rw [ValidatorSound.take_succ_getD xs pos h]; rfl

theorem not_sur_lt (u : Nat) (h : u < 0xD800) : isSur u = false := by
  simp [isSur]; omega

/-- **The scalar tail reports `SURROGATE` after units with no unpaired
surrogate.** -/
theorem cwfLoop_unp (xs : List Nat) (hu : U16 xs) (probe : Nat → Bool)
    (hp : ∀ p, probe p = true → p + 4 ≤ xs.length → ∀ k, k < 4 → g xs (p + k) < 0x80) :
    ∀ f burst pos out hi, unp (xs.take pos) = 0 → pos ≤ xs.length →
      (burst ≠ 0 → pos + burst ≤ xs.length ∧ ∀ k, k < burst → g xs (pos + k) < 0x80) →
      (cwfLoop xs probe f burst pos out hi).err = 1 →
      unp (xs.take (cwfLoop xs probe f burst pos out hi).inp) = 0 := by
  intro f
  induction f with
  | zero => intro _ _ _ _ _ _ _ h; simp [cwfLoop] at h
  | succ f ih =>
    intro burst pos out hi h0 hl hb
    unfold cwfLoop
    by_cases hbz : burst ≠ 0
    · rw [ite_eq_left hbz]
      obtain ⟨hb1, hb2⟩ := hb hbz
      have ha : g xs pos < 0x80 := by simpa using hb2 0 (by omega)
      apply ih
      · rw [take_succ_g xs pos (by omega)]
        exact unp_append _ _ h0 (unp_single _ (not_sur_lt _ (by omega)))
      · omega
      · intro _; refine ⟨by omega, fun k hk => ?_⟩
        have := hb2 (k + 1) (by omega); rwa [show pos + (k + 1) = pos + 1 + k by omega] at this
    rw [ite_eq_right hbz]
    by_cases hps : pos < xs.length
    · rw [ite_eq_left hps]
      have hlt := g_lt xs hu pos
      obtain ⟨u1, u2, u3, u4, -, -, -⟩ := ReplacementRest.uf (g xs pos) hlt
      have hs1 := take_succ_g xs pos hps
      split
      · rename_i hpr
        exact ih 4 pos out hi h0 hl (fun _ => ⟨hpr.1, hp pos hpr.2 hpr.1⟩)
      simp only
      split
      · rename_i ha; rw [u1] at ha
        have hn : unp (xs.take (pos + 1)) = 0 := by
          rw [hs1]; exact unp_append _ _ h0 (unp_single _ (not_sur_lt _ (by simp at ha; omega)))
        exact ih _ _ _ _ hn (by omega) (by simp)
      split
      · rename_i _ hb'; rw [u2] at hb'
        have hn : unp (xs.take (pos + 1)) = 0 := by
          rw [hs1]; exact unp_append _ _ h0 (unp_single _ (not_sur_lt _ (by simp at hb'; omega)))
        exact ih _ _ _ _ hn (by omega) (by simp)
      split
      · rename_i _ _ hc; rw [u3] at hc
        have hn : unp (xs.take (pos + 1)) = 0 := by
          rw [hs1]; exact unp_append _ _ h0 (unp_single _ (by simpa using hc))
        exact ih _ _ _ _ hn (by omega) (by simp)
      by_cases hl1 : xs.length ≤ pos + 1
      · rw [ite_eq_left hl1]; intro _; exact h0
      rw [ite_eq_right hl1]
      by_cases hd1 : 0x3FF < (g xs pos + 65536 - 0xD800) % 65536
      · rw [ite_eq_left hd1]; intro _; exact h0
      rw [ite_eq_right hd1]
      by_cases hd2 : 0x3FF < (g xs (pos + 1) + 65536 - 0xDC00) % 65536
      · rw [ite_eq_left hd2]; intro _; exact h0
      rw [ite_eq_right hd2]
      have hhi : isHigh (g xs pos) = true := by rw [← u4]; simpa using hd1
      obtain ⟨-, -, -, -, v5, -, -⟩ := ReplacementRest.uf (g xs (pos + 1)) (g_lt xs hu _)
      have hlo : isLow (g xs (pos + 1)) = true := by rw [← v5]; simpa using hd2
      have hs2 : xs.take (pos + 2) = xs.take pos ++ [g xs pos, g xs (pos + 1)] := by
        rw [show pos + 2 = pos + 1 + 1 by omega, take_succ_g xs (pos + 1) (by omega), hs1]; simp
      have hn : unp (xs.take (pos + 2)) = 0 := by
        rw [hs2]; exact unp_append _ _ h0 (unp_pair _ _ hhi hlo)
      exact ih _ _ _ _ hn (by omega) (by simp)
    · rw [ite_eq_right hps]; intro h; simp at h

/-- What the C++ does on each path of the vector loop: an iteration that does
not stop consumes no unpaired surrogate; a stopping one (the `SURROGATE`
return of the scalar fallback) reports the first unit of the failed pair,
with none before it in the iteration. -/
def CleanChoice (N : Nat) (choose : List Nat → Iter) (ek : List Nat → Nat) : Prop :=
  ∀ us, N ≤ us.length →
    ((choose us).stop = false → unp (us.take (choose us).consumed) = 0) ∧
    ((choose us).stop = true → unp (us.take (ek us)) = 0)

theorem vkLoop_unp (N : Nat) (choose : List Nat → Iter) (ek : List Nat → Nat)
    (hcl : CleanChoice N choose ek) (ys : List Nat) :
    ∀ f pos out hi, unp (ys.take pos) = 0 →
      unp (ys.take (vkLoop N choose ek ys f pos out hi).2.1) = 0 := by
  intro f
  induction f with
  | zero => intro pos out hi h; exact h
  | succ f ih =>
    intro pos out hi h
    unfold vkLoop
    by_cases hs : (ys.drop pos).length < N
    · rw [ite_eq_left hs]; exact h
    rw [ite_eq_right hs]
    obtain ⟨c1, c2⟩ := hcl (ys.drop pos) (by omega)
    simp only
    by_cases hst : (choose (ys.drop pos)).stop = true
    · rw [ite_eq_left hst]; simp only
      rw [List.take_add]; exact unp_append _ _ h (c2 hst)
    · rw [ite_eq_right hst]
      apply ih
      rw [List.take_add]; exact unp_append _ _ h (c1 (by simpa using hst))

/-- **`VKSpec'` holds for the generic kernels' model.** -/
theorem vkspec'_of_model (N : Nat) (paths : List Iter) (hT : TableOK N paths)
    (choose : List Nat → Iter) (hc : ∀ us, N ≤ us.length → choose us ∈ paths)
    (ek : List Nat → Nat) (hek : ∀ us, N ≤ us.length → ek us < us.length)
    (hcl : CleanChoice N choose ek)
    (probe : List Nat → Nat → Bool) (hpr : ProbeOK probe) (fuel : Nat) :
    VKSpec' (vk N choose ek probe fuel) := by
  refine ⟨vkspec_of_model N paths hT choose hc ek hek probe fuel, fun ys hu he => ?_⟩
  have hu0 := vkLoop_unp N choose ek hcl ys fuel 0 0 0 (by simp [unp])
  have l2 := (vkLoop_spec N paths hT choose hc ek hek ys fuel 0 0 0 (by simp) (by simp) (by omega)).2.1
  unfold vk at he ⊢
  generalize vkLoop N choose ek ys fuel 0 0 0 = r at hu0 l2 he ⊢
  obtain ⟨stp, pos, out, hi⟩ := r
  simp only at hu0 l2 he ⊢
  cases stp
  · simp only [Bool.false_eq_true, ↓reduceIte] at he ⊢
    have hpl := (l2 rfl).2
    by_cases hp : pos = ys.length
    · simp [hp] at he
    simp only [hp, ↓reduceIte] at he ⊢
    have hut := cwfLoop_unp (ys.drop pos) (ReplacementRest.U16_drop hu pos) (probe (ys.drop pos))
      (fun p h1 h2 => hpr _ p (ReplacementRest.U16_drop hu pos) h1 h2) (2 * (ys.drop pos).length + 2) 0 0 0 0 (by simp [unp])
      (by omega) (by simp)
    obtain ⟨-, -, t3⟩ := cwf_spec (ys.drop pos) (ReplacementRest.U16_drop hu pos) (probe (ys.drop pos))
    unfold cwf at he ⊢ t3
    generalize cwfLoop (ys.drop pos) (probe (ys.drop pos)) (2 * (ys.drop pos).length + 2) 0 0 0 0 = t
      at he hut t3 ⊢
    by_cases h0 : t.err = 0
    · simp [h0] at he
    simp only [h0, ↓reduceIte] at he ⊢
    rcases t3 with h | ⟨h1, -⟩
    · exact absurd h h0
    rw [List.take_add]; exact unp_append _ _ hu0 (hut h1)
  · simpa using hu0

/-! ## The `_safe` drivers with `SLSpec'` and `VKSpec'` -/

/-- `ReplacementRest.dLoop_safe`, with `SLSpec'` and `VKSpec'`: `SL` is only
applied to the units before a reported `SURROGATE`
(src/implementation.cpp:2061-2067). -/
theorem dLoop_safe' (VK : List Nat → ReplacementRest.KR) (SL : List Nat → Nat)
    (SK : List Nat → Nat → ReplacementRest.FR)
    (hVK : VKSpec' VK) (hSL : SLSpec' SL) (hSK : ReplacementRest.FSpec SK) (xs : List Nat)
    (hu : U16 xs) (cap : Nat) :
    ∀ f pos oc cap' hi, oc + cap' = cap → hi ≤ cap → pos ≤ xs.length →
      (ReplacementRest.dLoop VK SL SK xs f pos oc cap' hi).hi ≤ cap ∧
      (ReplacementRest.dLoop VK SL SK xs f pos oc cap' hi).outp ≤ cap ∧
      (ReplacementRest.dLoop VK SL SK xs f pos oc cap' hi).inp ≤ xs.length ∧
      ((ReplacementRest.dLoop VK SL SK xs f pos oc cap' hi).err = 1 ∨
        (ReplacementRest.dLoop VK SL SK xs f pos oc cap' hi).err = 2 →
        (ReplacementRest.dLoop VK SL SK xs f pos oc cap' hi).inp < xs.length) := by
  have tail : ∀ pos oc cap' hi, oc + cap' = cap → hi ≤ cap → pos ≤ xs.length →
      (ReplacementRest.dTail SK xs pos oc cap' hi).hi ≤ cap ∧
      (ReplacementRest.dTail SK xs pos oc cap' hi).outp ≤ cap ∧
      (ReplacementRest.dTail SK xs pos oc cap' hi).inp ≤ xs.length ∧
      ((ReplacementRest.dTail SK xs pos oc cap' hi).err = 1 ∨
        (ReplacementRest.dTail SK xs pos oc cap' hi).err = 2 →
        (ReplacementRest.dTail SK xs pos oc cap' hi).inp < xs.length) := by
    intro pos oc cap' hi h1 h2 h3
    unfold ReplacementRest.dTail
    split
    · simp only; omega
    · obtain ⟨s1, s2, s3, s4⟩ := hSK (xs.drop pos) cap' (ReplacementRest.U16_drop hu pos)
      simp only [List.length_drop] at s3 s4 ⊢
      refine ⟨by omega, by omega, by omega, fun h => ?_⟩
      have := s4 h; omega
  intro f
  induction f with
  | zero => intro pos oc cap' hi h1 h2 h3; simp [ReplacementRest.dLoop]; omega
  | succ f ih =>
    intro pos oc cap' hi h1 h2 h3
    unfold ReplacementRest.dLoop
    simp only
    split
    · exact tail pos oc cap' hi h1 h2 h3
    generalize hrl : (if min (xs.length - pos) (cap' / 3) < xs.length - pos ∧
        isHigh (g xs (pos + min (xs.length - pos) (cap' / 3) - 1)) = true
      then min (xs.length - pos) (cap' / 3) - 1 else min (xs.length - pos) (cap' / 3)) = rl
    have hrl1 : rl ≤ xs.length - pos ∧ 3 * rl ≤ cap' := by
      rw [← hrl]; split <;> omega
    split
    · exact tail pos oc cap' hi h1 h2 h3
    have hlen : ((xs.drop pos).take rl).length = rl := by simp; omega
    have hU := ReplacementRest.U16_take (ReplacementRest.U16_drop hu pos) rl
    obtain ⟨k1, k2, k3⟩ := hVK.1 ((xs.drop pos).take rl) hU
    have k4 := hVK.2 ((xs.drop pos).take rl) hU
    have hL := ReplacementRest.L_w8_le ((xs.drop pos).take rl)
    rw [hlen] at hL k3
    generalize VK ((xs.drop pos).take rl) = r at *
    split
    · rename_i he
      have hc := k3 he
      have hcl := k4 he
      rw [List.take_take, Nat.min_eq_left (by omega)] at hcl
      rw [hSL _ (ReplacementRest.U16_take (ReplacementRest.U16_drop hu pos) _) hcl]
      have := ReplacementRest.L_w8_le ((xs.drop pos).take r.cnt)
      have : ((xs.drop pos).take r.cnt).length = r.cnt := by simp; omega
      simp only
      refine ⟨by omega, by omega, by omega, fun _ => by omega⟩
    · rename_i he
      have hc := k2 (by simpa using he)
      apply ih <;> omega

theorem dSafe_spec' (VK : List Nat → ReplacementRest.KR) (SL : List Nat → Nat)
    (SK : List Nat → Nat → ReplacementRest.FR)
    (hVK : VKSpec' VK) (hSL : SLSpec' SL) (hSK : ReplacementRest.FSpec SK) :
    ReplacementRest.FSpec (ReplacementRest.dSafe VK SL SK) := by
  intro ys cap hu
  unfold ReplacementRest.dSafe
  split
  · simp only; omega
  have := dLoop_safe' VK SL SK hVK hSL hSK ys hu cap (ys.length + 1) 0 0 cap 0 (by omega)
    (by omega) (by omega)
  exact ⟨this.1, this.2.1, this.2.2.1, fun h => Or.inl (this.2.2.2 h)⟩

/-! ## End to end, with each kernel's length function -/

/-- The six generic kernels: `(path table, utf8_length_from_utf16)`, as
dispatched in their implementation.cpp (`LengthsSpec` table): westmere and lsx
`bmU8of16 8`, haswell and lasx `bmU8of16 16`, arm64 `arm8of16`, ppc64
`genU8of16`. -/
def kernels16 : List (List Iter × (List Nat → Nat)) :=
  [(Arch.Westmere.utf16to8, LengthsGeneric.bmU8of16 8),
   (Arch.Haswell.utf16to8, LengthsGeneric.bmU8of16 16),
   (Arch.Arm64.utf16to8, LengthsArch.arm8of16),
   (Arch.Ppc64.utf16to8, LengthsGeneric.genU8of16),
   (Kernels.utf16to8_lsx, LengthsGeneric.bmU8of16 8),
   (Arch.Lasx.utf16to8, LengthsGeneric.bmU8of16 16)]

theorem kernels16_ok : ∀ k ∈ kernels16, TableOK (need 16 12 false) k.1 ∧ SLSpec' k.2 := by
  intro k hk
  obtain ⟨t1, t2, t3, t4, t5, t6⟩ := tables_ok
  simp only [kernels16, List.mem_cons, List.mem_nil_iff, or_false] at hk
  rcases hk with rfl | rfl | rfl | rfl | rfl | rfl
  · exact ⟨t1, SLSpec'_bytemask 8⟩
  · exact ⟨t2, SLSpec'_bytemask 16⟩
  · exact ⟨t3, SLSpec'_arm64⟩
  · exact ⟨t4, SLSpec'_ppc64⟩
  · exact ⟨t5, SLSpec'_bytemask 8⟩
  · exact ⟨t6, SLSpec'_bytemask 16⟩

/-- **`convert_utf16_to_utf8_with_replacement_safe`
(src/implementation.cpp:2494-2539) on westmere, haswell, arm64, ppc64, lsx and
lasx, with each kernel's own `utf8_length_from_utf16`, from the entry point,
on every input of 16-bit units and every `utf8_len`**: every store ends inside
`utf8_len` and so does `output_count`. No length hypothesis; the model
hypotheses are those of `vk` (`choose` into the table, `CleanChoice`, `ek`
inside the input, a sound probe). -/
theorem replSafe_kernels (k : List Iter × (List Nat → Nat)) (hk : k ∈ kernels16)
    (choose : List Nat → Iter) (hc : ∀ us, need 16 12 false ≤ us.length → choose us ∈ k.1)
    (ek : List Nat → Nat) (hek : ∀ us, ek us < 23) (hcl : CleanChoice (need 16 12 false) choose ek)
    (probe : List Nat → Nat → Bool) (hpr : ProbeOK probe) (fuel : Nat)
    (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.tLoop (ReplacementRest.dSafe (vk (need 16 12 false) choose ek probe fuel) k.2
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).hi ≤ cap ∧
    (ReplacementRest.tLoop (ReplacementRest.dSafe (vk (need 16 12 false) choose ek probe fuel) k.2
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).outp ≤ cap := by
  obtain ⟨hT, hSL⟩ := kernels16_ok k hk
  have hVK := vkspec'_of_model _ _ hT choose hc ek (ek_inside ek hek) hcl probe hpr fuel
  have h := ReplacementRest.tLoop_safe _ _ (dSafe_spec' _ k.2 _ hVK hSL (ReplacementRest.cwe_spec probe))
    (ReplacementRest.crs_spec _ (ReplacementRest.cwe_spec probe)) xs hu cap (xs.length + 1) 0 0 0
    (by omega) (by omega) (by omega)
  exact ⟨h.1, h.2.1⟩

/-- **`convert_utf16_to_utf8_safe_with_details`** (src/implementation.cpp:2028-2091),
same kernels and length functions: stores and `output_count` within
`utf8_len`. No length hypothesis. -/
theorem dSafe_kernels (k : List Iter × (List Nat → Nat)) (hk : k ∈ kernels16)
    (choose : List Nat → Iter) (hc : ∀ us, need 16 12 false ≤ us.length → choose us ∈ k.1)
    (ek : List Nat → Nat) (hek : ∀ us, ek us < 23) (hcl : CleanChoice (need 16 12 false) choose ek)
    (probe : List Nat → Nat → Bool) (hpr : ProbeOK probe) (fuel : Nat)
    (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.dSafe (vk (need 16 12 false) choose ek probe fuel) k.2
      (ReplacementRest.cwe probe) xs cap).hi ≤ cap ∧
    (ReplacementRest.dSafe (vk (need 16 12 false) choose ek probe fuel) k.2
      (ReplacementRest.cwe probe) xs cap).outp ≤ cap := by
  obtain ⟨hT, hSL⟩ := kernels16_ok k hk
  have h := dSafe_spec' _ k.2 _ (vkspec'_of_model _ _ hT choose hc ek (ek_inside ek hek) hcl probe
    hpr fuel) hSL (ReplacementRest.cwe_spec probe) xs cap hu
  exact ⟨h.1, h.2.1⟩

/-- The six generic kernels for UTF-8 → UTF-16: `(xm, path list,
utf16_length_from_utf8)` (`LengthsSpec` table): westmere `bmU16of8 16`,
haswell `bmU16of8 32`, ppc64 `genU16of8` (`xm = false`); arm64 `genU16of8`,
lsx `bmU16of8 16`, lasx `bmU16of8 32` (`xm = true`). -/
def kernels8 : List (Bool × List MPath × (List Nat → Nat)) :=
  [(false, Utf8.x86_utf16, LengthsGeneric.bmU16of8 16),
   (false, Arch.Haswell.utf8to16, LengthsGeneric.bmU16of8 32),
   (false, Arch.Ppc64.ppc_utf16, LengthsGeneric.genU16of8),
   (true, Utf8.arm_utf16, LengthsGeneric.genU16of8),
   (true, Arch.Lsx.utf8to16, LengthsGeneric.bmU16of8 16),
   (true, Arch.Lasx.lasx_utf16, LengthsGeneric.bmU16of8 32)]

theorem kernels8_ok : ∀ k ∈ kernels8, gap4 k.2.1 = true ∧ ReplacementRest.ULSpec k.2.2 := by
  intro k hk
  obtain ⟨g1, g2, g3, -, g5, g6, g7⟩ := gap_all
  simp only [kernels8, List.mem_cons, List.mem_nil_iff, or_false] at hk
  rcases hk with rfl | rfl | rfl | rfl | rfl | rfl
  · exact ⟨g1, LengthsSpec.ULSpec_bytemask 16⟩
  · exact ⟨g2, LengthsSpec.ULSpec_bytemask 32⟩
  · exact ⟨g3, LengthsSpec.ULSpec_generic⟩
  · exact ⟨g7, LengthsSpec.ULSpec_generic⟩
  · exact ⟨g5, LengthsSpec.ULSpec_bytemask 16⟩
  · exact ⟨g6, LengthsSpec.ULSpec_bytemask 32⟩

/-- **`convert_utf8_to_utf16*_with_replacement` on the six generic kernels,
with each kernel's own `utf16_length_from_utf8`**: it returns
`utf16_length_from_utf8_with_replacement` and every store ends inside it. No
length hypothesis; the model hypotheses are `BOut` and `CVOut`. -/
theorem convertRepl_kernels (k : Bool × List MPath × (List Nat → Nat)) (hk : k ∈ kernels8)
    (K : List Nat → ReplacementRest.BRes)
    (hK : ∀ win, Bytes win → win ≠ [] → BOut k.1 k.2.1 win (K win))
    (d m : Nat) (hdm : CVMargin d m) (CV : List Nat → Nat × Nat)
    (hCV : ∀ v, ValidExact.Valid8 v → CVOut d m v (CV v)) (xs : List Nat) (hb : Bytes xs) :
    (ReplacementRest.convertRepl K (ValidatorSound.vecV k.1) CV k.2.2 xs).1 =
      lenRepl (ValidatorSound.vecV k.1) xs ∧
    (ReplacementRest.convertRepl K (ValidatorSound.vecV k.1) CV k.2.2 xs).2 ≤
      lenRepl (ValidatorSound.vecV k.1) xs :=
  convertRepl_generic k.1 k.2.1 (kernels8_ok k hk).1 K hK d m hdm CV hCV k.2.2 (kernels8_ok k hk).2 xs hb

/-- **`transcode_utf8_to_utf16_with_replacement`**, the same kernels: returns
`count xs`, stores inside it. No length hypothesis. -/
theorem trRun_kernels (k : Bool × List MPath × (List Nat → Nat)) (hk : k ∈ kernels8)
    (K : List Nat → ReplacementRest.BRes)
    (hK : ∀ win, Bytes win → win ≠ [] → BOut k.1 k.2.1 win (K win))
    (d m : Nat) (hdm : CVMargin d m) (CV : List Nat → Nat × Nat)
    (hCV : ∀ v, ValidExact.Valid8 v → CVOut d m v (CV v)) (xs : List Nat) (hb : Bytes xs) :
    ∃ w h, ReplacementRest.trRun K (ValidatorSound.vecV k.1) CV k.2.2 xs = some (w, h) ∧
      w = count xs ∧ h ≤ count xs :=
  trRun_generic k.1 k.2.1 (kernels8_ok k hk).1 K hK d m hdm CV hCV k.2.2 (kernels8_ok k hk).2 xs hb

/-- `arm64_not_SLSpec` does not reach these theorems: on 32 units ending with
a high surrogate arm64's length is 2 more than the per-unit sum, but that
input has an unpaired surrogate, so `SLSpec'` does not constrain it. -/
example : LengthsArch.arm8of16 (List.replicate 31 0x41 ++ [0xD800]) =
      L ((List.replicate 31 0x41 ++ [0xD800]).map w8) + 2 ∧
    unp (List.replicate 31 0x41 ++ [0xD800]) = 1 := by decide

end SafetyMargins.ReplacementLengths
