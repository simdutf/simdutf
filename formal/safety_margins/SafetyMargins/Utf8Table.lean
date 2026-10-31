import SafetyMargins.Utf8
import SafetyMargins.TableFacts
/-
The per-call facts of `Utf8.Step`, derived from the extracted `utf8bigindex`
and `shufutf8` tables.

The kernels compute the 12-bit end-of-code-point mask of a call at byte `t`
(bit `k` set when byte `t + k + 1` is leading), look up `utf8bigindex`, consume
`bigConsumed` bytes and, on a row below 209, produce `units` code points
(`TableFacts.units`). `table_step` shows that such a call is a `Step`.
-/
namespace SafetyMargins.Utf8
open SafetyMargins SafetyMargins.Tables SafetyMargins.TableFacts

/-- 1 when byte `i` is leading. -/
def lead (xs : List Nat) (i : Nat) : Nat := if xs.getD i 0 = 0 then 0 else 1

/-- End-of-code-point mask of `n` bytes after `t`. -/
def emask (xs : List Nat) (t : Nat) : Nat → Nat
  | 0 => 0
  | n + 1 => lead xs (t + 1) + 2 * emask xs (t + 1) n

theorem lead_le (xs : List Nat) (i : Nat) : lead xs i ≤ 1 := by
  unfold lead; split <;> omega

theorem emask_lt (xs : List Nat) (t n : Nat) : emask xs t n < 2 ^ n := by
  induction n generalizing t with
  | zero => simp [emask]
  | succ n ih =>
    have := ih (t + 1)
    have := lead_le xs (t + 1)
    simp only [emask, Nat.pow_succ]
    omega

theorem bit_emask (xs : List Nat) (t n k : Nat) (hk : k < n) :
    bit (emask xs t n) k = lead xs (t + k + 1) := by
  induction n generalizing t k with
  | zero => omega
  | succ n ih =>
    have hl := lead_le xs (t + 1)
    unfold bit
    cases k with
    | zero =>
      simp only [emask, Nat.pow_zero, Nat.div_one, Nat.add_zero]
      omega
    | succ k =>
      have h := ih (t + 1) k (by omega)
      unfold bit at h
      have hdiv : (lead xs (t + 1) + 2 * emask xs (t + 1) n) / 2 = emask xs (t + 1) n := by
        omega
      simp only [emask, Nat.pow_succ, Nat.mul_comm (2 ^ k) 2, ← Nat.div_div_eq_div_mul, hdiv]
      rw [h]
      congr 1
      omega

/-- Sum of `f k` for `k < c`. -/
def sumUpTo (f : Nat → Nat) : Nat → Nat
  | 0 => 0
  | c + 1 => f c + sumUpTo f c

theorem popcount_eq (n c : Nat) (f : Nat → Nat) (h : ∀ k, k < c → bit n k = f k) :
    popcount c n = sumUpTo f c := by
  induction c with
  | zero => rfl
  | succ c ih =>
    simp only [popcount, sumUpTo]
    rw [h c (by omega), ih (fun k hk => h k (by omega))]

theorem L_take_one (xs : List Nat) (u : Nat) : L ((xs.drop u).take 1) = xs.getD u 0 := by
  induction xs generalizing u with
  | nil => simp
  | cons x xs ih => cases u <;> simp_all

theorem L_window (xs : List Nat) (s c : Nat) :
    L ((xs.drop s).take c) = sumUpTo (fun k => xs.getD (s + k) 0) c := by
  induction c with
  | zero => simp [sumUpTo]
  | succ c ih =>
    have : (xs.drop s).take (c + 1) = (xs.drop s).take c ++ (xs.drop (s + c)).take 1 := by
      rw [List.take_add, List.drop_drop]
    rw [this, L_append, ih, L_take_one, sumUpTo]
    omega

theorem sumUpTo_congr (f g : Nat → Nat) (c : Nat) (h : ∀ k, k < c → f k = g k) :
    sumUpTo f c = sumUpTo g c := by
  induction c with
  | zero => rfl
  | succ c ih =>
    simp only [sumUpTo]
    rw [h c (by omega), ih (fun k hk => h k (by omega))]

theorem lead_le_getD (xs : List Nat) (i : Nat) : lead xs i ≤ xs.getD i 0 := by
  unfold lead; split <;> omega

theorem sumUpTo_mono (f g : Nat → Nat) (c : Nat) (h : ∀ k, k < c → f k ≤ g k) :
    sumUpTo f c ≤ sumUpTo g c := by
  induction c with
  | zero => exact Nat.le_refl _
  | succ c ih =>
    simp only [sumUpTo]
    have := h c (by omega)
    have := ih (fun k hk => h k (by omega))
    omega

theorem sumUpTo_le (f : Nat → Nat) {a c : Nat} (h : a ≤ c) : sumUpTo f a ≤ sumUpTo f c := by
  induction c with
  | zero => have : a = 0 := by omega
            subst this; exact Nat.le_refl _
  | succ c ih =>
    by_cases hac : a = c + 1
    · subst hac; exact Nat.le_refl _
    · have := ih (by omega); simp only [sumUpTo]; omega

