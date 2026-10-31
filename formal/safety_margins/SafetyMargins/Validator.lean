/-
The UTF-8 check of the generic transcoders, `check_utf8_bytes`
(src/generic/utf8_to_utf16/utf8_to_utf16.h:118 and the utf32 and latin1
copies): `check_special_cases` and `check_multibyte_lengths` of
src/generic/utf8_validation/utf8_lookup4_algorithm.h, applied to a 64-byte
block whose first chunk has `prev_input = 0`.
-/
namespace SafetyMargins.Validator

def TOO_SHORT : Nat := 1 <<< 0
def TOO_LONG : Nat := 1 <<< 1
def OVERLONG_3 : Nat := 1 <<< 2
def TOO_LARGE : Nat := 1 <<< 3
def SURROGATE : Nat := 1 <<< 4
def OVERLONG_2 : Nat := 1 <<< 5
def TOO_LARGE_1000 : Nat := 1 <<< 6
def OVERLONG_4 : Nat := 1 <<< 6
def TWO_CONTS : Nat := 1 <<< 7
def CARRY : Nat := TOO_SHORT ||| TOO_LONG ||| TWO_CONTS

def byte1High : Array Nat :=
  #[TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
    TWO_CONTS, TWO_CONTS, TWO_CONTS, TWO_CONTS,
    TOO_SHORT ||| OVERLONG_2,
    TOO_SHORT,
    TOO_SHORT ||| OVERLONG_3 ||| SURROGATE,
    TOO_SHORT ||| TOO_LARGE ||| TOO_LARGE_1000 ||| OVERLONG_4]

def byte1Low : Array Nat :=
  #[CARRY ||| OVERLONG_3 ||| OVERLONG_2 ||| OVERLONG_4,
    CARRY ||| OVERLONG_2,
    CARRY, CARRY,
    CARRY ||| TOO_LARGE,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000 ||| SURROGATE,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000,
    CARRY ||| TOO_LARGE ||| TOO_LARGE_1000]

def byte2High : Array Nat :=
  #[TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
    TOO_LONG ||| OVERLONG_2 ||| TWO_CONTS ||| OVERLONG_3 ||| TOO_LARGE_1000 ||| OVERLONG_4,
    TOO_LONG ||| OVERLONG_2 ||| TWO_CONTS ||| OVERLONG_3 ||| TOO_LARGE,
    TOO_LONG ||| OVERLONG_2 ||| TWO_CONTS ||| SURROGATE ||| TOO_LARGE,
    TOO_LONG ||| OVERLONG_2 ||| TWO_CONTS ||| SURROGATE ||| TOO_LARGE,
    TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT]

/-- `check_special_cases` on one byte and the byte before it. -/
def sc (p1 c : Nat) : Nat := byte1High[p1 / 16]! &&& byte1Low[p1 % 16]! &&& byte2High[c / 16]!

/-- `must_be_2_3_continuation`: `|` on westmere, haswell, ppc64; `^` on arm64,
lsx, lasx. -/
def must23 (xorMode : Bool) (p3 p2 : Nat) : Bool :=
  if xorMode then (decide (0xE0 ≤ p2)) != (decide (0xF0 ≤ p3))
  else decide (0xE0 ≤ p2) || decide (0xF0 ≤ p3)

/-- The error bits of one byte, `check_multibyte_lengths`. -/
def err (xorMode : Bool) (p3 p2 p1 c : Nat) : Nat :=
  (if must23 xorMode p3 p2 then 0x80 else 0) ^^^ sc p1 c

def isCont (b : Nat) : Bool := decide (0x80 ≤ b) && decide (b < 0xC0)

/-- Length of the character a non-continuation byte starts. -/
def clen (b : Nat) : Nat := if b < 0x80 then 1 else if b < 0xE0 then 2 else if b < 0xF0 then 3 else 4

/-- What a byte without error says about the byte before it, checked on all
pairs of bytes. -/
def pairOK (p1 c : Nat) : Bool :=
  let s := sc p1 c
  !(s == 0 || s == 0x80) ||
    ((s == 0x80) == (isCont p1 && isCont c)) &&
    (!isCont p1 && decide (0x80 ≤ p1) → isCont c) &&
    (decide (p1 < 0x80) → !isCont c) &&
    (p1 == 0xF0 && isCont c → decide (0x90 ≤ c)) &&
    (decide (0xC0 ≤ p1) → decide (0xC2 ≤ p1) && decide (p1 ≤ 0xF4))

