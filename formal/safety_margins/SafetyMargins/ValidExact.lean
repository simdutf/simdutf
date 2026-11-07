import SafetyMargins.Exact
import SafetyMargins.Arch.Arm64
import SafetyMargins.Arch.Lsx
import SafetyMargins.Arch.Lasx
/-
`valid_utf8_to_utf16.h` and `valid_utf8_to_utf32.h`, the `convert_valid`
decoders, on the input their contract allows: valid UTF-8 (`Valid8`, the
automaton of Unicode Table 3-7: shortest form, no surrogates, at most
U+10FFFF).

`Utf8.valid_utf16_store`, `Utf8.valid_utf32_store`,
`Arm64.valid_utf16_arm_store`, `Lsx.valid_utf32_lsx_store` and
`Lasx.valid_utf32_lasx_store` assume that the cursor is `L` of the consumed
prefix and that the unread bytes are complete characters. Here both are
proved, from a model of the loop (`VX`), as `ValidLatin1.lean` does for
`valid_utf8_to_latin1.h`:

* a block of valid UTF-8 that starts on a character boundary passes
  `check_utf8_bytes` (`checkedAt_of_valid`; the finite fact is `closure`), so
  the exactness lemmas of `Exact.lean` apply to every call on valid input
  (`vx_w01`, `vx_count01`, `vx_utf16_145`, `vx_utf16_3`, `vx_count16`,
  `vx_888`);
* by induction over `VX`, the cursor is `L` of the consumed bytes and they end
  on a character boundary (`vx_inv`);
* the bytes after a boundary are complete characters (`tail_expand`).

The store theorems (`valid_utf32_vx`, `valid_utf32_lsx_vx`,
`valid_utf32_lasx_vx`, `valid_utf16_vx`, `valid_utf16_arm_vx`) then apply the
five theorems above with no hypothesis on the cursor or the tail.
-/
namespace SafetyMargins.ValidExact
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.Validator SafetyMargins.Exact
  SafetyMargins.Tables SafetyMargins.TableFacts

/-! ## The contract: valid UTF-8 -/

/-- The UTF-8 automaton (Unicode Table 3-7, well-formed byte sequences). -/
def step (s b : Nat) : Nat :=
  if s = 0 then
    if b < 0x80 then 0
    else if b < 0xC2 then 8
    else if b < 0xE0 then 1
    else if b = 0xE0 then 4
    else if b = 0xED then 5
    else if b < 0xF0 then 2
    else if b = 0xF0 then 6
    else if b < 0xF4 then 3
    else if b = 0xF4 then 7
    else 8
  else if s = 1 then (if 0x80 ≤ b ∧ b < 0xC0 then 0 else 8)
  else if s = 2 then (if 0x80 ≤ b ∧ b < 0xC0 then 1 else 8)
  else if s = 3 then (if 0x80 ≤ b ∧ b < 0xC0 then 2 else 8)
  else if s = 4 then (if 0xA0 ≤ b ∧ b < 0xC0 then 1 else 8)
  else if s = 5 then (if 0x80 ≤ b ∧ b < 0xA0 then 1 else 8)
  else if s = 6 then (if 0x90 ≤ b ∧ b < 0xC0 then 2 else 8)
  else if s = 7 then (if 0x80 ≤ b ∧ b < 0x90 then 2 else 8)
  else 8

def run (s : Nat) (bs : List Nat) : Nat := bs.foldl step s

def Valid8 (bs : List Nat) : Prop := run 0 bs = 0

/-- Byte `t` is a character boundary. -/
def Bd (bs : List Nat) (t : Nat) : Prop := run 0 (bs.take t) = 0


/-! ## The check on valid input

`check_utf8_bytes` sees the byte `c`, the byte `p1` before it, and whether
`p2 ≥ 0xE0`, `p2 ≥ 0xF0` and `p3 ≥ 0xF0`. `goodA` is what the automaton state
before `c` says about those. -/
def goodA (s : Nat) (f3 e2 f2 : Bool) (p1 : Nat) : Bool :=
  if s = 0 then !f3 && !e2 && !f2 && (decide (p1 < 0x80) || isCont p1)
  else if s = 1 then !f2 && ((decide (0xC2 ≤ p1) && decide (p1 < 0xE0) && !e2 && !f3) ||
    (isCont p1 && e2 && !f3) || (isCont p1 && !e2 && f3))
  else if s = 2 then !f3 && ((decide (0xE1 ≤ p1) && decide (p1 < 0xF0) && p1 != 0xED && !e2 && !f2) ||
    (isCont p1 && e2 && f2))
  else if s = 3 then decide (0xF1 ≤ p1) && decide (p1 < 0xF4) && !f3 && !e2 && !f2
  else if s = 4 then p1 == 0xE0 && !f3 && !e2 && !f2
  else if s = 5 then p1 == 0xED && !f3 && !e2 && !f2
  else if s = 6 then p1 == 0xF0 && !f3 && !e2 && !f2
  else if s = 7 then p1 == 0xF4 && !f3 && !e2 && !f2
  else false

/-- `must23` through the two comparisons it uses. -/
def m23 (xm f3 e2 : Bool) : Bool := if xm then e2 != f3 else e2 || f3

def bools : List Bool := [false, true]

/-- From a state `goodA` allows, every byte the automaton accepts passes the
check (`sc` agrees with `must23`), and the next state is again allowed. -/
def closureOK : Bool :=
  bools.all fun xm => (List.range 8).all fun s => bools.all fun f3 => bools.all fun e2 =>
    bools.all fun f2 => (List.range 256).all fun p1 => !goodA s f3 e2 f2 p1 ||
      (List.range 256).all fun c => step s c == 8 ||
        (sc p1 c == (if m23 xm f3 e2 then 0x80 else 0) &&
          goodA (step s c) f2 (decide (0xE0 ≤ p1)) (decide (0xF0 ≤ p1)) c)


theorem closure : closureOK = true := by native_decide

theorem goodA_lt {s f3 e2 f2 p1} (h : goodA s f3 e2 f2 p1 = true) : s < 8 := by
  unfold goodA at h
  repeat (first | omega | (split at h; try omega))
  all_goals simp at h

theorem mem_bools (b : Bool) : b ∈ bools := by cases b <;> simp [bools]