/-- The consumed bytes weigh at least their first byte plus the bytes at two
later offsets. -/
theorem sumUpTo_three (f : Nat → Nat) (s1 s2 c : Nat) (h1 : 0 < s1) (h12 : s1 < s2)
    (h2 : s2 < c) : f 0 + f s1 + f s2 ≤ sumUpTo f c := by
  have a := sumUpTo_le f (Nat.succ_le_of_lt h2)
  have b := sumUpTo_le f (Nat.succ_le_of_lt h12)
  have d := sumUpTo_le f (Nat.succ_le_of_lt h1)
  simp only [sumUpTo] at a b d
  have : sumUpTo f 0 = 0 := rfl
  omega

/-- Mask, row and consumed count of a call, and what the table says about them. -/
structure Row (xs : List Nat) (t : Nat) where
  m : Nat
  hm : m = emask xs t 12
  hb : ∀ k, k < 12 → bit m k = lead xs (t + k + 1)
  hmem : m ∈ List.range 4096

def row (xs : List Nat) (t : Nat) : Row xs t :=
  ⟨emask xs t 12, rfl, fun k hk => bit_emask xs t 12 k hk,
    List.mem_range.mpr (emask_lt xs t 12)⟩

/-- A non-error row: the call ends just before a leading byte, and the code
points it produces (`units`) number at most `L` of what it consumed plus `cont`. -/
theorem row_conv (xs : List Nat) (t : Nat) (r : Row xs t) (hi : bigIdx[r.m]! < 209) :
    1 ≤ bigConsumed[r.m]! ∧ bigConsumed[r.m]! ≤ 12 ∧
    xs.getD (t + bigConsumed[r.m]!) 0 ≠ 0 ∧
    units bigIdx[r.m]! ≤ L ((xs.drop t).take bigConsumed[r.m]!) + cont xs t := by
  have hrow := (List.all_eq_true.mp bigindex_exact) r.m r.hmem
  have hle := of_decide_eq_true ((List.all_eq_true.mp bigconsumed_le) r.m r.hmem)
  simp only [hi, ↓reduceIte, Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq] at hrow
  obtain ⟨⟨hc1, hbit⟩, hpop⟩ := hrow
  generalize bigConsumed[r.m]! = c at *
  have hb := r.hb
  have hnext : xs.getD (t + c) 0 ≠ 0 := by
    have := hb (c - 1) (by omega)
    rw [hbit, show t + (c - 1) + 1 = t + c by omega] at this
    unfold lead at this
    split at this <;> omega
  refine ⟨hc1, hle, hnext, ?_⟩
  obtain ⟨c', rfl⟩ : ∃ c', c = c' + 1 := ⟨c - 1, by omega⟩
  have hunits : units bigIdx[r.m]! ≤ 1 + L ((xs.drop (t + 1)).take c') := by
    rw [← hpop, popcount_eq r.m (c' + 1) (fun k => lead xs (t + 1 + k))
      (fun k hk => by rw [hb k (by omega), Nat.add_right_comm])]
    simp only [sumUpTo]
    have := lead_le xs (t + 1 + c')
    have := sumUpTo_mono (fun k => lead xs (t + 1 + k)) (fun k => xs.getD (t + 1 + k) 0) c'
      (fun k _ => lead_le_getD xs _)
    rw [L_window]
    omega
  have hsplit := L_take_add xs t 1
  have hsplit2 := L_take_add xs (t + 1) c'
  rw [L_take_one] at hsplit
  rw [show t + 1 + c' = t + (c' + 1) by omega, L_take_add xs t (c' + 1)] at hsplit2
  unfold cont
  split <;> omega

/-- A call driven by `utf8bigindex` whose output is one unit per code point
(UTF-32, Latin-1, and the UTF-16 rows below 145, which hold no 4-byte
character) is a `Step`, for any weights. -/
theorem table_step (xs : List Nat) (t o : Nat) :
    let r := row xs t
    Step xs t o (t + bigConsumed[r.m]!)
      (o + if bigIdx[r.m]! < 209 then units bigIdx[r.m]! else 0) := by
  intro r
  by_cases hi : bigIdx[r.m]! < 209
  · obtain ⟨hc1, -, hnext, hp⟩ := row_conv xs t r hi
    simp only [hi, ↓reduceIte]
    exact Step.conv t o _ _ hc1 hnext hp
  · have hrow := (List.all_eq_true.mp bigindex_exact) r.m r.hmem
    simp only [hi, ↓reduceIte, beq_iff_eq] at hrow ⊢
    rw [hrow]
    exact Step.error t o

/-! ## UTF-16: surrogate pairs only from a lead byte `≥ 0xF0`

`utf16_length_from_utf8` weighs a continuation byte 0, a byte `≥ 0xF0` 2,
any other byte 1. On a row with three code points (`145 ≤ idx < 209`) the
kernels build a 32-bit lane `perm_j` per code point with the `shufutf8`
shuffle and write two units for lane `j` only when
* westmere, haswell, ppc64: `composed perm_j > 0x3c00000` (`pair_needs_f0`);
* arm64, lsx, lasx: `perm_j & 0xf8000000 == 0xf0000000` (`pair_needs_f0_arm`).
The top byte of `perm_j` is shuffle entry `16 idx + 4 j + 3`: zero for an
index of 16 or more (pshufb, `vqtbl1q` and `vshuf_b` on a masked index all
give 0 there), else the byte at that offset, which `shuf3_starts` says is the
first byte of code point `j`. -/

def w16 (b : Nat) : Nat := if 0x80 ≤ b ∧ b < 0xC0 then 0 else if 0xF0 ≤ b then 2 else 1

/-- The 32-bit value assembled per lane by the `idx < 209` path of
westmere, haswell (src/haswell/avx2_convert_utf8_to_utf16.cpp:137-156) and
ppc64. -/
def composed (perm : Nat) : Nat :=
  let ascii := perm &&& 0x7f
  let middlebyte_shifted := (perm &&& 0x3f00) >>> 2
  let correct := (perm &&& 0x400000) >>> 1
  let middlehighbyte_shifted := ((perm &&& 0x3f0000) ^^^ correct) >>> 4
  let highbyte_shifted := (perm &&& 0xff000000) >>> 6
  (ascii ||| middlebyte_shifted) ||| (highbyte_shifted ||| middlehighbyte_shifted)

theorem pair_needs_f0 (perm : Nat) (h : 0x3c00000 < composed perm) :
    0xF0 ≤ perm >>> 24 := by
  unfold composed at h
  simp only at h
  have ha : perm &&& 0x7f < 2 ^ 18 := Nat.lt_of_le_of_lt Nat.and_le_right (by decide)
  have hm : (perm &&& 0x3f00) >>> 2 < 2 ^ 18 := by
    rw [Nat.shiftRight_eq_div_pow]
    have : perm &&& 0x3f00 ≤ 0x3f00 := Nat.and_le_right
    omega
  have hmh : ((perm &&& 0x3f0000) ^^^ ((perm &&& 0x400000) >>> 1)) >>> 4 < 2 ^ 18 := by
    have h1 : perm &&& 0x3f0000 < 2 ^ 22 := Nat.lt_of_le_of_lt Nat.and_le_right (by decide)
    have h2 : (perm &&& 0x400000) >>> 1 < 2 ^ 22 := by
      rw [Nat.shiftRight_eq_div_pow]
      have : perm &&& 0x400000 ≤ 0x400000 := Nat.and_le_right
      omega
    have := Nat.xor_lt_two_pow h1 h2
    rw [Nat.shiftRight_eq_div_pow]
    omega
  generalize hhi : perm &&& 0xff000000 = hi at h
  have hmod : hi % 2 ^ 24 = 0 := by
    rw [← hhi, Nat.and_mod_two_pow]
    simp
  have hle : hi ≤ perm := by rw [← hhi]; exact Nat.and_le_left
  generalize perm &&& 0x7f = a at h ha
  generalize (perm &&& 0x3f00) >>> 2 = b at h hm
  generalize ((perm &&& 0x3f0000) ^^^ ((perm &&& 0x400000) >>> 1)) >>> 4 = c at h hmh
  have hlow : a ||| b ||| c < 2 ^ 18 := Nat.or_lt_two_pow (Nat.or_lt_two_pow ha hm) hmh
  have hq : hi >>> 6 = (hi / 2 ^ 24) <<< 18 := by
    rw [Nat.shiftRight_eq_div_pow, Nat.shiftLeft_eq]
    omega
  have hreorder : (a ||| b) ||| (hi >>> 6 ||| c) = (hi / 2 ^ 24) <<< 18 ||| (a ||| b ||| c) := by
    rw [hq]
    ac_rfl
  have hadd := Nat.shiftLeft_add_eq_or_of_lt hlow (hi / 2 ^ 24)
  rw [hreorder, ← hadd, Nat.shiftLeft_eq] at h
  have hdiv : hi / 2 ^ 24 ≤ perm / 2 ^ 24 := Nat.div_le_div_right hle
  rw [Nat.shiftRight_eq_div_pow]
  omega

theorem pair_needs_f0_arm (perm : Nat) (h : perm &&& 0xf8000000 = 0xf0000000) :
    0xF0 ≤ perm >>> 24 := by
  have : perm &&& 0xf8000000 ≤ perm := Nat.and_le_left
  rw [Nat.shiftRight_eq_div_pow]
  omega

/-- Top byte of lane `j` of row `i` for a call at `t` over bytes `bs` (a byte
past the end reads as a continuation byte, weight 0). -/
def topByte (bs : List Nat) (t i j : Nat) : Nat :=
  let k := shuf[16 * i + 4 * j + 3]!
  if k < 16 then bs.getD (t + k) 0x80 else 0

/-- One three-code-point call over bytes `bs`, with weights `bs.map w16`:
if every lane that writes a pair has top byte `≥ 0xF0` (`pair_needs_f0`,
`pair_needs_f0_arm`), it is a `Step`. -/
theorem table_step16 (bs : List Nat) (t o : Nat) (pair : Nat → Bool)
    (r : Row (bs.map w16) t) (hi1 : 145 ≤ bigIdx[r.m]!) (hi2 : bigIdx[r.m]! < 209)
    (hpair : ∀ j, j < 3 → pair j = true → 0xF0 ≤ topByte bs t bigIdx[r.m]! j) :
    Step (bs.map w16) t o (t + bigConsumed[r.m]!)
      (o + ((if pair 0 then 2 else 1) + (if pair 1 then 2 else 1) + (if pair 2 then 2 else 1))) := by
  have hs := (List.all_eq_true.mp shuf3_starts) r.m r.hmem
  simp only [hi1, hi2, decide_true, Bool.and_self, Bool.not_true, Bool.false_or,
    Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq, List.all_eq_true, List.mem_range] at hs
  obtain ⟨⟨⟨⟨⟨hb1, hb2⟩, h01⟩, h12⟩, h2c⟩, htop⟩ := hs
  obtain ⟨hc1, hle, hnext, -⟩ := row_conv _ t r hi2
  apply Step.conv t o _ _ hc1 hnext
  generalize hI : bigIdx[r.m]! = i at *
  generalize hC : bigConsumed[r.m]! = c at *
  have hb := r.hb
  -- byte at a code-point start is leading
  have hlead : ∀ s, 0 < s → s ≤ 12 → bit r.m (s - 1) = 1 → (bs.map w16).getD (t + s) 0 ≠ 0 := by
    intro s hs0 hs12 hbit
    have := hb (s - 1) (by omega)
    rw [hbit, show t + (s - 1) + 1 = t + s by omega] at this
    unfold lead at this
    split at this <;> omega
  have hgetD : ∀ k, (bs.map w16).getD k 0 = w16 (bs.getD k 0x80) := by
    intro k
    simp only [List.getD_eq_getElem?_getD, List.getElem?_map]
    cases bs[k]? <;> simp [w16]
  -- a lane that writes a pair starts at a byte ≥ 0xF0
  have hlane : ∀ j, j < 3 → pair j = true → 0xF0 ≤ bs.getD (t + start3 i j) 0x80 := by
    intro j hj hp
    have h := hpair j hj hp
    have ht := htop j hj
    unfold topByte at h
    generalize shuf[16 * i + 4 * j + 3]! = K at h ht
    by_cases hk : K < 16
    · simp only [hk, ↓reduceIte] at h
      simp only [Bool.or_eq_true, decide_eq_true_eq, beq_iff_eq] at ht
      rcases ht with h16 | heq
      · omega
      · rw [← heq]; exact h
    · simp only [hk, ↓reduceIte] at h
      omega
  have hlead' : ∀ j, j < 3 → (bs.map w16).getD (t + start3 i j) 0 ≠ 0 →
      (if pair j then 2 else 1) ≤ w16 (bs.getD (t + start3 i j) 0x80) := by
    intro j hj hne
    rw [hgetD] at hne
    by_cases hp : pair j = true
    · have := hlane j hj hp
      simp only [hp, ↓reduceIte, w16]
      split <;> (try split) <;> omega
    · simp only [hp, Bool.false_eq_true, ↓reduceIte]
      omega
  have hs0 : start3 i 0 = 0 := by simp [start3]
  have w0 : (if pair 0 then 2 else 1) ≤ w16 (bs.getD t 0x80) + cont (bs.map w16) t := by
    by_cases hp : pair 0 = true
    · have := hlane 0 (by omega) hp
      rw [hs0, Nat.add_zero] at this
      simp only [hp, ↓reduceIte, w16]
      split <;> (try split) <;> omega
    · simp only [hp, Bool.false_eq_true, ↓reduceIte, cont, hgetD]
      split <;> omega
  have w1 := hlead' 1 (by omega) (hlead _ h01 (by omega) hb1)
  have w2 := hlead' 2 (by omega) (hlead _ (by omega) (by omega) hb2)
  have hsum := sumUpTo_three (fun k => w16 (bs.getD (t + k) 0x80)) (start3 i 1) (start3 i 2) c h01 h12 h2c
  have hL : L (((bs.map w16).drop t).take c) = sumUpTo (fun k => w16 (bs.getD (t + k) 0x80)) c := by
    rw [L_window]
    exact sumUpTo_congr _ _ c (fun k _ => hgetD _)
  rw [hL]
  simp only [Nat.add_zero] at hsum
  omega

/-- Fast paths that test the mask directly (16 or 8 ASCII bytes, `0xaaa`,
`0x924`, ...): a call that produces at most one unit per code-point end among
bytes `t + 1 .. t + c`, the last of which is leading, is a `Step`. -/
theorem count_step (xs : List Nat) (t o c p : Nat) (hc : 1 ≤ c)
    (hnext : xs.getD (t + c) 0 ≠ 0)
    (hp : p ≤ sumUpTo (fun k => lead xs (t + 1 + k)) c) :
    Step xs t o (t + c) (o + p) := by
  apply Step.conv t o c p hc hnext
  obtain ⟨c', rfl⟩ : ∃ c', c = c' + 1 := ⟨c - 1, by omega⟩
  simp only [sumUpTo] at hp
  have := lead_le xs (t + 1 + c')
  have := sumUpTo_mono (fun k => lead xs (t + 1 + k)) (fun k => xs.getD (t + 1 + k) 0) c'
    (fun k _ => lead_le_getD xs _)
  have hsplit := L_take_add xs t 1
  have hsplit2 := L_take_add xs (t + 1) c'
  rw [L_take_one] at hsplit
  rw [show t + 1 + c' = t + (c' + 1) by omega, L_take_add xs t (c' + 1)] at hsplit2
  rw [← L_window] at this
  unfold cont
  split <;> omega

/-- The `0x888` path of arm64, lsx and lasx: three 4-byte characters. It
advances 6 units only after checking that bytes 0, 4 and 8 match
`0xf8 -> 0xf0` (each then weighs 2), and 12 bytes are consumed with byte 12
leading (bit 11 of `0x888`). Otherwise it advances 0 (an error-like step). -/
theorem step_888 (bs : List Nat) (t o : Nat)
    (hlead : ∀ j, j < 3 → bs.getD (t + 4 * j) 0x80 &&& 0xf8 = 0xf0)
    (hnext : (bs.map w16).getD (t + 12) 0 ≠ 0) :
    Step (bs.map w16) t o (t + 12) (o + 6) := by
  apply Step.conv t o 12 6 (by omega) hnext
  have hgetD : ∀ k, (bs.map w16).getD k 0 = w16 (bs.getD k 0x80) := by
    intro k
    simp only [List.getD_eq_getElem?_getD, List.getElem?_map]
    cases bs[k]? <;> simp [w16]
  have h2 : ∀ j, j < 3 → w16 (bs.getD (t + 4 * j) 0x80) = 2 := by
    intro j hj
    have h := hlead j hj
    have : bs.getD (t + 4 * j) 0x80 &&& 0xf8 ≤ bs.getD (t + 4 * j) 0x80 := Nat.and_le_left
    unfold w16
    split <;> (try split) <;> omega
  have hsum := sumUpTo_three (fun k => w16 (bs.getD (t + k) 0x80)) 4 8 12 (by omega) (by omega) (by omega)
  have a := h2 0 (by omega)
  have b := h2 1 (by omega)
  have c := h2 2 (by omega)
  simp only [Nat.mul_zero, Nat.add_zero, Nat.mul_one] at a b c hsum
  rw [show t + 4 * 2 = t + 8 by omega] at c
  rw [L_window, sumUpTo_congr _ (fun k => w16 (bs.getD (t + k) 0x80)) 12 (fun k _ => hgetD _)]
  omega

/-! ## The `hf` budget of `reach_okK`

`reach_okK` assumes `q.f ≤ cont + cnt` of the inspected window. The lemmas
below discharge that from the call shape. `f = p + 1` is the fast-path and
unit-weight table shape (produced units, plus the lead that follows the
call). The arm64 `0x888` store runs before the lead-byte check, so its budget
is the three leads the mask `0x888` guarantees, plus the interchangeable
`cont` / starting lead. The lasx store runs after the check and is paid in
`L`, by `reach_L`. -/

theorem cnt_take_one (xs : List Nat) (s : Nat) :
    cnt ((xs.drop s).take 1) = lead xs s := by
  induction xs generalizing s with
  | nil => simp [lead]
  | cons x xs ih =>
    cases s with
    | zero =>
      simp only [List.drop_zero, List.take_succ_cons, List.take_zero,
        cnt_cons, lead, List.getD_cons_zero]
      split <;> rfl
    | succ s =>
      have hlead : lead (x :: xs) (s + 1) = lead xs s := by
        unfold lead
        rw [List.getD_cons_succ]
      rw [List.drop_succ_cons, hlead]
      exact ih s

theorem cnt_window (xs : List Nat) (s c : Nat) :
    cnt ((xs.drop s).take c) = sumUpTo (fun k => lead xs (s + k)) c := by
  induction c with
  | zero => simp [sumUpTo]
  | succ c ih =>
    have htake : (xs.drop s).take (c + 1) =
        (xs.drop s).take c ++ (xs.drop (s + c)).take 1 := by
      rw [List.take_add, List.drop_drop]
    rw [htake, cnt_append, ih, cnt_take_one, sumUpTo]
    exact Nat.add_comm _ _

theorem cont_add_lead (xs : List Nat) (t : Nat) : cont xs t + lead xs t = 1 := by
  unfold cont lead
  by_cases h0 : xs.getD t 0 = 0
  · rw [ite_eq_left h0, ite_eq_left h0]
  · rw [ite_eq_right h0, ite_eq_right h0]

/-- `p` leading bytes in `t + 1 .. t + c` pay for the budget `p + 1`: the
missing unit is `cont` when byte `t` is a continuation, and byte `t` itself
when it is a lead. -/
theorem plus_budget (xs : List Nat) (t c p : Nat)
    (hp : p ≤ cnt ((xs.drop (t + 1)).take c)) :
    p + 1 ≤ cont xs t + cnt ((xs.drop t).take (c + 1)) := by
  have htake : (xs.drop t).take (c + 1) =
      (xs.drop t).take 1 ++ (xs.drop (t + 1)).take c := by
    rw [show c + 1 = 1 + c from Nat.add_comm c 1, List.take_add, List.drop_drop]
  rw [htake, cnt_append, cnt_take_one]
  have hassoc : cont xs t + (lead xs t + cnt ((xs.drop (t + 1)).take c)) =
      (cont xs t + lead xs t) + cnt ((xs.drop (t + 1)).take c) :=
    Eq.symm (Nat.add_assoc _ _ _)
  rw [hassoc, cont_add_lead, Nat.add_comm 1]
  exact Nat.add_le_add_right hp 1

/-- `count_step`'s lead sum is that budget. -/
theorem count_budget (xs : List Nat) (t c p : Nat)
    (hp : p ≤ sumUpTo (fun k => lead xs (t + 1 + k)) c) :
    p + 1 ≤ cont xs t + cnt ((xs.drop t).take (c + 1)) := by
  have hrew : sumUpTo (fun k => lead xs (t + 1 + k)) c =
      cnt ((xs.drop (t + 1)).take c) := by
    rw [cnt_window]
  exact plus_budget xs t c p (by rw [← hrew]; exact hp)

/-- A reachable fast-path call whose path records `f ≤ p + 1` stores inside
`L`. `p ≤` the lead sum is the mask (`count_step`); `q.s` is the transcribed
store width (`okK`). -/
theorem reach_count (K pos t o c p : Nat) (xs : List Nat) (ps : List MPath) (q : MPath)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hd : t + (c + 1) ≤ pos + 64)
    (hp : p ≤ sumUpTo (fun k => lead xs (t + 1 + k)) c)
    (hK : okK K ps = true) (hq : q ∈ ps) (hf : q.f ≤ p + 1) :
    o + q.s ≤ L xs := by
  apply reach_okK K pos t o (c + 1) xs ps q h hloop hd hK hq
  exact Nat.le_trans hf (count_budget xs t c p hp)

/-- `0xfff`: 12 leads in the counted window, 16 units stored, `K = 3`.
Bytes `t + 1 .. t + 12` are the twelve bits of the mask. -/
theorem reach_fff (pos t o : Nat) (xs : List Nat) (ps : List MPath)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk 3 xs + 1) ≤ xs.length)
    (hd : t + 13 ≤ pos + 64)
    (hp : 12 ≤ sumUpTo (fun k => lead xs (t + 1 + k)) 12)
    (hK : okK 3 ps = true) (hq : (⟨16, 13⟩ : MPath) ∈ ps) :
    o + 16 ≤ L xs :=
  reach_count 3 pos t o 12 12 xs ps ⟨16, 13⟩ h hloop hd hp hK hq (by decide)

theorem reach_x86_utf16_fff (pos t o : Nat) (xs : List Nat)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk 3 xs + 1) ≤ xs.length)
    (hd : t + 13 ≤ pos + 64)
    (hp : 12 ≤ sumUpTo (fun k => lead xs (t + 1 + k)) 12) :
    o + 16 ≤ L xs :=
  reach_fff pos t o xs x86_utf16 h hloop hd hp K_x86_utf16.1 List.mem_cons_self

theorem reach_x86_utf32_fff (pos t o : Nat) (xs : List Nat)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk 3 xs + 1) ≤ xs.length)
    (hd : t + 13 ≤ pos + 64)
    (hp : 12 ≤ sumUpTo (fun k => lead xs (t + 1 + k)) 12) :
    o + 16 ≤ L xs :=
  reach_fff pos t o xs x86_utf32 h hloop hd hp K_x86_utf32.1 List.mem_cons_self