theorem pair_ok : (List.range 256).all (fun p1 => (List.range 256).all fun c => pairOK p1 c) = true := by
  native_decide


theorem xor_zero {a b : Nat} (h : a ^^^ b = 0) : a = b := by
  apply Nat.eq_of_testBit_eq
  intro i
  have := congrArg (fun x => x.testBit i) h
  simp [Nat.testBit_xor] at this
  exact this

theorem pairOK_of (p1 c : Nat) (h1 : p1 < 256) (h2 : c < 256) : pairOK p1 c = true :=
  (List.all_eq_true.mp ((List.all_eq_true.mp pair_ok) p1 (List.mem_range.mpr h1))) c
    (List.mem_range.mpr h2)

/-- The facts at one checked position `q`: the byte `g q` and the three before. -/
structure At (xm : Bool) (g : Nat → Nat) (q : Nat) : Prop where
  ok : sc (g (q - 1)) (g q) = if must23 xm (g (q - 3)) (g (q - 2)) then 0x80 else 0
  b1 : g (q - 1) < 256
  b0 : g q < 256

theorem At.facts {xm : Bool} {g : Nat → Nat} {q : Nat} (h : At xm g q) :
    ((isCont (g (q - 1)) && isCont (g q)) = must23 xm (g (q - 3)) (g (q - 2))) ∧
    (isCont (g (q - 1)) = false → 0x80 ≤ g (q - 1) → isCont (g q) = true) ∧
    (g (q - 1) < 0x80 → isCont (g q) = false) ∧
    (g (q - 1) = 0xF0 → isCont (g q) = true → 0x90 ≤ g q) ∧
    (0xC0 ≤ g (q - 1) → 0xC2 ≤ g (q - 1) ∧ g (q - 1) ≤ 0xF4) := by
  have p := pairOK_of _ _ h.b1 h.b0
  have hs := h.ok
  unfold pairOK at p
  generalize sc (g (q - 1)) (g q) = s at p hs
  generalize must23 xm (g (q - 3)) (g (q - 2)) = m at hs ⊢
  generalize g (q - 1) = a at p ⊢
  generalize g q = b at p ⊢
  cases m <;> simp only [Bool.false_eq_true, ↓reduceIte] at hs <;> subst hs <;>
    simp_all [isCont] <;> omega


/-- A checked block: every position `q` in `[3, n)` passes (`g` holds the block
after three zero bytes, the `prev_input = 0` of the first chunk). -/
def Checked (xm : Bool) (g : Nat → Nat) (n : Nat) : Prop :=
  (∀ i, g i < 256) ∧ ∀ q, 3 ≤ q → q < n → At.ok' xm g q
where At.ok' xm g q := sc (g (q - 1)) (g q) = if must23 xm (g (q - 3)) (g (q - 2)) then 0x80 else 0

theorem Checked.at {xm g n} (h : Checked xm g n) {q : Nat} (h3 : 3 ≤ q) (hn : q < n) :
    At xm g q := ⟨h.2 q h3 hn, h.1 _, h.1 _⟩

