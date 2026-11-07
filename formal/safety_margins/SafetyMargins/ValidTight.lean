import SafetyMargins.ValidExact
import SafetyMargins.ValidLatin1
/-
A witness on valid input for the `convert_valid` UTF-8 → UTF-32 margin of 12
on x86, arm64 and POWER (`valid_utf8_to_utf32.h`), through the loop model of
`ValidExact`: with margin 11 the loop runs a block whose last call stores 16
units and ends one unit past `L`.
-/
namespace SafetyMargins.ValidTight
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.Exact SafetyMargins.ValidExact
  SafetyMargins.ValidLatin1

/-- The first 63 bytes of `ValidLatin1.lvTight` (24 ASCII bytes, seven `é`,
5 ASCII, two `é`, 16 ASCII), then three 4-byte characters (U+1F600,
`F0 9F 98 80`): 75 bytes of valid UTF-8. -/
def v32Tight : List Nat :=
  lvTight.take 63 ++ (List.replicate 3 [0xF0, 0x9F, 0x98, 0x80]).flatten

/-- **`valid_utf8_to_utf32.h` on x86, arm64 and POWER: margin 11 overflows on
valid input.** With margin 11 the loop test `pos + 64 + margin <= size`
admits the block at 0; with 12 it does not. The kernels
(src/arm64/arm_convert_utf8_to_utf32.cpp, src/ppc64/ppc64_convert_utf8_to_utf32.cpp,
src/westmere/sse_convert_utf8_to_utf32.cpp, src/haswell/avx2_convert_utf8_to_utf32.cpp)
convert six ASCII bytes per table row at 0, 6, 12 and 18 (masks `0xFFF`,
`0xABF`), then the rows of masks `0xAAA` at 24 (12 bytes; arm64 takes its
`0xaaa` fast path, which also consumes 12), `0xD7E` at 36 (7 bytes) and
`0xFFA` at 43 (8 bytes), 6 units each (`TableFacts.lv_rows`). The calls are
exact (`XStep`) and reach byte 51 with 42 units out. At 51 the whole mask is
`0xfff` (its 12 bits are the last of the block): the kernel stores 16 units
for 12 (`store_ascii_as_utf32`), and the store ends at 58, one unit past
`L = 57`: the three 4-byte characters after the block give only 3 units for
the 4 of overshoot. -/
theorem valid_utf32_tight :
    let bs := v32Tight
    let xs := bs.map w01
    Valid8 bs ∧ bs.length = 75 ∧
      emask xs 0 12 = 0xFFF ∧ emask xs 6 12 = 0xFFF ∧ emask xs 12 12 = 0xFFF ∧
      emask xs 18 12 = 0xABF ∧ emask xs 24 12 = 0xAAA ∧ emask xs 36 12 = 0xD7E ∧
      emask xs 43 12 = 0xFFA ∧ emask xs 51 12 = 0xFFF ∧ 51 + 12 = 63 ∧
      VX w01 bs true 0 51 42 ∧
      0 + 64 + 11 ≤ bs.length ∧ ¬ (0 + 64 + 12 ≤ bs.length) ∧
      XStep xs 51 12 12 ∧ 42 + 16 = L xs + 1 := by
  intro bs xs
  refine ⟨by unfold Valid8; decide +kernel, by decide, by decide, by decide, by decide, by decide,
    by decide, by decide, by decide, by decide, rfl, ?_, by decide, by decide,
    by unfold XStep; decide, by decide⟩
  have h1 : VX w01 bs true 0 0 0 := VX.enter 0 0 VX.start (by decide)
  have h2 : VX w01 bs true 0 6 6 := VX.call 0 0 0 6 6 h1 (by decide) (by unfold XStep; decide)
  have h3 : VX w01 bs true 0 12 12 := VX.call 0 6 6 6 6 h2 (by decide) (by unfold XStep; decide)
  have h4 : VX w01 bs true 0 18 18 := VX.call 0 12 12 6 6 h3 (by decide) (by unfold XStep; decide)
  have h5 : VX w01 bs true 0 24 24 := VX.call 0 18 18 6 6 h4 (by decide) (by unfold XStep; decide)
  have h6 : VX w01 bs true 0 36 30 := VX.call 0 24 24 12 6 h5 (by decide) (by unfold XStep; decide)
  have h7 : VX w01 bs true 0 43 36 := VX.call 0 36 30 7 6 h6 (by decide) (by unfold XStep; decide)
  exact VX.call 0 43 36 8 6 h7 (by decide) (by unfold XStep; decide)

end SafetyMargins.ValidTight
