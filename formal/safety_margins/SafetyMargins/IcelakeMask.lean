import SafetyMargins.ValidLatin1
import SafetyMargins.ValidExact
/-
64-bit masks for the Ice Lake UTF-8 decoders (`IcelakeUtf8.lean`,
`IcelakeUtf8Fixed.lean`).

A mask is a function from the bit index to `Bool`; the masks of the kernels
are 0 at index 64 and above. The operations are those of the C++:
`_mm_popcnt_u64` (`popcnt`), `64 - _lzcnt_u64` (`top`), `_pdep_u64`
(`pdep`), `_pext_u64` (`pext`), shifts by a constant. `cntB m n` counts the
set bits below `n`, which is also the rank of bit `n` in `pdep` and `pext`.
-/
namespace SafetyMargins.IcelakeUtf8
open SafetyMargins

abbrev Mask := Nat → Bool

def ind (b : Bool) : Nat := if b then 1 else 0

@[simp] theorem ind_true : ind true = 1 := rfl
@[simp] theorem ind_false : ind false = 0 := rfl
theorem ind_le_one (b : Bool) : ind b ≤ 1 := by cases b <;> simp

/-- Set bits of `m` below `n`. -/
def cntB (m : Mask) : Nat → Nat
  | 0 => 0
  | n + 1 => cntB m n + ind (m n)

/-- `_mm_popcnt_u64` of a 64-bit mask. -/
def popcnt (m : Mask) : Nat := cntB m 64

/-- One past the highest set bit below `n`, or 0. -/
def hiB (m : Mask) : Nat → Nat
  | 0 => 0
  | n + 1 => if m n then n + 1 else hiB m n

/-- `64 - _lzcnt_u64(m)`: one past the highest set bit, 0 for `m = 0`. -/
def top (m : Mask) : Nat := hiB m 64

/-- `_pdep_u64(src, m)`: the `r`-th set bit of `m` (from bit 0) receives bit
`r` of `src`. -/
def pdep (src m : Mask) : Mask := fun p => decide (p < 64) && m p && src (cntB m p)

/-- `_pext_u64(src, m)`: bit `r` is `src` at the `r`-th set bit of `m`. -/
def pext (src m : Mask) : Mask := fun r =>
  (List.range 64).any fun p => m p && cntB m p == r && src p

/-- `m << k` on 64 bits. -/
def shl (m : Mask) (k : Nat) : Mask := fun p => decide (p < 64) && decide (k ≤ p) && m (p - k)
/-- `m >> k` on 64 bits. -/
def shr (m : Mask) (k : Nat) : Mask := fun p => decide (p < 64) && m (p + k)

/-- The low `j` bits set (`0xFFFFFFFF` for `j = 32`). -/
def ones (j : Nat) : Mask := fun r => decide (r < j)

/-- Some bit below 64 is set (the mask is nonzero). -/
def anyB (m : Mask) : Bool := (List.range 64).any m

/-! ## Counting -/

theorem cntB_mono (m : Mask) {a b : Nat} (h : a ≤ b) : cntB m a ≤ cntB m b := by
  induction b with
  | zero => have : a = 0 := by omega
            subst this; exact Nat.le_refl _
  | succ b ih =>
    by_cases hab : a = b + 1
    · subst hab; exact Nat.le_refl _
    · have := ih (by omega); simp only [cntB]; omega

theorem cntB_le (m : Mask) (n : Nat) : cntB m n ≤ n := by
  induction n with
  | zero => exact Nat.le_refl _
  | succ n ih => simp only [cntB]; have := ind_le_one (m n); omega

theorem cntB_congr {a b : Mask} {n : Nat} (h : ∀ p, p < n → a p = b p) :
    cntB a n = cntB b n := by
  induction n with
  | zero => rfl
  | succ n ih =>
    simp only [cntB]
    rw [ih (fun p hp => h p (by omega)), h n (by omega)]

