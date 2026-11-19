import SafetyMargins.RvvExact
/-
The scalar code.

`Basic.written` charges the scalar code that finishes a conversion (fuel `0`,
and an iteration with fewer than `N` units left) `L` of the units it is given.
This file proves that charge for the scalar converters of
`include/simdutf/scalar/`, from transcriptions of their loops.

* **`run`** is a scalar loop: `step xs` either converts the units at the head
  of `xs` (`some (c, w)`: consume `c` units, write `w`) or returns
  (`none`). Every store of these loops is a single unit written at the output
  cursor, which then advances, so the furthest store is the number of units
  written. `run_le`: when every step writes at most `L` of the units it
  consumes, the loop writes at most `L` of its input, for any fuel (so for
  the run of the C++, which takes at most as many steps as there are units,
  `run_steps`).
* **The converters.** One step function per loop, transcribed with its
  fast path. The C++ `convert` and `convert_with_errors` of a pair make the
  same stores and differ only in what they return, so one step serves both
  (cited for both).
  - UTF-8 sources (`utf8_to_utf16.h`, `utf8_to_utf32.h`, `utf8_to_latin1.h`,
    and their `valid_*` versions): a step converts one character from its
    leading byte and writes at most the weight of that byte; the 16-byte (or
    8-byte) ASCII fast path writes one unit per byte, and its test, on the
    64-bit words the C++ reads, holds only when all the bytes are ASCII
    (`probe_ascii`). These hold for every byte string, so the `convert_valid`
    decoders are safe on every input too.
  - UTF-16 sources: every unit weighs at least 1 under
    `utf8_length_from_utf16`, so the 4-unit fast path is safe whatever its
    test says; the `convert_valid` versions convert a surrogate without
    checking its partner and are proved on valid UTF-16 (`Valid16`), their
    contract. On invalid input they can write more: `valid_16to8_over`.
  - UTF-32 and Latin-1 sources: one step per unit (or two, in the fast paths).
* **`rewind_and_convert_with_errors`** (`rewind`): it may step back over
  continuation bytes to a leading byte before the cursor and convert from
  there. It writes at most `L` of the bytes from that leading byte on
  (`rewind_le`); when the byte at the cursor is not a continuation byte, or
  the byte before it is ASCII, that is `L` of the bytes from the cursor
  (`rewind_le_rest`).
* **The inline fallbacks** (`runK`, `fallback_scalar`): the scalar loops
  inside the vector kernels (surrogates, invalid units, units Latin-1 cannot
  hold), which the path tables list as `Kernels.scalar k`, are iterations of
  `Kernels.scalar k` that write at most `L` of what they consume
  (`fbA_scalar`, `fbB_scalar`, `fbC_scalar`, `fbD_scalar`, `fbL_scalar`).
* **`writtenT`** is `Basic.written` with the scalar code as a function of the
  unit values; `writtenT_le` shows that any tail meeting the bound gives a
  furthest store at most `written`, so `safe_of_paths` covers the actual tail
  (`safe_of_paths_tail`).

Read from the C++: the transcriptions (the cited lines). Byte order is a
bijection applied to each unit as it is read; the models take the unit values
after it.
-/
namespace SafetyMargins.Scalar
open SafetyMargins
set_option autoImplicit false
set_option linter.unusedSimpArgs false

/-! ## Scalar loops -/

/-- A scalar loop with fuel: `step xs = some (c, w)` consumes `c` units and
writes `w` units at the cursor; `none` returns. The result is the number of
units written, which is where the furthest store ends. -/
def run (step : List Nat → Option (Nat × Nat)) : Nat → List Nat → Nat
  | 0, _ => 0
  | _ + 1, [] => 0
  | f + 1, x :: r =>
    match step (x :: r) with
    | none => 0
    | some (c, w) => w + run step f ((x :: r).drop c)

theorem L_map_take_add_drop (wt : Nat → Nat) (c : Nat) (xs : List Nat) :
    L (xs.map wt) = L ((xs.map wt).take c) + L ((xs.drop c).map wt) := by
  rw [List.map_drop]
  exact L_take_add_drop c _

/-- **The loop bound.** If every step writes at most `L` of the units it
consumes, the loop writes at most `L` of its input. `P` is an invariant of the
input (valid UTF-16 for the `convert_valid` loops; `fun _ => True` otherwise). -/
theorem run_le (wt : Nat → Nat) (P : List Nat → Prop) (step : List Nat → Option (Nat × Nat))
    (hs : ∀ x r c w, P (x :: r) → step (x :: r) = some (c, w) →
      w ≤ L (((x :: r).map wt).take c) ∧ P ((x :: r).drop c)) :
    ∀ f xs, P xs → run step f xs ≤ L (xs.map wt) := by
  intro f
  induction f with
  | zero => intro xs _; simp [run]
  | succ f ih =>
    intro xs hp
    rcases xs with _ | ⟨x, r⟩
    · simp [run]
    · simp only [run]
      split
      · exact Nat.zero_le _
      · rename_i c w h
        obtain ⟨h1, h2⟩ := hs x r c w hp h
        have := ih _ h2
        have := L_map_take_add_drop wt c (x :: r)
        omega

/-- The same with no invariant. -/
theorem run_le' (wt : Nat → Nat) (step : List Nat → Option (Nat × Nat))
    (hs : ∀ x r c w, step (x :: r) = some (c, w) → w ≤ L (((x :: r).map wt).take c)) :
    ∀ f xs, run step f xs ≤ L (xs.map wt) := fun f xs =>
  run_le wt (fun _ => True) step (fun x r c w _ h => ⟨hs x r c w h, trivial⟩) f xs trivial

/-- A step that consumes at least one unit: the loop of the C++, which runs
until the input is exhausted or a step returns, is `run` with fuel the input
length (each further unit of fuel changes nothing). -/
theorem run_steps (step : List Nat → Option (Nat × Nat))
    (hc : ∀ x r c w, step (x :: r) = some (c, w) → 1 ≤ c) :
    ∀ f g xs, xs.length ≤ f → xs.length ≤ g → run step f xs = run step g xs := by
  intro f
  induction f with
  | zero => intro g xs h _; cases xs <;> simp_all [run]; cases g <;> simp [run]
  | succ f ih =>
    intro g xs hf hg
    rcases xs with _ | ⟨x, r⟩
    · cases g <;> simp [run]
    · cases g with
      | zero => simp at hg
      | succ g =>
        simp only [run]
        split
        · rfl
        · rename_i c w h
          have := hc x r c w h
          have hl : ((x :: r).drop c).length ≤ r.length := by
            rw [List.length_drop, List.length_cons]; omega
          simp only [List.length_cons] at hf hg
          rw [ih g _ (by omega) (by omega)]

/-- The head's weight is paid by any step that consumes it. -/
theorem head_le (wt : Nat → Nat) (x : Nat) (r : List Nat) (c : Nat) (hc : 1 ≤ c) :
    wt x ≤ L (((x :: r).map wt).take c) := by
  obtain ⟨c', rfl⟩ : ∃ c', c = c' + 1 := ⟨c - 1, by omega⟩
  simp

/-- `k` units of weight 1 at the head pay for `k` units written. -/
theorem ones_le (wt : Nat → Nat) (k : Nat) (xs : List Nat) (hk : k ≤ xs.length)
    (h1 : ∀ i, i < k → wt (xs.getD i 0) = 1) : k ≤ L ((xs.map wt).take k) := by
  induction k generalizing xs with
  | zero => simp
  | succ k ih =>
    rcases xs with _ | ⟨x, r⟩
    · simp at hk
    · have hx := h1 0 (by omega)
      have := ih r (by simp at hk; omega) (fun i hi => by simpa using h1 (i + 1) (by omega))
      simp at hx ⊢
      omega

/-- Every unit weighs at least 1: `k` units pay for `k` written, for any `k`. -/
theorem pos_le (wt : Nat → Nat) (hw : ∀ v, 1 ≤ wt v) (k : Nat) (xs : List Nat) (hk : k ≤ xs.length) :
    k ≤ L ((xs.map wt).take k) := by
  induction k generalizing xs with
  | zero => simp
  | succ k ih =>
    rcases xs with _ | ⟨x, r⟩
    · simp at hk
    · have := ih r (by simp at hk; omega)
      have := hw x
      simp
      omega

/-! ## Masks

The C++ tests masks such as `word & 0xF800`. On a value below `2^n`, a mask of
the bits `k .. n-1` keeps `2^k * (x / 2^k)`, which `omega` handles. -/

