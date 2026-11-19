import SafetyMargins.Arch.Icelake
/-
Ice Lake real ends, derived from the lane values.

`Arch/Icelake.lean` states `safe_utf16to32_real` and `safe_utf16to8_real`
under two data hypotheses read from the C++:

* UTF-16 → UTF-32: the surrogate iteration runs only when `16 ≤ L` of the 32
  loaded lanes;
* UTF-16 → UTF-8: each `.ahead` iteration writes, past `L` of its 31 consumed
  units, the spill of lane 31, the 2-byte low-surrogate carry, or nothing.

Both theorems take `choose` and `extra` as functions of the weight list. The
kernels branch on the 16-bit values (and on a carry), which the weights do not
determine, so this file models each loop on the values themselves: one
iteration is a function of the 32 loaded lanes and the carry, with the
branch conditions and store sizes transcribed from the C++ (file:line in each
docstring), down to the bytes the `compress` masks keep. It then proves

* that the two hypotheses hold for every lane vector (`utf16to32_hsur`,
  `utf16to8_hreal`), and
* the real-end safety of the whole loop, iteration after iteration, with no
  data hypothesis (`safe_utf16to32_kernel`, `safe_utf16to8_kernel`).

Values are the units after the big-endian byte swap
(icelake_convert_utf16_to_utf8.inl.cpp:26,
icelake_convert_utf16_to_utf32.inl.cpp:148), so both endiannesses are covered. A 16-bit lane `v` is a high surrogate when
`(v & 0xfc00) == 0xd800`, that is `0xD800 ≤ v < 0xDC00` (`hiS`), and a low
surrogate when `(v & 0xfc00) == 0xdc00` (`loS`).
-/
namespace SafetyMargins.IcelakeReal
open SafetyMargins SafetyMargins.Arch.Icelake

/-! ## Lanes -/

/-- `(v & 0xfc00) == 0xd800` on a 16-bit lane. -/
def hiS (v : Nat) : Bool := decide (0xD800 ≤ v ∧ v < 0xDC00)
/-- `(v & 0xfc00) == 0xdc00` on a 16-bit lane. -/
def loS (v : Nat) : Bool := decide (0xDC00 ≤ v ∧ v < 0xE000)

theorem lo_not_hi (v : Nat) (h : loS v = true) : hiS v = false := by
  simp [hiS, loS] at *; omega

def cb (b : Bool) : Nat := if b then 1 else 0

/-- The surrogate check shared by both kernels, lane by lane: lane 0 is low
exactly when `p` (the carry), lane `i + 1` is low exactly when lane `i` is
high. This is `L ^ (carry | H << 1) == 0` on 32-bit masks (the bit of lane 31
of `H` is shifted out), and `(h + h + carry) ^ l == 0` (no overflow: `h < 2^31`). -/
def chk : Bool → List Nat → Bool
  | _, [] => true
  | p, x :: xs => (loS x == p) && chk (hiS x) xs

/-- The predecessor flag after the lanes: whether the last one is high. -/
def endP : Bool → List Nat → Bool
  | p, [] => p
  | _, x :: xs => endP (hiS x) xs

theorem L_map_take (f : Nat → Nat) (l : List Nat) (k : Nat) :
    L ((l.take k).map f) ≤ L (l.map f) := by
  rw [List.map_take]; exact L_take_le _ _

theorem L_map_split (f : Nat → Nat) (l : List Nat) (k : Nat) :
    L (l.map f) = L ((l.take k).map f) + L ((l.drop k).map f) := by
  rw [← L_append, ← List.map_append, List.take_append_drop]

/-! ## UTF-16 → UTF-32

src/icelake/icelake_convert_utf16_to_utf32.inl.cpp:144, `while (end - buf >= 32)`.
Per iteration, on the 32 loaded lanes `l` and the carry `c`:

* `H | L == 0` (line 158 false): no surrogate. Two unmasked 16-unit stores at
  the cursor and the cursor + 16 (lines 249-253): they end at 32. Advance 32
  (line 254), carry 0.
* `V == 0` with `V = L ^ (carry | H << 1)` (line 166): the unmasked store of
  `compressed_first` (16 units, line 227), then a store masked to `howmany2`
  units at the cursor + `howmany1` (line 234), with
  `howmany1 + howmany2 = popcount(~L & 0x7fffffff)`, the non-low lanes among
  lanes 0-30. Advance by that count (lines 228, 237), consume 31 (line 240),
  carry = lane 30 is high (line 241).
* otherwise return (line 244): nothing stored.

`utf32_length_from_utf16` gives a low surrogate 0 and every other unit 1
(`w32v`). Nothing in the iteration depends on the carry except the check. -/

/-- `utf32_length_from_utf16` of one unit. -/
def w32v (v : Nat) : Nat := if loS v then 0 else 1

/-- Number of lanes that are not low surrogates: `popcount` of `~L`. -/
def nonLo : List Nat → Nat
  | [] => 0
  | x :: xs => (if loS x then 0 else 1) + nonLo xs

def nLo : List Nat → Nat
  | [] => 0
  | x :: xs => (if loS x then 1 else 0) + nLo xs

theorem nonLo_eq (l : List Nat) : nonLo l = L (l.map w32v) := by
  induction l with
  | nil => rfl
  | cons x xs ih => simp [nonLo, w32v, ih]

theorem nonLo_add_nLo (l : List Nat) : nonLo l + nLo l = l.length := by
  induction l with
  | nil => rfl
  | cons x xs ih => cases h : loS x <;> simp [nonLo, nLo, h] <;> omega

/-- Under the check, low lanes are never adjacent, so at most half of them
(plus the carried-in one) are low. -/
theorem nLo_chk (p : Bool) (l : List Nat) (h : chk p l = true) :
    2 * nLo l ≤ l.length + cb p := by
  induction l generalizing p with
  | nil => simp [nLo]
  | cons x xs ih =>
    simp only [chk, Bool.and_eq_true, beq_iff_eq] at h
    have ih' := ih _ h.2
    cases hl : loS x
    · have : cb (hiS x) ≤ 1 := by cases hiS x <;> simp [cb]
      simp [nLo, hl]; omega
    · have hh := lo_not_hi x hl
      rw [hh] at ih'
      have hp : p = true := by rw [← h.1, hl]
      subst hp
      simp [nLo, hl, cb] at ih' ⊢; omega

