import SafetyMargins.LengthsGeneric
/-
The length functions that westmere, haswell, arm64, ppc64, lsx and lasx
implement themselves, and the fallback `utf8_length_from_latin1`.

| model | C++ | theorem |
|---|---|---|
| `blk32to16 4 4`, `blk32to16 8 4` | westmere, haswell `utf16_length_from_utf32` (`cmpeq(in & 0xffff0000, 0)`, `movemask_epi8`: four bits per lane) | `westmere16of32_eq`, `haswell16of32_eq` |
| `gt16of32` | arm64, lsx, lasx `utf16_length_from_utf32` (`in > 0xffff` unsigned) | `gt16of32_eq` |
| `latin1X` | westmere (`V = 16`), haswell (`V = 32`) `utf8_length_from_latin1` | `latin1X_eq` |
| `armLatin1` | arm64 `utf8_length_from_latin1` | `armLatin1_eq` |
| `laLatin1` | lsx, lasx `utf8_length_from_latin1` | `laLatin1_eq` |
| `ppcLatin1` | ppc64 `utf8_length_from_latin1` (nested chunks, saturating `vec_sum4s`) | `ppcLatin1_eq` |
| `arm8of32` | arm64 `utf8_length_from_utf32` | `arm8of32_eq` |
| `arm8of16` | arm64 `utf8_length_from_utf16` | `arm8of16_eq`: **two more than the per-unit sum** on some inputs |
| `lasxCount8` | lasx `count_utf8` (= `latin1_length_from_utf8`) | `lasxCount8_eq` |
| `swarLatin1` | fallback `utf8_length_from_latin1` (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h) | `swarLatin1_eq` |

**arm64 `utf8_length_from_utf16` (`arm8of16_eq`).** After the vector loop
(src/arm64/arm_convert_utf16_to_utf8.cpp:600-627, which already counts every
unit with its weight), a high surrogate in the last unit of the vector part
adds 2 more when `pos == size` (:631-633). The low surrogate that could pair
with it is past the input, so nothing compensates: on an input of
`16 k ≥ 32` units whose last unit is a high surrogate, the result is
`L (xs.map w8) + 2` (`arm8of16_witness`: 31 zero units then `0xD800`, result
35, per-unit sum 33). When `pos < size` and `in[pos]` is a low surrogate,
the 2 added for it are its weight and it is skipped (:634-636): exact. The
result is never below the per-unit sum, so buffers sized with it are large
enough; it is not the per-unit sum (`ReplacementRest.SLSpec` fails for it,
`arm64_not_SLSpec`).

The lane facts on 32-bit values use `Lengths.and_hi`; the byte facts are
`LengthsGeneric.lane8b` and checks on 256 values.
-/
namespace SafetyMargins.LengthsArch
open SafetyMargins SafetyMargins.ReplacementMore SafetyMargins.RvvExact SafetyMargins.Lengths
  SafetyMargins.LengthsGeneric
set_option autoImplicit false

/-! ## UTF-32 → UTF-16 length -/

/-- `(in & 0xffff0000) == 0` on a 32-bit lane. -/
def z16 (u : Nat) : Bool := u &&& 0xffff0000 == 0

