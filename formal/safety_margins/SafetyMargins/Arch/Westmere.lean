import SafetyMargins.Kernels
import SafetyMargins.Utf8
/-
Westmere (SSE4.2): every transcoding loop of src/westmere/*.cpp and of the
generic headers as instantiated for westmere (base64 excluded). `m_*`: optimal
margin; `cur_*`: the current margin suffices; `ok_*`: the optimal one suffices
(when lower); `wit_*`: one less overflows.
-/
namespace SafetyMargins.Arch.Westmere
open SafetyMargins SafetyMargins.Kernels SafetyMargins.Utf8

/-! ## UTF-16 to UTF-8 -/

/-- src/westmere/sse_convert_utf16_to_utf8.cpp:67 (`sse_convert_utf16_to_utf8`)
and :296 (`_with_errors`), loop `end - buf >= 16 + 12`. ASCII 16 (:96-102),
ASCII 8 then fall-through `in = nextin` (:83-92), 1/2-byte path (:118-123),
1/2/3-byte path (:224-228), `mask == 0` path (:197-208, second store at +12 =
L(first 4 units)), scalar fallback consuming at most 16 units (:235-268).
Also used by `convert_utf16*_to_utf8_with_replacement` (implementation.cpp:1243)
through `convert_utf16_to_utf8_with_details`. Same table as `Kernels`. -/
def utf16to8 : List Iter := Kernels.utf16to8_westmere

theorem m_utf16to8 : optimalMargin 16 false utf16to8 = 12 := by decide
theorem cur_utf16to8 : utf16to8.all (Iter.ok (need 16 12 false)) = true := by decide
/-- 8 ASCII, 4 x U+0800, 4 ASCII, 11 ASCII: 27 units, L = 35, store end 36. -/
theorem wit_utf16to8 :
    let it := afterAscii8 16 { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := ascii 8 ++ [3, 3, 3, 3] ++ ascii 4 ++ ascii 11
    it ∈ utf16to8 ∧ xs.length = need 16 11 false ∧
      written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-8 -/

/-- src/westmere/sse_convert_utf32_to_utf8.cpp:22, plain function, loop
`end - buf >= 16 + 12`; the ASCII test over 16 units falls through to the
second pair of registers (:70-89). -/
def utf32to8 : List Iter := Kernels.utf32to8_westmere

theorem m_utf32to8 : optimalMargin 16 false utf32to8 = 12 := by decide
theorem cur_utf32to8 : utf32to8.all (Iter.ok (need 16 12 false)) = true := by decide
theorem wit_utf32to8 :
    let it := afterAscii8 16 { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := ascii 8 ++ [3, 3, 3, 3] ++ ascii 4 ++ ascii 11
    it ∈ utf32to8 ∧ xs.length = need 16 11 false ∧
      written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-- src/westmere/sse_convert_utf32_to_utf8.cpp:349, `_with_errors`, loop
`end - buf >= 16 + 4`; ASCII path stores 16 bytes for 8 units and
`continue`s (:369-379). -/
def utf32to8_err : List Iter := Kernels.utf32to8_westmere_err

theorem m_utf32to8_err : optimalMargin 16 false utf32to8_err = 4 := by decide
theorem ok_utf32to8_err : utf32to8_err.all (Iter.ok (need 16 4 false)) = true := by decide

theorem safe_utf32to8_err (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 4 false ≤ xs.length → choose xs ∈ utf32to8_err) :
    ∀ n xs, Pos xs → written (need 16 4 false) choose n xs ≤ L xs :=
  safe_of_paths _ utf32to8_err ok_utf32to8_err choose hc
/-- 4 x U+0800, 4 ASCII, 11 ASCII: 19 units, L = 27, store end 28. -/
theorem wit_utf32to8_err :
    let it : Iter := { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := [3, 3, 3, 3] ++ ascii 4 ++ ascii 11
    it ∈ utf32to8_err ∧ xs.length = need 16 3 false ∧
      written (need 16 3 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-16 -/

/-- src/westmere/sse_convert_utf32_to_utf16.cpp:65, plain function, inline
loop `end - buf >= 16 + 4`: BMP path two stores of 8 (:94-95); otherwise
validated (:99) and four `sse_expand_surrogate` stores of 8 char16_t each
advancing `4 + popcount` (:103-117). -/
def utf32to16 : List Iter := Kernels.utf32to16_westmere

theorem m_utf32to16 : optimalMargin 16 false utf32to16 = 4 := by decide
theorem ok_utf32to16 : utf32to16.all (Iter.ok (need 16 4 false)) = true := by decide

theorem safe_utf32to16 (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 4 false ≤ xs.length → choose xs ∈ utf32to16) :
    ∀ n xs, Pos xs → written (need 16 4 false) choose n xs ≤ L xs :=
  safe_of_paths _ utf32to16 ok_utf32to16 choose hc
/-- U+10000 then 18 ASCII: 19 units, L = 20, fourth store ends at 21. -/
theorem wit_utf32to16 :
    let it : Iter := { consumed := 16, stores := [.rel 0 8, .rel 4 8, .rel 8 8, .rel 12 8] }
    let xs := [2] ++ ascii 18
    it ∈ utf32to16 ∧ xs.length = need 16 3 false ∧
      written (need 16 3 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-- src/westmere/sse_convert_utf32_to_utf16.cpp:140, `_with_errors`, loop
`end - buf >= 8`: BMP path stores exactly 8 (:164); otherwise scalar over
`forward = 7` units (:168-204). Same shape as `Kernels.utf32to16_haswell`. -/
def utf32to16_err : List Iter := Kernels.utf32to16_haswell

theorem m_utf32to16_err : optimalMargin 8 false utf32to16_err = 0 := by decide
theorem cur_utf32to16_err : utf32to16_err.all (Iter.ok (need 8 0 false)) = true := by decide

/-! ## Latin-1 to UTF-8 -/

/-- src/westmere/sse_convert_latin1_to_utf8.cpp:25, loop
`end - latin_input >= 16 + 8`: ASCII path (:29-34), otherwise two
`write_v_u16_11bits_to_utf8` 16-byte stores, the second at `row[0]` = L(first
8 bytes) (:44-47; internal/write_v_u16_11bits_to_utf8.cpp:49-52). -/
def latin1to8 : List Iter := Kernels.latin1to8_two

theorem m_latin1to8 : optimalMargin 16 false latin1to8 = 8 := by decide
theorem cur_latin1to8 : latin1to8.all (Iter.ok (need 16 8 false)) = true := by decide
/-- 0x80 then 22 ASCII: 23 bytes, L = 24, second store ends at 25. -/
theorem wit_latin1to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 8 16] }
    let xs := [2] ++ ascii 22
    it ∈ latin1to8 ∧ xs.length = need 16 7 false ∧
      written (need 16 7 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-- src/westmere/sse_convert_latin1_to_utf8.cpp:51, the one-shot block after
the loop, `end - latin_input >= 16`: ASCII 16 (:55-58) or one 16-byte store
for 8 bytes (:62-66). -/
def latin1to8_last : List Iter := Kernels.latin1to8_one

theorem m_latin1to8_last : optimalMargin 16 false latin1to8_last = 0 := by decide
theorem cur_latin1to8_last : latin1to8_last.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## Exact-store kernels (latin1 to UTF-16/32, to Latin-1, UTF-16 utilities) -/

/-- src/westmere/sse_convert_latin1_to_utf16.cpp:6, `i < rounded_len`
(16 units remain): two stores of 8 char16_t. -/
def latin1to16 : List Iter := [{ consumed := 16, stores := [.rel 0 8, .rel 8 8] }]
theorem m_latin1to16 : optimalMargin 16 false latin1to16 = 0 := by decide
theorem cur_latin1to16 : latin1to16.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/westmere/sse_convert_latin1_to_utf32.cpp:6, `end - buf >= 16`: four
stores of 4 char32_t. -/
def latin1to32 : List Iter :=
  [{ consumed := 16, stores := [.rel 0 4, .rel 4 4, .rel 8 4, .rel 12 4] }]
theorem m_latin1to32 : optimalMargin 16 false latin1to32 = 0 := by decide
theorem cur_latin1to32 : latin1to32.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/westmere/sse_convert_utf16_to_latin1.cpp:6, `end - buf >= 8`: 8-byte
store (:20) or return `nullptr` (:26). -/
def utf16tolatin1 : List Iter :=
  [{ consumed := 8, stores := [.rel 0 8] }, { consumed := 0, stores := [], stop := true }]
theorem m_utf16tolatin1 : optimalMargin 8 false utf16tolatin1 = 0 := by decide
theorem cur_utf16tolatin1 : utf16tolatin1.all (Iter.ok (need 8 0 false)) = true := by decide

/-- src/westmere/sse_convert_utf16_to_latin1.cpp:38, `_with_errors`: 8-byte
store or a scalar pass over the 8 units (:56-65). -/
def utf16tolatin1_err : List Iter := [{ consumed := 8, stores := [.rel 0 8] }] ++ scalar 8
theorem m_utf16tolatin1_err : optimalMargin 8 false utf16tolatin1_err = 0 := by decide
theorem cur_utf16tolatin1_err : utf16tolatin1_err.all (Iter.ok (need 8 0 false)) = true := by decide

/-- src/westmere/sse_convert_utf32_to_latin1.cpp:10 and :46, 16 units per
iteration, one 16-byte store (:28, :75); plain returns `nullptr` (:21),
`_with_errors` goes scalar over the 16 units (:58-68). -/
def utf32tolatin1 : List Iter :=
  [{ consumed := 16, stores := [.rel 0 16] }, { consumed := 0, stores := [], stop := true }]
def utf32tolatin1_err : List Iter := [{ consumed := 16, stores := [.rel 0 16] }] ++ scalar 16
theorem m_utf32tolatin1 : optimalMargin 16 false utf32tolatin1 = 0 := by decide
theorem cur_utf32tolatin1 : utf32tolatin1.all (Iter.ok (need 16 0 false)) = true := by decide
theorem m_utf32tolatin1_err : optimalMargin 16 false utf32tolatin1_err = 0 := by decide
theorem cur_utf32tolatin1_err : utf32tolatin1_err.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/generic/utf16/change_endianness.h:10 (westmere implementation.cpp:1102),
`pos < size / 32 * 32`: one 32-unit store. -/
def changeEndianness : List Iter := [{ consumed := 32, stores := [.rel 0 32] }]
theorem m_changeEndianness : optimalMargin 32 false changeEndianness = 0 := by decide
theorem cur_changeEndianness : changeEndianness.all (Iter.ok (need 32 0 false)) = true := by decide

/-- src/westmere/sse_utf16fix.cpp:66 and :72, `i + 8 < n` (strict): one
8-unit store at `out + i` (:46, :48); the write to `out[-1]` is behind the
cursor. The final block at `out + n - 8` (:70, :76) ends at `n`. -/
def utf16fix : List Iter := [{ consumed := 8, stores := [.rel 0 8] }]
theorem m_utf16fix : optimalMargin 8 true utf16fix = 0 := by decide
theorem cur_utf16fix : utf16fix.all (Iter.ok (need 8 0 true)) = true := by decide

/-! ## UTF-16 to UTF-32: min-0 length function

`utf32_length_from_utf16` gives 0 to a low surrogate, so `Pos` fails and the
fixed-margin theorem does not apply. The kernel never stores past its logical
output: the vector path (src/westmere/sse_convert_utf16_to_utf32.cpp:84-91 and
:164-171) runs only on 8 units without surrogates, which weigh exactly 1, and
stores exactly 8; the scalar path (:97-120, :177-202) writes one unit per
non-surrogate and one per high+low pair, and returns before writing on any
other surrogate. So every store ends at `L` of the consumed prefix
(`tail 0`), whatever follows. -/

def utf16to32 : List Iter := [{ consumed := 8, stores := [.tail 0] }] ++ scalar 16

theorem exact_utf16to32 : utf16to32.all Iter.exact = true := by decide

theorem safe_utf16to32 (choose : List Nat → Iter)
    (hc : ∀ xs, need 8 0 false ≤ xs.length → choose xs ∈ utf16to32) :
    ∀ n xs, written (need 8 0 false) choose n xs ≤ L xs :=
  safe_of_exact _ utf16to32 exact_utf16to32 choose hc

/-! ## UTF-8 sources (dynamic K)

src/westmere/sse_convert_utf8_to_utf16.cpp, sse_convert_utf8_to_utf32.cpp and
sse_convert_utf8_to_latin1.cpp are the `convert_masked_*` callees of
src/generic/utf8_to_utf16/utf8_to_utf16.h (:178 and :275),
src/generic/utf8_to_utf32/utf8_to_utf32.h (:175 and :269),
src/generic/utf8_to_latin1/utf8_to_latin1.h (:161 and :259) and
src/generic/utf8_to_latin1/valid_utf8_to_latin1.h (:18), all with `K = 3`.
The store paths are `Utf8.x86_utf16`, `x86_utf32`, `x86_latin1`; the
0xaaaa paths (utf16 :42-57, utf32 :37-53) and utf32 `0x924` and `idx < 145`
store exactly what they produce and are omitted, as in `Utf8`. `K = 3` is
optimal for all three (`Utf8.K_x86_utf16`, `K_x86_utf32`, `K_x86_latin1`). -/

end SafetyMargins.Arch.Westmere
