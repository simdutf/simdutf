import SafetyMargins.Lengths
/-
The length functions of src/generic/, shared by several kernels.

Each function is transcribed lane by lane: the loop runs over blocks of `N`
units (`g xs (b * N + i)` is lane `i` of block `b`), a lane predicate gives
each lane's contribution, narrow lane counters are flushed as in the C++,
and the scalar function finishes the rest. Each theorem states the result on
every input of the unit type.

| model | C++ | used by (`N`) | theorem |
|---|---|---|---|
| `genCount16` | `utf16::count_code_points`, src/generic/utf16.h:6-21 | westmere, haswell, arm64, ppc64 (32) | `genCount16_eq` |
| `genU8of16` | `utf16::utf8_length_from_utf16`, src/generic/utf16.h:23-47 | ppc64 (32) | `genU8of16_eq` |
| `genCount8` | `utf8::count_code_points`, src/generic/utf8.h:8-17 | see below (64) | `genCount8_eq` |
| `genU16of8` | `utf8::utf16_length_from_utf8`, src/generic/utf8.h:72-87 | arm64, ppc64 (64) | `genU16of8_eq` |
| `bmCount8` | `utf8::count_code_points_bytemask`, src/generic/utf8.h:19-69 | westmere (16), haswell (32) | `bmCount8_eq` |
| `bmU16of8` | src/generic/utf8/utf16_length_from_utf8_bytemask.h:8-48 | westmere, lsx (16), haswell, lasx (32) | `bmU16of8_eq` |
| `bmU8of16` | src/generic/utf16/utf8_length_from_utf16_bytemask.h:8-85 | westmere, lsx (8), haswell, lasx (16) | `bmU8of16_eq` |
| `bmCount16` | src/generic/utf16/count_code_points_bytemask.h:8-53 | lsx (8), lasx (16) | `bmCount16_eq` |
| `genU8of32` | `utf32::utf8_length_from_utf32`, src/generic/utf32.h:10-131 | westmere (4), haswell (8), icelake (16), ppc64 (4); lsx (4), lasx (8) with unsigned compares | `genU8of32_eq` |

`genCount8` is `utf32_length_from_utf8` on westmere, haswell, arm64, ppc64,
lsx and lasx, and `count_utf8`/`latin1_length_from_utf8` on arm64, ppc64 and
lsx.

Lane counters: `bmCount8` adds at most 4 per 8-bit lane per iteration and
flushes every `255 / 4 = 63` iterations (252 < 256); `bmU16of8` at most 2,
every `255 / 2 = 127` (254); `bmU8of16` at most 2 into 16-bit lanes, every
`65535 / 2 = 32767` (65534 < 65536); `bmCount16` at most 1, every 65535;
`genU8of32` at most 12 into 32-bit lanes, every `(2^32 - 1) / 12`
iterations, then at most 3, every `(2^32 - 1) / 3`. `fLoop_inv` and
`outer_eq` show that no lane wraps. The C++ comment at
src/generic/utf16/utf8_length_from_utf16_bytemask.h:23-24 says the increment
is 0, 1 or 2; `lanes16` checks it.

The lane facts are checked on all 256 byte values and all 65536 16-bit
values (`decide +kernel`); the 32-bit ones are `Lengths.min_and_hi`.
-/
namespace SafetyMargins.LengthsGeneric
open SafetyMargins SafetyMargins.ReplacementMore SafetyMargins.RvvExact SafetyMargins.Lengths
set_option autoImplicit false

/-! ## Lane facts -/

/-- `not_in_range(0xDC00, 0xDFFF)` (src/generic/utf16.h:16). -/
def nirLo (u : Nat) : Bool := decide (u > 0xDFFF ∨ u < 0xDC00)
/-- `lteq(0x7F)`, `lteq(0x7FF)`, `not_in_range(0xD800, 0xDFFF)` (src/generic/utf16.h:34-36). -/
def asc (u : Nat) : Bool := decide (u ≤ 0x7F)
def two (u : Nat) : Bool := decide (u ≤ 0x7FF)
def notPair (u : Nat) : Bool := decide (u > 0xDFFF ∨ u < 0xD800)
/-- The contribution of a lane to `2 * fourbyte + 3 * threebyte + 2 * twobyte + ascii`
(src/generic/utf16.h:38-43). -/
def lane8 (u : Nat) : Nat :=
  2 * (1 - ind (notPair u)) + 3 * ind (notPair u && !two u) + 2 * ind (two u && !asc u) + ind (asc u)
