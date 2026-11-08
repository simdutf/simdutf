import SafetyMargins.Lengths
import SafetyMargins.ValidatorSound
/-
arm64's `neon_counter` (src/arm64/arm_validate_utf8_with_counts.cpp:13-52)
counts what the generic `validate_utf8_with_counts` counts.

`ValidatorSound.vecV` reads `arm_validate_utf8_with_counts` as the generic
function, on the ground that its `neon_counter` adds, for every non-ASCII
block it accumulates, the continuation bytes (`ValidatorSound.conts`) and the
bytes `≥ 0xF0` (`ValidatorSound.fours`) of the block, which the generic code
adds per block (`ValidatorSound.bc`, `bf`). This file proves that.

* `accumulate_chunk` (:31-38) adds `vcleq_s8(raw, -65)` and `vcgtq_u8(raw,
  0xEF)` (0 or `0xFF`, which is -1 as `int8_t`) into 16 lanes of `int8_t`
  (`% 256`); `accumulate` (:40-51) does it for the four 16-byte chunks of a
  64-byte block, `pending += 4`, and calls `reduce` at `pending >= 124`.
* `reduce` (:22-29) adds `-int(vaddlvq_s8(lanes))`, the negated sum of the
  lanes read as `int8_t` and widened, then clears the lanes and `pending`.

`neon_counts`: after any sequence of 64-byte blocks, each accumulated when
it is not all ASCII (:74-76), and a final `reduce` (:65, :81, :92),
`continuations` and `four_byte` are `Σ bc` and `Σ bf` of the blocks. A lane
has gained at most `pending ≤ 124` since the last reduction, so it holds `-k`
with `k ≤ 124 < 128` and the int8 lanes never wrap (`s8_neg`).
-/
namespace SafetyMargins.LengthsNeon
open SafetyMargins SafetyMargins.ReplacementMore SafetyMargins.RvvExact SafetyMargins.Lengths
open SafetyMargins.ValidatorSound (allAscii conts fours bc bf)
set_option autoImplicit false

/-- An 8-bit lane read as `int8_t`. -/
def s8 (x : Nat) : Int := if x < 128 then (x : Int) else (x : Int) - 256

/-- A lane that gained `-1` `k` times holds `-k`, for `k ≤ 128`. -/
theorem s8_neg (k : Nat) (hk : k ≤ 128) : s8 ((255 * k) % 256) = -(k : Int) := by
  unfold s8
  split <;> omega

/-- The two comparisons of `accumulate_chunk`. -/
def cmpC (b : Nat) : Bool := decide (i8 b ≤ -65)
def cmpF (b : Nat) : Bool := decide (b > 0xEF)

theorem cmp_all : (List.range 256).all (fun b =>
    cmpC b == decide (0x80 ≤ b ∧ b < 0xC0) && cmpF b == decide (0xF0 ≤ b)) = true := by decide +kernel

theorem cmp_eq (b : Nat) (h : b < 256) :
    cmpC b = decide (0x80 ≤ b ∧ b < 0xC0) ∧ cmpF b = decide (0xF0 ≤ b) := by
  have := List.all_eq_true.mp cmp_all b (List.mem_range.mpr h)
  simp only [Bool.and_eq_true, beq_iff_eq] at this
  exact this

structure NC where
  cv : Nat → Nat
  fv : Nat → Nat
  pend : Nat
  cont : Nat
  four : Nat

/-- `reduce` (:22-29). -/
def reduce (s : NC) : NC :=
  ⟨fun _ => 0, fun _ => 0, 0, s.cont + (-(sumTo (fun i => s8 (s.cv i)) 16 : Int)).toNat,
    s.four + (-(sumTo (fun i => s8 (s.fv i)) 16 : Int)).toNat⟩

where
  /-- `sumTo` over `Int`. -/
  sumTo : (Nat → Int) → Nat → Int
    | _, 0 => 0
    | f, k + 1 => sumTo f k + f k

/-- `accumulate` (:40-51) of a 64-byte block: four chunks, lane `i` of chunk `k`
is byte `16 k + i`. -/
def accumulate (blk : List Nat) (s : NC) : NC :=
  let cv := fun i => sumTo (fun k => maskW 256 (cmpC (blk.getD (16 * k + i) 0))) 4
  let fv := fun i => sumTo (fun k => maskW 256 (cmpF (blk.getD (16 * k + i) 0))) 4
  let s' : NC := ⟨fun i => (s.cv i + cv i) % 256, fun i => (s.fv i + fv i) % 256, s.pend + 4, s.cont, s.four⟩
  if s'.pend ≥ 124 then reduce s' else s'

