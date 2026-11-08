import SafetyMargins.LengthsArch
/-
The icelake length functions (src/icelake/implementation.cpp,
src/icelake/icelake_utf8_length_from_utf16.inl.cpp).

| model | C++ (src/icelake/implementation.cpp unless noted) | theorem |
|---|---|---|
| `iceCount16` | `count_utf16le/be` = `utf32_length_from_utf16le/be` (src/icelake/implementation.cpp:1629-1700, :1793-1801) | `iceCount16_eq` |
| `iceCount8` | `count_utf8` = `latin1_length_from_utf8` = `utf32_length_from_utf8` (src/icelake/implementation.cpp:1702-1770, :1774-1777, :2040-2043) | `iceCount8_eq` |
| `ice8of16` | `icelake_utf8_length_from_utf16` (src/icelake/icelake_utf8_length_from_utf16.inl.cpp:1-112), for `utf8_length_from_utf16le/be` (src/icelake/implementation.cpp:1781-1789) | `ice8of16_eq`: the per-unit sum on 33 units or more, **`countRepl`** below |
| `ice16of8` | `utf16_length_from_utf8` (src/icelake/implementation.cpp:1889-1963) | `ice16of8_eq` |
| `iceLatin1` | `utf8_length_from_latin1` (src/icelake/implementation.cpp:1805-1886) | `iceLatin1_eq` |
| `ice16of32` | `utf16_length_from_utf32` (src/icelake/implementation.cpp:2006-2037) | `ice16of32_eq` |
| `genU8of32 false 16` | `utf8_length_from_utf32` (src/icelake/implementation.cpp:2000-2003) | `LengthsGeneric.genU8of32_eq` |

Masked tails: `_bzhi_u32/_u64(~0, r)` keeps lanes `0 .. r - 1`;
`maskz_loadu` reads zeros elsewhere; the masked compares and the
`maskz_shuffle` drop the other lanes, and in `ice8of16` and `iceLatin1`,
whose compares are not masked, a zero lane contributes nothing
(`lane16_zero`, `neg8 0 = false`).

**`icelake_utf8_length_from_utf16` on short inputs (`ice8of16_eq`).** For
fewer than 33 units it returns
`scalar::utf16::utf8_length_from_utf16_with_replacement(...).count`
(src/icelake/icelake_utf8_length_from_utf16.inl.cpp:7-12), which counts an
unpaired surrogate as 3 bytes (U+FFFD) instead of 2: `[0xD800]` gives 3, the
per-unit sum is 2 (`ice8of16_witness`). It is never below the per-unit sum
(`Replacement.std_le_repl`), so buffers sized with it are large enough, but
`ReplacementRest.SLSpec` fails for it (`icelake_not_SLSpec`). On valid
UTF-16 the two agree.

The byte table of `utf16_length_from_utf8`
(src/icelake/implementation.cpp:1894-1917, `iceTbl`) is indexed
by the high nibble: `_mm512_srli_epi32(utf8, 4)` then `& 0xf` leaves in each
byte its own high nibble (the bits shifted in from the next byte are
cleared); that is read from the C++.
-/
namespace SafetyMargins.LengthsIcelake
open SafetyMargins SafetyMargins.ReplacementMore SafetyMargins.RvvExact SafetyMargins.Lengths
  SafetyMargins.LengthsGeneric SafetyMargins.LengthsArch
set_option autoImplicit false

/-! ## `count_utf16` -/

/-- `ptr` advances by 32 while `ptr <= end`, `end = input + length - 32`, when
`length >= 32` (src/icelake/implementation.cpp:1636-1647): `length / 32`
iterations, each `count += count_ones(cmpgt_epu16(utf16, 0xdfff) |
cmplt_epu16(utf16, 0xdc00))` (:1640-1645); return if nothing is left (:1648-1650);
the masked tail on `r = length - 32 B` lanes (:1652-1658). The big-endian
function (:1661-1700) applies the byte flip to the vectors (masked-off lanes
are zero and stay zero, and are not compared). -/
def iceCount16 (xs : List Nat) : Nat :=
  let B := xs.length / 32
  let count := sumTo (fun b => sumTo (fun i => ind (nirLo (g xs (b * 32 + i)))) 32) B
  if B * 32 = xs.length then count
  else count + sumTo (fun i => ind (nirLo (g xs (B * 32 + i)))) (xs.length - B * 32)

