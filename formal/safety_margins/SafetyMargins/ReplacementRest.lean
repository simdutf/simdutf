import SafetyMargins.ReplacementMore
/-
The remaining pieces of the conversions "with replacement".

* **(a)** Ice Lake `utf8_length_from_utf16_with_replacement`
  (src/icelake/icelake_utf8_length_from_utf16.inl.cpp:114-269), its three
  loops transcribed at the level of 16-bit lanes and 32-lane masks (the byte
  flip of the non-native endianness, the 32-bit straddle words read with
  `memcpy`, the masked tail load): `iceLen_eq` proves it equals `countRepl`
  on every input; `ice_loads` puts its loads inside the input.
* **(b)** `utf8_boundary_before` (`boundaryCpp_eq`, `boundary_spec`): the
  window ends before a non-continuation byte, so an error the bulk kernel
  finds in the window is an error of the input at the same place
  (`window_error`, `bulk_window`). The whole driver
  `transcode_utf8_to_utf16_with_replacement<endian, true>` is transcribed
  (`tr`), and from any `RReach` state it returns `count xs` with every store
  inside it (`tr_safe`); it terminates (`tr_some`).
* **(d)** `short_prefixes` and `dense_prefix_limit` (`shortUpd`) are in `tr`:
  `tr_safe` holds for every value of the counter. `convert_using_locations`
  (`cul`) with its guards (:208, :223) and the recording loop of
  `utf16_length_from_utf8_with_replacement` (`locF`, 16 offsets,
  `more_errors`): `convertRepl_safe`, the guards never fire, the result is
  `count xs`, stores inside. `count_le_length`: the documented buffer of
  `length` units suffices.
* **(c)** UTF-16 → UTF-8: the scalar `convert_with_replacement` writes exactly
  `countRepl` (`scr_total`); `convert_with_errors<…, true>` (`cweLoop`),
  `scalar::…::convert_with_replacement_safe` (`crs`),
  `convert_utf16_to_utf8_safe_with_details` (`dLoop`) and
  `convert_utf16_to_utf8_with_replacement_safe` (`tLoop`) never store past
  the capacity and never return an output count above it
  (`replSafe_safe`, `crs_safe`).

Assumed (contracts of vector code, as in `Replacement` and
`ReplacementMore`): `BSpec` (the bulk UTF-8 → UTF-16 `_with_errors` call:
valid window on success, valid prefix and `dchar` failure at its error, stores
within 4 units past a valid prefix), `VSpec2` (the vector
`validate_utf8_with_counts`; the scalar one meets it, `scalarV_spec2`),
`CVSpec`/`ULSpec` (`convert_valid_prefix` and `utf16_length_from_utf8` exact
on valid input), `VKSpec`/`SLSpec` (the vector UTF-16 → UTF-8 `_with_errors`
kernel stores within the standard length of its input; the vector standard
length). The ASCII probes are oracles: arbitrary in the `_safe` models, sound
in `scr_total`.

`decide +kernel` is used for finite exhaustive checks only: `laneFacts_all`
and `u16Facts_all` (65536 values), `remMask_spec` (32 × 32), and one example.
-/
namespace SafetyMargins.ReplacementRest
open SafetyMargins SafetyMargins.Replacement SafetyMargins.ReplacementMore
open SafetyMargins.Utf8 (w16)
set_option autoImplicit false
set_option linter.unusedSimpArgs false

/-! # (a) Ice Lake `utf8_length_from_utf16_with_replacement`

Raw 16-bit words `rs` as they are in memory. With the non-native endianness
(`sw = true`) the loaded vectors go through `_mm512_shuffle_epi8(·, byteflip)`
(:131-134, :148-157, :204-207, :239-241; the byte indices `01 00 03 02 ...` swap the two bytes of
every 16-bit lane), so every lane holds `bswap` of the word; the straddle
words (:195-197, :228-229) are read raw from memory and use the swapped
`straddle_mask`/`straddle_pair` (:136-139). x86 is little-endian, so the
`uint32_t` read at `in + k` is `word k = in[k] + 65536 * in[k + 1]`.

A `__mmask32` is modelled as a predicate on lane indices (bit `i` = lane `i`,
`i < 32`); `count_ones32` is the sum over the 32 lanes; `_kshiftli_mask32(m, 1)`
moves bit `i - 1` to bit `i`, bit 0 is cleared, bit 31 is dropped. -/

/-- Byte swap of a 16-bit word (`u16_swap_bytes`, and `byteflip` per lane). -/
def bswap (r : Nat) : Nat := (r % 256) * 256 + r / 256

/-- The value of a lane after the optional byte flip. -/
def unitOf (sw : Bool) (r : Nat) : Nat := if sw then bswap r else r

def ind (b : Bool) : Nat := if b then 1 else 0

/-- `count_ones32`. -/
def popc (m : Nat → Bool) : Nat := sumTo (fun i => ind (m i)) 32
/-- `_kshiftli_mask32(m, 1)`. -/
def shl1 (m : Nat → Bool) (i : Nat) : Bool := if i = 0 then false else m (i - 1)
/-- `_mm512_test_epi16_mask(v, set1(c))`: lane `& c` nonzero. -/
def testM (v : Nat → Nat) (c : Nat) (i : Nat) : Bool := v i &&& c != 0
/-- `_mm512_cmpeq_epi16_mask(_mm512_and_si512(v, set1(a)), set1(b))`. -/
def eqM (v : Nat → Nat) (a b : Nat) (i : Nat) : Bool := v i &&& a == b
/-- A mask used as a condition (`if (mask)`): some lane set. -/
def anyM (m : Nat → Bool) : Bool := (List.range 32).any m

/-- `count_ones32(c0) + count_ones32(c1)` of a vector (:165-176, :212-217,
:246-252). -/
def blkCount (v : Nat → Nat) : Nat := popc (testM v 0xff80) + popc (testM v 0xf800)
/-- `is_surrogate` (:159-164, :209-211, :243-245). -/
def surM (v : Nat → Nat) : Nat → Bool := eqM v 0xf800 0xd800
/-- `count_ones32(_kand_mask32(_kshiftli_mask32(hi_surrogates, 1), lo_surrogates))`
(:178-194, :219-227, :253-262). -/
def inMatch (v : Nat → Nat) : Nat :=
  popc (fun i => shl1 (eqM v 0xfc00 0xd800) i && eqM v 0xfc00 0xdc00 i)

/-- The 32-bit word read by `memcpy(&straddle, in + k, 4)`. -/
def word (rs : List Nat) (k : Nat) : Nat := g rs k + 65536 * g rs (k + 1)
/-- `straddle_mask` (:136-137); `sw` is `!match_system(big_endian)`. -/
def smask (sw : Bool) : Nat := if sw then 0x00fc00fc else 0xfc00fc00
/-- `straddle_pair` (:138-139). -/
def spair (sw : Bool) : Nat := if sw then 0x00dc00d8 else 0xdc00d800
/-- `(straddle & straddle_mask) == straddle_pair` as 0/1 (:198-199, :230). -/
def straddle (sw : Bool) (rs : List Nat) (k : Nat) : Nat :=
  ind (word rs k &&& smask sw == spair sw)

/-- The 32 lanes of `_mm512_loadu_si512(in + p)` after the flip. -/
def blk (sw : Bool) (rs : List Nat) (p : Nat) : Nat → Nat := fun i => unitOf sw (g rs (p + i))

/-- `remaining_mask = uint32_t(0xFFFFFFFFULL << overshoot)` (:236), bit `i`. -/
def remMask (ov i : Nat) : Bool := ((0xFFFFFFFF <<< ov) % 2 ^ 32).testBit i

/-- The lanes of `_mm512_maskz_loadu_epi16(remaining_mask, in + pos - overshoot)`
after the flip (:237-241): masked-off lanes are 0 (and stay 0 after the flip). -/
def tailLanes (sw : Bool) (rs : List Nat) (p ov : Nat) : Nat → Nat :=
  fun i => if remMask ov i then unitOf sw (g rs (p - ov + i)) else 0

/-- `pos`, `count`, `matches`, `any_surrogates`. -/
structure IceSt where
  pos : Nat
  cnt8 : Nat
  mt : Nat
  anyS : Bool

def iceInit : IceSt := ⟨0, 0, 0, false⟩

/-- One iteration of the unrolled loop (:147-201) at `pos`: two vectors, the
counts, and, when either has a surrogate (`_kor_mask32`), the pairs inside each
vector and the two straddle words at `pos + N - 1` and `pos + 2N - 1`. -/
def iceStep2N (sw : Bool) (rs : List Nat) (st : IceSt) : IceSt :=
  let v1 := blk sw rs st.pos
  let v2 := blk sw rs (st.pos + 32)
  let c := st.cnt8 + blkCount v1 + blkCount v2
  if anyM (surM v1) || anyM (surM v2) then
    ⟨st.pos + 64, c, st.mt + inMatch v1 + inMatch v2 + straddle sw rs (st.pos + 31) +
      straddle sw rs (st.pos + 63), true⟩
  else ⟨st.pos + 64, c, st.mt, st.anyS⟩

/-- `for (; pos < (size - 1) / (2 * N) * (2 * N); pos += 2 * N)`: `b` iterations. -/
def iceLoop1 (sw : Bool) (rs : List Nat) : Nat → IceSt
  | 0 => iceInit
  | b + 1 => iceStep2N sw rs (iceLoop1 sw rs b)

/-- The single `N` block (:202-233), taken when `pos + N + 1 <= size`. -/
def iceStepN (sw : Bool) (rs : List Nat) (size : Nat) (st : IceSt) : IceSt :=
  if st.pos + 32 + 1 ≤ size then
    let v := blk sw rs st.pos
    let c := st.cnt8 + blkCount v
    if anyM (surM v) then ⟨st.pos + 32, c, st.mt + inMatch v + straddle sw rs (st.pos + 31), true⟩
    else ⟨st.pos + 32, c, st.mt, st.anyS⟩
  else st

/-- The masked tail (:235-263): `overshoot = 32 - (size - pos)`; then
`pos = size` (:264). -/
def iceTail (sw : Bool) (rs : List Nat) (size : Nat) (st : IceSt) : IceSt :=
  let ov := 32 - (size - st.pos)
  let v := tailLanes sw rs st.pos ov
  let c := st.cnt8 + blkCount v
  if anyM (surM v) then ⟨size, c, st.mt + inMatch v, true⟩ else ⟨size, c, st.mt, st.anyS⟩

/-- **`icelake_utf8_length_from_utf16_with_replacement`** (:114-269), the
`count` field: short inputs (`N + 1 > size`) go to the scalar function
(:125-128, `countRepl` of the flipped units); otherwise the three loops, then
`count += pos` (:265) and `count -= 2 * matches` (:267). -/
def iceLen (sw : Bool) (rs : List Nat) : Nat :=
  if 32 + 1 > rs.length then countRepl (rs.map (unitOf sw)) else
  let st := iceTail sw rs rs.length
    (iceStepN sw rs rs.length (iceLoop1 sw rs ((rs.length - 1) / 64)))
  st.cnt8 + st.pos - 2 * st.mt

/-! ## Lane facts -/

/-- What each mask test means on a 16-bit lane, and what the raw-word half of
a straddle test means on the flipped unit. -/
def laneFacts (u : Nat) : Bool :=
  ((u &&& 0xff80 != 0) == decide (u > 0x7F)) && ((u &&& 0xf800 != 0) == decide (u > 0x7FF)) &&
  ((u &&& 0xf800 == 0xd800) == isSur u) && ((u &&& 0xfc00 == 0xd800) == isHigh u) &&
  ((u &&& 0xfc00 == 0xdc00) == isLow u) &&
  ((u &&& 0xfc == 0xd8) == isHigh (bswap u)) && ((u &&& 0xfc == 0xdc) == isLow (bswap u)) &&
  decide (bswap u < 65536)

/-- Checked on all 65536 values (`decide +kernel`, a finite exhaustive check). -/
theorem laneFacts_all : (List.range 65536).all laneFacts = true := by decide +kernel

theorem lf (u : Nat) (h : u < 65536) :
    ((u &&& 0xff80 != 0) = decide (u > 0x7F)) ∧ ((u &&& 0xf800 != 0) = decide (u > 0x7FF)) ∧
    ((u &&& 0xf800 == 0xd800) = isSur u) ∧ ((u &&& 0xfc00 == 0xd800) = isHigh u) ∧
    ((u &&& 0xfc00 == 0xdc00) = isLow u) ∧
    ((u &&& 0xfc == 0xd8) = isHigh (bswap u)) ∧ ((u &&& 0xfc == 0xdc) = isLow (bswap u)) ∧
    bswap u < 65536 := by
  have := List.all_eq_true.mp laneFacts_all u (List.mem_range.mpr h)
  simp only [laneFacts, Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq] at this
  obtain ⟨⟨⟨⟨⟨⟨⟨a, b⟩, c⟩, d⟩, e⟩, f⟩, g'⟩, h'⟩ := this
  exact ⟨a, b, c, d, e, f, g', h'⟩

/-- `remaining_mask` has exactly the lanes `overshoot..31` (all `overshoot < 32`,
`decide +kernel`, a finite exhaustive check). -/
theorem remMask_spec : ∀ ov, ov < 32 → ∀ i, i < 32 → remMask ov i = decide (ov ≤ i) := by
  decide +kernel

