import SafetyMargins.IcelakeUtf8
/-
Ice Lake UTF-8 → UTF-32 and UTF-8 → Latin-1, on the bytes and the masks.

**UTF-32** (src/icelake/icelake_from_utf8.inl.cpp,
src/icelake/icelake_from_valid_utf8.inl.cpp, src/icelake/icelake_macros.inl.cpp,
and the callers in src/icelake/implementation.cpp:1160-1295). Each 16-byte
lane of a block stores, masked, `count_ones(leading_bytes)` units: the bytes
of the lane that are not continuation bytes (`expand_and_identify`,
src/icelake/icelake_utf8_common.inl.cpp:734-749; `SIMDUTF_ICELAKE_TRANSCODE16`,
icelake_macros.inl.cpp:42-88). That count is `utf32_length_from_utf8` of the
lane on every input, valid or not, and does not depend on the checker. So the
cursor is exactly `L` of the consumed bytes (`loop32_exact`), every store ends
at or before it, and the scalar tail after the skipped continuation bytes
stores at most `L` of the rest (`IcelakeMask.sc8_le`). The checker
(`avx512_utf8_checker`, icelake_utf8_validation.inl.cpp) only decides where
the function returns; the model takes its verdicts as arbitrary functions
`chk` (error after checking the bytes below `n`) and `chkEnd` (error after the
last check and `check_eof`), so the theorems hold for every verdict and need
no proof of the checker. `convert_utf8_to_utf32_with_errors` rewinds and
converts from the start of the output (implementation.cpp:1225-1228), at most
`L` of a suffix of the input.

**Latin-1** (src/icelake/icelake_convert_utf8_to_latin1.inl.cpp,
src/icelake/icelake_convert_valid_utf8_to_latin1.inl.cpp, implementation.cpp:1072-1099).
A block stores `popcount(retain)` bytes, the ASCII and continuation bytes. The
check `(nonascii ^ leading) == (leading << 1 | next_leading)` makes that
`L` of the block minus a lead in byte 63 plus the lead carried in
(`blockL1_count`), so the cursor plus the pending lead is `L` of the consumed
bytes (`runL1_true`). `convert_valid_utf8_to_latin1` has no check: on the
input its contract allows (`ValidLatin1.LV`) the identity holds anyway
(`lv_cont`); on valid UTF-8 outside that contract it overflows
(`latin1_valid_needs_LV`).

The masked loads read exactly the bytes they keep; every other load is
checked against the input length (`rEnd`).
-/
namespace SafetyMargins.IcelakeUtf8
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.ValidExact SafetyMargins.Validator
  SafetyMargins.Exact

/-! ## UTF-32 -/

/-- `utf32_length_from_utf8` of the `n` bytes at `p`: the sum of
`count_ones(leading_bytes)` over the lanes that cover them. -/
def cnt01 (xs : List Nat) (p n : Nat) : Nat := L (((xs.drop p).take n).map w01)

/-- `_mm512_test_epi8_mask(utf8, 0x80) == 0` on the 64 bytes at `p`
(icelake_utf8_validation.inl.cpp:101, icelake_from_valid_utf8.inl.cpp:48). -/
def ascii64 (xs : List Nat) (p : Nat) : Bool :=
  (List.range 64).all fun i => decide (xs.getD (p + i) 0 < 0x80)

/-- The main loop, `while (end - ptr >= 64 + 4)`
(src/icelake/icelake_from_utf8.inl.cpp:32 and :160, and
src/icelake/icelake_from_valid_utf8.inl.cpp:45). With `cc`
(`_with_constant_checks`), an error after checking the block returns before
any store of it (src/icelake/icelake_from_utf8.inl.cpp:162-165). An ASCII
block stores 64 units unmasked (`SIMDUTF_ICELAKE_STORE_ASCII`,
src/icelake/icelake_macros.inl.cpp:113-143); otherwise the four lanes store
their counts, masked, two lanes merged when their counts add up to at most 16
(src/icelake/icelake_from_utf8.inl.cpp:47-58 and :67-78); the last store ends
at the cursor plus the sum, and the iteration reads 4 bytes at `ptr + 64`
(src/icelake/icelake_from_utf8.inl.cpp:63). -/
def loop32 (cc : Bool) (chk : Nat → Bool) (xs : List Nat) : Nat → Nat → Nat → Run
  | 0, ptr, out => ⟨ptr, out, true, out, ptr⟩
  | f + 1, ptr, out =>
    if 68 ≤ xs.length - ptr then
      if cc && chk (ptr + 64) then ⟨ptr, out, false, out, ptr + 64⟩
      else
        let n := if ascii64 xs ptr then 64 else cnt01 xs ptr 64
        let rd := if ascii64 xs ptr then ptr + 64 else ptr + 68
        let r := loop32 cc chk xs f (ptr + 64) (out + n)
        ⟨r.pos, r.out, r.ok, max (out + n) r.wEnd, max rd r.rEnd⟩
    else ⟨ptr, out, true, out, ptr⟩

/-- The whole vector function: the loop, then one more block when 64 bytes
remain, of which an ASCII block stores 64 units and a non-ASCII block
transcodes three lanes, 48 bytes (src/icelake/icelake_from_utf8.inl.cpp:85-118
and :217-254, src/icelake/icelake_from_valid_utf8.inl.cpp:98-132); then the
masked check of the rest, `~0 >> (64 - (end - validatedptr))`
(src/icelake/icelake_from_utf8.inl.cpp:119-124 and :255-260) and `check_eof`.
`ok = false` is the `nullptr` / `false` return. `convert_valid` has no masked
check; its read end is then over-approximated by the input length. -/
def fixed32 (cc : Bool) (chk : Nat → Bool) (chkEnd : Bool) (xs : List Nat) (f : Nat) : Run :=
  let r := loop32 cc chk xs f 0 0
  if r.ok = false then r
  else if 64 ≤ xs.length - r.pos then
    if cc && chk (r.pos + 64) then ⟨r.pos, r.out, false, r.wEnd, max r.rEnd (r.pos + 64)⟩
    else
      let adv := if ascii64 xs r.pos then 64 else 48
      let n := if ascii64 xs r.pos then 64 else cnt01 xs r.pos 48
      ⟨r.pos + adv, r.out + n, !chkEnd, max r.wEnd (r.out + n), max r.rEnd xs.length⟩
  else ⟨r.pos, r.out, !chkEnd, r.wEnd, max r.rEnd xs.length⟩

