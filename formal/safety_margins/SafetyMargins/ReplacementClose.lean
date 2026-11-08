import SafetyMargins.ReplacementRest
import SafetyMargins.ValidatorSound
/-
The hypotheses of the conversions with replacement, discharged from the loop
models, and the two `_safe` entry points not modelled before.

`ReplacementRest` proves the drivers safe under contracts on the vector code:
`BSpec` (bulk UTF-8 → UTF-16 `_with_errors` call), `VSpec2` (vector
`validate_utf8_with_counts`), `CVSpec` (`convert_valid_prefix`), `VKSpec`
(vector UTF-16 → UTF-8 `_with_errors`), `ULSpec`/`SLSpec` (length functions).
Here, for the kernels built from src/generic (westmere, haswell, arm64, ppc64,
lsx, lasx), each of the first four is proved for every function whose
results are results of a loop model, composing the existing models:

* **`VSpec2`** (`vecV_spec2`): `ValidatorSound.vecV` meets it; a flagged
  input never succeeds (`flag_complete`, `vecV_spec`).
* **`BSpec`** (`bspec_of_model`): the bulk kernel's returns are
  `ValidatorSound.KOut` (`kout_spec`); its stores (`KStore`) are the calls of
  `ValidatorSound.KR` (a path of the kernel's list, overshoot at most 4,
  `Replacement.gap_all`), the ASCII stores, and the scalar run of the rewind
  (`rewind_store`: the bytes it steps back over weigh at most 2 units). Each
  ends at most 4 units past `L` of a valid prefix (`kstore_spec`).
* **`CVSpec`** (`cvspec_of_model`): `convert_valid` is `ValidExact.VX` and
  the scalar tail; its stores fit by `ValidExact.valid_utf16_vx` (margin 9)
  and `valid_utf16_arm_vx` (arm64, margin 3), `vx_ascii_store`, `vx_tail`.
* **`VKSpec`** (`vkspec_of_model`): `vk` is the vector loop over the path
  table (`vkLoop`, the cursor at `L` of the consumed units, `safe_of_paths`'
  store bound per iteration) and the scalar `convert_with_errors<…, false>`
  (`cwfLoop`, transcribed); `tables_ok` for the six tables.

End to end (each from the entry point, on every input, every store inside
the documented buffer):
* `convertRepl_generic`: `convert_utf8_to_utf16*_with_replacement`; the
  buffer is `utf16_length_from_utf8_with_replacement`. Hypothesis: `ULSpec`.
  `trRun_generic`: the driver `transcode_utf8_to_utf16_with_replacement`.
* `replSafe_generic`: `convert_utf16_to_utf8_with_replacement_safe`;
  `dSafe_generic`: `convert_utf16_to_utf8_safe_with_details`; the buffer is
  `utf8_len`. Hypothesis: `SLSpec`.
* `pSafe_generic`: the plain `convert_utf16_to_utf8_safe`
  (src/implementation.cpp:1971-2018, new model `pLoop` over the plain kernel `pk`).
  No hypothesis.
* `latin1Safe`: `convert_latin1_to_utf8_safe` (src/implementation.cpp:2693-2717,
  new model `l8Loop`, the kernel `lk` over the Latin-1 path tables, including
  icelake, and the scalar `convert_safe`, `csLoop`). No hypothesis.
`kScalar`/`cvScalar` show the model hypotheses are satisfiable
(`convertRepl_scalarKernels`); `replSafe_westmere`, `latin1Safe_westmere`
instantiate the tables.

Read from the C++, as in the files these build on: the transcriptions (the
path tables, the call shapes and `q.f ≤ p + 1` of each call, the per-path
deficit `s ≤ p + d` of `convert_valid`, that a vector-loop iteration
advances the cursor by `L` of what it consumed); that a `SURROGATE` return
of the UTF-16 → UTF-8 vector loop is less than 23 units past the iteration
(`ek`); that the scalar tails write one character at a time; the ASCII
probes are oracles. The theorems hold for every fuel.

Not discharged: icelake and rvv for `BSpec`, `CVSpec`, `VKSpec`, and the
icelake/rvv validators for `VSpec2`. Their store bounds exist
(`Icelake.safe_utf8`, `IcelakeReal.safe_utf16to8_kernel`,
`RvvExact.utf16to8_safe`, `RvvExact.utf8_loop_safe`), but their validation
(error position and valid prefix) and their returned counts are not
modelled, and `BSpec`/`VSpec2`/`VKSpec` need both. `ULSpec`/`SLSpec` are left
to the length-function proofs.
-/
namespace SafetyMargins.ReplacementClose
open SafetyMargins SafetyMargins.Replacement SafetyMargins.ReplacementMore
open SafetyMargins.ValidExact (Valid8 run)
open SafetyMargins.Utf8 (w16 MPath)
set_option autoImplicit false
set_option linter.unusedSimpArgs false

/-! # 1. The vector validator meets `VSpec2` -/

/-- **`generic_validate_utf8_with_counts` meets `ReplacementRest.VSpec2`**
(westmere, haswell, ppc64: `xm = false`; arm64, lsx, lasx: `xm = true`): on
success `input_count` is the whole input. The flagged path never succeeds:
a flag means the input is not valid (`flag_complete`), and a successful
rewind means it is (`rewind_spec`). -/
theorem vecV_spec2 (xm : Bool) : ReplacementRest.VSpec2 (ValidatorSound.vecV xm) := by
  refine ⟨ValidatorSound.vecV_spec xm, fun ys hb hne hok => ?_⟩
  unfold ValidatorSound.vecV at hok ⊢
  by_cases hf : (ValidatorSound.checkAll xm ys).1 = true
  · exfalso
    simp only [hf, ↓reduceIte] at hok
    have h1 := ((ValidatorSound.vecV_spec xm) ys hb hne).1
    unfold ValidatorSound.vecV at h1
    simp only [hf, ↓reduceIte] at h1
    have hv := (h1 hok).1
    have := ValidatorSound.flag_complete xm ys hb hv
    rw [hf] at this; cases this
  · have hf' : (ValidatorSound.checkAll xm ys).1 = false := by simpa using hf
    simp only [hf', Bool.false_eq_true, ↓reduceIte]

/-! # 2. The bulk UTF-8 → UTF-16 `_with_errors` kernel meets `BSpec`

`ValidatorSound.KR`/`KOut` model `validating_transcoder::convert_with_errors`
(src/generic/utf8_to_utf16/utf8_to_utf16.h:270-377) and its returns. Here are
its stores, every one of them (`KStore`):
* `call`: a `convert_masked_utf8_to_utf16` call (:346-349) stores `q.s` units
  at the cursor `o`, for a path `q` of the kernel's list with `q.f ≤ p + 1`
  (`p` the units it produces, `Utf8.MPath`, the reading of `Exact.xreach_okK`);
* `ascii`: the 64-unit ASCII stores (:296-299) and those of
  `convert_ascii_tail` (:140-171, :357-360), which end at the cursor after the
  ASCII bytes (the last, backward, store ends at the end of the input,
  `Exact.tail_last`);
* `rewind`: `rewind_and_convert_with_errors`
  (include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:296-342) runs the
  scalar `convert_with_errors` (:140-277) at the cursor `o` from the leading
  byte `t - i` it finds; that loop writes one unit at a time, the units of the
  characters it accepts (`vwcF`), so every store ends at most at `o` plus
  those units. -/

theorem findBack_le (bs : List Nat) (t : Nat) :
    ∀ n i k, ValidatorSound.findBack bs t i n = some k → k ≤ i + n := by
  intro n
  induction n with
  | zero =>
    intro i k h
    unfold ValidatorSound.findBack at h
    split at h <;> simp at h; omega
  | succ n ih =>
    intro i k h
    unfold ValidatorSound.findBack at h
    split at h
    · simp at h; omega
    · have := ih (i + 1) k h; omega

theorem sumUpTo_head (f : Nat → Nat) :
    ∀ i, (∀ k, 1 ≤ k → k < i → f k = 0) → f 0 ≤ 2 → Utf8.sumUpTo f i ≤ 2
  | 0, _, _ => by simp [Utf8.sumUpTo]
  | i + 1, h, h0 => by
    simp only [Utf8.sumUpTo]
    have ih := sumUpTo_head f i (fun k h1 h2 => h k h1 (by omega)) h0
    by_cases hi : i = 0
    · subst hi; simp [Utf8.sumUpTo]; omega
    · rw [h i (by omega) (by omega)]; omega

theorem w16_le2 (b : Nat) : w16 b ≤ 2 := by
  unfold w16; split <;> (try split) <;> omega

/-- The bytes `t - i .. t - 1` the rewind steps back over: a leading byte
followed by continuation bytes, so at most 2 units. -/
theorem slice_le (bs : List Nat) (t i : Nat)
    (hc : ∀ m, 0 ≤ m → m < i → Validator.isCont (bs.getD (t - m) 0) = true) (hit : i ≤ t) :
    L (((bs.drop (t - i)).take i).map w16) ≤ 2 := by
  rw [List.map_take, List.map_drop, Utf8.L_window]
  apply sumUpTo_head
  · intro k h1 h2
    by_cases hk : t - i + k < bs.length
    · rw [Exact.getD_map w16 bs _ hk]
      have := hc (i - k) (Nat.zero_le _) (by omega)
      rw [show t - (i - k) = t - i + k by omega] at this
      exact ValidExact.w16_cont _ this
    · rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by simp; omega)]; rfl
  · by_cases hk : t - i + 0 < bs.length
    · rw [Exact.getD_map w16 bs _ hk]; exact w16_le2 _
    · rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by simp; omega)]; simp

