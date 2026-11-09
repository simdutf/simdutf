import SafetyMargins.IcelakeMask
/-
Ice Lake UTF-8 → UTF-16: `process_block_utf8_to_utf16` and its three
drivers, on the bytes and the 64-bit masks.

`Arch/Icelake.lean` (`Icelake.utf8`, `safe_utf8`) models the Ice Lake UTF-8
decoders as a table of exact stores; the claim that each masked store writes
no more than `utf16_length_from_utf8` of the bytes it consumes was read from
the C++. This file transcribes the block function
(src/icelake/icelake_utf8_common.inl.cpp:22-434) at the level of the masks:
the byte classes, `mc`, `m1234`, `mend`, the `_pdep_u64` that selects the
processed units, `popcount` for the stored count, `64 - lzcnt` for the
consumed bytes, the high-surrogate exclusion `Mout`, and every check that
returns `false`. The decoded values matter only to the range checks (overlong,
surrogate, above U+10FFFF), which are transcribed for the differential test
(`fail3`, `fail4`) and play no part in the proofs: they can only turn a store
into an error return.

What is proved:

* **One block** (`block16_le`). Whenever a block stores, its count `n` and
  the bytes it consumes `c` satisfy `n ≤ utf16_length_from_utf8` of those `c`
  bytes. The structure check of the kernel (`mc` against `m1234`, which every
  non-ASCII path runs before its store when `validate` is on) makes the bytes
  of the block a sequence of characters cut at most at the end; the count is
  then bounded by a potential argument over windows of classes (`pot`, from
  the finite check `IcelakeMask.win_all`). The two-byte path in a full block
  stores `popcount` of the leads among bytes 0-31, which needs no check.
* **Where the block stops** (`EndOK`). The next block starts on a byte that is
  not a continuation byte, or the block consumed all 64 bytes and ends with
  an ASCII byte or a complete four-byte character. For the 4-byte path this
  needs `Mout`: without it the 32nd unit could be a high surrogate, the next
  block would start inside that character, and the error path would decode
  it a second time.
* **The drivers.** `convert_utf8_to_utf16{le,be}` (`conv16_safe`) and
  `convert_utf8_to_utf16{le,be}_with_errors` (`conv16err_safe`), on every
  input; `convert_valid_utf8_to_utf16{le,be}` (`conv16valid_safe`) on valid
  UTF-8 (`ValidExact.Valid8`), where the checks are compiled out and the
  structure comes from the validity of the input (`struct_of_valid`). No
  store ends past `utf16_length_from_utf8` of the input
  (`L (xs.map Utf8.w16)`), and no load reads past the input.
* **The error path** of `_with_errors` calls the scalar
  `rewind_and_convert_with_errors` at the output cursor, from up to three
  bytes before the failing block. `rewind16_safe`: the rewind decodes only
  from a byte that the kernel has not converted, because of `EndOK`.

What is read from the C++: the transcription itself; the scalar decoders are
transcribed below (`sc16`), with the 16-byte ASCII shortcut taken as 16
one-byte steps (same stores, same reads). The byte swap of big endian
(`byteflip`) does not change any count, so both endiannesses are covered.
-/
namespace SafetyMargins.IcelakeUtf8
set_option linter.unusedSimpArgs false
set_option linter.unusedVariables false
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.ValidExact SafetyMargins.Validator

/-! ## The masks of one block

`blk` holds the bytes the block sees: 64 for `SIMDUTF_FULL`, `gap < 64` for
`SIMDUTF_TAIL`, whose masked load (`_mm512_maskz_loadu_epi8`, line 51) reads
zeros past `gap`. The comparisons below are those of lines 52-54, 105-110,
112-114, 123-125; a comparison on a zero byte past `gap` is false, which is
why each mask is restricted to `p < gap`. -/

section Block
variable (blk : List Nat)

def byte (p : Nat) : Nat := blk.getD p 0

/-- `b`: the bytes of the block (line 48-49). -/
def bM : Mask := fun p => decide (p < blk.length)
/-- `m1`, ASCII (line 52-54). -/
def m1 : Mask := fun p => decide (p < blk.length) && decide (byte blk p < 0x80)
/-- `m234`, `0xc0 <= input` (line 105). -/
def m234 : Mask := fun p => decide (p < blk.length) && decide (0xC0 ≤ byte blk p)
/-- `m34`, `0xdf < input` against `mask_dfdfdfdf_tail`, whose bytes 62 and 63
are `0xff`, so bits 62 and 63 are never set (line 35-38, 108-110). -/
def m34 : Mask := fun p => decide (p < 62) && decide (p < blk.length) && decide (0xDF < byte blk p)
/-- `m4`, `0xf0 <= input` (line 123-125). -/
def m4 : Mask := fun p => decide (p < blk.length) && decide (0xF0 ≤ byte blk p)
def m1234 : Mask := fun p => m1 blk p || m234 blk p
/-- `milltwobytes`: a lead `0xc0` or `0xc1` (line 112-114). -/
def milltwo : Mask := fun p => m234 blk p && decide (byte blk p < 0xC2)
def mp1 : Mask := shl (m234 blk) 1
def mp2 : Mask := shl (m34 blk) 2
def mp3 : Mask := shl (m4 blk) 3
/-- Expected continuation bytes, 1-2-3-byte path (line 140). -/
def mc3 : Mask := fun p => mp1 blk p || mp2 blk p
/-- Expected continuation bytes, 4-byte path (line 246-247). -/
def mc4 : Mask := fun p => mp1 blk p || mp2 blk p || mp3 blk p
/-- The structure check: `xnor(mc, m1234) == 0` on a full block (lines 143-152,
322-330), `mc == b ^ m1234` on a tail (lines 153-157, 331-335), and with
`mc = m234 << 1`, `leading = m1234` on the two-byte path (lines 377-388). -/
def chkB (mc : Mask) : Bool :=
  (List.range 64).all fun p => mc p == (bM blk p && !m1234 blk p)
/-- The bit of the last byte of a tail (lines 161-163, 253-255). -/
def tailbit : Mask := fun p => decide (blk.length < 64) && decide (p + 1 = blk.length)
/-- `mend` of the 1-2-3-byte path (line 160). -/
def mend3 : Mask := fun p => shr (m1234 blk) 1 p || tailbit blk p
/-- `mend` of the 4-byte path (line 251-252). -/
def mend4 : Mask := fun p => shr (fun q => mp3 blk q || m1234 blk q) 1 p || mp3 blk p || tailbit blk p
def andB (a c : Mask) : Mask := fun p => a p && c p
/-- `Mhi` bit 31: bit 32 of `_pext_u64(mp3, mend)` (lines 295-297). -/
def mhi31 : Bool := pext (mp3 blk) (mend4 blk) 32
/-- `Mout = ~(Mhi & 0x80000000)`, a 32-bit mask (line 312). -/
def mout : Mask := fun r => decide (r < 32) && !(decide (r = 31) && mhi31 blk)
/-- `continuation_or_ascii` (line 367-369). -/
def coa : Mask := fun p => bM blk p && !m234 blk p

/-! ### Range checks (values), for the differential test only -/

/-- `mask_not_ascii` (lines 127-129). -/
def notAscii : Mask := fun p => decide (p < blk.length) && !m1 blk p
/-- `clearedbytes`: the high two bits cleared where not ASCII (line 171). -/
def cleared (p : Nat) : Nat := if notAscii blk p then byte blk p &&& 0x3F else byte blk p
/-- Index of the `k`-th set bit (`_mm512_maskz_compress_epi8(mend, identity)`,
line 165), 0 past the last one. -/
def selPos (m : Mask) (k : Nat) : Nat :=
  ((List.range 64).find? fun p => m p && cntB m p == k).getD 0
/-- `thirdsecondandlastbytes` / `Wout` of the 3-byte path, unit `k` (lines
165-204): the last byte, the byte before it if it precedes a non-ASCII byte,
shifted by 6, and the byte two before it if it is in `m34`, shifted by 12,
in 16 bits. The permutation indices are taken modulo 64. -/
def tsl (mend : Mask) (k : Nat) : Nat :=
  let e := selPos mend k
  let i2 := (e + 63) % 64
  let i3 := (e + 62) % 64
  let last := cleared blk e
  let second := if shr (notAscii blk) 1 i2 then cleared blk i2 else 0
  let third := if m34 blk i3 then cleared blk i3 else 0
  (last ||| (second <<< 6) ||| (third <<< 12)) % 65536
/-- `m3 = m34 & (b ^ m4)`, `M3 = pext(m3 << 2, mend)` (lines 219-221, 340-342). -/
def bigM3 (mend : Mask) : Mask := pext (shl (fun p => m34 blk p && bM blk p && !m4 blk p) 2) mend
def sub16 (a c : Nat) : Nat := (a + 65536 - c % 65536) % 65536
/-- Lines 217-231: a 3-byte unit below 0x800 or a surrogate. -/
def fail3 : Bool :=
  (List.range 32).any fun k =>
    bigM3 blk (mend3 blk) k &&
      (decide (tsl blk (mend3 blk) k < 0x800) || decide (sub16 (tsl blk (mend3 blk) k) 0xD800 < 0x800))
/-- Lines 295-309: `Wout` of the 4-byte path. -/
def wout4 (k : Nat) : Nat :=
  let t := tsl blk (mend4 blk) k
  let mlo := pext (mp3 blk) (mend4 blk) k
  let mhi := pext (mp3 blk) (mend4 blk) (k + 1)
  if mhi then ((t >>> 4) + 0xD7C0) % 65536 else (t ||| (if mlo then 0xDC00 else 0))
/-- Lines 338-356: also a high surrogate outside 0xD800-0xDBFF. -/
def fail4 : Bool :=
  (List.range 32).any fun k =>
    (pext (mp3 blk) (mend4 blk) (k + 1) && decide (0x400 ≤ sub16 (wout4 blk k) 0xD800)) ||
    (bigM3 blk (mend4 blk) k &&
      (decide (wout4 blk k < 0x800) || decide (sub16 (wout4 blk k) 0xD800 < 0x800)))

end Block

/-- `process_block_utf8_to_utf16<tail, big_endian, validate>`
(src/icelake/icelake_utf8_common.inl.cpp:24-434) on the bytes `blk` the block
sees (64, or `gap < 64` for a tail). `none` is `return false`; `some (c, n)`
advances the input by `c` bytes and stores `n` units, every store masked to
those `n` units except the two 32-unit stores of a full ASCII block, which
store exactly 64 (lines 58-74; tail: lines 75-101).

* ASCII (line 55): `_ktestc(m1, b)`.
* `validate && milltwobytes` (line 116): error.
* `m34 != 0` (line 120), `m4 == 0` (line 137): the 1-2-3-byte path; check
  (lines 143-158), range check (lines 217-231),
  `mprocessed = pdep(0xFFFFFFFF, mend [& b])` (lines 208-214),
  `nout = popcount` (line 233), `in += 64 - lzcnt` (line 234), store (line 238).
* `m4 != 0`: the 4-byte path; `mprocessed = pdep(Mout, mend [& b])`
  (lines 313-319), check (lines 321-336), range (lines 337-356), consume and
  store (lines 357-363).