/-- `min((input & 0xfc00) ^ 0xdc00, one)`
(src/generic/utf16/count_code_points_bytemask.h:30-35). -/
def t2 (u : Nat) : Nat := min ((u &&& 0xfc00) ^^^ 0xdc00) 1

def lane16OK (u : Nat) : Bool :=
  ind (nirLo u) == w32 u && lane8 u == w8 u && t2 u == w32 u &&
  (c0 u + c1 u + vsur u) % 65536 == w8 u - 1

/-- Checked on all 65536 values (`decide +kernel`, a finite exhaustive check). -/
theorem lanes16 : (List.range 65536).all lane16OK = true := by decide +kernel

theorem lane16 (u : Nat) (h : u < 65536) :
    ind (nirLo u) = w32 u ∧ lane8 u = w8 u ∧ t2 u = w32 u ∧ (c0 u + c1 u + vsur u) % 65536 = w8 u - 1 := by
  have := List.all_eq_true.mp lanes16 u (List.mem_range.mpr h)
  simp only [lane16OK, Bool.and_eq_true, beq_iff_eq] at this
  exact ⟨this.1.1.1, this.1.1.2, this.1.2, this.2⟩

/-- `gt(-65)` (src/generic/utf8.h:13). -/
def gtm65 (b : Nat) : Bool := decide (i8 b > -65)
/-- `lt(-65 + 1)` and `gteq_unsigned(240)` (src/generic/utf8.h:79, :83). -/
def ltm64 (b : Nat) : Bool := decide (i8 b < -64)
def ge240 (b : Nat) : Bool := decide (b ≥ 240)

def lane8OK (b : Nat) : Bool :=
  ind (gtm65 b) == w01 b && (1 - ind (ltm64 b)) + ind (ge240 b) == w16 b &&
  ind (gtm65 b) + ind (ge240 b) == w16 b

/-- Checked on all 256 values. -/
theorem lanes8 : (List.range 256).all lane8OK = true := by decide +kernel

theorem lane8b (b : Nat) (h : b < 256) :
    ind (gtm65 b) = w01 b ∧ (1 - ind (ltm64 b)) + ind (ge240 b) = w16 b ∧
    ind (gtm65 b) + ind (ge240 b) = w16 b := by
  have := List.all_eq_true.mp lanes8 b (List.mem_range.mpr h)
  simp only [lane8OK, Bool.and_eq_true, beq_iff_eq] at this
  exact ⟨this.1.1, this.1.2, this.2⟩

/-- A block loop whose block `b` counts `lane` of its `N` units, then a scalar
tail whose per-unit term is `tl`: if both equal the weight, the result is
`L (xs.map w)`. -/
theorem block_model (w lane tl : Nat → Nat) (n : Nat) (hn : 0 < n) (xs : List Nat) (hx : ∀ u ∈ xs, u < n)
    (hl : ∀ u, u < n → lane u = w u) (ht : ∀ u, u < n → tl u = w u)
    (N B : Nat) (hB : B * N ≤ xs.length) (blk : Nat → Nat)
    (hblk : ∀ b, b < B → blk b = sumTo (fun i => lane (g xs (b * N + i))) N) :
    sumTo blk B + scal tl (xs.drop (B * N)) = L (xs.map w) := by
  rw [scal_drop tl w xs n hx ht]
  apply assemble w xs N B hB
  intro b hb
  rw [hblk b hb]
  exact sumTo_congr _ _ N (fun i _ => hl _ (g_lt_of hx hn _))

theorem div_mul_le (a N : Nat) : a / N * N ≤ a := Nat.div_mul_le_self a N

/-! ## `utf16::count_code_points` (src/generic/utf16.h:6-21) -/

/-- The loop `for (; pos < size / 32 * 32; pos += 32)` (src/generic/utf16.h:11) runs
`size / 32` times; each block adds `count_ones(not_pair) / 2` (:16-17), two
bits per lane; then the scalar `count_code_points` (:19-20). -/
def genCount16 (xs : List Nat) : Nat :=
  sumTo (fun b => (2 * sumTo (fun i => ind (nirLo (g xs (b * 32 + i)))) 32) / 2) (xs.length / 32) +
    scal sc32of16 (xs.drop (xs.length / 32 * 32))