/-- Skip the continuation bytes at `p` (implementation.cpp:1180-1182). -/
def skipC (xs : List Nat) : Nat → Nat → Nat
  | 0, p => p
  | f + 1, p => if p < xs.length ∧ isCont (xs.getD p 0) = true then skipC xs f (p + 1) else p

/-- `convert_utf8_to_utf32` (implementation.cpp:1160-1194): the furthest store end. -/
def conv32 (chk : Nat → Bool) (chkEnd : Bool) (xs : List Nat) : Nat :=
  let r := fixed32 false chk chkEnd xs xs.length
  if r.ok = false then r.wEnd
  else
    let q := skipC xs xs.length r.pos
    max r.wEnd (r.out + sc8 .u32 xs.length (xs.drop q))

/-- `convert_utf8_to_utf32_with_errors` (implementation.cpp:1196-1260). On an
error: the position adjustment and the fourth-continuation test (:1205-1218),
then `rewind_and_convert_with_errors(pos, buf + pos, len - pos, utf32)`, which
stores from the start of the output (:1225-1228). -/
def conv32err (chk : Nat → Bool) (chkEnd : Bool) (xs : List Nat) : Nat :=
  let r := fixed32 true chk chkEnd xs xs.length
  if r.ok = false then
    let p := r.pos
    if 64 ≤ p ∧ isCont (xs.getD p 0) = false then
      max r.wEnd (rewind .u32 xs (p - 1) (min 3 (p - 1)))
    else if 64 ≤ p ∧ isCont (xs.getD p 0) = true ∧ isCont (xs.getD (p - 1) 0) = true ∧
        isCont (xs.getD (p - 2) 0) = true ∧ isCont (xs.getD (p - 3) 0) = true then r.wEnd
    else max r.wEnd (rewind .u32 xs p (min 3 p))
  else
    let q := skipC xs xs.length r.pos
    max r.wEnd (r.out + sc8 .u32 xs.length (xs.drop q))

/-- `convert_valid_utf8_to_utf32` (implementation.cpp:1262-1295). -/
def conv32valid (xs : List Nat) : Nat :=
  let r := fixed32 false (fun _ => false) false xs xs.length
  let q := skipC xs xs.length r.pos
  max r.wEnd (r.out + sc8 .u32v xs.length (xs.drop q))

section U32
variable {xs : List Nat}

theorem ascii64_L (p : Nat) (h : ascii64 xs p = true) (hp : 64 ≤ xs.length - p) :
    cnt01 xs p 64 = 64 := by
  unfold cnt01
  have hl : ((xs.drop p).take 64).length = 64 := by simp; omega
  have : ∀ i, i < 64 → w01 (((xs.drop p).take 64).getD i 0) = 1 := by
    intro i hi
    have := List.all_eq_true.mp h i (List.mem_range.mpr hi)
    simp only [decide_eq_true_eq] at this
    have e := view_byte (xs := xs) p i (by rw [view_length]; omega)
    unfold byte view at e
    rw [e]; unfold w01 isCont
    rw [ite_eq_right (by simp only [Bool.and_eq_true, decide_eq_true_eq]; omega)]
  generalize hl' : (xs.drop p).take 64 = l at hl this
  have key : ∀ k, k ≤ l.length → L ((l.take k).map w01) = k := by
    intro k
    induction k with
    | zero => simp
    | succ k ih => intro hk; rw [L_take_succ _ _ _ (by omega), ih (by omega), this k (by omega)]
  have := key 64 (by omega)
  rwa [List.take_of_length_le (by omega)] at this

theorem cnt01_split (p c : Nat) :
    L ((xs.take (p + c)).map w01) = L ((xs.take p).map w01) + cnt01 xs p c := Lt_add w01 p c

theorem Lw_le (p : Nat) : L ((xs.take p).map w01) ≤ L (xs.map w01) := by
  have := L_take_le (xs.map w01) p; rwa [← List.map_take] at this

/-- **The UTF-32 loop is exact**: the cursor is `utf32_length_from_utf8` of the
consumed bytes, on every input and for every verdict of the checker. -/
theorem loop32_exact (cc : Bool) (chk : Nat → Bool) : ∀ f ptr out,
    out = L ((xs.take ptr).map w01) → ptr ≤ xs.length →
    (loop32 cc chk xs f ptr out).out = L ((xs.take (loop32 cc chk xs f ptr out).pos).map w01) ∧
    (loop32 cc chk xs f ptr out).wEnd ≤ L (xs.map w01) ∧
    (loop32 cc chk xs f ptr out).pos ≤ xs.length ∧
    (loop32 cc chk xs f ptr out).rEnd ≤ xs.length := by
  intro f
  induction f with
  | zero => intro ptr out ho hp; exact ⟨ho, by rw [ho]; exact Lw_le _, hp, hp⟩
  | succ f ih =>
    intro ptr out ho hp
    unfold loop32
    by_cases h68 : 68 ≤ xs.length - ptr
    · rw [ite_eq_left h68]
      split
      · exact ⟨ho, by rw [ho]; exact Lw_le _, hp, show ptr + 64 ≤ _ by omega⟩
      · have hn : (if ascii64 xs ptr then 64 else cnt01 xs ptr 64) = cnt01 xs ptr 64 := by
          split
          · rename_i ha; exact (ascii64_L ptr ha (by omega)).symm
          · rfl
        simp only [hn]
        have hs := cnt01_split (xs := xs) ptr 64
        have hr := ih (ptr + 64) (out + cnt01 xs ptr 64) (by omega) (by omega)
        refine ⟨hr.1, Nat.max_le.mpr ⟨?_, hr.2.1⟩, hr.2.2.1, Nat.max_le.mpr ⟨?_, hr.2.2.2⟩⟩
        · have := Lw_le (xs := xs) (ptr + 64); omega
        · split <;> omega
    · rw [ite_eq_right h68]; exact ⟨ho, by rw [ho]; exact Lw_le _, hp, hp⟩

