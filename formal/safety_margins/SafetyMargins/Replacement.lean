import SafetyMargins.Basic
import SafetyMargins.Utf8
import SafetyMargins.Arch.Westmere
import SafetyMargins.Arch.Haswell
import SafetyMargins.Arch.Icelake
import SafetyMargins.Arch.Arm64
import SafetyMargins.Arch.Ppc64
import SafetyMargins.Arch.Lsx
import SafetyMargins.Arch.Lasx
import SafetyMargins.Arch.Rvv
/-
The conversions "with replacement".

Part A: UTF-16 → UTF-8 (`convert_utf16*_to_utf8_with_replacement`). The
driver `convert_with_replacement_via` calls the `_with_errors` kernel on the
whole rest of the input, then writes U+FFFD (3 bytes) for the unpaired
surrogate the kernel stopped at and starts again after it. The buffer holds
`utf8_length_from_utf16_with_replacement` bytes, which counts an unpaired
surrogate as 3 bytes where `utf8_length_from_utf16` counts 2.

Part B: UTF-8 → UTF-16 (`convert_utf8_to_utf16*_with_replacement`). The
driver runs the validating `_with_errors` kernel on a window that holds back
at least the last 32 bytes, and the scalar decoder of maximal subparts.
-/
namespace SafetyMargins.Replacement
open SafetyMargins SafetyMargins.Utf8

/-! # Part A: UTF-16 → UTF-8 -/

/-- `utf8_length_from_utf16` of one unit: include/simdutf/scalar/utf16.h:90-104
(1, plus 1 above 0x7F, plus 1 above 0x7FF outside the surrogates: a surrogate
weighs 2). This is the weight of the existing UTF-16 → UTF-8 models. -/
abbrev w8 : Nat → Nat := Arch.Icelake.w8

def isHigh (u : Nat) : Bool := decide (0xD800 ≤ u ∧ u ≤ 0xDBFF)
def isLow (u : Nat) : Bool := decide (0xDC00 ≤ u ∧ u ≤ 0xDFFF)
def isSur (u : Nat) : Bool := decide (0xD800 ≤ u ∧ u ≤ 0xDFFF)

/-- `utf8_length_from_utf16_with_replacement`, transcribed from
include/simdutf/scalar/utf16.h:152-181: a high surrogate followed by a low one
is 4 bytes, any other surrogate 3 (U+FFFD), any other unit 1, 2 or 3. -/
def countRepl : List Nat → Nat
  | [] => 0
  | [u] =>
    if isHigh u then 3 else if isLow u then 3
    else 1 + (if u > 0x7F then 1 else 0) + (if u > 0x7FF then 1 else 0)
  | u :: v :: r =>
    if isHigh u then
      (if isLow v then 4 + countRepl r else 3 + countRepl (v :: r))
    else if isLow u then 3 + countRepl (v :: r)
    else 1 + (if u > 0x7F then 1 else 0) + (if u > 0x7FF then 1 else 0) +
      countRepl (v :: r)

/-- The same count as per-unit weights: 2 + 2 for a pair, 3 for an unpaired
surrogate, `w8` otherwise. Every weight is at least 1 (`pos_wR`). -/
def wR : List Nat → List Nat
  | [] => []
  | [u] => [if isSur u then 3 else w8 u]
  | u :: v :: r =>
    if isHigh u && isLow v then 2 :: 2 :: wR r
    else (if isSur u then 3 else w8 u) :: wR (v :: r)

/-- Number of unpaired surrogates (same left-to-right pairing). -/
def unp : List Nat → Nat
  | [] => 0
  | [u] => if isSur u then 1 else 0
  | u :: v :: r =>
    if isHigh u && isLow v then unp r
    else (if isSur u then 1 else 0) + unp (v :: r)

theorem w8_ge (u : Nat) : 1 ≤ w8 u := by
  unfold w8 Arch.Icelake.w8; repeat' split
  all_goals omega

theorem w8_sur (u : Nat) (h : isSur u = true) : w8 u = 2 := by
  simp [isSur] at h
  unfold w8 Arch.Icelake.w8; repeat' split
  all_goals omega

theorem high_sur {u : Nat} (h : isHigh u = true) : isSur u = true := by
  simp [isHigh, isSur] at *; omega
theorem low_sur {u : Nat} (h : isLow u = true) : isSur u = true := by
  simp [isLow, isSur] at *; omega