theorem genCount16_eq (xs : List Nat) (hu : U16 xs) : genCount16 xs = L (xs.map w32) :=
  block_model w32 (fun u => ind (nirLo u)) sc32of16 65536 (by decide) xs hu (fun u h => (lane16 u h).1)
    (fun u h => (u16_ok u h).2) 32 _ (div_mul_le _ _) _ (fun b _ => half_two _)

/-! ## `utf16::utf8_length_from_utf16` (src/generic/utf16.h:23-47) -/

/-- One block (src/generic/utf16.h:34-43): the four counts from the three
bitmasks (two bits per lane), `fourbyte_count = 32 - count_ones(not_pair_mask) / 2`. -/
def genU8blk (f : Nat → Nat) : Nat :=
  let ascii := (2 * sumTo (fun i => ind (asc (f i))) 32) / 2
  let twob := (2 * sumTo (fun i => ind (two (f i) && !asc (f i))) 32) / 2
  let threeb := (2 * sumTo (fun i => ind (notPair (f i) && !two (f i))) 32) / 2
  let fourb := 32 - (2 * sumTo (fun i => ind (notPair (f i))) 32) / 2
  2 * fourb + 3 * threeb + 2 * twob + ascii

/-- The loop `for (; pos < size / 32 * 32; pos += 32)` (src/generic/utf16.h:29), then
the scalar `utf8_length_from_utf16` (:45-46). -/
def genU8of16 (xs : List Nat) : Nat :=
  sumTo (fun b => genU8blk (fun i => g xs (b * 32 + i))) (xs.length / 32) +
    scal sc8of16 (xs.drop (xs.length / 32 * 32))

theorem genU8blk_eq (f : Nat → Nat) : genU8blk f = sumTo (fun i => lane8 (f i)) 32 := by
  have key : ∀ m, sumTo (fun i => ind (notPair (f i))) m ≤ m ∧
      2 * (m - sumTo (fun i => ind (notPair (f i))) m) +
        3 * sumTo (fun i => ind (notPair (f i) && !two (f i))) m +
        2 * sumTo (fun i => ind (two (f i) && !asc (f i))) m + sumTo (fun i => ind (asc (f i))) m =
      sumTo (fun i => lane8 (f i)) m := by
    intro m
    induction m with
    | zero => simp [sumTo]
    | succ m ih =>
      simp only [sumTo_succ]
      have := ind_le (notPair (f m)); have := ind_le (notPair (f m) && !two (f m))
      have := ind_le (two (f m) && !asc (f m)); have := ind_le (asc (f m))
      have e : lane8 (f m) = 2 * (1 - ind (notPair (f m))) + 3 * ind (notPair (f m) && !two (f m)) +
        2 * ind (two (f m) && !asc (f m)) + ind (asc (f m)) := rfl
      omega
  unfold genU8blk
  simp only [half_two]
  exact (key 32).2

theorem genU8of16_eq (xs : List Nat) (hu : U16 xs) : genU8of16 xs = L (xs.map w8) :=
  block_model w8 lane8 sc8of16 65536 (by decide) xs hu (fun u h => (lane16 u h).2.1)
    (fun u h => (u16_ok u h).1) 32 _ (div_mul_le _ _) _ (fun b _ => genU8blk_eq _)

/-! ## `utf8::count_code_points` (src/generic/utf8.h:8-17) -/

/-- The loop `for (; pos + 64 <= size; pos += 64)` (src/generic/utf8.h:11) runs `size / 64`
times; each block adds `count_ones(input.gt(-65))` (:13-14); then the scalar
`count_code_points` (:16). -/
def genCount8 (xs : List Nat) : Nat :=
  sumTo (fun b => sumTo (fun i => ind (gtm65 (g xs (b * 64 + i)))) 64) (xs.length / 64) +
    scal scCount8 (xs.drop (xs.length / 64 * 64))

theorem genCount8_eq (xs : List Nat) (hb : Bytes xs) : genCount8 xs = L (xs.map w01) :=
  block_model w01 (fun b => ind (gtm65 b)) scCount8 256 (by decide) xs hb (fun u h => (lane8b u h).1)
    (fun u h => (byte_ok u h).1) 64 _ (div_mul_le _ _) _ (fun _ _ => rfl)