* otherwise the two-byte path: check (lines 377-388); full: `in += 32`, one
  more if byte 32 is in `0x80..0xc0` as a signed byte (lines 397-399), store
  `popcount(uint32(leading))` (lines 424-426); tail: `in += 64 -
  lzcnt(pdep(0xFFFFFFFF, continuation_or_ascii))` (line 406), store
  `popcount(pdep(0xFFFFFFFF, leading))` (lines 428-430). `leading` is
  `(m1 | m234) & b` on both branches: the expression on lines 373-376 parses
  as `(tail == (tail == SIMDUTF_FULL)) ? ... : ...`, whose test is false for
  both values of `tail`, which selects the `& b` form; with `b` all ones on a
  full block, both forms agree. -/
def block16 (v : Bool) (blk : List Nat) : Option (Nat × Nat) :=
  if (List.range 64).all (fun p => !bM blk p || m1 blk p) then some (blk.length, blk.length)
  else if v && anyB (milltwo blk) then none
  else if anyB (m34 blk) then
    if !anyB (m4 blk) then
      if v && !chkB blk (mc3 blk) then none
      else if v && fail3 blk then none
      else
        let mp := pdep (ones 32) (andB (mend3 blk) (bM blk))
        some (top mp, popcnt mp)
    else
      if v && !chkB blk (mc4 blk) then none
      else if v && fail4 blk then none
      else
        let mp := pdep (mout blk) (andB (mend4 blk) (bM blk))
        some (top mp, popcnt mp)
  else
    if v && !chkB blk (shl (m234 blk) 1) then none
    else if blk.length = 64 then
      some (32 + ind (decide (0x80 ≤ byte blk 32) && decide (byte blk 32 ≤ 0xC0)), cntB (m1234 blk) 32)
    else some (top (pdep (ones 32) (coa blk)), popcnt (pdep (ones 32) (m1234 blk)))

/-! ## Classes of the block -/

section Classes
variable (blk : List Nat)

def cls (p : Nat) : Cls := if p < blk.length then clsB (byte blk p) else .Z
/-- The class `d` bytes before `p`, `Z` before byte 0. -/
def clm (p d : Nat) : Cls := if d ≤ p then cls blk (p - d) else .Z

theorem clsB_eq (x : Nat) :
    (x < 0x80 ∧ clsB x = .A) ∨ (0x80 ≤ x ∧ x < 0xC0 ∧ clsB x = .C) ∨
    (0xC0 ≤ x ∧ x < 0xE0 ∧ clsB x = .L2) ∨ (0xE0 ≤ x ∧ x < 0xF0 ∧ clsB x = .L3) ∨
    (0xF0 ≤ x ∧ clsB x = .L4) := by
  unfold clsB
  by_cases h1 : x < 0x80
  · simp [h1]
  · by_cases h2 : x < 0xC0
    · simp [h1, h2]; omega
    · by_cases h3 : x < 0xE0
      · simp [h1, h2, h3]; omega
      · by_cases h4 : x < 0xF0
        · simp [h1, h2, h3, h4]; omega
        · simp [h1, h2, h3, h4]; omega

theorem clsB_m234 (x : Nat) : (clsB x).m234 = decide (0xC0 ≤ x) := by
  rcases clsB_eq x with ⟨h, e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, e⟩ <;>
    rw [e] <;> simp [Cls.m234] <;> omega
theorem clsB_m34 (x : Nat) : (clsB x).m34 = decide (0xDF < x) := by
  rcases clsB_eq x with ⟨h, e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, e⟩ <;>
    rw [e] <;> simp [Cls.m34] <;> omega
theorem clsB_m4 (x : Nat) : (clsB x).m4 = decide (0xF0 ≤ x) := by
  rcases clsB_eq x with ⟨h, e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, e⟩ <;>
    rw [e] <;> simp [Cls.m4] <;> omega
theorem clsB_m1234 (x : Nat) : (clsB x).m1234 = (decide (x < 0x80) || decide (0xC0 ≤ x)) := by
  rcases clsB_eq x with ⟨h, e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, e⟩ <;>
    rw [e] <;> simp [Cls.m1234] <;> omega
theorem clsB_C (x : Nat) : (clsB x == .C) = isCont x := by
  rcases clsB_eq x with ⟨h, e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, e⟩ <;>
    rw [e] <;> simp [isCont] <;> omega
theorem clsB_ne_Z (x : Nat) : clsB x ≠ .Z := by
  rcases clsB_eq x with ⟨h, e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, e⟩ <;> rw [e] <;> simp
theorem clsB_w (x : Nat) : (clsB x).w = w16 x := by
  rcases clsB_eq x with ⟨h, e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, h', e⟩ | ⟨h, e⟩ <;>
    rw [e] <;> unfold w16 <;> simp [Cls.w] <;>
    first | omega | (rw [ite_eq_right (by omega)]; rw [ite_eq_right (by omega)]) | (rw [ite_eq_right (by omega)]; rw [ite_eq_left (by omega)]) | (rw [ite_eq_left (by omega)])

theorem m234_cls (p : Nat) : m234 blk p = (cls blk p).m234 := by
  unfold m234 cls; by_cases h : p < blk.length <;> simp [h, clsB_m234]; rfl
theorem m4_cls (p : Nat) : m4 blk p = (cls blk p).m4 := by
  unfold m4 cls; by_cases h : p < blk.length <;> simp [h, clsB_m4]; rfl
theorem m34_cls (p : Nat) (hp : p < 62) : m34 blk p = (cls blk p).m34 := by
  unfold m34 cls; by_cases h : p < blk.length <;> simp [h, hp, clsB_m34]; rfl
theorem m1234_cls (p : Nat) : m1234 blk p = (cls blk p).m1234 := by
  unfold m1234 m1 m234 cls; by_cases h : p < blk.length <;> simp [h, clsB_m1234]; rfl
theorem bC_cls (p : Nat) : (bM blk p && !m1234 blk p) = (cls blk p == .C) := by
  rw [m1234_cls]; unfold bM cls
  by_cases h : p < blk.length
  · simp only [h, decide_true, Bool.true_and]
    cases hc : clsB (byte blk p) <;> simp [Cls.m1234]
    exact absurd hc (clsB_ne_Z _)
  · simp [h]

theorem cls_ge (p : Nat) (h : blk.length ≤ p) : cls blk p = .Z := by
  unfold cls; simp; omega
theorem cls_lt (p : Nat) (h : p < blk.length) : cls blk p = clsB (byte blk p) := by
  unfold cls; simp [h]

theorem clm_zero (p : Nat) : clm blk p 0 = cls blk p := by simp [clm]
theorem clm_succ (p d : Nat) : clm blk (p + 1) (d + 1) = clm blk p d := by
  unfold clm
  by_cases h : d ≤ p
  · simp [h, show d + 1 ≤ p + 1 by omega]
  · simp [h, show ¬ d + 1 ≤ p + 1 by omega]
theorem clm_lt_zero (p d : Nat) (h : p < d) : clm blk p d = .Z := by
  unfold clm; simp; omega

/-- `mc` of the 4-byte path at a position of the block, on the classes. -/
theorem mc4_cls (p : Nat) (hp : p < 64) :
    mc4 blk p = mcW (clm blk p 3) (clm blk p 2) (clm blk p 1) := by
  unfold mc4 mp1 mp2 mp3 shl mcW clm
  have h64 : decide (p < 64) = true := by simp [hp]
  rw [h64]
  by_cases h3 : 3 ≤ p
  · rw [m234_cls, m34_cls _ _ (by omega), m4_cls]; simp [h3, show 2 ≤ p by omega, show 1 ≤ p by omega]
  · by_cases h2 : 2 ≤ p
    · rw [m234_cls, m34_cls _ _ (by omega)]; simp [h3, h2, show 1 ≤ p by omega, Cls.m4]
    · by_cases h1 : 1 ≤ p
      · rw [m234_cls]; simp [h3, h2, h1, Cls.m4, Cls.m34]
      · simp [h3, h2, h1, Cls.m4, Cls.m34, Cls.m234]

/-- The structure check of the 4-byte path; every non-ASCII path checks the
same thing (`chk3_eq`, `chk2_eq`). -/
def Struct : Prop := chkB blk (mc4 blk) = true

theorem struct_at (hS : Struct blk) (p : Nat) (hp : p < 64) :
    chkW (clm blk p 3) (clm blk p 2) (clm blk p 1) (cls blk p) = true := by
  have := List.all_eq_true.mp hS p (List.mem_range.mpr hp)
  rw [mc4_cls _ _ hp, bC_cls] at this
  simpa [chkW] using this

theorem struct_at_iff (p : Nat) (hp : p < 64) :
    (mc4 blk p == (bM blk p && !m1234 blk p)) =
      chkW (clm blk p 3) (clm blk p 2) (clm blk p 1) (cls blk p) := by
  rw [mc4_cls _ _ hp, bC_cls]; rfl

theorem anyB_false {m : Mask} (h : anyB m = false) (p : Nat) (hp : p < 64) : m p = false := by
  unfold anyB at h
  rw [List.any_eq_false] at h
  have := h p (List.mem_range.mpr hp)
  simpa using this

variable {blk}

theorem m4_false_of (h64 : blk.length ≤ 64) (h : anyB (m4 blk) = false) (p : Nat) : m4 blk p = false := by
  by_cases hp : p < 64
  · exact anyB_false h p hp
  · unfold m4; simp; omega

theorem chk3_eq (h64 : blk.length ≤ 64) (h : anyB (m4 blk) = false) :
    chkB blk (mc3 blk) = chkB blk (mc4 blk) := by
  unfold chkB mc3 mc4 mp3 shl
  congr 1; funext p
  rw [m4_false_of h64 h]; simp

theorem chk2_eq (h : anyB (m34 blk) = false) :
    chkB blk (shl (m234 blk) 1) = chkB blk (mc4 blk) := by
  unfold chkB
  congr 1; funext p
  have e2 : mp2 blk p = false := by
    unfold mp2 shl
    by_cases hp : p < 64
    · by_cases h2 : 2 ≤ p
      · rw [anyB_false h (p - 2) (by omega)]; simp
      · simp [h2]
    · simp [hp]
  have e3 : mp3 blk p = false := by
    unfold mp3 shl
    by_cases hp : p < 64
    · by_cases h3 : 3 ≤ p
      · have hm := anyB_false h (p - 3) (by omega)
        unfold m34 at hm; unfold m4
        by_cases hl : p - 3 < blk.length
        · simp [hl, show p - 3 < 62 by omega] at hm
          simp [hl, h3, show ¬ 0xF0 ≤ byte blk (p - 3) by omega]
        · simp [hl]
      · simp [h3]
    · simp [hp]
  unfold mc4 mp1; rw [e2, e3]; simp

end Classes

/-! ## Sums of weights -/

theorem L_take_succ (l : List Nat) (f : Nat → Nat) (k : Nat) (hk : k < l.length) :
    L ((l.take (k + 1)).map f) = L ((l.take k).map f) + f (l.getD k 0) := by
  rw [List.take_add_one, List.getElem?_eq_getElem hk, List.map_append, L_append]
  simp [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hk]

theorem L_take_mono (l : List Nat) (f : Nat → Nat) {a b : Nat} (h : a ≤ b) :
    L ((l.take a).map f) ≤ L ((l.take b).map f) := by
  have : l.take a = (l.take b).take a := by rw [List.take_take]; congr 1; omega
  rw [this, List.map_take]
  exact L_take_le _ _

/-- A mask whose set bits weigh at least 1 counts at most the weight. -/
theorem cntB_le_L (blk : List Nat) (m : Mask) (f : Nat → Nat) (k : Nat) (hk : k ≤ blk.length)
    (h : ∀ p, p < k → m p = true → 1 ≤ f (blk.getD p 0)) :
    cntB m k ≤ L ((blk.take k).map f) := by
  induction k with
  | zero => simp [cntB]
  | succ k ih =>
    have ih' := ih (by omega) (fun p hp => h p (by omega))
    rw [L_take_succ _ _ _ (by omega)]
    simp only [cntB]
    cases hm : m k
    · simp only [ind_false]; omega
    · have := h k (by omega) hm; simp only [ind_true]; omega

