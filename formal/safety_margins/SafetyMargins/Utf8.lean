import SafetyMargins.Basic
/-
UTF-8 sources. The length functions (`utf16_length_from_utf8`,
`utf32_length_from_utf8`, `latin1_length_from_utf8`) give 0 for a continuation
byte and at least 1 for any other byte, so no fixed byte margin helps on
adversarial input. The validating kernels instead walk back from the end until
they have seen `K` leading bytes (`K = 3`):

    size_t leading_byte = 0;
    size_t margin = size;
    for (; margin > 0 && leading_byte < K; margin--)
      leading_byte += (int8_t(in[margin - 1]) > -65);
    const size_t safety_margin = size - margin + 1;
    while (pos + 64 + safety_margin <= size) { ... }

`size - margin` is the length `tk K xs` of the shortest suffix holding `K`
leading bytes (the whole input if there are fewer).

As before an input is the list of per-byte length-function contributions; a
leading byte is a byte with a non-zero contribution.
-/
namespace SafetyMargins.Utf8
open SafetyMargins

/-- Number of leading (non-continuation) bytes. -/
def cnt : List Nat → Nat
  | [] => 0
  | x :: xs => (if x = 0 then 0 else 1) + cnt xs

@[simp] theorem cnt_nil : cnt [] = 0 := rfl
@[simp] theorem cnt_cons (x : Nat) (xs : List Nat) :
    cnt (x :: xs) = (if x = 0 then 0 else 1) + cnt xs := rfl

theorem cnt_le_L (xs : List Nat) : cnt xs ≤ L xs := by
  induction xs with
  | nil => simp
  | cons x xs ih => simp; split <;> omega

theorem cnt_le_length (xs : List Nat) : cnt xs ≤ xs.length := by
  induction xs with
  | nil => simp
  | cons x xs ih => simp; split <;> omega

theorem cnt_append (xs ys : List Nat) : cnt (xs ++ ys) = cnt xs + cnt ys := by
  induction xs with
  | nil => simp
  | cons x xs ih => simp [ih, Nat.add_assoc]

