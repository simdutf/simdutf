import SafetyMargins.ReplacementProbe
import SafetyMargins.IcelakeReplacement16
import SafetyMargins.RvvReplacement16
import SafetyMargins.IcelakeReplacement
import SafetyMargins.IcelakeValidator
/-
The conversions with replacement on icelake and rvv, with no contract left.

`ReplacementRest` proves the drivers under four contracts on the vector code.
`ReplacementClose` and `ReplacementLengths` discharge them for the six kernels
built from src/generic; `ReplacementProbe` removes the probe hypothesis of the
scalar UTF-16 tail (`probeOK_real`). The icelake and rvv kernels are discharged
in their own files:

* UTF-16 → UTF-8 (`VKSpec'`): `IcelakeReplacement16.iceVK_spec`,
  `RvvReplacement16.rvvVK_spec`.
* UTF-8 → UTF-16: icelake `IcelakeReplacement.iceK_bspec`, `iceCV_cvspec`,
  `IcelakeValidator.iceV_spec2`; rvv `RvvReplacement.convertRepl_rvv`,
  `trRun_rvv`.

The icelake validator depends on the address of its input
(`misalignment`, src/icelake/implementation.cpp:327), which changes from call
to call of the driver (`input + pos`). `iceVA addr` is the validator called
with any misalignment chosen as a function of the remaining input; its
contract holds pointwise (`iceVA_spec2`), so the theorems below hold for
every buffer address.

Here the end-to-end theorems are stated with the C++ probe, so that no
hypothesis on the vector code is left for the `_safe` UTF-16 → UTF-8 entry
points on icelake and rvv.
-/
namespace SafetyMargins.ReplacementAll
open SafetyMargins SafetyMargins.Replacement SafetyMargins.ReplacementMore
  SafetyMargins.ReplacementLengths SafetyMargins.ReplacementProbe

/-- **`convert_utf16_to_utf8_with_replacement_safe` on icelake**
(src/implementation.cpp:2494-2539), both byte orders: every store and the
returned count inside `utf8_len`, for every input of 16-bit units. -/
theorem replSafe_icelake_real (be be' : Bool) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.tLoop (ReplacementRest.dSafe (IcelakeReplacement16.iceVK (realProbe be))
        LengthsIcelake.ice8of16 (ReplacementRest.cwe (realProbe be')))
        (ReplacementRest.crs (ReplacementRest.cwe (realProbe be')))
        xs cap (xs.length + 1) 0 0 0).hi ≤ cap ∧
    (ReplacementRest.tLoop (ReplacementRest.dSafe (IcelakeReplacement16.iceVK (realProbe be))
        LengthsIcelake.ice8of16 (ReplacementRest.cwe (realProbe be')))
        (ReplacementRest.crs (ReplacementRest.cwe (realProbe be')))
        xs cap (xs.length + 1) 0 0 0).outp ≤ cap :=
  IcelakeReplacement16.replSafe_icelake _ (probeOK_real be) _ xs hu cap

/-- **`convert_utf16_to_utf8_safe_with_details` on icelake**
(src/implementation.cpp:2028-2091). -/
theorem dSafe_icelake_real (be be' : Bool) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.dSafe (IcelakeReplacement16.iceVK (realProbe be)) LengthsIcelake.ice8of16
      (ReplacementRest.cwe (realProbe be')) xs cap).hi ≤ cap ∧
    (ReplacementRest.dSafe (IcelakeReplacement16.iceVK (realProbe be)) LengthsIcelake.ice8of16
      (ReplacementRest.cwe (realProbe be')) xs cap).outp ≤ cap :=
  IcelakeReplacement16.dSafe_icelake _ (probeOK_real be) _ xs hu cap

/-- The icelake validator with the misalignment of each call given by `addr`. -/
def iceVA (addr : List Nat → Nat) (ys : List Nat) : ReplacementMore.VRes :=
  IcelakeValidator.iceV (addr ys) ys

theorem iceVA_spec2 (addr : List Nat → Nat) : ReplacementRest.VSpec2 (iceVA addr) := by
  refine ⟨fun ys hb hne => ?_, fun ys hb hne h =>
    ((IcelakeValidator.iceV_good (addr ys) ys hb hne).1 h).2.2⟩
  obtain ⟨g1, g2⟩ := IcelakeValidator.iceV_good (addr ys) ys hb hne
  exact ⟨fun h => ⟨(g1 h).1, (g1 h).2.1⟩, g2⟩

/-- **`convert_utf8_to_utf16*_with_replacement` on icelake**
(src/utf8_to_utf16_with_replacement.inl.h:278-323), sized by
`utf16_length_from_utf8_with_replacement`: the returned count is that
length and every store is inside it, on every byte input, for every buffer
address. -/
theorem convertRepl_icelake (addr : List Nat → Nat) (xs : List Nat) (hb : Bytes xs) :
    (ReplacementRest.convertRepl IcelakeReplacement.iceK (iceVA addr) IcelakeReplacement.iceCV
      LengthsIcelake.ice16of8 xs).1 = ReplacementMore.lenRepl (iceVA addr) xs ∧
    (ReplacementRest.convertRepl IcelakeReplacement.iceK (iceVA addr) IcelakeReplacement.iceCV
      LengthsIcelake.ice16of8 xs).2 ≤ ReplacementMore.lenRepl (iceVA addr) xs := by
  rw [ReplacementMore.lenRepl_eq _ (iceVA_spec2 addr).1 xs hb]
  exact ReplacementRest.convertRepl_safe _ _ _ _ IcelakeReplacement.iceK_bspec (iceVA_spec2 addr)
    IcelakeReplacement.iceCV_cvspec LengthsSpec.ULSpec_icelake xs hb

/-- **`transcode_utf8_to_utf16_with_replacement<endian, true>` on icelake**
(src/utf8_to_utf16_with_replacement.inl.h:88-188). -/
theorem trRun_icelake (addr : List Nat → Nat) (xs : List Nat) (hb : Bytes xs) :
    ∃ w h, ReplacementRest.trRun IcelakeReplacement.iceK (iceVA addr) IcelakeReplacement.iceCV
      LengthsIcelake.ice16of8 xs = some (w, h) ∧ w = count xs ∧ h ≤ count xs :=
  ReplacementRest.trRun_safe _ _ _ _ IcelakeReplacement.iceK_bspec (iceVA_spec2 addr)
    IcelakeReplacement.iceCV_cvspec LengthsSpec.ULSpec_icelake xs hb

end SafetyMargins.ReplacementAll
