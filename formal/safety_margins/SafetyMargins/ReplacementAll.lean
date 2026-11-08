import SafetyMargins.ReplacementProbe
import SafetyMargins.IcelakeReplacement16
import SafetyMargins.RvvReplacement16
/-
The conversions with replacement on icelake and rvv, with no contract left.

`ReplacementRest` proves the drivers under four contracts on the vector code.
`ReplacementClose` and `ReplacementLengths` discharge them for the six kernels
built from src/generic; `ReplacementProbe` removes the probe hypothesis of the
scalar UTF-16 tail (`probeOK_real`). The icelake and rvv kernels are discharged
in their own files:

* UTF-16 → UTF-8 (`VKSpec'`): `IcelakeReplacement16.iceVK_spec`,
  `RvvReplacement16.rvvVK_spec`.

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

end SafetyMargins.ReplacementAll
