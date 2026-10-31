/-
The model shared by every kernel.

An input is the list of per-unit contributions of the length function the
caller uses to size the output buffer (`utf8_length_from_utf16`, ...).
`L` is their sum, so the buffer holds exactly `L xs` units.
`Pos` says each unit contributes at least 1, which holds for UTF-16, UTF-32
and Latin-1 sources and fails for a UTF-8 continuation byte or a low surrogate
counted by `utf32_length_from_utf16`.

A store is one of

* `rel a w`: `w` output units written at `L` of the first `a` input units of
  the iteration (a kernel that writes earlier, lagging behind `L`, only ends
  earlier, so this placement is an upper bound);
* `tail o`: the stores of this iteration end at most `o` past `L` of the
  consumed prefix;
* `ahead`: the store ends at most at `L` of the consumed prefix plus the next
  unit, which the loop test guarantees is present (one unit of look-ahead).

`stop` is an error return: the iteration writes at most `L` of what it
consumed, then nothing further is written.
-/

namespace SafetyMargins

/-- `true` adds the extra unit required by a strict `>` loop test. -/
def strictBump : Bool → Nat
  | true => 1
  | false => 0

inductive Place where
  | rel : Nat → Nat → Place
  | tail : Nat → Place
  | ahead : Place
deriving Repr, DecidableEq

structure Iter where
  consumed : Nat
  stores : List Place
  stop : Bool := false
deriving Repr, DecidableEq

def L : List Nat → Nat
  | [] => 0
  | x :: xs => x + L xs

@[simp] theorem L_nil : L [] = 0 := rfl
@[simp] theorem L_cons (x : Nat) (xs : List Nat) : L (x :: xs) = x + L xs := rfl

theorem L_append (xs ys : List Nat) : L (xs ++ ys) = L xs + L ys := by
  induction xs with
  | nil => simp
  | cons x xs ih => simp [ih, Nat.add_assoc]

theorem L_take_add_drop (n : Nat) (xs : List Nat) :
    L xs = L (xs.take n) + L (xs.drop n) := by
  rw [← L_append, List.take_append_drop]

theorem L_take_le (xs : List Nat) (c : Nat) : L (xs.take c) ≤ L xs := by
  have := L_take_add_drop c xs
  omega

/-- Every input unit contributes at least one output unit. -/
def Pos (xs : List Nat) : Prop := ∀ x ∈ xs, 1 ≤ x

theorem Pos.drop {xs : List Nat} (h : Pos xs) (n : Nat) : Pos (xs.drop n) := by
  intro x hx
  induction n generalizing xs with
  | zero =>
    simpa using h x hx
  | succ n ih =>
    cases xs with
    | nil => simp at hx
    | cons y ys =>
      apply ih (fun z hz => h z (List.mem_cons_of_mem y hz))
      simpa using hx

theorem L_drop_ge (xs : List Nat) (h : Pos xs) (n : Nat) (hn : n ≤ xs.length) :
    xs.length - n ≤ L (xs.drop n) := by
  induction xs generalizing n with
  | nil =>
    simp at hn
    simp [hn]
  | cons x xs ih =>
    cases n with
    | zero =>
      have hx : 1 ≤ x := h x (by simp)
      have htail : Pos xs := fun y hy => h y (List.mem_cons_of_mem x hy)
      have ih' := ih htail 0 (Nat.zero_le _)
      simp at ih' ⊢
      omega
    | succ n =>
      have htail : Pos xs := fun y hy => h y (List.mem_cons_of_mem x hy)
      have hn' : n ≤ xs.length := by simp at hn; omega
      simpa using ih htail n hn'