theorem and_high (x n k : Nat) (hx : x < 2 ^ n) (hk : k ≤ n) :
    x &&& (2 ^ n - 2 ^ k) = 2 ^ k * (x / 2 ^ k) := by
  have hm : 2 ^ n - 2 ^ k = 2 ^ k * (2 ^ (n - k) - 1) := by
    rw [Nat.mul_sub, Nat.mul_one, ← Nat.pow_add, Nat.add_sub_cancel' hk]
  rw [hm]
  apply Nat.eq_of_testBit_eq
  intro i
  rw [Nat.testBit_and, Nat.testBit_two_pow_mul, Nat.testBit_two_pow_mul, Nat.testBit_two_pow_sub_one,
    Nat.testBit_div_two_pow]
  by_cases hi : k ≤ i
  · have : i - k + k = i := by omega
    rw [this]
    by_cases hin : i < n
    · simp [hi, show i - k < n - k by omega]
    · have : x.testBit i = false := Nat.testBit_lt_two_pow (Nat.lt_of_lt_of_le hx
        (Nat.pow_le_pow_right (by omega) (by omega)))
      simp [this, hi]
  · simp [hi]

/-- A 16-bit mask of the bits `k .. 15`. -/
theorem and16 (x k : Nat) (hx : x < 65536) (hk : k ≤ 16) :
    x &&& (65536 - 2 ^ k) = 2 ^ k * (x / 2 ^ k) := and_high x 16 k hx hk

/-- A 32-bit mask of the bits `k .. 31`. -/
theorem and32 (x k : Nat) (hx : x < 2 ^ 32) (hk : k ≤ 32) :
    x &&& (2 ^ 32 - 2 ^ k) = 2 ^ k * (x / 2 ^ k) := and_high x 32 k hx hk


/-! ## The ASCII test on 64-bit words

`memcpy(&v, data + pos, 8)` reads eight bytes as a little-endian word
(`le64`). The UTF-8 fast paths test `(v1 | v2) & 0x8080808080808080` (16
bytes) or `v & 0x8080808080808080` (8 bytes). -/

/-- Little-endian value of a byte list. -/
def le64 : List Nat → Nat
  | [] => 0
  | b :: bs => b + 256 * le64 bs

/-- The word `memcpy` reads at byte `i`. -/
def word8 (xs : List Nat) (i : Nat) : Nat := le64 ((xs.drop i).take 8)

/-- `0x80` in each of `k` bytes. -/
def mask80 : Nat → Nat
  | 0 => 0
  | k + 1 => 128 + 256 * mask80 k

theorem mask80_8 : mask80 8 = 0x8080808080808080 := by decide

/-- The 16-byte test of `utf8_to_utf16.h:25-32`, `utf8_to_utf32.h:25-32`,
`utf8_to_latin1.h:27-35`, `valid_utf8_to_latin1.h:27-36` and
`latin1_to_utf8.h:28-37` (all in include/simdutf/scalar/). -/
def probe16 (xs : List Nat) : Bool := (word8 xs 0 ||| word8 xs 8) &&& 0x8080808080808080 == 0

/-- The 8-byte test of include/simdutf/scalar/utf8_to_utf16/valid_utf8_to_utf16.h:24-28 and
include/simdutf/scalar/utf8_to_utf32/valid_utf8_to_utf32.h:25-29. -/
def probe8 (xs : List Nat) : Bool := word8 xs 0 &&& 0x8080808080808080 == 0

def Bytes (xs : List Nat) : Prop := ∀ b ∈ xs, b < 256

theorem Bytes.drop {xs : List Nat} (h : Bytes xs) (n : Nat) : Bytes (xs.drop n) :=
  fun b hb => h b (List.mem_of_mem_drop hb)

theorem Bytes.take {xs : List Nat} (h : Bytes xs) (n : Nat) : Bytes (xs.take n) :=
  fun b hb => h b (List.mem_of_mem_take hb)

theorem and_byte (r m b c : Nat) (hb : b < 2 ^ 8) (hc : c < 2 ^ 8) :
    (2 ^ 8 * r + b) &&& (2 ^ 8 * m + c) = 2 ^ 8 * (r &&& m) + (b &&& c) := by
  apply Nat.eq_of_testBit_eq
  intro i
  have hbc : (b &&& c) < 2 ^ 8 := Nat.and_lt_two_pow b hc
  rw [Nat.testBit_and, Nat.testBit_two_pow_mul_add _ hb, Nat.testBit_two_pow_mul_add _ hc,
    Nat.testBit_two_pow_mul_add _ hbc, Nat.testBit_and, Nat.testBit_and]
  split <;> rfl

theorem byte_and80 : ∀ b : Fin 256, (b.val &&& 128 = 0) = (b.val < 128) := by decide +kernel

/-- **A mask applied to a little-endian word is the mask applied to each
byte.** -/
theorem le64_and_gen : ∀ (bs ms : List Nat), bs.length = ms.length → Bytes bs → Bytes ms →
    (le64 bs &&& le64 ms = 0 ↔ ∀ i, i < bs.length → bs.getD i 0 &&& ms.getD i 0 = 0) := by
  intro bs
  induction bs with
  | nil => intro ms _ _ _; simp [le64]
  | cons b r ih =>
    intro ms hl hb hm
    rcases ms with _ | ⟨m, ms⟩
    · simp at hl
    have hb0 : b < 256 := hb b (by simp)
    have hm0 : m < 256 := hm m (by simp)
    have hr : Bytes r := fun x hx => hb x (by simp [hx])
    have hms : Bytes ms := fun x hx => hm x (by simp [hx])
    simp only [List.length_cons, Nat.add_right_cancel_iff] at hl
    simp only [le64]
    rw [show b + 256 * le64 r = 2 ^ 8 * le64 r + b by omega,
      show m + 256 * le64 ms = 2 ^ 8 * le64 ms + m by omega,
      and_byte _ _ _ _ (by omega) (by omega)]
    have ih' := ih ms hl hr hms
    constructor
    · intro h i hi
      have h1 : (le64 r &&& le64 ms) = 0 := by omega
      have h2 : (b &&& m) = 0 := by omega
      rcases i with _ | i
      · simpa using h2
      · simpa using ih'.mp h1 i (by simp at hi; omega)
    · intro h
      have h2 : (b &&& m) = 0 := by simpa using h 0 (by simp)
      have h1 := ih'.mpr (fun i hi => by simpa using h (i + 1) (by simp; omega))
      rw [h1, h2]

theorem le64_and (bs : List Nat) (hb : Bytes bs) :
    le64 bs &&& mask80 bs.length = 0 ↔ ∀ b ∈ bs, b < 128 := by
  have hm : mask80 bs.length = le64 (List.replicate bs.length 0x80) := by
    generalize bs.length = n
    induction n with
    | zero => rfl
    | succ n ih => simp [mask80, le64, List.replicate_succ, ih]
  rw [hm, le64_and_gen _ _ (by simp) hb (by intro x hx; simp at hx; omega)]
  constructor
  · intro h b hbm
    obtain ⟨i, hi, rfl⟩ := List.getElem_of_mem hbm
    have := h i hi
    have hlt : bs[i] < 256 := hb _ hbm
    rw [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hi, Option.getD_some,
      List.getD_eq_getElem?_getD, List.getElem?_replicate, ite_eq_left hi] at this
    simp only [Option.getD_some] at this
    have h80 := byte_and80 ⟨_, hlt⟩
    simp only at h80
    rw [h80] at this
    exact this
  · intro h i hi
    have hmem : bs[i] ∈ bs := List.getElem_mem hi
    have hlt : bs[i] < 256 := hb _ hmem
    rw [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hi, Option.getD_some,
      List.getD_eq_getElem?_getD, List.getElem?_replicate, ite_eq_left hi]
    simp only [Option.getD_some]
    have h80 := byte_and80 ⟨_, hlt⟩
    simp only at h80
    rw [h80]
    exact h _ hmem

theorem word8_ascii (xs : List Nat) (hb : Bytes xs) (i : Nat) (hi : i + 8 ≤ xs.length)
    (h : word8 xs i &&& 0x8080808080808080 = 0) :
    ∀ j, j < 8 → xs.getD (i + j) 0 < 128 := by
  have hl : ((xs.drop i).take 8).length = 8 := by simp; omega
  have hb' : Bytes ((xs.drop i).take 8) := (hb.drop i).take 8
  rw [← mask80_8, ← hl] at h
  have hall := (le64_and _ hb').mp h
  intro j hj
  have hmem : xs.getD (i + j) 0 ∈ (xs.drop i).take 8 := by
    rw [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem (by omega), Option.getD_some,
      List.mem_iff_getElem]
    exact ⟨j, by simp; omega, by simp⟩
  exact hall _ hmem

/-- **The 16-byte test holds only on 16 ASCII bytes.** -/
theorem probe16_ascii (xs : List Nat) (hb : Bytes xs) (hl : 16 ≤ xs.length)
    (h : probe16 xs = true) : ∀ j, j < 16 → xs.getD j 0 < 128 := by
  simp only [probe16, beq_iff_eq, Nat.and_or_distrib_right, Nat.or_eq_zero_iff] at h
  intro j hj
  by_cases hj8 : j < 8
  · simpa using word8_ascii xs hb 0 (by omega) h.1 j hj8
  · simpa [show 8 + (j - 8) = j by omega] using word8_ascii xs hb 8 (by omega) h.2 (j - 8) (by omega)

/-- **The 8-byte test holds only on 8 ASCII bytes.** -/
theorem probe8_ascii (xs : List Nat) (hb : Bytes xs) (hl : 8 ≤ xs.length)
    (h : probe8 xs = true) : ∀ j, j < 8 → xs.getD j 0 < 128 := by
  simp only [probe8, beq_iff_eq] at h
  intro j hj
  simpa using word8_ascii xs hb 0 (by omega) h j hj

/-! ## UTF-8 sources

`utf16_length_from_utf8` weighs a byte `Utf8.w16` (0 for a continuation byte,
2 for a byte `≥ 0xF0`, else 1); `utf32_length_from_utf8` and
`latin1_length_from_utf8` weigh it `Exact.w01` (0 for a continuation byte,
else 1). -/

open SafetyMargins.Utf8 (w16)
open SafetyMargins.Exact (w01)

/-- The leading-byte classes of the scalar decoders and the weights that pay
for what each writes. -/
theorem lead_w : ∀ b : Fin 256,
    (b.val < 0x80 → w16 b.val = 1 ∧ w01 b.val = 1) ∧
    (b.val &&& 0xE0 = 0xC0 → w16 b.val = 1 ∧ w01 b.val = 1) ∧
    (b.val &&& 0xF0 = 0xE0 → w16 b.val = 1 ∧ w01 b.val = 1) ∧
    (b.val &&& 0xF8 = 0xF0 → w16 b.val = 2 ∧ w01 b.val = 1) := by decide +kernel

theorem ascii_w (b : Nat) (h : b < 128) : w16 b = 1 ∧ w01 b = 1 :=
  (lead_w ⟨b, by omega⟩).1 h

/-- The 16-byte (or `k`-byte) ASCII fast path pays for itself. -/
theorem fast_le (wt : Nat → Nat) (hw : ∀ b, b < 128 → wt b = 1) (k : Nat) (xs : List Nat)
    (hk : k ≤ xs.length) (ha : ∀ j, j < k → xs.getD j 0 < 128) :
    k ≤ L ((xs.map wt).take k) :=
  ones_le wt k xs hk fun i hi => hw _ (ha i hi)

section utf8
variable (xs : List Nat)

/-- Byte `i` (`data[pos + i]`), `0` past the end. -/
abbrev g (i : Nat) : Nat := xs.getD i 0

/-- `(b & 0b11000000) != 0b10000000`: not a continuation byte. -/
abbrev notCont (b : Nat) : Prop := b &&& 0xC0 ≠ 0x80

/-- One step of `scalar::utf8_to_utf16::convert` and `convert_with_errors`
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:19-136 and :148-276): the 16-byte ASCII
fast path (:25-41, :154-170); ASCII (:46-51); a 2-byte character, checked for
length, continuation and overlong (:52-71); 3-byte, also surrogates (:72-96);
4-byte, also the range, writing a surrogate pair (:97-132); anything else
returns (:133-135, :268-275). The two functions return different values on
an error, after the same stores. -/
def u8to16 : Option (Nat × Nat) :=
  if 16 ≤ xs.length ∧ probe16 xs = true then some (16, 16) else
  let b := g xs 0
  if b < 0x80 then some (1, 1)
  else if b &&& 0xE0 = 0xC0 then
    if xs.length ≤ 1 then none
    else if notCont (g xs 1) then none
    else if ((b &&& 0x1F) <<< 6 ||| (g xs 1 &&& 0x3F)) < 0x80 then none
    else some (2, 1)
  else if b &&& 0xF0 = 0xE0 then
    if xs.length ≤ 2 then none
    else if notCont (g xs 1) then none
    else if notCont (g xs 2) then none
    else
      let cp := (b &&& 0xF) <<< 12 ||| (g xs 1 &&& 0x3F) <<< 6 ||| (g xs 2 &&& 0x3F)
      if cp < 0x800 ∨ (0xD7FF < cp ∧ cp < 0xE000) then none else some (3, 1)
  else if b &&& 0xF8 = 0xF0 then
    if xs.length ≤ 3 then none
    else if notCont (g xs 1) then none
    else if notCont (g xs 2) then none
    else if notCont (g xs 3) then none
    else
      let cp := (b &&& 0x7) <<< 18 ||| (g xs 1 &&& 0x3F) <<< 12 ||| (g xs 2 &&& 0x3F) <<< 6 |||
        (g xs 3 &&& 0x3F)
      if cp ≤ 0xFFFF ∨ 0x10FFFF < cp then none else some (4, 2)
  else none

/-- One step of `scalar::utf8_to_utf32::convert` and `convert_with_errors`
(include/simdutf/scalar/utf8_to_utf32/utf8_to_utf32.h:19-112 and :124-228): as `u8to16`, but
a 4-byte character writes one unit (:107, :218). -/
def u8to32 : Option (Nat × Nat) :=
  if 16 ≤ xs.length ∧ probe16 xs = true then some (16, 16) else
  let b := g xs 0
  if b < 0x80 then some (1, 1)
  else if b &&& 0xE0 = 0xC0 then
    if xs.length ≤ 1 then none
    else if notCont (g xs 1) then none
    else if ((b &&& 0x1F) <<< 6 ||| (g xs 1 &&& 0x3F)) < 0x80 then none
    else some (2, 1)
  else if b &&& 0xF0 = 0xE0 then
    if xs.length ≤ 2 then none
    else if notCont (g xs 1) then none
    else if notCont (g xs 2) then none
    else
      let cp := (b &&& 0xF) <<< 12 ||| (g xs 1 &&& 0x3F) <<< 6 ||| (g xs 2 &&& 0x3F)
      if cp < 0x800 ∨ (0xD7FF < cp ∧ cp < 0xE000) then none else some (3, 1)
  else if b &&& 0xF8 = 0xF0 then
    if xs.length ≤ 3 then none
    else if notCont (g xs 1) then none
    else if notCont (g xs 2) then none
    else if notCont (g xs 3) then none
    else
      let cp := (b &&& 0x7) <<< 18 ||| (g xs 1 &&& 0x3F) <<< 12 ||| (g xs 2 &&& 0x3F) <<< 6 |||
        (g xs 3 &&& 0x3F)
      if cp ≤ 0xFFFF ∨ 0x10FFFF < cp then none else some (4, 1)
  else none

/-- One step of `scalar::utf8_to_latin1::convert` and `convert_with_errors`
(include/simdutf/scalar/utf8_to_latin1/utf8_to_latin1.h:21-83 and :96-170): the 16-byte fast
path (:27-45, :102-120); ASCII (:50-53); a 2-byte character whose code point
is in `0x80 .. 0xFF` (:54-79, :128-155); anything else returns (:80-82,
:156-169). -/
def u8toL : Option (Nat × Nat) :=
  if 16 ≤ xs.length ∧ probe16 xs = true then some (16, 16) else
  let b := g xs 0
  if b < 0x80 then some (1, 1)
  else if b &&& 0xE0 = 0xC0 then
    if xs.length ≤ 1 then none
    else if notCont (g xs 1) then none
    else
      let cp := (b &&& 0x1F) <<< 6 ||| (g xs 1 &&& 0x3F)
      if cp < 0x80 ∨ 0xFF < cp then none else some (2, 1)
  else none

/-- One step of `scalar::utf8_to_utf16::convert_valid` and
`scalar::utf8_to_utf32::convert_valid` (`four` is what a 4-byte character
writes: 2 and 1): include/simdutf/scalar/utf8_to_utf16/valid_utf8_to_utf16.h:19-102 and
include/simdutf/scalar/utf8_to_utf32/valid_utf8_to_utf32.h:19-76. The 8-byte fast path; no
check but the length (`break`, which returns what was written); a
continuation byte or a byte `≥ 0xF8` returns. -/
def u8valid (four : Nat) : Option (Nat × Nat) :=
  if 8 ≤ xs.length ∧ probe8 xs = true then some (8, 8) else
  let b := g xs 0
  if b < 0x80 then some (1, 1)
  else if b &&& 0xE0 = 0xC0 then
    if xs.length ≤ 1 then none else some (2, 1)
  else if b &&& 0xF0 = 0xE0 then
    if xs.length ≤ 2 then none else some (3, 1)
  else if b &&& 0xF8 = 0xF0 then
    if xs.length ≤ 3 then none else some (4, four)
  else none

/-- One step of `scalar::utf8_to_latin1::convert_valid`
(include/simdutf/scalar/utf8_to_latin1/valid_utf8_to_latin1.h:21-80): the 16-byte fast path;
ASCII; a 2-byte lead followed by a continuation byte; anything else returns. -/
def u8validL : Option (Nat × Nat) :=
  if 16 ≤ xs.length ∧ probe16 xs = true then some (16, 16) else
  let b := g xs 0
  if b < 0x80 then some (1, 1)
  else if b &&& 0xE0 = 0xC0 then
    if xs.length ≤ 1 then none
    else if notCont (g xs 1) then none
    else some (2, 1)
  else none
end utf8

/-- What every UTF-8 step does: the 16- or 8-byte fast path on ASCII bytes,
or a leading byte of one of the four classes, consumed with what follows it,
writing at most that byte's weight. -/
def LeadStep (wt : Nat → Nat) (xs : List Nat) (c w : Nat) : Prop :=
  (c ≤ xs.length ∧ w = c ∧ ∀ j, j < c → xs.getD j 0 < 128) ∨
  (1 ≤ c ∧ w ≤ wt (xs.getD 0 0))

theorem leadStep_le (wt : Nat → Nat) (hw : ∀ b, b < 128 → wt b = 1) (x : Nat) (r : List Nat)
    (c w : Nat) (h : LeadStep wt (x :: r) c w) : w ≤ L (((x :: r).map wt).take c) := by
  rcases h with ⟨hc, rfl, ha⟩ | ⟨hc, hw'⟩
  · exact fast_le wt hw w _ hc ha
  · have := head_le wt x r c hc
    simp at hw'
    omega

/-- The UTF-8 steps are `LeadStep`s for both weights. -/
theorem u8to16_lead (xs : List Nat) (hb : Bytes xs) (c w : Nat) (h : u8to16 xs = some (c, w)) :
    LeadStep w16 xs c w := by
  have hb0 : xs.getD 0 0 < 256 := by
    rcases xs with _ | ⟨x, r⟩
    · simp
    · simpa using hb x (by simp)
  have lw := lead_w ⟨_, hb0⟩
  simp only at lw
  unfold u8to16 at h
  split at h
  · rename_i hp
    simp at h; obtain ⟨rfl, rfl⟩ := h
    exact Or.inl ⟨hp.1, rfl, probe16_ascii xs hb hp.1 hp.2⟩
  · simp only at h
    repeat' (first | (split at h) | (simp at h; done))
    all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; right)
    all_goals (simp_all; try omega)

