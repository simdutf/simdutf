import SafetyMargins.IcelakeUtf8
import SafetyMargins.ValidatorSound
/-
Ice Lake UTF-8 → UTF-16, one block with the checks: soundness and
completeness of `process_block_utf8_to_utf16`
(src/icelake/icelake_utf8_common.inl.cpp:22-434), for `IcelakeReplacement.lean`.

`IcelakeUtf8.lean` transcribes the block (`block16`) at the level of the masks
and proves its store bounds; its range checks `fail3` (lines 217-231) and
`fail4` (lines 337-356), with the unit values `tsl` (lines 165-204) and `wout4`
(lines 295-309), played no part there. Here they do. The transcription is the
one of `IcelakeUtf8.lean`, checked again against the C++ for this file: the
compress of `mend` (lines 165, 256) is `selPos`; the three byte permutations
take their indices modulo 64 (lines 173-202, 264-292); `Wout` is the OR of the
three (ternary logic 254, lines 203-204, 293-294); `Mlo`/`Mhi` are bits `k`,
`k + 1` of `pext(mp3, mend)` (lines 295-297); `M3` is `pext(m3 << 2, mend)`
with `m3 = m34 & (b ^ m4)` (lines 219-221, 340-342); the comparisons are
unsigned 16-bit (lines 223-228, 344-352).

* **The range checks are exact** (finite checks, `decide +kernel`). On a
  three-byte character `E0`-`EF` with two continuation bytes, the value the
  kernel assembles is the code point and the test of lines 223-229 rejects
  exactly the overlong forms and the surrogates (`chk3_ok`); on the first
  three bytes of a four-byte lead `F0`-`FF`, the high surrogate of lines
  301-309 fails the test of lines 350-352 exactly for an overlong form, a code
  point above U+10FFFF, or a lead `F5`-`FF` (`chk4_ok`). The two-byte overlong
  leads `C0`, `C1` are `milltwobytes` (lines 112-119).
* **The units.** On a block that passes the structure check, every three-byte
  character and the high surrogate of every four-byte character that the
  block consumes is a processed unit (rank below 32, `rank_lt32`, `mend_at`),
  with the value above (`tsl_at`, `unit3`, `unit4`: on the 4-byte path a
  three-byte unit is neither `Mlo` nor `Mhi`).
* **Validity** (`valid34`, `dfa_inv`). The automaton `ValidExact.step` on the
  bytes of a checked block is a function of the classes of the last three
  bytes and of the byte before (`sOf`, `tagB`; finite check `dfa_ok` over the
  37 windows `reach3`); it stays out of the error state when the leads are
  acceptable and the second bytes in range, and is 0 at a boundary (`Bnd`,
  no lead of the last three bytes needs the next byte).
* **The count is exact** (`pot_exact`): the potential of `IcelakeUtf8.pot` is an
  equality (finite check `exact_ok`), so the stored units are
  `utf16_length_from_utf8` of the consumed bytes.
* **`block_sound`.** A block that returns `true` consumed `c` bytes that are
  valid UTF-8 (from a character boundary), stored exactly their `L`, stops
  where `EndOK` says, and consumes at least one byte when it sees one.
* **`block_complete`.** A block of valid UTF-8 that starts on a boundary
  passes every check: `block16 true = block16 false`.

`decide +kernel` is used for finite exhaustive checks only: `dfa_ok` (37 × 5 ×
256), `exact_ok` (37 × 6 × 6 × 3), `chk3_ok` and `chk4_ok` (16 × 64 × 64),
`comp_ok` (256 × 256).
-/
namespace SafetyMargins.IcelakeUtf8
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.ValidExact SafetyMargins.Validator
set_option linter.unusedSimpArgs false
set_option linter.unusedVariables false

/-! ## Finite facts -/

/-- The automaton state a lead enters when it restricts its second byte
(`ValidExact.step`: `0xE0` → 4, `0xED` → 5, `0xF0` → 6, `0xF4` → 7), else 0. -/
def tagB (x : Nat) : Nat :=
  if x = 0xE0 then 4 else if x = 0xED then 5 else if x = 0xF0 then 6 else if x = 0xF4 then 7 else 0

/-- The range of the second byte after a restricting lead (Unicode Table 3-7). -/
def rngT (t x : Nat) : Bool :=
  if t = 4 then decide (0xA0 ≤ x) else if t = 5 then decide (x < 0xA0)
  else if t = 6 then decide (0x90 ≤ x) else if t = 7 then decide (x < 0x90) else true

/-- The automaton state from the pending continuation bytes `r` and the tag of
the byte before. -/
def sOf (r t : Nat) : Nat := if t = 0 then r else t

/-- A byte that may start a character: not `0xC0`, `0xC1`, nor `0xF5` and above. -/
def leadOK (x : Nat) : Bool := !(decide (0xC0 ≤ x ∧ x < 0xC2)) && decide (x < 0xF5)

def tagCls (t : Nat) (c1 : Cls) : Bool :=
  t == 0 || ((t == 4 || t == 5) && c1 == .L3) || ((t == 6 || t == 7) && c1 == .L4)

def dfaOK : Bool :=
  reach3.all fun w => [0, 4, 5, 6, 7].all fun t => (List.range 256).all fun x =>
    !(tagCls t w.2.2 && chkW w.1 w.2.1 w.2.2 (clsB x) && (clsB x == .C || leadOK x) &&
        (t == 0 || rngT t x)) ||
      ValidExact.step (sOf (remW w.2.2 w.2.1 w.1) t) x == sOf (remW (clsB x) w.2.2 w.2.1) (tagB x)

/-- One step of the automaton on a window the structure check allows, when the
lead is acceptable and the second byte in range. -/
theorem dfa_ok : dfaOK = true := by decide +kernel

def exactOK : Bool :=
  reach3.all fun t => allCls.all fun c0 => allCls.all fun cn =>
    [PF.inner, PF.full, PF.tail].all fun pf =>
      !winPre pf t.1 t.2.1 t.2.2 c0 cn || (pf == .full && !mendW pf t.1 t.2.1 cn) ||
        (ind (mendW pf t.1 t.2.1 cn) + phi t.2.1 t.2.2 c0 == c0.w + phi t.1 t.2.1 t.2.2)

/-- The potential of `IcelakeMask.win_all` is exact, except at byte 63 of a
full block with no `mend` bit there. -/
theorem exact_ok : exactOK = true := by decide +kernel

def phi0OK : Bool := allCls.all fun a => allCls.all fun b => allCls.all fun c =>
  mcW a b c || phi a b c == 0

/-- Nothing is pending where no continuation byte is expected. -/
theorem phi0_ok : phi0OK = true := by decide

/-- The 16-bit value the kernel assembles from the third last, second last and
last bytes (`tsl` on a three-byte character or the high surrogate of a four-byte
character). -/
def tval (b0 b1 b2 : Nat) : Nat :=
  ((b2 &&& 0x3F) ||| ((b1 &&& 0x3F) <<< 6) ||| ((b0 &&& 0x3F) <<< 12)) % 65536

def chk3OK : Bool := (List.range 16).all fun i => (List.range 64).all fun j => (List.range 64).all fun k =>
  let t := tval (0xE0 + i) (0x80 + j) (0x80 + k)
  (!(decide (t < 0x800) || decide (sub16 t 0xD800 < 0x800))) == rngT (tagB (0xE0 + i)) (0x80 + j)

/-- The range check of a three-byte unit (lines 217-231, 344-349) rejects
exactly the overlong forms and the surrogates. -/
theorem chk3_ok : chk3OK = true := by decide +kernel

def chk4OK : Bool := (List.range 16).all fun i => (List.range 64).all fun j => (List.range 64).all fun k =>
  let t := tval (0xF0 + i) (0x80 + j) (0x80 + k)
  (!decide (0x400 ≤ sub16 (((t >>> 4) + 0xD7C0) % 65536) 0xD800)) ==
    (leadOK (0xF0 + i) && rngT (tagB (0xF0 + i)) (0x80 + j))

/-- The high-surrogate check (lines 350-352) rejects exactly the overlong
four-byte forms, those above U+10FFFF and the leads `0xF5`-`0xFF`. -/
theorem chk4_ok : chk4OK = true := by decide +kernel

def compOK : Bool := (List.range 256).all fun x0 => (List.range 256).all fun x1 =>
  ValidExact.step (ValidExact.step 0 x0) x1 == 8 || (leadOK x0 && rngT (tagB x0) x1)

/-- A lead and the byte after it that the automaton accepts from state 0. -/
theorem comp_ok : compOK = true := by decide +kernel

