import SafetyMargins.Exact
/-
`valid_utf8_to_latin1.h`, the `convert_valid` decoder to Latin-1.

Its contract (`include/simdutf/implementation.h`) requires valid UTF-8 that
Latin-1 can represent: every character is an ASCII byte, or `0xC2`/`0xC3`
followed by one continuation byte (`LV`). Unlike the validating decoders, its
loop does not reject a block that starts on a continuation byte, so `Reach`
does not describe it directly. On `LV` input that test is never needed:

* every block head is a leading byte (`vreach_reach`), because the input
  starts with one, an ASCII byte is never followed by a continuation byte,
  and every call ends just before a leading byte;
* no table call takes an error row (`table_conv`), because two continuation
  bytes are never adjacent, so the 12-bit mask is well shaped (`well_noerr`).

So every state of the loop is a state of `Reach`, and the store bounds of the
validating decoders apply with the same `K = 3` lists (`valid_latin1_okK`).
The tail is scalar and writes exactly `L` of the bytes it converts
(`valid_latin1_tail`).
-/
namespace SafetyMargins.ValidLatin1
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.Validator SafetyMargins.Exact
  SafetyMargins.Tables SafetyMargins.TableFacts

/-- Valid UTF-8 whose characters Latin-1 can represent. -/
inductive LV : List Nat → Prop
  | nil : LV []
  | one (a : Nat) (rest : List Nat) (ha : a < 0x80) (h : LV rest) : LV (a :: rest)
  | two (c d : Nat) (rest : List Nat) (hc : 0xC2 ≤ c ∧ c ≤ 0xC3)
      (hd : 0x80 ≤ d ∧ d < 0xC0) (h : LV rest) : LV (c :: d :: rest)

/-- ASCII input satisfies the contract; `read_K1_fails` (66 ASCII bytes) is
therefore an input this header accepts, so the loads alone rule out `K = 1`. -/
theorem lv_ascii (n : Nat) : LV (List.replicate n 0x41) := by
  induction n with
  | zero => exact LV.nil
  | succ n ih => exact LV.one 0x41 _ (by decide) ih

theorem LV.lt256 {bs : List Nat} (h : LV bs) : ∀ b ∈ bs, b < 256 := by
  induction h with
  | nil => simp
  | one a rest ha _ ih =>
    intro b hb; simp at hb; rcases hb with rfl | hb
    · omega
    · exact ih b hb
  | two c d rest hc hd _ ih =>
    intro b hb; simp at hb; rcases hb with rfl | rfl | hb
    · omega
    · omega
    · exact ih b hb

/-- The first byte is not a continuation byte. -/
theorem LV.first {bs : List Nat} (h : LV bs) (hne : bs ≠ []) : isCont (bs.getD 0 0) = false := by
  cases h with
  | nil => exact absurd rfl hne
  | one a rest ha _ => simp [isCont]; omega
  | two c d rest hc hd _ => simp [isCont]; omega

/-- A continuation byte follows only `0xC2` or `0xC3`: neither an ASCII byte
nor another continuation byte. -/
theorem LV.next {bs : List Nat} (h : LV bs) :
    ∀ i, i + 1 < bs.length → isCont (bs.getD (i + 1) 0) = true →
      0xC2 ≤ bs.getD i 0 ∧ bs.getD i 0 ≤ 0xC3 := by
  induction h with
  | nil => intro i hi; simp at hi
  | one a rest ha hr ih =>
    intro i hi hc
    cases i with
    | zero =>
      simp at hc ⊢
      cases hr with
      | nil => simp at hi
      | one a' rest' ha' _ => simp [isCont] at hc; omega
      | two c d rest' hc' hd _ => simp [isCont] at hc; omega
    | succ i =>
      simp only [List.getD_cons_succ] at hc ⊢
      exact ih i (by simp at hi; omega) hc
  | two c d rest hc hd hr ih =>
    intro i hi hcont
    cases i with
    | zero => simp; omega
    | succ i =>
      cases i with
      | zero =>
        simp at hcont ⊢
        cases hr with
        | nil => simp at hi
        | one a' rest' ha' _ => simp [isCont] at hcont; omega
        | two c' d' rest' hc' hd' _ => simp [isCont] at hcont; omega
      | succ i =>
        simp only [List.getD_cons_succ] at hcont ⊢
        exact ih i (by simp at hi; omega) hcont