theorem u8to32_lead (xs : List Nat) (hb : Bytes xs) (c w : Nat) (h : u8to32 xs = some (c, w)) :
    LeadStep w01 xs c w := by
  have hb0 : xs.getD 0 0 < 256 := by
    rcases xs with _ | ⟨x, r⟩
    · simp
    · simpa using hb x (by simp)
  have lw := lead_w ⟨_, hb0⟩
  simp only at lw
  unfold u8to32 at h
  split at h
  · rename_i hp
    simp at h; obtain ⟨rfl, rfl⟩ := h
    exact Or.inl ⟨hp.1, rfl, probe16_ascii xs hb hp.1 hp.2⟩
  · simp only at h
    repeat' (first | (split at h) | (simp at h; done))
    all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; right)
    all_goals (simp_all; try omega)

theorem u8toL_lead (xs : List Nat) (hb : Bytes xs) (c w : Nat) (h : u8toL xs = some (c, w)) :
    LeadStep w01 xs c w := by
  have hb0 : xs.getD 0 0 < 256 := by
    rcases xs with _ | ⟨x, r⟩
    · simp
    · simpa using hb x (by simp)
  have lw := lead_w ⟨_, hb0⟩
  simp only at lw
  unfold u8toL at h
  split at h
  · rename_i hp
    simp at h; obtain ⟨rfl, rfl⟩ := h
    exact Or.inl ⟨hp.1, rfl, probe16_ascii xs hb hp.1 hp.2⟩
  · simp only at h
    repeat' (first | (split at h) | (simp at h; done))
    all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; right)
    all_goals (simp_all; try omega)

theorem u8valid16_lead (xs : List Nat) (hb : Bytes xs) (c w : Nat) (h : u8valid xs 2 = some (c, w)) :
    LeadStep w16 xs c w := by
  have hb0 : xs.getD 0 0 < 256 := by
    rcases xs with _ | ⟨x, r⟩
    · simp
    · simpa using hb x (by simp)
  have lw := lead_w ⟨_, hb0⟩
  simp only at lw
  unfold u8valid at h
  split at h
  · rename_i hp
    simp at h; obtain ⟨rfl, rfl⟩ := h
    exact Or.inl ⟨hp.1, rfl, probe8_ascii xs hb hp.1 hp.2⟩
  · simp only at h
    repeat' (first | (split at h) | (simp at h; done))
    all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; right)
    all_goals (simp_all; try omega)

theorem u8valid32_lead (xs : List Nat) (hb : Bytes xs) (c w : Nat) (h : u8valid xs 1 = some (c, w)) :
    LeadStep w01 xs c w := by
  have hb0 : xs.getD 0 0 < 256 := by
    rcases xs with _ | ⟨x, r⟩
    · simp
    · simpa using hb x (by simp)
  have lw := lead_w ⟨_, hb0⟩
  simp only at lw
  unfold u8valid at h
  split at h
  · rename_i hp
    simp at h; obtain ⟨rfl, rfl⟩ := h
    exact Or.inl ⟨hp.1, rfl, probe8_ascii xs hb hp.1 hp.2⟩
  · simp only at h
    repeat' (first | (split at h) | (simp at h; done))
    all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; right)
    all_goals (simp_all; try omega)

theorem u8validL_lead (xs : List Nat) (hb : Bytes xs) (c w : Nat) (h : u8validL xs = some (c, w)) :
    LeadStep w01 xs c w := by
  have hb0 : xs.getD 0 0 < 256 := by
    rcases xs with _ | ⟨x, r⟩
    · simp
    · simpa using hb x (by simp)
  have lw := lead_w ⟨_, hb0⟩
  simp only at lw
  unfold u8validL at h
  split at h
  · rename_i hp
    simp at h; obtain ⟨rfl, rfl⟩ := h
    exact Or.inl ⟨hp.1, rfl, probe16_ascii xs hb hp.1 hp.2⟩
  · simp only at h
    repeat' (first | (split at h) | (simp at h; done))
    all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; right)
    all_goals (simp_all; try omega)

/-- Bytes stay bytes, and the UTF-8 loops stay within the length function. -/
theorem lead_run (wt : Nat → Nat) (hw : ∀ b, b < 128 → wt b = 1) (step : List Nat → Option (Nat × Nat))
    (hs : ∀ xs, Bytes xs → ∀ c w, step xs = some (c, w) → LeadStep wt xs c w) :
    ∀ f xs, Bytes xs → run step f xs ≤ L (xs.map wt) :=
  run_le wt Bytes step fun x r c w hb h =>
    ⟨leadStep_le wt hw x r c w (hs _ hb c w h), hb.drop c⟩

/-- **`scalar::utf8_to_utf16::convert` and `convert_with_errors` write at most
`utf16_length_from_utf8` of their input**, on every byte string. -/
theorem utf8to16_le (f : Nat) (xs : List Nat) (hb : Bytes xs) : run u8to16 f xs ≤ L (xs.map w16) :=
  lead_run w16 (fun b h => (ascii_w b h).1) u8to16 u8to16_lead f xs hb

/-- **`scalar::utf8_to_utf32::convert` and `convert_with_errors`**, within
`utf32_length_from_utf8`. -/
theorem utf8to32_le (f : Nat) (xs : List Nat) (hb : Bytes xs) : run u8to32 f xs ≤ L (xs.map w01) :=
  lead_run w01 (fun b h => (ascii_w b h).2) u8to32 u8to32_lead f xs hb

/-- **`scalar::utf8_to_latin1::convert` and `convert_with_errors`**, within
`latin1_length_from_utf8`. -/
theorem utf8toL_le (f : Nat) (xs : List Nat) (hb : Bytes xs) : run u8toL f xs ≤ L (xs.map w01) :=
  lead_run w01 (fun b h => (ascii_w b h).2) u8toL u8toL_lead f xs hb

/-- **`scalar::utf8_to_utf16::convert_valid`, on every byte string** (not only
the valid input of its contract), within `utf16_length_from_utf8`. -/
theorem valid8to16_le (f : Nat) (xs : List Nat) (hb : Bytes xs) :
    run (fun ys => u8valid ys 2) f xs ≤ L (xs.map w16) :=
  lead_run w16 (fun b h => (ascii_w b h).1) (fun ys => u8valid ys 2) u8valid16_lead f xs hb

/-- **`scalar::utf8_to_utf32::convert_valid`, on every byte string.** -/
theorem valid8to32_le (f : Nat) (xs : List Nat) (hb : Bytes xs) :
    run (fun ys => u8valid ys 1) f xs ≤ L (xs.map w01) :=
  lead_run w01 (fun b h => (ascii_w b h).2) (fun ys => u8valid ys 1) u8valid32_lead f xs hb

/-- **`scalar::utf8_to_latin1::convert_valid`, on every byte string.** -/
theorem valid8toL_le (f : Nat) (xs : List Nat) (hb : Bytes xs) :
    run u8validL f xs ≤ L (xs.map w01) :=
  lead_run w01 (fun b h => (ascii_w b h).2) u8validL u8validL_lead f xs hb

/-! ## `rewind_and_convert_with_errors`

include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:295-342,
include/simdutf/scalar/utf8_to_utf32/utf8_to_utf32.h:247-293 and
include/simdutf/scalar/utf8_to_latin1/utf8_to_latin1.h:174-220. `pre` are the
`prior_bytes` bytes before `buf`, `xs` the bytes from `buf` on. The loop
(utf8_to_utf16.h:308-321) looks at `buf[-i]` for `i = 0 .. how_far_back`
(`prior_bytes`; `min(3, prior_bytes)` in utf8_to_utf32.h:252-255) and stops
at the first byte that is not a continuation byte: if `i > 0` and that byte
is ASCII it returns without writing (:312-316); otherwise it converts from
there (:317-319, :337). If there is none it returns without writing
(:330-336). -/

/-- `buf[-i]`. -/
def back (pre xs : List Nat) (i : Nat) : Nat :=
  if i = 0 then xs.getD 0 0 else pre.getD (pre.length - i) 0

/-- The search, from `i`, over `k` more positions. -/
def seek (bk : Nat → Nat) : Nat → Nat → Option Nat
  | _, 0 => none
  | i, k + 1 => if bk i &&& 0xC0 ≠ 0x80 then some i else seek bk (i + 1) k

theorem seek_range (bk : Nat → Nat) : ∀ k i j, seek bk i k = some j → i ≤ j ∧ j < i + k := by
  intro k
  induction k with
  | zero => intro i j h; simp [seek] at h
  | succ k ih =>
    intro i j h
    simp only [seek] at h
    split at h
    · simp at h; omega
    · have := ih (i + 1) j h; omega

theorem cont_ge' : ∀ b : Fin 256, b.val &&& 0xC0 = 0x80 → 128 ≤ b.val := by decide +kernel

theorem cont_ge (b : Nat) (hb : b < 256) (h : b &&& 0xC0 = 0x80) : 128 ≤ b := cont_ge' ⟨b, hb⟩ h

/-- What `rewind_and_convert_with_errors` writes, `conv` being what
`convert_with_errors` writes. -/
def rewind (conv : List Nat → Nat) (hfb : Nat) (pre xs : List Nat) : Nat :=
  match seek (back pre xs) 0 (hfb + 1) with
  | none => 0
  | some i => if 0 < i ∧ back pre xs i < 128 then 0 else conv (pre.drop (pre.length - i) ++ xs)

/-- **The rewind writes at most `L` of the bytes from the leading byte it
found**, `i ≤ how_far_back` bytes before the cursor. -/
theorem rewind_le (wt : Nat → Nat) (conv : List Nat → Nat)
    (hconv : ∀ ys, Bytes ys → conv ys ≤ L (ys.map wt)) (hfb : Nat) (pre xs : List Nat)
    (hb : Bytes (pre ++ xs)) :
    ∃ i, i ≤ hfb ∧ rewind conv hfb pre xs ≤ L ((pre.drop (pre.length - i) ++ xs).map wt) := by
  unfold rewind
  split
  · exact ⟨0, Nat.zero_le _, Nat.zero_le _⟩
  · rename_i i h
    have := seek_range _ _ _ _ h
    refine ⟨i, by omega, ?_⟩
    split
    · exact Nat.zero_le _
    · apply hconv
      intro b hbm
      simp only [List.mem_append] at hbm
      rcases hbm with hbm | hbm
      · exact hb b (List.mem_append_left _ (List.mem_of_mem_drop hbm))
      · exact hb b (List.mem_append_right _ hbm)