/-- A 32-bit word of two 16-bit halves under a 32-bit mask. -/
theorem split32 (a b m1 m2 p1 p2 : Nat) (ha : a < 65536) (_hb : b < 65536) (hm1 : m1 < 65536)
    (hp1 : p1 < 65536) :
    ((a + 65536 * b) &&& (m1 + 65536 * m2) == p1 + 65536 * p2) =
      ((a &&& m1 == p1) && (b &&& m2 == p2)) := by
  have e16 : (65536 : Nat) = 2 ^ 16 := rfl
  have hlo : ((a + 65536 * b) &&& (m1 + 65536 * m2)) % 65536 = a &&& m1 := by
    rw [e16, Nat.and_mod_two_pow, ← e16]
    rw [show (a + 65536 * b) % 65536 = a by omega, show (m1 + 65536 * m2) % 65536 = m1 by omega]
  have hhi : ((a + 65536 * b) &&& (m1 + 65536 * m2)) / 65536 = b &&& m2 := by
    rw [e16, Nat.and_div_two_pow, ← e16]
    rw [show (a + 65536 * b) / 65536 = b by omega, show (m1 + 65536 * m2) / 65536 = m2 by omega]
  have hlt : a &&& m1 < 65536 := Nat.lt_of_le_of_lt Nat.and_le_left ha
  generalize (a + 65536 * b) &&& (m1 + 65536 * m2) = y at *
  have hy := Nat.mod_add_div y 65536
  rw [hlo, hhi] at hy
  apply Bool.eq_iff_iff.mpr
  simp only [beq_iff_eq, Bool.and_eq_true]
  constructor
  · intro h; subst h; omega
  · intro ⟨h1, h2⟩; rw [← hy, h1, h2]

theorem bswap_zero : bswap 0 = 0 := rfl
theorem unitOf_zero (sw : Bool) : unitOf sw 0 = 0 := by cases sw <;> rfl

theorem g_map (rs : List Nat) (f : Nat → Nat) (hf : f 0 = 0) (j : Nat) :
    g (rs.map f) j = f (g rs j) := by
  unfold g
  rw [List.getD_eq_getElem?_getD, List.getD_eq_getElem?_getD, List.getElem?_map]
  cases rs[j]? <;> simp [hf]

theorem unit_lt (sw : Bool) (u : Nat) (h : u < 65536) : unitOf sw u < 65536 := by
  cases sw
  · exact h
  · exact (lf u h).2.2.2.2.2.2.2

/-- The straddle test is "unit `k` high and unit `k + 1` low", both
endiannesses. -/
theorem straddle_eq (sw : Bool) (rs : List Nat) (hu : U16 rs) (k : Nat) :
    straddle sw rs k =
      ind (isHigh (unitOf sw (g rs k)) && isLow (unitOf sw (g rs (k + 1)))) := by
  have ha := g_lt rs hu k
  have hb := g_lt rs hu (k + 1)
  unfold straddle word
  cases sw
  · rw [show smask false = 0xfc00 + 65536 * 0xfc00 from rfl,
      show spair false = 0xd800 + 65536 * 0xdc00 from rfl,
      split32 _ _ _ _ _ _ ha hb (by decide) (by decide)]
    simp only [unitOf, Bool.false_eq_true, ite_false, (lf _ ha).2.2.2.1, (lf _ hb).2.2.2.2.1]
  · rw [show smask true = 0xfc + 65536 * 0xfc from rfl,
      show spair true = 0xd8 + 65536 * 0xdc from rfl,
      split32 _ _ _ _ _ _ ha hb (by decide) (by decide)]
    simp only [unitOf, ite_true, (lf _ ha).2.2.2.2.2.1, (lf _ hb).2.2.2.2.2.2.1]

/-! ## Mask counts as sums over lanes -/

/-- Per-unit `c0 + c1`. -/
def ccu (u : Nat) : Nat := ind (decide (u > 0x7F)) + ind (decide (u > 0x7FF))
/-- Per adjacent pair: high then low. -/
def pru (u w : Nat) : Nat := ind (isHigh u && isLow w)

theorem sumTo_succ_left (f : Nat → Nat) (m : Nat) :
    sumTo f (m + 1) = f 0 + sumTo (fun i => f (i + 1)) m := by
  have := sumTo_add f 1 m
  rw [Nat.add_comm 1 m] at this
  rw [this]
  simp only [sumTo, Nat.zero_add]
  congr 1
  apply sumTo_congr; intro i _; rw [Nat.add_comm]

theorem blkCount_eq (v : Nat → Nat) (hv : ∀ i, i < 32 → v i < 65536) :
    blkCount v = sumTo (fun i => ccu (v i)) 32 := by
  unfold blkCount popc ccu
  rw [sumTo_plus]
  congr 1
  · apply sumTo_congr; intro i hi; simp only [testM, (lf _ (hv i hi)).1]
  · apply sumTo_congr; intro i hi; simp only [testM, (lf _ (hv i hi)).2.1]

theorem inMatch_eq (v : Nat → Nat) (hv : ∀ i, i < 32 → v i < 65536) :
    inMatch v = sumTo (fun i => pru (v i) (v (i + 1))) 31 := by
  unfold inMatch popc
  rw [show (32 : Nat) = 31 + 1 from rfl, sumTo_succ_left]
  simp only [shl1, ite_true, Bool.false_and, ind, Bool.false_eq_true, ite_false, Nat.zero_add]
  apply sumTo_congr; intro i hi
  simp only [Nat.add_one_ne_zero, ite_false, Nat.add_sub_cancel, eqM, pru, ind,
    (lf _ (hv i (by omega))).2.2.2.1, (lf _ (hv (i + 1) (by omega))).2.2.2.2.1]

/-- With no surrogate lane, `inMatch` is 0 (the skipped branch adds nothing). -/
theorem inMatch_zero (v : Nat → Nat) (hv : ∀ i, i < 32 → v i < 65536)
    (hs : anyM (surM v) = false) : inMatch v = 0 := by
  rw [inMatch_eq v hv]
  rw [sumTo_congr _ (fun _ => 0) 31 ?_, sumTo_zero]
  intro i hi
  have h1 : surM v i = false := by
    simp only [anyM, List.any_eq_false, List.mem_range] at hs
    simpa using hs i (by omega)
  simp only [surM, eqM, (lf _ (hv i (by omega))).2.2.1] at h1
  have : isHigh (v i) = false := by
    cases e : isHigh (v i)
    · rfl
    · rw [high_sur e] at h1; cases h1
  simp [pru, ind, this]

theorem not_high_of_surM (v : Nat → Nat) (hv : ∀ i, i < 32 → v i < 65536)
    (hs : anyM (surM v) = false) (i : Nat) (hi : i < 32) : isHigh (v i) = false := by
  have h1 : surM v i = false := by
    simp only [anyM, List.any_eq_false, List.mem_range] at hs
    simpa using hs i hi
  simp only [surM, eqM, (lf _ (hv i hi)).2.2.1] at h1
  cases e : isHigh (v i)
  · rfl
  · rw [high_sur e] at h1; cases h1

/-! ## The loops count every unit and every adjacent pair once -/

section
variable (sw : Bool) (rs : List Nat)

/-- The flipped unit `j` (0 past the end). -/
def gx (j : Nat) : Nat := unitOf sw (g rs j)
/-- `c0 + c1` of the first `k` units. -/
def CC (k : Nat) : Nat := sumTo (fun j => ccu (gx sw rs j)) k
/-- Adjacent (high, low) pairs starting before `k`. -/
def PR (k : Nat) : Nat := sumTo (fun j => pru (gx sw rs j) (gx sw rs (j + 1))) k
end

theorem gx_lt (sw : Bool) (rs : List Nat) (hu : U16 rs) (j : Nat) : gx sw rs j < 65536 :=
  unit_lt sw _ (g_lt rs hu j)

theorem blk_lt (sw : Bool) (rs : List Nat) (hu : U16 rs) (p : Nat) :
    ∀ i, i < 32 → blk sw rs p i < 65536 := fun i _ => gx_lt sw rs hu (p + i)

/-- A block's count and pairs, the straddle pair included, whatever branch. -/
theorem blk_sums (sw : Bool) (rs : List Nat) (hu : U16 rs) (p : Nat) :
    blkCount (blk sw rs p) = CC sw rs (p + 32) - CC sw rs p ∧ CC sw rs p ≤ CC sw rs (p + 32) ∧
    inMatch (blk sw rs p) + straddle sw rs (p + 31) = PR sw rs (p + 32) - PR sw rs p ∧
    PR sw rs p ≤ PR sw rs (p + 32) ∧
    (anyM (surM (blk sw rs p)) = false → PR sw rs (p + 32) = PR sw rs p) := by
  have hc := sumTo_add (fun j => ccu (gx sw rs j)) p 32
  have hp := sumTo_add (fun j => pru (gx sw rs j) (gx sw rs (j + 1))) p 32
  have hbc := blkCount_eq _ (blk_lt sw rs hu p)
  have hm := inMatch_eq _ (blk_lt sw rs hu p)
  have hs := straddle_eq sw rs hu (p + 31)
  have h32 : sumTo (fun i => pru (gx sw rs (p + i)) (gx sw rs (p + i + 1))) 32 =
      sumTo (fun i => pru (blk sw rs p i) (blk sw rs p (i + 1))) 31 +
        straddle sw rs (p + 31) := by
    rw [show (32 : Nat) = 31 + 1 from rfl, sumTo, hs]
    rfl
  refine ⟨?_, ?_, ?_, ?_, ?_⟩
  · unfold CC; rw [hc, hbc, Nat.add_sub_cancel_left]; rfl
  · unfold CC; rw [hc]; omega
  · unfold PR; rw [hp, hm, ← h32]; simp only [Nat.add_assoc]; omega
  · unfold PR; rw [hp]; omega
  · intro hn
    unfold PR; rw [hp]
    rw [sumTo_congr _ (fun _ => 0) 32 ?_, sumTo_zero]
    · rfl
    intro i hi
    have := not_high_of_surM _ (blk_lt sw rs hu p) hn i hi
    simp only [blk] at this
    simp [pru, ind, gx, this]

/-- Invariant of the unrolled loop: after `b` iterations, `pos = 64 b`, `count`
is `CC` and `matches` is `PR` of `pos` (the straddle pair at `pos - 1` is in). -/
theorem loop1_inv (sw : Bool) (rs : List Nat) (hu : U16 rs) (b : Nat) :
    (iceLoop1 sw rs b).pos = 64 * b ∧ (iceLoop1 sw rs b).cnt8 = CC sw rs (64 * b) ∧
    (iceLoop1 sw rs b).mt = PR sw rs (64 * b) := by
  induction b with
  | zero => exact ⟨rfl, rfl, rfl⟩
  | succ b ih =>
    obtain ⟨i1, i2, i3⟩ := ih
    simp only [iceLoop1]
    generalize iceLoop1 sw rs b = st at *
    obtain ⟨a1, a2, a3, a4, a5⟩ := blk_sums sw rs hu st.pos
    obtain ⟨b1, b2, b3, b4, b5⟩ := blk_sums sw rs hu (st.pos + 32)
    rw [show st.pos + 32 + 31 = st.pos + 63 by omega, show st.pos + 32 + 32 = st.pos + 64 by omega]
      at *
    rw [show 64 * (b + 1) = st.pos + 64 by omega]
    unfold iceStep2N
    simp only
    split
    · refine ⟨rfl, ?_, ?_⟩
      · simp only; rw [a1, b1, i2, ← i1]; omega
      · simp only; rw [i3, ← i1]; omega
    · rename_i hn
      simp only [Bool.or_eq_true, not_or, Bool.not_eq_true] at hn
      refine ⟨rfl, ?_, ?_⟩
      · simp only; rw [a1, b1, i2, ← i1]; omega
      · simp only; rw [i3, ← i1, b5 hn.2, a5 hn.1]

/-- The single `N` block keeps the invariant. -/
theorem stepN_inv (sw : Bool) (rs : List Nat) (hu : U16 rs) (size : Nat) (st : IceSt)
    (h2 : st.cnt8 = CC sw rs st.pos) (h3 : st.mt = PR sw rs st.pos) :
    (iceStepN sw rs size st).cnt8 = CC sw rs (iceStepN sw rs size st).pos ∧
    (iceStepN sw rs size st).mt = PR sw rs (iceStepN sw rs size st).pos ∧
    (iceStepN sw rs size st).pos = (if st.pos + 33 ≤ size then st.pos + 32 else st.pos) := by
  obtain ⟨a1, a2, a3, a4, a5⟩ := blk_sums sw rs hu st.pos
  unfold iceStepN
  split
  · simp only
    split
    · refine ⟨?_, ?_, rfl⟩ <;> simp only <;> omega
    · rename_i hn
      simp only [Bool.not_eq_true] at hn
      refine ⟨?_, ?_, rfl⟩ <;> simp only
      · omega
      · rw [a5 hn]; exact h3
  · exact ⟨h2, h3, rfl⟩

theorem tail_lanes (sw : Bool) (rs : List Nat) (p ov : Nat) (hov : ov < 32) (i : Nat) (hi : i < 32) :
    tailLanes sw rs p ov i = if ov ≤ i then gx sw rs (p - ov + i) else 0 := by
  unfold tailLanes; rw [remMask_spec ov hov i hi]; simp [gx]

