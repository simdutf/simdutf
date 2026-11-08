import SafetyMargins.Scalar
/-
Ice Lake UTF-16 → UTF-32, the whole function.

`IcelakeReal.run32` models the AVX-512 loop of
src/icelake/icelake_convert_utf16_to_utf32.inl.cpp:144-258 and charges what
follows the loop as exact (`IcelakeReal.safe_utf16to32_kernel` holds under
that charge). This file transcribes the whole of `convert_utf16_to_utf32`
(:131-279): the loop again, with its returned input and output cursors and
its error return; the code after it (:259-278); the scalar tail for fewer
than 8 units (`Scalar.u16to32`, reused); and the masked tail
`convert_utf16_to_utf32_masked` (:3-125) with its masks: `todo`
(`_bzhi_u32`, :16), the masked load (:17), the surrogate masks and the check
`V` (:23-36, the same check `IcelakeReal.chk` as the loop), the two stores
masked to `howmany1`/`howmany2` units (:90-108), the two stores masked by
`todo` (:116-122), the returned count (:124) and the error return (:111).
Note that the masked tail is called with `buf`, not `rest` (:273-274): a
carried low surrogate is lane 0 of its load and is checked against the carry.

Results, for every input of 16-bit units (values after the byte swap of the
big-endian variant, :18-20 and :147-149, so both endiannesses), with no
hypothesis:

* `masked32_spec`: the masked tail's stores end within `L` of its units
  (weights `IcelakeReal.w32v`: 0 for a low surrogate, else 1), and its
  returned count is at most that; `masked32_loads`: its load reads only lanes
  below `end - buf`.
* `conv32_spec`: the whole function's stores end within `L` of the input;
  the returned input cursor is in the input, and the returned output cursor
  plus `L` of the units after the input cursor is at most `L` of the input;
  on success the whole input is consumed and the output cursor is `L` of it.
  This holds from every carry state the loop can reach (`Inv`: a carry only
  with a low surrogate at the cursor, which `V == 0` guarantees).
  `conv32_loads`: every vector load is inside the input.
* `safe_utf16to32_full`: the three entry points of
  src/icelake/implementation.cpp (plain :1532-1541 and :1543-1551,
  `_with_errors` :1553-1567 and :1569-1582, `convert_valid` :1584-1593 and
  :1595-1603) store within `utf32_length_from_utf16` of the input; for
  `_with_errors`, after a failure, the scalar `convert_with_errors` restarts
  at the returned cursors (:1559-1563).

Read from the C++: the transcription (cited lines). Stores and loads of
masked intrinsics touch only the lanes their mask selects (the AVX-512 fault
suppression), so a masked store ends at its offset plus the last selected
lane. A store whose mask is a prefix of `k` lanes ends `k` lanes past its
offset, or touches nothing when `k = 0` (`mEnd`).
-/
namespace SafetyMargins.IcelakeUtf32Tail
open SafetyMargins SafetyMargins.IcelakeReal
set_option autoImplicit false
set_option linter.unusedSimpArgs false

/-! ## Masks -/

/-- Bit `i` of `_bzhi_u32(0xFFFFFFFF, n)` (src/icelake/icelake_convert_utf16_to_utf32.inl.cpp:16): the bits from `n` up are
cleared. -/
def bzhiBit (n i : Nat) : Bool := (0xFFFFFFFF % 2 ^ n).testBit i

/-- `todo` selects exactly the lanes below `n` (all `n < 32`; `decide +kernel`,
a finite exhaustive check). -/
theorem bzhi_spec : ∀ n, n < 32 → ∀ i, i < 32 → bzhiBit n i = decide (i < n) := by
  decide +kernel

/-- Bit `i` of `__mmask16((1U << k) - 1)` (:97, :106). -/
def pmaskBit (k i : Nat) : Bool := (((1 <<< k) - 1) % 2 ^ 16).testBit i