/-! ## The potential argument -/

section Pot
variable {blk : List Nat}

def pfOf (blk : List Nat) (k : Nat) : PF :=
  if k + 1 < blk.length then .inner else if blk.length = 64 then .full else .tail

theorem mp3_cls (q : Nat) (hq : q < 64) (h3 : 3 ≤ q) : mp3 blk q = (cls blk (q - 3)).m4 := by
  unfold mp3 shl; rw [m4_cls]; simp [hq, h3]

theorem mp3_clm (k : Nat) (hk : k < 64) : mp3 blk k = (clm blk k 3).m4 := by
  unfold clm
  by_cases h3 : 3 ≤ k
  · rw [mp3_cls k hk h3]; simp [h3]
  · unfold mp3 shl; simp [h3, Cls.m4]

theorem mend4_cls (h64 : blk.length ≤ 64) (k : Nat) (hk : k < blk.length) :
    mend4 blk k = mendW (pfOf blk k) (clm blk k 3) (clm blk k 2) (cls blk (k + 1)) := by
  have hm3 := mp3_clm (blk := blk) k (by omega)
  unfold mend4 pfOf
  by_cases hin : k + 1 < blk.length
  · have h1 : mp3 blk (k + 1) = (clm blk k 2).m4 := by
      rw [mp3_clm (k + 1) (by omega), clm_succ]
    simp only [shr, tailbit, hin, ite_true, mendW]
    rw [h1, hm3, m1234_cls]
    simp [show k < 64 by omega, show ¬ k + 1 = blk.length by omega]
  · simp only [hin, ite_false]
    by_cases hf : blk.length = 64
    · have hk63 : k = 63 := by omega
      subst hk63
      simp only [hf, ite_true, mendW, shr, tailbit]
      rw [hm3]
      have : mp3 blk 64 = false := by simp [mp3, shl]
      rw [this, m1234_cls, cls_ge _ _ (by omega)]
      simp [Cls.m1234]
    · simp only [hf, ite_false, mendW, tailbit]
      simp [show blk.length < 64 by omega, show k + 1 = blk.length by omega]

theorem andB_bM (m : Mask) (k : Nat) (hk : k < blk.length) : andB m (bM blk) k = m k := by
  simp [andB, bM, hk]

