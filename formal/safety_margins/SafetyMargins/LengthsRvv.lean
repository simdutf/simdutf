import SafetyMargins.LengthsIcelake
/-
The rvv length functions (src/rvv/rvv_length_from.inl.cpp), for every `vl`
that `vsetvl` may return (`RvvExact.vlOf pick`, any `1 ≤ vl ≤ len`).

Each function is a loop `for (size_t vl; len > 0; len -= vl, src += vl)`
whose iteration adds, for the `vl` units it loads, `vcpop` of one or two
masks and possibly `vl` (`rvvSum`): the sum over the input of a per-unit
term (`rvvSum_eq`), which `rvv_terms` equates with the weight.

| model | C++ (src/rvv/rvv_length_from.inl.cpp) | weight |
|---|---|---|
| `rvvSum pick rvvC8` | `utf32_length_from_utf8` (:28-38), also `count_utf8` (:14-17), `latin1_length_from_utf8` (:21-24) | `w01` |
| `rvvSum pick rvvC16` | `rvv_utf32_length_from_utf16` (:42-56), `utf32_length_from_utf16le/be`, `count_utf16le/be` (:2-10, :58-69) | `w32` |
| `scL1` start `len` | `utf8_length_from_latin1` (:73-82) | `wL8` |
| `rvvSum pick rvv8of16` | `rvv_utf8_length_from_utf16` (:86-103), le/be (:105-116) | `w8` |
| `rvvSum pick sc8of32` | `utf8_length_from_utf32` (:120-133) | `w8of32` |
| `rvvSum pick rvv16of8` | `utf16_length_from_utf8` (:137-149) | `w16` |
| `rvvSum pick sc16of32` | `utf16_length_from_utf32` (:153-163) | `w16of32` |

The byte flip of the big-endian variants (`simdutf_byteflip`) is applied to
the loaded lanes; the theorems are on the units after it.
-/
namespace SafetyMargins.LengthsRvv
open SafetyMargins SafetyMargins.ReplacementMore SafetyMargins.RvvExact SafetyMargins.Lengths
  SafetyMargins.LengthsGeneric SafetyMargins.LengthsArch
set_option autoImplicit false

/-- The loop: `vl = vsetvl(len)`, the count of the `vl` units loaded, then
`len -= vl, src += vl`; `fuel` bounds the iterations. -/
def rvvSum (pick : Nat → Nat) (f : Nat → Nat) : Nat → List Nat → Nat
  | 0, _ => 0
  | n + 1, ys =>
    if ys.length = 0 then 0 else
    let vl := vlOf pick ys.length
    L ((ys.take vl).map f) + rvvSum pick f n (ys.drop vl)

theorem rvvSum_eq (pick : Nat → Nat) (f : Nat → Nat) :
    ∀ n ys, ys.length ≤ n → rvvSum pick f n ys = L (ys.map f) := by
  intro n
  induction n with
  | zero => intro ys h; simp [rvvSum, List.eq_nil_of_length_eq_zero (show ys.length = 0 by omega)]
  | succ n ih =>
    intro ys h
    simp only [rvvSum]
    split
    · rename_i h0; simp [List.eq_nil_of_length_eq_zero h0]
    · have hv : 1 ≤ vlOf pick ys.length := by unfold vlOf; omega
      rw [ih _ (by simp; omega), ← L_append, ← List.map_append, List.take_append_drop]

/-- `count += vcpop(vmsgt(v, -65))` (:34-35). -/
def rvvC8 (b : Nat) : Nat := ind (gtm65 b)
/-- `count += vcpop(vmor(v > 0xDFFF, v < 0xDC00))` (:50-53). -/
def rvvC16 (u : Nat) : Nat := ind (nirLo u)
/-- `count += vl + vcpop(m234) + vcpop(m34 & notSur)` (:94-100). -/
def rvv8of16 (u : Nat) : Nat :=
  1 + ind (decide (u > 0x7F)) + ind (decide (u > 0x7FF) && decide (u < 0xD800 ∨ u > 0xDFFF))