/-- Structure of a checked block: a character that starts at `s` and whose
next byte is in the block has `clen - 1` continuation bytes, then a
non-continuation byte; a 4-byte lead is at most `0xF4`, and `0xF0` is followed
by at least `0x90`. -/
theorem char_shape {xm : Bool} {g : Nat → Nat} {n s : Nat} (h : Checked xm g n)
    (hs : 3 ≤ s) (hlead : isCont (g s) = false) (hend : s + clen (g s) < n) :
    (∀ k, 1 ≤ k → k < clen (g s) → isCont (g (s + k)) = true) ∧
    isCont (g (s + clen (g s))) = false ∧
    (0xF0 ≤ g s → g s ≤ 0xF4 ∧ (g s = 0xF0 → 0x90 ≤ g (s + 1))) := by
  have hcl : 1 ≤ clen (g s) := by unfold clen; split <;> (try split) <;> (try split) <;> omega
  -- the byte before `s` is not a lead of length ≥ 2, hence below 0xF0
  have hprev : g (s - 1) < 0xF0 := by
    have f := (h.at hs (by omega)).facts
    by_cases hc : g (s - 1) < 0xF0
    · exact hc
    · have := f.2.1 (by simp [isCont]; omega) (by omega)
      rw [hlead] at this; exact absurd this (by decide)
  have hb := h.1
  have A1 := (h.at (q := s + 1) (by omega) (by omega)).facts
  simp only [show s + 1 - 1 = s by omega, show s + 1 - 3 = s - 2 by omega,
    show s + 1 - 2 = s - 1 by omega] at A1
  -- lead of a multi-byte character, or ASCII
  by_cases hasc : g s < 0x80
  · have hc1 : clen (g s) = 1 := by simp [clen, hasc]
    rw [hc1]
    refine ⟨fun k h1 h2 => by omega, A1.2.2.1 hasc, fun h => by omega⟩
  have hnc : isCont (g s) = false := hlead
  have hge : 0xC0 ≤ g s := by
    have := hb s; simp [isCont] at hnc; omega
  have c1 : isCont (g (s + 1)) = true := A1.2.1 hnc (by omega)
  have hrange := A1.2.2.2.2 hge
  have hF0 : g s = 0xF0 → 0x90 ≤ g (s + 1) := fun he => A1.2.2.2.1 he c1
  by_cases h2 : g s < 0xE0
  · have hc : clen (g s) = 2 := by simp [clen, hasc, h2]
    rw [hc] at hend ⊢
    have A2 := (h.at (q := s + 2) (by omega) hend).facts
    simp only [show s + 2 - 1 = s + 1 by omega, show s + 2 - 3 = s - 1 by omega,
      show s + 2 - 2 = s by omega] at A2
    have hm : must23 xm (g (s - 1)) (g s) = false := by
      unfold must23; cases xm <;> simp <;> omega
    rw [hm, c1] at A2
    refine ⟨fun k h1 h2 => by have : k = 1 := by omega
                              subst this; exact c1, by simpa using A2.1, fun h => by omega⟩
  by_cases h3 : g s < 0xF0
  · have hc : clen (g s) = 3 := by simp [clen, hasc, h2, h3]
    rw [hc] at hend ⊢
    have A2 := (h.at (q := s + 2) (by omega) (by omega)).facts
    simp only [show s + 2 - 1 = s + 1 by omega, show s + 2 - 3 = s - 1 by omega,
      show s + 2 - 2 = s by omega] at A2
    have hm : must23 xm (g (s - 1)) (g s) = true := by
      unfold must23; cases xm <;> simp <;> omega
    rw [hm, c1] at A2
    have c2 : isCont (g (s + 2)) = true := by simpa using A2.1
    have A3 := (h.at (q := s + 3) (by omega) hend).facts
    simp only [show s + 3 - 1 = s + 2 by omega, show s + 3 - 3 = s by omega,
      show s + 3 - 2 = s + 1 by omega] at A3
    have hm3 : must23 xm (g s) (g (s + 1)) = false := by
      have := hb (s + 1); simp [isCont] at c1
      unfold must23; cases xm <;> simp <;> omega
    rw [hm3, c2] at A3
    refine ⟨fun k h1 h2 => ?_, by simpa using A3.1, fun h => by omega⟩
    rcases (by omega : k = 1 ∨ k = 2) with rfl | rfl
    · exact c1
    · exact c2
  · have hc : clen (g s) = 4 := by simp [clen, hasc, h2, h3]
    rw [hc] at hend ⊢
    have A2 := (h.at (q := s + 2) (by omega) (by omega)).facts
    simp only [show s + 2 - 1 = s + 1 by omega, show s + 2 - 3 = s - 1 by omega,
      show s + 2 - 2 = s by omega] at A2
    have hm : must23 xm (g (s - 1)) (g s) = true := by
      unfold must23; cases xm <;> simp <;> omega
    rw [hm, c1] at A2
    have c2 : isCont (g (s + 2)) = true := by simpa using A2.1
    have A3 := (h.at (q := s + 3) (by omega) (by omega)).facts
    simp only [show s + 3 - 1 = s + 2 by omega, show s + 3 - 3 = s by omega,
      show s + 3 - 2 = s + 1 by omega] at A3
    have hm3 : must23 xm (g s) (g (s + 1)) = true := by
      have := hb (s + 1); simp [isCont] at c1
      unfold must23; cases xm <;> simp <;> omega
    rw [hm3, c2] at A3
    have c3 : isCont (g (s + 3)) = true := by simpa using A3.1
    have A4 := (h.at (q := s + 4) (by omega) hend).facts
    simp only [show s + 4 - 1 = s + 3 by omega, show s + 4 - 3 = s + 1 by omega,
      show s + 4 - 2 = s + 2 by omega] at A4
    have hm4 : must23 xm (g (s + 1)) (g (s + 2)) = false := by
      have := hb (s + 1); have := hb (s + 2)
      simp [isCont] at c1 c2
      unfold must23; cases xm <;> simp <;> omega
    rw [hm4, c3] at A4
    refine ⟨fun k h1 h2 => ?_, by simpa using A4.1, fun _ => ⟨hrange.2, hF0⟩⟩
    rcases (by omega : k = 1 ∨ k = 2 ∨ k = 3) with rfl | rfl | rfl
    · exact c1
    · exact c2
    · exact c3