theorem winAt (hS : Struct blk) (h64 : blk.length ≤ 64) {k : Nat}
    (hc : cons3 (clm blk k 3) (clm blk k 2) (clm blk k 1) = true) (hk' : k < blk.length) :
    winPre (pfOf blk k) (clm blk k 3) (clm blk k 2) (clm blk k 1) (cls blk k)
      (cls blk (k + 1)) = true := by
    have s1 : clm blk (k + 1) 3 = clm blk k 2 := clm_succ _ _ _
    have s2 : clm blk (k + 1) 2 = clm blk k 1 := clm_succ _ _ _
    have s3 : clm blk (k + 1) 1 = cls blk k := by rw [clm_succ, clm_zero]
    have hz : cls blk k ≠ .Z := by rw [cls_lt _ _ hk']; exact clsB_ne_Z _
    apply winPre_of hc hz (struct_at _ hS k (by omega))
    · by_cases hf : pfOf blk k = .full
      · exact Or.inl hf
      · right
        have hk1 : k + 1 < 64 := by
          unfold pfOf at hf
          by_cases hin : k + 1 < blk.length
          · omega
          · by_cases h6 : blk.length = 64
            · simp [hin, h6] at hf <;> omega
            · omega
        have := struct_at _ hS (k + 1) hk1
        rwa [s1, s2, s3] at this
    · unfold pfOf
      by_cases hin : k + 1 < blk.length
      · simp [hin, cls_lt _ _ hin, clsB_ne_Z]
      · rw [cls_ge _ _ (by omega)]; simp only [hin, ite_false]; split <;> simp

/-- **The potential.** On a block that passes the structure check, the
`mend` bits below any position `k`, plus the debt `phi` of the character in
progress, are at most `utf16_length_from_utf8` of the first `k` bytes. -/
theorem pot (hS : Struct blk) (h64 : blk.length ≤ 64) :
    ∀ k, k ≤ blk.length →
      cons3 (clm blk k 3) (clm blk k 2) (clm blk k 1) = true ∧
      cntB (andB (mend4 blk) (bM blk)) k + phi (clm blk k 3) (clm blk k 2) (clm blk k 1)
        ≤ L ((blk.take k).map w16) := by
  intro k
  induction k with
  | zero =>
    intro _
    simp [clm, cons3_start, cntB, phi]
  | succ k ih =>
    intro hk
    obtain ⟨hc, hi⟩ := ih (by omega)
    have hk' : k < blk.length := by omega
    have s1 : clm blk (k + 1) 3 = clm blk k 2 := clm_succ _ _ _
    have s2 : clm blk (k + 1) 2 = clm blk k 1 := clm_succ _ _ _
    have s3 : clm blk (k + 1) 1 = cls blk k := by rw [clm_succ, clm_zero]
    have hz : cls blk k ≠ .Z := by rw [cls_lt _ _ hk']; exact clsB_ne_Z _
    have hpre := winAt hS h64 hc hk'
    obtain ⟨hw, -, -, hc'⟩ := winPost_spec (win hpre)
    refine ⟨by rw [s1, s2, s3]; exact hc', ?_⟩
    rw [s1, s2, s3, L_take_succ _ _ _ hk']
    simp only [cntB]
    rw [andB_bM _ _ hk', mend4_cls h64 _ hk']
    have hw16 : (cls blk k).w = w16 (blk.getD k 0) := by rw [cls_lt _ _ hk', clsB_w]; rfl
    omega

end Pot

/-! ## One block -/

section BlockFacts
variable {blk : List Nat}

/-- Where a block may stop: before a byte that is not a continuation byte, or
after all 64 bytes with an ASCII byte or a four-byte character at the end. -/
def EndOK (blk : List Nat) (c : Nat) : Prop :=
  (c < blk.length → isCont (byte blk c) = false) ∧
  (c = 64 → byte blk 63 < 0x80 ∨
    (0xF0 ≤ byte blk 60 ∧ isCont (byte blk 61) = true ∧ isCont (byte blk 62) = true ∧
      isCont (byte blk 63) = true))

theorem isCont_cls (p : Nat) (hp : p < blk.length) : isCont (byte blk p) = (cls blk p == .C) := by
  rw [cls_lt _ _ hp, clsB_C]

theorem top_le_len (m : Mask) (h : ∀ p, m p = true → p < blk.length) : top m ≤ blk.length := by
  by_cases h0 : top m = 0
  · omega
  · have := h _ (top_set m (by omega)); omega

theorem chk_eq (hS : Struct blk) (p : Nat) (hp : p < 64) :
    mc4 blk p = (bM blk p && !m1234 blk p) := by
  have := List.all_eq_true.mp hS p (List.mem_range.mpr hp)
  simpa using this

theorem m4_m234 (p : Nat) (h : m4 blk p = true) : m234 blk p = true := by
  simp only [m4, m234, Bool.and_eq_true, decide_eq_true_eq] at h ⊢; omega

theorem mp3_spec (p : Nat) (h : mp3 blk p = true) : 3 ≤ p ∧ p < 64 ∧ m4 blk (p - 3) = true := by
  simp only [mp3, shl, Bool.and_eq_true, decide_eq_true_eq] at h; exact ⟨h.1.2, h.1.1, h.2⟩

theorem mc4_of_mp3 (p : Nat) (h : mp3 blk p = true) : mc4 blk p = true := by
  simp [mc4, h]

theorem mc4_of_m234 (p : Nat) (h1 : 1 ≤ p) (h2 : p < 64) (h : m234 blk (p - 1) = true) :
    mc4 blk p = true := by
  simp [mc4, mp1, shl, h, h1, h2]

theorem cls_zero_ne_C (hS : Struct blk) : (cls blk 0 == .C) = false := by
  have := struct_at _ hS 0 (by omega)
  simp only [clm_lt_zero _ 0 3 (by omega), clm_lt_zero _ 0 2 (by omega),
    clm_lt_zero _ 0 1 (by omega)] at this
  revert this; cases cls blk 0 <;> decide

/-- The 1-2-3-byte and 4-byte paths, for any selection `src` of the units,
provided the processed bits never stop on a high surrogate (`hT`). -/
theorem path34 (hS : Struct blk) (h64 : blk.length ≤ 64) (src mendX : Mask)
    (hE : ∀ p, andB mendX (bM blk) p = andB (mend4 blk) (bM blk) p)
    (hT : 0 < top (pdep src (andB mendX (bM blk))) → top (pdep src (andB mendX (bM blk))) < 64 →
      mp3 blk (top (pdep src (andB mendX (bM blk)))) = false) :
    top (pdep src (andB mendX (bM blk))) ≤ blk.length ∧
    popcnt (pdep src (andB mendX (bM blk))) ≤
      L ((blk.take (top (pdep src (andB mendX (bM blk))))).map w16) ∧
    EndOK blk (top (pdep src (andB mendX (bM blk)))) := by
  generalize hmp : pdep src (andB mendX (bM blk)) = mp at hT ⊢
  have hsub : ∀ p, mp p = true → p < blk.length := by
    intro p hp; rw [← hmp] at hp
    have := pdep_sub _ _ _ hp
    simp only [andB, bM, Bool.and_eq_true, decide_eq_true_eq] at this; exact this.2
  have hc := top_le_len mp hsub
  have hsubM : ∀ p, mp p = true → andB mendX (bM blk) p = true := by
    intro p hp; rw [← hmp] at hp; exact pdep_sub _ _ _ hp
  refine ⟨hc, ?_, ?_, ?_⟩
  · have h1 := popcnt_pdep_le src (andB mendX (bM blk))
    rw [hmp] at h1
    have h2 : cntB (andB mendX (bM blk)) (top mp) = cntB (andB (mend4 blk) (bM blk)) (top mp) :=
      cntB_congr (fun p _ => hE p)
    have h3 := (pot hS h64 (top mp) hc).2
    omega
  · intro hlt
    by_cases h0 : top mp = 0
    · rw [h0, isCont_cls 0 (by omega), cls_zero_ne_C hS]
    · have hq := top_set mp (by omega)
      have hq' := hsubM _ hq
      rw [hE, andB_bM _ _ (by omega)] at hq'
      have hk : top mp - 1 < blk.length := by omega
      rw [mend4_cls h64 _ hk] at hq'
      have hpf : pfOf blk (top mp - 1) = .inner := by
        unfold pfOf; simp [show top mp - 1 + 1 < blk.length by omega]
      have hw := winPost_spec (win (winAt hS h64 (pot hS h64 (top mp - 1) (by omega)).1 hk))
      have hm3 := hT (by omega) (by omega)
      rw [mp3_clm _ (by omega)] at hm3
      have e : clm blk (top mp) 3 = clm blk (top mp - 1) 2 := by
        rw [show top mp = top mp - 1 + 1 by omega, clm_succ]; simp
      rw [e] at hm3
      rw [hpf] at hq' hw
      have := hw.2.1 rfl hq' hm3
      rw [show top mp - 1 + 1 = top mp by omega] at this
      rw [isCont_cls _ hlt]; simpa using this
  · intro h64'
    have hl : blk.length = 64 := by omega
    have hq := top_set mp (by omega)
    rw [h64'] at hq
    have hq' := hsubM _ hq
    rw [hE, andB_bM _ _ (by omega), mend4_cls h64 _ (by omega)] at hq'
    have hpf : pfOf blk 63 = .full := by unfold pfOf; simp [hl]
    have hw := winPost_spec (win (winAt hS h64 (pot hS h64 63 (by omega)).1 (by omega)))
    rw [hpf] at hq' hw
    obtain ⟨e3, e2, e1, e0⟩ := hw.2.2.1 rfl hq'
    simp only [clm, show 3 ≤ 63 by omega, show 2 ≤ 63 by omega, show 1 ≤ 63 by omega,
      ite_true] at e3 e2 e1
    right
    rw [cls_lt _ _ (by omega)] at e3 e2 e1 e0
    refine ⟨?_, ?_, ?_, ?_⟩
    · have := clsB_m4 (byte blk 60); rw [e3] at this; simpa [Cls.m4] using this
    · rw [← clsB_C, e2]; rfl
    · rw [← clsB_C, e1]; rfl
    · rw [← clsB_C, e0]; rfl

/-- `Mout`: the processed units never end on a high surrogate, so the next
block never starts inside a four-byte character (lines 312-319). -/
theorem mout_top (hS : Struct blk) (h64 : blk.length ≤ 64)
    (h0 : 0 < top (pdep (mout blk) (andB (mend4 blk) (bM blk))))
    (h1 : top (pdep (mout blk) (andB (mend4 blk) (bM blk))) < 64) :
    mp3 blk (top (pdep (mout blk) (andB (mend4 blk) (bM blk)))) = false := by
  generalize hc : top (pdep (mout blk) (andB (mend4 blk) (bM blk))) = c at h0 h1
  -- `mend4` has no bit past the block
  have hMeq : ∀ p, andB (mend4 blk) (bM blk) p = mend4 blk p := by
    intro p
    by_cases hp : p < blk.length
    · exact andB_bM _ _ hp
    · have hb : bM blk p = false := by simp [bM, hp]
      simp only [andB, hb, Bool.and_false]
      symm
      simp only [mend4, shr, tailbit, Bool.or_eq_false_iff]
      refine ⟨⟨?_, ?_⟩, ?_⟩
      · by_cases h1 : p < 64
        · have e1 : m1234 blk (p + 1) = false := by
            rw [m1234_cls, cls_ge _ _ (by omega)]; rfl
          by_cases h2 : p + 1 < 64
          · have := chk_eq hS (p + 1) h2
            have hb1 : bM blk (p + 1) = false := by simp [bM]; omega
            rw [hb1] at this
            have : mp3 blk (p + 1) = false := by
              cases h : mp3 blk (p + 1)
              · rfl
              · have := mc4_of_mp3 (blk := blk) _ h; simp_all
            simp [this, e1]
          · have : mp3 blk (p + 1) = false := by simp [mp3, shl]; omega
            simp [this, e1]
        · simp [h1]
      · by_cases h1 : p < 64
        · have := chk_eq hS p h1
          rw [hb] at this
          cases h : mp3 blk p
          · rfl
          · have := mc4_of_mp3 (blk := blk) _ h; simp_all
        · simp [mp3, shl, h1]
      · simp; omega
  have hcnt : ∀ n, cntB (andB (mend4 blk) (bM blk)) n = cntB (mend4 blk) n :=
    fun n => cntB_congr (fun p _ => hMeq p)
  have hq := top_set (pdep (mout blk) (andB (mend4 blk) (bM blk))) (by omega)
  rw [hc] at hq
  have hMq := pdep_sub _ _ _ hq
  have hsrc := pdep_src _ _ _ hq
  rw [hMeq] at hMq
  rw [hcnt] at hsrc
  generalize hr : cntB (mend4 blk) (c - 1) = r at hsrc
  simp only [mout, Bool.and_eq_true, decide_eq_true_eq, Bool.not_eq_true',
    Bool.and_eq_false_iff] at hsrc
  cases hm3 : mp3 blk c
  · rfl
  · exfalso
    have hMc : mend4 blk c = true := by simp [mend4, hm3]
    have hrc : cntB (mend4 blk) c = r + 1 := by
      rw [show c = c - 1 + 1 by omega, cntB_succ_of _ _ hMq, hr]
    have hpc : pdep (mout blk) (andB (mend4 blk) (bM blk)) c = false :=
      top_above _ c (by omega) h1
    have hpc' : mout blk (r + 1) = false := by
      have e : pdep (mout blk) (andB (mend4 blk) (bM blk)) c = mout blk (r + 1) := by
        simp only [pdep, h1, decide_true, Bool.true_and, hMeq, hMc, hcnt, hrc]
      rw [← e]; exact hpc
    cases hh : mhi31 blk
    · -- `Mhi` bit 31 clear: then `r = 31`, and the 33rd `mend` bit is this
      -- low surrogate, so `Mhi` bit 31 is set
      simp only [mout, hh, Bool.and_false, Bool.not_false, Bool.and_true,
        decide_eq_false_iff_not] at hpc'
      have hr31 : r = 31 := by
        have := hsrc.1; omega
      have : mhi31 blk = true := by
        unfold mhi31; rw [show (32 : Nat) = cntB (mend4 blk) c by omega, pext_at _ _ c h1 hMc, hm3]
      rw [hh] at this; exact absurd this (by decide)
    -- `Mhi` bit 31 set: `r + 1 = 31`, and the 33rd `mend` bit is the low
    -- surrogate of a four-byte character whose high surrogate ends at `c`
    have hr30 : r + 1 = 31 := by
      have h1' := hsrc.1
      have h2' : r ≠ 31 := by
        rcases hsrc.2 with h | h
        · simpa using h
        · simp [h] at hh
      simp [mout, hh] at hpc'
      omega
    have hhi' : pext (mp3 blk) (mend4 blk) 32 = true := hh
    obtain ⟨q, hq64, hmq, hcq, hm3q⟩ := pext_true _ _ _ hhi'
    obtain ⟨hq3, -, hq4⟩ := mp3_spec q hm3q
    have hmq1 : mend4 blk (q - 1) = true := by
      simp [mend4, shr, show q - 1 < 64 by omega, show q - 1 + 1 = q by omega, hm3q]
    have hcq1 : cntB (mend4 blk) (q - 1) = 31 := by
      have := cntB_succ_of (mend4 blk) (q - 1) hmq1
      rw [show q - 1 + 1 = q by omega, hcq] at this; omega
    have hqc : q - 1 = c := rank_inj _ hmq1 hMc (by rw [hcq1, hrc]; omega)
    obtain ⟨hc3, -, hc4⟩ := mp3_spec c hm3
    have hm4a : m4 blk (c - 2) = true := by
      rw [show c - 2 = q - 3 by omega]; exact hq4
    have := chk_eq hS (c - 2) (by omega)
    rw [mc4_of_m234 (blk := blk) (c - 2) (by omega) (by omega)
      (by rw [show c - 2 - 1 = c - 3 by omega]; exact m4_m234 _ hc4)] at this
    have hm234 := m4_m234 _ hm4a
    simp [m1234, hm234] at this

theorem m1234_w (p : Nat) (hp : p < blk.length) (h : m1234 blk p = true) :
    1 ≤ w16 (blk.getD p 0) := by
  rw [m1234_cls, cls_lt _ _ hp] at h
  rw [← clsB_w]; show 1 ≤ (clsB (byte blk p)).w
  revert h; cases clsB (byte blk p) <;> simp [Cls.m1234, Cls.w]

theorem cntB_beyond (m : Mask) (n : Nat) (hn : n ≤ 64) (h : ∀ p, n ≤ p → m p = false) :
    cntB m 64 = cntB m n := by
  have : ∀ k, cntB m (n + k) = cntB m n := by
    intro k; induction k with
    | zero => rfl
    | succ k ih => rw [← Nat.add_assoc, cntB_succ_of_not _ _ (h _ (by omega)), ih]
  have := this (64 - n); rwa [show n + (64 - n) = 64 by omega] at this

theorem no34 (h : anyB (m34 blk) = false) (q : Nat) (hq : q < 62) :
    (cls blk q).m34 = false ∧ (cls blk q).m4 = false := by
  have e := anyB_false h q (by omega)
  rw [m34_cls _ _ hq] at e
  refine ⟨e, ?_⟩
  revert e; cases cls blk q <;> simp [Cls.m34, Cls.m4]

/-- On the two-byte path the leads and the last bytes of the characters
alternate: below `k`, the leads are the last bytes plus one if byte `k - 1`
is a lead. -/
theorem lead_coa (hS : Struct blk) (h64 : blk.length ≤ 64) (h34 : anyB (m34 blk) = false) :
    ∀ k, k ≤ blk.length → cntB (m1234 blk) k = cntB (coa blk) k + ind (clm blk k 1).m234 := by
  intro k
  induction k with
  | zero => intro _; simp [cntB, clm, Cls.m234]
  | succ k ih =>
    intro hk
    have ih' := ih (by omega)
    have hk' : k < blk.length := by omega
    have hc := struct_at _ hS k (by omega)
    have e2 : (clm blk k 2).m34 = false := by
      unfold clm; split
      · exact (no34 h34 _ (by omega)).1
      · rfl
    have e3 : (clm blk k 3).m4 = false := by
      unfold clm; split
      · exact (no34 h34 _ (by omega)).2
      · rfl
    simp only [chkW, mcW, e2, e3, Bool.or_false] at hc
    rw [clm_succ, clm_zero]
    simp only [cntB, ih', m1234_cls, coa, m234_cls]
    have hb : bM blk k = true := by simp [bM, hk']
    rw [hb]
    have hz := clsB_ne_Z (byte blk k)
    rw [← cls_lt _ _ hk'] at hz
    revert hc hz
    cases cls blk k <;> cases (clm blk k 1).m234 <;> simp [Cls.m1234, Cls.m234] <;> omega

/-- `utf16_length_from_utf8` of ASCII bytes is their number. -/
theorem cntB_true (k : Nat) : cntB (fun _ => true) k = k := by
  induction k with
  | zero => rfl
  | succ k ih => simp [cntB, ih]

theorem ascii_L (k : Nat) (hk : k ≤ blk.length) (h : ∀ p, p < k → byte blk p < 0x80) :
    k ≤ L ((blk.take k).map w16) := by
  have := cntB_le_L blk (fun _ => true) w16 k hk (fun p hp _ => by
    have hb := h p hp; unfold byte at hb
    rw [← clsB_w]
    rcases clsB_eq (blk.getD p 0) with ⟨_, e⟩ | ⟨h1, _, _⟩ | ⟨h1, _, _⟩ | ⟨h1, _, _⟩ | ⟨h1, _⟩
    · rw [e]; simp [Cls.w]
    all_goals omega)
  rw [cntB_true] at this; exact this

theorem ascii_test (h : (List.range 64).all (fun p => !bM blk p || m1 blk p) = true)
    (h64 : blk.length ≤ 64) (p : Nat) (hp : p < blk.length) : byte blk p < 0x80 := by
  have := List.all_eq_true.mp h p (List.mem_range.mpr (by omega))
  simp [bM, m1, hp] at this; exact this

theorem struct_ascii (h : (List.range 64).all (fun p => !bM blk p || m1 blk p) = true)
    (h64 : blk.length ≤ 64) : Struct blk ∧ anyB (milltwo blk) = false := by
  have hc : ∀ q, (cls blk q).m234 = false := by
    intro q; unfold cls; split
    · rw [clsB_m234]; have := ascii_test h h64 q (by omega); simp; omega
    · rfl
  have hn : ∀ q, (cls blk q) ≠ .C := by
    intro q; unfold cls; split
    · rcases clsB_eq (byte blk q) with ⟨_, e⟩ | ⟨h1, _, _⟩ | ⟨h1, _, _⟩ | ⟨h1, _, _⟩ | ⟨h1, _⟩
      · rw [e]; simp
      all_goals (have := ascii_test h h64 q (by omega); omega)
    · simp
  have h34 : ∀ q, (cls blk q).m34 = false := by
    intro q; have := hc q; revert this; cases cls blk q <;> simp [Cls.m234, Cls.m34]
  have h4 : ∀ q, (cls blk q).m4 = false := by
    intro q; have := hc q; revert this; cases cls blk q <;> simp [Cls.m234, Cls.m4]
  constructor
  · unfold Struct chkB
    rw [List.all_eq_true]; intro p hp
    have hp' := List.mem_range.mp hp
    rw [mc4_cls _ _ hp', bC_cls]
    have g1 : ∀ d, (clm blk p d).m234 = false := by
      intro d; unfold clm; split
      · exact hc _
      · rfl
    have g2 : ∀ d, (clm blk p d).m34 = false := by
      intro d; unfold clm; split
      · exact h34 _
      · rfl
    have g3 : ∀ d, (clm blk p d).m4 = false := by
      intro d; unfold clm; split
      · exact h4 _
      · rfl
    have : (cls blk p == .C) = false := by simpa using hn p
    simp [mcW, g1, g2, g3, this]
  · unfold anyB; rw [List.any_eq_false]; intro p _
    simp [milltwo, m234_cls, hc]

/-- **One block of `process_block_utf8_to_utf16`.** On a block that passes
the structure check and has no `0xC0`/`0xC1` lead, every outcome of the block
(checks compiled out, as in `convert_valid`; with them, see `block16_true`)
consumes `c` bytes of the block, stores `n ≤ utf16_length_from_utf8` of those
bytes, and stops where `EndOK` says. -/
theorem block16_core (h64 : blk.length ≤ 64) (hS : Struct blk)
    (hm : anyB (milltwo blk) = false) {c n : Nat} (h : block16 false blk = some (c, n)) :
    c ≤ blk.length ∧ n ≤ L ((blk.take c).map w16) ∧ EndOK blk c := by
  unfold block16 at h
  by_cases ha : (List.range 64).all (fun p => !bM blk p || m1 blk p) = true
  · rw [ite_eq_left ha] at h
    simp only [Option.some.injEq, Prod.mk.injEq] at h
    obtain ⟨rfl, rfl⟩ := h
    refine ⟨Nat.le_refl _, ascii_L _ (Nat.le_refl _) (fun p hp => ascii_test ha h64 p hp), ?_, ?_⟩
    · intro h; omega
    · intro h6; left; exact ascii_test ha h64 63 (by omega)
  rw [ite_eq_right ha] at h
  simp only [Bool.false_and, Bool.false_eq_true, ite_false] at h
  by_cases h34 : anyB (m34 blk) = true
  · rw [ite_eq_left h34] at h
    by_cases h4 : anyB (m4 blk) = true
    · simp only [h4, Bool.not_true, Bool.false_eq_true, ite_false, Option.some.injEq,
        Prod.mk.injEq] at h
      obtain ⟨rfl, rfl⟩ := h
      exact path34 hS h64 _ _ (fun _ => rfl) (fun h0 h1 => mout_top hS h64 h0 h1)
    · have h4' : anyB (m4 blk) = false := by simpa using h4
      simp only [h4', Bool.not_false, ite_true, Option.some.injEq, Prod.mk.injEq] at h
      obtain ⟨rfl, rfl⟩ := h
      have hmp3 : ∀ p, mp3 blk p = false := by
        intro p; unfold mp3 shl; rw [m4_false_of h64 h4']; simp
      refine path34 hS h64 _ _ (fun p => ?_) (fun _ _ => hmp3 _)
      simp [andB, mend3, mend4, hmp3]
  · have h34' : anyB (m34 blk) = false := by simpa using h34
    rw [ite_eq_right h34] at h
    by_cases hf : blk.length = 64
    · rw [ite_eq_left hf] at h
      simp only [Option.some.injEq, Prod.mk.injEq] at h
      obtain ⟨rfl, rfl⟩ := h
      have hi := ind_le_one (decide (0x80 ≤ byte blk 32) && decide (byte blk 32 ≤ 0xC0))
      refine ⟨by omega, ?_, ?_, ?_⟩
      · have h1 := cntB_le_L blk (m1234 blk) w16 32 (by omega)
          (fun p hp hm => m1234_w p (by omega) hm)
        have h2 := L_take_mono blk w16 (show 32 ≤ 32 + ind (decide (0x80 ≤ byte blk 32) &&
          decide (byte blk 32 ≤ 0xC0)) by omega)
        omega
      · intro _
        cases hb : (decide (0x80 ≤ byte blk 32) && decide (byte blk 32 ≤ 0xC0))
        · simp only [ind_false, Nat.add_zero]
          simp only [Bool.and_eq_false_iff, decide_eq_false_iff_not] at hb
          simp [isCont]; omega
        · simp only [ind_true]
          simp only [Bool.and_eq_true, decide_eq_true_eq] at hb
          -- byte 32 is not `0xC0` (no `milltwobytes`), so it is a continuation
          -- byte, and the check at byte 33 says byte 33 is not one
          have hne : byte blk 32 ≠ 0xC0 := by
            intro e
            have := anyB_false hm 32 (by omega)
            simp [milltwo, m234, e, hf] at this
          have hc32 : cls blk 32 = .C := by
            rw [cls_lt _ _ (by omega)]
            rcases clsB_eq (byte blk 32) with ⟨h1, _⟩ | ⟨_, _, e⟩ | ⟨h1, _, _⟩ | ⟨h1, _, _⟩ | ⟨h1, _⟩
            all_goals first | exact e | omega
          have hc : chkW (cls blk 30) (cls blk 31) (cls blk 32) (cls blk 33) = true := by
            have := struct_at _ hS 33 (by omega); simpa [clm] using this
          rw [hc32] at hc
          have e31 := (no34 h34' 31 (by omega)).1
          have e30 := (no34 h34' 30 (by omega)).2
          rw [show (32 : Nat) + 1 = 33 from rfl, isCont_cls _ (by omega)]
          simp only [chkW, mcW, e31, e30, Cls.m234, Bool.or_false] at hc
          revert hc; cases cls blk 33 <;> simp [chkW, mcW, Cls.m234]
      · intro h; omega
    · rw [ite_eq_right hf] at h
      simp only [Option.some.injEq, Prod.mk.injEq] at h
      obtain ⟨rfl, rfl⟩ := h
      have hlt : blk.length < 64 := by omega
      have hsub : ∀ p, pdep (ones 32) (coa blk) p = true → p < blk.length := by
        intro p hp
        have := pdep_sub _ _ _ hp
        simp only [coa, bM, Bool.and_eq_true, decide_eq_true_eq] at this; exact this.1
      have hc := top_le_len _ hsub
      have hd := lead_coa hS h64 h34'
      refine ⟨hc, ?_, ?_, ?_⟩
      · -- nothing is pending at the end of the tail
        have hend : (clm blk blk.length 1).m234 = false := by
          have := struct_at _ hS blk.length hlt
          rw [cls_ge _ _ (Nat.le_refl _)] at this
          revert this; simp only [chkW, mcW]
          cases (clm blk blk.length 1).m234 <;> simp
        have e1 : popcnt (m1234 blk) = cntB (m1234 blk) blk.length :=
          cntB_beyond _ _ h64 (fun p hp => by rw [m1234_cls, cls_ge _ _ hp]; rfl)
        have e2 : popcnt (coa blk) = cntB (coa blk) blk.length :=
          cntB_beyond _ _ h64 (fun p hp => by simp [coa, bM]; omega)
        have e3 := hd _ (Nat.le_refl _)
        rw [hend] at e3
        have e4 := cntB_top_pdep_ones 32 (coa blk)
        have e5 := hd _ hc
        have e6 := cntB_le_L blk (m1234 blk) w16 _ hc (fun p hp hm => m1234_w p (by omega) hm)
        rw [popcnt_pdep_ones]
        simp only [ind_false] at e3
        omega
      · intro hlt'
        by_cases h0 : top (pdep (ones 32) (coa blk)) = 0
        · rw [h0, isCont_cls 0 (by omega), cls_zero_ne_C hS]
        · have hq := pdep_sub _ _ _ (top_set (pdep (ones 32) (coa blk)) (by omega))
          generalize ht : top (pdep (ones 32) (coa blk)) = t at hq hlt' h0
          have hq' : (cls blk (t - 1)).m234 = false := by
            simp only [coa, Bool.and_eq_true, Bool.not_eq_true'] at hq
            rw [← m234_cls]; exact hq.2
          have hc := struct_at _ hS t (by omega)
          have e1 : (clm blk t 1).m234 = false := by
            unfold clm; rw [ite_eq_left (by omega)]; exact hq'
          have e2 : (clm blk t 2).m34 = false := by
            unfold clm; split
            · exact (no34 h34' _ (by omega)).1
            · rfl
          have e3 : (clm blk t 3).m4 = false := by
            unfold clm; split
            · exact (no34 h34' _ (by omega)).2
            · rfl
          simp only [chkW, mcW, e1, e2, e3] at hc
          rw [isCont_cls _ hlt']
          revert hc; cases cls blk t <;> simp
      · intro h; omega

/-- With the checks on, a block that returns `true` is a block of
`block16_core`: it passed the structure check and has no `0xC0`/`0xC1` lead,
and its outcome is the outcome without the checks. -/
theorem block16_true (h64 : blk.length ≤ 64) {r : Nat × Nat} (h : block16 true blk = some r) :
    block16 false blk = some r ∧ Struct blk ∧ anyB (milltwo blk) = false := by
  unfold block16 at h ⊢
  by_cases ha : (List.range 64).all (fun p => !bM blk p || m1 blk p) = true
  · rw [ite_eq_left ha] at h ⊢; exact ⟨h, struct_ascii ha h64⟩
  rw [ite_eq_right ha] at h ⊢
  by_cases hm : anyB (milltwo blk) = true
  · simp [hm] at h
  have hm' : anyB (milltwo blk) = false := by simpa using hm
  simp only [hm', Bool.and_false, Bool.false_eq_true, ite_false, Bool.true_and,
    Bool.false_and] at h ⊢
  by_cases h34 : anyB (m34 blk) = true
  · rw [ite_eq_left h34] at h ⊢
    by_cases h4 : anyB (m4 blk) = true
    · simp only [h4, Bool.not_true, Bool.false_eq_true, ite_false] at h ⊢
      by_cases hc : chkB blk (mc4 blk) = true
      · simp only [hc, Bool.not_true, Bool.false_eq_true, ite_false] at h
        by_cases hf : fail4 blk = true
        · simp [hf] at h
        · simp only [hf, Bool.false_eq_true, ite_false] at h
          exact ⟨h, hc, trivial⟩
      · simp [hc] at h
    · have h4' : anyB (m4 blk) = false := by simpa using h4
      simp only [h4', Bool.not_false, ite_true] at h ⊢
      by_cases hc : chkB blk (mc3 blk) = true
      · simp only [hc, Bool.not_true, Bool.false_eq_true, ite_false] at h
        by_cases hf : fail3 blk = true
        · simp [hf] at h
        · simp only [hf, Bool.false_eq_true, ite_false] at h
          exact ⟨h, by rw [Struct, ← chk3_eq h64 h4']; exact hc, trivial⟩
      · simp [hc] at h
  · rw [ite_eq_right h34] at h ⊢
    have h34' : anyB (m34 blk) = false := by simpa using h34
    by_cases hc : chkB blk (shl (m234 blk) 1) = true
    · simp only [hc, Bool.not_true, Bool.false_eq_true, ite_false] at h
      exact ⟨h, by rw [Struct, ← chk2_eq h34']; exact hc, trivial⟩
    · simp [hc] at h

end BlockFacts

/-! ## Valid UTF-8 passes the structure check

`convert_valid` compiles the checks out. On valid UTF-8, a block that starts
on a character boundary passes the structure check anyway: by induction over
the bytes, the automaton state of `ValidExact.step` and the classes of the last
three bytes agree on how many continuation bytes are expected (`remS`,
`remW`; finite facts `IcelakeMask.step_ok`, `IcelakeMask.validWin_ok`). -/

section Valid
variable {blk : List Nat}

theorem step_le8 (s x : Nat) : ValidExact.step s x ≤ 8 := by
  unfold ValidExact.step; repeat (first | omega | split)

theorem run_le8 (bs : List Nat) : ∀ s, s ≤ 8 → ValidExact.run s bs ≤ 8 := by
  induction bs with
  | nil => intro s hs; exact hs
  | cons x bs ih => intro s _; rw [run_cons]; exact ih _ (step_le8 _ _)

theorem stepOK_at (s x : Nat) (hs : s < 8) (hx : x < 256) :
    (ValidExact.step s x == 8 ||
      (if s = 0 then clsB x != .C && remS (ValidExact.step s x) == (clsB x).len - 1
       else clsB x == .C && remS (ValidExact.step s x) + 1 == remS s)) = true := by
  have := List.all_eq_true.mp step_ok s (List.mem_range.mpr hs)
  exact List.all_eq_true.mp this x (List.mem_range.mpr hx)

theorem remS_pos (s : Nat) (h1 : s ≠ 0) (h2 : s < 8) : remS s ≠ 0 := by
  have : s ∈ [1, 2, 3, 4, 5, 6, 7] := by simp; omega
  simp at this; rcases this with h | h | h | h | h | h | h <;> subst h <;> decide

theorem remS_zero (s : Nat) (h1 : remS s = 0) (h2 : s < 8) : s = 0 := by
  by_cases h : s = 0
  · exact h
  · exact absurd h1 (remS_pos s h h2)

theorem validWin_at {c3 c2 c1 : Cls} (h : cons3 c3 c2 c1 = true) (c0 : Cls) :
    let r := remW c1 c2 c3
    (mcW c3 c2 c1 == (r != 0)) = true ∧
    (c0 == .Z || ((r == 0) != (c0 != .C)) ||
      (chkW c3 c2 c1 c0 && remW c0 c1 c2 == (if r = 0 then c0.len - 1 else r - 1))) = true ∧
    (r != 0 || (!mcW c2 c1 .Z && !mcW c1 .Z .Z)) = true := by
  have := List.all_eq_true.mp validWin_ok _ (cons3_mem h)
  have := List.all_eq_true.mp this c0 (mem_allCls c0)
  simp only [Bool.and_eq_true] at this
  exact ⟨this.1.1, this.1.2, this.2⟩

theorem validWin_step {c3 c2 c1 c0 : Cls} (h : cons3 c3 c2 c1 = true) (hz : c0 ≠ .Z)
    (hrc : remW c1 c2 c3 = 0 ↔ c0 ≠ .C) :
    chkW c3 c2 c1 c0 = true ∧
      remW c0 c1 c2 = (if remW c1 c2 c3 = 0 then c0.len - 1 else remW c1 c2 c3 - 1) := by
  obtain ⟨-, hw2, -⟩ := validWin_at h c0
  have e1 : (c0 == .Z) = false := by simpa using hz
  have e2 : ((remW c1 c2 c3 == 0) != (c0 != .C)) = false := by
    by_cases hr : remW c1 c2 c3 = 0 <;> by_cases hc : c0 = .C <;> simp_all
  rw [e1, e2] at hw2
  simp only [Bool.false_or, Bool.and_eq_true, beq_iff_eq] at hw2
  exact hw2

theorem valid_inv (h64 : blk.length ≤ 64) (hv : ∀ p, p ≤ blk.length → run 0 (blk.take p) ≠ 8) :
    ∀ p, p ≤ blk.length →
      cons3 (clm blk p 3) (clm blk p 2) (clm blk p 1) = true ∧
      remS (run 0 (blk.take p)) = remW (clm blk p 1) (clm blk p 2) (clm blk p 3) ∧
      ∀ q, q < p → chkW (clm blk q 3) (clm blk q 2) (clm blk q 1) (cls blk q) = true := by
  intro p
  induction p with
  | zero => intro _; simp [clm, cons3_start, remW, run, remS, Cls.len]
  | succ p ih =>
    intro hp
    obtain ⟨hc, hr, hq⟩ := ih (by omega)
    have hp' : p < blk.length := by omega
    have e1 := run_take_succ 0 blk p hp'
    generalize hs : run 0 (blk.take p) = s at hr e1
    have hs8 : s ≠ 8 := by rw [← hs]; exact hv p (by omega)
    have hs8' : s < 8 := by have := run_le8 (blk.take p) 0 (by omega); omega
    have hn8 : ValidExact.step s (blk.getD p 0) ≠ 8 := by rw [← e1]; exact hv (p + 1) hp
    have hx : blk.getD p 0 < 256 := by
      by_cases h : blk.getD p 0 < 256
      · exact h
      · exact absurd (step_big s _ (by omega)) hn8
    have hst := stepOK_at s _ hs8' hx
    have hb8 : (ValidExact.step s (blk.getD p 0) == 8) = false := by simpa using hn8
    simp only [hb8, Bool.false_or] at hst
    have hcp : cls blk p = clsB (blk.getD p 0) := cls_lt _ _ hp'
    have s1 : clm blk (p + 1) 3 = clm blk p 2 := clm_succ _ _ _
    have s2 : clm blk (p + 1) 2 = clm blk p 1 := clm_succ _ _ _
    have s3 : clm blk (p + 1) 1 = cls blk p := by rw [clm_succ, clm_zero]
    have hz : cls blk p ≠ .Z := by rw [hcp]; exact clsB_ne_Z _
    have key : chkW (clm blk p 3) (clm blk p 2) (clm blk p 1) (cls blk p) = true ∧
        remW (cls blk p) (clm blk p 1) (clm blk p 2) = remS (ValidExact.step s (blk.getD p 0)) := by
      by_cases h0 : s = 0
      · subst h0
        rw [ite_eq_left rfl] at hst
        simp only [Bool.and_eq_true, bne_iff_ne, ne_eq, beq_iff_eq, ← hcp] at hst
        have hr0 : remW (clm blk p 1) (clm blk p 2) (clm blk p 3) = 0 := by rw [← hr]; rfl
        have := validWin_step hc hz (by rw [hr0]; simp [hst.1])
        rw [hr0, ite_eq_left rfl] at this
        exact ⟨this.1, by rw [this.2, hst.2]⟩
      · rw [ite_eq_right h0] at hst
        simp only [Bool.and_eq_true, beq_iff_eq, ← hcp] at hst
        have hrp := remS_pos s h0 hs8'
        rw [hr] at hrp
        have := validWin_step hc hz (by simp [hrp, hst.1])
        rw [ite_eq_right hrp] at this
        exact ⟨this.1, by rw [this.2, ← hr]; omega⟩
    refine ⟨?_, ?_, ?_⟩
    · rw [s1, s2, s3]; exact cons3_step hc hz key.1
    · rw [s1, s2, s3, e1, key.2]
    · intro q hq'
      by_cases hqp : q < p
      · exact hq q hqp
      · rw [show q = p by omega]; exact key.1

/-- **Valid UTF-8 needs no check.** A block of a valid UTF-8 input that starts
on a character boundary (every prefix of the block is a prefix of valid UTF-8,
and a tail block is complete) passes the structure check and holds no
`0xC0`/`0xC1`. -/
theorem struct_of_valid (h64 : blk.length ≤ 64)
    (hv : ∀ p, p ≤ blk.length → run 0 (blk.take p) ≠ 8)
    (hend : blk.length < 64 → run 0 blk = 0) :
    Struct blk ∧ anyB (milltwo blk) = false := by
  have hI := valid_inv h64 hv
  constructor
  · unfold Struct chkB
    rw [List.all_eq_true]; intro q hq
    have hq' := List.mem_range.mp hq
    have hc := struct_at_iff blk q hq'
    rw [hc]
    by_cases hql : q < blk.length
    · exact (hI blk.length (Nat.le_refl _)).2.2 q hql
    · have hl : blk.length < 64 := by omega
      obtain ⟨hc3, hr3, -⟩ := hI blk.length (Nat.le_refl _)
      have hr0 : remW (clm blk blk.length 1) (clm blk blk.length 2) (clm blk blk.length 3) = 0 := by
        rw [← hr3, List.take_of_length_le (Nat.le_refl _), hend hl]; rfl
      obtain ⟨w1, -, w3⟩ := validWin_at hc3 .Z
      rw [hr0] at w1 w3
      simp only [bne_self_eq_false, Bool.false_or, Bool.and_eq_true, Bool.not_eq_true',
        beq_iff_eq] at w1 w3
      have hZ : ∀ j, blk.length ≤ j → cls blk j = .Z := fun j hj => cls_ge _ _ hj
      rw [hZ q (by omega)]
      simp only [chkW, Cls.beq_def, decide_false, beq_iff_eq]
      have a1 : clm blk (blk.length + 1) 1 = .Z := by
        rw [clm_succ, clm_zero, hZ _ (Nat.le_refl _)]
      have a2 : clm blk (blk.length + 1) 2 = clm blk blk.length 1 := clm_succ _ _ _
      have a3 : clm blk (blk.length + 1) 3 = clm blk blk.length 2 := clm_succ _ _ _
      have b1 : clm blk (blk.length + 2) 1 = .Z := by
        rw [show blk.length + 2 = blk.length + 1 + 1 from rfl, clm_succ, clm_zero, hZ _ (by omega)]
      have b2 : clm blk (blk.length + 2) 2 = .Z := by
        rw [show blk.length + 2 = blk.length + 1 + 1 from rfl]
        exact (clm_succ blk (blk.length + 1) 1).trans a1
      have b3 : clm blk (blk.length + 2) 3 = clm blk blk.length 1 := by
        rw [show blk.length + 2 = blk.length + 1 + 1 from rfl]
        exact (clm_succ blk (blk.length + 1) 2).trans a2
      by_cases e0 : q = blk.length
      · subst e0; exact w1
      by_cases e1 : q = blk.length + 1
      · subst e1; rw [a1, a2, a3]; exact w3.1
      by_cases e2 : q = blk.length + 2
      · subst e2; rw [b1, b2, b3]; exact w3.2
      simp only [clm, show 3 ≤ q by omega, show 2 ≤ q by omega, show 1 ≤ q by omega, ite_true]
      rw [hZ _ (by omega), hZ _ (by omega), hZ _ (by omega)]; rfl
  · unfold anyB; rw [List.any_eq_false]; intro p hp
    have hp' := List.mem_range.mp hp
    simp only [milltwo, m234, Bool.and_eq_true, decide_eq_true_eq]
    intro ⟨⟨hl, h1⟩, h2⟩
    have h8 := hv (p + 1) (by omega)
    rw [run_take_succ 0 blk p hl] at h8
    have hb : blk.getD p 0 = 0xC0 ∨ blk.getD p 0 = 0xC1 := by unfold byte at h1 h2; omega
    have hs := run_le8 (blk.take p) 0 (by omega)
    generalize run 0 (blk.take p) = s at h8 hs
    have : s ∈ List.range 9 := List.mem_range.mpr (by omega)
    rcases hb with hb | hb <;> rw [hb] at h8 <;> revert h8 <;> revert s <;> decide

end Valid

/-! ## The drivers

src/icelake/icelake_convert_utf8_to_utf16.inl.cpp. The input is the byte list
`xs`; the block at `pos` sees `(xs.drop pos).take 64`: `SIMDUTF_FULL` when
`final_in - in >= 64` (lines 23, 51), `SIMDUTF_TAIL` with `gap = final_in - in`
otherwise (lines 26-28, 54-56). The model returns the furthest store end
(`wEnd`, in units from the start of the output) and the furthest load end
(`rEnd`, in bytes). -/

structure Run where
  pos : Nat
  out : Nat
  ok : Bool
  wEnd : Nat
  rEnd : Nat
deriving Repr

def view (xs : List Nat) (pos : Nat) : List Nat := (xs.drop pos).take 64

/-- `fast_avx512_convert_utf8_to_utf16<_, validate>` (lines 16-40) for `v =
validate`, and the loop of `_with_errors` (lines 49-60) for `v = true`, from
input position `pos` and output cursor `out`, with fuel `f` (the number of
blocks). A failing block leaves `in` and `out` unchanged (line 36: `out =
nullptr`); with `validate = false`, a block that does not advance stops the
loop after its store (lines 32-34). The full block loads 64 bytes and reads
the byte `in + 32` inside them (src/icelake/icelake_utf8_common.inl.cpp:398); the tail
loads `gap` bytes, masked. -/
def run16 (v : Bool) (xs : List Nat) : Nat → Nat → Nat → Run
  | 0, pos, out => ⟨pos, out, true, out, pos⟩
  | f + 1, pos, out =>
    if pos < xs.length then
      match block16 v (view xs pos) with
      | none => ⟨pos, out, false, out, pos + (view xs pos).length⟩
      | some (c, n) =>
        if v = false ∧ c = 0 then ⟨pos, out + n, false, out + n, pos + (view xs pos).length⟩
        else
          let r := run16 v xs f (pos + c) (out + n)
          ⟨r.pos, r.out, r.ok, max (out + n) r.wEnd, max (pos + (view xs pos).length) r.rEnd⟩
    else ⟨pos, out, true, out, pos⟩

/-- The search of `rewind_and_convert_with_errors` for a byte that is not a
continuation byte, `i = 0, 1, ..., how_far_back`
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:308-321,
include/simdutf/scalar/utf8_to_utf32/utf8_to_utf32.h:258-271). -/
def rwSearch (xs : List Nat) (pos : Nat) : Nat → Nat → Option Nat
  | i, rem =>
    if isCont (xs.getD (pos - i) 0) = false then some i
    else match rem with
      | 0 => none
      | r + 1 => rwSearch xs pos (i + 1) r

theorem rwSearch_succ (xs : List Nat) (pos i r : Nat) :
    rwSearch xs pos i (r + 1) =
      if isCont (xs.getD (pos - i) 0) = false then some i else rwSearch xs pos (i + 1) r := rfl

theorem rwSearch_eq (xs : List Nat) (pos i : Nat) (h : isCont (xs.getD (pos - i) 0) = false)
    (r : Nat) : rwSearch xs pos i r = some i := by
  unfold rwSearch; rw [ite_eq_left h]

/-- Units stored by `rewind_and_convert_with_errors` called at `pos` with
`how_far_back = hfb`: none if no leading byte is found or if it is ASCII and
`i > 0`; otherwise the scalar `convert_with_errors` from `pos - i`
(utf8_to_utf16.h:296-342: `how_far_back = prior_bytes`; utf8_to_utf32.h:247-293:
`min(3, prior_bytes)`). -/
def rewind (k : SK) (xs : List Nat) (pos hfb : Nat) : Nat :=
  match rwSearch xs pos 0 hfb with
  | none => 0
  | some i => if 0 < i ∧ xs.getD (pos - i) 0 < 128 then 0 else sc8 k xs.length (xs.drop (pos - i))

/-- The test of `_with_errors` before the rewind: a fourth continuation byte
returns `TOO_LONG` with no store (icelake_convert_utf8_to_utf16.inl.cpp:63-71). -/
def guard (xs : List Nat) (pos : Nat) : Bool :=
  decide (pos < xs.length) && isCont (xs.getD pos 0) && decide (64 ≤ pos) &&
    isCont (xs.getD (pos - 1) 0) && isCont (xs.getD (pos - 2) 0) && isCont (xs.getD (pos - 3) 0)

/-- `convert_utf8_to_utf16{le,be}` (src/icelake/implementation.cpp:1103-1122):
the furthest store end. A failing block returns 0 after the stores already
made. -/
def conv16 (xs : List Nat) (f : Nat) : Nat := (run16 true xs f 0 0).wEnd

/-- `convert_utf8_to_utf16{le,be}_with_errors` (implementation.cpp:1124-1134,
icelake_convert_utf8_to_utf16.inl.cpp:42-84): on failure at `in`, the guard,
then `rewind_and_convert_with_errors(in - init_in, in, final_in - in, out)`
stores at the cursor. -/
def conv16err (xs : List Nat) (f : Nat) : Nat :=
  let r := run16 true xs f 0 0
  if r.ok then r.wEnd
  else if guard xs r.pos then r.wEnd
  else max r.wEnd (r.out + rewind .u16 xs r.pos r.pos)

/-- `convert_valid_utf8_to_utf16{le,be}` (implementation.cpp:1136-1158). -/
def conv16valid (xs : List Nat) (f : Nat) : Nat := (run16 false xs f 0 0).wEnd

/-- Where a block may start for the error path to be safe (`EndOK` of the
block before). -/
def Good (xs : List Nat) (pos : Nat) : Prop :=
  pos = 0 ∨ xs.length ≤ pos ∨ isCont (xs.getD pos 0) = false ∨
    (1 ≤ pos ∧ xs.getD (pos - 1) 0 < 0x80) ∨
    (64 ≤ pos ∧ isCont (xs.getD (pos - 1) 0) = true ∧ isCont (xs.getD (pos - 2) 0) = true ∧
      isCont (xs.getD (pos - 3) 0) = true)

section Drivers
variable {xs : List Nat}

theorem view_length (pos : Nat) : (view xs pos).length = min 64 (xs.length - pos) := by
  simp [view] <;> omega

theorem view_byte (pos i : Nat) (hi : i < (view xs pos).length) :
    byte (view xs pos) i = xs.getD (pos + i) 0 := by
  rw [view_length] at hi
  simp only [byte, view, List.getD_eq_getElem?_getD, List.getElem?_take, List.getElem?_drop]
  simp [show i < 64 by omega]

theorem view_take (pos c : Nat) (hc : c ≤ 64) : (view xs pos).take c = (xs.drop pos).take c := by
  simp only [view, List.take_take]; congr 1; omega

theorem Lt_add (f : Nat → Nat) (pos c : Nat) :
    L ((xs.take (pos + c)).map f) = L ((xs.take pos).map f) + L (((xs.drop pos).take c).map f) := by
  rw [List.take_add, List.map_append, L_append]

theorem Lt_take_view {pos : Nat} (hb : Bd xs pos) :
    run 0 (xs.take (pos + 64)) = run 0 ((view xs pos).take 64) := by
  rw [List.take_add, run_append, hb, view_take _ _ (Nat.le_refl _)]

theorem good_of_end (pos c : Nat) (hc : c ≤ (view xs pos).length) (h : EndOK (view xs pos) c) :
    Good xs (pos + c) := by
  have hl := view_length (xs := xs) pos
  by_cases h1 : c < (view xs pos).length
  · right; right; left
    rw [← view_byte pos c h1]; exact h.1 h1
  · by_cases h64 : (view xs pos).length = 64
    · have hc64 : c = 64 := by omega
      rcases h.2 hc64 with ha | ⟨-, a1, a2, a3⟩
      · right; right; right; left
        rw [view_byte pos 63 (by omega)] at ha
        refine ⟨by omega, ?_⟩
        rw [show pos + c - 1 = pos + 63 by omega]; exact ha
      · right; right; right; right
        rw [view_byte pos 61 (by omega)] at a1
        rw [view_byte pos 62 (by omega)] at a2
        rw [view_byte pos 63 (by omega)] at a3
        refine ⟨by omega, ?_, ?_, ?_⟩
        · rw [show pos + c - 1 = pos + 63 by omega]; exact a3
        · rw [show pos + c - 2 = pos + 62 by omega]; exact a2
        · rw [show pos + c - 3 = pos + 61 by omega]; exact a1
    · right; left; omega

/-- The whole validating loop: every store ends inside `utf16_length_from_utf8`
of the input, and the state it leaves is one the error path can resume from. -/
theorem run16_true : ∀ f pos out, Good xs pos → out ≤ L ((xs.take pos).map w16) →
    pos ≤ xs.length →
    (run16 true xs f pos out).wEnd ≤ L (xs.map w16) ∧
    (run16 true xs f pos out).out ≤ L ((xs.take (run16 true xs f pos out).pos).map w16) ∧
    Good xs (run16 true xs f pos out).pos ∧
    ((run16 true xs f pos out).ok = false → (run16 true xs f pos out).pos < xs.length) ∧
    (run16 true xs f pos out).rEnd ≤ xs.length := by
  intro f
  induction f with
  | zero =>
    intro pos out hg ho hp
    have := L_take_le (xs.map w16) pos
    rw [← List.map_take] at this
    exact ⟨Nat.le_trans ho this, ho, hg, by simp [run16], hp⟩
  | succ f ih =>
    intro pos out hg ho hp
    have hT := L_take_le (xs.map w16) pos
    rw [← List.map_take] at hT
    have hl := view_length (xs := xs) pos
    unfold run16
    by_cases hlt : pos < xs.length
    · rw [ite_eq_left hlt]
      cases hb : block16 true (view xs pos) with
      | none => simp only; exact ⟨by omega, ho, hg, fun _ => hlt, by omega⟩
      | some cn =>
        obtain ⟨c, n⟩ := cn
        obtain ⟨hb', hS, hm⟩ := block16_true (by omega) hb
        obtain ⟨hc, hn, he⟩ := block16_core (by omega) hS hm hb'
        simp only [Bool.true_eq_false, false_and, ite_false]
        rw [view_take _ _ (by omega)] at hn
        have hsplit := Lt_add (xs := xs) w16 pos c
        have hr := ih (pos + c) (out + n) (good_of_end pos c hc he) (by omega) (by omega)
        refine ⟨Nat.max_le.mpr ⟨?_, hr.1⟩, hr.2.1, hr.2.2.1, hr.2.2.2.1,
          Nat.max_le.mpr ⟨by omega, hr.2.2.2.2⟩⟩
        have := L_take_le (xs.map w16) (pos + c)
        rw [← List.map_take] at this
        omega
    · rw [ite_eq_right hlt]; exact ⟨Nat.le_trans ho hT, ho, hg, by simp, hp⟩

/-- **`convert_utf8_to_utf16{le,be}` on Ice Lake**: on every input, no store
ends past `utf16_length_from_utf8` of the input, and no load reads past it. -/
theorem conv16_safe (f : Nat) :
    conv16 xs f ≤ L (xs.map w16) ∧ (run16 true xs f 0 0).rEnd ≤ xs.length := by
  have := run16_true (xs := xs) f 0 0 (Or.inl rfl) (by simp) (Nat.zero_le _)
  exact ⟨this.1, this.2.2.2.2⟩

/-- The error path: from a block start allowed by `Good`, the rewind decodes
only from a byte that the vector loop has not converted, or stores nothing. -/
theorem rewind16_safe (pos out : Nat) (hg : Good xs pos) (hp : pos < xs.length)
    (hgu : guard xs pos = false) (ho : out ≤ L ((xs.take pos).map w16)) :
    out + rewind .u16 xs pos pos ≤ L (xs.map w16) := by
  have hsplit := L_take_add_drop pos (xs.map w16)
  rw [← List.map_take, ← List.map_drop] at hsplit
  have hsc := sc8_le .u16 xs.length (xs.drop pos)
  simp only [wOf] at hsc
  unfold rewind
  by_cases hc : isCont (xs.getD pos 0) = false
  · have : rwSearch xs pos 0 pos = some 0 := by
      unfold rwSearch; rw [ite_eq_left (by rw [Nat.sub_zero]; exact hc)]
    rw [this]; simp only [Nat.lt_irrefl, false_and, ite_false, Nat.sub_zero]; omega
  · have hc' : isCont (xs.getD pos 0) = true := by simpa using hc
    rcases hg with h0 | hl | h | ⟨h1, ha⟩ | ⟨h64, a1, a2, a3⟩
    · subst h0
      have : rwSearch xs 0 0 0 = none := by
        unfold rwSearch; rw [ite_eq_right (by rw [Nat.sub_zero, hc']; decide)]
      rw [this]; simp only [Nat.add_zero]
      have := L_take_le (xs.map w16) 0; rw [← List.map_take] at this; omega
    · omega
    · exact absurd h hc
    · obtain ⟨r, hr⟩ : ∃ r, pos = r + 1 := ⟨pos - 1, by omega⟩
      have hna : isCont (xs.getD (pos - 1) 0) = false := by
        unfold isCont; simp only [Bool.and_eq_false_iff, decide_eq_false_iff_not]; omega
      have : rwSearch xs pos 0 pos = some 1 := by
        subst hr
        rw [rwSearch_succ, Nat.sub_zero, ite_eq_right (by rw [hc']; decide)]
        exact rwSearch_eq _ _ _ hna r
      rw [this]
      simp only [show 0 < 1 from Nat.one_pos, true_and, ha, ite_true, Nat.add_zero]
      have := L_take_le (xs.map w16) pos; rw [← List.map_take] at this; omega
    · have : guard xs pos = true := by
        unfold guard
        simp only [Bool.and_eq_true, decide_eq_true_eq]
        exact ⟨⟨⟨⟨⟨hp, hc'⟩, h64⟩, a1⟩, a2⟩, a3⟩
      rw [this] at hgu; exact absurd hgu (by decide)

/-- **`convert_utf8_to_utf16{le,be}_with_errors` on Ice Lake**: on every
input, no store ends past `utf16_length_from_utf8` of the input, the vector
loop and the scalar rewind included. -/
theorem conv16err_safe (f : Nat) : conv16err xs f ≤ L (xs.map w16) := by
  have h := run16_true (xs := xs) f 0 0 (Or.inl rfl) (by simp) (Nat.zero_le _)
  unfold conv16err
  simp only
  split
  · exact h.1
  split
  · exact h.1
  · rename_i hok hgu
    have hpos := h.2.2.2.1 (by simpa using hok)
    exact Nat.max_le.mpr ⟨h.1, rewind16_safe _ _ h.2.2.1 hpos (by simpa using hgu) h.2.1⟩

/-- **The guard is needed.** Sixteen four-byte characters fill the first
block exactly (32 units, the last one a low surrogate, so `Mout` keeps it and
the block consumes 64 bytes), then a continuation byte makes the next block
fail at byte 64. The rewind of `utf8_to_utf16.h` searches back as far as
`prior_bytes` and would find the lead at byte 60 and decode that character
again at the cursor: 2 units past `utf16_length_from_utf8`. The fourth-
continuation test before the rewind (icelake_convert_utf8_to_utf16.inl.cpp:63-71)
returns first. -/
theorem guard_needed :
    let xs := List.flatten (List.replicate 16 [0xF0, 0x9F, 0x98, 0x80]) ++ [0x80]
    (run16 true xs 2 0 0).ok = false ∧ (run16 true xs 2 0 0).pos = 64 ∧ guard xs 64 = true ∧
      (run16 true xs 2 0 0).out + rewind .u16 xs 64 64 = L (xs.map w16) + 2 := by
  decide +kernel

/-! ### `convert_valid` -/

theorem block16_false_some (blk : List Nat) : ∃ c n, block16 false blk = some (c, n) := by
  unfold block16
  simp only [Bool.false_and, Bool.false_eq_true, ite_false]
  split
  · exact ⟨_, _, rfl⟩
  split
  · split
    · exact ⟨_, _, rfl⟩
    · exact ⟨_, _, rfl⟩
  split
  · exact ⟨_, _, rfl⟩
  · exact ⟨_, _, rfl⟩

theorem view_valid (hv : Valid8 xs) {pos : Nat} (hb : Bd xs pos) :
    (∀ p, p ≤ (view xs pos).length → run 0 ((view xs pos).take p) ≠ 8) ∧
    ((view xs pos).length < 64 → run 0 (view xs pos) = 0) := by
  have hd := valid_drop hv hb
  constructor
  · intro p hp
    rw [view_take _ _ (by rw [view_length] at hp; omega)]
    exact run_take_ne8 hd p
  · intro h
    have : view xs pos = xs.drop pos := by
      rw [view_length] at h; simp only [view]; apply List.take_of_length_le; simp; omega
    rw [this]; exact hd

theorem bd_of_end (hv : Valid8 xs) {pos c : Nat} (hb : Bd xs pos) (hp : pos < xs.length)
    (hc : c ≤ (view xs pos).length) (h : EndOK (view xs pos) c) : Bd xs (pos + c) := by
  have hl := view_length (xs := xs) pos
  by_cases h1 : c < (view xs pos).length
  · apply lead_bd hv (by omega)
    rw [← view_byte pos c h1]; exact h.1 h1
  by_cases hend : pos + c = xs.length
  · unfold Bd; rw [hend, List.take_of_length_le (Nat.le_refl _)]; exact hv
  have hc64 : c = 64 := by omega
  -- the state after the 64 bytes is 0
  obtain ⟨hv1, -⟩ := view_valid hv hb
  have hI := (valid_inv (blk := view xs pos) (by omega) hv1) 64 (by omega)
  obtain ⟨hcons, hrem, -⟩ := hI
  have e1 : clm (view xs pos) 64 1 = cls (view xs pos) 63 := by simp [clm]
  have e2 : clm (view xs pos) 64 2 = cls (view xs pos) 62 := by simp [clm]
  have e3 : clm (view xs pos) 64 3 = cls (view xs pos) 61 := by simp [clm]
  rw [e1, e2, e3] at hcons hrem
  have hw : remW (cls (view xs pos) 63) (cls (view xs pos) 62) (cls (view xs pos) 61) = 0 := by
    rcases h.2 hc64 with ha | ⟨-, a1, a2, a3⟩
    · have hA : cls (view xs pos) 63 = .A := by
        rw [cls_lt _ _ (by omega)]
        rcases clsB_eq (byte (view xs pos) 63) with ⟨_, e⟩ | ⟨h1, _⟩ | ⟨h1, _⟩ | ⟨h1, _⟩ | ⟨h1, _⟩
        · exact e
        all_goals omega
      rw [hA] at hcons ⊢
      have := List.all_eq_true.mp ascii_end_ok _ (cons3_mem hcons)
      simpa using this
    · have hC : ∀ i, i < 64 → isCont (byte (view xs pos) i) = true → cls (view xs pos) i = .C := by
        intro i hi hci
        rw [isCont_cls _ (by omega)] at hci; simpa using hci
      rw [hC 63 (by omega) a3, hC 62 (by omega) a2, hC 61 (by omega) a1]; rfl
  rw [hw] at hrem
  subst hc64
  unfold Bd
  rw [Lt_take_view hb]
  have h8 : run 0 ((view xs pos).take 64) ≠ 8 := hv1 64 (by omega)
  have hle := run_le8 ((view xs pos).take 64) 0 (by omega)
  exact remS_zero _ hrem (by omega)

/-- The `convert_valid` loop on valid UTF-8: blocks start on character
boundaries, and every store ends inside `utf16_length_from_utf8`. -/
theorem run16_valid (hv : Valid8 xs) : ∀ f pos out, Bd xs pos → out ≤ L ((xs.take pos).map w16) →
    pos ≤ xs.length →
    (run16 false xs f pos out).wEnd ≤ L (xs.map w16) ∧ (run16 false xs f pos out).rEnd ≤ xs.length := by
  intro f
  induction f with
  | zero =>
    intro pos out _ ho hp
    have := L_take_le (xs.map w16) pos
    rw [← List.map_take] at this
    exact ⟨Nat.le_trans ho this, hp⟩
  | succ f ih =>
    intro pos out hb ho hp
    have hl := view_length (xs := xs) pos
    unfold run16
    by_cases hlt : pos < xs.length
    · rw [ite_eq_left hlt]
      obtain ⟨hv1, hv2⟩ := view_valid hv hb
      obtain ⟨hS, hm⟩ := struct_of_valid (by omega) hv1 hv2
      obtain ⟨c, n, hbk⟩ := block16_false_some (view xs pos)
      rw [hbk]
      obtain ⟨hc, hn, he⟩ := block16_core (by omega) hS hm hbk
      rw [view_take _ _ (by omega)] at hn
      have hsplit := Lt_add (xs := xs) w16 pos c
      have hT := L_take_le (xs.map w16) (pos + c)
      rw [← List.map_take] at hT
      simp only
      split
      · exact ⟨show out + n ≤ _ by omega, show pos + (view xs pos).length ≤ _ by omega⟩
      · have hr := ih (pos + c) (out + n) (bd_of_end hv hb hlt hc he) (by omega) (by omega)
        exact ⟨Nat.max_le.mpr ⟨by omega, hr.1⟩, Nat.max_le.mpr ⟨by omega, hr.2⟩⟩
    · rw [ite_eq_right hlt]
      have := L_take_le (xs.map w16) pos
      rw [← List.map_take] at this
      exact ⟨Nat.le_trans ho this, hp⟩

/-- **`convert_valid_utf8_to_utf16{le,be}` on Ice Lake**: on valid UTF-8 (the
contract), no store ends past `utf16_length_from_utf8` of the input, and no
load reads past it. -/
theorem conv16valid_safe (hv : Valid8 xs) (f : Nat) :
    conv16valid xs f ≤ L (xs.map w16) ∧ (run16 false xs f 0 0).rEnd ≤ xs.length :=
  run16_valid hv f 0 0 (by simp [Bd, run]) (by simp) (Nat.zero_le _)

end Drivers

end SafetyMargins.IcelakeUtf8