theorem valid_append {a b : List Nat} (ha : Valid8 a) (hb : Valid8 b) : Valid8 (a ++ b) := by
  unfold Valid8 at *; rw [ValidExact.run_append, ha, hb]

/-- **The rewind's stores.** From a block head `t` after a valid prefix, the
scalar run from the leading byte `t - i` stores at most `L` of a valid prefix
plus 2. -/
theorem rewind_store (bs : List Nat) (hb : Bytes bs) (t : Nat) (ht : t < bs.length)
    (h0 : run 0 (bs.take t) = 0) (i : Nat) (hf : ValidatorSound.findBack bs t 0 t = some i) :
    ∃ c, c ≤ bs.length ∧ Valid8 (bs.take c) ∧
      L ((bs.map w16).take t) + (vwcF (bs.drop (t - i)).length (bs.drop (t - i))).2.2 ≤
        L ((bs.take c).map w16) + 2 := by
  obtain ⟨f1, -⟩ := ValidatorSound.findBack_spec bs t t 0
  obtain ⟨-, hl, hc⟩ := f1 i hf
  have hit : i ≤ t := by have := findBack_le bs t t 0 i hf; omega
  have hvj : run 0 (bs.take (t - i)) = 0 := by
    by_cases hi0 : i = 0
    · subst hi0; simpa using h0
    · have hne8 := ValidExact.run_take_ne8 h0 (t - i + 1)
      rw [List.take_take, show min (t - i + 1) t = t - i + 1 by omega] at hne8
      exact ValidatorSound.lead_zero bs (t - i) (by omega) hne8 hl
  have hsl := slice_le bs t i hc hit
  have hsplit : L ((bs.map w16).take t) =
      L ((bs.map w16).take (t - i)) + L (((bs.drop (t - i)).take i).map w16) := by
    rw [show t = (t - i) + i from by omega, Utf8.L_take_add, Nat.add_sub_cancel,
      List.map_take, List.map_drop]
  obtain ⟨s1, s2⟩ := vwcF_spec (bs.drop (t - i)).length (bs.drop (t - i))
    (bytes_drop hb _) (Nat.le_refl _)
  cases hok : (vwcF (bs.drop (t - i)).length (bs.drop (t - i))).1
  · obtain ⟨hlt, hv, hu, -⟩ := s2 hok
    generalize (vwcF (bs.drop (t - i)).length (bs.drop (t - i))).2.1 = e at hlt hv hu
    refine ⟨t - i + e, by simp at hlt; omega, ?_, ?_⟩
    · rw [List.take_add]; exact valid_append hvj hv
    · rw [hu, List.take_add, List.map_append, L_append, hsplit, ← List.map_take]
      omega
  · obtain ⟨hv, hu⟩ := s1 hok
    refine ⟨bs.length, Nat.le_refl _, ?_, ?_⟩
    · rw [List.take_length, ← List.take_append_drop (t - i) bs]; exact valid_append hvj hv
    · rw [hu, List.take_length, hsplit, L_map_split bs (t - i), ← List.map_take]
      omega

/-- Every store of the bulk kernel on `bs` (see the section header). `ps` is
the kernel's `convert_masked_utf8_to_utf16` path list. -/
inductive KStore (xm : Bool) (ps : List MPath) (bs : List Nat) : Nat → Prop
  | call (pos t o c p : Nat) (q : MPath) (h : ValidatorSound.KR xm bs true pos t o)
      (hs : Exact.XStep (bs.map w16) t c p) (hd : t + c + 1 ≤ pos + 64)
      (hq : q ∈ ps) (hf : q.f ≤ p + 1) : KStore xm ps bs (o + q.s)
  | ascii (t o a : Nat) (h : ValidatorSound.KR xm bs false t t o) (hle : t + a ≤ bs.length)
      (hascii : ∀ i, i < a → bs.getD (t + i) 0 < 0x80) : KStore xm ps bs (o + a)
  | rewind (t o i : Nat) (h : ValidatorSound.KR xm bs false t t o) (ht : t < bs.length)
      (hf : ValidatorSound.findBack bs t 0 t = some i) :
      KStore xm ps bs (o + (vwcF (bs.drop (t - i)).length (bs.drop (t - i))).2.2)

/-- **Every store of the bulk kernel ends at most 4 units past the output of
a valid prefix of its window** (the store clause of `BSpec`), for a path list
of overshoot 4 (`Replacement.gap_all`). -/
theorem kstore_spec (xm : Bool) (ps : List MPath) (hg : gap4 ps = true) (bs : List Nat)
    (hb : Bytes bs) (s : Nat) (h : KStore xm ps bs s) :
    ∃ c, c ≤ bs.length ∧ Valid8 (bs.take c) ∧ s ≤ L ((bs.take c).map w16) + 4 := by
  cases h with
  | call pos t o c p q h hs hd hq hf =>
    have h' := ValidatorSound.KR.call pos t o c p h hs hd
    obtain ⟨i1, i2, i3, -⟩ := ValidatorSound.kr_inv xm bs hb _ _ _ _ h'
    have := of_decide_eq_true ((List.all_eq_true.mp hg) q hq)
    refine ⟨t + c, i3, i2, ?_⟩
    rw [List.map_take, ← i1]; omega
  | ascii t o a h hle hascii =>
    have h' := ValidatorSound.KR.ascii t o a h hle hascii
    obtain ⟨i1, i2, i3, -⟩ := ValidatorSound.kr_inv xm bs hb _ _ _ _ h'
    exact ⟨t + a, i3, i2, by rw [List.map_take, ← i1]; omega⟩
  | rewind t o i h ht hf =>
    obtain ⟨i1, i2, -, -⟩ := ValidatorSound.kr_inv xm bs hb _ _ _ _ h
    obtain ⟨c, hc, hv, hle⟩ := rewind_store bs hb t ht i2 i hf
    exact ⟨c, hc, hv, by rw [i1]; omega⟩

/-- A result of the bulk kernel on `win`, as the driver reads it
(`ReplacementRest.BRes`): `ok` and `cnt` are a return of the model (`KOut`),
`verr` is any value (the driver tests it only on an error, and every error of
this kernel is a validation error), and `se`, the end of the stores, is one of
the stores (or 0 when there is none). -/
def BOut (xm : Bool) (ps : List MPath) (win : List Nat) (r : ReplacementRest.BRes) : Prop :=
  ValidatorSound.KOut xm win r.ok r.cnt ∧ (r.se = 0 ∨ ∃ s, KStore xm ps win s ∧ r.se ≤ s)

/-- **`BSpec` holds for the generic kernels.** Any function that returns, on
each window, a result of the model meets the contract `tr_safe` and
`convertRepl_safe` assume. -/
theorem bspec_of_model (xm : Bool) (ps : List MPath) (hg : gap4 ps = true)
    (K : List Nat → ReplacementRest.BRes)
    (hK : ∀ win, Bytes win → win ≠ [] → BOut xm ps win (K win)) : ReplacementRest.BSpec K := by
  intro win hb hne
  obtain ⟨hout, hse⟩ := hK win hb hne
  obtain ⟨k1, k2⟩ := ValidatorSound.kout_spec xm win hb _ _ hout
  refine ⟨k1, fun hok _ _ => (k2 hok).2, ?_⟩
  rcases hse with h0 | ⟨s, hs, hle⟩
  · exact ⟨0, Nat.zero_le _, rfl, by rw [h0]; exact Nat.zero_le _⟩
  · obtain ⟨c, hc, hv, hs'⟩ := kstore_spec xm ps hg win hb s hs
    exact ⟨c, hc, hv, by omega⟩

/-- The path lists of the generic kernels, with the `must_be_2_3_continuation`
of each (`xm`): westmere (`x86_utf16`), haswell, ppc64 (`xm = false`); arm64,
lsx, lasx (`xm = true`). -/
def genericKernels : List (Bool × List MPath) :=
  [(false, Utf8.x86_utf16), (false, Arch.Haswell.utf8to16), (false, Arch.Ppc64.ppc_utf16),
   (true, Utf8.arm_utf16), (true, Arch.Lsx.utf8to16), (true, Arch.Lasx.lasx_utf16)]

theorem genericKernels_gap4 : ∀ k ∈ genericKernels, gap4 k.2 = true := by
  intro k hk
  obtain ⟨g1, g2, g3, -, g5, g6, g7⟩ := gap_all
  simp only [genericKernels, List.mem_cons, List.mem_nil_iff, or_false] at hk
  rcases hk with rfl | rfl | rfl | rfl | rfl | rfl <;> assumption

/-! # 3. `convert_valid_prefix` meets `CVSpec`

`convert_valid_prefix` (src/utf8_to_utf16_with_replacement.inl.h:26-34) is
`convert_valid_utf8_to_utf16{le,be}`, which for westmere, haswell, arm64,
ppc64, lsx and lasx is `utf8_to_utf16::convert_valid`
(src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:9-77): the loop
`ValidExact.VX` (`pos + 64 + safety_margin <= size`, :23; margin 9, or 3 on
arm64, :19-22), then the scalar `convert_valid` on the rest (:73-74), which
writes `L` of the rest, one character at a time. Its stores (`CVStore`): a
call storing `s ≤ p + d` units at the cursor (`d = 4`, or `d = 2` on arm64,
the per-path deficits of `ValidExact`), a 64-unit ASCII block (:26-29), the
scalar tail. -/

inductive CVStore (d m : Nat) (bs : List Nat) : Nat → Prop
  | call (pos t o c p s : Nat) (h : ValidExact.VX w16 bs true pos t o) (ht : t < pos + 52)
      (hx : Exact.XStep (bs.map w16) t c p) (hd : t + c + 1 ≤ pos + 64)
      (hloop : pos + 64 + m ≤ bs.length) (hs : s ≤ p + d) : CVStore d m bs (o + s)
  | ascii (t o : Nat) (h : ValidExact.VX w16 bs false t t o) (hlen : t + 64 ≤ bs.length)
      (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) : CVStore d m bs (o + 64)
  | tail (t o : Nat) (h : ValidExact.VX w16 bs false t t o) :
      CVStore d m bs (o + L ((bs.map w16).drop t))