/-- **The surrogate iteration's hypothesis.** When the check `V == 0` passes
on 32 lanes (whatever the carry), at least 16 of them are not low surrogates:
the unmasked 16-unit store ends inside `L` of the 32 loaded lanes. -/
theorem sur16 (c : Bool) (l : List Nat) (hl : l.length = 32) (h : chk c l = true) :
    16 ≤ L (l.map w32v) := by
  have h1 := nLo_chk c l h
  have h2 := nonLo_add_nLo l
  have : cb c ≤ 1 := by cases c <;> simp [cb]
  rw [← nonLo_eq]; omega

/-- No surrogate among the lanes: every one weighs 1. -/
theorem nosur_L (l : List Nat) (h : l.any (fun v => hiS v || loS v) = false) :
    L (l.map w32v) = l.length := by
  induction l with
  | nil => rfl
  | cons x xs ih =>
    simp only [List.any_cons, Bool.or_eq_false_iff] at h
    simp [w32v, h.1.2, ih h.2]; omega

/-- The loop, on the units `us` not yet consumed, with carry `c`: the furthest
end of a store, from the cursor. After the loop (fewer than 32 units left) or
when the fuel is spent, the rest is charged as exact, as in `realWritten32`
(`convert_utf16_to_utf32_masked`, line 3, uses masked stores of the produced
count; the scalar tail writes one unit per non-low unit). The masked tail and
the whole function are transcribed and proved in `IcelakeUtf32Tail`
(`safe_utf16to32_full`), with no such charge. -/
def run32 : Nat → Bool → List Nat → Nat
  | 0, _, us => L (us.map w32v)
  | n + 1, c, us =>
    if us.length < 32 then L (us.map w32v)
    else
      let l := us.take 32
      if l.any (fun v => hiS v || loS v) then
        if chk c l then
          let adv := nonLo (l.take 31)
          max (max 16 adv) (adv + run32 n (hiS (us.getD 30 0)) (us.drop 31))
        else 0
      else max 32 (32 + run32 n false (us.drop 32))

/-- `hsur` of `safe_utf16to32_real`, for every lane vector that takes the
surrogate iteration (`H | L ≠ 0` and `V == 0`): `16 ≤ L` of the 32 loaded
lanes. Whatever the carry. -/
theorem utf16to32_hsur (c : Bool) (us : List Nat) (h32 : 32 ≤ us.length)
    (hv : chk c (us.take 32) = true) :
    16 ≤ L ((us.map w32v).take 32) := by
  rw [← List.map_take]
  exact sur16 c _ (by simp; omega) hv

/-- The surrogate iteration's store end is `surrogateEnd` of the weights, the
real end that `safe_utf16to32_real` bounds. -/
theorem run32_surrogateEnd (us : List Nat) :
    max 16 (nonLo ((us.take 32).take 31)) = surrogateEnd (us.map w32v) := by
  simp [surrogateEnd, nonLo_eq, List.take_take, List.map_take]

/-- **UTF-16 → UTF-32 on Ice Lake, real ends, no data hypothesis.** For every
input and carry, no store of any iteration ends past `utf32_length_from_utf16`
of the input. -/
theorem safe_utf16to32_kernel :
    ∀ n c us, run32 n c us ≤ L (us.map w32v) := by
  intro n
  induction n with
  | zero => intro c us; simp [run32]
  | succ n ih =>
    intro c us
    unfold run32
    by_cases hs : us.length < 32
    · rw [ite_eq_left hs]; exact Nat.le_refl _
    · rw [ite_eq_right hs]
      have h32 : 32 ≤ us.length := by omega
      simp only
      by_cases ha : (us.take 32).any (fun v => hiS v || loS v) = true
      · rw [ite_eq_left ha]
        by_cases hc : chk c (us.take 32) = true
        · rw [ite_eq_left hc]
          have h16 := utf16to32_hsur c us h32 hc
          have hadv : nonLo ((us.take 32).take 31) = L ((us.take 31).map w32v) := by
            rw [nonLo_eq, List.take_take]; rfl
          have hsplit := L_map_split w32v us 31
          have h31 := L_take_le (us.map w32v) 32
          have hr := ih (hiS (us.getD 30 0)) (us.drop 31)
          rw [hadv]
          omega
        · rw [ite_eq_right hc]; exact Nat.zero_le _
      · rw [ite_eq_right ha]
        have hl := nosur_L (us.take 32) (by simpa using ha)
        have hlen : (us.take 32).length = 32 := by simp; omega
        have hsplit := L_map_split w32v us 32
        have hr := ih false (us.drop 32)
        omega

/-! ## UTF-16 → UTF-8

src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:23, `while (inlen >= 32)`.
Each iteration loads 32 lanes and consumes 31 (`inmask = 0x7fffffff`, line 13;
`inbuf += 31`, line 30). From the label `failiteration` (line 32), with the
active lanes `a` (those of `inmask`) and the inactive ones `z`:

* **A** (line 36): every active lane `< 0x80`. `_mm512_mask_cvtepi16_storeu_epi8`
  under `inmask` stores one byte per active lane (line 38); advance 31.
* **B** (line 52, `_ktestc`: `~is12byte & inmask == 0`): every active lane
  `< 0x800`. Line 58 rewrites each active lane `≥ 0x80` as the two bytes
  `0xc0 | x >> 6`, `0x80 | x & 0x3f`; `smoosh` (line 63) keeps a byte that is
  `≥` its byte of `cmpmask` (line 60): `0x00`, `0x08` on active lanes, `0xff`,
  `0xff` on inactive ones. The store is masked to `popcount(smoosh)` bytes
  (lines 66-69); advance `31 + popcount(is234byte)` (line 70).
* **C** (otherwise): the surrogate check (line 117) runs when some active lane
  is high or some lane is low (line 93; `losurr` is not masked by `inmask`).
  On a mismatch, `failiteration` again with the lanes before the first error
  (`tzcnt(hinolo | lonohi)`, lines 118-125), then the loop ends. Otherwise
  two stores masked to `popcount(wantlo)` and `popcount(wanthi)` bytes, end to
  end (lines 183-187); advance by their sum (line 188); carry = active lane 30
  is high (lines 112, 130).

`utf8_length_from_utf16` is `w8`. -/

/-- Bytes kept by an unsigned byte comparison `bytes ≥ cmp` (`_mm512_cmp_epu8_mask`
with `_MM_CMPINT_NLT`), counted. -/
def cntGe : List Nat → List Nat → Nat
  | b :: bs, m :: ms => (if m ≤ b then 1 else 0) + cntGe bs ms
  | _, _ => 0

/-! ### Path B -/