theorem high_not_low {u : Nat} (h : isHigh u = true) : isLow u = false := by
  simp [isHigh, isLow] at *; omega

theorem w8_nonsur (u : Nat) (h : isSur u = false) :
    w8 u = 1 + (if u > 0x7F then 1 else 0) + (if u > 0x7FF then 1 else 0) := by
  simp [isSur] at h
  unfold w8 Arch.Icelake.w8; repeat' split
  all_goals omega

/-- The transcribed C++ count is `L` of the weights. -/
theorem countRepl_eq (xs : List Nat) : countRepl xs = L (wR xs) := by
  induction xs using wR.induct with
  | case1 => rfl
  | case2 u =>
    simp only [countRepl, wR, L_cons, L_nil]
    by_cases hh : isHigh u = true
    · simp [hh, high_sur hh]
    · by_cases hl : isLow u = true
      · simp [hh, hl, low_sur hl]
      · have hs : isSur u = false := by simp [isHigh, isLow, isSur] at *; omega
        simp [hh, hl, hs, w8_nonsur u hs]
  | case3 u v r hp ih =>
    simp only [Bool.and_eq_true] at hp
    simp only [countRepl, wR, hp.1, hp.2, ite_true, Bool.and_self, L_cons, ih]
    omega
  | case4 u v r hp ih =>
    rw [wR, ite_eq_right hp, L_cons, ← ih]
    simp only [countRepl]
    by_cases hh : isHigh u = true
    · have hv : isLow v = false := by
        cases h : isLow v
        · rfl
        · exact absurd (by simp [hh, h]) hp
      simp [hh, hv, high_sur hh]
    · by_cases hl : isLow u = true
      · simp [hh, hl, low_sur hl]
      · have hs : isSur u = false := by simp [isHigh, isLow, isSur] at *; omega
        simp [hh, hl, hs, w8_nonsur u hs]

/-- Replacement length = standard length + one byte per unpaired surrogate. -/
theorem L_wR (xs : List Nat) : L (wR xs) = L (xs.map w8) + unp xs := by
  induction xs using wR.induct with
  | case1 => rfl
  | case2 u =>
    simp only [wR, unp, List.map, L_cons, L_nil]
    by_cases hs : isSur u = true
    · simp [hs, w8_sur u hs]
    · simp [hs]
  | case3 u v r hp ih =>
    simp only [Bool.and_eq_true] at hp
    simp only [wR, unp, hp.1, hp.2, Bool.and_self, ite_true, List.map, L_cons, ih,
      w8_sur u (high_sur hp.1), w8_sur v (low_sur hp.2)]
    omega
  | case4 u v r hp ih =>
    rw [wR, ite_eq_right hp, unp, ite_eq_right hp, L_cons, ih]
    simp only [List.map, L_cons]
    by_cases hs : isSur u = true
    · simp [hs, w8_sur u hs]; omega
    · simp [hs]; omega

/-- On a prefix with no unpaired surrogate, both length functions agree. -/
theorem std_eq_repl (p : List Nat) (h : unp p = 0) : L (wR p) = L (p.map w8) := by
  rw [L_wR, h, Nat.add_zero]

/-- The standard length never exceeds the replacement length. -/
theorem std_le_repl (xs : List Nat) : L (xs.map w8) ≤ L (wR xs) := by
  rw [L_wR]; omega

theorem pos_w8 (xs : List Nat) : Pos (xs.map w8) := by
  intro x hx
  obtain ⟨u, _, rfl⟩ := List.mem_map.mp hx
  exact w8_ge u

def startsLow : List Nat → Bool
  | [] => false
  | v :: _ => isLow v

def endsHigh (a : List Nat) : Bool :=
  match a.getLast? with
  | some u => isHigh u
  | none => false