theorem cnt_drop_split (xs : List Nat) (a d : Nat) :
    cnt (xs.drop a) = cnt ((xs.drop a).take d) + cnt (xs.drop (a + d)) := by
  induction xs generalizing a d with
  | nil => simp
  | cons x xs ih =>
    cases a with
    | zero =>
      cases d with
      | zero => simp
      | succ d =>
        simp only [List.drop_zero, List.take_succ_cons, cnt_cons]
        have ih' : cnt xs = cnt (xs.take d) + cnt (xs.drop d) := by
          simpa [List.drop_zero] using ih 0 d
        have hdrop : (x :: xs).drop (d + 1) = xs.drop d := List.drop_succ_cons
        rw [Nat.zero_add, hdrop, ih', Nat.add_assoc]
    | succ a =>
      simp only [List.drop_succ_cons]
      have hidx : (x :: xs).drop (a + 1 + d) = xs.drop (a + d) := by
        have : a + 1 + d = a + d + 1 := by omega
        rw [this, List.drop_succ_cons]
      rw [hidx]
      exact ih a d

theorem cnt_drop_mono (xs : List Nat) {a b : Nat} (h : a ≤ b) :
    cnt (xs.drop b) ≤ cnt (xs.drop a) := by
  have := cnt_drop_split xs a (b - a)
  have hb : a + (b - a) = b := by omega
  rw [hb] at this
  omega

/-- Length of the suffix scanned by the backward loop. -/
def tk (K : Nat) : List Nat → Nat
  | [] => 0
  | _ :: xs => if K ≤ cnt xs then tk K xs else xs.length + 1

theorem tk_le (K : Nat) (xs : List Nat) : tk K xs ≤ xs.length := by
  induction xs with
  | nil => simp [tk]
  | cons x xs ih => simp only [tk]; split <;> simp <;> omega

/-- If the scan stopped before the beginning of the input, the scanned suffix
holds at least `K` leading bytes. -/
theorem tk_spec (K : Nat) (xs : List Nat) (h : tk K xs < xs.length) :
    K ≤ cnt (xs.drop (xs.length - tk K xs)) := by
  induction xs with
  | nil => simp [tk] at h
  | cons x xs ih =>
    by_cases hK : K ≤ cnt xs
    · have htk : tk K (x :: xs) = tk K xs := by
        simp only [tk, ite_eq_left hK]
      rw [htk, List.length_cons] at h ⊢
      have hle := tk_le K xs
      have e : xs.length + 1 - tk K xs = (xs.length - tk K xs) + 1 := by omega
      rw [e, List.drop_succ_cons]
      by_cases hlt : tk K xs < xs.length
      · exact ih hlt
      · have hz : xs.length - tk K xs = 0 := by omega
        simp [hz]
        exact hK
    · have htk : tk K (x :: xs) = xs.length + 1 := by
        simp only [tk, ite_eq_right hK]
      simp [htk, List.length_cons] at h

/-- The loop body only runs if the scan stopped early, so `K` leading bytes
lie after the block. This also gives `tk ≥ K`, which is what keeps the
16-byte loads of the last call (up to byte `pos + 66`) inside the input when
`K ≥ 2`. -/
theorem tk_ge (K : Nat) (xs : List Nat) (h : tk K xs < xs.length) : K ≤ tk K xs := by
  have h1 := tk_spec K xs h
  have h2 := cnt_le_length (xs.drop (xs.length - tk K xs))
  have h3 : (xs.drop (xs.length - tk K xs)).length = tk K xs := by
    have := tk_le K xs
    simp
    omega
  omega

/-- **Covering lemma for the dynamic margin.** Consider the outer block at
`pos` (the loop condition `pos + 64 + safety_margin <= size` holds) and a call
of `convert_masked_utf8_to_*` starting at byte `t` that writes `s` units at
output position `o`. Suppose
* `o + D ≤ L (xs.take t)`: the output so far lags `L` of the consumed input by
  at least `D` (the invariant, with `D = 0` for an aligned call);
* the `d` bytes from `t` end inside the block, `t + d ≤ pos + 64`;
* `s ≤ D + cnt (those d bytes) + K`.
Then the store ends inside the buffer. -/
theorem cover (K pos t d o s D : Nat) (xs : List Nat)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hd : t + d ≤ pos + 64)
    (ho : o + D ≤ L (xs.take t))
    (hs : s ≤ D + cnt ((xs.drop t).take d) + K) :
    o + s ≤ L xs := by
  have htk := tk_le K xs
  have hlt : tk K xs < xs.length := by omega
  have hK := tk_spec K xs hlt
  have hsplit := cnt_drop_split xs t d
  have hidx : t + d ≤ xs.length - tk K xs := by omega
  have hmono := cnt_drop_mono xs hidx
  have htail : K ≤ cnt (xs.drop (t + d)) := Nat.le_trans hK hmono
  have hcnt : cnt ((xs.drop t).take d) + K ≤ cnt (xs.drop t) := by
    rw [hsplit]
    exact Nat.add_le_add_left htail _
  have hLdrop : cnt (xs.drop t) ≤ L (xs.drop t) := cnt_le_L (xs.drop t)
  have hstore : o + s ≤ L (xs.take t) + (cnt ((xs.drop t).take d) + K) := by
    omega
  calc
    o + s ≤ L (xs.take t) + (cnt ((xs.drop t).take d) + K) := hstore
    _ ≤ L (xs.take t) + cnt (xs.drop t) := Nat.add_le_add_left hcnt _
    _ ≤ L (xs.take t) + L (xs.drop t) := Nat.add_le_add_left hLdrop _
    _ = L xs := (L_take_add_drop t xs).symm

