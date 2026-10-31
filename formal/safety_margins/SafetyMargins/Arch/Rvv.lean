import SafetyMargins.Kernels
import SafetyMargins.Utf8
/-
rvv: every transcoding loop of src/rvv/*.cpp (base64 excluded). Every loop is
`for (size_t vl; len > 0; len -= vl, ...) { vl = __riscv_vsetvl_*(len); ... }`:
an iteration consumes `vl ≤ len` units and stores exactly what it produced
(`vl`, or `vcpop` of the `vcompress` mask). Every store is `tail 0`, so no
margin is needed for any VLEN and any length function (`safe_vla`).
-/
namespace SafetyMargins.Arch.Rvv
open SafetyMargins SafetyMargins.Kernels SafetyMargins.Utf8

/-- An iteration that consumes `c` units and stores exactly their output. -/
def exactIter (c : Nat) : Iter := { consumed := c, stores := [.tail 0] }

/-- One loop with `VLMAX = V` (in input units): an iteration consumes between 1
and `V` units (`vl = vsetvl(len)`, or fewer when a block is cut at a surrogate
or before a pair), or returns on an error without storing. -/
def vlaFamily (V : Nat) : List Iter :=
  (List.range V).map (fun c => exactIter (c + 1)) ++ [{ consumed := 0, stores := [], stop := true }]

theorem exact_vla (V : Nat) : (vlaFamily V).all Iter.exact = true := by
  simp [vlaFamily, exactIter, Iter.exact]

/-- Any rvv loop below, any `VLMAX`, any path selection: safe. `need 0 0 true`
is the loop test `len > 0`. -/
theorem safe_vla (V : Nat) (choose : List Nat → Iter)
    (hc : ∀ xs, 1 ≤ xs.length → choose xs ∈ vlaFamily V) :
    ∀ n xs, written (need 0 0 true) choose n xs ≤ L xs :=
  safe_of_exact _ _ (exact_vla V) choose hc

/-- Why the fixed model is not used: it would charge a `.tail 0` store of a
`V`-unit iteration `V - 1` units of margin, but `vl ≤ len` already provides
them. -/
theorem fixed_model_mismatch : optimalMargin 0 true (vlaFamily 16) = 15 := by decide

/-! ## The loops

Each name is `vlaFamily`; the doc comment records why every path is exact. -/

/-- src/rvv/rvv_latin1_to.inl.cpp:5, latin1 to UTF-8: `vlOut = vl + cnt`
(:11) with `cnt` = number of bytes ≥ 0x80, which is `utf8_length_from_latin1`
of the block; both the ASCII store (:13) and the compressed store (:28) use
`vlOut`. -/
def latin1to8 := vlaFamily
/-- src/rvv/rvv_latin1_to.inl.cpp:38, :49 (UTF-16LE/BE) and :64 (UTF-32):
`vl` units stored for `vl` bytes. -/
def latin1to16 := vlaFamily
def latin1to32 := vlaFamily
/-- src/rvv/rvv_utf16_to.inl.cpp:6 (`_with_errors`, error test before the
store, :10-12) and :51, :62 (`convert_valid`): `vl` bytes for `vl` units. -/
def utf16toLatin1 := vlaFamily
/-- src/rvv/rvv_utf32_to.inl.cpp:11: `vl` bytes for `vl` units, error test
before the store. -/
def utf32toLatin1 := vlaFamily
/-- src/rvv/rvv_utf16_to.inl.cpp:82, UTF-16 to UTF-8 (all variants, including
`_with_replacement` via the generic driver). ASCII: `vl` bytes (:89-95).
1/2-byte: the high byte of each lane is kept iff nonzero, the low byte always
(:118-125), so `vlOut` = 1 + [unit > 0x7F] per unit. 1/2/3-byte, on the prefix
before the first surrogate (:131-176): byte 2 of each 32-bit lane is always
kept (`m4mulp2`), byte 1 is `v2 | 0x80` on m234 lanes and 0 elsewhere, byte 0 is
`v3 | 0xE0` on m34 lanes and 0 elsewhere, byte 3 is 0, so `vlOut` is the UTF-8
length of the prefix. Then scalar pairs (4 bytes for weight 2 + 2), returning
before writing on a bad pair (:178-212). -/
def utf16to8 := vlaFamily
/-- src/rvv/rvv_utf32_to.inl.cpp:45, UTF-32 to UTF-8: same three vector paths
on the prefix before the first unit > 0xFFFF (:110-154), then scalar 4-byte
units, returning before writing on > 0x10FFFF (:156-169). Values > 0x10FFFF
and surrogates are rejected before any store in the validating version
(:94-108); `convert_valid` writes 3 bytes (weight 3) for a surrogate and
returns on > 0x10FFFF. -/
def utf32to8 := vlaFamily
/-- src/rvv/rvv_utf32_to.inl.cpp:205 and :277, UTF-32 to UTF-16:
BMP path `vl` units (:218-222, :282-286); otherwise
`rvv_utf32_store_utf16_m4` (src/rvv/rvv_helpers.inl.cpp:1-23) keeps the even
half of every lane and the odd half iff it is nonzero (only for a unit
> 0xFFFF), and stores `vcpop` of that mask: 1 or 2 per unit, as
`utf16_length_from_utf32`. -/
def utf32to16 := vlaFamily
/-- src/rvv/rvv_utf16_to.inl.cpp:271, UTF-16 to UTF-32 (weight 0 for a low
surrogate). No surrogate: `vl` units (:280-288). Otherwise the block is
shortened by one (:297-301), every high/low mismatch in it returns before the
store (:315-325), so the `vcpop` of "not a low surrogate" (:311-353) is `L` of
the shortened block; a low surrogate following it (already paired) is
consumed as well and weighs 0 (:359-363). -/
def utf16to32 := vlaFamily
/-- src/rvv/rvv_utf8_to.inl.cpp:49, UTF-8 to UTF-16/UTF-32, validating
(`validate = true`). `vlOut` = number of non-continuation bytes of the block
(:107); the code point of a lead byte is produced in the block of the lead
byte (the next three bytes are loaded as `next0..2`, :56-58), its continuation
bytes weigh 0, so the transcribed output of a block is at most `L` of the
block. UTF-32 and the UTF-16 1/2 and 1/2/3 paths (max < 0xF0) store `vlOut`.
The UTF-16 4-byte path stores `rvv_utf32_store_utf16_m4`, 2 units only for a
value > 0xFFFF, which needs a lead byte ≥ 0xF0 (`rvv_bmp_lead`), weight 2.
An invalid block returns before its stores (:80-100). `convert_valid`
instantiates the same helper with `validate = false`; its reparse step at :252
(`--dst` before the scalar tail) is not modelled. -/
def utf8to16 := vlaFamily
def utf8to32 := vlaFamily
/-- src/rvv/rvv_utf8_to.inl.cpp:277, UTF-8 to Latin-1 (validating). `vlOut` =
ASCII + continuation bytes of the block (:296-298); the check at :300-315
rejects (before the store) any continuation not preceded by a C2/C3 lead and
any lead not followed by a continuation, across the block boundary through
`last`. So every continuation maps to the distinct lead byte just before it,
and the output after a block is at most `L` of the prefix (the output may lag
`L` by one byte, a lead whose continuation is in the next block). -/
def utf8toLatin1 := vlaFamily
/-- src/rvv/rvv_utf16fix.cpp:68/:73 (`n > VL`, `in += VL`) and the final call
(:71/:76) for `to_well_formed_utf16`: `vl` units for `vl` units, plus
`out[-1]` and `out[n - 1]`, both inside the output. -/
def utf16fix := vlaFamily
/-- src/rvv/rvv_utf16fix.cpp:97, `change_endianness_utf16`: `vl` for `vl`. -/
def changeEndianness := vlaFamily

/-! ## Decoding UTF-8 on rvv: a surrogate pair needs a lead byte ≥ 0xF0

src/rvv/rvv_utf8_to.inl.cpp:197-220. `shift` is `max(lead >> 4 - 10, 0)`,
with 12 mapped to 3; `c1 = (lead << shift) >> shift` in 8 bits; the code point
is `(c1 << 18 | c2 << 12 | c3 << 6 | c4) >> (6 * (3 - max(shift - 2, 0)))`,
each `c_i ≤ 0x3F`. -/

def rvvShift (b : Nat) : Nat := if b >>> 4 = 12 then 3 else (b >>> 4) - 10
def rvvC1 (b : Nat) : Nat := ((b <<< rvvShift b) % 256) >>> rvvShift b
def rvvValue (b c2 c3 c4 : Nat) : Nat :=
  ((rvvC1 b <<< 18) ||| (c2 <<< 12) ||| (c3 <<< 6) ||| c4) >>> (6 * (3 - (rvvShift b - 2)))

set_option maxRecDepth 100000 in
/-- With the continuation fields at their maximum (the fields are disjoint, so
this is the maximum), a lead byte below 0xF0 decodes to at most 0xFFFF, so
`rvv_utf32_store_utf16_m4` writes one unit for it. -/
theorem rvv_bmp_lead : ∀ b, b < 0xF0 → rvvValue b 0x3F 0x3F 0x3F ≤ 0xFFFF := by
  decide

end SafetyMargins.Arch.Rvv