theorem z16_eq (u : Nat) (hu : u < 2 ^ 32) : z16 u = decide (u < 2 ^ 16) := by
  unfold z16
  have := and_hi u 16 (by decide) hu
  rw [show (0xffff0000 : Nat) = 2 ^ 32 - 2 ^ 16 by decide]
  by_cases h : u < 2 ^ 16
  · simp [this.mpr h, h]
  · have h' : u &&& (2 ^ 32 - 2 ^ 16) ≠ 0 := fun e => h (this.mp e)
    simp [h', h]

theorem lane_z16 (u : Nat) (hu : u < 2 ^ 32) : 1 + (1 - ind (z16 u)) = w16of32 u := by
  rw [z16_eq u hu]; unfold w16of32 ind; simp only [decide_eq_true_eq]
  repeat' split
  all_goals omega

/-- One block of `N` lanes of westmere/haswell `utf16_length_from_utf32`:
`count += N + (4 N - count_ones(bitmask)) / 4`, the bitmask having four bits
per lane where `z16` holds. -/
def blk32to16 (N : Nat) (f : Nat → Nat) : Nat :=
  N + (4 * N - 4 * sumTo (fun i => ind (z16 (f i))) N) / 4

theorem blk32to16_eq (N : Nat) (f : Nat → Nat) :
    blk32to16 N f = sumTo (fun i => 1 + (1 - ind (z16 (f i)))) N := by
  unfold blk32to16
  have hle := sumTo_le (fun i => ind (z16 (f i))) 1 N (fun i _ => ind_le _)
  have hc := sumTo_compl (fun i => ind (z16 (f i))) N (fun i _ => ind_le _)
  have := sumTo_plus (fun _ => 1) (fun i => 1 - ind (z16 (f i))) N
  rw [sumTo_const] at this
  rw [this]; omega

/-- westmere: the loop `for (; pos + 4 <= length; pos += 4)` (src/westmere/implementation.cpp:1280),
`count += 4 + (16 - count_ones(bitmask)) / 4` (:1282-1287), then the scalar
`utf16_length_from_utf32` (:1289-1290). haswell: the loop
`for (; pos + 8 <= length; pos += 8)` (src/haswell/implementation.cpp:1250),
`count += 8 + (32 - count_ones(bitmask)) / 4` (:1251-1257), then :1259-1260. -/
def x86_16of32 (N : Nat) (xs : List Nat) : Nat :=
  sumTo (fun b => blk32to16 N (fun i => g xs (b * N + i))) (xs.length / N) +
    scal sc16of32 (xs.drop (xs.length / N * N))

theorem x86_16of32_eq (N : Nat) (xs : List Nat) (hu : U32 xs) :
    x86_16of32 N xs = L (xs.map w16of32) :=
  block_model w16of32 (fun u => 1 + (1 - ind (z16 u))) sc16of32 (2 ^ 32) (by decide) xs hu
    lane_z16 (fun u _ => sc16of32_eq u) N _ (div_mul_le _ _) _ (fun _ _ => blk32to16_eq _ _)

theorem westmere16of32_eq (xs : List Nat) (hu : U32 xs) : x86_16of32 4 xs = L (xs.map w16of32) :=
  x86_16of32_eq 4 xs hu
theorem haswell16of32_eq (xs : List Nat) (hu : U32 xs) : x86_16of32 8 xs = L (xs.map w16of32) :=
  x86_16of32_eq 8 xs hu

/-- arm64 (src/arm64/implementation.cpp:1249-1268): the loop
`for (; pos + 4 <= length; pos += 4)` (:1255), `vcgtq_u32(in, 0xffff)`
(:1257), the four 0/1 lanes packed by `vpaddq_u16` and counted (:1258-1263),
`count += 4 + surrogate_count` (:1264), the scalar tail (:1266-1267). lsx
(src/lsx/implementation.cpp:1220-1234) and lasx
(src/lasx/implementation.cpp:1334-1348): the loop `for (; pos + 4 <= length; pos += 4)`
(src/lsx/implementation.cpp:1225), `__lsx_vslt_wu(v_ffff, in)` (:1227),
`vmskltz_w` and `vpcnt_b` count the lanes (:1228-1229), `count += 4 +
surrogate_count` (:1230), the scalar tail (:1232-1233); lasx uses the same
LSX code (src/lasx/implementation.cpp:1339-1347). -/
def gt16of32 (xs : List Nat) : Nat :=
  sumTo (fun b => 4 + sumTo (fun i => ind (decide (g xs (b * 4 + i) > 0xffff))) 4) (xs.length / 4) +
    scal sc16of32 (xs.drop (xs.length / 4 * 4))

theorem gt16of32_eq (xs : List Nat) (hu : U32 xs) : gt16of32 xs = L (xs.map w16of32) :=
  block_model w16of32 (fun u => 1 + ind (decide (u > 0xffff))) sc16of32 (2 ^ 32) (by decide) xs hu
    (fun u _ => by unfold w16of32; simp only [gt_iff_lt]) (fun u _ => sc16of32_eq u) 4 _
    (div_mul_le _ _) _ (fun b _ => by
      have := sumTo_plus (fun _ => 1) (fun i => ind (decide (g xs (b * 4 + i) > 0xffff))) 4
      rw [sumTo_const, Nat.mul_one] at this; rw [this])

/-! ## Latin-1 → UTF-8 length on westmere and haswell -/

/-- `_mm_cmpgt_epi8(0, x)`: the byte is negative as `int8_t`, that is `≥ 0x80`. -/
def neg8 (b : Nat) : Bool := decide (b ≥ 0x80)

/-- The unrolled step `runner = sub(runner, add(add(c1, c2), add(c3, c4)))`
(src/westmere/implementation.cpp:1174-1181) on one 8-bit lane adds the
number of negative bytes. -/
theorem lat4_eq (r : Nat) (p1 p2 p3 p4 : Bool) :
    subW 256 r (((maskW 256 p1 + maskW 256 p2) % 256 + (maskW 256 p3 + maskW 256 p4) % 256) % 256) =
      (r + sumTo (fun k => ind ([p1, p2, p3, p4].getD k false)) 4) % 256 := by
  cases p1 <;> cases p2 <;> cases p3 <;> cases p4 <;>
    simp [subW, maskW, ind, sumTo] <;> omega

/-- The increment of lane `l` by vector `b` (`V` bytes). -/
def incNeg (V : Nat) (xs : List Nat) (b l : Nat) : Nat := ind (neg8 (g xs (b * V + l)))

/-- The long-string loop `while (i + V <= len)`
(src/westmere/implementation.cpp:1159): `iterations = min((len - i) / V, 255)`
(:1161-1164), `max_i` (:1165); the unrolled loop
`for (; i + 4 * V <= max_i; i += 4 * V)` (:1166) and the single loop
`for (; i <= max_i; i += V)` (:1183) on the 8-bit `runner`
(`grp`, steps `lat4_eq` and `subW_mask`), then
`two_64bits += sad(runner)` (:1188-1189). In vector units, `blocks = (len -
i) / V` and the chunk ends at `i + iterations V` (`grp_two`), so the next
`blocks` is `blocks - iterations`. -/
def latOuter (V : Nat) (xs : List Nat) : Nat → Nat → Nat → Nat → Nat
  | 0, _, _, acc => acc
  | f + 1, blocks, b0, acc =>
    if blocks = 0 then acc else
    let it := min blocks 255
    let p := grp 256 4 (incNeg V xs) b0 (it - 1) it 0 (fun _ => 0)
    let q := grp 256 1 (incNeg V xs) b0 it (it + 1) p.1 p.2
    latOuter V xs f (blocks - it) (b0 + it) (acc + sumTo q.2 V)

theorem latOuter_eq (V : Nat) (xs : List Nat) : ∀ f blocks b0 acc, blocks ≤ f →
    latOuter V xs f blocks b0 acc = acc + sumTo (fun b => sumTo (incNeg V xs (b0 + b)) V) blocks := by
  intro f
  induction f with
  | zero => intro blocks b0 acc h; simp [latOuter, show blocks = 0 by omega, sumTo]
  | succ f ih =>
    intro blocks b0 acc h
    simp only [latOuter]
    split
    · subst blocks; simp [sumTo]
    · generalize hit : min blocks 255 = it
      have hit1 : it ≤ 255 := by omega
      have hit2 : it ≤ blocks := by omega
      generalize hp : grp 256 4 (incNeg V xs) b0 (it - 1) it 0 (fun _ => 0) = p
      generalize hq : grp 256 1 (incNeg V xs) b0 it (it + 1) p.1 p.2 = q
      have hle : ∀ l, sumTo (fun s => incNeg V xs (b0 + s) l) it ≤ it :=
        fun l => by have := sumTo_le (fun s => incNeg V xs (b0 + s) l) 1 it (fun _ _ => ind_le _); omega
      obtain ⟨-, hq2⟩ := grp_two 256 4 (incNeg V xs) b0 it (by decide) (fun _ => 0)
        (fun l => Or.inr (by have := hle l; omega)) p q hp.symm hq.symm
      rw [ih _ _ _ (by omega)]
      have e1 : sumTo q.2 V = sumTo (fun b => sumTo (incNeg V xs (b0 + b)) V) it := by
        rw [sumTo_congr _ _ V (fun l _ => by rw [hq2 l, Nat.zero_add])]
        exact (sumTo_comm (fun b l => incNeg V xs (b0 + b) l) it V).symm
      have e2 := sumTo_add (fun b => sumTo (incNeg V xs (b0 + b)) V) it (blocks - it)
      rw [show it + (blocks - it) = blocks by omega] at e2
      rw [e1, e2, Nat.add_assoc]
      congr 2
      exact sumTo_congr _ _ _ (fun b _ => by rw [Nat.add_assoc])

/-- The westmere short loops run `len / 32` double then `(len - 32 (len / 32)) / 16`
single iterations: `len / 16` vectors, ending at `len / 16 * 16`. -/
theorem short_count (n : Nat) :
    2 * (n / 32) + (n - n / 32 * 32) / 16 = n / 16 ∧
    n / 32 * 32 + (n - n / 32 * 32) / 16 * 16 = n / 16 * 16 := by omega

/-- **westmere (`V = 16`, src/westmere/implementation.cpp:1152-1209) and
haswell (`V = 32`, src/haswell/implementation.cpp:1179-1233)
`utf8_length_from_latin1`.** `answer = len / V * V`
(src/westmere/implementation.cpp:1155); the long path when `answer >= 2048`
(src/westmere/implementation.cpp:1157), `answer +=` the `sad` sums
(src/westmere/implementation.cpp:1191-1192); else, when `answer > 0`, the short
loops `for (; i + 2 * V <= len; i += 2 * V)` (src/westmere/implementation.cpp:1194)
and `for (; i + V <= len; i += V)` (src/westmere/implementation.cpp:1202) add
`count_ones(movemask)`; haswell has only the single loop
(src/haswell/implementation.cpp:1225). Then `answer +` the scalar function on
the rest (src/westmere/implementation.cpp:1208-1209). The westmere double loop
counts the same two vectors per iteration as two single iterations, and the
two short loops together run `len / V` vectors (`short_count`). -/
def latin1X (V : Nat) (xs : List Nat) : Nat :=
  let B := xs.length / V
  let answer := B * V
  if answer ≥ 2048 then answer + latOuter V xs B B 0 0 + scal scL1 (xs.drop (B * V))
  else if answer > 0 then
    answer + sumTo (fun b => sumTo (incNeg V xs b) V) B + scal scL1 (xs.drop (B * V))
  else answer + scal scL1 xs

theorem latin1X_eq (V : Nat) (xs : List Nat) (hb : Bytes xs) : latin1X V xs = L (xs.map wL8) := by
  unfold latin1X
  simp only
  have hB : xs.length / V * V ≤ xs.length := div_mul_le _ _
  have key : xs.length / V * V + sumTo (fun b => sumTo (incNeg V xs b) V) (xs.length / V) +
      scal scL1 (xs.drop (xs.length / V * V)) = L (xs.map wL8) := by
    have := block_model wL8 (fun u => 1 + ind (neg8 u)) scL1 256 (by decide) xs hb
      (fun u _ => by unfold wL8 neg8; simp only [ge_iff_le])
      (fun u h => (byte_ok u h).2.2) V (xs.length / V) hB
      (fun b => V + sumTo (incNeg V xs b) V)
      (fun b _ => by
        have := sumTo_plus (fun _ => 1) (fun i => ind (neg8 (g xs (b * V + i)))) V
        rw [sumTo_const, Nat.mul_one] at this
        exact this.symm)
    rw [← this]
    have e := sumTo_plus (fun _ => V) (fun b => sumTo (incNeg V xs b) V) (xs.length / V)
    rw [sumTo_const] at e
    rw [e]
  split
  · rw [latOuter_eq V xs _ _ _ _ (Nat.le_refl _)]
    simp only [Nat.zero_add] at key ⊢; exact key
  · split
    · exact key
    · rename_i h1 h2
      have h0 : xs.length / V * V = 0 := by omega
      rw [h0] at key ⊢
      have : sumTo (fun b => sumTo (incNeg V xs b) V) (xs.length / V) = 0 := by
        rcases Nat.eq_zero_or_pos V with hv | hv
        · subst hv; exact sumTo_eq_zero _ _ (fun _ _ => rfl)
        · have : xs.length / V = 0 := by
            rcases Nat.mul_eq_zero.mp h0 with h | h
            · exact h
            · omega
          rw [this]; rfl
      rw [this] at key; simpa using key

/-! ## Latin-1 → UTF-8 length on arm64, lsx, lasx, ppc64 -/

/-- arm64 (src/arm64/implementation.cpp:1114-1135): the loop
`for (; data < simd_end; data += lanes)` (:1125) over `length / 16` vectors;
`vcgeq_u8(input, 0x80)` (:1129); `result -= vaddvq_s8(...)` (:1131), the
`int8_t` sum of the 16 lanes (0 or -1 each) is minus the count, in
`[-16, 0]`; then `result + length / 16 * 16 +` the scalar function (:1133-1134). -/
def armLatin1 (xs : List Nat) : Nat :=
  sumTo (fun b => sumTo (incNeg 16 xs b) 16) (xs.length / 16) + xs.length / 16 * 16 +
    scal scL1 (xs.drop (xs.length / 16 * 16))

theorem armLatin1_eq (xs : List Nat) (hb : Bytes xs) : armLatin1 xs = L (xs.map wL8) := by
  have := latin1X_eq 16 xs hb
  unfold latin1X at this; unfold armLatin1
  simp only at this
  split at this
  · rw [latOuter_eq 16 xs _ _ _ _ (Nat.le_refl _)] at this
    simp only [Nat.zero_add] at this; omega
  · split at this
    · omega
    · have h0 : xs.length / 16 = 0 := by omega
      rw [h0] at this ⊢; simpa [sumTo] using this

/-- lsx (src/lsx/implementation.cpp:1128-1143) and lasx
(src/lasx/implementation.cpp:1242-1257): the loop `while (data_end - data > 16)`
(src/lsx/implementation.cpp:1133) runs `(length - 1) / 16` times
(`laLatin1_count`); `result += 16 + popcount(vmskltz_b)` (:1134-1138); the
scalar function on the 1 to 16 bytes left (:1141-1142). -/
def laLatin1 (xs : List Nat) : Nat :=
  let B := (xs.length - 1) / 16
  sumTo (fun b => 16 + sumTo (incNeg 16 xs b) 16) B + scal scL1 (xs.drop (B * 16))

/-- The loop count: after `B` iterations at most 16 bytes remain, before each at least 17. -/
theorem laLatin1_count (n : Nat) :
    (n - 1) / 16 * 16 ≤ n ∧ n - (n - 1) / 16 * 16 ≤ 16 ∧ ∀ k, k < (n - 1) / 16 → n - k * 16 > 16 := by
  refine ⟨by omega, by omega, fun k hk => ?_⟩
  have : (k + 1) * 16 ≤ n - 1 := by
    have := (Nat.le_div_iff_mul_le (by decide : 0 < 16)).mp (show k + 1 ≤ (n - 1) / 16 by omega)
    exact this
  rw [Nat.succ_mul] at this; omega

theorem laLatin1_eq (xs : List Nat) (hb : Bytes xs) : laLatin1 xs = L (xs.map wL8) :=
  block_model wL8 (fun u => 1 + ind (neg8 u)) scL1 256 (by decide) xs hb
    (fun u _ => by unfold wL8 neg8; simp only [ge_iff_le])
    (fun u h => (byte_ok u h).2.2) 16 _ (laLatin1_count _).1 _
    (fun b _ => by
      have := sumTo_plus (fun _ => 1) (fun i => ind (neg8 (g xs (b * 16 + i)))) 16
      rw [sumTo_const] at this
      exact this.symm)

/-- ppc64 inner chunks (src/ppc64/ppc64_utf8_length_from_latin1.cpp:15-29):
while `chunk != 0`, `n = min(chunk, 255)` vectors into the 8-bit `local`
(`local -= as_vector_u8(in < 0)`, :21-26, `subW_mask`), then
`partial = sum4bytes(local, partial)` (:28): `vec_sum4s` adds bytes `4 j ..
4 j + 3` into the 32-bit lane `j` with unsigned saturation (`vsum4ubs`,
src/simdutf/ppc64/simd.h:168-170). -/
def ppcInner (inc : Nat → Nat → Nat) : Nat → Nat → Nat → (Nat → Nat) → Nat → Nat
  | 0, _, _, p => p
  | f + 1, chunk, b0, p =>
    if chunk = 0 then p else
    let n := min chunk 255
    let loc := inner 256 inc b0 n
    ppcInner inc f (chunk - n) (b0 + n) (fun j => min (p j + sumTo (fun k => loc (4 * j + k)) 4) (2 ^ 32 - 1))

theorem ppcInner_eq (inc : Nat → Nat → Nat) (hc : ∀ b i, inc b i ≤ 1) : ∀ f chunk b0 p, chunk ≤ f →
    (∀ j, p j + 4 * chunk ≤ 2 ^ 32 - 1) →
    ∀ j, ppcInner inc f chunk b0 p j =
      p j + sumTo (fun b => sumTo (fun k => inc (b0 + b) (4 * j + k)) 4) chunk := by
  intro f
  induction f with
  | zero => intro chunk b0 p h _ j; simp [ppcInner, show chunk = 0 by omega, sumTo]
  | succ f ih =>
    intro chunk b0 p h hp j
    simp only [ppcInner]
    split
    · subst chunk; simp [sumTo]
    · generalize hn : min chunk 255 = n
      have hloc : ∀ i, inner 256 inc b0 n i = sumTo (fun t => inc (b0 + t) i) n :=
        fun i => inner_eq 256 1 inc b0 hc n (by omega) i
      have hsum : ∀ j, sumTo (fun k => inner 256 inc b0 n (4 * j + k)) 4 =
          sumTo (fun b => sumTo (fun k => inc (b0 + b) (4 * j + k)) 4) n := by
        intro j
        rw [sumTo_congr _ _ 4 (fun k _ => hloc (4 * j + k))]
        exact sumTo_comm (fun k t => inc (b0 + t) (4 * j + k)) 4 n
      have hbound : ∀ j, sumTo (fun b => sumTo (fun k => inc (b0 + b) (4 * j + k)) 4) n ≤ 4 * n := by
        intro j
        have := sumTo_le (fun b => sumTo (fun k => inc (b0 + b) (4 * j + k)) 4) 4 n
          (fun b _ => by have := sumTo_le (fun k => inc (b0 + b) (4 * j + k)) 1 4 (fun _ _ => hc _ _); omega)
        omega
      rw [ih _ _ _ (by omega) (fun j => by
        rw [hsum j]; have := hbound j; have := hp j
        rw [Nat.min_eq_left (by omega)]; omega) j]
      rw [hsum j, Nat.min_eq_left (by have := hbound j; have := hp j; omega)]
      have e2 := sumTo_add (fun b => sumTo (fun k => inc (b0 + b) (4 * j + k)) 4) n (chunk - n)
      rw [show n + (chunk - n) = chunk by omega] at e2
      rw [e2, Nat.add_assoc]
      congr 2
      exact sumTo_congr _ _ _ (fun b _ => by rw [Nat.add_assoc])

/-- The outer loop `while (length != 0)` (src/ppc64/ppc64_utf8_length_from_latin1.cpp:9):
`chunk = min(length, 0xffffffff / N)` (:13-14), `partial` from zero (:10),
`count += partial.value[i]` for the four lanes (:31-33). -/
def ppcOuter (inc : Nat → Nat → Nat) : Nat → Nat → Nat → Nat → Nat
  | 0, _, _, c => c
  | f + 1, len, b0, c =>
    if len = 0 then c else
    let chunk := min len (0xffffffff / 16)
    let p := ppcInner inc chunk chunk b0 (fun _ => 0)
    ppcOuter inc f (len - chunk) (b0 + chunk) (c + sumTo p 4)

theorem ppcOuter_eq (inc : Nat → Nat → Nat) (hc : ∀ b i, inc b i ≤ 1) : ∀ f len b0 c, len ≤ f →
    ppcOuter inc f len b0 c = c + sumTo (fun b => sumTo (inc (b0 + b)) 16) len := by
  intro f
  induction f with
  | zero => intro len b0 c h; simp [ppcOuter, show len = 0 by omega, sumTo]
  | succ f ih =>
    intro len b0 c h
    simp only [ppcOuter]
    split
    · subst len; simp [sumTo]
    · generalize hch : min len (0xffffffff / 16) = chunk
      rw [ih _ _ _ (by omega)]
      have hp := ppcInner_eq inc hc chunk chunk b0 (fun _ => 0) (Nat.le_refl _)
        (fun j => by simp only [Nat.zero_add]; omega)
      have e1 : sumTo (ppcInner inc chunk chunk b0 (fun _ => 0)) 4 =
          sumTo (fun b => sumTo (inc (b0 + b)) 16) chunk := by
        rw [sumTo_congr _ _ 4 (fun j _ => by rw [hp j, Nat.zero_add])]
        rw [sumTo_comm (fun j b => sumTo (fun k => inc (b0 + b) (4 * j + k)) 4) 4 chunk]
        refine sumTo_congr _ _ chunk (fun b _ => ?_)
        have := sum_blocks (inc (b0 + b)) 4 4
        simp only [Nat.mul_comm _ 4] at this ⊢
        exact this
      have e2 := sumTo_add (fun b => sumTo (inc (b0 + b)) 16) chunk (len - chunk)
      rw [show chunk + (len - chunk) = len by omega] at e2
      rw [e1, e2, Nat.add_assoc]
      congr 2
      exact sumTo_congr _ _ _ (fun b _ => by rw [Nat.add_assoc])

/-- **ppc64 `utf8_length_from_latin1`** (src/ppc64/implementation.cpp:748-760):
`ppc64_utf8_length_from_latin1` (src/ppc64/ppc64_utf8_length_from_latin1.cpp:3-37,
`length = length / N`, `count = length * N`, :6-8) then the scalar function on
the rest unless it is empty (src/ppc64/implementation.cpp:753-759). -/
def ppcLatin1 (xs : List Nat) : Nat :=
  let B := xs.length / 16
  let c := ppcOuter (incNeg 16 xs) B B 0 (B * 16)
  if B * 16 = xs.length then c else scal scL1 (xs.drop (B * 16)) + c

theorem ppcLatin1_eq (xs : List Nat) (hb : Bytes xs) : ppcLatin1 xs = L (xs.map wL8) := by
  have := armLatin1_eq xs hb
  unfold armLatin1 at this
  unfold ppcLatin1
  simp only
  rw [ppcOuter_eq (incNeg 16 xs) (fun b i => ind_le _) _ _ _ _ (Nat.le_refl _)]
  simp only [Nat.zero_add]
  split
  · rename_i h
    rw [h] at this; simp [scal] at this; omega
  · omega

/-! ## arm64 `utf8_length_from_utf32` -/

/-- The lane of src/arm64/implementation.cpp:1213-1241: `vcleq_u32` against
`0x7f`, `0x7ff`, `0xffff` (:1215-1220), the exclusive-or masks, and the
contribution to `16 - 3 * ascii_count - 2 * two_bytes_count - three_bytes_count`. -/
def armA (u : Nat) : Nat := ind (decide (u ≤ 0x7f))
def armT (u : Nat) : Nat := ind (decide (u ≤ 0x7ff) != decide (u ≤ 0x7f))
def armH (u : Nat) : Nat := ind (decide (u ≤ 0xffff) != decide (u ≤ 0x7ff))

theorem armlane (u : Nat) : 3 * armA u + 2 * armT u + armH u ≤ 3 ∧
    4 - 3 * armA u - 2 * armT u - armH u = w8of32 u := by
  unfold armA armT armH w8of32 ind
  by_cases h1 : u ≤ 0x7f <;> by_cases h2 : u ≤ 0x7ff <;> by_cases h3 : u ≤ 0xffff <;>
    simp [h1, h2, h3] <;> (repeat' split) <;> omega

/-- The loop `for (; pos + 4 <= length; pos += 4)` (src/arm64/implementation.cpp:1213), the
three counts of the packed 0/1 lanes (:1222-1239), `count += 16 - ...` (:1241),
the scalar function (:1243-1244). -/
def arm8of32 (xs : List Nat) : Nat :=
  sumTo (fun b => 16 - 3 * sumTo (fun i => armA (g xs (b * 4 + i))) 4 -
      2 * sumTo (fun i => armT (g xs (b * 4 + i))) 4 - sumTo (fun i => armH (g xs (b * 4 + i))) 4)
    (xs.length / 4) + scal sc8of32 (xs.drop (xs.length / 4 * 4))

theorem arm8of32_blk (f : Nat → Nat) : ∀ m,
    3 * sumTo (fun i => armA (f i)) m + 2 * sumTo (fun i => armT (f i)) m + sumTo (fun i => armH (f i)) m ≤ 3 * m ∧
    4 * m - 3 * sumTo (fun i => armA (f i)) m - 2 * sumTo (fun i => armT (f i)) m -
      sumTo (fun i => armH (f i)) m = sumTo (fun i => w8of32 (f i)) m := by
  intro m
  induction m with
  | zero => simp [sumTo]
  | succ m ih =>
    simp only [sumTo_succ]
    have := armlane (f m)
    have : w8of32 (f m) ≥ 1 := by unfold w8of32; omega
    omega

theorem arm8of32_eq (xs : List Nat) (hu : U32 xs) : arm8of32 xs = L (xs.map w8of32) :=
  block_model w8of32 w8of32 sc8of32 (2 ^ 32) (by decide) xs hu (fun _ _ => rfl)
    (fun u _ => sc8of32_eq u) 4 _ (div_mul_le _ _) _ (fun b _ => (arm8of32_blk _ 4).2)

/-! ## arm64 `utf8_length_from_utf16` -/

/-- **arm64 `utf8_length_from_utf16`**
(src/arm64/arm_convert_utf16_to_utf8.cpp:579-641), on the units after the
byte swap: short inputs (`N + 1 > size`) go to the scalar function (:585-587);
the loop `for (; size - pos >= N; pos += N)` (:600) runs `size / 16` times,
`count += vaddlvq_u8(v_count)` of the 8-bit lanes `c1 + c0 + is_surrogate`
(:601-618, `ReplacementMore.armLane`, the deinterleaved bytes `msb = u / 256`,
`lsb = u % 256`); `count += pos` (:627); a high surrogate at `pos - 1` adds 2
when `pos == size`, or when `in[pos]` is low, and then skips it (:631-638);
the scalar function on the rest (:639-640). -/
def arm8of16 (xs : List Nat) : Nat :=
  if 16 + 1 > xs.length then scal sc8of16 xs else
  let P := xs.length / 16 * 16
  let count := sumTo (fun b => sumTo (fun i => armLane (g xs (b * 16 + i))) 16) (xs.length / 16) + P
  if Replacement.isHigh (g xs (P - 1)) then
    (if P = xs.length then count + 2 + scal sc8of16 (xs.drop P)
     else if Replacement.isLow (g xs P) then count + 2 + scal sc8of16 (xs.drop (P + 1))
     else count + scal sc8of16 (xs.drop P))
  else count + scal sc8of16 (xs.drop P)

theorem arm8of16_eq (xs : List Nat) (hu : U16 xs) :
    arm8of16 xs = L (xs.map w8) +
      (if 17 ≤ xs.length ∧ xs.length % 16 = 0 ∧ Replacement.isHigh (g xs (xs.length - 1)) = true
       then 2 else 0) := by
  unfold arm8of16
  split
  · rw [iteF (by omega)]
    exact (scalar_lengths xs).1 hu |>.1
  rename_i hlen
  simp only
  have hP : xs.length / 16 * 16 ≤ xs.length := div_mul_le _ _
  generalize hPdef : xs.length / 16 * 16 = P at *
  have hP1 : 16 ≤ P := by omega
  have hvec : sumTo (fun b => sumTo (fun i => armLane (g xs (b * 16 + i))) 16) (xs.length / 16) + P =
      sumTo (fun j => w8 (g xs j)) P := by
    rw [sumTo_congr (fun b => sumTo (fun i => armLane (g xs (b * 16 + i))) 16)
      (fun b => sumTo (fun i => w8 (g xs (b * 16 + i)) - 1) 16) _
      (fun b _ => sumTo_congr _ _ 16 (fun i _ => (arm_lane _ (g_u16 hu _)).1))]
    rw [sum_blocks (fun j => w8 (g xs j) - 1) 16, hPdef]
    exact sumTo_pred _ _ (fun j _ => Replacement.w8_ge _)
  rw [hvec]
  have ht := sum_tail w8 xs P hP
  have hs : ∀ k, scal sc8of16 (xs.drop k) = L ((xs.drop k).map w8) :=
    fun k => scal_drop sc8of16 w8 xs 65536 hu (fun u h => (u16_ok u h).1) k
  have hmod : (P = xs.length) ↔ xs.length % 16 = 0 := by omega
  by_cases hh : Replacement.isHigh (g xs (P - 1)) = true
  · rw [iteT hh]
    by_cases he : P = xs.length
    · rw [iteT he, hs, iteT ⟨by omega, hmod.mp he, by rw [← he]; exact hh⟩]
      rw [he] at ht ⊢; omega
    · have hr : (if 17 ≤ xs.length ∧ xs.length % 16 = 0 ∧
          Replacement.isHigh (g xs (xs.length - 1)) = true then 2 else 0) = 0 :=
        iteF (fun h => he (hmod.mpr h.2.1)) _ _
      rw [iteF he, hr]
      by_cases hl : Replacement.isLow (g xs P) = true
      · rw [iteT hl, hs]
        have hd := drop_g xs P (by omega)
        have hw : w8 (g xs P) = 2 := Replacement.w8_sur _ (Replacement.low_sur hl)
        rw [hd] at ht; simp only [List.map_cons, L_cons, hw] at ht
        omega
      · rw [iteF hl, hs]; omega
  · have hr : (if 17 ≤ xs.length ∧ xs.length % 16 = 0 ∧
        Replacement.isHigh (g xs (xs.length - 1)) = true then 2 else 0) = 0 :=
      iteF (fun h => by have := hmod.mpr h.2.1; rw [← this] at h; exact hh h.2.2) _ _
    rw [iteF hh, hs, hr]
    omega

/-- The witness: 31 zero units then a high surrogate. -/
def armWit : List Nat := List.replicate 31 0 ++ [0xD800]

theorem arm8of16_witness : arm8of16 armWit = 35 ∧ L (armWit.map w8) = 33 := by
  have hu : U16 armWit := by
    intro u hu; simp [armWit] at hu; rcases hu with h | h <;> omega
  have := arm8of16_eq armWit hu
  have hL : L (armWit.map w8) = 33 := by decide
  rw [hL] at this
  refine ⟨?_, hL⟩
  rw [this]
  have : Replacement.isHigh (g armWit (armWit.length - 1)) = true := by decide
  rw [iteT ⟨by decide, by decide, this⟩]

/-- `ReplacementRest.SLSpec` does not hold for arm64. -/
theorem arm64_not_SLSpec : ¬ ReplacementRest.SLSpec arm8of16 := by
  intro h
  have hu : U16 armWit := by
    intro u hu; simp [armWit] at hu; rcases hu with h | h <;> omega
  have := h armWit hu
  rw [arm8of16_witness.1, arm8of16_witness.2] at this
  cases this

/-! ## lasx `count_utf8` -/

/-- **lasx `count_utf8`** (src/lasx/implementation.cpp:1206-1226), also its
`latin1_length_from_utf8` (:1235-1238). The prologue
`while (((input + pos) & 0x1F) && pos < length)` (:1212) counts the bytes
`> -65` one by one (`char` is signed on LoongArch) up to a 32-byte aligned
address: `a` bytes, which depends on the address; the theorem holds for every
`a` (the C++ has `a ≤ length`). The
loop `for (; pos + 32 <= length; pos += 32)` (:1218): `xvslt_b(0xBF, in)` (signed),
`vmskltz_b` and `vpcnt_h` count each 128-bit half (:1219-1223); then the scalar
`count_code_points` (:1225). -/
def lasxCount8 (a : Nat) (xs : List Nat) : Nat :=
  let B := (xs.length - a) / 32
  scal scCount8 (xs.take a) +
    sumTo (fun b => sumTo (fun i => ind (gtm65 (g xs (a + b * 32 + i)))) 32) B +
    scal scCount8 (xs.drop (a + B * 32))

theorem lasxCount8_eq (a : Nat) (xs : List Nat) (hb : Bytes xs) :
    lasxCount8 a xs = L (xs.map w01) := by
  unfold lasxCount8
  simp only
  have hB : (xs.length - a) / 32 * 32 ≤ xs.length - a := div_mul_le _ _
  have hbd : Bytes (xs.drop a) := bytes_drop hb a
  have hblk := block_model w01 (fun b => ind (gtm65 b)) scCount8 256 (by decide) (xs.drop a) hbd
    (fun u h => (lane8b u h).1) (fun u h => (byte_ok u h).1) 32 ((xs.length - a) / 32)
    (by simp only [List.length_drop]; exact hB)
    (fun b => sumTo (fun i => ind (gtm65 (g xs (a + b * 32 + i)))) 32)
    (fun b _ => sumTo_congr _ _ 32 (fun i _ => by
      rw [g_drop, Nat.add_assoc]))
  rw [List.drop_drop] at hblk
  rw [scal_eq scCount8 w01 _ 256 (fun u hu => hb u (List.mem_of_mem_take hu)) (fun u h => (byte_ok u h).1)]
  have := L_append ((xs.take a).map w01) ((xs.drop a).map w01)
  rw [← List.map_append, List.take_append_drop] at this
  omega

/-! ## The fallback `utf8_length_from_latin1` -/

/-- The 64-bit word that `memcpy` reads from 8 bytes (little-endian: byte `k`
at bits `8 k .. 8 k + 7`; on a big-endian target the same bytes in the other
order, which `pop_eq` covers as well since it holds for every list of 8 bytes). -/
def wordLo : List Nat → Nat
  | [] => 0
  | c :: cs => c + 256 * wordLo cs

theorem testBit_wordL (cs : List Nat) (h : ∀ c ∈ cs, c < 256) (j : Nat) :
    (wordLo cs).testBit j = (cs.getD (j / 8) 0).testBit (j % 8) := by
  induction cs generalizing j with
  | nil => simp [wordLo]
  | cons c cs ih =>
    have hc : c < 2 ^ 8 := h c (by simp)
    rw [wordLo, show c + 256 * wordLo cs = 2 ^ 8 * wordLo cs + c by omega,
      Nat.testBit_two_pow_mul_add _ hc]
    split
    · rename_i hj
      rw [show j / 8 = 0 by omega, show j % 8 = j by omega]; rfl
    · rename_i hj
      rw [ih (fun c hc => h c (by simp [hc])) (j - 8)]
      rw [show j / 8 = (j - 8) / 8 + 1 by omega, show (j - 8) % 8 = j % 8 by omega]
      rfl

/-- Byte facts behind `pop` (all 256 values). -/
def popByte (c : Nat) : Bool :=
  c.testBit 7 == decide (c ≥ 128) && (c / 128).testBit 0 == decide (c ≥ 128) &&
  (List.range 7).all (fun r => !(c / 128).testBit (r + 1)) && c / 128 < 256 &&
  c >>> 7 == ind (decide (c ≥ 128))

theorem popBytes : (List.range 256).all popByte = true := by decide +kernel

theorem pop_byte (c : Nat) (h : c < 256) :
    c.testBit 7 = decide (c ≥ 128) ∧ (c / 128).testBit 0 = decide (c ≥ 128) ∧
    (∀ r, r < 7 → (c / 128).testBit (r + 1) = false) ∧ c / 128 < 256 ∧ c >>> 7 = ind (decide (c ≥ 128)) := by
  have := List.all_eq_true.mp popBytes c (List.mem_range.mpr h)
  simp only [popByte, Bool.and_eq_true, beq_iff_eq, List.all_eq_true, List.mem_range,
    Bool.not_eq_eq_eq_not, Bool.not_true, decide_eq_true_eq] at this
  exact ⟨this.1.1.1.1, this.1.1.1.2, this.1.1.2, this.1.2, this.2⟩

theorem ones_eq : (0x0101010101010101 : Nat) = wordLo [1, 1, 1, 1, 1, 1, 1, 1] := by decide

/-- `(v >> 7) & 0x0101010101010101` keeps the top bit of each byte, at the bottom of
that byte. -/
theorem mask_eq (cs : List Nat) (h : ∀ c ∈ cs, c < 256) (hl : cs.length = 8) :
    wordLo cs >>> 7 &&& 0x0101010101010101 = wordLo (cs.map (· / 128)) := by
  have hm : ∀ c ∈ cs.map (· / 128), c < 256 := by
    intro c hc; simp only [List.mem_map] at hc
    obtain ⟨d, hd, rfl⟩ := hc; exact (pop_byte d (h d hd)).2.2.2.1
  apply Nat.eq_of_testBit_eq
  intro j
  rw [Nat.testBit_and, Nat.testBit_shiftRight, ones_eq,
    testBit_wordL cs h, testBit_wordL _ (by simp) j, testBit_wordL _ hm j]
  have hg : ∀ k, (cs.map (· / 128)).getD k 0 = cs.getD k 0 / 128 := by
    intro k; simp [List.getD_eq_getElem?_getD, List.getElem?_map]
    cases cs[k]? <;> rfl
  rw [hg]
  by_cases hj8 : j / 8 < 8
  · have hc : cs.getD (j / 8) 0 < 256 := by
      rw [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem (by omega)]
      exact h _ (List.getElem_mem _)
    have h1 : ([1, 1, 1, 1, 1, 1, 1, 1] : List Nat).getD (j / 8) 0 = 1 :=
      (by decide : ∀ k, k < 8 → ([1, 1, 1, 1, 1, 1, 1, 1] : List Nat).getD k 0 = 1) _ hj8
    rw [h1]
    by_cases hr : j % 8 = 0
    · rw [show (7 + j) / 8 = j / 8 by omega, show (7 + j) % 8 = 7 by omega, hr,
        (pop_byte _ hc).1, (pop_byte _ hc).2.1, show Nat.testBit 1 0 = true from rfl, Bool.and_true]
    · have h1 : Nat.testBit 1 (j % 8) = false :=
        Nat.testBit_lt_two_pow (by
          have : 2 ^ 1 ≤ 2 ^ (j % 8) := Nat.pow_le_pow_right (by decide) (by omega)
          omega)
      have hz : (cs.getD (j / 8) 0 / 128).testBit (j % 8) = false := by
        have := (pop_byte _ hc).2.2.1 (j % 8 - 1) (by omega)
        rwa [show j % 8 - 1 + 1 = j % 8 by omega] at this
      rw [h1, hz, Bool.and_false]
  · have e1 : ([1, 1, 1, 1, 1, 1, 1, 1] : List Nat).getD (j / 8) 0 = 0 := by
      rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by simp; omega)]; rfl
    have e2 : cs.getD (j / 8) 0 = 0 := by
      rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by omega)]; rfl
    rw [e1, e2]; simp

