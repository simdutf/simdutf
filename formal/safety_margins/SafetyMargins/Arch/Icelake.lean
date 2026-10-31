import SafetyMargins.Kernels
import SafetyMargins.Utf8
/-
Ice Lake (AVX-512): every transcoding loop of src/icelake (base64 excluded).
Most kernels use masked stores of exactly the produced count (`tail 0`), which
need no margin and no `Pos` (`Basic.safe_of_exact`).
-/
namespace SafetyMargins.Arch.Icelake
open SafetyMargins SafetyMargins.Kernels SafetyMargins.Utf8

/-! ## Latin-1 to UTF-8 -/

/-- src/icelake/icelake_convert_latin1_to_utf8.inl.cpp:90, loop
`pos + 96 <= len`, 64 bytes per iteration (`B = 64`, `m = 32`).
`latin1_to_utf8_avx512_branch` (line 74): ASCII stores 64 bytes (line 80);
otherwise `latin1_to_utf8_avx512_vec(..., mask_output = 0)` stores 64 bytes at
the cursor (line 67) and 64 bytes at `output_sizeA` = 32 + nonascii(first 32)
= L(first 32 bytes) (lines 53, 68-69). The two calls after the loop (lines 95,
101) use masked stores of exactly the produced count. -/
def latin1to8 : List Iter :=
  [ { consumed := 64, stores := [.rel 0 64] },
    { consumed := 64, stores := [.rel 0 64, .rel 32 64] } ]

theorem m_latin1to8 : optimalMargin 64 false latin1to8 = 32 := by decide
theorem ok_latin1to8 : latin1to8.all (Iter.ok (need 64 32 false)) = true := by decide

theorem safe_latin1to8 (choose : List Nat → Iter)
    (hc : ∀ xs, need 64 32 false ≤ xs.length → choose xs ∈ latin1to8) :
    ∀ n xs, Pos xs → written (need 64 32 false) choose n xs ≤ L xs :=
  safe_of_paths _ latin1to8 ok_latin1to8 choose hc