/-! ## `utf8::utf16_length_from_utf8` (src/generic/utf8.h:72-87) -/

/-- The loop `for (; pos + 64 <= size; pos += 64)` (src/generic/utf8.h:77): `count += 64 -
count_ones(lt(-65 + 1))` and `count += count_ones(gteq_unsigned(240))`
(:79-84); then the scalar `utf16_length_from_utf8` (:86). -/
def genU16of8 (xs : List Nat) : Nat :=
  sumTo (fun b => 64 - sumTo (fun i => ind (ltm64 (g xs (b * 64 + i)))) 64 +
      sumTo (fun i => ind (ge240 (g xs (b * 64 + i)))) 64) (xs.length / 64) +
    scal sc16of8 (xs.drop (xs.length / 64 * 64))

theorem genU16_blk (f : Nat → Nat) : ∀ m,
    m - sumTo (fun i => ind (ltm64 (f i))) m + sumTo (fun i => ind (ge240 (f i))) m =
      sumTo (fun i => (1 - ind (ltm64 (f i))) + ind (ge240 (f i))) m := by
  have hle : ∀ m, sumTo (fun i => ind (ltm64 (f i))) m ≤ m :=
    fun m => by have := sumTo_le (fun i => ind (ltm64 (f i))) 1 m (fun i _ => ind_le _); omega
  intro m
  induction m with
  | zero => simp [sumTo]
  | succ m ih =>
    simp only [sumTo_succ]
    have := hle m; have := ind_le (ltm64 (f m)); omega

theorem genU16of8_eq (xs : List Nat) (hb : Bytes xs) : genU16of8 xs = L (xs.map w16) :=
  block_model w16 (fun b => (1 - ind (ltm64 b)) + ind (ge240 b)) sc16of8 256 (by decide) xs hb
    (fun u h => (lane8b u h).2.1) (fun u h => (byte_ok u h).2.1) 64 _ (div_mul_le _ _) _
    (fun b _ => genU16_blk _ 64)

/-! ## `utf8::count_code_points_bytemask` (src/generic/utf8.h:19-69) -/

/-- The four `local -= vector_u8(mask_k)` of one iteration (src/generic/utf8.h:49-52)
on one 8-bit lane. -/
def lane4 (x : Nat) (p0 p1 p2 p3 : Bool) : Nat :=
  subW 256 (subW 256 (subW 256 (subW 256 x (maskW 256 p0)) (maskW 256 p1)) (maskW 256 p2))
    (maskW 256 p3)

/-- They add the number of comparisons that hold, modulo 256. -/
theorem lane4_eq (x : Nat) (p0 p1 p2 p3 : Bool) :
    lane4 x p0 p1 p2 p3 = (x + (ind p0 + ind p1 + ind p2 + ind p3)) % 256 := by
  unfold lane4
  simp only [subW_mask 256 _ _ (by decide), Nat.mod_add_mod]
  congr 1; omega

/-- The increment of lane `i` in iteration `b`: lane `i` of the four vectors
at `pos + k N` (:36-47), `pos = b * 4 N`. -/
def incCount8 (N : Nat) (xs : List Nat) (b i : Nat) : Nat :=
  sumTo (fun k => ind (gtm65 (g xs (b * (4 * N) + k * N + i)))) 4

/-- The loop `for (; pos + 4 * N <= size; pos += 4 * N)` (src/generic/utf8.h:35) runs
`size / (4 N)` times, with 8-bit lanes flushed into `counters` every
`max_iterations = 255 / 4` iterations (:27, :54-59); then
`if (iterations > 0) count += local.sum_bytes()`, `count += counters.sum()`
(:62-66), and the scalar `count_code_points` (:68). -/
def bmCount8 (N : Nat) (xs : List Nat) : Nat :=
  let B := xs.length / (4 * N)
  let s := fLoop N 256 (255 / 4) (incCount8 N xs) B
  (if s.it > 0 then sumTo s.loc N else 0) + s.tot + scal scCount8 (xs.drop (B * (4 * N)))