theorem mulpop (h0 h1 h2 h3 h4 h5 h6 h7 : Nat) (e0 : h0 ≤ 1) (e1 : h1 ≤ 1) (e2 : h2 ≤ 1)
    (e3 : h3 ≤ 1) (e4 : h4 ≤ 1) (e5 : h5 ≤ 1) (e6 : h6 ≤ 1) (e7 : h7 ≤ 1) :
    (wordLo [h0, h1, h2, h3, h4, h5, h6, h7] * 0x0101010101010101 % 2 ^ 64) >>> 56 =
      h0 + h1 + h2 + h3 + h4 + h5 + h6 + h7 := by
  rw [Nat.shiftRight_eq_div_pow]
  simp only [wordLo]
  rcases (show h0 = 0 ∨ h0 = 1 by omega) with rfl | rfl <;>
  rcases (show h1 = 0 ∨ h1 = 1 by omega) with rfl | rfl <;>
  rcases (show h2 = 0 ∨ h2 = 1 by omega) with rfl | rfl <;>
  rcases (show h3 = 0 ∨ h3 = 1 by omega) with rfl | rfl <;>
  rcases (show h4 = 0 ∨ h4 = 1 by omega) with rfl | rfl <;>
  rcases (show h5 = 0 ∨ h5 = 1 by omega) with rfl | rfl <;>
  rcases (show h6 = 0 ∨ h6 = 1 by omega) with rfl | rfl <;>
  rcases (show h7 = 0 ∨ h7 = 1 by omega) with rfl | rfl <;> decide

