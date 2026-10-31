import SafetyMargins.Validator
import SafetyMargins.Utf8Table
/-
On input that passed `check_utf8_bytes` the decoders' output is exact: every
call in a checked block produces exactly `L` of what it consumes and ends on
a code-point boundary. Then the cursor equals `L` of the consumed input at the
end of the main loop, which `convert_ascii_tail` needs: its last store starts
before the cursor.
-/
namespace SafetyMargins.Exact
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.Validator SafetyMargins.Tables
  SafetyMargins.TableFacts

/-- The block at `pos` as `check_utf8_bytes` sees it: three zero bytes, then
the 64 bytes. -/
def blk (bs : List Nat) (pos i : Nat) : Nat := if i < 3 then 0 else bs.getD (pos + i - 3) 0

/-- The block at `pos` is inside the input and passed the check. -/
def CheckedAt (xm : Bool) (bs : List Nat) (pos : Nat) : Prop :=
  (∀ b ∈ bs, b < 256) ∧ pos + 64 ≤ bs.length ∧ Checked xm (blk bs pos) 67

theorem getD_lt (bs : List Nat) (hb : ∀ b ∈ bs, b < 256) (i : Nat) : bs.getD i 0 < 256 := by
  rw [List.getD_eq_getElem?_getD]
  cases h : bs[i]? with
  | none => simp
  | some x => simpa using hb x (List.mem_of_getElem? h)

/-- Four consecutive continuation bytes never pass. -/
theorem no4 {xm bs pos} (h : CheckedAt xm bs pos) (u : Nat) (hu : pos ≤ u) (hu4 : u + 3 < pos + 64) :
    isCont (bs.getD u 0) = false ∨ isCont (bs.getD (u + 1) 0) = false ∨
      isCont (bs.getD (u + 2) 0) = false ∨ isCont (bs.getD (u + 3) 0) = false := by
  have A := (h.2.2.at (q := u + 3 - pos + 3) (by omega) (by omega)).facts.1
  have e : ∀ k, k ≤ 3 → blk bs pos (u + 3 - pos + 3 - k) = bs.getD (u + 3 - k) 0 := by
    intro k hk
    unfold blk
    rw [ite_eq_right_iff.mpr (fun h => absurd h (by omega))]
    congr 1; omega
  have e0 := e 0 (by omega); have e1 := e 1 (by omega)
  have e2 := e 2 (by omega); have e3 := e 3 (by omega)
  simp only [Nat.sub_zero] at e0
  rw [e1, e0, e3, e2] at A
  simp only [show u + 3 - 1 = u + 2 by omega, show u + 3 - 3 = u by omega,
    show u + 3 - 2 = u + 1 by omega] at A
  have b1 := getD_lt bs h.1 u; have b2 := getD_lt bs h.1 (u + 1)
  by_cases c0 : isCont (bs.getD u 0) = false
  · exact Or.inl c0
  by_cases c1 : isCont (bs.getD (u + 1) 0) = false
  · exact Or.inr (Or.inl c1)
  by_cases c2 : isCont (bs.getD (u + 2) 0) = false
  · exact Or.inr (Or.inr (Or.inl c2))
  by_cases c3 : isCont (bs.getD (u + 3) 0) = false
  · exact Or.inr (Or.inr (Or.inr c3))
  simp only [Bool.not_eq_false] at c0 c1 c2 c3
  rw [c2, c3] at A
  simp [isCont] at c0 c1
  unfold must23 at A
  cases xm <;> simp at A <;> omega

/-- Weight functions whose zero is exactly a continuation byte. -/
def ContZero (w : Nat → Nat) : Prop := ∀ b, b < 256 → (w b = 0 ↔ isCont b = true)

theorem lead_map {w : Nat → Nat} (hw : ContZero w) {bs : List Nat} (hb : ∀ b ∈ bs, b < 256)
    (i : Nat) (hi : i < bs.length) : lead (bs.map w) i = if isCont (bs.getD i 0) then 0 else 1 := by
  have hl := getD_lt bs hb i
  have hg : (bs.map w).getD i 0 = w (bs.getD i 0) := by
    simp [List.getD_eq_getElem?_getD, List.getElem?_map, List.getElem?_eq_getElem hi]
  unfold lead
  rw [hg]
  have := hw _ hl
  generalize bs.getD i 0 = b at *
  by_cases hc : isCont b = true
  · rw [if_pos (this.mpr hc), if_pos hc]
  · have h0 : w b ≠ 0 := fun h0 => hc (this.mp h0)
    rw [if_neg h0, if_neg hc]

/-- The mask of a call inside a checked block is well shaped. -/
theorem well_call {xm w bs pos t} (hw : ContZero w) (h : CheckedAt xm bs pos) (ht1 : pos ≤ t)
    (ht2 : t < pos + 52) : wellShaped (emask (bs.map w) t 12) = true := by
  unfold wellShaped
  apply List.all_eq_true.mpr
  intro j hj
  have hj9 := List.mem_range.mp hj
  have hb := fun k (hk : k < 12) => bit_emask (bs.map w) t 12 k hk
  rw [hb j (by omega), hb (j + 1) (by omega), hb (j + 2) (by omega), hb (j + 3) (by omega)]
  have hlen := h.2.1
  rw [lead_map hw h.1 _ (by omega), lead_map hw h.1 _ (by omega), lead_map hw h.1 _ (by omega),
    lead_map hw h.1 _ (by omega)]
  have := no4 h (t + j + 1) (by omega) (by omega)
  simp only [show t + j + 1 + 1 = t + (j + 1) + 1 by omega, show t + j + 1 + 2 = t + (j + 2) + 1 by omega,
    show t + j + 1 + 3 = t + (j + 3) + 1 by omega] at this
  generalize isCont (bs.getD (t + j + 1) 0) = c0 at *
  generalize isCont (bs.getD (t + (j + 1) + 1) 0) = c1 at *
  generalize isCont (bs.getD (t + (j + 2) + 1) 0) = c2 at *
  generalize isCont (bs.getD (t + (j + 3) + 1) 0) = c3 at *
  revert this
  cases c0 <;> cases c1 <;> cases c2 <;> cases c3 <;> decide

theorem sumUpTo_shift (f : Nat → Nat) (t c : Nat) :
    sumUpTo (fun k => f (t + k)) (c + 1) = f t + sumUpTo (fun k => f (t + 1 + k)) c := by
  induction c with
  | zero => simp [sumUpTo]
  | succ c ih =>
    simp only [sumUpTo] at ih ⊢
    rw [ih, show t + (c + 1) = t + 1 + c by omega]
    omega

