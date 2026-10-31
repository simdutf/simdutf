import SafetyMargins.Kernels
import SafetyMargins.Utf8
/-
Haswell (AVX2): every transcoding loop of src/haswell/*.cpp and of the generic
headers as instantiated for haswell (base64 excluded). `m_*`: optimal margin;
`cur_*`: the current margin suffices; `ok_*`: the optimal one suffices (when
lower); `wit_*`: one less overflows.
-/
namespace SafetyMargins.Arch.Haswell
open SafetyMargins SafetyMargins.Kernels SafetyMargins.Utf8

/-! ## Latin-1 to UTF-8 -/

/-- src/haswell/avx2_convert_latin1_to_utf8.cpp:10, loop `end - in >= 16 + 8`.
ASCII path: one 16-byte store for 16 bytes (line 16). Otherwise one 16-byte
store per 8 input bytes (lines 70, 73), the second at `row[0]` = L(first 8
bytes) (`TableFacts.pack12_exact`). Same table as `Kernels.latin1to8_two`. -/
def latin1to8 : List Iter := latin1to8_two

theorem m_latin1to8 : optimalMargin 16 false latin1to8 = 8 := by decide
theorem ok_latin1to8 : latin1to8.all (Iter.ok (need 16 8 false)) = true := by decide

theorem safe_latin1to8 (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 8 false ≤ xs.length → choose xs ∈ latin1to8) :
    ∀ n xs, Pos xs → written (need 16 8 false) choose n xs ≤ L xs :=
  safe_of_paths _ latin1to8 ok_latin1to8 choose hc
/-- m = 7: 0x80 then 22 ASCII (23 bytes, L = 24); the second store ends at 25. -/
theorem wit_latin1to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 8 16] }
    let xs := [2] ++ ascii 22
    it ∈ latin1to8 ∧ xs.length = need 16 7 false ∧
    written (need 16 7 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## Latin-1 to UTF-16 and UTF-32: exact stores -/

/-- src/haswell/avx2_convert_latin1_to_utf16.cpp:8, `i < (len & ~15)`, i.e.
16 units guaranteed; one 32-byte store of 16 `char16_t` (line 24). -/
def latin1to16 : List Iter := [{ consumed := 16, stores := [.rel 0 16] }]
theorem m_latin1to16 : optimalMargin 16 false latin1to16 = 0 := by decide
theorem cur_latin1to16 : latin1to16.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/haswell/avx2_convert_latin1_to_utf32.cpp:6, `i < (len | 7) ^ 7`; one
store of 8 `char32_t` (line 15). -/
def latin1to32 : List Iter := [{ consumed := 8, stores := [.rel 0 8] }]
theorem m_latin1to32 : optimalMargin 8 false latin1to32 = 0 := by decide
theorem cur_latin1to32 : latin1to32.all (Iter.ok (need 8 0 false)) = true := by decide

/-! ## UTF-16 to Latin-1 -/

/-- src/haswell/avx2_convert_utf16_to_latin1.cpp:6, `end - buf >= 32`: one
32-byte store for 32 units (line 27), or return on a unit above 0xFF. -/
def utf16to1 : List Iter :=
  [{ consumed := 32, stores := [.rel 0 32] }, { consumed := 0, stores := [], stop := true }]
theorem m_utf16to1 : optimalMargin 32 false utf16to1 = 0 := by decide
theorem cur_utf16to1 : utf16to1.all (Iter.ok (need 32 0 false)) = true := by decide

/-- src/haswell/avx2_convert_utf16_to_latin1.cpp:44, `_with_errors`,
`end - buf >= 16`: two 8-byte stores (lines 60, 62), or the scalar loop of
line 68, which returns on the first unit above 0xFF. -/
def utf16to1_err : List Iter :=
  [{ consumed := 16, stores := [.rel 0 8, .rel 8 8] }] ++ scalar 16
theorem m_utf16to1_err : optimalMargin 16 false utf16to1_err = 0 := by decide
theorem cur_utf16to1_err : utf16to1_err.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## UTF-32 to Latin-1 -/

/-- src/haswell/avx2_convert_utf32_to_latin1.cpp:9 (plain, return on error)
and :70 (`_with_errors`, scalar loop of line 81 that always returns, since the
vector test guarantees a unit above 0xFF): `i < len & ~31`, one 32-byte store
(lines 51, 108). -/
def utf32to1 : List Iter := [{ consumed := 32, stores := [.rel 0 32] }] ++ scalar 32
theorem m_utf32to1 : optimalMargin 32 false utf32to1 = 0 := by decide
theorem cur_utf32to1 : utf32to1.all (Iter.ok (need 32 0 false)) = true := by decide

/-! ## UTF-16 to UTF-8 and UTF-32 to UTF-8 -/

/-- src/haswell/avx2_convert_utf16_to_utf8.cpp:66 (plain) and :345
(`_with_errors`), loop `end - buf >= 16 + 12`. ASCII store (81), 1/2-byte path
(139, 142), 1/2/3-byte path (270, 272, 274, 276: units 0-3, 4-7, 8-11, 12-15,
from the per-lane unpack of `out0`/`out1`), scalar fallback of at most 16 units
(15 plus the low half of a pair). Same table as `Kernels.wide16`.

src/haswell/avx2_convert_utf32_to_utf8.cpp:17 (plain) and :301
(`_with_errors`): the same paths on the packed 16-unit register, the scalar
fallback consuming at most 15 units. -/
def utf16to8 : List Iter := wide16
def utf32to8 : List Iter := wide16

theorem m_utf16to8 : optimalMargin 16 false utf16to8 = 12 := by decide
theorem cur_utf16to8 : utf16to8.all (Iter.ok (need 16 12 false)) = true := by decide
theorem m_utf32to8 : optimalMargin 16 false utf32to8 = 12 := by decide
theorem cur_utf32to8 : utf32to8.all (Iter.ok (need 16 12 false)) = true := by decide

/-- m = 11: twelve U+0800, 4 ASCII, 11 ASCII (27 units, L = 51); the fourth
store ends at 52. -/
theorem wit_utf16to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 4 16, .rel 8 16, .rel 12 16] }
    let xs := List.replicate 12 3 ++ ascii 4 ++ ascii 11
    it ∈ utf16to8 ∧ xs.length = need 16 11 false ∧
    written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

theorem wit_utf32to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 4 16, .rel 8 16, .rel 12 16] }
    let xs := List.replicate 12 3 ++ ascii 4 ++ ascii 11
    it ∈ utf32to8 ∧ xs.length = need 16 11 false ∧
    written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-16 -/

/-- src/haswell/avx2_convert_utf32_to_utf16.cpp:16 (plain) and :98
(`_with_errors`), loop `end - buf >= 8`: BMP path stores exactly 8 units
(lines 33, 119); otherwise a scalar loop of at most 7 units. Same table as
`Kernels.utf32to16_haswell`. -/
def utf32to16 : List Iter := utf32to16_haswell
theorem m_utf32to16 : optimalMargin 8 false utf32to16 = 0 := by decide
theorem ok_utf32to16 : utf32to16.all (Iter.ok (need 8 0 false)) = true := by decide

theorem safe_utf32to16 (choose : List Nat → Iter)
    (hc : ∀ xs, need 8 0 false ≤ xs.length → choose xs ∈ utf32to16) :
    ∀ n xs, Pos xs → written (need 8 0 false) choose n xs ≤ L xs :=
  safe_of_paths _ utf32to16 ok_utf32to16 choose hc

/-! ## UTF-16 byte swap -/

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`: one
exact store of 32 units per 32 units (`simd16x32::store`). -/
def swap16 : List Iter := [{ consumed := 32, stores := [.rel 0 32] }]
theorem m_swap16 : optimalMargin 32 false swap16 = 0 := by decide
theorem cur_swap16 : swap16.all (Iter.ok (need 32 0 false)) = true := by decide

/-- src/haswell/avx2_utf16fix.cpp:157/163 (`i + 16 < n`, strict) and the SSE
variant :124/130 (`i + 8 < n`): each block stores 16 (8) units at its own input
offset, plus `out[-1]`, which is the already-written previous unit. The final
block is at `n - 16` (`n - 8`), inside the buffer since `n ≥ 17` (`n ≥ 9`). -/
def utf16fix : List Iter := [{ consumed := 16, stores := [.rel 0 16] }]
theorem m_utf16fix : optimalMargin 16 true utf16fix = 0 := by decide
theorem cur_utf16fix : utf16fix.all (Iter.ok (need 16 0 true)) = true := by decide
def utf16fix_sse : List Iter := [{ consumed := 8, stores := [.rel 0 8] }]
theorem m_utf16fix_sse : optimalMargin 8 true utf16fix_sse = 0 := by decide
theorem cur_utf16fix_sse : utf16fix_sse.all (Iter.ok (need 8 0 true)) = true := by decide

/-! ## UTF-16 to UTF-32: a length function with minimum 0

`utf32_length_from_utf16` gives 0 to a low surrogate, so `Pos` fails and the
fixed-margin model does not apply. The haswell kernel
(src/haswell/avx2_convert_utf16_to_utf32.cpp:62 and :144, `end - buf >= 16`)
only has exact stores:

* vector path (lines 86-90, 168-172): taken only when none of the 16 units is a
  surrogate, so each weighs 1 and the 16 stored units are `L` of the 16
  consumed units;
* scalar fallback: a non-surrogate writes 1 (weight 1); a high surrogate
  followed by a low surrogate writes 1 (weights 1 + 0) and consumes both; a
  lone high or a low surrogate returns before writing.

Every store ends at `L` of the consumed prefix (`tail 0`), so `safe_of_exact`
applies without `Pos` or margin. -/
def utf16to32 : List Iter := [{ consumed := 16, stores := [.tail 0] }] ++ scalar 16

theorem exact_utf16to32 : utf16to32.all Iter.exact = true := by decide

theorem safe_utf16to32 (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 0 false ≤ xs.length → choose xs ∈ utf16to32) :
    ∀ n xs, written (need 16 0 false) choose n xs ≤ L xs :=
  safe_of_exact _ utf16to32 exact_utf16to32 choose hc

/-! ## UTF-8 sources (dynamic margin `K`)

src/haswell/avx2_convert_utf8_to_utf16.cpp, `convert_masked_utf8_to_utf16`:
`0xfff` stores 16 units (line 36), advances 12; `0xaaaa` stores 8, advances 8
(exact); `0x924` stores 8 (line 76), advances 4; `idx < 64` stores 8 (line 100),
advances 6; `idx < 145` stores 8 (line 121), advances 4; `idx < 209` writes
exactly what it produces; `idx >= 209` writes nothing and consumes 12. -/
def utf8to16 : List MPath := [⟨16, 13⟩, ⟨8, 5⟩, ⟨8, 7⟩, ⟨8, 5⟩]

/-- src/haswell/avx2_convert_utf8_to_utf32.cpp: `0xfff` two stores of 8
(lines 26, 28), advances 12; `idx < 64` stores 8 (line 88), advances 6;
`idx < 209` stores 4 (line 128), advances 3; the others are exact. -/
def utf8to32 : List MPath := [⟨16, 13⟩, ⟨8, 7⟩, ⟨4, 4⟩]

/-- src/haswell/avx2_convert_utf8_to_latin1.cpp: `0xfff` stores 16 bytes
(line 28), advances 12; `idx < 64` stores 8 (line 57), advances 6. -/
def utf8to1 : List MPath := [⟨16, 13⟩, ⟨8, 7⟩]

/-- `K = 3`, the header value, is optimal for all three. -/
theorem m_utf8to16 : okK 3 utf8to16 = true ∧ okK 2 utf8to16 = false := by decide
theorem m_utf8to32 : okK 3 utf8to32 = true ∧ okK 2 utf8to32 = false := by decide
theorem m_utf8to1 : okK 3 utf8to1 = true ∧ okK 2 utf8to1 = false := by decide

end SafetyMargins.Arch.Haswell