theorem closure_at (xm : Bool) {s : Nat} {f3 e2 f2 : Bool} {p1 c : Nat}
    (hg : goodA s f3 e2 f2 p1 = true) (hp1 : p1 < 256) (hc : c < 256) (hs : step s c ≠ 8) :
    sc p1 c = (if m23 xm f3 e2 then 0x80 else 0) ∧
      goodA (step s c) f2 (decide (0xE0 ≤ p1)) (decide (0xF0 ≤ p1)) c = true := by
  have h := closure
  unfold closureOK at h
  have h := List.all_eq_true.mp h xm (mem_bools xm)
  have h := List.all_eq_true.mp h s (List.mem_range.mpr (goodA_lt hg))
  have h := List.all_eq_true.mp h f3 (mem_bools f3)
  have h := List.all_eq_true.mp h e2 (mem_bools e2)
  have h := List.all_eq_true.mp h f2 (mem_bools f2)
  have h := List.all_eq_true.mp h p1 (List.mem_range.mpr hp1)
  rw [hg] at h
  simp only [Bool.not_true, Bool.false_or] at h
  have h := List.all_eq_true.mp h c (List.mem_range.mpr hc)
  have hs' : (step s c == 8) = false := by simpa using hs
  rw [hs'] at h
  simp only [Bool.false_or, Bool.and_eq_true, beq_iff_eq] at h
  exact h

theorem must23_m23 (xm : Bool) (p3 p2 : Nat) :
    must23 xm p3 p2 = m23 xm (decide (0xF0 ≤ p3)) (decide (0xE0 ≤ p2)) := rfl

/-! ### The automaton -/

theorem run_cons (s x : Nat) (r : List Nat) : run s (x :: r) = run (step s x) r := rfl

theorem run_append (s : Nat) (a b : List Nat) : run s (a ++ b) = run (run s a) b := by
  unfold run; rw [List.foldl_append]

theorem step_8 (x : Nat) : step 8 x = 8 := by unfold step; simp

theorem run_8 (r : List Nat) : run 8 r = 8 := by
  induction r with
  | nil => rfl
  | cons x r ih => rw [run_cons, step_8, ih]

theorem step_big (s x : Nat) (hx : 256 ≤ x) : step s x = 8 := by
  unfold step
  repeat (first | omega | split)

theorem step_noncont (s x : Nat) (hs : s ≠ 0) (hx : isCont x = false) : step s x = 8 := by
  simp only [isCont, Bool.and_eq_false_iff, decide_eq_false_iff_not] at hx
  unfold step
  repeat (first | omega | split)

theorem step_ascii (s x : Nat) (hx : x < 0x80) : step s x = 0 ∨ step s x = 8 := by
  unfold step
  repeat (first | omega | split)

theorem step0_cont (x : Nat) (hx : isCont x = true) : step 0 x = 8 := by
  simp only [isCont, Bool.and_eq_true, decide_eq_true_eq] at hx
  unfold step
  repeat (first | omega | split)

theorem run_take_succ (s : Nat) (ys : List Nat) (q : Nat) (hq : q < ys.length) :
    run s (ys.take (q + 1)) = step (run s (ys.take q)) (ys.getD q 0) := by
  rw [List.take_add_one, List.getElem?_eq_getElem hq, run_append]
  simp [run, List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hq]

theorem run_take_ne8 {s : Nat} {ys : List Nat} (h : run s ys = 0) (q : Nat) :
    run s (ys.take q) ≠ 8 := by
  intro h8
  have := run_append s (ys.take q) (ys.drop q)
  rw [List.take_append_drop, h8, run_8, h] at this
  exact absurd this (by decide)

theorem valid_drop {bs : List Nat} (hv : Valid8 bs) {t : Nat} (hb : Bd bs t) :
    Valid8 (bs.drop t) := by
  unfold Valid8 at *
  have := run_append 0 (bs.take t) (bs.drop t)
  rw [List.take_append_drop, hb] at this
  rw [← this, hv]

theorem valid_lt256 {bs : List Nat} (hv : Valid8 bs) : ∀ b ∈ bs, b < 256 := by
  intro b hb
  obtain ⟨u, v, rfl⟩ := List.append_of_mem hb
  unfold Valid8 at hv
  rw [run_append, run_cons] at hv
  by_cases h : b < 256
  · exact h
  · rw [step_big _ _ (by omega), run_8] at hv
    exact absurd hv (by decide)

theorem getD_valid {bs : List Nat} (hv : Valid8 bs) (i : Nat) : bs.getD i 0 < 256 :=
  getD_lt bs (valid_lt256 hv) i

/-- A non-continuation byte is a character boundary. -/
theorem lead_bd {bs : List Nat} (hv : Valid8 bs) {i : Nat} (hi : i < bs.length)
    (hl : isCont (bs.getD i 0) = false) : Bd bs i := by
  have h1 := run_take_ne8 hv (i + 1)
  rw [run_take_succ 0 bs i hi] at h1
  unfold Bd
  by_cases hs : run 0 (bs.take i) = 0
  · exact hs
  · exact absurd (step_noncont _ _ hs hl) h1

/-- The byte at a boundary is not a continuation byte. -/
theorem bd_lead {bs : List Nat} (hv : Valid8 bs) {i : Nat} (hb : Bd bs i) (hi : i < bs.length) :
    isCont (bs.getD i 0) = false := by
  have h1 := run_take_ne8 hv (i + 1)
  rw [run_take_succ 0 bs i hi, hb] at h1
  cases hc : isCont (bs.getD i 0)
  · rfl
  · exact absurd (step0_cont _ hc) h1

/-- An ASCII byte ends a character. -/
theorem bd_ascii {bs : List Nat} (hv : Valid8 bs) {i : Nat} (hi : i < bs.length)
    (ha : bs.getD i 0 < 0x80) : Bd bs (i + 1) := by
  have h1 := run_take_ne8 hv (i + 1)
  unfold Bd
  rw [run_take_succ 0 bs i hi] at h1 ⊢
  rcases step_ascii (run 0 (bs.take i)) _ ha with h | h
  · exact h
  · exact absurd h h1

/-! ### Valid input passes the check -/

/-- The automaton state before byte `q` of `g`. -/
def stg (g : Nat → Nat) : Nat → Nat
  | 0 => 0
  | q + 1 => step (stg g q) (g q)

/-- A block whose automaton never fails passes `check_utf8_bytes`. -/
theorem checked_of_stg (xm : Bool) (g : Nat → Nat) (n : Nat) (hg : ∀ i, g i < 256)
    (h0 : g 0 = 0) (h1 : g 1 = 0) (h2 : g 2 = 0) (hne : ∀ q, q ≤ n → stg g q ≠ 8) :
    Checked xm g n := by
  have inv : ∀ k, 3 + k ≤ n → goodA (stg g (k + 3)) (decide (0xF0 ≤ g k))
      (decide (0xE0 ≤ g (k + 1))) (decide (0xF0 ≤ g (k + 1))) (g (k + 2)) = true := by
    intro k
    induction k with
    | zero =>
      intro _
      simp only [stg, h0, h1, h2]
      decide
    | succ k ih =>
      intro hk
      have hn8 := hne (k + 1 + 3) (by omega)
      have hstep : stg g (k + 1 + 3) = step (stg g (k + 3)) (g (k + 3)) := rfl
      rw [hstep] at hn8 ⊢
      have := (closure_at false (ih (by omega)) (hg _) (hg _) hn8).2
      simpa [show k + 1 + 1 = k + 2 by omega, show k + 1 + 2 = k + 3 by omega] using this
  refine ⟨hg, fun q h3 hq => ?_⟩
  obtain ⟨k, rfl⟩ : ∃ k, q = k + 3 := ⟨q - 3, by omega⟩
  have hn8 := hne (k + 3 + 1) (by omega)
  have hstep : stg g (k + 3 + 1) = step (stg g (k + 3)) (g (k + 3)) := rfl
  rw [hstep] at hn8
  have := (closure_at xm (inv k (by omega)) (hg _) (hg _) hn8).1
  show sc (g (k + 3 - 1)) (g (k + 3)) = if must23 xm (g (k + 3 - 3)) (g (k + 3 - 2)) then 0x80 else 0
  rw [show k + 3 - 1 = k + 2 by omega, show k + 3 - 3 = k by omega,
    show k + 3 - 2 = k + 1 by omega, must23_m23]
  exact this

theorem stg_blk (bs : List Nat) (pos : Nat) :
    ∀ q, q ≤ (bs.drop pos).length → stg (blk bs pos) (q + 3) = run 0 ((bs.drop pos).take q) := by
  intro q
  induction q with
  | zero => intro _; simp [stg, blk, step, run]
  | succ q ih =>
    intro hq
    have hstep : stg (blk bs pos) (q + 1 + 3) = step (stg (blk bs pos) (q + 3)) (blk bs pos (q + 3)) := rfl
    rw [hstep, ih (by omega), run_take_succ 0 _ q (by omega)]
    congr 1
    unfold blk
    simp only [show ¬ (q + 3 < 3) by omega, ↓reduceIte]
    rw [List.getD_eq_getElem?_getD, List.getD_eq_getElem?_getD, List.getElem?_drop]
    congr 2

/-- **A block of valid UTF-8 that starts on a character boundary passes the
check**, with either `must_be_2_3_continuation`. So every lemma of
`Exact.lean` that assumes `CheckedAt` holds at the blocks the loop enters. -/
theorem checkedAt_of_valid (xm : Bool) {bs : List Nat} (hv : Valid8 bs) {pos : Nat}
    (hb : Bd bs pos) (hlen : pos + 64 ≤ bs.length) : CheckedAt xm bs pos := by
  refine ⟨valid_lt256 hv, hlen, ?_⟩
  have hd := valid_drop hv hb
  apply checked_of_stg xm _ 67
  · intro i; unfold blk; split
    · omega
    · exact getD_valid hv _
  · rfl
  · rfl
  · rfl
  · intro q hq
    by_cases h3 : q < 3
    · rcases (by omega : q = 0 ∨ q = 1 ∨ q = 2) with rfl | rfl | rfl <;> simp [stg, blk, step]
    · obtain ⟨k, rfl⟩ : ∃ k, q = k + 3 := ⟨q - 3, by omega⟩
      rw [stg_blk bs pos k (by simp; omega)]
      exact run_take_ne8 hd k

/-! ## The loop -/

/-- The loop of `valid_utf8_to_utf16.h` and `valid_utf8_to_utf32.h` on bytes
`bs`, weights `bs.map w`: `false t t o` is the head of the outer loop,
`true pos t o` the head of the inner loop of the block at `pos`. As
`ValidLatin1.VReach`, entering a block has no test on its first byte, only the
loop test (weakened to `t + 64 ≤ length`). A call is exact (`XStep`): it ends
just before a leading byte and advances the cursor by `L` of what it consumed.
On valid input every call of the kernels is such a call (`vx_w01`,
`vx_count01`, `vx_utf16_145`, `vx_utf16_3`, `vx_count16`, `vx_888`). -/
inductive VX (w : Nat → Nat) (bs : List Nat) : Bool → Nat → Nat → Nat → Prop
  | start : VX w bs false 0 0 0
  /-- `in.is_ascii()`: 64 ASCII bytes, 64 units out. -/
  | ascii (t o : Nat) (h : VX w bs false t t o) (hlen : t + 64 ≤ bs.length)
      (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) :
      VX w bs false (t + 64) (t + 64) (o + 64)
  /-- The slow path: no test on the first byte. -/
  | enter (t o : Nat) (h : VX w bs false t t o) (hlen : t + 64 ≤ bs.length) :
      VX w bs true t t o
  /-- `while (pos < max_starting_point)`, one call. -/
  | call (pos t o c p : Nat) (h : VX w bs true pos t o) (ht : t < pos + 52)
      (hs : XStep (bs.map w) t c p) : VX w bs true pos (t + c) (o + p)
  | leave (pos t o : Nat) (h : VX w bs true pos t o) (ht : pos + 52 ≤ t) :
      VX w bs false t t o

/-- The weights of `utf32_length_from_utf8` and `utf16_length_from_utf8`: zero
exactly on continuation bytes, one on ASCII. -/
def WOK (w : Nat → Nat) : Prop := ContZero w ∧ ∀ b, b < 0x80 → w b = 1

theorem wok_w01 : WOK w01 :=
  ⟨w01_contZero, fun b hb => by unfold w01 isCont; simp; omega⟩

theorem wok_w16 : WOK w16 :=
  ⟨w16_contZero, fun b hb => by unfold w16; split <;> (try split) <;> omega⟩

/-- **The invariant.** At every state the cursor is exactly `L` of the
consumed bytes and the consumed bytes end on a character boundary. Inside a
block, the block starts on a boundary inside the input, and the next call
starts on a leading byte inside the input. -/
theorem vx_inv {w : Nat → Nat} (hw : WOK w) {bs : List Nat} (hv : Valid8 bs) :
    ∀ b pos t o, VX w bs b pos t o →
      o = L ((bs.map w).take t) ∧ Bd bs t ∧
      (b = true → Bd bs pos ∧ pos + 64 ≤ bs.length ∧ pos ≤ t ∧ t < bs.length ∧
        isCont (bs.getD t 0) = false) := by
  intro b pos t o h
  induction h with
  | start => exact ⟨by simp, rfl, fun h => by simp at h⟩
  | ascii t o h hlen hascii ih =>
    refine ⟨?_, ?_, fun h => by simp at h⟩
    · rw [L_take_add, L_ones (bs.map w) t 64 (fun i hi => by
        rw [getD_map w bs (t + i) (by omega), hw.2 _ (hascii i hi)]), ← ih.1]
    · have := bd_ascii hv (i := t + 63) (by omega) (hascii 63 (by omega))
      simpa using this
  | enter t o h hlen ih =>
    exact ⟨ih.1, ih.2.1, fun _ => ⟨ih.2.1, hlen, Nat.le_refl _, by omega,
      bd_lead hv ih.2.1 (by omega)⟩⟩
  | call pos t o c p h ht hs ih =>
    obtain ⟨hc, hnext, hp⟩ := hs
    obtain ⟨hbp, hlen, hpt, -, -⟩ := ih.2.2 rfl
    have hin : t + c < bs.length := by
      refine Classical.byContradiction fun hge => ?_
      apply hnext
      rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by simp; omega)]
      rfl
    have hl : isCont (bs.getD (t + c) 0) = false := by
      rw [getD_map w bs _ hin] at hnext
      cases hcb : isCont (bs.getD (t + c) 0)
      · rfl
      · exact absurd ((hw.1 _ (getD_valid hv _)).mpr hcb) hnext
    refine ⟨by rw [L_take_add, ← ih.1, hp], lead_bd hv hin hl,
      fun _ => ⟨hbp, hlen, by omega, hin, hl⟩⟩
  | leave pos t o h ht ih => exact ⟨ih.1, ih.2.1, fun h => by simp at h⟩