/-- A masked tail on the last `r` units counts them. -/
theorem tail_sum (w lane : Nat → Nat) (n : Nat) (hn : 0 < n) (xs : List Nat) (hx : ∀ u ∈ xs, u < n)
    (hl : ∀ u, u < n → lane u = w u) (P : Nat) :
    sumTo (fun i => lane (g xs (P + i))) (xs.length - P) = L ((xs.drop P).map w) := by
  rw [sumTo_congr _ (fun i => w (g (xs.drop P) i)) _ (fun i _ => by rw [g_drop]; exact hl _ (g_lt_of hx hn _))]
  have := sum_all w (xs.drop P)
  simpa using this

theorem iceCount16_eq (xs : List Nat) (hu : U16 xs) : iceCount16 xs = L (xs.map w32) := by
  unfold iceCount16
  simp only
  have hB := div_mul_le xs.length 32
  have h := block_model w32 (fun u => ind (nirLo u)) (fun u => ind (nirLo u)) 65536 (by decide) xs hu
    (fun u h => (lane16 u h).1) (fun u h => (lane16 u h).1) 32 _ hB _ (fun _ _ => rfl)
  have ht := tail_sum w32 (fun u => ind (nirLo u)) 65536 (by decide) xs hu (fun u h => (lane16 u h).1) (xs.length / 32 * 32)
  rw [scal_drop _ w32 xs 65536 hu (fun u h => (lane16 u h).1)] at h
  split
  · rename_i he
    have : L ((xs.drop (xs.length / 32 * 32)).map w32) = 0 := by rw [he]; simp
    omega
  · rw [ht]; exact h

/-! ## `count_utf8` -/

/-- `_mm512_cmple_epi8_mask(input, 0b10111111)`, signed: a continuation byte. -/
def cont8 (b : Nat) : Bool := decide (i8 b ≤ -65)

theorem cont8_all : (List.range 256).all (fun b => 1 - ind (cont8 b) == w01 b) = true := by
  decide +kernel

theorem cont8_eq (b : Nat) (h : b < 256) : 1 - ind (cont8 b) = w01 b ∧ ind (cont8 b) ≤ 1 := by
  have := List.all_eq_true.mp cont8_all b (List.mem_range.mpr h)
  simp only [beq_iff_eq] at this
  exact ⟨this, ind_le _⟩

/-- The popcount of the continuation mask of 64-byte block `b`. -/
def contBlk (xs : List Nat) (b _l : Nat) : Nat := sumTo (fun j => ind (cont8 (g xs (b * 64 + j)))) 64

/-- **icelake `count_utf8`** (src/icelake/implementation.cpp:1702-1770).
`answer = length / 64 * 64` (:1705-1707). The loop `while (i + 64 <= length)` (:1713)
runs once when `length >= 64`: `iterations = (length - i) / 64` (:1714) and
the two inner loops (`grp_sep`) end at `i + 64 iterations`, after which
`i + 64 > length`. The unrolled loop `for (; i + 8 * 64 <= max_i; i += 8 * 64)` (:1717)
adds the eight popcounts into `unrolled_popcount` (:1734-1748); the single loop
`for (; i <= max_i; i += 64)` (:1750) subtracts each from `answer` (:1751-1755);
`answer -= reduce_add(unrolled_popcount)` (:1758). If `i == length` return
(:1760-1762); else the masked tail adds `length - i` minus its continuation
bytes (:1764-1768). The `size_t` subtractions do not wrap: what is subtracted
is at most `answer`. -/
def iceCount8 (xs : List Nat) : Nat :=
  let it := xs.length / 64
  let p := grp 0 8 (contBlk xs) 0 (it - 1) it 0 (fun _ => 0)
  let q := grp 0 1 (contBlk xs) 0 it (it + 1) p.1 (fun _ => 0)
  let answer := it * 64 - q.2 0 - p.2 0
  if it * 64 = xs.length then answer
  else answer + (xs.length - it * 64) - sumTo (fun j => ind (cont8 (g xs (it * 64 + j)))) (xs.length - it * 64)