theorem bmCount8_eq (N : Nat) (xs : List Nat) (hb : Bytes xs) : bmCount8 N xs = L (xs.map w01) := by
  unfold bmCount8
  simp only
  have hc : ∀ b i, i < N → incCount8 N xs b i ≤ 4 := fun b i _ =>
    Nat.le_trans (sumTo_le _ 1 4 (fun _ _ => ind_le _)) (by omega)
  have := fLoop_final N 256 (255 / 4) 4 (incCount8 N xs) (by decide) (by decide) hc (xs.length / (4 * N))
  rw [show ∀ a b c : Nat, a + b + c = (b + a) + c from fun a b c => by omega, this]
  refine block_model w01 (fun b => ind (gtm65 b)) scCount8 256 (by decide) xs hb (fun u h => (lane8b u h).1)
    (fun u h => (byte_ok u h).1) (4 * N) _ (div_mul_le _ _) _ (fun b _ => ?_)
  unfold incCount8
  exact sum_unrolled (fun j => ind (gtm65 (g xs j))) 4 N (b * (4 * N))

/-! ## `utf16_length_from_utf8_bytemask`
(src/generic/utf8/utf16_length_from_utf8_bytemask.h:8-48) -/

/-- `local -= vector_u8(continuation)`, `local -= vector_u8(utf_4bytes)` (:27-31)
on one lane. -/
def lane2 (x : Nat) (p0 p1 : Bool) : Nat := subW 256 (subW 256 x (maskW 256 p0)) (maskW 256 p1)

theorem lane2_eq (x : Nat) (p0 p1 : Bool) : lane2 x p0 p1 = (x + (ind p0 + ind p1)) % 256 := by
  unfold lane2
  simp only [subW_mask 256 _ _ (by decide), Nat.mod_add_mod]
  congr 1; omega

def incU16of8 (N : Nat) (xs : List Nat) (b i : Nat) : Nat :=
  ind (gtm65 (g xs (b * N + i))) + ind (ge240 (g xs (b * N + i)))

/-- The loop `for (; pos + N <= size; pos += N)` (src/generic/utf8/utf16_length_from_utf8_bytemask.h:23)
runs `size / N` times; flush every `max_iterations = 255 / 2` (:15, :33-38);
then :41-47. -/
def bmU16of8 (N : Nat) (xs : List Nat) : Nat :=
  let B := xs.length / N
  let s := fLoop N 256 (255 / 2) (incU16of8 N xs) B
  (if s.it > 0 then sumTo s.loc N else 0) + s.tot + scal sc16of8 (xs.drop (B * N))

theorem bmU16of8_eq (N : Nat) (xs : List Nat) (hb : Bytes xs) : bmU16of8 N xs = L (xs.map w16) := by
  unfold bmU16of8
  simp only
  have hc : ∀ b i, i < N → incU16of8 N xs b i ≤ 2 := fun b i _ => by
    unfold incU16of8; have := ind_le (gtm65 (g xs (b * N + i))); have := ind_le (ge240 (g xs (b * N + i)))
    omega
  have := fLoop_final N 256 (255 / 2) 2 (incU16of8 N xs) (by decide) (by decide) hc (xs.length / N)
  rw [show ∀ a b c : Nat, a + b + c = (b + a) + c from fun a b c => by omega, this]
  exact block_model w16 (fun b => ind (gtm65 b) + ind (ge240 b)) sc16of8 256 (by decide) xs hb
    (fun u h => (lane8b u h).2.2) (fun u h => (byte_ok u h).2.1) N _ (div_mul_le _ _) _
    (fun _ _ => rfl)

/-! ## `utf8_length_from_utf16_bytemask`
(src/generic/utf16/utf8_length_from_utf16_bytemask.h:8-85) -/

/-- `v_count += c0; v_count += c1; v_count += vector_u16(is_surrogate)` (:67-69)
on one 16-bit lane: the increment is `(c0 + c1 + vsur) % 65536`. -/
theorem lane3_eq (x u : Nat) :
    addw (addw (addw x (c0 u)) (c1 u)) (vsur u) = (x + (c0 u + c1 u + vsur u) % 65536) % 65536 := by
  rw [addw3, Nat.add_mod_mod]

def incU8of16 (N : Nat) (xs : List Nat) (b i : Nat) : Nat :=
  (c0 (g xs (b * N + i)) + c1 (g xs (b * N + i)) + vsur (g xs (b * N + i))) % 65536