/-- **With the cursor at most `L` of the bytes before that leading byte, the
whole conversion stays within `L` of the input.** At a block head the
decoders keep the cursor at most `L` of the consumed bytes
(`Utf8.reach_lag`, `Exact.xreach_exact`); `rewind_rest` is the case where
the rewind does not move. -/
theorem rewind_total (wt : Nat → Nat) (conv : List Nat → Nat)
    (hconv : ∀ ys, Bytes ys → conv ys ≤ L (ys.map wt)) (hfb : Nat) (pre xs : List Nat)
    (hb : Bytes (pre ++ xs)) (o : Nat)
    (ho : ∀ i, i ≤ hfb → o ≤ L ((pre.take (pre.length - i)).map wt)) :
    o + rewind conv hfb pre xs ≤ L ((pre ++ xs).map wt) := by
  obtain ⟨i, hi, h⟩ := rewind_le wt conv hconv hfb pre xs hb
  have h1 := ho i hi
  have h2 := L_map_take_add_drop wt (pre.length - i) pre
  rw [List.map_append, L_append] at h ⊢
  rw [List.map_take] at h1
  omega

/-- **The rewind does not move** when the byte at the cursor is not a
continuation byte (it converts from the cursor), or when the byte before the
cursor is ASCII (it returns without writing): then it writes at most `L` of
the bytes from the cursor. -/
theorem rewind_rest (wt : Nat → Nat) (conv : List Nat → Nat)
    (hconv : ∀ ys, Bytes ys → conv ys ≤ L (ys.map wt)) (hfb : Nat) (pre xs : List Nat)
    (hb : Bytes xs)
    (hc : xs.getD 0 0 &&& 0xC0 ≠ 0x80 ∨ (1 ≤ hfb ∧ 1 ≤ pre.length ∧ pre.getD (pre.length - 1) 0 < 128)) :
    rewind conv hfb pre xs ≤ L (xs.map wt) := by
  have b0 : back pre xs 0 = xs.getD 0 0 := by simp [back]
  have b1 : back pre xs 1 = pre.getD (pre.length - 1) 0 := by simp [back]
  have at0 : xs.getD 0 0 &&& 0xC0 ≠ 0x80 → rewind conv hfb pre xs ≤ L (xs.map wt) := by
    intro hc
    have : seek (back pre xs) 0 (hfb + 1) = some 0 := by
      simp only [seek]; rw [b0, ite_eq_left hc]
    unfold rewind; rw [this]
    simp only [Nat.lt_irrefl, false_and, ite_false, Nat.sub_zero, List.drop_length, List.nil_append]
    exact hconv xs hb
  rcases hc with hc | ⟨h1, h2, h3⟩
  · exact at0 hc
  · by_cases hc : xs.getD 0 0 &&& 0xC0 ≠ 0x80
    · exact at0 hc
    · have h128 : pre.getD (pre.length - 1) 0 &&& 0xC0 ≠ 0x80 := by
        intro heq
        have := cont_ge _ (by omega) heq
        omega
      obtain ⟨k, rfl⟩ : ∃ k, hfb = k + 1 := ⟨hfb - 1, by omega⟩
      have : seek (back pre xs) 0 (k + 1 + 1) = some 1 := by
        simp only [seek]; rw [b0, ite_eq_right hc, b1, ite_eq_left h128]
      unfold rewind; rw [this]
      dsimp only
      rw [b1, ite_eq_left ⟨Nat.one_pos, h3⟩]
      exact Nat.zero_le _

/-! ## UTF-16 sources

`utf8_length_from_utf16` weighs a unit `Arch.Icelake.w8` (1, 2 below 0x800
or for a surrogate, else 3), `utf32_length_from_utf16` weighs it
`IcelakeReal.w32v` (0 for a low surrogate, else 1), `latin1_length_from_utf16`
weighs it 1. `U16`: every unit is below `2^16`. -/

open SafetyMargins.IcelakeReal (w32v loS)

abbrev w8 : Nat → Nat := Arch.Icelake.w8

def U16 (xs : List Nat) : Prop := ∀ u ∈ xs, u < 65536

theorem U16.drop {xs : List Nat} (h : U16 xs) (n : Nat) : U16 (xs.drop n) :=
  fun b hb => h b (List.mem_of_mem_drop hb)

/-- Valid UTF-16: no unpaired surrogate. -/
def Valid16 : List Nat → Prop
  | [] => True
  | [u] => u < 0xD800 ∨ (0xE000 ≤ u ∧ u < 65536)
  | u :: v :: r =>
    ((u < 0xD800 ∨ (0xE000 ≤ u ∧ u < 65536)) ∧ Valid16 (v :: r)) ∨
    (0xD800 ≤ u ∧ u < 0xDC00 ∧ 0xDC00 ≤ v ∧ v < 0xE000 ∧ Valid16 r)

theorem w8_ge (v : Nat) : 1 ≤ w8 v := by
  unfold w8 Arch.Icelake.w8; repeat' split
  all_goals omega

theorem mask16 (w k : Nat) (hw : w < 65536) (hk : k ≤ 16) :
    w &&& (65536 - 2 ^ k) = 2 ^ k * (w / 2 ^ k) := and16 w k hw hk

theorem m80 (w : Nat) (hw : w < 65536) : w &&& 0xFF80 = 128 * (w / 128) := by
  simpa using mask16 w 7 hw (by omega)

theorem m800 (w : Nat) (hw : w < 65536) : w &&& 0xF800 = 2048 * (w / 2048) := by
  simpa using mask16 w 11 hw (by omega)

theorem m100 (w : Nat) (hw : w < 65536) : w &&& 0xFF00 = 256 * (w / 256) := by
  simpa using mask16 w 8 hw (by omega)

/-- The 64-bit word of four UTF-16 units in memory, on a little-endian host
(every target of this study): a unit `u` stored in the input's byte order is
the bytes `u % 256, u / 256`, or, for the other byte order (`be`), the bytes
`u / 256, u % 256`. -/
def mem16 (be : Bool) : List Nat → List Nat
  | [] => []
  | u :: r => (if be then [u / 256, u % 256] else [u % 256, u / 256]) ++ mem16 be r

/-- `(v >> 8) | (v << (64 - 8))` on a `uint64_t`. -/
def rotr8 (v : Nat) : Nat := (v >>> 8) ||| ((v <<< 56) % 2 ^ 64)

/-- The 4-unit ASCII test of include/simdutf/scalar/utf16_to_utf8/valid_utf16_to_utf8.h:26-33
(and of utf16_to_utf8.h:26-33): the word read by `memcpy`, rotated when
`!match_system(big_endian)` (:30-32), masked with `0xFF80FF80FF80FF80`. -/
def probeU16 (be : Bool) (us : List Nat) : Bool :=
  let v := le64 ((mem16 be us).take 8)
  (if be then rotr8 v else v) &&& 0xFF80FF80FF80FF80 == 0

theorem le64_lt : ∀ bs : List Nat, Bytes bs → le64 bs < 256 ^ bs.length := by
  intro bs
  induction bs with
  | nil => intro _; simp [le64]
  | cons b r ih =>
    intro hb
    have hb0 : b < 256 := hb b (by simp)
    have := ih (fun x hx => hb x (by simp [hx]))
    simp only [le64, List.length_cons, Nat.pow_succ]
    have : 256 * le64 r + b < 256 * 256 ^ r.length := by
      have : le64 r + 1 ≤ 256 ^ r.length := this
      have := Nat.mul_le_mul_left 256 this
      omega
    omega

theorem le64_append : ∀ xs ys : List Nat, le64 (xs ++ ys) = le64 xs + 256 ^ xs.length * le64 ys := by
  intro xs ys
  induction xs with
  | nil => simp [le64]
  | cons x r ih => simp only [List.cons_append, le64, ih, List.length_cons, Nat.pow_succ]; rw [Nat.mul_add,
      ← Nat.mul_assoc, Nat.mul_comm 256 (256 ^ r.length)]; omega

theorem rotr8_le64 (b : Nat) (bs : List Nat) (hb : b < 256) (hbs : Bytes bs) (hl : bs.length = 7) :
    rotr8 (le64 (b :: bs)) = le64 (bs ++ [b]) := by
  have hlt := le64_lt bs hbs
  rw [hl] at hlt
  unfold rotr8
  simp only [le64]
  have h1 : (b + 256 * le64 bs) >>> 8 = le64 bs := by
    rw [Nat.shiftRight_eq_div_pow]; omega
  have h2 : ((b + 256 * le64 bs) <<< 56) % 2 ^ 64 = 2 ^ 56 * b := by
    rw [Nat.shiftLeft_eq]
    have : (b + 256 * le64 bs) * 2 ^ 56 = 2 ^ 56 * b + 2 ^ 64 * le64 bs := by
      rw [Nat.add_mul, show 2 ^ 64 = 256 * 2 ^ 56 by decide]; rw [Nat.mul_comm b, Nat.mul_assoc,
        Nat.mul_comm (le64 bs), Nat.mul_assoc]
    rw [this, Nat.add_mul_mod_self_left, Nat.mod_eq_of_lt]
    have : 2 ^ 56 * b < 2 ^ 56 * 256 := Nat.mul_lt_mul_of_pos_left hb (by decide)
    omega
  rw [h1, h2, Nat.or_comm, ← Nat.two_pow_add_eq_or_of_lt (by simpa using hlt), le64_append, hl]
  simp [le64]
  omega

theorem lo_hi (u : Nat) (hu : u < 65536) (h1 : u % 256 &&& 0x80 = 0) (h2 : u / 256 &&& 0xFF = 0) :
    u < 128 := by
  have a := byte_and80 ⟨u % 256, by omega⟩
  simp only at a
  rw [a] at h1
  have : u / 256 &&& 0xFF = u / 256 :=
    Nat.and_two_pow_sub_one_of_lt_two_pow (n := 8) (by show u / 256 < 256; omega)
  omega

/-- **The 4-unit test holds only on four units below `0x80`, in both byte
orders.** -/
theorem probeU16_ascii (be : Bool) (us : List Nat) (hu : U16 us) (hl : 4 ≤ us.length)
    (h : probeU16 be us = true) : ∀ j, j < 4 → us.getD j 0 < 128 := by
  obtain ⟨u0, u1, u2, u3, r, rfl⟩ : ∃ u0 u1 u2 u3 r, us = u0 :: u1 :: u2 :: u3 :: r := by
    rcases us with _ | ⟨u0, _ | ⟨u1, _ | ⟨u2, _ | ⟨u3, r⟩⟩⟩⟩ <;> simp at hl
    exact ⟨_, _, _, _, _, rfl⟩
  have h0 : u0 < 65536 := hu u0 (by simp)
  have h1 : u1 < 65536 := hu u1 (by simp)
  have h2 : u2 < 65536 := hu u2 (by simp)
  have h3 : u3 < 65536 := hu u3 (by simp)
  have hM : (0xFF80FF80FF80FF80 : Nat) = le64 [0x80, 0xFF, 0x80, 0xFF, 0x80, 0xFF, 0x80, 0xFF] := by decide
  have hB : ∀ l : List Nat, (∀ x ∈ l, x < 256) → Bytes l := fun l h => h
  intro j hj
  cases be with
  | false =>
    simp only [probeU16, mem16, beq_iff_eq, ite_false, Bool.false_eq_true, List.cons_append, List.nil_append,
      List.take_succ_cons, List.take_zero, reduceIte] at h
    rw [hM, le64_and_gen _ _ (by simp) (hB _ (by simp; omega)) (hB _ (by simp))] at h
    have e := fun i hi => h i hi
    have a0 := e 0 (by simp); have a1 := e 1 (by simp); have a2 := e 2 (by simp)
    have a3 := e 3 (by simp); have a4 := e 4 (by simp); have a5 := e 5 (by simp)
    have a6 := e 6 (by simp); have a7 := e 7 (by simp)
    simp only [List.getD_cons_zero, List.getD_cons_succ] at a0 a1 a2 a3 a4 a5 a6 a7
    have := lo_hi u0 h0 a0 a1; have := lo_hi u1 h1 a2 a3
    have := lo_hi u2 h2 a4 a5; have := lo_hi u3 h3 a6 a7
    rcases j with _ | _ | _ | _ | j <;> simp <;> omega
  | true =>
    simp only [probeU16, mem16, beq_iff_eq, ite_true, Bool.false_eq_true, List.cons_append, List.nil_append,
      List.take_succ_cons, List.take_zero, reduceIte] at h
    rw [rotr8_le64 _ _ (by omega) (hB _ (by simp; omega)) (by simp)] at h
    rw [hM, le64_and_gen _ _ (by simp) (hB _ (by simp; omega)) (hB _ (by simp))] at h
    have e := fun i hi => h i hi
    have a0 := e 0 (by simp); have a1 := e 1 (by simp); have a2 := e 2 (by simp)
    have a3 := e 3 (by simp); have a4 := e 4 (by simp); have a5 := e 5 (by simp)
    have a6 := e 6 (by simp); have a7 := e 7 (by simp)
    simp only [List.getD_cons_zero, List.getD_cons_succ, List.cons_append, List.nil_append] at a0 a1 a2 a3 a4 a5 a6 a7
    have := lo_hi u0 h0 a0 a7; have := lo_hi u1 h1 a2 a1
    have := lo_hi u2 h2 a4 a3; have := lo_hi u3 h3 a6 a5
    rcases j with _ | _ | _ | _ | j <;> simp <;> omega