/-- `(1U << k) - 1` for `k ≤ 16` selects exactly the first `k` lanes, so a
store under it is a prefix store of `k` lanes (`decide +kernel`). -/
theorem pmask_spec : ∀ k, k ≤ 16 → ∀ i, i < 16 → pmaskBit k i = decide (i < k) := by
  decide +kernel

/-- Bit `i` of `__mmask16(todo)` and of `__mmask16(todo >> 16)` (:119-121):
lanes `i < 16` and `16 + i < n`. -/
theorem todo_halves : ∀ n, n < 32 → ∀ i, i < 16 →
    bzhiBit n i = decide (i < min n 16) ∧ bzhiBit n (16 + i) = decide (i < n - 16) := by
  decide +kernel

/-- End of a store masked to its first `k` lanes at offset `off`; nothing is
touched when `k = 0`. -/
def mEnd (off k : Nat) : Nat := if k = 0 then 0 else off + k

theorem mEnd_le (off k : Nat) : mEnd off k ≤ off + k := by
  unfold mEnd; split <;> omega

/-! ## The masked tail `convert_utf16_to_utf32_masked` (:3-125) -/

/-- The 32 lanes of `_mm512_maskz_loadu_epi16(todo, buf)` (:17), `n = end - buf`:
lane `i` is unit `i` if bit `i` of `todo` is set, else 0. -/
def maskLanes (us : List Nat) : List Nat :=
  (List.range 32).map (fun i => if bzhiBit us.length i then us.getD i 0 else 0)

/-- What the masked tail does: the furthest end of its stores (from
`utf32_output`) and the count it returns. -/
structure Masked where
  reach : Nat
  written : Nat

/-- `convert_utf16_to_utf32_masked(buf, end, utf32_output, carry)` on the units
`us = buf[0 .. end - buf]` and the carry `c`.

* `if (H | L)` (:23-29) on the loaded lanes;
* `V == 0` (:31-36) is `chk c` of the lanes;
* valid case: `valid = ~L & todo` (:90), the lanes below `n` that are not low
  surrogates; `howmany1 = count_ones((uint16_t)valid)` (:95), the non-low
  units among the first 16; `howmany2` the non-low units from 16 on (:102).
  Store masked to `howmany1` lanes at the cursor (:96-98), cursor
  `+= howmany1` (:99), store masked to `howmany2` lanes (:105-107), cursor
  `+= howmany2` (:108), return the advance (:124);
* invalid case: return 0 (:111), nothing stored;
* no surrogate: stores masked by `todo` (`min n 16` lanes) at the cursor and
  by `todo >> 16` (`n - 16` lanes) at the cursor + 16 (:119-121), advance
  `end - buf` (:122), return it (:124). -/
def masked32 (c : Bool) (us : List Nat) : Masked :=
  let n := us.length
  let v := maskLanes us
  if v.any (fun x => hiS x || loS x) then
    if chk c v then
      let howmany1 := nonLo (us.take 16)
      let howmany2 := nonLo (us.drop 16)
      ⟨max (mEnd 0 howmany1) (mEnd howmany1 howmany2), howmany1 + howmany2⟩
    else ⟨0, 0⟩
  else ⟨max (mEnd 0 (min n 16)) (mEnd 16 (n - 16)), n⟩

theorem maskLanes_eq (us : List Nat) (hn : us.length < 32) :
    maskLanes us = us ++ List.replicate (32 - us.length) 0 := by
  apply List.ext_getElem
  · simp [maskLanes]; omega
  · intro i h1 h2
    simp only [maskLanes, List.length_map, List.length_range] at h1
    simp only [maskLanes, List.getElem_map, List.getElem_range, bzhi_spec _ hn i h1]
    rw [List.getElem_append]
    split
    · rename_i h
      simp only [decide_eq_true_eq] at h
      simp [h, List.getD_eq_getElem?_getD]
    · rename_i h
      simp only [decide_eq_true_eq] at h
      simp [h]

theorem nonLo_append (a b : List Nat) : nonLo (a ++ b) = nonLo a + nonLo b := by
  rw [nonLo_eq, nonLo_eq, nonLo_eq, List.map_append, L_append]

