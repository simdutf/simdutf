import SafetyMargins.Basic
import SafetyMargins.IcelakeReal
import SafetyMargins.Exact
import SafetyMargins.ValidLatin1
import SafetyMargins.Replacement
/-
rvv: the `vl`-exact stores, from the lanes.

`Arch/Rvv.lean` models every rvv loop as `vlaFamily`: each iteration consumes
`vl ≤ len` units and its stores end at `L` of what it consumed (`tail 0`).
That every rvv store has exactly that size was read from the C++. This file
transcribes each transcoding loop of src/rvv/ (base64 excluded; `utf16fix`
is `Utf16Fix.rvv_inBounds`) as a function of the remaining input, down to
the lanes the `vcompress` masks keep, and proves that every store ends at or
before `L` of the whole input, for every input and every `vl` that
`vsetvl` may return.

* **Units.** An input is the list of unit values (bytes, 16-bit units after
  the byte flip of the big-endian variants, 32-bit units). The weight `w` of
  a unit is the per-unit term of the length function the caller sizes the
  buffer with (src/rvv/rvv_length_from.inl.cpp, which equals the scalar one);
  the buffer holds `L (xs.map w)` units.
* **`vsetvl`.** `vl = vlOf pick n` for an arbitrary `pick`: any `vl` with
  `1 ≤ vl ≤ n` (the RVV specification guarantees `vl ≤ AVL`, and `vl ≥ 1`
  when `AVL ≥ 1`). The theorems hold for every `pick`, so for every `VLEN`,
  `LMUL` and implementation choice of `vl`.
* **Iterations (`It`).** One iteration on the remaining input `ys` consumes
  `c` units, issues stores `(offset from the cursor, element count)`, advances
  the cursor by `adv`, and carries a state (`next`). `stop` is a `return`
  after the stores of that iteration.
* **Loops (`Reach`, `Store`).** `Reach` is the set of loop heads (state,
  cursor, units consumed); `Store a n` is a store of `n` elements at `a` issued
  by some iteration from a reachable head. `store_le` proves `a + n ≤ L` of
  the input from a per-iteration condition (`Fits`): every store ends within
  `g s + L` of the consumed units, and `adv + g next ≤ g s + L` of them, where
  `g` is the lag the state carries (0 for every loop except UTF-8 → Latin-1,
  whose cursor lags `L` by one byte after a block that ends on a lead byte,
  and the UTF-8 decoder, whose last lane may have written fewer units than its
  weight).

Data-dependent counts (`vcpop` of compress masks, the prefix before the first
surrogate, the scalar pair loops) are computed from the unit values; the
lane functions (`lane12`, `lane123`, `helperLane`, `c1234`) transcribe the
lane arithmetic and the bytes each compress mask keeps, and their counts are
proved for every lane value.

No store writes past `cursor + adv` of its iteration, so no store needs a
margin. The validating checks are not needed for the bounds: they only add
`return`s (`stop`), and every theorem holds whatever they decide (the UTF-8
check is an arbitrary `chk`).

Results (each for every `pick`, every input, every number of iterations):
* `copy_safe`: Latin-1 → UTF-16LE/BE and UTF-32, UTF-16LE/BE → Latin-1
  (`_with_errors`, plain, `convert_valid`), UTF-32 → Latin-1,
  `change_endianness_utf16`;
* `latin1to8_safe`: Latin-1 → UTF-8;
* `utf32to16_safe`: UTF-32 → UTF-16LE/BE (`_with_errors`, plain, `convert_valid`);
* `utf16to32_safe`: UTF-16LE/BE → UTF-32 (all variants);
* `utf16to8_safe`, `utf16to8_safe_repl`: UTF-16LE/BE → UTF-8 (all variants,
  and each kernel call of the conversions with replacement);
* `utf32to8_safe`: UTF-32 → UTF-8 (`_with_errors`, plain, `convert_valid`);
* `utf8toLatin1_safe`, `validUtf8toLatin1_safe`: UTF-8 → Latin-1 (checking
  kernel on every input; `convert_valid` on its contract `ValidLatin1.LV`);
* `utf8_loop_safe`: the vector loop of UTF-8 → UTF-16LE/BE and UTF-32 (all
  variants); `utf8_tail`, `utf8_validating_tail`: its reparse step and scalar
  tail (cursor never below 2 before the step back, tail within `L`).

Read from the C++ and the RVV specification, not checked by Lean:
* the transcription itself (one step function per loop, file:line in each
  docstring), including the range forms of the bit tests (`(v & 0xF800) ==
  0xD800`, `v + 0xFFFF2000 > 0xFFFFF7FF`, `int8 > -65`, `vredmaxu < c`);
* RVV semantics: `vsetvl` returns `1 ≤ vl ≤ AVL` (and `AVL` itself when
  `AVL ≤ VLMAX`, used for the `M1` split of the UTF-8 4-byte path, whose
  second half is lanes `M1..` because `VLMAX(e8m2) = 2 VLMAX(e8m1)`);
  `vcompress` packs the selected lanes in order and `vcpop` counts them; a
  unit-stride store with length `n` writes `n` elements and nothing past them
  (tail elements are not stored); the LMUL ratios make every `vl * 2` /
  `vl * 4` fit its register group;
* UTF-8 tail: the scalar functions write at most `L` of their input; the
  scalar validator rejects a first byte that is a continuation byte; `dst[-1]`
  holds the last lane's first unit when that lane wrote two units;
* the units are bytes / 16-bit / 32-bit values (`utf16to8_safe` assumes
  `< 65536`), and the length functions are the per-unit sums
  (src/rvv/rvv_length_from.inl.cpp).
-/
namespace SafetyMargins.RvvExact
open SafetyMargins SafetyMargins.Validator SafetyMargins.Exact SafetyMargins.Utf8
  SafetyMargins.IcelakeReal

set_option linter.unusedSimpArgs false

/-! ## Loops -/

/-- One iteration: units consumed, stores (offset from the cursor, element
count), cursor advance, next state, and whether the kernel returns after it. -/
structure It (σ : Type) where
  c : Nat
  st : List (Nat × Nat)
  adv : Nat
  next : σ
  stop : Bool := false

/-- Loop heads reachable from state `s0`, cursor 0, 0 units consumed. The loop
runs while more than `T` units remain (`T = 0`: `len > 0`; the UTF-8 decoder
keeps `T = 3` units for its look-ahead). -/
inductive Reach {σ : Type} (T : Nat) (f : σ → List Nat → It σ) (xs : List Nat) (s0 : σ) :
    σ → Nat → Nat → Prop
  | init : Reach T f xs s0 s0 0 0
  | step {s : σ} {o i : Nat} : Reach T f xs s0 s o i → T < (xs.drop i).length →
      (f s (xs.drop i)).stop = false →
      Reach T f xs s0 (f s (xs.drop i)).next (o + (f s (xs.drop i)).adv)
        (i + (f s (xs.drop i)).c)

/-- A store of `n` elements at output index `a`, issued from a reachable head. -/
def Store {σ : Type} (T : Nat) (f : σ → List Nat → It σ) (xs : List Nat) (s0 : σ)
    (a n : Nat) : Prop :=
  ∃ s o i, Reach T f xs s0 s o i ∧ T < (xs.drop i).length ∧
    ∃ p ∈ (f s (xs.drop i)).st, a = o + p.1 ∧ n = p.2

/-- The per-iteration condition, for every remaining input `ys` satisfying
the invariant `G` (a bound on the unit values, or the contract of a
`convert_valid` function). -/
def Fits {σ : Type} (T : Nat) (f : σ → List Nat → It σ) (w : Nat → Nat) (g : σ → Nat)
    (G : σ → List Nat → Prop) : Prop :=
  ∀ s ys, G s ys → T < ys.length →
    (∀ p ∈ (f s ys).st, p.1 + p.2 ≤ g s + L ((ys.take (f s ys).c).map w)) ∧
    ((f s ys).stop = false →
      (f s ys).adv + g (f s ys).next ≤ g s + L ((ys.take (f s ys).c).map w) ∧
      G (f s ys).next (ys.drop (f s ys).c))

theorem L_map_take_add (w : Nat → Nat) (xs : List Nat) (i c : Nat) :
    L ((xs.take (i + c)).map w) = L ((xs.take i).map w) + L (((xs.drop i).take c).map w) := by
  rw [List.take_add, List.map_append, L_append]

theorem L_map_take_le (w : Nat → Nat) (xs : List Nat) (i : Nat) :
    L ((xs.take i).map w) ≤ L (xs.map w) := by
  rw [List.map_take]; exact L_take_le _ _

theorem reach_inv {σ : Type} {T : Nat} {f : σ → List Nat → It σ} {w : Nat → Nat}
    {g : σ → Nat} {G : σ → List Nat → Prop} {xs : List Nat} {s0 : σ}
    (hf : Fits T f w g G) (h0 : G s0 xs) (hg0 : g s0 = 0) {s : σ} {o i : Nat}
    (h : Reach T f xs s0 s o i) : G s (xs.drop i) ∧ o + g s ≤ L ((xs.take i).map w) := by
  induction h with
  | init => simp [h0, hg0]
  | @step s o i _ hT hstop ih =>
    obtain ⟨hG, hle⟩ := ih
    obtain ⟨_, hadv⟩ := hf s _ hG hT
    obtain ⟨hadv, hG'⟩ := hadv hstop
    refine ⟨by rw [← List.drop_drop]; exact hG', ?_⟩
    rw [L_map_take_add]; omega

/-- **Every store of the loop ends at or before `L` of the input.** -/
theorem store_le {σ : Type} {T : Nat} {f : σ → List Nat → It σ} {w : Nat → Nat}
    {g : σ → Nat} {G : σ → List Nat → Prop} {xs : List Nat} {s0 : σ}
    (hf : Fits T f w g G) (h0 : G s0 xs) (hg0 : g s0 = 0) {a n : Nat}
    (h : Store T f xs s0 a n) : a + n ≤ L (xs.map w) := by
  obtain ⟨s, o, i, hR, hT, p, hp, ha, hn⟩ := h
  obtain ⟨hG, hle⟩ := reach_inv hf h0 hg0 hR
  have h1 := (hf s _ hG hT).1 p hp
  have h2 := L_map_take_add w xs i (f s (xs.drop i)).c
  have h3 := L_map_take_le w xs (i + (f s (xs.drop i)).c)
  omega

