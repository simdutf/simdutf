import SafetyMargins.Kernels
import SafetyMargins.Utf8
/-
arm64: every transcoding loop of src/arm64/*.cpp and of the src/generic
headers instantiated by src/arm64/implementation.cpp (base64 excluded).
`m_*` optimal margin, `cur_*` the source margin sufficient, `ok_*` optimal
margin sufficient (the source margin when it was lowered), `wit_*` one less
overflows.
-/
namespace SafetyMargins.Arch.Arm64
open SafetyMargins SafetyMargins.Kernels SafetyMargins.Utf8

/-! ## Latin-1 to UTF-16 / UTF-32 (exact stores, weight 1 per unit) -/

/-- src/arm64/arm_convert_latin1_to_utf16.cpp:7, `while (end - buf >= 16)`:
two stores of 8 `char16_t`, one per 8 input bytes. -/
def latin1to16 : List Iter := [{ consumed := 16, stores := [.rel 0 8, .rel 8 8] }]

/-- src/arm64/arm_convert_latin1_to_utf32.cpp:6, `while (end - buf >= 16)`:
four stores of 4 `char32_t`. -/
def latin1to32 : List Iter :=
  [{ consumed := 16, stores := [.rel 0 4, .rel 4 4, .rel 8 4, .rel 12 4] }]

theorem m_latin1to16 : optimalMargin 16 false latin1to16 = 0 := by decide
theorem cur_latin1to16 : latin1to16.all (Iter.ok (need 16 0 false)) = true := by decide
theorem m_latin1to32 : optimalMargin 16 false latin1to32 = 0 := by decide
theorem cur_latin1to32 : latin1to32.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## Latin-1 to UTF-8 -/

/-- src/arm64/arm_convert_latin1_to_utf8.cpp:13, `while (end - in >= 16)`:
ASCII path stores 16 for 16; otherwise only the low 8 bytes are converted and
one 16-byte store is done (`Kernels.latin1to8_one`). -/
def latin1to8 : List Iter := latin1to8_one

theorem m_latin1to8 : optimalMargin 16 false latin1to8 = 0 := by decide
theorem ok_latin1to8 : latin1to8.all (Iter.ok (need 16 0 false)) = true := by decide

theorem safe_latin1to8 (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 0 false ≤ xs.length → choose xs ∈ latin1to8) :
    ∀ n xs, Pos xs → written (need 16 0 false) choose n xs ≤ L xs :=
  safe_of_paths _ latin1to8 ok_latin1to8 choose hc

/-! ## UTF-16 / UTF-32 to Latin-1 (exact 8-byte stores) -/

/-- src/arm64/arm_convert_utf16_to_latin1.cpp:7 and :33,
src/arm64/arm_convert_utf32_to_latin1.cpp:5 and :31, `while (end - buf >= 8)`:
8 bytes stored for 8 units; otherwise return (plain) or a scalar loop over the
8 units that writes one byte per unit and returns at the first unit > 0xFF
(`_with_errors`). -/
def toLatin1 : List Iter := [{ consumed := 8, stores := [.rel 0 8] }] ++ scalar 8

theorem m_toLatin1 : optimalMargin 8 false toLatin1 = 0 := by decide
theorem cur_toLatin1 : toLatin1.all (Iter.ok (need 8 0 false)) = true := by decide

/-! ## UTF-16 to UTF-8 -/

/-- src/arm64/arm_convert_utf16_to_utf8.cpp:65 (plain) and :334
(`_with_errors`), `while (end - buf >= 16 + 12)`. ASCII test on 16 units; if
only the first 8 are ASCII, an 8-byte store and `in = nextin` fall-through
into the 1/2-byte path (one 16-byte store), the 1/2/3-byte path (16-byte
stores at `L` of 0 and 4 units), or the scalar fallback (at most 16 units).
Same as `Kernels.utf16to8_arm64`. Also used, unchanged, by
`convert_utf16*_to_utf8_with_replacement` (implementation.cpp:1183). -/
def utf16to8 : List Iter := utf16to8_arm64

theorem m_utf16to8 : optimalMargin 16 false utf16to8 = 12 := by decide
theorem cur_utf16to8 : utf16to8.all (Iter.ok (need 16 12 false)) = true := by decide

/-- 8 ASCII, 4 x U+0800, 4 ASCII, 11 ASCII: 27 units, `L = 35`. The
fall-through 1/2/3-byte path stores 16 bytes at `L(12 units) = 20`, ending at 36. -/
theorem wit_utf16to8 :
    let it := afterAscii8 8 { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := ascii 8 ++ [3, 3, 3, 3] ++ ascii 4 ++ ascii 11
    it ∈ utf16to8 ∧ xs.length = need 16 11 false ∧
      written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-8 -/

/-- src/arm64/arm_convert_utf32_to_utf8.cpp:13 (plain) and :264
(`_with_errors`), `while (buf + 16 + 3 < end)` (strict): 8 units per
iteration, ASCII 8-byte store, 1/2-byte path (one 16-byte store), 1/2/3-byte
path (16-byte stores at 0 and `L(4 units)`), scalar fallback (at most 15
units). Same as `Kernels.utf32to8_arm64`. -/
def utf32to8 : List Iter := utf32to8_arm64

theorem m_utf32to8 : optimalMargin 16 true utf32to8 = 3 := by decide
theorem ok_utf32to8 : utf32to8.all (Iter.ok (need 16 3 true)) = true := by decide

theorem safe_utf32to8 (choose : List Nat → Iter)
    (hc : ∀ xs, need 16 3 true ≤ xs.length → choose xs ∈ utf32to8) :
    ∀ n xs, Pos xs → written (need 16 3 true) choose n xs ≤ L xs :=
  safe_of_paths _ utf32to8 ok_utf32to8 choose hc

/-- U+0800, 3 ASCII, 4 ASCII, 11 ASCII: 19 units, `L = 21`; the second store
ends at 22. -/
theorem wit_utf32to8 :
    let it : Iter := { consumed := 8, stores := [.rel 0 16, .rel 4 16] }
    let xs := [3, 1, 1, 1] ++ ascii 4 ++ ascii 11
    it ∈ utf32to8 ∧ xs.length = need 16 2 true ∧
      written (need 16 2 true) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-16 -/

/-- src/arm64/arm_convert_utf32_to_utf16.cpp:77 (plain) and :132
(`_with_errors`), `while (end - buf >= 8 + 4)`: BMP path stores 8 for 8; the
supplementary path stores 8 `char16_t` per group of 4 units, at
`u16count = 4 + popcount` = `L` of the first 4. The `_with_errors` error path
writes the valid prefix scalarly and returns. Same as `Kernels.utf32to16_arm64`. -/
def utf32to16 : List Iter := utf32to16_arm64

theorem m_utf32to16 : optimalMargin 8 false utf32to16 = 4 := by decide
theorem cur_utf32to16 : utf32to16.all (Iter.ok (need 8 4 false)) = true := by decide

/-- U+10000 then 10 ASCII: 11 units, `L = 12`; the second store ends at 13. -/
theorem wit_utf32to16 :
    let it : Iter := { consumed := 8, stores := [.rel 0 8, .rel 4 8] }
    let xs := [2] ++ ascii 10
    it ∈ utf32to16 ∧ xs.length = need 8 3 false ∧
      written (need 8 3 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-16 utilities that write as many units as they read -/

/-- src/generic/utf16/change_endianness.h:10, `while (pos < size / 32 * 32)`,
i.e. at least 32 units remain: one 32-unit store. -/
def changeEndianness : List Iter := [{ consumed := 32, stores := [.rel 0 32] }]

theorem m_changeEndianness : optimalMargin 32 false changeEndianness = 0 := by decide
theorem cur_changeEndianness :
    changeEndianness.all (Iter.ok (need 32 0 false)) = true := by decide

/-- src/arm64/arm_utf16fix.cpp:156/167, `for (i = 1; i + 64 < n; i += 64)`
(strict): 64 units stored at `out + i`. A replacement may also be written at
`out[i - 1]`, which is before the store and inside the buffer since `i ≥ 1`. -/
def utf16fix64 : List Iter := [{ consumed := 64, stores := [.rel 0 64] }]
/-- src/arm64/arm_utf16fix.cpp:160/170, `for (; i + 16 < n; i += 16)`
(strict): 16 units at `out + i` (and possibly `out[i - 1]`). The final call
at `n - 16` (:165/:174) ends exactly at `n` and needs `n ≥ 17`, which is the
guard at :147. -/
def utf16fix16 : List Iter := [{ consumed := 16, stores := [.rel 0 16] }]

theorem m_utf16fix64 : optimalMargin 64 true utf16fix64 = 0 := by decide
theorem cur_utf16fix64 : utf16fix64.all (Iter.ok (need 64 0 true)) = true := by decide
theorem m_utf16fix16 : optimalMargin 16 true utf16fix16 = 0 := by decide
theorem cur_utf16fix16 : utf16fix16.all (Iter.ok (need 16 0 true)) = true := by decide

/-! ## UTF-16 to UTF-32: a length function with minimum 0

`utf32_length_from_utf16` gives 0 to a low surrogate, so `Pos` fails and no
margin helps. The kernel is safe because every store ends at or before `L` of
what the iteration consumed (`safe_exact`). -/

/-- The vector path of src/arm64/arm_convert_utf16_to_utf32.cpp:63 and :133
(`while (end - buf >= 8)`): taken only when none of the 8 units is a
surrogate, so each weighs 1; it stores 8 `char32_t`. -/
def utf16to32Vec : Iter := { consumed := 8, stores := [.rel 0 8] }

/-- Vector path or scalar fallback (at most 16 units: `forward = 15`, plus the
low half of a pair). The scalar loop writes one unit per non-surrogate and one
per valid pair (weights 1 + 0), and returns on any unpaired surrogate. -/
def utf16to32 : List Iter := [utf16to32Vec] ++ scalar 16

theorem exact_vec (xs : List Nat) (h : xs.take 8 = List.replicate 8 1) :
    exactOn xs utf16to32Vec := by
  intro s hs
  simp only [utf16to32Vec, List.mem_singleton] at hs
  subst hs
  show L (xs.take 0) + 8 ≤ L (xs.take 8)
  rw [h, List.take_zero]
  decide

theorem scalar16_no_store : ∀ it ∈ scalar 16, it.stores = [] := by decide

/-- The UTF-16 to UTF-32 kernel, with any path selection that takes the vector
path only on 8 non-surrogate units, never writes past `L`, for every input,
including inputs whose tail has weight 0. -/
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
arm64 `convert_masked_utf8_to_*`. Pairs are `(stored, produced + 1)`, see
`Utf8.MPath`; exact paths are omitted. -/

/-- src/arm64/arm_convert_utf8_to_utf16.cpp: `0xaaa` (:49, 8 stored, 6
produced), `idx < 64` (:72, 8, 6), `0x888` (:133, 6 stored, then 6 or 0
produced: 3 leading bytes consumed + the next). ASCII (:24, 16/16), `0x924`
(:35, 4/4), `idx < 145` (:85, 4/4) and the 3-code-point scalar path (:294,
writes 2 units only for a 0xF0..0xF7 lead, which also weighs 2) are exact.
These pairs are `Utf8.arm_utf16`. -/
example : Utf8.arm_utf16 = [⟨8, 7⟩, ⟨8, 7⟩, ⟨6, 4⟩] := rfl

/-- src/arm64/arm_convert_utf8_to_utf32.cpp: `0xfff` (:23, 16 stored by
`store_ascii_as_utf32_tbl`, 12 produced), `0xaaa` (:44, 8, 6), `idx < 64`
(:61, 8, 6), `0x888` (:103, 4, 3), `idx < 209` (:138, 4, 3). `0x924` and
`idx < 145` are exact. Same pairs as `Utf8.x86_utf32`. -/
def arm_utf32 : List MPath := [⟨16, 13⟩, ⟨8, 7⟩, ⟨8, 7⟩, ⟨4, 4⟩, ⟨4, 4⟩]

/-- src/arm64/arm_convert_utf8_to_latin1.cpp: `0xfff` (:23, 16 stored, 12
produced), `idx < 64` (:61, 8 stored, 6 produced). Same as `Utf8.x86_latin1`. -/
def arm_latin1 : List MPath := [⟨16, 13⟩, ⟨8, 7⟩]

theorem K_arm_utf32 : okK 3 arm_utf32 = true ∧ okK 2 arm_utf32 = false := by decide
theorem K_arm_latin1 : okK 3 arm_latin1 = true ∧ okK 2 arm_latin1 = false := by decide

theorem curK_arm_utf16 : okK 3 Utf8.arm_utf16 = true := by decide

/-- Tightness of `K = 2` for UTF-16, `K = 1`: 48 ASCII, then the `0x888`
call at byte 48 (`a`, 3 continuations, `b`, 3, `c`, 3, `d`) that stores six
units and fails its lead-byte check, then 4 continuation bytes and one ASCII
byte. 66 bytes, `L = 53`, store end 54. -/
theorem arm_888_tight :
    let xs := List.replicate 48 1 ++ [1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1] ++
      [0, 0, 0, 0, 1]
    tk 1 xs = 1 ∧ 0 + 64 + (tk 1 xs + 1) ≤ xs.length ∧
      L (xs.take 48) + 6 = L xs + 1 := by decide

/-- Tightness of `K = 3` for UTF-32 and Latin-1, `K = 2`: 61 ASCII, 4
continuation bytes, 2 ASCII (67 bytes, `L = 63`). The call at byte 48 sees
the full mask `0xfff` (bytes 49..60 leading, 61..63 not), stores 16 units at
48. -/
theorem arm_fff_tight :
    let xs := List.replicate 61 1 ++ [0, 0, 0, 0] ++ [1, 1]
    tk 2 xs = 2 ∧ 0 + 64 + (tk 2 xs + 1) ≤ xs.length ∧
      L (xs.take 48) + 16 = L xs + 1 := by decide

/-! ## `convert_valid` with a fixed margin (valid input only)

src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:15 (9) and
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:12 (12). On valid input the arm64
UTF-16 paths overshoot by at most 2 (`0xaaa`, `idx < 64`; `0x888` is exact
on valid input), the UTF-32 paths by at most 4 (`0xfff`), which needs 12
(`Utf8.valid_utf32`, `Utf8.valid_utf32_tight`). -/

theorem valid_utf16_arm (ks : List Nat) (h : Valid ks) (m : Nat) (hm : 3 ≤ m)
    (hb : 1 + m ≤ bytes ks) : 2 ≤ u16 ks := by
  have := bytes_le_3u16 ks h
  omega

/-- and 2 does not: one 3-byte character is 3 bytes and 1 unit. -/
theorem valid_utf16_arm_tight : bytes [3] = 1 + 2 ∧ u16 [3] = 1 := by decide

/-- Store form of `valid_utf16_arm`: overshoot at most 2, tail of `1 + m`
bytes of complete characters with `m ≥ 3`. The shared header keeps margin 9
(`Utf8.valid_utf16_store`) because x86 overshoots by 4. -/
theorem valid_utf16_arm_store (pre : List Nat) (ks : List Nat) (s m : Nat)
    (h : Valid ks) (hm : 3 ≤ m) (hb : 1 + m ≤ bytes ks)
    (hs : s ≤ L pre + 2) : s ≤ L (pre ++ expand16 ks) := by
  have hden := valid_utf16_arm ks h m hm hb
  exact overshoot_le pre (expand16 ks) s 2 hs (by rw [L_expand16 ks h]; exact hden)

end SafetyMargins.Arch.Arm64
