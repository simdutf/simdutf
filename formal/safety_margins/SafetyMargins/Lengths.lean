import SafetyMargins.ReplacementRest
import SafetyMargins.RvvExact
/-
The length functions: the shared part.

The safety theorems size the output buffer as `L (xs.map w)`, the sum of a
per-unit weight `w` over the input. simdutf documents the result of the
ordinary length functions as implementation defined on invalid input
(include/simdutf/implementation.h:2064-2065). `Lengths*.lean` prove, for the
scalar code and for every SIMD kernel, which value each one returns on every
input of its type (bytes, 16-bit units, 32-bit units), valid or not.

The weights are the per-unit terms of the scalar functions, reused from the
files that need them:

| function | weight | of a unit |
|---|---|---|
| `utf8_length_from_utf16` | `Replacement.w8` (`Arch.Icelake.w8`) | 1, 2, 3; a surrogate 2 |
| `utf32_length_from_utf16`, `count_utf16` | `IcelakeReal.w32v` | 0 for `0xDC00..0xDFFF`, else 1 |
| `utf16_length_from_utf8` | `Utf8.w16` | 0 for a continuation byte, 2 for `≥ 0xF0`, else 1 |
| `utf32_length_from_utf8`, `count_utf8`, `latin1_length_from_utf8` | `Exact.w01` | 0 for a continuation byte, else 1 |
| `utf8_length_from_utf32` | `RvvExact.w8of32` | `1 + [> 0x7F] + [> 0x7FF] + [> 0xFFFF]` |
| `utf16_length_from_utf32` | `RvvExact.w16of32` | `1 + [> 0xFFFF]` |
| `utf8_length_from_latin1` | `RvvExact.wL8` | `1 + [≥ 0x80]` |

`utf16_length_from_latin1`, `utf32_length_from_latin1`,
`latin1_length_from_utf16` and `latin1_length_from_utf32` return `length`
(include/simdutf/implementation.h:1082-1085, :1095-1098, :3854-3857,
:3867-3870): weight 1 per unit (`trivial_lengths`).

This file holds what the other `Lengths*.lean` files share:

* **Units.** An input is the list of unit values: bytes (`Bytes`), 16-bit
  units after the byte swap of the non-native endianness (`U16`; every
  kernel applies the swap to every loaded lane and the scalar tail to every
  unit it reads, so the swap commutes with the count), 32-bit units (`U32`).
  `g xs j` is unit `j`.
* **Sums.** A loop that has consumed the first `P` units and counted
  `sumTo (w ∘ g xs) P`, followed by a scalar tail on the rest, returns
  `L (xs.map w)` (`sum_tail`). Blocks of `N` lanes sum to the same
  (`sum_blocks`); so do blocks of `K` vectors of `N` lanes whose lane counters
  add the `K` vectors lane by lane (`sum_unrolled`).
* **Lane counters.** `fLoop` is a loop that adds a per-lane increment into
  `W`-bit lanes (`% W`) and flushes them into a wide counter every `M`
  iterations; `fLoop_inv`: if no increment exceeds `c` and `c * M < W`, no
  lane wraps and the flushed total plus the lanes is the sum of the
  increments. `fLoopDown` is the count-down form of the same loop
  (src/generic/utf16/utf8_length_from_utf16_bytemask.h:25-76).
* **Scalar functions** (`scal`): the scalar loops of include/simdutf/scalar
  add a per-unit term (`sc8of16`, `sc32of16`, `scCount8`, `sc16of8`,
  `sc8of32`, `sc16of32`, `scL1`); each term equals the weight
  (`sc*_eq`, all values, by cases or `decide`).
* **Signed bytes.** `i8 b` is the byte `b` read as `int8_t`.
* **32-bit masks.** `and_hi`: for `u < 2^32` and `k ≤ 32`,
  `u &&& (2^32 - 2^k) = 0 ↔ u < 2^k` (the `min(one, in & 0xffffff80)`
  forms).

Sums of `size_t` are `Nat`: every result is at most 4 per unit, and an
input of `n` units of `s` bytes occupies `s n` bytes of memory, so the
64-bit sums do not wrap. The narrow lane counters are modelled with their
width (`% 256`, `% 65536`, `% 2^32`) and `fLoop_inv` shows they do not wrap.

What remains read from the C++: the transcriptions (each cites its lines);
that a bitmask of a vector comparison has bit `i` set exactly when lane `i`
satisfies the comparison (`count_ones` of it is the number of such lanes;
the `simd16x32` bitmasks of westmere, haswell, arm64 and ppc64 carry two bits
per 16-bit lane, src/simdutf/westmere/simd16-inl.h:192-205,
src/simdutf/haswell/simd16-inl.h:210-214,
src/simdutf/arm64/simd16-inl.h:326-347, src/simdutf/ppc64/simd16-inl.h:281-294);
that the horizontal sums (`sum()`, `sum_bytes`, `sum_8bytes`, `_mm_sad_epu8`,
`vaddlvq`) add the lanes without wrapping (they widen to 64 bits, except
where noted); the RVV semantics of `vsetvl` and `vcpop` as in `RvvExact`.
-/
namespace SafetyMargins.Lengths
open SafetyMargins SafetyMargins.ReplacementMore SafetyMargins.RvvExact
set_option autoImplicit false