/-- The pairing is local: it splits anywhere except between a high surrogate
and the low surrogate after it. -/
theorem wR_append (a b : List Nat) (h : ¬ (endsHigh a = true ∧ startsLow b = true)) :
    wR (a ++ b) = wR a ++ wR b := by
  induction a using wR.induct with
  | case1 => rfl
  | case2 u =>
    cases b with
    | nil => rfl
    | cons v r =>
      have hp : ¬ ((isHigh u && isLow v) = true) := by
        simp only [endsHigh, List.getLast?_singleton, startsLow] at h
        simpa using h
      show wR (u :: v :: r) = _
      simp only [wR]
      rw [ite_eq_right hp]
      rfl
  | case3 u v r hp ih =>
    show wR (u :: v :: (r ++ b)) = _
    simp only [wR]
    rw [ite_eq_left hp, ite_eq_left hp]
    cases r with
    | nil => rfl
    | cons w r' =>
      have h' : ¬ (endsHigh (w :: r') = true ∧ startsLow b = true) := by
        simpa [endsHigh, List.getLast?_cons_cons] using h
      rw [ih h']
      rfl
  | case4 u v r hp ih =>
    show wR (u :: v :: (r ++ b)) = _
    simp only [wR]
    rw [ite_eq_right hp, ite_eq_right hp]
    have h' : ¬ (endsHigh (v :: r) = true ∧ startsLow b = true) := by
      simpa [endsHigh, List.getLast?_cons_cons] using h
    have := ih h'
    simp only [List.cons_append] at this
    rw [this]
    rfl

/-- What the kernel reports with `SURROGATE` at `ic`: unit `ic` of its input
`r` is an unpaired surrogate, a low one not preceded by a high one, or a high
one not followed by a low one. From src/westmere/sse_convert_utf16_to_utf8.cpp
:476-487 (`buf - start + k - 1`, the index of the first unit of the failed
pair) and include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:178-190
(`SURROGATE, pos`). -/
def UnpairedAt (r : List Nat) (ic : Nat) : Prop :=
  ∃ u rest, r.drop ic = u :: rest ∧
    ((isLow u = true ∧ endsHigh (r.take ic) = false) ∨
     (isHigh u = true ∧ startsLow rest = false))

theorem wR_unpaired (r : List Nat) (ic : Nat) (h : UnpairedAt r ic) :
    wR r = wR (r.take ic) ++ 3 :: wR (r.drop (ic + 1)) := by
  obtain ⟨u, rest, hd, hc⟩ := h
  have hdrop : r.drop (ic + 1) = rest := by
    rw [← List.drop_drop, hd]; rfl
  have hsplit : wR r = wR (r.take ic) ++ wR (u :: rest) := by
    conv => lhs; rw [← List.take_append_drop ic r]
    rw [hd]
    apply wR_append
    rcases hc with ⟨_, he⟩ | ⟨hh, _⟩
    · simp [he]
    · simp [startsLow, high_not_low hh]
  have hhead : wR (u :: rest) = 3 :: wR rest := by
    rcases hc with ⟨hl, _⟩ | ⟨hh, hs⟩
    · have hnh : isHigh u = false := by simp [isHigh, isLow] at *; omega
      cases rest with
      | nil => simp [wR, low_sur hl]
      | cons v r' => simp [wR, hnh, low_sur hl]
    · cases rest with
      | nil => simp [wR, high_sur hh]
      | cons v r' =>
        simp only [startsLow] at hs
        simp [wR, hh, hs, high_sur hh]
  rw [hsplit, hhead, hdrop]

/-- States `(pos, out)` of `convert_with_replacement_via`
(src/generic/utf16_to_utf8/utf16_to_utf8_with_replacement.h:13-31) on input
`xs`: `pos` units consumed, `out` bytes committed. A kernel call
(`convert_utf16_to_utf8_with_details`, e.g.
src/westmere/implementation.cpp:749-767) on `r = xs.drop pos` either succeeds
(the loop `break`s) or reports `SURROGATE` at `ic` having written
`oc = utf8_length_from_utf16` of the units before it (`output_count`); the
driver adds `oc`, writes the 3 bytes of U+FFFD and resumes at `ic + 1`. -/
inductive Via (xs : List Nat) : Nat → Nat → Prop
  | start : Via xs 0 0
  | surrogate (pos out ic oc : Nat) (h : Via xs pos out)
      (hu : UnpairedAt (xs.drop pos) ic)
      (hfirst : unp ((xs.drop pos).take ic) = 0)
      (hoc : oc = L (((xs.drop pos).take ic).map w8)) :
      Via xs (pos + ic + 1) (out + oc + 3)

/-- Invariant: the committed output plus the replacement length of the rest is
the replacement length of the whole input. -/
theorem via_inv (xs : List Nat) (pos out : Nat) (h : Via xs pos out) :
    L (wR xs) = out + L (wR (xs.drop pos)) := by
  induction h with
  | start => simp
  | surrogate pos out ic oc h hu hfirst hoc ih =>
    rw [ih, wR_unpaired _ ic hu, L_append, L_cons, std_eq_repl _ hfirst, ← hoc,
      List.drop_drop]
    have : pos + (ic + 1) = pos + ic + 1 := by omega
    rw [this]
    omega

/-- The U+FFFD bytes end at the new `out`, inside the buffer. -/
theorem via_repl_store (xs : List Nat) (pos out : Nat) (h : Via xs pos out) :
    out ≤ L (wR xs) := by
  rw [via_inv xs pos out h]; omega

/-- A kernel call whose stores end at most `se` past `out`, with `se` bounded by
the standard length of its input (what every existing UTF-16 → UTF-8 kernel
theorem gives), stays inside the replacement-sized buffer. -/
theorem via_call_store (xs : List Nat) (pos out se : Nat) (h : Via xs pos out)
    (hse : se ≤ L ((xs.drop pos).map w8)) : out + se ≤ L (wR xs) := by
  have := std_le_repl (xs.drop pos)
  rw [via_inv xs pos out h]; omega

/-- **UTF-16 → UTF-8 with replacement, fixed-margin kernels.** For a path table
whose stores fit at loop test `N` (`cur_utf16to8` of westmere, haswell, arm64,
ppc64, lsx, lasx, and the icelake model), every store of every kernel call
made by the driver ends inside `utf8_length_from_utf16_with_replacement`. -/
theorem via_paths (N : Nat) (paths : List Iter) (hp : paths.all (Iter.ok N) = true)
    (choose : List Nat → Iter) (hc : ∀ ys, N ≤ ys.length → choose ys ∈ paths)
    (xs : List Nat) (pos out n : Nat) (h : Via xs pos out) :
    out + written N choose n ((xs.drop pos).map w8) ≤ L (wR xs) :=
  via_call_store xs pos out _ h
    (safe_of_paths N paths hp choose hc n _ (pos_w8 _))

/-- The tables the driver calls, with their current margin. -/
theorem via_tables :
    Arch.Westmere.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    Arch.Haswell.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    Arch.Arm64.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    Arch.Ppc64.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    Kernels.utf16to8_lsx.all (Iter.ok (need 16 12 false)) = true ∧
    Arch.Lasx.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    Arch.Icelake.utf16to8.all (Iter.ok (need 31 1 false)) = true :=
  ⟨Arch.Westmere.cur_utf16to8, Arch.Haswell.cur_utf16to8, Arch.Arm64.cur_utf16to8,
    Arch.Ppc64.cur_utf16to8, Arch.Lsx.cur_utf16to8, Arch.Lasx.cur_utf16to8,
    Arch.Icelake.cur_utf16to8⟩

/-- Ice Lake, real ends (`safe_utf16to8_real`, same hypotheses on `extra`). -/
theorem via_icelake_real (choose : List Nat → Iter) (extra : List Nat → Nat)
    (hc : ∀ ys, need 31 1 false ≤ ys.length → choose ys ∈ Arch.Icelake.utf16to8)
    (hreal : ∀ ys, need 31 1 false ≤ ys.length →
      choose ys = Arch.Icelake.ahead8 → ∃ v, v < 65536 ∧
        ys.take 32 = ys.take 31 ++ [Arch.Icelake.w8 v] ∧
        (extra ys ≤ Arch.Icelake.spill v ∨ (0xDC00 ≤ v ∧ v < 0xE000 ∧ extra ys ≤ 2) ∨
          extra ys = 0))
    (xs : List Nat) (pos out n : Nat) (h : Via xs pos out) :
    out + Arch.Icelake.realWritten8 (need 31 1 false) choose extra n
      ((xs.drop pos).map w8) ≤ L (wR xs) :=
  via_call_store xs pos out _ h
    (Arch.Icelake.safe_utf16to8_real choose extra hc hreal n _)

/-- rvv (`vl`-exact stores). -/
theorem via_rvv (V : Nat) (choose : List Nat → Iter)
    (hc : ∀ ys, 1 ≤ ys.length → choose ys ∈ Arch.Rvv.vlaFamily V)
    (xs : List Nat) (pos out n : Nat) (h : Via xs pos out) :
    out + written (need 0 0 true) choose n ((xs.drop pos).map w8) ≤ L (wR xs) :=
  via_call_store xs pos out _ h (Arch.Rvv.safe_vla V choose hc n _)

/-- Unpaired surrogates cost 1 byte more under replacement: `[0xD800, 0x41]`
is 3 + 1 bytes with replacement, 2 + 1 without. -/
example : countRepl [0xD800, 0x41] = 4 ∧ L ([0xD800, 0x41].map w8) = 3 ∧
    countRepl [0xD800, 0xDC00] = 4 ∧ L ([0xD800, 0xDC00].map w8) = 4 := by decide

/-! # Part B: UTF-8 → UTF-16

## (i) Store overshoot of the UTF-8 → UTF-16 kernels

An `MPath` is `(stored, f)` with `f` the leading bytes of the consumed bytes
plus the next one: `f - 1` code points for a call that starts on a leading
byte, and a call on a validated block produces at least one UTF-16 unit per
code point. So `s - (f - 1)` bounds what a call stores past what it produces.
For arm64's `0x888` pair `(6, 4)`: on a block that passed the check the leads
are `F0..F4`, the check passes and the call produces 6 units, more than
`f - 1 = 3`. -/

/-- Every store of the list ends at most 4 units past the produced output. -/
def gap4 (ps : List MPath) : Bool := ps.all fun p => decide (p.s ≤ p.f - 1 + 4)

/-- Every UTF-8 → UTF-16 path list of the generic decoder: westmere
(`Utf8.x86_utf16`), haswell, ppc64, lsx, lasx (with the exact paths kept), and
arm64. Ice Lake and rvv store exact counts (`Icelake.safe_utf8`,
`Rvv.utf8to16`), overshoot 0. -/
theorem gap_all :
    gap4 x86_utf16 = true ∧ gap4 Arch.Haswell.utf8to16 = true ∧
    gap4 Arch.Ppc64.ppc_utf16 = true ∧ gap4 la_utf16 = true ∧
    gap4 Arch.Lsx.utf8to16 = true ∧ gap4 Arch.Lasx.lasx_utf16 = true ∧
    gap4 arm_utf16 = true := by decide

/-- 4 is attained: `0xfff` stores 16 units for 12 (x86, ppc64), `0x924` 8 for
4 (x86, ppc64, lsx, lasx). -/
theorem gap_tight :
    x86_utf16.any (fun p => p.s == 16 && p.f == 13) = true ∧ 16 = (13 - 1) + 4 ∧
    la_utf16.any (fun p => p.s == 8 && p.f == 5) = true ∧ 8 = (5 - 1) + 4 := by decide

/-- `gap4` is `okK 3`, which the header proofs already check. -/
theorem gap4_of_okK (ps : List MPath) (h : okK 3 ps = true) : gap4 ps = true := by
  simp only [okK, gap4, List.all_eq_true, decide_eq_true_eq] at *
  intro p hp
  have := h p hp
  omega

/-- A call that starts at output `o`, produces `e ≥ f - 1` units, and takes a
path of a `gap4` list stores no further than `o + e + 4`. -/
theorem path_overshoot (ps : List MPath) (hg : gap4 ps = true) (q : MPath)
    (hq : q ∈ ps) (o e : Nat) (he : q.f ≤ e + 1) : o + q.s ≤ o + e + 4 := by
  have := of_decide_eq_true ((List.all_eq_true.mp hg) q hq)
  omega

/-! ## (ii) The scalar decoder with replacement

`transcode_with_replacement`
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:77-188)
transcribed as an automaton over bytes (`Nat`). The 16-byte ASCII fast path
(:110-127) stores one unit per ASCII byte and leaves the state as is, which is
what `lead` does for each of those bytes. `x & 0x3F` is `x % 64` etc. -/