theorem iceCount8_eq (xs : List Nat) (hb : Bytes xs) : iceCount8 xs = L (xs.map w01) := by
  unfold iceCount8
  simp only
  generalize hit : xs.length / 64 = it
  have hB : it * 64 ≤ xs.length := hit ▸ div_mul_le _ _
  generalize hp : grp 0 8 (contBlk xs) 0 (it - 1) it 0 (fun _ => 0) = p
  generalize hq : grp 0 1 (contBlk xs) 0 it (it + 1) p.1 (fun _ => 0) = q
  obtain ⟨-, hs⟩ := grp_sep 8 (contBlk xs) 0 it (by decide) p q hp.symm hq.symm
  have hs0 := hs 0
  simp only [Nat.zero_add] at hs0
  unfold contBlk at hs0
  rw [sum_blocks (fun j => ind (cont8 (g xs j))) 64 it] at hs0
  -- the vector part: `64 it - (continuation bytes) = sum of w01`
  have hc : ∀ m, sumTo (fun j => ind (cont8 (g xs j))) m ≤ m ∧
      m - sumTo (fun j => ind (cont8 (g xs j))) m = sumTo (fun j => w01 (g xs j)) m := by
    intro m
    induction m with
    | zero => simp [sumTo]
    | succ m ih =>
      simp only [sumTo_succ]
      have := cont8_eq _ (g_byte hb m); omega
  have hct : ∀ m, sumTo (fun j => ind (cont8 (g xs (it * 64 + j)))) m ≤ m ∧
      m - sumTo (fun j => ind (cont8 (g xs (it * 64 + j)))) m = sumTo (fun j => w01 (g xs (it * 64 + j))) m := by
    intro m
    induction m with
    | zero => simp [sumTo]
    | succ m ih =>
      simp only [sumTo_succ]
      have := cont8_eq _ (g_byte hb (it * 64 + m)); omega
  have h1 := hc (it * 64)
  have ht := sum_tail w01 xs (it * 64) hB
  have htail := tail_sum w01 w01 256 (by decide) xs hb (fun _ _ => rfl) (it * 64)
  have h2 := hct (xs.length - it * 64)
  split
  · rename_i he
    have : L ((xs.drop (it * 64)).map w01) = 0 := by rw [he]; simp
    omega
  · omega

/-! ## `utf8_length_from_utf16` -/

/-- The lane of `count_ones32(c0) + count_ones32(c1) - count_ones32(is_surrogate)`
(src/icelake/icelake_utf8_length_from_utf16.inl.cpp:35-57): `test(0xff80)`,
`test(0xf800)`, `(v & 0xf800) == 0xd800`. -/
def iceA (u : Nat) : Nat := ind (u &&& 0xff80 != 0)
def iceC (u : Nat) : Nat := ind (u &&& 0xf800 != 0)
def iceS (u : Nat) : Nat := ind (u &&& 0xf800 == 0xd800)

def iceLaneOK (u : Nat) : Bool := iceS u ≤ iceC u && iceA u + iceC u - iceS u == w8 u - 1

theorem iceLanes : (List.range 65536).all iceLaneOK = true := by decide +kernel

theorem ice_lane (u : Nat) (h : u < 65536) : iceS u ≤ iceC u ∧ iceA u + iceC u - iceS u = w8 u - 1 := by
  have := List.all_eq_true.mp iceLanes u (List.mem_range.mpr h)
  simp only [iceLaneOK, Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq] at this
  exact this