/-- Every block the loop enters passes `check_utf8_bytes`. -/
theorem vx_checked (xm : Bool) {w : Nat → Nat} (hw : WOK w) {bs : List Nat} (hv : Valid8 bs)
    {pos t o : Nat} (h : VX w bs true pos t o) : CheckedAt xm bs pos :=
  have hi := (vx_inv hw hv _ _ _ _ h).2.2 rfl
  checkedAt_of_valid xm hv hi.1 hi.2.1

/-! ## Every call on valid input is exact

The lemmas of `Exact.lean` on a block the loop entered (`vx_checked`). -/

/-- UTF-32: every table row (`utf8bigindex`, all rows `< 209`). -/
theorem vx_w01 {bs : List Nat} (hv : Valid8 bs) {pos t o : Nat} (h : VX w01 bs true pos t o)
    (ht : t < pos + 52) :
    let r := row (bs.map w01) t
    bigIdx[r.m]! < 209 ∧ XStep (bs.map w01) t bigConsumed[r.m]! (units bigIdx[r.m]!) :=
  have hi := (vx_inv wok_w01 hv _ _ _ _ h).2.2 rfl
  exact_w01 (vx_checked false wok_w01 hv h) hi.2.2.1 ht hi.2.2.2.2

/-- UTF-32: the fast paths (`0xfff`, `0x924`, `0xaaa`, `0x888`, 16 ASCII
bytes), which produce one unit per code-point end and end just before a
leading byte (the mask bit `c - 1` is set). -/
theorem vx_count01 {bs : List Nat} (hv : Valid8 bs) {pos t o c : Nat} (h : VX w01 bs true pos t o)
    (hc : 1 ≤ c) (hin : t + c < bs.length) (hnext : isCont (bs.getD (t + c) 0) = false) :
    XStep (bs.map w01) t c (sumUpTo (fun k => lead (bs.map w01) (t + 1 + k)) c) := by
  have hi := (vx_inv wok_w01 hv _ _ _ _ h).2.2 rfl
  have hn : (bs.map w01).getD (t + c) 0 ≠ 0 := by
    rw [getD_map _ _ _ hin]; exact w01_ne_zero hnext
  refine ⟨hc, hn, count_exact _ t c hc ?_ hn ?_⟩
  · rw [lead_map w01_contZero (valid_lt256 hv) t hi.2.2.2.1, hi.2.2.2.2]; rfl
  · intro k _
    unfold lead
    rcases getD_w01 bs (t + k) with h0 | h1
    · rw [h0]; rfl
    · rw [h1]; rfl