structure St where
  needed : Nat
  seen : Nat
  cp : Nat
  lo : Nat
  hi : Nat
deriving DecidableEq, Repr

def init : St := ⟨0, 0, 0, 0x80, 0xBF⟩

/-- Units stored by `store(c)` (:86-108). -/
def units (c : Nat) : Nat := if 0x10000 ≤ c then 2 else 1

/-- The `needed == 0` branch (:129-161): new state and units stored. -/
def lead (b : Nat) : St × Nat :=
  if b < 0x80 then (init, 1)
  else if b ≤ 0xDF then
    (if b < 0xC2 then (init, 1) else (⟨1, 0, b % 32, 0x80, 0xBF⟩, 0))
  else if b ≤ 0xEF then
    (⟨2, 0, b % 16, if b = 0xE0 then 0xA0 else 0x80, if b = 0xED then 0x9F else 0xBF⟩, 0)
  else if b ≤ 0xF4 then
    (⟨3, 0, b % 8, if b = 0xF0 then 0x90 else 0x80, if b = 0xF4 then 0x8F else 0xBF⟩, 0)
  else (init, 1)

/-- One byte. Out of range (:163-171): store U+FFFD, reset, and process the
same byte again (`continue` without `i++`), which is `lead b`. Otherwise
(:172-182) accumulate, and store when the sequence is complete. -/
def step (s : St) (b : Nat) : St × Nat :=
  if s.needed = 0 then lead b
  else if b < s.lo ∨ s.hi < b then ((lead b).1, 1 + (lead b).2)
  else if s.seen + 1 = s.needed then (init, units (s.cp * 64 + b % 64))
  else (⟨s.needed, s.seen + 1, s.cp * 64 + b % 64, 0x80, 0xBF⟩, 0)