theorem reach_x86_latin1_fff (pos t o : Nat) (xs : List Nat)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk 3 xs + 1) ≤ xs.length)
    (hd : t + 13 ≤ pos + 64)
    (hp : 12 ≤ sumUpTo (fun k => lead xs (t + 1 + k)) 12) :
    o + 16 ≤ L xs :=
  reach_fff pos t o xs x86_latin1 h hloop hd hp K_x86_latin1.1 List.mem_cons_self

/-- `0x924`: 4 leads, 8 units stored. This pair forces `K = 3` on the UTF-16
lists together with `0xfff`. -/
theorem reach_924 (pos t o c : Nat) (xs : List Nat) (ps : List MPath)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk 3 xs + 1) ≤ xs.length)
    (hd : t + c + 1 ≤ pos + 64)
    (hp : 4 ≤ sumUpTo (fun k => lead xs (t + 1 + k)) c)
    (hK : okK 3 ps = true) (hq : (⟨8, 5⟩ : MPath) ∈ ps) :
    o + 8 ≤ L xs :=
  reach_count 3 pos t o c 4 xs ps ⟨8, 5⟩ h hloop hd hp hK hq (by decide)

theorem reach_x86_utf16_924 (pos t o c : Nat) (xs : List Nat)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk 3 xs + 1) ≤ xs.length)
    (hd : t + c + 1 ≤ pos + 64)
    (hp : 4 ≤ sumUpTo (fun k => lead xs (t + 1 + k)) c) :
    o + 8 ≤ L xs :=
  reach_924 pos t o c xs x86_utf16 h hloop hd hp K_x86_utf16.1
    (List.mem_cons_of_mem _ List.mem_cons_self)