/-- The masked tail adds the units `pos .. size - 1` and the pairs starting at
`pos .. size - 2`; the pair at `pos - 1` came from the last straddle word. -/
theorem tail_inv (sw : Bool) (rs : List Nat) (hu : U16 rs) (size : Nat) (st : IceSt)
    (hsz : size = rs.length) (h32 : 32 ≤ size) (hp1 : st.pos + 1 ≤ size) (hp2 : size ≤ st.pos + 32)
    (h2 : st.cnt8 = CC sw rs st.pos) (h3 : st.mt = PR sw rs st.pos) :
    (iceTail sw rs size st).cnt8 = CC sw rs size ∧
    (iceTail sw rs size st).mt = PR sw rs size ∧ (iceTail sw rs size st).pos = size := by
  generalize hov : 32 - (size - st.pos) = ov
  have hov32 : ov < 32 := by omega
  have hlt : ∀ i, i < 32 → tailLanes sw rs st.pos ov i < 65536 := by
    intro i hi; rw [tail_lanes sw rs st.pos ov hov32 i hi]; split
    · exact gx_lt sw rs hu _
    · decide
  have hidx : ∀ k, st.pos - ov + (ov + k) = st.pos + k := by intro k; omega
  -- counts
  have hcount : blkCount (tailLanes sw rs st.pos ov) = CC sw rs size - CC sw rs st.pos ∧
      CC sw rs st.pos ≤ CC sw rs size := by
    rw [blkCount_eq _ hlt]
    have e1 := sumTo_add (fun i => ccu (tailLanes sw rs st.pos ov i)) ov (32 - ov)
    rw [show ov + (32 - ov) = 32 by omega] at e1
    rw [e1, sumTo_congr _ (fun _ => 0) ov (fun i hi => by
      rw [tail_lanes sw rs st.pos ov hov32 i (by omega)]; simp [show ¬ ov ≤ i by omega, ccu, ind]),
      sumTo_zero, Nat.zero_add]
    rw [sumTo_congr _ (fun k => ccu (gx sw rs (st.pos + k))) (32 - ov) (fun k hk => by
      rw [tail_lanes sw rs st.pos ov hov32 _ (by omega)]
      simp only [show ov ≤ ov + k by omega, ite_true, hidx])]
    have e2 := sumTo_add (fun j => ccu (gx sw rs j)) st.pos (32 - ov)
    rw [show st.pos + (32 - ov) = size by omega] at e2
    unfold CC; rw [e2]; omega
  -- pairs
  have hlast : pru (gx sw rs (size - 1)) (gx sw rs (size - 1 + 1)) = 0 := by
    rw [show size - 1 + 1 = size by omega]
    simp [pru, ind, gx, g_past rs size (by omega), unitOf_zero]
    simp [isLow]
  have hmatch : inMatch (tailLanes sw rs st.pos ov) = PR sw rs size - PR sw rs st.pos ∧
      PR sw rs st.pos ≤ PR sw rs size := by
    rw [inMatch_eq _ hlt]
    have e1 := sumTo_add (fun i => pru (tailLanes sw rs st.pos ov i)
      (tailLanes sw rs st.pos ov (i + 1))) ov (31 - ov)
    rw [show ov + (31 - ov) = 31 by omega] at e1
    rw [e1, sumTo_congr _ (fun _ => 0) ov (fun i hi => by
      rw [tail_lanes sw rs st.pos ov hov32 i (by omega)]; simp [show ¬ ov ≤ i by omega, pru, ind]
      simp [isHigh]),
      sumTo_zero, Nat.zero_add]
    rw [sumTo_congr _ (fun k => pru (gx sw rs (st.pos + k)) (gx sw rs (st.pos + k + 1))) (31 - ov)
      (fun k hk => by
        rw [tail_lanes sw rs st.pos ov hov32 _ (by omega),
          tail_lanes sw rs st.pos ov hov32 _ (by omega)]
        simp only [show ov ≤ ov + k by omega, show ov ≤ ov + k + 1 by omega, ite_true, hidx,
          show st.pos - ov + (ov + k + 1) = st.pos + k + 1 by omega])]
    have e2 := sumTo_add (fun j => pru (gx sw rs j) (gx sw rs (j + 1))) st.pos (31 - ov)
    rw [show st.pos + (31 - ov) = size - 1 by omega] at e2
    have e3 : PR sw rs size = PR sw rs (size - 1) := by
      unfold PR
      conv => lhs; rw [show size = size - 1 + 1 by omega]
      rw [sumTo, hlast]; rfl
    rw [e3]; unfold PR; rw [e2]; omega
  unfold iceTail
  simp only [hov]
  split
  · refine ⟨?_, ?_, rfl⟩ <;> simp only
    · rw [hcount.1, h2]; have := hcount.2; omega
    · rw [hmatch.1, h3]; have := hmatch.2; omega
  · rename_i hn
    simp only [Bool.not_eq_true] at hn
    refine ⟨?_, ?_, rfl⟩ <;> simp only
    · rw [hcount.1, h2]; have := hcount.2; omega
    · have hz := inMatch_zero _ hlt hn
      rw [hmatch.1] at hz
      have := hmatch.2
      rw [h3]; omega

theorem L_map_iw (xs : List Nat) :
    L (xs.map iw) = sumTo (fun j => ccu (g xs j)) xs.length + xs.length := by
  induction xs with
  | nil => rfl
  | cons u r ih =>
    simp only [List.map, L_cons, List.length_cons, sumTo_succ_left, ih]
    have : g (u :: r) 0 = u := rfl
    rw [this]
    have e : ∀ j, g (u :: r) (j + 1) = g r j := fun _ => rfl
    simp only [e]
    have hiw : iw u = ccu u + 1 := by
      simp only [iw, ccu, ind]; split <;> split <;> simp_all <;> omega
    rw [hiw]; omega

theorem prs_sum (xs : List Nat) :
    prs xs = sumTo (fun j => pru (g xs j) (g xs (j + 1))) xs.length := by
  induction xs with
  | nil => rfl
  | cons u r ih =>
    rw [List.length_cons, sumTo_succ_left]
    have e : ∀ j, g (u :: r) (j + 1) = g r j := fun _ => rfl
    simp only [e]
    rw [← ih]
    cases r with
    | nil => simp [prs, pru, ind, g, isLow]
    | cons v r' =>
      simp only [prs, pru, ind]
      rfl

/-- **(a) Ice Lake.** The transcribed loops compute `countRepl` of the units,
both endiannesses, on every input; `count -= 2 * matches` does not wrap. -/
theorem iceLen_eq (sw : Bool) (rs : List Nat) (hu : U16 rs) :
    iceLen sw rs = countRepl (rs.map (unitOf sw)) := by
  unfold iceLen
  split
  · rfl
  rename_i hlen
  have hB := Nat.div_mul_le_self (rs.length - 1) 64
  have hB2 := Nat.lt_div_mul_add (a := rs.length - 1) (b := 64) (by decide)
  obtain ⟨i1, i2, i3⟩ := loop1_inv sw rs hu ((rs.length - 1) / 64)
  generalize iceLoop1 sw rs ((rs.length - 1) / 64) = st1 at *
  obtain ⟨n1, n2, n3⟩ := stepN_inv sw rs hu rs.length st1 (by rw [i2, i1]) (by rw [i3, i1])
  generalize iceStepN sw rs rs.length st1 = st2 at *
  have hp : st2.pos + 1 ≤ rs.length ∧ rs.length ≤ st2.pos + 32 := by
    rw [n3]; split <;> omega
  obtain ⟨t1, t2, t3⟩ := tail_inv sw rs hu rs.length st2 rfl (by omega) hp.1 hp.2 n1 n2
  simp only
  rw [t1, t2, t3]
  have hid := icelake_identity (rs.map (unitOf sw))
  rw [L_map_iw, prs_sum] at hid
  have hg : ∀ j, g (rs.map (unitOf sw)) j = gx sw rs j := fun j => g_map rs _ (unitOf_zero sw) j
  simp only [hg, List.length_map] at hid
  unfold CC PR
  omega

/-- The loads of the three loops read inside the input: the unrolled loop
reads `in[pos .. pos + 2N]` (the second straddle word ends at `pos + 2N`),
the `N` block `in[pos .. pos + N]`, the masked tail `in[pos .. size - 1]`
(lanes `overshoot .. 31` of `in + size - 32`), and `overshoot < 32`. -/
theorem ice_loads (size : Nat) (hs : 33 ≤ size) :
    (∀ b, b < (size - 1) / 64 → 64 * b + 64 < size) ∧
    (let p := (size - 1) / 64 * 64
     (p + 33 ≤ size → p + 32 < size) ∧
     let q := if p + 33 ≤ size then p + 32 else p
     q < size ∧ size ≤ q + 32 ∧ 32 - (size - q) < 32 ∧ q - (32 - (size - q)) = size - 32) := by
  have hB := Nat.div_mul_le_self (size - 1) 64
  have hB2 := Nat.lt_div_mul_add (a := size - 1) (b := 64) (by decide)
  refine ⟨fun b hb => ?_, ?_⟩
  · have : (b + 1) * 64 ≤ (size - 1) / 64 * 64 := Nat.mul_le_mul_right 64 hb
    rw [Nat.succ_mul] at this; omega
  · simp only
    refine ⟨fun h => by omega, ?_⟩
    split <;> omega


/-! # (b), (d) UTF-8 → UTF-16: the window boundary and the driver

Line numbers without a file are in src/utf8_to_utf16_with_replacement.inl.h. -/

theorem boundary_spec (bs : List Nat) (l : Nat) :
    boundaryBefore bs l ≤ l ∧
    (boundaryBefore bs l = 0 ∨ isCont (bs.getD (boundaryBefore bs l) 0) = false) := by
  induction l with
  | zero => simp [boundaryBefore]
  | succ i ih =>
    unfold boundaryBefore
    split
    · exact ⟨by omega, ih.2⟩
    · rename_i h; simp only [Bool.not_eq_true] at h; exact ⟨Nat.le_refl _, Or.inr h⟩

/-- `utf8_boundary_before` (src/utf8_to_utf16_with_replacement.inl.h:55-65) as
written: the early return, then the `while` with `(b & 0xC0) == 0x80`. -/
def backoff (bs : List Nat) : Nat → Nat
  | 0 => 0
  | i + 1 => if bs.getD (i + 1) 0 &&& 0xC0 == 0x80 then backoff bs i else i + 1

def boundaryCpp (bs : List Nat) (limit : Nat) : Nat :=
  if !(bs.getD limit 0 &&& 0xC0 == 0x80) then limit else backoff bs limit

theorem getD_byte (bs : List Nat) (hb : Bytes bs) (j : Nat) : bs.getD j 0 < 256 := by
  rw [List.getD_eq_getElem?_getD]
  cases e : bs[j]? with
  | none => simp
  | some v => simpa using hb v (List.mem_of_getElem? e)

theorem boundaryCpp_eq (bs : List Nat) (hb : Bytes bs) (limit : Nat) :
    boundaryCpp bs limit = boundaryBefore bs limit := by
  have hc : ∀ j, (bs.getD j 0 &&& 0xC0 == 0x80) = isCont (bs.getD j 0) := by
    intro j
    rw [(masks _ (getD_byte bs hb j)).1]; unfold isCont; exact decide_eq_decide.mpr (by omega)
  have hk : ∀ l, backoff bs l = boundaryBefore bs l := by
    intro l; induction l with
    | zero => rfl
    | succ i ih => simp only [backoff, boundaryBefore, hc, ih]
  unfold boundaryCpp
  rw [hk]
  cases limit with
  | zero => simp [boundaryBefore]
  | succ i =>
    rw [hc]
    cases h : isCont (bs.getD (i + 1) 0)
    · simp only [h, Bool.not_false, ite_true, boundaryBefore, Bool.false_eq_true, ite_false]
    · rfl

theorem ite_le' {p : Prop} [Decidable p] {a b x : Nat} :
    ((if p then a else b) ≤ x) ↔ ((p → a ≤ x) ∧ (¬p → b ≤ x)) := by
  split <;> simp_all
theorem le_ite' {p : Prop} [Decidable p] {a b x : Nat} :
    (x ≤ (if p then a else b)) ↔ ((p → x ≤ a) ∧ (¬p → x ≤ b)) := by
  split <;> simp_all

theorem dchar_cont (zs : List Nat) (k u : Nat) (h : dchar zs = some (k, u)) :
    ∀ j, 1 ≤ j → j < k → isCont (zs.getD j 0) = true := by
  rcases zs with _ | ⟨b, _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩⟩ <;> simp only [dchar] at h <;>
    repeat' (first | (simp only [reduceCtorEq] at h) | split at h)
  all_goals
    simp only [Option.some.injEq, Prod.mk.injEq] at h
    obtain ⟨rfl, rfl⟩ := h
  all_goals
    intro j hj1 hj2
    try simp only [ite_le', le_ite'] at *
    rcases j with _ | _ | _ | _ | j <;>
      simp only [List.getD_cons_succ, List.getD_cons_zero, isCont, decide_eq_true_eq] <;> omega

theorem dchar_prefix (zs : List Nat) (k u m : Nat) (h : dchar zs = some (k, u)) (hm : k ≤ m) :
    dchar (zs.take m) = some (k, u) := by
  rcases zs with _ | ⟨b, _ | ⟨b1, _ | ⟨b2, _ | ⟨b3, r⟩⟩⟩⟩ <;> simp only [dchar] at h <;>
    repeat' (first | (simp only [reduceCtorEq] at h) | split at h)
  all_goals
    simp only [Option.some.injEq, Prod.mk.injEq] at h
    obtain ⟨rfl, rfl⟩ := h
  all_goals
    obtain ⟨t, rfl⟩ := Nat.exists_eq_add_of_le hm
    rw [Nat.add_comm]
    simp only [List.take_succ_cons, dchar]
    repeat' (first | rfl | omega | (split <;> try omega))

/-- If `dchar` accepts at the head of `zs`, it accepts the same character on
`zs.take m` when byte `m ≥ 1` is not a continuation byte (or `m` is past the
end). -/
theorem dchar_take (zs : List Nat) (m k u : Nat) (h : dchar zs = some (k, u)) (hm : 1 ≤ m)
    (hc : m < zs.length → isCont (zs.getD m 0) = false) : dchar (zs.take m) = some (k, u) := by
  by_cases hkm : k ≤ m
  · exact dchar_prefix zs k u m h hkm
  · have hlen := (dchar_run zs k u h).2.2
    have := dchar_cont zs k u h m hm (by omega)
    rw [hc (by omega)] at this; cases this