/-- The loop body on one block: accumulate unless it is all ASCII (:74-76). -/
def step (s : NC) (blk : List Nat) : NC := if allAscii blk then s else accumulate blk s

theorem intSum_neg (k : Nat → Nat) (m : Nat) (hk : ∀ i, i < m → k i ≤ 128) :
    reduce.sumTo (fun i => s8 ((255 * k i) % 256)) m = -((sumTo k m : Nat) : Int) := by
  induction m with
  | zero => rfl
  | succ m ih =>
    simp only [reduce.sumTo, sumTo_succ]
    rw [ih (fun i hi => hk i (by omega)), s8_neg _ (hk m (by omega))]
    push_cast; omega

/-- Lane counts: `k i` matches since the last reduction. -/
def Inv (s : NC) (kc kf : Nat → Nat) (C F : Nat) : Prop :=
  s.pend ≤ 120 ∧ s.pend % 4 = 0 ∧ (∀ i, i < 16 → kc i ≤ s.pend ∧ kf i ≤ s.pend ∧
    s.cv i = (255 * kc i) % 256 ∧ s.fv i = (255 * kf i) % 256) ∧
  s.cont + sumTo kc 16 = C ∧ s.four + sumTo kf 16 = F

theorem countP_sum (p : Nat → Bool) (blk : List Nat) :
    blk.countP p = sumTo (fun j => ind (p (g blk j))) blk.length := by
  rw [sum_all (fun b => ind (p b)) blk]
  induction blk with
  | nil => rfl
  | cons b r ih => simp [List.countP_cons, ih, ind]; split <;> omega

/-- The 64 bytes of a block as four chunks of 16 lanes. -/
theorem chunks (blk : List Nat) (hl : blk.length = 64) (p : Nat → Bool) :
    blk.countP p = sumTo (fun i => sumTo (fun k => ind (p (blk.getD (16 * k + i) 0))) 4) 16 := by
  rw [countP_sum, hl]
  have := sum_unrolled (fun j => ind (p (g blk j))) 4 16 0
  simp only [Nat.zero_add] at this
  rw [show (64 : Nat) = 4 * 16 from rfl, ← this]
  refine sumTo_congr _ _ _ (fun i _ => sumTo_congr _ _ _ (fun k _ => ?_))
  unfold g; rw [Nat.mul_comm 16 k]

theorem maskW_ind (W : Nat) (p : Bool) : maskW W p = (W - 1) * ind p := by
  unfold maskW ind; cases p <;> simp