section utf16
variable (pr : List Nat → Bool) (xs : List Nat)

/-- One step of `scalar::utf16_to_utf8::convert` and of `convert_with_errors`
with `check_output = false` (include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:20-82 and
:103-180): the 4-unit ASCII fast path (:26-41, :111-127), here with any test
`pr` (each unit weighs at least 1, so what the test says does not matter);
one, two or three bytes for a unit outside the surrogates (:45-66,
:130-158); a surrogate returns unless it is a high surrogate followed by a
low one, which write four bytes (:67-82, :159-180). `uint16_t(word - 0xD800)`
is `(word + 0x10000 - 0xD800) % 0x10000`. -/
def u16to8 : Option (Nat × Nat) :=
  if 4 ≤ xs.length ∧ pr xs = true then some (4, 4) else
  let w := g xs 0
  if w &&& 0xFF80 = 0 then some (1, 1)
  else if w &&& 0xF800 = 0 then some (1, 2)
  else if w &&& 0xF800 ≠ 0xD800 then some (1, 3)
  else if xs.length ≤ 1 then none
  else if 0x3FF < (w + 0x10000 - 0xD800) % 0x10000 then none
  else if 0x3FF < (g xs 1 + 0x10000 - 0xDC00) % 0x10000 then none
  else some (2, 4)

/-- One step of `scalar::utf16_to_utf8::convert_valid`
(include/simdutf/scalar/utf16_to_utf8/valid_utf16_to_utf8.h:20-84): the fast path with its
real test (`probeU16`, :26-43), and a surrogate converted with the next unit,
unchecked (:65-83). -/
def u16to8valid (be : Bool) : Option (Nat × Nat) :=
  if 4 ≤ xs.length ∧ probeU16 be xs = true then some (4, 4) else
  let w := g xs 0
  if w &&& 0xFF80 = 0 then some (1, 1)
  else if w &&& 0xF800 = 0 then some (1, 2)
  else if w &&& 0xF800 ≠ 0xD800 then some (1, 3)
  else if xs.length ≤ 1 then none
  else some (2, 4)

/-- One step of `scalar::utf16_to_utf32::convert` and `convert_with_errors`
(include/simdutf/scalar/utf16_to_utf32/utf16_to_utf32.h:14-41 and :50-77): a unit outside
the surrogates writes one unit (:17-20); a surrogate returns unless it is a
high surrogate (:23-26) followed by a low one (:27-36), which write one unit. -/
def u16to32 : Option (Nat × Nat) :=
  let w := g xs 0
  if w &&& 0xF800 ≠ 0xD800 then some (1, 1)
  else if 0x3FF < (w + 0x10000 - 0xD800) % 0x10000 then none
  else if xs.length ≤ 1 then none
  else if 0x3FF < (g xs 1 + 0x10000 - 0xDC00) % 0x10000 then none
  else some (2, 1)

/-- One step of `scalar::utf16_to_utf32::convert_valid`
(include/simdutf/scalar/utf16_to_utf32/valid_utf16_to_utf32.h:14-35): a surrogate is converted
with the next unit, unchecked. -/
def u16to32valid : Option (Nat × Nat) :=
  let w := g xs 0
  if w &&& 0xF800 ≠ 0xD800 then some (1, 1)
  else if xs.length ≤ 1 then none
  else some (2, 1)

/-- One step of `scalar::utf16_to_latin1::convert_with_errors`
(include/simdutf/scalar/utf16_to_latin1/utf16_to_latin1.h:53-99): the 16-unit fast path
(:58-88), any test; a unit below 0x100 writes one byte (:92-95); else return. -/
def u16toL : Option (Nat × Nat) :=
  if 16 ≤ xs.length ∧ pr xs = true then some (16, 16)
  else if g xs 0 &&& 0xFF00 = 0 then some (1, 1) else none
end utf16

/-- `scalar::utf16_to_latin1::convert` (include/simdutf/scalar/utf16_to_latin1/utf16_to_latin1.h:26-31) and
`convert_valid` (include/simdutf/scalar/utf16_to_latin1/valid_utf16_to_latin1.h:20-24) write one byte
per unit, whatever it is, then return. -/
def oneEach (_ : List Nat) : Option (Nat × Nat) := some (1, 1)

theorem u16to8_le (pr : List Nat → Bool) (x : Nat) (r : List Nat) (c w : Nat) (hu : U16 (x :: r))
    (h : u16to8 pr (x :: r) = some (c, w)) : w ≤ L (((x :: r).map w8).take c) := by
  have hx : x < 65536 := hu x (by simp)
  unfold u16to8 at h
  split at h
  · rename_i hp
    simp at h; obtain ⟨rfl, rfl⟩ := h
    exact pos_le w8 w8_ge 4 _ hp.1
  · simp only [List.getD_cons_zero, m80 x hx, m800 x hx] at h
    have hw := w8_ge x
    repeat' (first | (split at h) | (simp at h; done))
    all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
    all_goals simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil]
    all_goals (try (unfold w8 Arch.Icelake.w8; repeat' split) <;> omega)
    -- the pair
    rcases r with _ | ⟨y, r⟩
    · simp at *
    · have hy : y < 65536 := hu y (by simp)
      simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil, List.getD_cons_succ,
        List.getD_cons_zero] at *
      unfold w8 Arch.Icelake.w8; repeat' split
      all_goals omega

theorem Valid16.u16 : ∀ xs, Valid16 xs → U16 xs := by
  intro xs
  induction xs using Valid16.induct with
  | case1 => intro _ u hu; simp at hu
  | case2 u => intro h v hv; simp at hv; subst hv; simp [Valid16] at h; omega
  | case3 u v r ih1 ih2 =>
    intro h x hx
    simp only [Valid16] at h
    rcases h with ⟨h1, h2⟩ | ⟨h1, h2, h3, h4, h5⟩
    · simp at hx; rcases hx with rfl | hx
      · omega
      · exact ih1 h2 x (by simpa using hx)
    · simp at hx; rcases hx with rfl | rfl | hx
      · omega
      · omega
      · exact ih2 h5 x hx

/-- A unit outside the surrogates is followed by valid UTF-16. -/
theorem Valid16.tail (u : Nat) (r : List Nat) (h : Valid16 (u :: r)) (hu : u < 0xD800 ∨ 0xE000 ≤ u) :
    Valid16 r := by
  rcases r with _ | ⟨v, r⟩
  · trivial
  · simp only [Valid16] at h
    rcases h with ⟨_, h⟩ | ⟨h1, _⟩
    · exact h
    · omega

/-- The head of valid UTF-16 is not a low surrogate. -/
theorem Valid16.head (u : Nat) (r : List Nat) (h : Valid16 (u :: r)) :
    u < 0xDC00 ∨ 0xE000 ≤ u := by
  rcases r with _ | ⟨v, r⟩
  · simp only [Valid16] at h; omega
  · simp only [Valid16] at h; omega

/-- A high surrogate at the head of valid UTF-16 is followed by a low one, then valid UTF-16. -/
theorem Valid16.pair (u : Nat) (r : List Nat) (h : Valid16 (u :: r)) (hu : 0xD800 ≤ u ∧ u < 0xE000) :
    ∃ v r', r = v :: r' ∧ 0xD800 ≤ u ∧ u < 0xDC00 ∧ 0xDC00 ≤ v ∧ v < 0xE000 ∧ Valid16 r' := by
  rcases r with _ | ⟨v, r⟩
  · simp only [Valid16] at h; omega
  · simp only [Valid16] at h
    rcases h with ⟨h1, _⟩ | ⟨h1, h2, h3, h4, h5⟩
    · omega
    · exact ⟨v, r, rfl, h1, h2, h3, h4, h5⟩

theorem Valid16.drop_ascii (k : Nat) : ∀ xs, Valid16 xs → k ≤ xs.length →
    (∀ j, j < k → xs.getD j 0 < 128) → Valid16 (xs.drop k) := by
  induction k with
  | zero => intro xs h _ _; simpa using h
  | succ k ih =>
    intro xs h hl ha
    rcases xs with _ | ⟨u, r⟩
    · simp at hl
    · have hu := ha 0 (by omega)
      simp at hu
      have := Valid16.tail u r h (by omega)
      simpa using ih r this (by simp at hl; omega) (fun j hj => by simpa using ha (j + 1) (by omega))

theorem u16to8valid_le (be : Bool) (x : Nat) (r : List Nat) (c w : Nat) (hv : Valid16 (x :: r))
    (h : u16to8valid (x :: r) be = some (c, w)) :
    w ≤ L (((x :: r).map w8).take c) ∧ Valid16 ((x :: r).drop c) := by
  have hu := Valid16.u16 _ hv
  have hx : x < 65536 := hu x (by simp)
  unfold u16to8valid at h
  split at h
  · rename_i hp
    simp at h; obtain ⟨rfl, rfl⟩ := h
    exact ⟨pos_le w8 w8_ge 4 _ hp.1, Valid16.drop_ascii 4 _ hv hp.1 (probeU16_ascii be _ hu hp.1 hp.2)⟩
  · simp only [List.getD_cons_zero, m80 x hx, m800 x hx] at h
    have hw := w8_ge x
    have h0 := Valid16.head x r hv
    repeat' (first | (split at h) | (simp at h; done))
    all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
    all_goals simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil,
      List.drop_succ_cons, List.drop_zero]
    · exact ⟨by omega, Valid16.tail x r hv (by omega)⟩
    · exact ⟨by unfold w8 Arch.Icelake.w8; repeat' split
                all_goals omega, Valid16.tail x r hv (by omega)⟩
    · exact ⟨by unfold w8 Arch.Icelake.w8; repeat' split
                all_goals omega, Valid16.tail x r hv (by omega)⟩
    · obtain ⟨v, r', rfl, h1, h2, h3, h4, h5⟩ := Valid16.pair x r hv (by omega)
      refine ⟨?_, by simpa using h5⟩
      simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil]
      unfold w8 Arch.Icelake.w8; repeat' split
      all_goals omega

theorem u16to32_le (x : Nat) (r : List Nat) (c w : Nat) (hu : U16 (x :: r))
    (h : u16to32 (x :: r) = some (c, w)) : w ≤ L (((x :: r).map w32v).take c) := by
  have hx : x < 65536 := hu x (by simp)
  unfold u16to32 at h
  simp only [List.getD_cons_zero, m800 x hx] at h
  repeat' (first | (split at h) | (simp at h; done))
  all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals rcases r with _ | ⟨y, r⟩
  all_goals simp only [List.map_cons, List.map_nil, List.take_succ_cons, List.take_zero, List.take_nil,
    L_cons, L_nil, List.getD_cons_succ, List.getD_cons_zero, List.length_cons, List.length_nil] at *
  all_goals (unfold w32v loS; simp only [decide_eq_true_eq]; repeat' split)
  all_goals omega

theorem u16to32valid_le (x : Nat) (r : List Nat) (c w : Nat) (hv : Valid16 (x :: r))
    (h : u16to32valid (x :: r) = some (c, w)) :
    w ≤ L (((x :: r).map w32v).take c) ∧ Valid16 ((x :: r).drop c) := by
  have hu := Valid16.u16 _ hv
  have hx : x < 65536 := hu x (by simp)
  have h0 := Valid16.head x r hv
  unfold u16to32valid at h
  simp only [List.getD_cons_zero, m800 x hx] at h
  repeat' (first | (split at h) | (simp at h; done))
  all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
  · refine ⟨?_, by simpa using Valid16.tail x r hv (by omega)⟩
    simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil]
    unfold w32v loS; simp only [decide_eq_true_eq]; repeat' split
    all_goals omega
  · obtain ⟨v, r', rfl, h1, h2, h3, h4, h5⟩ := Valid16.pair x r hv (by omega)
    refine ⟨?_, by simpa using h5⟩
    simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil]
    unfold w32v loS; simp only [decide_eq_true_eq]; repeat' split
    all_goals omega

theorem u16toL_le (pr : List Nat → Bool) (x : Nat) (r : List Nat) (c w : Nat)
    (h : u16toL pr (x :: r) = some (c, w)) : w ≤ L (((x :: r).map fun _ => 1).take c) := by
  unfold u16toL at h
  split at h
  · rename_i hp; simp at h; obtain ⟨rfl, rfl⟩ := h; exact pos_le _ (fun _ => Nat.le_refl 1) 16 _ hp.1
  · split at h
    · simp at h; obtain ⟨rfl, rfl⟩ := h; simp
    · simp at h

theorem oneEach_le (x : Nat) (r : List Nat) (c w : Nat) (h : oneEach (x :: r) = some (c, w)) :
    w ≤ L (((x :: r).map fun _ => 1).take c) := by
  simp [oneEach] at h; obtain ⟨rfl, rfl⟩ := h; simp