theorem LV.after_ascii {bs : List Nat} (h : LV bs) (i : Nat) (hi : i + 1 < bs.length)
    (ha : bs.getD i 0 < 0x80) : isCont (bs.getD (i + 1) 0) = false := by
  cases hc : isCont (bs.getD (i + 1) 0)
  · rfl
  · have := h.next i hi hc; omega

theorem LV.no2 {bs : List Nat} (h : LV bs) (i : Nat) (hi : i + 1 < bs.length)
    (hc : isCont (bs.getD i 0) = true) : isCont (bs.getD (i + 1) 0) = false := by
  cases hc' : isCont (bs.getD (i + 1) 0)
  · rfl
  · have := h.next i hi hc'
    simp only [isCont, Bool.and_eq_true, decide_eq_true_eq] at hc; omega

/-- The loop of `valid_utf8_to_latin1.h` on bytes `bs`, weights `bs.map w01`.
As `Reach`, except that entering a non-ASCII block has no test on its first
byte, only the loop test (`hlen`), and that a call is a non-error call: it
ends just before a leading byte (`table_conv` shows that table calls are
such calls on `LV` input; the fast paths are `count_step`). -/
inductive VReach (bs : List Nat) : Bool → Nat → Nat → Nat → Prop
  | start : VReach bs false 0 0 0
  | ascii (t o : Nat) (h : VReach bs false t t o) (hlen : t + 64 ≤ bs.length)
      (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) :
      VReach bs false (t + 64) (t + 64) (o + 64)
  | enter (t o : Nat) (h : VReach bs false t t o) (hlen : t + 64 ≤ bs.length) :
      VReach bs true t t o
  | call (pos t o c p : Nat) (h : VReach bs true pos t o) (ht : t < pos + 52)
      (hc : 1 ≤ c) (hnext : (bs.map w01).getD (t + c) 0 ≠ 0)
      (hp : p ≤ L (((bs.map w01).drop t).take c) + cont (bs.map w01) t) :
      VReach bs true pos (t + c) (o + p)
  | leave (pos t o : Nat) (h : VReach bs true pos t o) (ht : pos + 52 ≤ t) :
      VReach bs false t t o

theorem getD_w01_map (bs : List Nat) (i : Nat) (hi : i < bs.length) :
    (bs.map w01).getD i 0 = w01 (bs.getD i 0) := by
  simp [List.getD_eq_getElem?_getD, List.getElem?_map, List.getElem?_eq_getElem hi]

theorem w01_ne_zero (b : Nat) (h : isCont b = false) : w01 b ≠ 0 := by
  simp [w01, h]

/-- On `LV` input, every state of the loop is a state of `Reach`, and every
block head is a leading byte or the end of the input. -/
theorem vreach_reach {bs : List Nat} (hv : LV bs) :
    ∀ b pos t o, VReach bs b pos t o →
      Reach (bs.map w01) b pos t o ∧
      (b = false → bs.length ≤ t ∨ (bs.map w01).getD t 0 ≠ 0) ∧
      (b = true → (bs.map w01).getD t 0 ≠ 0) := by
  intro b pos t o h
  induction h with
  | start =>
    refine ⟨Reach.start, fun _ => ?_, fun h => by simp at h⟩
    by_cases hne : bs = []
    · left; simp [hne]
    · right
      have hlen : 0 < bs.length := by
        cases bs with
        | nil => exact absurd rfl hne
        | cons _ _ => simp
      rw [getD_w01_map bs 0 hlen]
      exact w01_ne_zero _ (hv.first hne)
  | ascii t o h hlen hascii ih =>
    refine ⟨Reach.ascii t o ih.1 (fun i hi => ?_), fun _ => ?_, fun h => by simp at h⟩
    · rw [getD_w01_map bs (t + i) (by omega)]
      have ha := hascii i hi
      have hnc : isCont (bs.getD (t + i) 0) = false := by
        simp only [isCont, Bool.and_eq_false_iff, decide_eq_false_iff_not]
        left; omega
      simp only [w01, hnc, Bool.false_eq_true, ↓reduceIte]
    · by_cases hend : bs.length ≤ t + 64
      · left; exact hend
      · right
        rw [getD_w01_map bs (t + 64) (by omega)]
        apply w01_ne_zero
        have := hv.after_ascii (t + 63) (by omega) (hascii 63 (by omega))
        simpa [show t + 63 + 1 = t + 64 by omega] using this
  | enter t o h hlen ih =>
    have hlead : (bs.map w01).getD t 0 ≠ 0 := by
      rcases ih.2.1 rfl with hend | hl
      · omega
      · exact hl
    exact ⟨Reach.enter t o ih.1 hlead, fun h => by simp at h, fun _ => hlead⟩
  | call pos t o c p h ht hc hnext hp ih =>
    exact ⟨Reach.call pos t o (t + c) (o + p) ih.1 ht (Step.conv t o c p hc hnext hp),
      fun h => by simp at h, fun _ => hnext⟩
  | leave pos t o h ht ih =>
    exact ⟨Reach.leave pos t o ih.1 ht, fun _ => Or.inr (ih.2.2 rfl), fun h => by simp at h⟩