/-- A result `(returned units, end of stores)` of the model: the loop exits
at some head `(t, o)` and the tail adds `L` of the rest; the end of the
stores is one of the stores, or 0. -/
def CVOut (d m : Nat) (bs : List Nat) (r : Nat × Nat) : Prop :=
  (∃ t o, ValidExact.VX w16 bs false t t o ∧ r.1 = o + L ((bs.map w16).drop t)) ∧
  (r.2 = 0 ∨ ∃ s, CVStore d m bs s ∧ r.2 ≤ s)

/-- The margins of `valid_utf8_to_utf16.h`: deficit 4 with margin 9 (x86,
ppc64, lsx, lasx), deficit 2 with margin 3 (arm64). -/
def CVMargin (d m : Nat) : Prop := (d = 4 ∧ 9 ≤ m) ∨ (d = 2 ∧ 3 ≤ m)

theorem cvstore_le (d m : Nat) (hdm : CVMargin d m) (bs : List Nat) (hv : Valid8 bs) (s : Nat)
    (h : CVStore d m bs s) : s ≤ L (bs.map w16) := by
  cases h with
  | call pos t o c p s h ht hx hd hloop hs =>
    rcases hdm with ⟨rfl, hm⟩ | ⟨rfl, hm⟩
    · exact ValidExact.valid_utf16_vx hv h ht hx hd hloop hm hs
    · exact ValidExact.valid_utf16_arm_vx hv h ht hx hd hloop hm hs
  | ascii t o h hlen hascii => exact ValidExact.vx_ascii_store ValidExact.wok_w16 hv h hlen hascii
  | tail t o h => exact Nat.le_of_eq (ValidExact.vx_tail ValidExact.wok_w16 hv h).1

/-- **`CVSpec` holds for the generic `convert_valid`.** Any function that
returns, on valid input, a result of the model is exact and stores inside
`L` of its input. -/
theorem cvspec_of_model (d m : Nat) (hdm : CVMargin d m) (CV : List Nat → Nat × Nat)
    (hCV : ∀ v, Valid8 v → CVOut d m v (CV v)) : ReplacementRest.CVSpec CV := by
  intro v hv
  obtain ⟨⟨t, o, hx, h1⟩, h2⟩ := hCV v hv
  refine ⟨by rw [h1]; exact (ValidExact.vx_tail ValidExact.wok_w16 hv hx).1, ?_⟩
  rcases h2 with h0 | ⟨s, hs, hle⟩
  · rw [h0]; exact Nat.zero_le _
  · exact Nat.le_trans hle (cvstore_le d m hdm v hv s hs)

/-! # 4. UTF-8 → UTF-16 with replacement, end to end (generic kernels) -/

/-- **`convert_utf8_to_utf16{le,be,}_with_replacement`
(src/utf8_to_utf16_with_replacement.inl.h:278-323) on westmere, haswell,
arm64, ppc64, lsx and lasx, from the entry point, on every input.** With the
vector validator `vecV xm` (`validate_utf8_with_counts`, which also computes
the buffer size), a bulk kernel `K` and a `convert_valid_prefix` `CV` whose
results are those of the models (`BOut`, `CVOut`), the conversion returns the
buffer size `utf16_length_from_utf8_with_replacement` (`lenRepl (vecV xm)`)
and every store ends inside it. Remaining hypothesis: `ULSpec UL`
(`utf16_length_from_utf8` exact on valid input). -/
theorem convertRepl_generic (xm : Bool) (ps : List MPath) (hg : gap4 ps = true)
    (K : List Nat → ReplacementRest.BRes)
    (hK : ∀ win, Bytes win → win ≠ [] → BOut xm ps win (K win))
    (d m : Nat) (hdm : CVMargin d m) (CV : List Nat → Nat × Nat)
    (hCV : ∀ v, Valid8 v → CVOut d m v (CV v))
    (UL : List Nat → Nat) (hUL : ReplacementRest.ULSpec UL) (xs : List Nat) (hb : Bytes xs) :
    (ReplacementRest.convertRepl K (ValidatorSound.vecV xm) CV UL xs).1 =
      lenRepl (ValidatorSound.vecV xm) xs ∧
    (ReplacementRest.convertRepl K (ValidatorSound.vecV xm) CV UL xs).2 ≤
      lenRepl (ValidatorSound.vecV xm) xs := by
  rw [ValidatorSound.lenRepl_vec xm xs hb]
  exact ReplacementRest.convertRepl_safe K _ CV UL (bspec_of_model xm ps hg K hK) (vecV_spec2 xm)
    (cvspec_of_model d m hdm CV hCV) hUL xs hb

/-- **`transcode_utf8_to_utf16_with_replacement<endian, true>`** (:88-188), the
driver `convert_using_locations` hands the rest to, on its own: it returns
`count xs` and stores inside it, under the same models and `ULSpec`. -/
theorem trRun_generic (xm : Bool) (ps : List MPath) (hg : gap4 ps = true)
    (K : List Nat → ReplacementRest.BRes)
    (hK : ∀ win, Bytes win → win ≠ [] → BOut xm ps win (K win))
    (d m : Nat) (hdm : CVMargin d m) (CV : List Nat → Nat × Nat)
    (hCV : ∀ v, Valid8 v → CVOut d m v (CV v))
    (UL : List Nat → Nat) (hUL : ReplacementRest.ULSpec UL) (xs : List Nat) (hb : Bytes xs) :
    ∃ w h, ReplacementRest.trRun K (ValidatorSound.vecV xm) CV UL xs = some (w, h) ∧
      w = count xs ∧ h ≤ count xs :=
  ReplacementRest.trRun_safe K _ CV UL (bspec_of_model xm ps hg K hK) (vecV_spec2 xm)
    (cvspec_of_model d m hdm CV hCV) hUL xs hb

/-! # 5. The vector UTF-16 → UTF-8 `_with_errors` kernel meets `VKSpec`

`convert_utf16_to_utf8_with_details` of westmere (src/westmere/implementation.cpp:748-767),
haswell (src/haswell/implementation.cpp:721-739), arm64
(src/arm64/implementation.cpp:706-723), ppc64 (src/ppc64/implementation.cpp:530-544), lsx
(src/lsx/implementation.cpp:719-736), lasx (src/lasx/implementation.cpp:817-834): the
vector loop (e.g. src/westmere/sse_convert_utf16_to_utf8.cpp:282-500), then, if it
did not consume everything, the scalar `convert_with_errors<…, false>`
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:93-203) on the rest. The
returned `result` is the error and the input position on an error, else
the output count (include/simdutf/error.h:117-123).

* **The vector loop (`vkLoop`).** As `Basic.written`: while `N` units
  remain, an iteration of the kernel's path table (`choose`, any function
  into the table, as in `safe_of_paths`); the cursor advances by `L` of the
  consumed units; a `stop` iteration is the `SURROGATE` return of the scalar
  fallback, at `buf - start + k - 1`
  (src/westmere/sse_convert_utf16_to_utf8.cpp:485), which is fewer than 23 units
  past the iteration start (`forward ≤ 15`, after at most an 8-unit ASCII
  half: `ek`). The fuel is arbitrary (fuel 0 hands over to the tail), so the
  theorems hold for the C++'s iteration count.
* **The scalar tail (`cwfLoop`).** `convert_with_errors` with
  `check_output = false`: `ReplacementRest.cweLoop` without the capacity
  tests; the 4-unit ASCII probe is an arbitrary oracle. -/

theorem Lw8_succ (xs : List Nat) (pos : Nat) (h : pos < xs.length) :
    L ((xs.take (pos + 1)).map w8) = L ((xs.take pos).map w8) + w8 (g xs pos) := by
  rw [ValidatorSound.take_succ_getD xs pos h, List.map_append, L_append]
  simp [g]

theorem Lw8_take_le (xs : List Nat) (k : Nat) : L ((xs.take k).map w8) ≤ L (xs.map w8) := by
  rw [List.map_take]; exact L_take_le _ _

theorem Lw8_take_mono (xs : List Nat) {a b : Nat} (h : a ≤ b) :
    L ((xs.take a).map w8) ≤ L ((xs.take b).map w8) := by
  have := Lw8_take_le (xs.take b) a
  rwa [List.take_take, Nat.min_eq_left h] at this

section
variable (xs : List Nat) (probe : Nat → Bool)

/-- **`scalar::utf16_to_utf8::convert_with_errors<big_endian, false>`**
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:93-203) with fuel; as
`ReplacementRest.cweLoop` with `check_output = false` (no capacity test).
The plain `scalar::utf16_to_utf8::convert` (:11-91) stores the same bytes and
returns 0 where this one returns `SURROGATE`. -/
def cwfLoop : Nat → Nat → Nat → Nat → Nat → ReplacementRest.FR
  | 0, _, pos, out, hi => ⟨3, pos, out, hi⟩
  | f + 1, burst, pos, out, hi =>
    if burst ≠ 0 then cwfLoop f (burst - 1) (pos + 1) (out + 1) (max hi (out + 1))
    else if pos < xs.length then
      if pos + 4 ≤ xs.length ∧ probe pos = true then cwfLoop f 4 pos out hi
      else
        let w := g xs pos
        if w &&& 0xFF80 == 0 then cwfLoop f 0 (pos + 1) (out + 1) (max hi (out + 1))
        else if w &&& 0xF800 == 0 then cwfLoop f 0 (pos + 1) (out + 2) (max hi (out + 2))
        else if w &&& 0xF800 != 0xD800 then cwfLoop f 0 (pos + 1) (out + 3) (max hi (out + 3))
        else if xs.length ≤ pos + 1 then ⟨1, pos, out, hi⟩
        else if 0x3FF < (w + 65536 - 0xD800) % 65536 then ⟨1, pos, out, hi⟩
        else if 0x3FF < (g xs (pos + 1) + 65536 - 0xDC00) % 65536 then ⟨1, pos, out, hi⟩
        else cwfLoop f 0 (pos + 2) (out + 4) (max hi (out + 4))
    else ⟨0, pos, out, hi⟩