/-! ## Weights and unit types -/

abbrev w32 : Nat → Nat := IcelakeReal.w32v
abbrev w16 : Nat → Nat := Utf8.w16
abbrev w01 : Nat → Nat := Exact.w01

/-- Every unit is a 32-bit value. -/
def U32 (xs : List Nat) : Prop := ∀ u ∈ xs, u < 2 ^ 32

theorem g_lt_of {xs : List Nat} {n : Nat} (h : ∀ u ∈ xs, u < n) (hn : 0 < n) (j : Nat) :
    g xs j < n := by
  unfold g
  rw [List.getD_eq_getElem?_getD]
  cases e : xs[j]? with
  | none => simpa using hn
  | some v => simpa using h v (List.mem_of_getElem? e)

theorem g_drop (xs : List Nat) (a j : Nat) : g (xs.drop a) j = g xs (a + j) := by
  unfold g
  rw [List.getD_eq_getElem?_getD, List.getD_eq_getElem?_getD, List.getElem?_drop]

theorem g_byte {xs : List Nat} (h : Bytes xs) (j : Nat) : g xs j < 256 := g_lt_of h (by decide) j
theorem g_u16 {xs : List Nat} (h : U16 xs) (j : Nat) : g xs j < 65536 := g_lt_of h (by decide) j
theorem g_u32 {xs : List Nat} (h : U32 xs) (j : Nat) : g xs j < 2 ^ 32 := g_lt_of h (by decide) j

/-- The four length functions that return `length`
(include/simdutf/implementation.h:1082-1085, :1095-1098, :3854-3857, :3867-3870):
the per-unit sum of the weight 1. -/
theorem trivial_lengths (xs : List Nat) : xs.length = L (xs.map fun _ => 1) :=
  (L_map_one xs).symm

theorem iteT {α : Type} {c : Prop} [Decidable c] (h : c) (a b : α) : (if c then a else b) = a := by
  simp [h]
theorem iteF {α : Type} {c : Prop} [Decidable c] (h : ¬c) (a b : α) : (if c then a else b) = b := by
  simp [h]

/-! ## Sums over units -/

theorem sumTo_succ (f : Nat → Nat) (k : Nat) : sumTo f (k + 1) = sumTo f k + f k := rfl

/-- `sumTo` of the first `k` units plus `L` of the rest is `L` of the input. -/
theorem sum_tail (w : Nat → Nat) (xs : List Nat) :
    ∀ k, k ≤ xs.length → sumTo (fun j => w (g xs j)) k + L ((xs.drop k).map w) = L (xs.map w) := by
  intro k
  induction k with
  | zero => intro _; simp [sumTo]
  | succ k ih =>
    intro hk
    have h := ih (by omega)
    rw [drop_g xs k (by omega)] at h
    simp only [List.map_cons, L_cons] at h
    rw [sumTo_succ]; omega

/-- The whole input as `sumTo`. -/
theorem sum_all (w : Nat → Nat) (xs : List Nat) :
    sumTo (fun j => w (g xs j)) xs.length = L (xs.map w) := by
  have := sum_tail w xs xs.length (Nat.le_refl _)
  simpa using this

/-- Blocks of `N` consecutive units. -/
theorem sum_blocks (f : Nat → Nat) (N : Nat) :
    ∀ B, sumTo (fun b => sumTo (fun i => f (b * N + i)) N) B = sumTo f (B * N) := by
  intro B
  induction B with
  | zero => simp [sumTo]
  | succ B ih =>
    rw [sumTo_succ, ih, Nat.succ_mul, sumTo_add]

theorem sumTo_comm (f : Nat → Nat → Nat) (A : Nat) :
    ∀ B, sumTo (fun a => sumTo (fun b => f a b) B) A = sumTo (fun b => sumTo (fun a => f a b) A) B := by
  intro B
  induction B with
  | zero => simp [sumTo, sumTo_zero]
  | succ B ih =>
    simp only [sumTo_succ]
    rw [sumTo_plus, ih]

/-- `K` vectors of `N` lanes added lane by lane: the lane sums are the sum over
the `K N` units of the block. -/
theorem sum_unrolled (f : Nat → Nat) (K N base : Nat) :
    sumTo (fun i => sumTo (fun k => f (base + k * N + i)) K) N = sumTo (fun j => f (base + j)) (K * N) := by
  rw [sumTo_comm (fun i k => f (base + k * N + i)) N K]
  have := sum_blocks (fun j => f (base + j)) N K
  simp only [Nat.add_assoc] at this ⊢
  exact this

theorem sumTo_mono (f h : Nat → Nat) (m : Nat) (e : ∀ i, i < m → f i ≤ h i) :
    sumTo f m ≤ sumTo h m := by
  induction m with
  | zero => exact Nat.le_refl _
  | succ m ih =>
    simp only [sumTo_succ]
    have := ih (fun i hi => e i (by omega)); have := e m (by omega); omega