/-- A store of a call fits, as in `reach_store`. -/
theorem valid_latin1_store {bs : List Nat} (hv : LV bs) (K pos t o d s : Nat)
    (h : VReach bs true pos t o)
    (hloop : pos + 64 + (tk K (bs.map w01) + 1) ≤ (bs.map w01).length)
    (hd : t + d ≤ pos + 64)
    (hs : s ≤ cont (bs.map w01) t + cnt (((bs.map w01).drop t).take d) + K) :
    o + s ≤ L (bs.map w01) :=
  reach_store K pos t o d s (bs.map w01) (vreach_reach hv _ _ _ _ h).1 hloop hd hs

/-- The same with a store path of an `okK` list, as in `reach_okK`. -/
theorem valid_latin1_okK {bs : List Nat} (hv : LV bs) (K pos t o d : Nat)
    (ps : List MPath) (q : MPath)
    (h : VReach bs true pos t o)
    (hloop : pos + 64 + (tk K (bs.map w01) + 1) ≤ (bs.map w01).length)
    (hd : t + d ≤ pos + 64) (hK : okK K ps = true) (hq : q ∈ ps)
    (hf : q.f ≤ cont (bs.map w01) t + cnt (((bs.map w01).drop t).take d)) :
    o + q.s ≤ L (bs.map w01) :=
  reach_okK K pos t o d (bs.map w01) ps q (vreach_reach hv _ _ _ _ h).1 hloop hd hK hq hf

/-- The scalar tail converts the rest exactly: one output byte per leading
byte, `L` of the rest. With the cursor at most `L` of the consumed bytes
(`reach_lag`), the tail ends inside the buffer. -/
theorem valid_latin1_tail {bs : List Nat} (hv : LV bs) (t o : Nat)
    (h : VReach bs false t t o) :
    o + L ((bs.map w01).drop t) ≤ L (bs.map w01) := by
  have hr := (vreach_reach hv _ _ _ _ h).1
  have ho := (reach_lag (bs.map w01) _ _ _ _ hr).1 rfl
  have := L_take_add_drop t (bs.map w01)
  omega