theorem cntB_imp {a b : Mask} {n : Nat} (h : ∀ p, p < n → a p = true → b p = true) :
    cntB a n ≤ cntB b n := by
  induction n with
  | zero => exact Nat.le_refl _
  | succ n ih =>
    simp only [cntB]
    have ih' := ih (fun p hp => h p (by omega))
    have : ind (a n) ≤ ind (b n) := by
      cases ha : a n
      · simp
      · rw [h n (by omega) ha]; simp
    omega

theorem cntB_succ_of (m : Mask) (n : Nat) (h : m n = true) : cntB m (n + 1) = cntB m n + 1 := by
  simp [cntB, h]

theorem cntB_succ_of_not (m : Mask) (n : Nat) (h : m n = false) : cntB m (n + 1) = cntB m n := by
  simp [cntB, h]

/-- Set bits have distinct ranks. -/
theorem rank_inj (m : Mask) {p q : Nat} (hp : m p = true) (hq : m q = true)
    (h : cntB m p = cntB m q) : p = q := by
  rcases Nat.lt_trichotomy p q with hlt | heq | hgt
  · have h1 := cntB_succ_of m p hp
    have h2 := cntB_mono m (show p + 1 ≤ q by omega)
    omega
  · exact heq
  · have h1 := cntB_succ_of m q hq
    have h2 := cntB_mono m (show q + 1 ≤ p by omega)
    omega

/-! ## `top` -/

theorem hiB_le (m : Mask) (n : Nat) : hiB m n ≤ n := by
  induction n with
  | zero => exact Nat.le_refl _
  | succ n ih => simp only [hiB]; split <;> omega

theorem hiB_above (m : Mask) (n p : Nat) (h1 : hiB m n ≤ p) (h2 : p < n) : m p = false := by
  induction n with
  | zero => omega
  | succ n ih =>
    simp only [hiB] at h1
    by_cases hn : m n = true
    · rw [ite_eq_left hn] at h1; omega
    · rw [ite_eq_right hn] at h1
      by_cases hp : p = n
      · subst hp; simpa using hn
      · exact ih h1 (by omega)

theorem hiB_set (m : Mask) (n : Nat) (h : 0 < hiB m n) : m (hiB m n - 1) = true := by
  induction n with
  | zero => simp [hiB] at h
  | succ n ih =>
    simp only [hiB] at h ⊢
    by_cases hn : m n = true
    · rw [ite_eq_left hn]; simpa using hn
    · rw [ite_eq_right hn] at h ⊢; exact ih h

theorem cntB_hiB (m : Mask) (n : Nat) : cntB m n = cntB m (hiB m n) := by
  induction n with
  | zero => rfl
  | succ n ih =>
    simp only [hiB]
    by_cases hn : m n = true
    · rw [ite_eq_left hn]
    · rw [ite_eq_right hn]; simp only [cntB]; rw [← ih]; simp [Bool.eq_false_iff.mpr hn]

theorem top_le (m : Mask) : top m ≤ 64 := hiB_le m 64

theorem popcnt_top (m : Mask) : popcnt m = cntB m (top m) := cntB_hiB m 64

theorem top_set (m : Mask) (h : 0 < top m) : m (top m - 1) = true := hiB_set m 64 h

theorem top_above (m : Mask) (p : Nat) (h1 : top m ≤ p) (h2 : p < 64) : m p = false :=
  hiB_above m 64 p h1 h2

theorem lt_top (m : Mask) (p : Nat) (hp : p < 64) (h : m p = true) : p < top m := by
  by_cases hlt : p < top m
  · exact hlt
  · have := top_above m p (by omega) hp; simp_all

/-! ## `pdep`, `pext` -/

theorem pdep_sub (src m : Mask) (p : Nat) (h : pdep src m p = true) : m p = true := by
  simp only [pdep, Bool.and_eq_true, decide_eq_true_eq] at h; exact h.1.2