/-- **The masked tail's stores and count.** Every store ends within `L` of
the units `buf .. end`, and the returned count is at most that, for every
carry. -/
theorem masked32_spec (c : Bool) (us : List Nat) (hn : us.length < 32) :
    (masked32 c us).reach ≤ L (us.map w32v) ∧ (masked32 c us).written ≤ L (us.map w32v) := by
  have hsplit : nonLo (us.take 16) + nonLo (us.drop 16) = L (us.map w32v) := by
    rw [← nonLo_append, List.take_append_drop, nonLo_eq]
  unfold masked32
  simp only
  split
  · split
    · have a := mEnd_le 0 (nonLo (us.take 16))
      have b := mEnd_le (nonLo (us.take 16)) (nonLo (us.drop 16))
      simp only
      omega
    · simp
  · rename_i hs
    rw [maskLanes_eq us hn] at hs
    have hs' : us.any (fun v => hiS v || loS v) = false := by
      simp only [List.any_append, Bool.or_eq_true, not_or, Bool.not_eq_true] at hs
      exact hs.1
    have hl := nosur_L us hs'
    have a := mEnd_le 0 (min us.length 16)
    have b : mEnd 16 (us.length - 16) ≤ us.length := by
      unfold mEnd; split <;> omega
    simp only
    omega

/-- **The masked tail's load** reads only the lanes of `todo`, which are below
`end - buf`: the units `buf .. end - 1`. -/
theorem masked32_loads (us : List Nat) (hn : us.length < 32) :
    ∀ i, i < 32 → bzhiBit us.length i = true → i < us.length := by
  intro i hi h
  rw [bzhi_spec _ hn i hi] at h
  simpa using h

/-! ## The scalar tail's return value -/

/-- Whether a scalar loop (`Scalar.run`) runs to the end of its input without
returning an error. With fuel the input length (each step consumes at least
one unit, `Scalar.run_steps`) this is the C++ loop; running out of fuel
counts as a failure, which only makes the model fail more often. -/
def runOk (step : List Nat → Option (Nat × Nat)) : Nat → List Nat → Bool
  | _, [] => true
  | 0, _ :: _ => false
  | f + 1, x :: r =>
    match step (x :: r) with
    | none => false
    | some (c, _) => runOk step f ((x :: r).drop c)