theorem acc_inv (blk : List Nat) (hl : blk.length = 64) (hb : Bytes blk) (s : NC) (kc kf : Nat → Nat)
    (C F : Nat) (h : Inv s kc kf C F) :
    ∃ kc' kf', Inv (accumulate blk s) kc' kf' (C + conts blk) (F + fours blk) := by
  obtain ⟨h1, h0, h2, h3, h4⟩ := h
  let ac := fun i => sumTo (fun k => ind (cmpC (blk.getD (16 * k + i) 0))) 4
  let af := fun i => sumTo (fun k => ind (cmpF (blk.getD (16 * k + i) 0))) 4
  have hac : ∀ i, ac i ≤ 4 := fun i => Nat.le_trans (sumTo_le _ 1 4 (fun _ _ => ind_le _)) (by omega)
  have haf : ∀ i, af i ≤ 4 := fun i => Nat.le_trans (sumTo_le _ 1 4 (fun _ _ => ind_le _)) (by omega)
  have hbyte : ∀ j, blk.getD j 0 < 256 := fun j => by
    have := g_byte hb j; unfold g at this; exact this
  have hconts : conts blk = sumTo ac 16 := by
    unfold conts; rw [chunks blk hl]
    refine sumTo_congr _ _ _ (fun i _ => sumTo_congr _ _ _ (fun k _ => ?_))
    rw [(cmp_eq _ (hbyte _)).1]
  have hfours : fours blk = sumTo af 16 := by
    unfold fours; rw [chunks blk hl]
    refine sumTo_congr _ _ _ (fun i _ => sumTo_congr _ _ _ (fun k _ => ?_))
    rw [(cmp_eq _ (hbyte _)).2]
  -- the lanes after the four chunks
  have hcv : ∀ i, (s.cv i + sumTo (fun k => maskW 256 (cmpC (blk.getD (16 * k + i) 0))) 4) % 256 =
      (255 * (kc i + ac i)) % 256 ∨ ¬ i < 16 := by
    intro i
    by_cases hi : i < 16
    · left
      rw [(h2 i hi).2.2.1, sumTo_congr _ (fun k => 255 * ind (cmpC (blk.getD (16 * k + i) 0))) 4
        (fun k _ => maskW_ind 256 _), sumTo_scale, Nat.mod_add_mod, Nat.mul_add]
    · right; exact hi
  have hfv : ∀ i, (s.fv i + sumTo (fun k => maskW 256 (cmpF (blk.getD (16 * k + i) 0))) 4) % 256 =
      (255 * (kf i + af i)) % 256 ∨ ¬ i < 16 := by
    intro i
    by_cases hi : i < 16
    · left
      rw [(h2 i hi).2.2.2, sumTo_congr _ (fun k => 255 * ind (cmpF (blk.getD (16 * k + i) 0))) 4
        (fun k _ => maskW_ind 256 _), sumTo_scale, Nat.mod_add_mod, Nat.mul_add]
    · right; exact hi
  have sc := sumTo_plus kc ac 16
  have sf := sumTo_plus kf af 16
  unfold accumulate
  simp only
  split
  · -- `reduce`: the lanes hold `-(k + a)` with `k + a ≤ 124`
    rename_i hp
    refine ⟨fun _ => 0, fun _ => 0, ⟨by simp [reduce], by simp [reduce], fun i hi => by simp [reduce], ?_, ?_⟩⟩
    · simp only [reduce, sumTo_zero, Nat.add_zero]
      rw [show reduce.sumTo (fun i => s8 ((s.cv i + sumTo (fun k => maskW 256
          (cmpC (blk.getD (16 * k + i) 0))) 4) % 256)) 16 =
          reduce.sumTo (fun i => s8 ((255 * (kc i + ac i)) % 256)) 16 by
        have : ∀ m, m ≤ 16 → reduce.sumTo (fun i => s8 ((s.cv i + sumTo (fun k => maskW 256
            (cmpC (blk.getD (16 * k + i) 0))) 4) % 256)) m =
            reduce.sumTo (fun i => s8 ((255 * (kc i + ac i)) % 256)) m := by
          intro m hm
          induction m with
          | zero => rfl
          | succ m ih =>
            simp only [reduce.sumTo]
            rw [ih (by omega)]
            rcases hcv m with e | e
            · rw [e]
            · omega
        exact this 16 (Nat.le_refl _)]
      rw [intSum_neg (fun i => kc i + ac i) 16 (fun i hi => by have := (h2 i hi).1; have := hac i; omega)]
      simp only [Int.neg_neg, Int.toNat_natCast]
      omega
    · simp only [reduce, sumTo_zero, Nat.add_zero]
      rw [show reduce.sumTo (fun i => s8 ((s.fv i + sumTo (fun k => maskW 256
          (cmpF (blk.getD (16 * k + i) 0))) 4) % 256)) 16 =
          reduce.sumTo (fun i => s8 ((255 * (kf i + af i)) % 256)) 16 by
        have : ∀ m, m ≤ 16 → reduce.sumTo (fun i => s8 ((s.fv i + sumTo (fun k => maskW 256
            (cmpF (blk.getD (16 * k + i) 0))) 4) % 256)) m =
            reduce.sumTo (fun i => s8 ((255 * (kf i + af i)) % 256)) m := by
          intro m hm
          induction m with
          | zero => rfl
          | succ m ih =>
            simp only [reduce.sumTo]
            rw [ih (by omega)]
            rcases hfv m with e | e
            · rw [e]
            · omega
        exact this 16 (Nat.le_refl _)]
      rw [intSum_neg (fun i => kf i + af i) 16 (fun i hi => by have := (h2 i hi).2.1; have := haf i; omega)]
      simp only [Int.neg_neg, Int.toNat_natCast]
      omega
  · rename_i hp
    refine ⟨fun i => kc i + ac i, fun i => kf i + af i, ⟨by show s.pend + 4 ≤ 120; omega, by show (s.pend + 4) % 4 = 0; omega, fun i hi => ?_, ?_, ?_⟩⟩
    · have := (h2 i hi).1; have := (h2 i hi).2.1; have := hac i; have := haf i
      refine ⟨by show kc i + ac i ≤ s.pend + 4; omega, by show kf i + af i ≤ s.pend + 4; omega, ?_, ?_⟩
      · rcases hcv i with e | e
        · exact e
        · exact absurd hi e
      · rcases hfv i with e | e
        · exact e
        · exact absurd hi e
    · dsimp only; rw [sc, ← h3, hconts]; omega
    · dsimp only; rw [sf, ← h4, hfours]; omega