/-- **`scalar::utf16_to_utf8::convert` and `convert_with_errors` write at most
`utf8_length_from_utf16` of their input**, on every UTF-16 string, whatever
the fast-path test. -/
theorem utf16to8_le (pr : List Nat → Bool) (f : Nat) (xs : List Nat) (hu : U16 xs) :
    run (u16to8 pr) f xs ≤ L (xs.map w8) :=
  run_le w8 U16 _ (fun x r c w hu h => ⟨u16to8_le pr x r c w hu h, hu.drop c⟩) f xs hu

/-- **`scalar::utf16_to_utf8::convert_valid`, on valid UTF-16** (its
contract), in both byte orders. -/
theorem valid16to8_le (be : Bool) (f : Nat) (xs : List Nat) (hv : Valid16 xs) :
    run (fun ys => u16to8valid ys be) f xs ≤ L (xs.map w8) :=
  run_le w8 Valid16 _ (fun x r c w hv h => u16to8valid_le be x r c w hv h) f xs hv

/-- **`scalar::utf16_to_utf32::convert` and `convert_with_errors`**, within
`utf32_length_from_utf16`, on every UTF-16 string. -/
theorem utf16to32_le (f : Nat) (xs : List Nat) (hu : U16 xs) :
    run u16to32 f xs ≤ L (xs.map w32v) :=
  run_le w32v U16 _ (fun x r c w hu h => ⟨u16to32_le x r c w hu h, hu.drop c⟩) f xs hu

/-- **`scalar::utf16_to_utf32::convert_valid`, on valid UTF-16.** -/
theorem valid16to32_le (f : Nat) (xs : List Nat) (hv : Valid16 xs) :
    run u16to32valid f xs ≤ L (xs.map w32v) :=
  run_le w32v Valid16 _ (fun x r c w hv h => u16to32valid_le x r c w hv h) f xs hv

/-- **`scalar::utf16_to_latin1::convert_with_errors`**, within
`latin1_length_from_utf16`; `convert` and `convert_valid` (`oneEach`) too. -/
theorem utf16toL_le (pr : List Nat → Bool) (f : Nat) (xs : List Nat) :
    run (u16toL pr) f xs ≤ L (xs.map fun _ => 1) :=
  run_le' _ _ (u16toL_le pr) f xs

theorem oneEach_run (f : Nat) (xs : List Nat) : run oneEach f xs ≤ L (xs.map fun _ => 1) :=
  run_le' _ _ oneEach_le f xs

/-- `convert_valid` on invalid UTF-16 can write more than
`utf8_length_from_utf16`: a high surrogate followed by an ASCII unit writes 4
bytes where the length function counts 3. The contract excludes it. -/
theorem valid_16to8_over : run (fun ys => u16to8valid ys false) 2 [0xD800, 0x41] = 4 ∧
    L ([0xD800, 0x41].map w8) = 3 := by decide

/-! ## UTF-32 and Latin-1 sources

`utf8_length_from_utf32` weighs a unit `RvvExact.w8of32` (1, plus one above
0x7F, 0x7FF and 0xFFFF), `utf16_length_from_utf32` weighs it
`RvvExact.w16of32` (2 above 0xFFFF, else 1), `latin1_length_from_utf32` 1;
`utf8_length_from_latin1` weighs a byte `RvvExact.wL8` (2 from 0x80, else 1),
`utf16`/`utf32_length_from_latin1` 1. Every weight is at least 1, so the fast
paths are safe whatever their tests say (`pr`). -/

open SafetyMargins.RvvExact (w8of32 w16of32 wL8 ind)

def U32 (xs : List Nat) : Prop := ∀ u ∈ xs, u < 2 ^ 32

theorem U32.drop {xs : List Nat} (h : U32 xs) (n : Nat) : U32 (xs.drop n) :=
  fun b hb => h b (List.mem_of_mem_drop hb)

theorem m32 (w k : Nat) (hw : w < 2 ^ 32) (hk : k ≤ 32) :
    w &&& (2 ^ 32 - 2 ^ k) = 2 ^ k * (w / 2 ^ k) := and32 w k hw hk

section utf32
variable (pr : List Nat → Bool) (xs : List Nat)

/-- One step of `scalar::utf32_to_utf8::convert` and `convert_with_errors`
(include/simdutf/scalar/utf32_to_utf8/utf32_to_utf8.h:20-71 and :84-135): the 2-unit fast path
(:24-36, :88-100), any test; 1, 2 or 3 bytes by `word & 0xFFFFFF80`,
`0xFFFFF800`, `0xFFFF0000`, a surrogate returning (:38-58, :102-122); else
four bytes, or return above 0x10FFFF (:59-70, :123-134). -/
def u32to8 : Option (Nat × Nat) :=
  if 2 ≤ xs.length ∧ pr xs = true then some (2, 2) else
  let w := g xs 0
  if w &&& 0xFFFFFF80 = 0 then some (1, 1)
  else if w &&& 0xFFFFF800 = 0 then some (1, 2)
  else if w &&& 0xFFFF0000 = 0 then
    if 0xD800 ≤ w ∧ w ≤ 0xDFFF then none else some (1, 3)
  else if 0x10FFFF < w then none else some (1, 4)

/-- One step of `scalar::utf32_to_utf8::convert_valid`
(include/simdutf/scalar/utf32_to_utf8/valid_utf32_to_utf8.h:20-65): no check. -/
def u32to8valid : Option (Nat × Nat) :=
  if 2 ≤ xs.length ∧ pr xs = true then some (2, 2) else
  let w := g xs 0
  if w &&& 0xFFFFFF80 = 0 then some (1, 1)
  else if w &&& 0xFFFFF800 = 0 then some (1, 2)
  else if w &&& 0xFFFF0000 = 0 then some (1, 3)
  else some (1, 4)

/-- One step of `scalar::utf32_to_utf16::convert` and `convert_with_errors`
(include/simdutf/scalar/utf32_to_utf16/utf32_to_utf16.h:14-40 and :49-75): a BMP unit writes one
unit, a surrogate returns (:16-23); above, two units, or return above
0x10FFFF (:24-38). -/
def u32to16 : Option (Nat × Nat) :=
  let w := g xs 0
  if w &&& 0xFFFF0000 = 0 then
    if 0xD800 ≤ w ∧ w ≤ 0xDFFF then none else some (1, 1)
  else if 0x10FFFF < w then none else some (1, 2)

/-- One step of `scalar::utf32_to_utf16::convert_valid`
(include/simdutf/scalar/utf32_to_utf16/valid_utf32_to_utf16.h:14-35): no check. -/
def u32to16valid : Option (Nat × Nat) :=
  if g xs 0 &&& 0xFFFF0000 = 0 then some (1, 1) else some (1, 2)

/-- One step of `scalar::utf32_to_latin1::convert_with_errors`
(include/simdutf/scalar/utf32_to_latin1/utf32_to_latin1.h:35-61): the 2-unit fast path (:39-51),
any test; one byte below 0x100 (:53-57); else return. -/
def u32toL : Option (Nat × Nat) :=
  if 2 ≤ xs.length ∧ pr xs = true then some (2, 2)
  else if g xs 0 &&& 0xFFFFFF00 = 0 then some (1, 1) else none

/-- One step of `scalar::utf32_to_latin1::convert_valid`
(include/simdutf/scalar/utf32_to_latin1/valid_utf32_to_latin1.h:21-54): with two units left,
the fast path or return (:30-43); else one byte below 0x100, or return
(:47-52). -/
def u32toLvalid : Option (Nat × Nat) :=
  if 2 ≤ xs.length then (if pr xs = true then some (2, 2) else none)
  else if g xs 0 &&& 0xFFFFFF00 = 0 then some (1, 1) else none

/-- One step of `scalar::latin1_to_utf8::convert`
(include/simdutf/scalar/latin1_to_utf8/latin1_to_utf8.h:22-61): the 16-byte fast path
(:28-47), any test; one byte for ASCII, two otherwise (:50-60). -/
def l1to8 : Option (Nat × Nat) :=
  if 16 ≤ xs.length ∧ pr xs = true then some (16, 16)
  else if g xs 0 &&& 0x80 = 0 then some (1, 1) else some (1, 2)
end utf32

theorem w8of32_cases (w : Nat) :
    (w < 0x80 → w8of32 w = 1) ∧ (0x80 ≤ w → w < 0x800 → w8of32 w = 2) ∧
    (0x800 ≤ w → w < 0x10000 → w8of32 w = 3) ∧ (0x10000 ≤ w → w8of32 w = 4) := by
  unfold w8of32 ind; refine ⟨?_, ?_, ?_, ?_⟩ <;> intros <;> simp <;> repeat' split
  all_goals omega

theorem w8of32_ge (w : Nat) : 1 ≤ w8of32 w := by unfold w8of32; omega

theorem w16of32_ge (w : Nat) : 1 ≤ w16of32 w := by unfold w16of32; omega

theorem wL8_ge (w : Nat) : 1 ≤ wL8 w := by unfold wL8; omega

theorem u32to8_le (pr : List Nat → Bool) (valid : Bool) (x : Nat) (r : List Nat) (c w : Nat)
    (hu : U32 (x :: r))
    (h : (if valid then u32to8valid pr (x :: r) else u32to8 pr (x :: r)) = some (c, w)) :
    w ≤ L (((x :: r).map w8of32).take c) := by
  have hx : x < 2 ^ 32 := hu x (by simp)
  have e7 : x &&& 0xFFFFFF80 = 128 * (x / 128) := by simpa using m32 x 7 hx (by omega)
  have e11 : x &&& 0xFFFFF800 = 2048 * (x / 2048) := by simpa using m32 x 11 hx (by omega)
  have e16 : x &&& 0xFFFF0000 = 65536 * (x / 65536) := by simpa using m32 x 16 hx (by omega)
  have wc := w8of32_cases x
  cases valid <;> simp only [ite_true, ite_false, Bool.false_eq_true] at h
  all_goals (first | unfold u32to8 at h | unfold u32to8valid at h)
  all_goals split at h
  all_goals (try (rename_i hp; simp at h; obtain ⟨rfl, rfl⟩ := h; exact pos_le _ w8of32_ge 2 _ hp.1))
  all_goals simp only [List.getD_cons_zero, e7, e11, e16] at h
  all_goals repeat' (first | (split at h) | (simp at h; done))
  all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil]
  all_goals omega

