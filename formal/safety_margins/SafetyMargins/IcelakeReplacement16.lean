import SafetyMargins.ReplacementLengths
import SafetyMargins.IcelakeReal
/-
The icelake UTF-16 → UTF-8 kernel with details meets `ReplacementLengths.VKSpec'`.

`convert_utf16_to_utf8_with_details` (src/icelake/implementation.cpp:1360-1373)
runs the AVX-512 loop `utf16_to_utf8_avx512i`
(src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:9-206) and, when it did not
consume every unit (`inlen != len`), the scalar
`convert_with_errors<big_endian, false>`
(include/simdutf/scalar/utf16_to_utf8/utf16_to_utf8.h:93-203) on the rest; it
returns `full_result(res.error, inlen + res.input_count, outlen + res.output_count)`
or `full_result(SUCCESS, len, outlen)`. It is the kernel of
`convert_utf16{le,be}_to_utf8_with_errors` (src/icelake/implementation.cpp:1375-1385)
and so of the `_safe` drivers, which read the `result` conversion: the input
position on an error, the output count on success (include/simdutf/error.h:117-123).

Reused transcriptions:
* `IcelakeReal.pass` (one pass from `failiteration`, src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:32-188, paths A, B,
  C, mismatch), `IcelakeReal.firstErr` (`tzcnt(hinolo | lonohi)`, src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:118-120)
  and the lemmas on them; `IcelakeReal.run8` (store ends) and
  `safe_utf16to8_kernel`.
* `ReplacementClose.cwf` / `cwfLoop`: the scalar tail with `check_output = false`.

New here (`loop`): what the C++ returns, `inbuf - inbuf_orig + adjust`
(src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:205) and `outbuf - outbuf_orig + adjust` (:204). A pass that ends the
function (the tail, src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:192-202, or the second pass after a mismatch,
src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:120-125, with `adjust = inlen - 31`) adds `adv + adjust` to `outbuf`
on paths A and B (they jump to `tail`, src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:42-43 and :73-74, skipping :190)
and `adv` on path C (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:190 cancels `adjust`): `outAdd`.

Values are units after the byte swap (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:25-27, :197-199), as in
`IcelakeReal`; the scalar tail's 4-unit ASCII probe on raw memory is the
parameter `probe` (its soundness is `ReplacementLengths.ProbeOK`; the real one
is `Scalar.probeU16 be` on the input from the probe position, which is how the
byte order enters).

Results: `loop_spec` (exact output count `2·carry + out = L` of the consumed
units; on a stop, the consumed units pass the surrogate check and do not end
high); `iceVK_spec : VKSpec' (iceVK probe)`; end to end `replSafe_icelake`,
`dSafe_icelake` with the icelake length function `LengthsIcelake.ice8of16`.
-/
namespace SafetyMargins.IcelakeReplacement16
open SafetyMargins SafetyMargins.Replacement SafetyMargins.ReplacementMore
  SafetyMargins.ReplacementClose SafetyMargins.ReplacementLengths
open SafetyMargins.IcelakeReal (pass firstErr Res chk endP cb loS hiS run8)
set_option autoImplicit false

/-! ## The model -/

/-- What a pass that ends the function adds to `*outlen` (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:190, :204):
on paths A and B (every active lane `< 0x800`, src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:36, :52) the cursor moved
`adv` (31, or `31 + popcount(is234byte)`) and `adjust = n - 31` corrects it;
on path C `outbuf += -adjust` (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:190) cancels the `+ adjust` of :204.
`n` is the `inlen` that set `adjust` (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:123, :200). -/
def outAdd (a : List Nat) (n adv : Nat) : Nat :=
  if a.all (· < 0x800) then adv + n - 31 else adv