/-- A zero lane (masked off) counts nothing. -/
theorem lane16_zero : iceA 0 = 0 ∧ iceC 0 = 0 ∧ iceS 0 = 0 := by decide

/-- One 32-lane vector: `count += A; count += C; count -= S`. -/
def iceVec (count : Nat) (f : Nat → Nat) : Nat :=
  count + sumTo (fun i => iceA (f i)) 32 + sumTo (fun i => iceC (f i)) 32 - sumTo (fun i => iceS (f i)) 32

theorem iceVec_eq (count : Nat) (f : Nat → Nat) (hf : ∀ i, f i < 65536) (m : Nat) :
    count + sumTo (fun i => iceA (f i)) m + sumTo (fun i => iceC (f i)) m - sumTo (fun i => iceS (f i)) m =
      count + sumTo (fun i => w8 (f i) - 1) m := by
  have : ∀ m, sumTo (fun i => iceS (f i)) m ≤ sumTo (fun i => iceC (f i)) m ∧
      sumTo (fun i => iceA (f i)) m + sumTo (fun i => iceC (f i)) m - sumTo (fun i => iceS (f i)) m =
        sumTo (fun i => w8 (f i) - 1) m := by
    intro m
    induction m with
    | zero => simp [sumTo]
    | succ m ih =>
      simp only [sumTo_succ]
      have := ice_lane _ (hf m); omega
  have := this m; omega

theorem iceVec_eq' (count : Nat) (f : Nat → Nat) (hf : ∀ i, f i < 65536) :
    iceVec count f = count + sumTo (fun i => w8 (f i) - 1) 32 := iceVec_eq count f hf 32

/-- **`icelake_utf8_length_from_utf16`**
(src/icelake/icelake_utf8_length_from_utf16.inl.cpp:1-112), on the units after
the byte flip. `N + 1 > size`: the scalar function with replacement, its
`count` (:7-12, `countRepl`). Otherwise the loop
`for (; pos < size / (2 * N) * (2 * N); pos += 2 * N)` (:22) over two vectors
(:24-57); one more vector if `pos + N <= size` (:59-82); the masked tail of
`size - pos` lanes, `remaining_mask = 0xFFFFFFFF >> (32 - (size - pos))`
(:85-107); `pos = size`, `count += pos` (:108-111). -/
def ice8of16 (xs : List Nat) : Nat :=
  if 32 + 1 > xs.length then Replacement.countRepl xs else
  let B := xs.length / 64
  let c1 := sumTo (fun b => iceVec (iceVec 0 (fun i => g xs (b * 64 + i))) (fun i => g xs (b * 64 + 32 + i))) B
  let one := B * 64 + 32 ≤ xs.length
  let c2 := if one then iceVec c1 (fun i => g xs (B * 64 + i)) else c1
  let pos := if one then B * 64 + 32 else B * 64
  let r := xs.length - pos
  let c3 := iceVec c2 (fun i => if (0xFFFFFFFF >>> (32 - r)).testBit i then g xs (pos + i) else 0)
  c3 + xs.length

/-- `remaining_mask` has the lanes below `r` (all `r ≤ 32`). -/
theorem remMask32 : ∀ r, r ≤ 32 → ∀ i, i < 32 → (0xFFFFFFFF >>> (32 - r)).testBit i = decide (i < r) := by
  decide +kernel

