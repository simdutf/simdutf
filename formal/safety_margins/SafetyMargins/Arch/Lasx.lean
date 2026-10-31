import SafetyMargins.Kernels
import SafetyMargins.Utf8
/-
LoongArch LASX: every transcoding loop of src/lasx/*.cpp and of the generic
headers as instantiated for lasx (base64 excluded). A `__lsx_vst` is 16 bytes
(16 UTF-8 bytes, 8 char16, 4 char32); a `__lasx_xvst` is 32 bytes.
-/
namespace SafetyMargins.Arch.Lasx
open SafetyMargins SafetyMargins.Kernels SafetyMargins.Utf8

/-! ## Latin-1 to UTF-8 -/

/-- src/lasx/lasx_convert_latin1_to_utf8.cpp:15, `end - latin1_input >= 16 + 8`.
ASCII (:18-23): one 16-byte store, advance 16. Otherwise (:42-61): a 16-byte
store at 0 and a 16-byte store at `row0[0]` = L(bytes 0..7)
(`lasx_1_2_utf8_bytes_mask` maps the 8-bit ASCII mask to a `pack_1_2_utf8_bytes`
row whose count is that sum; checked over all 256 masks). No scalar path, no
error path (Latin-1 is always valid). Same table as `Kernels.latin1to8_two`. -/
def latin1to8 : List Iter := Kernels.latin1to8_two

theorem m_latin1to8 : optimalMargin 16 false latin1to8 = 8 := by decide
theorem ok_latin1to8 : latin1to8.all (Iter.ok (need 16 8 false)) = true := by decide

theorem safe_latin1to8 (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 8 false ≤ xs.length → choose xs ∈ latin1to8) :
    ∀ n xs, Pos xs → written (need 16 8 false) choose n xs ≤ L xs :=
  safe_of_paths _ latin1to8 ok_latin1to8 choose hc
/-- 0x80 then 22 ASCII (23 bytes, L = 24): the second store starts at 2 + 7 = 9
and ends at 25. -/
theorem wit_latin1to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 8 16] }
    let xs := [2] ++ ascii 22
    it ∈ latin1to8 ∧ xs.length = need 16 7 false ∧
      written (need 16 7 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## Latin-1 to UTF-16 and UTF-32 (exact stores) -/

/-- src/lasx/lasx_convert_latin1_to_utf16.cpp:12 (LE) and :50 (BE),
`end - buf >= 32`: two `xvst` of 16 char16 for 32 bytes. -/
def latin1to16 : List Iter := [{ consumed := 32, stores := [.rel 0 16, .rel 16 16] }]
/-- The one-shot block after the loop, :25 and :63, `end - buf >= 16`: two
`vst` of 8 char16. -/
def latin1to16_tail : List Iter := [{ consumed := 16, stores := [.rel 0 8, .rel 8 8] }]
/-- src/lasx/lasx_convert_latin1_to_utf32.cpp:12, `end - buf >= 32`: four `xvst`
of 8 char32. -/
def latin1to32 : List Iter :=
  [{ consumed := 32, stores := [.rel 0 8, .rel 8 8, .rel 16 8, .rel 24 8] }]
/-- :34, `end - buf >= 16`: four `vst` of 4 char32. -/
def latin1to32_tail : List Iter :=
  [{ consumed := 16, stores := [.rel 0 4, .rel 4 4, .rel 8 4, .rel 12 4] }]

theorem m_latin1to16 : optimalMargin 32 false latin1to16 = 0 := by decide
theorem cur_latin1to16 : latin1to16.all (Iter.ok (need 32 0 false)) = true := by decide
theorem m_latin1to16_tail : optimalMargin 16 false latin1to16_tail = 0 := by decide
theorem cur_latin1to16_tail : latin1to16_tail.all (Iter.ok (need 16 0 false)) = true := by decide
theorem m_latin1to32 : optimalMargin 32 false latin1to32 = 0 := by decide
theorem cur_latin1to32 : latin1to32.all (Iter.ok (need 32 0 false)) = true := by decide
theorem m_latin1to32_tail : optimalMargin 16 false latin1to32_tail = 0 := by decide
theorem cur_latin1to32_tail : latin1to32_tail.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## UTF-16 and UTF-32 to Latin-1 (exact stores) -/

/-- src/lasx/lasx_convert_utf16_to_latin1.cpp:6 and :34, and
src/lasx/lasx_convert_utf32_to_latin1.cpp:9 and :42, all `end - buf >= 16`:
the all-Latin-1 path stores 16 bytes for 16 units; otherwise the plain function
returns (`stop`) and `_with_errors` writes one byte per unit until the first
unit above 0xFF (scalar). -/
def toLatin1 : List Iter := [{ consumed := 16, stores := [.rel 0 16] }] ++ scalar 16

theorem m_toLatin1 : optimalMargin 16 false toLatin1 = 0 := by decide
theorem cur_toLatin1 : toLatin1.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## UTF-16 to UTF-8 -/

/-- src/lasx/lasx_convert_utf16_to_utf8.cpp:67 and :319 (`_with_errors`),
`end - buf >= 16 + 12`: ASCII (:72-83, one 16-byte store), 1/2-byte (:85-129,
16-byte stores at 0 and `row1[0]` = L(units 0..7)), 1/2/3-byte (:134-251, four
16-byte stores at L of 0, 4, 8, 12 units, `pack_1_2_3_utf8_bytes`), surrogates
(:253-292, scalar, 15 or 16 units, returns on a bad pair). Every path
`continue`s or ends the iteration: no half-register fall-through. This is
`Kernels.wide16`. -/
def utf16to8 : List Iter := Kernels.wide16

theorem m_utf16to8 : optimalMargin 16 false utf16to8 = 12 := by decide
theorem cur_utf16to8 : utf16to8.all (Iter.ok (need 16 12 false)) = true := by decide
/-- 12 x U+0800, 15 ASCII (27 units, L = 51): the fourth store of the 1/2/3
path starts at L(12 units) = 36 and ends at 52. -/
theorem wit_utf16to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 4 16, .rel 8 16, .rel 12 16] }
    let xs := List.replicate 12 3 ++ ascii 4 ++ ascii 11
    it ∈ utf16to8 ∧ xs.length = need 16 11 false ∧
      written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-8 -/

/-- src/lasx/lasx_convert_utf32_to_utf8.cpp:45 and :340 (`_with_errors`),
strict `end - buf > 16 + 11`: the same three vector paths as UTF-16 (after
packing 16 BMP units to 16 bits), and a scalar fallback of 15 units for any unit
above 0xFFFF. -/
def utf32to8 : List Iter := Kernels.wide16

theorem m_utf32to8 : optimalMargin 16 true utf32to8 = 11 := by decide
theorem ok_utf32to8 : utf32to8.all (Iter.ok (need 16 11 true)) = true := by decide

theorem safe_utf32to8 (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 11 true ≤ xs.length → choose xs ∈ utf32to8) :
    ∀ n xs, Pos xs → written (need 16 11 true) choose n xs ≤ L xs :=
  safe_of_paths _ utf32to8 ok_utf32to8 choose hc
theorem wit_utf32to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 4 16, .rel 8 16, .rel 12 16] }
    let xs := List.replicate 12 3 ++ ascii 4 ++ ascii 11
    it ∈ utf32to8 ∧ xs.length = need 16 10 true ∧
      written (need 16 10 true) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-16 -/

/-- src/lasx/lasx_convert_utf32_to_utf16.cpp:43 and :150, `end - buf >= 16`:
the BMP path stores 16 char16 for 16 units (one `xvst`); otherwise scalar over
15 units, returning on a surrogate or a value above 0x10FFFF. -/
def utf32to16 : List Iter := [{ consumed := 16, stores := [.rel 0 16] }] ++ scalar 15

theorem m_utf32to16 : optimalMargin 16 false utf32to16 = 0 := by decide
theorem cur_utf32to16 : utf32to16.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## UTF-16 to UTF-16 utilities -/

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`, i.e.
32 units remain: `simd16x32::store`, two `xvst` of 16 units. -/
def changeEndianness : List Iter := [{ consumed := 32, stores := [.rel 0 16, .rel 16 16] }]
theorem m_changeEndianness : optimalMargin 32 false changeEndianness = 0 := by decide
theorem cur_changeEndianness : changeEndianness.all (Iter.ok (need 32 0 false)) = true := by decide

/-! ## UTF-16 to UTF-32: the length function can be 0

`utf32_length_from_utf16` gives 0 to a low surrogate, so `Pos` fails and the
fixed-margin model does not apply. The lasx kernel
(src/lasx/lasx_convert_utf16_to_utf32.cpp:36 and :130, `end - buf >= 16`) only
uses a vector store when the 16-unit register has no surrogate at all
(:46, :140), so every unit under that store weighs exactly 1; the scalar path
writes one unit per non-surrogate and one per valid pair, and returns before
writing on a lone surrogate. -/

/-- Vector path: 16 surrogate-free units, 16 char32 stored. -/
def utf16to32Vec : Iter := { consumed := 16, stores := [.rel 0 16] }

/-- Vector path or scalar fallback (at most 16 units). -/
def utf16to32 : List Iter := [utf16to32Vec] ++ scalar 16

/-- Any path selection that takes the vector path only on 16 non-surrogate
units never writes past `L`, for every input. -/
theorem utf16to32_safe (choose : List Nat → Iter)
    (hc : ∀ xs, 16 ≤ xs.length → choose xs ∈ utf16to32)
    (hv : ∀ xs, choose xs = utf16to32Vec → xs.take 16 = List.replicate 16 1) :
    ∀ n xs, written 16 choose n xs ≤ L xs := by
  apply safe_exact
  intro xs hlen
  by_cases heq : choose xs = utf16to32Vec
  · rw [heq]
    intro s hs
    simp only [utf16to32Vec, List.mem_singleton] at hs
    subst hs
    show L (xs.take 0) + 16 ≤ L (xs.take 16)
    rw [hv xs heq, List.take_zero]
    decide
  · have hm := hc xs hlen
    have hsc : choose xs ∈ scalar 16 := by
      simp only [utf16to32, List.singleton_append, List.mem_cons] at hm
      exact hm.resolve_left heq
    have hnil : ∀ it ∈ scalar 16, it.stores = [] := by decide
    intro s hs
    rw [hnil _ hsc] at hs
    simp at hs

/-! ## UTF-8 sources (dynamic margin `K`)

Store paths `(stored, available)` of the lasx `convert_masked_utf8_to_*`, all
paths including the exact ones. `available` is the number of leading bytes in
the consumed bytes plus the next one (`e + 1` for a call that produced `e`
units from an aligned start). -/

/-- src/lasx/lasx_convert_utf8_to_utf16.cpp: ASCII :24 (16, 16 + 1), `0x924`
:43 (8, 4 + 1), `0xAAAA` :58 (8, 8 + 1: 8 two-byte characters), `idx < 64` :80 (8, 6 + 1), `idx < 145` :92
(8, 4 + 1), `0x888` :140 (8, 7): the three lead bytes are checked to be
F0..F7, which the length function counts 2 each, so the 6 units produced equal
L of the 12 consumed bytes and the byte at 12 is leading (6 + 1). The generic
3-code-point path :201-291 stores to a stack buffer and writes the output one
unit at a time (exact). -/
def lasx_utf16 : List MPath := [⟨16, 17⟩, ⟨8, 5⟩, ⟨8, 9⟩, ⟨8, 7⟩, ⟨8, 5⟩, ⟨8, 7⟩]
/-- src/lasx/lasx_convert_utf8_to_utf32.cpp: ASCII :23 (16, 17), `0x924` :44
(4, 5), `0xaaa` :55 (8, 6 + 1), `idx < 64` :75 (8, 7), `idx < 145` :86 (4, 5),
`0x888` :121 (4, 3 + 1), 3-code-point :155 (4, 4). -/
def lasx_utf32 : List MPath := [⟨16, 17⟩, ⟨4, 5⟩, ⟨8, 7⟩, ⟨8, 7⟩, ⟨4, 5⟩, ⟨4, 4⟩, ⟨4, 4⟩]
/-- src/lasx/lasx_convert_utf8_to_latin1.cpp: ASCII :19 (16, 17), `idx < 64`
:43-74 (`memcpy` of 6 bytes, 6 + 1). Every other index returns without writing. -/
def lasx_latin1 : List MPath := [⟨16, 17⟩, ⟨6, 7⟩]

theorem K_lasx_utf16 : okK 3 lasx_utf16 = true ∧ okK 2 lasx_utf16 = false := by decide
theorem K_lasx_utf32 : okK 1 lasx_utf32 = true ∧ okK 0 lasx_utf32 = false := by decide
theorem K_lasx_latin1 : okK 0 lasx_latin1 = true := by decide

/-- The headers' `K = 3` is sufficient for the write paths. -/
theorem curK : okK 3 lasx_utf32 = true ∧ okK 3 lasx_latin1 = true := by decide

/-! ## `convert_valid` on valid input

UTF-16 (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:15): the worst lasx
deficit is 8 - 4 = 4 (`0x924`, `idx < 145`), so margin 9 suffices
(`Utf8.valid_utf16`, tight by `Utf8.valid_utf16_tight`).
UTF-32 (src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:12): `0xaaa` and
`idx < 64` store 8 units and produce 6, deficit 2, so the `1 + m` bytes after
the call must hold 2 code points: m = 4. The 16-byte loads need only m ≥ 3. -/

theorem valid_utf32_lasx (ks : List Nat) (h : Valid ks) (m : Nat) (hm : 4 ≤ m)
    (hb : 1 + m ≤ bytes ks) : 2 ≤ ks.length := by
  have := bytes_le_4u32 ks h
  omega

/-- and 3 does not: one 4-byte character fills 4 bytes with 1 unit. -/
theorem valid_utf32_lasx_tight : bytes [4] = 1 + 3 ∧ [4].length = 1 := by decide

/-- Store form of `valid_utf32_lasx`, same deficit as lsx. The shared header
stays `Utf8.valid_utf32_store`. -/
theorem valid_utf32_lasx_store (pre : List Nat) (ks : List Nat) (s m : Nat)
    (h : Valid ks) (hm : 4 ≤ m) (hb : 1 + m ≤ bytes ks)
    (hs : s ≤ L pre + 2) : s ≤ L (pre ++ expand32 ks) := by
  have hden := valid_utf32_lasx ks h m hm hb
  exact overshoot_le pre (expand32 ks) s 2 hs (by rw [L_expand32 ks h]; exact hden)

end SafetyMargins.Arch.Lasx