theorem fixed32_exact (cc : Bool) (chk : Nat → Bool) (chkEnd : Bool) (f : Nat) :
    (fixed32 cc chk chkEnd xs f).out = L ((xs.take (fixed32 cc chk chkEnd xs f).pos).map w01) ∧
    (fixed32 cc chk chkEnd xs f).wEnd ≤ L (xs.map w01) ∧
    (fixed32 cc chk chkEnd xs f).pos ≤ xs.length ∧
    (fixed32 cc chk chkEnd xs f).rEnd ≤ xs.length := by
  have h := loop32_exact (xs := xs) cc chk f 0 0 (by simp) (Nat.zero_le _)
  unfold fixed32
  simp only
  generalize loop32 cc chk xs f 0 0 = r at h
  obtain ⟨ho, hw, hp, hr⟩ := h
  split
  · exact ⟨ho, hw, hp, hr⟩
  split
  · split
    · exact ⟨ho, hw, hp, Nat.max_le.mpr ⟨hr, by omega⟩⟩
    · rename_i h64 _
      have hs48 := cnt01_split (xs := xs) r.pos 48
      have hs64 := cnt01_split (xs := xs) r.pos 64
      by_cases ha : ascii64 xs r.pos = true
      · simp only [ha, ite_true]
        rw [ascii64_L _ ha h64] at hs64
        refine ⟨by omega, Nat.max_le.mpr ⟨hw, ?_⟩, by omega, Nat.max_le.mpr ⟨hr, Nat.le_refl _⟩⟩
        have := Lw_le (xs := xs) (r.pos + 64); omega
      · simp only [ha, Bool.false_eq_true, ite_false]
        refine ⟨by omega, Nat.max_le.mpr ⟨hw, ?_⟩, by omega, Nat.max_le.mpr ⟨hr, Nat.le_refl _⟩⟩
        have := Lw_le (xs := xs) (r.pos + 48); omega
  · exact ⟨ho, hw, hp, Nat.max_le.mpr ⟨hr, Nat.le_refl _⟩⟩

theorem skipC_spec : ∀ f p, p ≤ xs.length →
    p ≤ skipC xs f p ∧ skipC xs f p ≤ xs.length ∧
    L ((xs.take (skipC xs f p)).map w01) = L ((xs.take p).map w01) := by
  intro f
  induction f with
  | zero => intro p hp; exact ⟨Nat.le_refl _, hp, rfl⟩
  | succ f ih =>
    intro p hp
    unfold skipC
    split
    · rename_i h
      have := ih (p + 1) (by omega)
      have e : L ((xs.take (p + 1)).map w01) = L ((xs.take p).map w01) := by
        rw [L_take_succ _ _ _ h.1]; unfold w01; rw [ite_eq_left h.2]; rfl
      exact ⟨by omega, this.2.1, this.2.2.trans e⟩
    · exact ⟨Nat.le_refl _, hp, rfl⟩

/-- The scalar tail after the vector part: from the skipped position, at the
cursor, within `L` of the input. -/
theorem tail32 (k : SK) (hk : k ≠ .u16) (pos out : Nat) (hp : pos ≤ xs.length)
    (ho : out = L ((xs.take pos).map w01)) :
    out + sc8 k xs.length (xs.drop (skipC xs xs.length pos)) ≤ L (xs.map w01) := by
  obtain ⟨-, hq, he⟩ := skipC_spec (xs := xs) xs.length pos hp
  have hsc := sc8_le k xs.length (xs.drop (skipC xs xs.length pos))
  have hw : wOf k = w01 := by cases k <;> simp_all [wOf]
  rw [hw] at hsc
  have hsplit := L_take_add_drop (skipC xs xs.length pos) (xs.map w01)
  rw [← List.map_take, ← List.map_drop] at hsplit
  omega

/-- **`convert_utf8_to_utf32` on Ice Lake**: on every input and for every
verdict of the checker, no store ends past `utf32_length_from_utf8` of the
input, and no vector load reads past it. -/
theorem conv32_safe (chk : Nat → Bool) (chkEnd : Bool) :
    conv32 chk chkEnd xs ≤ L (xs.map w01) ∧
    (fixed32 false chk chkEnd xs xs.length).rEnd ≤ xs.length := by
  obtain ⟨ho, hw, hp, hr⟩ := fixed32_exact (xs := xs) false chk chkEnd xs.length
  refine ⟨?_, hr⟩
  unfold conv32; simp only
  split
  · exact hw
  · exact Nat.max_le.mpr ⟨hw, tail32 .u32 (by decide) _ _ hp ho⟩

theorem rewind_le (k : SK) (hk : k ≠ .u16) (pos hfb : Nat) : rewind k xs pos hfb ≤ L (xs.map w01) := by
  unfold rewind
  rcases rwSearch xs pos 0 hfb with _ | i
  · exact Nat.zero_le _
  · simp only
    split
    · exact Nat.zero_le _
    · have hsc := sc8_le k xs.length (xs.drop (pos - i))
      have hw : wOf k = w01 := by cases k <;> simp_all [wOf]
      rw [hw] at hsc
      have hsplit := L_take_add_drop (pos - i) (xs.map w01)
      rw [← List.map_take, ← List.map_drop] at hsplit
      omega

/-- **`convert_utf8_to_utf32_with_errors` on Ice Lake**: on every input and
for every verdict of the checker, the vector stores, the scalar tail and the
rewind on error all end inside `utf32_length_from_utf8` of the input. -/
theorem conv32err_safe (chk : Nat → Bool) (chkEnd : Bool) :
    conv32err chk chkEnd xs ≤ L (xs.map w01) ∧
    (fixed32 true chk chkEnd xs xs.length).rEnd ≤ xs.length := by
  obtain ⟨ho, hw, hp, hr⟩ := fixed32_exact (xs := xs) true chk chkEnd xs.length
  refine ⟨?_, hr⟩
  unfold conv32err; simp only
  split
  · split
    · exact Nat.max_le.mpr ⟨hw, rewind_le .u32 (by decide) _ _⟩
    · split
      · exact hw
      · exact Nat.max_le.mpr ⟨hw, rewind_le .u32 (by decide) _ _⟩
  · exact Nat.max_le.mpr ⟨hw, tail32 .u32 (by decide) _ _ hp ho⟩

/-- **`convert_valid_utf8_to_utf32` on Ice Lake**: on every input, valid or
not (stronger than its contract), no store ends past `utf32_length_from_utf8`
of the input. -/
theorem conv32valid_safe :
    conv32valid xs ≤ L (xs.map w01) ∧
    (fixed32 false (fun _ => false) false xs xs.length).rEnd ≤ xs.length := by
  obtain ⟨ho, hw, hp, hr⟩ := fixed32_exact (xs := xs) false (fun _ => false) false xs.length
  exact ⟨Nat.max_le.mpr ⟨hw, tail32 .u32v (by decide) _ _ hp ho⟩, hr⟩