theorem sumTo_const (c m : Nat) : sumTo (fun _ => c) m = m * c := by
  induction m with
  | zero => simp [sumTo]
  | succ m ih => rw [sumTo_succ, ih, Nat.succ_mul]

theorem sumTo_eq_zero (f : Nat → Nat) (m : Nat) (e : ∀ i, i < m → f i = 0) : sumTo f m = 0 := by
  rw [sumTo_congr f (fun _ => 0) m e, sumTo_zero]

theorem sumTo_scale (c : Nat) (f : Nat → Nat) (m : Nat) :
    sumTo (fun i => c * f i) m = c * sumTo f m := by
  induction m with
  | zero => simp [sumTo]
  | succ m ih => simp only [sumTo_succ, ih, Nat.mul_add]

/-- A loop that consumed `B` blocks of `N` units, block `b` counted as the sum of
its units, then the scalar tail on the rest: `L` of the input. -/
theorem assemble (w : Nat → Nat) (xs : List Nat) (N B : Nat) (hB : B * N ≤ xs.length)
    (blk : Nat → Nat) (h : ∀ b, b < B → blk b = sumTo (fun i => w (g xs (b * N + i))) N) :
    sumTo blk B + L ((xs.drop (B * N)).map w) = L (xs.map w) := by
  rw [sumTo_congr _ _ B h, sum_blocks (fun j => w (g xs j)) N B]
  exact sum_tail w xs _ hB

/-! ## Lane counters flushed every `M` iterations -/

/-- Narrow lanes `loc` (lane index → value), the iterations since the last
flush, the flushed total. -/
structure FS where
  loc : Nat → Nat
  it : Nat
  tot : Nat

/-- One iteration: every lane `i < N` adds `inc i` modulo `W`; the iteration
count goes up; at `M` the lanes are added into the total and cleared. -/
def fStep (N W M : Nat) (inc : Nat → Nat) (s : FS) : FS :=
  let loc' := fun i => (s.loc i + inc i) % W
  if s.it + 1 = M then ⟨fun _ => 0, 0, s.tot + sumTo loc' N⟩ else ⟨loc', s.it + 1, s.tot⟩

/-- `B` iterations; iteration `b` adds `inc b`. -/
def fLoop (N W M : Nat) (inc : Nat → Nat → Nat) : Nat → FS
  | 0 => ⟨fun _ => 0, 0, 0⟩
  | b + 1 => fStep N W M (inc b) (fLoop N W M inc b)

/-- **No lane wraps.** -/
theorem fLoop_inv (N W M c : Nat) (inc : Nat → Nat → Nat) (hM : 0 < M) (hW : c * M < W)
    (hc : ∀ b i, i < N → inc b i ≤ c) : ∀ B,
    (fLoop N W M inc B).it < M ∧ (∀ i, i < N → (fLoop N W M inc B).loc i ≤ c * (fLoop N W M inc B).it) ∧
    (fLoop N W M inc B).tot + sumTo (fLoop N W M inc B).loc N = sumTo (fun b => sumTo (inc b) N) B := by
  intro B
  induction B with
  | zero => simp [fLoop, sumTo_zero, sumTo]; omega
  | succ B ih =>
    obtain ⟨i1, i2, i3⟩ := ih
    simp only [fLoop]
    generalize fLoop N W M inc B = s at *
    have hl : ∀ i, i < N → (s.loc i + inc B i) % W = s.loc i + inc B i := by
      intro i hi
      apply Nat.mod_eq_of_lt
      have := i2 i hi; have := hc B i hi
      have : c * s.it + c ≤ c * M := by
        rw [← Nat.mul_succ]; exact Nat.mul_le_mul_left c (by omega)
      omega
    have hs : sumTo (fun i => (s.loc i + inc B i) % W) N = sumTo s.loc N + sumTo (inc B) N := by
      rw [sumTo_congr _ _ N hl, sumTo_plus]
    unfold fStep
    simp only
    split
    · refine ⟨hM, fun i _ => by simp, ?_⟩
      dsimp only
      rw [sumTo_zero, hs, sumTo_succ]; omega
    · refine ⟨by dsimp only; omega, fun i hi => ?_, ?_⟩
      · dsimp only
        rw [hl i hi]; have := i2 i hi; have := hc B i hi
        rw [Nat.mul_succ]; omega
      · dsimp only
        rw [hs, sumTo_succ]; omega