/-- `pop` (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:207-211):
the product is a `uint64_t`, modulo `2^64`. -/
def pop (v : Nat) : Nat := ((v >>> 7 &&& 0x0101010101010101) * 0x0101010101010101 % 2 ^ 64) >>> 56

/-- `pop` of 8 bytes counts those `≥ 0x80`. -/
theorem pop_eq (cs : List Nat) (h : ∀ c ∈ cs, c < 256) (hl : cs.length = 8) :
    pop (wordLo cs) = L (cs.map fun c => ind (decide (c ≥ 128))) := by
  unfold pop
  rw [mask_eq cs h hl]
  match cs, hl with
  | [c0, c1, c2, c3, c4, c5, c6, c7], _ =>
    have b : ∀ c, c < 256 → c / 128 = ind (decide (c ≥ 128)) := by
      intro c hc; unfold ind; split <;> simp_all <;> omega
    have hb := fun c (hc : c ∈ [c0, c1, c2, c3, c4, c5, c6, c7]) => h c hc
    simp only [List.map_cons, List.map_nil]
    rw [b c0 (hb _ (by simp)), b c1 (hb _ (by simp)), b c2 (hb _ (by simp)), b c3 (hb _ (by simp)),
      b c4 (hb _ (by simp)), b c5 (hb _ (by simp)), b c6 (hb _ (by simp)), b c7 (hb _ (by simp)),
      mulpop _ _ _ _ _ _ _ _ (ind_le _) (ind_le _) (ind_le _) (ind_le _) (ind_le _) (ind_le _)
        (ind_le _) (ind_le _)]
    simp only [L_cons, L_nil]; omega