theorem u32to16_le (valid : Bool) (x : Nat) (r : List Nat) (c w : Nat) (hu : U32 (x :: r))
    (h : (if valid then u32to16valid (x :: r) else u32to16 (x :: r)) = some (c, w)) :
    w ≤ L (((x :: r).map w16of32).take c) := by
  have hx : x < 2 ^ 32 := hu x (by simp)
  have e16 : x &&& 0xFFFF0000 = 65536 * (x / 65536) := by simpa using m32 x 16 hx (by omega)
  cases valid <;> simp only [ite_true, ite_false, Bool.false_eq_true] at h
  all_goals (first | unfold u32to16 at h | unfold u32to16valid at h)
  all_goals simp only [List.getD_cons_zero, e16] at h
  all_goals repeat' (first | (split at h) | (simp at h; done))
  all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil]
  all_goals (unfold w16of32 ind; simp only [decide_eq_true_eq]; repeat' split)
  all_goals omega

theorem u32toL_le (pr : List Nat → Bool) (valid : Bool) (x : Nat) (r : List Nat) (c w : Nat)
    (h : (if valid then u32toLvalid pr (x :: r) else u32toL pr (x :: r)) = some (c, w)) :
    w ≤ L (((x :: r).map fun _ => 1).take c) := by
  cases valid <;> simp only [ite_true, ite_false, Bool.false_eq_true] at h
  all_goals (first | unfold u32toL at h | unfold u32toLvalid at h)
  all_goals repeat' (first | (split at h) | (simp at h; done))
  all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals (first | (simp; done) | exact pos_le _ (fun _ => Nat.le_refl 1) 2 _ (by assumption) |
    exact pos_le _ (fun _ => Nat.le_refl 1) 2 _ (by simp_all))

theorem l1to8_le (pr : List Nat → Bool) (x : Nat) (r : List Nat) (c w : Nat) (hb : Bytes (x :: r))
    (h : l1to8 pr (x :: r) = some (c, w)) : w ≤ L (((x :: r).map wL8).take c) := by
  have hx : x < 256 := hb x (by simp)
  unfold l1to8 at h
  split at h
  · rename_i hp; simp at h; obtain ⟨rfl, rfl⟩ := h; exact pos_le _ wL8_ge 16 _ hp.1
  · simp only [List.getD_cons_zero] at h
    have a := byte_and80 ⟨x, hx⟩
    simp only at a
    split at h <;> (simp at h; obtain ⟨rfl, rfl⟩ := h)
    · simpa using wL8_ge x
    · rename_i hn
      rw [a] at hn
      simp only [List.map_cons, List.take_succ_cons, List.take_zero, L_cons, L_nil]
      unfold wL8 ind; simp only [decide_eq_true_eq]; split <;> omega

/-- **`scalar::utf32_to_utf8::convert`, `convert_with_errors` and
`convert_valid`**, within `utf8_length_from_utf32`, on every input. -/
theorem utf32to8_le (pr : List Nat → Bool) (valid : Bool) (f : Nat) (xs : List Nat) (hu : U32 xs) :
    run (fun ys => if valid then u32to8valid pr ys else u32to8 pr ys) f xs ≤ L (xs.map w8of32) :=
  run_le w8of32 U32 _ (fun x r c w hu h => ⟨u32to8_le pr valid x r c w hu h, hu.drop c⟩) f xs hu

/-- **`scalar::utf32_to_utf16::convert`, `convert_with_errors` and
`convert_valid`**, within `utf16_length_from_utf32`, on every input. -/
theorem utf32to16_le (valid : Bool) (f : Nat) (xs : List Nat) (hu : U32 xs) :
    run (fun ys => if valid then u32to16valid ys else u32to16 ys) f xs ≤ L (xs.map w16of32) :=
  run_le w16of32 U32 _ (fun x r c w hu h => ⟨u32to16_le valid x r c w hu h, hu.drop c⟩) f xs hu

/-- **`scalar::utf32_to_latin1::convert_with_errors` and `convert_valid`**
(`convert` writes one byte per unit: `oneEach_run`). -/
theorem utf32toL_le (pr : List Nat → Bool) (valid : Bool) (f : Nat) (xs : List Nat) :
    run (fun ys => if valid then u32toLvalid pr ys else u32toL pr ys) f xs ≤ L (xs.map fun _ => 1) :=
  run_le' _ _ (u32toL_le pr valid) f xs

/-- **`scalar::latin1_to_utf8::convert`**, within `utf8_length_from_latin1`
(`latin1_to_utf16` and `latin1_to_utf32` write one unit per byte:
`oneEach_run`). -/
theorem latin1to8_le (pr : List Nat → Bool) (f : Nat) (xs : List Nat) (hb : Bytes xs) :
    run (l1to8 pr) f xs ≤ L (xs.map wL8) :=
  run_le wL8 Bytes _ (fun x r c w hb h => ⟨l1to8_le pr x r c w hb h, hb.drop c⟩) f xs hb

/-! ## The scalar fallbacks inside the vector loops

When a register holds a surrogate, an invalid unit or a unit Latin-1 cannot
represent, several kernels convert a few units with an inline scalar loop, and
the path tables list that iteration as `Kernels.scalar k`: it consumes between
1 and `k` units (or returns after at most `k`) and has no vector store, so
`Basic.written` charges it `L` of what it consumed. `runK` is such a loop,
`for (; k < forward; k++)`, with a step that may also consume the next unit
(`k++` inside the body). `fallback_scalar` proves the charge and the
membership in `Kernels.scalar`.

The loops, by step:
* `fbA` (UTF-16 → UTF-8, `forward = 15`, the pair test `(diff | diff2) > 0x3FF`):
  src/westmere/sse_convert_utf16_to_utf8.cpp:233-266 and :460-495,
  src/haswell/avx2_convert_utf16_to_utf8.cpp:282-315 and :559-594,
  src/arm64/arm_convert_utf16_to_utf8.cpp:269-303 and :536-571,
  src/lsx/lsx_convert_utf16_to_utf8.cpp:239-273 and :473-508,
  src/lasx/lasx_convert_utf16_to_utf8.cpp:255-289 and :505-540,
  src/ppc64/ppc64_convert_utf16_to_utf8.cpp:255-289.
* `fbB` (UTF-16 → UTF-32, `forward = 15`):
  src/westmere/sse_convert_utf16_to_utf32.cpp:97-120 and :177-202,
  src/haswell/avx2_convert_utf16_to_utf32.cpp:98-122 and :180-206,
  src/arm64/arm_convert_utf16_to_utf32.cpp:85-109 and :155-180,
  src/lsx/lsx_convert_utf16_to_utf32.cpp:34-58 and :103-128,
  src/lasx/lasx_convert_utf16_to_utf32.cpp:59-83 and :153-178,
  src/ppc64/ppc64_convert_utf16_to_utf32.cpp:58-82.
* `u32to8` with no fast path (UTF-32 → UTF-8, `forward = 15`):
  src/westmere/sse_convert_utf32_to_utf8.cpp:282-311 and :550-581,
  src/haswell/avx2_convert_utf32_to_utf8.cpp:234-263 and :529-560,
  src/icelake/icelake_convert_utf32_to_utf8.inl.cpp:238-267 and :534-565,
  src/arm64/arm_convert_utf32_to_utf8.cpp:208-239 and :462-495,
  src/lsx/lsx_convert_utf32_to_utf8.cpp:184-215 and :416-449,
  src/lasx/lasx_convert_utf32_to_utf8.cpp:249-280 and :546-579; and
  src/ppc64/ppc64_convert_utf32_to_utf8.cpp:119-149, whose checks are skipped
  without error reporting (`u32to8valid`).
* `u32to16` (UTF-32 → UTF-16; `forward = 7`, 15 or 3):
  src/haswell/avx2_convert_utf32_to_utf16.cpp:35-69 and :118-154,
  src/westmere/sse_convert_utf32_to_utf16.cpp:169-205,
  src/lasx/lasx_convert_utf32_to_utf16.cpp:64-98 and :176-212,
  src/lsx/lsx_convert_utf32_to_utf16.cpp:31-65 and :113-149,
  src/ppc64/ppc64_convert_utf32_to_utf16.cpp:73-105; and the arm64 loop over
  the units before the first invalid one (`u32to16valid`, `pos ≤ 8`):
  src/arm64/arm_convert_utf32_to_utf16.cpp:160-180.
* `fbL` (UTF-16 or UTF-32 → Latin-1, `k < 8`, 16 or 32, one byte per unit
  `<= 0xff`, else return):
  src/westmere/sse_convert_utf16_to_latin1.cpp:56-65,
  src/westmere/sse_convert_utf32_to_latin1.cpp:58-67,
  src/haswell/avx2_convert_utf16_to_latin1.cpp:68-78,
  src/haswell/avx2_convert_utf32_to_latin1.cpp:81-90,
  src/arm64/arm_convert_utf16_to_latin1.cpp:48-57,
  src/arm64/arm_convert_utf32_to_latin1.cpp:47-56,
  src/lsx/lsx_convert_utf16_to_latin1.cpp:51-60,
  src/lsx/lsx_convert_utf32_to_latin1.cpp:53-62,
  src/lasx/lasx_convert_utf16_to_latin1.cpp:51-60,
  src/lasx/lasx_convert_utf32_to_latin1.cpp:60-69.
-/

/-- `for (k = 0; k < lim; k++)` over a step: (units consumed, units written,
returned early). It stops at the end of `xs`; the C++ loop tests guarantee the
units it reads. -/
def runK (step : List Nat → Option (Nat × Nat)) : Nat → Nat → List Nat → Nat × Nat × Bool
  | 0, _, _ => (0, 0, false)
  | _ + 1, _, [] => (0, 0, false)
  | f + 1, lim, x :: r =>
    if lim = 0 then (0, 0, false) else
    match step (x :: r) with
    | none => (0, 0, true)
    | some (c, w) =>
      let q := runK step f (lim - c) ((x :: r).drop c)
      (c + q.1, w + q.2.1, q.2.2)

theorem runK_le (wt : Nat → Nat) (P : List Nat → Prop) (step : List Nat → Option (Nat × Nat))
    (hs : ∀ x r c w, P (x :: r) → step (x :: r) = some (c, w) →
      w ≤ L (((x :: r).map wt).take c) ∧ P ((x :: r).drop c)) :
    ∀ f lim xs, P xs → (runK step f lim xs).2.1 ≤ L ((xs.map wt).take (runK step f lim xs).1) := by
  intro f
  induction f with
  | zero => intro lim xs _; simp [runK]
  | succ f ih =>
    intro lim xs hp
    rcases xs with _ | ⟨x, r⟩
    · simp [runK]
    · simp only [runK]
      split
      · simp
      · split
        · simp
        · rename_i c w h
          obtain ⟨h1, h2⟩ := hs x r c w hp h
          have h3 := ih (lim - c) _ h2
          simp only
          rw [List.take_add, L_append, ← List.map_drop]
          omega

theorem runK_len (step : List Nat → Option (Nat × Nat)) (m : Nat)
    (hc : ∀ x r c w, step (x :: r) = some (c, w) → 1 ≤ c ∧ c ≤ m) :
    ∀ f lim xs, (runK step f lim xs).1 ≤ lim + (m - 1) ∧ (lim = 0 → (runK step f lim xs).1 = 0) := by
  intro f
  induction f with
  | zero => intro lim xs; simp [runK]
  | succ f ih =>
    intro lim xs
    rcases xs with _ | ⟨x, r⟩
    · simp [runK]
    · simp only [runK]
      split
      · simp
      · rename_i hl
        split
        · simp
        · rename_i c w h
          have := hc x r c w h
          have := ih (lim - c) ((x :: r).drop c)
          simp only
          refine ⟨?_, by omega⟩
          by_cases h0 : lim - c = 0
          · have := this.2 h0; omega
          · omega

theorem runK_pos (step : List Nat → Option (Nat × Nat))
    (hc : ∀ x r c w, step (x :: r) = some (c, w) → 1 ≤ c) (f lim : Nat) (xs : List Nat)
    (hf : 1 ≤ f) (hl : 1 ≤ lim) (hx : xs ≠ []) (hs : (runK step f lim xs).2.2 = false) :
    1 ≤ (runK step f lim xs).1 := by
  obtain ⟨f, rfl⟩ : ∃ f', f = f' + 1 := ⟨f - 1, by omega⟩
  rcases xs with _ | ⟨x, r⟩
  · exact absurd rfl hx
  · simp only [runK] at hs ⊢
    rw [ite_eq_right (by omega)] at hs ⊢
    revert hs
    split
    · simp
    · rename_i c w h
      intro _
      have := hc x r c w h
      simp only
      omega

/-- `Kernels.scalar k` holds every iteration that consumes `1 .. k` units, and
every early return after at most `k`. -/
theorem mem_scalar (c k : Nat) (s : Bool)
    (h : (s = false ∧ 1 ≤ c ∧ c ≤ k) ∨ (s = true ∧ c ≤ k)) :
    ({ consumed := c, stores := [], stop := s } : Iter) ∈ Kernels.scalar k := by
  unfold Kernels.scalar
  rw [List.mem_append, List.mem_map, List.mem_map]
  rcases h with ⟨rfl, h1, h2⟩ | ⟨rfl, h1⟩
  · exact Or.inl ⟨c - 1, List.mem_range.mpr (by omega), by simp; omega⟩
  · exact Or.inr ⟨c, List.mem_range.mpr (by omega), rfl⟩

/-- **A scalar fallback is an iteration of `Kernels.scalar` that writes at
most `L` of what it consumes.** `m` is the most a step consumes; `lim`
the loop bound (`forward`); the loop runs on at least one unit. -/
theorem fallback_scalar (wt : Nat → Nat) (P : List Nat → Prop) (step : List Nat → Option (Nat × Nat))
    (m : Nat)
    (hs : ∀ x r c w, P (x :: r) → step (x :: r) = some (c, w) →
      w ≤ L (((x :: r).map wt).take c) ∧ P ((x :: r).drop c))
    (hc : ∀ x r c w, step (x :: r) = some (c, w) → 1 ≤ c ∧ c ≤ m)
    (lim : Nat) (xs : List Nat) (hp : P xs) (hl : 1 ≤ lim) (hx : xs ≠ []) :
    let q := runK step lim lim xs
    q.2.1 ≤ L ((xs.map wt).take q.1) ∧
      ({ consumed := q.1, stores := [], stop := q.2.2 } : Iter) ∈ Kernels.scalar (lim + (m - 1)) := by
  intro q
  refine ⟨runK_le wt P step hs lim lim xs hp, mem_scalar _ _ _ ?_⟩
  have hlen := (runK_len step m hc lim lim xs).1
  cases hq : q.2.2
  · exact Or.inl ⟨rfl, runK_pos step (fun x r c w h => (hc x r c w h).1) lim lim xs hl hl hx hq, hlen⟩
  · exact Or.inr ⟨rfl, hlen⟩

/-- `Kernels.scalar` grows with `k`. -/
theorem scalar_mono (j k : Nat) (h : j ≤ k) (it : Iter) (hm : it ∈ Kernels.scalar j) :
    it ∈ Kernels.scalar k := by
  unfold Kernels.scalar at *
  rw [List.mem_append, List.mem_map, List.mem_map] at *
  rcases hm with ⟨c, hc, rfl⟩ | ⟨c, hc, rfl⟩
  · exact Or.inl ⟨c, List.mem_range.mpr (by simp at hc; omega), rfl⟩
  · exact Or.inr ⟨c, List.mem_range.mpr (by simp at hc; omega), rfl⟩

section fb
variable (xs : List Nat)

/-- The UTF-16 → UTF-8 fallback step: one, two or three bytes outside the
surrogates; a surrogate reads the next unit (`k++`) and returns unless
`(diff | diff2) <= 0x3FF`, else four bytes. -/
def fbA : Option (Nat × Nat) :=
  let w := g xs 0
  if w &&& 0xFF80 = 0 then some (1, 1)
  else if w &&& 0xF800 = 0 then some (1, 2)
  else if w &&& 0xF800 ≠ 0xD800 then some (1, 3)
  else if 0x3FF < ((w + 0x10000 - 0xD800) % 0x10000 ||| (g xs 1 + 0x10000 - 0xDC00) % 0x10000)
    then none
  else some (2, 4)

/-- The UTF-16 → UTF-32 fallback step. -/
def fbB : Option (Nat × Nat) :=
  let w := g xs 0
  if w &&& 0xF800 ≠ 0xD800 then some (1, 1)
  else if 0x3FF < ((w + 0x10000 - 0xD800) % 0x10000 ||| (g xs 1 + 0x10000 - 0xDC00) % 0x10000)
    then none
  else some (2, 1)

/-- The Latin-1 fallback step. -/
def fbL : Option (Nat × Nat) := if g xs 0 ≤ 0xFF then some (1, 1) else none
end fb

/-- `(diff | diff2) > 0x3FF` is `diff > 0x3FF || diff2 > 0x3FF`. -/
theorem or_gt (a b : Nat) : (0x3FF < (a ||| b)) = (0x3FF < a ∨ 0x3FF < b) := by
  apply propext
  constructor
  · intro h
    apply Classical.byContradiction
    intro hn
    have : a ||| b < 2 ^ 10 := Nat.or_lt_two_pow (by omega) (by omega)
    omega
  · intro h
    rcases h with h | h
    · exact Nat.lt_of_lt_of_le h (Nat.left_le_or (m := b))
    · exact Nat.lt_of_lt_of_le h (Nat.right_le_or (n := a))

theorem fbA_le (x : Nat) (r : List Nat) (c w : Nat) (hu : U16 (x :: r))
    (h : fbA (x :: r) = some (c, w)) :
    (w ≤ L (((x :: r).map w8).take c) ∧ U16 ((x :: r).drop c)) ∧ (1 ≤ c ∧ c ≤ 2) := by
  have hx : x < 65536 := hu x (by simp)
  refine ⟨⟨?_, hu.drop c⟩, ?_⟩
  all_goals unfold fbA at h
  all_goals simp only [List.getD_cons_zero, m80 x hx, m800 x hx, or_gt] at h
  all_goals repeat' (first | (split at h) | (simp at h; done))
  all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals (try omega)
  all_goals rcases r with _ | ⟨y, r⟩
  all_goals simp only [List.map_cons, List.map_nil, List.take_succ_cons, List.take_zero, List.take_nil,
    L_cons, L_nil, List.getD_cons_succ, List.getD_cons_zero, List.getD_nil] at *
  all_goals (try have hy : y < 65536 := hu y (by simp))
  all_goals (unfold w8 Arch.Icelake.w8; repeat' split)
  all_goals omega

theorem fbB_le (x : Nat) (r : List Nat) (c w : Nat) (hu : U16 (x :: r))
    (h : fbB (x :: r) = some (c, w)) :
    (w ≤ L (((x :: r).map w32v).take c) ∧ U16 ((x :: r).drop c)) ∧ (1 ≤ c ∧ c ≤ 2) := by
  have hx : x < 65536 := hu x (by simp)
  refine ⟨⟨?_, hu.drop c⟩, ?_⟩
  all_goals unfold fbB at h
  all_goals simp only [List.getD_cons_zero, m800 x hx, or_gt] at h
  all_goals repeat' (first | (split at h) | (simp at h; done))
  all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals (try omega)
  all_goals rcases r with _ | ⟨y, r⟩
  all_goals simp only [List.map_cons, List.map_nil, List.take_succ_cons, List.take_zero, List.take_nil,
    L_cons, L_nil, List.getD_cons_succ, List.getD_cons_zero, List.getD_nil] at *
  all_goals (unfold w32v loS; simp only [decide_eq_true_eq]; repeat' split)
  all_goals omega

/-- **The UTF-16 → UTF-8 fallbacks** (`fbA`, `forward = 15`): an iteration of
`Kernels.scalar 16` writing at most `utf8_length_from_utf16` of what it
consumed, on every input. -/
theorem fbA_scalar (xs : List Nat) (hu : U16 xs) (hx : xs ≠ []) :
    let q := runK fbA 15 15 xs
    q.2.1 ≤ L ((xs.map w8).take q.1) ∧
      ({ consumed := q.1, stores := [], stop := q.2.2 } : Iter) ∈ Kernels.scalar 16 :=
  fallback_scalar w8 U16 fbA 2 (fun x r c w hu h => (fbA_le x r c w hu h).1)
    (fun x r c w h => by
      unfold fbA at h
      dsimp only at h
      repeat' (first | (split at h) | (simp at h; done))
      all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; omega))
    15 xs hu (by decide) hx

/-- **The UTF-16 → UTF-32 fallbacks** (`fbB`, `forward = 15`). -/
theorem fbB_scalar (xs : List Nat) (hu : U16 xs) (hx : xs ≠ []) :
    let q := runK fbB 15 15 xs
    q.2.1 ≤ L ((xs.map w32v).take q.1) ∧
      ({ consumed := q.1, stores := [], stop := q.2.2 } : Iter) ∈ Kernels.scalar 16 :=
  fallback_scalar w32v U16 fbB 2 (fun x r c w hu h => (fbB_le x r c w hu h).1)
    (fun x r c w h => by
      unfold fbB at h
      dsimp only at h
      repeat' (first | (split at h) | (simp at h; done))
      all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; omega))
    15 xs hu (by decide) hx

/-- **The UTF-32 → UTF-8 fallbacks** (`forward = 15`), with the checks
(`valid = false`) or without (ppc64 without error reporting). -/
theorem fbC_scalar (valid : Bool) (xs : List Nat) (hu : U32 xs) (hx : xs ≠ []) :
    let step := fun ys => if valid then u32to8valid (fun _ => false) ys else u32to8 (fun _ => false) ys
    let q := runK step 15 15 xs
    q.2.1 ≤ L ((xs.map w8of32).take q.1) ∧
      ({ consumed := q.1, stores := [], stop := q.2.2 } : Iter) ∈ Kernels.scalar 16 := by
  intro step q
  have := fallback_scalar w8of32 U32 step 2
    (fun x r c w hu h => ⟨u32to8_le (fun _ => false) valid x r c w hu h, hu.drop c⟩)
    (fun x r c w h => by
      simp only [step] at h
      cases valid <;> simp only [ite_true, ite_false, Bool.false_eq_true] at h
      all_goals (first | unfold u32to8 at h | unfold u32to8valid at h)
      all_goals (try dsimp only at h)
      all_goals repeat' (first | (split at h) | (simp at h; done))
      all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; omega))
    15 xs hu (by decide) hx
  exact this