theorem chk3_at {b0 b1 b2 : Nat} (h0 : 0xE0 ≤ b0) (h0' : b0 < 0xF0) (h1 : 0x80 ≤ b1) (h1' : b1 < 0xC0)
    (h2 : 0x80 ≤ b2) (h2' : b2 < 0xC0) :
    (!(decide (tval b0 b1 b2 < 0x800) || decide (sub16 (tval b0 b1 b2) 0xD800 < 0x800))) =
      rngT (tagB b0) b1 := by
  obtain ⟨i, rfl⟩ : ∃ i, b0 = 0xE0 + i := ⟨b0 - 0xE0, by omega⟩
  obtain ⟨j, rfl⟩ : ∃ j, b1 = 0x80 + j := ⟨b1 - 0x80, by omega⟩
  obtain ⟨k, rfl⟩ : ∃ k, b2 = 0x80 + k := ⟨b2 - 0x80, by omega⟩
  have := List.all_eq_true.mp chk3_ok i (List.mem_range.mpr (by omega))
  have := List.all_eq_true.mp this j (List.mem_range.mpr (by omega))
  have := List.all_eq_true.mp this k (List.mem_range.mpr (by omega))
  simpa using this

theorem chk4_at {b0 b1 b2 : Nat} (h0 : 0xF0 ≤ b0) (h0' : b0 < 0x100) (h1 : 0x80 ≤ b1) (h1' : b1 < 0xC0)
    (h2 : 0x80 ≤ b2) (h2' : b2 < 0xC0) :
    (!decide (0x400 ≤ sub16 (((tval b0 b1 b2 >>> 4) + 0xD7C0) % 65536) 0xD800)) =
      (leadOK b0 && rngT (tagB b0) b1) := by
  obtain ⟨i, rfl⟩ : ∃ i, b0 = 0xF0 + i := ⟨b0 - 0xF0, by omega⟩
  obtain ⟨j, rfl⟩ : ∃ j, b1 = 0x80 + j := ⟨b1 - 0x80, by omega⟩
  obtain ⟨k, rfl⟩ : ∃ k, b2 = 0x80 + k := ⟨b2 - 0x80, by omega⟩
  have := List.all_eq_true.mp chk4_ok i (List.mem_range.mpr (by omega))
  have := List.all_eq_true.mp this j (List.mem_range.mpr (by omega))
  have := List.all_eq_true.mp this k (List.mem_range.mpr (by omega))
  simpa using this

theorem comp_at {x0 x1 : Nat} (h0 : x0 < 256) (h1 : x1 < 256)
    (h : ValidExact.step (ValidExact.step 0 x0) x1 ≠ 8) : leadOK x0 = true ∧ rngT (tagB x0) x1 = true := by
  have := List.all_eq_true.mp comp_ok x0 (List.mem_range.mpr h0)
  have := List.all_eq_true.mp this x1 (List.mem_range.mpr h1)
  simp only [Bool.or_eq_true, beq_iff_eq, Bool.and_eq_true] at this
  rcases this with h' | h'
  · exact absurd h' h
  · exact h'

theorem dfa_at {c3 c2 c1 : Cls} (hc : cons3 c3 c2 c1 = true) {t x : Nat} (ht : tagCls t c1 = true)
    (ht' : t = 0 ∨ t = 4 ∨ t = 5 ∨ t = 6 ∨ t = 7) (hx : x < 256)
    (hk : chkW c3 c2 c1 (clsB x) = true) (hl : (clsB x == .C) = true ∨ leadOK x = true)
    (hr : t = 0 ∨ rngT t x = true) :
    ValidExact.step (sOf (remW c1 c2 c3) t) x = sOf (remW (clsB x) c1 c2) (tagB x) := by
  have := List.all_eq_true.mp dfa_ok _ (cons3_mem hc)
  have := List.all_eq_true.mp this t (by rcases ht' with h | h | h | h | h <;> subst h <;> simp)
  have := List.all_eq_true.mp this x (List.mem_range.mpr hx)
  have e1 : (clsB x == .C || leadOK x) = true := by rcases hl with h | h <;> simp [h]
  have e2 : (t == 0 || rngT t x) = true := by rcases hr with h | h <;> simp [h]
  simp only [ht, hk, e1, e2, Bool.and_self, Bool.not_true, Bool.false_or, beq_iff_eq] at this
  exact this

theorem phi0_at {a b c : Cls} (h : mcW a b c = false) : phi a b c = 0 := by
  have := List.all_eq_true.mp phi0_ok a (mem_allCls a)
  have := List.all_eq_true.mp this b (mem_allCls b)
  have := List.all_eq_true.mp this c (mem_allCls c)
  simpa [h] using this

theorem exact_at {pf : PF} {c3 c2 c1 c0 cn : Cls} (h : winPre pf c3 c2 c1 c0 cn = true)
    (hf : pf = .full → mendW pf c3 c2 cn = true) :
    ind (mendW pf c3 c2 cn) + phi c2 c1 c0 = c0.w + phi c3 c2 c1 := by
  have hm : (c3, c2, c1) ∈ reach3 := by
    simp only [winPre, Bool.and_eq_true] at h; exact cons3_mem h.1.1.1.1
  have := List.all_eq_true.mp exact_ok _ hm
  have := List.all_eq_true.mp this c0 (mem_allCls c0)
  have := List.all_eq_true.mp this cn (mem_allCls cn)
  have := List.all_eq_true.mp this pf (by cases pf <;> simp)
  simp only [h, Bool.not_true, Bool.false_or, Bool.or_eq_true, Bool.and_eq_true, beq_iff_eq,
    Bool.not_eq_true'] at this
  rcases this with ⟨h1, h2⟩ | h3
  · rw [hf h1] at h2; cases h2
  · exact h3

theorem tagB_ne (x : Nat) : tagB x = 0 ∨ tagB x = 4 ∨ tagB x = 5 ∨ tagB x = 6 ∨ tagB x = 7 := by
  unfold tagB; repeat' split
  all_goals simp

theorem tagCls_B (x : Nat) : tagCls (tagB x) (clsB x) = true := by
  unfold tagB; repeat' split
  all_goals (subst_vars; first | rfl | simp [tagCls])

theorem tagB_ge (x : Nat) (h : tagB x ≠ 0) : 0xE0 ≤ x ∧ x ≤ 0xF4 := by
  unfold tagB at h; repeat' split at h
  all_goals omega
/-! ## The automaton on a checked block -/

section Dfa
variable {blk : List Nat}

/-- The tag of the byte before position `k`. -/
def tg (blk : List Nat) (k : Nat) : Nat := if k = 0 then 0 else tagB (byte blk (k - 1))

theorem byte_lt_of {k : Nat} (hk : k < blk.length)
    (hL : (cls blk k == .C) = false → leadOK (byte blk k) = true) : byte blk k < 256 := by
  rw [cls_lt _ _ hk] at hL
  by_cases hC : (clsB (byte blk k) == .C) = true
  · rcases clsB_eq (byte blk k) with ⟨h1, e⟩ | ⟨_, h1, _⟩ | ⟨_, _, e⟩ | ⟨_, _, e⟩ | ⟨_, e⟩
    all_goals first | omega | (rw [e] at hC; simp at hC)
  · have := hL (by simpa using hC)
    simp only [leadOK, Bool.and_eq_true, decide_eq_true_eq] at this; omega

theorem dfa_inv (hS : Struct blk) (h64 : blk.length ≤ 64) (c : Nat) (hc : c ≤ blk.length)
    (hL : ∀ k, k < c → (cls blk k == .C) = false → leadOK (byte blk k) = true)
    (hR : ∀ k, 1 ≤ k → k < c → rngT (tagB (byte blk (k - 1))) (byte blk k) = true) :
    ∀ k, k ≤ c → cons3 (clm blk k 3) (clm blk k 2) (clm blk k 1) = true ∧
      run 0 (blk.take k) = sOf (remW (clm blk k 1) (clm blk k 2) (clm blk k 3)) (tg blk k) := by
  intro k
  induction k with
  | zero => intro _; simp [clm, cons3_start, remW, run, sOf, tg, Cls.len]
  | succ k ih =>
    intro hk
    obtain ⟨hc3, hr⟩ := ih (by omega)
    have hk' : k < blk.length := by omega
    have hx := byte_lt_of hk' (hL k (by omega))
    have hcl : cls blk k = clsB (byte blk k) := cls_lt _ _ hk'
    have s1 : clm blk (k + 1) 3 = clm blk k 2 := clm_succ _ _ _
    have s2 : clm blk (k + 1) 2 = clm blk k 1 := clm_succ _ _ _
    have s3 : clm blk (k + 1) 1 = cls blk k := by rw [clm_succ, clm_zero]
    have hz : cls blk k ≠ .Z := by rw [hcl]; exact clsB_ne_Z _
    have hchk := struct_at _ hS k (by omega)
    have htc : tagCls (tg blk k) (clm blk k 1) = true := by
      unfold tg
      split
      · simp [tagCls]
      · rename_i h0
        have : clm blk k 1 = clsB (byte blk (k - 1)) := by
          unfold clm; rw [ite_eq_left (by omega), cls_lt _ _ (by omega)]
        rw [this]; exact tagCls_B _
    have ht' : tg blk k = 0 ∨ tg blk k = 4 ∨ tg blk k = 5 ∨ tg blk k = 6 ∨ tg blk k = 7 := by
      unfold tg; split
      · exact Or.inl rfl
      · exact tagB_ne _
    have hl : (clsB (byte blk k) == .C) = true ∨ leadOK (byte blk k) = true := by
      by_cases h : (cls blk k == .C) = true
      · left; rwa [← hcl]
      · right; exact hL k (by omega) (by simpa using h)
    have hrr : tg blk k = 0 ∨ rngT (tg blk k) (byte blk k) = true := by
      unfold tg; split
      · exact Or.inl rfl
      · right; exact hR k (by omega) (by omega)
    rw [hcl] at hchk hz
    have hstep := dfa_at hc3 htc ht' hx hchk hl hrr
    refine ⟨by rw [s1, s2, s3, hcl]; exact cons3_step hc3 hz hchk, ?_⟩
    rw [run_take_succ 0 blk k hk', hr]
    show ValidExact.step _ (byte blk k) = _
    rw [hstep, s1, s2, s3, hcl]
    simp [tg]

/-- Where nothing is expected: no lead of the last three bytes needs the byte
at `c`. -/
def Bnd (blk : List Nat) (c : Nat) : Prop := mcW (clm blk c 3) (clm blk c 2) (clm blk c 1) = false

theorem remW_of_bnd {c : Nat} (hc3 : cons3 (clm blk c 3) (clm blk c 2) (clm blk c 1) = true)
    (hb : Bnd blk c) : remW (clm blk c 1) (clm blk c 2) (clm blk c 3) = 0 := by
  obtain ⟨w1, -, -⟩ := validWin_at hc3 .Z
  unfold Bnd at hb
  rw [hb] at w1
  simpa using w1

theorem tg_of_bnd {c : Nat} (hc : c ≤ blk.length) (hb : Bnd blk c) : tg blk c = 0 := by
  unfold tg; split
  · rfl
  · rename_i h0
    unfold Bnd mcW at hb
    simp only [Bool.or_eq_false_iff] at hb
    have e : clm blk c 1 = clsB (byte blk (c - 1)) := by
      unfold clm; rw [ite_eq_left (by omega), cls_lt _ _ (by omega)]
    have h1 := hb.1.1
    rw [e, clsB_m234] at h1
    simp only [decide_eq_false_iff_not] at h1
    rcases Nat.eq_zero_or_pos (tagB (byte blk (c - 1))) with h | h
    · exact h
    · have := tagB_ge (byte blk (c - 1)) (by omega); omega

/-- **A checked block is valid up to a boundary** when its leads are
acceptable and the second bytes in range. -/
theorem valid_of_checks (hS : Struct blk) (h64 : blk.length ≤ 64) {c : Nat} (hc : c ≤ blk.length)
    (hL : ∀ k, k < c → (cls blk k == .C) = false → leadOK (byte blk k) = true)
    (hR : ∀ k, 1 ≤ k → k < c → rngT (tagB (byte blk (k - 1))) (byte blk k) = true)
    (hb : Bnd blk c) : run 0 (blk.take c) = 0 := by
  obtain ⟨hc3, hr⟩ := dfa_inv hS h64 c hc hL hR c (Nat.le_refl _)
  rw [hr, remW_of_bnd hc3 hb, tg_of_bnd hc hb]; rfl

theorem bnd_of_end (hS : Struct blk) (h64 : blk.length ≤ 64) {c : Nat} (hc : c ≤ blk.length)
    (hE : EndOK blk c) : Bnd blk c := by
  by_cases h6 : c < 64
  · have := struct_at _ hS c h6
    have hnc : (cls blk c == .C) = false := by
      by_cases hl : c < blk.length
      · rw [← isCont_cls c hl]; exact hE.1 hl
      · rw [cls_ge _ _ (by omega)]; rfl
    unfold Bnd
    simpa [chkW, hnc] using this
  · have hc64 : c = 64 := by omega
    subst hc64
    have hl : blk.length = 64 := by omega
    have e : ∀ d, 1 ≤ d → d ≤ 3 → clm blk 64 d = clsB (byte blk (64 - d)) := by
      intro d h1 h3; unfold clm; rw [ite_eq_left (by omega), cls_lt _ _ (by omega)]
    unfold Bnd
    rw [e 3 (by omega) (by omega), e 2 (by omega) (by omega), e 1 (by omega) (by omega)]
    rcases hE.2 rfl with ha | ⟨a0, a1, a2, a3⟩
    · have h63 := struct_at _ hS 63 (by omega)
      simp only [clm, show 3 ≤ 63 by omega, show 2 ≤ 63 by omega, show 1 ≤ 63 by omega,
        ite_true] at h63
      rw [cls_lt _ _ (by omega), cls_lt _ _ (by omega), cls_lt _ _ (by omega),
        cls_lt _ _ (by omega)] at h63
      have hA : clsB (byte blk 63) = .A := by
        rcases clsB_eq (byte blk 63) with ⟨_, e⟩ | ⟨h1, _⟩ | ⟨h1, _⟩ | ⟨h1, _⟩ | ⟨h1, _⟩
        · exact e
        all_goals omega
      rw [hA] at h63 ⊢
      revert h63
      simp only [show (64 : Nat) - 2 = 62 from rfl, show (64 : Nat) - 3 = 61 from rfl]
      cases clsB (byte blk 60) <;> cases clsB (byte blk 61) <;> cases clsB (byte blk 62) <;> decide
    · have hc : ∀ i, isCont (byte blk i) = true → clsB (byte blk i) = .C := by
        intro i h; rw [← clsB_C] at h; simpa using h
      simp only [show (64 : Nat) - 1 = 63 from rfl, show (64 : Nat) - 2 = 62 from rfl,
        show (64 : Nat) - 3 = 61 from rfl]
      rw [hc 61 a1, hc 62 a2, hc 63 a3]; rfl

end Dfa

/-! ## The units of the 3- and 4-byte paths -/

section Units
variable {blk : List Nat}

/-- `mend` of the 4-byte path has no bit past the block (as in `mout_top`). -/
theorem mend4_in (hS : Struct blk) (p : Nat) : andB (mend4 blk) (bM blk) p = mend4 blk p := by
  by_cases hp : p < blk.length
  · exact andB_bM _ _ hp
  · have hb : bM blk p = false := by simp [bM, hp]
    simp only [andB, hb, Bool.and_false]
    symm
    simp only [mend4, shr, tailbit, Bool.or_eq_false_iff]
    refine ⟨⟨?_, ?_⟩, ?_⟩
    · by_cases h1 : p < 64
      · have e1 : m1234 blk (p + 1) = false := by
          rw [m1234_cls, cls_ge _ _ (by omega)]; rfl
        by_cases h2 : p + 1 < 64
        · have := chk_eq hS (p + 1) h2
          have hb1 : bM blk (p + 1) = false := by simp [bM]; omega
          rw [hb1] at this
          have : mp3 blk (p + 1) = false := by
            cases h : mp3 blk (p + 1)
            · rfl
            · have := mc4_of_mp3 (blk := blk) _ h; simp_all
          simp [this, e1]
        · have : mp3 blk (p + 1) = false := by simp [mp3, shl]; omega
          simp [this, e1]
      · simp [h1]
    · by_cases h1 : p < 64
      · have := chk_eq hS p h1
        rw [hb] at this
        cases h : mp3 blk p
        · rfl
        · have := mc4_of_mp3 (blk := blk) _ h; simp_all
      · simp [mp3, shl, h1]
    · simp; omega

theorem mend4_lt (hS : Struct blk) {p : Nat} (h : mend4 blk p = true) : p < blk.length := by
  rw [← mend4_in hS] at h
  simp only [andB, bM, Bool.and_eq_true, decide_eq_true_eq] at h; exact h.2

theorem mend3_eq (h64 : blk.length ≤ 64) (h4 : anyB (m4 blk) = false) : mend3 blk = mend4 blk := by
  have hmp3 : ∀ p, mp3 blk p = false := by
    intro p; unfold mp3 shl; rw [m4_false_of h64 h4]; simp
  funext p
  simp [mend3, mend4, shr, hmp3]

theorem selPos_at {m : Mask} {e : Nat} (he : e < 64) (hm : m e = true) : selPos m (cntB m e) = e := by
  unfold selPos
  cases h : (List.range 64).find? (fun p => m p && cntB m p == cntB m e) with
  | none =>
    have := List.find?_eq_none.mp h e (List.mem_range.mpr he)
    simp [hm] at this
  | some p =>
    have hp := List.find?_some h
    simp only [Bool.and_eq_true, beq_iff_eq] at hp
    simp only [Option.getD_some]
    exact rank_inj m hp.1 hm hp.2

theorem struct_C (hS : Struct blk) {p : Nat} (hp : p < 64)
    (h : mcW (clm blk p 3) (clm blk p 2) (clm blk p 1) = true) : cls blk p = .C := by
  have := struct_at _ hS p hp
  simp only [chkW, h] at this
  simpa using this

theorem clm_eq {p d : Nat} (h : d ≤ p) : clm blk p d = cls blk (p - d) := by
  unfold clm; rw [ite_eq_left h]

/-- A character that starts before a boundary ends before it. -/
theorem fits {c : Nat} (hb : Bnd blk c) {q : Nat} (hq : q < c) :
    q + (cls blk q).len ≤ c := by
  unfold Bnd mcW at hb
  simp only [Bool.or_eq_false_iff] at hb
  obtain ⟨⟨h1, h2⟩, h3⟩ := hb
  refine Classical.byContradiction fun hlt => ?_
  have hcases : c = q + 1 ∨ c = q + 2 ∨ c = q + 3 := by
    have : (cls blk q).len ≤ 4 := by cases cls blk q <;> simp [Cls.len]
    omega
  rcases hcases with e | e | e
  · rw [clm_eq (by omega), show c - 1 = q by omega] at h1
    revert h1 hlt; cases cls blk q <;> simp [Cls.m234, Cls.len] <;> omega
  · rw [clm_eq (by omega), show c - 2 = q by omega] at h2
    revert h2 hlt; cases cls blk q <;> simp [Cls.m34, Cls.len] <;> omega
  · rw [clm_eq (by omega), show c - 3 = q by omega] at h3
    revert h3 hlt; cases cls blk q <;> simp [Cls.m4, Cls.len] <;> omega

/-- The bytes after a lead are continuation bytes (the structure check). -/
theorem conts_after (hS : Struct blk) {q : Nat} (hq : q + (cls blk q).len ≤ 64) :
    ∀ j, 1 ≤ j → j < (cls blk q).len → cls blk (q + j) = .C := by
  intro j h1 h2
  apply struct_C hS (by omega)
  have hl : (cls blk q).len ≤ 4 := by cases cls blk q <;> simp [Cls.len]
  have : j = 1 ∨ j = 2 ∨ j = 3 := by omega
  rcases this with rfl | rfl | rfl
  · rw [show clm blk (q + 1) 1 = cls blk q by rw [clm_eq (by omega)]; simp]
    revert h2; cases cls blk q <;> simp [mcW, Cls.m234, Cls.len]
  · rw [show clm blk (q + 2) 2 = cls blk q by rw [clm_eq (by omega)]; simp]
    revert h2; cases cls blk q <;> simp [mcW, Cls.m34, Cls.len]
  · rw [show clm blk (q + 3) 3 = cls blk q by rw [clm_eq (by omega)]; simp]
    revert h2; cases cls blk q <;> simp [mcW, Cls.m4, Cls.len]

theorem cls_C_lt {p : Nat} (h : cls blk p = .C) : p < blk.length := by
  refine Classical.byContradiction fun hp => ?_
  rw [cls_ge _ _ (by omega)] at h; cases h

theorem byte_C {p : Nat} (h : cls blk p = .C) : 0x80 ≤ byte blk p ∧ byte blk p < 0xC0 := by
  have hp := cls_C_lt h
  rw [cls_lt _ _ hp] at h
  rcases clsB_eq (byte blk p) with ⟨_, e⟩ | ⟨h1, h2, _⟩ | ⟨_, _, e⟩ | ⟨_, _, e⟩ | ⟨_, e⟩
  all_goals first | exact ⟨h1, h2⟩ | (rw [e] at h; cases h)

theorem cleared_C {p : Nat} (h : cls blk p = .C) : cleared blk p = byte blk p &&& 0x3F := by
  have hp := cls_C_lt h
  have hb := byte_C h
  simp [cleared, notAscii, m1, hp]; omega

theorem cleared_lead {p : Nat} (hp : p < blk.length) (h : 0xC0 ≤ byte blk p) :
    cleared blk p = byte blk p &&& 0x3F := by
  simp [cleared, notAscii, m1, hp]; omega

/-- The value of unit `cntB m e` (`tsl`) when `e` ends a character of three
bytes, or the first three bytes of a four-byte one (lines 165-204). -/
theorem tsl_at {m : Mask} {e : Nat} (he : e < 64) (h2 : 2 ≤ e) (hm : m e = true)
    (hc1 : cls blk (e - 1) = .C) (hc0 : cls blk e = .C) (h34 : m34 blk (e - 2) = true) :
    tsl blk m (cntB m e) = tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) := by
  have hl2 : e - 2 < blk.length := by
    simp only [m34, Bool.and_eq_true, decide_eq_true_eq] at h34; exact h34.1.2
  have hb2 : 0xC0 ≤ byte blk (e - 2) := by
    simp only [m34, Bool.and_eq_true, decide_eq_true_eq] at h34; omega
  unfold tsl
  simp only [selPos_at he hm]
  rw [show (e + 63) % 64 = e - 1 by omega, show (e + 62) % 64 = e - 2 by omega]
  have hna : shr (notAscii blk) 1 (e - 1) = true := by
    have hl := cls_C_lt hc0
    have hb := byte_C hc0
    simp [shr, notAscii, m1, hl, show e - 1 + 1 = e by omega]; omega
  rw [hna, h34]
  simp only [ite_true]
  rw [cleared_C hc0, cleared_C hc1, cleared_lead hl2 hb2]
  rfl


theorem m34_of_cls {p : Nat} (hp : p < 62) (h : (cls blk p).m34 = true) : m34 blk p = true := by
  rw [m34_cls _ _ hp]; exact h

theorem cls_byte {p : Nat} (h : cls blk p ≠ .Z) : p < blk.length ∧ cls blk p = clsB (byte blk p) := by
  by_cases hp : p < blk.length
  · exact ⟨hp, cls_lt _ _ hp⟩
  · exact absurd (cls_ge _ _ (by omega)) h

/-- The rank of the unit following a high surrogate. -/
theorem mend4_hi (hS : Struct blk) {q : Nat} (hq : q + 3 < 64) (h4 : cls blk q = .L4) :
    mend4 blk (q + 2) = true ∧ mend4 blk (q + 3) = true ∧ mp3 blk (q + 3) = true ∧
      cntB (mend4 blk) (q + 3) = cntB (mend4 blk) (q + 2) + 1 := by
  have hm4 : m4 blk q = true := by rw [m4_cls]; rw [h4]; rfl
  have hmp : mp3 blk (q + 3) = true := by
    simp [mp3, shl, hq, hm4]
  have h2 : mend4 blk (q + 2) = true := by
    simp [mend4, shr, show q + 2 < 64 by omega, show q + 2 + 1 = q + 3 by omega, hmp]
  refine ⟨h2, by simp [mend4, hmp], hmp, ?_⟩
  rw [show q + 3 = q + 2 + 1 by omega, cntB_succ_of _ _ h2]

/-- **A three-byte character's unit** (`e` its last byte, `q = e - 2` its
lead): it is in `M3`, its value is the code point, and on the 4-byte path it is
neither a high nor a low surrogate. -/
theorem unit3 (hS : Struct blk) {e : Nat} (he : e < 64) (h2 : 2 ≤ e) (hme : mend4 blk e = true)
    (hL3 : cls blk (e - 2) = .L3) (h62 : e - 2 < 62) :
    bigM3 blk (mend4 blk) (cntB (mend4 blk) e) = true ∧
    pext (mp3 blk) (mend4 blk) (cntB (mend4 blk) e) = false ∧
    pext (mp3 blk) (mend4 blk) (cntB (mend4 blk) e + 1) = false ∧
    tsl blk (mend4 blk) (cntB (mend4 blk) e) =
      tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) := by
  have hca := conts_after hS (q := e - 2) (by rw [hL3]; simp [Cls.len]; omega)
  have hc1 : cls blk (e - 1) = .C := by
    have := hca 1 (by omega) (by rw [hL3]; simp [Cls.len]); rwa [show e - 2 + 1 = e - 1 by omega] at this
  have hc0 : cls blk e = .C := by
    have := hca 2 (by omega) (by rw [hL3]; simp [Cls.len]); rwa [show e - 2 + 2 = e by omega] at this
  have h34 : m34 blk (e - 2) = true := m34_of_cls h62 (by rw [hL3]; rfl)
  have hn4 : m4 blk (e - 2) = false := by rw [m4_cls, hL3]; rfl
  have hb2 : bM blk (e - 2) = true := by
    simp only [m34, Bool.and_eq_true, decide_eq_true_eq] at h34; simp [bM, h34.1.2]
  refine ⟨?_, ?_, ?_, tsl_at he h2 hme hc1 hc0 h34⟩
  · unfold bigM3
    rw [pext_at _ _ e he hme]
    simp [shl, he, h2, h34, hb2, hn4]
  · rw [pext_at _ _ e he hme]
    simp only [mp3, shl, Bool.and_eq_false_iff, decide_eq_false_iff_not]
    by_cases h3 : 3 ≤ e
    · right
      cases hm : m4 blk (e - 3)
      · rfl
      · exfalso
        have : cls blk (e - 2) = .C := by
          apply struct_C hS (by omega)
          rw [clm_eq (blk := blk) (p := e - 2) (d := 1) (by omega), show e - 2 - 1 = e - 3 by omega]
          rw [m4_cls] at hm
          revert hm; cases cls blk (e - 3) <;> simp [mcW, Cls.m4, Cls.m234]
        rw [hL3] at this; cases this
    · simp [h3]
  · cases hp : pext (mp3 blk) (mend4 blk) (cntB (mend4 blk) e + 1)
    · rfl
    · exfalso
      obtain ⟨e', he', hme', hce', hm3'⟩ := pext_true _ _ _ hp
      obtain ⟨h3', -, h4'⟩ := mp3_spec e' hm3'
      have hm1 : mend4 blk (e' - 1) = true := by
        simp [mend4, shr, show e' - 1 < 64 by omega, show e' - 1 + 1 = e' by omega, hm3']
      have : cntB (mend4 blk) (e' - 1) = cntB (mend4 blk) e := by
        have := cntB_succ_of (mend4 blk) (e' - 1) hm1
        rw [show e' - 1 + 1 = e' by omega] at this; omega
      have hee := rank_inj _ hm1 hme this
      rw [show e' - 3 = e - 2 by omega, hn4] at h4'
      cases h4'

/-- **The high surrogate of a four-byte character** at `q`: its unit (at byte
`q + 2`) is flagged in `Mhi` and its value is `(tval >> 4) + 0xD7C0`. -/
theorem unit4 (hS : Struct blk) {q : Nat} (hq : q + 3 < 64) (hL4 : cls blk q = .L4) :
    mend4 blk (q + 2) = true ∧
    pext (mp3 blk) (mend4 blk) (cntB (mend4 blk) (q + 2) + 1) = true ∧
    wout4 blk (cntB (mend4 blk) (q + 2)) =
      ((tval (byte blk q) (byte blk (q + 1)) (byte blk (q + 2)) >>> 4) + 0xD7C0) % 65536 := by
  obtain ⟨h2, h3, hmp, hc⟩ := mend4_hi hS hq hL4
  have hca := conts_after hS (q := q) (by rw [hL4]; simp [Cls.len]; omega)
  have hc1 : cls blk (q + 1) = .C := hca 1 (by omega) (by rw [hL4]; simp [Cls.len])
  have hc2 : cls blk (q + 2) = .C := hca 2 (by omega) (by rw [hL4]; simp [Cls.len])
  have h34 : m34 blk q = true := m34_of_cls (by omega) (by rw [hL4]; rfl)
  have hhi : pext (mp3 blk) (mend4 blk) (cntB (mend4 blk) (q + 2) + 1) = true := by
    rw [← hc, pext_at _ _ _ hq h3, hmp]
  have ht := tsl_at (blk := blk) (m := mend4 blk) (e := q + 2) (by omega) (by omega) h2
    (by rw [show q + 2 - 1 = q + 1 by omega]; exact hc1) hc2 (by rw [show q + 2 - 2 = q by omega]; exact h34)
  rw [show q + 2 - 2 = q by omega, show q + 2 - 1 = q + 1 by omega] at ht
  refine ⟨h2, hhi, ?_⟩
  unfold wout4
  simp only [hhi, ite_true, ht]

theorem mout_ones : mout blk = ones (if mhi31 blk then 31 else 32) := by
  funext r
  cases h : mhi31 blk
  · simp [mout, ones, h]
  · simp only [mout, ones, h, Bool.and_true, ite_true]
    by_cases h1 : r < 31
    · simp [h1, show r < 32 by omega, show r ≠ 31 by omega]
    · simp [h1]; omega

/-- A processed unit has rank below 32. -/
theorem rank_lt32 {src M : Mask} (hsrc : ∀ r, src r = true → r < 32) {e : Nat}
    (he : e < top (pdep src M)) : cntB M e < 32 := by
  have hs := top_set (pdep src M) (by omega)
  have h1 := hsrc _ (pdep_src _ _ _ hs)
  have := cntB_mono M (show e ≤ top (pdep src M) - 1 by omega)
  omega

/-- The last byte of a character inside the consumed bytes ends a unit. -/
theorem mend_at (hS : Struct blk) (h64 : blk.length ≤ 64) {src : Mask} {e : Nat}
    (he : e < top (pdep src (andB (mend4 blk) (bM blk))))
    (hn : e + 1 < blk.length → (cls blk (e + 1) == .C) = false) : mend4 blk e = true := by
  have htop := top_le (pdep src (andB (mend4 blk) (bM blk)))
  have hsub : ∀ p, pdep src (andB (mend4 blk) (bM blk)) p = true → p < blk.length := by
    intro p hp
    have := pdep_sub _ _ _ hp
    simp only [andB, bM, Bool.and_eq_true, decide_eq_true_eq] at this; exact this.2
  have hc := top_le_len _ hsub
  by_cases h1 : e + 1 < blk.length
  · have hm : m1234 blk (e + 1) = true := by
      have := hn h1
      rw [m1234_cls, cls_lt _ _ h1]
      rw [cls_lt _ _ h1] at this
      revert this
      have := clsB_ne_Z (byte blk (e + 1))
      revert this
      cases clsB (byte blk (e + 1)) <;> simp [Cls.m1234]
    simp [mend4, shr, show e < 64 by omega, hm]
  · by_cases hl : blk.length < 64
    · simp [mend4, tailbit, hl]; omega
    · have he63 : e = 63 := by omega
      have hs := top_set (pdep src (andB (mend4 blk) (bM blk))) (by omega)
      rw [show top (pdep src (andB (mend4 blk) (bM blk))) - 1 = e by omega] at hs
      have := pdep_sub _ _ _ hs
      rw [mend4_in hS] at this; exact this

end Units

/-! ## Counts -/

section Counts
variable {blk : List Nat}

/-- **The exact potential.** Below any `k` (byte 63 of a full block only if it
ends a unit), the units plus the debt of the character in progress are
`utf16_length_from_utf8` of the first `k` bytes. -/
theorem pot_exact (hS : Struct blk) (h64 : blk.length ≤ 64) :
    ∀ k, k ≤ blk.length → (k < 64 ∨ mend4 blk 63 = true) →
      cons3 (clm blk k 3) (clm blk k 2) (clm blk k 1) = true ∧
      cntB (andB (mend4 blk) (bM blk)) k + phi (clm blk k 3) (clm blk k 2) (clm blk k 1)
        = L ((blk.take k).map w16) := by
  intro k
  induction k with
  | zero =>
    intro _ _
    simp [clm, cons3_start, cntB, phi]
  | succ k ih =>
    intro hk h63
    obtain ⟨hc, hi⟩ := ih (by omega) (Or.inl (by omega))
    have hk' : k < blk.length := by omega
    have s1 : clm blk (k + 1) 3 = clm blk k 2 := clm_succ _ _ _
    have s2 : clm blk (k + 1) 2 = clm blk k 1 := clm_succ _ _ _
    have s3 : clm blk (k + 1) 1 = cls blk k := by rw [clm_succ, clm_zero]
    have hpre := winAt hS h64 hc hk'
    obtain ⟨-, -, -, hc'⟩ := winPost_spec (win hpre)
    have hfull : pfOf blk k = .full →
        mendW (pfOf blk k) (clm blk k 3) (clm blk k 2) (cls blk (k + 1)) = true := by
      intro hf
      have hk63 : k = 63 := by
        unfold pfOf at hf
        by_cases hin : k + 1 < blk.length
        · simp [hin] at hf
        · rw [ite_eq_right hin] at hf
          split at hf
          · omega
          · cases hf
      subst hk63
      rw [← mend4_cls h64 _ hk']
      rcases h63 with h | h
      · omega
      · exact h
    have hx := exact_at hpre hfull
    refine ⟨by rw [s1, s2, s3]; exact hc', ?_⟩
    rw [s1, s2, s3, L_take_succ _ _ _ hk']
    simp only [cntB]
    rw [andB_bM _ _ hk', mend4_cls h64 _ hk']
    have hw16 : (cls blk k).w = w16 (blk.getD k 0) := by rw [cls_lt _ _ hk', clsB_w]; rfl
    omega

/-- A mask with a set bit below `n` has a lowest one, of rank 0. -/
theorem first_bit (M : Mask) : ∀ n, 0 < cntB M n → ∃ p, p < n ∧ M p = true ∧ cntB M p = 0 := by
  intro n
  induction n with
  | zero => intro h; simp [cntB] at h
  | succ n ih =>
    intro h
    by_cases h0 : 0 < cntB M n
    · obtain ⟨p, hp, h1, h2⟩ := ih h0; exact ⟨p, by omega, h1, h2⟩
    · simp only [cntB] at h
      cases hm : M n
      · rw [hm] at h; simp at h; omega
      · exact ⟨n, by omega, hm, by omega⟩

theorem top_pos {src M : Mask} (h0 : src 0 = true) {n : Nat} (hn : n ≤ 64) (h : 0 < cntB M n) :
    1 ≤ top (pdep src M) := by
  obtain ⟨p, hp, h1, h2⟩ := first_bit M n h
  have : pdep src M p = true := by simp [pdep, show p < 64 by omega, h1, h2, h0]
  have := lt_top _ p (by omega) this
  omega

theorem bnd_len (hS : Struct blk) (hl : blk.length < 64) : Bnd blk blk.length := by
  have := struct_at _ hS blk.length hl
  rw [cls_ge _ _ (Nat.le_refl _)] at this
  unfold Bnd
  simpa [chkW] using this

/-- The first character ends at a boundary, inside the block, and weighs at
least one unit. -/
theorem bnd_first (hS : Struct blk) (h64 : blk.length ≤ 64) (hne : 0 < blk.length) :
    Bnd blk (cls blk 0).len ∧ (cls blk 0).len ≤ blk.length ∧ 1 ≤ (cls blk 0).len ∧
      1 ≤ L ((blk.take (cls blk 0).len).map w16) := by
  have h0 : cls blk 0 = clsB (byte blk 0) := cls_lt _ _ hne
  have hz : cls blk 0 ≠ .Z := by rw [h0]; exact clsB_ne_Z _
  have hnc : cls blk 0 ≠ .C := by
    have := cls_zero_ne_C hS; intro h; rw [h] at this; simp at this
  have hl4 : (cls blk 0).len ≤ 4 := by cases cls blk 0 <;> simp [Cls.len]
  have hl1 : 1 ≤ (cls blk 0).len := by
    revert hz hnc; cases cls blk 0 <;> simp [Cls.len]
  have hca := conts_after hS (q := 0) (by omega)
  simp only [Nat.zero_add] at hca
  have hb : Bnd blk (cls blk 0).len := by
    unfold Bnd
    generalize hc0 : cls blk 0 = c0 at hca hl4 hl1 hz hnc
    cases c0 <;> simp only [Cls.len] at hca ⊢
    all_goals first | exact absurd rfl hz | exact absurd rfl hnc | skip
    · simp [clm, hc0, mcW, Cls.m234, Cls.m34, Cls.m4]
    · simp [clm, hca 1 (by omega) (by omega), hc0, mcW, Cls.m234, Cls.m34, Cls.m4]
    · simp [clm, hca 1 (by omega) (by omega), hca 2 (by omega) (by omega), hc0, mcW, Cls.m234,
        Cls.m34, Cls.m4]
    · simp [clm, hca 1 (by omega) (by omega), hca 2 (by omega) (by omega), hca 3 (by omega) (by omega),
        mcW, Cls.m234, Cls.m34, Cls.m4]
  have hle : (cls blk 0).len ≤ blk.length := by
    by_cases hl : blk.length < 64
    · have := fits (bnd_len hS hl) (q := 0) hne; omega
    · omega
  refine ⟨hb, hle, hl1, ?_⟩
  have h1 := L_take_mono blk w16 hl1
  rw [L_take_succ _ _ _ hne] at h1
  have : 1 ≤ w16 (blk.getD 0 0) := by
    rw [← clsB_w]; show 1 ≤ (clsB (byte blk 0)).w
    rw [← h0]; revert hz hnc; cases cls blk 0 <;> simp [Cls.w]
  have h2 : L ((blk.take 0).map w16) = 0 := by simp [L]
  omega

/-- Below a boundary, the 2-byte path sees no three- or four-byte lead. -/
theorem no34_below (h : anyB (m34 blk) = false) {c : Nat} (hc : c ≤ 64) (hb : Bnd blk c) :
    ∀ p, p < c → (cls blk p).m34 = false := by
  intro p hp
  have hf := fits hb hp
  cases e : (cls blk p).m34
  · rfl
  · exfalso
    have : 3 ≤ (cls blk p).len := by revert e; cases cls blk p <;> simp [Cls.m34, Cls.len]
    have := (no34 h p (by omega)).1
    rw [e] at this; cases this

theorem L_m1234 : ∀ k, k ≤ blk.length → (∀ p, p < k → (cls blk p).m34 = false) →
    L ((blk.take k).map w16) = cntB (m1234 blk) k := by
  intro k
  induction k with
  | zero => intro _ _; simp [cntB]
  | succ k ih =>
    intro hk h
    have hk' : k < blk.length := by omega
    rw [L_take_succ _ _ _ hk', ih (by omega) (fun p hp => h p (by omega))]
    simp only [cntB]
    have hw : w16 (blk.getD k 0) = (cls blk k).w := by rw [cls_lt _ _ hk', clsB_w]; rfl
    rw [hw, m1234_cls]
    have := h k (by omega)
    revert this; cases cls blk k <;> simp [Cls.m34, Cls.w, Cls.m1234]

end Counts

/-! ## Validity of the consumed bytes -/

section Valid34
variable {blk : List Nat}

theorem cls_range {p : Nat} (hp : p < blk.length) :
    (cls blk p = .A ↔ byte blk p < 0x80) ∧ (cls blk p = .C ↔ 0x80 ≤ byte blk p ∧ byte blk p < 0xC0) ∧
    (cls blk p = .L2 ↔ 0xC0 ≤ byte blk p ∧ byte blk p < 0xE0) ∧
    (cls blk p = .L3 ↔ 0xE0 ≤ byte blk p ∧ byte blk p < 0xF0) ∧ (cls blk p = .L4 ↔ 0xF0 ≤ byte blk p) := by
  rw [cls_lt _ _ hp]
  rcases clsB_eq (byte blk p) with ⟨h1, e⟩ | ⟨h1, h2, e⟩ | ⟨h1, h2, e⟩ | ⟨h1, h2, e⟩ | ⟨h1, e⟩ <;>
    rw [e] <;> simp <;> omega

theorem cls_ne_Z_lt {p : Nat} (h : cls blk p ≠ .Z) : p < blk.length := (cls_byte h).1

/-- The checks the 3- and 4-byte paths make, read on the characters: the
consumed bytes are valid UTF-8. -/
theorem valid34 (hS : Struct blk) (h64 : blk.length ≤ 64) (hB : ∀ p, byte blk p < 256)
    (hm : anyB (milltwo blk) = false) {c : Nat} (hcl : c ≤ blk.length) (hb : Bnd blk c)
    (U3 : ∀ e, e < c → 2 ≤ e → cls blk (e - 2) = .L3 →
      (decide (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) < 0x800) ||
        decide (sub16 (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e)) 0xD800 < 0x800)) = false)
    (U4 : ∀ q, q + 3 < c → cls blk q = .L4 →
      decide (0x400 ≤ sub16 (((tval (byte blk q) (byte blk (q + 1)) (byte blk (q + 2)) >>> 4) + 0xD7C0)
        % 65536) 0xD800) = false) :
    run 0 (blk.take c) = 0 := by
  have r3 : ∀ q, q < c → cls blk q = .L3 → rngT (tagB (byte blk q)) (byte blk (q + 1)) = true := by
    intro q hq h3
    have hf := fits hb hq
    rw [h3] at hf; simp only [Cls.len] at hf
    have hca := conts_after hS (q := q) (by rw [h3]; simp [Cls.len]; omega)
    have c1 := byte_C (hca 1 (by omega) (by rw [h3]; simp [Cls.len]))
    have c2 := byte_C (hca 2 (by omega) (by rw [h3]; simp [Cls.len]))
    have hb0 := ((cls_range (blk := blk) (p := q) (by omega)).2.2.2.1).mp h3
    have := U3 (q + 2) (by omega) (by omega) (by rw [show q + 2 - 2 = q by omega]; exact h3)
    rw [show q + 2 - 2 = q by omega, show q + 2 - 1 = q + 1 by omega] at this
    have e := chk3_at hb0.1 hb0.2 c1.1 c1.2 c2.1 c2.2
    rw [this] at e; simp at e; exact e
  have r4 : ∀ q, q < c → cls blk q = .L4 →
      leadOK (byte blk q) = true ∧ rngT (tagB (byte blk q)) (byte blk (q + 1)) = true := by
    intro q hq h4
    have hf := fits hb hq
    rw [h4] at hf; simp only [Cls.len] at hf
    have hca := conts_after hS (q := q) (by rw [h4]; simp [Cls.len]; omega)
    have c1 := byte_C (hca 1 (by omega) (by rw [h4]; simp [Cls.len]))
    have c2 := byte_C (hca 2 (by omega) (by rw [h4]; simp [Cls.len]))
    have hb0 := ((cls_range (blk := blk) (p := q) (by omega)).2.2.2.2).mp h4
    have := U4 q (by omega) h4
    have e := chk4_at hb0 (hB q) c1.1 c1.2 c2.1 c2.2
    rw [this] at e; simp at e; exact ⟨e.1, e.2⟩
  apply valid_of_checks hS h64 hcl _ _ hb
  · intro k hk hC
    have hk' : k < blk.length := by omega
    have hr := cls_range (blk := blk) hk'
    by_cases h4 : cls blk k = .L4
    · exact (r4 k hk h4).1
    · have hmk := anyB_false hm k (by omega)
      simp only [milltwo, m234, Bool.and_eq_false_iff, decide_eq_false_iff_not] at hmk
      have hnC : cls blk k ≠ .C := by intro h; rw [h] at hC; simp at hC
      have hlt : byte blk k < 0xF0 := by
        refine Classical.byContradiction fun h => h4 (hr.2.2.2.2.mpr (by omega))
      simp only [leadOK, Bool.and_eq_true, Bool.not_eq_true', decide_eq_false_iff_not,
        decide_eq_true_eq]
      constructor
      · intro ⟨h1, h2⟩
        rcases hmk with (h | h) | h
        · simp [hk'] at h
        · exact h h1
        · exact h h2
      · omega
  · intro k h1 hk
    cases ht : tagB (byte blk (k - 1)) with
    | zero => simp [rngT]
    | succ t =>
      have hg := tagB_ge (byte blk (k - 1)) (by omega)
      have hr := cls_range (blk := blk) (p := k - 1) (by omega)
      have e : k - 1 + 1 = k := by omega
      rw [← ht]
      by_cases h3 : byte blk (k - 1) < 0xF0
      · have := r3 (k - 1) (by omega) (hr.2.2.2.1.mpr ⟨hg.1, h3⟩); rwa [e] at this
      · have := (r4 (k - 1) (by omega) (hr.2.2.2.2.mpr (by omega))).2; rwa [e] at this

end Valid34

/-! ## The range checks, read on the characters -/

section Checks
variable {blk : List Nat}

/-- The last byte of a three-byte character is followed by a byte that is not
a continuation byte. -/
theorem after3 (hS : Struct blk) {e : Nat} (h2 : 2 ≤ e) (hL3 : cls blk (e - 2) = .L3)
    (he : e + 1 < 64) : (e + 1 < blk.length → (cls blk (e + 1) == .C) = false) ∧
      cls blk (e - 1) = .C ∧ cls blk e = .C := by
  have hca := conts_after hS (q := e - 2) (by rw [hL3]; simp [Cls.len]; omega)
  have hc1 : cls blk (e - 1) = .C := by
    have := hca 1 (by omega) (by rw [hL3]; simp [Cls.len]); rwa [show e - 2 + 1 = e - 1 by omega] at this
  have hc0 : cls blk e = .C := by
    have := hca 2 (by omega) (by rw [hL3]; simp [Cls.len]); rwa [show e - 2 + 2 = e by omega] at this
  refine ⟨fun _ => ?_, hc1, hc0⟩
  have := struct_at _ hS (e + 1) he
  rw [clm_eq (by omega), clm_eq (by omega), clm_eq (by omega)] at this
  rw [show e + 1 - 3 = e - 2 by omega, show e + 1 - 2 = e - 1 by omega, show e + 1 - 1 = e by omega,
    hL3, hc1, hc0] at this
  simpa [chkW, mcW, Cls.m234, Cls.m34, Cls.m4] using this

theorem cntB_M (hS : Struct blk) (n : Nat) :
    cntB (andB (mend4 blk) (bM blk)) n = cntB (mend4 blk) n :=
  cntB_congr (fun p _ => mend4_in hS p)

theorem u3_of_fail4 (hS : Struct blk) (h64 : blk.length ≤ 64) (hf4 : fail4 blk = false) {src : Mask}
    (hsrc : ∀ r, src r = true → r < 32) :
    ∀ e, e < top (pdep src (andB (mend4 blk) (bM blk))) → 2 ≤ e → cls blk (e - 2) = .L3 →
      (decide (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) < 0x800) ||
        decide (sub16 (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e)) 0xD800 < 0x800)) = false := by
  intro e he h2 hL3
  have htop := top_le (pdep src (andB (mend4 blk) (bM blk)))
  have hsub : ∀ p, pdep src (andB (mend4 blk) (bM blk)) p = true → p < blk.length := by
    intro p hp
    have := pdep_sub _ _ _ hp
    simp only [andB, bM, Bool.and_eq_true, decide_eq_true_eq] at this; exact this.2
  have hc := top_le_len _ hsub
  have hme : mend4 blk e = true := by
    apply mend_at hS h64 he
    intro hl; exact (after3 hS h2 hL3 (by omega)).1 hl
  obtain ⟨hbig, hlo, hhi, ht⟩ := unit3 hS (by omega) h2 hme hL3 (by omega)
  have hk := rank_lt32 hsrc he
  rw [cntB_M hS] at hk
  have := List.any_eq_false.mp hf4 _ (List.mem_range.mpr hk)
  have hw : wout4 blk (cntB (mend4 blk) e) = tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) := by
    unfold wout4; simp only [hhi, hlo, ht]; simp
  rw [hw, hbig, hhi] at this
  simpa using this

theorem u4_of_fail4 (hS : Struct blk) (h64 : blk.length ≤ 64) (hf4 : fail4 blk = false) {src : Mask}
    (hsrc : ∀ r, src r = true → r < 32) :
    ∀ q, q + 3 < top (pdep src (andB (mend4 blk) (bM blk))) → cls blk q = .L4 →
      decide (0x400 ≤ sub16 (((tval (byte blk q) (byte blk (q + 1)) (byte blk (q + 2)) >>> 4) + 0xD7C0)
        % 65536) 0xD800) = false := by
  intro q hq h4
  have htop := top_le (pdep src (andB (mend4 blk) (bM blk)))
  obtain ⟨h2, hhi, hw⟩ := unit4 hS (by omega) h4
  have hk := rank_lt32 (M := andB (mend4 blk) (bM blk)) (e := q + 2) hsrc (by omega)
  rw [cntB_M hS] at hk
  have := List.any_eq_false.mp hf4 _ (List.mem_range.mpr hk)
  rw [hhi, hw] at this
  simp only [Bool.true_and, Bool.or_eq_true, not_or] at this
  simpa using this.1

theorem u3_of_fail3 (hS : Struct blk) (h64 : blk.length ≤ 64) (h4 : anyB (m4 blk) = false)
    (hf3 : fail3 blk = false) :
    ∀ e, e < top (pdep (ones 32) (andB (mend4 blk) (bM blk))) → 2 ≤ e → cls blk (e - 2) = .L3 →
      (decide (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) < 0x800) ||
        decide (sub16 (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e)) 0xD800 < 0x800)) = false := by
  intro e he h2 hL3
  have htop := top_le (pdep (ones 32) (andB (mend4 blk) (bM blk)))
  have hme : mend4 blk e = true := by
    apply mend_at hS h64 he
    intro hl; exact (after3 hS h2 hL3 (by omega)).1 hl
  obtain ⟨hbig, -, -, ht⟩ := unit3 hS (by omega) h2 hme hL3 (by omega)
  have hk := rank_lt32 (fun r h => by simpa [ones] using h) he
  rw [cntB_M hS] at hk
  unfold fail3 at hf3
  rw [mend3_eq h64 h4] at hf3
  have := List.any_eq_false.mp hf3 _ (List.mem_range.mpr hk)
  rw [hbig, ht] at this
  simpa using this

end Checks

/-! ## One block, with the checks: soundness -/

section Sound
variable {blk : List Nat}

theorem sound34 (hS : Struct blk) (h64 : blk.length ≤ 64) (hB : ∀ p, byte blk p < 256)
    (hm : anyB (milltwo blk) = false) {j : Nat} (hj : 31 ≤ j)
    (hb : Bnd blk (top (pdep (ones j) (andB (mend4 blk) (bM blk)))))
    (U3 : ∀ e, e < top (pdep (ones j) (andB (mend4 blk) (bM blk))) → 2 ≤ e → cls blk (e - 2) = .L3 →
      (decide (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) < 0x800) ||
        decide (sub16 (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e)) 0xD800 < 0x800)) = false)
    (U4 : ∀ q, q + 3 < top (pdep (ones j) (andB (mend4 blk) (bM blk))) → cls blk q = .L4 →
      decide (0x400 ≤ sub16 (((tval (byte blk q) (byte blk (q + 1)) (byte blk (q + 2)) >>> 4) + 0xD7C0)
        % 65536) 0xD800) = false) :
    top (pdep (ones j) (andB (mend4 blk) (bM blk))) ≤ blk.length ∧
    run 0 (blk.take (top (pdep (ones j) (andB (mend4 blk) (bM blk))))) = 0 ∧
    popcnt (pdep (ones j) (andB (mend4 blk) (bM blk))) =
      L ((blk.take (top (pdep (ones j) (andB (mend4 blk) (bM blk))))).map w16) ∧
    (0 < blk.length → 1 ≤ top (pdep (ones j) (andB (mend4 blk) (bM blk)))) := by
  generalize hM : andB (mend4 blk) (bM blk) = M at *
  have hsub : ∀ p, pdep (ones j) M p = true → p < blk.length := by
    intro p hp
    have := pdep_sub _ _ _ hp
    rw [← hM] at this
    simp only [andB, bM, Bool.and_eq_true, decide_eq_true_eq] at this; exact this.2
  have hc := top_le_len _ hsub
  have htop := top_le (pdep (ones j) M)
  refine ⟨hc, valid34 hS h64 hB hm hc hb U3 U4, ?_, ?_⟩
  · rw [popcnt_pdep_ones, ← cntB_top_pdep_ones]
    have h63 : top (pdep (ones j) M) < 64 ∨ mend4 blk 63 = true := by
      by_cases h : top (pdep (ones j) M) < 64
      · exact Or.inl h
      · right
        have hs := top_set (pdep (ones j) M) (by omega)
        rw [show top (pdep (ones j) M) - 1 = 63 by omega] at hs
        have := pdep_sub _ _ _ hs
        rw [← hM, mend4_in hS] at this; exact this
    have := (pot_exact hS h64 _ hc h63).2
    rw [hM, phi0_at hb] at this
    omega
  · intro hne
    obtain ⟨hb1, hle, hl1, hL1⟩ := bnd_first hS h64 hne
    have hl4 : (cls blk 0).len ≤ 4 := by cases cls blk 0 <;> simp [Cls.len]
    have := (pot_exact hS h64 _ hle (Or.inl (by omega))).2
    rw [hM, phi0_at hb1] at this
    exact top_pos (by simp [ones]; omega) (by omega : (cls blk 0).len ≤ 64) (by omega)

theorem ascii_cls (h : (List.range 64).all (fun p => !bM blk p || m1 blk p) = true)
    (h64 : blk.length ≤ 64) (p : Nat) (hp : p < blk.length) : cls blk p = .A := by
  have := ascii_test h h64 p hp
  exact ((cls_range hp).1).mpr this

/-- **One block of `process_block_utf8_to_utf16` with the checks is sound.**
If it returns `true`, the `c` bytes it consumes are valid UTF-8 and the `n`
units it stores are their `utf16_length_from_utf8`; it stops where `EndOK`
says, and a nonempty block consumes at least one byte. -/
theorem block_sound (h64 : blk.length ≤ 64) (hB : ∀ p, byte blk p < 256) {c n : Nat}
    (h : block16 true blk = some (c, n)) :
    c ≤ blk.length ∧ EndOK blk c ∧ run 0 (blk.take c) = 0 ∧ n = L ((blk.take c).map w16) ∧
      (0 < blk.length → 1 ≤ c) := by
  obtain ⟨hf, hS, hm⟩ := block16_true h64 h
  obtain ⟨hc, -, he⟩ := block16_core h64 hS hm hf
  have hb := bnd_of_end hS h64 hc he
  refine ⟨hc, he, ?_⟩
  unfold block16 at h
  by_cases ha : (List.range 64).all (fun p => !bM blk p || m1 blk p) = true
  · rw [ite_eq_left ha] at h
    simp only [Option.some.injEq, Prod.mk.injEq] at h
    obtain ⟨rfl, rfl⟩ := h
    have hA := ascii_cls ha h64
    have h34 : ∀ p, p < blk.length → (cls blk p).m34 = false := by
      intro p hp; rw [hA p hp]; rfl
    refine ⟨valid34 hS h64 hB hm (Nat.le_refl _) hb ?_ ?_, ?_, fun h => h⟩
    · intro e he' h2 h3; rw [hA _ (by omega)] at h3; cases h3
    · intro q hq h4; rw [hA _ (by omega)] at h4; cases h4
    · rw [L_m1234 _ (Nat.le_refl _) h34]
      have : cntB (m1234 blk) blk.length = cntB (fun _ => true) blk.length :=
        cntB_congr (fun p hp => by rw [m1234_cls, hA p hp]; rfl)
      rw [this, cntB_true]
  rw [ite_eq_right ha] at h
  simp only [hm, Bool.and_false, Bool.false_eq_true, ite_false, Bool.true_and] at h
  by_cases h34 : anyB (m34 blk) = true
  · rw [ite_eq_left h34] at h
    by_cases h4 : anyB (m4 blk) = true
    · have hS' : chkB blk (mc4 blk) = true := hS
      simp only [h4, Bool.not_true, Bool.false_eq_true, ite_false, hS'] at h
      cases hf4 : fail4 blk
      · simp only [hf4, Bool.false_eq_true, ite_false, Option.some.injEq, Prod.mk.injEq] at h
        obtain ⟨rfl, rfl⟩ := h
        rw [mout_ones] at hb ⊢
        have hsrc : ∀ r, ones (if mhi31 blk = true then 31 else 32) r = true → r < 32 := by
          intro r hr; simp only [ones, decide_eq_true_eq] at hr; split at hr <;> omega
        obtain ⟨-, v, n, p⟩ := sound34 hS h64 hB hm (j := if mhi31 blk = true then 31 else 32)
          (by split <;> omega) hb (u3_of_fail4 hS h64 hf4 hsrc) (u4_of_fail4 hS h64 hf4 hsrc)
        exact ⟨v, n, p⟩
      · simp [hf4] at h
    · have h4' : anyB (m4 blk) = false := by simpa using h4
      have hS' : chkB blk (mc3 blk) = true := by rw [chk3_eq h64 h4']; exact hS
      simp only [h4', Bool.not_false, ite_true, hS', Bool.not_true, Bool.false_eq_true,
        ite_false] at h
      cases hf3 : fail3 blk
      · simp only [hf3, Bool.false_eq_true, ite_false, Option.some.injEq, Prod.mk.injEq] at h
        obtain ⟨rfl, rfl⟩ := h
        rw [mend3_eq h64 h4'] at hb ⊢
        have U4 : ∀ q, q + 3 < top (pdep (ones 32) (andB (mend4 blk) (bM blk))) → cls blk q = .L4 →
            decide (0x400 ≤ sub16 (((tval (byte blk q) (byte blk (q + 1)) (byte blk (q + 2)) >>> 4)
              + 0xD7C0) % 65536) 0xD800) = false := by
          intro q _ hq4
          have := m4_false_of h64 h4' q
          rw [m4_cls, hq4] at this; cases this
        obtain ⟨-, v, n, p⟩ := sound34 hS h64 hB hm (j := 32) (by omega) hb
          (u3_of_fail3 hS h64 h4' hf3) U4
        exact ⟨v, n, p⟩
      · simp [hf3] at h
  · have h34' : anyB (m34 blk) = false := by simpa using h34
    rw [ite_eq_right h34] at h
    have hS' : chkB blk (shl (m234 blk) 1) = true := by rw [chk2_eq h34']; exact hS
    simp only [hS', Bool.not_true, Bool.false_eq_true, ite_false] at h
    have hno := no34_below h34' (by omega) hb
    have hval : run 0 (blk.take c) = 0 := by
      refine valid34 hS h64 hB hm hc hb ?_ ?_
      · intro e he' h2 h3
        have := hno (e - 2) (by omega); rw [h3] at this; cases this
      · intro q hq h4
        have := hno q (by omega); rw [h4] at this; cases this
    refine ⟨hval, ?_⟩
    rw [L_m1234 _ hc hno]
    by_cases hfull : blk.length = 64
    · rw [ite_eq_left hfull] at h
      simp only [Option.some.injEq, Prod.mk.injEq] at h
      obtain ⟨rfl, rfl⟩ := h
      refine ⟨?_, fun _ => by omega⟩
      cases hb32 : (decide (0x80 ≤ byte blk 32) && decide (byte blk 32 ≤ 0xC0))
      · rfl
      · simp only [ind_true]
        simp only [Bool.and_eq_true, decide_eq_true_eq] at hb32
        have hne : byte blk 32 ≠ 0xC0 := by
          intro e
          have := anyB_false hm 32 (by omega)
          simp [milltwo, m234, e, hfull] at this
        have hn32 : m1234 blk 32 = false := by
          rw [m1234_cls, cls_lt _ _ (by omega)]
          rcases clsB_eq (byte blk 32) with ⟨h1, _⟩ | ⟨_, _, e⟩ | ⟨h1, _, _⟩ | ⟨h1, _, _⟩ | ⟨h1, _⟩
          all_goals first | omega | (rw [e]; rfl)
        rw [cntB_succ_of_not (m1234 blk) 32 hn32]
    · rw [ite_eq_right hfull] at h
      simp only [Option.some.injEq, Prod.mk.injEq] at h
      obtain ⟨rfl, rfl⟩ := h
      have hlt : blk.length < 64 := by omega
      have hd := lead_coa hS h64 h34'
      have hbl := bnd_len hS hlt
      have hend : (clm blk blk.length 1).m234 = false := by
        unfold Bnd mcW at hbl; simp only [Bool.or_eq_false_iff] at hbl; exact hbl.1.1
      have hcm : (clm blk (top (pdep (ones 32) (coa blk))) 1).m234 = false := by
        unfold Bnd mcW at hb; simp only [Bool.or_eq_false_iff] at hb; exact hb.1.1
      refine ⟨?_, ?_⟩
      · have e1 : popcnt (m1234 blk) = cntB (m1234 blk) blk.length :=
          cntB_beyond _ _ h64 (fun p hp => by rw [m1234_cls, cls_ge _ _ hp]; rfl)
        have e2 : popcnt (coa blk) = cntB (coa blk) blk.length :=
          cntB_beyond _ _ h64 (fun p hp => by simp [coa, bM]; omega)
        have e3 := hd _ (Nat.le_refl _)
        rw [hend] at e3
        have e4 := cntB_top_pdep_ones 32 (coa blk)
        have e5 := hd _ hc
        rw [hcm] at e5
        rw [popcnt_pdep_ones]
        simp only [ind_false] at e3 e5
        omega
      · intro hne
        have hcoa : coa blk (blk.length - 1) = true := by
          have e : clm blk blk.length 1 = cls blk (blk.length - 1) := clm_eq (by omega)
          rw [e, ← m234_cls] at hend
          simp [coa, bM, hend]; omega
        apply top_pos (n := blk.length) (by simp [ones]) h64
        have := cntB_succ_of (coa blk) (blk.length - 1) hcoa
        rw [show blk.length - 1 + 1 = blk.length by omega] at this
        omega

end Sound

/-! ## One block of valid UTF-8: the checks pass -/

section Complete
variable {blk : List Nat}

theorem m34_L3 {p : Nat} (h34 : m34 blk p = true) (hn4 : m4 blk p = false) : cls blk p = .L3 ∧ p < 62 := by
  simp only [m34, Bool.and_eq_true, decide_eq_true_eq] at h34
  have hp := h34.1.2
  have hr := cls_range (blk := blk) hp
  simp only [m4, Bool.and_eq_false_iff, decide_eq_false_iff_not] at hn4
  exact ⟨hr.2.2.2.1.mpr ⟨by omega, by omega⟩, h34.1.1⟩

theorem block_complete (h64 : blk.length ≤ 64)
    (hv : ∀ p, p ≤ blk.length → run 0 (blk.take p) ≠ 8)
    (hend : blk.length < 64 → run 0 blk = 0) :
    block16 true blk = block16 false blk := by
  obtain ⟨hS, hm⟩ := struct_of_valid h64 hv hend
  have hB : ∀ p, p < blk.length → byte blk p < 256 := by
    intro p hp
    have := hv (p + 1) (by omega)
    rw [run_take_succ 0 blk p hp] at this
    refine Classical.byContradiction fun h => this (step_big _ _ (by unfold byte at h; omega))
  -- a lead and the byte after it
  have hrng : ∀ q, q + 1 < blk.length → (cls blk q == .C) = false →
      leadOK (byte blk q) = true ∧ rngT (tagB (byte blk q)) (byte blk (q + 1)) = true := by
    intro q hq hc
    have h8 := hv (q + 2) (by omega)
    have hlz : run 0 (blk.take q) = 0 := by
      apply ValidatorSound.lead_zero blk q (by omega) (hv (q + 1) (by omega))
      rw [← isCont_cls q (by omega)] at hc; exact hc
    rw [show q + 2 = q + 1 + 1 from rfl, run_take_succ 0 blk (q + 1) hq,
      run_take_succ 0 blk q (by omega), hlz] at h8
    exact comp_at (hB q (by omega)) (hB (q + 1) hq) h8
  -- a three-byte unit
  have hu3 : ∀ e, e < 64 → mend4 blk e = true → 2 ≤ e → cls blk (e - 2) = .L3 →
      (decide (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) < 0x800) ||
        decide (sub16 (tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e)) 0xD800 < 0x800)) = false := by
    intro e he hme h2 hL3
    have hca := conts_after hS (q := e - 2) (by rw [hL3]; simp [Cls.len]; omega)
    have hc1 : cls blk (e - 1) = .C := by
      have := hca 1 (by omega) (by rw [hL3]; simp [Cls.len]); rwa [show e - 2 + 1 = e - 1 by omega] at this
    have hc0 : cls blk e = .C := by
      have := hca 2 (by omega) (by rw [hL3]; simp [Cls.len]); rwa [show e - 2 + 2 = e by omega] at this
    have b1 := byte_C hc1
    have b2 := byte_C hc0
    have hb0 := ((cls_range (blk := blk) (p := e - 2) (cls_ne_Z_lt (by rw [hL3]; simp))).2.2.2.1).mp hL3
    have hr := (hrng (e - 2) (by have := cls_C_lt hc1; omega) (by rw [hL3]; rfl)).2
    rw [show e - 2 + 1 = e - 1 by omega] at hr
    have := chk3_at hb0.1 hb0.2 b1.1 b1.2 b2.1 b2.2
    rw [hr] at this; simpa using this
  -- the four-byte high surrogate
  have hu4 : ∀ q, q + 3 < 64 → cls blk q = .L4 →
      decide (0x400 ≤ sub16 (((tval (byte blk q) (byte blk (q + 1)) (byte blk (q + 2)) >>> 4) + 0xD7C0)
        % 65536) 0xD800) = false := by
    intro q hq h4
    have hca := conts_after hS (q := q) (by rw [h4]; simp [Cls.len]; omega)
    have hc1 := hca 1 (by omega) (by rw [h4]; simp [Cls.len])
    have hc2 := hca 2 (by omega) (by rw [h4]; simp [Cls.len])
    have b1 := byte_C hc1
    have b2 := byte_C hc2
    have hql : q < blk.length := cls_ne_Z_lt (by rw [h4]; simp)
    have hb0 := ((cls_range (blk := blk) hql).2.2.2.2).mp h4
    have hr := hrng q (by have := cls_C_lt hc1; omega) (by rw [h4]; rfl)
    have := chk4_at hb0 (hB q hql) b1.1 b1.2 b2.1 b2.2
    rw [hr.1, hr.2] at this; simpa using this
  -- `M3` units are three-byte characters
  have hbig : ∀ k, bigM3 blk (mend4 blk) k = true → ∃ e, e < 64 ∧ mend4 blk e = true ∧
      cntB (mend4 blk) e = k ∧ 2 ≤ e ∧ cls blk (e - 2) = .L3 ∧ e - 2 < 62 := by
    intro k hk
    obtain ⟨e, he, hme, hce, hs⟩ := pext_true _ _ _ hk
    simp only [shl, Bool.and_eq_true, decide_eq_true_eq, Bool.not_eq_true'] at hs
    obtain ⟨⟨-, h2⟩, ⟨h34, -⟩, hn4⟩ := hs
    obtain ⟨hL3, h62⟩ := m34_L3 h34 hn4
    exact ⟨e, he, hme, hce, h2, hL3, h62⟩
  have hf4 : fail4 blk = false := by
    unfold fail4
    rw [List.any_eq_false]
    intro k hk
    have hk' := List.mem_range.mp hk
    simp only [Bool.or_eq_true, Bool.and_eq_true, not_or, not_and, decide_eq_true_eq]
    constructor
    · intro hp
      obtain ⟨e', he', hme', hce', hm3'⟩ := pext_true _ _ _ hp
      obtain ⟨h3', -, h4'⟩ := mp3_spec e' hm3'
      have hL4 : cls blk (e' - 3) = .L4 := by
        rw [m4_cls] at h4'; revert h4'; cases cls blk (e' - 3) <;> simp [Cls.m4]
      obtain ⟨-, hhi, hw⟩ := unit4 hS (q := e' - 3) (by omega) hL4
      obtain ⟨h2, -, -, hc⟩ := mend4_hi hS (q := e' - 3) (by omega) hL4
      rw [show e' - 3 + 3 = e' by omega, hce'] at hc
      have hk2 : cntB (mend4 blk) (e' - 3 + 2) = k := by omega
      rw [hk2] at hw
      rw [hw]
      have := hu4 (e' - 3) (by omega) hL4
      simpa using this
    · intro hb
      obtain ⟨e, he, hme, hce, h2, hL3, h62⟩ := hbig k hb
      obtain ⟨-, hlo, hhi, ht⟩ := unit3 hS he h2 hme hL3 h62
      rw [hce] at hlo hhi ht
      have hw : wout4 blk k = tval (byte blk (e - 2)) (byte blk (e - 1)) (byte blk e) := by
        unfold wout4; simp only [hhi, hlo, ht]; simp
      rw [hw]
      have := hu3 e he hme h2 hL3
      simpa using this
  unfold block16
  by_cases ha : (List.range 64).all (fun p => !bM blk p || m1 blk p) = true
  · rw [ite_eq_left ha, ite_eq_left ha]
  rw [ite_eq_right ha, ite_eq_right ha]
  simp only [hm, Bool.and_false, Bool.false_eq_true, ite_false, Bool.true_and, Bool.false_and]
  by_cases h34 : anyB (m34 blk) = true
  · rw [ite_eq_left h34, ite_eq_left h34]
    by_cases h4 : anyB (m4 blk) = true
    · have hS' : chkB blk (mc4 blk) = true := hS
      simp [h4, hS', hf4]
    · have h4' : anyB (m4 blk) = false := by simpa using h4
      have hS' : chkB blk (mc3 blk) = true := by rw [chk3_eq h64 h4']; exact hS
      have hf3 : fail3 blk = false := by
        unfold fail3
        rw [mend3_eq h64 h4', List.any_eq_false]
        intro k hk
        simp only [Bool.and_eq_true, not_and]
        intro hb
        obtain ⟨e, he, hme, hce, h2, hL3, h62⟩ := hbig k hb
        obtain ⟨-, -, -, ht⟩ := unit3 hS he h2 hme hL3 h62
        rw [hce] at ht
        rw [ht]
        have := hu3 e he hme h2 hL3
        simpa using this
      simp [h4', hS', hf3]
  · rw [ite_eq_right h34, ite_eq_right h34]
    have h34' : anyB (m34 blk) = false := by simpa using h34
    have hS' : chkB blk (shl (m234 blk) 1) = true := by rw [chk2_eq h34']; exact hS
    simp [hS']

end Complete
end SafetyMargins.IcelakeUtf8