/-- Line 58: an active lane `x ≥ 0x80` (here `x < 0x800`) becomes
`((x << 8) | (x >> 6)) & 0x3f3f` plus `0x80c0`: low byte `0xc0 + (x >> 6 & 0x3f)`,
high byte `0x80 + (x & 0x3f)` (no carry between the bytes). -/
def twoB (x : Nat) : Nat :=
  if x < 0x80 then x else (x % 64 + 0x80) * 256 + (x / 64 % 64 + 0xc0)

/-- Bytes of an active lane kept by `smoosh` (cmpmask `0x0800`: bytes `0x00`, `0x08`). -/
def laneB (x : Nat) : Nat := cntGe [twoB x % 256, twoB x / 256] [0x00, 0x08]

/-- Bytes of an inactive lane (raw value) kept by `smoosh` (cmpmask `0xffff`). -/
def laneBz (y : Nat) : Nat := cntGe [y % 256, y / 256] [0xff, 0xff]

theorem laneB_eq (x : Nat) (hx : x < 0x800) : laneB x = w8 x := by
  simp only [laneB, twoB, cntGe, w8]
  repeat' split
  all_goals omega

theorem laneBz_eq (y : Nat) (hy : y < 65536) : laneBz y = spill y := by
  simp only [laneBz, cntGe, spill]
  repeat' split
  all_goals omega

/-! ### Path C -/

/-- `_mm512_multishift_epi64_epi8` with control `0x20262c3200060c12` (lines
132-136) puts in byte `j` of each dword the 8 bits from offset 18, 12, 6, 0
(32 more for the high dword); the ternary logic `0xea` (lines 163-166) keeps
6 of them (`& 0x3f`) and ors the tag byte. -/
def fld (d s : Nat) : Nat := d / 2 ^ s % 64

def bytes4 (d t0 t1 t2 t3 : Nat) : List Nat :=
  [t0 ||| fld d 18, t1 ||| fld d 12, t2 ||| fld d 6, t3 ||| fld d 0]

/-- The four bytes of the dword of an active lane `x`, `nx` the next lane.
* `x < 0x80` (`is1byte`, line 167): `lo << 24`, bytes `0, 0, 0, x`.
* `x < 0x800` (`is12byte`, tag `0x80c00000`, line 145).
* high surrogate: the dword is `(x << 10) + nx + 0xfca02400` (lines 105-110,
  32-bit), tag `0x808080f0` (line 100).
* otherwise tag `0x8080e000` (line 82).
Tags in little-endian byte order. -/
def bytesC (x nx : Nat) : List Nat :=
  if x < 0x80 then [0, 0, 0, x]
  else if x < 0x800 then bytes4 x 0x00 0x00 0xc0 0x80
  else if hiS x then bytes4 ((x * 1024 + (nx + 0xfca02400) % 2 ^ 32) % 2 ^ 32) 0xf0 0x80 0x80 0x80
  else bytes4 x 0x00 0xe0 0x80 0x80

/-- `magic` (lines 149-161): `0x00010101` on `outmask = inmask & ~losurr`
(line 138), `0xffffffff` elsewhere. -/
def magicC (x : Nat) : List Nat :=
  if loS x then [0xff, 0xff, 0xff, 0xff] else [1, 1, 1, 0]

/-- Bytes of an active lane in `wantlo`/`wanthi` (lines 171-174). -/
def laneC (x nx : Nat) : Nat := cntGe (bytesC x nx) (magicC x)

/-- An inactive lane is 0 in path C: lane 31 is cleared by
`hi = _mm512_maskz_mov_epi32(0x7fff, hi)` (line 129), and after a mismatch or
in the tail the lanes past `inmask` are already 0 (lines 122, 196). It is
`is1byte` (`is234byte` is masked by `inmask`), so its dword is `0 << 24`, and
it is outside `outmask`. -/
def laneCz : Nat := cntGe [0, 0, 0, 0] [0xff, 0xff, 0xff, 0xff]

theorem laneCz_eq : laneCz = 0 := by decide

/-- What a lane emits in path C: nothing for a low surrogate (its pair was
emitted with the high one), 4 bytes for a high one, else `w8`. -/
def eC (x : Nat) : Nat := if loS x then 0 else if hiS x then 4 else w8 x

theorem or_lemmas : ∀ f, f < 64 →
    1 ≤ (0x80 ||| f) ∧ (0x80 ||| f) < 0xff ∧ 1 ≤ (0xc0 ||| f) ∧ 1 ≤ (0xe0 ||| f) ∧
      1 ≤ (0xf0 ||| f) := by decide

theorem fld_lt (d s : Nat) : fld d s < 64 := Nat.mod_lt _ (by decide)