/-- Lower bound on the cursor, from a lower bound on each advance. -/
theorem reach_lower {σ : Type} {T : Nat} {f : σ → List Nat → It σ} {w w' : Nat → Nat}
    {g : σ → Nat} {G : σ → List Nat → Prop} {xs : List Nat} {s0 : σ}
    (hf : Fits T f w g G) (h0 : G s0 xs) (hg0 : g s0 = 0)
    (hl : ∀ s ys, G s ys → T < ys.length → (f s ys).stop = false →
      L ((ys.take (f s ys).c).map w') ≤ (f s ys).adv)
    {s : σ} {o i : Nat} (h : Reach T f xs s0 s o i) : L ((xs.take i).map w') ≤ o := by
  induction h with
  | init => simp
  | @step s o i hR hT hstop ih =>
    have hG := (reach_inv hf h0 hg0 hR).1
    have := hl s _ hG hT hstop
    rw [L_map_take_add]; omega

/-- Units consumed, from a bound on each iteration. -/
theorem reach_len {σ : Type} {T : Nat} {f : σ → List Nat → It σ} {w : Nat → Nat}
    {g : σ → Nat} {G : σ → List Nat → Prop} {xs : List Nat} {s0 : σ}
    (hf : Fits T f w g G) (h0 : G s0 xs) (hg0 : g s0 = 0) (hT0 : T ≤ xs.length)
    (hc : ∀ s ys, G s ys → T < ys.length → (f s ys).c + T ≤ ys.length)
    {s : σ} {o i : Nat} (h : Reach T f xs s0 s o i) : i + T ≤ xs.length := by
  induction h with
  | init => simpa using hT0
  | @step s o i hR hT _ ih =>
    have hG := (reach_inv hf h0 hg0 hR).1
    have := hc s _ hG hT
    simp at this; omega

/-- Stateless loops with a bound on the unit values. -/
def Bnd (B : Nat) : Unit → List Nat → Prop := fun _ ys => ∀ x ∈ ys, x < B

theorem bnd_drop {B : Nat} {ys : List Nat} (h : Bnd B () ys) (c : Nat) : Bnd B () (ys.drop c) :=
  fun x hx => h x (List.mem_of_mem_drop hx)

/-! ## Lists and lanes -/

def ind (b : Bool) : Nat := if b then 1 else 0

theorem ind_le (b : Bool) : ind b ≤ 1 := by cases b <;> simp [ind]

/-- `vl = __riscv_vsetvl_*(n)`: any value in `[1, n]` (`vl ≤ AVL`). -/
def vlOf (pick : Nat → Nat) (n : Nat) : Nat := min (max (pick n) 1) n

theorem vlOf_le (pick : Nat → Nat) (n : Nat) : vlOf pick n ≤ n := Nat.min_le_right _ _

theorem L_map_le {α : Type} (l : List α) (f g : α → Nat) (h : ∀ x ∈ l, f x ≤ g x) :
    L (l.map f) ≤ L (l.map g) := by
  induction l with
  | nil => simp
  | cons x r ih =>
    simp only [List.map_cons, L_cons]
    have := h x (by simp)
    have := ih (fun y hy => h y (by simp [hy]))
    omega

theorem L_map_congr {α : Type} (l : List α) (f g : α → Nat) (h : ∀ x ∈ l, f x = g x) :
    L (l.map f) = L (l.map g) :=
  Nat.le_antisymm (L_map_le l f g (fun x hx => Nat.le_of_eq (h x hx)))
    (L_map_le l g f (fun x hx => Nat.le_of_eq (h x hx).symm))

theorem L_map_one {α : Type} (l : List α) : L (l.map fun _ => 1) = l.length := by
  induction l with
  | nil => simp
  | cons x r ih => simp [ih]; omega

theorem L_map_add {α : Type} (l : List α) (f g : α → Nat) :
    L (l.map fun x => f x + g x) = L (l.map f) + L (l.map g) := by
  induction l with
  | nil => simp
  | cons x r ih => simp only [List.map_cons, L_cons, ih]; omega

/-- Units of the block are at least 1 each: `vl` elements fit. -/
theorem len_le_L (l : List Nat) (w : Nat → Nat) (h : ∀ x ∈ l, 1 ≤ w x) :
    l.length ≤ L (l.map w) := by
  rw [← L_map_one l]; exact L_map_le l _ _ h

theorem take_vl_len (ys : List Nat) {vl : Nat} (h : vl ≤ ys.length) : (ys.take vl).length = vl := by
  simp [List.length_take]; omega

theorem L_take_mono (w : Nat → Nat) (ys : List Nat) {a b : Nat} (h : a ≤ b) :
    L ((ys.take a).map w) ≤ L ((ys.take b).map w) := by
  have : ys.take a = (ys.take b).take a := by rw [List.take_take]; congr 1; omega
  rw [this]; exact L_map_take_le w _ a

/-- `vcpop` of an all-false mask is 0, of an all-true mask the length. -/
theorem L_ind_eq_len {l : List Nat} {p : Nat → Bool}
    (h : L (l.map fun x => ind (p x)) = l.length) : ∀ x ∈ l, p x = true := by
  induction l with
  | nil => simp
  | cons y r ih =>
    simp only [List.map_cons, L_cons, List.length_cons] at h
    have h1 := ind_le (p y)
    have h2 : L (r.map fun x => ind (p x)) ≤ r.length := by
      rw [← L_map_one r]; exact L_map_le r _ _ (fun x _ => ind_le _)
    intro x hx
    simp at hx
    rcases hx with rfl | hx
    · cases hpy : p x <;> simp_all [ind] <;> omega
    · exact ih (by omega) x hx

/-! ### Bitwise facts -/

theorem or_ge_right (a b : Nat) : b ≤ a ||| b := Nat.right_le_or

theorem or_div256 (a b : Nat) : (a ||| b) / 256 = a / 256 ||| b / 256 := by
  have h := @Nat.shiftRight_or_distrib 8 a b
  simpa [Nat.shiftRight_eq_div_pow] using h

theorem or_mod256 (a b : Nat) : (a ||| b) % 256 = a % 256 ||| b % 256 := by
  simpa using @Nat.or_mod_two_pow a b 8

theorem or_lt256 {a b : Nat} (ha : a < 256) (hb : b < 256) : a ||| b < 256 :=
  Nat.or_lt_two_pow (n := 8) ha hb

theorem or_low (a b : Nat) (hb : b < 256) : 256 * a ||| b = 256 * a + b := by
  have := @Nat.two_pow_add_eq_or_of_lt 8 b hb a
  simpa using this.symm

theorem ff_lo (x : Nat) : (x ||| 0xFF) % 256 ≠ 0 := by
  rw [or_mod256]; have := or_ge_right (x % 256) (0xFF % 256); simp at this ⊢

theorem ff_hi (x : Nat) : (x ||| 0xFF) / 256 = x / 256 := by
  rw [or_div256]; simp

theorem c0_hi (t : Nat) (ht : t < 65536) : (t ||| 0x80C0) / 256 % 256 ≠ 0 := by
  rw [or_div256]
  have h1 : t / 256 < 256 := by omega
  have h3 := or_lt256 h1 (show (0x80C0 : Nat) / 256 < 256 by decide)
  have h4 := or_ge_right (t / 256) (0x80C0 / 256)
  simp at h3 h4 ⊢
  omega

/-! ### The 1/2-byte lane (UTF-16/UTF-32 → UTF-8)

src/rvv/rvv_utf32_to.inl.cpp:70-85 and src/rvv/rvv_utf16_to.inl.cpp:107-122,
on the 16-bit lane `vn`: `twoByte = ((vn << 8) | (vn >> 6)) & k` (`k` is
`0x3F3F` for UTF-32, `0x3F1F` for UTF-16), `vout16 = m234 ? twoByte | 0x80C0
: vn`; the compress mask keeps each byte of `vout16 | 0xFF` that is nonzero.
`lane12` is the number of bytes kept. -/
def lane12 (m234 : Bool) (k vn : Nat) : Nat :=
  let twoByte := ((((vn <<< 8) % 65536) ||| (vn >>> 6)) &&& k)
  let vout16 := if m234 then twoByte ||| 0x80C0 else vn
  let y := vout16 ||| 0xFF
  ind (y % 256 != 0) + ind (y / 256 % 256 != 0)

theorem lane12_eq (k vn : Nat) (hk : k < 65536) (hv : vn < 0x800) :
    lane12 (decide (0x7F < vn)) k vn = 1 + ind (decide (0x7F < vn)) := by
  unfold lane12
  simp only
  generalize ht : (((vn <<< 8) % 65536 ||| vn >>> 6) &&& k) = t
  have htl : t < 65536 := by
    rw [← ht]; exact Nat.lt_of_le_of_lt Nat.and_le_right hk
  by_cases h7 : 0x7F < vn
  · simp [h7, ind, ff_lo, ff_hi, c0_hi t htl]
  · have : vn / 256 % 256 = 0 := by omega
    simp [h7, ind, ff_lo, ff_hi, this]

/-! ### The 1/2/3-byte lane (UTF-16/UTF-32 → UTF-8)

src/rvv/rvv_utf32_to.inl.cpp:127-148 and src/rvv/rvv_utf16_to.inl.cpp:149-170,
on the 16-bit lane `vn`, widened to the 32-bit `w123`; the compress mask
keeps byte 2 of every lane (`m4mulp2`, rvv_utf32_to.inl.cpp:42-43 /
rvv_utf16_to.inl.cpp:79-80) and every nonzero
byte. `lane123` is the number of bytes kept. -/
def lane123 (m234 m34 : Bool) (vn : Nat) : Nat :=
  let v1 := if m234 then (vn &&& 0x3F) ||| 0x80 else vn
  let v1 := (v1 <<< 8) % 65536
  let v2 := ((vn >>> 6) &&& 0x3F) ||| 0x80
  let v2 := if !m34 then v2 ||| 0x40 else v2
  let v3 := (vn >>> 12) ||| 0xE0
  let v12 := if m234 then v1 ||| v2 else v1
  let w12 := v12 * 256
  let w123 := if m34 then (w12 + v3) % 4294967296 else w12
  ind (w123 % 256 != 0) + ind (w123 / 256 % 256 != 0) + 1 + ind (w123 / 16777216 % 256 != 0)

theorem lane123_eq (vn : Nat) (hv : vn < 65536) :
    lane123 (decide (0x7F < vn)) (decide (0x7FF < vn)) vn
      = 1 + ind (decide (0x7F < vn)) + ind (decide (0x7FF < vn)) := by
  unfold lane123
  simp only
  generalize hv1 : ((if decide (0x7F < vn) = true then (vn &&& 0x3F) ||| 0x80 else vn) <<< 8) % 65536 = v1
  have hv1a : v1 % 256 = 0 ∧ v1 < 65536 := by
    rw [← hv1, Nat.shiftLeft_eq]; omega
  generalize hv2 : (vn >>> 6 &&& 0x3F) ||| 0x80 = v2
  have hv2a : v2 < 256 ∧ 0x80 ≤ v2 := by
    rw [← hv2]
    exact ⟨or_lt256 (Nat.lt_of_le_of_lt Nat.and_le_right (by decide)) (by decide), or_ge_right _ _⟩
  generalize hv2' : (if (!decide (0x7FF < vn)) = true then v2 ||| 0x40 else v2) = v2'
  have hv2b : v2' < 256 ∧ 0x80 ≤ v2' := by
    rw [← hv2']; split
    · exact ⟨or_lt256 hv2a.1 (by decide), Nat.le_trans hv2a.2 Nat.left_le_or⟩
    · exact hv2a
  generalize hv3 : vn >>> 12 ||| 0xE0 = v3
  have hv3a : v3 < 256 ∧ 0xE0 ≤ v3 := by
    rw [← hv3]
    refine ⟨or_lt256 ?_ (by decide), or_ge_right _ _⟩
    rw [Nat.shiftRight_eq_div_pow]; omega
  have hor : v1 ||| v2' = v1 + v2' := by
    have : v1 = 256 * (v1 / 256) := by omega
    rw [this, or_low _ _ hv2b.1]
  have hb : ∀ a c : Nat, a < 256 → c < 256 →
      ((v1 + a) * 256 + c) % 4294967296 = (v1 + a) * 256 + c ∧
      ((v1 + a) * 256 + c) % 256 = c ∧ ((v1 + a) * 256 + c) / 256 % 256 = a ∧
      ((v1 + a) * 256 + c) / 16777216 % 256 = 0 := by
    intro a c ha hc; omega
  by_cases h7 : 0x7F < vn <;> by_cases h11 : 0x7FF < vn
  · simp only [h7, h11, decide_true, decide_false, Bool.not_true, Bool.not_false,
      Bool.false_eq_true, ite_true, ite_false, hor]
    obtain ⟨e1, e2, e3, e4⟩ := hb v2' v3 hv2b.1 hv3a.1
    have : v3 ≠ 0 := by omega
    have : v2' ≠ 0 := by omega
    rw [e1, e2, e3, e4]; simp [ind, *]
  · simp only [h7, h11, decide_true, decide_false, Bool.not_true, Bool.not_false,
      Bool.false_eq_true, ite_true, ite_false, hor]
    obtain ⟨_, e2, e3, e4⟩ := hb v2' 0 hv2b.1 (by decide)
    simp only [Nat.add_zero] at e2 e3 e4
    have : v2' ≠ 0 := by omega
    rw [e2, e3, e4]; simp [ind, *]
  · omega
  · simp only [h7, h11, decide_true, decide_false, Bool.not_true, Bool.not_false,
      Bool.false_eq_true, ite_true, ite_false, hor]
    obtain ⟨_, e2, e3, e4⟩ := hb 0 0 (by decide) (by decide)
    simp only [Nat.add_zero] at e2 e3 e4
    rw [e2, e3, e4]; simp [ind]

/-! ### `rvv_utf32_store_utf16_m4` (src/rvv/rvv_helpers.inl.cpp:1-23)

On a 32-bit lane `u`: `sur` is the surrogate pair of `u - 0x10000` (lines
7-11), the lane is `sur` when `u > 0xFFFF` and `u` otherwise (lines 13-15),
reinterpreted as two 16-bit halves; the mask keeps the even (low) half always
(`m4even`) and the odd (high) half when nonzero (lines 17-18); the store
writes the `vcpop` (lines 19-21). `helperLane u` is the list of 16-bit units
kept, in order. -/
def surOf (s : Nat) : Nat := ((((s <<< 16) % 4294967296) ||| (s >>> 10)) &&& 0x3FF03FF) ||| 0xDC00D800

def merged (u : Nat) : Nat :=
  if 0xFFFF < u then surOf ((u + 4294967296 - 0x10000) % 4294967296) else u

def helperLane (u : Nat) : List Nat :=
  merged u % 65536 :: (if merged u / 65536 % 65536 != 0 then [merged u / 65536 % 65536] else [])

theorem helper_small (u : Nat) (h : u ≤ 0xFFFF) : helperLane u = [u] := by
  have h1 : merged u = u := by simp [merged]; omega
  have h2 : u / 65536 % 65536 = 0 := by omega
  have h3 : u % 65536 = u := by omega
  simp only [helperLane, h1, h2, h3]; rfl

theorem helper_len (u : Nat) : 1 ≤ (helperLane u).length ∧ (helperLane u).length ≤ 2 := by
  unfold helperLane; split <;> simp

/-- The helper writes one unit per BMP lane and at most two otherwise. -/
theorem helper_le (u : Nat) : (helperLane u).length ≤ 1 + ind (decide (0xFFFF < u)) := by
  by_cases h : u ≤ 0xFFFF
  · rw [helper_small u h]; simp
  · have := (helper_len u).2
    have hu : 0xFFFF < u := by omega
    simp only [ind, hu, decide_true, ite_true]; omega

theorem sur_lo (B : Nat) :
    0xD800 ≤ ((B &&& 0x3FF03FF) ||| 0xDC00D800) % 65536 ∧
    ((B &&& 0x3FF03FF) ||| 0xDC00D800) % 65536 ≤ 0xDBFF := by
  have e4 : (65536 : Nat) = 2^16 := by decide
  rw [e4, Nat.or_mod_two_pow, Nat.and_mod_two_pow]
  have e2 : (0x3FF03FF : Nat) % 2^16 = 2^10 - 1 := by decide
  have e3 : (0xDC00D800 : Nat) % 2^16 = 2^10 * 54 := by decide
  rw [e2, e3, Nat.and_two_pow_sub_one_eq_mod, Nat.or_comm,
    ← Nat.two_pow_add_eq_or_of_lt (Nat.mod_lt _ (by decide))]
  have := Nat.mod_lt (B % 2^16) (show 2^10 > 0 by decide)
  have e5 : (2:Nat)^10 = 1024 := by decide
  rw [e5] at this ⊢
  omega

theorem surOf_lo (s : Nat) : 0xD800 ≤ surOf s % 65536 ∧ surOf s % 65536 ≤ 0xDBFF := sur_lo _

/-- A lane that writes two units writes a high surrogate first. -/
theorem helper_two (u : Nat) (h : (helperLane u).length = 2) :
    hiS (helperLane u).head! = true := by
  by_cases hu : u ≤ 0xFFFF
  · rw [helper_small u hu] at h; simp only [List.length_singleton] at h; omega
  · have hu' : merged u = surOf ((u + 4294967296 - 0x10000) % 4294967296) := by
      unfold merged; rw [ite_eq_left_iff.mpr (fun h => absurd h (by omega))]
    show hiS (merged u % 65536) = true
    rw [hu']
    have := surOf_lo ((u + 4294967296 - 0x10000) % 4294967296)
    generalize surOf ((u + 4294967296 - 0x10000) % 4294967296) % 65536 = x at this ⊢
    unfold hiS; exact decide_eq_true ⟨this.1, by omega⟩


/-! ### Prefixes -/

theorem mem_takeWhile {p : Nat → Bool} {l : List Nat} {x : Nat} (h : x ∈ l.takeWhile p) :
    p x = true := by
  induction l with
  | nil => simp at h
  | cons y r ih =>
    simp only [List.takeWhile_cons] at h
    split at h
    · simp at h; rcases h with rfl | h
      · assumption
      · exact ih h
    · simp at h

theorem mem_takeWhile_sub {p : Nat → Bool} {l : List Nat} {x : Nat} (h : x ∈ l.takeWhile p) :
    x ∈ l := by
  rw [← List.takeWhile_append_dropWhile (p := p) (l := l)]; exact List.mem_append_left _ h

/-- The prefix before the first lane failing `p` (the `vfirst` of the C++)
is the first `pre.length` units of the input. -/
theorem take_takeWhile (ys : List Nat) (vl : Nat) (p : Nat → Bool) :
    ys.take ((ys.take vl).takeWhile p).length = (ys.take vl).takeWhile p := by
  have hy : ys = ys.take vl ++ ys.drop vl := (List.take_append_drop vl ys).symm
  generalize hb0 : ys.take vl = blk at hy
  have hb : blk = blk.takeWhile p ++ blk.dropWhile p := List.takeWhile_append_dropWhile.symm
  generalize blk.takeWhile p = pre at hb ⊢
  rw [hy, hb, List.append_assoc]
  exact List.take_left' rfl

theorem takeWhile_len_le (ys : List Nat) (vl : Nat) (p : Nat → Bool) :
    ((ys.take vl).takeWhile p).length ≤ vl := by
  have h := congrArg List.length (List.takeWhile_append_dropWhile (p := p) (l := ys.take vl))
  rw [List.length_append, List.length_take] at h; omega

/-! ## Loops that store `vl` units for `vl` units

`copyStep bad`: `vl = vsetvl(len)`; return (before the store) if a lane is
`bad`; otherwise one store of `vl` elements, advance `vl`. Every unit weighs
1 (the length functions are `len`). The loops:

* src/rvv/rvv_latin1_to.inl.cpp:38-42 and :49-55, Latin-1 → UTF-16LE/BE
  (`vse16 ... vl`), :64-68 Latin-1 → UTF-32 (`vse32 ... vl`); `bad = false`;
* src/rvv/rvv_utf16_to.inl.cpp:6-13, UTF-16 → Latin-1 `_with_errors` (both
  endiannesses, and the plain conversion through it, :20-30): `bad = (> 255)`
  (:10-12), store `vse8 ... vl` (:13); :51-55 and :62-66, `convert_valid`
  LE/BE: `bad = false`;
* src/rvv/rvv_utf32_to.inl.cpp:11-22, UTF-32 → Latin-1 (`_with_errors`, plain
  and `convert_valid` through it, :2-6, :27-30): `bad = (> 255)` (:14-16),
  store `vse8 ... vl` (:20-22);
* src/rvv/rvv_utf16fix.cpp:97-101, `change_endianness_utf16`: `bad = false`. -/
def copyStep (bad : Nat → Bool) (pick : Nat → Nat) : Unit → List Nat → It Unit := fun _ ys =>
  let vl := vlOf pick ys.length
  if (ys.take vl).any bad then ⟨0, [], 0, (), true⟩ else ⟨vl, [(0, vl)], vl, (), false⟩

theorem copy_fits (bad : Nat → Bool) (pick : Nat → Nat) :
    Fits 0 (copyStep bad pick) (fun _ => 1) (fun _ => 0) (fun _ _ => True) := by
  intro s ys _ _
  have hvl := vlOf_le pick ys.length
  unfold copyStep; dsimp only
  generalize vlOf pick ys.length = vl at hvl
  have hL : L ((ys.take vl).map fun _ => 1) = vl := by
    rw [L_map_one]; exact take_vl_len ys hvl
  split
  · simp
  · simp [hL, -List.map_take]

/-- **Latin-1 → UTF-16/UTF-32, UTF-16/UTF-32 → Latin-1, `change_endianness`:
every store ends within the `len`-unit output.** -/
theorem copy_safe (bad : Nat → Bool) (pick : Nat → Nat) (xs : List Nat) {a n : Nat}
    (h : Store 0 (copyStep bad pick) xs () a n) : a + n ≤ xs.length := by
  have := store_le (copy_fits bad pick) trivial rfl h
  rwa [L_map_one] at this

/-! ## Latin-1 → UTF-8 (src/rvv/rvv_latin1_to.inl.cpp:2-31) -/

/-- `utf8_length_from_latin1` per byte (src/rvv/rvv_length_from.inl.cpp:73-82:
`len` plus the bytes with `int8 < 0`). -/
def wL8 (b : Nat) : Nat := 1 + ind (decide (0x80 ≤ b))

/-- `vl = vsetvl_e8m2(len)` (rvv_latin1_to.inl.cpp:6); `cnt = vcpop(int8 < 0)` (:8-10);
`vlOut = vl + cnt` (:11); the ASCII store `vse8 ... vlOut` (:13) and the
compressed store `vse8 ... vlOut` (:28); advance `vlOut` (:5). -/
def latin1to8Step (pick : Nat → Nat) : Unit → List Nat → It Unit := fun _ ys =>
  let vl := vlOf pick ys.length
  let cnt := L ((ys.take vl).map fun b => ind (decide (0x80 ≤ b)))
  if cnt = 0 then ⟨vl, [(0, vl + cnt)], vl + cnt, (), false⟩
  else ⟨vl, [(0, vl + cnt)], vl + cnt, (), false⟩

theorem latin1to8_fits (pick : Nat → Nat) :
    Fits 0 (latin1to8Step pick) wL8 (fun _ => 0) (fun _ _ => True) := by
  intro s ys _ _
  have hvl := vlOf_le pick ys.length
  unfold latin1to8Step; dsimp only
  generalize vlOf pick ys.length = vl at hvl
  have hL : L ((ys.take vl).map wL8) = vl +
      L ((ys.take vl).map fun b => ind (decide (0x80 ≤ b))) := by
    rw [show wL8 = fun b => 1 + ind (decide (0x80 ≤ b)) from rfl, L_map_add, L_map_one,
      take_vl_len ys hvl]
  split <;> simp [hL, -List.map_take]

/-- **Latin-1 → UTF-8: every store ends within `utf8_length_from_latin1`.** -/
theorem latin1to8_safe (pick : Nat → Nat) (xs : List Nat) {a n : Nat}
    (h : Store 0 (latin1to8Step pick) xs () a n) : a + n ≤ L (xs.map wL8) :=
  store_le (latin1to8_fits pick) trivial rfl h

/-! ## UTF-32 → UTF-16 (src/rvv/rvv_utf32_to.inl.cpp:196-306) -/

/-- A surrogate code point. The C++ tests `v + 0xFFFF2000 > 0xFFFFF7FF` on
32-bit lanes (:208-210), and `(v & 0xF800) == 0xD800` on 16-bit lanes; both
are this range. -/
def isSurV (v : Nat) : Bool := decide (0xD800 ≤ v ∧ v < 0xE000)

/-- `utf16_length_from_utf32` per unit (src/rvv/rvv_length_from.inl.cpp:153-163). -/
def w16of32 (u : Nat) : Nat := 1 + ind (decide (0xFFFF < u))

/-- `validate = true`: `rvv_convert_utf32_to_utf16_with_errors` (rvv_utf32_to.inl.cpp:205-236; the
plain conversions call it, :241-251). `vl = vsetvl_e32m4(len)` (:206). No lane
above 0xFFFF (:213): return on a surrogate (:214-216), else `vse16 ... vl`
(:218-221). Otherwise return on a lane above 0x10FFFF or a surrogate
(:225-234), else `rvv_utf32_store_utf16_m4` (:236), which stores and returns
its `vcpop`, the advance. `validate = false`: `rvv_convert_valid_utf32_to_utf16`
(:277-288), the same without the returns. -/
def utf32to16Step (validate : Bool) (pick : Nat → Nat) : Unit → List Nat → It Unit := fun _ ys =>
  let vl := vlOf pick ys.length
  let blk := ys.take vl
  if blk.all (fun u => decide (u ≤ 0xFFFF)) then
    if validate && blk.any isSurV then ⟨0, [], 0, (), true⟩ else ⟨vl, [(0, vl)], vl, (), false⟩
  else if validate && (blk.any (fun u => decide (0x10FFFF < u)) || blk.any isSurV) then
    ⟨0, [], 0, (), true⟩
  else
    let k := L (blk.map fun u => (helperLane u).length)
    ⟨vl, [(0, k)], k, (), false⟩

theorem utf32to16_fits (validate : Bool) (pick : Nat → Nat) :
    Fits 0 (utf32to16Step validate pick) w16of32 (fun _ => 0) (fun _ _ => True) := by
  intro s ys _ _
  have hvl := vlOf_le pick ys.length
  unfold utf32to16Step; dsimp only
  generalize vlOf pick ys.length = vl at hvl
  have h1 : vl ≤ L ((ys.take vl).map w16of32) := by
    have := len_le_L (ys.take vl) w16of32 (fun x _ => by simp [w16of32])
    rwa [take_vl_len ys hvl] at this
  have h2 : L ((ys.take vl).map fun u => (helperLane u).length) ≤ L ((ys.take vl).map w16of32) :=
    L_map_le _ _ _ (fun u _ => helper_le u)
  split
  · split
    · simp
    · simp only [List.mem_singleton, forall_eq]; simp [-List.map_take]; omega
  · split
    · simp
    · simp only [List.mem_singleton, forall_eq]; simp [-List.map_take]; omega

/-- **UTF-32 → UTF-16 (all variants): every store ends within
`utf16_length_from_utf32`.** -/
theorem utf32to16_safe (validate : Bool) (pick : Nat → Nat) (xs : List Nat) {a n : Nat}
    (h : Store 0 (utf32to16Step validate pick) xs () a n) : a + n ≤ L (xs.map w16of32) :=
  store_le (utf32to16_fits validate pick) trivial rfl h

/-! ## UTF-16 → UTF-32 (src/rvv/rvv_utf16_to.inl.cpp:257-367) -/

/-- The high/low mismatch test of :315-325 on the shortened block. -/
def diffFail (ys : List Nat) (n : Nat) : Bool :=
  (List.range n).any fun i => hiS (ys.getD i 0) != loS (ys.getD (i + 1) 0)

/-- `vl = vsetvl_e16m2(len)` (:272). No surrogate (:276-288): `vse32 ... vl`,
advance `vl`. Otherwise: return if the first unit is low (:291-294); `vl' =
vsetvl_e16m2(vl - 1)` (:298, any `vl' ≤ vl - 1`), return if 0 (:299-301);
return on a high/low mismatch (:315-325); store the `vcpop` of "not a low
surrogate" over `vl'` lanes (:311-313, :351-353), advance by it (:357),
consume `vl'` (:355-356), plus the unit `src[vl']` when it is low
(:327, :359-363). The values are those after the byte flip (:274), so both
endiannesses are covered. -/
def utf16to32Step (pick pick2 : Nat → Nat) : Unit → List Nat → It Unit := fun _ ys =>
  let vl := vlOf pick ys.length
  let blk := ys.take vl
  if !(blk.any isSurV) then ⟨vl, [(0, vl)], vl, (), false⟩
  else if loS (ys.getD 0 0) then ⟨0, [], 0, (), true⟩
  else
    let vl2 := min (pick2 (vl - 1)) (vl - 1)
    if vl2 = 0 then ⟨0, [], 0, (), true⟩
    else if diffFail ys vl2 then ⟨0, [], 0, (), true⟩
    else
      let k := L ((ys.take vl2).map fun v => ind (!loS v))
      ⟨vl2 + ind (loS (ys.getD vl2 0)), [(0, k)], k, (), false⟩

theorem utf16to32_fits (pick pick2 : Nat → Nat) :
    Fits 0 (utf16to32Step pick pick2) w32v (fun _ => 0) (fun _ _ => True) := by
  intro s ys _ _
  have hvl := vlOf_le pick ys.length
  unfold utf16to32Step; dsimp only
  generalize vlOf pick ys.length = vl at hvl
  split
  · rename_i hno
    have hns : ∀ x ∈ ys.take vl, w32v x = 1 := by
      intro x hx
      have : isSurV x = false := by
        cases h : isSurV x
        · rfl
        · simp only [Bool.not_eq_true', List.any_eq_false] at hno
          exact absurd h (by simpa using hno x hx)
      simp [w32v, loS]; simp [isSurV] at this; omega
    have : L ((ys.take vl).map w32v) = vl := by
      rw [L_map_congr _ _ _ hns, L_map_one, take_vl_len ys hvl]
    simp only [List.mem_singleton, forall_eq]; simp [this, -List.map_take]
  · split
    · simp
    · split
      · simp
      · split
        · simp
        · rename_i _ _ _ _
          generalize min (pick2 (vl - 1)) (vl - 1) = vl2
          have he : L ((ys.take vl2).map fun v => ind (!loS v)) = L ((ys.take vl2).map w32v) :=
            L_map_congr _ _ _ (fun v _ => by simp [w32v, ind]; cases loS v <;> simp)
          have hm := L_take_mono w32v ys (Nat.le_add_right vl2 (ind (loS (ys.getD vl2 0))))
          generalize ind (loS (ys.getD vl2 0)) = e at hm ⊢
          simp only [List.mem_singleton, forall_eq]; simp [-List.map_take]; omega

/-- **UTF-16 → UTF-32 (all variants): every store ends within
`utf32_length_from_utf16`** (low surrogates weigh 0). -/
theorem utf16to32_safe (pick pick2 : Nat → Nat) (xs : List Nat) {a n : Nat}
    (h : Store 0 (utf16to32Step pick pick2) xs () a n) : a + n ≤ L (xs.map w32v) :=
  store_le (utf16to32_fits pick pick2) trivial rfl h

/-! ## UTF-16 → UTF-8 (src/rvv/rvv_utf16_to.inl.cpp:72-217) -/

/-- `utf8_length_from_utf16` per unit (src/rvv/rvv_length_from.inl.cpp:88-103:
1, plus 1 above 0x7F, plus 1 above 0x7FF outside the surrogates). -/
abbrev w8 : Nat → Nat := Arch.Icelake.w8

theorem w8_eq (v : Nat) : w8 v
    = 1 + ind (decide (0x7F < v)) + ind (decide (0x7FF < v) && !isSurV v) := by
  unfold w8 Arch.Icelake.w8 ind isSurV
  by_cases h1 : v < 0x80
  · have : ¬ 0x7F < v := by omega
    have : ¬ 0x7FF < v := by omega
    simp [*]
  · have h7 : 0x7F < v := by omega
    by_cases h2 : v < 0x800
    · have : ¬ 0x7FF < v := by omega
      simp [h1, h2, h7, this]
    · have h11 : 0x7FF < v := by omega
      by_cases h3 : 0xD800 ≤ v ∧ v < 0xE000
      · simp [h1, h2, h3, h7, h11]
      · simp [h1, h2, h3, h7, h11]

theorem w8_sur (v : Nat) (h : isSurV v = true) : w8 v = 2 := by
  rw [w8_eq]
  have a : 0x7F < v := by simp [isSurV] at h; omega
  simp [ind, a, h]

/-- The scalar pair loop (rvv_utf16_to.inl.cpp:179-212) on the units after the vector prefix:
stop (`break`) at a non-surrogate (:181-186) or at the end; return at a lone
unit (:189-191), a unit that is not high (:192-195) or a second unit that is
not low (:196-199); otherwise write 4 bytes (:205-208) and consume 2.
Result: pairs written, and whether the kernel returns. -/
def pairs : List Nat → Nat × Bool
  | [] => (0, false)
  | [w] => (0, isSurV w)
  | w :: w2 :: r =>
    if !isSurV w then (0, false) else if !hiS w then (0, true)
    else if !loS w2 then (0, true) else ((pairs r).1 + 1, (pairs r).2)

theorem pairs_L : ∀ ys : List Nat, L ((ys.take (2 * (pairs ys).1)).map w8) = 4 * (pairs ys).1
  | [] => by simp [pairs]
  | [w] => by simp [pairs]
  | w :: w2 :: r => by
    unfold pairs
    by_cases h1 : isSurV w = true
    · by_cases h2 : hiS w = true
      · by_cases h3 : loS w2 = true
        · simp only [h1, h2, h3, Bool.not_true, Bool.false_eq_true, ite_false]
          rw [show 2 * ((pairs r).1 + 1) = 2 * (pairs r).1 + 1 + 1 by omega]
          simp only [List.take_succ_cons, List.map_cons, L_cons]
          rw [pairs_L r]
          have e1 : w8 w = 2 := w8_sur w h1
          have e2 : w8 w2 = 2 := w8_sur w2 (by simp [loS] at h3; simp [isSurV]; omega)
          omega
        · simp [h1, h2, h3]
      · simp [h1, h2]
    · simp [h1]

theorem w8_lane (v : Nat) (hs : isSurV v = false) :
    1 + ind (decide (0x7F < v)) + ind (decide (0x7FF < v)) = w8 v := by
  rw [w8_eq, hs]; simp

/-- `vl = vsetvl_e16m2(n)` (:83). ASCII (:89-95): `vse8 ... vl`. All below
0x800 (:97-129): store the `vcpop` of the compress mask (`lane12` per lane,
:118-125). Otherwise (:131-176): `pre` is the prefix before the first
surrogate (:131-135), stored with the `vcpop` of its compress mask (`lane123`
per lane, :168-173) when non-empty (:137); then the scalar pairs (`pairs`,
:179-212), 4 bytes each at the cursor. The values are those after the byte
flip (:86, :180, :196), so both endiannesses are covered; `convert`,
`_with_errors`, `convert_valid` (:219-253) and the two conversions with
replacement (src/rvv/implementation.cpp:125-147) run this loop. -/
def utf16to8Step (pick : Nat → Nat) : Unit → List Nat → It Unit := fun _ ys =>
  let vl := vlOf pick ys.length
  let blk := ys.take vl
  if blk.all (fun v => decide (v ≤ 0x7F)) then ⟨vl, [(0, vl)], vl, (), false⟩
  else if blk.all (fun v => decide (v ≤ 0x7FF)) then
    let k := L (blk.map fun v => lane12 (decide (0x7F < v)) 0x3F1F v)
    ⟨vl, [(0, k)], k, (), false⟩
  else
    let pre := blk.takeWhile (fun v => !isSurV v)
    let k := L (pre.map fun v => lane123 (decide (0x7F < v)) (decide (0x7FF < v)) v)
    let q := pairs (ys.drop pre.length)
    ⟨pre.length + 2 * q.1,
      (if 0 < pre.length then [(0, k)] else []) ++ (List.range q.1).map (fun j => (k + 4 * j, 4)),
      k + 4 * q.1, (), q.2⟩

theorem w8_ge (v : Nat) : 1 ≤ w8 v := by
  rw [w8_eq]; omega

theorem utf16to8_fits (pick : Nat → Nat) :
    Fits 0 (utf16to8Step pick) w8 (fun _ => 0) (Bnd 65536) := by
  intro s ys hG _
  have hvl := vlOf_le pick ys.length
  unfold utf16to8Step; dsimp only
  generalize hvv : vlOf pick ys.length = vl at hvl
  have hlen : (ys.take vl).length = vl := take_vl_len ys hvl
  split
  · have := len_le_L (ys.take vl) w8 (fun x _ => w8_ge x)
    rw [hlen] at this
    simp [-List.map_take]
    exact ⟨this, fun x hx => hG x (List.mem_of_mem_drop hx)⟩
  split
  · rename_i _ h12
    have hk : L ((ys.take vl).map fun v => lane12 (decide (0x7F < v)) 0x3F1F v)
        ≤ L ((ys.take vl).map w8) := by
      apply L_map_le
      intro v hv
      have hv' : v ≤ 0x7FF := by
        have := List.all_eq_true.mp h12 v hv; simpa using this
      rw [lane12_eq 0x3F1F v (by decide) (by omega), w8_eq]; omega
    simp [-List.map_take]
    exact ⟨hk, fun x hx => hG x (List.mem_of_mem_drop hx)⟩
  · generalize hpre : (ys.take vl).takeWhile (fun v => !isSurV v) = pre
    have htp : ys.take pre.length = pre := by rw [← hpre]; exact take_takeWhile ys vl _
    have hk : L (pre.map fun v => lane123 (decide (0x7F < v)) (decide (0x7FF < v)) v)
        ≤ L (pre.map w8) := by
      apply L_map_le
      intro v hv
      have hs : isSurV v = false := by
        have := mem_takeWhile (hpre ▸ hv); simpa using this
      have hb : v < 65536 := hG v (List.mem_of_mem_take (mem_takeWhile_sub (hpre ▸ hv)))
      rw [lane123_eq v hb, w8_lane v hs]; exact Nat.le_refl _
    have hq := pairs_L (ys.drop pre.length)
    have hL : L ((ys.take (pre.length + 2 * (pairs (ys.drop pre.length)).1)).map w8)
        = L (pre.map w8) + 4 * (pairs (ys.drop pre.length)).1 := by
      rw [L_map_take_add, htp, hq]
    generalize L (pre.map fun v => lane123 (decide (0x7F < v)) (decide (0x7FF < v)) v) = k at hk
    rw [hL]
    refine ⟨?_, fun _ => ⟨by simp; omega, bnd_drop hG _⟩⟩
    intro p hp
    simp only [List.mem_append, List.mem_map, List.mem_range] at hp
    rcases hp with hp | ⟨j, hj, rfl⟩
    · split at hp
      · simp at hp; subst hp; simp; omega
      · simp at hp
    · simp; omega

/-- **UTF-16 → UTF-8 (all variants, both endiannesses): every store ends
within `utf8_length_from_utf16`.** -/
theorem utf16to8_safe (pick : Nat → Nat) (xs : List Nat) (hx : ∀ x ∈ xs, x < 65536)
    {a n : Nat} (h : Store 0 (utf16to8Step pick) xs () a n) : a + n ≤ L (xs.map w8) :=
  store_le (utf16to8_fits pick) hx rfl h

/-- With replacement: the buffer holds `utf8_length_from_utf16_with_replacement`
units, at least `utf8_length_from_utf16` (`Replacement.std_le_repl`), so each
call of the kernel on a suffix stays within the replacement length of that
suffix. -/
theorem utf16to8_safe_repl (pick : Nat → Nat) (xs : List Nat) (hx : ∀ x ∈ xs, x < 65536)
    {a n : Nat} (h : Store 0 (utf16to8Step pick) xs () a n) :
    a + n ≤ Replacement.countRepl xs := by
  rw [Replacement.countRepl_eq]
  exact Nat.le_trans (utf16to8_safe pick xs hx h) (Replacement.std_le_repl xs)

/-! ## UTF-32 → UTF-8 (src/rvv/rvv_utf32_to.inl.cpp:33-193) -/

/-- `utf8_length_from_utf32` per unit (src/rvv/rvv_length_from.inl.cpp:120-133). -/
def w8of32 (u : Nat) : Nat :=
  1 + ind (decide (0x7F < u)) + ind (decide (0x7FF < u)) + ind (decide (0xFFFF < u))

/-- The scalar loop (rvv_utf32_to.inl.cpp:157-169): stop at a unit below 0x10000 or at the end;
return at a unit above 0x10FFFF (:161-162); otherwise write 4 bytes
(:163-166). Result: units written, and whether the kernel returns. -/
def quads : List Nat → Nat × Bool
  | [] => (0, false)
  | w :: r =>
    if w < 0x10000 then (0, false) else if 0x10FFFF < w then (0, true)
    else ((quads r).1 + 1, (quads r).2)

theorem quads_L : ∀ ys : List Nat, L ((ys.take (quads ys).1).map w8of32) = 4 * (quads ys).1
  | [] => by simp [quads]
  | w :: r => by
    unfold quads
    by_cases h1 : w < 0x10000
    · simp [h1]
    · by_cases h2 : 0x10FFFF < w
      · simp [h1, h2]
      · simp only [h1, h2, ite_false, List.take_succ_cons, List.map_cons, L_cons]
        rw [quads_L r]
        have : w8of32 w = 4 := by
          have a : 0x7F < w := by omega
          have b : 0x7FF < w := by omega
          have c : 0xFFFF < w := by omega
          simp [w8of32, ind, a, b, c]
        omega

/-- `with_validation` is `validate`: `convert_utf32_to_utf8_with_errors` and
the plain conversion through it (:175-185) take `true`, `convert_valid`
(:187-192) `false`. `vl = vsetvl_e32m4(n)` (:46). ASCII (:52-58): `vse8 ...
vl`. All below 0x800 (:62-92): the `vcpop` of the compress mask (`lane12` on
the narrowed lane `vn`, :50, :81-88). Otherwise: with validation, return on a
lane above 0x10FFFF or a surrogate (:94-108); `pre` is the prefix before the
first lane above 0xFFFF (:110-113), stored with the `vcpop` of its compress
mask (`lane123`, :146-151) when non-empty (:115); then the scalar loop
(`quads`), which runs whenever `tail` is nonzero, that is always
(`tail = vl - first`, :112, is `vl + 1` when `first = -1`). -/
def utf32to8Step (validate : Bool) (pick : Nat → Nat) : Unit → List Nat → It Unit := fun _ ys =>
  let vl := vlOf pick ys.length
  let blk := ys.take vl
  if blk.all (fun v => decide (v ≤ 0x7F)) then ⟨vl, [(0, vl)], vl, (), false⟩
  else if blk.all (fun v => decide (v ≤ 0x7FF)) then
    let k := L (blk.map fun v => lane12 (decide (0x7F < v)) 0x3F3F (v % 65536))
    ⟨vl, [(0, k)], k, (), false⟩
  else if validate && blk.any (fun v => decide (0x10FFFF < v) || isSurV v) then
    ⟨0, [], 0, (), true⟩
  else
    let pre := blk.takeWhile (fun v => decide (v ≤ 0xFFFF))
    let k := L (pre.map fun v => lane123 (decide (0x7F < v)) (decide (0x7FF < v)) (v % 65536))
    let q := quads (ys.drop pre.length)
    ⟨pre.length + q.1,
      (if 0 < pre.length then [(0, k)] else []) ++ (List.range q.1).map (fun j => (k + 4 * j, 4)),
      k + 4 * q.1, (), q.2⟩

theorem w8of32_ge (v : Nat) : 1 ≤ w8of32 v := by simp [w8of32]; omega

theorem utf32to8_fits (validate : Bool) (pick : Nat → Nat) :
    Fits 0 (utf32to8Step validate pick) w8of32 (fun _ => 0) (fun _ _ => True) := by
  intro s ys _ _
  have hvl := vlOf_le pick ys.length
  unfold utf32to8Step; dsimp only
  generalize hvv : vlOf pick ys.length = vl at hvl
  have hlen : (ys.take vl).length = vl := take_vl_len ys hvl
  split
  · have := len_le_L (ys.take vl) w8of32 (fun x _ => w8of32_ge x)
    rw [hlen] at this
    simpa [-List.map_take] using this
  split
  · rename_i _ h12
    have hk : L ((ys.take vl).map fun v => lane12 (decide (0x7F < v)) 0x3F3F (v % 65536))
        ≤ L ((ys.take vl).map w8of32) := by
      apply L_map_le
      intro v hv
      have hv' : v ≤ 0x7FF := by
        have := List.all_eq_true.mp h12 v hv; simpa using this
      have hm : v % 65536 = v := by omega
      rw [hm, lane12_eq 0x3F3F v (by decide) (by omega)]
      simp only [w8of32]; omega
    simpa [-List.map_take] using hk
  · split
    · simp
    · generalize hpre : (ys.take vl).takeWhile (fun v => decide (v ≤ 0xFFFF)) = pre
      have htp : ys.take pre.length = pre := by rw [← hpre]; exact take_takeWhile ys vl _
      have hk : L (pre.map fun v => lane123 (decide (0x7F < v)) (decide (0x7FF < v)) (v % 65536))
          ≤ L (pre.map w8of32) := by
        apply L_map_le
        intro v hv
        have hs : v ≤ 0xFFFF := by
          have := mem_takeWhile (hpre ▸ hv); simpa using this
        have hm : v % 65536 = v := by omega
        rw [hm, lane123_eq v (by omega)]
        simp only [w8of32]
        have : ind (decide (0xFFFF < v)) = 0 := by simp [ind]; omega
        omega
      have hq := quads_L (ys.drop pre.length)
      have hL : L ((ys.take (pre.length + (quads (ys.drop pre.length)).1)).map w8of32)
          = L (pre.map w8of32) + 4 * (quads (ys.drop pre.length)).1 := by
        rw [L_map_take_add, htp, hq]
      generalize L (pre.map fun v => lane123 (decide (0x7F < v)) (decide (0x7FF < v)) (v % 65536)) = k at hk
      rw [hL]
      refine ⟨?_, fun _ => ⟨by simp; omega, trivial⟩⟩
      intro p hp
      simp only [List.mem_append, List.mem_map, List.mem_range] at hp
      rcases hp with hp | ⟨j, hj, rfl⟩
      · split at hp
        · simp at hp; subst hp; simp; omega
        · simp at hp
      · simp; omega

/-- **UTF-32 → UTF-8 (all variants): every store ends within
`utf8_length_from_utf32`.** -/
theorem utf32to8_safe (validate : Bool) (pick : Nat → Nat) (xs : List Nat) {a n : Nat}
    (h : Store 0 (utf32to8Step validate pick) xs () a n) : a + n ≤ L (xs.map w8of32) :=
  store_le (utf32to8_fits validate pick) trivial rfl h

/-! ## UTF-8 → Latin-1 (src/rvv/rvv_utf8_to.inl.cpp:272-360)

The buffer holds `latin1_length_from_utf8` bytes (src/rvv/rvv_length_from.inl.cpp:21-38:
one per byte that is not a continuation byte, `w01`). The kernels store the
ASCII and continuation bytes of a block, so the cursor lags `L` of the consumed
bytes by one when the last consumed byte is a lead byte: the lag `g last` of
the state `last = src[-1]`. -/

/-- `0xBF < b`: a lead byte (the `leading0` of rvv_utf8_to.inl.cpp:302). -/
def lead (b : Nat) : Bool := decide (0xBF < b)

/-- The check of :300-315: `leading0` (the previous byte, `last` for lane 0,
is `> 0xBF`) must equal `trailing1` (the byte is a continuation byte), and a
lead byte must be `0xC2` or `0xC3` (`tobig`: `(v0 ^ 0xC2) > 1`). `bad` is true
when some lane fails. -/
def bad : Nat → List Nat → Bool
  | _, [] => false
  | p, x :: r => (lead p != isCont x) || (lead p && decide (1 < (p ^^^ 0xC2))) || bad x r

/-- Every continuation byte follows a lead byte, and every lead byte is followed
by a continuation byte (what the check enforces, and what valid UTF-8 that
Latin-1 represents satisfies). -/
def paired : Nat → List Nat → Bool
  | _, [] => true
  | p, x :: r => (isCont x == lead p) && paired x r

theorem paired_of_bad : ∀ (p : Nat) (l : List Nat), bad p l = false → paired p l
  | _, [] => fun _ => rfl
  | p, x :: r => by
    intro h
    simp only [bad, Bool.or_eq_false_iff] at h
    simp only [paired, Bool.and_eq_true]
    refine ⟨?_, paired_of_bad x r h.2⟩
    have := h.1.1; cases hl : lead p <;> cases hc : isCont x <;> simp_all

theorem paired_take : ∀ (n p : Nat) (l : List Nat), paired p l = true → paired p (l.take n) = true
  | 0, _, _, _ => rfl
  | _ + 1, _, [], _ => rfl
  | n + 1, p, x :: r, h => by
    simp only [paired, Bool.and_eq_true] at h
    simp only [List.take_succ_cons, paired, Bool.and_eq_true]
    exact ⟨h.1, paired_take n x r h.2⟩

theorem paired_drop : ∀ (n p : Nat) (l : List Nat), paired p l = true →
    paired ((l.take n).getLastD p) (l.drop n) = true
  | 0, _, _, h => by simpa using h
  | _ + 1, _, [], _ => rfl
  | n + 1, p, x :: r, h => by
    simp only [paired, Bool.and_eq_true] at h
    simp only [List.take_succ_cons, List.drop_succ_cons, List.getLastD_cons]
    exact paired_drop n x r h.2

/-- The telescoping count: the bytes below 0xC0 of a paired block, plus the
lag after it, are the lag before it plus `L` of the block. -/
theorem paired_count : ∀ (p : Nat) (l : List Nat), paired p l = true →
    L (l.map fun b => ind (decide (b < 0xC0))) + ind (lead (l.getLastD p))
      = ind (lead p) + L (l.map w01)
  | p, [] => by simp
  | p, x :: r => by
    intro h
    simp only [paired, Bool.and_eq_true, beq_iff_eq] at h
    simp only [List.map_cons, L_cons, List.getLastD_cons]
    have ih := paired_count x r h.2
    have hx : ind (decide (x < 0xC0)) + ind (lead x) = ind (lead p) + w01 x := by
      have e := h.1
      by_cases h1 : x < 0x80
      · have c1 : isCont x = false := by simp [isCont]; omega
        have c2 : lead x = false := by simp [lead]; omega
        have c3 : decide (x < 0xC0) = true := by simp; omega
        rw [c1] at e
        simp [w01, c1, c2, c3, ← e, ind]
      · by_cases h2 : x < 0xC0
        · have c1 : isCont x = true := by simp [isCont]; omega
          have c2 : lead x = false := by simp [lead]; omega
          have c3 : decide (x < 0xC0) = true := by simp; omega
          rw [c1] at e
          simp [w01, c1, c2, c3, ← e, ind]
        · have c1 : isCont x = false := by simp [isCont]; omega
          have c2 : lead x = true := by simp [lead]; omega
          have c3 : decide (x < 0xC0) = false := by simp; omega
          rw [c1] at e
          simp [w01, c1, c2, c3, ← e, ind]
    omega

theorem paired_ascii : ∀ (p : Nat) (l : List Nat), lead p = false → (∀ x ∈ l, x < 0x80) →
    paired p l = true
  | _, [], _, _ => rfl
  | p, x :: r, hp, h => by
    have hx := h x (by simp)
    simp only [paired, Bool.and_eq_true, beq_iff_eq]
    refine ⟨?_, paired_ascii x r ?_ (fun y hy => h y (by simp [hy]))⟩
    · simp [isCont, hp]; omega
    · simp [lead]; omega

/-- `validate = true`: `convert_utf8_to_latin1` (:273-329; `_with_errors`,
:331-337, calls it and falls back to the scalar function). `vl =
vsetvl_e8m2(len)` (:279); `vlOut` = ASCII bytes (:282-284). Not all ASCII
(:293): `vlOut` = bytes below 0xC0 (:296-298), return on the check (:300-315),
store the compressed `vlOut` bytes (:317-319, :324). All ASCII: return if
`last ≥ 0xC0` (:320-322), else store `vl` bytes (:324). Advance `vlOut`,
`last = src[-1]` (:277-278). `validate = false`: `convert_valid_utf8_to_latin1`
(:339-360), the same without the returns. -/
def utf8toLatin1Step (validate : Bool) (pick : Nat → Nat) : Nat → List Nat → It Nat := fun last ys =>
  let vl := vlOf pick ys.length
  let blk := ys.take vl
  let nxt := blk.getLastD last
  if L (blk.map fun b => ind (decide (b < 0x80))) != vl then
    let vlOut := L (blk.map fun b => ind (decide (b < 0xC0)))
    if validate && bad last blk then ⟨0, [], 0, last, true⟩
    else ⟨vl, [(0, vlOut)], vlOut, nxt, false⟩
  else if validate && decide (0xC0 ≤ last) then ⟨0, [], 0, last, true⟩
  else ⟨vl, [(0, vl)], vl, nxt, false⟩

/-- The contract: the checking kernel needs nothing; `convert_valid` needs the
input to be `paired` (from `last = 0`). -/
def latin1G (validate : Bool) : Nat → List Nat → Prop := fun last ys =>
  validate = true ∨ paired last ys = true

theorem utf8toLatin1_fits (validate : Bool) (pick : Nat → Nat) :
    Fits 0 (utf8toLatin1Step validate pick) w01 (fun last => ind (lead last)) (latin1G validate) := by
  intro last ys hG hT
  have hvl := vlOf_le pick ys.length
  have hvl1 : 1 ≤ vlOf pick ys.length := by
    simp only [vlOf]; omega
  unfold utf8toLatin1Step; dsimp only
  generalize vlOf pick ys.length = vl at hvl hvl1
  have hlen : (ys.take vl).length = vl := take_vl_len ys hvl
  have hGnext : paired last ys = true → latin1G validate ((ys.take vl).getLastD last) (ys.drop vl) :=
    fun h => Or.inr (paired_drop vl last ys h)
  split
  · split
    · simp
    · rename_i hb
      have hp : paired last (ys.take vl) = true := by
        rcases hG with hv | hp
        · subst hv; exact paired_of_bad _ _ (by simpa using hb)
        · exact paired_take vl last ys hp
      have hc := paired_count last (ys.take vl) hp
      have hGn := hGnext
      generalize (ys.take vl).getLastD last = nl at hc hGn ⊢
      refine ⟨?_, fun _ => ⟨?_, ?_⟩⟩
      · simp [-List.map_take]; omega
      · simp [-List.map_take]; omega
      · rcases hG with hv | hp'
        · exact Or.inl hv
        · exact hGn hp'
  · split
    · simp
    · rename_i hasc hl
      have hall : ∀ x ∈ ys.take vl, x < 0x80 := by
        have h := L_ind_eq_len (l := ys.take vl) (p := fun b => decide (b < 0x80))
          (by rw [hlen]; simpa using hasc)
        intro x hx; simpa using h x hx
      have hp : paired last (ys.take vl) = true := by
        rcases hG with hv | hp
        · subst hv
          apply paired_ascii _ _ _ hall
          simp [lead]; simp at hl; omega
        · exact paired_take vl last ys hp
      have hc := paired_count last (ys.take vl) hp
      have h80 : L ((ys.take vl).map fun b => ind (decide (b < 0xC0))) = vl := by
        rw [L_map_congr _ _ (fun _ => 1) (fun x hx => by
          have := hall x hx; simp [ind]; omega), L_map_one, hlen]
      rw [h80] at hc
      have hGn := hGnext
      generalize (ys.take vl).getLastD last = nl at hc hGn ⊢
      refine ⟨?_, fun _ => ⟨?_, ?_⟩⟩
      · simp [-List.map_take]; omega
      · simp [-List.map_take]; omega
      · rcases hG with hv | hp'
        · exact Or.inl hv
        · exact hGn hp'

/-- **UTF-8 → Latin-1, checking kernel: every store ends within
`latin1_length_from_utf8`, for every input.** -/
theorem utf8toLatin1_safe (pick : Nat → Nat) (xs : List Nat) {a n : Nat}
    (h : Store 0 (utf8toLatin1Step true pick) xs 0 a n) : a + n ≤ L (xs.map w01) :=
  store_le (utf8toLatin1_fits true pick) (Or.inl rfl) (by simp [ind, lead]) h

/-- Valid UTF-8 that Latin-1 represents (`ValidLatin1.LV`, the contract of
`convert_valid_utf8_to_latin1`) is `paired`. -/
theorem LV_paired : ∀ {xs : List Nat}, ValidLatin1.LV xs → ∀ p, lead p = false → paired p xs = true
  | _, .nil, _, _ => rfl
  | _, .one a rest ha h, p, hp => by
    simp only [paired, Bool.and_eq_true, beq_iff_eq]
    refine ⟨?_, LV_paired h a (by simp [lead]; omega)⟩
    simp [isCont, hp]; omega
  | _, .two c d rest hc hd h, p, hp => by
    simp only [paired, Bool.and_eq_true, beq_iff_eq]
    refine ⟨?_, ?_, LV_paired h d (by simp [lead]; omega)⟩
    · simp [isCont, hp]; omega
    · have e1 : isCont d = true := by simp [isCont]; omega
      have e2 : lead c = true := by simp [lead]; omega
      rw [e1, e2]

/-- **`convert_valid_utf8_to_latin1`: every store ends within
`latin1_length_from_utf8`, on every input of its contract** (valid UTF-8 that
Latin-1 represents, `ValidLatin1.LV`). -/
theorem validUtf8toLatin1_safe (pick : Nat → Nat) (xs : List Nat) (hv : ValidLatin1.LV xs)
    {a n : Nat} (h : Store 0 (utf8toLatin1Step false pick) xs 0 a n) : a + n ≤ L (xs.map w01) :=
  store_le (utf8toLatin1_fits false pick) (Or.inr (LV_paired hv 0 (by simp [lead])))
    (by simp [ind, lead]) h

/-! ## UTF-8 → UTF-16 / UTF-32 (src/rvv/rvv_utf8_to.inl.cpp:2-268, the template)

`rvv_utf8_to_common<Tdst, bflip, validate>`: inputs shorter than 32 bytes go to
the scalar function (:17-18); the loop runs on `n = len - 3` bytes (:29-30)
and reads the three bytes after each block (`next0..2`, :56-58), all inside
the input; then the reparse step and the scalar tail (:251-267).

The buffer holds `utf16_length_from_utf8` units (`w16`: 0 for a continuation
byte, 2 from 0xF0, else 1; src/rvv/rvv_length_from.inl.cpp:137-149) or
`utf32_length_from_utf8` units (`w01`, :28-38). Every output lane comes from a
non-continuation byte of the block (`m`, rvv_utf8_to.inl.cpp:105-107; `vcompress`, :110-111,
:137, :166-167), so the output of a block is a sum over those bytes. A lane
writes at most the weight of its byte: one unit, or for UTF-16 on the 4-byte
path the `rvv_utf32_store_utf16_m4` count, which is 1 below 0xF0
(`c1234_bmp`) and at most 2. The state is the last lane written: its byte,
its unit count and whether it wrote two units starting with a high surrogate;
its lag (weight minus count) is what the reparse step needs. -/

/-- The weight of the destination format. -/
def wU8 (is16 : Bool) : Nat → Nat := if is16 then w16 else w01

theorem wU8_cont (is16 : Bool) (b : Nat) (h : isCont b = true) : wU8 is16 b = 0 := by
  cases is16
  · show w01 b = 0; simp [w01, h]
  · show w16 b = 0; have h' := h; simp [isCont] at h'; simp [w16, h']

theorem wU8_nc (is16 : Bool) (b : Nat) (h : isCont b = false) : 1 ≤ wU8 is16 b := by
  have h' := h
  simp [isCont] at h'
  cases is16
  · show 1 ≤ w01 b; simp [w01, h]
  · show 1 ≤ w16 b; unfold w16; split
    · omega
    · split <;> omega

theorem wU8_le2 (is16 : Bool) (b : Nat) : wU8 is16 b ≤ 2 := by
  cases is16
  · show w01 b ≤ 2; unfold w01; split <;> omega
  · show w16 b ≤ 2; unfold w16; split
    · omega
    · split <;> omega

/-- Each byte with the next three (the lanes of `v0`, `v1`, `v2`, `v3`,
:52, :76-78: `vslide1down` with `next0..2`). -/
def win : List Nat → List (Nat × Nat × Nat × Nat)
  | [] => []
  | b :: r => (b, r.getD 0 0, r.getD 1 0, r.getD 2 0) :: win r

theorem win_fst : ∀ l : List Nat, (win l).map Prod.fst = l
  | [] => rfl
  | b :: r => by simp [win, win_fst r]

/-! ### The 4-byte path lane (:188-226) -/

/-- `shift = max((c1 >> 4) - 10, 0)`, with 12 mapped to 3 (:197-200). -/
def rvvShift8 (b : Nat) : Nat := if b >>> 4 = 12 then 3 else (b >>> 4) - 10

/-- The code point of the lane: the prefix removed from `c1` by the 8-bit
shifts (:201-202), the fields combined by widening multiply-adds (:204-209),
then shifted right by `6 * (3 - max(shift - 2, 0))` (:212-220). -/
def c1234 (b x2 x3 x4 : Nat) : Nat :=
  let c2 := x2 &&& 0x3F
  let c3 := x3 &&& 0x3F
  let c4 := x4 &&& 0x3F
  let sh := rvvShift8 b
  let c1 := ((b <<< sh) % 256) >>> sh
  let c34 := c3 * 64 + c4
  let c12 := c1 * 64 + c2
  (c12 * 4096 + c34) >>> ((3 - (sh - 2)) * 6)

/-- A non-continuation byte below 0xF0 decodes to at most 0xFFFF, whatever the
three following bytes (the existing `Arch.Rvv.rvv_bmp_lead` checks the
maximal continuation fields). -/
theorem c1234_bmp (b x2 x3 x4 : Nat) (hb : b < 0xF0) (hnc : isCont b = false) :
    c1234 b x2 x3 x4 ≤ 0xFFFF := by
  simp [isCont] at hnc
  unfold c1234
  simp only
  generalize hc2 : x2 &&& 0x3F = c2
  generalize hc3 : x3 &&& 0x3F = c3
  generalize hc4 : x4 &&& 0x3F = c4
  have h2 : c2 < 64 := by rw [← hc2]; exact Nat.lt_of_le_of_lt Nat.and_le_right (by decide)
  have h3 : c3 < 64 := by rw [← hc3]; exact Nat.lt_of_le_of_lt Nat.and_le_right (by decide)
  have h4 : c4 < 64 := by rw [← hc4]; exact Nat.lt_of_le_of_lt Nat.and_le_right (by decide)
  have hs : b >>> 4 = b / 16 := by rw [Nat.shiftRight_eq_div_pow]
  have cases : b < 0x80 ∨ b / 16 = 12 ∨ b / 16 = 13 ∨ b / 16 = 14 := by omega
  rcases cases with h | h | h | h
  · have : rvvShift8 b = 0 := by simp only [rvvShift8, hs]; split <;> omega
    rw [this]; simp only [Nat.shiftLeft_eq, Nat.shiftRight_eq_div_pow]
    simp; omega
  · have : rvvShift8 b = 3 := by simp only [rvvShift8, hs, h]; rfl
    rw [this]; simp only [Nat.shiftLeft_eq, Nat.shiftRight_eq_div_pow]
    simp; omega
  · have : rvvShift8 b = 3 := by simp only [rvvShift8, hs, h]; rfl
    rw [this]; simp only [Nat.shiftLeft_eq, Nat.shiftRight_eq_div_pow]
    simp; omega
  · have : rvvShift8 b = 4 := by simp only [rvvShift8, hs, h]; rfl
    rw [this]; simp only [Nat.shiftLeft_eq, Nat.shiftRight_eq_div_pow]
    simp; omega

/-- The units the helper writes for a lane (:223-224). -/
def hunits (q : Nat × Nat × Nat × Nat) : List Nat := helperLane (c1234 q.1 q.2.1 q.2.2.1 q.2.2.2)
def hcnt (q : Nat × Nat × Nat × Nat) : Nat := (hunits q).length
/-- The lane wrote two units, the first a high surrogate. -/
def hph (q : Nat × Nat × Nat × Nat) : Bool := (hcnt q == 2) && hiS (hunits q).head!

theorem hcnt_le (q : Nat × Nat × Nat × Nat) (hnc : isCont q.1 = false) : hcnt q ≤ wU8 true q.1 := by
  by_cases hb : q.1 < 0xF0
  · rw [hcnt, hunits, helper_small _ (c1234_bmp _ _ _ _ hb hnc)]
    exact wU8_nc true _ hnc
  · have := (helper_len (c1234 q.1 q.2.1 q.2.2.1 q.2.2.2)).2
    have e : wU8 true q.1 = 2 := by
      simp [isCont] at hnc
      have a : ¬(0x80 ≤ q.1 ∧ q.1 < 0xC0) := by omega
      have b : 0xF0 ≤ q.1 := by omega
      simp [wU8, w16, a, b]
    rw [e]; exact this

/-- The state after a block: its last lane, or the previous state when the
block has no lane. -/
def upd (s : Option (Nat × Nat × Bool)) (lanes : List (Nat × Nat × Nat × Nat))
    (cnt : Nat × Nat × Nat × Nat → Nat) (ph : Nat × Nat × Nat × Nat → Bool) :
    Option (Nat × Nat × Bool) :=
  match lanes.getLast? with
  | none => s
  | some q => some (q.1, cnt q, ph q)

/-- The lag of the last lane. -/
def lag8 (is16 : Bool) : Option (Nat × Nat × Bool) → Nat
  | none => 0
  | some (b, k, _) => wU8 is16 b - k

/-- One iteration of :49-249. `vl = vsetvl_e8m2(n)` (:50); the lanes are the
non-continuation bytes of the block with their next three bytes.
* ASCII, including `next0..2` (:61-72): store `vl` units.
* `validate` and the check fails (:80-100; `chk` is arbitrary): return 0.
* `max < 0xE0` (:114-133) and `max < 0xF0` (:136-163): store `vlOut` units,
  one per lane (:107, :127-131, :156-161).
* Otherwise the lanes are split at `M1 = vsetvlmax_e8m1` (:231-246): UTF-16
  stores the helper count of each part, at the cursor and after the first
  (:223-224, :239); UTF-32 stores one unit per lane (:225-226). Advance by
  the sum (:235, :239, :245). -/
def utf8Step (is16 validate : Bool) (chk : List Nat → Bool) (pick : Nat → Nat) (M1 : Nat) :
    Option (Nat × Nat × Bool) → List Nat → It (Option (Nat × Nat × Bool)) := fun s ys =>
  let vl := vlOf pick (ys.length - 3)
  let blk := ys.take vl
  let lanes := ((win ys).take vl).filter (fun q => !isCont q.1)
  if (ys.take (vl + 3)).all (fun b => decide (b < 0x80)) then
    ⟨vl, [(0, vl)], vl, upd s lanes (fun _ => 1) (fun _ => false), false⟩
  else if validate && !chk (ys.take (vl + 3)) then ⟨0, [], 0, s, true⟩
  else if blk.all (fun b => decide (b < 0xE0)) then
    ⟨vl, [(0, lanes.length)], lanes.length, upd s lanes (fun _ => 1) (fun _ => false), false⟩
  else if blk.all (fun b => decide (b < 0xF0)) then
    ⟨vl, [(0, lanes.length)], lanes.length, upd s lanes (fun _ => 1) (fun _ => false), false⟩
  else if is16 then
    let a := L ((lanes.take M1).map hcnt)
    let b := L ((lanes.drop M1).map hcnt)
    ⟨vl, if lanes.length ≤ M1 then [(0, a)] else [(0, a), (a, b)], a + b, upd s lanes hcnt hph,
      false⟩
  else
    let a := min lanes.length M1
    ⟨vl, if lanes.length ≤ M1 then [(0, a)] else [(0, a), (a, lanes.length - a)], lanes.length,
      upd s lanes (fun _ => 1) (fun _ => false), false⟩

/-! ### Sums over the lanes -/

theorem sum_lag {α : Type} (l : List α) (f w : α → Nat) (h : ∀ q ∈ l, f q ≤ w q) :
    L (l.map f) + (match l.getLast? with | none => 0 | some q => w q - f q) ≤ L (l.map w) := by
  induction l with
  | nil => simp
  | cons x r ih =>
    have hx := h x (by simp)
    have ih' := ih (fun q hq => h q (by simp [hq]))
    cases r with
    | nil => simp; omega
    | cons y r' =>
      simp only [List.getLast?_cons_cons] at ih' ⊢
      simp only [List.map_cons, L_cons] at ih' ⊢
      omega

theorem L_filter_nc (w : Nat → Nat) (hw : ∀ b, isCont b = true → w b = 0) :
    ∀ l : List (Nat × Nat × Nat × Nat),
      L ((l.filter (fun q => !isCont q.1)).map (fun q => w q.1)) = L (l.map (fun q => w q.1))
  | [] => rfl
  | q :: r => by
    have ih := L_filter_nc w hw r
    by_cases hc : isCont q.1 = true
    · simp [List.filter_cons, hc, ih, hw _ hc]
    · simp at hc; simp [List.filter_cons, hc, ih]

theorem lanes_L (is16 : Bool) (ys : List Nat) (vl : Nat) :
    L ((((win ys).take vl).filter (fun q => !isCont q.1)).map (fun q => wU8 is16 q.1))
      = L ((ys.take vl).map (wU8 is16)) := by
  rw [L_filter_nc _ (wU8_cont is16)]
  have : ((win ys).take vl).map (fun q => wU8 is16 q.1)
      = (((win ys).map Prod.fst).take vl).map (wU8 is16) := by
    rw [← List.map_take, List.map_map]; rfl
  rw [this, win_fst]

theorem lanes_nc {ys : List Nat} {vl : Nat} {q : Nat × Nat × Nat × Nat}
    (h : q ∈ ((win ys).take vl).filter (fun q => !isCont q.1)) : isCont q.1 = false := by
  have := (List.mem_filter.mp h).2; simpa using this

theorem lanes_mem {ys : List Nat} {vl : Nat} {q : Nat × Nat × Nat × Nat}
    (h : q ∈ ((win ys).take vl).filter (fun q => !isCont q.1)) : q.1 ∈ ys.take vl := by
  have h1 := (List.mem_filter.mp h).1
  have : q.1 ∈ ((win ys).take vl).map Prod.fst := List.mem_map_of_mem h1
  rwa [List.map_take, win_fst] at this

/-- The core inequality: lanes writing at most their weight leave
`adv + lag ≤ lag + L` of the block. -/
theorem lanes_fit (is16 : Bool) (s : Option (Nat × Nat × Bool)) (ys : List Nat) (vl : Nat)
    (cnt : Nat × Nat × Nat × Nat → Nat) (ph : Nat × Nat × Nat × Nat → Bool)
    (h : ∀ q ∈ ((win ys).take vl).filter (fun q => !isCont q.1), cnt q ≤ wU8 is16 q.1) :
    L ((((win ys).take vl).filter (fun q => !isCont q.1)).map cnt)
      + lag8 is16 (upd s (((win ys).take vl).filter (fun q => !isCont q.1)) cnt ph)
      ≤ lag8 is16 s + L ((ys.take vl).map (wU8 is16)) := by
  rw [← lanes_L]
  generalize ((win ys).take vl).filter (fun q => !isCont q.1) = l at h ⊢
  have := sum_lag l cnt (fun q => wU8 is16 q.1) h
  unfold upd
  cases hl : l.getLast? with
  | none =>
    rw [List.getLast?_eq_none_iff] at hl; subst hl; simp
  | some q =>
    rw [hl] at this; dsimp only at this; simp only [lag8]; omega

theorem utf8_fits (is16 validate : Bool) (chk : List Nat → Bool) (pick : Nat → Nat) (M1 : Nat) :
    Fits 3 (utf8Step is16 validate chk pick M1) (wU8 is16) (lag8 is16) (fun _ _ => True) := by
  intro s ys _ hT
  have hvl := vlOf_le pick (ys.length - 3)
  unfold utf8Step; dsimp only
  generalize vlOf pick (ys.length - 3) = vl at hvl
  have hlen : (ys.take vl).length = vl := take_vl_len ys (by omega)
  have hone : ∀ q ∈ ((win ys).take vl).filter (fun q => !isCont q.1), 1 ≤ wU8 is16 q.1 :=
    fun q hq => wU8_nc is16 _ (lanes_nc hq)
  have hlen1 := lanes_fit is16 s ys vl (fun _ => 1) (fun _ => false) hone
  rw [L_map_one] at hlen1
  generalize hl : ((win ys).take vl).filter (fun q => !isCont q.1) = lanes at hlen1 hone
  split
  · -- ASCII: every byte is a lane
    rename_i hasc
    have hall : ∀ q ∈ (win ys).take vl, (!isCont q.1) = true := by
      intro q hq
      have h1 : q.1 ∈ ((win ys).take vl).map Prod.fst := List.mem_map_of_mem hq
      rw [List.map_take, win_fst] at h1
      have h2 : q.1 ∈ ys.take (vl + 3) := by
        have : ys.take vl = (ys.take (vl + 3)).take vl := by rw [List.take_take]; congr 1; omega
        rw [this] at h1; exact List.mem_of_mem_take h1
      have := List.all_eq_true.mp hasc _ h2
      simp [isCont] at this ⊢; omega
    have hlanes : lanes.length = vl := by
      rw [← hl, List.filter_eq_self.mpr hall, List.length_take]
      have : (win ys).length = ys.length := by
        have := congrArg List.length (win_fst ys); simpa using this
      omega
    rw [hlanes] at hlen1
    simp [-List.map_take, -List.map_drop]; omega
  · split
    · simp
    · split
      · simp [-List.map_take, -List.map_drop]; omega
      · split
        · simp [-List.map_take, -List.map_drop]; omega
        · split
          · -- UTF-16, 4-byte path
            rename_i h16
            have hle : ∀ q ∈ lanes, hcnt q ≤ wU8 is16 q.1 := by
              intro q hq; rw [h16]; exact hcnt_le q (lanes_nc (hl ▸ hq))
            have hf := lanes_fit is16 s ys vl hcnt hph (hl ▸ hle)
            rw [hl] at hf
            have hab : L ((lanes.take M1).map hcnt) + L ((lanes.drop M1).map hcnt)
                = L (lanes.map hcnt) := by
              rw [← L_append, ← List.map_append, List.take_append_drop]
            refine ⟨?_, fun _ => ⟨by simp [-List.map_take, -List.map_drop]; omega, trivial⟩⟩
            intro p hp
            split at hp
            · simp [-List.map_take, -List.map_drop] at hp; subst hp; simp [-List.map_take, -List.map_drop]; omega
            · simp [-List.map_take, -List.map_drop] at hp; rcases hp with rfl | rfl <;> simp [-List.map_take, -List.map_drop] <;> omega
          · refine ⟨?_, fun _ => ⟨by simp [-List.map_take, -List.map_drop]; omega, trivial⟩⟩
            intro p hp
            split at hp
            · simp [-List.map_take, -List.map_drop] at hp; subst hp; simp [-List.map_take, -List.map_drop]; omega
            · simp [-List.map_take, -List.map_drop] at hp; rcases hp with rfl | rfl <;> simp [-List.map_take, -List.map_drop] <;> omega

/-- **UTF-8 → UTF-16 / UTF-32, the vector loop (all variants: `validate` and
`convert_valid`, both endiannesses, any check, any `M1`): every store ends
within `utf16_length_from_utf8` / `utf32_length_from_utf8`.** -/
theorem utf8_loop_safe (is16 validate : Bool) (chk : List Nat → Bool) (pick : Nat → Nat) (M1 : Nat)
    (xs : List Nat) {a n : Nat} (h : Store 3 (utf8Step is16 validate chk pick M1) xs none a n) :
    a + n ≤ L (xs.map (wU8 is16)) :=
  store_le (utf8_fits is16 validate chk pick M1) trivial rfl h

/-! ### The reparse step and the scalar tail (:251-267)

After the loop, `src` is at `p = len - 3`. If `src[0]` is a continuation byte
the cursor steps back one unit (:253-254), `src` steps back over continuation
bytes to `back xs p` (:255-256), and for UTF-16 the cursor steps back once
more when `dst[-1]` is a high surrogate (:257-262). The scalar function then
converts `xs.drop (back xs p)` at the cursor (:264); that it writes at most
`L` of its input is read from the scalar code. -/

/-- Last non-continuation byte. -/
def lastNC (l : List Nat) : Option Nat := (l.filter (fun b => !isCont b)).getLast?

theorem lastNC_append (a b : List Nat) : lastNC (a ++ b) = (lastNC b).or (lastNC a) := by
  simp [lastNC, List.filter_append, List.getLast?_append]

theorem lanes_fst (ys : List Nat) (vl : Nat) :
    (((win ys).take vl).filter (fun q => !isCont q.1)).map Prod.fst
      = (ys.take vl).filter (fun b => !isCont b) := by
  rw [← win_fst ys, ← List.map_take, List.filter_map, win_fst]; rfl

theorem ascii_lanes {ys : List Nat} {vl : Nat}
    (hasc : (ys.take (vl + 3)).all (fun b => decide (b < 0x80)) = true) (hvl : vl ≤ ys.length) :
    (((win ys).take vl).filter (fun q => !isCont q.1)).length = vl := by
  have hall : ∀ q ∈ (win ys).take vl, (!isCont q.1) = true := by
    intro q hq
    have h1 : q.1 ∈ ((win ys).take vl).map Prod.fst := List.mem_map_of_mem hq
    rw [List.map_take, win_fst] at h1
    have h2 : q.1 ∈ ys.take (vl + 3) := by
      have : ys.take vl = (ys.take (vl + 3)).take vl := by rw [List.take_take]; congr 1; omega
      rw [this] at h1; exact List.mem_of_mem_take h1
    have := List.all_eq_true.mp hasc _ h2
    simp [isCont] at this ⊢; omega
  rw [List.filter_eq_self.mpr hall, List.length_take]
  have : (win ys).length = ys.length := by
    have := congrArg List.length (win_fst ys); simpa using this
  omega

/-- What every non-returning iteration does: consume `vl`, advance by the sum
of the lane counts, and move the state to the last lane; each lane writes at
least one unit, and a lane writing two writes a high surrogate first. -/
theorem utf8_next (is16 validate : Bool) (chk : List Nat → Bool) (pick : Nat → Nat) (M1 : Nat)
    (s : Option (Nat × Nat × Bool)) (ys : List Nat) (hT : 3 < ys.length)
    (hst : (utf8Step is16 validate chk pick M1 s ys).stop = false) :
    (utf8Step is16 validate chk pick M1 s ys).c = vlOf pick (ys.length - 3) ∧
    ∃ cnt ph, (utf8Step is16 validate chk pick M1 s ys).next =
        upd s (((win ys).take (vlOf pick (ys.length - 3))).filter (fun q => !isCont q.1)) cnt ph ∧
      (utf8Step is16 validate chk pick M1 s ys).adv =
        L ((((win ys).take (vlOf pick (ys.length - 3))).filter (fun q => !isCont q.1)).map cnt) ∧
      ∀ q, 1 ≤ cnt q ∧ (2 ≤ cnt q → ph q = true) := by
  have hvl := vlOf_le pick (ys.length - 3)
  revert hst
  unfold utf8Step; dsimp only
  generalize vlOf pick (ys.length - 3) = vl at hvl
  have hone : ∀ q : Nat × Nat × Nat × Nat, 1 ≤ (fun _ => 1) q ∧
      (2 ≤ (fun (_ : Nat × Nat × Nat × Nat) => 1) q → (fun _ => false) q = true) := by
    intro q; simp
  have hh : ∀ q, 1 ≤ hcnt q ∧ (2 ≤ hcnt q → hph q = true) := by
    intro q
    have hl : 1 ≤ hcnt q ∧ hcnt q ≤ 2 := helper_len _
    refine ⟨hl.1, fun h2 => ?_⟩
    have e : hcnt q = 2 := by omega
    simp only [hph, e, beq_self_eq_true, Bool.true_and]
    exact helper_two _ (by unfold hcnt at e; exact e)
  split
  · rename_i hasc
    intro _
    refine ⟨rfl, _, _, rfl, ?_, hone⟩
    simp only [L_map_one]; rw [ascii_lanes hasc (by omega)]
  · split
    · intro hst; simp at hst
    · intro _
      split
      · exact ⟨rfl, _, _, rfl, by simp only [L_map_one], hone⟩
      · split
        · exact ⟨rfl, _, _, rfl, by simp only [L_map_one], hone⟩
        · split
          · refine ⟨rfl, hcnt, hph, rfl, ?_, hh⟩
            dsimp only
            rw [← L_append, ← List.map_append, List.take_append_drop]
          · exact ⟨rfl, _, _, rfl, by simp only [L_map_one], hone⟩

/-- The state is the last lane written. -/
def SInv (s : Option (Nat × Nat × Bool)) (P : List Nat) : Prop :=
  s.map Prod.fst = lastNC P ∧ ∀ b k ph, s = some (b, k, ph) → 1 ≤ k ∧ (2 ≤ k → ph = true)

theorem utf8_sinv (is16 validate : Bool) (chk : List Nat → Bool) (pick : Nat → Nat) (M1 : Nat)
    (xs : List Nat) {s : Option (Nat × Nat × Bool)} {o i : Nat}
    (h : Reach 3 (utf8Step is16 validate chk pick M1) xs none s o i) : SInv s (xs.take i) := by
  induction h with
  | init => simp [SInv, lastNC]
  | @step s o i _ hT hstop ih =>
    obtain ⟨hc, cnt, ph, hn, _, hcp⟩ := utf8_next is16 validate chk pick M1 s _ hT hstop
    rw [hc, hn, List.take_add]
    unfold SInv
    rw [lastNC_append]
    generalize hl : ((win (xs.drop i)).take (vlOf pick ((xs.drop i).length - 3))).filter
      (fun q => !isCont q.1) = lanes
    have hf := lanes_fst (xs.drop i) (vlOf pick ((xs.drop i).length - 3))
    rw [hl] at hf
    have hlast : lastNC ((xs.drop i).take (vlOf pick ((xs.drop i).length - 3)))
        = lanes.getLast?.map Prod.fst := by
      rw [lastNC, ← hf, List.getLast?_map]
    rw [hlast]
    unfold upd
    cases hq : lanes.getLast? with
    | none => exact ⟨by simpa using ih.1, ih.2⟩
    | some q =>
      refine ⟨by simp, ?_⟩
      intro b k ph' he
      simp at he
      obtain ⟨_, rfl, rfl⟩ := he
      exact hcp q

theorem L_take_succ (w : Nat → Nat) (xs : List Nat) (p : Nat) (hp : p < xs.length) :
    L ((xs.take (p + 1)).map w) = L ((xs.take p).map w) + w (xs.getD p 0) := by
  rw [List.take_add_one, List.map_append, L_append]
  simp [List.getElem?_eq_getElem hp, List.getD_eq_getElem?_getD]

theorem lastNC_take_succ (xs : List Nat) (p : Nat) (hp : p < xs.length) :
    lastNC (xs.take (p + 1)) =
      if isCont (xs.getD p 0) then lastNC (xs.take p) else some (xs.getD p 0) := by
  rw [List.take_add_one, lastNC_append]
  simp only [List.getElem?_eq_getElem hp, Option.toList_some, List.getD_eq_getElem?_getD,
    Option.getD_some]
  cases h : isCont xs[p] <;> simp [lastNC, h]

/-- `while ((src[0] >> 6) == 0b10 && tail < len) --src, ++tail` (:255-256),
from index `p`: the last index `≤ p` that is not a continuation byte, or 0. -/
def back (xs : List Nat) : Nat → Nat
  | 0 => 0
  | p + 1 => if isCont (xs.getD (p + 1) 0) then back xs p else p + 1

theorem back_spec (w : Nat → Nat) (hw : ∀ b, isCont b = true → w b = 0) (xs : List Nat) :
    ∀ p, p < xs.length →
      L ((xs.take (p + 1)).map w) = L ((xs.take (back xs p)).map w) + w (xs.getD (back xs p) 0) ∧
      (isCont (xs.getD (back xs p) 0) = false →
        lastNC (xs.take (p + 1)) = some (xs.getD (back xs p) 0)) ∧
      (back xs p = 0 ∨ isCont (xs.getD (back xs p) 0) = false) ∧ back xs p ≤ p
  | 0, hp => by
    refine ⟨by rw [L_take_succ w xs 0 hp]; rfl, fun h => ?_, Or.inl rfl, Nat.le_refl _⟩
    rw [lastNC_take_succ xs 0 hp]; simp [back] at h ⊢; simp [h]
  | p + 1, hp => by
    have ih := back_spec w hw xs p (by omega)
    by_cases hc : isCont (xs.getD (p + 1) 0) = true
    · have hb : back xs (p + 1) = back xs p := by simp only [back, hc, ite_true]
      rw [hb, L_take_succ w xs (p + 1) hp, hw _ hc, lastNC_take_succ xs (p + 1) hp]
      simp only [hc, ite_true]
      exact ⟨by rw [ih.1]; omega, ih.2.1, ih.2.2.1, by omega⟩
    · have hc' : isCont (xs.getD (p + 1) 0) = false := by simpa using hc
      have hb : back xs (p + 1) = p + 1 := by simp only [back, hc', Bool.false_eq_true, ite_false]
      rw [hb, L_take_succ w xs (p + 1) hp, lastNC_take_succ xs (p + 1) hp]
      simp only [hc', Bool.false_eq_true, ite_false]
      simp [hc']

/-- The cursor after the reparse step: `--dst` when `src[0]` is a continuation
byte (:253-254), then for UTF-16 `--dst` when `dst[-1]` is a high surrogate
(`hi`, :257-262). -/
def reparse (is16 contP : Bool) (o : Nat) (hi : Bool) : Nat :=
  let d1 := if contP then o - 1 else o
  if is16 && hi then d1 - 1 else d1

theorem wU8_false_le1 (b : Nat) : wU8 false b ≤ 1 := by
  show w01 b ≤ 1; unfold w01; split <;> omega

/-- **The reparse step and the scalar tail of `rvv_utf8_to_common`.** At the
loop exit (`¬ 3 < remaining`), with at least two non-continuation bytes among
the bytes the loop consumed (`prologue_two`): the loop consumed `len - 3`
bytes, the cursor is at least 2 (so `dst - 1` and the read of `dst[-1]` are
inside the buffer), and the scalar tail, writing at most `L` of
`xs.drop (back xs p)`, ends within `L` of the input. `hi` is what the C++
reads at `dst[-1]`: any value, except that when the last lane wrote two units
starting with a high surrogate (`ph = true`), the unit there is that high
surrogate (the lanes are stored in order and the last lane ends at the
cursor; `src[0]` being a continuation byte, it is `dst[-2]` before the first
step back). -/
theorem utf8_tail (is16 validate : Bool) (chk : List Nat → Bool) (pick : Nat → Nat) (M1 : Nat)
    (xs : List Nat) (hlen : 32 ≤ xs.length) {s : Option (Nat × Nat × Bool)} {o i : Nat}
    (hR : Reach 3 (utf8Step is16 validate chk pick M1) xs none s o i)
    (hexit : ¬ 3 < (xs.drop i).length)
    (h2 : ∀ j, 7 ≤ j → 2 ≤ L ((xs.take j).map w01))
    (hi : Bool) (hforce : isCont (xs.getD i 0) = true → ∀ b k, s = some (b, k, true) → hi = true)
    (scal : Nat) (hscal : scal ≤ L ((xs.drop (back xs i)).map (wU8 is16))) :
    i = xs.length - 3 ∧ 2 ≤ o ∧
      reparse is16 (isCont (xs.getD i 0)) o hi + scal ≤ L (xs.map (wU8 is16)) := by
  have hf := utf8_fits is16 validate chk pick M1
  have hinv := (reach_inv hf trivial rfl hR).2
  have hlow : L ((xs.take i).map w01) ≤ o := by
    refine reach_lower hf trivial rfl (fun s ys _ hT hst => ?_) hR
    obtain ⟨hc, cnt, ph, _, hadv, hcp⟩ := utf8_next is16 validate chk pick M1 s ys hT hst
    rw [hc, hadv]
    have e := lanes_L false ys (vlOf pick (ys.length - 3))
    have e2 : (wU8 false) = w01 := rfl
    rw [e2] at e
    rw [← e]
    apply L_map_le
    intro q hq
    have := (hcp q).1
    simp [w01, lanes_nc hq]; omega
  have hle : i + 3 ≤ xs.length := by
    refine reach_len hf trivial rfl (by omega) (fun s ys _ hT => ?_) hR
    unfold utf8Step; dsimp only
    have := vlOf_le pick (ys.length - 3)
    repeat' split
    all_goals simp; omega
  have hieq : i = xs.length - 3 := by simp at hexit; omega
  have h2 := h2 i (by omega)
  refine ⟨hieq, by omega, ?_⟩
  have hs_i := utf8_sinv is16 validate chk pick M1 xs hR
  have hsplit : L (xs.map (wU8 is16)) =
      L ((xs.take (back xs i)).map (wU8 is16)) + L ((xs.drop (back xs i)).map (wU8 is16)) := by
    rw [L_take_add_drop (back xs i) (xs.map (wU8 is16)), List.map_take, List.map_drop]
  obtain ⟨p, rfl⟩ : ∃ p, i = p + 1 := ⟨i - 1, by omega⟩
  by_cases hc : isCont (xs.getD (p + 1) 0) = true
  · have hb : back xs (p + 1) = back xs p := by simp only [back, hc, ite_true]
    rw [hb] at hscal hsplit
    have bs := back_spec (wU8 is16) (wU8_cont is16) xs p (by omega)
    have bs01 := back_spec w01 (wU8_cont false) xs p (by omega)
    have hnc : isCont (xs.getD (back xs p) 0) = false := by
      rcases bs.2.2.1 with h0 | h0
      · cases hh : isCont (xs.getD (back xs p) 0)
        · rfl
        · exfalso
          have h1 := bs01.1
          have e : w01 (xs.getD (back xs p) 0) = 0 := wU8_cont false _ hh
          have z : L ((xs.take (back xs p)).map w01) = 0 := by rw [h0]; rfl
          omega
      · exact h0
    have hlast := bs.2.1 hnc
    obtain ⟨hs1, hs2⟩ := hs_i
    rw [hlast] at hs1
    obtain ⟨⟨b, k, ph⟩, rfl⟩ : ∃ t, s = some t := by
      cases s with
      | none => simp at hs1
      | some t => exact ⟨t, rfl⟩
    simp at hs1
    subst hs1
    obtain ⟨hk1, hk2⟩ := hs2 _ k ph rfl
    have hW1 := wU8_nc is16 _ hnc
    have hW2 := wU8_le2 is16 (xs.getD (back xs p) 0)
    have hW3 : is16 = false → wU8 is16 (xs.getD (back xs p) 0) ≤ 1 := by
      intro h; subst h; exact wU8_false_le1 _
    have hinv' : o + (wU8 is16 (xs.getD (back xs p) 0) - k)
        ≤ L ((xs.take (back xs p)).map (wU8 is16)) + wU8 is16 (xs.getD (back xs p) 0) := by
      have := hinv; rw [bs.1] at this; exact this
    have hforce' := hforce hc
    clear hinv bs bs01 hforce hlast hnc
    generalize wU8 is16 (xs.getD (back xs p) 0) = W at *
    generalize L ((xs.take (back xs p)).map (wU8 is16)) = Lb at *
    generalize L ((xs.drop (back xs p)).map (wU8 is16)) = Ld at *
    generalize L (xs.map (wU8 is16)) = Lt at *
    simp only [reparse, hc, ↓reduceIte]
    by_cases hk : 2 ≤ k
    · have hph := hk2 hk
      subst hph
      have hh := hforce' _ k rfl
      subst hh
      cases is16
      · have := hW3 rfl
        simp only [Bool.and_true, Bool.false_and, Bool.false_eq_true, ↓reduceIte]; omega
      · simp only [Bool.and_true, Bool.true_and, ↓reduceIte]; omega
    · cases is16 <;> cases hi <;>
        simp only [Bool.and_true, Bool.and_false, Bool.true_and, Bool.false_and,
          Bool.false_eq_true, ↓reduceIte] <;> omega
  · have hc' : isCont (xs.getD (p + 1) 0) = false := by simpa using hc
    have hb : back xs (p + 1) = p + 1 := by simp only [back, hc', Bool.false_eq_true, ite_false]
    rw [hb] at hscal hsplit
    simp only [reparse, hc', Bool.false_eq_true, ↓reduceIte]
    generalize L ((xs.take (p + 1)).map (wU8 is16)) = Lb at *
    generalize L ((xs.drop (p + 1)).map (wU8 is16)) = Ld at *
    generalize lag8 is16 s = g at *
    cases is16 <;> cases hi <;>
      simp only [Bool.and_true, Bool.and_false, Bool.true_and, Bool.false_and,
        Bool.false_eq_true, ↓reduceIte] <;> omega

/-- The prologue `idx = 3; while (idx < len && (src[idx] >> 6) == 0b10) ++idx`
(:22-24), with fuel. -/
def skipCont (xs : List Nat) (len : Nat) : Nat → Nat → Nat
  | 0, idx => idx
  | f + 1, idx =>
    if idx < len ∧ isCont (xs.getD idx 0) = true then skipCont xs len f (idx + 1) else idx

theorem skip_spec (xs : List Nat) (len : Nat) : ∀ f idx,
    idx ≤ skipCont xs len f idx ∧
    (skipCont xs len f idx < idx + f → skipCont xs len f idx < len →
      isCont (xs.getD (skipCont xs len f idx) 0) = false)
  | 0, idx => by simp [skipCont]
  | f + 1, idx => by
    have ih := skip_spec xs len f (idx + 1)
    by_cases h : idx < len ∧ isCont (xs.getD idx 0) = true
    · simp only [skipCont, h, and_self, ite_true]
      exact ⟨by omega, fun h1 h2 => ih.2 (by omega) h2⟩
    · simp only [skipCont, h, ite_false]
      refine ⟨Nat.le_refl _, fun _ h2 => ?_⟩
      simpa [h2] using h

/-- **The prologue gives the hypothesis of `utf8_tail`.** The validating
kernels return 0 unless `idx ≤ 6` and `scalar::utf8::validate(src, idx)`
holds (:25-26); that the scalar validator rejects a first byte that is a
continuation byte (`h0`) is read from the scalar code. Valid UTF-8 (the
contract of `convert_valid`) also has both: a first byte that is not a
continuation byte, and at most three continuation bytes in a row. -/
theorem prologue_two (xs : List Nat) (hlen : 32 ≤ xs.length)
    (h0 : isCont (xs.getD 0 0) = false) (hp : skipCont xs xs.length xs.length 3 ≤ 6) :
    ∀ i, 7 ≤ i → 2 ≤ L ((xs.take i).map w01) := by
  intro i hi
  have sp := skip_spec xs xs.length xs.length 3
  generalize skipCont xs xs.length xs.length 3 = j at sp hp
  have hj := sp.2 (by omega) (by omega)
  have e1 := L_take_succ w01 xs j (by omega)
  have e0 := L_take_succ w01 xs 0 (by omega)
  have m1 := L_take_mono w01 xs (show 1 ≤ j by omega)
  have m2 := L_take_mono w01 xs (show j + 1 ≤ i by omega)
  have w0 : w01 (xs.getD 0 0) = 1 := by simp only [w01, h0, Bool.false_eq_true, ↓reduceIte]
  have wj : w01 (xs.getD j 0) = 1 := by simp only [w01, hj, Bool.false_eq_true, ↓reduceIte]
  have z : L ((xs.take 0).map w01) = 0 := rfl
  simp only [Nat.zero_add] at e0
  omega

/-- **The validating UTF-8 decoders, end to end:** after the prologue passed
(:20-27), the loop's stores (`utf8_loop_safe`), the reparse step and the
scalar tail stay within `L`. -/
theorem utf8_validating_tail (is16 : Bool) (chk : List Nat → Bool) (pick : Nat → Nat) (M1 : Nat)
    (xs : List Nat) (hlen : 32 ≤ xs.length)
    (h0 : isCont (xs.getD 0 0) = false) (hp : skipCont xs xs.length xs.length 3 ≤ 6)
    {s : Option (Nat × Nat × Bool)} {o i : Nat}
    (hR : Reach 3 (utf8Step is16 true chk pick M1) xs none s o i)
    (hexit : ¬ 3 < (xs.drop i).length)
    (hi : Bool) (hforce : isCont (xs.getD i 0) = true → ∀ b k, s = some (b, k, true) → hi = true)
    (scal : Nat) (hscal : scal ≤ L ((xs.drop (back xs i)).map (wU8 is16))) :
    i = xs.length - 3 ∧ 2 ≤ o ∧
      reparse is16 (isCont (xs.getD i 0)) o hi + scal ≤ L (xs.map (wU8 is16)) :=
  utf8_tail is16 true chk pick M1 xs hlen hR hexit (prologue_two xs hlen h0 hp) hi hforce scal hscal

/-! ## Non-vacuity: the loops issue the stores the C++ issues -/

/-- `é` (0xE9) in Latin-1: one iteration, a 2-byte store at 0. -/
example : Store 0 (latin1to8Step id) [0xE9] () 0 2 :=
  ⟨(), 0, 0, .init, by decide, (0, 2), by decide, rfl, rfl⟩

/-- U+1F600 in UTF-32 → UTF-16: the helper stores 2 units. -/
example : Store 0 (utf32to16Step true id) [0x1F600] () 0 2 :=
  ⟨(), 0, 0, .init, by decide, (0, 2), by decide, rfl, rfl⟩

/-- U+1F600 in UTF-16 → UTF-8: no vector prefix, the scalar pair loop writes
4 bytes at 0. -/
example : Store 0 (utf16to8Step id) [0xD83D, 0xDE00] () 0 4 :=
  ⟨(), 0, 0, .init, by decide, (0, 4), by decide, rfl, rfl⟩

end SafetyMargins.RvvExact