/-- A unit-weight table row (`w01`, and UTF-16 rows below 145). `units + 1`
is the `hf` budget, continuation start included: `row_conv` gives
`units ≤ L(consumed) + cont`, and the following lead adds one to `cnt`. -/
theorem row_budget (xs : List Nat) (t : Nat) (r : Row xs t)
    (hi : bigIdx[r.m]! < 209)
    (h01 : ∀ k, k < bigConsumed[r.m]! + 1 →
      xs.getD (t + k) 0 = 0 ∨ xs.getD (t + k) 0 = 1) :
    units bigIdx[r.m]! + 1 ≤
      cont xs t + cnt ((xs.drop t).take (bigConsumed[r.m]! + 1)) := by
  obtain ⟨_, _, hnext, hp⟩ := row_conv xs t r hi
  have hLc : L ((xs.drop t).take bigConsumed[r.m]!) =
      cnt ((xs.drop t).take bigConsumed[r.m]!) := by
    rw [L_window, cnt_window]
    apply sumUpTo_congr
    intro k hk
    rcases h01 k (by omega) with h0 | h1
    · unfold lead
      rw [h0]
      rfl
    · unfold lead
      rw [h1]
      exact Eq.symm (ite_eq_right (by decide : ¬ (1 = 0)))
  have hsplit : (xs.drop t).take (bigConsumed[r.m]! + 1) =
      (xs.drop t).take bigConsumed[r.m]! ++
        (xs.drop (t + bigConsumed[r.m]!)).take 1 := by
    rw [List.take_add, List.drop_drop]
  have hone : cnt ((xs.drop (t + bigConsumed[r.m]!)).take 1) = 1 := by
    rw [cnt_take_one]
    rcases h01 bigConsumed[r.m]! (by omega) with h0 | h1
    · exact absurd h0 hnext
    · unfold lead
      rw [h1]
      exact ite_eq_right (by decide : ¬ (1 = 0))
  rw [hsplit, cnt_append, hone, ← hLc]
  calc
    units bigIdx[r.m]! + 1 ≤
        L ((xs.drop t).take bigConsumed[r.m]!) + cont xs t + 1 :=
      Nat.add_le_add_right hp 1
    _ = cont xs t + (L ((xs.drop t).take bigConsumed[r.m]!) + 1) := by
      rw [Nat.add_assoc, Nat.add_left_comm]