/-- The second pass after a mismatch (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:117-125): `inlen` is the first
error lane `k`, `inmask` keeps the `k` lanes before it, the others are cleared,
`adjust = k - 31`, `inlen = 0`; the function then ends.
`(input consumed, output count, end of stores, stopped)`; the second pass never
mismatches (`rerun_spec`), the `.mismatch` value is unreachable. -/
def rerun (c : Bool) (a z : List Nat) : Nat × Nat × Nat × Bool :=
  let k := firstErr c a z
  match pass c (a.take k) (List.replicate (32 - k) 0) with
  | .ok b adv _ => (k, outAdd (a.take k) k adv, b, true)
  | .mismatch => (k, 0, 0, true)

/-- **`utf16_to_utf8_avx512i`** (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:9-206)
on the units `us` not yet consumed, carry `c`: `(input consumed, output
count, end of stores, stopped on a mismatch)`, from the real cursor.
* `inlen >= 32` (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:23): load 32 lanes, 31 active (:13, :24-30); a pass
  that succeeds advances 31 units and `adv` bytes (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:39, :70, :188) and loops
  with the new carry; a mismatch reruns (`rerun`) and ends.
* fewer than 32 units: none, nothing (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:193); else the tail pass on them,
  the other lanes 0 (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:195-202, `adjust = inlen - 31`), which ends the
  function, or its rerun on a mismatch. -/
def loop (c : Bool) (us : List Nat) : Nat × Nat × Nat × Bool :=
  if us.length < 32 then
    if us.length = 0 then (0, 0, 0, false)
    else
      match pass c us (List.replicate (32 - us.length) 0) with
      | .ok b adv _ => (us.length, outAdd us us.length adv, b, false)
      | .mismatch => rerun c us (List.replicate (32 - us.length) 0)
  else
    match pass c (us.take 31) [(us.drop 31).headD 0] with
    | .ok b adv co =>
      let r := loop co (us.drop 31)
      (31 + r.1, adv + r.2.1, max b (adv + r.2.2.1), r.2.2.2)
    | .mismatch => rerun c (us.take 31) [(us.drop 31).headD 0]
termination_by us.length
decreasing_by simp; omega

/-- **`convert_utf16_to_utf8_with_details`** (src/icelake/implementation.cpp:1360-1373),
as the `result` the drivers read (include/simdutf/error.h:117-123): the loop
from no carry (src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:21), then, if `inlen != len`, the scalar
`convert_with_errors<big_endian, false>` on the rest, its output at
`utf8_output + outlen`. `probe zs p` is the scalar 4-unit ASCII test at `p`
of the tail's input `zs`. -/
def iceVK (probe : List Nat → Nat → Bool) (ys : List Nat) : ReplacementRest.KR :=
  let r := loop false ys
  if r.1 ≠ ys.length then
    let t := cwf (probe (ys.drop r.1)) (ys.drop r.1)
    if t.err = 0 then ⟨0, r.2.1 + t.outp, max r.2.2.1 (r.2.1 + t.hi)⟩
    else ⟨t.err, r.1 + t.inp, max r.2.2.1 (r.2.1 + t.hi)⟩
  else ⟨0, r.2.1, r.2.2.1⟩

/-! ## The surrogate check and unpaired surrogates -/

theorem chk_append (p : Bool) (a b : List Nat) :
    chk p (a ++ b) = (chk p a && chk (endP p a) b) := by
  induction a generalizing p with
  | nil => simp [chk, endP]
  | cons x xs ih => simp [chk, endP, ih, Bool.and_assoc]

theorem endP_append (p : Bool) (a b : List Nat) : endP p (a ++ b) = endP (endP p a) b := by
  induction a generalizing p with
  | nil => rfl
  | cons x xs ih => exact ih _

theorem isHigh_hiS (u : Nat) : isHigh u = hiS u := by
  simp only [isHigh, hiS]; apply decide_eq_decide.mpr; omega

theorem isLow_loS (u : Nat) : isLow u = loS u := by
  simp only [isLow, loS]; apply decide_eq_decide.mpr; omega

theorem isSur_eq (u : Nat) : isSur u = (hiS u || loS u) := by
  simp only [isSur, hiS, loS]
  cases h1 : decide (0xD800 ≤ u ∧ u < 0xDC00) <;> cases h2 : decide (0xDC00 ≤ u ∧ u < 0xE000) <;>
    simp_all <;> omega