def run : St → List Nat → St × Nat
  | s, [] => (s, 0)
  | s, b :: bs => ((run (step s b).1 bs).1, (step s b).2 + (run (step s b).1 bs).2)

/-- `count_with_replacement` (:203-207): the units of the loop plus the final
U+FFFD of a pending sequence (:184-186). -/
def count (bs : List Nat) : Nat :=
  (run init bs).2 + (if (run init bs).1.needed = 0 then 0 else 1)

def Inv (s : St) (n w : Nat) : Prop :=
  (s.needed = 0 → n ≤ 3 * w) ∧
  (s.needed ≠ 0 → n ≤ 3 * w + 1 + s.seen ∧ s.seen < s.needed ∧ s.needed ≤ 3 ∧
    0x80 ≤ s.lo ∧ s.hi ≤ 0xBF ∧
    (s.needed = 3 → s.seen = 0 → 1 ≤ s.cp ∨ 0x90 ≤ s.lo) ∧
    (s.needed = 3 → s.seen = 1 → 16 ≤ s.cp) ∧
    (s.needed = 3 → s.seen = 2 → 1024 ≤ s.cp))

theorem lead_inv (b n w : Nat) (h : n ≤ 3 * w) :
    Inv (lead b).1 (n + 1) (w + (lead b).2) := by
  unfold lead Inv init
  repeat' split
  all_goals (refine ⟨?_, ?_⟩ <;> intros <;> simp_all <;> omega)