/-- `count = size / N * N` (:21); the loop `for (; pos < size / N * N; pos += N)`
(src/generic/utf16/utf8_length_from_utf16_bytemask.h:28) with the count-down
flush (:25-26, :71-76); `if (iteration > 0) count += v_count.sum()` (:79-81);
the scalar `utf8_length_from_utf16` (:83-84). -/
def bmU8of16 (N : Nat) (xs : List Nat) : Nat :=
  let B := xs.length / N
  let s := fLoopDown N 65536 (65535 / 2) (incU8of16 N xs) B
  B * N + s.tot + (if s.it > 0 then sumTo s.loc N else 0) + scal sc8of16 (xs.drop (B * N))

theorem bmU8of16_eq (N : Nat) (xs : List Nat) (hu : U16 xs) : bmU8of16 N xs = L (xs.map w8) := by
  unfold bmU8of16
  simp only
  generalize hB : xs.length / N = B
  have hc : ∀ b i, i < N → incU8of16 N xs b i ≤ 2 := fun b i _ => by
    unfold incU8of16; rw [(lane16 _ (g_u16 hu _)).2.2.2]
    have : w8 (g xs (b * N + i)) ≤ 3 := by unfold w8 Arch.Icelake.w8; repeat' (first | omega | split)
    omega
  obtain ⟨e1, e2, e3, e4⟩ := fLoopDown_eq N 65536 (65535 / 2) (incU8of16 N xs) (by decide) B
  obtain ⟨-, -, i3⟩ := fLoop_inv N 65536 (65535 / 2) 2 (incU8of16 N xs) (by decide) (by decide) hc B
  rw [e1, e2, e3]
  simp only [show 65535 / 2 - (fLoop N 65536 (65535 / 2) (incU8of16 N xs) B).it > 0 by omega, ite_true]
  have hB' : B * N ≤ xs.length := hB ▸ div_mul_le _ _
  have hblk := block_model (fun u => w8 u - 1) (fun u => w8 u - 1) (fun u => w8 u - 1) 65536 (by decide) xs hu
    (fun _ _ => rfl) (fun _ _ => rfl) N B hB' (fun b => sumTo (incU8of16 N xs b) N)
    (fun b _ => sumTo_congr _ _ N (fun i _ => (lane16 _ (g_u16 hu _)).2.2.2))
  have hs := sum_tail (fun u => w8 u - 1) xs (B * N) hB'
  have hp := sumTo_pred (fun j => w8 (g xs j)) (B * N) (fun j _ => Replacement.w8_ge _)
  have ht := sum_tail w8 xs (B * N) hB'
  rw [scal_drop sc8of16 w8 xs 65536 hu (fun u h => (u16_ok u h).1)]
  rw [scal_drop _ _ xs 65536 hu (fun _ _ => rfl)] at hblk
  omega

/-! ## `utf16::count_code_points`, bytemask form
(src/generic/utf16/count_code_points_bytemask.h:8-53) -/

def incCount16 (N : Nat) (xs : List Nat) (b i : Nat) : Nat := t2 (g xs (b * N + i))

/-- The loop `for (; pos < size / N * N; pos += N)`
(src/generic/utf16/count_code_points_bytemask.h:24): `counters += t2` on 16-bit
lanes (:37), `count += counters.sum()` every `max_iterations = 65535`
iterations (:39-44) and once more if `iteration > 0` (:47-49); the scalar
`count_code_points` (:51-52). -/
def bmCount16 (N : Nat) (xs : List Nat) : Nat :=
  let B := xs.length / N
  let s := fLoop N 65536 65535 (incCount16 N xs) B
  s.tot + (if s.it > 0 then sumTo s.loc N else 0) + scal sc32of16 (xs.drop (B * N))

theorem bmCount16_eq (N : Nat) (xs : List Nat) (hu : U16 xs) : bmCount16 N xs = L (xs.map w32) := by
  unfold bmCount16
  simp only
  have hc : ∀ b i, i < N → incCount16 N xs b i ≤ 1 := fun b i _ => by
    unfold incCount16 t2; omega
  rw [fLoop_final N 65536 65535 1 (incCount16 N xs) (by decide) (by decide) hc (xs.length / N)]
  exact block_model w32 t2 sc32of16 65536 (by decide) xs hu (fun u h => (lane16 u h).2.2.1)
    (fun u h => (u16_ok u h).2) N _ (div_mul_le _ _) _ (fun _ _ => rfl)