/-- **Units that pass the check from no carry and do not end high have no
unpaired surrogate.** -/
theorem unp_of_chk : ∀ n (xs : List Nat), xs.length ≤ n → chk false xs = true →
    endP false xs = false → unp xs = 0 := by
  intro n
  induction n with
  | zero => intro xs hl _ _; cases xs with | nil => rfl | cons _ _ => simp at hl
  | succ n ih =>
    intro xs hl hc he
    cases xs with
    | nil => rfl
    | cons x rest =>
      simp only [chk, Bool.and_eq_true, beq_iff_eq] at hc
      simp only [endP] at he
      obtain ⟨hlx, hcr⟩ := hc
      cases hh : hiS x
      · rw [hh] at hcr he
        have hs : isSur x = false := by rw [isSur_eq, hh, hlx]; rfl
        cases rest with
        | nil => simp [unp, hs]
        | cons y r =>
          have hx : isHigh x = false := by rw [isHigh_hiS, hh]
          simp only [unp, hx, Bool.false_and, Bool.false_eq_true, ↓reduceIte, hs, Nat.zero_add]
          exact ih _ (by simp at hl ⊢; omega) hcr he
      · rw [hh] at hcr he
        cases rest with
        | nil => simp [endP] at he
        | cons y r =>
          simp only [chk, Bool.and_eq_true, beq_iff_eq] at hcr
          obtain ⟨hly, hcr'⟩ := hcr
          have hhy := IcelakeReal.lo_not_hi y hly
          rw [hhy] at hcr'
          simp only [endP] at he
          rw [hhy] at he
          have hx : isHigh x = true := by rw [isHigh_hiS, hh]
          have hy : isLow y = true := by rw [isLow_loS, hly]
          simp only [unp, hx, hy, Bool.and_self, ↓reduceIte]
          exact ih _ (by simp at hl ⊢; omega) hcr' he

/-! ## One final pass -/