/-- The lanes are zero when no iteration ran since the last flush, so the
`if (iterations > 0)` before the last horizontal sum changes nothing. -/
theorem fLoop_final (N W M c : Nat) (inc : Nat → Nat → Nat) (hM : 0 < M) (hW : c * M < W)
    (hc : ∀ b i, i < N → inc b i ≤ c) (B : Nat) :
    (fLoop N W M inc B).tot + (if (fLoop N W M inc B).it > 0 then sumTo (fLoop N W M inc B).loc N else 0) =
      sumTo (fun b => sumTo (inc b) N) B := by
  obtain ⟨-, i2, i3⟩ := fLoop_inv N W M c inc hM hW hc B
  split
  · exact i3
  · have : sumTo (fLoop N W M inc B).loc N = 0 :=
      sumTo_eq_zero _ _ (fun i hi => by have := i2 i hi; rw [show (fLoop N W M inc B).it = 0 by omega] at this; omega)
    omega

/-- The count-down form (src/generic/utf16/utf8_length_from_utf16_bytemask.h:25-76):
`iteration` starts at `M`, goes down by one per iteration, and the flush
happens when it reaches 0, after which it is `M` again. -/
def fStepDown (N W M : Nat) (inc : Nat → Nat) (s : FS) : FS :=
  let loc' := fun i => (s.loc i + inc i) % W
  if s.it - 1 = 0 then ⟨fun _ => 0, M, s.tot + sumTo loc' N⟩ else ⟨loc', s.it - 1, s.tot⟩

def fLoopDown (N W M : Nat) (inc : Nat → Nat → Nat) : Nat → FS
  | 0 => ⟨fun _ => 0, M, 0⟩
  | b + 1 => fStepDown N W M (inc b) (fLoopDown N W M inc b)

/-- The count-down loop is the count-up loop with `iteration = M - it`. -/
theorem fLoopDown_eq (N W M : Nat) (inc : Nat → Nat → Nat) (hM : 0 < M) : ∀ B,
    (fLoopDown N W M inc B).loc = (fLoop N W M inc B).loc ∧
    (fLoopDown N W M inc B).tot = (fLoop N W M inc B).tot ∧
    (fLoopDown N W M inc B).it = M - (fLoop N W M inc B).it ∧ (fLoop N W M inc B).it < M := by
  intro B
  induction B with
  | zero => simp [fLoopDown, fLoop]; omega
  | succ B ih =>
    obtain ⟨e1, e2, e3, e4⟩ := ih
    simp only [fLoopDown, fLoop]
    generalize fLoopDown N W M inc B = d at *
    generalize fLoop N W M inc B = s at *
    unfold fStepDown fStep
    simp only [e1, e2, e3]
    by_cases h : s.it + 1 = M
    · simp only [show M - s.it - 1 = 0 by omega, h, ite_true]
      exact ⟨trivial, trivial, by omega, hM⟩
    · simp only [show M - s.it - 1 ≠ 0 by omega, h, ite_false]
      exact ⟨trivial, trivial, by omega, by omega⟩

/-! ## Scalar functions -/

/-- A scalar loop that adds `f` of each unit: `L (xs.map f)`. -/
def scal (f : Nat → Nat) (xs : List Nat) : Nat := L (xs.map f)

theorem scal_eq (f w : Nat → Nat) (xs : List Nat) (n : Nat) (hx : ∀ u ∈ xs, u < n)
    (h : ∀ u, u < n → f u = w u) : scal f xs = L (xs.map w) :=
  L_map_congr xs f w (fun u hu => h u (hx u hu))

/-- `int8_t(b)`. -/
def i8 (b : Nat) : Int := if b < 128 then (b : Int) else (b : Int) - 256

/-- include/simdutf/scalar/utf16.h:89-103 (`word` after `swap_if_needed`). -/
def sc8of16 (u : Nat) : Nat :=
  1 + ind (decide (u > 0x7F)) + ind (decide ((u > 0x7FF ∧ u ≤ 0xD7FF) ∨ u ≥ 0xE000))

/-- include/simdutf/scalar/utf16.h:106-116 and :78-87 (`count_code_points`). -/
def sc32of16 (u : Nat) : Nat := ind (u &&& 0xFC00 != 0xDC00)

/-- include/simdutf/scalar/utf8.h:450-465 (`count_code_points`). -/
def scCount8 (b : Nat) : Nat := ind (decide (i8 b > -65))

/-- include/simdutf/scalar/utf8.h:467-482 (`utf16_length_from_utf8`). -/
def sc16of8 (b : Nat) : Nat := ind (decide (i8 b > -65)) + ind (decide (b ≥ 240))

/-- include/simdutf/scalar/utf32.h:53-65 (`utf8_length_from_utf32`). -/
def sc8of32 (u : Nat) : Nat :=
  1 + ind (decide (u > 0x7F)) + ind (decide (u > 0x7FF)) + ind (decide (u > 0xFFFF))

/-- include/simdutf/scalar/utf32.h:67-76 (`utf16_length_from_utf32`). -/
def sc16of32 (u : Nat) : Nat := 1 + ind (decide (u > 0xFFFF))

/-- include/simdutf/scalar/latin1.h:9-19 (`answer++` when `c >> 7`, then `+ len`). -/
def scL1 (b : Nat) : Nat := ind (b >>> 7 != 0) + 1

/-- The byte facts, on all 256 values. -/
def byteOK (b : Nat) : Bool :=
  scCount8 b == w01 b && sc16of8 b == w16 b && scL1 b == wL8 b

