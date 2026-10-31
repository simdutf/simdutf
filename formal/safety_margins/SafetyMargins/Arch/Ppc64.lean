import SafetyMargins.Kernels
import SafetyMargins.Utf8
/-
ppc64: every transcoding loop of src/ppc64/*.cpp and of the src/generic
headers instantiated by src/ppc64/implementation.cpp (base64 excluded).
`m_*` optimal margin, `cur_*` the source margin sufficient, `ok_*` optimal
margin sufficient (the source margin when it was lowered), `wit_*` one less
overflows.
-/
namespace SafetyMargins.Arch.Ppc64
open SafetyMargins SafetyMargins.Kernels SafetyMargins.Utf8

/-! ## UTF-16 to UTF-8 -/

/-- src/ppc64/ppc64_convert_utf16_to_utf8.cpp:186,
`while (end - buf >= 16 + safety_margin)`, `safety_margin = 12`.
One 8-unit register (`reg8ppc`):
* :229 1/2-byte path, `write_v_u16_11bits_to_utf8` (ppc64_write_to_utf8.cpp:56):
  one 16-byte store;
* :247 1/2/3-byte path, `ppc64_convert_utf16_to_1_2_3_bytes_of_utf8`:
  16-byte stores at +0 and at `row0[0]` = L(first 4 units) (:157-159; the
  ppc64 table `ppc64_utf16_to_utf8::pack_1_2_3_utf8_bytes`);
* :119-140 `mask == 0` (eight 3-byte units): 16-byte stores at +0 and +16,
  advance 24: ends 8 past L of the 8 units (`tail 8`);
* :253-291 scalar fallback: `forward = 15` plus the low half of a pair, so
  1..16 units, one byte at a time, returns on a bad pair.
:192-218: when the first 8 units are ASCII and the next 8 are not, a 16-byte
store for 8 units, then `in = nextin` and the register path runs in the same
iteration (`afterAscii8 16`). -/
def utf16to8 : List Iter := fallthrough8 16 reg8ppc

theorem m_utf16to8 : optimalMargin 16 false utf16to8 = 12 := by decide
theorem cur_utf16to8 : utf16to8.all (Iter.ok (need 16 12 false)) = true := by decide

/-- m = 11: 8 ASCII, 4 x U+0800, 4 ASCII, 11 ASCII (27 units, L = 35). The
ASCII half falls through to the 1/2/3-byte path, whose second store ends at
8 + 12 + 16 = 36. -/
theorem wit_utf16to8 :
    let it := afterAscii8 16 { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := ascii 8 ++ [3, 3, 3, 3] ++ ascii 4 ++ ascii 11
    it ∈ utf16to8 ∧ xs.length = need 16 11 false ∧
      written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-8 -/

/-- src/ppc64/ppc64_convert_utf32_to_utf8.cpp:22,
`while (end - buf >= 16 + safety_margin)`, `safety_margin = 12`. Same register
paths as UTF-16 (the 8 units are packed with unsigned saturation, :34, so a
unit > 0xFFFF never looks like 1, 2 or 3 bytes), but the scalar fallback
(:121-151) consumes at most 15 units (no pair). ASCII fall-through :45-74. -/
def reg8ppc32 : List Iter :=
  [ { consumed := 8, stores := [.rel 0 16] },
    { consumed := 8, stores := [.rel 0 16, .rel 4 16] },
    { consumed := 8, stores := [.rel 0 16, .tail 8] } ] ++ scalar 15

def utf32to8 : List Iter := fallthrough8 16 reg8ppc32

theorem m_utf32to8 : optimalMargin 16 false utf32to8 = 12 := by decide
theorem cur_utf32to8 : utf32to8.all (Iter.ok (need 16 12 false)) = true := by decide

/-- Same weights as `wit_utf16to8` (UTF-32 0x800 also weighs 3). -/
theorem wit_utf32to8 :
    let it := afterAscii8 16 { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := ascii 8 ++ [3, 3, 3, 3] ++ ascii 4 ++ ascii 11
    it ∈ utf32to8 ∧ xs.length = need 16 11 false ∧
      written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## Latin-1 to UTF-8 -/

/-- src/ppc64/ppc64_convert_latin1_to_utf8.cpp:30, `while (end - latin_input >= 16 + 8)`:
ASCII path (one 16-byte store for 16 bytes, :34-39), otherwise two calls of
`write_v_u16_11bits_to_utf8` (:50-51), the second at `row[0]` = L(first 8
bytes). -/
def latin1to8 : List Iter := latin1to8_two

theorem m_latin1to8 : optimalMargin 16 false latin1to8 = 8 := by decide
theorem cur_latin1to8 : latin1to8.all (Iter.ok (need 16 8 false)) = true := by decide

/-- m = 7: 0x80 then 22 ASCII (23 bytes, L = 24); the second store ends at
9 + 16 = 25. -/
theorem wit_latin1to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 8 16] }
    let xs := [2] ++ ascii 22
    it ∈ latin1to8 ∧ xs.length = need 16 7 false ∧
      written (need 16 7 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-- src/ppc64/ppc64_convert_latin1_to_utf8.cpp:55, the single block after the
loop, `if (end - latin_input >= 16)`: ASCII (16 for 16) or the low 8 bytes
with one 16-byte store (:69-71). -/
def latin1to8Last : List Iter := latin1to8_one

theorem m_latin1to8Last : optimalMargin 16 false latin1to8Last = 0 := by decide
theorem cur_latin1to8Last : latin1to8Last.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## Latin-1 to UTF-16 / UTF-32 (exact) -/

/-- src/ppc64/ppc64_convert_latin1_to_utf16.cpp:6 (`i < align_down<16>(len)`):
`store_bytes_as_utf16` writes 16 `char16_t` (two 16-byte stores). -/
def latin1to16 : List Iter := [{ consumed := 16, stores := [.rel 0 8, .rel 8 8] }]
/-- src/ppc64/ppc64_convert_latin1_to_utf32.cpp:6: `store_bytes_as_utf32`
writes 16 `char32_t` (four 16-byte stores). -/
def latin1to32 : List Iter :=
  [{ consumed := 16, stores := [.rel 0 4, .rel 4 4, .rel 8 4, .rel 12 4] }]

theorem m_latin1to16 : optimalMargin 16 false latin1to16 = 0 := by decide
theorem cur_latin1to16 : latin1to16.all (Iter.ok (need 16 0 false)) = true := by decide
theorem m_latin1to32 : optimalMargin 16 false latin1to32 = 0 := by decide
theorem cur_latin1to32 : latin1to32.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## UTF-16 / UTF-32 to Latin-1 (exact) -/

/-- src/ppc64/ppc64_convert_utf16_to_latin1.cpp:11, `while (end - buf >= 8)`:
`memcpy` of 8 bytes for 8 units, done before the high bytes are tested; on a
unit > 0xFF it returns without advancing (:50-58), so the 8 bytes were written
for an iteration that consumed nothing (`stop`). -/
def utf16toLatin1 : List Iter :=
  [ { consumed := 8, stores := [.rel 0 8] },
    { consumed := 0, stores := [.rel 0 8], stop := true } ]

theorem m_utf16toLatin1 : optimalMargin 8 false utf16toLatin1 = 0 := by decide
theorem cur_utf16toLatin1 : utf16toLatin1.all (Iter.ok (need 8 0 false)) = true := by decide

/-- src/ppc64/ppc64_convert_utf32_to_latin1.cpp:17, `i < align_down<16>(len)`:
16 bytes for 16 units; error check before the store (:23-31). -/
def utf32toLatin1 : List Iter :=
  [ { consumed := 16, stores := [.rel 0 16] },
    { consumed := 0, stores := [], stop := true } ]

theorem m_utf32toLatin1 : optimalMargin 16 false utf32toLatin1 = 0 := by decide
theorem cur_utf32toLatin1 : utf32toLatin1.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## UTF-32 to UTF-16 -/

/-- src/ppc64/ppc64_convert_utf32_to_utf16.cpp:18, `while (end - buf >= 8)`:
BMP path stores 8 `char16_t` for 8 units (:69); otherwise scalar over
`forward = 7` units (:73-105). Same shape as `Kernels.utf32to16_haswell`. -/
def utf32to16 : List Iter := [{ consumed := 8, stores := [.rel 0 8] }] ++ scalar 7

theorem m_utf32to16 : optimalMargin 8 false utf32to16 = 0 := by decide
theorem cur_utf32to16 : utf32to16.all (Iter.ok (need 8 0 false)) = true := by decide

/-! ## change_endianness_utf16 -/

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`:
`simd16x32::store` writes 4 x 8 units = 32 for 32. -/
def changeEndianness : List Iter :=
  [{ consumed := 32, stores := [.rel 0 8, .rel 8 8, .rel 16 8, .rel 24 8] }]

theorem m_changeEndianness : optimalMargin 32 false changeEndianness = 0 := by decide
theorem cur_changeEndianness :
    changeEndianness.all (Iter.ok (need 32 0 false)) = true := by decide

/-! ## UTF-16 to UTF-32: weight-0 units

`utf32_length_from_utf16` gives 0 to a low surrogate, so `Pos` fails and the
fixed-margin theorem does not apply. The kernel is safe because every store
ends at or before `L` of the units the iteration consumed. -/

/-- src/ppc64/ppc64_convert_utf16_to_utf32.cpp:16, `while (end - buf >= 8)`:
the vector path (:32-52) runs only when none of the 8 units is a surrogate, so
each weighs 1; it stores 8 `char32_t` (two 16-byte stores). -/
def utf16to32Vec : Iter := { consumed := 8, stores := [.rel 0 4, .rel 4 4] }

/-- Vector path or scalar fallback (:54-82, `forward = 15` plus the low half of
a pair: 1..16 units). The scalar loop writes one unit per non-surrogate and one
per valid pair (weights 1 + 0) and returns before writing on a bad pair. -/
def utf16to32 : List Iter := [utf16to32Vec] ++ scalar 16

theorem exact_vec (xs : List Nat) (h : xs.take 8 = List.replicate 8 1) :
    exactOn xs utf16to32Vec := by
  intro s hs
  simp only [utf16to32Vec, List.mem_cons] at hs
  rcases hs with rfl | rfl | hs
  · show L (xs.take 0) + 4 ≤ L (xs.take 8)
    rw [h, List.take_zero]; decide
  · show L (xs.take 4) + 4 ≤ L (xs.take 8)
    have h4 : xs.take 4 = List.replicate 4 1 := by
      have : xs.take 4 = (xs.take 8).take 4 := by simp [List.take_take]
      rw [this, h]; decide
    rw [h4, h]; decide
  · simp at hs

theorem scalar16_no_store : ∀ it ∈ scalar 16, it.stores = [] := by decide

theorem utf16to32_safe (choose : List Nat → Iter)
    (hc : ∀ xs, 8 ≤ xs.length → choose xs ∈ utf16to32)
    (hv : ∀ xs, choose xs = utf16to32Vec → xs.take 8 = List.replicate 8 1) :
    ∀ n xs, written 8 choose n xs ≤ L xs := by
  apply safe_exact
  intro xs hlen
  by_cases heq : choose xs = utf16to32Vec
  · rw [heq]
    exact exact_vec xs (hv xs heq)
  · have hm := hc xs hlen
    simp only [utf16to32, List.singleton_append, List.mem_cons] at hm
    rcases hm with h | h
    · exact absurd h heq
    · intro s hs
      rw [scalar16_no_store _ h] at hs
      simp at hs

/-! ## UTF-8 sources: dynamic margin `K`

src/generic/utf8_to_utf16/utf8_to_utf16.h:178/:275,
src/generic/utf8_to_utf32/utf8_to_utf32.h:175/:269,
src/generic/utf8_to_latin1/utf8_to_latin1.h:161 and :259,
src/generic/utf8_to_latin1/valid_utf8_to_latin1.h:18 (all `K = 3`), calling the
ppc64 `convert_masked_utf8_to_*`. Pairs are `(stored, produced + 1)`
(`Utf8.MPath`); exact paths are omitted. The 64-byte ASCII stores and
`convert_ascii_tail` write 1 unit per ASCII byte. -/

/-- src/ppc64/ppc64_convert_utf8_to_utf16.cpp: `0xfff` (:25-31, 16 stored, 12
produced), `0x924` (:52-79, 8 stored by `pack(composed, composed)`, 4),
`idx < 64` (:88-112, 8, 6), `idx < 145` (:113-135, 8, 4). `0xaaaa` (8, 8) and
`idx < 209` (scalar, a pair only when the lead byte is `≥ 0xF0`, see
`ppc_surrogate_lead`) are exact. -/
def ppc_utf16 : List MPath := [⟨16, 13⟩, ⟨8, 5⟩, ⟨8, 7⟩, ⟨8, 5⟩]
/-- src/ppc64/ppc64_convert_utf8_to_utf32.cpp: `0xfff` (:24-29, 16, 12),
`idx < 64` (:87-106, 8 by `store_words_as_utf32`, 6), `idx < 209`
(:125-148, 4, 3). `0xaaaa` (8, 8), `0x924` (4, 4), `idx < 145` (4, 4) exact. -/
def ppc_utf32 : List MPath := [⟨16, 13⟩, ⟨8, 7⟩, ⟨4, 4⟩]
/-- src/ppc64/ppc64_convert_utf8_to_latin1.cpp: `0xfff` (:24-29, 16, 12) and,
GCC build only, `idx < 64` (:63-65, `memcpy` of 8 for 6; the clang build copies
exactly 6, :58-61). `idx ≥ 64` writes nothing (:36-38). -/
def ppc_latin1 : List MPath := [⟨16, 13⟩, ⟨8, 7⟩]

theorem K_ppc_utf16 : okK 3 ppc_utf16 = true ∧ okK 2 ppc_utf16 = false := by decide
theorem K_ppc_utf32 : okK 3 ppc_utf32 = true ∧ okK 2 ppc_utf32 = false := by decide
theorem K_ppc_latin1 : okK 3 ppc_latin1 = true ∧ okK 2 ppc_latin1 = false := by decide

set_option maxRecDepth 100000 in
/-- The `idx < 209` UTF-16 path (:136-206) emits a surrogate pair when
`composed > 0x3c00000`. With the lead byte in the top byte of `perm` and every
other field at its maximum, a lead byte `< 0xF0` keeps `composed ≤ 0x3BFFFFF`:
so a pair (weight 2) is only produced from a lead byte that
`utf16_length_from_utf8` also counts twice. -/
theorem ppc_surrogate_lead :
    ∀ b, b < 0xF0 →
      ((b <<< 24) >>> 6) ||| (0x3f0000 >>> 4) ||| (0x3f00 >>> 2) ||| 0x7f ≤ 0x3c00000 := by
  decide

/-! ## `convert_valid` with a fixed margin

src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:15 and
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:12 use `safety_margin = 9` and `12`.
For valid input the overshoot is at most 4 units, which needs `m ≥ 9`
(UTF-16) and `m ≥ 12` (UTF-32): `Utf8.valid_utf16` and `Utf8.valid_utf32`. -/

end SafetyMargins.Arch.Ppc64