/-- On `LV` input, a table call inside a block never takes an error row: two
continuation bytes are never adjacent, so its 12-bit mask is well shaped, and
`well_noerr` excludes error rows on well-shaped masks. The call then ends
just before a leading byte (`row_conv`), which is the `call` rule of
`VReach`. -/
theorem table_conv {bs : List Nat} (hv : LV bs) (t : Nat) (hlen : t + 13 ≤ bs.length) :
    let r := row (bs.map w01) t
    bigIdx[r.m]! < 209 ∧ 1 ≤ bigConsumed[r.m]! ∧
      (bs.map w01).getD (t + bigConsumed[r.m]!) 0 ≠ 0 ∧
      units bigIdx[r.m]! ≤ L (((bs.map w01).drop t).take bigConsumed[r.m]!) +
        cont (bs.map w01) t := by
  intro r
  have hwell : wellShaped (emask (bs.map w01) t 12) = true := by
    unfold wellShaped
    apply List.all_eq_true.mpr
    intro j hj
    have hj9 := List.mem_range.mp hj
    have hb := fun k (hk : k < 12) => bit_emask (bs.map w01) t 12 k hk
    rw [hb j (by omega), hb (j + 1) (by omega), hb (j + 2) (by omega), hb (j + 3) (by omega)]
    rw [lead_map w01_contZero hv.lt256 _ (by omega), lead_map w01_contZero hv.lt256 _ (by omega),
      lead_map w01_contZero hv.lt256 _ (by omega), lead_map w01_contZero hv.lt256 _ (by omega)]
    have h01 := hv.no2 (t + j + 1) (by omega)
    have h12 := hv.no2 (t + (j + 1) + 1) (by omega)
    simp only [show t + j + 1 + 1 = t + (j + 1) + 1 by omega] at h01
    simp only [show t + (j + 1) + 1 + 1 = t + (j + 2) + 1 by omega] at h12
    generalize isCont (bs.getD (t + j + 1) 0) = c0 at *
    generalize isCont (bs.getD (t + (j + 1) + 1) 0) = c1 at *
    generalize isCont (bs.getD (t + (j + 2) + 1) 0) = c2 at *
    generalize isCont (bs.getD (t + (j + 3) + 1) 0) = c3 at *
    revert h01 h12
    cases c0 <;> cases c1 <;> cases c2 <;> cases c3 <;> decide
  have hi : bigIdx[r.m]! < 209 := by
    have := (List.all_eq_true.mp well_noerr) r.m r.hmem
    simp only [Bool.or_eq_true, Bool.not_eq_true', decide_eq_true_eq] at this
    rcases this with hw | hw
    · rw [r.hm, hwell] at hw; exact absurd hw (by decide)
    · exact hw
  obtain ⟨hc1, -, hnext, hp⟩ := row_conv _ t r hi
  exact ⟨hi, hc1, hnext, hp⟩

/-- The ASCII fast path (`0xfff`: 12 leads, 16 bytes stored) of the x86,
arm64 and POWER Latin-1 kernels, at `K = 3`. -/
theorem valid_latin1_fff {bs : List Nat} (hv : LV bs) (pos t o : Nat)
    (h : VReach bs true pos t o)
    (hloop : pos + 64 + (tk 3 (bs.map w01) + 1) ≤ (bs.map w01).length)
    (hd : t + 13 ≤ pos + 64)
    (hp : 12 ≤ sumUpTo (fun k => lead (bs.map w01) (t + 1 + k)) 12) :
    o + 16 ≤ L (bs.map w01) :=
  reach_x86_latin1_fff pos t o (bs.map w01) (vreach_reach hv _ _ _ _ h).1 hloop hd hp

/-- A table call, with a store path of an `okK` list. -/
theorem valid_latin1_row {bs : List Nat} (hv : LV bs) (K pos t o : Nat)
    (ps : List MPath) (q : MPath) (h : VReach bs true pos t o)
    (hloop : pos + 64 + (tk K (bs.map w01) + 1) ≤ (bs.map w01).length)
    (hi : bigIdx[(row (bs.map w01) t).m]! < 209)
    (hd : t + bigConsumed[(row (bs.map w01) t).m]! + 1 ≤ pos + 64)
    (hK : okK K ps = true) (hq : q ∈ ps)
    (hf : q.f ≤ units bigIdx[(row (bs.map w01) t).m]! + 1) :
    o + q.s ≤ L (bs.map w01) :=
  reach_row K pos t o (bs.map w01) ps q (row (bs.map w01) t) (vreach_reach hv _ _ _ _ h).1
    hloop hi (fun k _ => getD_w01 bs (t + k)) hd hK hq hf

/-- The header counts `K = 3` leading bytes on every target. With the
kernels' Latin-1 store paths, `K = 3` covers every store
(`Utf8.K_x86_latin1`, `Arm64.K_arm_latin1`, `Ppc64.K_ppc_latin1`; the lsx
and lasx lists need only `K = 0`), and the 16-byte loads at `pos + 51` need
only `K ≥ 2` (`read_ok`). -/
theorem valid_latin1_K3 : okK 3 x86_latin1 = true := K_x86_latin1.1

end SafetyMargins.ValidLatin1