theorem step_inv (s : St) (b n w : Nat) (h : Inv s n w) :
    Inv (step s b).1 (n + 1) (w + (step s b).2) := by
  obtain ⟨h0, h1⟩ := h
  unfold step
  by_cases hn : s.needed = 0
  · rw [ite_eq_left hn]; exact lead_inv b n w (h0 hn)
  · rw [ite_eq_right hn]
    obtain ⟨hn1, hsn, hn3, hlo, hhi, c0, c1, c2⟩ := h1 hn
    by_cases hr : b < s.lo ∨ s.hi < b
    · rw [ite_eq_left hr]
      have := lead_inv b n (w + 1) (by omega)
      simpa [Nat.add_assoc] using this
    · rw [ite_eq_right hr]
      by_cases hc : s.seen + 1 = s.needed
      · rw [ite_eq_left hc]
        refine ⟨fun _ => ?_, fun h => absurd rfl h⟩
        unfold units
        split
        · omega
        · -- a 4-byte sequence always reaches 0x10000
          have : s.needed ≤ 2 := by
            by_cases h3 : s.needed = 3
            · have := c2 h3 (by omega)
              omega
            · omega
          omega
      · rw [ite_eq_right hc]
        refine ⟨fun h => absurd h hn, fun _ => ?_⟩
        simp only
        refine ⟨by omega, by omega, hn3, by omega, by omega, ?_, ?_, ?_⟩
        · intro _ h; omega
        · intro h3 hs1
          have hs0 : s.seen = 0 := by omega
          rcases c0 h3 hs0 with hcp | hlo9
          · omega
          · omega
        · intro h3 hs2
          have := c1 h3 (by omega)
          omega

theorem run_inv (bs : List Nat) : ∀ (s : St) (n w : Nat), Inv s n w →
    Inv (run s bs).1 (n + bs.length) (w + (run s bs).2) := by
  induction bs with
  | nil => intro s n w h; simpa [run] using h
  | cons b bs ih =>
    intro s n w h
    have h1 := step_inv s b n w h
    have h2 := ih _ _ _ h1
    simp only [run, List.length_cons]
    have e1 : n + (bs.length + 1) = n + 1 + bs.length := by omega
    have e2 : w + ((step s b).2 + (run (step s b).1 bs).2) =
        w + (step s b).2 + (run (step s b).1 bs).2 := by omega
    rw [e1, e2]
    exact h2