/-- **The window ends on a boundary, so its errors are errors of the input.**
`ys` is the rest of the input, `w` the window: either `w` is the end of `ys`
or byte `w` is not a continuation byte. Then `dchar` failing at `e < w` in the
window fails at `e` in `ys`: no sequence is cut by the window's end. -/
theorem window_error (ys : List Nat) (w e : Nat) (hw : w ≤ ys.length)
    (hb : w < ys.length → isCont (ys.getD w 0) = false) (he : e < w)
    (hd : dchar ((ys.take w).drop e) = none) : dchar (ys.drop e) = none := by
  cases h : dchar (ys.drop e) with
  | none => rfl
  | some ku =>
    obtain ⟨k, u⟩ := ku
    have := dchar_take (ys.drop e) (w - e) k u h (by omega) (by
      intro hl
      have : (ys.drop e).getD (w - e) 0 = ys.getD w 0 := by
        simp only [List.getD_eq_getElem?_getD, List.getElem?_drop]
        rw [show e + (w - e) = w by omega]
      rw [this]; apply hb; simp at hl; omega)
    rw [← List.drop_take] at this
    rw [this] at hd; cases hd

/-- The bulk window of the driver (:109-113) when it is used (`window >= 64`):
it ends before a non-continuation byte of the rest of the input. -/
theorem bulk_window (ys : List Nat) (l : Nat) (hl : l < ys.length) (h64 : 64 ≤ boundaryBefore ys l) :
    boundaryBefore ys l ≤ ys.length ∧
    (boundaryBefore ys l < ys.length → isCont (ys.getD (boundaryBefore ys l) 0) = false) := by
  obtain ⟨h1, h2⟩ := boundary_spec ys l
  refine ⟨by omega, fun _ => ?_⟩
  rcases h2 with h | h
  · omega
  · exact h

/-! ## The driver `transcode_utf8_to_utf16_with_replacement<endian, true>` -/

/-- What the driver reads from the bulk `_with_errors` call (:114-115): `ok`
(`error == SUCCESS`), `verr` (`is_utf8_validation_error`), `cnt` (`count`),
and `se`, the end of its stores relative to its output pointer. -/
structure BRes where
  ok : Bool
  verr : Bool
  cnt : Nat
  se : Nat

/-- The contract of the bulk call, on its window `win` (assumed: it is what
`Replacement.lean` part (i) and `ReplacementMore` B2 read from the kernels).
Success: the window is valid and `count` is its UTF-16 length. A validation
error at `count < window`: the bytes before it are valid and `dchar` fails
there, in the window. Stores: at most 4 units past the output of a valid
prefix (`Replacement.gap_all`). -/
def BSpec (K : List Nat → BRes) : Prop :=
  ∀ win : List Nat, Bytes win → win ≠ [] →
    ((K win).ok = true → ValidExact.Valid8 win ∧ (K win).cnt = L (win.map w16)) ∧
    ((K win).ok = false → (K win).verr = true → (K win).cnt < win.length →
      ValidExact.Valid8 (win.take (K win).cnt) ∧ dchar (win.drop (K win).cnt) = none) ∧
    (∃ c, c ≤ win.length ∧ ValidExact.Valid8 (win.take c) ∧
      (K win).se ≤ L ((win.take c).map w16) + 4)

/-- `convert_valid_prefix` (:27-34) returns `(units, end of stores)`: on valid
input, exact (`ValidExact.lean`). -/
def CVSpec (CV : List Nat → Nat × Nat) : Prop :=
  ∀ v, ValidExact.Valid8 v → (CV v).1 = L (v.map w16) ∧ (CV v).2 ≤ L (v.map w16)

/-- `utf16_length_from_utf8` on valid input (:125). -/
def ULSpec (UL : List Nat → Nat) : Prop :=
  ∀ v, ValidExact.Valid8 v → UL v = L (v.map w16)

/-- `VSpec`, and `input_count` is the whole input on success (the driver
converts `input_count` bytes, :161-163). -/
def VSpec2 (V : List Nat → VRes) : Prop :=
  VSpec V ∧ ∀ ys, Bytes ys → ys ≠ [] → (V ys).ok = true → (V ys).p = ys.length

theorem vwcF_len : ∀ f (ys : List Nat), Bytes ys → ys.length ≤ f → (vwcF f ys).1 = true →
    (vwcF f ys).2.1 = ys.length := by
  intro f
  induction f with
  | zero => intro ys _ hl _; simp [vwcF]; omega
  | succ f ih =>
    intro ys hb hl h
    rcases ys with _ | ⟨b, r⟩
    · rfl
    have hv : vchar (b :: r) = dchar (b :: r) := vchar_eq_dchar _ hb
    cases hd : dchar (b :: r) with
    | none =>
      have e : vwcF (f + 1) (b :: r) = (false, 0, 0) := by simp only [vwcF, hv, hd]
      rw [e] at h; cases h
    | some ku =>
      obtain ⟨k, u⟩ := ku
      obtain ⟨-, hk1, hk2⟩ := dchar_run _ k u hd
      have e : vwcF (f + 1) (b :: r) = ((vwcF f ((b :: r).drop k)).1,
          k + (vwcF f ((b :: r).drop k)).2.1, u + (vwcF f ((b :: r).drop k)).2.2) := by
        simp only [vwcF, hv, hd]
      rw [e] at h ⊢
      have := ih ((b :: r).drop k) (bytes_drop hb k) (by simp at hl ⊢; omega) h
      simp only at this ⊢
      rw [this, List.length_drop]; omega

theorem scalarV_spec2 : VSpec2 scalarV :=
  ⟨scalarV_spec, fun ys hb _ h => vwcF_len ys.length ys hb (Nat.le_refl _) h⟩

/-- `dense_prefix_limit` (:24) and the update of `short_prefixes`
(:138-142, :180-184). -/
def shortUpd (short p : Nat) : Nat := if 512 ≤ p then 0 else if p ≠ 0 then short + 1 else short

section
variable (K : List Nat → BRes) (V : List Nat → VRes) (CV : List Nat → Nat × Nat)
  (UL : List Nat → Nat) (xs : List Nat)

/-- **`transcode_utf8_to_utf16_with_replacement<endian, true>`** (:88-188) on
`xs`, with fuel: `some (written, hi)` where `hi` bounds the end of every store
(`none`: fuel exhausted). `finish_with_scalar` (:67-86) writes `count` of the
rest one unit at a time (`Replacement.count`). -/
def tr : Nat → Nat → Nat → Nat → Nat → Option (Nat × Nat)
  | 0, _, _, _, _ => none
  | f + 1, pos, written, short, hi =>
    if pos < xs.length then
      let ys := xs.drop pos
      if 2 ≤ short then some (written + count ys, max hi (written + count ys)) else
      let rem := xs.length - pos
      let win := boundaryBefore ys (rem - 32)
      if 32 + 64 < rem ∧ 64 ≤ win then
        let r := K (ys.take win)
        let hi1 := max hi (written + r.se)
        if r.ok then tr f (pos + win) (written + r.cnt) 0 hi1
        else if r.verr = true ∧ r.cnt < win then
          let w1 := written + (if r.cnt ≠ 0 then UL (ys.take r.cnt) else 0)
          tr f (pos + r.cnt + msub (xs.drop (pos + r.cnt))) (w1 + 1) (shortUpd short r.cnt)
            (max hi1 (w1 + 1))
        else some (written + count ys, max hi1 (written + count ys))
      else
        let v := V ys
        if v.ok = false ∧ (v.verr = false ∨ rem ≤ v.p) then
          some (written + count ys, max hi (written + count ys))
        else
          let units := if v.p ≠ 0 then (CV (ys.take v.p)).1 else v.u
          let hi1 := if v.p ≠ 0 then max hi (written + (CV (ys.take v.p)).2) else hi
          if v.ok then some (written + units, hi1)
          else
            tr f (pos + v.p + msub (xs.drop (pos + v.p))) (written + units + 1)
              (shortUpd short v.p) (max hi1 (written + units + 1))
    else some (written, hi)
end

theorem count_of_valid8 (v : List Nat) (h : ValidExact.Valid8 v) : count v = L (v.map w16) := by
  have := count_valid_append v [] _ (validRun_of_valid8 v h)
  rw [List.append_nil] at this; rw [this]; rfl

theorem take_ne_nil (ys : List Nat) (n : Nat) (h1 : 1 ≤ n) (h2 : 1 ≤ ys.length) : ys.take n ≠ [] := by
  intro h; have := congrArg List.length h
  simp only [List.length_take, List.length_nil] at this; omega