theorem laneC_eq (x nx : Nat) (hx : x < 65536) : laneC x nx = eC x := by
  have h18 : fld x 18 = 0 := by
    unfold fld; show x / 262144 % 64 = 0; omega
  unfold laneC eC
  by_cases h1 : x < 0x80
  · have hl : loS x = false := by simp [loS]; omega
    have hh : hiS x = false := by simp [hiS]; omega
    simp [bytesC, magicC, h1, hl, hh, cntGe, w8]
  by_cases h2 : x < 0x800
  · have hl : loS x = false := by simp [loS]; omega
    have hh : hiS x = false := by simp [hiS]; omega
    have h12 : fld x 12 = 0 := by
      unfold fld; show x / 4096 % 64 = 0; omega
    have o6 := or_lemmas _ (fld_lt x 6)
    have o0 := or_lemmas _ (fld_lt x 0)
    simp [bytesC, magicC, bytes4, h1, h2, hl, hh, cntGe, w8, h18, h12] <;> (repeat' split) <;> omega
  by_cases hh : hiS x = true
  · have hl : loS x = false := by simp [hiS, loS] at *; omega
    have hr : 0xD800 ≤ x ∧ x < 0xDC00 := by simpa [hiS] using hh
    generalize hd : (x * 1024 + (nx + 0xfca02400) % 2 ^ 32) % 2 ^ 32 = d
    have o18 := or_lemmas _ (fld_lt d 18)
    have o12 := or_lemmas _ (fld_lt d 12)
    have o6 := or_lemmas _ (fld_lt d 6)
    simp [bytesC, magicC, bytes4, h1, h2, hl, hh, hd, cntGe] <;> (repeat' split) <;> omega
  · have hh' : hiS x = false := by simpa using hh
    by_cases hl : loS x = true
    · have hr : 0xDC00 ≤ x ∧ x < 0xE000 := by simpa [loS] using hl
      have h12 : fld x 12 = 13 := by
        unfold fld; show x / 4096 % 64 = 13; omega
      have o6 := or_lemmas _ (fld_lt x 6)
      have o0 := or_lemmas _ (fld_lt x 0)
      have hed : (0xe0 ||| 13) = 0xed := by decide
      simp [bytesC, magicC, bytes4, h1, h2, hl, hh', cntGe, h18, h12, hed] <;> (repeat' split) <;> omega
    · have hl' : loS x = false := by simpa using hl
      have o12 := or_lemmas _ (fld_lt x 12)
      have o6 := or_lemmas _ (fld_lt x 6)
      have hr : ¬ (0xD800 ≤ x ∧ x < 0xE000) := by
        simp [hiS, loS] at hh' hl'; omega
      simp [bytesC, magicC, bytes4, h1, h2, hl', hh', cntGe, h18, w8, hr] <;> (repeat' split) <;> omega

/-- Path C bytes of the active lanes; `n` is the lane after the last one. -/
def sumC : List Nat → Nat → Nat
  | [], _ => 0
  | x :: xs, n => laneC x (xs.headD n) + sumC xs n

theorem sumC_eq (a : List Nat) (n : Nat) (ha : ∀ v ∈ a, v < 65536) :
    sumC a n = L (a.map eC) := by
  induction a with
  | nil => rfl
  | cons x xs ih =>
    simp only [sumC, List.map_cons, L_cons]
    rw [laneC_eq x _ (ha x (by simp)), ih (fun v hv => ha v (by simp [hv]))]

/-- **Pairing.** Under the check, path C emits `L` of the lanes, minus the 2
bytes of a low surrogate carried in (lane 0, emitted with the previous
iteration's lane 30), plus the 2 bytes of the low surrogate that follows a
high last lane (emitted now, consumed next time). -/
theorem w8_sur (x : Nat) (h : 0xD800 ≤ x ∧ x < 0xE000) : w8 x = 2 := by
  unfold w8; repeat' split
  all_goals omega

theorem pairing (p : Bool) (a : List Nat) (h : chk p a = true) :
    L (a.map eC) + 2 * cb p = L (a.map w8) + 2 * cb (endP p a) := by
  induction a generalizing p with
  | nil => simp [endP]
  | cons x xs ih =>
    simp only [chk, Bool.and_eq_true, beq_iff_eq] at h
    have ih' := ih _ h.2
    simp only [List.map_cons, L_cons, endP]
    have cbt : cb true = 1 := rfl
    have cbf : cb false = 0 := rfl
    cases hl : loS x
    · have hp : p = false := by rw [← h.1, hl]
      subst hp
      cases hh : hiS x
      · have : eC x = w8 x := by simp [eC, hl, hh]
        rw [this]; rw [hh] at ih'; omega
      · have hr : 0xD800 ≤ x ∧ x < 0xE000 := by simp [hiS] at hh; omega
        have : eC x = 4 := by simp [eC, hl, hh]
        rw [this, w8_sur x hr]; rw [hh] at ih'; omega
    · have hp : p = true := by rw [← h.1, hl]
      subst hp
      have hh := lo_not_hi x hl
      have hr : 0xDC00 ≤ x ∧ x < 0xE000 := by simpa [loS] using hl
      have : eC x = 0 := by simp [eC, hl]
      rw [this, w8_sur x (by omega)]; rw [hh] at ih' ⊢; omega

/-- The check on the inactive lanes: lane `|a|` is low exactly when the last
active lane is high; past it `hisurr` is 0. -/
def chkZ : Bool → List Nat → Bool
  | _, [] => true
  | p, y :: ys => (loS y == p) && chkZ false ys

/-- `(a ++ z)[0]` is a low surrogate. -/
def headLo (a z : List Nat) : Bool := loS ((a ++ z).headD 0)

/-- `tzcnt(hinolo | lonohi)` on the inactive lanes (`hinolo` is masked by
`inmask` through `h`): `lonohi` = low and not preceded by a high lane. -/
def errZ : Bool → List Nat → Nat
  | _, [] => 0
  | p, y :: ys => if loS y && !p then 0 else errZ false ys + 1

/-- `tzcnt(hinolo | lonohi)` (lines 118-120): the first lane that is low
without a high predecessor (`lonohi`), or high without a low successor
(`hinolo = h & ~(l >> 1)`). -/
def firstErr : Bool → List Nat → List Nat → Nat
  | p, [], z => errZ p z
  | p, x :: xs, z =>
    if (loS x && !p) || (hiS x && !headLo xs z) then 0 else firstErr (hiS x) xs z + 1

inductive Res where
  | ok (bytes adv : Nat) (cout : Bool)
  | mismatch

/-- One pass from `failiteration` (line 32), active lanes `a`, inactive lanes
`z`, carry `c`. `bytes` is where the stores end, from the cursor; `adv` how far
the cursor moves; `cout` the next carry. -/
def pass (c : Bool) (a z : List Nat) : Res :=
  if a.all (· < 0x80) then .ok a.length 31 false
  else if a.all (· < 0x800) then
    .ok (L (a.map laneB) + L (z.map laneBz)) (31 + L (a.map fun x => if x < 0x80 then 0 else 1)) false
  else
    if (a.any hiS || (a ++ z).any loS) && !(chk c a && chkZ (endP c a) z) then .mismatch
    else
      .ok (sumC a (z.headD 0) + z.length * laneCz) (sumC a (z.headD 0) + z.length * laneCz)
        ((a.drop 30).any hiS)

inductive Step where
  | ok (bytes adv : Nat) (cout : Bool)
  | fail (k bytes : Nat)

/-- One pass, and on a mismatch the second pass on the `k` lanes before the
first error, the others cleared (`in = _mm512_maskz_mov_epi16(inmask, in)`,
line 122), after which the loop ends (`inlen = 0`). The second pass never
mismatches (`rerun_ok`); `.fail k 0` there is unreachable. -/
def attempt (c : Bool) (a z : List Nat) : Step :=
  match pass c a z with
  | .ok b adv co => .ok b adv co
  | .mismatch =>
    match pass c (a.take (firstErr c a z)) (List.replicate (32 - firstErr c a z) 0) with
    | .ok b _ _ => .fail (firstErr c a z) b
    | .mismatch => .fail (firstErr c a z) 0

/-- The loop on the units `us` not yet consumed, with carry `c`: the furthest
end of a store, from the real cursor. While 32 units remain, an iteration on
lanes 0-30 (active) and lane 31 (inactive). After the loop the tail (lines
192-203) is the same pass on the remaining `inlen < 32` units, the other lanes
0 (`_mm512_maskz_loadu_epi16`). When the fuel is spent the rest is assumed
exact, as in `realWritten8`. -/
def run8 : Nat → Bool → List Nat → Nat
  | 0, c, us => L (us.map w8) - 2 * cb c
  | n + 1, c, us =>
    if us.length < 32 then
      match attempt c us (List.replicate (32 - us.length) 0) with
      | .ok b _ _ => b
      | .fail _ b => b
    else
      match attempt c (us.take 31) [(us.drop 31).headD 0] with
      | .ok b adv co => max b (adv + run8 n co (us.drop 31))
      | .fail _ b => b

/-! ### Lemmas on the check -/

theorem chkZ_false_of (z : List Nat) (h : ∀ y ∈ z, loS y = false) : chkZ false z = true := by
  induction z with
  | nil => rfl
  | cons y ys ih =>
    simp only [chkZ, Bool.and_eq_true, beq_iff_eq]
    exact ⟨h y (by simp), ih fun v hv => h v (by simp [hv])⟩

theorem loS_zero : loS 0 = false := by decide

theorem chkZ_zeros (n : Nat) : chkZ false (List.replicate n 0) = true :=
  chkZ_false_of _ fun y hy => by rw [List.eq_of_mem_replicate hy]; exact loS_zero

/-- No error in the active lanes nor in lane `|a|`: the check passes. -/
theorem chk_of_noErr (p : Bool) (a z : List Nat) (hinv : p = true → headLo a z = true)
    (hz : ∀ y ∈ z.drop 1, loS y = false) (hk : a.length < firstErr p a z) :
    chk p a = true ∧ chkZ (endP p a) z = true := by
  induction a generalizing p with
  | nil =>
    cases z with
    | nil => simp [firstErr, errZ] at hk
    | cons y ys =>
      simp only [firstErr, errZ] at hk
      have hne : ¬ (loS y && !p) = true := by
        intro h; rw [ite_eq_left h] at hk; simp at hk
      refine ⟨rfl, ?_⟩
      simp only [endP, chkZ, Bool.and_eq_true, beq_iff_eq]
      refine ⟨?_, chkZ_false_of _ (by simpa using hz)⟩
      cases p
      · simpa using hne
      · simpa [headLo] using hinv rfl
  | cons x xs ih =>
    simp only [firstErr] at hk
    split at hk
    · simp at hk
    · rename_i hne
      simp only [Bool.or_eq_true, Bool.and_eq_true, Bool.not_eq_true'] at hne
      have hinv' : hiS x = true → headLo xs z = true := by
        intro h; cases hq : headLo xs z
        · exact absurd (Or.inr ⟨h, hq⟩) hne
        · rfl
      have := ih (hiS x) hinv' (by simp at hk ⊢; omega)
      refine ⟨?_, this.2⟩
      simp only [chk, Bool.and_eq_true, beq_iff_eq]
      refine ⟨?_, this.1⟩
      cases p
      · cases hl : loS x
        · rfl
        · exact absurd (Or.inl ⟨hl, rfl⟩) hne
      · simpa [headLo] using hinv rfl

/-- The lanes before the first error pass the check, and the last of them is
not high. -/
theorem chk_take_firstErr (p : Bool) (a z : List Nat) (hinv : p = true → headLo a z = true)
    (hk : firstErr p a z ≤ a.length) :
    chk p (a.take (firstErr p a z)) = true ∧ endP p (a.take (firstErr p a z)) = false := by
  induction a generalizing p with
  | nil =>
    simp only [List.length_nil, Nat.le_zero] at hk
    rw [hk]
    refine ⟨rfl, ?_⟩
    cases p
    · rfl
    · exfalso
      cases z with
      | nil => simp [headLo, loS] at hinv
      | cons y ys =>
        simp [firstErr, errZ] at hk
  | cons x xs ih =>
    have hdis : ¬ (loS x = true ∧ hiS x = true) := fun h => by
      have := lo_not_hi x h.1; rw [this] at h; simp at h
    by_cases he : ((loS x && !p) || (hiS x && !headLo xs z)) = true
    · have h0 : firstErr p (x :: xs) z = 0 := by simp only [firstErr]; rw [ite_eq_left he]
      rw [h0]
      refine ⟨rfl, ?_⟩
      cases p
      · rfl
      · exfalso
        have hl : loS x = true := by simpa [headLo] using hinv rfl
        simp only [Bool.or_eq_true, Bool.and_eq_true, Bool.not_eq_true'] at he
        rcases he with ⟨_, h⟩ | ⟨hh, _⟩
        · simp at h
        · exact hdis ⟨hl, hh⟩
    · have h1 : firstErr p (x :: xs) z = firstErr (hiS x) xs z + 1 := by
        simp only [firstErr]; rw [ite_eq_right he]
      rw [h1] at hk ⊢
      simp only [Bool.or_eq_true, Bool.and_eq_true, Bool.not_eq_true'] at he
      have hinv' : hiS x = true → headLo xs z = true := by
        intro h; cases hq : headLo xs z
        · exact absurd (Or.inr ⟨h, hq⟩) he
        · rfl
      have := ih (hiS x) hinv' (by simp at hk; omega)
      simp only [List.take_succ_cons, chk, endP, Bool.and_eq_true, beq_iff_eq]
      refine ⟨⟨?_, this.1⟩, this.2⟩
      cases p
      · cases hl : loS x
        · rfl
        · exact absurd (Or.inl ⟨hl, rfl⟩) he
      · simpa [headLo] using hinv rfl

theorem endP_nohi (p : Bool) (a : List Nat) (h : a.any hiS = false) (hp : p = false) :
    endP p a = false := by
  induction a generalizing p with
  | nil => simpa [endP] using hp
  | cons x xs ih =>
    simp only [List.any_cons, Bool.or_eq_false_iff] at h
    exact ih _ h.2 h.1

theorem chk_nosur (a : List Nat) (hh : a.any hiS = false) (hl : a.any loS = false) :
    chk false a = true := by
  induction a with
  | nil => rfl
  | cons x xs ih =>
    simp only [List.any_cons, Bool.or_eq_false_iff] at hh hl
    simp only [chk, Bool.and_eq_true, beq_iff_eq, hl.1, hh.1, true_and]
    exact ih hh.2 hl.2

/-! ### One pass -/

theorem endP_snoc (p : Bool) (xs : List Nat) (x : Nat) : endP p (xs ++ [x]) = hiS x := by
  induction xs generalizing p with
  | nil => rfl
  | cons y ys ih => exact ih _

theorem endP_drop30 (p : Bool) (a : List Nat) (h : a.length = 31) :
    (a.drop 30).any hiS = endP p a := by
  have hl : (a.drop 30).length = 1 := by simp; omega
  obtain ⟨x, hx⟩ := List.length_eq_one_iff.mp hl
  have e := List.take_append_drop 30 a
  rw [hx] at e
  rw [hx, ← e, endP_snoc]
  simp

theorem c_false (c : Bool) (a : List Nat) (hinv : c = true → loS (a.headD 0) = true)
    (hall : a.all (· < 0x800) = true) : c = false := by
  cases c
  · rfl
  · have h := hinv rfl
    cases a with
    | nil => simp [loS] at h
    | cons x xs =>
      simp only [List.all_cons, Bool.and_eq_true, decide_eq_true_eq] at hall
      simp [loS] at h; omega

theorem L_w8_ascii (a : List Nat) (h : a.all (· < 0x80) = true) : L (a.map w8) = a.length := by
  induction a with
  | nil => rfl
  | cons x xs ih =>
    simp only [List.all_cons, Bool.and_eq_true, decide_eq_true_eq] at h
    simp [w8, h.1, ih h.2]; omega

theorem L_laneB (a : List Nat) (h : a.all (· < 0x800) = true) :
    L (a.map laneB) = L (a.map w8) := by
  induction a with
  | nil => rfl
  | cons x xs ih =>
    simp only [List.all_cons, Bool.and_eq_true, decide_eq_true_eq] at h
    simp [laneB_eq x h.1, ih h.2]

theorem L_is2 (a : List Nat) (h : a.all (· < 0x800) = true) :
    a.length + L (a.map fun x => if x < 0x80 then 0 else 1) = L (a.map w8) := by
  induction a with
  | nil => rfl
  | cons x xs ih =>
    simp only [List.all_cons, Bool.and_eq_true, decide_eq_true_eq] at h
    have := ih h.2
    simp only [List.length_cons, List.map_cons, L_cons, w8]
    by_cases h1 : x < 0x80 <;> simp [h1, h.1] <;> omega

theorem L_rep0 (m : Nat) : L (List.replicate m 0) = 0 := by
  induction m with
  | zero => rfl
  | succ m ih => simp [List.replicate_succ, ih]

/-- Path C runs with the check passed: either it ran and passed, or there was
no surrogate at all, and then (by the carry invariant) no carry either. -/
theorem checked (c : Bool) (a z : List Nat) (hinv : c = true → loS (a.headD 0) = true)
    (h : ((a.any hiS || (a ++ z).any loS) && !(chk c a && chkZ (endP c a) z)) = false) :
    chk c a = true ∧ chkZ (endP c a) z = true := by
  by_cases hb : (a.any hiS || (a ++ z).any loS) = true
  · rw [hb] at h; simpa using h
  · simp only [Bool.or_eq_true, not_or, Bool.not_eq_true, List.any_append] at hb
    obtain ⟨hh, hla, hlz⟩ := hb
    have hc : c = false := by
      cases c
      · rfl
      · have := hinv rfl
        cases a with
        | nil => simp [loS] at this
        | cons x xs =>
          simp only [List.any_cons, Bool.or_eq_false_iff] at hla
          rw [show loS x = true from this] at hla
          cases hla.1
    subst hc
    refine ⟨chk_nosur a hh hla, ?_⟩
    rw [endP_nohi false a hh rfl]
    exact chkZ_false_of z (by simpa using hlz)

theorem pass_mismatch (c : Bool) (a z : List Nat) (h : pass c a z = .mismatch) :
    (chk c a && chkZ (endP c a) z) = false := by
  unfold pass at h
  split at h
  · cases h
  · split at h
    · cases h
    · split at h
      · rename_i hm
        simp only [Bool.and_eq_true, Bool.not_eq_true'] at hm
        exact hm.2
      · cases h

/-- Path C output under the check. -/
theorem pathChk (c : Bool) (a : List Nat) (n : Nat) (zl : Nat) (hv : ∀ v ∈ a, v < 65536)
    (hc : chk c a = true) :
    2 * cb c + (sumC a n + zl * laneCz) = L (a.map w8) + 2 * cb (endP c a) := by
  rw [laneCz_eq, sumC_eq a n hv, Nat.mul_zero, Nat.add_zero]
  have := pairing c a hc
  omega

/-- **The normal iteration** (lanes 0-30 active, lane 31 `y` inactive):
the cursor moves by `L` of the 31 lanes, corrected by the carries, and the
stores end `extra` past `L` of the 31 lanes, `extra` being the spill of lane
31 (path B), 2 when lane 31 is a low surrogate carried out (path C), or 0. -/
theorem pass_ok_normal (c : Bool) (a : List Nat) (y b adv : Nat) (co : Bool)
    (ha : a.length = 31) (hv : ∀ v ∈ a, v < 65536) (hy : y < 65536)
    (hinv : c = true → loS (a.headD 0) = true) (h : pass c a [y] = .ok b adv co) :
    2 * cb c + adv = L (a.map w8) + 2 * cb co ∧ (co = true → loS y = true) ∧
      ∃ e, 2 * cb c + b = L (a.map w8) + e ∧
        (e ≤ spill y ∨ (0xDC00 ≤ y ∧ y < 0xE000 ∧ e ≤ 2) ∨ e = 0) := by
  unfold pass at h
  split at h
  · rename_i hA
    cases h
    have hc := c_false c a hinv (by
      simp only [List.all_eq_true, decide_eq_true_eq] at hA ⊢
      exact fun x hx => Nat.lt_trans (hA x hx) (by decide))
    subst hc
    have := L_w8_ascii a hA
    refine ⟨by simp [cb]; omega, by simp, 0, by simp [cb]; omega, by simp⟩
  · split at h
    · rename_i _ hB
      cases h
      have hc := c_false c a hinv hB
      subst hc
      have h1 := L_laneB a hB
      have h2 := L_is2 a hB
      refine ⟨by simp [cb]; omega, by simp, laneBz y, ?_, Or.inl ?_⟩
      · simp [cb, h1]
      · rw [laneBz_eq y hy]; exact Nat.le_refl _
    · split at h
      · cases h
      · rename_i hm
        cases h
        obtain ⟨hc, hz⟩ := checked c a [y] hinv (Bool.eq_false_iff.mpr hm)
        have hP := pathChk c a ([y].headD 0) [y].length hv hc
        have hco := endP_drop30 c a ha
        simp only [chkZ, Bool.and_eq_true, beq_iff_eq] at hz
        rw [hco]
        refine ⟨hP, fun h => by rw [hz.1]; exact h, 2 * cb (endP c a), hP, ?_⟩
        cases he : endP c a
        · simp [cb]
        · rw [he] at hz
          have : 0xDC00 ≤ y ∧ y < 0xE000 := by simpa [loS] using hz.1
          right; left; simp [cb]; omega

/-- **The tail pass, and the second pass after a mismatch** (inactive lanes
0): the stores end at most at `L` of the active lanes, from the model
cursor. -/
theorem pass_ok_zeros (c : Bool) (a : List Nat) (m b adv : Nat) (co : Bool) (hm : 1 ≤ m)
    (hv : ∀ v ∈ a, v < 65536) (hinv : c = true → loS (a.headD 0) = true)
    (h : pass c a (List.replicate m 0) = .ok b adv co) :
    2 * cb c + b ≤ L (a.map w8) := by
  unfold pass at h
  split at h
  · rename_i hA
    cases h
    have hc := c_false c a hinv (by
      simp only [List.all_eq_true, decide_eq_true_eq] at hA ⊢
      exact fun x hx => Nat.lt_trans (hA x hx) (by decide))
    subst hc
    have := L_w8_ascii a hA
    simp [cb]; omega
  · split at h
    · rename_i _ hB
      cases h
      have hc := c_false c a hinv hB
      subst hc
      have h1 := L_laneB a hB
      have h0 : L ((List.replicate m 0).map laneBz) = 0 := by
        rw [List.map_replicate, show laneBz 0 = 0 by decide]; exact L_rep0 m
      simp only [cb, h0, h1]; simp
    · split at h
      · cases h
      · rename_i hm'
        cases h
        obtain ⟨hc, hz⟩ := checked c a (List.replicate m 0) hinv (Bool.eq_false_iff.mpr hm')
        have hP := pathChk c a ((List.replicate m 0).headD 0) (List.replicate m 0).length hv hc
        obtain ⟨m', rfl⟩ : ∃ m', m = m' + 1 := ⟨m - 1, by omega⟩
        simp only [List.replicate_succ, chkZ, Bool.and_eq_true, beq_iff_eq] at hz
        rw [loS_zero] at hz
        rw [← hz.1] at hP
        simp only [cb] at hP ⊢
        simp at hP ⊢; omega

theorem headLo_cons (x : Nat) (xs z : List Nat) : headLo (x :: xs) z = loS x := rfl

theorem headLo_zeros (a : List Nat) (m : Nat) :
    headLo a (List.replicate (m + 1) 0) = loS (a.headD 0) := by
  cases a <;> rfl

theorem take_headD (a : List Nat) (k : Nat) (hk : 1 ≤ k) : (a.take k).headD 0 = a.headD 0 := by
  obtain ⟨k', rfl⟩ : ∃ k', k = k' + 1 := ⟨k - 1, by omega⟩
  cases a <;> rfl

/-- **The error path** (lines 116-125): the first pass mismatches, the error
lane `k` is at most `|a|` (never past lane 31), the lanes before it pass the
check and do not end high, so the second pass does not mismatch and its
stores end at most at `L` of those `k` lanes. -/
theorem attempt_fail (c : Bool) (a z : List Nat) (k b : Nat) (ha : a.length ≤ 31)
    (hv : ∀ v ∈ a, v < 65536) (hz : ∀ y ∈ z.drop 1, loS y = false)
    (hinv : c = true → headLo a z = true) (hinv' : c = true → loS (a.headD 0) = true)
    (h : attempt c a z = .fail k b) :
    2 * cb c + b ≤ L (a.map w8) := by
  unfold attempt at h
  split at h
  · cases h
  · rename_i hmis
    have hk : firstErr c a z ≤ a.length := by
      apply Classical.byContradiction
      intro hlt
      have := chk_of_noErr c a z hinv hz (by omega)
      have hm := pass_mismatch c a z hmis
      rw [this.1, this.2] at hm
      simp at hm
    obtain ⟨hc', he'⟩ := chk_take_firstErr c a z hinv hk
    obtain ⟨K, hK⟩ : ∃ K, firstErr c a z = K := ⟨_, rfl⟩
    rw [hK] at hk hc' he' h
    have hv' : ∀ v ∈ a.take K, v < 65536 := fun v hv0 => hv v (List.mem_of_mem_take hv0)
    have hinvK : c = true → loS ((a.take K).headD 0) = true := by
      intro hc
      by_cases hK0 : K = 0
      · subst hK0; simp [endP, hc] at he'
      · rw [take_headD a K (by omega)]; exact hinv' hc
    cases hp : pass c (a.take K) (List.replicate (32 - K) 0) with
    | ok b' adv' co' =>
      rw [hp] at h
      simp only [Step.fail.injEq] at h
      obtain ⟨-, rfl⟩ := h
      have := pass_ok_zeros c (a.take K) (32 - K) b' adv' co' (by omega) hv' hinvK hp
      exact Nat.le_trans this (L_map_take w8 a K)
    | mismatch =>
      have hm := pass_mismatch c _ _ hp
      obtain ⟨m', hm'⟩ : ∃ m', 32 - K = m' + 1 := ⟨31 - K, by omega⟩
      rw [hc', he', hm', List.replicate_succ] at hm
      simp [chkZ, loS_zero, chkZ_zeros] at hm

/-! ### The loop -/

theorem drop31_cons (us : List Nat) (h : 32 ≤ us.length) :
    us.drop 31 = (us.drop 31).headD 0 :: (us.drop 32) := by
  cases hd : us.drop 31 with
  | nil => have := congrArg List.length hd; simp at this; omega
  | cons y rest =>
    have : us.drop 32 = rest := by
      rw [show 32 = 31 + 1 from rfl, ← List.drop_drop, hd]; rfl
    rw [this]; rfl

/-- **`hreal` of `safe_utf16to8_real`, derived.** For every 32-lane load
whose iteration completes (paths A, B, C without mismatch), with the carry
invariant that a carry is followed by a low surrogate in lane 0: the bytes
written past `L` of the 31 consumed units, measured from the model cursor
(the real cursor is `2 * cb c` ahead after a carry), are the spill of lane 31,
the 2-byte carry with lane 31 a low surrogate, or 0, and lane 31 is the
32nd weight. -/
theorem utf16to8_hreal (c : Bool) (us : List Nat) (b adv : Nat) (co : Bool)
    (h32 : 32 ≤ us.length) (hv : ∀ v ∈ us, v < 65536)
    (hinv : c = true → loS (us.headD 0) = true)
    (h : attempt c (us.take 31) [(us.drop 31).headD 0] = .ok b adv co) :
    ∃ v extra, 2 * cb c + b = L ((us.map w8).take 31) + extra ∧ v < 65536 ∧
      (us.map w8).take 32 = (us.map w8).take 31 ++ [w8 v] ∧
      (extra ≤ spill v ∨ (0xDC00 ≤ v ∧ v < 0xE000 ∧ extra ≤ 2) ∨ extra = 0) := by
  have hd := drop31_cons us h32
  have hy : (us.drop 31).headD 0 < 65536 :=
    hv _ (List.mem_of_mem_drop (l := us) (i := 31) (by rw [hd]; simp))
  have hpass : pass c (us.take 31) [(us.drop 31).headD 0] = .ok b adv co := by
    unfold attempt at h
    split at h
    · rename_i hp; cases h; exact hp
    · split at h <;> cases h
  have hinv' : c = true → loS ((us.take 31).headD 0) = true := by
    rw [take_headD us 31 (by decide)]; exact hinv
  obtain ⟨_, _, e, he, hdis⟩ := pass_ok_normal c (us.take 31) _ b adv co (by simp; omega)
    (fun v hv0 => hv v (List.mem_of_mem_take hv0)) hy hinv' hpass
  refine ⟨(us.drop 31).headD 0, e, by rw [← List.map_take]; exact he, hy, ?_, hdis⟩
  have e0 := List.take_append_drop 31 us
  rw [hd] at e0
  rw [← e0]
  have hm : min 31 us.length = 31 := by omega
  simp [List.take_append, hm, List.take_take]

/-- With `utf16to8_extra_le_ahead`: the real end of the iteration is at most
`storeEnd` of `.ahead`, `L` of the 32 loaded lanes. -/
theorem utf16to8_real_end (c : Bool) (us : List Nat) (b adv : Nat) (co : Bool)
    (h32 : 32 ≤ us.length) (hv : ∀ v ∈ us, v < 65536)
    (hinv : c = true → loS (us.headD 0) = true)
    (h : attempt c (us.take 31) [(us.drop 31).headD 0] = .ok b adv co) :
    2 * cb c + b ≤ storeEnd (us.map w8) 31 .ahead := by
  obtain ⟨v, e, he, hv0, ht, hd⟩ := utf16to8_hreal c us b adv co h32 hv hinv h
  rw [he]; exact utf16to8_extra_le_ahead _ v e hv0 ht hd

/-- The real end of every store, iteration after iteration, from the model
cursor, is at most `utf8_length_from_utf16` of the input, under the carry
invariant (which the loop maintains: `pass_ok_normal`). -/
theorem safe_run8 : ∀ n c us, (∀ v ∈ us, v < 65536) →
    (c = true → loS (us.headD 0) = true) → 2 * cb c + run8 n c us ≤ L (us.map w8) := by
  intro n
  induction n with
  | zero =>
    intro c us _ hinv
    simp only [run8]
    cases c
    · simp [cb]
    · have h := hinv rfl
      cases us with
      | nil => simp [loS] at h
      | cons x xs =>
        have : w8 x = 2 := w8_sur x (by simp [loS] at h; omega)
        simp [cb, this]
  | succ n ih =>
    intro c us hv hinv
    unfold run8
    by_cases hs : us.length < 32
    · rw [ite_eq_left hs]
      obtain ⟨m, hm⟩ : ∃ m, 32 - us.length = m + 1 := ⟨31 - us.length, by omega⟩
      split
      · rename_i b adv co hA
        unfold attempt at hA
        split at hA
        · rename_i hp
          cases hA
          exact pass_ok_zeros c us _ b adv co (by omega) hv hinv hp
        · split at hA <;> cases hA
      · rename_i k b hF
        refine attempt_fail c us _ k b (by omega) hv ?_ ?_ hinv hF
        · rw [hm]; intro y hy
          simp [List.replicate_succ] at hy; rw [hy.2]; exact loS_zero
        · rw [hm, headLo_zeros]; exact hinv
    · rw [ite_eq_right hs]
      have h32 : 32 ≤ us.length := by omega
      have hd := drop31_cons us h32
      have hsplit := L_map_split w8 us 31
      have hv31 : ∀ v ∈ us.take 31, v < 65536 := fun v h0 => hv v (List.mem_of_mem_take h0)
      have hinv' : c = true → loS ((us.take 31).headD 0) = true := by
        rw [take_headD us 31 (by decide)]; exact hinv
      split
      · rename_i b adv co hA
        have hy : (us.drop 31).headD 0 < 65536 :=
          hv _ (List.mem_of_mem_drop (l := us) (i := 31) (by rw [hd]; simp))
        have hpass : pass c (us.take 31) [(us.drop 31).headD 0] = .ok b adv co := by
          unfold attempt at hA
          split at hA
          · rename_i hp; cases hA; exact hp
          · split at hA <;> cases hA
        obtain ⟨hadv, hco, -⟩ := pass_ok_normal c (us.take 31) _ b adv co (by simp; omega)
          hv31 hy hinv' hpass
        have hend := utf16to8_real_end c us b adv co h32 hv hinv hA
        simp only [storeEnd] at hend
        have hle := L_take_le (us.map w8) (31 + 1)
        have hr := ih co (us.drop 31) (fun v h0 => hv v (List.mem_of_mem_drop h0))
          (by intro h; rw [hd]; exact hco h)
        rcases Nat.le_total b (adv + run8 n co (us.drop 31)) with hm | hm
        · rw [Nat.max_eq_right hm]; omega
        · rw [Nat.max_eq_left hm]; omega
      · rename_i k b hF
        have := attempt_fail c (us.take 31) _ k b (by rw [List.length_take]; exact Nat.min_le_left _ _) hv31 (by simp) ?_ hinv' hF
        · have := L_map_take w8 us 31
          omega
        · intro hc; rw [take_headD us 31 (by decide)] at hinv'
          have : headLo (us.take 31) [(us.drop 31).headD 0] = loS (us.headD 0) := by
            cases us with
            | nil => simp at h32
            | cons x xs => rfl
          rw [this]; exact hinv hc

/-- **UTF-16 → UTF-8 on Ice Lake, real ends, no data hypothesis.** From the
start of the loop (no carry), on every input of 16-bit units, no store of any
iteration, of the error path, or of the tail ends past
`utf8_length_from_utf16` of the input. -/
theorem safe_utf16to8_kernel (n : Nat) (us : List Nat) (hv : ∀ v ∈ us, v < 65536) :
    run8 n false us ≤ L (us.map w8) := by
  have := safe_run8 n false us hv (by simp)
  simpa [cb] using this

end SafetyMargins.IcelakeReal