theorem reach_row (K pos t o : Nat) (xs : List Nat) (ps : List MPath) (q : MPath)
    (r : Row xs t) (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hi : bigIdx[r.m]! < 209)
    (h01 : ∀ k, k < bigConsumed[r.m]! + 1 →
      xs.getD (t + k) 0 = 0 ∨ xs.getD (t + k) 0 = 1)
    (hd : t + bigConsumed[r.m]! + 1 ≤ pos + 64)
    (hK : okK K ps = true) (hq : q ∈ ps)
    (hf : q.f ≤ units bigIdx[r.m]! + 1) :
    o + q.s ≤ L xs := by
  have hb := row_budget xs t r hi h01
  generalize hcons : bigConsumed[r.m]! = c at hb hd
  apply reach_okK K pos t o (c + 1) xs ps q h hloop hd hK hq
  exact Nat.le_trans hf hb

/-- Bits 3, 7 and 11 of `0x888`. The end-of-code-point mask sets bit `k` when
byte `t + k + 1` is a lead, so these are bytes `t + 4`, `t + 8` and `t + 12`. -/
theorem bits_888 : bit 0x888 3 = 1 ∧ bit 0x888 7 = 1 ∧ bit 0x888 11 = 1 := by
  decide

/-- Arm64 writes 6 units on `0x888` before the `F0` check
(`arm_convert_utf8_to_utf16.cpp`). The mask alone gives three leads. With the
`cont` / starting-lead swap the budget is 4, and `K = 2` (`K_arm_utf16`)
covers the store. -/
theorem sumUpTo_four (f : Nat → Nat) (s1 s2 s3 c : Nat)
    (h1 : 0 < s1) (h12 : s1 < s2) (h23 : s2 < s3) (h3 : s3 < c) :
    f 0 + f s1 + f s2 + f s3 ≤ sumUpTo f c := by
  have a := sumUpTo_le f (Nat.succ_le_of_lt h3)
  have b := sumUpTo_le f (Nat.succ_le_of_lt h23)
  have d := sumUpTo_le f (Nat.succ_le_of_lt h12)
  have e := sumUpTo_le f (Nat.succ_le_of_lt h1)
  simp only [sumUpTo] at a b d e
  have : sumUpTo f 0 = 0 := rfl
  omega