def cwf (ys : List Nat) : ReplacementRest.FR := cwfLoop ys probe (2 * ys.length + 2) 0 0 0 0
end

/-- The scalar tail writes at most `utf8_length_from_utf16` of what it
converted, never runs out of fuel, and reports `SURROGATE` at a unit of the
input. -/
theorem cwfLoop_spec (xs : List Nat) (hu : U16 xs) (probe : Nat → Bool) :
    ∀ f burst pos out hi, out ≤ L ((xs.take pos).map w8) → hi ≤ L ((xs.take pos).map w8) →
      pos ≤ xs.length → (burst ≠ 0 → pos + burst ≤ xs.length) →
      2 * (xs.length - pos) + (if burst = 0 then 1 else 0) ≤ f →
      (cwfLoop xs probe f burst pos out hi).hi ≤ L (xs.map w8) ∧
      (cwfLoop xs probe f burst pos out hi).outp ≤ L (xs.map w8) ∧
      ((cwfLoop xs probe f burst pos out hi).err = 0 ∨
        ((cwfLoop xs probe f burst pos out hi).err = 1 ∧
          (cwfLoop xs probe f burst pos out hi).inp < xs.length)) := by
  intro f
  induction f with
  | zero =>
    intro burst pos out hi _ _ _ h4 h5
    by_cases hb : burst = 0
    · simp [hb] at h5
    · have := h4 hb; simp [hb] at h5; omega
  | succ f ih =>
    intro burst pos out hi h1 h2 h3 h4 h5
    have hall := Lw8_take_le xs pos
    unfold cwfLoop
    by_cases hb : burst ≠ 0
    · rw [ite_eq_left hb]
      have hp := h4 hb
      have hs := Lw8_succ xs pos (by omega)
      have := w8_ge (g xs pos)
      apply ih <;> first | omega | (intro; omega) | (simp [hb] at h5; split <;> omega)
    rw [ite_eq_right hb]
    have hb0 : burst = 0 := by omega
    by_cases hp : pos < xs.length
    · rw [ite_eq_left hp]
      have hs := Lw8_succ xs pos hp
      have hlt := g_lt xs hu pos
      obtain ⟨u1, u2, u3, u4, u5, -, -⟩ := ReplacementRest.uf (g xs pos) hlt
      split
      · apply ih <;> first | omega | (intro; omega) | (simp [hb0] at h5 ⊢; omega)
      simp only
      split
      · rename_i ha
        rw [u1] at ha; have := ReplacementRest.w8_lt80 _ (by simpa using ha)
        apply ih <;> first | omega | (intro; omega) | (simp [hb0] at h5 ⊢; omega)
      split
      · rename_i ha hb'
        rw [u1] at ha; rw [u2] at hb'
        have := ReplacementRest.w8_mid (g xs pos) (by simp at ha; omega) (by simpa using hb')
        apply ih <;> first | omega | (intro; omega) | (simp [hb0] at h5 ⊢; omega)
      split
      · rename_i ha hb' hc
        rw [u2] at hb'; rw [u3] at hc
        have := ReplacementRest.w8_hi (g xs pos) (by simp at hb'; omega) (by simpa using hc)
        apply ih <;> first | omega | (intro; omega) | (simp [hb0] at h5 ⊢; omega)
      by_cases hl1 : xs.length ≤ pos + 1
      · rw [ite_eq_left hl1]; dsimp only; exact ⟨by omega, by omega, Or.inr ⟨rfl, hp⟩⟩
      rw [ite_eq_right hl1]
      by_cases hd1 : 0x3FF < (g xs pos + 65536 - 0xD800) % 65536
      · rw [ite_eq_left hd1]; dsimp only; exact ⟨by omega, by omega, Or.inr ⟨rfl, hp⟩⟩
      rw [ite_eq_right hd1]
      by_cases hd2 : 0x3FF < (g xs (pos + 1) + 65536 - 0xDC00) % 65536
      · rw [ite_eq_left hd2]; dsimp only; exact ⟨by omega, by omega, Or.inr ⟨rfl, hp⟩⟩
      rw [ite_eq_right hd2]
      have hhi : isHigh (g xs pos) = true := by rw [← u4]; simpa using hd1
      obtain ⟨-, -, -, -, v5, -, -⟩ := ReplacementRest.uf (g xs (pos + 1)) (g_lt xs hu _)
      have hlo : isLow (g xs (pos + 1)) = true := by rw [← v5]; simpa using hd2
      have e1 := w8_sur _ (high_sur hhi)
      have e2 := w8_sur _ (low_sur hlo)
      have hs2 := Lw8_succ xs (pos + 1) (by omega)
      rw [show pos + 1 + 1 = pos + 2 by omega] at hs2
      apply ih <;> first | omega | (intro; omega) | (simp [hb0] at h5 ⊢; omega)
    · rw [ite_eq_right hp]; dsimp only
      exact ⟨by omega, by omega, Or.inl rfl⟩

theorem cwf_spec (ys : List Nat) (hu : U16 ys) (probe : Nat → Bool) :
    (cwf probe ys).hi ≤ L (ys.map w8) ∧ (cwf probe ys).outp ≤ L (ys.map w8) ∧
    ((cwf probe ys).err = 0 ∨ ((cwf probe ys).err = 1 ∧ (cwf probe ys).inp < ys.length)) :=
  cwfLoop_spec ys hu probe _ 0 0 0 0 (by simp) (by simp) (by omega) (by simp) (by simp)

section
variable (N : Nat) (choose : List Nat → Iter) (ek : List Nat → Nat) (ys : List Nat)

/-- The vector loop on the units `ys`, with fuel: `(stopped, pos, cursor, end
of stores)`. `choose` and `ek` see the remaining units. -/
def vkLoop : Nat → Nat → Nat → Nat → Bool × Nat × Nat × Nat
  | 0, pos, out, hi => (false, pos, out, hi)
  | f + 1, pos, out, hi =>
    if (ys.drop pos).length < N then (false, pos, out, hi)
    else
      let it := choose (ys.drop pos)
      let ws := (ys.drop pos).map w8
      let hi1 := max hi (out + maxEnd ws it.consumed it.stores)
      if it.stop then (true, pos + ek (ys.drop pos), out, hi1)
      else vkLoop f (pos + it.consumed) (out + L (ws.take it.consumed)) hi1
end

/-- What the loop's path table must satisfy: every store fits at `N`
(`Iter.ok`, the `cur_utf16to8` theorems), and no iteration consumes more
than `N` units. -/
def TableOK (N : Nat) (paths : List Iter) : Prop :=
  paths.all (Iter.ok N) = true ∧ paths.all (fun it => decide (it.consumed ≤ N)) = true

theorem vkLoop_spec (N : Nat) (paths : List Iter) (hT : TableOK N paths)
    (choose : List Nat → Iter) (hc : ∀ us, N ≤ us.length → choose us ∈ paths)
    (ek : List Nat → Nat) (hek : ∀ us, N ≤ us.length → ek us < us.length)
    (ys : List Nat) : ∀ f pos out hi, out = L ((ys.take pos).map w8) → hi ≤ L (ys.map w8) →
      pos ≤ ys.length →
      (vkLoop N choose ek ys f pos out hi).2.2.2 ≤ L (ys.map w8) ∧
      ((vkLoop N choose ek ys f pos out hi).1 = false →
        (vkLoop N choose ek ys f pos out hi).2.2.1 =
          L ((ys.take (vkLoop N choose ek ys f pos out hi).2.1).map w8) ∧
        (vkLoop N choose ek ys f pos out hi).2.1 ≤ ys.length) ∧
      ((vkLoop N choose ek ys f pos out hi).1 = true →
        (vkLoop N choose ek ys f pos out hi).2.1 < ys.length) := by
  intro f
  induction f with
  | zero => intro pos out hi h1 h2 h3; simp only [vkLoop]; exact ⟨h2, fun _ => ⟨h1, h3⟩, by simp⟩
  | succ f ih =>
    intro pos out hi h1 h2 h3
    unfold vkLoop
    by_cases hs : (ys.drop pos).length < N
    · rw [ite_eq_left hs]; exact ⟨h2, fun _ => ⟨h1, h3⟩, by simp⟩
    rw [ite_eq_right hs]
    have hlen : N ≤ (ys.drop pos).length := by omega
    have hmem := hc _ hlen
    have hok : Iter.ok N (choose (ys.drop pos)) = true := (List.all_eq_true.mp hT.1) _ hmem
    have hcons : (choose (ys.drop pos)).consumed ≤ N := by
      have := (List.all_eq_true.mp hT.2) _ hmem; simpa using this
    generalize hit : choose (ys.drop pos) = it at hok hcons
    have hwl : (((ys.drop pos).map w8)).length = (ys.drop pos).length := by simp
    have hstores : ∀ s ∈ it.stores, storeEnd ((ys.drop pos).map w8) it.consumed s ≤
        L ((ys.drop pos).map w8) := by
      intro s hs'
      have hb : decide (placeReq it.consumed s ≤ N) = true := (List.all_eq_true.mp hok) s hs'
      exact storeEnd_le _ (pos_w8 _) N _ (by rw [hwl]; exact hlen) s (of_decide_eq_true hb)
    have hhere : maxEnd ((ys.drop pos).map w8) it.consumed it.stores ≤ L ((ys.drop pos).map w8) := by
      simpa [maxEnd] using maxEnd_le _ _ _ 0 _ (Nat.zero_le _) hstores
    have hsplit : L (ys.map w8) = L ((ys.take pos).map w8) + L ((ys.drop pos).map w8) := by
      rw [← L_append, ← List.map_append, List.take_append_drop]
    have hhi1 : max hi (out + maxEnd ((ys.drop pos).map w8) it.consumed it.stores) ≤
        L (ys.map w8) := by omega
    simp only
    by_cases hstop : it.stop = true
    · rw [ite_eq_left hstop]
      refine ⟨hhi1, fun h => by simp at h, fun _ => ?_⟩
      have := hek _ hlen
      simp at this ⊢; omega
    · rw [ite_eq_right hstop]
      apply ih
      · rw [h1, List.map_take, List.map_take, Utf8.L_take_add, List.map_drop]
      · exact hhi1
      · simp at hlen; omega

section
variable (N : Nat) (choose : List Nat → Iter) (ek : List Nat → Nat)
  (probe : List Nat → Nat → Bool) (fuel : Nat)

/-- **`convert_utf16_to_utf8_with_details`** (the vector `_with_errors`
kernel): the vector loop, then the scalar tail if units remain
(e.g. src/westmere/implementation.cpp:752-766), as the `result` the driver
reads: error code (0 `SUCCESS`, 1 `SURROGATE`), count (output on success,
input position on an error), end of stores. -/
def vk (ys : List Nat) : ReplacementRest.KR :=
  let r := vkLoop N choose ek ys fuel 0 0 0
  if r.1 then ⟨1, r.2.1, r.2.2.2⟩
  else if r.2.1 = ys.length then ⟨0, r.2.2.1, r.2.2.2⟩
  else
    let t := cwf (probe (ys.drop r.2.1)) (ys.drop r.2.1)
    if t.err = 0 then ⟨0, r.2.2.1 + t.outp, max r.2.2.2 (r.2.2.1 + t.hi)⟩
    else ⟨t.err, r.2.1 + t.inp, max r.2.2.2 (r.2.2.1 + t.hi)⟩

/-- **The plain vector `convert_utf16_to_utf8`** (e.g.
src/westmere/implementation.cpp:708-726): the same loop (the plain and
`_with_errors` functions share the path table, `Kernels.fallthrough8`), an
error returns 0 (`nullptr`, :712-714), then the plain scalar `convert` on the
rest, 0 on its error (:716-722). `(returned, end of stores)`. -/
def pk (ys : List Nat) : Nat × Nat :=
  let r := vkLoop N choose ek ys fuel 0 0 0
  if r.1 then (0, r.2.2.2)
  else if r.2.1 = ys.length then (r.2.2.1, r.2.2.2)
  else
    let t := cwf (probe (ys.drop r.2.1)) (ys.drop r.2.1)
    if t.err = 0 then (r.2.2.1 + t.outp, max r.2.2.2 (r.2.2.1 + t.hi))
    else (0, max r.2.2.2 (r.2.2.1 + t.hi))
end

/-- Both kernels: the stores and the returned count stay within
`utf8_length_from_utf16` of the input, and an error count is a unit of the
input. -/
theorem vk_pk_spec (N : Nat) (paths : List Iter) (hT : TableOK N paths)
    (choose : List Nat → Iter) (hc : ∀ us, N ≤ us.length → choose us ∈ paths)
    (ek : List Nat → Nat) (hek : ∀ us, N ≤ us.length → ek us < us.length)
    (probe : List Nat → Nat → Bool) (fuel : Nat) (ys : List Nat) (hu : U16 ys) :
    ((vk N choose ek probe fuel ys).se ≤ L (ys.map w8) ∧
      ((vk N choose ek probe fuel ys).e = 0 → (vk N choose ek probe fuel ys).cnt ≤ L (ys.map w8)) ∧
      ((vk N choose ek probe fuel ys).e ≠ 0 → (vk N choose ek probe fuel ys).cnt < ys.length)) ∧
    (pk N choose ek probe fuel ys).2 ≤ L (ys.map w8) ∧
    (pk N choose ek probe fuel ys).1 ≤ L (ys.map w8) := by
  obtain ⟨l1, l2, l3⟩ := vkLoop_spec N paths hT choose hc ek hek ys fuel 0 0 0 (by simp) (by simp)
    (by omega)
  unfold vk pk
  generalize vkLoop N choose ek ys fuel 0 0 0 = r at l1 l2 l3
  obtain ⟨stp, pos, out, hi⟩ := r
  simp only at l1 l2 l3 ⊢
  cases stp
  · obtain ⟨e1, e2⟩ := l2 rfl
    have hle := Lw8_take_le ys pos
    simp only [Bool.false_eq_true, ↓reduceIte]
    by_cases hp : pos = ys.length
    · simp only [hp, ↓reduceIte]
      exact ⟨⟨l1, fun _ => by omega, fun h => absurd rfl h⟩, l1, by omega⟩
    simp only [hp, ↓reduceIte]
    obtain ⟨t1, t2, t3⟩ := cwf_spec (ys.drop pos) (ReplacementRest.U16_drop hu pos) (probe (ys.drop pos))
    have hsplit : L (ys.map w8) = L ((ys.take pos).map w8) + L ((ys.drop pos).map w8) := by
      rw [← L_append, ← List.map_append, List.take_append_drop]
    generalize cwf (probe (ys.drop pos)) (ys.drop pos) = t at t1 t2 t3
    by_cases he : t.err = 0
    · simp only [he, ↓reduceIte]
      exact ⟨⟨by omega, fun _ => by omega, fun h => absurd rfl h⟩, by omega, by omega⟩
    · simp only [he, ↓reduceIte]
      rcases t3 with h0 | ⟨_, hlt⟩
      · exact absurd h0 he
      simp at hlt
      exact ⟨⟨by omega, by intro h; simp_all, fun _ => by omega⟩, by omega, by omega⟩
  · simp only [↓reduceIte]
    have := l3 rfl
    exact ⟨⟨l1, fun h => by simp at h, fun _ => this⟩, l1, by omega⟩

/-- **`VKSpec` holds for the generic kernels**, for every path choice, every
error offset inside the remaining units, every ASCII probe and every fuel. -/
theorem vkspec_of_model (N : Nat) (paths : List Iter) (hT : TableOK N paths)
    (choose : List Nat → Iter) (hc : ∀ us, N ≤ us.length → choose us ∈ paths)
    (ek : List Nat → Nat) (hek : ∀ us, N ≤ us.length → ek us < us.length)
    (probe : List Nat → Nat → Bool) (fuel : Nat) :
    ReplacementRest.VKSpec (vk N choose ek probe fuel) :=
  fun ys hu => (vk_pk_spec N paths hT choose hc ek hek probe fuel ys hu).1

/-- The six generic tables at their loop test `end - buf >= 16 + 12`. The
`SURROGATE` offset `ek` is below 23 (`forward ≤ 15`, after an ASCII half of
8), so inside the `N = 28` units the test guarantees. -/
theorem tables_ok :
    TableOK (need 16 12 false) Arch.Westmere.utf16to8 ∧
    TableOK (need 16 12 false) Arch.Haswell.utf16to8 ∧
    TableOK (need 16 12 false) Arch.Arm64.utf16to8 ∧
    TableOK (need 16 12 false) Arch.Ppc64.utf16to8 ∧
    TableOK (need 16 12 false) Kernels.utf16to8_lsx ∧
    TableOK (need 16 12 false) Arch.Lasx.utf16to8 := by
  refine ⟨⟨Arch.Westmere.cur_utf16to8, by decide⟩, ⟨Arch.Haswell.cur_utf16to8, by decide⟩,
    ⟨Arch.Arm64.cur_utf16to8, by decide⟩, ⟨Arch.Ppc64.cur_utf16to8, by decide⟩,
    ⟨Arch.Lsx.cur_utf16to8, by decide⟩, ⟨Arch.Lasx.cur_utf16to8, by decide⟩⟩

/-- `ek < 23` (the C++ offsets) is inside the units the loop test guarantees. -/
theorem ek_inside (ek : List Nat → Nat) (h : ∀ us, ek us < 23) :
    ∀ us, need 16 12 false ≤ us.length → ek us < us.length := by
  intro us hu; have := h us; simp [need, strictBump] at hu; omega

/-! # 6. UTF-16 → UTF-8 `_safe` entry points, end to end -/

/-- **`convert_utf16_to_utf8_with_replacement_safe`**
(src/implementation.cpp:2494-2539) **on westmere, haswell, arm64, ppc64, lsx and
lasx, from the entry point, on every input of 16-bit units**: with the vector
kernel of the model (`vk` over any of the six tables, `tables_ok`), the
scalar tails `cwe` and `crs`, every store ends inside `utf8_len` and the
returned `output_count` is at most `utf8_len`. Remaining hypothesis: `SLSpec`
of the vector `utf8_length_from_utf16`. -/
theorem replSafe_generic (N : Nat) (paths : List Iter) (hT : TableOK N paths)
    (choose : List Nat → Iter) (hc : ∀ us, N ≤ us.length → choose us ∈ paths)
    (ek : List Nat → Nat) (hek : ∀ us, N ≤ us.length → ek us < us.length)
    (probe : List Nat → Nat → Bool) (fuel : Nat)
    (SL : List Nat → Nat) (hSL : ReplacementRest.SLSpec SL)
    (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.tLoop (ReplacementRest.dSafe (vk N choose ek probe fuel) SL
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).hi ≤ cap ∧
    (ReplacementRest.tLoop (ReplacementRest.dSafe (vk N choose ek probe fuel) SL
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).outp ≤ cap :=
  ReplacementRest.replSafe_safe _ SL (vkspec_of_model N paths hT choose hc ek hek probe fuel) hSL
    probe xs hu cap

/-- **`convert_utf16_to_utf8_safe_with_details`** (src/implementation.cpp:2028-2091),
same kernels: stores and `output_count` within `utf8_len`. Remaining
hypothesis: `SLSpec`. -/
theorem dSafe_generic (N : Nat) (paths : List Iter) (hT : TableOK N paths)
    (choose : List Nat → Iter) (hc : ∀ us, N ≤ us.length → choose us ∈ paths)
    (ek : List Nat → Nat) (hek : ∀ us, N ≤ us.length → ek us < us.length)
    (probe : List Nat → Nat → Bool) (fuel : Nat)
    (SL : List Nat → Nat) (hSL : ReplacementRest.SLSpec SL)
    (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.dSafe (vk N choose ek probe fuel) SL (ReplacementRest.cwe probe) xs cap).hi ≤ cap ∧
    (ReplacementRest.dSafe (vk N choose ek probe fuel) SL (ReplacementRest.cwe probe) xs cap).outp ≤ cap :=
  let h := ReplacementRest.dSafe_spec _ SL _ (vkspec_of_model N paths hT choose hc ek hek probe fuel)
    hSL (ReplacementRest.cwe_spec probe) xs cap hu
  ⟨h.1, h.2.1⟩

/-! ## The plain `convert_utf16_to_utf8_safe` -/

section
variable (PK : List Nat → Nat × Nat) (SK : List Nat → Nat → ReplacementRest.FR) (xs : List Nat)

/-- After the loop (src/implementation.cpp:2011-2025): the scalar
`convert_with_errors<…, true>` on the rest with the room left; 0 on an error
other than `OUTPUT_BUFFER_TOO_SMALL`, else its `output_count` plus what the
loop wrote. `(returned, end of stores)`. -/
def pTail (pos oc cap' hi : Nat) : Nat × Nat :=
  let r := SK (xs.drop pos) cap'
  if r.err ≠ 0 ∧ r.err ≠ 2 then (0, max hi (oc + r.hi)) else (oc + r.outp, max hi (oc + r.hi))

/-- **`convert_utf16_to_utf8_safe`** (src/implementation.cpp:1971-2018), the loop
with fuel: `read_len = min(len, utf8_len / 3)` (:1984); stop at
`read_len <= 16` (:1985-1987); back off before a final high surrogate
(:1988-1994); stop at 0 (:1995-1998); the plain kernel `convert_utf16_to_utf8`
(`PK`) on `read_len` units (:1999-2000); 0 is a failure (:2001-2004); else
advance (:2006-2009). `cap'` is `utf8_len` as the loop decrements it. Fuel
exhaustion returns 0 (the C++ needs at most `len` iterations; the theorem
holds for every fuel). -/
def pLoop : Nat → Nat → Nat → Nat → Nat → Nat × Nat
  | 0, _, _, _, hi => (0, hi)
  | f + 1, pos, oc, cap', hi =>
    let rl0 := min (xs.length - pos) (cap' / 3)
    if rl0 ≤ 16 then pTail SK xs pos oc cap' hi else
    let rl := if rl0 < xs.length - pos ∧ isHigh (g xs (pos + rl0 - 1)) = true then rl0 - 1 else rl0
    if rl = 0 then pTail SK xs pos oc cap' hi else
    let r := PK ((xs.drop pos).take rl)
    let hi1 := max hi (oc + r.2)
    if r.1 = 0 then (0, hi1)
    else pLoop f (pos + rl) (oc + r.1) (cap' - r.1) hi1
end

theorem pLoop_safe (PK : List Nat → Nat × Nat)
    (hPK : ∀ ys, U16 ys → (PK ys).2 ≤ L (ys.map w8) ∧ (PK ys).1 ≤ L (ys.map w8))
    (SK : List Nat → Nat → ReplacementRest.FR) (hSK : ReplacementRest.FSpec SK)
    (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    ∀ f pos oc cap' hi, oc + cap' = cap → hi ≤ cap →
      (pLoop PK SK xs f pos oc cap' hi).1 ≤ cap ∧ (pLoop PK SK xs f pos oc cap' hi).2 ≤ cap := by
  have tail : ∀ pos oc cap' hi, oc + cap' = cap → hi ≤ cap →
      (pTail SK xs pos oc cap' hi).1 ≤ cap ∧ (pTail SK xs pos oc cap' hi).2 ≤ cap := by
    intro pos oc cap' hi h1 h2
    obtain ⟨s1, s2, -, -⟩ := hSK (xs.drop pos) cap' (ReplacementRest.U16_drop hu pos)
    unfold pTail; dsimp only; split <;> (dsimp only; omega)
  intro f
  induction f with
  | zero => intro pos oc cap' hi h1 h2; simp only [pLoop]; omega
  | succ f ih =>
    intro pos oc cap' hi h1 h2
    unfold pLoop
    simp only
    split
    · exact tail pos oc cap' hi h1 h2
    generalize hrl : (if min (xs.length - pos) (cap' / 3) < xs.length - pos ∧
        isHigh (g xs (pos + min (xs.length - pos) (cap' / 3) - 1)) = true
      then min (xs.length - pos) (cap' / 3) - 1 else min (xs.length - pos) (cap' / 3)) = rl
    have hrl1 : rl ≤ xs.length - pos ∧ 3 * rl ≤ cap' := by
      rw [← hrl]; split <;> omega
    split
    · exact tail pos oc cap' hi h1 h2
    obtain ⟨k1, k2⟩ := hPK ((xs.drop pos).take rl)
      (ReplacementRest.U16_take (ReplacementRest.U16_drop hu pos) rl)
    have hL := ReplacementRest.L_w8_le ((xs.drop pos).take rl)
    have hlen : ((xs.drop pos).take rl).length = rl := by simp; omega
    rw [hlen] at hL
    generalize PK ((xs.drop pos).take rl) = r at *
    split
    · dsimp only; omega
    · apply ih <;> omega

/-- **`convert_utf16_to_utf8_safe` (src/implementation.cpp:1971-2018) on westmere,
haswell, arm64, ppc64, lsx and lasx, from the entry point, on every input of
16-bit units and every `utf8_len`**: with the plain kernel of the model
(`pk` over a table of `tables_ok`) and the scalar tail `cwe`, every store
ends inside `utf8_len`, and so does the returned count. No hypothesis on a
length function is needed. -/
theorem pSafe_generic (N : Nat) (paths : List Iter) (hT : TableOK N paths)
    (choose : List Nat → Iter) (hc : ∀ us, N ≤ us.length → choose us ∈ paths)
    (ek : List Nat → Nat) (hek : ∀ us, N ≤ us.length → ek us < us.length)
    (probe : List Nat → Nat → Bool) (fuel fuel' : Nat)
    (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (pLoop (pk N choose ek probe fuel) (ReplacementRest.cwe probe) xs fuel' 0 0 cap 0).1 ≤ cap ∧
    (pLoop (pk N choose ek probe fuel) (ReplacementRest.cwe probe) xs fuel' 0 0 cap 0).2 ≤ cap :=
  pLoop_safe _ (fun ys hu' => (vk_pk_spec N paths hT choose hc ek hek probe fuel ys hu').2)
    _ (ReplacementRest.cwe_spec probe) xs hu cap fuel' 0 0 cap 0 (by omega) (by omega)

/-! # 7. `convert_latin1_to_utf8_safe` -/

/-- `utf8_length_from_latin1` per byte (include/simdutf/scalar/latin1.h:9-19):
1, plus 1 when the top bit is set. -/
def wl8 (b : Nat) : Nat := if b < 0x80 then 1 else 2

theorem wl8_ge (b : Nat) : 1 ≤ wl8 b := by unfold wl8; split <;> omega
theorem wl8_le (b : Nat) : wl8 b ≤ 2 := by unfold wl8; split <;> omega

theorem pos_wl8 (xs : List Nat) : Pos (xs.map wl8) := by
  intro x hx; obtain ⟨b, -, rfl⟩ := List.mem_map.mp hx; exact wl8_ge b

theorem L_wl8_le (ys : List Nat) : L (ys.map wl8) ≤ 2 * ys.length := by
  induction ys with
  | nil => simp
  | cons u r ih => simp only [List.map, L_cons, List.length_cons]; have := wl8_le u; omega

section
variable (ys : List Nat)

/-- One vector loop of a Latin-1 → UTF-8 kernel, with fuel: while `N` bytes
remain, an iteration of its path table (no table of `Arch/*.lean` for this
conversion has a `stop` iteration); the cursor advances by `L` of the
consumed bytes. `(pos, cursor, end of stores)`. -/
def lkLoop (N : Nat) (choose : List Nat → Iter) : Nat → Nat → Nat → Nat → Nat × Nat × Nat
  | 0, pos, out, hi => (pos, out, hi)
  | f + 1, pos, out, hi =>
    if (ys.drop pos).length < N then (pos, out, hi)
    else
      let it := choose (ys.drop pos)
      let ws := (ys.drop pos).map wl8
      lkLoop N choose f (pos + it.consumed) (out + L (ws.take it.consumed))
        (max hi (out + maxEnd ws it.consumed it.stores))

/-- The loops of a kernel in order (westmere and ppc64 run a second, one-shot
block after the main loop), each `(N, choose)`. -/
def lkStages (fuel : Nat) : List (Nat × (List Nat → Iter)) → Nat × Nat × Nat → Nat × Nat × Nat
  | [], st => st
  | (N, ch) :: rest, st => lkStages fuel rest (lkLoop ys N ch fuel st.1 st.2.1 st.2.2)

/-- **`convert_latin1_to_utf8`** of a kernel: its loops, then the scalar
`latin1_to_utf8::convert` on the rest (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:11-62,
one or two bytes per byte, `L` of the rest; e.g. src/westmere/implementation.cpp:440-454),
or on Ice Lake the two masked calls (src/icelake/icelake_convert_latin1_to_utf8.inl.cpp:85-107),
which store exactly what they produce. `(returned, end of stores)`. -/
def lk (fuel : Nat) (stages : List (Nat × (List Nat → Iter))) : Nat × Nat :=
  let r := lkStages ys fuel stages (0, 0, 0)
  (r.2.1 + L ((ys.drop r.1).map wl8), max r.2.2 (r.2.1 + L ((ys.drop r.1).map wl8)))
end

/-- A loop table of the Latin-1 kernels: stores fit at `N`, no iteration
consumes more than `N` bytes, none stops. -/
def LTableOK (N : Nat) (paths : List Iter) : Prop :=
  paths.all (Iter.ok N) = true ∧ paths.all (fun it => decide (it.consumed ≤ N) && !it.stop) = true

theorem Lwl8_take_le (xs : List Nat) (k : Nat) : L ((xs.take k).map wl8) ≤ L (xs.map wl8) := by
  rw [List.map_take]; exact L_take_le _ _

theorem lkLoop_spec (N : Nat) (paths : List Iter) (hT : LTableOK N paths)
    (choose : List Nat → Iter) (hc : ∀ us, N ≤ us.length → choose us ∈ paths) (ys : List Nat) :
    ∀ f pos out hi, out = L ((ys.take pos).map wl8) → hi ≤ L (ys.map wl8) → pos ≤ ys.length →
      (lkLoop ys N choose f pos out hi).2.1 = L ((ys.take (lkLoop ys N choose f pos out hi).1).map wl8) ∧
      (lkLoop ys N choose f pos out hi).2.2 ≤ L (ys.map wl8) ∧
      (lkLoop ys N choose f pos out hi).1 ≤ ys.length := by
  intro f
  induction f with
  | zero => intro pos out hi h1 h2 h3; exact ⟨h1, h2, h3⟩
  | succ f ih =>
    intro pos out hi h1 h2 h3
    unfold lkLoop
    by_cases hs : (ys.drop pos).length < N
    · rw [ite_eq_left hs]; exact ⟨h1, h2, h3⟩
    rw [ite_eq_right hs]
    have hlen : N ≤ (ys.drop pos).length := by omega
    have hmem := hc _ hlen
    have hok : Iter.ok N (choose (ys.drop pos)) = true := (List.all_eq_true.mp hT.1) _ hmem
    have hcons : (choose (ys.drop pos)).consumed ≤ N := by
      have := (List.all_eq_true.mp hT.2) _ hmem; simp at this; exact this.1
    generalize choose (ys.drop pos) = it at hok hcons
    have hwl : (((ys.drop pos).map wl8)).length = (ys.drop pos).length := by simp
    have hstores : ∀ s ∈ it.stores, storeEnd ((ys.drop pos).map wl8) it.consumed s ≤
        L ((ys.drop pos).map wl8) := by
      intro s hs'
      have hb : decide (placeReq it.consumed s ≤ N) = true := (List.all_eq_true.mp hok) s hs'
      exact storeEnd_le _ (pos_wl8 _) N _ (by rw [hwl]; exact hlen) s (of_decide_eq_true hb)
    have hhere : maxEnd ((ys.drop pos).map wl8) it.consumed it.stores ≤ L ((ys.drop pos).map wl8) := by
      simpa [maxEnd] using maxEnd_le _ _ _ 0 _ (Nat.zero_le _) hstores
    have hsplit : L (ys.map wl8) = L ((ys.take pos).map wl8) + L ((ys.drop pos).map wl8) := by
      rw [← L_append, ← List.map_append, List.take_append_drop]
    apply ih
    · rw [h1, List.map_take, List.map_take, Utf8.L_take_add, List.map_drop]
    · omega
    · simp at hlen; omega

theorem lkStages_spec (ys : List Nat) (fuel : Nat) :
    ∀ (stages : List (Nat × (List Nat → Iter))),
      (∀ st ∈ stages, ∃ paths, LTableOK st.1 paths ∧
        ∀ us, st.1 ≤ us.length → st.2 us ∈ paths) →
      ∀ pos out hi, out = L ((ys.take pos).map wl8) → hi ≤ L (ys.map wl8) → pos ≤ ys.length →
      (lkStages ys fuel stages (pos, out, hi)).2.1 =
          L ((ys.take (lkStages ys fuel stages (pos, out, hi)).1).map wl8) ∧
      (lkStages ys fuel stages (pos, out, hi)).2.2 ≤ L (ys.map wl8) ∧
      (lkStages ys fuel stages (pos, out, hi)).1 ≤ ys.length := by
  intro stages
  induction stages with
  | nil => intro _ pos out hi h1 h2 h3; exact ⟨h1, h2, h3⟩
  | cons st rest ih =>
    intro hst pos out hi h1 h2 h3
    obtain ⟨N, ch⟩ := st
    obtain ⟨paths, hT, hc⟩ := hst (N, ch) (by simp)
    obtain ⟨a1, a2, a3⟩ := lkLoop_spec N paths hT ch hc ys fuel pos out hi h1 h2 h3
    exact ih (fun s hs => hst s (by simp [hs])) _ _ _ a1 a2 a3

/-- **A Latin-1 → UTF-8 kernel returns exactly `utf8_length_from_latin1` and
stores inside it.** -/
theorem lk_spec (ys : List Nat) (fuel : Nat) (stages : List (Nat × (List Nat → Iter)))
    (hst : ∀ st ∈ stages, ∃ paths, LTableOK st.1 paths ∧ ∀ us, st.1 ≤ us.length → st.2 us ∈ paths) :
    (lk ys fuel stages).1 = L (ys.map wl8) ∧ (lk ys fuel stages).2 ≤ L (ys.map wl8) := by
  obtain ⟨a1, a2, a3⟩ := lkStages_spec ys fuel stages hst 0 0 0 (by simp) (by simp) (by omega)
  unfold lk
  generalize lkStages ys fuel stages (0, 0, 0) = r at a1 a2 a3
  have hsplit : L (ys.map wl8) = L ((ys.take r.1).map wl8) + L ((ys.drop r.1).map wl8) := by
    rw [← L_append, ← List.map_append, List.take_append_drop]
  simp only
  constructor <;> omega

/-- The tables of the eight kernels: westmere (main loop at `16 + 8`, then the
one-shot block at 16), haswell and lasx (`16 + 8`), arm64 and lsx (16), ppc64
(`16 + 8`, then 16), icelake (`64 + 32`). -/
theorem latin1_tables_ok :
    LTableOK (need 16 8 false) Arch.Westmere.latin1to8 ∧
    LTableOK (need 16 0 false) Arch.Westmere.latin1to8_last ∧
    LTableOK (need 16 8 false) Arch.Haswell.latin1to8 ∧
    LTableOK (need 16 8 false) Arch.Lasx.latin1to8 ∧
    LTableOK (need 16 0 false) Arch.Arm64.latin1to8 ∧
    LTableOK (need 16 0 false) Kernels.latin1to8_one ∧
    LTableOK (need 16 8 false) Arch.Ppc64.latin1to8 ∧
    LTableOK (need 16 0 false) Arch.Ppc64.latin1to8Last ∧
    LTableOK (need 64 32 false) Arch.Icelake.latin1to8 :=
  ⟨⟨Arch.Westmere.cur_latin1to8, by decide⟩, ⟨Arch.Westmere.cur_latin1to8_last, by decide⟩,
   ⟨Arch.Haswell.ok_latin1to8, by decide⟩, ⟨Arch.Lasx.ok_latin1to8, by decide⟩,
   ⟨Arch.Arm64.ok_latin1to8, by decide⟩, ⟨Arch.Lsx.cur_latin1to8, by decide⟩,
   ⟨Arch.Ppc64.cur_latin1to8, by decide⟩, ⟨Arch.Ppc64.cur_latin1to8Last, by decide⟩,
   ⟨Arch.Icelake.ok_latin1to8, by decide⟩⟩

section
variable (xs : List Nat) (cap : Nat)

/-- **`scalar::latin1_to_utf8::convert_safe`**
(include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:71-117) with fuel:
`(utf8_pos, end of stores)`. The 16-byte block (:79-99) is copied only when
`utf8_pos + 16 <= utf8_len`; otherwise one byte, or two when
`utf8_pos + 2 <= utf8_len` (:101-113). -/
def csLoop : Nat → Nat → Nat → Nat → Nat → Nat × Nat
  | 0, _, _, upos, hi => (upos, hi)
  | f + 1, pos, skip, upos, hi =>
    if pos < xs.length ∧ upos < cap then
      if skip ≤ pos ∧ pos + 16 ≤ xs.length ∧ upos + 16 ≤ cap then
        if (List.range 16).all (fun k => decide (xs.getD (pos + k) 0 < 0x80)) then
          csLoop f (pos + 16) skip (upos + 16) (max hi (upos + 16))
        else csLoop f pos (pos + 16) upos hi
      else if xs.getD pos 0 < 0x80 then csLoop f (pos + 1) skip (upos + 1) (max hi (upos + 1))
      else if upos + 2 ≤ cap then csLoop f (pos + 1) skip (upos + 2) (max hi (upos + 2))
      else (upos, hi)
    else (upos, hi)

/-- Every store and the returned count stay within `cap`. -/
theorem csLoop_safe : ∀ f pos skip upos hi, upos ≤ cap → hi ≤ cap →
    (csLoop xs cap f pos skip upos hi).1 ≤ cap ∧ (csLoop xs cap f pos skip upos hi).2 ≤ cap := by
  intro f
  induction f with
  | zero => intro _ _ upos hi h1 h2; exact ⟨h1, h2⟩
  | succ f ih =>
    intro pos skip upos hi h1 h2
    unfold csLoop
    repeat' split
    all_goals first | exact ⟨h1, h2⟩ | (apply ih <;> omega)

/-- The safe driver (src/implementation.cpp:2693-2717), with fuel:
`read_len = min(len, utf8_len >> 1)` (:2699); stop at `read_len <= 16`
(:2700-2702); the kernel on `read_len` bytes (:2704-2705); advance
(:2707-2710); then `convert_safe` on the rest with the room left (:2713-2714).
`(returned, end of stores)`. -/
def l8Loop (LK : List Nat → Nat × Nat) (fs : Nat) : Nat → Nat → Nat → Nat → Nat → Nat × Nat
  | 0, _, oc, _, hi => (oc, hi)
  | f + 1, pos, oc, cap', hi =>
    let rl := min (xs.length - pos) (cap' / 2)
    if rl ≤ 16 then
      let t := csLoop (xs.drop pos) cap' fs 0 0 0 0
      (oc + t.1, max hi (oc + t.2))
    else
      let r := LK ((xs.drop pos).take rl)
      l8Loop LK fs f (pos + rl) (oc + r.1) (cap' - r.1) (max hi (oc + r.2))
end

theorem l8Loop_safe (LK : List Nat → Nat × Nat)
    (hLK : ∀ ys, (LK ys).1 ≤ L (ys.map wl8) ∧ (LK ys).2 ≤ L (ys.map wl8))
    (xs : List Nat) (cap fs : Nat) :
    ∀ f pos oc cap' hi, oc + cap' = cap → hi ≤ cap →
      (l8Loop xs LK fs f pos oc cap' hi).1 ≤ cap ∧ (l8Loop xs LK fs f pos oc cap' hi).2 ≤ cap := by
  intro f
  induction f with
  | zero => intro _ oc cap' hi h1 h2; simp only [l8Loop]; omega
  | succ f ih =>
    intro pos oc cap' hi h1 h2
    unfold l8Loop
    simp only
    split
    · have := csLoop_safe (xs.drop pos) cap' fs 0 0 0 0 (by omega) (by omega)
      dsimp only; omega
    · obtain ⟨k1, k2⟩ := hLK ((xs.drop pos).take (min (xs.length - pos) (cap' / 2)))
      have hL := L_wl8_le ((xs.drop pos).take (min (xs.length - pos) (cap' / 2)))
      have hlen : ((xs.drop pos).take (min (xs.length - pos) (cap' / 2))).length ≤ cap' / 2 := by
        simp; omega
      apply ih <;> omega

/-- **`convert_latin1_to_utf8_safe` (src/implementation.cpp:2693-2717) on every
kernel with a path table (westmere, haswell, icelake, arm64, ppc64, lsx,
lasx), from the entry point, on every input and every `utf8_len`**: with the
kernel `lk` over its loops (`latin1_tables_ok`) and the scalar
`convert_safe`, every store ends inside `utf8_len` and so does the returned
count. No hypothesis is left; the theorem holds for every fuel. -/
theorem latin1Safe (stages : List (Nat × (List Nat → Iter)))
    (hst : ∀ st ∈ stages, ∃ paths, LTableOK st.1 paths ∧ ∀ us, st.1 ≤ us.length → st.2 us ∈ paths)
    (fuel fs f : Nat) (xs : List Nat) (cap : Nat) :
    (l8Loop xs (fun ys => lk ys fuel stages) fs f 0 0 cap 0).1 ≤ cap ∧
    (l8Loop xs (fun ys => lk ys fuel stages) fs f 0 0 cap 0).2 ≤ cap :=
  l8Loop_safe _ (fun ys => let h := lk_spec ys fuel stages hst; ⟨Nat.le_of_eq h.1, h.2⟩) xs cap fs
    f 0 0 cap 0 (by omega) (by omega)

/-! # 8. The hypotheses on the models are satisfiable

The end-to-end theorems quantify over every kernel function whose results
are results of the models (`BOut`, `CVOut`, any `choose` into a table). These
hypotheses are not vacuous: the functions below meet them (they are the
paths the C++ takes on short inputs: the bulk kernel's `size < 64` scalar
call, `src/generic/utf8_to_utf16/utf8_to_utf16.h:273-276`, is the rewind from
0; `convert_valid` on fewer than `64 + margin` bytes is the scalar tail). -/

/-- The bulk kernel when it goes straight to the scalar code (the `tail`
return from the start state, no vector store). -/
def kScalar (win : List Nat) : ReplacementRest.BRes :=
  ⟨(ValidatorSound.rewindK win 0).1, true,
    if (ValidatorSound.rewindK win 0).1 then 0 + (ValidatorSound.rewindK win 0).2
    else (ValidatorSound.rewindK win 0).2, 0⟩

theorem kScalar_out (xm : Bool) (ps : List MPath) (win : List Nat) (hne : win ≠ []) :
    BOut xm ps win (kScalar win) := by
  refine ⟨?_, Or.inl rfl⟩
  have h := ValidatorSound.KOut.tail (xm := xm) (bs := win) 0 0 ValidatorSound.KR.start
    (List.length_pos_iff.mpr hne)
  simpa [kScalar] using h

/-- `convert_valid` when the loop does not run. -/
def cvScalar (v : List Nat) : Nat × Nat := (L (v.map w16), 0)

theorem cvScalar_out (d m : Nat) (v : List Nat) : CVOut d m v (cvScalar v) :=
  ⟨⟨0, 0, ValidExact.VX.start, by simp [cvScalar]⟩, Or.inl rfl⟩

/-- **The UTF-8 → UTF-16 entry point with these kernels**: no hypothesis
but `ULSpec`. -/
theorem convertRepl_scalarKernels (xm : Bool) (UL : List Nat → Nat)
    (hUL : ReplacementRest.ULSpec UL) (xs : List Nat) (hb : Bytes xs) :
    (ReplacementRest.convertRepl kScalar (ValidatorSound.vecV xm) cvScalar UL xs).1 =
      lenRepl (ValidatorSound.vecV xm) xs ∧
    (ReplacementRest.convertRepl kScalar (ValidatorSound.vecV xm) cvScalar UL xs).2 ≤
      lenRepl (ValidatorSound.vecV xm) xs :=
  convertRepl_generic xm Utf8.x86_utf16 gap_all.1 kScalar
    (fun win _ hne => kScalar_out xm _ win hne) 4 9 (Or.inl ⟨rfl, Nat.le_refl _⟩) cvScalar
    (fun v _ => cvScalar_out 4 9 v) UL hUL xs hb

/-- Westmere, UTF-16 → UTF-8 with replacement, `_safe`: the instance of
`replSafe_generic` for its table, any path choice and any error offset below
23. -/
theorem replSafe_westmere (choose : List Nat → Iter)
    (hc : ∀ us, need 16 12 false ≤ us.length → choose us ∈ Arch.Westmere.utf16to8)
    (ek : List Nat → Nat) (hek : ∀ us, ek us < 23)
    (probe : List Nat → Nat → Bool) (fuel : Nat)
    (SL : List Nat → Nat) (hSL : ReplacementRest.SLSpec SL)
    (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.tLoop (ReplacementRest.dSafe (vk (need 16 12 false) choose ek probe fuel) SL
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).hi ≤ cap :=
  (replSafe_generic _ _ tables_ok.1 choose hc ek (ek_inside ek hek) probe fuel SL hSL xs hu cap).1

/-- Westmere, `convert_latin1_to_utf8_safe`: the main loop and the one-shot
block, any path choice. -/
theorem latin1Safe_westmere (ch1 ch2 : List Nat → Iter)
    (hc1 : ∀ us, need 16 8 false ≤ us.length → ch1 us ∈ Arch.Westmere.latin1to8)
    (hc2 : ∀ us, need 16 0 false ≤ us.length → ch2 us ∈ Arch.Westmere.latin1to8_last)
    (fuel fs f : Nat) (xs : List Nat) (cap : Nat) :
    (l8Loop xs (fun ys => lk ys fuel [(need 16 8 false, ch1), (need 16 0 false, ch2)]) fs f 0 0
      cap 0).2 ≤ cap := by
  refine (latin1Safe _ ?_ fuel fs f xs cap).2
  intro st hst
  simp only [List.mem_cons, List.mem_nil_iff, or_false] at hst
  rcases hst with rfl | rfl
  · exact ⟨_, latin1_tables_ok.1, hc1⟩
  · exact ⟨_, latin1_tables_ok.2.1, hc2⟩

/-- The scalar tail reports an unpaired high surrogate at its index, and an
input of 30 units with a pair crosses the vector loop: `vk` converts the
whole input, exactly `L` bytes. -/
example :
    (cwf (fun _ => false) [0x41, 0xD800, 0x41]).err = 1 ∧
    (cwf (fun _ => false) [0x41, 0xD800, 0x41]).inp = 1 ∧
    (vk (need 16 12 false) (fun _ => { consumed := 16, stores := [.rel 0 16] }) (fun _ => 0)
      (fun _ _ => false) 5 (List.replicate 28 0x41 ++ [0xD83D, 0xDE00])).cnt = 32 := by decide

end SafetyMargins.ReplacementClose