/-! ## `utf32::utf8_length_from_utf32` (src/generic/utf32.h:10-131) -/

/-- One `acc += min(one, in & m)` (src/generic/utf32.h:69-82, :111-113, without
unsigned compares) or `acc -= as_vector_u32(in > c)` (:54-67, :107-109, with
them; `SIMDUTF_SIMD_HAS_UNSIGNED_CMP`, src/simdutf/lsx/begin.h:2,
src/simdutf/lasx/begin.h:2): what it adds to a 32-bit lane, for the three
thresholds `k = 7, 11, 16` (`0x7f`/`0xffffff80`, `0x7ff`/`0xfffff800`,
`0xffff`/`0xffff0000`). -/
def term (uc : Bool) (k u : Nat) : Nat :=
  if uc then ind (decide (u > 2 ^ k - 1)) else min (u &&& (2 ^ 32 - 2 ^ k)) 1

theorem masks_eq : (0xffffff80 : Nat) = 2 ^ 32 - 2 ^ 7 ∧ (0xfffff800 : Nat) = 2 ^ 32 - 2 ^ 11 ∧
    (0xffff0000 : Nat) = 2 ^ 32 - 2 ^ 16 ∧ (0x7f : Nat) = 2 ^ 7 - 1 ∧ (0x7ff : Nat) = 2 ^ 11 - 1 ∧
    (0xffff : Nat) = 2 ^ 16 - 1 := by decide

/-- Both statement forms add `term` modulo `2^32`. -/
theorem stmt_eq (x u k : Nat) :
    subW (2 ^ 32) x (maskW (2 ^ 32) (decide (u > 2 ^ k - 1))) = (x + term true k u) % 2 ^ 32 ∧
    (x + min (u &&& (2 ^ 32 - 2 ^ k)) 1) % 2 ^ 32 = (x + term false k u) % 2 ^ 32 :=
  ⟨subW_mask _ _ _ (by decide), rfl⟩

/-- The three terms of a unit. -/
def terms (uc : Bool) (u : Nat) : Nat := term uc 7 u + term uc 11 u + term uc 16 u

theorem terms_eq (uc : Bool) (u : Nat) (hu : u < 2 ^ 32) : terms uc u = w8of32 u - 1 := by
  unfold terms term w8of32
  cases uc
  · simp only [Bool.false_eq_true, ite_false]
    rw [min_and_hi u 7 (by decide) hu, min_and_hi u 11 (by decide) hu, min_and_hi u 16 (by decide) hu]
    unfold ind; simp only [decide_eq_true_eq]
    repeat' split
    all_goals omega
  · simp only [ite_true]
    unfold ind; simp only [decide_eq_true_eq]
    repeat' split
    all_goals omega

theorem terms_le (uc : Bool) (u : Nat) (hu : u < 2 ^ 32) : terms uc u ≤ 3 := by
  rw [terms_eq uc u hu]; unfold w8of32 ind; repeat' split
  all_goals omega

/-- Section 1 (src/generic/utf32.h:35-90): iteration `b` adds, to lane `i`, the
terms of lane `i` of the four vectors at `input + k N` (:48-82). The twelve
statements add to the lane in sequence (`addSeq`), which is adding their sum
(`addSeq_eq`). -/
def inc1 (uc : Bool) (N : Nat) (xs : List Nat) (b i : Nat) : Nat :=
  sumTo (fun k => terms uc (g xs (b * (4 * N) + k * N + i))) 4

/-- Section 2 (src/generic/utf32.h:92-121): one vector per iteration, from `base`. -/
def inc2 (uc : Bool) (N base : Nat) (xs : List Nat) (b i : Nat) : Nat :=
  terms uc (g xs (base + b * N + i))