/-- A table row of an aligned call whose consumed code points each weigh 1
produces exactly `L` of what it consumes. -/
theorem row_exact (xs : List Nat) (t : Nat) (r : Row xs t) (hi : bigIdx[r.m]! < 209)
    (hlead : lead xs t = 1)
    (hw1 : ∀ k, k < bigConsumed[r.m]! → xs.getD (t + k) 0 = lead xs (t + k)) :
    units bigIdx[r.m]! = L ((xs.drop t).take bigConsumed[r.m]!) := by
  obtain ⟨hc1, hle, hnext, -⟩ := row_conv xs t r hi
  have hrow := (List.all_eq_true.mp bigindex_exact) r.m r.hmem
  simp only [hi, ↓reduceIte, Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq] at hrow
  obtain ⟨-, hpop⟩ := hrow
  generalize bigConsumed[r.m]! = c at *
  have hb := r.hb
  obtain ⟨c', rfl⟩ : ∃ c', c = c' + 1 := ⟨c - 1, by omega⟩
  rw [← hpop, popcount_eq r.m (c' + 1) (fun k => lead xs (t + 1 + k))
    (fun k hk => by rw [hb k (by omega), Nat.add_right_comm]), L_window,
    sumUpTo_congr _ (fun k => lead xs (t + k)) _ hw1, sumUpTo_shift (lead xs) t c']
  have hn : lead xs (t + 1 + c') = 1 := by
    unfold lead; rw [show t + 1 + c' = t + (c' + 1) by omega, if_neg hnext]
  show lead xs (t + 1 + c') + sumUpTo (fun k => lead xs (t + 1 + k)) c' =
    lead xs t + sumUpTo (fun k => lead xs (t + 1 + k)) c'
  rw [hn, hlead]

/-- An exact call: `c` bytes consumed, the next one leading, the output
advanced by exactly `L` of the consumed bytes. -/
def XStep (xs : List Nat) (t c p : Nat) : Prop :=
  1 ≤ c ∧ xs.getD (t + c) 0 ≠ 0 ∧ p = L ((xs.drop t).take c)

/-- UTF-32 and Latin-1 (`w01`): in a checked block, every table call is exact
and never takes an error row. -/
def w01 (b : Nat) : Nat := if isCont b then 0 else 1

theorem w01_contZero : ContZero w01 := by
  intro b _; unfold w01; split <;> simp_all

theorem exact_w01 {xm bs pos t} (h : CheckedAt xm bs pos) (ht1 : pos ≤ t) (ht2 : t < pos + 52)
    (hlead : isCont (bs.getD t 0) = false) :
    let r := row (bs.map w01) t
    bigIdx[r.m]! < 209 ∧ XStep (bs.map w01) t bigConsumed[r.m]! (units bigIdx[r.m]!) := by
  intro r
  have hwell := well_call w01_contZero h ht1 ht2
  have hi : bigIdx[r.m]! < 209 := by
    have := (List.all_eq_true.mp well_noerr) r.m r.hmem
    simp only [Bool.or_eq_true, Bool.not_eq_true', decide_eq_true_eq] at this
    rcases this with hw | hw
    · rw [r.hm, hwell] at hw; exact absurd hw (by decide)
    · exact hw
  obtain ⟨hc1, hle, hnext, -⟩ := row_conv _ t r hi
  refine ⟨hi, hc1, hnext, ?_⟩
  have hlen := h.2.1
  apply row_exact _ t r hi
  · rw [lead_map w01_contZero h.1 t (by omega), hlead]; rfl
  · intro k hk
    rw [lead_map w01_contZero h.1 _ (by omega)]
    simp [List.getD_eq_getElem?_getD, List.getElem?_map, List.getElem?_eq_getElem (show t + k < bs.length by omega), w01]

theorem getD_map (w : Nat → Nat) (bs : List Nat) (i : Nat) (hi : i < bs.length) :
    (bs.map w).getD i 0 = w (bs.getD i 0) := by
  simp [List.getD_eq_getElem?_getD, List.getElem?_map, List.getElem?_eq_getElem hi]

theorem w16_contZero : ContZero w16 := by
  intro b _; unfold w16 isCont; split <;> (try split) <;> simp_all <;> omega

/-- In a checked block, a byte `≥ 0xF0` at `u` is followed by continuation
bytes up to the block end (at most three). -/
theorem f0_conts {xm bs pos u i} (h : CheckedAt xm bs pos) (hu : pos ≤ u)
    (hF : 0xF0 ≤ bs.getD u 0) (hi1 : 1 ≤ i) (hi3 : i ≤ 3) (hin : u + i < pos + 64) :
    isCont (bs.getD (u + i) 0) = true := by
  have hb := getD_lt bs h.1 u
  have e : ∀ j, blk bs pos (u - pos + 3 + j) = bs.getD (u + j) 0 := by
    intro j; unfold blk; rw [if_neg (by omega)]; congr 1; omega
  have e0 : blk bs pos (u - pos + 3) = bs.getD u 0 := e 0
  generalize hB : bs.getD u 0 = B at hF hb e0
  have := cont_of_lead (k := i) h.2.2 (s := u - pos + 3) (by omega)
    (by rw [e0]; unfold isCont; simp; omega) (by rw [e0]; omega) hi1
    (by rw [e0]; unfold clen; rw [if_neg (by omega), if_neg (by omega), if_neg (by omega)]; omega)
    (by omega)
  rwa [e i] at this

/-- UTF-16, rows below 145: in a checked block every such call is exact (no
4-byte character is consumed, so every code point weighs 1). -/
theorem exact_utf16_145 {xm bs pos t} (h : CheckedAt xm bs pos) (ht1 : pos ≤ t) (ht2 : t < pos + 52)
    (hlead : isCont (bs.getD t 0) = false)
    (hi : bigIdx[(row (bs.map w16) t).m]! < 145) :
    XStep (bs.map w16) t bigConsumed[(row (bs.map w16) t).m]! (units bigIdx[(row (bs.map w16) t).m]!) := by
  have hwell := well_call w16_contZero h ht1 ht2
  have hrm : (row (bs.map w16) t).m = emask (bs.map w16) t 12 := rfl
  generalize row (bs.map w16) t = r at *
  have hlen := h.2.1
  obtain ⟨hc1, hle, hnext, -⟩ := row_conv _ t r (by omega)
  refine ⟨hc1, hnext, ?_⟩
  apply row_exact _ t r (by omega)
  · rw [lead_map w16_contZero h.1 t (by omega), hlead]; rfl
  · intro k hk
    have hbk := getD_lt bs h.1 (t + k)
    rw [lead_map w16_contZero h.1 _ (by omega), getD_map _ _ _ (by omega)]
    by_cases hF : 0xF0 ≤ bs.getD (t + k) 0
    · exfalso
      have hcont : ∀ i, 1 ≤ i → i ≤ 3 → t + k + i ≤ t + bigConsumed[r.m]! →
          isCont (bs.getD (t + k + i) 0) = true :=
        fun i h1 h3 hin => f0_conts h (by omega) hF h1 h3 (by omega)
      have hnext' : isCont (bs.getD (t + bigConsumed[r.m]!) 0) = false := by
        rw [getD_map _ _ _ (by omega)] at hnext
        have := w16_contZero _ (getD_lt bs h.1 (t + bigConsumed[r.m]!))
        cases hc : isCont (bs.getD (t + bigConsumed[r.m]!) 0)
        · rfl
        · exact absurd (this.mpr hc) hnext
      by_cases hfar : t + k + 3 < t + bigConsumed[r.m]!
      · have h145 := (List.all_eq_true.mp well_145) r.m r.hmem
        have hwell' : wellShaped r.m = true := hrm ▸ hwell
        simp only [hwell', Bool.not_true, Bool.false_or] at h145
        simp only [hi, decide_true, Bool.not_true, Bool.false_or, List.all_eq_true,
          List.mem_range] at h145
        have := h145 k hk
        have hb : ∀ j, j < 12 → bit r.m j = lead (bs.map w16) (t + j + 1) := r.hb
        have e0 := hb k (by omega); have e1 := hb (k + 1) (by omega); have e2 := hb (k + 2) (by omega)
        rw [lead_map w16_contZero h.1 _ (by omega)] at e0 e1 e2
        rw [show t + k + 1 = t + k + 1 from rfl, hcont 1 (by omega) (by omega) (by omega)] at e0
        rw [show t + (k + 1) + 1 = t + k + 2 by omega, hcont 2 (by omega) (by omega) (by omega)] at e1
        rw [show t + (k + 2) + 1 = t + k + 3 by omega, hcont 3 (by omega) (by omega) (by omega)] at e2
        have hprev : k = 0 ∨ bit r.m (k - 1) = 1 := by
          rcases Nat.eq_zero_or_pos k with hk0 | hk0
          · exact Or.inl hk0
          · right
            have hnc : isCont (bs.getD (t + k) 0) = false := by
              generalize bs.getD (t + k) 0 = b at hF hbk
              unfold isCont; simp only [Bool.and_eq_false_iff, decide_eq_false_iff_not]; omega
            rw [hb (k - 1) (by omega), lead_map w16_contZero h.1 _ (by omega),
              show t + (k - 1) + 1 = t + k by omega, hnc]
            rfl
        have b0 : bit r.m k = 0 := by rw [e0]; rfl
        have b1 : bit r.m (k + 1) = 0 := by rw [e1]; rfl
        have b2 : bit r.m (k + 2) = 0 := by rw [e2]; rfl
        rw [b0, b1, b2] at this
        have hk2 : k + 2 < 12 := by omega
        rcases hprev with hp | hp
        · subst hp; simp [hk2] at this
        · rw [hp] at this; simp [hk2] at this
      · have := hcont (t + bigConsumed[r.m]! - (t + k)) (by omega) (by omega) (by omega)
        rw [show t + k + (t + bigConsumed[r.m]! - (t + k)) = t + bigConsumed[r.m]! by omega] at this
        rw [this] at hnext'; exact absurd hnext' (by decide)
    · generalize bs.getD (t + k) 0 = b at hF hbk ⊢
      unfold w16 isCont
      by_cases hc : 0x80 ≤ b ∧ b < 0xC0
      · simp [hc]
      · have : ¬ (decide (0x80 ≤ b) && decide (b < 0xC0)) = true := by simp; omega
        simp only [hc, if_false, this]; rw [if_neg hF]; rfl

theorem sumUpTo_add (f g : Nat → Nat) (c : Nat) :
    sumUpTo (fun k => f k + g k) c = sumUpTo f c + sumUpTo g c := by
  induction c with
  | zero => rfl
  | succ c ih => simp only [sumUpTo, ih]; omega

theorem sumUpTo_zero (c : Nat) : sumUpTo (fun _ => 0) c = 0 := by
  induction c with
  | zero => rfl
  | succ c ih => simp only [sumUpTo]; omega

theorem sum_ind (f : Nat → Nat) (a c : Nat) :
    sumUpTo (fun k => if k = a then f k else 0) c = if a < c then f a else 0 := by
  induction c with
  | zero => simp [sumUpTo]
  | succ c ih =>
    simp only [sumUpTo, ih]
    by_cases h : c = a
    · subst h; simp
    · by_cases h2 : a < c
      · simp [h, h2, show a < c + 1 by omega]
      · simp [h, h2, show ¬ a < c + 1 by omega]

theorem sum_zero (f : Nat → Nat) (c : Nat) (h : sumUpTo f c = 0) : ∀ k, k < c → f k = 0 := by
  induction c with
  | zero => intro k hk; omega
  | succ c ih =>
    simp only [sumUpTo] at h
    intro k hk
    by_cases hkc : k = c
    · subst hkc; omega
    · exact ih (by omega) k (by omega)

/-- Split a sum at three distinct indices. -/
theorem decomp3 (f : Nat → Nat) (a b d c : Nat) (hab : a ≠ b) (had : a ≠ d) (hbd : b ≠ d)
    (ha : a < c) (hb : b < c) (hd : d < c) :
    sumUpTo f c = f a + f b + f d +
      sumUpTo (fun k => if k ≠ a ∧ k ≠ b ∧ k ≠ d then f k else 0) c := by
  have hpt : ∀ k, f k = ((if k = a then f k else 0) + (if k = b then f k else 0)) +
      (if k = d then f k else 0) + (if k ≠ a ∧ k ≠ b ∧ k ≠ d then f k else 0) := by
    intro k
    by_cases h1 : k = a
    · subst h1; simp [hab, had]
    · by_cases h2 : k = b
      · subst h2; simp [h1, hbd]
      · by_cases h3 : k = d
        · subst h3; simp [h1, h2]
        · simp [h1, h2, h3]
  rw [sumUpTo_congr f _ c (fun k _ => hpt k), sumUpTo_add, sumUpTo_add, sumUpTo_add,
    sum_ind, sum_ind, sum_ind]
  simp [ha, hb, hd]

theorem blk_at (bs : List Nat) (pos u j : Nat) (hu : pos ≤ u) :
    blk bs pos (u - pos + 3 + j) = bs.getD (u + j) 0 := by
  unfold blk; rw [if_neg (by omega)]; congr 1; omega

/-- `char_shape` at input position `u`. -/
theorem shape_at {xm bs pos u} (h : CheckedAt xm bs pos) (hu : pos ≤ u)
    (hlead : isCont (bs.getD u 0) = false) (hend : u + clen (bs.getD u 0) < pos + 64) :
    isCont (bs.getD (u + clen (bs.getD u 0)) 0) = false ∧
    (0xF0 ≤ bs.getD u 0 → bs.getD u 0 ≤ 0xF4 ∧ (bs.getD u 0 = 0xF0 → 0x90 ≤ bs.getD (u + 1) 0)) ∧
    (∀ k, 1 ≤ k → k < clen (bs.getD u 0) → isCont (bs.getD (u + k) 0) = true) := by
  have e0 : blk bs pos (u - pos + 3) = bs.getD u 0 := blk_at bs pos u 0 hu
  have := char_shape h.2.2 (s := u - pos + 3) (by omega) (by rw [e0]; exact hlead)
    (by rw [e0]; omega)
  rw [e0, blk_at bs pos u _ hu, blk_at bs pos u 1 hu] at this
  refine ⟨this.2.1, this.2.2, fun k h1 h2 => ?_⟩
  have := this.1 k h1 h2
  rwa [blk_at bs pos u k hu] at this

/-- `cont_of_lead` at input position `u`. -/
theorem conts_at {xm bs pos u k} (h : CheckedAt xm bs pos) (hu : pos ≤ u)
    (hlead : isCont (bs.getD u 0) = false) (h80 : 0x80 ≤ bs.getD u 0) (hk1 : 1 ≤ k)
    (hk : k < clen (bs.getD u 0)) (hn : u + k < pos + 64) : isCont (bs.getD (u + k) 0) = true := by
  have e0 : blk bs pos (u - pos + 3) = bs.getD u 0 := blk_at bs pos u 0 hu
  have := cont_of_lead (k := k) h.2.2 (s := u - pos + 3) (by omega) (by rw [e0]; exact hlead)
    (by rw [e0]; exact h80) hk1 (by rw [e0]; exact hk) (by omega)
  rwa [blk_at bs pos u k hu] at this

/-- Byte `e` of the shuffle source, zero for an index of 16 or more. -/
def byteAt (bs : List Nat) (t e : Nat) : Nat := if e < 16 then bs.getD (t + e) 0 else 0

/-- Lane `j` of `perm` on row `i` (little-endian 32-bit lanes). -/
def perm (bs : List Nat) (t i j : Nat) : Nat :=
  byteAt bs t shuf[16 * i + 4 * j]! + 256 * byteAt bs t shuf[16 * i + 4 * j + 1]! +
    65536 * byteAt bs t shuf[16 * i + 4 * j + 2]! + 16777216 * byteAt bs t shuf[16 * i + 4 * j + 3]!

/-- The pair test: arm64/lsx/lasx (`arm = true`) or westmere/haswell/ppc64. -/
def pairTest (arm : Bool) (p : Nat) : Bool :=
  if arm then p &&& 0xf8000000 == 0xf0000000 else decide (0x3c00000 < composed p)

/-- Both pair tests fire on every valid 4-byte character. -/
theorem pair4 :
    (List.range 5).all (fun a => (List.range 64).all fun b => (List.range 64).all fun c =>
      (List.range 64).all fun d =>
        let P := (0x80 + d) + 256 * (0x80 + c) + 65536 * (0x80 + b) + 16777216 * (0xF0 + a)
        (a == 0 && decide (b < 0x10)) || (pairTest false P && pairTest true P)) = true := by
  native_decide

theorem pair_of_valid4 (arm : Bool) (b0 b1 b2 b3 : Nat) (h0 : 0xF0 ≤ b0) (h0' : b0 ≤ 0xF4)
    (h1 : 0x80 ≤ b1) (h1' : b1 < 0xC0) (h2 : 0x80 ≤ b2) (h2' : b2 < 0xC0)
    (h3 : 0x80 ≤ b3) (h3' : b3 < 0xC0) (hF0 : b0 = 0xF0 → 0x90 ≤ b1) :
    pairTest arm (b3 + 256 * b2 + 65536 * b1 + 16777216 * b0) = true := by
  have := List.all_eq_true.mp (List.all_eq_true.mp (List.all_eq_true.mp
    (List.all_eq_true.mp pair4 (b0 - 0xF0) (List.mem_range.mpr (by omega)))
    (b1 - 0x80) (List.mem_range.mpr (by omega))) (b2 - 0x80) (List.mem_range.mpr (by omega)))
    (b3 - 0x80) (List.mem_range.mpr (by omega))
  simp only [show 0x80 + (b3 - 0x80) = b3 by omega, show 0x80 + (b2 - 0x80) = b2 by omega,
    show 0x80 + (b1 - 0x80) = b1 by omega, show 0xF0 + (b0 - 0xF0) = b0 by omega] at this
  have hnot : (b0 - 0xF0 == 0 && decide (b1 - 0x80 < 0x10)) = false := by
    by_cases hb : b0 = 0xF0
    · have := hF0 hb; simp; omega
    · simp; omega
  rw [hnot] at this
  simp only [Bool.false_or, Bool.and_eq_true] at this
  cases arm
  · exact this.1
  · exact this.2

theorem no_pair_top0 (arm : Bool) (b0 b1 b2 : Nat) (h0 : b0 < 256) (h1 : b1 < 256) (h2 : b2 < 256) :
    pairTest arm (b0 + 256 * b1 + 65536 * b2 + 16777216 * 0) = false := by
  have hlt : (b0 + 256 * b1 + 65536 * b2 + 16777216 * 0) >>> 24 = 0 := by
    rw [Nat.shiftRight_eq_div_pow]; omega
  cases arm
  · simp only [pairTest, Bool.false_eq_true, ↓reduceIte, decide_eq_false_iff_not]
    intro hc
    have := pair_needs_f0 _ hc
    omega
  · simp only [pairTest, ↓reduceIte, beq_eq_false_iff_ne, ne_eq]
    intro hc
    have := pair_needs_f0_arm _ hc
    omega

theorem cont_range (b : Nat) (h : isCont b = true) : 0x80 ≤ b ∧ b < 0xC0 := by
  unfold isCont at h; simpa using h

theorem w16_split (b : Nat) :
    w16 b = (if isCont b then 0 else 1) + (if 0xF0 ≤ b then 1 else 0) := by
  unfold w16 isCont
  by_cases hc : 0x80 ≤ b ∧ b < 0xC0
  · have : ¬ 0xF0 ≤ b := by omega
    simp [hc, this]
  · have h2 : (decide (0x80 ≤ b) && decide (b < 0xC0)) = false := by simp; omega
    simp only [hc, if_false, h2]
    by_cases hF : 0xF0 ≤ b <;> simp [hF]

/-- UTF-16, rows with three code points: in a checked block the call is exact
with either pair test. A lane writes a pair exactly for a 4-byte character. -/
theorem exact_utf16_3 {xm bs pos t} (arm : Bool) (h : CheckedAt xm bs pos) (ht1 : pos ≤ t)
    (ht2 : t < pos + 52) (hlead : isCont (bs.getD t 0) = false)
    (hi1 : 145 ≤ bigIdx[(row (bs.map w16) t).m]!) (hi2 : bigIdx[(row (bs.map w16) t).m]! < 209) :
    XStep (bs.map w16) t bigConsumed[(row (bs.map w16) t).m]!
      ((if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 0) then 2 else 1) +
       (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 1) then 2 else 1) +
       (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 2) then 2 else 1)) := by
  have hwell := well_call w16_contZero h ht1 ht2
  have hrm : (row (bs.map w16) t).m = emask (bs.map w16) t 12 := rfl
  generalize row (bs.map w16) t = r at *
  have hwell' : wellShaped r.m = true := hrm ▸ hwell
  have hlen := h.2.1
  obtain ⟨hc1, hle, hnext, -⟩ := row_conv _ t r (by omega)
  have hS := (List.all_eq_true.mp shuf3_starts) r.m r.hmem
  have hW := (List.all_eq_true.mp well_3) r.m r.hmem
  have hP := (List.all_eq_true.mp bigindex_exact) r.m r.hmem
  generalize hI : bigIdx[r.m]! = i at *
  generalize hC : bigConsumed[r.m]! = c at *
  simp only [hi1, hi2, decide_true, Bool.and_self, Bool.not_true, Bool.false_or,
    Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq, List.all_eq_true, List.mem_range] at hS
  obtain ⟨⟨⟨⟨⟨hb1, hb2⟩, h01⟩, h12⟩, h2c⟩, -⟩ := hS
  simp only [hwell', hi1, hi2, decide_true, Bool.and_self, Bool.not_true, Bool.false_or,
    List.all_eq_true, List.mem_range] at hW
  simp only [hi2, ↓reduceIte, Bool.and_eq_true, beq_iff_eq, decide_eq_true_eq] at hP
  obtain ⟨-, hpop⟩ := hP
  have hu3 : units i = 3 := by unfold units; rw [if_neg (by omega), if_neg (by omega)]
  refine ⟨hc1, hnext, ?_⟩
  -- leads of the consumed bytes
  have hb := r.hb
  let g := fun k => lead (bs.map w16) (t + k)
  have hg : ∀ k, k ≤ c → g k = if isCont (bs.getD (t + k) 0) then 0 else 1 :=
    fun k hk => lead_map w16_contZero h.1 _ (by omega)
  have g0 : g 0 = 1 := by rw [hg 0 (by omega), Nat.add_zero, hlead]; rfl
  have gs : ∀ s, 0 < s → s ≤ c → bit r.m (s - 1) = 1 → g s = 1 := by
    intro s hs0 hsc hbit
    have := hb (s - 1) (by omega)
    rw [hbit, show t + (s - 1) + 1 = t + s by omega] at this
    exact this.symm
  have gc : g c = 1 := by
    show lead _ _ = 1
    unfold lead; rw [if_neg hnext]
  have hsum3 : sumUpTo g c = 3 := by
    obtain ⟨c', rfl⟩ : ∃ c', c = c' + 1 := ⟨c - 1, by omega⟩
    have e := popcount_eq r.m (c' + 1) (fun k => lead (bs.map w16) (t + 1 + k))
      (fun k hk => by rw [hb k (by omega), Nat.add_right_comm])
    rw [hpop, hu3] at e
    have hsh := sumUpTo_shift (lead (bs.map w16)) t c'
    have gc' : lead (bs.map w16) (t + 1 + c') = 1 := by
      rw [show t + 1 + c' = t + (c' + 1) by omega]; exact gc
    simp only [sumUpTo] at e
    show sumUpTo (fun k => lead (bs.map w16) (t + k)) (c' + 1) = 3
    rw [hsh]
    have : lead (bs.map w16) t = 1 := g0
    omega
  have g1 := gs _ h01 (by omega) hb1
  have g2 := gs _ (by omega) (by omega) hb2
  -- no other lead in the consumed bytes
  have hdec := decomp3 g 0 (start3 i 1) (start3 i 2) c (by omega) (by omega) (by omega)
    (by omega) (by omega) h2c
  have g0' : g 0 = 1 := g0
  rw [hsum3, g0', g1, g2] at hdec
  have hrest := sum_zero (fun k => if k ≠ 0 ∧ k ≠ start3 i 1 ∧ k ≠ start3 i 2 then g k else 0) c
    (by omega)
  have hother : ∀ k, k < c → k ≠ 0 → k ≠ start3 i 1 → k ≠ start3 i 2 → g k = 0 := by
    intro k hk h0 h1 h2
    have := hrest k hk
    rw [if_pos ⟨h0, h1, h2⟩] at this
    exact this
  -- the weight of the consumed bytes
  let F := fun k => if 0xF0 ≤ bs.getD (t + k) 0 then 1 else 0
  have hFle : ∀ k, k < c → F k ≤ g k := by
    intro k hk
    show (if 0xF0 ≤ bs.getD (t + k) 0 then 1 else 0) ≤ g k
    rw [hg k (by omega)]
    have hbk := getD_lt bs h.1 (t + k)
    generalize bs.getD (t + k) 0 = b at hbk ⊢
    by_cases hF : 0xF0 ≤ b
    · have : isCont b = false := by unfold isCont; simp; omega
      rw [if_pos hF, this]; simp
    · rw [if_neg hF]; exact Nat.zero_le _
  have hL : L (((bs.map w16).drop t).take c) = sumUpTo g c + sumUpTo F c := by
    rw [L_window, ← sumUpTo_add]
    apply sumUpTo_congr
    intro k hk
    rw [getD_map _ _ _ (by omega), w16_split, hg k (by omega)]
  have hdecF := decomp3 F 0 (start3 i 1) (start3 i 2) c (by omega) (by omega) (by omega)
    (by omega) (by omega) h2c
  have hrestF : sumUpTo (fun k => if k ≠ 0 ∧ k ≠ start3 i 1 ∧ k ≠ start3 i 2 then F k else 0) c = 0 := by
    have := sumUpTo_mono (fun k => if k ≠ 0 ∧ k ≠ start3 i 1 ∧ k ≠ start3 i 2 then F k else 0)
      (fun _ => 0) c (fun k hk => by
        show (if k ≠ 0 ∧ k ≠ start3 i 1 ∧ k ≠ start3 i 2 then F k else 0) ≤ 0
        split
        · rename_i hh
          have := hFle k hk
          rw [hother k hk hh.1 hh.2.1 hh.2.2] at this
          exact this
        · exact Nat.le_refl _)
    have h0 := sumUpTo_zero c
    omega
  rw [hrestF, Nat.add_zero] at hdecF
  rw [hL, hsum3, hdecF]
  -- each lane: a pair exactly for a 4-byte character
  have lane : ∀ j, j < 3 → F (start3 i j) = if pairTest arm (perm bs t i j) then 1 else 0 := by
    intro j hj
    have hsj : start3 i j < c ∧ (start3 i j = 0 ∨ g (start3 i j) = 1) := by
      rcases (by omega : j = 0 ∨ j = 1 ∨ j = 2) with rfl | rfl | rfl
      · exact ⟨by simp [start3]; omega, Or.inl (by simp [start3])⟩
      · exact ⟨by omega, Or.inr g1⟩
      · exact ⟨by omega, Or.inr g2⟩
    have hend : start3 i j < end3 i c j ∧ end3 i c j ≤ c ∧ g (end3 i c j) = 1 ∧
        ∀ k, start3 i j < k → k < end3 i c j → g k = 0 := by
      rcases (by omega : j = 0 ∨ j = 1 ∨ j = 2) with rfl | rfl | rfl
      · simp only [end3, show (0 : Nat) < 2 by omega, ↓reduceIte, Nat.reduceAdd, show start3 i 0 = 0 by simp [start3]]
        exact ⟨h01, by omega, g1, fun k hk1 hk2 => hother k (by omega) (by omega) (by omega) (by omega)⟩
      · simp only [end3, show (1 : Nat) < 2 by omega, ↓reduceIte, Nat.reduceAdd]
        exact ⟨h12, by omega, g2, fun k hk1 hk2 => hother k (by omega) (by omega) (by omega) (by omega)⟩
      · simp only [end3, show ¬ (2 : Nat) < 2 by omega, ↓reduceIte]
        exact ⟨h2c, Nat.le_refl _, gc, fun k hk1 hk2 => hother k (by omega) (by omega) (by omega) (by omega)⟩
    obtain ⟨hse, hec, gend, hbetween⟩ := hend
    have hW' := hW j hj
    generalize start3 i j = s at hsj hse hbetween hW' ⊢
    generalize end3 i c j = e at hse hec gend hbetween hW'
    -- the lane's lead byte
    have hls : isCont (bs.getD (t + s) 0) = false := by
      rcases hsj.2 with h0 | h1
      · subst h0; simpa using hlead
      · rw [hg s (by omega)] at h1
        cases hc : isCont (bs.getD (t + s) 0)
        · rfl
        · rw [hc] at h1; exact absurd h1 (by decide)
    have hbs := getD_lt bs h.1 (t + s)
    have h80 : bs.getD (t + s) 0 < 0x80 ∨ 0x80 ≤ bs.getD (t + s) 0 := by omega
    have hcl : clen (bs.getD (t + s) 0) ≤ 4 ∧ 1 ≤ clen (bs.getD (t + s) 0) := by
      unfold clen; split <;> (try split) <;> (try split) <;> omega
    -- the character ends exactly at the next lead
    have hge : s + clen (bs.getD (t + s) 0) ≤ e := by
      by_cases hgt : e < s + clen (bs.getD (t + s) 0)
      · exfalso
        have hcl1 : 1 < clen (bs.getD (t + s) 0) := by omega
        have h80' : 0x80 ≤ bs.getD (t + s) 0 := by
          unfold clen at hcl1; split at hcl1 <;> omega
        have := conts_at (k := e - s) h (by omega) hls h80' (by omega) (by omega) (by omega)
        rw [show t + s + (e - s) = t + e by omega] at this
        rw [hg e hec, this] at gend
        exact absurd gend (by decide)
      · omega
    have heq : e = s + clen (bs.getD (t + s) 0) := by
      have hsh := shape_at h (u := t + s) (by omega) hls (by omega)
      by_cases hlt : s + clen (bs.getD (t + s) 0) < e
      · exfalso
        have := hbetween (s + clen (bs.getD (t + s) 0)) (by omega) hlt
        rw [hg _ (by omega), show t + (s + clen (bs.getD (t + s) 0)) = t + s + clen (bs.getD (t + s) 0) by omega,
          hsh.1] at this
        exact absurd this (by decide)
      · omega
    have hsh := shape_at h (u := t + s) (by omega) hls (by omega)
    by_cases hF : 0xF0 ≤ bs.getD (t + s) 0
    · -- a 4-byte character: the lane gathers it and the test fires
      have hc4 : clen (bs.getD (t + s) 0) = 4 := by
        unfold clen; rw [if_neg (by omega), if_neg (by omega), if_neg (by omega)]
      rw [heq, hc4] at hW'
      simp only [↓reduceIte, Bool.and_eq_true, beq_iff_eq] at hW'
      obtain ⟨⟨⟨e0, e1⟩, e2⟩, e3⟩ := hW'
      have hc3 := fun k h1 h2 => hsh.2.2 k h1 (by rw [hc4]; exact h2)
      have c1 := hc3 1 (by omega) (by omega)
      have c2 := hc3 2 (by omega) (by omega)
      have c3 := hc3 3 (by omega) (by omega)
      have hv := hsh.2.1 hF
      unfold perm byteAt
      rw [e0, e1, e2, e3, if_pos (show s + 3 < 16 by omega), if_pos (show s + 2 < 16 by omega),
        if_pos (show s + 1 < 16 by omega), if_pos (show s < 16 by omega),
        show t + (s + 3) = t + s + 3 by omega, show t + (s + 2) = t + s + 2 by omega,
        show t + (s + 1) = t + s + 1 by omega]
      have := pair_of_valid4 arm (bs.getD (t + s) 0) (bs.getD (t + s + 1) 0) (bs.getD (t + s + 2) 0)
        (bs.getD (t + s + 3) 0) hF hv.1
        (cont_range _ c1).1 (cont_range _ c1).2 (cont_range _ c2).1 (cont_range _ c2).2
        (cont_range _ c3).1 (cont_range _ c3).2 hv.2
      rw [this]
      show (if 0xF0 ≤ bs.getD (t + s) 0 then 1 else 0) = 1
      rw [if_pos hF]
    · -- shorter: the top byte is zero and the test fails
      have hc4 : clen (bs.getD (t + s) 0) ≠ 4 := by
        unfold clen; split <;> (try split) <;> (try split) <;> omega
      rw [heq, if_neg (by omega)] at hW'
      simp only [decide_eq_true_eq] at hW'
      have b0 : byteAt bs t shuf[16 * i + 4 * j]! < 256 := by
        unfold byteAt; split
        · exact getD_lt bs h.1 _
        · omega
      have b1 : byteAt bs t shuf[16 * i + 4 * j + 1]! < 256 := by
        unfold byteAt; split
        · exact getD_lt bs h.1 _
        · omega
      have b2 : byteAt bs t shuf[16 * i + 4 * j + 2]! < 256 := by
        unfold byteAt; split
        · exact getD_lt bs h.1 _
        · omega
      have b3 : byteAt bs t shuf[16 * i + 4 * j + 3]! = 0 := by
        unfold byteAt; rw [if_neg (by omega)]
      unfold perm
      rw [b3, no_pair_top0 arm _ _ _ b0 b1 b2]
      show (if 0xF0 ≤ bs.getD (t + s) 0 then 1 else 0) = 0
      rw [if_neg hF]
  have l0 := lane 0 (by omega)
  have l1 := lane 1 (by omega)
  have l2 := lane 2 (by omega)
  have s0 : start3 i 0 = 0 := by simp [start3]
  rw [s0] at l0
  show _ = 3 + (F 0 + F (start3 i 1) + F (start3 i 2))
  rw [l0, l1, l2]
  split <;> split <;> split <;> rfl

/-- One unit per code point is exact when the consumed code points each weigh 1. -/
theorem count_exact (xs : List Nat) (t c : Nat) (hc : 1 ≤ c) (hlead : lead xs t = 1)
    (hnext : xs.getD (t + c) 0 ≠ 0)
    (hw1 : ∀ k, k < c → xs.getD (t + k) 0 = lead xs (t + k)) :
    sumUpTo (fun k => lead xs (t + 1 + k)) c = L ((xs.drop t).take c) := by
  obtain ⟨c', rfl⟩ : ∃ c', c = c' + 1 := ⟨c - 1, by omega⟩
  rw [L_window, sumUpTo_congr _ (fun k => lead xs (t + k)) _ hw1, sumUpTo_shift (lead xs) t c']
  have hn : lead xs (t + 1 + c') = 1 := by
    unfold lead; rw [show t + 1 + c' = t + (c' + 1) by omega, if_neg hnext]
  show lead xs (t + 1 + c') + sumUpTo (fun k => lead xs (t + 1 + k)) c' =
    lead xs t + sumUpTo (fun k => lead xs (t + 1 + k)) c'
  rw [hn, hlead]

/-- No byte `≥ 0xF0` among consumed bytes whose leads are never followed by
three continuation bytes inside them. -/
theorem noF0 {xm bs pos t c} (h : CheckedAt xm bs pos) (ht1 : pos ≤ t) (htc : t + c < pos + 64)
    (hnext : isCont (bs.getD (t + c) 0) = false)
    (hshape : ∀ j, j + 3 < c → isCont (bs.getD (t + j) 0) = false →
      ¬ (isCont (bs.getD (t + j + 1) 0) = true ∧ isCont (bs.getD (t + j + 2) 0) = true ∧
          isCont (bs.getD (t + j + 3) 0) = true)) :
    ∀ k, k < c → bs.getD (t + k) 0 < 0xF0 := by
  intro k hk
  by_cases hF : 0xF0 ≤ bs.getD (t + k) 0
  · exfalso
    have hcont : ∀ i, 1 ≤ i → i ≤ 3 → t + k + i ≤ t + c → isCont (bs.getD (t + k + i) 0) = true :=
      fun i h1 h3 hin => f0_conts h (by omega) hF h1 h3 (by omega)
    by_cases hfar : k + 3 < c
    · apply hshape k hfar
      · generalize bs.getD (t + k) 0 = b at hF ⊢; unfold isCont; simp; omega
      · exact ⟨hcont 1 (by omega) (by omega) (by omega), hcont 2 (by omega) (by omega) (by omega),
          hcont 3 (by omega) (by omega) (by omega)⟩
    · have := hcont (c - k) (by omega) (by omega) (by omega)
      rw [show t + k + (c - k) = t + c by omega, hnext] at this
      exact absurd this (by decide)
  · omega

/-- The mask fast paths (16 or 8 ASCII bytes, `0xaaaa`, `0x924`, ...): a call
that produces one unit per code-point end and whose consumed characters are
at most 3 bytes long is exact. -/
theorem exact_count {xm bs pos t c} (h : CheckedAt xm bs pos) (ht1 : pos ≤ t) (htc : t + c < pos + 64)
    (hc : 1 ≤ c) (hlead : isCont (bs.getD t 0) = false) (hnext : isCont (bs.getD (t + c) 0) = false)
    (hshape : ∀ j, j + 3 < c → isCont (bs.getD (t + j) 0) = false →
      ¬ (isCont (bs.getD (t + j + 1) 0) = true ∧ isCont (bs.getD (t + j + 2) 0) = true ∧
          isCont (bs.getD (t + j + 3) 0) = true)) :
    XStep (bs.map w16) t c (sumUpTo (fun k => lead (bs.map w16) (t + 1 + k)) c) := by
  have hlen := h.2.1
  have hn : (bs.map w16).getD (t + c) 0 ≠ 0 := by
    rw [getD_map _ _ _ (by omega)]
    intro h0
    rw [(w16_contZero _ (getD_lt bs h.1 _)).mp h0] at hnext
    exact absurd hnext (by decide)
  refine ⟨hc, hn, count_exact _ t c hc ?_ hn ?_⟩
  · rw [lead_map w16_contZero h.1 t (by omega), hlead]; rfl
  · intro k hk
    have hlt := noF0 h ht1 htc hnext hshape k hk
    rw [lead_map w16_contZero h.1 _ (by omega), getD_map _ _ _ (by omega), w16_split,
      if_neg (show ¬ 0xF0 ≤ bs.getD (t + k) 0 by omega), Nat.add_zero]

/-- The `0x888` path (arm64, lsx, lasx): in a checked block its lead check
passes, and its 6 units are exactly `L` of the 12 bytes. -/
theorem exact_888 {xm bs pos t} (h : CheckedAt xm bs pos) (ht1 : pos ≤ t) (ht2 : t < pos + 52)
    (hlead : isCont (bs.getD t 0) = false) (hm : emask (bs.map w16) t 12 = 0x888) :
    (∀ j, j < 3 → bs.getD (t + 4 * j) 0 &&& 0xf8 = 0xf0) ∧ XStep (bs.map w16) t 12 6 := by
  have hlen := h.2.1
  have hbit : ∀ k, k < 12 → (if isCont (bs.getD (t + k + 1) 0) then 0 else 1) = bit 0x888 k := by
    intro k hk
    rw [← hm, bit_emask _ t 12 k hk, lead_map w16_contZero h.1 _ (by omega)]
  have cont : ∀ k, k < 12 → bit 0x888 k = 0 → isCont (bs.getD (t + k + 1) 0) = true := by
    intro k hk h0
    have := hbit k hk
    rw [h0] at this
    cases hc : isCont (bs.getD (t + k + 1) 0)
    · rw [hc] at this; exact absurd this (by decide)
    · rfl
  have ld : ∀ k, k < 12 → bit 0x888 k = 1 → isCont (bs.getD (t + k + 1) 0) = false := by
    intro k hk h1
    have := hbit k hk
    rw [h1] at this
    cases hc : isCont (bs.getD (t + k + 1) 0)
    · rfl
    · rw [hc] at this; exact absurd this (by decide)
  -- each of the three characters is 4 bytes, lead in [0xF0, 0xF4]
  have four : ∀ s, s = 0 ∨ s = 4 ∨ s = 8 →
      0xF0 ≤ bs.getD (t + s) 0 ∧ bs.getD (t + s) 0 ≤ 0xF4 := by
    intro s hs
    have hl : isCont (bs.getD (t + s) 0) = false := by
      rcases hs with rfl | rfl | rfl
      · simpa using hlead
      · exact ld 3 (by omega) (by decide)
      · exact ld 7 (by omega) (by decide)
    have hc1 : isCont (bs.getD (t + s + 1) 0) = true := by
      rcases hs with rfl | rfl | rfl
      · simpa using cont 0 (by omega) (by decide)
      · exact cont 4 (by omega) (by decide)
      · exact cont 8 (by omega) (by decide)
    have hc2 : isCont (bs.getD (t + s + 2) 0) = true := by
      rcases hs with rfl | rfl | rfl
      · simpa using cont 1 (by omega) (by decide)
      · exact cont 5 (by omega) (by decide)
      · exact cont 9 (by omega) (by decide)
    have hc3 : isCont (bs.getD (t + s + 3) 0) = true := by
      rcases hs with rfl | rfl | rfl
      · simpa using cont 2 (by omega) (by decide)
      · exact cont 6 (by omega) (by decide)
      · exact cont 10 (by omega) (by decide)
    have hcl : 1 ≤ clen (bs.getD (t + s) 0) ∧ clen (bs.getD (t + s) 0) ≤ 4 := by
      unfold clen; split <;> (try split) <;> (try split) <;> omega
    have hsh := shape_at h (u := t + s) (by omega) hl (by rcases hs with rfl | rfl | rfl <;> omega)
    have h4 : clen (bs.getD (t + s) 0) = 4 := by
      rcases (by omega : clen (bs.getD (t + s) 0) = 1 ∨ clen (bs.getD (t + s) 0) = 2 ∨
        clen (bs.getD (t + s) 0) = 3 ∨ clen (bs.getD (t + s) 0) = 4) with e | e | e | e
      · rw [e, hc1] at hsh; exact absurd hsh.1 (by decide)
      · rw [e, hc2] at hsh; exact absurd hsh.1 (by decide)
      · rw [e, hc3] at hsh; exact absurd hsh.1 (by decide)
      · exact e
    have hF : 0xF0 ≤ bs.getD (t + s) 0 := by
      unfold clen at h4; split at h4 <;> (try split at h4) <;> (try split at h4) <;> omega
    exact ⟨hF, (hsh.2.1 hF).1⟩
  refine ⟨fun j hj => ?_, by omega, ?_, ?_⟩
  · have := four (4 * j) (by omega)
    generalize bs.getD (t + 4 * j) 0 = b at this
    rcases (by omega : b = 0xF0 ∨ b = 0xF1 ∨ b = 0xF2 ∨ b = 0xF3 ∨ b = 0xF4) with
      rfl | rfl | rfl | rfl | rfl <;> decide
  · rw [getD_map _ _ _ (by omega)]
    have := ld 11 (by omega) (by decide)
    intro h0
    rw [(w16_contZero _ (getD_lt bs h.1 _)).mp h0] at this
    exact absurd this (by decide)
  · rw [L_window]
    have wc : ∀ u, 1 ≤ u → u ≤ 11 → bit 0x888 (u - 1) = 0 → (bs.map w16).getD (t + u) 0 = 0 := by
      intro u hu1 hu2 h0
      rw [getD_map _ _ _ (by omega), (w16_contZero _ (getD_lt bs h.1 _)).mpr]
      have := cont (u - 1) (by omega) h0
      rwa [show t + (u - 1) + 1 = t + u by omega] at this
    have wl : ∀ s, s = 0 ∨ s = 4 ∨ s = 8 → (bs.map w16).getD (t + s) 0 = 2 := by
      intro s hs
      have := four s hs
      rw [getD_map _ _ _ (by omega)]
      unfold w16
      rw [if_neg (by omega), if_pos this.1]
    have wl0 : (bs.map w16).getD t 0 = 2 := wl 0 (by omega)
    simp only [sumUpTo, Nat.add_zero]
    rw [wl0, wl 4 (by omega), wl 8 (by omega), wc 1 (by omega) (by omega) (by decide),
      wc 2 (by omega) (by omega) (by decide), wc 3 (by omega) (by omega) (by decide),
      wc 5 (by omega) (by omega) (by decide), wc 6 (by omega) (by omega) (by decide),
      wc 7 (by omega) (by omega) (by decide), wc 9 (by omega) (by omega) (by decide),
      wc 10 (by omega) (by omega) (by decide), wc 11 (by omega) (by omega) (by decide)]

/-! ## The loop on checked blocks, and the ASCII tail -/

/-- The decoder loop when every non-ASCII block passed the check, so every
call is exact (`exact_w01`, `exact_utf16_*`). -/
inductive XReach (xs : List Nat) : Bool → Nat → Nat → Nat → Prop
  | start : XReach xs false 0 0 0
  | ascii (t o : Nat) (h : XReach xs false t t o)
      (hascii : ∀ i, i < 64 → xs.getD (t + i) 0 = 1) :
      XReach xs false (t + 64) (t + 64) (o + 64)
  | enter (t o : Nat) (h : XReach xs false t t o) (hlead : xs.getD t 0 ≠ 0) :
      XReach xs true t t o
  | call (pos t o c p : Nat) (h : XReach xs true pos t o) (ht : t < pos + 52)
      (hs : XStep xs t c p) : XReach xs true pos (t + c) (o + p)
  | leave (pos t o : Nat) (h : XReach xs true pos t o) (ht : pos + 52 ≤ t) :
      XReach xs false t t o

theorem L_ones (xs : List Nat) (t n : Nat) (h : ∀ i, i < n → xs.getD (t + i) 0 = 1) :
    L ((xs.drop t).take n) = n := by
  rw [L_window]
  induction n with
  | zero => rfl
  | succ n ih =>
    simp only [sumUpTo]
    rw [h n (by omega), ih (fun i hi => h i (by omega))]
    omega

/-- The cursor is exactly `L` of the consumed input, and inside a block the
next call starts on a leading byte. -/
theorem xreach_exact (xs : List Nat) :
    ∀ b pos t o, XReach xs b pos t o →
      o = L (xs.take t) ∧ (b = true → pos ≤ t ∧ xs.getD t 0 ≠ 0) := by
  intro b pos t o h
  induction h with
  | start => simp
  | ascii t o h hascii ih =>
    refine ⟨?_, fun hb => by simp at hb⟩
    rw [L_take_add, L_ones xs t 64 hascii, ih.1]
  | enter t o h hlead ih => exact ⟨ih.1, fun _ => ⟨Nat.le_refl _, hlead⟩⟩
  | call pos t o c p h ht hs ih =>
    obtain ⟨hc, hnext, hp⟩ := hs
    refine ⟨by rw [L_take_add, ← ih.1, hp], fun _ => ⟨by have := (ih.2 rfl).1; omega, hnext⟩⟩
  | leave pos t o h ht ih => exact ⟨ih.1, fun hb => by simp at hb⟩

/-- `convert_ascii_tail`, the full 64-byte blocks: an exact store of 64 ASCII
units keeps the cursor at `L`. -/
theorem tail_block (xs : List Nat) (p o : Nat) (ho : o = L (xs.take p))
    (hascii : ∀ i, i < 64 → xs.getD (p + i) 0 = 1) : o + 64 = L (xs.take (p + 64)) := by
  rw [L_take_add, L_ones xs p 64 hascii, ho]

/-- `convert_ascii_tail`, the last store: 64 units written at
`cursor - (pos - (size - 64))`. With the cursor at `L` of the consumed input
and the last 64 bytes ASCII, the store starts inside the buffer and ends at
`L xs`. -/
theorem tail_last (xs : List Nat) (p o : Nat) (ho : o = L (xs.take p))
    (h64 : 64 ≤ xs.length) (hp1 : xs.length - 64 < p) (hp2 : p ≤ xs.length)
    (hascii : ∀ i, xs.length - 64 ≤ i → i < xs.length → xs.getD i 0 = 1) :
    p - (xs.length - 64) ≤ o ∧ o - (p - (xs.length - 64)) + 64 = L xs := by
  have hw := L_ones xs (xs.length - 64) (p - (xs.length - 64))
    (fun i hi => hascii _ (by omega) (by omega))
  have hsplit := L_take_add xs (xs.length - 64) (p - (xs.length - 64))
  rw [show xs.length - 64 + (p - (xs.length - 64)) = p by omega] at hsplit
  have hrest := L_ones xs p (xs.length - p) (fun i hi => hascii _ (by omega) (by omega))
  have hall := L_take_add xs p (xs.length - p)
  rw [show p + (xs.length - p) = xs.length by omega, List.take_length] at hall
  have : (xs.drop p).take (xs.length - p) = xs.drop p := by
    apply List.take_of_length_le; simp
  rw [this] at hrest hall
  constructor <;> omega

/-- **The ASCII tail fits.** If every non-ASCII block the loop converted
passed the check (so each call is exact: `exact_w01`, `exact_utf16_145`,
`exact_utf16_3`, `exact_count`, `exact_888`), then at the end of the loop
the cursor is `L` of the consumed input, and the last, backward, store of
`convert_ascii_tail` (after its exact 64-byte blocks, `tail_block`) starts
inside the buffer and ends at `L xs`. -/
theorem ascii_tail_safe (xs : List Nat) (t o : Nat) (h : XReach xs false t t o)
    (h64 : 64 ≤ xs.length) (hp1 : xs.length - 64 < t) (hp2 : t ≤ xs.length)
    (hascii : ∀ i, xs.length - 64 ≤ i → i < xs.length → xs.getD i 0 = 1) :
    t - (xs.length - 64) ≤ o ∧ o - (t - (xs.length - 64)) + 64 = L xs :=
  tail_last xs t o (xreach_exact xs _ _ _ _ h).1 h64 hp1 hp2 hascii

/-- A `w01` weight is 0 or 1, including past the end of the input. -/
theorem getD_w01 (bs : List Nat) (i : Nat) :
    (bs.map w01).getD i 0 = 0 ∨ (bs.map w01).getD i 0 = 1 := by
  by_cases hi : i < bs.length
  · rw [getD_map w01 bs i hi]
    unfold w01
    split
    · left; rfl
    · right; rfl
  · rw [List.getD_eq_getElem?_getD,
      List.getElem?_eq_none (by rw [List.length_map]; omega)]
    left
    rfl

/-- One call inside a checked block, on unit weights (UTF-32 and Latin-1:
`w01`). `XStep` gives `p = L(consumed) = cnt(consumed)`, the following byte is
a lead, and `s ≤ p + 1 + K` is the `okK` inequality. The cursor is `L` of the
prefix (`xreach_exact`), which is the same invariant `ascii_tail_safe` uses
once the loop has left the block. A UTF-16 4-byte lead weighs 2, so this
theorem does not apply to it; those stores are `reach_count`, `reach_arm_888`
and `reach_lasx_888`. -/
theorem xreach_store (K pos t c p s : Nat) (xs : List Nat) (o : Nat)
    (h : XReach xs true pos t o)
    (hloop : pos + 64 + (tk K xs + 1) ≤ xs.length)
    (hx : XStep xs t c p)
    (h01 : ∀ i, i < c + 1 → xs.getD (t + i) 0 = 0 ∨ xs.getD (t + i) 0 = 1)
    (hs : s ≤ p + 1 + K) (hd : t + c + 1 ≤ pos + 64) :
    o + s ≤ L xs := by
  obtain ⟨_, hnext, hp⟩ := hx
  have hLc : L ((xs.drop t).take c) = cnt ((xs.drop t).take c) := by
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
  have hsplit : (xs.drop t).take (c + 1) =
      (xs.drop t).take c ++ (xs.drop (t + c)).take 1 := by
    rw [List.take_add, List.drop_drop]
  have hone : cnt ((xs.drop (t + c)).take 1) = 1 := by
    rw [cnt_take_one]
    rcases h01 c (by omega) with h0 | h1
    · exact absurd h0 hnext
    · unfold lead
      rw [h1]
      exact ite_eq_right (by decide : ¬ (1 = 0))
  have hbudget : p + 1 ≤ cnt ((xs.drop t).take (c + 1)) := by
    rw [hsplit, cnt_append, hone, ← hLc, ← hp]
    exact Nat.le_refl _
  apply cover_aligned K pos t c o s p xs hloop hd
  · exact Nat.le_of_eq (xreach_exact xs _ _ _ _ h).1
  · exact hbudget
  · exact hs

/-- The same call on `w01` weights. `valid_utf8_to_latin1.h` does not enter
this theorem: its loop does not reject a continuation-first block, and
`XReach.enter` records that test. -/
theorem xreach_store_w01 (K pos t c p s : Nat) (bs : List Nat) (o : Nat)
    (h : XReach (bs.map w01) true pos t o)
    (hloop : pos + 64 + (tk K (bs.map w01) + 1) ≤ (bs.map w01).length)
    (hx : XStep (bs.map w01) t c p)
    (hs : s ≤ p + 1 + K) (hd : t + c + 1 ≤ pos + 64) :
    o + s ≤ L (bs.map w01) :=
  xreach_store K pos t c p s (bs.map w01) o h hloop hx
    (fun i _ => getD_w01 bs (t + i)) hs hd

/-- `okK` at that call: `q.f ≤ p + 1` is the produced-plus-one reading of the
path list, and `XStep` supplies `p`. -/
theorem xreach_okK (K pos t c p : Nat) (bs : List Nat) (o : Nat)
    (ps : List MPath) (q : MPath)
    (h : XReach (bs.map w01) true pos t o)
    (hloop : pos + 64 + (tk K (bs.map w01) + 1) ≤ (bs.map w01).length)
    (hx : XStep (bs.map w01) t c p)
    (hd : t + c + 1 ≤ pos + 64)
    (hK : okK K ps = true) (hq : q ∈ ps) (hf : q.f ≤ p + 1) :
    o + q.s ≤ L (bs.map w01) := by
  have hs : q.s ≤ q.f + K := of_decide_eq_true ((List.all_eq_true.mp hK) q hq)
  exact xreach_store_w01 K pos t c p q.s bs o h hloop hx (by omega) hd

end SafetyMargins.Exact