theorem budget_888 (xs : List Nat) (t : Nat)
    (h4 : lead xs (t + 4) = 1) (h8 : lead xs (t + 8) = 1)
    (h12 : lead xs (t + 12) = 1) :
    4 ≤ cont xs t + cnt ((xs.drop t).take 13) := by
  rw [cnt_window]
  have hsum := sumUpTo_four (fun k => lead xs (t + k)) 4 8 12 13
    (by omega) (by omega) (by omega) (by omega)
  simp only [h4, h8, h12, Nat.add_zero] at hsum
  have h1 := cont_add_lead xs t
  omega

theorem reach_arm_888 (pos t o : Nat) (xs : List Nat)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk 2 xs + 1) ≤ xs.length)
    (hd : t + 13 ≤ pos + 64)
    (h4 : lead xs (t + 4) = 1) (h8 : lead xs (t + 8) = 1)
    (h12 : lead xs (t + 12) = 1) :
    o + 6 ≤ L xs := by
  apply reach_okK 2 pos t o 13 xs arm_utf16 ⟨6, 4⟩ h hloop hd K_arm_utf16.1
    (List.mem_cons_of_mem _ (List.mem_cons_of_mem _ List.mem_cons_self))
  exact budget_888 xs t h4 h8 h12

/-- After the `F0` check the three leads weigh 2, the following byte is a
lead, and the call starts on a lead (`cont = 0`). -/
theorem window_888 (bs : List Nat) (t : Nat)
    (hlead : ∀ j, j < 3 → bs.getD (t + 4 * j) 0x80 &&& 0xf8 = 0xf0)
    (hnext : (bs.map w16).getD (t + 12) 0 ≠ 0) :
    cont (bs.map w16) t = 0 ∧
      6 ≤ L (((bs.map w16).drop t).take 12) ∧
      7 ≤ L (((bs.map w16).drop t).take 13) := by
  have hgetD : ∀ k, (bs.map w16).getD k 0 = w16 (bs.getD k 0x80) := by
    intro k
    simp only [List.getD_eq_getElem?_getD, List.getElem?_map]
    cases bs[k]? <;> simp [w16]
  have h2 : ∀ j, j < 3 → w16 (bs.getD (t + 4 * j) 0x80) = 2 := by
    intro j hj
    have h := hlead j hj
    have : bs.getD (t + 4 * j) 0x80 &&& 0xf8 ≤ bs.getD (t + 4 * j) 0x80 := Nat.and_le_left
    unfold w16
    split <;> (try split) <;> omega
  have h0 : (bs.map w16).getD t 0 = 2 := by
    have h := h2 0 (by omega)
    simp only [Nat.mul_zero, Nat.add_zero] at h
    rw [← hgetD t] at h
    exact h
  have hcont : cont (bs.map w16) t = 0 := by
    unfold cont
    have hne : ¬ ((bs.map w16).getD t 0 = 0) := by rw [h0]; decide
    rw [ite_eq_right hne]
  have hL : 6 ≤ L (((bs.map w16).drop t).take 12) := by
    have hsum := sumUpTo_three (fun k => w16 (bs.getD (t + k) 0x80)) 4 8 12
      (by omega) (by omega) (by omega)
    have a := h2 0 (by omega)
    have b := h2 1 (by omega)
    have c := h2 2 (by omega)
    simp only [Nat.mul_zero, Nat.add_zero, Nat.mul_one] at a b c hsum
    rw [show t + 4 * 2 = t + 8 by omega] at c
    rw [L_window, sumUpTo_congr _ (fun k => w16 (bs.getD (t + k) 0x80)) 12
      (fun k _ => hgetD _)]
    omega
  have hsplit : ((bs.map w16).drop t).take 13 =
      ((bs.map w16).drop t).take 12 ++ ((bs.map w16).drop (t + 12)).take 1 := by
    rw [show (13 : Nat) = 12 + 1 from rfl, List.take_add, List.drop_drop]
  have h13 : 7 ≤ L (((bs.map w16).drop t).take 13) := by
    rw [hsplit, L_append, L_take_one]
    have hone : 1 ≤ (bs.map w16).getD (t + 12) 0 :=
      Nat.succ_le_of_lt (Nat.pos_of_ne_zero hnext)
    calc
      7 = 6 + 1 := rfl
      _ ≤ L (((bs.map w16).drop t).take 12) + (bs.map w16).getD (t + 12) 0 :=
        Nat.add_le_add hL hone
  exact ⟨hcont, hL, h13⟩

