import SafetyMargins.Kernels
import SafetyMargins.Utf8
/-
LoongArch LSX: every transcoding loop of src/lsx/*.cpp and of the generic
headers as instantiated for lsx (base64 excluded). Line numbers refer to commit
6b733169.
-/
namespace SafetyMargins.Arch.Lsx
open SafetyMargins SafetyMargins.Kernels SafetyMargins.Utf8

/-! ## Exact-store loops: latin1 sources, UTF-16 to Latin-1, UTF-32 to UTF-16 -/

/-- src/lsx/lsx_convert_latin1_to_utf16.cpp:7 (LE) and :27 (BE), loop
`end - buf >= 16`: two stores of 8 `char16_t`, advance 16. -/
def latin1to16 : List Iter := [{ consumed := 16, stores := [.rel 0 8, .rel 8 8] }]

theorem m_latin1to16 : optimalMargin 16 false latin1to16 = 0 := by decide
theorem cur_latin1to16 : latin1to16.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/lsx/lsx_convert_latin1_to_utf32.cpp:6, loop `end - buf >= 16`: four
stores of 4 `char32_t`, advance 16. -/
def latin1to32 : List Iter :=
  [{ consumed := 16, stores := [.rel 0 4, .rel 4 4, .rel 8 4, .rel 12 4] }]

theorem m_latin1to32 : optimalMargin 16 false latin1to32 = 0 := by decide
theorem cur_latin1to32 : latin1to32.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/lsx/lsx_convert_latin1_to_utf8.cpp:15, loop `end - latin1_input >= 16`:
the existing `Kernels.latin1to8_one` (ASCII: 16 stored, 16 consumed; otherwise
16 bytes stored for 8 consumed, advance `row[0]`). -/
theorem m_latin1to8 : optimalMargin 16 false latin1to8_one = 0 := by decide
theorem cur_latin1to8 : latin1to8_one.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/lsx/lsx_convert_utf16_to_latin1.cpp:6 (plain) and :34 (`_with_errors`),
loop `end - buf >= 16`: 16 bytes stored for 16 units; otherwise return
(plain) or a scalar loop over at most 16 units. -/
def utf16toLatin1 : List Iter := [{ consumed := 16, stores := [.rel 0 16] }] ++ scalar 16

theorem m_utf16toLatin1 : optimalMargin 16 false utf16toLatin1 = 0 := by decide
theorem cur_utf16toLatin1 : utf16toLatin1.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/lsx/lsx_convert_utf32_to_utf16.cpp:11 (plain) and :88 (`_with_errors`),
loop `end - buf >= 8`: BMP path stores 8 `char16_t` for 8 units; otherwise a
scalar loop over at most 3 units (`forward = 3`). -/
def utf32to16 : List Iter := [{ consumed := 8, stores := [.rel 0 8] }] ++ scalar 3

theorem m_utf32to16 : optimalMargin 8 false utf32to16 = 0 := by decide
theorem cur_utf32to16 : utf32to16.all (Iter.ok (need 8 0 false)) = true := by decide

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`: one
`simd16x32` store of 32 units per 32 units. -/
def changeEndianness : List Iter := [{ consumed := 32, stores := [.rel 0 32] }]

theorem m_changeEndianness : optimalMargin 32 false changeEndianness = 0 := by decide
theorem cur_changeEndianness : changeEndianness.all (Iter.ok (need 32 0 false)) = true := by decide

/-- src/generic/utf16/to_well_formed.h:71 and :78, `i + N < n` with `N = 8`
(strict): one 8-unit store at `out + i`. The final block at `out + n - 8` and
the `out[-1]` fix-up lie inside `[0, n)` by the guard `n ≥ 9`. -/
def toWellFormed : List Iter := [{ consumed := 8, stores := [.rel 0 8] }]

theorem m_toWellFormed : optimalMargin 8 true toWellFormed = 0 := by decide
theorem cur_toWellFormed : toWellFormed.all (Iter.ok (need 8 0 true)) = true := by decide

/-! ## UTF-32 to Latin-1: margin 8 hidden in the loop bound -/

/-- src/lsx/lsx_convert_utf32_to_latin1.cpp:8 (plain) and :37 (`_with_errors`),
loop `end - buf >= 16` but each iteration consumes 8 units: the store is a full
16-byte `__lsx_vst` at the output cursor. So
`B = 8`, `m = 8`. Otherwise return (plain) or a scalar loop over at most 8. -/
def utf32toLatin1 : List Iter := [{ consumed := 8, stores := [.rel 0 16] }] ++ scalar 8

theorem m_utf32toLatin1 : optimalMargin 8 false utf32toLatin1 = 8 := by decide
theorem cur_utf32toLatin1 : utf32toLatin1.all (Iter.ok (need 8 8 false)) = true := by decide

/-- With `>= 15` (m = 7): 15 Latin-1 code points (L = 15), the store ends at 16. -/
theorem wit_utf32toLatin1 :
    let it : Iter := { consumed := 8, stores := [.rel 0 16] }
    let xs := ascii 15
    it ∈ utf32toLatin1 ∧ xs.length = need 8 7 false ∧
    written (need 8 7 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-16 to UTF-8 -/

/-- src/lsx/lsx_convert_utf16_to_utf8.cpp:64 (plain) and :299 (`_with_errors`),
loop `end - buf >= 16 + 12`: the existing `Kernels.utf16to8_lsx` (16-byte ASCII
store, then `in = nextin` fall-through into the 1/2-byte, 1/2/3-byte or scalar
path, `forward = 15`). -/
theorem m_utf16to8 : optimalMargin 16 false utf16to8_lsx = 12 := by decide
theorem cur_utf16to8 : utf16to8_lsx.all (Iter.ok (need 16 12 false)) = true := by decide

/-- 8 ASCII, 4 x U+0800, 4 ASCII, 11 ASCII (27 units, L = 35): ASCII half,
fall-through, second 1/2/3 store at input offset 12 ends at 36. -/
theorem wit_utf16to8 :
    let it := afterAscii8 16 { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := ascii 8 ++ [3, 3, 3, 3] ++ ascii 4 ++ ascii 11
    it ∈ utf16to8_lsx ∧ xs.length = need 16 11 false ∧
    written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-8 -/

/-- src/lsx/lsx_convert_utf32_to_utf8.cpp:16 (plain) and :245 (`_with_errors`),
loop `end - buf > 16 + 3`: the existing `Kernels.utf32to8_lsx` (8 units per
iteration, ASCII path `continue`s; scalar `forward = 15`, a subset of
`scalar 16`). -/
theorem m_utf32to8 : optimalMargin 16 true utf32to8_lsx = 3 := by decide
theorem ok_utf32to8 : utf32to8_lsx.all (Iter.ok (need 16 3 true)) = true := by decide

theorem safe_utf32to8 (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 3 true ≤ xs.length → choose xs ∈ utf32to8_lsx) :
    ∀ n xs, Pos xs → written (need 16 3 true) choose n xs ≤ L xs :=
  safe_of_paths _ utf32to8_lsx ok_utf32to8 choose hc

/-- m = 2, i.e. `> 18`: U+0800, 3 ASCII, 4 ASCII, 11 ASCII (19 units, L = 21);
the second 16-byte store at L(first 4) = 6 ends at 22. -/
theorem wit_utf32to8 :
    let it : Iter := { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := [3, 1, 1, 1] ++ ascii 4 ++ ascii 11
    it ∈ utf32to8_lsx ∧ xs.length = need 16 2 true ∧
    written (need 16 2 true) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-16 to UTF-32: length function with minimum 0

`utf32_length_from_utf16` gives 0 to a low surrogate, so `Pos` fails and
`safe_of_paths` does not apply. Both lsx loops
(src/lsx/lsx_convert_utf16_to_utf32.cpp:12 and :83, `end - buf >= 8`) only
store exactly what they produce: the vector path runs on 8 non-surrogate units
(L = 8) and stores 8 `char32_t`, recorded as `tail 0`; the scalar path writes one
unit per non-surrogate and one per (high, low) pair (L = 1 + 0), and returns
before writing on a lone low surrogate or a high surrogate not followed by a
low one. -/
def utf16to32 : List Iter := [{ consumed := 8, stores := [.tail 0] }] ++ scalar 16

theorem exact_utf16to32 : utf16to32.all Iter.exact = true := by decide
theorem m_utf16to32 : optimalMargin 8 false utf16to32 = 0 := by decide

/-- No `Pos` hypothesis: any input, including a tail of low surrogates. -/
theorem utf16to32_safe (choose : List Nat → Iter)
    (hc : ∀ xs, need 8 0 false ≤ xs.length → choose xs ∈ utf16to32) :
    ∀ n xs, written (need 8 0 false) choose n xs ≤ L xs :=
  safe_of_exact _ utf16to32 exact_utf16to32 choose hc

/-! ## UTF-8 sources (`convert_masked_utf8_to_*`, dynamic `K`)

Pairs `(stored, produced + 1)`; paths storing exactly what they produce are
listed with `f = s + 1` where kept, and need `K ≥ 0`. -/

/-- src/lsx/lsx_convert_utf8_to_utf16.cpp: `0x924` (:36, 8 stored, 4 produced),
`idx < 145` (:85, 8, 4), `0xAAAA` (:51, 8, 6), `idx < 64` (:73, 8, 6). The
ASCII path (:24, 16, 16), `0x888` (:133, `memcpy` 12 bytes = 6, after the lead
check, else `return 12` with no store) and `idx < 209` (:197, scalar, exact)
are exact; `else return 12` (:289) stores nothing. Same list as
`Utf8.la_utf16`. -/
def utf8to16 : List MPath := [⟨8, 5⟩, ⟨8, 5⟩, ⟨8, 7⟩, ⟨8, 7⟩, ⟨16, 17⟩, ⟨6, 7⟩]

theorem K_utf8to16 : okK 3 utf8to16 = true ∧ okK 2 utf8to16 = false := by decide

/-- src/lsx/lsx_convert_utf8_to_utf32.cpp: `0xaaa` (:44, two stores = 8, 6
produced), `idx < 64` (:64, 8, 6), `0x888` (:110, 4 stored, 3 produced, no lead
check), `idx < 209` (:144, 4, 3). Exact: ASCII (:23, 16), `0x924` (:33, 4),
`idx < 145` (:75, 4). `Utf8.la_utf32` has the same pairs (one `(4, 4)` once). -/
def utf8to32 : List MPath := [⟨8, 7⟩, ⟨8, 7⟩, ⟨4, 4⟩, ⟨4, 4⟩, ⟨16, 17⟩, ⟨4, 5⟩]

theorem K_utf8to32 : okK 1 utf8to32 = true ∧ okK 0 utf8to32 = false := by decide
theorem curK_utf8to32 : okK 3 utf8to32 = true := by decide

/-- src/lsx/lsx_convert_utf8_to_latin1.cpp: ASCII (:19, 16 stored, 16
produced) and `idx < 64` (`memcpy` 6 bytes, 6 produced); `idx >= 64` stores
nothing. Every store is exact: `K = 0` for the writes. -/
def utf8toLatin1 : List MPath := [⟨16, 17⟩, ⟨6, 7⟩]

theorem K_utf8toLatin1 : okK 0 utf8toLatin1 = true := by decide
theorem curK_utf8toLatin1 : okK 3 utf8toLatin1 = true := by decide

/-! ## `convert_valid` on valid input

src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:12, fixed margin 12. On
`0xaaa` and `idx < 64` the lsx kernel stores 8 units and produces 6, a deficit
of 2, so the 1 + m bytes after the call must hold 2 code points: m = 4
(against 12 elsewhere, `Utf8.valid_utf32`). UTF-16 keeps deficit 4
(`Utf8.valid_utf16`, margin 9). -/

theorem valid_utf32_lsx (ks : List Nat) (h : Valid ks) (m : Nat) (hm : 4 ≤ m)
    (hb : 1 + m ≤ bytes ks) : 2 ≤ ks.length := by
  have := bytes_le_4u32 ks h
  omega

theorem valid_utf32_lsx_tight : bytes [4] = 1 + 3 ∧ [4].length = 1 := by decide

/-- Store form of `valid_utf32_lsx`. Deficit 2 is this kernel. The shared
header `valid_utf8_to_utf32.h` is `Utf8.valid_utf32_store` (deficit 4, `m ≥ 12`). -/
theorem valid_utf32_lsx_store (pre : List Nat) (ks : List Nat) (s m : Nat)
    (h : Valid ks) (hm : 4 ≤ m) (hb : 1 + m ≤ bytes ks)
    (hs : s ≤ L pre + 2) : s ≤ L (pre ++ expand32 ks) := by
  have hden := valid_utf32_lsx ks h m hm hb
  exact overshoot_le pre (expand32 ks) s 2 hs (by rw [L_expand32 ks h]; exact hden)

end SafetyMargins.Arch.Lsx