end U32

/-! ## Latin-1 -/

section L1Block
variable (blk : List Nat)

/-- `nonascii = _mm512_movepi8_mask(input)` (line 12 of both kernels). -/
def na : Mask := fun p => decide (p < blk.length) && decide (0x80 ≤ byte blk p)
/-- `leading = _mm512_cmpge_epu8_mask(input, minus64)`, `0xc0 <= input` (line 23 / 21). -/
def ld : Mask := fun p => decide (p < blk.length) && decide (0xC0 ≤ byte blk p)
/-- `leading_shift = (leading << 1) | next_leading` (line 33). -/
def lsh (eIn : Bool) : Mask := fun p =>
  decide (p < 64) && ((decide (1 ≤ p) && ld blk (p - 1)) || (decide (p = 0) && eIn))
/-- `retain = ~leading & load_mask` (line 43 / 32). -/
def ret : Mask := fun p => decide (p < blk.length) && !ld blk p
/-- `invalid_leading_bytes`: a lead whose `input ^ 0xc2` exceeds 1, i.e. not
`0xc2`/`0xc3` (lines 25-31). -/
def invL : Bool := anyB fun p => ld blk p && decide (1 < (byte blk p ^^^ 0xC2))
/-- `(nonascii ^ leading) != leading_shift` (line 35), negated. -/
def chkL (eIn : Bool) : Bool := (List.range 64).all fun p => (na blk p != ld blk p) == lsh blk eIn p

end L1Block

/-- `process_block_from_utf8_to_latin1<is_remaining>` (v = true,
icelake_convert_utf8_to_latin1.inl.cpp:5-57) and
`process_valid_block_from_utf8_to_latin1` (v = false,
icelake_convert_valid_utf8_to_latin1.inl.cpp:5-42) on the bytes of the block
(64, or `len < 64` with `load_mask = _bzhi_u64(~0, len)`) and the carried
`next_leading`: `none` is the error return 0; `some (w, e)` stores `w` bytes
(the ASCII block unmasked or masked to `len`, otherwise masked to
`written_out`, line 52-54 / 38-40) and leaves `next_leading = e`. -/
def blockL1 (v : Bool) (blk : List Nat) (eIn : Bool) : Option (Nat × Bool) :=
  if !anyB (na blk) then (if v && eIn then none else some (blk.length, eIn))
  else if v && invL blk then none
  else if v && !chkL blk eIn then none
  else if popcnt (ret blk) = 0 then (if v then none else some (0, ld blk 63))
  else some (popcnt (ret blk), ld blk 63)

structure RunL where
  pos : Nat
  out : Nat
  e : Bool
  ok : Bool
  wEnd : Nat
  rEnd : Nat
deriving Repr