/-- **The driver's stores.** From any reachable state, every store of the rest
of the run (bulk calls, U+FFFD, `convert_valid_prefix`, `finish_with_scalar`)
ends inside `count xs`, and the result is `count xs`, whatever the
`short_prefixes` counter: it only picks between paths. -/
theorem tr_safe (K : List Nat → BRes) (V : List Nat → VRes) (CV : List Nat → Nat × Nat)
    (UL : List Nat → Nat) (hK : BSpec K) (hV : VSpec2 V) (hCV : CVSpec CV) (hUL : ULSpec UL)
    (xs : List Nat) (hb : Bytes xs) :
    ∀ f pos written short hi w' hi', RReach xs pos written → hi ≤ count xs →
      tr K V CV UL xs f pos written short hi = some (w', hi') → w' = count xs ∧ hi' ≤ count xs := by
  intro f
  induction f with
  | zero => intro _ _ _ _ _ _ _ _ h; simp [tr] at h
  | succ f ih =>
    intro pos written short hi w' hi' h hhi hres
    have hinv := rreach_inv xs pos written h
    unfold tr at hres
    simp only at hres
    by_cases hp' : xs.length ≤ pos
    · -- pos ≥ length
      rw [ite_eq_right (by omega)] at hres
      simp only [Option.some.injEq, Prod.mk.injEq] at hres
      obtain ⟨rfl, rfl⟩ := hres
      rw [List.drop_eq_nil_of_le (by omega), count_nil] at hinv
      omega
    have hp : pos < xs.length := by omega
    rw [ite_eq_left hp] at hres
    have hne : xs.drop pos ≠ [] := by
      intro h; have := congrArg List.length h; simp at this; omega
    have hbys : Bytes (xs.drop pos) := bytes_drop hb pos
    split at hres
    · simp only [Option.some.injEq, Prod.mk.injEq] at hres
      obtain ⟨rfl, rfl⟩ := hres; omega
    split at hres
    · -- the bulk call
      rename_i _ hbulk
      obtain ⟨hr, h64⟩ := hbulk
      generalize hw : boundaryBefore (xs.drop pos) (xs.length - pos - 32) = win at *
      have hheld := held_back xs pos hr
      rw [hw] at hheld
      have hwl : (xs.drop pos).take win ≠ [] := take_ne_nil _ win (by omega) (by simp; omega)
      obtain ⟨s1, s2, ⟨c, hc, hv8, hse⟩⟩ :=
        hK ((xs.drop pos).take win) (bytes_take hbys win) hwl
      have hlenw : ((xs.drop pos).take win).length = win := by simp; omega
      rw [hlenw] at hc
      rw [List.take_take, Nat.min_eq_left hc] at hv8 hse
      have hstore := bulk_store xs pos written win c _ _ h hheld hc
        (validRun_of_valid8 _ hv8) hse
      split at hres
      · rename_i hok
        obtain ⟨hv, hcnt⟩ := s1 hok
        have h' := rreach_valid8 h hv
        rw [← hcnt] at h'
        exact ih _ _ _ _ _ _ h' (by omega) hres
      · split at hres
        · rename_i hok hv
          obtain ⟨hverr, hlt⟩ := hv
          have hok' : (K ((xs.drop pos).take win)).ok = false := by simpa using hok
          rw [hlenw] at s2
          obtain ⟨hv8', hd⟩ := s2 hok' hverr hlt
          generalize (K ((xs.drop pos).take win)).cnt = vb at *
          rw [List.take_take, Nat.min_eq_left (by omega)] at hv8'
          obtain ⟨bw1, bw2⟩ := bulk_window (xs.drop pos) (xs.length - pos - 32)
            (by simp; omega) (by rw [hw]; exact h64)
          rw [hw] at bw1 bw2
          have hd' := window_error (xs.drop pos) win vb bw1 bw2 hlt hd
          rw [List.drop_drop] at hd'
          have h' := rreach_error8 hb h hv8' (by omega) hd'
          have hul : (if vb ≠ 0 then UL ((xs.drop pos).take vb) else 0) =
              L (((xs.drop pos).take vb).map w16) := by
            split
            · exact hUL _ hv8'
            · rename_i hz; simp only [Classical.not_not] at hz; subst hz; rfl
          rw [hul] at hres
          have hi2 := rreach_inv xs _ _ h'
          exact ih _ _ _ _ _ _ h' (by omega) hres
        · simp only [Option.some.injEq, Prod.mk.injEq] at hres
          obtain ⟨rfl, rfl⟩ := hres; omega
    · -- the validating path
      obtain ⟨s1, s2⟩ := hV.1 (xs.drop pos) hbys hne
      split at hres
      · simp only [Option.some.injEq, Prod.mk.injEq] at hres
        obtain ⟨rfl, rfl⟩ := hres; omega
      rename_i hnf
      split at hres
      · rename_i hok
        obtain ⟨hv, hu⟩ := s1 hok
        have hp' := hV.2 _ hbys hne hok
        have hc := count_of_valid8 _ hv
        have hpz : (V (xs.drop pos)).p ≠ 0 := by rw [hp']; simp; omega
        have e1 : (xs.drop pos).take (V (xs.drop pos)).p = xs.drop pos := by
          rw [hp']; exact List.take_length
        rw [e1] at hres
        simp only [hpz, ne_eq, not_false_eq_true, ite_true, Option.some.injEq,
          Prod.mk.injEq] at hres
        obtain ⟨rfl, rfl⟩ := hres
        rw [(hCV _ hv).1]
        have := (hCV _ hv).2
        omega
      · rename_i hok
        have hok' : (V (xs.drop pos)).ok = false := by simpa using hok
        have hverr : (V (xs.drop pos)).verr = true := by
          cases e : (V (xs.drop pos)).verr
          · exact absurd ⟨hok', Or.inl e⟩ hnf
          · rfl
        have hlt : (V (xs.drop pos)).p < (xs.drop pos).length := by
          have : ¬ (xs.length - pos ≤ (V (xs.drop pos)).p) := fun hh => hnf ⟨hok', Or.inr hh⟩
          simp; omega
        obtain ⟨hv8, hu, hd⟩ := s2 hok' hverr hlt
        rw [List.drop_drop] at hd
        generalize (V (xs.drop pos)).p = p at *
        have hunits : (if p ≠ 0 then (CV ((xs.drop pos).take p)).1 else (V (xs.drop pos)).u) =
            L (((xs.drop pos).take p).map w16) := by
          split
          · exact (hCV _ hv8).1
          · exact hu
        have hhi1 : (if p ≠ 0 then max hi (written + (CV ((xs.drop pos).take p)).2) else hi) ≤
            max hi (written + L (((xs.drop pos).take p).map w16)) := by
          split
          · have := (hCV _ hv8).2; omega
          · omega
        rw [hunits] at hres
        have h' := rreach_error8 hb h hv8 (by simp at hlt; omega) hd
        have hi2 := rreach_inv xs _ _ h'
        refine ih _ _ _ _ _ _ h' ?_ hres
        omega

theorem msub_pos (ys : List Nat) : 1 ≤ msub ys := by
  unfold msub
  cases ys with
  | nil => exact Nat.le_refl _
  | cons b r =>
    simp only
    repeat' split
    all_goals first | omega | (unfold msubTail; split <;> (try split) <;> omega)

/-- Every iteration advances `pos` (a window of at least 64 bytes, or a
`maximal_subpart` of at least one byte), so `length + 1` steps of fuel
always suffice: the driver terminates. -/
theorem tr_some (K : List Nat → BRes) (V : List Nat → VRes) (CV : List Nat → Nat × Nat)
    (UL : List Nat → Nat) (xs : List Nat) :
    ∀ f pos written short hi, xs.length - pos < f →
      (tr K V CV UL xs f pos written short hi).isSome := by
  intro f
  induction f with
  | zero => intro _ _ _ _ h; omega
  | succ f ih =>
    intro pos written short hi hf
    unfold tr
    simp only
    repeat' split
    all_goals first
      | rfl
      | (apply ih; have := msub_pos (xs.drop (pos + (K ((xs.drop pos).take
          (boundaryBefore (xs.drop pos) (xs.length - pos - 32)))).cnt)); omega)
      | (apply ih; have := msub_pos (xs.drop (pos + (V (xs.drop pos)).p)); omega)
      | (apply ih; omega)

section
variable (K : List Nat → BRes) (V : List Nat → VRes) (CV : List Nat → Nat × Nat)
  (UL : List Nat → Nat) (xs : List Nat)

/-- `transcode_utf8_to_utf16_with_replacement<endian, true>` on a whole input,
with enough fuel (`tr_some`). -/
def trRun (ys : List Nat) : Option (Nat × Nat) := tr K V CV UL ys (ys.length + 1) 0 0 0 0

/-- The recording loop of `utf16_length_from_utf8_with_replacement`
(src/utf8_to_utf16_with_replacement.inl.h:238-276): every error offset
`err = pos + input_count` in order (:264), and whether it gave up on a
non-validation error or an `input_count` past the rest (:250-258). The C++
keeps the first `max_errors = 16` (:267-272, include/simdutf/error.h:162) and
sets `more_errors` for the others or when it gave up. -/
def locF : Nat → Nat → List Nat × Bool
  | 0, _ => ([], false)
  | f + 1, pos =>
    if xs.drop pos = [] then ([], false)
    else if (V (xs.drop pos)).ok then ([], false)
    else if (V (xs.drop pos)).verr = false ∨ (xs.drop pos).length ≤ (V (xs.drop pos)).p then
      ([], true)
    else
      let err := pos + (V (xs.drop pos)).p
      (err :: (locF f (err + msub (xs.drop err))).1, (locF f (err + msub (xs.drop err))).2)

/-- **`convert_using_locations`** (src/utf8_to_utf16_with_replacement.inl.h:194-236), from the loop over the recorded
offsets on: the guard `err < pos || err >= length` (:208-212) hands the rest
to `transcode`; the gap before `err` goes to `convert_valid_prefix`
(:213-216); one U+FFFD (:217-218); skip the `maximal_subpart` (:219-221).
After the offsets: the guard `pos > length` (:223-225); without
`more_errors`, the last gap (:226-231); with it, `transcode` on the rest
(:233-235). Returns `(returned count, end of stores)`. -/
def cul : List Nat → Bool → Nat → Nat → Nat → Nat × Nat
  | [], more, pos, written, hi =>
    if xs.length < pos then (written, hi)
    else if more = false then
      if pos < xs.length then
        (written + (CV (xs.drop pos)).1, max hi (written + (CV (xs.drop pos)).2))
      else (written, hi)
    else match trRun K V CV UL (xs.drop pos) with
      | some (w, h) => (written + w, max hi (written + h))
      | none => (written, hi)
  | err :: es, more, pos, written, hi =>
    if err < pos ∨ xs.length ≤ err then
      match trRun K V CV UL (xs.drop pos) with
      | some (w, h) => (written + w, max hi (written + h))
      | none => (written, hi)
    else
      let w1 := if pos < err then written + (CV ((xs.drop pos).take (err - pos))).1 else written
      let hi1 := if pos < err then max hi (written + (CV ((xs.drop pos).take (err - pos))).2)
        else hi
      cul es more (err + msub (xs.drop err)) (w1 + 1) (max hi1 (w1 + 1))

/-- `convert_utf8_to_utf16{le,be,}_with_replacement(input, length, out)`
(src/utf8_to_utf16_with_replacement.inl.h:278-323): the locations of `utf16_length_from_utf8_with_replacement`,
capped at 16 offsets, then `convert_using_locations`. The cap of :200-203
does not fire (`error_count ≤ 16`). -/
def convertRepl : Nat × Nat :=
  cul K V CV UL xs ((locF V xs (xs.length + 1) 0).1.take 16)
    ((locF V xs (xs.length + 1) 0).2 || decide (16 < (locF V xs (xs.length + 1) 0).1.length)) 0 0 0
end

theorem trRun_safe (K : List Nat → BRes) (V : List Nat → VRes) (CV : List Nat → Nat × Nat)
    (UL : List Nat → Nat) (hK : BSpec K) (hV : VSpec2 V) (hCV : CVSpec CV) (hUL : ULSpec UL)
    (ys : List Nat) (hb : Bytes ys) :
    ∃ w h, trRun K V CV UL ys = some (w, h) ∧ w = count ys ∧ h ≤ count ys := by
  have hs := tr_some K V CV UL ys (ys.length + 1) 0 0 0 0 (by omega)
  unfold trRun
  cases e : tr K V CV UL ys (ys.length + 1) 0 0 0 0 with
  | none => rw [e] at hs; cases hs
  | some r =>
    obtain ⟨w, h⟩ := r
    obtain ⟨h1, h2⟩ := tr_safe K V CV UL hK hV hCV hUL ys hb _ 0 0 0 0 w h RReach.start
      (Nat.zero_le _) e
    exact ⟨w, h, rfl, h1, h2⟩

theorem cul_rest (K : List Nat → BRes) (V : List Nat → VRes) (CV : List Nat → Nat × Nat)
    (UL : List Nat → Nat) (hK : BSpec K) (hV : VSpec2 V) (hCV : CVSpec CV) (hUL : ULSpec UL)
    (xs : List Nat) (hb : Bytes xs) (pos written hi : Nat) (hinv : written + count (xs.drop pos) = count xs)
    (hhi : hi ≤ count xs) :
    (match trRun K V CV UL (xs.drop pos) with
      | some (w, h) => (written + w, max hi (written + h))
      | none => (written, hi)).1 = count xs ∧
    (match trRun K V CV UL (xs.drop pos) with
      | some (w, h) => (written + w, max hi (written + h))
      | none => (written, hi)).2 ≤ count xs := by
  obtain ⟨w, h, e, h1, h2⟩ := trRun_safe K V CV UL hK hV hCV hUL (xs.drop pos) (bytes_drop hb pos)
  rw [e]
  exact ⟨by simp only; rw [h1, hinv], by simp only; omega⟩

/-- Invariant of `convert_using_locations` on the offsets recorded from `pos`
on (any prefix of `n` of them, `more_errors` set when some are left out):
it returns `count xs` and every store ends inside it. Along the way the
guards of src/utf8_to_utf16_with_replacement.inl.h:208 and :223 never fire (`err ≥ pos`, `err < length`,
`pos ≤ length`). -/
theorem cul_inv (K : List Nat → BRes) (V : List Nat → VRes) (CV : List Nat → Nat × Nat)
    (UL : List Nat → Nat) (hK : BSpec K) (hV : VSpec2 V) (hCV : CVSpec CV) (hUL : ULSpec UL)
    (xs : List Nat) (hb : Bytes xs) :
    ∀ f n pos written hi, RReach xs pos written → pos ≤ xs.length → hi ≤ count xs →
      xs.length - pos < f →
      (cul K V CV UL xs ((locF V xs f pos).1.take n)
        ((locF V xs f pos).2 || decide (n < (locF V xs f pos).1.length)) pos written hi).1 = count xs ∧
      (cul K V CV UL xs ((locF V xs f pos).1.take n)
        ((locF V xs f pos).2 || decide (n < (locF V xs f pos).1.length)) pos written hi).2 ≤ count xs := by
  intro f
  induction f with
  | zero => intro _ _ _ _ _ _ _ h; omega
  | succ f ih =>
    intro n pos written hi h hpos hhi hf
    have hinv := rreach_inv xs pos written h
    have hrest := cul_rest K V CV UL hK hV hCV hUL xs hb pos written hi hinv hhi
    by_cases hne : xs.drop pos = []
    · have e : locF V xs (f + 1) pos = ([], false) := by simp only [locF, hne, ite_true]
      rw [e]
      have hp : xs.length ≤ pos := by simpa using hne
      rw [hne, count_nil] at hinv
      simp only [List.take_nil, List.length_nil, Bool.false_or, cul]
      simp [show ¬ xs.length < pos by omega, show ¬ pos < xs.length by omega]
      omega
    obtain ⟨s1, s2⟩ := hV.1 (xs.drop pos) (bytes_drop hb pos) hne
    by_cases hok : (V (xs.drop pos)).ok = true
    · have e : locF V xs (f + 1) pos = ([], false) := by simp only [locF, hne, hok, ite_true, ite_false]
      rw [e]
      obtain ⟨hv, -⟩ := s1 hok
      have hp : pos < xs.length := by
        refine Nat.lt_of_not_le fun h' => hne ?_; simpa using h'
      simp only [List.take_nil, List.length_nil, Nat.not_lt_zero, decide_false, Bool.or_false, cul]
      rw [ite_eq_right (by omega), ite_eq_left trivial, ite_eq_left hp]
      have := (hCV _ hv).2
      rw [(hCV _ hv).1, ← count_of_valid8 _ hv] at *
      exact ⟨hinv, by simp only; omega⟩
    have hok' : (V (xs.drop pos)).ok = false := by simpa using hok
    by_cases hfb : (V (xs.drop pos)).verr = false ∨ (xs.drop pos).length ≤ (V (xs.drop pos)).p
    · have e : locF V xs (f + 1) pos = ([], true) := by
        simp only [locF, hne, hok', hfb, ite_true, ite_false, Bool.false_eq_true]
      rw [e]
      simp only [List.take_nil, Bool.true_or, cul]
      rw [ite_eq_right (by omega), ite_eq_right (by simp)]
      exact hrest
    have hverr : (V (xs.drop pos)).verr = true := by
      cases e : (V (xs.drop pos)).verr
      · exact absurd (Or.inl e) hfb
      · rfl
    have hlt : (V (xs.drop pos)).p < (xs.drop pos).length := by
      have := fun hh => hfb (Or.inr hh); omega
    obtain ⟨hv8, -, hd⟩ := s2 hok' hverr hlt
    rw [List.drop_drop] at hd
    have e : locF V xs (f + 1) pos =
        ((pos + (V (xs.drop pos)).p) :: (locF V xs f (pos + (V (xs.drop pos)).p +
          msub (xs.drop (pos + (V (xs.drop pos)).p)))).1,
         (locF V xs f (pos + (V (xs.drop pos)).p + msub (xs.drop (pos + (V (xs.drop pos)).p)))).2) := by
      simp only [locF, hne, hok', hfb, ite_false, Bool.false_eq_true]
    rw [e]
    generalize hP : (V (xs.drop pos)).p = p at *
    have hlt' : pos + p < xs.length := by simp at hlt; omega
    have h' := rreach_error8 hb h hv8 hlt' hd
    have hsub := subpart_of_none (xs.drop (pos + p)) (bytes_drop hb _)
      (by intro hh; have := congrArg List.length hh; simp at this; omega) hd
    have hi2 := rreach_inv xs _ _ h'
    cases n with
    | zero =>
      simp only [List.take_zero, List.length_cons, Nat.zero_lt_succ, decide_true, Bool.or_true, cul]
      rw [ite_eq_right (by omega), ite_eq_right (by simp)]
      exact hrest
    | succ n =>
      simp only [List.take_succ_cons, List.length_cons, Nat.add_lt_add_iff_right, cul]
      rw [ite_eq_right (by omega)]
      have hgap : (if pos < pos + p then written + (CV ((xs.drop pos).take (pos + p - pos))).1
          else written) = written + L (((xs.drop pos).take p).map w16) := by
        split
        · rw [show pos + p - pos = p by omega, (hCV _ hv8).1]
        · rw [show p = 0 by omega]; rfl
      have hgap2 : (if pos < pos + p then max hi (written + (CV ((xs.drop pos).take (pos + p - pos))).2)
          else hi) ≤ max hi (written + L (((xs.drop pos).take p).map w16)) := by
        split
        · rw [show pos + p - pos = p by omega]; have := (hCV _ hv8).2; omega
        · omega
      rw [hgap]
      exact ih n _ _ _ h' (by have := hsub.2.2; simp at this; omega)
        (by omega) (by have := hsub.2.1; omega)

/-- **(b)+(d), `convert_utf8_to_utf16_with_replacement`.** With the locations
of `utf16_length_from_utf8_with_replacement` on the same input, the
conversion returns `count xs`, the buffer size (`ReplacementMore.lenRepl_eq`),
and every store ends inside it. -/
theorem convertRepl_safe (K : List Nat → BRes) (V : List Nat → VRes) (CV : List Nat → Nat × Nat)
    (UL : List Nat → Nat) (hK : BSpec K) (hV : VSpec2 V) (hCV : CVSpec CV) (hUL : ULSpec UL)
    (xs : List Nat) (hb : Bytes xs) :
    (convertRepl K V CV UL xs).1 = count xs ∧ (convertRepl K V CV UL xs).2 ≤ count xs :=
  cul_inv K V CV UL hK hV hCV hUL xs hb _ 16 0 0 0 RReach.start (Nat.zero_le _) (Nat.zero_le _)
    (by omega)

/-! ## The buffer of `length` units -/

/-- Invariant for `count ≤ length`: `w` units out after `n` bytes in. -/
def Inv2 (s : St) (n w : Nat) : Prop :=
  (s.needed = 0 → w ≤ n) ∧
  (s.needed ≠ 0 → w + 1 + s.seen ≤ n ∧ s.seen < s.needed ∧ s.needed ≤ 3 ∧
    (s.needed = 1 → s.cp < 32) ∧ (s.needed = 2 → s.seen = 0 → s.cp < 16) ∧
    (s.needed = 2 → s.seen = 1 → s.cp < 1024))

theorem lead_inv2 (b n w : Nat) (h : w + 1 ≤ n + 1) :
    Inv2 (lead b).1 (n + 1) (w + (lead b).2) := by
  unfold lead Inv2 init
  repeat' split
  all_goals (refine ⟨?_, ?_⟩ <;> intros <;> simp_all <;> omega)

theorem step_inv2 (s : St) (b n w : Nat) (h : Inv2 s n w) :
    Inv2 (step s b).1 (n + 1) (w + (step s b).2) := by
  obtain ⟨h0, h1⟩ := h
  unfold step
  by_cases hn : s.needed = 0
  · rw [ite_eq_left hn]; exact lead_inv2 b n w (by have := h0 hn; omega)
  · rw [ite_eq_right hn]
    obtain ⟨hw, hsn, hn3, c1, c20, c21⟩ := h1 hn
    by_cases hr : b < s.lo ∨ s.hi < b
    · rw [ite_eq_left hr]
      have := lead_inv2 b n (w + 1) (by omega)
      simpa [Nat.add_assoc] using this
    · rw [ite_eq_right hr]
      by_cases hc : s.seen + 1 = s.needed
      · rw [ite_eq_left hc]
        refine ⟨fun _ => ?_, fun h => absurd rfl h⟩
        unfold units
        split
        · -- two units: only a 4-byte sequence reaches 0x10000
          have h3 : s.needed = 3 := by
            rcases (show s.needed = 1 ∨ s.needed = 2 ∨ s.needed = 3 by omega) with e | e | e
            · have := c1 e; omega
            · have := c21 e (by omega); omega
            · exact e
          omega
        · omega
      · rw [ite_eq_right hc]
        refine ⟨fun h => absurd h hn, fun _ => ?_⟩
        simp only
        refine ⟨by omega, by omega, hn3, fun e => by omega, fun _ e => by omega, fun e _ => ?_⟩
        have := c20 e (by omega); omega

theorem run_inv2 (bs : List Nat) : ∀ (s : St) (n w : Nat), Inv2 s n w →
    Inv2 (run s bs).1 (n + bs.length) (w + (run s bs).2) := by
  induction bs with
  | nil => intro s n w h; simpa [run] using h
  | cons b bs ih =>
    intro s n w h
    have h2 := ih _ _ _ (step_inv2 s b n w h)
    simp only [run, List.length_cons]
    have e1 : n + (bs.length + 1) = n + 1 + bs.length := by omega
    have e2 : w + ((step s b).2 + (run (step s b).1 bs).2) =
        w + (step s b).2 + (run (step s b).1 bs).2 := by omega
    rw [e1, e2]; exact h2

/-- **A buffer of `length` units is always large enough**
(include/simdutf/implementation.h:1526-1527, :2101-2102): `count ≤ length`. -/
theorem count_le_length (bs : List Nat) : count bs ≤ bs.length := by
  have h := run_inv2 bs init 0 0 ⟨fun _ => by omega, fun h => absurd rfl h⟩
  simp only [Nat.zero_add] at h
  obtain ⟨h0, h1⟩ := h
  unfold count
  by_cases hn : (run init bs).1.needed = 0
  · rw [ite_eq_left hn]; have := h0 hn; omega
  · rw [ite_eq_right hn]; obtain ⟨a, _⟩ := h1 hn; omega

/-! # (c) UTF-16 → UTF-8: the scalar `convert_with_replacement` and the `_safe` entry points

Units after the byte swap of the non-native endianness (`u16_swap_bytes`, as
the C++ applies it to every read). -/

/-- The unit tests of include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h,
per 16-bit value: `(w & 0xFF80) == 0`, `(w & 0xF800) == 0`,
`(w & 0xF800) != 0xD800`, `uint16_t(w - 0xD800) <= 0x3FF`,
`uint16_t(w - 0xDC00) <= 0x3FF`, `(w & 0xfc00) == 0xdc00`, `== 0xd800`. -/
def u16Facts (u : Nat) : Bool :=
  ((u &&& 0xFF80 == 0) == decide (u < 0x80)) && ((u &&& 0xF800 == 0) == decide (u < 0x800)) &&
  ((u &&& 0xF800 != 0xD800) == !isSur u) &&
  (decide ((u + 65536 - 0xD800) % 65536 ≤ 0x3FF) == isHigh u) &&
  (decide ((u + 65536 - 0xDC00) % 65536 ≤ 0x3FF) == isLow u) &&
  ((u &&& 0xfc00 == 0xdc00) == isLow u) && ((u &&& 0xfc00 == 0xd800) == isHigh u)

/-- Checked on all 65536 values (`decide +kernel`, a finite exhaustive check). -/
theorem u16Facts_all : (List.range 65536).all u16Facts = true := by decide +kernel

theorem uf (u : Nat) (h : u < 65536) :
    ((u &&& 0xFF80 == 0) = decide (u < 0x80)) ∧ ((u &&& 0xF800 == 0) = decide (u < 0x800)) ∧
    ((u &&& 0xF800 != 0xD800) = !isSur u) ∧
    (decide ((u + 65536 - 0xD800) % 65536 ≤ 0x3FF) = isHigh u) ∧
    (decide ((u + 65536 - 0xDC00) % 65536 ≤ 0x3FF) = isLow u) ∧
    ((u &&& 0xfc00 == 0xdc00) = isLow u) ∧ ((u &&& 0xfc00 == 0xd800) = isHigh u) := by
  have := List.all_eq_true.mp u16Facts_all u (List.mem_range.mpr h)
  simp only [u16Facts, Bool.and_eq_true, beq_iff_eq] at this
  obtain ⟨⟨⟨⟨⟨⟨a, b⟩, c⟩, d⟩, e⟩, f⟩, g'⟩ := this
  exact ⟨a, b, c, d, e, f, g'⟩

/-! ## The scalar `convert_with_replacement` (fallback, and `consteval`) -/

section
variable (xs : List Nat) (probe : Nat → Bool)

/-- **`scalar::utf16_to_utf8::convert_with_replacement`**
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:211-290), with fuel:
the number of bytes written (it writes them in order, so the end of its
stores). `probe pos` is the 8-byte ASCII test (:223-230): it writes one byte
for each of 4 units (:231-238). -/
def scr : Nat → Nat → Nat → Nat
  | 0, _, out => out
  | f + 1, pos, out =>
    if pos < xs.length then
      if pos + 4 ≤ xs.length ∧ probe pos = true then scr f (pos + 4) (out + 4)
      else
        let w := g xs pos
        if w &&& 0xFF80 == 0 then scr f (pos + 1) (out + 1)
        else if w &&& 0xF800 == 0 then scr f (pos + 1) (out + 2)
        else if w &&& 0xF800 != 0xD800 then scr f (pos + 1) (out + 3)
        else if (w + 65536 - 0xD800) % 65536 ≤ 0x3FF ∧ pos + 1 < xs.length ∧
            (g xs (pos + 1) + 65536 - 0xDC00) % 65536 ≤ 0x3FF then scr f (pos + 2) (out + 4)
        else scr f (pos + 1) (out + 3)
    else out
end

theorem cR_drop_plain (xs : List Nat) (k : Nat) (hk : k < xs.length) (hs : isSur (g xs k) = false) :
    countRepl (xs.drop k) = w8 (g xs k) + countRepl (xs.drop (k + 1)) := by
  rw [drop_g xs k hk, cR_plain _ _ hs]

theorem w8_lt80 (u : Nat) (h : u < 0x80) : w8 u = 1 := by
  unfold w8 Arch.Icelake.w8; simp [h]

theorem w8_mid (u : Nat) (h1 : 0x80 ≤ u) (h2 : u < 0x800) : w8 u = 2 := by
  unfold w8 Arch.Icelake.w8; repeat' split
  all_goals omega

theorem w8_hi (u : Nat) (h1 : 0x800 ≤ u) (h3 : isSur u = false) : w8 u = 3 := by
  simp only [isSur, decide_eq_false_iff_not] at h3
  unfold w8 Arch.Icelake.w8; repeat' split
  all_goals omega

/-- **The scalar `convert_with_replacement` writes exactly
`utf8_length_from_utf16_with_replacement` bytes** (`countRepl`), given that
the ASCII probe is sound (the 64-bit test `(v & 0xFF80FF80FF80FF80) == 0`,
with the rotation `v >> 8 | v << 56` in the swapped case, is read from the
C++ as "4 ASCII units"). -/
theorem scr_eq (xs : List Nat) (probe : Nat → Bool) (hu : U16 xs)
    (hprobe : ∀ p, probe p = true → p + 4 ≤ xs.length → ∀ k, k < 4 → g xs (p + k) < 0x80) :
    ∀ f pos out, xs.length - pos < f → scr xs probe f pos out = out + countRepl (xs.drop pos) := by
  intro f
  induction f with
  | zero => intro _ _ h; omega
  | succ f ih =>
    intro pos out hf
    unfold scr
    by_cases hp' : xs.length ≤ pos
    · rw [ite_eq_right (by omega), List.drop_eq_nil_of_le hp']; rfl
    have hp : pos < xs.length := by omega
    rw [ite_eq_left hp]
    have hw := g_lt xs hu pos
    obtain ⟨a1, a2, a3, a4, a5, -, -⟩ := uf _ hw
    have hd := drop_g xs pos hp
    simp only
    split
    · rename_i hpr
      obtain ⟨h4, hpb⟩ := hpr
      have ha := hprobe pos hpb h4
      rw [ih _ _ (by omega)]
      have step : ∀ k, k < 4 →
          countRepl (xs.drop (pos + k)) = 1 + countRepl (xs.drop (pos + k + 1)) := by
        intro k hk
        have hlt := ha k hk
        rw [cR_drop_plain xs (pos + k) (by omega) (by simp [isSur]; omega), w8_lt80 _ hlt]
      have s0 : countRepl (xs.drop pos) = 1 + countRepl (xs.drop (pos + 1)) := step 0 (by omega)
      have s1 : countRepl (xs.drop (pos + 1)) = 1 + countRepl (xs.drop (pos + 2)) :=
        step 1 (by omega)
      have s2 : countRepl (xs.drop (pos + 2)) = 1 + countRepl (xs.drop (pos + 3)) :=
        step 2 (by omega)
      have s3 : countRepl (xs.drop (pos + 3)) = 1 + countRepl (xs.drop (pos + 4)) :=
        step 3 (by omega)
      omega
    rw [a1, a2, a3]
    split
    · rename_i h1; replace h1 : g xs pos < 0x80 := by simpa using h1
      rw [ih _ _ (by omega), cR_drop_plain xs pos hp (by simp [isSur]; omega), w8_lt80 _ h1]
      omega
    split
    · rename_i h1 h2
      replace h1 : ¬ g xs pos < 0x80 := by simpa using h1
      replace h2 : g xs pos < 0x800 := by simpa using h2
      rw [ih _ _ (by omega), cR_drop_plain xs pos hp (by simp [isSur]; omega)]
      have : w8 (g xs pos) = 2 := w8_mid _ (by omega) h2
      omega
    split
    · rename_i h1 h2 h3
      replace h1 : ¬ g xs pos < 0x80 := by simpa using h1
      replace h2 : ¬ g xs pos < 0x800 := by simpa using h2
      replace h3 : isSur (g xs pos) = false := by simpa using h3
      rw [ih _ _ (by omega), cR_drop_plain xs pos hp h3]
      have : w8 (g xs pos) = 3 := w8_hi _ (by omega) h3
      omega
    rename_i h1 h2 h3
    replace h3 : isSur (g xs pos) = true := by simpa using h3
    split
    · rename_i hc
      obtain ⟨hh, hp1, hl⟩ := hc
      have hl' : isLow (g xs (pos + 1)) = true := by
        rw [← (uf _ (g_lt xs hu (pos + 1))).2.2.2.2.1]; simpa using hl
      have hh' : isHigh (g xs pos) = true := by rw [← a4]; simpa using hh
      rw [ih _ _ (by omega), hd, drop_g xs (pos + 1) hp1, cR_pair _ _ _ hh' hl',
        show pos + 1 + 1 = pos + 2 by omega]
      omega
    · rename_i hc
      rw [ih _ _ (by omega), hd]
      by_cases hh : isHigh (g xs pos) = true
      · rw [cR_high _ _ hh]
        · omega
        intro v r' hv
        have hp1 : pos + 1 < xs.length := by
          refine Nat.lt_of_not_le fun hc' => ?_
          rw [List.drop_eq_nil_of_le hc'] at hv; cases hv
        rw [drop_g xs (pos + 1) hp1] at hv
        simp only [List.cons.injEq] at hv
        rw [← hv.1]
        cases e : isLow (g xs (pos + 1))
        · rfl
        · exfalso; apply hc
          refine ⟨by rw [← a4] at hh; simpa using hh, hp1, ?_⟩
          have := (uf _ (g_lt xs hu (pos + 1))).2.2.2.2.1
          rw [e] at this; simpa using this
      · have hl : isLow (g xs pos) = true := by
          simp [isSur, isHigh, isLow] at h3 hh ⊢; omega
        rw [cR_low _ _ hl]; omega

/-- From the start, with fuel `length + 1`: the scalar `convert_with_replacement`
writes `countRepl xs` bytes, in order, so its stores end at
`utf8_length_from_utf16_with_replacement`. -/
theorem scr_total (xs : List Nat) (probe : Nat → Bool) (hu : U16 xs)
    (hprobe : ∀ p, probe p = true → p + 4 ≤ xs.length → ∀ k, k < 4 → g xs (p + k) < 0x80) :
    scr xs probe (xs.length + 1) 0 0 = countRepl xs := by
  have := scr_eq xs probe hu hprobe (xs.length + 1) 0 0 (by omega)
  simpa using this

/-! ## The `_safe` conversion -/

/-- A `full_result` (`error`, `input_count`, `output_count`) and `hi`, the end
of the stores (relative to the output pointer of the call). Error codes:
0 `SUCCESS`, 1 `SURROGATE`, 2 `OUTPUT_BUFFER_TOO_SMALL`, others as they come
(3 also marks exhausted fuel of a model). -/
structure FR where
  err : Nat
  inp : Nat
  outp : Nat
  hi : Nat

/-- The contract the drivers need from a conversion with a capacity `cap`:
stores and output count within `cap`, input count within the input, and a
`SURROGATE` or `OUTPUT_BUFFER_TOO_SMALL` reported at a unit of the input. -/
def FSpec (F : List Nat → Nat → FR) : Prop :=
  ∀ ys cap, U16 ys → (F ys cap).hi ≤ cap ∧ (F ys cap).outp ≤ cap ∧ (F ys cap).inp ≤ ys.length ∧
    ((F ys cap).err = 1 ∨ (F ys cap).err = 2 → (F ys cap).inp < ys.length ∨ ys.length = 0)

section
variable (xs : List Nat) (cap : Nat) (probe : Nat → Bool)

/-- **`scalar::utf16_to_utf8::convert_with_errors<big_endian, true>`**
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:93-203), the loop with
fuel; `burst` is what is left of the 4-unit ASCII block (:116-136), each
unit checked `end - utf8_output < 1` before its store. Every other unit is
checked for the room its encoding needs (1, 2, 3, or 4 for a surrogate,
before the pairing is examined, :173-176) before storing. -/
def cweLoop : Nat → Nat → Nat → Nat → Nat → FR
  | 0, _, pos, out, hi => ⟨3, pos, out, hi⟩
  | f + 1, burst, pos, out, hi =>
    if burst ≠ 0 then
      if cap - out < 1 then ⟨2, pos, out, hi⟩
      else cweLoop f (burst - 1) (pos + 1) (out + 1) (max hi (out + 1))
    else if pos < xs.length then
      if pos + 4 ≤ xs.length ∧ probe pos = true then cweLoop f 4 pos out hi
      else
        let w := g xs pos
        if w &&& 0xFF80 == 0 then
          if cap - out < 1 then ⟨2, pos, out, hi⟩
          else cweLoop f 0 (pos + 1) (out + 1) (max hi (out + 1))
        else if w &&& 0xF800 == 0 then
          if cap - out < 2 then ⟨2, pos, out, hi⟩
          else cweLoop f 0 (pos + 1) (out + 2) (max hi (out + 2))
        else if w &&& 0xF800 != 0xD800 then
          if cap - out < 3 then ⟨2, pos, out, hi⟩
          else cweLoop f 0 (pos + 1) (out + 3) (max hi (out + 3))
        else if cap - out < 4 then ⟨2, pos, out, hi⟩
        else if xs.length ≤ pos + 1 then ⟨1, pos, out, hi⟩
        else if 0x3FF < (w + 65536 - 0xD800) % 65536 then ⟨1, pos, out, hi⟩
        else if 0x3FF < (g xs (pos + 1) + 65536 - 0xDC00) % 65536 then ⟨1, pos, out, hi⟩
        else cweLoop f 0 (pos + 2) (out + 4) (max hi (out + 4))
    else ⟨0, pos, out, hi⟩
end

/-- `convert_with_errors<big_endian, true>(data, len, out, utf8_len)`:
`utf8_len == 0` returns at once (:102-104). `probe ys pos` is the ASCII test
on the call's input. -/
def cwe (probe : List Nat → Nat → Bool) (ys : List Nat) (cap : Nat) : FR :=
  if cap = 0 then ⟨2, 0, 0, 0⟩ else cweLoop ys cap (probe ys) (2 * ys.length + 2) 0 0 0 0

theorem cweLoop_safe (xs : List Nat) (cap : Nat) (probe : Nat → Bool) :
    ∀ f burst pos out hi, out ≤ cap → hi ≤ cap → pos ≤ xs.length →
      (burst ≠ 0 → pos + burst ≤ xs.length) →
      (cweLoop xs cap probe f burst pos out hi).hi ≤ cap ∧
      (cweLoop xs cap probe f burst pos out hi).outp ≤ cap ∧
      (cweLoop xs cap probe f burst pos out hi).inp ≤ xs.length ∧
      ((cweLoop xs cap probe f burst pos out hi).err = 1 ∨
        (cweLoop xs cap probe f burst pos out hi).err = 2 →
        (cweLoop xs cap probe f burst pos out hi).inp < xs.length) := by
  intro f
  induction f with
  | zero => intro burst pos out hi h1 h2 h3 _; simp [cweLoop]; omega
  | succ f ih =>
    intro burst pos out hi h1 h2 h3 h4
    unfold cweLoop
    simp only
    repeat' split
    all_goals first
      | (simp only; omega)
      | (apply ih <;> intros <;> omega)
      | (refine ⟨by omega, by omega, by omega, fun _ => by omega⟩)

theorem cwe_spec (probe : List Nat → Nat → Bool) : FSpec (cwe probe) := by
  intro ys cap _
  unfold cwe
  split
  · simp only; omega
  · have := cweLoop_safe ys cap (probe ys) (2 * ys.length + 2) 0 0 0 0 (by omega) (by omega)
      (by omega) (by omega)
    exact ⟨this.1, this.2.1, this.2.2.1, fun h => Or.inl (this.2.2.2 h)⟩

theorem U16_drop {ys : List Nat} (h : U16 ys) (k : Nat) : U16 (ys.drop k) :=
  fun u hu => h u (List.mem_of_mem_drop hu)
theorem U16_take {ys : List Nat} (h : U16 ys) (k : Nat) : U16 (ys.take k) :=
  fun u hu => h u (List.mem_of_mem_take hu)

/-- The unpaired test of `convert_with_replacement_safe` on an
`OUTPUT_BUFFER_TOO_SMALL` (include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:319-340):
a low surrogate, or a high one at the end or not followed by a low one. It
reads `data[input_count]`, inside the input (`crs_safe`), and
`data[input_count + 1]` only when it exists. -/
def unpairedAt (xs : List Nat) (i : Nat) : Bool :=
  (g xs i &&& 0xfc00 == 0xdc00) ||
    ((g xs i &&& 0xfc00 == 0xd800) && (i + 1 == xs.length || !(g xs (i + 1) &&& 0xfc00 == 0xdc00)))

section
variable (SK : List Nat → Nat → FR) (xs : List Nat) (cap : Nat)

/-- **`scalar::utf16_to_utf8::convert_with_replacement_safe`**
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:292-355), the loop with
fuel, over a kernel `SK` (`convert_with_errors<big_endian, true>`, `cwe`): on
`OUTPUT_BUFFER_TOO_SMALL` it returns unless 3 bytes are left and the unit is
an unpaired surrogate; on any error but `SURROGATE` it returns; otherwise it
checks for 3 bytes, writes U+FFFD and skips one unit (:345-352). -/
def crsLoop : Nat → Nat → Nat → Nat → FR
  | 0, ic, oc, hi => ⟨3, ic, oc, hi⟩
  | f + 1, ic, oc, hi =>
    if ic < xs.length then
      let r := SK (xs.drop ic) (cap - oc)
      let ic1 := ic + r.inp
      let oc1 := oc + r.outp
      let hi1 := max hi (oc + r.hi)
      if r.err = 0 then ⟨0, ic1, oc1, hi1⟩
      else if r.err = 2 ∧ (cap - oc1 < 3 ∨ unpairedAt xs ic1 = false) then ⟨2, ic1, oc1, hi1⟩
      else if r.err ≠ 2 ∧ r.err ≠ 1 then ⟨r.err, ic1, oc1, hi1⟩
      else if cap - oc1 < 3 then ⟨2, ic1, oc1, hi1⟩
      else crsLoop f (ic1 + 1) (oc1 + 3) (max hi1 (oc1 + 3))
    else ⟨0, ic, oc, hi⟩

/-- With its early returns for `len == 0` and `utf8_len == 0` (:299-304). -/
def crs : FR :=
  if xs.length = 0 then ⟨0, 0, 0, 0⟩ else if cap = 0 then ⟨2, 0, 0, 0⟩
  else crsLoop SK xs cap (xs.length + 1) 0 0 0
end

theorem crsLoop_safe (SK : List Nat → Nat → FR) (hSK : FSpec SK) (xs : List Nat) (hu : U16 xs)
    (cap : Nat) : ∀ f ic oc hi, oc ≤ cap → hi ≤ cap → ic ≤ xs.length →
      (crsLoop SK xs cap f ic oc hi).hi ≤ cap ∧ (crsLoop SK xs cap f ic oc hi).outp ≤ cap ∧
      (crsLoop SK xs cap f ic oc hi).inp ≤ xs.length ∧
      ((crsLoop SK xs cap f ic oc hi).err = 1 ∨ (crsLoop SK xs cap f ic oc hi).err = 2 →
        (crsLoop SK xs cap f ic oc hi).inp < xs.length ∨ xs.length = 0) := by
  intro f
  induction f with
  | zero => intro ic oc hi h1 h2 h3; simp [crsLoop]; omega
  | succ f ih =>
    intro ic oc hi h1 h2 h3
    unfold crsLoop
    by_cases hic' : xs.length ≤ ic
    · rw [ite_eq_right (by omega)]; simp only; omega
    have hic : ic < xs.length := by omega
    rw [ite_eq_left hic]
    obtain ⟨s1, s2, s3, s4⟩ := hSK (xs.drop ic) (cap - oc) (U16_drop hu ic)
    simp only [List.length_drop] at s3 s4
    generalize SK (xs.drop ic) (cap - oc) = r at *
    simp only
    split
    · simp only; omega
    split
    · rename_i _ h; simp only; refine ⟨by omega, by omega, by omega, fun _ => ?_⟩
      have := s4 (Or.inr h.1); omega
    split
    · simp only; refine ⟨by omega, by omega, by omega, fun hh => by omega⟩
    split
    · rename_i _ _ hh _
      simp only; refine ⟨by omega, by omega, by omega, fun _ => ?_⟩
      have : r.err = 1 ∨ r.err = 2 := by omega
      have := s4 this; omega
    rename_i _ _ hh _
    have : r.err = 1 ∨ r.err = 2 := by omega
    have := s4 this
    apply ih <;> omega

/-- **`convert_with_replacement_safe` stays inside its buffer**: every store
and the returned output count are within `utf8_len`, for every kernel that
meets `FSpec` (the scalar one does: `cwe_spec`). -/
theorem crs_spec (SK : List Nat → Nat → FR) (hSK : FSpec SK) : FSpec (crs SK) := by
  intro xs cap hu
  unfold crs
  split
  · simp only; omega
  split
  · simp only; omega
  have := crsLoop_safe SK hSK xs hu cap (xs.length + 1) 0 0 0 (by omega) (by omega) (by omega)
  exact this

/-- What `convert_utf16_to_utf8_safe_with_details` reads from
`simdutf::convert_utf16_to_utf8_with_errors` (src/implementation.cpp:2059-2060):
`e` (error code, 0 = `SUCCESS`), `cnt` (`count`), and `se`, the end of its
stores. -/
structure KR where
  e : Nat
  cnt : Nat
  se : Nat

/-- The contract of the vector `_with_errors` kernel, assumed: its stores end
within `utf8_length_from_utf16` of its input (what the store theorems of the
UTF-16 → UTF-8 kernels give, `Replacement.via_paths`, `via_icelake_real`,
`via_rvv`), so does `count` on success, and on error `count` is the position
of a unit of the input. -/
def VKSpec (VK : List Nat → KR) : Prop :=
  ∀ ys, U16 ys → (VK ys).se ≤ L (ys.map w8) ∧ ((VK ys).e = 0 → (VK ys).cnt ≤ L (ys.map w8)) ∧
    ((VK ys).e ≠ 0 → (VK ys).cnt < ys.length)

/-- `simdutf::utf8_length_from_utf16` (src/implementation.cpp:2062-2063): the standard length. -/
def SLSpec (SL : List Nat → Nat) : Prop := ∀ ys, U16 ys → SL ys = L (ys.map w8)

theorem L_w8_le (ys : List Nat) : L (ys.map w8) ≤ 3 * ys.length := by
  induction ys with
  | nil => simp
  | cons u r ih =>
    simp only [List.map, L_cons, List.length_cons]
    have : w8 u ≤ 3 := by unfold w8 Arch.Icelake.w8; repeat' (first | omega | split)
    omega

section
variable (VK : List Nat → KR) (SL : List Nat → Nat) (SK : List Nat → Nat → FR) (xs : List Nat)

/-- The scalar end of `convert_utf16_to_utf8_safe_with_details`
(src/implementation.cpp:2077-2090): done if nothing is left, else
`convert_with_errors<…, true>` on the rest with the room left. -/
def dTail (pos oc cap' hi : Nat) : FR :=
  if xs.length - pos = 0 then ⟨0, pos, oc, hi⟩
  else
    let r := SK (xs.drop pos) cap'
    ⟨r.err, pos + r.inp, oc + r.outp, max hi (oc + r.hi)⟩

/-- **`convert_utf16_to_utf8_safe_with_details`** (src/implementation.cpp:2028-2091),
the vector loop with fuel: `read_len = min(len, utf8_len / 3)` (:2044); stop
at `read_len <= 16` (:2045-2047); back off one unit before a high surrogate
that is not the last unit (:2048-2054, `high_surrogate` of
include/simdutf/scalar/utf16.h:147-149); stop at `read_len == 0`
(src/implementation.cpp:2055-2058); the kernel on `read_len` units; on error, return with the
standard length of the units before the error (:2061-2067); else advance
(:2069-2075). `cap'` is `utf8_len` as the loop decrements it. -/
def dLoop : Nat → Nat → Nat → Nat → Nat → FR
  | 0, pos, oc, _, hi => ⟨3, pos, oc, hi⟩
  | f + 1, pos, oc, cap', hi =>
    let rl0 := min (xs.length - pos) (cap' / 3)
    if rl0 ≤ 16 then dTail SK xs pos oc cap' hi else
    let rl := if rl0 < xs.length - pos ∧ isHigh (g xs (pos + rl0 - 1)) = true then rl0 - 1 else rl0
    if rl = 0 then dTail SK xs pos oc cap' hi else
    let r := VK ((xs.drop pos).take rl)
    let hi1 := max hi (oc + r.se)
    if r.e ≠ 0 then ⟨r.e, pos + r.cnt, oc + SL ((xs.drop pos).take r.cnt), hi1⟩
    else dLoop f (pos + rl) (oc + r.cnt) (cap' - r.cnt) hi1

/-- With `len == 0` (src/implementation.cpp:2031-2033). -/
def dSafe (ys : List Nat) (cap : Nat) : FR :=
  if ys.length = 0 then ⟨0, 0, 0, 0⟩ else dLoop VK SL SK ys (ys.length + 1) 0 0 cap 0
end

theorem dLoop_safe (VK : List Nat → KR) (SL : List Nat → Nat) (SK : List Nat → Nat → FR)
    (hVK : VKSpec VK) (hSL : SLSpec SL) (hSK : FSpec SK) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    ∀ f pos oc cap' hi, oc + cap' = cap → hi ≤ cap → pos ≤ xs.length →
      (dLoop VK SL SK xs f pos oc cap' hi).hi ≤ cap ∧ (dLoop VK SL SK xs f pos oc cap' hi).outp ≤ cap ∧
      (dLoop VK SL SK xs f pos oc cap' hi).inp ≤ xs.length ∧
      ((dLoop VK SL SK xs f pos oc cap' hi).err = 1 ∨ (dLoop VK SL SK xs f pos oc cap' hi).err = 2 →
        (dLoop VK SL SK xs f pos oc cap' hi).inp < xs.length) := by
  have tail : ∀ pos oc cap' hi, oc + cap' = cap → hi ≤ cap → pos ≤ xs.length →
      (dTail SK xs pos oc cap' hi).hi ≤ cap ∧ (dTail SK xs pos oc cap' hi).outp ≤ cap ∧
      (dTail SK xs pos oc cap' hi).inp ≤ xs.length ∧
      ((dTail SK xs pos oc cap' hi).err = 1 ∨ (dTail SK xs pos oc cap' hi).err = 2 →
        (dTail SK xs pos oc cap' hi).inp < xs.length) := by
    intro pos oc cap' hi h1 h2 h3
    unfold dTail
    split
    · simp only; omega
    · obtain ⟨s1, s2, s3, s4⟩ := hSK (xs.drop pos) cap' (U16_drop hu pos)
      simp only [List.length_drop] at s3 s4 ⊢
      refine ⟨by omega, by omega, by omega, fun h => ?_⟩
      have := s4 h; omega
  intro f
  induction f with
  | zero => intro pos oc cap' hi h1 h2 h3; simp [dLoop]; omega
  | succ f ih =>
    intro pos oc cap' hi h1 h2 h3
    unfold dLoop
    simp only
    split
    · exact tail pos oc cap' hi h1 h2 h3
    generalize hrl : (if min (xs.length - pos) (cap' / 3) < xs.length - pos ∧
        isHigh (g xs (pos + min (xs.length - pos) (cap' / 3) - 1)) = true
      then min (xs.length - pos) (cap' / 3) - 1 else min (xs.length - pos) (cap' / 3)) = rl
    have hrl1 : rl ≤ xs.length - pos ∧ 3 * rl ≤ cap' := by
      rw [← hrl]; split <;> omega
    split
    · exact tail pos oc cap' hi h1 h2 h3
    have hlen : ((xs.drop pos).take rl).length = rl := by simp; omega
    obtain ⟨k1, k2, k3⟩ := hVK ((xs.drop pos).take rl) (U16_take (U16_drop hu pos) rl)
    have hL := L_w8_le ((xs.drop pos).take rl)
    rw [hlen] at hL k3
    generalize VK ((xs.drop pos).take rl) = r at *
    split
    · rename_i he
      have hc := k3 he
      rw [hSL _ (U16_take (U16_drop hu pos) _)]
      have := L_w8_le ((xs.drop pos).take r.cnt)
      have : ((xs.drop pos).take r.cnt).length = r.cnt := by simp; omega
      simp only
      refine ⟨by omega, by omega, by omega, fun _ => by omega⟩
    · rename_i he
      have hc := k2 (by simpa using he)
      apply ih <;> omega

/-- **`convert_utf16_to_utf8_safe_with_details` stays inside its buffer.** -/
theorem dSafe_spec (VK : List Nat → KR) (SL : List Nat → Nat) (SK : List Nat → Nat → FR)
    (hVK : VKSpec VK) (hSL : SLSpec SL) (hSK : FSpec SK) : FSpec (dSafe VK SL SK) := by
  intro ys cap hu
  unfold dSafe
  split
  · simp only; omega
  have := dLoop_safe VK SL SK hVK hSL hSK ys hu cap (ys.length + 1) 0 0 cap 0 (by omega)
    (by omega) (by omega)
  exact ⟨this.1, this.2.1, this.2.2.1, fun h => Or.inl (this.2.2.2 h)⟩

section
variable (D R : List Nat → Nat → FR) (xs : List Nat) (cap : Nat)

/-- **`convert_utf16_to_utf8_with_replacement_safe`** (src/implementation.cpp:2494-2539),
the loop with fuel, over `D` (`convert_utf16_to_utf8_safe_with_details`,
`dSafe`) and `R` (`scalar::…::convert_with_replacement_safe`, `crs`): on
success return; on `OUTPUT_BUFFER_TOO_SMALL` hand the rest to `R` with the
room left (:2508-2525); on any error but `SURROGATE` return; else check for
3 bytes, write U+FFFD and skip one unit (:2526-2537). -/
def tLoop : Nat → Nat → Nat → Nat → FR
  | 0, ic, oc, hi => ⟨3, ic, oc, hi⟩
  | f + 1, ic, oc, hi =>
    if ic < xs.length then
      let r := D (xs.drop ic) (cap - oc)
      let ic1 := ic + r.inp
      let oc1 := oc + r.outp
      let hi1 := max hi (oc + r.hi)
      if r.err = 0 then ⟨0, ic1, oc1, hi1⟩
      else if r.err = 2 then
        let t := R (xs.drop ic1) (cap - oc1)
        ⟨t.err, ic1 + t.inp, oc1 + t.outp, max hi1 (oc1 + t.hi)⟩
      else if r.err ≠ 1 then ⟨r.err, ic1, oc1, hi1⟩
      else if cap - oc1 < 3 then ⟨2, ic1, oc1, hi1⟩
      else tLoop f (ic1 + 1) (oc1 + 3) (max hi1 (oc1 + 3))
    else ⟨0, ic, oc, hi⟩
end

theorem tLoop_safe (D R : List Nat → Nat → FR) (hD : FSpec D) (hR : FSpec R) (xs : List Nat)
    (hu : U16 xs) (cap : Nat) : ∀ f ic oc hi, oc ≤ cap → hi ≤ cap → ic ≤ xs.length →
      (tLoop D R xs cap f ic oc hi).hi ≤ cap ∧ (tLoop D R xs cap f ic oc hi).outp ≤ cap ∧
      (tLoop D R xs cap f ic oc hi).inp ≤ xs.length := by
  intro f
  induction f with
  | zero => intro ic oc hi h1 h2 h3; simp [tLoop]; omega
  | succ f ih =>
    intro ic oc hi h1 h2 h3
    unfold tLoop
    by_cases hic' : xs.length ≤ ic
    · rw [ite_eq_right (by omega)]; simp only; omega
    rw [ite_eq_left (by omega)]
    obtain ⟨s1, s2, s3, s4⟩ := hD (xs.drop ic) (cap - oc) (U16_drop hu ic)
    simp only [List.length_drop] at s3 s4
    generalize D (xs.drop ic) (cap - oc) = r at *
    simp only
    split
    · simp only; omega
    split
    · rename_i _ h
      obtain ⟨t1, t2, t3, -⟩ := hR (xs.drop (ic + r.inp)) (cap - (oc + r.outp))
        (U16_drop hu _)
      simp only [List.length_drop] at t3
      simp only; omega
    split
    · simp only; omega
    split
    · simp only; omega
    rename_i _ _ h1' _
    have := s4 (Or.inl (by omega))
    apply ih <;> omega

/-- **(c) `convert_utf16_to_utf8_with_replacement_safe` never writes past
`utf8_len`**, and its `output_count` is at most `utf8_len`: with `dSafe` over
the vector kernel and the scalar tail `cwe`, and `crs` over `cwe`, every store
of every path ends inside the buffer. Assumed: `VKSpec` of the vector
`_with_errors` kernel and `SLSpec` of the vector standard length. -/
theorem replSafe_safe (VK : List Nat → KR) (SL : List Nat → Nat) (hVK : VKSpec VK)
    (hSL : SLSpec SL) (probe : List Nat → Nat → Bool) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (tLoop (dSafe VK SL (cwe probe)) (crs (cwe probe)) xs cap (xs.length + 1) 0 0 0).hi ≤ cap ∧
    (tLoop (dSafe VK SL (cwe probe)) (crs (cwe probe)) xs cap (xs.length + 1) 0 0 0).outp ≤ cap :=
  let h := tLoop_safe _ _ (dSafe_spec VK SL _ hVK hSL (cwe_spec probe))
    (crs_spec _ (cwe_spec probe)) xs hu cap (xs.length + 1) 0 0 0 (by omega) (by omega) (by omega)
  ⟨h.1, h.2.1⟩

/-- **(c) The scalar counterpart** (`scalar::utf16_to_utf8::convert_with_replacement_safe`,
used by the `consteval` branch of include/simdutf/implementation.h:2856-2872)
never writes past `utf8_len`. -/
theorem crs_safe (probe : List Nat → Nat → Bool) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (crs (cwe probe) xs cap).hi ≤ cap ∧ (crs (cwe probe) xs cap).outp ≤ cap :=
  let h := crs_spec _ (cwe_spec probe) xs cap hu
  ⟨h.1, h.2.1⟩

/-! # Examples -/

/-- Ice Lake on 101 units: one unrolled iteration (pairs at units 31/32,
across the two vectors, and 63/64, the second straddle word), the single `N`
block at 64, the masked tail at `in + 69` (`overshoot = 27`) with a pair at
97/98, and unpaired surrogates at 65, 96 and 100. The transcribed loops
agree with `countRepl`, both endiannesses. -/
example :
    let xs := (List.replicate 31 0x41) ++ [0xD800, 0xDC00] ++ (List.replicate 30 0x3B1) ++
      [0xD83D, 0xDE00, 0xDC00] ++ (List.replicate 30 0x20AC) ++ [0xD800, 0xDBFF, 0xDFFF] ++
      [0x41, 0xD800]
    xs.length = 101 ∧ iceLen false xs = countRepl xs ∧
      iceLen true (xs.map bswap) = countRepl xs := by
  decide +kernel

/-- A window cut inside a character would turn a valid character into an
error: `C3 A9` (é) cut after `C3`. `window_error` rules this out. -/
example : dchar ([0xC3, 0xA9].take 1) = none ∧ dchar [0xC3, 0xA9] = some (2, 1) := by decide

end SafetyMargins.ReplacementRest