/-- `count += vcpop(v > -65) + vcpop(v >u 0b11101111)` (:143-146). -/
def rvv16of8 (b : Nat) : Nat := ind (gtm65 b) + ind (decide (b > 0xEF))
/-- `count = len`, `count += vcpop(v < 0)` (:75-79). -/
def rvvL1 (b : Nat) : Nat := 1 + ind (decide (i8 b < 0))

def rvvByteOK (b : Nat) : Bool := rvvC8 b == w01 b && rvv16of8 b == w16 b && rvvL1 b == wL8 b

theorem rvvBytes : (List.range 256).all rvvByteOK = true := by decide +kernel

def rvvU16OK (u : Nat) : Bool := rvvC16 u == w32 u && rvv8of16 u == w8 u

theorem rvvU16 : (List.range 65536).all rvvU16OK = true := by decide +kernel

theorem rvv_byte (b : Nat) (h : b < 256) : rvvC8 b = w01 b ∧ rvv16of8 b = w16 b ∧ rvvL1 b = wL8 b := by
  have := List.all_eq_true.mp rvvBytes b (List.mem_range.mpr h)
  simp only [rvvByteOK, Bool.and_eq_true, beq_iff_eq] at this
  exact ⟨this.1.1, this.1.2, this.2⟩

theorem rvv_u16 (u : Nat) (h : u < 65536) : rvvC16 u = w32 u ∧ rvv8of16 u = w8 u := by
  have := List.all_eq_true.mp rvvU16 u (List.mem_range.mpr h)
  simp only [rvvU16OK, Bool.and_eq_true, beq_iff_eq] at this
  exact this

/-- `utf8_length_from_latin1` starts from `count = len` (:75). -/
def rvvLatin1 (pick : Nat → Nat) (xs : List Nat) : Nat :=
  xs.length + rvvSum pick (fun b => ind (decide (i8 b < 0))) xs.length xs

/-- **The rvv length functions return the per-unit sums**, for every `pick`. -/
theorem rvv_lengths (pick : Nat → Nat) (xs : List Nat) :
    (Bytes xs → rvvSum pick rvvC8 xs.length xs = L (xs.map w01) ∧
      rvvSum pick rvv16of8 xs.length xs = L (xs.map w16) ∧ rvvLatin1 pick xs = L (xs.map wL8)) ∧
    (U16 xs → rvvSum pick rvvC16 xs.length xs = L (xs.map w32) ∧
      rvvSum pick rvv8of16 xs.length xs = L (xs.map w8)) ∧
    rvvSum pick sc8of32 xs.length xs = L (xs.map w8of32) ∧
    rvvSum pick sc16of32 xs.length xs = L (xs.map w16of32) := by
  have r := fun f => rvvSum_eq pick f xs.length xs (Nat.le_refl _)
  refine ⟨fun hb => ⟨?_, ?_, ?_⟩, fun hu => ⟨?_, ?_⟩, ?_, ?_⟩
  · rw [r]; exact L_map_congr _ _ _ (fun b h => (rvv_byte b (hb b h)).1)
  · rw [r]; exact L_map_congr _ _ _ (fun b h => (rvv_byte b (hb b h)).2.1)
  · unfold rvvLatin1; rw [r, ← L_map_one xs, ← L_map_add]
    exact L_map_congr _ _ _ (fun b h => (rvv_byte b (hb b h)).2.2)
  · rw [r]; exact L_map_congr _ _ _ (fun u h => (rvv_u16 u (hu u h)).1)
  · rw [r]; exact L_map_congr _ _ _ (fun u h => (rvv_u16 u (hu u h)).2)
  · rw [r]; exact L_map_congr _ _ _ (fun u _ => sc8of32_eq u)
  · rw [r]; exact L_map_congr _ _ _ (fun u _ => sc16of32_eq u)

end SafetyMargins.LengthsRvv