/-- `utf8_to_latin1_avx512` (v = true,
src/icelake/icelake_convert_utf8_to_latin1.inl.cpp:59-104: full blocks while
`pos + 64 <= len`, line 70, then one masked block, line 83) and
`valid_utf8_to_latin1_avx512` (v = false,
src/icelake/icelake_convert_valid_utf8_to_latin1.inl.cpp:44-69: line 53, then
line 60). -/
def runL1 (v : Bool) (xs : List Nat) : Nat → Nat → Nat → Bool → RunL
  | 0, pos, out, e => ⟨pos, out, e, true, out, pos⟩
  | f + 1, pos, out, e =>
    if pos < xs.length then
      match blockL1 v (view xs pos) e with
      | none => ⟨pos, out, e, false, out, pos + (view xs pos).length⟩
      | some (w, e') =>
        let r := runL1 v xs f (pos + (view xs pos).length) (out + w) e'
        ⟨r.pos, r.out, r.e, r.ok, max (out + w) r.wEnd, max (pos + (view xs pos).length) r.rEnd⟩
    else ⟨pos, out, e, true, out, pos⟩

/-- `convert_utf8_to_latin1` (implementation.cpp:1072-1075). -/
def convL1 (xs : List Nat) : Nat := (runL1 true xs xs.length 0 0 false).wEnd

/-- `convert_utf8_to_latin1_with_errors` (implementation.cpp:1077-1094): unless
the kernel converted everything, the scalar `convert_with_errors` resumes at
`inbuf = buf + pos - next_leading` (lines 75, 89, 97 of the kernel), at the
cursor. -/
def convL1err (xs : List Nat) : Nat :=
  let r := runL1 true xs xs.length 0 0 false
  if r.ok = true ∧ r.e = false then r.wEnd
  else max r.wEnd (r.out + sc8 .l1 xs.length (xs.drop (r.pos - ind r.e)))

/-- `convert_valid_utf8_to_latin1` (implementation.cpp:1096-1099). -/
def convL1valid (xs : List Nat) : Nat := (runL1 false xs xs.length 0 0 false).wEnd

section L1
variable {blk : List Nat}

theorem cntB_add {a b c : Mask} {n : Nat} (h : ∀ p, p < n → ind (c p) = ind (a p) + ind (b p)) :
    cntB c n = cntB a n + cntB b n := by
  induction n with
  | zero => rfl
  | succ n ih =>
    simp only [cntB]; rw [ih (fun p hp => h p (by omega)), h n (by omega)]; omega

theorem lsh_cnt (eIn : Bool) : ∀ k, k + 1 ≤ 64 → cntB (lsh blk eIn) (k + 1) = ind eIn + cntB (ld blk) k := by
  intro k
  induction k with
  | zero => intro _; simp [cntB, lsh]
  | succ k ih =>
    intro hk
    have e : lsh blk eIn (k + 1) = ld blk k := by simp [lsh, show k + 1 < 64 by omega]
    rw [show cntB (lsh blk eIn) (k + 1 + 1) = cntB (lsh blk eIn) (k + 1) + ind (lsh blk eIn (k + 1))
      from rfl, show cntB (ld blk) (k + 1) = cntB (ld blk) k + ind (ld blk k) from rfl,
      ih (by omega), e]
    omega

/-- **The Latin-1 count.** On a block that passes the check, the bytes
stored plus the lead left in byte 63 are `L` of the block plus the lead
carried in. -/
theorem blockL1_count (h64 : blk.length ≤ 64) (eIn : Bool) (hc : chkL blk eIn = true) :
    popcnt (ret blk) + ind (ld blk 63) = L (blk.map w01) + ind eIn := by
  let asc : Mask := fun p => decide (p < blk.length) && decide (byte blk p < 0x80)
  let cn : Mask := fun p => na blk p && !ld blk p
  have h1 : cntB (ret blk) 64 = cntB asc 64 + cntB cn 64 := by
    apply cntB_add; intro p _
    simp only [ret, asc, cn, na, ld]
    by_cases hp : p < blk.length
    · simp only [hp, decide_true, Bool.true_and]
      by_cases a : byte blk p < 128 <;> by_cases b : 192 ≤ byte blk p <;>
        by_cases c : 128 ≤ byte blk p <;> simp [a, b, c, ind] <;> omega
    · simp [hp]
  have h2 : cntB cn 64 = cntB (lsh blk eIn) 64 := by
    apply cntB_congr; intro p hp
    have := List.all_eq_true.mp hc p (List.mem_range.mpr hp)
    simp only [beq_iff_eq] at this
    rw [← this]; simp only [cn, na, ld]
    cases decide (p < blk.length) <;> simp
    by_cases h : 0xC0 ≤ byte blk p <;> simp [h] <;> omega
  have h3 : cntB (lsh blk eIn) 64 = ind eIn + cntB (ld blk) 63 := lsh_cnt (blk := blk) eIn 63 (by omega)
  have h4 : cntB (ld blk) 64 = cntB (ld blk) 63 + ind (ld blk 63) := rfl
  have h5 : L (blk.map w01) = cntB asc 64 + cntB (ld blk) 64 := by
    have key : ∀ k, k ≤ blk.length → L ((blk.take k).map w01) = cntB asc k + cntB (ld blk) k := by
      intro k
      induction k with
      | zero => simp [cntB]
      | succ k ih =>
        intro hk
        have hstep : w01 (byte blk k) = ind (decide (byte blk k < 0x80)) + ind (decide (0xC0 ≤ byte blk k)) := by
          unfold w01 isCont
          by_cases a : byte blk k < 0x80 <;> by_cases b : 0xC0 ≤ byte blk k <;> simp [a, b] <;> omega
        have ea : asc k = decide (byte blk k < 0x80) := by simp [asc, show k < blk.length by omega]
        have eb : ld blk k = decide (0xC0 ≤ byte blk k) := by simp [ld, show k < blk.length by omega]
        rw [L_take_succ _ _ _ (by omega), ih (by omega),
          show cntB asc (k + 1) = cntB asc k + ind (asc k) from rfl,
          show cntB (ld blk) (k + 1) = cntB (ld blk) k + ind (ld blk k) from rfl, ea, eb]
        have : blk.getD k 0 = byte blk k := rfl
        rw [this, hstep]; omega
    have := key blk.length (Nat.le_refl _)
    rw [List.take_of_length_le (Nat.le_refl _)] at this
    rw [this, cntB_beyond asc _ h64 (fun p hp => by simp [asc]; omega),
      cntB_beyond (ld blk) _ h64 (fun p hp => by simp [ld]; omega)]
  unfold popcnt
  omega

theorem w01_lt (x : Nat) (h : x < 0x80) : w01 x = 1 := by
  unfold w01 isCont; rw [ite_eq_right (by simp only [Bool.and_eq_true, decide_eq_true_eq]; omega)]
theorem w01_ge (x : Nat) (h : 0xC0 ≤ x) : w01 x = 1 := by
  unfold w01 isCont; rw [ite_eq_right (by simp only [Bool.and_eq_true, decide_eq_true_eq]; omega)]

theorem L_ones (f : Nat → Nat) (h : ∀ p, p < blk.length → f (blk.getD p 0) = 1) :
    L (blk.map f) = blk.length := by
  have key : ∀ k, k ≤ blk.length → L ((blk.take k).map f) = k := by
    intro k
    induction k with
    | zero => simp
    | succ k ih => intro hk; rw [L_take_succ _ _ _ (by omega), ih (by omega), h k (by omega)]
  have := key blk.length (Nat.le_refl _)
  rwa [List.take_of_length_le (Nat.le_refl _)] at this

theorem na_false (h : anyB (na blk) = false) (h64 : blk.length ≤ 64) (p : Nat) (hp : p < blk.length) :
    byte blk p < 0x80 := by
  have := anyB_false h p (by omega)
  simp [na, hp] at this; exact this

theorem ld63 (h : ld blk 63 = true) : blk.length = 64 ∨ 63 < blk.length := by
  simp [ld] at h; omega

/-- A block of the validating kernel that stores: the count, and a lead left
in byte 63 when `next_leading` is set. -/
theorem blockL1_true (h64 : blk.length ≤ 64) {eIn e' : Bool} {w : Nat}
    (h : blockL1 true blk eIn = some (w, e')) :
    w + ind e' = L (blk.map w01) + ind eIn ∧
      (e' = true → 63 < blk.length ∧ 0xC0 ≤ byte blk 63) := by
  unfold blockL1 at h
  by_cases ha : anyB (na blk) = false
  · simp only [ha, Bool.not_false, ite_true, Bool.true_and] at h
    cases eIn
    · simp only [Bool.false_eq_true, ite_false, Option.some.injEq, Prod.mk.injEq] at h
      obtain ⟨rfl, rfl⟩ := h
      refine ⟨?_, by simp⟩
      rw [L_ones w01 (fun p hp => w01_lt _ (na_false ha h64 p hp))]
    · simp at h
  · have ha' : anyB (na blk) = true := by simpa using ha
    simp only [ha', Bool.not_true, Bool.false_eq_true, ite_false, Bool.true_and] at h
    by_cases hi : invL blk = true
    · simp [hi] at h
    · simp only [hi, Bool.false_eq_true, ite_false] at h
      by_cases hc : chkL blk eIn = true
      · simp only [hc, Bool.not_true, Bool.false_eq_true, ite_false] at h
        split at h
        · simp at h
        · simp only [Option.some.injEq, Prod.mk.injEq] at h
          obtain ⟨rfl, rfl⟩ := h
          refine ⟨blockL1_count h64 eIn hc, fun h6 => ?_⟩
          simp [ld] at h6; exact ⟨h6.1, h6.2⟩
      · simp [hc] at h

end L1

section L1Run
variable {xs : List Nat}

/-- The validating Latin-1 kernel: the cursor plus the pending lead is `L` of
the consumed bytes, and every store ends inside `L` of the input. -/
theorem runL1_true : ∀ f pos out (e : Bool), out + ind e = L ((xs.take pos).map w01) →
    (e = true → 1 ≤ pos ∧ 0xC0 ≤ xs.getD (pos - 1) 0) → pos ≤ xs.length →
    (runL1 true xs f pos out e).wEnd ≤ L (xs.map w01) ∧
    (runL1 true xs f pos out e).out + ind (runL1 true xs f pos out e).e =
      L ((xs.take (runL1 true xs f pos out e).pos).map w01) ∧
    ((runL1 true xs f pos out e).e = true → 1 ≤ (runL1 true xs f pos out e).pos ∧
      0xC0 ≤ xs.getD ((runL1 true xs f pos out e).pos - 1) 0) ∧
    (runL1 true xs f pos out e).pos ≤ xs.length ∧ (runL1 true xs f pos out e).rEnd ≤ xs.length := by
  intro f
  induction f with
  | zero =>
    intro pos out e ho he hp
    have := Lw_le (xs := xs) pos
    exact ⟨show out ≤ _ by omega, ho, he, hp, hp⟩
  | succ f ih =>
    intro pos out e ho he hp
    have hl := view_length (xs := xs) pos
    have hW := Lw_le (xs := xs) pos
    unfold runL1
    by_cases hlt : pos < xs.length
    · rw [ite_eq_left hlt]
      cases hb : blockL1 true (view xs pos) e with
      | none => exact ⟨show out ≤ _ by omega, ho, he, hp, show pos + _ ≤ _ by omega⟩
      | some we =>
        obtain ⟨w, e'⟩ := we
        obtain ⟨hc, he'⟩ := blockL1_true (by omega) hb
        have hs := Lt_add (xs := xs) w01 pos (view xs pos).length
        rw [← view_take _ _ (by omega), List.take_of_length_le (Nat.le_refl _)] at hs
        have hr := ih (pos + (view xs pos).length) (out + w) e' (by omega)
          (fun h => by
            obtain ⟨h1, h2⟩ := he' h
            rw [view_byte pos 63 h1] at h2
            exact ⟨by omega, by rw [show pos + (view xs pos).length - 1 = pos + 63 by omega]; exact h2⟩)
          (by omega)
        have hW' := Lw_le (xs := xs) (pos + (view xs pos).length)
        exact ⟨Nat.max_le.mpr ⟨by cases e' <;> simp [ind] at * <;> omega, hr.1⟩, hr.2.1, hr.2.2.1,
          hr.2.2.2.1, Nat.max_le.mpr ⟨by omega, hr.2.2.2.2⟩⟩
    · rw [ite_eq_right hlt]; exact ⟨show out ≤ _ by omega, ho, he, hp, hp⟩

/-- **`convert_utf8_to_latin1` on Ice Lake**: on every input, no store ends
past `latin1_length_from_utf8` of the input, and no load reads past it. -/
theorem convL1_safe :
    convL1 xs ≤ L (xs.map w01) ∧ (runL1 true xs xs.length 0 0 false).rEnd ≤ xs.length := by
  have h := runL1_true (xs := xs) xs.length 0 0 false (by simp) (by simp) (Nat.zero_le _)
  exact ⟨h.1, h.2.2.2.2⟩

/-- **`convert_utf8_to_latin1_with_errors` on Ice Lake**: on every input, the
kernel and the scalar resumption at `pos - next_leading` stay inside
`latin1_length_from_utf8` of the input. -/
theorem convL1err_safe : convL1err xs ≤ L (xs.map w01) := by
  have h := runL1_true (xs := xs) xs.length 0 0 false (by simp) (by simp) (Nat.zero_le _)
  obtain ⟨hw, ho, he, hp, -⟩ := h
  unfold convL1err
  simp only
  split
  · exact hw
  · generalize runL1 true xs xs.length 0 0 false = r at hw ho he hp
    refine Nat.max_le.mpr ⟨hw, ?_⟩
    have hsc := sc8_le .l1 xs.length (xs.drop (r.pos - ind r.e))
    simp only [wOf] at hsc
    have hsplit := L_take_add_drop (r.pos - ind r.e) (xs.map w01)
    rw [← List.map_take, ← List.map_drop] at hsplit
    cases hre : r.e
    · simp only [hre, ind_false, Nat.sub_zero, Nat.add_zero] at ho hsc hsplit ⊢; omega
    · simp only [hre, ind_true] at ho he hsc hsplit ⊢
      obtain ⟨h1, h2⟩ := he trivial
      have e1 := L_take_succ xs w01 (r.pos - 1) (by omega)
      rw [show r.pos - 1 + 1 = r.pos by omega, w01_ge _ h2] at e1
      omega

/-! ### `convert_valid_utf8_to_latin1` -/

/-- On `LV` input a byte is a continuation byte exactly when the byte before
it is a lead, and the last byte is not a lead. -/
theorem lv_facts {ys : List Nat} (h : ValidLatin1.LV ys) : ∀ i, i < ys.length →
    (isCont (ys.getD i 0) = true ↔ (1 ≤ i ∧ 0xC0 ≤ ys.getD (i - 1) 0)) ∧
    (0xC0 ≤ ys.getD i 0 → i + 1 < ys.length) := by
  induction h with
  | nil => intro i hi; simp at hi
  | one a rest ha _ ih =>
    intro i hi
    cases i with
    | zero => simp [isCont]; omega
    | succ j =>
      simp only [List.getD_cons_succ, List.length_cons] at hi ⊢
      obtain ⟨i1, i2⟩ := ih j (by omega)
      refine ⟨?_, fun h => by have := i2 h; omega⟩
      rw [i1]
      cases j with
      | zero => simp; omega
      | succ k => simp
  | two c d rest hc hd _ ih =>
    intro i hi
    rcases i with _ | _ | j
    · simp [isCont]; omega
    · simp [isCont]; omega
    · simp only [List.getD_cons_succ, List.length_cons] at hi ⊢
      obtain ⟨i1, i2⟩ := ih j (by omega)
      refine ⟨?_, fun h => by have := i2 h; omega⟩
      rw [i1]
      cases j with
      | zero => simp; omega
      | succ k => simp

theorem bne_isCont (x : Nat) : (decide (0x80 ≤ x) != decide (0xC0 ≤ x)) = isCont x := by
  unfold isCont
  by_cases a : 0x80 ≤ x <;> by_cases b : x < 0xC0 <;> simp [a, b] <;> omega

theorem chkL_of_lv (hv : ValidLatin1.LV xs) (pos : Nat) (hp : pos < xs.length) (e : Bool)
    (he : e = true ↔ (1 ≤ pos ∧ 0xC0 ≤ xs.getD (pos - 1) 0)) : chkL (view xs pos) e = true := by
  have hl := view_length (xs := xs) pos
  unfold chkL; rw [List.all_eq_true]; intro p hp'
  have hp64 := List.mem_range.mp hp'
  simp only [beq_iff_eq]
  by_cases hpl : p < (view xs pos).length
  · have hb := view_byte (xs := xs) pos p hpl
    obtain ⟨f1, -⟩ := lv_facts hv (pos + p) (by omega)
    have lhs : (na (view xs pos) p != ld (view xs pos) p) = isCont (xs.getD (pos + p) 0) := by
      simp only [na, ld, hpl, decide_true, Bool.true_and, hb]; exact bne_isCont _
    rw [lhs, Bool.eq_iff_iff, f1]
    by_cases p0 : p = 0
    · subst p0
      have : lsh (view xs pos) e 0 = e := by simp [lsh]
      rw [this, he, Nat.add_zero]
    · have hb1 := view_byte (xs := xs) pos (p - 1) (by omega)
      have hld : ld (view xs pos) (p - 1) = decide (0xC0 ≤ xs.getD (pos + p - 1) 0) := by
        simp only [ld, show p - 1 < (view xs pos).length by omega, decide_true, Bool.true_and, hb1,
          show pos + (p - 1) = pos + p - 1 by omega]
      have : lsh (view xs pos) e p = ld (view xs pos) (p - 1) := by
        simp [lsh, hp64, show 1 ≤ p by omega, p0]
      rw [this, hld, decide_eq_true_iff]
      exact ⟨fun h => h.2, fun h => ⟨by omega, h⟩⟩
  · have lhs : (na (view xs pos) p != ld (view xs pos) p) = false := by
      simp [na, ld, hpl]
    rw [lhs]
    have hp1 : 1 ≤ p := by omega
    simp only [lsh, hp64, decide_true, Bool.true_and, hp1, show ¬ p = 0 by omega, decide_false,
      Bool.false_and, Bool.or_false]
    by_cases hq : p - 1 < (view xs pos).length
    · have hend : pos + (view xs pos).length = xs.length := by omega
      have hb1 := view_byte (xs := xs) pos (p - 1) hq
      obtain ⟨-, f2⟩ := lv_facts hv (pos + (p - 1)) (by omega)
      simp only [ld, hq, decide_true, Bool.true_and, hb1]
      symm; simp only [decide_eq_false_iff_not]
      intro h; have := f2 h; omega
    · simp [ld, hq]

/-- The `convert_valid` Latin-1 kernel on `LV` input: `next_leading` is set
exactly when the byte before the cursor is a lead, the cursor plus that lead
is `L` of the consumed bytes. -/
theorem runL1_valid (hv : ValidLatin1.LV xs) : ∀ f pos out (e : Bool),
    out + ind e = L ((xs.take pos).map w01) →
    (e = true ↔ (1 ≤ pos ∧ 0xC0 ≤ xs.getD (pos - 1) 0)) → pos ≤ xs.length →
    (runL1 false xs f pos out e).wEnd ≤ L (xs.map w01) ∧
    (runL1 false xs f pos out e).rEnd ≤ xs.length := by
  intro f
  induction f with
  | zero =>
    intro pos out e ho _ hp
    have := Lw_le (xs := xs) pos
    exact ⟨show out ≤ _ by omega, hp⟩
  | succ f ih =>
    intro pos out e ho he hp
    have hl := view_length (xs := xs) pos
    have hW := Lw_le (xs := xs) pos
    unfold runL1
    by_cases hlt : pos < xs.length
    · rw [ite_eq_left hlt]
      have hs := Lt_add (xs := xs) w01 pos (view xs pos).length
      rw [← view_take _ _ (by omega), List.take_of_length_le (Nat.le_refl _)] at hs
      have hW' := Lw_le (xs := xs) (pos + (view xs pos).length)
      have hlast : pos + (view xs pos).length < xs.length → (view xs pos).length = 64 := by omega
      -- the block's outcome and the new carry
      have key : ∃ w e', blockL1 false (view xs pos) e = some (w, e') ∧
          w + ind e' = L ((view xs pos).map w01) + ind e ∧
          (e' = true ↔ (1 ≤ pos + (view xs pos).length ∧
            0xC0 ≤ xs.getD (pos + (view xs pos).length - 1) 0)) := by
        have hlastb := view_byte (xs := xs) pos ((view xs pos).length - 1) (by omega)
        rw [show pos + ((view xs pos).length - 1) = pos + (view xs pos).length - 1 by omega] at hlastb
        obtain ⟨-, flast⟩ := lv_facts hv (pos + (view xs pos).length - 1) (by omega)
        unfold blockL1
        by_cases ha : anyB (na (view xs pos)) = false
        · -- ASCII block: no lead before it
          have he0 : e = false := by
            cases e
            · rfl
            · have hc := (lv_facts hv pos hlt).1.mpr (he.mp rfl)
              have := na_false ha (by omega) 0 (by omega)
              rw [view_byte pos 0 (by omega), Nat.add_zero] at this
              unfold isCont at hc; simp only [Bool.and_eq_true, decide_eq_true_eq] at hc; omega
          subst he0
          refine ⟨(view xs pos).length, false, by simp [ha], ?_, ?_⟩
          · rw [L_ones w01 (fun p hp => w01_lt _ (na_false ha (by omega) p hp))]
          · have := na_false ha (by omega) _ (show (view xs pos).length - 1 < _ by omega)
            rw [hlastb] at this
            simp only [Bool.false_eq_true, false_iff, not_and]; intro _ h; omega
        · have ha' : anyB (na (view xs pos)) = true := by simpa using ha
          have hc := chkL_of_lv hv pos hlt e he
          have hcount := blockL1_count (by omega) e hc
          simp only [ha', Bool.not_true, Bool.false_eq_true, ite_false, Bool.false_and]
          have hld : ld (view xs pos) 63 = true ↔ (1 ≤ pos + (view xs pos).length ∧
              0xC0 ≤ xs.getD (pos + (view xs pos).length - 1) 0) := by
            by_cases h64 : (view xs pos).length = 64
            · rw [← hlastb, h64]; simp [ld, h64] <;> omega
            · have hend : pos + (view xs pos).length = xs.length := by omega
              simp only [ld, show ¬ 63 < (view xs pos).length by omega, decide_false,
                Bool.false_and, Bool.false_eq_true, false_iff, not_and]
              intro _ h; have := flast h; omega
          split
          · rename_i h0
            refine ⟨_, _, rfl, by rw [← hcount, h0], hld⟩
          · exact ⟨_, _, rfl, hcount, hld⟩
      obtain ⟨w, e', hb, hc, he'⟩ := key
      rw [hb]
      have hr := ih (pos + (view xs pos).length) (out + w) e' (by omega) he' (by omega)
      exact ⟨Nat.max_le.mpr ⟨by cases e' <;> simp [ind] at * <;> omega, hr.1⟩,
        Nat.max_le.mpr ⟨by omega, hr.2⟩⟩
    · rw [ite_eq_right hlt]; exact ⟨show out ≤ _ by omega, hp⟩

/-- **`convert_valid_utf8_to_latin1` on Ice Lake**: on the input its contract
allows (valid UTF-8 that Latin-1 represents, `ValidLatin1.LV`), no store ends
past `latin1_length_from_utf8` of the input, and no load reads past it. -/
theorem convL1valid_safe (hv : ValidLatin1.LV xs) :
    convL1valid xs ≤ L (xs.map w01) ∧ (runL1 false xs xs.length 0 0 false).rEnd ≤ xs.length :=
  runL1_valid hv xs.length 0 0 false (by simp) (by simp) (Nat.zero_le _)

end L1Run

/-- `Valid8` (`run 0 xs = 0`) alone does not suffice for `convert_valid_utf8_to_latin1`: on the
valid UTF-8 `€` (`E2 82 AC`), outside the contract, the kernel keeps the two
continuation bytes and stores 2 bytes into a buffer of
`latin1_length_from_utf8 = 1`. -/
theorem latin1_valid_needs_LV :
    run 0 [0xE2, 0x82, 0xAC] = 0 ∧ convL1valid [0xE2, 0x82, 0xAC] = 2 ∧
      L ([0xE2, 0x82, 0xAC].map w01) = 1 := by decide

end SafetyMargins.IcelakeUtf8

/-! ## Summary, and the functions for the differential test -/

namespace SafetyMargins.IcelakeUtf8
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.ValidExact SafetyMargins.Exact

/-- **The Ice Lake UTF-8 decoders, with real stores.** This replaces the path
table `Arch.Icelake.utf8` and `Arch.Icelake.safe_utf8`, which assumed the
exactness of the masked stores. For every byte list `xs` (every input for the
validating functions, every verdict of the UTF-32 checker), the furthest store
of each entry point ends at or before the buffer size its caller computes,
and (for the vector loops) the furthest load ends at or before the input:

* UTF-16, `utf16_length_from_utf8` (`w16`): `convert`, `_with_errors`, and
  `convert_valid` on valid UTF-8;
* UTF-32, `utf32_length_from_utf8` (`w01`): `convert`, `_with_errors`,
  `convert_valid` (on every input);
* Latin-1, `latin1_length_from_utf8` (`w01`): `convert`, `_with_errors`, and
  `convert_valid` on valid Latin-1-representable UTF-8 (`LV`). -/
theorem icelake_utf8_safe (xs : List Nat) (f : Nat) (chk : Nat → Bool) (chkEnd : Bool) :
    conv16 xs f ≤ L (xs.map w16) ∧ conv16err xs f ≤ L (xs.map w16) ∧
    (Valid8 xs → conv16valid xs f ≤ L (xs.map w16)) ∧
    (run16 true xs f 0 0).rEnd ≤ xs.length ∧
    (Valid8 xs → (run16 false xs f 0 0).rEnd ≤ xs.length) ∧
    conv32 chk chkEnd xs ≤ L (xs.map w01) ∧ conv32err chk chkEnd xs ≤ L (xs.map w01) ∧
    conv32valid xs ≤ L (xs.map w01) ∧
    (fixed32 false chk chkEnd xs xs.length).rEnd ≤ xs.length ∧
    (fixed32 true chk chkEnd xs xs.length).rEnd ≤ xs.length ∧
    convL1 xs ≤ L (xs.map w01) ∧ convL1err xs ≤ L (xs.map w01) ∧
    (ValidLatin1.LV xs → convL1valid xs ≤ L (xs.map w01)) ∧
    (runL1 true xs xs.length 0 0 false).rEnd ≤ xs.length := by
  refine ⟨(conv16_safe f).1, conv16err_safe f, fun hv => (conv16valid_safe hv f).1,
    (conv16_safe f).2, fun hv => (conv16valid_safe hv f).2,
    (conv32_safe chk chkEnd).1, (conv32err_safe chk chkEnd).1, conv32valid_safe.1,
    (conv32_safe chk chkEnd).2, (conv32err_safe chk chkEnd).2,
    convL1_safe.1, convL1err_safe, fun hv => (convL1valid_safe hv).1, convL1_safe.2⟩

/-- The checker's verdict used to evaluate the UTF-32 models: an error once
the checked bytes are not a prefix of valid UTF-8 (`ValidExact.step` reaches
its dead state 8); at the end, unless the input is valid. This is the
intended semantics of `avx512_utf8_checker` (lookup tables, carry between
blocks, `is_incomplete`, `check_eof`); the differential test checks it. -/
def chkOf (xs : List Nat) : Nat → Bool := fun n => run 0 (xs.take n) == 8
def chkEndOf (xs : List Nat) : Bool := run 0 xs != 0

/-! Furthest store end of each entry point on a byte list, in output units,
for the differential test (fuel: one block per byte suffices). -/
def end16 (xs : List Nat) : Nat := conv16 xs (xs.length + 1)
def end16err (xs : List Nat) : Nat := conv16err xs (xs.length + 1)
def end16valid (xs : List Nat) : Nat := conv16valid xs (xs.length + 1)
def end32 (xs : List Nat) : Nat := conv32 (chkOf xs) (chkEndOf xs) xs
def end32err (xs : List Nat) : Nat := conv32err (chkOf xs) (chkEndOf xs) xs
def end32valid (xs : List Nat) : Nat := conv32valid xs
def endL1 (xs : List Nat) : Nat := convL1 xs
def endL1err (xs : List Nat) : Nat := convL1err xs
def endL1valid (xs : List Nat) : Nat := convL1valid xs

end SafetyMargins.IcelakeUtf8