/-- Same payment as `cover`, with `L` of the inspected bytes in place of their
leading-byte count. A 4-byte lead weighs 2 under `utf16_length_from_utf8` and
pays 2 here. `K` is still a count of leading bytes; `cnt ≤ L` converts it. -/
theorem coverL (K pos t d o s D : Nat) (xs : List Nat)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hd : t + d ≤ pos + 64)
    (ho : o + D ≤ L (xs.take t))
    (hs : s ≤ D + L ((xs.drop t).take d) + K) :
    o + s ≤ L xs := by
  have htk := tk_le K xs
  have hlt : tk K xs < xs.length := by omega
  have hK := tk_spec K xs hlt
  have hidx : t + d ≤ xs.length - tk K xs := by omega
  have hmono := cnt_drop_mono xs hidx
  have htail : K ≤ cnt (xs.drop (t + d)) := Nat.le_trans hK hmono
  have hLtail : K ≤ L (xs.drop (t + d)) := Nat.le_trans htail (cnt_le_L _)
  have hsplit : L (xs.drop t) =
      L ((xs.drop t).take d) + L (xs.drop (t + d)) := by
    have hdrop : (xs.drop t).drop d = xs.drop (t + d) := by
      rw [List.drop_drop, Nat.add_comm]
    rw [← hdrop]
    exact L_take_add_drop d (xs.drop t)
  have hpay : L ((xs.drop t).take d) + K ≤ L (xs.drop t) := by
    rw [hsplit]
    exact Nat.add_le_add_left hLtail _
  have hstore : o + s ≤ L (xs.take t) + (L ((xs.drop t).take d) + K) := by
    omega
  calc
    o + s ≤ L (xs.take t) + (L ((xs.drop t).take d) + K) := hstore
    _ ≤ L (xs.take t) + L (xs.drop t) := Nat.add_le_add_left hpay _
    _ = L xs := (L_take_add_drop t xs).symm

/-- The usual shape. A call starting at a leading byte, consuming `c` bytes
that end at a code-point boundary inside the block (`t + c ≤ pos + 63`, the
byte at `t + c` is a leading byte) and producing `e` code units with
`e ≤ cnt (consumed bytes)` (`TableFacts.bigindex_exact`; for UTF-16 a surrogate
pair is only produced from a lead byte `≥ 0xF0`, which the length function also
counts twice) is safe as soon as `s ≤ e + 1 + K`. So the optimal `K` is
`max (s - e) - 1` over the store paths. -/
theorem cover_aligned (K pos t c o s e : Nat) (xs : List Nat)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hc : t + c + 1 ≤ pos + 64)
    (ho : o ≤ L (xs.take t))
    (he : e + 1 ≤ cnt ((xs.drop t).take (c + 1)))
    (hs : s ≤ e + 1 + K) :
    o + s ≤ L xs :=
  cover K pos t (c + 1) o s 0 xs hloop (by omega) (by omega) (by omega)

/-- Reads: a call at `pos + 51` loads 16 bytes, through `pos + 66`. With
`K ≥ 2` the loop condition keeps that inside the input. -/
theorem read_ok (K pos : Nat) (xs : List Nat) (hK : 2 ≤ K)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length) : pos + 66 < xs.length := by
  have hlt : tk K xs < xs.length := by omega
  have := tk_ge K xs hlt
  omega

/-- and `K = 1` does not: 66 ASCII bytes, `tk 1 = 1`, the loop runs at
`pos = 0`, byte 66 is past the end. -/
theorem read_K1_fails :
    let xs := List.replicate 66 1
    0 + 64 + (tk 1 xs + 1) ≤ xs.length ∧ ¬ (0 + 66 < xs.length) := by decide

/-- A store path of `convert_masked_utf8_to_*`: `s` units stored, `f` leading
bytes guaranteed in the consumed bytes plus the next one (`e + 1` for a normal
path). -/
structure MPath where
  s : Nat
  f : Nat
deriving Repr

def okK (K : Nat) (ps : List MPath) : Bool := ps.all fun p => decide (p.s ≤ p.f + K)

/-! ## The lag invariant

The generic validating decoders, as a transition system. The output cursor
never runs ahead of `L` of the consumed input, and inside a block it lags by
one more when the next call starts on a continuation byte. `cover` then bounds
every store. -/