/-- Each step of `scalar::utf16_to_utf32::convert` writes exactly `L` of what
it consumes. -/
theorem u16to32_eq (x : Nat) (r : List Nat) (c w : Nat) (hu : Scalar.U16 (x :: r))
    (h : Scalar.u16to32 (x :: r) = some (c, w)) : w = L (((x :: r).map w32v).take c) := by
  have hx : x < 65536 := hu x (by simp)
  unfold Scalar.u16to32 at h
  simp only [List.getD_cons_zero, Scalar.m800 x hx] at h
  repeat' (first | (split at h) | (simp at h; done))
  all_goals (simp at h; obtain ⟨rfl, rfl⟩ := h)
  all_goals rcases r with _ | ⟨y, r⟩
  all_goals (try have hy : y < 65536 := hu y (by simp))
  all_goals simp only [List.map_cons, List.map_nil, List.take_succ_cons, List.take_zero, List.take_nil,
    L_cons, L_nil, List.getD_cons_succ, List.getD_cons_zero, List.length_cons, List.length_nil] at *
  all_goals (unfold w32v loS; simp only [decide_eq_true_eq]; repeat' split)
  all_goals omega

/-- When the scalar loop succeeds it has written `L` of its input. -/
theorem run_ok_eq : ∀ f xs, Scalar.U16 xs → runOk Scalar.u16to32 f xs = true →
    Scalar.run Scalar.u16to32 f xs = L (xs.map w32v) := by
  intro f
  induction f with
  | zero =>
    intro xs _ h
    rcases xs with _ | ⟨x, r⟩
    · rfl
    · simp [runOk] at h
  | succ f ih =>
    intro xs hu h
    rcases xs with _ | ⟨x, r⟩
    · rfl
    · simp only [runOk] at h
      simp only [Scalar.run]
      split at h
      · simp at h
      · rename_i c w hs
        rw [hs]
        simp only
        rw [ih _ (hu.drop c) h, u16to32_eq x r c w hu hs]
        exact (Scalar.L_map_take_add_drop w32v c (x :: r)).symm

/-! ## The whole function `convert_utf16_to_utf32` (:131-279) -/

/-- The returned tuple and the stores: `reach` is the furthest end of a store
from `utf32_output`, `inPos` is `std::get<0> - buf`, `outPos` is
`std::get<1> - utf32_output`, `ok` is `std::get<2>`. -/
structure Ret where
  reach : Nat
  inPos : Nat
  outPos : Nat
  ok : Bool

/-- After the loop (:259-278), on the units `us` from `buf` (fewer than 32)
and the carry `c`. `rest = buf + carry` (:259), `remaining = end - rest`
(:260). Nothing left: success (:261-263). Fewer than 8: the scalar
`convert<big_endian>(rest, remaining, utf32_output)` (:264-267), whose result
is 0 on error and the count written otherwise; on 0, `(rest, utf32_output,
false)` (:268-270), else `(end, utf32_output + written, true)` (:271). Else
the masked tail on `buf` (:273-274), with the same two returns (:275-278). -/
def tail32 (c : Bool) (us : List Nat) : Ret :=
  let rest := us.drop (cb c)
  if rest.length = 0 then ⟨0, cb c, 0, true⟩
  else if rest.length < 8 then
    let stores := Scalar.run Scalar.u16to32 rest.length rest
    let written := if runOk Scalar.u16to32 rest.length rest then stores else 0
    if written = 0 then ⟨stores, cb c, 0, false⟩ else ⟨stores, us.length, written, true⟩
  else
    let m := masked32 c us
    if m.written = 0 then ⟨m.reach, cb c, 0, false⟩ else ⟨m.reach, us.length, m.written, true⟩

/-- `convert_utf16_to_utf32<big_endian>(buf, len, utf32_output)` from the loop
on, on the units `us` from `buf` and the carry `c`. `while (end - buf >= 32)`
(:144): load 32 lanes (:146).

* `H | L` (:158): if `V == 0` (:166), the unmasked store of
  `compressed_first` (16 units, :227), cursor `+= howmany1` (:228, the
  non-low lanes among 0-15, `valid = ~L & 0x7fffffff`, :221), the store
  masked to `howmany2` lanes (:234-236, the non-low lanes among 16-30),
  cursor `+= howmany2` (:237), `buf += 31` (:240), carry = lane 30 is high
  (:241). Else return `(buf + carry, utf32_output, false)` (:244).
* no surrogate: two unmasked stores of 16 units at the cursor and the cursor
  + 16 (:249-253), `+= 32` (:254-255), carry 0 (:256). -/
def conv32 (c : Bool) (us : List Nat) : Ret :=
  if h : 32 ≤ us.length then
    let l := us.take 32
    if l.any (fun v => hiS v || loS v) then
      if chk c l then
        let howmany1 := nonLo (l.take 16)
        let howmany2 := nonLo ((l.take 31).drop 16)
        let r := conv32 (hiS (l.getD 30 0)) (us.drop 31)
        ⟨max (max 16 (mEnd howmany1 howmany2)) (howmany1 + howmany2 + r.reach),
          r.inPos + 31, howmany1 + howmany2 + r.outPos, r.ok⟩
      else ⟨0, cb c, 0, false⟩
    else
      let r := conv32 false (us.drop 32)
      ⟨max 32 (32 + r.reach), r.inPos + 32, 32 + r.outPos, r.ok⟩
  else tail32 c us
termination_by us.length
decreasing_by all_goals (simp only [List.length_drop]; omega)

theorem L_drop_le (us : List Nat) (k : Nat) : L ((us.drop k).map w32v) ≤ L (us.map w32v) := by
  have := L_map_split w32v us k; omega

theorem cb_le (c : Bool) : cb c ≤ 1 := by cases c <;> simp [cb]

/-- The carry invariant: a carry is set only when unit 0 is a low surrogate
(lane 31 of the previous load, checked by `V == 0`). At entry the carry is 0
(:139). -/
def Inv (c : Bool) (us : List Nat) : Prop := c = true → loS (us.getD 0 0) = true

theorem L_drop_cb (c : Bool) (us : List Nat) (hi : Inv c us) :
    L ((us.drop (cb c)).map w32v) = L (us.map w32v) := by
  cases c with
  | false => rfl
  | true =>
    have h := hi rfl
    rcases us with _ | ⟨x, r⟩
    · rfl
    · simp only [List.getD_cons_zero] at h
      simp [cb, w32v, h]

theorem len_cb (c : Bool) (us : List Nat) (hi : Inv c us) : cb c ≤ us.length := by
  cases c with
  | false => simp [cb]
  | true =>
    have h := hi rfl
    rcases us with _ | ⟨x, r⟩
    · simp [loS] at h
    · simp [cb]

/-- The code after the loop: stores within `L`, and the returned cursors. -/
theorem tail32_spec (c : Bool) (us : List Nat) (hu : Scalar.U16 us) (hn : us.length < 32)
    (hi : Inv c us) :
    (tail32 c us).reach ≤ L (us.map w32v) ∧ (tail32 c us).inPos ≤ us.length ∧
    (tail32 c us).outPos + L ((us.drop (tail32 c us).inPos).map w32v) ≤ L (us.map w32v) ∧
    ((tail32 c us).ok = true → (tail32 c us).inPos = us.length ∧
      (tail32 c us).outPos = L (us.map w32v)) := by
  have hrest := L_drop_cb c us hi
  have hcb := len_cb c us hi
  have hsc := Scalar.utf16to32_le (us.drop (cb c)).length (us.drop (cb c)) (hu.drop _)
  have hrest' := hrest
  rw [List.map_drop] at hrest'
  unfold tail32
  simp only
  split
  · rename_i h0
    have hl : us.drop (cb c) = [] := List.eq_nil_of_length_eq_zero h0
    have hd : us.length = cb c := by
      have := congrArg List.length hl; simp only [List.length_drop, List.length_nil] at this; omega
    rw [hl] at hrest
    simp only [List.map_nil, L_nil] at hrest
    refine ⟨Nat.zero_le _, hcb, ?_, fun _ => ⟨hd.symm, hrest⟩⟩
    rw [hl]; simp
  · split
    · by_cases hok : runOk Scalar.u16to32 (us.drop (cb c)).length (us.drop (cb c)) = true
      · have he := run_ok_eq _ _ (hu.drop _) hok
        simp only [hok, ite_true]
        split
        · refine ⟨by dsimp only; omega, hcb, ?_, by simp⟩
          simp; omega
        · refine ⟨by dsimp only; omega, Nat.le_refl _, ?_, fun _ => ⟨rfl, by dsimp only; omega⟩⟩
          simp only [List.drop_length, List.map_nil, L_nil, Nat.add_zero]; omega
      · simp only [hok, Bool.false_eq_true, ite_false, ite_true]
        refine ⟨by omega, hcb, ?_, by simp⟩
        simp; omega
    · have hm := masked32_spec c us hn
      split
      · refine ⟨hm.1, hcb, ?_, by simp⟩
        simp; omega
      · rename_i hw0
        refine ⟨hm.1, Nat.le_refl _, ?_, fun _ => ⟨rfl, ?_⟩⟩
        · simp; exact hm.2
        · -- a nonzero count is `nonLo us` or `us.length`, both `L`
          unfold masked32 at hw0 ⊢
          simp only at hw0 ⊢
          split
          · split
            · simp only
              rw [← nonLo_append, List.take_append_drop, nonLo_eq]
            · rename_i h1 h2; simp [h1, h2] at hw0
          · rename_i hs
            rw [maskLanes_eq us hn] at hs
            simp only [List.any_append, Bool.or_eq_true, not_or, Bool.not_eq_true] at hs
            exact (nosur_L us hs.1).symm

/-- `V == 0` ties lane `i + 1` (low) to lane `i` (high). -/
theorem chk_get : ∀ (l : List Nat) (p : Bool) (i : Nat), chk p l = true → i + 1 < l.length →
    loS (l.getD (i + 1) 0) = hiS (l.getD i 0) := by
  intro l
  induction l with
  | nil => intro p i _ h; simp at h
  | cons x xs ih =>
    intro p i hc hi
    simp only [chk, Bool.and_eq_true, beq_iff_eq] at hc
    cases i with
    | zero =>
      rcases xs with _ | ⟨y, ys⟩
      · simp at hi
      · simp only [chk, Bool.and_eq_true, beq_iff_eq] at hc
        simpa using hc.2.1
    | succ k =>
      simp only [List.getD_cons_succ]
      exact ih _ k hc.2 (by simp at hi; omega)

theorem nonLo_take31 (us : List Nat) :
    nonLo ((us.take 32).take 16) + nonLo (((us.take 32).take 31).drop 16) =
      L ((us.take 31).map w32v) := by
  rw [List.take_take, List.take_take, show min 16 32 = 16 from rfl, show min 31 32 = 31 from rfl,
    show us.take 16 = (us.take 31).take 16 by rw [List.take_take]; rfl, ← nonLo_append,
    List.take_append_drop, nonLo_eq]

/-- **The whole function, from the loop on.** For every carry state the C++
can reach (`Inv`) and every input of 16-bit units: every store ends within
`L` of the units; the returned input cursor is in the input; the returned
output cursor plus `L` of the units after the input cursor is at most `L` of
the units; on success the whole input is consumed and the output cursor is
`L` of it. -/
theorem conv32_spec : ∀ n c us, us.length = n → Scalar.U16 us → Inv c us →
    (conv32 c us).reach ≤ L (us.map w32v) ∧ (conv32 c us).inPos ≤ us.length ∧
    (conv32 c us).outPos + L ((us.drop (conv32 c us).inPos).map w32v) ≤ L (us.map w32v) ∧
    ((conv32 c us).ok = true → (conv32 c us).inPos = us.length ∧
      (conv32 c us).outPos = L (us.map w32v)) := by
  intro n
  induction n using Nat.strongRecOn with
  | _ n ih =>
  intro c us hn hu hi
  rw [conv32]
  by_cases h32 : 32 ≤ us.length
  · rw [dite_eq_left h32]
    simp only
    by_cases ha : (us.take 32).any (fun v => hiS v || loS v) = true
    · rw [ite_eq_left ha]
      by_cases hc : chk c (us.take 32) = true
      · rw [ite_eq_left hc]
        -- the next carry state is reachable
        have hi' : Inv (hiS ((us.take 32).getD 30 0)) (us.drop 31) := by
          intro h
          have := chk_get (us.take 32) c 30 hc (by simp; omega)
          rw [h] at this
          rw [← this]
          simp [List.getD_eq_getElem?_getD]
        have hr := ih (us.length - 31) (by omega) _ (us.drop 31) (by simp) (hu.drop 31) hi'
        generalize conv32 (hiS ((us.take 32).getD 30 0)) (us.drop 31) = r at hr
        obtain ⟨r1, r2, r3, r4⟩ := hr
        have h16 := utf16to32_hsur c us h32 hc
        have h32le := L_take_le (us.map w32v) 32
        have hsplit := L_map_split w32v us 31
        have hadv := nonLo_take31 us
        have hme := mEnd_le (nonLo ((us.take 32).take 16)) (nonLo (((us.take 32).take 31).drop 16))
        have hdd : (us.drop 31).drop r.inPos = us.drop (r.inPos + 31) := by
          rw [List.drop_drop, Nat.add_comm]
        refine ⟨?_, ?_, ?_, ?_⟩
        · simp only; omega
        · simp only [List.length_drop] at r2; simp only; omega
        · simp only; rw [← hdd]; omega
        · intro hok
          simp only at hok ⊢
          obtain ⟨o1, o2⟩ := r4 hok
          simp only [List.length_drop] at o1
          constructor <;> omega
      · rw [ite_eq_right hc]
        have hl := L_drop_le us (cb c)
        exact ⟨Nat.zero_le _, len_cb c us hi, by simp only; omega, by simp⟩
    · rw [ite_eq_right ha]
      have hr := ih (us.length - 32) (by omega) false (us.drop 32) (by simp) (hu.drop 32)
        (fun h => by cases h)
      generalize conv32 false (us.drop 32) = r at hr
      obtain ⟨r1, r2, r3, r4⟩ := hr
      have hl := nosur_L (us.take 32) (by simpa using ha)
      have hlen : (us.take 32).length = 32 := by simp; omega
      have hsplit := L_map_split w32v us 32
      have hdd : (us.drop 32).drop r.inPos = us.drop (r.inPos + 32) := by
        rw [List.drop_drop, Nat.add_comm]
      refine ⟨?_, ?_, ?_, ?_⟩
      · simp only; omega
      · simp only [List.length_drop] at r2; simp only; omega
      · simp only; rw [← hdd]; omega
      · intro hok
        simp only at hok ⊢
        obtain ⟨o1, o2⟩ := r4 hok
        simp only [List.length_drop] at o1
        constructor <;> omega
  · rw [dite_eq_right h32]
    exact tail32_spec c us hu (by omega) hi

/-! ## Loads -/

/-- End of the masked load (:17): one past the last lane of `todo`. -/
def maskedLoadEnd (us : List Nat) : Nat :=
  ((List.range 32).filter (bzhiBit us.length)).foldr (fun i m => max (i + 1) m) 0

theorem foldr_max_le (l : List Nat) (b : Nat) (h : ∀ i ∈ l, i + 1 ≤ b) :
    l.foldr (fun i m => max (i + 1) m) 0 ≤ b := by
  induction l with
  | nil => simp
  | cons x xs ih =>
    simp only [List.foldr_cons]
    have := h x (by simp)
    have := ih (fun i hi => h i (by simp [hi]))
    omega

theorem maskedLoadEnd_le (us : List Nat) (hn : us.length < 32) : maskedLoadEnd us ≤ us.length := by
  apply foldr_max_le
  intro i hi
  simp only [List.mem_filter, List.mem_range] at hi
  have := masked32_loads us hn i hi.1 hi.2
  omega

/-- The furthest unit read by a vector load of `conv32`, from `buf`: the
32-lane load at the cursor, `_mm512_loadu_si512` (:146), after which the
cursor moves by 31 or 32; the masked load of the tail (:17). The scalar tail
reads unit `pos + 1` only after returning when `pos + 1 >= len` (include/simdutf/scalar/utf16_to_utf32/utf16_to_utf32.h:27). -/
def loadEnd32 (c : Bool) (us : List Nat) : Nat :=
  if h : 32 ≤ us.length then
    let l := us.take 32
    if l.any (fun v => hiS v || loS v) then
      if chk c l then max 32 (31 + loadEnd32 (hiS (l.getD 30 0)) (us.drop 31)) else 32
    else max 32 (32 + loadEnd32 false (us.drop 32))
  else if (us.drop (cb c)).length = 0 ∨ (us.drop (cb c)).length < 8 then 0
  else maskedLoadEnd us
termination_by us.length
decreasing_by all_goals (simp only [List.length_drop]; omega)

/-- **Loads.** Every vector load of the function reads inside the input. -/
theorem conv32_loads : ∀ n c us, us.length = n → loadEnd32 c us ≤ us.length := by
  intro n
  induction n using Nat.strongRecOn with
  | _ n ih =>
  intro c us hn
  rw [loadEnd32]
  by_cases h32 : 32 ≤ us.length
  · rw [dite_eq_left h32]
    simp only
    have h1 := ih (us.length - 31) (by omega) (hiS ((us.take 32).getD 30 0)) (us.drop 31) (by simp)
    have h2 := ih (us.length - 32) (by omega) false (us.drop 32) (by simp)
    simp only [List.length_drop] at h1 h2
    split
    · split <;> omega
    · omega
  · rw [dite_eq_right h32]
    split
    · exact Nat.zero_le _
    · exact maskedLoadEnd_le us (by omega)

/-! ## The entry points of src/icelake/implementation.cpp -/

/-- `convert_utf16le_to_utf32` / `convert_utf16be_to_utf32` (src/icelake/implementation.cpp:1532-1541,
:1543-1551) and `convert_valid_utf16le_to_utf32` / `..be..` (:1584-1593,
:1595-1603), which are the same code: 0 on failure, else
`std::get<1>(ret) - utf32_output`. -/
def plain32 (us : List Nat) : Nat :=
  let r := conv32 false us
  if r.ok then r.outPos else 0

/-- Furthest store of `convert_utf16le_to_utf32_with_errors` /
`..be..` (:1553-1567, :1569-1582): the vector function, then on failure the
scalar `convert_with_errors` from `std::get<0>(ret)` on
`len - (std::get<0>(ret) - buf)` units, writing at `std::get<1>(ret)`
(`Scalar.u16to32`, the same step as `convert`). -/
def withErrors32Reach (us : List Nat) : Nat :=
  let r := conv32 false us
  if r.ok then r.reach
  else max r.reach (r.outPos + Scalar.run Scalar.u16to32 (us.length - r.inPos) (us.drop r.inPos))

/-- **Ice Lake UTF-16 → UTF-32, the whole function, no assumption.** For
every input of 16-bit units (values after the byte swap, so both
endiannesses):

* the stores of `convert_utf16_to_utf32` (the plain and `convert_valid`
  entry points) end within `utf32_length_from_utf16` of the input, and the
  count they return is that length on success (0 on failure);
* the stores of the `_with_errors` entry points, the scalar restart included,
  end within it, and the scalar restarts inside the input;
* every vector load is inside the input. -/
theorem safe_utf16to32_full (us : List Nat) (hu : Scalar.U16 us) :
    (conv32 false us).reach ≤ L (us.map w32v) ∧
    (plain32 us = 0 ∨ plain32 us = L (us.map w32v)) ∧
    withErrors32Reach us ≤ L (us.map w32v) ∧
    (conv32 false us).inPos ≤ us.length ∧
    loadEnd32 false us ≤ us.length := by
  obtain ⟨r1, r2, r3, r4⟩ := conv32_spec us.length false us rfl hu (fun h => by cases h)
  have hsc := Scalar.utf16to32_le (us.length - (conv32 false us).inPos)
    (us.drop (conv32 false us).inPos) (hu.drop _)
  refine ⟨r1, ?_, ?_, r2, conv32_loads us.length false us rfl⟩
  · unfold plain32; simp only
    split
    · rename_i h; exact Or.inr (r4 h).2
    · exact Or.inl rfl
  · unfold withErrors32Reach; simp only
    split
    · exact r1
    · omega

/-- The masked tail is reached: an input of 40 units with a surrogate pair
straddling lanes 30-31 leaves 9 units after the carried low surrogate, so the
masked tail runs with carry 1 and succeeds with the full count. -/
example :
    let us := List.replicate 30 0x41 ++ [0xD83D, 0xDE00] ++ List.replicate 8 0x42
    (conv32 false us).ok = true ∧ (conv32 false us).outPos = 39 ∧ L (us.map w32v) = 39 := by
  decide +kernel

/-- A high surrogate as the last unit of the masked tail: lane `n` of the
masked load is 0, so `V` has bit `n` set and the tail fails; nothing is
stored past the units before it. -/
example :
    let us := List.replicate 40 0x41 ++ [0xD800]
    (conv32 false us).ok = false ∧ (conv32 false us).inPos = 32 ∧ (conv32 false us).outPos = 32 ∧
      plain32 us = 0 ∧ withErrors32Reach us = 40 := by
  decide +kernel

end SafetyMargins.IcelakeUtf32Tail