theorem bytes_all : (List.range 256).all byteOK = true := by decide +kernel

theorem byte_ok (b : Nat) (h : b < 256) : scCount8 b = w01 b ∧ sc16of8 b = w16 b ∧ scL1 b = wL8 b := by
  have := List.all_eq_true.mp bytes_all b (List.mem_range.mpr h)
  simp only [byteOK, Bool.and_eq_true, beq_iff_eq] at this
  exact ⟨this.1.1, this.1.2, this.2⟩

/-- The 16-bit facts, on all 65536 values. -/
def u16OK (u : Nat) : Bool := sc8of16 u == w8 u && sc32of16 u == w32 u

theorem u16_all : (List.range 65536).all u16OK = true := by decide +kernel

theorem u16_ok (u : Nat) (h : u < 65536) : sc8of16 u = w8 u ∧ sc32of16 u = w32 u := by
  have := List.all_eq_true.mp u16_all u (List.mem_range.mpr h)
  simp only [u16OK, Bool.and_eq_true, beq_iff_eq] at this
  exact this

theorem sc8of32_eq (u : Nat) : sc8of32 u = w8of32 u := by
  unfold sc8of32 w8of32; simp only [gt_iff_lt]

theorem sc16of32_eq (u : Nat) : sc16of32 u = w16of32 u := by
  unfold sc16of32 w16of32; simp only [gt_iff_lt]

/-- **The scalar functions** (the fallback kernel, and the tail of every
vector kernel) return the per-unit sum on every input. -/
theorem scalar_lengths (xs : List Nat) :
    (U16 xs → scal sc8of16 xs = L (xs.map w8) ∧ scal sc32of16 xs = L (xs.map w32)) ∧
    (Bytes xs → scal scCount8 xs = L (xs.map w01) ∧ scal sc16of8 xs = L (xs.map w16) ∧
      scal scL1 xs = L (xs.map wL8)) ∧
    scal sc8of32 xs = L (xs.map w8of32) ∧ scal sc16of32 xs = L (xs.map w16of32) := by
  refine ⟨fun h => ⟨scal_eq _ _ xs _ h (fun u hu => (u16_ok u hu).1),
      scal_eq _ _ xs _ h (fun u hu => (u16_ok u hu).2)⟩,
    fun h => ⟨scal_eq _ _ xs _ h (fun u hu => (byte_ok u hu).1),
      scal_eq _ _ xs _ h (fun u hu => (byte_ok u hu).2.1),
      scal_eq _ _ xs _ h (fun u hu => (byte_ok u hu).2.2)⟩,
    L_map_congr xs _ _ (fun u _ => sc8of32_eq u), L_map_congr xs _ _ (fun u _ => sc16of32_eq u)⟩

/-- The scalar tail of a loop that consumed `P` units. -/
theorem scal_drop (f w : Nat → Nat) (xs : List Nat) (n : Nat) (hx : ∀ u ∈ xs, u < n)
    (h : ∀ u, u < n → f u = w u) (P : Nat) : scal f (xs.drop P) = L ((xs.drop P).map w) :=
  scal_eq f w _ n (fun u hu => hx u (List.mem_of_mem_drop hu)) h

/-! ## 32-bit masks -/

theorem testBit_hiMask (k i : Nat) (hk : k ≤ 32) :
    (2 ^ 32 - 2 ^ k).testBit i = decide (k ≤ i ∧ i < 32) := by
  have e : 2 ^ 32 - 2 ^ k = (2 ^ (32 - k) - 1) * 2 ^ k := by
    rw [Nat.sub_mul, Nat.one_mul, ← Nat.pow_add, show 32 - k + k = 32 by omega]
  rw [e, Nat.testBit_mul_two_pow, Nat.testBit_two_pow_sub_one]
  by_cases h : k ≤ i
  · by_cases h2 : i < 32
    · simp [h, h2]; omega
    · simp [h, h2]; omega
  · simp [h]

/-- `u & (2^32 - 2^k)` is zero exactly when `u < 2^k`, for a 32-bit `u`. -/
theorem and_hi (u k : Nat) (hk : k ≤ 32) (hu : u < 2 ^ 32) :
    (u &&& (2 ^ 32 - 2 ^ k) = 0) ↔ u < 2 ^ k := by
  constructor
  · intro h
    apply Nat.lt_pow_two_of_testBit
    intro i hi
    by_cases h32 : i < 32
    · have := congrArg (fun x => x.testBit i) h
      simp only [Nat.testBit_and, testBit_hiMask k i hk, Nat.zero_testBit] at this
      simpa [hi, h32] using this
    · exact Nat.testBit_lt_two_pow (Nat.lt_of_lt_of_le hu (Nat.pow_le_pow_right (by decide) (by omega)))
  · intro h
    apply Nat.eq_of_testBit_eq
    intro i
    simp only [Nat.testBit_and, testBit_hiMask k i hk, Nat.zero_testBit]
    by_cases hi : k ≤ i
    · rw [Nat.testBit_lt_two_pow (Nat.lt_of_lt_of_le h (Nat.pow_le_pow_right (by decide) hi))]
      simp
    · simp [hi]