/-- **`utf32::utf8_length_from_utf32`.** Section 1: `blocks = length / (N * 4)`
(:40-41), chunks of `max_iterations = UINT32_MAX / (max_increment * 4)`
(:38-39, :43-44), `counter += acc.sum()` (:88). Section 2: `blocks =
length / N` of what is left (:96-97), `max_iterations = UINT32_MAX /
max_increment` (:94-95). Then `counter += consumed` (:123-128) and the scalar
`utf8_length_from_utf32` (:130). The outer loops are `outer`, with fuel
`blocks`. -/
def genU8of32 (uc : Bool) (N : Nat) (xs : List Nat) : Nat :=
  let B1 := xs.length / (N * 4)
  let r1 := xs.length - B1 * (N * 4)
  let B2 := r1 / N
  let c1 := outer N (2 ^ 32) ((2 ^ 32 - 1) / (3 * 4)) (inc1 uc N xs) B1 B1 0 0
  let c2 := outer N (2 ^ 32) ((2 ^ 32 - 1) / 3) (inc2 uc N (B1 * (4 * N)) xs) B2 B2 0 c1
  let consumed := B1 * (N * 4) + B2 * N
  (if consumed ≠ 0 then c2 + consumed else c2) + scal sc8of32 (xs.drop consumed)

theorem genU8of32_eq (uc : Bool) (N : Nat) (xs : List Nat) (hu : U32 xs) :
    genU8of32 uc N xs = L (xs.map w8of32) := by
  unfold genU8of32
  simp only
  rw [Nat.mul_comm N 4]
  generalize hB1 : xs.length / (4 * N) = B1
  generalize hB2 : (xs.length - B1 * (4 * N)) / N = B2
  have hc1 : ∀ b i, inc1 uc N xs b i ≤ 12 := fun b i =>
    Nat.le_trans (sumTo_le _ 3 4 (fun _ _ => terms_le uc _ (g_u32 hu _))) (by omega)
  have hc2 : ∀ b i, inc2 uc N (B1 * (4 * N)) xs b i ≤ 3 := fun b i => terms_le uc _ (g_u32 hu _)
  rw [outer_eq N _ _ 12 _ (by decide) (by decide) hc1 B1 B1 0 0 (Nat.le_refl _),
    outer_eq N _ _ 3 _ (by decide) (by decide) hc2 B2 B2 0 _ (Nat.le_refl _)]
  have hP1 : B1 * (4 * N) ≤ xs.length := hB1 ▸ div_mul_le _ _
  have hP2 : B1 * (4 * N) + B2 * N ≤ xs.length := by
    have := hB2 ▸ div_mul_le (xs.length - B1 * (4 * N)) N; omega
  -- the per-unit term is `w8of32 - 1`
  have e1 : sumTo (fun b => sumTo (inc1 uc N xs (0 + b)) N) B1 = sumTo (fun j => w8of32 (g xs j) - 1) (B1 * (4 * N)) := by
    rw [← sum_blocks (fun j => w8of32 (g xs j) - 1) (4 * N) B1]
    refine sumTo_congr _ _ B1 (fun b _ => ?_)
    unfold inc1
    rw [Nat.zero_add, sum_unrolled (fun j => terms uc (g xs j)) 4 N (b * (4 * N))]
    exact sumTo_congr _ _ _ (fun j _ => terms_eq uc _ (g_u32 hu _))
  have e2 : sumTo (fun b => sumTo (inc2 uc N (B1 * (4 * N)) xs (0 + b)) N) B2 =
      sumTo (fun j => w8of32 (g xs (B1 * (4 * N) + j)) - 1) (B2 * N) := by
    rw [← sum_blocks (fun j => w8of32 (g xs (B1 * (4 * N) + j)) - 1) N B2]
    refine sumTo_congr _ _ B2 (fun b _ => sumTo_congr _ _ N (fun i _ => ?_))
    unfold inc2
    rw [Nat.zero_add, terms_eq uc _ (g_u32 hu _), Nat.add_assoc]
  have e3 := sumTo_add (fun j => w8of32 (g xs j) - 1) (B1 * (4 * N)) (B2 * N)
  have e4 := sumTo_pred (fun j => w8of32 (g xs j)) (B1 * (4 * N) + B2 * N)
    (fun j _ => by unfold w8of32; omega)
  have e5 := sum_tail w8of32 xs _ hP2
  rw [scal_drop sc8of32 w8of32 xs (2 ^ 32) hu (fun u _ => sc8of32_eq u)]
  rw [e1, e2]
  split
  · omega
  · have h0 : B1 * (4 * N) + B2 * N = 0 := by omega
    rw [h0] at e3 e4 e5 ⊢
    simp only [Nat.zero_add] at e5 ⊢
    have : B1 * (4 * N) = 0 := by omega
    have : B2 * N = 0 := by omega
    simp only [sumTo] at *
    omega

end SafetyMargins.LengthsGeneric