/-- If `a + w ≤ N ≤ |xs|` and every unit weighs at least 1, a store of width
`w` placed at `L (xs.take a)` ends inside `L xs`. -/
theorem take_add_le (xs : List Nat) (h : Pos xs) (a w N : Nat)
    (hw : a + w ≤ N) (hN : N ≤ xs.length) :
    L (xs.take a) + w ≤ L xs := by
  have ha : a ≤ xs.length := by omega
  have hdrop := L_drop_ge xs h a ha
  have hwN : a + w ≤ xs.length := Nat.le_trans hw hN
  have hw' : w ≤ xs.length - a := by
    rw [Nat.le_sub_iff_add_le ha, Nat.add_comm]
    exact hwN
  calc
    L (xs.take a) + w ≤ L (xs.take a) + L (xs.drop a) :=
      Nat.add_le_add_left (Nat.le_trans hw' hdrop) _
    _ = L xs := (L_take_add_drop a xs).symm

/-- Units the loop requires to be present: `B + m`, or `B + m + 1` when the
comparison is strict. -/
def need (B m : Nat) (strict : Bool) : Nat :=
  B + m + strictBump strict

/-- Input-offset plus width that this store needs the loop to have reserved. -/
def placeReq (consumed : Nat) : Place → Nat
  | .rel a w => a + w
  | .tail o => consumed + o
  | .ahead => consumed + 1

/-- Smallest margin that puts `req` inside `need B m strict`. -/
def marginOf (B : Nat) (strict : Bool) (req : Nat) : Nat :=
  req - B - strictBump strict

def Iter.ok (N : Nat) (it : Iter) : Bool :=
  it.stores.all fun s => decide (placeReq it.consumed s ≤ N)

def optimalMargin (B : Nat) (strict : Bool) (paths : List Iter) : Nat :=
  paths.foldl (fun acc it =>
    it.stores.foldl (fun acc s =>
      max acc (marginOf B strict (placeReq it.consumed s))) acc) 0

def storeEnd (xs : List Nat) (consumed : Nat) : Place → Nat
  | .rel a w => L (xs.take a) + w
  | .tail o => L (xs.take consumed) + o
  | .ahead => L (xs.take (consumed + 1))

def maxEnd (xs : List Nat) (consumed : Nat) (ss : List Place) : Nat :=
  ss.foldl (fun m s => max m (storeEnd xs consumed s)) 0

theorem maxEnd_le (xs : List Nat) (c bound acc : Nat) (ss : List Place)
    (hacc : acc ≤ bound) (h : ∀ s ∈ ss, storeEnd xs c s ≤ bound) :
    ss.foldl (fun m s => max m (storeEnd xs c s)) acc ≤ bound := by
  induction ss generalizing acc with
  | nil => simpa using hacc
  | cons s ss ih =>
    simp only [List.foldl_cons]
    apply ih
    · have hs := h s (List.mem_cons_self (a := s) (l := ss))
      exact Nat.max_le.mpr ⟨hacc, hs⟩
    · intro u hu
      exact h u (List.mem_cons_of_mem s hu)

theorem storeEnd_le (xs : List Nat) (h : Pos xs) (N c : Nat) (hN : N ≤ xs.length)
    (s : Place) (hs : placeReq c s ≤ N) : storeEnd xs c s ≤ L xs := by
  cases s with
  | rel a w =>
    simpa [storeEnd, placeReq] using take_add_le xs h a w N hs hN
  | tail o =>
    simpa [storeEnd, placeReq] using take_add_le xs h c o N hs hN
  | ahead =>
    exact L_take_le xs _

/-- Furthest output position written by `n` iterations, then a scalar tail.
Fuel `0` is the scalar tail, which writes exactly `L`. An iteration whose
input is shorter than `N` is also finished by that tail. -/
def written (N : Nat) (choose : List Nat → Iter) : Nat → List Nat → Nat
  | 0, xs => L xs
  | n + 1, xs =>
    if xs.length < N then
      L xs
    else if (choose xs).stop then
      max (maxEnd xs (choose xs).consumed (choose xs).stores)
        (L (xs.take (choose xs).consumed))
    else
      max (maxEnd xs (choose xs).consumed (choose xs).stores)
        (L (xs.take (choose xs).consumed) +
          written N choose n (xs.drop (choose xs).consumed))

/-- If every iteration the kernel can choose has stores inside `N`, then no
run writes past `L xs`. `choose` is arbitrary, so the path-selection logic of
the C++ kernel is not part of the proof. -/
theorem safe_of_paths (N : Nat) (paths : List Iter)
    (hp : paths.all (Iter.ok N) = true)
    (choose : List Nat → Iter)
    (hc : ∀ xs, N ≤ xs.length → choose xs ∈ paths) :
    ∀ n xs, Pos xs → written N choose n xs ≤ L xs := by
  intro n
  induction n with
  | zero =>
    intro xs _
    simp [written]
  | succ n ih =>
    intro xs hpos
    by_cases hshort : xs.length < N
    · unfold written
      rw [ite_eq_left hshort]
      exact Nat.le_refl _
    · have hlen : N ≤ xs.length := Nat.le_of_not_lt hshort
      have hmem : choose xs ∈ paths := hc xs hlen
      have hok : Iter.ok N (choose xs) = true :=
        (List.all_eq_true.mp hp) _ hmem
      have hstores : ∀ s ∈ (choose xs).stores,
          storeEnd xs (choose xs).consumed s ≤ L xs := by
        intro s hs
        have hb : decide (placeReq (choose xs).consumed s ≤ N) = true :=
          (List.all_eq_true.mp hok) s hs
        exact storeEnd_le xs hpos N _ hlen s (of_decide_eq_true hb)
      have hhere : maxEnd xs (choose xs).consumed (choose xs).stores ≤ L xs := by
        simpa [maxEnd] using maxEnd_le xs _ (L xs) 0 _ (Nat.zero_le _) hstores
      unfold written
      rw [ite_eq_right hshort]
      by_cases hstop : (choose xs).stop
      · rw [ite_eq_left hstop]
        exact Nat.max_le.mpr ⟨hhere, L_take_le xs _⟩
      · rw [ite_eq_right hstop]
        have hrest := ih (xs.drop (choose xs).consumed) (Pos.drop hpos _)
        have hsum := L_take_add_drop (choose xs).consumed xs
        exact Nat.max_le.mpr ⟨hhere, by omega⟩

/-- Every store of `it` ends at or before `L` of the prefix it consumed. -/
def exactOn (xs : List Nat) (it : Iter) : Prop :=
  ∀ s ∈ it.stores, storeEnd xs it.consumed s ≤ L (xs.take it.consumed)

/-- Every store of `it` ends inside `L xs`. -/
def fitsOn (xs : List Nat) (it : Iter) : Prop :=
  ∀ s ∈ it.stores, storeEnd xs it.consumed s ≤ L xs

/-- A kernel whose every store fits needs no `Pos`: what follows a store is
accounted at `L` of the consumed prefix. -/
theorem safe_fits (N : Nat) (choose : List Nat → Iter)
    (hc : ∀ xs, N ≤ xs.length → fitsOn xs (choose xs)) :
    ∀ n xs, written N choose n xs ≤ L xs := by
  intro n
  induction n with
  | zero =>
    intro xs
    simp [written]
  | succ n ih =>
    intro xs
    by_cases hshort : xs.length < N
    · unfold written
      rw [ite_eq_left hshort]
      exact Nat.le_refl _
    · have hlen : N ≤ xs.length := Nat.le_of_not_lt hshort
      have hstores : ∀ s ∈ (choose xs).stores,
          storeEnd xs (choose xs).consumed s ≤ L xs := hc xs hlen
      have hhere : maxEnd xs (choose xs).consumed (choose xs).stores ≤ L xs := by
        simpa [maxEnd] using maxEnd_le xs _ (L xs) 0 _ (Nat.zero_le _) hstores
      unfold written
      rw [ite_eq_right hshort]
      by_cases hstop : (choose xs).stop
      · rw [ite_eq_left hstop]
        exact Nat.max_le.mpr ⟨hhere, L_take_le xs _⟩
      · rw [ite_eq_right hstop]
        have hrest := ih (xs.drop (choose xs).consumed)
        have hsum := L_take_add_drop (choose xs).consumed xs
        exact Nat.max_le.mpr ⟨hhere, by omega⟩

/-- A kernel that never stores past the output of what it consumed needs
neither a margin nor `Pos`. This covers sources with units of weight 0. -/
theorem safe_exact (N : Nat) (choose : List Nat → Iter)
    (hc : ∀ xs, N ≤ xs.length → exactOn xs (choose xs)) :
    ∀ n xs, written N choose n xs ≤ L xs :=
  safe_fits N choose fun xs h s hs => Nat.le_trans (hc xs h s hs) (L_take_le _ _)

/-- Every store of the iteration is `tail 0`. -/
def Iter.exact (it : Iter) : Bool := it.stores.all fun s => s == .tail 0

theorem exactOn_of_exact (xs : List Nat) (it : Iter) (h : it.exact = true) :
    exactOn xs it := by
  intro s hs
  have hs0 : s = .tail 0 := by simpa using (List.all_eq_true.mp h) s hs
  subst hs0
  simp [storeEnd]

/-- Path tables made only of `tail 0` stores (masked or `vl`-exact stores):
safe for every input. -/
theorem safe_of_exact (N : Nat) (paths : List Iter)
    (hp : paths.all Iter.exact = true)
    (choose : List Nat → Iter)
    (hc : ∀ xs, N ≤ xs.length → choose xs ∈ paths) :
    ∀ n xs, written N choose n xs ≤ L xs :=
  safe_exact N choose fun xs h =>
    exactOn_of_exact xs _ ((List.all_eq_true.mp hp) _ (hc xs h))

end SafetyMargins