/-- LSX `0x888` (`lsx_convert_utf8_to_utf16.cpp`): the check returns before
the store, and the store is `memcpy` of 12 bytes (6 units). That is `L` of
the 12 consumed bytes. -/
theorem reach_lsx_888 (K pos t o s : Nat) (bs : List Nat)
    (hs : s ≤ 6) (h : Reach (bs.map w16) true pos t o)
    (hloop : pos + 64 + (tk K (bs.map w16) + 1) ≤ (bs.map w16).length)
    (hd : t + 12 ≤ pos + 64)
    (hlead : ∀ j, j < 3 → bs.getD (t + 4 * j) 0x80 &&& 0xf8 = 0xf0)
    (hnext : (bs.map w16).getD (t + 12) 0 ≠ 0) :
    o + s ≤ L (bs.map w16) := by
  have hw := window_888 bs t hlead hnext
  apply reach_L K pos t o 12 s (bs.map w16) h hloop hd
  have hL : 6 ≤ L (((bs.map w16).drop t).take 12) := hw.2.1
  have hc : cont (bs.map w16) t = 0 := hw.1
  omega

/-- LASX `0x888` (`lasx_convert_utf8_to_utf16.cpp`): the check returns before
the store, then `__lsx_vst` writes 16 bytes (8 units) and the pointer advances
6. `L` of the 13-byte window is at least 7, so `K ≥ 1` covers the store. The
UTF-16 header's `K = 3` is forced by `0x924` / `0xfff`, which `reach_924` and
`reach_fff` bound. -/
theorem reach_lasx_888 (pos t o : Nat) (bs : List Nat)
    (h : Reach (bs.map w16) true pos t o)
    (hloop : pos + 64 + (tk 1 (bs.map w16) + 1) ≤ (bs.map w16).length)
    (hd : t + 13 ≤ pos + 64)
    (hlead : ∀ j, j < 3 → bs.getD (t + 4 * j) 0x80 &&& 0xf8 = 0xf0)
    (hnext : (bs.map w16).getD (t + 12) 0 ≠ 0) :
    o + 8 ≤ L (bs.map w16) := by
  have hw := window_888 bs t hlead hnext
  apply reach_L 1 pos t o 13 8 (bs.map w16) h hloop hd
  have hL : 7 ≤ L (((bs.map w16).drop t).take 13) := hw.2.2
  have hc : cont (bs.map w16) t = 0 := hw.1
  omega

end SafetyMargins.Utf8