theorem min_and_hi (u k : Nat) (hk : k ≤ 32) (hu : u < 2 ^ 32) :
    min (u &&& (2 ^ 32 - 2 ^ k)) 1 = ind (decide (2 ^ k ≤ u)) := by
  have := and_hi u k hk hu
  unfold ind
  by_cases h : u < 2 ^ k
  · rw [this.mpr h, iteF (by simp; omega)]; rfl
  · have h0 : u &&& (2 ^ 32 - 2 ^ k) ≠ 0 := fun e => h (this.mp e)
    rw [iteT (by simp; omega)]
    exact Nat.min_eq_right (Nat.pos_of_ne_zero h0)

/-! ## Lane arithmetic -/

/-- `x -= m` on a `W`-valued lane. -/
def subW (W x m : Nat) : Nat := (x + W - m) % W

/-- A comparison result on a `W`-valued lane: all ones or zero. -/
def maskW (W : Nat) (p : Bool) : Nat := if p then W - 1 else 0

/-- Subtracting a comparison mask adds one when the comparison holds. -/
theorem subW_mask (W x : Nat) (p : Bool) (hW : 0 < W) : subW W x (maskW W p) = (x + ind p) % W := by
  unfold subW maskW ind
  cases p
  · simp [Nat.add_mod_right]
  · simp only [ite_true]; congr 1; omega

/-- `x += y` on a `W`-valued lane, several times. -/
def addSeq (W : Nat) (x : Nat) (ys : List Nat) : Nat := ys.foldl (fun a y => (a + y) % W) x

theorem addSeq_eq (W : Nat) (hW : 0 < W) (ys : List Nat) :
    ∀ x, x < W → addSeq W x ys = (x + L ys) % W := by
  induction ys with
  | nil => intro x hx; simp [addSeq, Nat.mod_eq_of_lt hx]
  | cons y r ih =>
    intro x _
    simp only [addSeq, List.foldl_cons] at ih ⊢
    rw [ih _ (Nat.mod_lt _ hW), L_cons, Nat.mod_add_mod, Nat.add_assoc]

theorem sumTo_compl (f : Nat → Nat) (m : Nat) (h : ∀ i, i < m → f i ≤ 1) :
    sumTo f m + sumTo (fun i => 1 - f i) m = m := by
  rw [← sumTo_plus, sumTo_congr _ (fun _ => 1) m (fun i hi => by have := h i hi; omega),
    sumTo_const, Nat.mul_one]

theorem sumTo_pred (f : Nat → Nat) (m : Nat) (h : ∀ i, i < m → 1 ≤ f i) :
    sumTo (fun i => f i - 1) m + m = sumTo f m := by
  have := sumTo_plus (fun i => f i - 1) (fun _ => 1) m
  rw [sumTo_const, Nat.mul_one] at this
  rw [← this]
  exact sumTo_congr _ _ m (fun i hi => by have := h i hi; omega)

/-- A count of a 64-bit (`simd16x32`) bitmask with two bits per lane, halved. -/
theorem half_two (s : Nat) : (2 * s) / 2 = s := by omega

/-! ## Lane counters summed in chunks

src/generic/utf32.h:42-89: the outer loop takes `iterations = min(blocks,
max_iterations)`, the inner loop adds that many blocks into the lanes, then
`counter += acc.sum()`. -/

/-- The lanes after `t` inner iterations from block `b0`. -/
def inner (W : Nat) (inc : Nat → Nat → Nat) (b0 : Nat) : Nat → Nat → Nat
  | 0 => fun _ => 0
  | t + 1 => fun i => (inner W inc b0 t i + inc (b0 + t) i) % W