theorem L_take_add (xs : List Nat) (t c : Nat) :
    L (xs.take (t + c)) = L (xs.take t) + L ((xs.drop t).take c) := by
  rw [List.take_add, L_append]

theorem getD_le_take (xs : List Nat) (t c : Nat) (hc : 1 ≤ c) :
    xs.getD t 0 ≤ L ((xs.drop t).take c) := by
  obtain ⟨c, rfl⟩ : ∃ c', c = c' + 1 := ⟨c - 1, by omega⟩
  induction xs generalizing t with
  | nil => simp
  | cons x xs ih =>
    cases t with
    | zero => simp [List.take_succ_cons]
    | succ t => simpa using ih t

/-- 1 when byte `t` is a continuation byte. -/
def cont (xs : List Nat) (t : Nat) : Nat := if xs.getD t 0 = 0 then 1 else 0

/-- The lag invariant: the output is behind `L` of the consumed input, by at
least one when the next call starts on a continuation byte. -/
def Lag (xs : List Nat) (t o : Nat) : Prop := o + cont xs t ≤ L (xs.take t)

/-- One call of `convert_masked_utf8_to_*` at byte `t`, as the inner loop sees
it: `c` bytes consumed, the output advanced by `p`. -/
inductive Step (xs : List Nat) : Nat → Nat → Nat → Nat → Prop
  /-- A table row below 209 or a fast path: the call ends just before a
  leading byte, and every code point it produces starts at a byte of the
  consumed prefix and weighs at most that byte's `L` (or 1 for a call that
  starts on a continuation byte). -/
  | conv (t o c p : Nat) (hc : 1 ≤ c) (hnext : xs.getD (t + c) 0 ≠ 0)
      (hp : p ≤ L ((xs.drop t).take c) + cont xs t) : Step xs t o (t + c) (o + p)
  /-- An error row (`idx >= 209`, or arm64's failed `0x888` check): 12 bytes
  consumed, the output does not advance. -/
  | error (t o : Nat) : Step xs t o (t + 12) o

theorem Step.lag {xs : List Nat} {t o t' o' : Nat} (hs : Step xs t o t' o')
    (h : Lag xs t o) : Lag xs t' o' := by
  unfold Lag at *
  cases hs with
  | conv c p hc hnext hp =>
    have hcont : cont xs (t + c) = 0 := by unfold cont; exact ite_eq_right_iff.mpr (fun h => absurd h hnext)
    rw [hcont, L_take_add]
    omega
  | error =>
    rw [L_take_add]
    have hhead := getD_le_take xs t 12 (by omega)
    have hc : cont xs (t + 12) ≤ 1 := by unfold cont; split <;> omega
    unfold cont at h
    split at h
    · omega
    · rename_i hne
      have : 1 ≤ xs.getD t 0 := Nat.pos_of_ne_zero hne
      omega

/-- States reached by the generic validating decoders
(`utf8_to_utf16.h`, `utf8_to_utf32.h`, `utf8_to_latin1.h`): `block t o` is the
head of the outer loop, `inner pos t o` the head of the inner loop of the block
at `pos`. -/
inductive Reach (xs : List Nat) : Bool → Nat → Nat → Nat → Prop
  | start : Reach xs false 0 0 0
  /-- ASCII block: 64 ASCII bytes, 64 units out. -/
  | ascii (t o : Nat) (h : Reach xs false t t o)
      (hascii : ∀ i, i < 64 → xs.getD (t + i) 0 = 1) :
      Reach xs false (t + 64) (t + 64) (o + 64)
  /-- Non-ASCII block: rejected unless its first byte is leading
  (`utf8_continuation_mask & 1`). -/
  | enter (t o : Nat) (h : Reach xs false t t o) (hlead : xs.getD t 0 ≠ 0) :
      Reach xs true t t o
  /-- Inner loop: `while (pos < max_starting_point)`. -/
  | call (pos t o t' o' : Nat) (h : Reach xs true pos t o) (ht : t < pos + 52)
      (hs : Step xs t o t' o') : Reach xs true pos t' o'
  /-- Leaving the inner loop: the next block starts where the calls stopped. -/
  | leave (pos t o : Nat) (h : Reach xs true pos t o) (ht : pos + 52 ≤ t) :
      Reach xs false t t o

theorem ascii_take (xs : List Nat) (t : Nat)
    (hascii : ∀ i, i < 64 → xs.getD (t + i) 0 = 1) :
    64 ≤ L ((xs.drop t).take 64) := by
  have key : ∀ n, n ≤ 64 → n ≤ L ((xs.drop t).take n) := by
    intro n
    induction n with
    | zero => intro _; simp
    | succ n ih =>
      intro hn
      have h1 := ih (by omega)
      have hsplit : (xs.drop t).take (n + 1) = (xs.drop t).take n ++ ((xs.drop (t + n)).take 1) := by
        rw [List.take_add, List.drop_drop]
      rw [hsplit, L_append]
      have := getD_le_take xs (t + n) 1 (by omega)
      have := hascii n (by omega)
      omega
  exact key 64 (Nat.le_refl _)

/-- The invariant of every reachable state: at a block head the output is at
most `L` of the consumed input; inside a block it lags by one more when the
next call starts on a continuation byte. -/
theorem reach_lag (xs : List Nat) :
    ∀ b pos t o, Reach xs b pos t o →
      (b = false → o ≤ L (xs.take t)) ∧ (b = true → Lag xs t o) := by
  intro b pos t o h
  induction h with
  | start => simp
  | ascii t o h hascii ih =>
    refine ⟨fun _ => ?_, fun hb => by simp at hb⟩
    have := ih.1 rfl
    rw [L_take_add]
    have := ascii_take xs t hascii
    omega
  | enter t o h hlead ih =>
    refine ⟨fun hb => by simp at hb, fun _ => ?_⟩
    have := ih.1 rfl
    have hc : cont xs t = 0 := by unfold cont; exact ite_eq_right_iff.mpr (fun h => absurd h hlead)
    unfold Lag
    omega
  | call pos t o t' o' h ht hs ih =>
    exact ⟨fun hb => by simp at hb, fun _ => hs.lag (ih.2 rfl)⟩
  | leave pos t o h ht ih =>
    refine ⟨fun _ => ?_, fun hb => by simp at hb⟩
    have := ih.2 rfl
    unfold Lag at this
    omega

/-- Every store of a call in a reachable state fits: a call at `t` that
inspects `d` bytes inside its block and stores `s` units with
`s ≤ cont + cnt (those bytes) + K` (the `okK` condition, with one leading
byte fewer for a call that starts on a continuation byte). -/
theorem reach_store (K pos t o d s : Nat) (xs : List Nat)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hd : t + d ≤ pos + 64)
    (hs : s ≤ cont xs t + cnt ((xs.drop t).take d) + K) :
    o + s ≤ L xs :=
  cover K pos t d o s (cont xs t) xs hloop hd ((reach_lag xs _ _ _ _ h).2 rfl) hs