theorem pdep_lt (src m : Mask) (p : Nat) (h : pdep src m p = true) : p < 64 := by
  simp only [pdep, Bool.and_eq_true, decide_eq_true_eq] at h; exact h.1.1

theorem pdep_src (src m : Mask) (p : Nat) (h : pdep src m p = true) : src (cntB m p) = true := by
  simp only [pdep, Bool.and_eq_true, decide_eq_true_eq] at h; exact h.2

/-- The kernels' count: `popcount(pdep(src, m))` is at most the set bits of
`m` below `64 - lzcnt` of the result. -/
theorem popcnt_pdep_le (src m : Mask) :
    popcnt (pdep src m) ≤ cntB m (top (pdep src m)) := by
  rw [popcnt_top]
  exact cntB_imp (fun p _ h => pdep_sub src m p h)

theorem pext_at (src m : Mask) (q : Nat) (hq : q < 64) (hm : m q = true) :
    pext src m (cntB m q) = src q := by
  unfold pext
  cases hs : src q
  · rw [Bool.eq_false_iff]
    intro h
    rw [List.any_eq_true] at h
    obtain ⟨p, _, hp⟩ := h
    simp only [Bool.and_eq_true, beq_iff_eq] at hp
    have := rank_inj m hp.1.1 hm hp.1.2
    subst this
    simp_all
  · rw [List.any_eq_true]
    exact ⟨q, List.mem_range.mpr hq, by simp [hm, hs]⟩

theorem pext_true (src m : Mask) (r : Nat) (h : pext src m r = true) :
    ∃ q, q < 64 ∧ m q = true ∧ cntB m q = r ∧ src q = true := by
  unfold pext at h
  rw [List.any_eq_true] at h
  obtain ⟨p, hp, hh⟩ := h
  simp only [Bool.and_eq_true, beq_iff_eq] at hh
  exact ⟨p, List.mem_range.mp hp, hh.1.1, hh.1.2, hh.2⟩