where
  w01_ne_zero {b : Nat} (h : isCont b = false) : w01 b ≠ 0 := by simp [w01, h]

/-- UTF-16: table rows below 145. -/
theorem vx_utf16_145 {bs : List Nat} (hv : Valid8 bs) {pos t o : Nat} (h : VX w16 bs true pos t o)
    (ht : t < pos + 52) (hi : bigIdx[(row (bs.map w16) t).m]! < 145) :
    XStep (bs.map w16) t bigConsumed[(row (bs.map w16) t).m]! (units bigIdx[(row (bs.map w16) t).m]!) :=
  have hs := (vx_inv wok_w16 hv _ _ _ _ h).2.2 rfl
  exact_utf16_145 (vx_checked false wok_w16 hv h) hs.2.2.1 ht hs.2.2.2.2 hi

/-- UTF-16: table rows with three code points, with the pair test of x86 and
ppc64 (`arm = false`) or of arm64, lsx and lasx (`arm = true`): a lane writes
a surrogate pair exactly for a 4-byte character, which weighs 2. -/
theorem vx_utf16_3 (arm : Bool) {bs : List Nat} (hv : Valid8 bs) {pos t o : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52)
    (hi1 : 145 ≤ bigIdx[(row (bs.map w16) t).m]!) (hi2 : bigIdx[(row (bs.map w16) t).m]! < 209) :
    XStep (bs.map w16) t bigConsumed[(row (bs.map w16) t).m]!
      ((if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 0) then 2 else 1) +
       (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 1) then 2 else 1) +
       (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 2) then 2 else 1)) :=
  have hs := (vx_inv wok_w16 hv _ _ _ _ h).2.2 rfl
  exact_utf16_3 arm (vx_checked arm wok_w16 hv h) hs.2.2.1 ht hs.2.2.2.2 hi1 hi2

/-- UTF-16: the mask fast paths (16 ASCII bytes, `0xaaa`, `0x924`, ...), one
unit per code-point end, on characters of at most 3 bytes. -/
theorem vx_count16 {bs : List Nat} (hv : Valid8 bs) {pos t o c : Nat} (h : VX w16 bs true pos t o)
    (htc : t + c < pos + 64) (hc : 1 ≤ c) (hnext : isCont (bs.getD (t + c) 0) = false)
    (hshape : ∀ j, j + 3 < c → isCont (bs.getD (t + j) 0) = false →
      ¬ (isCont (bs.getD (t + j + 1) 0) = true ∧ isCont (bs.getD (t + j + 2) 0) = true ∧
          isCont (bs.getD (t + j + 3) 0) = true)) :
    XStep (bs.map w16) t c (sumUpTo (fun k => lead (bs.map w16) (t + 1 + k)) c) :=
  have hs := (vx_inv wok_w16 hv _ _ _ _ h).2.2 rfl
  exact_count (vx_checked false wok_w16 hv h) hs.2.2.1 htc hc hs.2.2.2.2 hnext hshape