/-- **The UTF-32 → UTF-16 fallbacks**: `forward` units at most (7, 15 or 3,
`k` the table's bound), with the checks, or without them (arm64, over the
units before the first invalid one, `pos ≤ 8`). -/
theorem fbD_scalar (valid : Bool) (fwd k : Nat) (xs : List Nat) (hu : U32 xs) (hx : xs ≠ [])
    (h1 : 1 ≤ fwd) (hk : fwd ≤ k) :
    let step := fun ys => if valid then u32to16valid ys else u32to16 ys
    let q := runK step fwd fwd xs
    q.2.1 ≤ L ((xs.map w16of32).take q.1) ∧
      ({ consumed := q.1, stores := [], stop := q.2.2 } : Iter) ∈ Kernels.scalar k := by
  intro step q
  have := fallback_scalar w16of32 U32 step 1
    (fun x r c w hu h => ⟨u32to16_le valid x r c w hu h, hu.drop c⟩)
    (fun x r c w h => by
      simp only [step] at h
      cases valid <;> simp only [ite_true, ite_false, Bool.false_eq_true] at h
      all_goals (first | unfold u32to16 at h | unfold u32to16valid at h)
      all_goals (try dsimp only at h)
      all_goals repeat' (first | (split at h) | (simp at h; done))
      all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h; omega))
    fwd xs hu h1 hx
  exact ⟨this.1, scalar_mono _ k (by omega) _ this.2⟩

/-- **The Latin-1 fallbacks** (`k < n`, n = 8, 16 or 32): an iteration of
`Kernels.scalar n` writing at most one byte per unit consumed. -/
theorem fbL_scalar (n : Nat) (xs : List Nat) (hx : xs ≠ []) (h1 : 1 ≤ n) :
    let q := runK fbL n n xs
    q.2.1 ≤ L ((xs.map fun _ => 1).take q.1) ∧
      ({ consumed := q.1, stores := [], stop := q.2.2 } : Iter) ∈ Kernels.scalar n := by
  intro q
  have := fallback_scalar (fun _ => 1) (fun _ => True) fbL 1
    (fun x r c w _ h => by
      unfold fbL at h; split at h
      · simp at h; obtain ⟨rfl, rfl⟩ := h; simp
      · simp at h)
    (fun x r c w h => by
      unfold fbL at h; split at h
      · simp at h; omega
      · simp at h)
    n xs trivial h1 hx
  simpa using this

/-! ## The tail charged by `Basic.written` -/

/-- `Basic.written` on the unit values `us`, with the scalar code that
finishes the conversion given as `T`: the iterations see the weights
`us.map wt`. -/
def writtenT (N : Nat) (choose : List Nat → Iter) (wt : Nat → Nat) (T : List Nat → Nat) :
    Nat → List Nat → Nat
  | 0, us => T us
  | n + 1, us =>
    if (us.map wt).length < N then T us
    else if (choose (us.map wt)).stop then
      max (maxEnd (us.map wt) (choose (us.map wt)).consumed (choose (us.map wt)).stores)
        (L ((us.map wt).take (choose (us.map wt)).consumed))
    else
      max (maxEnd (us.map wt) (choose (us.map wt)).consumed (choose (us.map wt)).stores)
        (L ((us.map wt).take (choose (us.map wt)).consumed) +
          writtenT N choose wt T n (us.drop (choose (us.map wt)).consumed))

/-- **A scalar tail within `L` of what it is given keeps the whole conversion
within `Basic.written`.** `Q` is what the tail needs of its input (bytes,
UTF-16 units, valid UTF-16), kept by dropping a prefix. -/
theorem writtenT_le (N : Nat) (choose : List Nat → Iter) (wt : Nat → Nat) (T : List Nat → Nat)
    (Q : List Nat → Prop) (hQ : ∀ us k, Q us → Q (us.drop k))
    (hT : ∀ us, Q us → T us ≤ L (us.map wt)) :
    ∀ n us, Q us → writtenT N choose wt T n us ≤ written N choose n (us.map wt) := by
  intro n
  induction n with
  | zero => intro us hq; simpa [writtenT, written] using hT us hq
  | succ n ih =>
    intro us hq
    simp only [writtenT, written]
    split
    · exact hT us hq
    · split
      · exact Nat.le_refl _
      · have := ih (us.drop (choose (us.map wt)).consumed) (hQ _ _ hq)
        rw [List.map_drop] at this
        exact Nat.max_le.mpr ⟨Nat.le_max_left _ _, Nat.le_trans (Nat.add_le_add_left this _) (Nat.le_max_right _ _)⟩

/-- **`safe_of_paths` with the scalar code**: a path table that fits, and a
scalar tail within `L`, keep every store of the conversion within the
buffer. -/
theorem safe_of_paths_tail (N : Nat) (paths : List Iter) (hp : paths.all (Iter.ok N) = true)
    (choose : List Nat → Iter) (hc : ∀ xs, N ≤ xs.length → choose xs ∈ paths)
    (wt : Nat → Nat) (T : List Nat → Nat) (Q : List Nat → Prop) (hQ : ∀ us k, Q us → Q (us.drop k))
    (hT : ∀ us, Q us → T us ≤ L (us.map wt)) (n : Nat) (us : List Nat) (hq : Q us)
    (hpos : Pos (us.map wt)) :
    writtenT N choose wt T n us ≤ L (us.map wt) :=
  Nat.le_trans (writtenT_le N choose wt T Q hQ hT n us hq)
    (safe_of_paths N paths hp choose hc n _ hpos)

/-- For example, the UTF-16 → UTF-8 kernels, finished by
`scalar::utf16_to_utf8::convert` on what they leave (any number of
iterations, any fast-path test). -/
example (N : Nat) (paths : List Iter) (hp : paths.all (Iter.ok N) = true)
    (choose : List Nat → Iter) (hc : ∀ xs, N ≤ xs.length → choose xs ∈ paths)
    (pr : List Nat → Bool) (n : Nat) (us : List Nat) (hu : U16 us) :
    writtenT N choose w8 (fun ys => run (u16to8 pr) ys.length ys) n us ≤ L (us.map w8) :=
  safe_of_paths_tail N paths hp choose hc w8 _ U16 (fun _ k h => h.drop k)
    (fun ys h => utf16to8_le pr _ ys h) n us hu (fun x hx => by
      simp only [List.mem_map] at hx; obtain ⟨u, _, rfl⟩ := hx; exact w8_ge u)

end SafetyMargins.Scalar