/-- **`neon_counter` counts what the generic code counts.** For any list of
64-byte blocks, accumulating the non-ASCII ones and then reducing gives
`Σ bc` continuation bytes and `Σ bf` four-byte leads. -/
theorem neon_counts (blks : List (List Nat)) (hl : ∀ b ∈ blks, b.length = 64 ∧ Bytes b) :
    (reduce (blks.foldl step ⟨fun _ => 0, fun _ => 0, 0, 0, 0⟩)).cont = L (blks.map bc) ∧
    (reduce (blks.foldl step ⟨fun _ => 0, fun _ => 0, 0, 0, 0⟩)).four = L (blks.map bf) := by
  have key : ∀ (bs : List (List Nat)) (s : NC) (kc kf : Nat → Nat) (C F : Nat),
      (∀ b ∈ bs, b.length = 64 ∧ Bytes b) → Inv s kc kf C F →
      ∃ kc' kf', Inv (bs.foldl step s) kc' kf' (C + L (bs.map bc)) (F + L (bs.map bf)) := by
    intro bs
    induction bs with
    | nil => intro s kc kf C F _ h; exact ⟨kc, kf, by simpa using h⟩
    | cons b r ih =>
      intro s kc kf C F hb h
      simp only [List.foldl_cons, List.map_cons, L_cons]
      have hb1 := hb b (by simp)
      have hr : ∀ b ∈ r, b.length = 64 ∧ Bytes b := fun b' h' => hb b' (by simp [h'])
      unfold step
      split
      · rename_i ha
        obtain ⟨kc', kf', h'⟩ := ih s kc kf C F hr h
        refine ⟨kc', kf', ?_⟩
        simp only [bc, bf, ha, ite_true, Nat.zero_add]; exact h'
      · rename_i ha
        obtain ⟨kc1, kf1, h1⟩ := acc_inv b hb1.1 hb1.2 s kc kf C F h
        obtain ⟨kc', kf', h'⟩ := ih _ kc1 kf1 _ _ hr h1
        refine ⟨kc', kf', ?_⟩
        simp only [bc, bf, ha, Bool.false_eq_true, ite_false]
        rw [← Nat.add_assoc, ← Nat.add_assoc]; exact h'
  obtain ⟨kc, kf, h1, -, h2, h3, h4⟩ := key blks ⟨fun _ => 0, fun _ => 0, 0, 0, 0⟩ (fun _ => 0) (fun _ => 0) 0 0 hl
    ⟨by simp, by simp, fun i _ => by simp, by simp [sumTo_zero], by simp [sumTo_zero]⟩
  generalize blks.foldl step _ = s at *
  have rc : ∀ (v k : Nat → Nat), (∀ i, i < 16 → k i ≤ 128 ∧ v i = (255 * k i) % 256) →
      reduce.sumTo (fun i => s8 (v i)) 16 = -((sumTo k 16 : Nat) : Int) := by
    intro v k hv
    have : ∀ m, m ≤ 16 → reduce.sumTo (fun i => s8 (v i)) m = reduce.sumTo (fun i => s8 ((255 * k i) % 256)) m := by
      intro m hm
      induction m with
      | zero => rfl
      | succ m ih => simp only [reduce.sumTo]; rw [ih (by omega), (hv m (by omega)).2]
    rw [this 16 (Nat.le_refl _), intSum_neg k 16 (fun i hi => (hv i hi).1)]
  simp only [Nat.zero_add] at h3 h4
  refine ⟨?_, ?_⟩
  · simp only [reduce]
    rw [rc s.cv kc (fun i hi => ⟨by have := (h2 i hi).1; omega, (h2 i hi).2.2.1⟩)]
    simp only [Int.neg_neg, Int.toNat_natCast]; omega
  · simp only [reduce]
    rw [rc s.fv kf (fun i hi => ⟨by have := (h2 i hi).2.1; omega, (h2 i hi).2.2.2⟩)]
    simp only [Int.neg_neg, Int.toNat_natCast]; omega

end SafetyMargins.LengthsNeon