/-- The 8 bytes at `o`. -/
def bytes8 (xs : List Nat) (o : Nat) : List Nat := (List.range 8).map fun k => g xs (o + k)

theorem pop8 (xs : List Nat) (hb : Bytes xs) (o : Nat) :
    pop (wordLo (bytes8 xs o)) = sumTo (fun k => ind (neg8 (g xs (o + k)))) 8 := by
  rw [pop_eq _ (fun c hc => by
    simp only [bytes8, List.mem_map, List.mem_range] at hc
    obtain ⟨k, -, rfl⟩ := hc; exact g_byte hb _) (by simp [bytes8])]
  simp [bytes8, List.range_succ, sumTo, neg8]
  omega

/-- **The fallback `utf8_length_from_latin1`**
(include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:198-233, used by
src/fallback/implementation.cpp:449-452): `answer = length`
(include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:200); the loop
`for (; i + 32 <= length; i += 32)` (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:212)
adds `pop` of four words (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:213-221);
the loop `for (; i + 8 <= length; i += 8)` (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:223)
one word (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:224-226); the loop
`for (; i + 1 <= length; i += 1)` (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:229)
adds `byte >> 7` (include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:230). -/
def swarLatin1 (xs : List Nat) : Nat :=
  let B32 := xs.length / 32
  let B8 := (xs.length - B32 * 32) / 8
  let P := B32 * 32 + B8 * 8
  xs.length + sumTo (fun b => sumTo (fun k => pop (wordLo (bytes8 xs (b * 32 + k * 8)))) 4) B32 +
    sumTo (fun b => pop (wordLo (bytes8 xs (B32 * 32 + b * 8)))) B8 + L ((xs.drop P).map fun c => c >>> 7)