theorem ice8of16_eq (xs : List Nat) (hu : U16 xs) :
    ice8of16 xs = if 32 + 1 > xs.length then Replacement.countRepl xs else L (xs.map w8) := by
  unfold ice8of16
  split
  · rfl
  rename_i hlen
  simp only
  have hf : ∀ j, g xs j < 65536 := g_u16 hu
  have hB : xs.length / 64 * 64 ≤ xs.length := div_mul_le _ _
  generalize hBd : xs.length / 64 = B at *
  -- the vector blocks
  have hc1 : sumTo (fun b => iceVec (iceVec 0 (fun i => g xs (b * 64 + i))) (fun i => g xs (b * 64 + 32 + i))) B =
      sumTo (fun j => w8 (g xs j) - 1) (B * 64) := by
    rw [← sum_blocks (fun j => w8 (g xs j) - 1) 64 B]
    refine sumTo_congr _ _ _ (fun b _ => ?_)
    rw [iceVec_eq' 0 _ (fun i => hf _), iceVec_eq' _ _ (fun i => hf _)]
    have := sumTo_add (fun i => w8 (g xs (b * 64 + i)) - 1) 32 32
    simp only [Nat.zero_add] at this ⊢
    rw [show (64 : Nat) = 32 + 32 from rfl, this]
    congr 1
  -- the masked tail lanes
  have htail : ∀ c pos, pos ≤ xs.length → xs.length - pos ≤ 32 →
      iceVec c (fun i => if (0xFFFFFFFF >>> (32 - (xs.length - pos))).testBit i then g xs (pos + i) else 0) =
        c + sumTo (fun i => w8 (g xs (pos + i)) - 1) (xs.length - pos) := by
    intro c pos h1 h2
    rw [iceVec_eq' c _ (fun i => by split <;> first | exact hf _ | decide)]
    congr 1
    generalize hr : xs.length - pos = r at *
    let F := fun i => w8 (if (0xFFFFFFFF >>> (32 - r)).testBit i then g xs (pos + i) else 0) - 1
    have e := sumTo_add F r (32 - r)
    rw [show r + (32 - r) = 32 by omega] at e
    have z : sumTo (fun i => F (r + i)) (32 - r) = 0 :=
      sumTo_eq_zero _ _ (fun i hi => by
        simp only [F]
        rw [remMask32 _ h2 _ (by omega), show decide (r + i < r) = false by simp, iteF (by simp)]
        decide)
    have e2 : sumTo F r = sumTo (fun i => w8 (g xs (pos + i)) - 1) r :=
      sumTo_congr _ _ _ (fun i hi => by
        simp only [F]; rw [remMask32 _ h2 _ (by omega), iteT (by simp [hi])])
    show sumTo F 32 = _
    rw [e, z, Nat.add_zero, e2]
  have hp := sumTo_pred (fun j => w8 (g xs j)) xs.length (fun j _ => Replacement.w8_ge _)
  have hall := sum_all w8 xs
  split
  · rename_i hone
    rw [htail _ _ (by omega) (by omega), iceVec_eq' _ _ (fun i => hf _), hc1]
    have e1 := sumTo_add (fun j => w8 (g xs j) - 1) (B * 64) 32
    have e2 := sumTo_add (fun j => w8 (g xs j) - 1) (B * 64 + 32) (xs.length - (B * 64 + 32))
    rw [show B * 64 + 32 + (xs.length - (B * 64 + 32)) = xs.length by omega] at e2
    omega
  · rename_i hone
    rw [htail _ _ (by omega) (by omega), hc1]
    have e2 := sumTo_add (fun j => w8 (g xs j) - 1) (B * 64) (xs.length - B * 64)
    rw [show B * 64 + (xs.length - B * 64) = xs.length by omega] at e2
    omega

def iceWit : List Nat := [0xD800]

theorem ice8of16_witness : ice8of16 iceWit = 3 ∧ L (iceWit.map w8) = 2 := by
  refine ⟨?_, by decide⟩
  rw [ice8of16_eq iceWit (by intro u hu; simp [iceWit] at hu; omega)]
  decide

/-- `ReplacementRest.SLSpec` does not hold for icelake. -/
theorem icelake_not_SLSpec : ¬ ReplacementRest.SLSpec ice8of16 := by
  intro h
  have := h iceWit (by intro u hu; simp [iceWit] at hu; omega)
  rw [ice8of16_witness.1, ice8of16_witness.2] at this
  cases this

/-! ## `utf16_length_from_utf8` -/

/-- `utf8_length_128` (src/icelake/implementation.cpp:1894-1917), indexed by the high nibble. -/
def iceTbl : List Nat := [1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 2]

/-- `t2 = shuffle_epi8(char_length, t1)`, `t1` the high nibble (:1927-1930). -/
def iceT2 (b : Nat) : Nat := iceTbl.getD (b / 16) 0

theorem iceT2_all : (List.range 256).all (fun b => iceT2 b == w16 b && decide (iceT2 b ≤ 2)) = true := by
  decide +kernel

theorem iceT2_eq (b : Nat) (h : b < 256) : iceT2 b = w16 b := by
  have := List.all_eq_true.mp iceT2_all b (List.mem_range.mpr h)
  simp only [Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq] at this
  exact this.1

theorem iceT2_le (b : Nat) (h : b < 256) : iceT2 b ≤ 2 := by
  have := List.all_eq_true.mp iceT2_all b (List.mem_range.mpr h)
  simp only [Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq] at this
  exact this.2

def incIce16 (xs : List Nat) (b i : Nat) : Nat := iceT2 (g xs (b * 64 + i))

/-- **icelake `utf16_length_from_utf8`** (src/icelake/implementation.cpp:1889-1963).
The loop `for (; pos + 64 <= length; pos += 64)` (:1926) adds `t2` into the 8-bit
`local` (:1927-1931) and flushes it into the 64-bit `counters` with `sad`
every `max_iterations = 255 / 2` iterations (:1920, :1933-1938); if
`pos < length` the masked tail adds `t2` of the remaining bytes (`maskz_shuffle`
zeroes the others, :1941-1951); then `counters += sad(local)` (:1952) and the
sum of the counters (:1954-1962). -/
def ice16of8 (xs : List Nat) : Nat :=
  let B := xs.length / 64
  let s := fLoop 64 256 (255 / 2) (incIce16 xs) B
  let loc := if B * 64 < xs.length then
      fun i => (s.loc i + (if i < xs.length - B * 64 then iceT2 (g xs (B * 64 + i)) else 0)) % 256
    else s.loc
  s.tot + sumTo loc 64

theorem ice16of8_eq (xs : List Nat) (hb : Bytes xs) : ice16of8 xs = L (xs.map w16) := by
  unfold ice16of8
  simp only
  generalize hBd : xs.length / 64 = B
  have hB : B * 64 ≤ xs.length := hBd ▸ div_mul_le _ _
  have hle : ∀ b, b < 256 → iceT2 b ≤ 2 := iceT2_le
  have hc : ∀ b i, i < 64 → incIce16 xs b i ≤ 2 := fun b i _ => hle _ (g_byte hb _)
  obtain ⟨i1, i2, i3⟩ := fLoop_inv 64 256 (255 / 2) 2 (incIce16 xs) (by decide) (by decide) hc B
  have hv := block_model w16 iceT2 iceT2 256 (by decide) xs hb iceT2_eq iceT2_eq 64 B hB
    (fun b => sumTo (incIce16 xs b) 64) (fun _ _ => rfl)
  rw [scal_drop _ w16 xs 256 hb iceT2_eq] at hv
  have ht := tail_sum w16 iceT2 256 (by decide) xs hb iceT2_eq (B * 64)
  split
  · rename_i hlt
    have e : sumTo (fun i => ((fLoop 64 256 (255 / 2) (incIce16 xs) B).loc i +
          (if i < xs.length - B * 64 then iceT2 (g xs (B * 64 + i)) else 0)) % 256) 64 =
        sumTo (fLoop 64 256 (255 / 2) (incIce16 xs) B).loc 64 +
          sumTo (fun i => iceT2 (g xs (B * 64 + i))) (xs.length - B * 64) := by
      rw [sumTo_congr _ (fun i => (fLoop 64 256 (255 / 2) (incIce16 xs) B).loc i +
          (if i < xs.length - B * 64 then iceT2 (g xs (B * 64 + i)) else 0)) 64 (fun i hi => by
        apply Nat.mod_eq_of_lt
        have := i2 i hi; have : (if i < xs.length - B * 64 then iceT2 (g xs (B * 64 + i)) else 0) ≤ 2 := by
          split
          · exact hle _ (g_byte hb _)
          · omega
        omega), sumTo_plus]
      congr 1
      have hr : xs.length - B * 64 ≤ 64 := by omega
      have e := sumTo_add (fun i => if i < xs.length - B * 64 then iceT2 (g xs (B * 64 + i)) else 0)
        (xs.length - B * 64) (64 - (xs.length - B * 64))
      rw [show xs.length - B * 64 + (64 - (xs.length - B * 64)) = 64 by omega] at e
      have z : sumTo (fun i => if xs.length - B * 64 + i < xs.length - B * 64 then
          iceT2 (g xs (B * 64 + (xs.length - B * 64 + i))) else 0) (64 - (xs.length - B * 64)) = 0 :=
        sumTo_eq_zero _ _ (fun i _ => iteF (by omega) _ _)
      rw [e, z, Nat.add_zero]
      exact sumTo_congr _ _ _ (fun i hi => iteT hi _ _)
    rw [e]
    omega
  · rename_i hge
    have : B * 64 = xs.length := by omega
    rw [this] at hv; simp at hv; omega

/-! ## `utf8_length_from_latin1` -/

/-- The unrolled step (src/icelake/implementation.cpp:1850-1853) is four
`runner = sub(runner, mask_set1(0, mask, 0xFF))` on one lane: `lane4_eq`. -/
theorem ice_lat4 (r : Nat) (p1 p2 p3 p4 : Bool) :
    lane4 r p1 p2 p3 p4 = (r + (ind p1 + ind p2 + ind p3 + ind p4)) % 256 := lane4_eq r p1 p2 p3 p4

/-- **icelake `utf8_length_from_latin1`** (src/icelake/implementation.cpp:1805-1886).
`answer = length / 64 * 64` (:1808); when `answer >= 2048` (:1810) the loop
`while (i + 64 <= length)` (:1814) is `latOuter 64` (iterations capped at 255
:1816-1819, the unrolled loop :1821, the single loop :1856, `sad` :1866-1868,
`answer +=` :1870); else the loop `for (; i + 64 <= length; i += 64)` (:1872)
adds `count_ones(movepi8_mask)` (:1873-1875); then return if `i == length`
(:1878-1880), else `answer + (length - i) + count_ones` of the masked tail,
whose masked-off bytes are zero (:1882-1884). -/
def iceLatin1 (xs : List Nat) : Nat :=
  let B := xs.length / 64
  let answer := if B * 64 ≥ 2048 then B * 64 + latOuter 64 xs B B 0 0
    else if B * 64 > 0 then B * 64 + sumTo (fun b => sumTo (incNeg 64 xs b) 64) B else B * 64
  let i := B * 64
  if i = xs.length then answer
  else answer + (xs.length - i) + sumTo (fun j => ind (neg8 (g xs (i + j)))) (xs.length - i)

theorem iceLatin1_eq (xs : List Nat) (hb : Bytes xs) : iceLatin1 xs = L (xs.map wL8) := by
  have hx := latin1X_eq 64 xs hb
  unfold latin1X at hx
  unfold iceLatin1
  simp only at hx ⊢
  have hB : xs.length / 64 * 64 ≤ xs.length := div_mul_le _ _
  have ht := tail_sum wL8 (fun u => 1 + ind (neg8 u)) 256 (by decide) xs hb
    (fun u _ => by unfold wL8 neg8; simp only [ge_iff_le]) (xs.length / 64 * 64)
  have hs := scal_drop scL1 wL8 xs 256 hb (fun u h => (byte_ok u h).2.2) (xs.length / 64 * 64)
  have hsplit := sumTo_plus (fun _ => 1) (fun j => ind (neg8 (g xs (xs.length / 64 * 64 + j))))
    (xs.length - xs.length / 64 * 64)
  rw [sumTo_const, Nat.mul_one] at hsplit
  -- the vector part of both functions is the same
  have hz : ∀ (a : Nat), (if xs.length / 64 * 64 = xs.length then a
      else a + (xs.length - xs.length / 64 * 64) +
        sumTo (fun j => ind (neg8 (g xs (xs.length / 64 * 64 + j)))) (xs.length - xs.length / 64 * 64)) =
      a + scal scL1 (xs.drop (xs.length / 64 * 64)) := by
    intro a
    rw [hs]
    split
    · rename_i he; rw [he]; simp
    · rw [← ht, hsplit]; omega
  rw [hz]
  generalize hA : xs.length / 64 * 64 = A at *
  by_cases c1 : A ≥ 2048
  · rw [iteT c1] at hx ⊢; exact hx
  · rw [iteF c1] at hx ⊢
    by_cases c2 : A > 0
    · rw [iteT c2] at hx ⊢; exact hx
    · rw [iteF c2] at hx ⊢
      have : A = 0 := by omega
      subst this; simpa using hx

/-! ## `utf16_length_from_utf32` -/

/-- **icelake `utf16_length_from_utf32`** (src/icelake/implementation.cpp:2006-2037):
`ptr` advances by 16 while `ptr <= end` when `length >= 16` (:2012-2023),
`count += 16 + count_ones(cmpgt_epu32(utf32, 0xffff))` (:2016-2021); return if
nothing is left (:2026-2029); the masked tail of `remaining_count` lanes
(:2030-2035). -/
def ice16of32 (xs : List Nat) : Nat :=
  let B := xs.length / 16
  let count := sumTo (fun b => 16 + sumTo (fun i => ind (decide (g xs (b * 16 + i) > 0xffff))) 16) B
  let r := xs.length - B * 16
  if r = 0 then count else count + r + sumTo (fun i => ind (decide (g xs (B * 16 + i) > 0xffff))) r

theorem ice16of32_eq (xs : List Nat) (hu : U32 xs) : ice16of32 xs = L (xs.map w16of32) := by
  unfold ice16of32
  simp only
  have hB := div_mul_le xs.length 16
  have hl : ∀ u, u < 2 ^ 32 → 1 + ind (decide (u > 0xffff)) = w16of32 u :=
    fun u _ => by unfold w16of32; simp only [gt_iff_lt]
  have h := block_model w16of32 (fun u => 1 + ind (decide (u > 0xffff))) sc16of32 (2 ^ 32) (by decide) xs hu
    hl (fun u _ => sc16of32_eq u) 16 _ hB (fun b => 16 + sumTo (fun i => ind (decide (g xs (b * 16 + i) > 0xffff))) 16)
    (fun b _ => by
      have := sumTo_plus (fun _ => 1) (fun i => ind (decide (g xs (b * 16 + i) > 0xffff))) 16
      rw [sumTo_const, Nat.mul_one] at this
      exact this.symm)
  rw [scal_drop sc16of32 w16of32 xs (2 ^ 32) hu (fun u _ => sc16of32_eq u)] at h
  have ht := tail_sum w16of32 (fun u => 1 + ind (decide (u > 0xffff))) (2 ^ 32) (by decide) xs hu hl (xs.length / 16 * 16)
  have hsplit := sumTo_plus (fun _ => 1) (fun i => ind (decide (g xs (xs.length / 16 * 16 + i) > 0xffff)))
    (xs.length - xs.length / 16 * 16)
  rw [sumTo_const, Nat.mul_one] at hsplit
  split
  · rename_i he
    rw [show xs.length / 16 * 16 = xs.length by omega] at h; simp at h; omega
  · omega

end SafetyMargins.LengthsIcelake