/-- A pass on `a` with the other lanes 0, that does not mismatch: its
`outAdd` is the exact output, `L` of `a` (minus the carried-in low surrogate
already written). -/
theorem pass_out_zeros (c : Bool) (a : List Nat) (m b adv : Nat) (co : Bool) (hm : 1 ≤ m)
    (hv : ∀ v ∈ a, v < 65536) (hinv : c = true → loS (a.headD 0) = true)
    (h : pass c a (List.replicate m 0) = .ok b adv co) :
    outAdd a a.length adv + 2 * cb c = L (a.map w8) := by
  show _ = L (a.map Arch.Icelake.w8)
  unfold pass at h
  split at h
  · rename_i hA
    cases h
    have hc := IcelakeReal.c_false c a hinv (by
      simp only [List.all_eq_true, decide_eq_true_eq] at hA ⊢
      exact fun x hx => Nat.lt_trans (hA x hx) (by decide))
    subst hc
    have := IcelakeReal.L_w8_ascii a hA
    have hB : a.all (· < 0x800) = true := by
      simp only [List.all_eq_true, decide_eq_true_eq] at hA ⊢
      exact fun x hx => Nat.lt_trans (hA x hx) (by decide)
    simp only [outAdd, hB, ↓reduceIte, cb]
    simp only [Bool.false_eq_true, ↓reduceIte] at this ⊢
    omega
  · split at h
    · rename_i _ hB
      cases h
      have hc := IcelakeReal.c_false c a hinv hB
      subst hc
      have h2 := IcelakeReal.L_is2 a hB
      simp only [outAdd, hB, ↓reduceIte, cb, Bool.false_eq_true]
      omega
    · rename_i _ hB
      split at h
      · cases h
      · rename_i hm'
        cases h
        obtain ⟨hc, hz⟩ := IcelakeReal.checked c a (List.replicate m 0) hinv (Bool.eq_false_iff.mpr hm')
        have hP := IcelakeReal.pathChk c a ((List.replicate m 0).headD 0) (List.replicate m 0).length hv hc
        obtain ⟨m', rfl⟩ : ∃ m', m = m' + 1 := ⟨m - 1, by omega⟩
        simp only [List.replicate_succ, IcelakeReal.chkZ, Bool.and_eq_true, beq_iff_eq] at hz
        rw [IcelakeReal.loS_zero] at hz
        rw [← hz.1] at hP
        simp only [outAdd, hB, Bool.false_eq_true, ↓reduceIte]
        simp only [cb, Bool.false_eq_true, ↓reduceIte] at hP ⊢
        omega

/-- **The second pass after a mismatch.** The error lane `k` is within the
active lanes; the `k` lanes before it pass the check and do not end high; the
second pass does not mismatch and its output count is exact. -/
theorem rerun_spec (c : Bool) (a z : List Nat) (ha : a.length ≤ 31)
    (hv : ∀ v ∈ a, v < 65536) (hz : ∀ y ∈ z.drop 1, loS y = false)
    (hinv : c = true → IcelakeReal.headLo a z = true) (hinv' : c = true → loS (a.headD 0) = true)
    (hmis : pass c a z = .mismatch) :
    (rerun c a z).1 ≤ a.length ∧
    (rerun c a z).2.1 + 2 * cb c = L ((a.take (rerun c a z).1).map w8) ∧
    (rerun c a z).2.2.2 = true ∧
    chk c (a.take (rerun c a z).1) = true ∧ endP c (a.take (rerun c a z).1) = false := by
  have hk : firstErr c a z ≤ a.length := by
    apply Classical.byContradiction
    intro hlt
    have := IcelakeReal.chk_of_noErr c a z hinv hz (by omega)
    have hm := IcelakeReal.pass_mismatch c a z hmis
    rw [this.1, this.2] at hm
    simp at hm
  obtain ⟨hc', he'⟩ := IcelakeReal.chk_take_firstErr c a z hinv hk
  unfold rerun
  generalize firstErr c a z = K at hk hc' he' ⊢
  have hv' : ∀ v ∈ a.take K, v < 65536 := fun v hv0 => hv v (List.mem_of_mem_take hv0)
  have hinvK : c = true → loS ((a.take K).headD 0) = true := by
    intro hc
    by_cases hK0 : K = 0
    · subst hK0; simp [endP, hc] at he'
    · rw [IcelakeReal.take_headD a K (by omega)]; exact hinv' hc
  have hlen : (a.take K).length = K := by simp; omega
  cases hp : pass c (a.take K) (List.replicate (32 - K) 0) with
  | ok b' adv' co' =>
    simp only [hp]
    have := pass_out_zeros c (a.take K) (32 - K) b' adv' co' (by omega) hv' hinvK hp
    rw [hlen] at this
    exact ⟨hk, this, trivial, hc', he'⟩
  | mismatch =>
    exfalso
    have hm := IcelakeReal.pass_mismatch c _ _ hp
    obtain ⟨m', hm'⟩ : ∃ m', 32 - K = m' + 1 := ⟨31 - K, by omega⟩
    rw [hc', he', hm', List.replicate_succ] at hm
    simp [IcelakeReal.chkZ, IcelakeReal.loS_zero, IcelakeReal.chkZ_zeros] at hm

/-- A pass of the main loop that succeeds: the 31 units pass the check and the
carry out is whether they end high. -/
theorem pass_ok_chk (c : Bool) (a : List Nat) (y b adv : Nat) (co : Bool) (ha : a.length = 31)
    (hinv : c = true → loS (a.headD 0) = true) (h : pass c a [y] = .ok b adv co) :
    chk c a = true ∧ endP c a = co := by
  unfold pass at h
  split at h
  · rename_i hA
    cases h
    have hB : a.all (· < 0x800) = true := by
      simp only [List.all_eq_true, decide_eq_true_eq] at hA ⊢
      exact fun x hx => Nat.lt_trans (hA x hx) (by decide)
    have hc := IcelakeReal.c_false c a hinv hB
    subst hc
    have hh : a.any hiS = false := by
      simp only [List.any_eq_false]; intro x hx
      simp only [List.all_eq_true, decide_eq_true_eq] at hB
      have := hB x hx; simp [hiS]; omega
    have hl : a.any loS = false := by
      simp only [List.any_eq_false]; intro x hx
      simp only [List.all_eq_true, decide_eq_true_eq] at hB
      have := hB x hx; simp [loS]; omega
    exact ⟨IcelakeReal.chk_nosur a hh hl, IcelakeReal.endP_nohi false a hh rfl⟩
  · split at h
    · rename_i _ hB
      cases h
      have hc := IcelakeReal.c_false c a hinv hB
      subst hc
      have hh : a.any hiS = false := by
        simp only [List.any_eq_false]; intro x hx
        simp only [List.all_eq_true, decide_eq_true_eq] at hB
        have := hB x hx; simp [hiS]; omega
      have hl : a.any loS = false := by
        simp only [List.any_eq_false]; intro x hx
        simp only [List.all_eq_true, decide_eq_true_eq] at hB
        have := hB x hx; simp [loS]; omega
      exact ⟨IcelakeReal.chk_nosur a hh hl, IcelakeReal.endP_nohi false a hh rfl⟩
    · split at h
      · cases h
      · rename_i hm
        cases h
        obtain ⟨hc, -⟩ := IcelakeReal.checked c a [y] hinv (Bool.eq_false_iff.mpr hm)
        exact ⟨hc, (IcelakeReal.endP_drop30 c a ha).symm⟩

/-! ## The loop -/

theorem headLo_take31 (us : List Nat) (h : 32 ≤ us.length) (y : Nat) :
    IcelakeReal.headLo (us.take 31) [y] = loS (us.headD 0) := by
  cases us with
  | nil => simp at h
  | cons x xs => rfl

/-- **The loop.** For every input of 16-bit units and carry `c` (followed by
a low surrogate when set, the loop's invariant):
* the input count is within the input;
* the output count is exact: `2·c + out = L` of the consumed units (the low
  surrogate carried in was written with its pair);
* if it did not stop, it consumed everything;
* if it stopped, the consumed units pass the surrogate check and do not end
  high. -/
theorem loop_spec : ∀ n (c : Bool) (us : List Nat), us.length ≤ n → (∀ v ∈ us, v < 65536) →
    (c = true → loS (us.headD 0) = true) →
    (loop c us).1 ≤ us.length ∧
    (loop c us).2.1 + 2 * cb c = L ((us.take (loop c us).1).map w8) ∧
    ((loop c us).2.2.2 = false → (loop c us).1 = us.length) ∧
    ((loop c us).2.2.2 = true →
      chk c (us.take (loop c us).1) = true ∧ endP c (us.take (loop c us).1) = false) := by
  intro n
  induction n using Nat.strongRecOn with
  | ind n ih =>
  intro c us hn hv hinv
  rw [loop]
  by_cases hs : us.length < 32
  · rw [ite_eq_left hs]
    by_cases h0 : us.length = 0
    · rw [ite_eq_left h0]
      have hc : c = false := by
        cases c
        · rfl
        · have := hinv rfl
          rw [List.length_eq_zero_iff] at h0; subst h0; simp [loS] at this
      subst hc
      simp [cb, h0]
    rw [ite_eq_right h0]
    obtain ⟨m, hm⟩ : ∃ m, 32 - us.length = m + 1 := ⟨31 - us.length, by omega⟩
    cases hp : pass c us (List.replicate (32 - us.length) 0) with
    | ok b adv co =>
      simp only
      have := pass_out_zeros c us _ b adv co (by omega) hv hinv hp
      refine ⟨Nat.le_refl _, by rw [List.take_length]; exact this, fun _ => by simp, fun h => by simp at h⟩
    | mismatch =>
      have := rerun_spec c us _ (by omega) hv
        (by rw [hm]; intro y hy; simp [List.replicate_succ] at hy; rw [hy.2]; exact IcelakeReal.loS_zero)
        (by rw [hm, IcelakeReal.headLo_zeros]; exact hinv) hinv hp
      dsimp only
      obtain ⟨h1, h2, h3, h4, h5⟩ := this
      generalize rerun c us (List.replicate (32 - us.length) 0) = r at h1 h2 h3 h4 h5 ⊢
      refine ⟨h1, h2, fun h => ?_, fun _ => ⟨h4, h5⟩⟩
      rw [h3] at h; cases h
  · rw [ite_eq_right hs]
    have h32 : 32 ≤ us.length := by omega
    have hd := IcelakeReal.drop31_cons us h32
    have hv31 : ∀ v ∈ us.take 31, v < 65536 := fun v h0 => hv v (List.mem_of_mem_take h0)
    have hinv' : c = true → loS ((us.take 31).headD 0) = true := by
      rw [IcelakeReal.take_headD us 31 (by decide)]; exact hinv
    have hl31 : (us.take 31).length = 31 := by simp; omega
    cases hp : pass c (us.take 31) [(us.drop 31).headD 0] with
    | ok b adv co =>
      simp only
      have hy : (us.drop 31).headD 0 < 65536 :=
        hv _ (List.mem_of_mem_drop (l := us) (i := 31) (by rw [hd]; simp))
      obtain ⟨hadv, hco, -⟩ := IcelakeReal.pass_ok_normal c (us.take 31) _ b adv co hl31
        hv31 hy hinv' hp
      obtain ⟨hck, hen⟩ := pass_ok_chk c (us.take 31) _ b adv co hl31 hinv' hp
      have hr := ih (us.length - 31) (by omega) co (us.drop 31) (by simp)
        (fun v h0 => hv v (List.mem_of_mem_drop h0)) (by intro h; rw [hd]; exact hco h)
      obtain ⟨r1, r2, r3, r4⟩ := hr
      generalize loop co (us.drop 31) = r at r1 r2 r3 r4
      have htk : us.take (31 + r.1) = us.take 31 ++ (us.drop 31).take r.1 := List.take_add ..
      refine ⟨by simp at r1; omega, ?_, fun h => by rw [r3 h]; simp; omega, fun h => ?_⟩
      · have hadv' : 2 * cb c + adv = L ((us.take 31).map w8) + 2 * cb co := hadv
        rw [htk, List.map_append, L_append]; omega
      · rw [htk, chk_append, endP_append, hen, hck]
        exact ⟨by simpa using (r4 h).1, (r4 h).2⟩
    | mismatch =>
      simp only
      have := rerun_spec c (us.take 31) [(us.drop 31).headD 0] (by omega) hv31 (by simp)
        (by rw [headLo_take31 us h32]; exact hinv) hinv' hp
      obtain ⟨h1, h2, h3, h4, h5⟩ := this
      generalize rerun c (us.take 31) [(us.drop 31).headD 0] = r at h1 h2 h3 h4 h5
      have ht : (us.take 31).take r.1 = us.take r.1 := by
        rw [List.take_take, Nat.min_eq_left (by omega)]
      rw [ht] at h2 h4 h5
      refine ⟨by omega, h2, fun h => ?_, fun _ => ⟨h4, h5⟩⟩
      rw [h3] at h; cases h

/-- The end of the loop's stores is `run8` (with enough fuel), whose bound
`safe_utf16to8_kernel` is proved in `IcelakeReal`. -/
theorem loop_hi_run8 : ∀ n (c : Bool) (us : List Nat), us.length < n →
    (loop c us).2.2.1 = run8 n c us := by
  intro n
  induction n with
  | zero => intro _ _ h; omega
  | succ n ih =>
    intro c us hn
    rw [loop]
    unfold run8
    by_cases hs : us.length < 32
    · rw [ite_eq_left hs, ite_eq_left hs]
      by_cases h0 : us.length = 0
      · rw [ite_eq_left h0]
        rw [List.length_eq_zero_iff] at h0; subst h0
        rfl
      rw [ite_eq_right h0]
      unfold IcelakeReal.attempt rerun
      cases pass c us (List.replicate (32 - us.length) 0) with
      | ok b adv co => rfl
      | mismatch =>
        simp only
        cases pass c (us.take (firstErr c us (List.replicate (32 - us.length) 0)))
          (List.replicate (32 - firstErr c us (List.replicate (32 - us.length) 0)) 0) <;> rfl
    · rw [ite_eq_right hs, ite_eq_right hs]
      unfold IcelakeReal.attempt rerun
      cases pass c (us.take 31) [(us.drop 31).headD 0] with
      | ok b adv co =>
        simp only
        rw [ih co (us.drop 31) (by simp; omega)]
      | mismatch =>
        simp only
        cases pass c ((us.take 31).take (firstErr c (us.take 31) [(us.drop 31).headD 0]))
          (List.replicate (32 - firstErr c (us.take 31) [(us.drop 31).headD 0]) 0) <;> rfl

/-! ## `VKSpec'` -/

theorem getD_drop (xs : List Nat) (p k : Nat) : (xs.drop p).getD k 0 = xs.getD (p + k) 0 := by
  simp [List.getD_eq_getElem?_getD, List.getElem?_drop]

/-- **The icelake kernel with details meets `VKSpec'`**, for every scalar
probe that is sound on 16-bit units: stores end within
`utf8_length_from_utf16` of the input, so does the count on success, and on
an error the count is a unit of the input with no unpaired surrogate before
it. -/
theorem iceVK_spec (probe : List Nat → Nat → Bool) (hpr : ProbeOK probe) :
    VKSpec' (iceVK probe) := by
  have key : ∀ ys, U16 ys →
      (loop false ys).1 ≤ ys.length ∧
      (loop false ys).2.1 = L ((ys.take (loop false ys).1).map w8) ∧
      (loop false ys).2.2.1 ≤ L (ys.map w8) ∧
      ((loop false ys).2.2.2 = false → (loop false ys).1 = ys.length) ∧
      ((loop false ys).2.2.2 = true → unp (ys.take (loop false ys).1) = 0) := by
    intro ys hu
    obtain ⟨l1, l2, l3, l4⟩ := loop_spec ys.length false ys (Nat.le_refl _) hu (by simp)
    have hhi := IcelakeReal.safe_utf16to8_kernel (ys.length + 1) ys hu
    rw [← loop_hi_run8 _ _ _ (by omega)] at hhi
    refine ⟨l1, by simpa [cb] using l2, hhi, l3, fun h => ?_⟩
    obtain ⟨c1, c2⟩ := l4 h
    exact unp_of_chk _ _ (Nat.le_refl _) c1 c2
  refine ⟨fun ys hu => ?_, fun ys hu => ?_⟩
  · obtain ⟨k1, k2, k3, -, -⟩ := key ys hu
    unfold iceVK
    generalize loop false ys = r at k1 k2 k3
    have hsp := IcelakeReal.L_map_split w8 ys r.1
    simp only
    split
    · rename_i hne
      obtain ⟨t1, t2, t3⟩ := cwf_spec (ys.drop r.1) (ReplacementRest.U16_drop hu r.1)
        (probe (ys.drop r.1))
      split
      · refine ⟨by simp only; omega, fun _ => by simp only; omega, fun h => by simp at h⟩
      · rename_i he
        rcases t3 with h | ⟨-, h⟩
        · exact absurd h he
        refine ⟨by simp only; omega, fun h => by simp at h; exact absurd h he, fun _ => ?_⟩
        simp only [List.length_drop] at h ⊢; omega
    · rename_i heq
      have heq' : r.1 = ys.length := by simpa using heq
      refine ⟨by simp only; omega, fun _ => ?_, fun h => by simp at h⟩
      simp only; rw [k2, heq', List.take_length]; exact Nat.le_refl _
  · intro he
    obtain ⟨k1, -, -, k4, k5⟩ := key ys hu
    unfold iceVK at he ⊢
    generalize loop false ys = r at k1 k4 k5 he ⊢
    simp only at he ⊢
    by_cases hne : r.1 ≠ ys.length
    · rw [ite_eq_left hne] at he ⊢
      have hstop : r.2.2.2 = true := by
        cases h : r.2.2.2
        · exact absurd (k4 h) hne
        · rfl
      have hpre := k5 hstop
      have hU := ReplacementRest.U16_drop hu r.1
      have hut := cwfLoop_unp (ys.drop r.1) hU (probe (ys.drop r.1))
        (fun p h1 h2 k hk => by
          have := hpr (ys.drop r.1) p hU h1 h2 k hk; exact this)
        (2 * (ys.drop r.1).length + 2) 0 0 0 0 (by simp [unp]) (by omega) (by simp)
      obtain ⟨-, -, t3⟩ := cwf_spec (ys.drop r.1) hU (probe (ys.drop r.1))
      unfold cwf at he ⊢ t3
      generalize cwfLoop (ys.drop r.1) (probe (ys.drop r.1)) (2 * (ys.drop r.1).length + 2)
        0 0 0 0 = t at he hut t3 ⊢
      by_cases h0 : t.err = 0
      · simp [h0] at he
      simp only [h0, ↓reduceIte] at he ⊢
      rcases t3 with h | ⟨h1, -⟩
      · exact absurd h h0
      rw [List.take_add]; exact unp_append _ _ hpre (hut h1)
    · rw [ite_eq_right hne] at he; simp at he

/-! ## End to end -/

/-- **`convert_utf16_to_utf8_with_replacement_safe`**
(src/implementation.cpp:2494-2539) on icelake, from the entry point, on every
input of 16-bit units and every `utf8_len`: every store ends inside
`utf8_len` and so does `output_count`. The kernel is `iceVK` (with the scalar
probe `kprobe` of its tail, sound on 16-bit units), the length function is
icelake's `utf8_length_from_utf16` (`LengthsIcelake.ice8of16`); the driver's
own scalar pieces (`convert_with_errors<…, true>`, `convert_with_replacement_safe`)
are `ReplacementRest.cwe`/`crs` with any probe. No hypothesis but `ProbeOK`. -/
theorem replSafe_icelake (kprobe : List Nat → Nat → Bool) (hpr : ProbeOK kprobe)
    (probe : List Nat → Nat → Bool) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.tLoop (ReplacementRest.dSafe (iceVK kprobe) LengthsIcelake.ice8of16
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).hi ≤ cap ∧
    (ReplacementRest.tLoop (ReplacementRest.dSafe (iceVK kprobe) LengthsIcelake.ice8of16
        (ReplacementRest.cwe probe)) (ReplacementRest.crs (ReplacementRest.cwe probe))
        xs cap (xs.length + 1) 0 0 0).outp ≤ cap := by
  have h := ReplacementRest.tLoop_safe _ _
    (dSafe_spec' _ _ _ (iceVK_spec kprobe hpr) SLSpec'_icelake (ReplacementRest.cwe_spec probe))
    (ReplacementRest.crs_spec _ (ReplacementRest.cwe_spec probe)) xs hu cap (xs.length + 1) 0 0 0
    (by omega) (by omega) (by omega)
  exact ⟨h.1, h.2.1⟩

/-- **`convert_utf16_to_utf8_safe_with_details`** (src/implementation.cpp:2028-2091)
on icelake: stores and `output_count` within `utf8_len`. No hypothesis but
`ProbeOK`. -/
theorem dSafe_icelake (kprobe : List Nat → Nat → Bool) (hpr : ProbeOK kprobe)
    (probe : List Nat → Nat → Bool) (xs : List Nat) (hu : U16 xs) (cap : Nat) :
    (ReplacementRest.dSafe (iceVK kprobe) LengthsIcelake.ice8of16
      (ReplacementRest.cwe probe) xs cap).hi ≤ cap ∧
    (ReplacementRest.dSafe (iceVK kprobe) LengthsIcelake.ice8of16
      (ReplacementRest.cwe probe) xs cap).outp ≤ cap := by
  have h := dSafe_spec' _ _ _ (iceVK_spec kprobe hpr) SLSpec'_icelake
    (ReplacementRest.cwe_spec probe) xs cap hu
  exact ⟨h.1, h.2.1⟩

end SafetyMargins.IcelakeReplacement16