theorem swarLatin1_eq (xs : List Nat) (hb : Bytes xs) : swarLatin1 xs = L (xs.map wL8) := by
  unfold swarLatin1
  simp only
  generalize hB32 : xs.length / 32 = B32
  generalize hB8 : (xs.length - B32 * 32) / 8 = B8
  have h1 : B32 * 32 ≤ xs.length := hB32 ▸ div_mul_le _ _
  have h2 : B32 * 32 + B8 * 8 ≤ xs.length := by have := hB8 ▸ div_mul_le (xs.length - B32 * 32) 8; omega
  let f := fun j => ind (neg8 (g xs j))
  have e1 : sumTo (fun b => sumTo (fun k => pop (wordLo (bytes8 xs (b * 32 + k * 8)))) 4) B32 =
      sumTo f (B32 * 32) := by
    rw [← sum_blocks f 32 B32]
    refine sumTo_congr _ _ _ (fun b _ => ?_)
    rw [sumTo_congr _ _ 4 (fun k _ => pop8 xs hb _)]
    have := sum_blocks (fun j => f (b * 32 + j)) 8 4
    simp only [f, Nat.add_assoc] at this ⊢
    exact this
  have e2 : sumTo (fun b => pop (wordLo (bytes8 xs (B32 * 32 + b * 8)))) B8 =
      sumTo (fun j => f (B32 * 32 + j)) (B8 * 8) := by
    rw [← sum_blocks (fun j => f (B32 * 32 + j)) 8 B8]
    refine sumTo_congr _ _ _ (fun b _ => ?_)
    rw [pop8 xs hb]
    simp only [f, Nat.add_assoc]
  have e3 := sumTo_add f (B32 * 32) (B8 * 8)
  have e4 : L ((xs.drop (B32 * 32 + B8 * 8)).map fun c => c >>> 7) =
      L ((xs.drop (B32 * 32 + B8 * 8)).map fun c => ind (neg8 c)) :=
    L_map_congr _ _ _ (fun c hc => by
      rw [(pop_byte c (hb c (List.mem_of_mem_drop hc))).2.2.2.2]; rfl)
  have e5 := sum_tail (fun c => ind (neg8 c)) xs _ h2
  have e6 : L (xs.map wL8) = xs.length + L (xs.map fun c => ind (neg8 c)) := by
    rw [show wL8 = fun c => 1 + ind (neg8 c) from rfl, L_map_add, L_map_one]
  rw [e1, e2, e4, e6]
  simp only [f] at e3 ⊢
  omega

end SafetyMargins.LengthsArch
