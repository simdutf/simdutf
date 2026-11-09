import SafetyMargins.ReplacementLengths
/-
The ASCII probe of the scalar UTF-16 → UTF-8 tail, discharged.

`ReplacementLengths.vkspec'_of_model` and the end-to-end theorems
`replSafe_kernels`, `dSafe_kernels` take `ProbeOK probe` as a hypothesis. The
probe is the 4-unit test of
include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:116-124: the 64-bit word
of the four units at the cursor, rotated by 8 bits for the other byte order,
masked with `0xFF80FF80FF80FF80`. `Scalar.probeU16` transcribes it and
`Scalar.probeU16_ascii` proves that it passes only on four units below
`0x80`, in both byte orders. Here that soundness is stated as `ProbeOK` for
`realProbe be` (the probe at cursor `p` of the call's input `zs` is
`probeU16 be (zs.drop p)`), and the end-to-end theorems are restated without
the probe hypothesis (`replSafe_kernels_real`, `dSafe_kernels_real`).
-/
namespace SafetyMargins.ReplacementProbe
open SafetyMargins SafetyMargins.Replacement SafetyMargins.ReplacementMore
  SafetyMargins.ReplacementClose SafetyMargins.ReplacementLengths

/-- The C++ probe on the call's input `zs` at cursor `p`. -/
def realProbe (be : Bool) (zs : List Nat) (p : Nat) : Bool := Scalar.probeU16 be (zs.drop p)

theorem probeOK_real (be : Bool) : ProbeOK (realProbe be) := by
  intro zs p hu h hl k hk
  have hu' : Scalar.U16 (zs.drop p) := fun u hm => hu u (List.mem_of_mem_drop hm)
  have := Scalar.probeU16_ascii be (zs.drop p) hu' (by simp; omega) h k hk
  have e : g zs (p + k) = (zs.drop p).getD k 0 := by
    simp [g, List.getD_eq_getElem?_getD, List.getElem?_drop]
  rw [e]; exact this

/-- `replSafe_kernels` with the C++ probe: no probe hypothesis. -/
theorem replSafe_kernels_real (be : Bool) (k : List Iter × (List Nat → Nat)) (hk : k ∈ kernels16)
    (choose : List Nat → Iter) (hc : ∀ us, need 16 12 false ≤ us.length → choose us ∈ k.1)
    (ek : List Nat → Nat) (hek : ∀ us, ek us < 23) (hcl : CleanChoice (need 16 12 false) choose ek)
    (fuel : Nat) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.tLoop (ReplacementRest.dSafe (vk (need 16 12 false) choose ek (realProbe be) fuel) k.2
        (ReplacementRest.cwe (realProbe be))) (ReplacementRest.crs (ReplacementRest.cwe (realProbe be)))
        xs cap (xs.length + 1) 0 0 0).hi ≤ cap ∧
    (ReplacementRest.tLoop (ReplacementRest.dSafe (vk (need 16 12 false) choose ek (realProbe be) fuel) k.2
        (ReplacementRest.cwe (realProbe be))) (ReplacementRest.crs (ReplacementRest.cwe (realProbe be)))
        xs cap (xs.length + 1) 0 0 0).outp ≤ cap :=
  replSafe_kernels k hk choose hc ek hek hcl _ (probeOK_real be) fuel xs hu cap

/-- `dSafe_kernels` with the C++ probe: no probe hypothesis. -/
theorem dSafe_kernels_real (be : Bool) (k : List Iter × (List Nat → Nat)) (hk : k ∈ kernels16)
    (choose : List Nat → Iter) (hc : ∀ us, need 16 12 false ≤ us.length → choose us ∈ k.1)
    (ek : List Nat → Nat) (hek : ∀ us, ek us < 23) (hcl : CleanChoice (need 16 12 false) choose ek)
    (fuel : Nat) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.dSafe (vk (need 16 12 false) choose ek (realProbe be) fuel) k.2
      (ReplacementRest.cwe (realProbe be)) xs cap).hi ≤ cap ∧
    (ReplacementRest.dSafe (vk (need 16 12 false) choose ek (realProbe be) fuel) k.2
      (ReplacementRest.cwe (realProbe be)) xs cap).outp ≤ cap :=
  dSafe_kernels k hk choose hc ek hek hcl _ (probeOK_real be) fuel xs hu cap

end SafetyMargins.ReplacementProbe