/-- The same with a store path of `okK`: `p.f ≤ cont + cnt` of the
inspected bytes is `TableFacts.bigindex_exact` (`e + 1` leading bytes for an
aligned call, `e` for one that starts on a continuation byte). -/
theorem reach_okK (K pos t o d : Nat) (xs : List Nat) (ps : List MPath) (q : MPath)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hd : t + d ≤ pos + 64)
    (hK : okK K ps = true) (hq : q ∈ ps)
    (hf : q.f ≤ cont xs t + cnt ((xs.drop t).take d)) :
    o + q.s ≤ L xs := by
  have := of_decide_eq_true ((List.all_eq_true.mp hK) q hq)
  exact reach_store K pos t o d q.s xs h hloop hd (by omega)

/-- `coverL` at a reachable call. The slack `cont` is the lag invariant. -/
theorem reach_L (K pos t o d s : Nat) (xs : List Nat)
    (h : Reach xs true pos t o)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hd : t + d ≤ pos + 64)
    (hs : s ≤ cont xs t + L ((xs.drop t).take d) + K) :
    o + s ≤ L xs :=
  coverL K pos t d o s (cont xs t) xs hloop hd ((reach_lag xs _ _ _ _ h).2 rfl) hs

/-- Store paths, as `(stored, produced + 1)`; paths that store exactly what
they produce are omitted (they need `K ≥ 0`). Sources:
src/{westmere,haswell,ppc64}/*_convert_utf8_to_utf16.cpp: `0xfff` (16 stored,
12 produced), `0x924` (8, 4), `idx < 64` (8, 6), `idx < 145` (8, 4). -/
def x86_utf16 : List MPath := [⟨16, 13⟩, ⟨8, 5⟩, ⟨8, 7⟩, ⟨8, 5⟩]
/-- src/{lsx,lasx}/*_convert_utf8_to_utf16.cpp. -/
def la_utf16 : List MPath := [⟨8, 5⟩, ⟨8, 5⟩, ⟨8, 7⟩, ⟨8, 7⟩]
/-- src/arm64/arm_convert_utf8_to_utf16.cpp: `0xaaa` and `idx < 64` (8, 6), and
the `0x888` path that writes 6 units before rejecting bad lead bytes and then
advances 0: 3 leading bytes consumed plus the next one. -/
def arm_utf16 : List MPath := [⟨8, 7⟩, ⟨8, 7⟩, ⟨6, 4⟩]
/-- utf32 on westmere, haswell, ppc64, arm64: `0xfff` (16, 12), `idx < 64`
(8, 6), 3-unit path (4, 3). -/
def x86_utf32 : List MPath := [⟨16, 13⟩, ⟨8, 7⟩, ⟨4, 4⟩]
/-- utf32 on lsx, lasx. -/
def la_utf32 : List MPath := [⟨8, 7⟩, ⟨8, 7⟩, ⟨4, 4⟩]
/-- latin1 on westmere, haswell, arm64, ppc64: `0xfff` (16, 12), `idx < 64` (8, 6). -/
def x86_latin1 : List MPath := [⟨16, 13⟩, ⟨8, 7⟩]

theorem K_x86_utf16 : okK 3 x86_utf16 = true ∧ okK 2 x86_utf16 = false := by decide
theorem K_la_utf16 : okK 3 la_utf16 = true ∧ okK 2 la_utf16 = false := by decide
theorem K_arm_utf16 : okK 2 arm_utf16 = true ∧ okK 1 arm_utf16 = false := by decide
theorem K_x86_utf32 : okK 3 x86_utf32 = true ∧ okK 2 x86_utf32 = false := by decide
theorem K_la_utf32 : okK 1 la_utf32 = true ∧ okK 0 la_utf32 = false := by decide
theorem K_x86_latin1 : okK 3 x86_latin1 = true ∧ okK 2 x86_latin1 = false := by decide

/-- Tightness of `K = 3` for the `0xfff` path, with `K = 2`. Byte weights:
a 51-byte prefix (any contents: only the suffix matters, here modelled as
ASCII), then 13 leading bytes (the call at byte 51 takes the `0xfff` path:
bytes 51..62 ASCII, byte 63 a lead byte), one continuation byte, two ASCII
bytes. 67 bytes; the 16-unit store at `L (prefix)` ends one unit past `L`.
This is the weight list only. It does not prove that a kernel reaches byte 51. -/
theorem fff_tight :
    let xs := List.replicate 64 1 ++ [0] ++ [1, 1]
    tk 2 xs = 2 ∧ 0 + 64 + (tk 2 xs + 1) ≤ xs.length ∧
    L (xs.take 51) + 16 = L xs + 1 := by decide

/-! ## `convert_valid` with a fixed margin

For valid input the output is exact (`o = L (xs.take t)`) and the bytes from
the end of the last call (at most `pos + 63`) to the end of the input form at
least `1 + m` bytes of complete characters. A valid string is described by the
list of its character lengths. -/

def bytes : List Nat → Nat
  | [] => 0
  | k :: ks => k + bytes ks

/-- UTF-16 units of a valid string. -/
def u16 : List Nat → Nat
  | [] => 0
  | k :: ks => (if k = 4 then 2 else 1) + u16 ks

def Valid (ks : List Nat) : Prop := ∀ k ∈ ks, 1 ≤ k ∧ k ≤ 4

theorem bytes_le_3u16 (ks : List Nat) (h : Valid ks) : bytes ks ≤ 3 * u16 ks := by
  induction ks with
  | nil => simp [bytes, u16]
  | cons k ks ih =>
    have hk := h k (by simp)
    have := ih (fun j hj => h j (by simp [hj]))
    simp only [bytes, u16]
    split <;> omega

theorem bytes_le_4u32 (ks : List Nat) (h : Valid ks) : bytes ks ≤ 4 * ks.length := by
  induction ks with
  | nil => simp [bytes]
  | cons k ks ih =>
    have hk := h k (by simp)
    have := ih (fun j hj => h j (by simp [hj]))
    simp only [bytes, List.length_cons]
    omega

/-- UTF-16 output, overshoot 4 (x86, ppc64, lsx, lasx): margin 9 suffices. -/
theorem valid_utf16 (ks : List Nat) (h : Valid ks) (m : Nat) (hm : 9 ≤ m)
    (hb : 1 + m ≤ bytes ks) : 4 ≤ u16 ks := by
  have := bytes_le_3u16 ks h
  omega

/-- and 8 does not: three 3-byte characters fill 9 bytes with 3 units. -/
theorem valid_utf16_tight : bytes [3, 3, 3] = 1 + 8 ∧ u16 [3, 3, 3] = 3 := by decide

/-- UTF-32 output, overshoot 4 (x86, ppc64, arm64): margin 12 suffices. -/
theorem valid_utf32 (ks : List Nat) (h : Valid ks) (m : Nat) (hm : 12 ≤ m)
    (hb : 1 + m ≤ bytes ks) : 4 ≤ ks.length := by
  have := bytes_le_4u32 ks h
  omega

/-- and 11 does not: three 4-byte characters fill 12 bytes with 3 units. -/
theorem valid_utf32_tight : bytes [4, 4, 4] = 1 + 11 ∧ [4, 4, 4].length = 3 := by decide

/-- A store that ends at most `d` past `L` of the consumed prefix lands in the
buffer once the unread tail weighs at least `d`. -/
theorem overshoot_le (pre tail : List Nat) (s d : Nat)
    (hs : s ≤ L pre + d) (ht : d ≤ L tail) : s ≤ L (pre ++ tail) := by
  rw [L_append]
  omega

/-- Byte weights of one valid character under `utf16_length_from_utf8`. -/
def chars16 : Nat → List Nat
  | 1 => [1]
  | 2 => [1, 0]
  | 3 => [1, 0, 0]
  | 4 => [2, 0, 0, 0]
  | _ => []

/-- Byte weights under `utf32_length_from_utf8` (one unit per character). -/
def chars32 : Nat → List Nat
  | 1 => [1]
  | 2 => [1, 0]
  | 3 => [1, 0, 0]
  | 4 => [1, 0, 0, 0]
  | _ => []

def expand16 : List Nat → List Nat
  | [] => []
  | k :: ks => chars16 k ++ expand16 ks

def expand32 : List Nat → List Nat
  | [] => []
  | k :: ks => chars32 k ++ expand32 ks

theorem L_chars16 (k : Nat) (h1 : 1 ≤ k) (h2 : k ≤ 4) :
    L (chars16 k) = if k = 4 then 2 else 1 := by
  have hk : k = 1 ∨ k = 2 ∨ k = 3 ∨ k = 4 := by omega
  rcases hk with rfl | rfl | rfl | rfl <;> simp [chars16, L]

theorem L_chars32 (k : Nat) (h1 : 1 ≤ k) (h2 : k ≤ 4) : L (chars32 k) = 1 := by
  have hk : k = 1 ∨ k = 2 ∨ k = 3 ∨ k = 4 := by omega
  rcases hk with rfl | rfl | rfl | rfl <;> simp [chars32, L]

theorem length_chars16 (k : Nat) (h1 : 1 ≤ k) (h2 : k ≤ 4) :
    (chars16 k).length = k := by
  have hk : k = 1 ∨ k = 2 ∨ k = 3 ∨ k = 4 := by omega
  rcases hk with rfl | rfl | rfl | rfl <;> simp [chars16]

theorem length_chars32 (k : Nat) (h1 : 1 ≤ k) (h2 : k ≤ 4) :
    (chars32 k).length = k := by
  have hk : k = 1 ∨ k = 2 ∨ k = 3 ∨ k = 4 := by omega
  rcases hk with rfl | rfl | rfl | rfl <;> simp [chars32]

theorem L_expand16 (ks : List Nat) (h : Valid ks) : L (expand16 ks) = u16 ks := by
  induction ks with
  | nil => simp [expand16, u16]
  | cons k ks ih =>
    have hk := h k (by simp)
    rw [expand16, L_append, L_chars16 k hk.1 hk.2,
      ih (fun j hj => h j (List.mem_cons_of_mem k hj))]
    simp [u16]

theorem L_expand32 (ks : List Nat) (h : Valid ks) : L (expand32 ks) = ks.length := by
  induction ks with
  | nil => simp [expand32]
  | cons k ks ih =>
    have hk := h k (by simp)
    rw [expand32, L_append, L_chars32 k hk.1 hk.2,
      ih (fun j hj => h j (List.mem_cons_of_mem k hj)), List.length_cons,
      Nat.add_comm]

theorem length_expand16 (ks : List Nat) (h : Valid ks) :
    (expand16 ks).length = bytes ks := by
  induction ks with
  | nil => simp [expand16, bytes]
  | cons k ks ih =>
    have hk := h k (by simp)
    rw [expand16, List.length_append, length_chars16 k hk.1 hk.2,
      ih (fun j hj => h j (List.mem_cons_of_mem k hj)), bytes]

theorem length_expand32 (ks : List Nat) (h : Valid ks) :
    (expand32 ks).length = bytes ks := by
  induction ks with
  | nil => simp [expand32, bytes]
  | cons k ks ih =>
    have hk := h k (by simp)
    rw [expand32, List.length_append, length_chars32 k hk.1 hk.2,
      ih (fun j hj => h j (List.mem_cons_of_mem k hj)), bytes]

/-- Store form of `valid_utf16`. The kernel hypotheses, read from the C++ and
not parsed here, are that the cursor is `L` of the consumed prefix and the
store ends at most 4 past it, and that the unread tail is `1 + m` bytes of
complete characters (`length_expand16`). The shared header
`valid_utf8_to_utf16.h` has this deficit on x86, ppc64, lsx and lasx. -/
theorem valid_utf16_store (pre : List Nat) (ks : List Nat) (s m : Nat)
    (h : Valid ks) (hm : 9 ≤ m) (hb : 1 + m ≤ bytes ks)
    (hs : s ≤ L pre + 4) : s ≤ L (pre ++ expand16 ks) := by
  have hden := valid_utf16 ks h m hm hb
  exact overshoot_le pre (expand16 ks) s 4 hs (by rw [L_expand16 ks h]; exact hden)

/-- Store form of `valid_utf32`, deficit 4. This is the shared header
`valid_utf8_to_utf32.h`: x86, ppc64 and arm64 write 16 and produce 12. -/
theorem valid_utf32_store (pre : List Nat) (ks : List Nat) (s m : Nat)
    (h : Valid ks) (hm : 12 ≤ m) (hb : 1 + m ≤ bytes ks)
    (hs : s ≤ L pre + 4) : s ≤ L (pre ++ expand32 ks) := by
  have hden := valid_utf32 ks h m hm hb
  exact overshoot_le pre (expand32 ks) s 4 hs (by rw [L_expand32 ks h]; exact hden)

end SafetyMargins.Utf8
