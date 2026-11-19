import SafetyMargins.Basic
/-
Path tables shared by several architectures. Store offsets are in input units,
widths in output units. They are transcribed from the C++ sources cited next to
each table; `Arch/*.lean` states the margins.
-/
namespace SafetyMargins.Kernels
open SafetyMargins

/-- Scalar fallback: writes one unit at a time, so it has no vector store; it
consumes between 1 and `k` units, or returns on an error after converting at
most `k` units. -/
def scalar (k : Nat) : List Iter :=
  ((List.range k).map fun c => ({ consumed := c + 1, stores := [] } : Iter))
    ++ (List.range (k + 1)).map fun c => ({ consumed := c, stores := [], stop := true } : Iter)

/-- Shift a path by an ASCII half block of 8 units that was handled first in the
same iteration (the `in = nextin` fall-through), whose own store has width `w`. -/
def afterAscii8 (w : Nat) (it : Iter) : Iter :=
  { consumed := 8 + it.consumed
    stores := .rel 0 w :: it.stores.map fun
      | .rel a v => .rel (8 + a) v
      | .tail o => .tail o
      | .ahead => .ahead
    stop := it.stop }

/-! ## UTF-16 to UTF-8 and UTF-32 to UTF-8, 8 units per register -/

/-- One 8-unit register (westmere, arm64, lsx; utf16 and utf32 sources):
* 1/2-byte path, `write_v_u16_11bits_to_utf8`: one 16-byte store;
* 1/2/3-byte path: a 16-byte store per group of 4 units, the second one at
  `row0[0]` = L(first 4 units) (`pack_1_2_3_utf8_bytes`, see `TableFacts`);
  the `mask == 0` path (all 3-byte) stores at +12 = L(first 4 units) as well;
* surrogates or invalid input: scalar fallback, at most 16 units. -/
def reg8 : List Iter :=
  [ { consumed := 8, stores := [.rel 0 16] },
    { consumed := 8, stores := [.rel 0 16, .rel 4 16] } ] ++ scalar 16

/-- ppc64: same, but its `mask == 0` path stores 16 bytes at +0 and +16 and
advances 24, so the second store ends 8 bytes past the 8-unit block
(src/ppc64/ppc64_convert_utf16_to_utf8.cpp). -/
def reg8ppc : List Iter :=
  reg8 ++ [{ consumed := 8, stores := [.rel 0 16, .tail 8] }]

/-- Kernels whose ASCII test covers 16 units and that fall through to the
second register when only the first 8 units are ASCII:
src/westmere/sse_convert_utf16_to_utf8.cpp (both functions),
src/arm64/arm_convert_utf16_to_utf8.cpp (both, 8-byte ASCII store),
src/lsx/lsx_convert_utf16_to_utf8.cpp (both),
src/westmere/sse_convert_utf32_to_utf8.cpp (plain function only). -/
def fallthrough8 (asciiStore : Nat) (r : List Iter) : List Iter :=
  [{ consumed := 16, stores := [.rel 0 16] }] ++ r ++ r.map (afterAscii8 asciiStore)

def utf16to8_westmere := fallthrough8 16 reg8
def utf16to8_arm64 := fallthrough8 8 reg8
def utf16to8_lsx := fallthrough8 16 reg8
def utf16to8_ppc64 := fallthrough8 16 reg8ppc
def utf32to8_westmere := fallthrough8 16 reg8
def utf32to8_ppc64 := fallthrough8 16 reg8ppc

/-- src/westmere/sse_convert_utf32_to_utf8.cpp, `_with_errors`: the ASCII
path stores 16 bytes for 8 units and `continue`s; no fall-through. -/
def utf32to8_westmere_err : List Iter :=
  [{ consumed := 8, stores := [.rel 0 16] }] ++ reg8

/-- src/arm64/arm_convert_utf32_to_utf8.cpp (8-byte ASCII store) and
src/lsx/lsx_convert_utf32_to_utf8.cpp (16-byte ASCII store), both functions:
8 units per iteration, no fall-through, loop `end - buf > 16 + m`. -/
def utf32to8_arm64 : List Iter := [{ consumed := 8, stores := [.rel 0 8] }] ++ reg8
def utf32to8_lsx : List Iter := [{ consumed := 8, stores := [.rel 0 16] }] ++ reg8

/-! ## UTF-16/UTF-32 to UTF-8, 16 units per iteration -/

/-- src/haswell/avx2_convert_utf16_to_utf8.cpp, src/lasx/lasx_convert_utf16_to_utf8.cpp,
src/haswell/avx2_convert_utf32_to_utf8.cpp, src/icelake/icelake_convert_utf32_to_utf8.inl.cpp,
src/lasx/lasx_convert_utf32_to_utf8.cpp: ASCII path, 1/2-byte path (two 16-byte
stores, one per 8 units), 1/2/3-byte path (four 16-byte stores, one per 4
units), scalar fallback. -/
def wide16 : List Iter :=
  [ { consumed := 16, stores := [.rel 0 16] },
    { consumed := 16, stores := [.rel 0 16, .rel 8 16] },
    { consumed := 16, stores := [.rel 0 16, .rel 4 16, .rel 8 16, .rel 12 16] } ]
    ++ scalar 16

/-! ## UTF-32 to UTF-16 -/

/-- src/haswell/avx2_convert_utf32_to_utf16.cpp (both): BMP path stores exactly
8 units; anything else is scalar. -/
def utf32to16_haswell : List Iter := [{ consumed := 8, stores := [.rel 0 8] }] ++ scalar 7

/-- src/arm64/arm_convert_utf32_to_utf16.cpp (both): BMP path, and the
supplementary path storing 8 char16_t per group of 4 units, the second one at
`u16count` = L(first 4 units). -/
def utf32to16_arm64 : List Iter :=
  [ { consumed := 8, stores := [.rel 0 8] },
    { consumed := 8, stores := [.rel 0 8, .rel 4 8] } ] ++ scalar 8

/-- src/westmere/sse_convert_utf32_to_utf16.cpp (plain, loop `>= 16 + 8`):
BMP path (two stores of 8) and four stores of 8, one per group of 4 units. -/
def utf32to16_westmere : List Iter :=
  [ { consumed := 16, stores := [.rel 0 8, .rel 8 8] },
    { consumed := 16, stores := [.rel 0 8, .rel 4 8, .rel 8 8, .rel 12 8] } ]
    ++ scalar 16

/-! ## Latin-1 to UTF-8 -/

/-- src/haswell/avx2_convert_latin1_to_utf8.cpp, src/lasx/lasx_convert_latin1_to_utf8.cpp,
src/westmere/sse_convert_latin1_to_utf8.cpp, src/ppc64/ppc64_convert_latin1_to_utf8.cpp:
16 bytes per iteration, one 16-byte store per 8 input bytes, the second one at
`row[0]` = L(first 8 bytes) (`pack_1_2_utf8_bytes`). -/
def latin1to8_two : List Iter :=
  [ { consumed := 16, stores := [.rel 0 16] },
    { consumed := 16, stores := [.rel 0 16, .rel 8 16] } ]

/-- src/arm64/arm_convert_latin1_to_utf8.cpp, src/lsx/lsx_convert_latin1_to_utf8.cpp:
the non-ASCII path converts only 8 bytes and does one 16-byte store. -/
def latin1to8_one : List Iter :=
  [ { consumed := 16, stores := [.rel 0 16] },
    { consumed := 8, stores := [.rel 0 16] } ]

/-- Weight list of `k` ASCII units. -/
def ascii (k : Nat) : List Nat := List.replicate k 1

end SafetyMargins.Kernels