/-- `pdep` of the low `j` ones keeps the first `j` set bits. -/
theorem cntB_pdep_ones (j : Nat) (m : Mask) (n : Nat) (hn : n ≤ 64) :
    cntB (pdep (ones j) m) n = min j (cntB m n) := by
  induction n with
  | zero => simp [cntB]
  | succ n ih =>
    have ih' := ih (by omega)
    simp only [cntB, ih']
    cases hm : m n
    · simp [pdep, hm]
    · simp only [pdep, hm, ones, Bool.and_true, ind_true]
      have : decide (n < 64) = true := by simp; omega
      rw [this, Bool.true_and]
      by_cases hc : cntB m n < j
      · simp [hc]; omega
      · simp [hc]; omega

theorem popcnt_pdep_ones (j : Nat) (m : Mask) :
    popcnt (pdep (ones j) m) = min j (popcnt m) := cntB_pdep_ones j m 64 (Nat.le_refl _)

/-- Below `top` of the first `j` set bits, `m` and its `pdep` agree. -/
theorem cntB_top_pdep_ones (j : Nat) (m : Mask) :
    cntB m (top (pdep (ones j) m)) = min j (popcnt m) := by
  have htop := top_le (pdep (ones j) m)
  have e1 : cntB m (top (pdep (ones j) m)) = cntB (pdep (ones j) m) (top (pdep (ones j) m)) := by
    by_cases h0 : top (pdep (ones j) m) = 0
    · rw [h0]; rfl
    · have hs := top_set (pdep (ones j) m) (by omega)
      have hsrc := pdep_src _ _ _ hs
      simp only [ones, decide_eq_true_eq] at hsrc
      apply cntB_congr
      intro p hp
      have hr := cntB_mono m (show p ≤ top (pdep (ones j) m) - 1 by omega)
      simp only [pdep, ones]
      have h64 : decide (p < 64) = true := by simp; omega
      have hj : decide (cntB m p < j) = true := by simp; omega
      rw [h64, hj]; simp
  rw [e1, ← popcnt_top, popcnt_pdep_ones]


/-! ## Byte classes and the window facts

The UTF-16 decoder classifies each byte by unsigned comparisons
(src/icelake/icelake_utf8_common.inl.cpp:52-54, :105-110, :123-125):
ASCII (`< 0x80`), continuation (`< 0xC0`), two-byte lead (`< 0xE0`),
three-byte lead (`< 0xF0`), four-byte lead (`≥ 0xF0`). `Z` is a position
outside the block (beyond the `gap` bytes of a masked load, or before byte
0, where the shifted masks bring in zeros).

The kernel's structure check says that a byte is a continuation byte exactly
when the byte before it is a lead, or the byte two before is a three- or
four-byte lead, or the byte three before is a four-byte lead (`chkW`). The
facts below are finite checks over windows of classes, restricted to the
37 windows of three classes that a sequence passing the check can produce
from the start of a block (`reach3`, closed under the check: `reach3_closed`). -/

inductive Cls | Z | A | C | L2 | L3 | L4
deriving DecidableEq, Repr

@[simp] theorem Cls.beq_def (a b : Cls) : (a == b) = decide (a = b) := rfl

def clsB (x : Nat) : Cls :=
  if x < 0x80 then .A else if x < 0xC0 then .C else if x < 0xE0 then .L2
  else if x < 0xF0 then .L3 else .L4

def Cls.m234 : Cls → Bool | .L2 | .L3 | .L4 => true | _ => false
def Cls.m34 : Cls → Bool | .L3 | .L4 => true | _ => false
def Cls.m4 : Cls → Bool | .L4 => true | _ => false
def Cls.m1234 : Cls → Bool | .A | .L2 | .L3 | .L4 => true | _ => false
/-- `utf16_length_from_utf8` of a byte of this class. -/
def Cls.w : Cls → Nat | .A => 1 | .L2 => 1 | .L3 => 1 | .L4 => 2 | _ => 0
/-- Length of the character a lead starts (0 for a continuation byte and `Z`). -/
def Cls.len : Cls → Nat | .A => 1 | .L2 => 2 | .L3 => 3 | .L4 => 4 | _ => 0

def allCls : List Cls := [.Z, .A, .C, .L2, .L3, .L4]

theorem mem_allCls (c : Cls) : c ∈ allCls := by cases c <;> simp [allCls]

/-- Expected continuation byte, from the three bytes before it. -/
def mcW (c3 c2 c1 : Cls) : Bool := c1.m234 || c2.m34 || c3.m4

/-- The structure check at one position. -/
def chkW (c3 c2 c1 c0 : Cls) : Bool := mcW c3 c2 c1 == (c0 == .C)

def reach3 : List (Cls × Cls × Cls) :=
  [(.A, .A, .A), (.A, .A, .L2), (.A, .A, .L3), (.A, .A, .L4), (.A, .L2, .C), (.A, .L3, .C),
   (.A, .L4, .C), (.C, .A, .A), (.C, .A, .L2), (.C, .A, .L3), (.C, .A, .L4), (.C, .C, .A),
   (.C, .C, .C), (.C, .C, .L2), (.C, .C, .L3), (.C, .C, .L4), (.C, .L2, .C), (.C, .L3, .C),
   (.C, .L4, .C), (.L2, .C, .A), (.L2, .C, .L2), (.L2, .C, .L3), (.L2, .C, .L4), (.L3, .C, .C),
   (.L4, .C, .C), (.Z, .A, .A), (.Z, .A, .L2), (.Z, .A, .L3), (.Z, .A, .L4), (.Z, .L2, .C),
   (.Z, .L3, .C), (.Z, .L4, .C), (.Z, .Z, .A), (.Z, .Z, .L2), (.Z, .Z, .L3), (.Z, .Z, .L4),
   (.Z, .Z, .Z)]

def cons3 (c3 c2 c1 : Cls) : Bool := reach3.contains (c3, c2, c1)

theorem cons3_mem {c3 c2 c1 : Cls} (h : cons3 c3 c2 c1 = true) : (c3, c2, c1) ∈ reach3 := by
  simpa [cons3] using h

/-- Mending debt: the `mend` bits still to come for the character holding
the last byte of the window. -/
def phi (c2 c1 c0 : Cls) : Nat :=
  match c0 with
  | .L4 => 2
  | .L2 | .L3 => 1
  | .C =>
    match c1 with
    | .L4 => 2
    | .L3 => 1
    | .C => if c2 == .L4 then 1 else 0
    | _ => 0
  | _ => 0

/-- Where the window sits: the next byte is in the block (`inner`), or the
byte is byte 63 of a full block (`full`), or the last byte of a masked tail
(`tail`). -/
inductive PF | inner | full | tail
deriving DecidableEq, Repr

/-- `mend` at position `k` from the classes at `k - 3, k - 2` and `k + 1`
(src/icelake/icelake_utf8_common.inl.cpp:251-255): a lead follows, or the
next byte ends a four-byte character (the high surrogate ends here), or this
byte ends one; in a tail, the last byte. -/
def mendW : PF → Cls → Cls → Cls → Bool
  | .inner, c3, c2, cn => c2.m4 || cn.m1234 || c3.m4
  | .full, c3, _, _ => c3.m4
  | .tail, _, _, _ => true

def winPre (pf : PF) (c3 c2 c1 c0 cn : Cls) : Bool :=
  cons3 c3 c2 c1 && c0 != .Z && chkW c3 c2 c1 c0 && (pf == .full || chkW c2 c1 c0 cn) &&
    ((pf == .inner) == (cn != .Z))

def winPost (pf : PF) (c3 c2 c1 c0 cn : Cls) : Bool :=
  decide (ind (mendW pf c3 c2 cn) + phi c2 c1 c0 ≤ c0.w + phi c3 c2 c1) &&
    (pf != .inner || !mendW pf c3 c2 cn || c2.m4 || cn != .C) &&
    (pf != .full || !mendW pf c3 c2 cn || (c3 == .L4 && c2 == .C && c1 == .C && c0 == .C)) &&
    cons3 c2 c1 c0

def winAll : Bool :=
  reach3.all fun t => allCls.all fun c0 => allCls.all fun cn =>
    [PF.inner, PF.full, PF.tail].all fun pf =>
      !winPre pf t.1 t.2.1 t.2.2 c0 cn || winPost pf t.1 t.2.1 t.2.2 c0 cn

/-- The window facts, on every window the check allows. -/
theorem win_all : winAll = true := by decide +kernel

theorem win {pf : PF} {c3 c2 c1 c0 cn : Cls} (h : winPre pf c3 c2 c1 c0 cn = true) :
    winPost pf c3 c2 c1 c0 cn = true := by
  have hm : (c3, c2, c1) ∈ reach3 := by
    simp only [winPre, Bool.and_eq_true] at h; exact cons3_mem h.1.1.1.1
  have := List.all_eq_true.mp win_all _ hm
  have := List.all_eq_true.mp this c0 (mem_allCls c0)
  have := List.all_eq_true.mp this cn (mem_allCls cn)
  have := List.all_eq_true.mp this pf (by cases pf <;> simp)
  simp only [h, Bool.not_true, Bool.false_or] at this
  exact this

theorem cons3_start : cons3 .Z .Z .Z = true := by decide

theorem winPre_of {pf : PF} {c3 c2 c1 c0 cn : Cls} (h1 : cons3 c3 c2 c1 = true) (h2 : c0 ≠ .Z)
    (h3 : chkW c3 c2 c1 c0 = true) (h4 : pf = .full ∨ chkW c2 c1 c0 cn = true)
    (h5 : pf = .inner ↔ cn ≠ .Z) : winPre pf c3 c2 c1 c0 cn = true := by
  unfold winPre
  have e4 : (pf == .full || chkW c2 c1 c0 cn) = true := by
    rcases h4 with h | h <;> simp [h]
  have e5 : ((pf == .inner) == (cn != .Z)) = true := by
    cases pf <;> cases cn <;> simp_all
  simp [h1, h2, h3, e4, e5]

theorem winPost_spec {pf : PF} {c3 c2 c1 c0 cn : Cls} (h : winPost pf c3 c2 c1 c0 cn = true) :
    ind (mendW pf c3 c2 cn) + phi c2 c1 c0 ≤ c0.w + phi c3 c2 c1 ∧
    (pf = .inner → mendW pf c3 c2 cn = true → c2.m4 = false → cn ≠ .C) ∧
    (pf = .full → mendW pf c3 c2 cn = true → c3 = .L4 ∧ c2 = .C ∧ c1 = .C ∧ c0 = .C) ∧
    cons3 c2 c1 c0 = true := by
  unfold winPost at h
  simp only [Bool.and_eq_true, decide_eq_true_eq, Bool.or_eq_true, bne_iff_ne, ne_eq,
    Bool.not_eq_true', Cls.beq_def] at h
  obtain ⟨⟨⟨ha, hb⟩, hc⟩, hd⟩ := h
  refine ⟨ha, ?_, ?_, hd⟩
  · intro hp hm h4
    rcases hb with ((hb | hb) | hb) | hb
    · exact absurd hp (by simpa using hb)
    · simp_all
    · simp_all
    · simpa using hb
  · intro hp hm
    rcases hc with (hc | hc) | hc
    · exact absurd hp (by simpa using hc)
    · simp_all
    · exact ⟨hc.1.1.1, hc.1.1.2, hc.1.2, hc.2⟩

def closedOK : Bool :=
  reach3.all fun t => allCls.all fun c0 =>
    c0 == .Z || !chkW t.1 t.2.1 t.2.2 c0 || cons3 t.2.1 t.2.2 c0

/-- `reach3` is closed under the check. -/
theorem reach3_closed : closedOK = true := by decide

theorem cons3_step {c3 c2 c1 c0 : Cls} (h : cons3 c3 c2 c1 = true) (h0 : c0 ≠ .Z)
    (hc : chkW c3 c2 c1 c0 = true) : cons3 c2 c1 c0 = true := by
  have := List.all_eq_true.mp reach3_closed _ (cons3_mem h)
  have := List.all_eq_true.mp this c0 (mem_allCls c0)
  simp only [hc, Bool.not_true, Bool.or_false] at this
  cases c0 <;> simp_all

/-! ### Valid UTF-8 and the classes

`ValidExact.step` is the UTF-8 automaton. `remS s` is the number of
continuation bytes still expected in state `s`; `remW` reads the same
number off the classes of the last three bytes. -/

def remS (s : Nat) : Nat :=
  match s with
  | 1 => 1 | 2 => 2 | 3 => 3 | 4 => 2 | 5 => 2 | 6 => 3 | 7 => 3 | _ => 0

def remW (c1 c2 c3 : Cls) : Nat := max (c1.len - 1) (max (c2.len - 2) (c3.len - 3))

/-- One step of the automaton, on the class of the byte. -/
def stepOK : Bool :=
  (List.range 8).all fun s => (List.range 256).all fun x =>
    ValidExact.step s x == 8 ||
      (if s = 0 then clsB x != .C && remS (ValidExact.step s x) == (clsB x).len - 1
       else clsB x == .C && remS (ValidExact.step s x) + 1 == remS s)

theorem step_ok : stepOK = true := by decide +kernel

def validWinOK : Bool :=
  reach3.all fun t => allCls.all fun c0 =>
    let r := remW t.2.2 t.2.1 t.1
    (mcW t.1 t.2.1 t.2.2 == (r != 0)) &&
    (c0 == .Z || ((r == 0) != (c0 != .C)) ||
      (chkW t.1 t.2.1 t.2.2 c0 && remW c0 t.2.2 t.2.1 == (if r = 0 then c0.len - 1 else r - 1))) &&
    (r != 0 || (!mcW t.2.1 t.2.2 .Z && !mcW t.2.2 .Z .Z))

theorem validWin_ok : validWinOK = true := by decide +kernel

/-- After an ASCII byte nothing is pending. -/
theorem ascii_end_ok : reach3.all (fun t => t.2.2 != .A || remW .A t.2.1 t.1 == 0) = true := by
  decide

/-! ## The scalar decoders

The scalar tails and error paths (include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h,
include/simdutf/scalar/utf8_to_utf32/utf8_to_utf32.h,
include/simdutf/scalar/utf8_to_utf32/valid_utf8_to_utf32.h,
include/simdutf/scalar/utf8_to_latin1/utf8_to_latin1.h), as the number of
units they store before they return. Every store follows the tests on the
lead and its continuation bytes; the ASCII shortcuts (16 bytes, or 8 for
`convert_valid`) store one unit per byte, as the one-byte branch does, so
they are taken as one-byte steps. -/

/-- `u16`: `utf8_to_utf16::convert` and `convert_with_errors` (same stores);
`u32`: `utf8_to_utf32::convert` and `convert_with_errors`; `u32v`:
`utf8_to_utf32::convert_valid`; `l1`: `utf8_to_latin1::convert_with_errors`. -/
inductive SK | u16 | u32 | u32v | l1
deriving DecidableEq, Repr

/-- `(b & 0b11000000) == 0b10000000`. -/
def ct (y : Nat) : Bool := y &&& 0xC0 == 0x80
def cp2 (x y : Nat) : Nat := ((x &&& 0x1F) <<< 6) ||| (y &&& 0x3F)
def cp3 (x y z : Nat) : Nat := ((x &&& 0x0F) <<< 12) ||| ((y &&& 0x3F) <<< 6) ||| (z &&& 0x3F)
def cp4 (x y z t : Nat) : Nat :=
  ((x &&& 0x07) <<< 18) ||| ((y &&& 0x3F) <<< 12) ||| ((z &&& 0x3F) <<< 6) ||| (t &&& 0x3F)

/-- Units stored for a four-byte character: a surrogate pair in UTF-16. -/
def u4 (k : SK) : Nat := if k = .u16 then 2 else 1

theorem u4_pos (k : SK) : 1 ≤ u4 k := by unfold u4; split <;> omega

/-- Units stored by a scalar decoder on `bs` (fuel: at least `bs.length`).
utf8_to_utf16.h:46-136 and :175-275, utf8_to_utf32.h:42-111 and :147-227,
valid_utf8_to_utf32.h:39-75, utf8_to_latin1.h:123-169. -/
def sc8 (k : SK) : Nat → List Nat → Nat
  | 0, _ => 0
  | _ + 1, [] => 0
  | n + 1, x :: r =>
    if x < 0x80 then 1 + sc8 k n r
    else if x &&& 0xE0 = 0xC0 then
      match r with
      | y :: r' =>
        if k = .u32v then 1 + sc8 k n r'
        else if !ct y || decide (cp2 x y < 0x80) || (decide (k = .l1) && decide (0xFF < cp2 x y)) then 0
        else 1 + sc8 k n r'
      | [] => 0
    else if k = .l1 then 0
    else if x &&& 0xF0 = 0xE0 then
      match r with
      | y :: z :: r' =>
        if k = .u32v then 1 + sc8 k n r'
        else if !ct y || !ct z || decide (cp3 x y z < 0x800) ||
            (decide (0xD7FF < cp3 x y z) && decide (cp3 x y z < 0xE000)) then 0
        else 1 + sc8 k n r'
      | _ => 0
    else if x &&& 0xF8 = 0xF0 then
      match r with
      | y :: z :: t :: r' =>
        if k = .u32v then 1 + sc8 k n r'
        else if !ct y || !ct z || !ct t || decide (cp4 x y z t ≤ 0xFFFF) ||
            decide (0x10FFFF < cp4 x y z t) then 0
        else u4 k + sc8 k n r'
      | _ => 0
    else 0

/-- The length function the caller sizes the buffer with. -/
def wOf : SK → Nat → Nat
  | .u16 => Utf8.w16
  | _ => Exact.w01

theorem and_ge {x m v : Nat} (h : x &&& m = v) : v ≤ x := h ▸ Nat.and_le_left

theorem wOf_lead (k : SK) (x : Nat) (h : x < 0x80 ∨ 0xC0 ≤ x) : 1 ≤ wOf k x := by
  have hc : ¬(0x80 ≤ x ∧ x < 0xC0) := by omega
  cases k <;> simp only [wOf]
  · unfold Utf8.w16; rw [ite_eq_right hc]; split <;> omega
  all_goals (unfold Exact.w01 Validator.isCont; simp; rw [ite_eq_right hc]; exact Nat.le_refl _)

theorem wOf_four (k : SK) (x : Nat) (h : 0xF0 ≤ x) : u4 k ≤ wOf k x := by
  have hc : ¬(0x80 ≤ x ∧ x < 0xC0) := by omega
  cases k <;> simp only [wOf, u4, reduceCtorEq, ite_true, ite_false]
  · unfold Utf8.w16; rw [ite_eq_right hc, ite_eq_left h]; exact Nat.le_refl _
  all_goals (unfold Exact.w01 Validator.isCont; simp; rw [ite_eq_right hc]; exact Nat.le_refl _)

/-- **The scalar decoders** store at most the length of what they read. -/
theorem sc8_le (k : SK) : ∀ n bs, sc8 k n bs ≤ L (bs.map (wOf k)) := by
  intro n
  induction n with
  | zero => intro bs; simp [sc8]
  | succ n ih =>
    intro bs
    cases bs with
    | nil => simp [sc8]
    | cons x r =>
      have h1 : x < 0x80 → 1 ≤ wOf k x := fun h => wOf_lead k x (Or.inl h)
      have h2 : x &&& 0xE0 = 0xC0 → 1 ≤ wOf k x := fun h => wOf_lead k x (Or.inr (and_ge h))
      have h3 : x &&& 0xF0 = 0xE0 → 1 ≤ wOf k x :=
        fun h => wOf_lead k x (Or.inr (by have := and_ge h; omega))
      have h4 : x &&& 0xF8 = 0xF0 → u4 k ≤ wOf k x :=
        fun h => wOf_four k x (and_ge h)
      have i0 := ih r
      have hu := u4_pos k
      rcases r with _ | ⟨y, r1⟩
      · simp only [sc8, List.map_cons, L_cons, List.map_nil, L_nil] at *
        repeat' split
        all_goals first | omega | (have := h1 ‹_›; omega)
      have i1 := ih r1
      rcases r1 with _ | ⟨z, r2⟩
      · simp only [sc8, List.map_cons, L_cons, List.map_nil, L_nil] at *
        repeat' split
        all_goals first | omega | (have := h1 ‹_›; omega) | (have := h2 ‹_›; omega)
      have i2 := ih r2
      rcases r2 with _ | ⟨t, r3⟩
      · simp only [sc8, List.map_cons, L_cons, List.map_nil, L_nil] at *
        repeat' split
        all_goals first | omega | (have := h1 ‹_›; omega) | (have := h2 ‹_›; omega) |
          (have := h3 ‹_›; omega)
      have i3 := ih r3
      simp only [sc8, List.map_cons, L_cons] at *
      repeat' split
      all_goals first | omega | (have := h1 ‹_›; omega) | (have := h2 ‹_›; omega) |
        (have := h3 ‹_›; omega) | (have := h4 ‹_›; omega)

end SafetyMargins.IcelakeUtf8