/-- The continuation bytes of a character that starts at `s`, as far as the
block goes. -/
theorem cont_of_lead {xm : Bool} {g : Nat → Nat} {n s k : Nat} (h : Checked xm g n)
    (hs : 3 ≤ s) (hlead : isCont (g s) = false) (h80 : 0x80 ≤ g s) (hk1 : 1 ≤ k)
    (hk : k < clen (g s)) (hn : s + k < n) : isCont (g (s + k)) = true := by
  have hprev : g (s - 1) < 0xF0 := by
    have f := (h.at hs (by omega)).facts
    by_cases hc : g (s - 1) < 0xF0
    · exact hc
    · have := f.2.1 (by simp [isCont]; omega) (by omega)
      rw [hlead] at this; exact absurd this (by decide)
  have hb := h.1
  have A1 := (h.at (q := s + 1) (by omega) (by omega)).facts
  simp only [show s + 1 - 1 = s by omega, show s + 1 - 3 = s - 2 by omega,
    show s + 1 - 2 = s - 1 by omega] at A1
  have c1 : isCont (g (s + 1)) = true := A1.2.1 hlead h80
  have hc4 : clen (g s) ≤ 4 := by unfold clen; split <;> (try split) <;> (try split) <;> omega
  rcases (by omega : k = 1 ∨ k = 2 ∨ k = 3) with rfl | rfl | rfl
  · exact c1
  · have hE : 0xE0 ≤ g s := by
      have := hb s; unfold clen at hk; split at hk <;> (try split at hk) <;> (try split at hk) <;> omega
    have A2 := (h.at (q := s + 2) (by omega) hn).facts
    simp only [show s + 2 - 1 = s + 1 by omega, show s + 2 - 3 = s - 1 by omega,
      show s + 2 - 2 = s by omega] at A2
    have hm : must23 xm (g (s - 1)) (g s) = true := by
      unfold must23; cases xm <;> simp <;> omega
    rw [hm, c1] at A2
    simpa using A2.1
  · have hF : 0xF0 ≤ g s := by
      have := hb s; unfold clen at hk; split at hk <;> (try split at hk) <;> (try split at hk) <;> omega
    have A3 := (h.at (q := s + 3) (by omega) hn).facts
    simp only [show s + 3 - 1 = s + 2 by omega, show s + 3 - 3 = s by omega,
      show s + 3 - 2 = s + 1 by omega] at A3
    have hm3 : must23 xm (g s) (g (s + 1)) = true := by
      have := hb (s + 1); simp [isCont] at c1
      unfold must23; cases xm <;> simp <;> omega
    rw [hm3] at A3
    simp only [Bool.and_eq_true] at A3
    exact A3.1.2

end SafetyMargins.Validator