/-- **Every byte string decodes to at least `ceil(len / 3)` units**: a
replacement character consumes at most 3 bytes, a valid character gives at
least one unit per 3 bytes (two for four bytes). -/
theorem count_ge (bs : List Nat) : bs.length ≤ 3 * count bs := by
  have h := run_inv bs init 0 0 ⟨fun _ => by omega, fun h => absurd rfl h⟩
  simp only [Nat.zero_add] at h
  obtain ⟨h0, h1⟩ := h
  unfold count
  by_cases hn : (run init bs).1.needed = 0
  · rw [ite_eq_left hn]; have := h0 hn; omega
  · rw [ite_eq_right hn]; obtain ⟨a, b, c, _⟩ := h1 hn; omega

/-- 32 held-back bytes give at least 11 units. -/
theorem count_ge_11 (bs : List Nat) (h : 32 ≤ bs.length) : 11 ≤ count bs := by
  have := count_ge bs; omega

/-- `F0 90 80` ×10, then `F0 90`: each truncated 4-byte sequence is one maximal
subpart and one U+FFFD. 32 bytes, 11 units: `count_ge_11` is tight. -/
def tight32 : List Nat :=
  (List.replicate 10 [0xF0, 0x90, 0x80]).flatten ++ [0xF0, 0x90]

theorem tight32_count : tight32.length = 32 ∧ count tight32 = 11 := by decide

/-- On 32 bytes of valid 4-byte characters the count is 16, the figure that
an earlier C++ comment gave for the held-back bytes; it holds only for valid
input. The comment (src/utf8_to_utf16_with_replacement.inl.h:46-52) now says
11, after `count_ge_11` and `tight32_count`. -/
example : count (List.replicate 8 [0xF0, 0x9F, 0x98, 0x80]).flatten = 16 := by decide

/-- After a complete valid prefix the decoder is back in its initial state. -/
def ValidRun (v : List Nat) (u : Nat) : Prop := run init v = (init, u)

theorem run_append (a b : List Nat) : ∀ s : St,
    run s (a ++ b) = ((run (run s a).1 b).1, (run s a).2 + (run (run s a).1 b).2) := by
  induction a with
  | nil => intro s; simp [run]
  | cons x a ih =>
    intro s
    simp only [List.cons_append, run, ih]
    simp only [Prod.mk.injEq, true_and]
    omega

theorem count_valid_append (v r : List Nat) (u : Nat) (h : ValidRun v u) :
    count (v ++ r) = u + count r := by
  unfold count
  rw [run_append, h]
  simp only
  omega

example : ValidRun [0x41, 0xC3, 0xA9, 0xF0, 0x9F, 0x98, 0x80] 4 := by
  unfold ValidRun; decide

/-! ## (iii) The driver

src/utf8_to_utf16_with_replacement.inl.h. `utf8_boundary_before` (:55-65) and
the window (:109-112). -/

def isCont (b : Nat) : Bool := decide (0x80 ≤ b ∧ b ≤ 0xBF)

/-- `utf8_boundary_before(input, limit)`: back off over continuation bytes. -/
def boundaryBefore (bs : List Nat) : Nat → Nat
  | 0 => 0
  | i + 1 => if isCont (bs.getD (i + 1) 0) then boundaryBefore bs i else i + 1

theorem boundaryBefore_le (bs : List Nat) (l : Nat) : boundaryBefore bs l ≤ l := by
  induction l with
  | zero => simp [boundaryBefore]
  | succ i ih => unfold boundaryBefore; split <;> omega

/-- The window handed to the kernel leaves at least `kernel_overflow_tail = 32`
bytes (:53, :109-112). -/
theorem held_back (bs : List Nat) (pos : Nat) (hr : 32 + 64 < bs.length - pos) :
    pos + boundaryBefore (bs.drop pos) (bs.length - pos - 32) + 32 ≤ bs.length := by
  have := boundaryBefore_le (bs.drop pos) (bs.length - pos - 32)
  omega

/-- One U+FFFD for the `m`-byte maximal subpart at the head of `ys`, after
which the decoder restarts: `maximal_subpart`
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16_with_replacement.h:33-69)
follows the same `lo`/`hi` automaton as `transcode_with_replacement`. -/
def Subpart (ys : List Nat) (m : Nat) : Prop := count ys = 1 + count (ys.drop m)