/-- UTF-16, the `0x888` path of arm64, lsx and lasx: the lead check passes (so
arm64's store of 6 units before the check is the output of the call), and the
call is exact: 12 bytes, 6 units. -/
theorem vx_888 {bs : List Nat} (hv : Valid8 bs) {pos t o : Nat} (h : VX w16 bs true pos t o)
    (ht : t < pos + 52) (hm : emask (bs.map w16) t 12 = 0x888) :
    (∀ j, j < 3 → bs.getD (t + 4 * j) 0 &&& 0xf8 = 0xf0) ∧ XStep (bs.map w16) t 12 6 :=
  have hs := (vx_inv wok_w16 hv _ _ _ _ h).2.2 rfl
  exact_888 (vx_checked true wok_w16 hv h) hs.2.2.1 ht hs.2.2.2.2 hm

/-! ## Valid input is a list of complete characters -/

/-- Continuation bytes still needed in state `s`. -/
def need (s : Nat) : Nat := if s = 1 then 1 else if s = 2 ∨ s = 4 ∨ s = 5 then 2 else 3

theorem step_le (s x : Nat) : step s x ≤ 8 := by
  unfold step
  repeat (first | omega | split)

theorem step1 (x : Nat) : step 1 x = if 0x80 ≤ x ∧ x < 0xC0 then 0 else 8 := rfl
theorem step2 (x : Nat) : step 2 x = if 0x80 ≤ x ∧ x < 0xC0 then 1 else 8 := rfl
theorem step3 (x : Nat) : step 3 x = if 0x80 ≤ x ∧ x < 0xC0 then 2 else 8 := rfl
theorem step4 (x : Nat) : step 4 x = if 0xA0 ≤ x ∧ x < 0xC0 then 1 else 8 := rfl
theorem step5 (x : Nat) : step 5 x = if 0x80 ≤ x ∧ x < 0xA0 then 1 else 8 := rfl
theorem step6 (x : Nat) : step 6 x = if 0x90 ≤ x ∧ x < 0xC0 then 2 else 8 := rfl
theorem step7 (x : Nat) : step 7 x = if 0x80 ≤ x ∧ x < 0x90 then 2 else 8 := rfl

theorem step_cont_next (s x : Nat) (h1 : 1 ≤ s) (h7 : s ≤ 7) (h : step s x ≠ 8) :
    isCont x = true ∧ ((s = 1 ∧ step s x = 0) ∨
      (1 ≤ step s x ∧ step s x ≤ 7 ∧ need (step s x) + 1 = need s)) := by
  simp only [isCont, Bool.and_eq_true, decide_eq_true_eq]
  rcases (by omega : s = 1 ∨ s = 2 ∨ s = 3 ∨ s = 4 ∨ s = 5 ∨ s = 6 ∨ s = 7) with
    rfl | rfl | rfl | rfl | rfl | rfl | rfl
  all_goals
    first
      | rw [step1] at h ⊢ | rw [step2] at h ⊢ | rw [step3] at h ⊢ | rw [step4] at h ⊢
      | rw [step5] at h ⊢ | rw [step6] at h ⊢ | rw [step7] at h ⊢
    split at h
    · rename_i hx
      refine ⟨⟨by omega, by omega⟩, ?_⟩
      simp only [hx, and_self, ↓reduceIte] <;> decide
    · exact absurd rfl h

theorem step0_cases (b : Nat) :
    (b < 0x80 ∧ step 0 b = 0) ∨ (0x80 ≤ b ∧ b < 0xC2 ∧ step 0 b = 8) ∨
    (0xC2 ≤ b ∧ b < 0xE0 ∧ step 0 b = 1) ∨ (b = 0xE0 ∧ step 0 b = 4) ∨
    (b = 0xED ∧ step 0 b = 5) ∨ (0xE1 ≤ b ∧ b < 0xF0 ∧ step 0 b = 2) ∨
    (b = 0xF0 ∧ step 0 b = 6) ∨ (0xF1 ≤ b ∧ b < 0xF4 ∧ step 0 b = 3) ∨
    (b = 0xF4 ∧ step 0 b = 7) ∨ (0xF5 ≤ b ∧ step 0 b = 8) := by
  have e : step 0 b = if b < 0x80 then 0
    else if b < 0xC2 then 8
    else if b < 0xE0 then 1
    else if b = 0xE0 then 4
    else if b = 0xED then 5
    else if b < 0xF0 then 2
    else if b = 0xF0 then 6
    else if b < 0xF4 then 3
    else if b = 0xF4 then 7
    else 8 := rfl
  rw [e]
  by_cases a1 : b < 0x80
  · exact Or.inl ⟨a1, by simp only [a1, ↓reduceIte]⟩
  simp only [a1, ↓reduceIte]
  by_cases a2 : b < 0xC2
  · exact Or.inr (Or.inl ⟨by omega, a2, by simp only [a2, ↓reduceIte]⟩)
  simp only [a2, ↓reduceIte]
  by_cases a3 : b < 0xE0
  · exact Or.inr (Or.inr (Or.inl ⟨by omega, a3, by simp only [a3, ↓reduceIte]⟩))
  simp only [a3, ↓reduceIte]
  by_cases a4 : b = 0xE0
  · subst a4; decide
  simp only [a4, ↓reduceIte]
  by_cases a5 : b = 0xED
  · subst a5; decide
  simp only [a5, ↓reduceIte]
  by_cases a6 : b < 0xF0
  · refine Or.inr (Or.inr (Or.inr (Or.inr (Or.inr (Or.inl ⟨by omega, a6, ?_⟩)))))
    simp only [a6, ↓reduceIte]
  simp only [a6, ↓reduceIte]
  by_cases a7 : b = 0xF0
  · subst a7; decide
  simp only [a7, ↓reduceIte]
  by_cases a8 : b < 0xF4
  · refine Or.inr (Or.inr (Or.inr (Or.inr (Or.inr (Or.inr (Or.inr (Or.inl ⟨by omega, a8, ?_⟩)))))))
    simp only [a8, ↓reduceIte]
  simp only [a8, ↓reduceIte]
  by_cases a9 : b = 0xF4
  · subst a9; decide
  simp only [a9, ↓reduceIte]
  exact Or.inr (Or.inr (Or.inr (Or.inr (Or.inr (Or.inr (Or.inr (Or.inr (Or.inr ⟨by omega, trivial⟩))))))))

theorem conts_of_run : ∀ k s r, need s = k → 1 ≤ s → s ≤ 7 → run s r = 0 →
    ∃ cs r', r = cs ++ r' ∧ cs.length = k ∧ (∀ x ∈ cs, isCont x = true) ∧ run 0 r' = 0 := by
  intro k
  induction k with
  | zero => intro s r hk; unfold need at hk; split at hk <;> (try split at hk) <;> omega
  | succ k ih =>
    intro s r hk h1 h7 hr
    cases r with
    | nil => simp [run] at hr; omega
    | cons x r1 =>
      rw [run_cons] at hr
      have h8 : step s x ≠ 8 := fun h8 => by rw [h8, run_8] at hr; exact absurd hr (by decide)
      obtain ⟨hcx, hn⟩ := step_cont_next s x h1 h7 h8
      rcases hn with ⟨rfl, h0⟩ | ⟨a1, a7, hneed⟩
      · rw [h0] at hr
        refine ⟨[x], r1, rfl, ?_, by simpa using hcx, hr⟩
        simp [need] at hk; simp; omega
      · obtain ⟨cs, r', rfl, hl, hc, hr'⟩ := ih (step s x) r1 (by omega) a1 a7 hr
        refine ⟨x :: cs, r', rfl, by simp [hl], ?_, hr'⟩
        intro y hy
        simp at hy
        rcases hy with rfl | hy
        · exact hcx
        · exact hc y hy

theorem step0_zero (b : Nat) (h : step 0 b = 0) : b < 0x80 := by
  rcases step0_cases b with h' | h' | h' | h' | h' | h' | h' | h' | h' | h' <;> omega

theorem step0_lead (b : Nat) (h1 : 1 ≤ step 0 b) (h7 : step 0 b ≤ 7) :
    w16 b = (if need (step 0 b) = 3 then 2 else 1) ∧ w01 b = 1 := by
  have key : (0xC2 ≤ b ∧ b < 0xF0 ∧ need (step 0 b) ≠ 3) ∨
      (0xF0 ≤ b ∧ b ≤ 0xF4 ∧ need (step 0 b) = 3) := by
    rcases step0_cases b with ⟨_, h⟩ | ⟨_, _, h⟩ | ⟨_, _, h⟩ | ⟨_, h⟩ | ⟨_, h⟩ | ⟨_, _, h⟩ |
      ⟨_, h⟩ | ⟨_, _, h⟩ | ⟨_, h⟩ | ⟨_, h⟩ <;>
    first
      | omega
      | (left; rw [h]; exact ⟨by omega, by omega, by decide⟩)
      | (right; rw [h]; exact ⟨by omega, by omega, by decide⟩)
  have hw01 : w01 b = 1 := by unfold w01 isCont; simp; omega
  refine ⟨?_, hw01⟩
  unfold w16
  rcases key with ⟨k1, k2, k3⟩ | ⟨k1, k2, k3⟩
  · simp only [k3, show ¬ (0x80 ≤ b ∧ b < 0xC0) by omega, show ¬ 0xF0 ≤ b by omega, ↓reduceIte]
  · simp only [k3, show ¬ (0x80 ≤ b ∧ b < 0xC0) by omega, k1, ↓reduceIte]

theorem map_conts (w : Nat → Nat) (hw : ∀ x, isCont x = true → w x = 0) (cs : List Nat)
    (hc : ∀ x ∈ cs, isCont x = true) : cs.map w = List.replicate cs.length 0 := by
  induction cs with
  | nil => rfl
  | cons x cs ih =>
    simp only [List.map_cons, List.length_cons, List.replicate_succ]
    rw [hw x (hc x (by simp)), ih (fun y hy => hc y (by simp [hy]))]

theorem w16_cont (x : Nat) (h : isCont x = true) : w16 x = 0 := by
  simp only [isCont, Bool.and_eq_true, decide_eq_true_eq] at h
  unfold w16; simp only [h, and_self, ↓reduceIte]

theorem w01_cont (x : Nat) (h : isCont x = true) : w01 x = 0 := by simp [w01, h]

/-- **Valid UTF-8 is the expansion of a list of complete characters**, under
both length functions. -/
theorem expand_valid : ∀ n (ys : List Nat), ys.length ≤ n → run 0 ys = 0 →
    ∃ ks, Valid ks ∧ ys.map w16 = expand16 ks ∧ ys.map w01 = expand32 ks ∧
      bytes ks = ys.length := by
  intro n
  induction n with
  | zero =>
    intro ys hl _
    have : ys = [] := List.eq_nil_of_length_eq_zero (by omega)
    subst this
    exact ⟨[], fun k hk => by simp at hk, rfl, rfl, rfl⟩
  | succ n ih =>
    intro ys hl hr
    cases ys with
    | nil => exact ⟨[], fun k hk => by simp at hk, rfl, rfl, rfl⟩
    | cons b r =>
      rw [run_cons] at hr
      by_cases h0 : step 0 b = 0
      · have hb := step0_zero b h0
        rw [h0] at hr
        obtain ⟨ks, hks, e16, e32, eb⟩ := ih r (by simp at hl; omega) hr
        refine ⟨1 :: ks, ?_, ?_, ?_, ?_⟩
        · intro k hk; simp at hk
          rcases hk with rfl | hk
          · omega
          · exact hks k hk
        · simp only [List.map_cons, expand16, chars16, e16, List.singleton_append]
          congr 1; unfold w16; split <;> (try split) <;> omega
        · simp only [List.map_cons, expand32, chars32, e32, List.singleton_append]
          congr 1; unfold w01 isCont; simp; omega
        · simp [bytes, eb]; omega
      · have h8 : step 0 b ≠ 8 := fun h8 => by rw [h8, run_8] at hr; exact absurd hr (by decide)
        have hle := step_le 0 b
        obtain ⟨cs, r', rfl, hlen, hc, hr'⟩ :=
          conts_of_run (need (step 0 b)) (step 0 b) r rfl (by omega) (by omega) hr
        obtain ⟨ks, hks, e16, e32, eb⟩ := ih r' (by simp at hl; omega) hr'
        obtain ⟨w16b, w01b⟩ := step0_lead b (by omega) (by omega)
        have hn : need (step 0 b) = 1 ∨ need (step 0 b) = 2 ∨ need (step 0 b) = 3 := by
          unfold need; split <;> (try split) <;> omega
        refine ⟨(need (step 0 b) + 1) :: ks, ?_, ?_, ?_, ?_⟩
        · intro k hk; simp at hk
          rcases hk with rfl | hk
          · omega
          · exact hks k hk
        · rw [List.map_cons, List.map_append, map_conts w16 w16_cont cs hc,
            e16, hlen, w16b, expand16, ← List.cons_append]
          congr 1
          rcases hn with e | e | e <;> rw [e] <;> rfl
        · rw [List.map_cons, List.map_append, map_conts w01 w01_cont cs hc,
            e32, hlen, w01b, expand32, ← List.cons_append]
          congr 1
          rcases hn with e | e | e <;> rw [e] <;> rfl
        · simp [bytes, eb, hlen]; omega

/-- The bytes after a boundary are complete characters. -/
theorem tail_expand {bs : List Nat} (hv : Valid8 bs) {t : Nat} (hb : Bd bs t) :
    ∃ ks, Valid ks ∧ (bs.map w16).drop t = expand16 ks ∧ (bs.map w01).drop t = expand32 ks ∧
      bytes ks = bs.length - t := by
  obtain ⟨ks, hks, e16, e32, eb⟩ := expand_valid _ (bs.drop t) (Nat.le_refl _) (valid_drop hv hb)
  exact ⟨ks, hks, by rw [← List.map_drop, e16], by rw [← List.map_drop, e32], by rw [eb]; simp⟩

/-! ## The stores

The hypotheses of `Utf8.valid_utf16_store`, `Utf8.valid_utf32_store`,
`Arm64.valid_utf16_arm_store`, `Lsx.valid_utf32_lsx_store` and
`Lasx.valid_utf32_lasx_store` (the cursor is `L` of the consumed prefix; the
unread bytes are complete characters) now follow from `vx_inv` and
`tail_expand`. What stays read from the C++ is, per path, the deficit
`s ≤ p + d` (as for the `okK` lists) and that a call ends by `pos + 63`
(`hd`; the end-of-code-point mask has bit 63 clear, and a call ends at a set
bit). -/

/-- After a call: the cursor is `L` of the consumed bytes, which end on a
character boundary. -/
theorem vx_after_call {w : Nat → Nat} (hw : WOK w) {bs : List Nat} (hv : Valid8 bs)
    {pos t o c p : Nat} (h : VX w bs true pos t o) (ht : t < pos + 52)
    (hx : XStep (bs.map w) t c p) :
    o + p = L ((bs.map w).take (t + c)) ∧ Bd bs (t + c) :=
  have hi := vx_inv hw hv _ _ _ _ (VX.call pos t o c p h ht hx)
  ⟨hi.1, hi.2.1⟩

/-- **`valid_utf8_to_utf32.h`, x86, ppc64 and arm64 (margin 12).** A store of
`s ≤ p + 4` units at a call that produces `p` fits. -/
theorem valid_utf32_vx {bs : List Nat} (hv : Valid8 bs) {m pos t o c p s : Nat}
    (h : VX w01 bs true pos t o) (ht : t < pos + 52) (hx : XStep (bs.map w01) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hloop : pos + 64 + m ≤ bs.length) (hm : 12 ≤ m)
    (hs : s ≤ p + 4) : o + s ≤ L (bs.map w01) := by
  obtain ⟨hL, hb⟩ := vx_after_call wok_w01 hv h ht hx
  obtain ⟨ks, hks, -, e32, eb⟩ := tail_expand hv hb
  have := valid_utf32_store ((bs.map w01).take (t + c)) ks (o + s) m hks hm (by omega) (by omega)
  rwa [← e32, List.take_append_drop] at this

/-- **`valid_utf8_to_utf32.h`, lsx (margin 4)**, deficit 2. -/
theorem valid_utf32_lsx_vx {bs : List Nat} (hv : Valid8 bs) {m pos t o c p s : Nat}
    (h : VX w01 bs true pos t o) (ht : t < pos + 52) (hx : XStep (bs.map w01) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hloop : pos + 64 + m ≤ bs.length) (hm : 4 ≤ m)
    (hs : s ≤ p + 2) : o + s ≤ L (bs.map w01) := by
  obtain ⟨hL, hb⟩ := vx_after_call wok_w01 hv h ht hx
  obtain ⟨ks, hks, -, e32, eb⟩ := tail_expand hv hb
  have := Arch.Lsx.valid_utf32_lsx_store ((bs.map w01).take (t + c)) ks (o + s) m hks hm
    (by omega) (by omega)
  rwa [← e32, List.take_append_drop] at this

/-- **`valid_utf8_to_utf32.h`, lasx (margin 4)**, deficit 2. -/
theorem valid_utf32_lasx_vx {bs : List Nat} (hv : Valid8 bs) {m pos t o c p s : Nat}
    (h : VX w01 bs true pos t o) (ht : t < pos + 52) (hx : XStep (bs.map w01) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hloop : pos + 64 + m ≤ bs.length) (hm : 4 ≤ m)
    (hs : s ≤ p + 2) : o + s ≤ L (bs.map w01) := by
  obtain ⟨hL, hb⟩ := vx_after_call wok_w01 hv h ht hx
  obtain ⟨ks, hks, -, e32, eb⟩ := tail_expand hv hb
  have := Arch.Lasx.valid_utf32_lasx_store ((bs.map w01).take (t + c)) ks (o + s) m hks hm
    (by omega) (by omega)
  rwa [← e32, List.take_append_drop] at this

/-- **`valid_utf8_to_utf16.h`, x86, ppc64, lsx and lasx (margin 9).** -/
theorem valid_utf16_vx {bs : List Nat} (hv : Valid8 bs) {m pos t o c p s : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52) (hx : XStep (bs.map w16) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hloop : pos + 64 + m ≤ bs.length) (hm : 9 ≤ m)
    (hs : s ≤ p + 4) : o + s ≤ L (bs.map w16) := by
  obtain ⟨hL, hb⟩ := vx_after_call wok_w16 hv h ht hx
  obtain ⟨ks, hks, e16, -, eb⟩ := tail_expand hv hb
  have := valid_utf16_store ((bs.map w16).take (t + c)) ks (o + s) m hks hm (by omega) (by omega)
  rwa [← e16, List.take_append_drop] at this

/-- **`valid_utf8_to_utf16.h`, arm64 (margin 3)**, deficit 2. The `0x888`
path stores before its check, but on valid input the check passes and the
store is the call's output (`vx_888`: deficit 0). -/
theorem valid_utf16_arm_vx {bs : List Nat} (hv : Valid8 bs) {m pos t o c p s : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52) (hx : XStep (bs.map w16) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hloop : pos + 64 + m ≤ bs.length) (hm : 3 ≤ m)
    (hs : s ≤ p + 2) : o + s ≤ L (bs.map w16) := by
  obtain ⟨hL, hb⟩ := vx_after_call wok_w16 hv h ht hx
  obtain ⟨ks, hks, e16, -, eb⟩ := tail_expand hv hb
  have := Arch.Arm64.valid_utf16_arm_store ((bs.map w16).take (t + c)) ks (o + s) m hks hm
    (by omega) (by omega)
  rwa [← e16, List.take_append_drop] at this

/-- The ASCII block: its 64 units end at `L` of the consumed bytes. -/
theorem vx_ascii_store {w : Nat → Nat} (hw : WOK w) {bs : List Nat} (hv : Valid8 bs)
    {t o : Nat} (h : VX w bs false t t o) (hlen : t + 64 ≤ bs.length)
    (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) : o + 64 ≤ L (bs.map w) := by
  have := (vx_inv hw hv _ _ _ _ (VX.ascii t o h hlen hascii)).1
  rw [this]
  exact L_take_le _ _

/-- The scalar tail: at the end of the loop the cursor is `L` of the
consumed bytes, and the rest is complete characters; one that writes `L` of
the rest ends at `L` of the input. -/
theorem vx_tail {w : Nat → Nat} (hw : WOK w) {bs : List Nat} (hv : Valid8 bs) {t o : Nat}
    (h : VX w bs false t t o) :
    o + L ((bs.map w).drop t) = L (bs.map w) ∧
    ∃ ks, Valid ks ∧ (bs.map w16).drop t = expand16 ks ∧ (bs.map w01).drop t = expand32 ks ∧
      bytes ks = bs.length - t := by
  have hi := vx_inv hw hv _ _ _ _ h
  refine ⟨?_, tail_expand hv hi.2.1⟩
  rw [hi.1, ← L_append, List.take_append_drop]

/-- A table row ends by `pos + 63`: `hd` holds for every table call. -/
theorem row_hd {xs : List Nat} {pos t : Nat} (ht : t < pos + 52) :
    t + bigConsumed[(row xs t).m]! + 1 ≤ pos + 64 := by
  have := (List.all_eq_true.mp bigconsumed_le) (row xs t).m (row xs t).hmem
  simp only [decide_eq_true_eq] at this
  omega

/-! ### Table calls, end to end

A table call needs no hypothesis but its deficit: on valid input it is exact
(`vx_w01`, `vx_utf16_145`, `vx_utf16_3`) and ends by `pos + 63` (`row_hd`). -/

/-- UTF-32, margin 12: a table call storing `s ≤ units + 4`. -/
theorem valid_utf32_row {bs : List Nat} (hv : Valid8 bs) {m pos t o s : Nat}
    (h : VX w01 bs true pos t o) (ht : t < pos + 52) (hloop : pos + 64 + m ≤ bs.length)
    (hm : 12 ≤ m) (hs : s ≤ units bigIdx[(row (bs.map w01) t).m]! + 4) :
    o + s ≤ L (bs.map w01) :=
  valid_utf32_vx hv h ht (vx_w01 hv h ht).2 (row_hd ht) hloop hm hs

/-- UTF-32, lsx and lasx, margin 4: a table call storing `s ≤ units + 2`. -/
theorem valid_utf32_la_row {bs : List Nat} (hv : Valid8 bs) {m pos t o s : Nat}
    (h : VX w01 bs true pos t o) (ht : t < pos + 52) (hloop : pos + 64 + m ≤ bs.length)
    (hm : 4 ≤ m) (hs : s ≤ units bigIdx[(row (bs.map w01) t).m]! + 2) :
    o + s ≤ L (bs.map w01) :=
  valid_utf32_lsx_vx hv h ht (vx_w01 hv h ht).2 (row_hd ht) hloop hm hs

/-- UTF-16, margin 9: a row below 145 storing `s ≤ units + 4`. -/
theorem valid_utf16_row145 {bs : List Nat} (hv : Valid8 bs) {m pos t o s : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52) (hloop : pos + 64 + m ≤ bs.length)
    (hm : 9 ≤ m) (hi : bigIdx[(row (bs.map w16) t).m]! < 145)
    (hs : s ≤ units bigIdx[(row (bs.map w16) t).m]! + 4) :
    o + s ≤ L (bs.map w16) :=
  valid_utf16_vx hv h ht (vx_utf16_145 hv h ht hi) (row_hd ht) hloop hm hs

/-- UTF-16, arm64, margin 3: a row below 145 storing `s ≤ units + 2`. -/
theorem valid_utf16_arm_row145 {bs : List Nat} (hv : Valid8 bs) {m pos t o s : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52) (hloop : pos + 64 + m ≤ bs.length)
    (hm : 3 ≤ m) (hi : bigIdx[(row (bs.map w16) t).m]! < 145)
    (hs : s ≤ units bigIdx[(row (bs.map w16) t).m]! + 2) :
    o + s ≤ L (bs.map w16) :=
  valid_utf16_arm_vx hv h ht (vx_utf16_145 hv h ht hi) (row_hd ht) hloop hm hs

/-- UTF-16, margin 9: a three-code-point row, whose output (one or two units
per lane, by the pair test) is exact; its store is at most the output plus 4. -/
theorem valid_utf16_row3 (arm : Bool) {bs : List Nat} (hv : Valid8 bs) {m pos t o s : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52) (hloop : pos + 64 + m ≤ bs.length)
    (hm : 9 ≤ m) (hi1 : 145 ≤ bigIdx[(row (bs.map w16) t).m]!)
    (hi2 : bigIdx[(row (bs.map w16) t).m]! < 209)
    (hs : s ≤ (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 0) then 2 else 1) +
       (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 1) then 2 else 1) +
       (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 2) then 2 else 1) + 4) :
    o + s ≤ L (bs.map w16) :=
  valid_utf16_vx hv h ht (vx_utf16_3 arm hv h ht hi1 hi2) (row_hd ht) hloop hm hs

/-- arm64 UTF-16, the `0x888` path: on valid input its 6 units fit with margin 3. -/
theorem valid_utf16_arm_888 {bs : List Nat} (hv : Valid8 bs) {m pos t o : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52) (hloop : pos + 64 + m ≤ bs.length)
    (hm : 3 ≤ m) (hmask : emask (bs.map w16) t 12 = 0x888) :
    o + 6 ≤ L (bs.map w16) :=
  valid_utf16_arm_vx hv h ht (vx_888 hv h ht hmask).2 (by omega) hloop hm (by omega)

set_option maxRecDepth 4000 in
/-- The model is not vacuous: 64 ASCII bytes, then `é` and 5 ASCII bytes, is
valid; the loop takes the ASCII block and then enters the next block only if
64 more bytes remain. -/
example : let bs := List.replicate 64 0x41 ++ [0xC3, 0xA9] ++ List.replicate 5 0x41
    Valid8 bs ∧ VX w01 bs false 64 64 64 :=
  ⟨show run 0 _ = 0 by decide, VX.ascii 0 0 VX.start (by decide) (by decide)⟩

end SafetyMargins.ValidExact