theorem inner_eq (W c : Nat) (inc : Nat → Nat → Nat) (b0 : Nat) (hc : ∀ b i, inc b i ≤ c) :
    ∀ t, t * c < W → ∀ i, inner W inc b0 t i = sumTo (fun t' => inc (b0 + t') i) t := by
  intro t
  induction t with
  | zero => intro _ i; rfl
  | succ t ih =>
    intro ht i
    simp only [inner]
    rw [ih (by rw [Nat.succ_mul] at ht; omega) i, sumTo_succ]
    apply Nat.mod_eq_of_lt
    have := sumTo_le (fun t' => inc (b0 + t') i) c t (fun t' _ => hc _ _)
    have := hc (b0 + t) i
    rw [Nat.succ_mul] at ht; omega

/-- The outer loop: `fuel` bounds the number of chunks. -/
def outer (N W maxI : Nat) (inc : Nat → Nat → Nat) : Nat → Nat → Nat → Nat → Nat
  | 0, _, _, counter => counter
  | f + 1, blocks, b0, counter =>
    if blocks = 0 then counter else
    let it := min blocks maxI
    outer N W maxI inc f (blocks - it) (b0 + it) (counter + sumTo (inner W inc b0 it) N)

theorem outer_eq (N W maxI c : Nat) (inc : Nat → Nat → Nat) (hM : 0 < maxI) (hW : maxI * c < W)
    (hc : ∀ b i, inc b i ≤ c) : ∀ f blocks b0 counter, blocks ≤ f →
    outer N W maxI inc f blocks b0 counter =
      counter + sumTo (fun b => sumTo (inc (b0 + b)) N) blocks := by
  intro f
  induction f with
  | zero => intro blocks b0 counter h; simp [outer, show blocks = 0 by omega, sumTo]
  | succ f ih =>
    intro blocks b0 counter h
    simp only [outer]
    split
    · subst blocks; simp [sumTo]
    · have hit : min blocks maxI * c < W :=
        Nat.lt_of_le_of_lt (Nat.mul_le_mul_right c (Nat.min_le_right _ _)) hW
      rw [ih _ _ _ (by omega)]
      have e1 : sumTo (inner W inc b0 (min blocks maxI)) N =
          sumTo (fun b => sumTo (inc (b0 + b)) N) (min blocks maxI) := by
        rw [sumTo_congr _ _ N (fun i _ => inner_eq W c inc b0 hc _ hit i)]
        exact (sumTo_comm (fun b i => inc (b0 + b) i) (min blocks maxI) N).symm
      have e2 := sumTo_add (fun b => sumTo (inc (b0 + b)) N) (min blocks maxI) (blocks - min blocks maxI)
      rw [show min blocks maxI + (blocks - min blocks maxI) = blocks by omega] at e2
      rw [e1, e2]
      simp only [Nat.add_assoc]

/-! ## Unrolled inner loops

The Latin-1 functions of westmere, haswell and icelake, and the icelake
`count_utf8`, run an unrolled loop `for (; i + K * V <= max_i; i += K * V)` then
`for (; i <= max_i; i += V)`, with `max_i = i0 + (it - 1) V`: in vector units
`t + K ≤ it - 1`, then `t + 1 ≤ it`. `grp` is such a loop on lanes `r` (`% W`;
`W = 0` is a 64-bit sum that does not wrap, `x % 0 = x`); vector `t` is block
`b0 + t`. -/

def grp (W K : Nat) (inc : Nat → Nat → Nat) (b0 lim : Nat) :
    Nat → Nat → (Nat → Nat) → Nat × (Nat → Nat)
  | 0, t, r => (t, r)
  | f + 1, t, r =>
    if t + K ≤ lim then
      grp W K inc b0 lim f (t + K) (fun l => (r l + sumTo (fun k => inc (b0 + t + k) l) K) % W)
    else (t, r)

theorem grp_spec (W K : Nat) (inc : Nat → Nat → Nat) (b0 lim : Nat) (hK : 0 < K) :
    ∀ f t r, ((t ≤ (grp W K inc b0 lim f t r).1) ∧
      ((grp W K inc b0 lim f t r).1 = t ∨ (grp W K inc b0 lim f t r).1 ≤ lim) ∧
      (lim < t + f → lim < (grp W K inc b0 lim f t r).1 + K) ∧
      ∀ l, (grp W K inc b0 lim f t r).2 l =
        (r l + sumTo (fun s => inc (b0 + t + s) l) ((grp W K inc b0 lim f t r).1 - t)) % W ∨
        ((grp W K inc b0 lim f t r).1 = t ∧ (grp W K inc b0 lim f t r).2 l = r l)) := by
  intro f
  induction f with
  | zero => intro t r; simp [grp]; omega
  | succ f ih =>
    intro t r
    simp only [grp]
    split
    · obtain ⟨a1, a2, a3, a4⟩ := ih (t + K) (fun l => (r l + sumTo (fun k => inc (b0 + t + k) l) K) % W)
      generalize grp W K inc b0 lim f (t + K) _ = q at *
      refine ⟨by omega, Or.inr (by omega), fun h => a3 (by omega), fun l => Or.inl ?_⟩
      rcases a4 l with e | ⟨e1, e2⟩
      · rw [e, Nat.mod_add_mod, Nat.add_assoc]
        congr 2
        have := sumTo_add (fun s => inc (b0 + t + s) l) K (q.1 - (t + K))
        rw [show K + (q.1 - (t + K)) = q.1 - t by omega] at this
        rw [this]
        congr 1
        exact sumTo_congr _ _ _ (fun s _ => by rw [show b0 + (t + K) + s = b0 + t + (K + s) by omega])
      · rw [e2, e1, show t + K - t = K by omega]
    · rename_i h
      refine ⟨Nat.le_refl _, Or.inl rfl, fun _ => by simp; omega, fun l => Or.inr ⟨rfl, rfl⟩⟩

/-- The unrolled loop (`K` vectors per iteration while `t + K ≤ it - 1`) then the
single loop (while `t + 1 ≤ it`) run exactly `it` vectors: the lanes are the
sums of the `it` increments. -/
theorem grp_two (W K : Nat) (inc : Nat → Nat → Nat) (b0 it : Nat) (hK : 0 < K) (r0 : Nat → Nat)
    (hW : ∀ l, W = 0 ∨ r0 l + sumTo (fun s => inc (b0 + s) l) it < W) (p q : Nat × (Nat → Nat))
    (hp : p = grp W K inc b0 (it - 1) it 0 r0) (hq : q = grp W 1 inc b0 it (it + 1) p.1 p.2) :
    q.1 = it ∧ ∀ l, q.2 l = r0 l + sumTo (fun s => inc (b0 + s) l) it := by
  obtain ⟨a1, a2, -, a4⟩ := grp_spec W K inc b0 (it - 1) hK it 0 r0
  rw [← hp] at a1 a2 a4
  have hp1 : p.1 ≤ it := by rcases a2 with h | h <;> omega
  obtain ⟨b1, b2, b3, b4⟩ := grp_spec W 1 inc b0 it (by decide) (it + 1) p.1 p.2
  rw [← hq] at b1 b2 b3 b4
  have hq1 : q.1 = it := by have := b3 (by omega); rcases b2 with h | h <;> omega
  refine ⟨hq1, fun l => ?_⟩
  have hfin : (r0 l + sumTo (fun s => inc (b0 + s) l) it) % W = r0 l + sumTo (fun s => inc (b0 + s) l) it := by
    rcases hW l with h | h
    · subst h; simp
    · exact Nat.mod_eq_of_lt h
  have hpl : p.2 l = (r0 l + sumTo (fun s => inc (b0 + s) l) p.1) % W ∨ (p.1 = 0 ∧ p.2 l = r0 l) := by
    rcases a4 l with e | ⟨e1, e2⟩
    · left; simpa using e
    · right; exact ⟨e1, e2⟩
  have hsplit := sumTo_add (fun s => inc (b0 + s) l) p.1 (it - p.1)
  rw [show p.1 + (it - p.1) = it by omega] at hsplit
  have hcongr : sumTo (fun s => inc (b0 + p.1 + s) l) (it - p.1) =
      sumTo (fun s => inc (b0 + (p.1 + s)) l) (it - p.1) :=
    sumTo_congr _ _ _ (fun s _ => by rw [Nat.add_assoc])
  rcases b4 l with e | ⟨e1, e2⟩
  · rw [e, hq1, hcongr]
    rcases hpl with e' | ⟨e1', e2'⟩
    · rw [e', Nat.mod_add_mod, Nat.add_assoc, ← hsplit, hfin]
    · rw [e2', e1']; simp only [Nat.zero_add, Nat.sub_zero]
      exact hfin
  · rw [e2]
    rcases hpl with e' | ⟨e1', e2'⟩
    · rw [e', show p.1 = it by omega, hfin]
    · rw [e2']; have : it = 0 := by omega
      subst this; simp [sumTo]

/-- The same two loops with separate 64-bit accumulators (`W = 0`), as in
the icelake `count_utf8`: together they hold the sum of the `it` increments. -/
theorem grp_sep (K : Nat) (inc : Nat → Nat → Nat) (b0 it : Nat) (hK : 0 < K) (p q : Nat × (Nat → Nat))
    (hp : p = grp 0 K inc b0 (it - 1) it 0 (fun _ => 0))
    (hq : q = grp 0 1 inc b0 it (it + 1) p.1 (fun _ => 0)) :
    q.1 = it ∧ ∀ l, p.2 l + q.2 l = sumTo (fun s => inc (b0 + s) l) it := by
  obtain ⟨a1, a2, -, a4⟩ := grp_spec 0 K inc b0 (it - 1) hK it 0 (fun _ => 0)
  rw [← hp] at a1 a2 a4
  have hp1 : p.1 ≤ it := by rcases a2 with h | h <;> omega
  obtain ⟨b1, b2, b3, b4⟩ := grp_spec 0 1 inc b0 it (by decide) (it + 1) p.1 (fun _ => 0)
  rw [← hq] at b1 b2 b3 b4
  have hq1 : q.1 = it := by have := b3 (by omega); rcases b2 with h | h <;> omega
  refine ⟨hq1, fun l => ?_⟩
  have hpl : p.2 l = sumTo (fun s => inc (b0 + s) l) p.1 := by
    rcases a4 l with e | ⟨e1, e2⟩
    · simpa using e
    · rw [e2, e1]; rfl
  have hql : q.2 l = sumTo (fun s => inc (b0 + (p.1 + s)) l) (it - p.1) := by
    rcases b4 l with e | ⟨e1, e2⟩
    · rw [e, hq1, Nat.mod_zero, Nat.zero_add]
      exact sumTo_congr _ _ _ (fun s _ => by rw [Nat.add_assoc])
    · rw [e2, show it - p.1 = 0 by omega]; rfl
  rw [hpl, hql]
  have := sumTo_add (fun s => inc (b0 + s) l) p.1 (it - p.1)
  rw [show p.1 + (it - p.1) = it by omega] at this
  exact this.symm

end SafetyMargins.Lengths