/-- m = 31: 0x80 then 94 ASCII (95 bytes, L = 96); the second 64-byte store
starts at L(first 32) = 33 and ends at 97. -/
theorem wit_latin1to8 :
    let it : Iter := { consumed := 64, stores := [.rel 0 64, .rel 32 64] }
    let xs := [2] ++ ascii 94
    it ∈ latin1to8 ∧ xs.length = need 64 31 false ∧
    written (need 64 31 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## Latin-1 to UTF-16, Latin-1 to UTF-32, endianness swap: exact stores -/

/-- src/icelake/icelake_convert_latin1_to_utf16.inl.cpp:11,
`i < (len & ~0x1F)`: one store of 32 `char16_t` for 32 bytes (line 20);
masked tail (line 32). -/
def latin1to16 : List Iter := [{ consumed := 32, stores := [.rel 0 32] }]
theorem m_latin1to16 : optimalMargin 32 false latin1to16 = 0 := by decide
theorem cur_latin1to16 : latin1to16.all (Iter.ok (need 32 0 false)) = true := by decide

/-- src/icelake/icelake_convert_latin1_to_utf32.inl.cpp:3, `len >= 16`: one
store of 16 `char32_t` (line 12); masked tail (line 22). -/
def latin1to32 : List Iter := [{ consumed := 16, stores := [.rel 0 16] }]
theorem m_latin1to32 : optimalMargin 16 false latin1to32 = 0 := by decide
theorem cur_latin1to32 : latin1to32.all (Iter.ok (need 16 0 false)) = true := by decide

/-- src/icelake/implementation.cpp:1615 (`change_endianness_utf16`),
`pos + 32 <= length`: one store of 32 units (line 1618); masked tail. -/
def swap16 : List Iter := [{ consumed := 32, stores := [.rel 0 32] }]
theorem m_swap16 : optimalMargin 32 false swap16 = 0 := by decide
theorem cur_swap16 : swap16.all (Iter.ok (need 32 0 false)) = true := by decide

/-- src/icelake/icelake_utf16fix.cpp:122 and :128, `i + 32 < n` (strict):
`utf16fix_block` stores 32 units at `out + i` (lines 47-55) and possibly one
unit at `out + i - 1` (line 42), inside what was already written. The final
block is at `out + n - 32` (lines 126, 132). Same length in and out. -/
def utf16fix : List Iter := [{ consumed := 32, stores := [.rel 0 32] }]
theorem m_utf16fix : optimalMargin 32 true utf16fix = 0 := by decide
theorem cur_utf16fix : utf16fix.all (Iter.ok (need 32 0 true)) = true := by decide

/-! ## UTF-16 / UTF-32 to Latin-1 -/

/-- src/icelake/icelake_convert_utf16_to_latin1.inl.cpp:15 and :60
(`_with_errors`), `end - buf >= 32`: one 32-byte store (lines 23, 75), or an
error return; `_with_errors` first copies the units before the offending one
one at a time (line 67). Masked tail. -/
def utf16to1 : List Iter := [{ consumed := 32, stores := [.rel 0 32] }] ++ scalar 32
theorem m_utf16to1 : optimalMargin 32 false utf16to1 = 0 := by decide
theorem cur_utf16to1 : utf16to1.all (Iter.ok (need 32 0 false)) = true := by decide

/-- src/icelake/icelake_convert_utf32_to_latin1.inl.cpp:10 and :44,
`end - buf >= 16`: one 16-byte store (lines 15, 53) or an error return. -/
def utf32to1 : List Iter := [{ consumed := 16, stores := [.rel 0 16] }] ++ scalar 16
theorem m_utf32to1 : optimalMargin 16 false utf32to1 = 0 := by decide
theorem cur_utf32to1 : utf32to1.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## UTF-32 to UTF-8 -/

/-- src/icelake/icelake_convert_utf32_to_utf8.inl.cpp:21 and :306
(`_with_errors`), `end - buf >= 16 + 12`. The haswell code: ASCII path one
16-byte store (`continue`), 1/2-byte path two 16-byte stores at L of 0 and 8
units (`pack_1_2_utf8_bytes`), 1/2/3-byte path four 16-byte stores at L of 0,
4, 8, 12 units (`pack_1_2_3_utf8_bytes`), else scalar for 15 units. No
half-block fall-through. This is `Kernels.wide16`. -/
def utf32to8 : List Iter := wide16
theorem m_utf32to8 : optimalMargin 16 false utf32to8 = 12 := by decide
theorem cur_utf32to8 : utf32to8.all (Iter.ok (need 16 12 false)) = true := by decide
/-- m = 11: 12 x U+0800, 4 ASCII, 11 ASCII (27 units, L = 51); the fourth
store ends at 52. -/
theorem wit_utf32to8 :
    let it : Iter := { consumed := 16, stores := [.rel 0 16, .rel 4 16, .rel 8 16, .rel 12 16] }
    let xs := List.replicate 12 3 ++ ascii 4 ++ ascii 11
    it ∈ utf32to8 ∧ xs.length = need 16 11 false ∧
    written (need 16 11 false) (fun _ => it) 1 xs = L xs + 1 := by decide

/-! ## UTF-32 to UTF-16 -/

/-- src/icelake/icelake_convert_utf32_to_utf16.inl.cpp:19 and :174
(`_with_errors`), `end - buf >= 16`. BMP path: 16 units for 16 (lines 37, 200).
Otherwise a masked store of `popcnt(output_mask)` units (lines 77, 244), where
`output_mask = ~pdep(saturation, 0xAAAAAAAA)` has bit `2i` set and bit `2i+1`
set iff unit `i` exceeds 0xFFFF: exactly `utf16_length_from_utf32` of the 16
units (`tail 0`). Errors: the plain function returns before storing (line 51);
`_with_errors` stores the units before the error (lines 194, 222) and returns. -/
def utf32to16 : List Iter :=
  [ { consumed := 16, stores := [.rel 0 16] },
    { consumed := 16, stores := [.tail 0] },
    { consumed := 16, stores := [.tail 0], stop := true },
    { consumed := 0, stores := [], stop := true } ]
theorem m_utf32to16 : optimalMargin 16 false utf32to16 = 0 := by decide
theorem cur_utf32to16 : utf32to16.all (Iter.ok (need 16 0 false)) = true := by decide

/-! ## UTF-16 to UTF-32 (length function has minimum 0)

src/icelake/icelake_convert_utf16_to_utf32.inl.cpp:144, `end - buf >= 32`.
`utf32_length_from_utf16` gives 0 to a low surrogate, so `Pos` fails.
* No surrogate (lines 249-254): 32 units in, 32 out, exact.
* Surrogates with `V = L ^ (carry | H << 1) == 0` (line 165): 31 consumed;
  an unmasked 16-unit store at the cursor (line 227), then a store masked to
  the produced count (line 234). The output is exact: a high surrogate in lane
  30 is paired with lane 31, which is consumed (and weighs 0) next time.
* `V != 0`: return without storing.

`vzero_noAdj`: `V == 0` forbids two adjacent low lanes. `utf16to32_store`:
then the 16-unit store ends within the UTF-32 length of the 32 loaded lanes,
which is `L` of the 31 consumed units plus the next one (`ahead`). -/

/-- `true` marks a low surrogate lane. -/
def lows : List Bool → Nat
  | [] => 0
  | b :: bs => (if b then 1 else 0) + lows bs

/-- `utf32_length_from_utf16` of a list of lanes: the non-low lanes. -/
def w32 (bs : List Bool) : Nat := bs.length - lows bs

def noAdj : List Bool → Bool
  | true :: true :: _ => false
  | _ :: bs => noAdj bs
  | [] => true

theorem lows_le_length (bs : List Bool) : lows bs ≤ bs.length := by
  induction bs with
  | nil => simp [lows]
  | cons b bs ih => cases b <;> simp [lows] <;> omega

theorem lows_noAdj (bs : List Bool) (h : noAdj bs = true) :
    2 * lows bs ≤ bs.length + 1 ∧ (bs.head? ≠ some true → 2 * lows bs ≤ bs.length) := by
  induction bs with
  | nil => simp [lows]
  | cons b bs ih =>
    cases b with
    | false =>
      have h' : noAdj bs = true := by
        cases bs with
        | nil => rfl
        | cons c cs => simpa [noAdj] using h
      have := (ih h').1
      simp [lows]
      omega
    | true =>
      cases bs with
      | nil => simp [lows]
      | cons c cs =>
        cases c with
        | true => simp [noAdj] at h
        | false =>
          have h' : noAdj (false :: cs) = true := by simpa [noAdj] using h
          have := (ih h').2 (by simp)
          simp [lows] at this ⊢
          omega

theorem noAdj_append_left (a b : List Bool) (h : noAdj (a ++ b) = true) : noAdj a = true := by
  induction a with
  | nil => rfl
  | cons x xs ih =>
    cases xs with
    | nil => cases x <;> rfl
    | cons y ys =>
      cases x <;> cases y
      · exact ih (by simpa [noAdj] using h)
      · exact ih (by simpa [noAdj] using h)
      · exact ih (by simpa [noAdj] using h)
      · simp [noAdj] at h

theorem noAdj_append_right (a b : List Bool) (h : noAdj (a ++ b) = true) : noAdj b = true := by
  induction a with
  | nil => simpa using h
  | cons x xs ih =>
    apply ih
    cases xs with
    | nil =>
      cases b with
      | nil => rfl
      | cons y ys => cases x <;> cases y <;> simp_all [noAdj]
    | cons y ys => cases x <;> cases y <;> simp_all [noAdj]

/-- The unmasked 16-unit store of the surrogate path fits: the low lanes among
the first 16 never outnumber the non-low lanes among the next 16. -/
theorem utf16to32_store (a b : List Bool) (ha : a.length = 16) (hb : b.length = 16)
    (h : noAdj (a ++ b) = true) : 16 ≤ w32 a + w32 b := by
  have h1 := (lows_noAdj a (noAdj_append_left a b h)).1
  have h2 := (lows_noAdj b (noAdj_append_right a b h)).1
  have h3 := lows_le_length a
  have h4 := lows_le_length b
  unfold w32
  omega

/-- Tight: 16 (high, low) pairs, exactly 32 units. `L = 16` and the first store
ends at 16, the last unit of the buffer. -/
theorem utf16to32_tight :
    let a := (List.replicate 8 [false, true]).flatten
    noAdj (a ++ a) = true ∧ a.length = 16 ∧ w32 a + w32 a = 16 := by decide

/-- `V == 0`: lane 0 is low exactly when `carry`, lane `i + 1` exactly when
lane `i` is high, and no lane is both. -/
def vzero : Bool → List Bool → List Bool → Bool
  | c, h :: hs, l :: ls => (l == c) && !(h && l) && vzero h hs ls
  | _, [], [] => true
  | _, _, _ => false

theorem vzero_noAdj (c : Bool) (hs ls : List Bool) (h : vzero c hs ls = true) :
    noAdj ls = true := by
  induction hs generalizing c ls with
  | nil => cases ls <;> simp_all [vzero, noAdj]
  | cons h0 hs ih =>
    cases ls with
    | nil => simp [vzero] at h
    | cons l0 ls =>
      simp only [vzero, Bool.and_eq_true, beq_iff_eq, Bool.not_eq_true'] at h
      obtain ⟨⟨hl0, hboth⟩, hrest⟩ := h
      have ih' := ih h0 ls hrest
      cases ls with
      | nil => cases l0 <;> rfl
      | cons l1 ls' =>
        cases hs with
        | nil => simp [vzero] at hrest
        | cons h1 hs' =>
          simp only [vzero, Bool.and_eq_true, beq_iff_eq] at hrest
          cases l0 <;> cases l1 <;> simp_all [noAdj]

theorem L_lanes (bs : List Bool) :
    L (bs.map fun b => if b then 0 else 1) = w32 bs := by
  induction bs with
  | nil => simp [w32, lows]
  | cons b bs ih =>
    have := lows_le_length bs
    cases b <;> simp [w32, lows] at ih ⊢ <;> omega

theorem lows_append (a b : List Bool) : lows (a ++ b) = lows a + lows b := by
  induction a with
  | nil => simp [lows]
  | cons x xs ih => simp only [List.cons_append, lows, ih]; omega

/-- The 16-unit store of the surrogate path, with weights: under `V == 0`,
it ends within `L` of the 32 loaded lanes. -/
theorem utf16to32_ahead (c : Bool) (hs a b : List Bool) (ha : a.length = 16)
    (hb : b.length = 16) (hv : vzero c hs (a ++ b) = true) :
    16 ≤ L ((a ++ b).map fun x => if x then 0 else 1) := by
  rw [L_lanes]
  have := utf16to32_store a b ha hb (vzero_noAdj c hs _ hv)
  unfold w32 at *
  have h1 := lows_le_length a
  have h2 := lows_le_length b
  rw [lows_append]
  simp
  omega

def utf16to32 : List Iter :=
  [ { consumed := 32, stores := [.tail 0] },
    { consumed := 31, stores := [.ahead, .tail 0] },
    { consumed := 0, stores := [], stop := true } ]

theorem m_utf16to32 : optimalMargin 32 false utf16to32 = 0 := by decide

/-- The model end of `.ahead` is `L` of the 32 loaded lanes. This bounds that
model on every input. The unmasked store writes 16 units
(`_mm512_storeu_si512` of `compressed_first`); `safe_utf16to32_real` is what
puts those 16 inside the model end. -/
theorem safe_utf16to32 (choose : List Nat → Iter)
    (hc : ∀ xs, 32 ≤ xs.length → choose xs ∈ utf16to32) :
    ∀ n xs, written 32 choose n xs ≤ L xs :=
  safe_fits 32 choose fun xs h s hs => by
    have hm := hc xs h
    simp only [utf16to32, List.mem_cons, List.not_mem_nil, or_false] at hm
    rcases hm with hm | hm | hm <;> rw [hm] at hs <;>
      simp only [List.mem_cons, List.not_mem_nil, or_false] at hs
    · subst hs; simpa [storeEnd, hm] using L_take_le xs 32
    · rcases hs with rfl | rfl
      · exact L_take_le xs _
      · simpa [storeEnd, hm] using L_take_le xs 31

theorem L_take_succ_le (xs : List Nat) (n : Nat) :
    L (xs.take n) ≤ L (xs.take (n + 1)) := by
  have h : xs.take (n + 1) = xs.take n ++ (xs.drop n).take 1 := by
    rw [List.take_add]
  rw [h, L_append]
  exact Nat.le_add_right _ _

theorem L_take_mono (xs : List Nat) {a b : Nat} (h : a ≤ b) :
    L (xs.take a) ≤ L (xs.take b) := by
  induction b generalizing a with
  | zero =>
    have : a = 0 := by omega
    subst this
    exact Nat.le_refl _
  | succ b ih =>
    by_cases hab : a = b + 1
    · subst hab
      exact Nat.le_refl _
    · exact Nat.le_trans (ih (by omega)) (L_take_succ_le xs b)

/-- Real end of the surrogate iteration, from its output cursor: the unmasked
16-store, and the masked store which ends at `L` of the 31 consumed lanes. -/
def surrogateEnd (xs : List Nat) : Nat := max 16 (L (xs.take 31))

theorem surrogateEnd_le (xs : List Nat) (h : 16 ≤ L (xs.take 32)) :
    surrogateEnd xs ≤ L (xs.take 32) := by
  unfold surrogateEnd
  exact Nat.max_le.mpr ⟨h, L_take_mono xs (by omega)⟩

theorem take_all (xs ys : List Nat) : (xs ++ ys).take xs.length = xs := by
  rw [List.take_append_of_le_length (Nat.le_refl _)]
  simp

/-- `V == 0` on the 32 loaded lanes (`vzero`) is the condition of the unmasked
store. Lane weight 0 is a low surrogate, which is `utf32_length_from_utf16`. -/
theorem hsur_of_vzero (c : Bool) (hs a b : List Bool) (rest : List Nat)
    (ha : a.length = 16) (hb : b.length = 16)
    (hv : vzero c hs (a ++ b) = true) :
    16 ≤ L ((((a ++ b).map fun (x : Bool) => if x then 0 else 1) ++ rest).take 32) := by
  have hlen : ((a ++ b).map fun (x : Bool) => if x then 0 else 1).length = 32 := by
    rw [List.length_map, List.length_append, ha, hb]
  have h := utf16to32_ahead c hs a b ha hb hv
  have htake := take_all ((a ++ b).map fun (x : Bool) => if x then 0 else 1) rest
  rw [hlen] at htake
  rw [htake]
  exact h

def realWritten32 (choose : List Nat → Iter) : Nat → List Nat → Nat
  | 0, xs => L xs
  | n + 1, xs =>
    if xs.length < 32 then
      L xs
    else if (choose xs).stop then
      max (if (choose xs).consumed = 31 then surrogateEnd xs
           else maxEnd xs (choose xs).consumed (choose xs).stores)
        (L (xs.take (choose xs).consumed))
    else
      max (if (choose xs).consumed = 31 then surrogateEnd xs
           else maxEnd xs (choose xs).consumed (choose xs).stores)
        (L (xs.take (choose xs).consumed) +
          realWritten32 choose n (xs.drop (choose xs).consumed))

theorem utf16to32_here (xs : List Nat) (it : Iter)
    (hmem : it ∈ utf16to32) (h16 : it.consumed = 31 → 16 ≤ L (xs.take 32)) :
    (if it.consumed = 31 then surrogateEnd xs else maxEnd xs it.consumed it.stores) ≤
      L xs := by
  simp only [utf16to32, List.mem_cons, List.not_mem_nil, or_false] at hmem
  rcases hmem with rfl | rfl | rfl
  · have hne : ¬ (32 = 31) := by decide
    rw [ite_eq_right hne]
    simp [maxEnd, storeEnd]
    exact L_take_le xs 32
  · have heq : 31 = 31 := rfl
    rw [ite_eq_left heq]
    exact Nat.le_trans (surrogateEnd_le xs (h16 rfl)) (L_take_le xs 32)
  · have hne : ¬ (0 = 31) := by decide
    rw [ite_eq_right hne]
    simp [maxEnd]

/-- The surrogate iteration's real stores, across every later iteration.
`hsur` is `hsur_of_vzero` when the kernel takes that iteration only for
`V == 0`. The no-surrogate iteration stores 32 and consumes 32. The error
iteration stores nothing. -/
theorem safe_utf16to32_real (choose : List Nat → Iter)
    (hc : ∀ xs, 32 ≤ xs.length → choose xs ∈ utf16to32)
    (hsur : ∀ xs, 32 ≤ xs.length →
      (choose xs).consumed = 31 → 16 ≤ L (xs.take 32)) :
    ∀ n xs, realWritten32 choose n xs ≤ L xs := by
  intro n
  induction n with
  | zero =>
    intro xs
    simp [realWritten32]
  | succ n ih =>
    intro xs
    by_cases hshort : xs.length < 32
    · unfold realWritten32
      rw [ite_eq_left hshort]
      exact Nat.le_refl _
    · have hlen : 32 ≤ xs.length := Nat.le_of_not_lt hshort
      have hhere := utf16to32_here xs (choose xs) (hc xs hlen) (hsur xs hlen)
      unfold realWritten32
      rw [ite_eq_right hshort]
      by_cases hstop : (choose xs).stop
      · rw [ite_eq_left hstop]
        exact Nat.max_le.mpr ⟨hhere, L_take_le xs _⟩
      · rw [ite_eq_right hstop]
        have hrest := ih (xs.drop (choose xs).consumed)
        have hsum := L_take_add_drop (choose xs).consumed xs
        exact Nat.max_le.mpr ⟨hhere, by omega⟩

/-! ## UTF-16 to UTF-8: one unit of look-ahead

src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:23, `while (inlen >= 32)`:
32 units loaded, 31 consumed (`inmask = 0x7fffffff`), so `B = 31`. Every
store is masked to its produced count (lines 38, 66, 183, 185), but two
effects put its end past `L` of the 31 consumed units, both bounded by the
weight of lane 31, which the loop test guarantees is present (`ahead`):

* carry: a high surrogate in lane 30 emits all 4 bytes. Its partner in lane
  31 is checked by `(h + h + carry) ^ l` (else `failiteration`), weighs 2,
  and emits nothing in the next iteration (`outmask` excludes `losurr`), so
  the next iteration's output is lower by the same 2 (`ahead_carry`).
* spill in the 1/2-byte path (lines 60-69): lane 31 is outside `inmask`, so
  its bytes are compared with 0xFF and not converted; a byte equal to 0xFF is
  kept by `smoosh` and stored past the count (`ahead_spill`).

The two never meet: the 1/2-byte path needs every lane below 0x800, so it
neither carries out nor starts on a carried-in low surrogate. After a
mismatch (`failiteration`) the iteration keeps the lanes before the error,
stores exactly and leaves the loop. -/

/-- `utf8_length_from_utf16` of one unit. -/
def w8 (v : Nat) : Nat :=
  if v < 0x80 then 1 else if v < 0x800 then 2 else if 0xD800 ≤ v ∧ v < 0xE000 then 2 else 3

/-- Bytes of lane 31 kept by `smoosh` in the 1/2-byte path. -/
def spill (v : Nat) : Nat :=
  (if v % 256 = 255 then 1 else 0) + (if v / 256 = 255 then 1 else 0)

theorem spill_lt (v : Nat) (hv : v < 65536) : spill v < w8 v := by
  unfold spill w8
  repeat' split
  all_goals omega

theorem ahead_spill (pre : List Nat) (v : Nat) (hv : v < 65536) :
    L pre + spill v ≤ L (pre ++ [w8 v]) := by
  have := spill_lt v hv
  rw [L_append]
  simp
  omega

theorem ahead_carry (pre : List Nat) (v : Nat) (hv : 0xDC00 ≤ v ∧ v < 0xE000) :
    L pre + 2 ≤ L (pre ++ [w8 v]) := by
  rw [L_append]
  simp only [L_cons, L_nil]
  unfold w8
  repeat' split
  all_goals omega

def utf16to8 : List Iter :=
  { consumed := 31, stores := [.ahead] } ::
    (List.range 32).map fun k => ({ consumed := k, stores := [.tail 0], stop := true } : Iter)

theorem m_utf16to8 : optimalMargin 31 false utf16to8 = 1 := by decide
theorem cur_utf16to8 : utf16to8.all (Iter.ok (need 31 1 false)) = true := by decide

theorem safe_utf16to8 (choose : List Nat → Iter)
    (hc : ∀ xs, need 31 1 false ≤ xs.length → choose xs ∈ utf16to8) :
    ∀ n xs, Pos xs → written (need 31 1 false) choose n xs ≤ L xs :=
  safe_of_paths _ utf16to8 cur_utf16to8 choose hc

/-- The bytes written past `L` of the 31 consumed units. ASCII stores 31 and
advances 31 (`extra = 0`). The 1/2-byte path keeps at most `spill` bytes of
lane 31. A matched surrogate emits the low surrogate's 2 bytes early. Each
ends at or before `L` of the 32 loaded lanes, which is `storeEnd` of `.ahead`. -/
theorem utf16to8_real_le_ahead (pre : List Nat) (v extra : Nat) (hv : v < 65536)
    (h : extra ≤ spill v ∨ (0xDC00 ≤ v ∧ v < 0xE000 ∧ extra ≤ 2) ∨ extra = 0) :
    L pre + extra ≤ L (pre ++ [w8 v]) := by
  rcases h with h | ⟨h1, h2, h3⟩ | h0
  · exact Nat.le_trans (Nat.add_le_add_left h _) (ahead_spill pre v hv)
  · exact Nat.le_trans (Nat.add_le_add_left h3 _) (ahead_carry pre v ⟨h1, h2⟩)
  · rw [h0, Nat.add_zero, L_append, L_cons, L_nil]
    omega

theorem utf16to8_extra_le_ahead (xs : List Nat) (v extra : Nat) (hv : v < 65536)
    (htake : xs.take 32 = xs.take 31 ++ [w8 v])
    (h : extra ≤ spill v ∨ (0xDC00 ≤ v ∧ v < 0xE000 ∧ extra ≤ 2) ∨ extra = 0) :
    L (xs.take 31) + extra ≤ storeEnd xs 31 .ahead := by
  have h1 := utf16to8_real_le_ahead (xs.take 31) v extra hv h
  simpa [storeEnd, htake] using h1

def ahead8 : Iter := { consumed := 31, stores := [.ahead] }

/-- Real end of one iteration: the `.ahead` iteration writes `extra` past
`L` of its 31 units; every stop iteration is `tail 0`. -/
def end8 (xs : List Nat) (it : Iter) (extra : Nat) : Nat :=
  if it = ahead8 then L (xs.take 31) + extra else maxEnd xs it.consumed it.stores

def realWritten8 (choose : List Nat → Iter) (extra : List Nat → Nat) :
    Nat → List Nat → Nat
  | 0, xs => L xs
  | n + 1, xs =>
    if xs.length < need 31 1 false then
      L xs
    else if (choose xs).stop then
      max (end8 xs (choose xs) (extra xs)) (L (xs.take (choose xs).consumed))
    else
      max (end8 xs (choose xs) (extra xs))
        (L (xs.take (choose xs).consumed) +
          realWritten8 choose extra n (xs.drop (choose xs).consumed))

theorem end8_le (xs : List Nat) (it : Iter) (extra : Nat)
    (hmem : it ∈ utf16to8)
    (hreal : it = ahead8 → ∃ v, v < 65536 ∧ xs.take 32 = xs.take 31 ++ [w8 v] ∧
        (extra ≤ spill v ∨ (0xDC00 ≤ v ∧ v < 0xE000 ∧ extra ≤ 2) ∨ extra = 0)) :
    end8 xs it extra ≤ L xs := by
  by_cases ha : it = ahead8
  · rw [end8, ite_eq_left ha]
    obtain ⟨v, hv, htake, hdisj⟩ := hreal ha
    exact Nat.le_trans (utf16to8_extra_le_ahead xs v extra hv htake hdisj) (L_take_le xs _)
  · rw [end8, ite_eq_right ha]
    simp only [utf16to8, List.mem_cons] at hmem
    rcases hmem with rfl | hmem
    · exact absurd rfl ha
    · obtain ⟨k, _, rfl⟩ := List.mem_map.mp hmem
      simp [maxEnd, storeEnd]
      exact L_take_le xs k

/-- `safe_utf16to8` bounds the model end `.ahead`. This bounds the real end,
once each `.ahead` iteration's `extra` is one of the three C++ effects. -/
theorem safe_utf16to8_real (choose : List Nat → Iter) (extra : List Nat → Nat)
    (hc : ∀ xs, need 31 1 false ≤ xs.length → choose xs ∈ utf16to8)
    (hreal : ∀ xs, need 31 1 false ≤ xs.length →
      choose xs = ahead8 → ∃ v, v < 65536 ∧ xs.take 32 = xs.take 31 ++ [w8 v] ∧
        (extra xs ≤ spill v ∨ (0xDC00 ≤ v ∧ v < 0xE000 ∧ extra xs ≤ 2) ∨
          extra xs = 0)) :
    ∀ n xs, realWritten8 choose extra n xs ≤ L xs := by
  intro n
  induction n with
  | zero =>
    intro xs
    simp [realWritten8]
  | succ n ih =>
    intro xs
    by_cases hshort : xs.length < need 31 1 false
    · unfold realWritten8
      rw [ite_eq_left hshort]
      exact Nat.le_refl _
    · have hlen : need 31 1 false ≤ xs.length := Nat.le_of_not_lt hshort
      have hhere := end8_le xs (choose xs) (extra xs) (hc xs hlen)
        (by intro heq; exact hreal xs hlen heq)
      unfold realWritten8
      rw [ite_eq_right hshort]
      by_cases hstop : (choose xs).stop
      · rw [ite_eq_left hstop]
        exact Nat.max_le.mpr ⟨hhere, L_take_le xs _⟩
      · rw [ite_eq_right hstop]
        have hrest := ih (xs.drop (choose xs).consumed)
        have hsum := L_take_add_drop (choose xs).consumed xs
        exact Nat.max_le.mpr ⟨hhere, by omega⟩

/-! ## UTF-8 sources: exact counts, no dynamic margin

Ice Lake does not include the generic `utf8_to_utf16.h` / `utf8_to_utf32.h` /
`utf8_to_latin1.h` headers, so there is no `K`. Every store is masked to the
number of code units it produces.

* `process_block_utf8_to_utf16` (src/icelake/icelake_utf8_common.inl.cpp:24,
  driven by src/icelake/icelake_convert_utf8_to_utf16.inl.cpp:21 and :50,
  `final_in - in >= 64` for a full block, masked load otherwise). With
  `validate = true` every store path runs after the block passed the structure
  check (`mc` against `m1234`, lines 146-157, 324-335, 376-387), which forces
  byte 0 to be a leading byte and every continuation byte to belong to a lead
  in the block. Then the produced count is the number of character ends in the
  consumed bytes, which is exactly `utf16_length_from_utf8` of them. `tail 0`.
* `validating_utf8_to_fixed_length` (src/icelake/icelake_from_utf8.inl.cpp:32),
  `_with_constant_checks` (:160) and `valid_utf8_to_fixed_length`
  (src/icelake/icelake_from_valid_utf8.inl.cpp:45): each 16-byte lane emits
  `count_ones(leading_bytes)`, the number of non-continuation bytes of that
  lane (`expand_and_identify`, icelake_utf8_common.inl.cpp:734), stored masked
  (`SIMDUTF_ICELAKE_WRITE_UTF16_OR_UTF32(..., true)`). That is exactly
  `utf32_length_from_utf8` of the lane, for every input, valid or not, and
  even though the validating version only checks errors after the loop.
  The `64 + 4` loop bound is for the 4-byte read at `ptr + 64`.
* `utf8_to_latin1_avx512` (src/icelake/icelake_convert_utf8_to_latin1.inl.cpp:70):
  a validated block stores `ascii + continuation` bytes, which is `L(block)`
  minus a lead in byte 63 plus a lead carried in. `latin1_lag` shows the
  cursor is `L(prefix) - e` with `e` the pending lead, so every store ends at
  or before `L(prefix)`. `tail 0`.

So every UTF-8 loop is exact stores or an error return. -/

def utf8 : List Iter :=
  [ { consumed := 64, stores := [.tail 0] },
    { consumed := 0, stores := [], stop := true } ]
theorem exact_utf8 : utf8.all Iter.exact = true := by decide

theorem safe_utf8 (choose : List Nat → Iter)
    (hc : ∀ xs, 64 ≤ xs.length → choose xs ∈ utf8) :
    ∀ n xs, written 64 choose n xs ≤ L xs :=
  safe_of_exact 64 utf8 exact_utf8 choose hc

/-- One block of the Latin-1 kernel: `a` = L of the block, `e` = lead in byte
63 (0 or 1), `w` = bytes stored. A validated block satisfies
`w + e = a + eIn`, `eIn` the lead carried in. -/
def latin1L : List (Nat × Nat × Nat) → Nat
  | [] => 0
  | (a, _, _) :: bs => a + latin1L bs

def latin1Valid : List (Nat × Nat × Nat) → Nat → Prop
  | [], _ => True
  | (a, e, w) :: bs, eIn => w + e = a + eIn ∧ latin1Valid bs e

/-- Cursor after the blocks, and the lead pending after the last one. -/
def latin1Run : List (Nat × Nat × Nat) → Nat → Nat × Nat
  | [], eIn => (0, eIn)
  | (_, e, w) :: bs, _ =>
    let r := latin1Run bs e
    (w + r.1, r.2)

theorem latin1_lag (bs : List (Nat × Nat × Nat)) (eIn : Nat) (h : latin1Valid bs eIn) :
    (latin1Run bs eIn).1 + (latin1Run bs eIn).2 = latin1L bs + eIn := by
  induction bs generalizing eIn with
  | nil => simp [latin1Run, latin1L]
  | cons blk bs ih =>
    obtain ⟨a, e, w⟩ := blk
    simp only [latin1Valid] at h
    have := ih e h.2
    simp only [latin1Run, latin1L]
    omega

end SafetyMargins.Arch.Icelake