/-- States `(pos, written)` of `transcode_utf8_to_utf16_with_replacement`
(src/utf8_to_utf16_with_replacement.inl.h:88-188) and of
`convert_using_locations` (:194-236) on input `xs`.
* `valid`: a valid run of `w` bytes converted (bulk success :114-120; or a
  `convert_valid_prefix` before an error :159-166, :213-216, :227-230).
* `error`: a valid run of `k` bytes, then U+FFFD for the maximal subpart of
  `m` bytes at the error (:122-143, :170-185, :217-221). -/
inductive RReach (xs : List Nat) : Nat → Nat → Prop
  | start : RReach xs 0 0
  | valid (pos written w u : Nat) (h : RReach xs pos written)
      (hv : ValidRun ((xs.drop pos).take w) u) :
      RReach xs (pos + w) (written + u)
  | error (pos written k u m : Nat) (h : RReach xs pos written)
      (hv : ValidRun ((xs.drop pos).take k) u)
      (hm : Subpart (xs.drop (pos + k)) m) :
      RReach xs (pos + k + m) (written + u + 1)

theorem split_at (xs : List Nat) (pos w : Nat) :
    (xs.drop pos).take w ++ xs.drop (pos + w) = xs.drop pos := by
  rw [← List.drop_drop, List.take_append_drop]

theorem rreach_inv (xs : List Nat) (pos written : Nat) (h : RReach xs pos written) :
    written + count (xs.drop pos) = count xs := by
  induction h with
  | start => simp
  | valid pos written w u h hv ih =>
    have := count_valid_append _ (xs.drop (pos + w)) u hv
    rw [split_at] at this
    rw [← ih, this]; omega
  | error pos written k u m h hv hm ih =>
    have := count_valid_append _ (xs.drop (pos + k)) u hv
    rw [split_at] at this
    unfold Subpart at hm
    rw [List.drop_drop] at hm
    rw [← ih, this, hm]; omega

/-- U+FFFD stores (:127, :174, :217), `convert_valid_prefix` stores bounded by
the units of the valid run (`Utf8.valid_utf16_store` and its arm64 form), and
`finish_with_scalar` (:67-86, writing `count` of the rest one unit at a time)
end inside the buffer. A nested `transcode` on the rest (:209, :233) whose own
stores end within `count (xs.drop pos)` does too. -/
theorem rreach_exact_store (xs : List Nat) (pos written se : Nat)
    (h : RReach xs pos written) (hse : se ≤ count (xs.drop pos)) :
    written + se ≤ count xs := by
  have := rreach_inv xs pos written h; omega

/-- **The bulk `_with_errors` call.** In a reachable state at `pos`, the kernel
runs on a window ending 32 bytes before the end of the input (`held_back`).
Take a store of the kernel, at a moment when it has committed the output of
a valid prefix of `c ≤ window` bytes (`u` units), ending at most 4 past it
(part (i)). It ends inside `count xs`, with 7 units to spare. -/
theorem bulk_store (xs : List Nat) (pos written window c u se : Nat)
    (h : RReach xs pos written) (hwin : pos + window + 32 ≤ xs.length)
    (hc : c ≤ window) (hv : ValidRun ((xs.drop pos).take c) u)
    (hse : se ≤ u + 4) : written + se + 7 ≤ count xs := by
  have hi := rreach_inv xs pos written h
  have hsplit := count_valid_append _ (xs.drop (pos + c)) u hv
  rw [split_at] at hsplit
  have hlen : 32 ≤ (xs.drop (pos + c)).length := by simp; omega
  have := count_ge_11 _ hlen
  omega

/-- The same with a path of the kernel's list: the call starts at kernel
output `o`, produces `e ≥ q.f - 1` units, and `o + e = u` is the committed
output of the valid prefix after the call. -/
theorem bulk_path_store (xs : List Nat) (pos written window c u : Nat)
    (ps : List MPath) (hg : gap4 ps = true) (q : MPath) (hq : q ∈ ps) (o e : Nat)
    (h : RReach xs pos written) (hwin : pos + window + 32 ≤ xs.length)
    (hc : c ≤ window) (hv : ValidRun ((xs.drop pos).take c) u)
    (hoe : o + e = u) (he : q.f ≤ e + 1) :
    written + (o + q.s) ≤ count xs := by
  have := path_overshoot ps hg q hq o e he
  have := bulk_store xs pos written window c u (o + q.s) h hwin hc hv (by omega)
  omega

end SafetyMargins.Replacement
