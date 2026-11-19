import SafetyMargins.Scalar
import SafetyMargins.ValidExact
import SafetyMargins.Arch.Westmere
import SafetyMargins.Arch.Haswell
import SafetyMargins.Arch.Ppc64
/-
The generic validating UTF-8 decoders end to end, on every byte string:
`validating_transcoder::convert` and `convert_with_errors` of
src/generic/utf8_to_utf16/utf8_to_utf16.h (`convert_utf8_to_utf16{le,be}` and
`_with_errors`), src/generic/utf8_to_utf32/utf8_to_utf32.h and
src/generic/utf8_to_latin1/utf8_to_latin1.h, as westmere, haswell, ppc64,
arm64, lsx and lasx compile them. Every store ends at or before `L` of the
input's weights (`Utf8.w16`, `Exact.w01` for UTF-32 and Latin-1), the size of
the buffer the length function gives, and every vector load ends inside the
input. Citations below are to the UTF-16 header; `GR` and `Store` give the
UTF-32 and Latin-1 lines.

* **The loop (`GR`).** The block states of `Utf8.Reach`, on bytes, with the
  loop test at each block head (`Loop`,
  src/generic/utf8_to_utf16/utf8_to_utf16.h:196 and :294, `safety_margin`
  from the backward scan :188-195): an ASCII block (:198-201), a block
  entered on a leading byte (`utf8_continuation_mask & 1`, :219-222 and :318;
  in Latin-1 `_with_errors` the check rejects a continuation byte first,
  `l1_enter`), the calls of the inner `while` (:230 and :336) and leaving it
  at `pos + 52`. A call is a `Utf8.Step` (in any block: `convert` converts a
  block before it looks at the error flag, :251) or, with `chk`, only an exact
  call `Exact.XStep` (the calls of a block that passed `check_utf8_bytes`,
  which is all `_with_errors` converts, :318; the per-call fact
  `ValidatorSound.KR` also reads from the C++). `gr_inv`: every state is a
  `Utf8.Reach` state, so `Utf8.reach_lag` holds; a block head satisfies
  `HeadOK`; with exact calls the cursor is `L` of the consumed bytes.
* **The rewind (`rewind_head`).** `rewind_and_convert_with_errors` steps back
  from a continuation byte to the previous non-continuation byte and converts
  again from there. After a complete character this duplicates it
  (`dup_example`: one unit past `L` after a 2-byte character, two after a
  4-byte one, and the stray byte weighs nothing). It does not happen at the
  heads where the drivers call it. `HeadOK`: the head is 0 (nothing before
  it: on a continuation byte the search finds nothing and returns), or
  follows an ASCII byte (the search stops there and returns,
  `Scalar.rewind_rest`), or is a leading byte (it converts from the head,
  `Scalar.rewind_rest`), or, only when calls need not be exact, the cursor
  lags `L` by one unit (`Utf8.Lag` after an error row), which pays for the
  bytes stepped over: one byte and continuation bytes, at most one unit
  under `Exact.w01` (`window_le1`). A vector block ends on a leading byte
  (`XStep` and `Step.conv` end before one) or after an error row, an ASCII
  block or the ASCII tail after an ASCII byte. So the rewind needs no
  margin, and `Scalar.rewind_total` is not used: its hypothesis asks the
  cursor to be at most `L` of the bytes before every byte the search could
  reach, which a cursor at `L` of the consumed bytes does not satisfy. The
  UTF-16 `_with_errors` theorem uses exact calls (`chk = true`), as a 4-byte
  lead weighs 2; the UTF-32 and Latin-1 ones do not need them.
* **Stores (`Store`).** The scalar conversion of an input shorter than 64
  bytes (:176-177, :273-274); the 64-unit ASCII stores; a call storing `q.s`
  units for a path `q` of the target's list with `q.f ≤ cont + L` of the
  bytes it inspects (`Utf8.reach_L`; `okK K` gives `q.s ≤ q.f + K`);
  `convert_ascii_tail` (:140-171, :254-257, :357-360), whose stores end at
  the cursor after the ASCII bytes (the last, backward, one at the end of the
  input; it starts inside the buffer when the calls are exact,
  `gen_tail_start`); in `convert` the scalar `convert` of the rest
  (:262-263); in `_with_errors` the rewind at a block head (:318-328) and
  after the ASCII tail (:368-369). `conv` is the scalar run
  (`Scalar.run Scalar.u8to16`, `u8to32`, `u8toL`; `Scalar.utf8to16_le`,
  `utf8to32_le`, `utf8toL_le`).
* **Loads (`Load`).** The 64-byte block load, the 16-byte load of each call
  at `t < pos + 52` (`Utf8.read_ok`, `K ≥ 2`), and those of
  `convert_ascii_tail` (8 bytes when 8 remain, the 64-byte chunks, the last 64
  bytes). The scalar code reads inside the slice it is given; the rewind reads
  `buf[-i]` for `i ≤ prior_bytes = pos` (`min(3, pos)` for UTF-32), inside
  the input (`hfb t ≤ t` in `gen_safe`).
* **End to end (`gen_safe`)** for any weight with `WOK`, `K ≥ 2`, a path list
  with `okK K` and a scalar conversion within `L`; **per target** (`targets`:
  the header's `K`, :12-16 and src/generic/utf8_to_utf32/utf8_to_utf32.h:12-16,
  src/generic/utf8_to_latin1/utf8_to_latin1.h:12-16, and the kernel's path
  lists from `Utf8` and `Arch.*`): `gen_utf16_safe`, `gen_utf16_errors_safe`,
  `gen_utf32_safe`, `gen_utf32_errors_safe`, `gen_latin1_safe`,
  `gen_latin1_errors_safe`.

Read from the C++, as in `Utf8`, `Exact` and `ValidatorSound`: the per-call
facts (a call is a `Utf8.Step`, its store a path of the list within its
budget, and, in a block that passed the check, an `Exact.XStep`); the
transcriptions.
-/
namespace SafetyMargins.GenericUtf8
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.Exact SafetyMargins.Validator
open SafetyMargins.ValidExact (WOK wok_w16 wok_w01)
open SafetyMargins.Scalar (Bytes)
set_option autoImplicit false

/-! ## The loop -/

/-- The loop test `pos + 64 + safety_margin <= size`
(src/generic/utf8_to_utf16/utf8_to_utf16.h:196, :294;
src/generic/utf8_to_utf32/utf8_to_utf32.h:193, :288;
src/generic/utf8_to_latin1/utf8_to_latin1.h:180, :278) with
`safety_margin = size - margin + 1`, `size - margin` the length `tk K` of the
suffix the backward scan reads (src/generic/utf8_to_utf16/utf8_to_utf16.h:188-195,
`Utf8.tk`). -/
def Loop (w : Nat → Nat) (K : Nat) (bs : List Nat) (pos : Nat) : Prop :=
  pos + 64 + (tk K (bs.map w) + 1) ≤ bs.length

/-- The states of the loop of `convert` and `convert_with_errors`, as
`Utf8.Reach` (`b` in a block, the block at `pos`, the call at `t`, the cursor
`o`), on bytes, with the loop test at each block head:
* `ascii`: an ASCII block (src/generic/utf8_to_utf16/utf8_to_utf16.h:198-201,
  :296-299; src/generic/utf8_to_utf32/utf8_to_utf32.h:195-198, :290-293;
  src/generic/utf8_to_latin1/utf8_to_latin1.h:182-185, :280-283);
* `enter`: a non-ASCII block, converted only from a leading byte
  (src/generic/utf8_to_utf16/utf8_to_utf16.h:219-222, :318;
  src/generic/utf8_to_utf32/utf8_to_utf32.h:216-219, :312;
  src/generic/utf8_to_latin1/utf8_to_latin1.h:206-208, and :301 with
  `l1_enter`);
* `step`, `xcall`: a call of `convert_masked_utf8_to_*` at `t < pos + 52`
  (src/generic/utf8_to_utf16/utf8_to_utf16.h:230-244, :336-350;
  src/generic/utf8_to_utf32/utf8_to_utf32.h:227-241, :326-340;
  src/generic/utf8_to_latin1/utf8_to_latin1.h:216-230, :319-333): any
  `Utf8.Step`, or, the only calls with `chk`, an exact one (`Exact.XStep`);
* `leave`: the inner `while` ends at `pos + 52`. -/
inductive GR (w : Nat → Nat) (K : Nat) (chk : Bool) (bs : List Nat) :
    Bool → Nat → Nat → Nat → Prop
  | start : GR w K chk bs false 0 0 0
  | ascii (t o : Nat) (h : GR w K chk bs false t t o) (hloop : Loop w K bs t)
      (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) :
      GR w K chk bs false (t + 64) (t + 64) (o + 64)
  | enter (t o : Nat) (h : GR w K chk bs false t t o) (hloop : Loop w K bs t)
      (hlead : isCont (bs.getD t 0) = false) : GR w K chk bs true t t o
  | step (pos t o t' o' : Nat) (hc : chk = false) (h : GR w K chk bs true pos t o)
      (ht : t < pos + 52) (hs : Step (bs.map w) t o t' o') : GR w K chk bs true pos t' o'
  | xcall (pos t o c p : Nat) (h : GR w K chk bs true pos t o) (ht : t < pos + 52)
      (hs : XStep (bs.map w) t c p) : GR w K chk bs true pos (t + c) (o + p)
  | leave (pos t o : Nat) (h : GR w K chk bs true pos t o) (ht : pos + 52 ≤ t) :
      GR w K chk bs false t t o

/-- What a block head guarantees to the rewind. -/
def HeadOK (w : Nat → Nat) (chk : Bool) (bs : List Nat) (t o : Nat) : Prop :=
  t = 0 ∨ (0 < t ∧ bs.getD (t - 1) 0 < 0x80) ∨ isCont (bs.getD t 0) = false ∨
    (chk = false ∧ o + 1 ≤ L ((bs.map w).take t))

theorem getD_w {w : Nat → Nat} {bs : List Nat} {i : Nat} (hi : i < bs.length) :
    (bs.map w).getD i 0 = w (bs.getD i 0) := getD_map w bs i hi

theorem lead_ne {w : Nat → Nat} (hw : WOK w) {bs : List Nat} (hb : Bytes bs) {i : Nat}
    (hi : i < bs.length) (hl : isCont (bs.getD i 0) = false) : (bs.map w).getD i 0 ≠ 0 := by
  rw [getD_w hi]
  intro h0
  rw [(hw.1 _ (getD_lt bs hb i)).mp h0] at hl
  cases hl

theorem ne_lead {w : Nat → Nat} (hw : WOK w) {bs : List Nat} (hb : Bytes bs) {i : Nat}
    (hne : (bs.map w).getD i 0 ≠ 0) : i < bs.length ∧ isCont (bs.getD i 0) = false := by
  have hi : i < bs.length := by
    refine Classical.byContradiction fun hge => hne ?_
    rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by simp; omega)]; rfl
  refine ⟨hi, ?_⟩
  cases hc : isCont (bs.getD i 0)
  · rfl
  · exact absurd ((getD_w hi).trans ((hw.1 _ (getD_lt bs hb i)).mpr hc)) hne

theorem ascii_L {w : Nat → Nat} (hw : WOK w) {bs : List Nat} {t a : Nat}
    (ha : t + a ≤ bs.length) (hascii : ∀ i, i < a → bs.getD (t + i) 0 < 0x80) :
    L (((bs.map w).drop t).take a) = a :=
  L_ones (bs.map w) t a fun i hi => by
    rw [getD_w (by omega)]; exact hw.2 _ (hascii i hi)

/-- **The invariant.** Every state is a `Utf8.Reach` state; inside a block
the block passed the loop test, and with exact calls (`chk`) the next call
starts on a leading byte; a block head satisfies `HeadOK`; with exact calls
the cursor is `L` of the consumed bytes. -/
theorem gr_inv {w : Nat → Nat} (hw : WOK w) {K : Nat} {chk : Bool} {bs : List Nat}
    (hb : Bytes bs) :
    ∀ b pos t o, GR w K chk bs b pos t o →
      Reach (bs.map w) b pos t o ∧ t ≤ bs.length ∧
      (b = true → Loop w K bs pos ∧ (chk = true → isCont (bs.getD t 0) = false)) ∧
      (b = false → HeadOK w chk bs t o) ∧
      (chk = true → o = L ((bs.map w).take t)) := by
  intro b pos t o h
  induction h with
  | start =>
    exact ⟨Reach.start, by omega, fun h => by simp at h, fun _ => Or.inl rfl,
      fun _ => by simp⟩
  | ascii t o h hloop hascii ih =>
    obtain ⟨r, -, -, -, ix⟩ := ih
    have hl : t + 64 ≤ bs.length := by unfold Loop at hloop; omega
    refine ⟨Reach.ascii t o r (fun i hi => by rw [getD_w (by omega)]; exact hw.2 _ (hascii i hi)),
      hl, fun h => by simp at h, fun _ => Or.inr (Or.inl ⟨by omega, ?_⟩), fun hc => ?_⟩
    · have := hascii 63 (by omega); simpa using this
    · rw [L_take_add, ascii_L hw hl hascii, ← ix hc]
  | enter t o h hloop hlead ih =>
    obtain ⟨r, -, -, -, ix⟩ := ih
    have hl : t + 64 ≤ bs.length := by unfold Loop at hloop; omega
    exact ⟨Reach.enter t o r (lead_ne hw hb (by omega) hlead), by omega,
      fun _ => ⟨hloop, fun _ => hlead⟩, fun h => by simp at h, ix⟩
  | step pos t o t' o' hc h ht hs ih =>
    obtain ⟨r, -, i3, -, -⟩ := ih
    obtain ⟨hloop, -⟩ := i3 rfl
    have hl : pos + 64 ≤ bs.length := by unfold Loop at hloop; omega
    refine ⟨Reach.call pos t o t' o' r ht hs, ?_, fun _ => ⟨hloop, fun h => by simp [hc] at h⟩,
      fun h => by simp at h, fun h => by simp [hc] at h⟩
    cases hs with
    | conv c p hc1 hnext hp => exact Nat.le_of_lt (ne_lead hw hb hnext).1
    | error => omega
  | xcall pos t o c p h ht hs ih =>
    obtain ⟨r, -, i3, -, ix⟩ := ih
    obtain ⟨hloop, -⟩ := i3 rfl
    obtain ⟨hc1, hnext, hp⟩ := hs
    obtain ⟨hin, hlc⟩ := ne_lead hw hb hnext
    refine ⟨Reach.call pos t o (t + c) (o + p) r ht (Step.conv t o c p hc1 hnext (by omega)),
      by omega, fun _ => ⟨hloop, fun _ => hlc⟩, fun h => by simp at h, fun hc => ?_⟩
    rw [L_take_add, ← ix hc, hp]
  | leave pos t o h ht ih =>
    obtain ⟨r, i2, i3, -, ix⟩ := ih
    obtain ⟨-, i5⟩ := i3 rfl
    have hlag := (reach_lag _ _ _ _ _ r).2 rfl
    refine ⟨Reach.leave pos t o r ht, i2, fun h => by simp at h, fun _ => ?_, ix⟩
    unfold HeadOK
    cases hc : isCont (bs.getD t 0)
    · exact Or.inr (Or.inr (Or.inl rfl))
    · right; right; right
      cases chk
      · refine ⟨rfl, ?_⟩
        unfold Lag at hlag
        have : cont (bs.map w) t = 1 := by
          unfold cont
          rw [ite_eq_left_iff.mpr]
          intro hne
          exact absurd hc (by rw [(ne_lead hw hb hne).2]; simp)
        omega
      · rw [i5 rfl] at hc; cases hc

/-! ## The rewind at a block head -/

theorem isCont_and : ∀ b : Fin 256, (isCont b.val = true ↔ b.val &&& 0xC0 = 0x80) := by
  decide +kernel

theorem cont_of_and {b : Nat} (hb : b < 256) (h : b &&& 0xC0 = 0x80) : isCont b = true :=
  (isCont_and ⟨b, hb⟩).mpr h

theorem and_of_lead {b : Nat} (hb : b < 256) (h : isCont b = false) : b &&& 0xC0 ≠ 0x80 :=
  fun h' => by rw [cont_of_and hb h'] at h; cases h

theorem and_of_cont {b : Nat} (h : isCont b = true) : b &&& 0xC0 = 0x80 := by
  have hb : b < 256 := by simp [isCont] at h; omega
  exact (isCont_and ⟨b, hb⟩).mp h

/-- The search of the rewind passes only continuation bytes before the byte it
stops at. -/
theorem seek_conts (bk : Nat → Nat) : ∀ k i j, Scalar.seek bk i k = some j →
    ∀ m, i ≤ m → m < j → bk m &&& 0xC0 = 0x80 := by
  intro k
  induction k with
  | zero => intro i j h; simp [Scalar.seek] at h
  | succ k ih =>
    intro i j h m h1 h2
    simp only [Scalar.seek] at h
    split at h
    · simp at h; omega
    · rename_i hne
      by_cases hm : m = i
      · subst hm; simpa using hne
      · exact ih (i + 1) j h m (by omega) h2

/-- `rewind` writes nothing, or converts from a byte `i ≤ hfb` back, having
stepped over continuation bytes only. -/
theorem rewind_cases (conv : List Nat → Nat) (hfb : Nat) (pre xs : List Nat) :
    Scalar.rewind conv hfb pre xs = 0 ∨ ∃ i, i ≤ hfb ∧
      (∀ m, m < i → Scalar.back pre xs m &&& 0xC0 = 0x80) ∧
      Scalar.rewind conv hfb pre xs = conv (pre.drop (pre.length - i) ++ xs) := by
  unfold Scalar.rewind
  split
  · exact Or.inl rfl
  · rename_i i h
    have hr := Scalar.seek_range _ _ _ _ h
    split
    · exact Or.inl rfl
    · exact Or.inr ⟨i, by omega, fun m hm => seek_conts _ _ _ _ h m (Nat.zero_le _) hm, rfl⟩

theorem sum_le_head (f : Nat → Nat) : ∀ i, (∀ k, 1 ≤ k → k < i → f k = 0) → sumUpTo f i ≤ f 0
  | 0, _ => by simp [sumUpTo]
  | i + 1, h => by
    simp only [sumUpTo]
    have ih := sum_le_head f i (fun k h1 h2 => h k h1 (by omega))
    by_cases hi : i = 0
    · subst hi; simp [sumUpTo]
    · rw [h i (by omega) (by omega)]; omega

/-- The bytes the rewind steps back over, a byte and continuation bytes, weigh
at most 1 under a weight of at most 1 per byte (`Exact.w01`). -/
theorem window_le1 {w : Nat → Nat} (hw : WOK w) (hone : ∀ b, w b ≤ 1) (pre xs : List Nat)
    (hb : Bytes pre) (i : Nat) (hi : i ≤ pre.length)
    (hcs : ∀ m, m < i → Scalar.back pre xs m &&& 0xC0 = 0x80) :
    L ((pre.drop (pre.length - i)).map w) ≤ 1 := by
  have e : (pre.drop (pre.length - i)).map w = ((pre.map w).drop (pre.length - i)).take i := by
    rw [List.map_drop, List.take_of_length_le]; simp; omega
  rw [e, L_window]
  refine Nat.le_trans (sum_le_head _ i fun k h1 h2 => ?_) ?_
  · have hk : pre.length - i + k < pre.length := by omega
    rw [getD_w hk]
    have := hcs (i - k) (by omega)
    simp only [Scalar.back, show i - k ≠ 0 by omega, ↓reduceIte,
      show pre.length - (i - k) = pre.length - i + k by omega] at this
    exact (hw.1 _ (getD_lt pre hb _)).mpr (cont_of_and (getD_lt pre hb _) this)
  · by_cases h0 : pre.length - i + 0 < pre.length
    · rw [getD_w h0]; exact hone _
    · rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by simp; omega)]; simp

theorem getD_drop0 (bs : List Nat) (t : Nat) : (bs.drop t).getD 0 0 = bs.getD t 0 := by
  simp [List.getD_eq_getElem?_getD, List.getElem?_drop]

/-- **The rewind at a block head.** With the cursor at most `L` of the
consumed bytes, `rewind_and_convert_with_errors(t, in + t, size - t, out)`
stays within `L` of the input when the head satisfies `HeadOK`: the byte at
`t` is not a continuation byte (it converts from `t`, `Scalar.rewind_rest`),
the byte before it is ASCII (it returns, `Scalar.rewind_rest`), `t = 0` (no
byte before, it returns), or the cursor lags by one unit and the bytes
stepped back over weigh at most one (`window_le1`). -/
theorem rewind_head {w : Nat → Nat} (hw : WOK w) {conv : List Nat → Nat}
    (hconv : ∀ ys, Bytes ys → conv ys ≤ L (ys.map w)) {bs : List Nat} (hb : Bytes bs)
    {t o hfb : Nat} (ht : t ≤ bs.length) (h1 : hfb ≤ t) (h2 : 1 ≤ t → 1 ≤ hfb)
    (ho : o ≤ L ((bs.map w).take t))
    (hh : t = 0 ∨ (0 < t ∧ bs.getD (t - 1) 0 < 0x80) ∨ isCont (bs.getD t 0) = false ∨
      ((∀ b, w b ≤ 1) ∧ o + 1 ≤ L ((bs.map w).take t))) :
    o + Scalar.rewind conv hfb (bs.take t) (bs.drop t) ≤ L (bs.map w) := by
  have hsplit : L (bs.map w) = L ((bs.map w).take t) + L ((bs.drop t).map w) := by
    rw [List.map_drop]; exact L_take_add_drop t _
  have hpre : (bs.take t).length = t := by simp; omega
  have hbd : Bytes (bs.drop t) := hb.drop t
  by_cases hc : isCont (bs.getD t 0) = false
  · have := Scalar.rewind_rest w conv hconv hfb (bs.take t) (bs.drop t) hbd
      (Or.inl (by rw [getD_drop0]; exact and_of_lead (getD_lt bs hb t) hc))
    omega
  · have hc' : isCont (bs.getD t 0) = true := by simpa using hc
    rcases hh with h0 | ⟨hpos, ha⟩ | hl | ⟨hone, hlag⟩
    · subst h0
      have hz : hfb = 0 := by omega
      subst hz
      have : Scalar.rewind conv 0 (bs.take 0) (bs.drop 0) = 0 := by
        have hs : Scalar.seek (Scalar.back (bs.take 0) (bs.drop 0)) 0 (0 + 1) = none := by
          simp only [Scalar.seek, Scalar.back, ↓reduceIte, getD_drop0]
          rw [and_of_cont hc']; simp
        unfold Scalar.rewind; rw [hs]
      omega
    · have := Scalar.rewind_rest w conv hconv hfb (bs.take t) (bs.drop t) hbd
        (Or.inr ⟨h2 hpos, by omega, by
          rw [hpre, List.getD_eq_getElem?_getD, List.getElem?_take]
          simp only [show t - 1 < t by omega, ↓reduceIte]
          rw [← List.getD_eq_getElem?_getD]; exact ha⟩)
      omega
    · rw [hl] at hc'; cases hc'
    · rcases rewind_cases conv hfb (bs.take t) (bs.drop t) with h0 | ⟨i, hi, hcs, he⟩
      · rw [h0]; omega
      · rw [he]
        have hbt : Bytes ((bs.take t).drop ((bs.take t).length - i) ++ bs.drop t) := by
          intro b hbm
          simp only [List.mem_append] at hbm
          rcases hbm with hbm | hbm
          · exact hb.take t b (List.mem_of_mem_drop hbm)
          · exact hbd b hbm
        have hcv := hconv _ hbt
        rw [List.map_append, L_append] at hcv
        have hw1 := window_le1 hw hone (bs.take t) (bs.drop t) (hb.take t) i (by omega) hcs
        omega

/-! ## Stores and loads, end to end -/

/-- Every store of `convert` (`plain = true`) or `convert_with_errors`
(`plain = false`), by the furthest unit it writes (cited for UTF-16; the
UTF-32 and Latin-1 headers have the same lines, see `GR`):
* `small`: fewer than 64 bytes, the scalar conversion of all of them
  (src/generic/utf8_to_utf16/utf8_to_utf16.h:176-177, :273-274);
* `ascii`: a 64-unit ASCII store at a head that passed the test (:198-201, :296-299);
* `call`: a call stores `q.s` units at the cursor, `q` a path of the list
  with budget `q.f` at most `cont` plus `L` of the `d` bytes it inspects
  inside the block (`Utf8.reach_L`);
* `tailAscii`: `convert_ascii_tail` (:140-171), called at a head (:254-255,
  :357-358): every store ends at the cursor after `a` ASCII bytes (the
  64-byte chunks :153-160; the last one, backward, :161-170, ends at the end
  of the input, after the `size - pos` ASCII bytes it converts);
* `tailScalar`: `convert` only, the scalar `convert` of the rest after the
  ASCII tail (:262-263);
* `rewindBlock`: `_with_errors` only, a block that failed the check or starts
  on a continuation byte: the rewind at the head (:318-325);
* `rewindTail`: `_with_errors` only, the rewind after the ASCII tail
  (:368-369). -/
inductive Store (w : Nat → Nat) (K : Nat) (chk plain : Bool) (ps : List MPath)
    (conv : List Nat → Nat) (hfb : Nat → Nat) (bs : List Nat) : Nat → Prop
  | small (h : bs.length < 64) : Store w K chk plain ps conv hfb bs (conv bs)
  | ascii (t o : Nat) (h : GR w K chk bs false t t o) (hloop : Loop w K bs t)
      (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) :
      Store w K chk plain ps conv hfb bs (o + 64)
  | call (pos t o d : Nat) (q : MPath) (h : GR w K chk bs true pos t o) (ht : t < pos + 52)
      (hd : t + d ≤ pos + 64) (hq : q ∈ ps)
      (hf : q.f ≤ cont (bs.map w) t + L (((bs.map w).drop t).take d)) :
      Store w K chk plain ps conv hfb bs (o + q.s)
  | tailAscii (t o a : Nat) (h : GR w K chk bs false t t o) (ha : t + a ≤ bs.length)
      (hascii : ∀ i, i < a → bs.getD (t + i) 0 < 0x80) :
      Store w K chk plain ps conv hfb bs (o + a)
  | tailScalar (t o a : Nat) (hp : plain = true) (h : GR w K chk bs false t t o)
      (ha : t + a ≤ bs.length) (hascii : ∀ i, i < a → bs.getD (t + i) 0 < 0x80) :
      Store w K chk plain ps conv hfb bs (o + a + conv (bs.drop (t + a)))
  | rewindBlock (t o : Nat) (hp : plain = false) (h : GR w K chk bs false t t o) :
      Store w K chk plain ps conv hfb bs (o + Scalar.rewind conv (hfb t) (bs.take t) (bs.drop t))
  | rewindTail (t o a : Nat) (hp : plain = false) (h : GR w K chk bs false t t o)
      (ha : t + a ≤ bs.length) (hascii : ∀ i, i < a → bs.getD (t + i) 0 < 0x80) :
      Store w K chk plain ps conv hfb bs
        (o + a + Scalar.rewind conv (hfb (t + a)) (bs.take (t + a)) (bs.drop (t + a)))

/-- The ends of the vector loads: the 64-byte block at a head that passed the
test (src/generic/utf8_to_utf16/utf8_to_utf16.h:197, :295); the 16 bytes a
call loads at `t` (`_mm_loadu_si128`, `vld1q_u8`, `vector_u8::load`,
`__lsx_vld` at `input`); `convert_ascii_tail`: 8 bytes when 8 remain
(src/generic/utf8_to_utf16/utf8_to_utf16.h:145-147), the 64-byte chunks
(:153-154), the last 64 bytes when the input has 64 (:161-164). The call load,
e.g. src/westmere/sse_convert_utf8_to_utf16.cpp:24. -/
inductive Load (w : Nat → Nat) (K : Nat) (chk : Bool) (bs : List Nat) : Nat → Prop
  | block (t o : Nat) (h : GR w K chk bs false t t o) (hloop : Loop w K bs t) :
      Load w K chk bs (t + 64)
  | call (pos t o : Nat) (h : GR w K chk bs true pos t o) (ht : t < pos + 52) :
      Load w K chk bs (t + 16)
  | tail8 (t o : Nat) (h : GR w K chk bs false t t o) (h8 : 8 ≤ bs.length - t) :
      Load w K chk bs (t + 8)
  | tailChunk (t o a : Nat) (h : GR w K chk bs false t t o) (hc : t + a + 64 ≤ bs.length) :
      Load w K chk bs (t + a + 64)
  | tailLast (t o : Nat) (h : GR w K chk bs false t t o) (h64 : 64 ≤ bs.length) :
      Load w K chk bs (bs.length - 64 + 64)

/-- **The generic validating decoders, end to end.** -/
theorem gen_safe {w : Nat → Nat} (hw : WOK w) {K : Nat} (hK : 2 ≤ K) {chk plain : Bool}
    {ps : List MPath} (hps : okK K ps = true) {conv : List Nat → Nat}
    (hconv : ∀ ys, Bytes ys → conv ys ≤ L (ys.map w)) {hfb : Nat → Nat}
    (hfb1 : ∀ t, hfb t ≤ t) (hfb2 : ∀ t, 1 ≤ t → 1 ≤ hfb t)
    (hone : plain = false → chk = false → ∀ b, w b ≤ 1) {bs : List Nat} (hb : Bytes bs) :
    (∀ s, Store w K chk plain ps conv hfb bs s → s ≤ L (bs.map w)) ∧
    (∀ e, Load w K chk bs e → e ≤ bs.length) := by
  have hsplit : ∀ t, L (bs.map w) = L ((bs.map w).take t) + L ((bs.drop t).map w) := fun t => by
    rw [List.map_drop]; exact L_take_add_drop t _
  have hrest : ∀ t, L ((bs.map w).take t) ≤ L (bs.map w) := fun t => L_take_le _ t
  refine ⟨fun s h => ?_, fun e h => ?_⟩
  · cases h with
    | small h => exact hconv bs hb
    | ascii t o h hloop hascii =>
      obtain ⟨-, -, -, -, -⟩ := gr_inv hw hb _ _ _ _ h
      have ho := ((reach_lag _ _ _ _ _ (gr_inv hw hb _ _ _ _ h).1).1 rfl)
      have hl : t + 64 ≤ bs.length := by unfold Loop at hloop; omega
      have := L_take_add (bs.map w) t 64
      rw [ascii_L hw hl hascii] at this
      have := hrest (t + 64)
      omega
    | call pos t o d q h ht hd hq hf =>
      obtain ⟨r, -, i3, -, -⟩ := gr_inv hw hb _ _ _ _ h
      obtain ⟨hloop, -⟩ := i3 rfl
      have hs : q.s ≤ q.f + K := of_decide_eq_true ((List.all_eq_true.mp hps) q hq)
      have := reach_L K pos t o d q.s (bs.map w) r (by simpa [Loop] using hloop) hd (by omega)
      simpa using this
    | tailAscii t o a h ha hascii =>
      have ho := ((reach_lag _ _ _ _ _ (gr_inv hw hb _ _ _ _ h).1).1 rfl)
      have := L_take_add (bs.map w) t a
      rw [ascii_L hw ha hascii] at this
      have := hrest (t + a)
      omega
    | tailScalar t o a hp h ha hascii =>
      have ho := ((reach_lag _ _ _ _ _ (gr_inv hw hb _ _ _ _ h).1).1 rfl)
      have := L_take_add (bs.map w) t a
      rw [ascii_L hw ha hascii] at this
      have := hconv _ (hb.drop (t + a))
      have := hsplit (t + a)
      omega
    | rewindBlock t o hp h =>
      obtain ⟨r, i2, -, i4, -⟩ := gr_inv hw hb _ _ _ _ h
      have ho := (reach_lag _ _ _ _ _ r).1 rfl
      refine rewind_head hw hconv hb i2 (hfb1 t) (hfb2 t) ho ?_
      rcases i4 rfl with h0 | ha | hl | ⟨hc, hlag⟩
      · exact Or.inl h0
      · exact Or.inr (Or.inl ha)
      · exact Or.inr (Or.inr (Or.inl hl))
      · exact Or.inr (Or.inr (Or.inr ⟨hone hp hc, hlag⟩))
    | rewindTail t o a hp h ha hascii =>
      obtain ⟨r, i2, -, i4, -⟩ := gr_inv hw hb _ _ _ _ h
      have ho := (reach_lag _ _ _ _ _ r).1 rfl
      have hta := L_take_add (bs.map w) t a
      rw [ascii_L hw ha hascii] at hta
      have := rewind_head (o := o + a) hw hconv hb ha (hfb1 (t + a)) (hfb2 (t + a)) (by omega) (by
        by_cases ha0 : a = 0
        · subst ha0
          simp only [Nat.add_zero]
          rcases i4 rfl with h0 | ha | hl | ⟨hc, hlag⟩
          · exact Or.inl h0
          · exact Or.inr (Or.inl ha)
          · exact Or.inr (Or.inr (Or.inl hl))
          · exact Or.inr (Or.inr (Or.inr ⟨hone hp hc, hlag⟩))
        · right; left
          refine ⟨by omega, ?_⟩
          have := hascii (a - 1) (by omega)
          rwa [show t + (a - 1) = t + a - 1 by omega] at this)
      omega
  · cases h with
    | block t o h hloop => unfold Loop at hloop; omega
    | call pos t o h ht =>
      obtain ⟨-, -, i3, -, -⟩ := gr_inv hw hb _ _ _ _ h
      obtain ⟨hloop, -⟩ := i3 rfl
      have := read_ok K pos (bs.map w) hK (by simpa [Loop] using hloop)
      simp at this
      omega
    | tail8 t o h h8 =>
      have := (gr_inv hw hb _ _ _ _ h).2.1
      omega
    | tailChunk t o a h hc => exact hc
    | tailLast t o h h64 => omega

/-- **The backward store of `convert_ascii_tail` starts inside the buffer**
when every call was exact (`chk = true`): the cursor is then `L` of the
consumed bytes (`gr_inv`), and `Exact.tail_last` applies at `pos = t + a`,
after the `a` bytes of the 64-byte ASCII chunks. -/
theorem gen_tail_start {w : Nat → Nat} (hw : WOK w) {K : Nat} {bs : List Nat} (hb : Bytes bs)
    {t o a : Nat} (h : GR w K true bs false t t o) (ha : t + a ≤ bs.length)
    (hascii : ∀ i, i < a → bs.getD (t + i) 0 < 0x80) (h64 : 64 ≤ bs.length)
    (hp1 : bs.length - 64 < t + a)
    (hlast : ∀ i, bs.length - 64 ≤ i → i < bs.length → bs.getD i 0 < 0x80) :
    t + a - (bs.length - 64) ≤ o + a ∧
      o + a - (t + a - (bs.length - 64)) + 64 = L (bs.map w) := by
  have ix := (gr_inv hw hb _ _ _ _ h).2.2.2.2 rfl
  have hta := L_take_add (bs.map w) t a
  rw [ascii_L hw ha hascii, ← ix] at hta
  have := tail_last (bs.map w) (t + a) (o + a) hta.symm (by simpa using h64)
    (by simpa using hp1) (by simpa using ha)
    (fun i h1 h2 => by
      simp only [List.length_map] at h1 h2
      rw [getD_w h2]; exact hw.2 _ (hlast i h1 h2))
  simpa using this

/-! ## The Latin-1 check on the first byte of a block

`convert_with_errors` of src/generic/utf8_to_latin1/utf8_to_latin1.h has no
`utf8_continuation_mask & 1` test (:301-310): a block whose first byte is a
continuation byte is rejected by the check. The tables of
`check_special_cases` (:52-65, :68-88, :89-104) with the constants of
:30-50; the first byte is checked against `prev1 = 0` (:117-123, chunk 0
with `zero`, :291-299). -/

def l1B1High (h : Nat) : Nat :=
  [2, 2, 2, 2, 2, 2, 2, 2, 128, 128, 128, 128, 33, 0xff, 0xff, 0xff].getD h 0
def l1B1Low (l : Nat) : Nat :=
  [231, 163, 131, 131, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff].getD l 0
def l1B2High (h : Nat) : Nat :=
  [1, 1, 1, 1, 1, 1, 1, 1, 230, 174, 186, 186, 1, 1, 1, 1].getD h 0

/-- `byte_1_high & byte_1_low & byte_2_high` (:105). -/
def l1sc (prev1 input : Nat) : Nat :=
  l1B1High (prev1 >>> 4) &&& l1B1Low (prev1 &&& 0xF) &&& l1B2High (input >>> 4)

/-- The first byte of a block, checked after a zero byte, is flagged when it
is a continuation byte (`TOO_LONG`): a block the Latin-1 `_with_errors` loop
converts starts on a leading byte, as `GR.enter` records. -/
theorem l1_first_cont : ∀ b : Fin 256, isCont b.val = true → l1sc 0 b.val ≠ 0 := by decide +kernel

theorem l1_enter {bs : List Nat} (hb : Bytes bs) {t : Nat} (hok : l1sc 0 (bs.getD t 0) = 0) :
    isCont (bs.getD t 0) = false := by
  cases hc : isCont (bs.getD t 0)
  · rfl
  · exact absurd hok (l1_first_cont ⟨_, getD_lt bs hb t⟩ hc)

/-! ## Why `HeadOK` is needed

If the rewind ran at a head right after a complete 2-byte (or 4-byte)
character, with the cursor at `L` of the consumed bytes and a stray
continuation byte next, it would step back and convert that character again
(include/simdutf/scalar/utf8_to_utf16/utf8_to_utf16.h:308-321, :337), one
unit (two) past `L`. `HeadOK` excludes that head: the vector loop leaves a
block on a leading byte (`XStep` ends before one), after an ASCII byte, or at
0. So `Scalar.rewind_total`'s hypothesis (the cursor at most `L` of the
bytes before every byte the search could reach) is not what holds at these
heads, and is not used: `rewind_head` is. -/
theorem dup_example :
    Scalar.rewind (Scalar.run Scalar.u8to16 4) 2 [0xC3, 0xA9] [0x80] = 1 ∧
      L ([0xC3, 0xA9].map w16) + 1 = L (([0xC3, 0xA9] ++ [0x80]).map w16) + 1 ∧
    Scalar.rewind (Scalar.run Scalar.u8to16 6) 4 [0xF0, 0x9F, 0x98, 0x80] [0x80] = 2 ∧
      L ([0xF0, 0x9F, 0x98, 0x80].map w16) + 2 =
        L (([0xF0, 0x9F, 0x98, 0x80] ++ [0x80]).map w16) + 2 := by decide

/-! ## The six generic targets -/

/-- The header constants and the `convert_masked_utf8_to_*` path lists of a
target: `K` for UTF-16 (src/generic/utf8_to_utf16/utf8_to_utf16.h:12-16),
UTF-32 (src/generic/utf8_to_utf32/utf8_to_utf32.h:12-16) and Latin-1
(src/generic/utf8_to_latin1/utf8_to_latin1.h:12-16). -/
structure Target where
  k16 : Nat
  k32 : Nat
  kL : Nat
  p16 : List MPath
  p32 : List MPath
  pL : List MPath

def westmere : Target := ⟨3, 3, 3, x86_utf16, x86_utf32, x86_latin1⟩
def haswell : Target :=
  ⟨3, 3, 3, Arch.Haswell.utf8to16, Arch.Haswell.utf8to32, Arch.Haswell.utf8to1⟩
def ppc64 : Target := ⟨3, 3, 3, Arch.Ppc64.ppc_utf16, Arch.Ppc64.ppc_utf32, Arch.Ppc64.ppc_latin1⟩
def arm64 : Target := ⟨2, 3, 3, arm_utf16, Arch.Arm64.arm_utf32, Arch.Arm64.arm_latin1⟩
def lsx : Target := ⟨3, 2, 2, Arch.Lsx.utf8to16, Arch.Lsx.utf8to32, Arch.Lsx.utf8toLatin1⟩
def lasx : Target :=
  ⟨3, 2, 2, Arch.Lasx.lasx_utf16, Arch.Lasx.lasx_utf32, Arch.Lasx.lasx_latin1⟩

def targets : List Target := [westmere, haswell, ppc64, arm64, lsx, lasx]

/-- Each target's `K` covers its path list (`okK`) and is at least 2 (the
16-byte loads, `read_ok`). -/
theorem targets_ok : ∀ T ∈ targets,
    2 ≤ T.k16 ∧ okK T.k16 T.p16 = true ∧ 2 ≤ T.k32 ∧ okK T.k32 T.p32 = true ∧
      2 ≤ T.kL ∧ okK T.kL T.pL = true := by
  decide

theorem w01_le1 (b : Nat) : w01 b ≤ 1 := by unfold w01; split <;> omega

/-- **`convert_utf8_to_utf16{le,be}`** (src/generic/utf8_to_utf16/utf8_to_utf16.h:174-268). -/
theorem gen_utf16_safe (T : Target) (hT : T ∈ targets) (f : Nat) {bs : List Nat}
    (hb : Bytes bs) :
    (∀ s, Store w16 T.k16 false true T.p16 (Scalar.run Scalar.u8to16 f) id bs s →
      s ≤ L (bs.map w16)) ∧ (∀ e, Load w16 T.k16 false bs e → e ≤ bs.length) :=
  have h := targets_ok T hT
  gen_safe wok_w16 h.1 h.2.1 (fun ys hy => Scalar.utf8to16_le f ys hy) (fun _ => Nat.le_refl _)
    (fun _ h => h) (fun h => by cases h) hb

/-- **`convert_utf8_to_utf16{le,be}_with_errors`**
(src/generic/utf8_to_utf16/utf8_to_utf16.h:271-377). -/
theorem gen_utf16_errors_safe (T : Target) (hT : T ∈ targets) (f : Nat) {bs : List Nat}
    (hb : Bytes bs) :
    (∀ s, Store w16 T.k16 true false T.p16 (Scalar.run Scalar.u8to16 f) id bs s →
      s ≤ L (bs.map w16)) ∧ (∀ e, Load w16 T.k16 true bs e → e ≤ bs.length) :=
  have h := targets_ok T hT
  gen_safe wok_w16 h.1 h.2.1 (fun ys hy => Scalar.utf8to16_le f ys hy) (fun _ => Nat.le_refl _)
    (fun _ h => h) (fun _ h => by cases h) hb

/-- **`convert_utf8_to_utf32`** (src/generic/utf8_to_utf32/utf8_to_utf32.h:171-264). -/
theorem gen_utf32_safe (T : Target) (hT : T ∈ targets) (f : Nat) {bs : List Nat}
    (hb : Bytes bs) :
    (∀ s, Store w01 T.k32 false true T.p32 (Scalar.run Scalar.u8to32 f) id bs s →
      s ≤ L (bs.map w01)) ∧ (∀ e, Load w01 T.k32 false bs e → e ≤ bs.length) :=
  have h := targets_ok T hT
  gen_safe wok_w01 h.2.2.1 h.2.2.2.1 (fun ys hy => Scalar.utf8to32_le f ys hy)
    (fun _ => Nat.le_refl _) (fun _ h => h) (fun h => by cases h) hb

/-- **`convert_utf8_to_utf32_with_errors`**
(src/generic/utf8_to_utf32/utf8_to_utf32.h:266-366); the rewind looks back at
most `min(3, pos)` bytes
(include/simdutf/scalar/utf8_to_utf32/utf8_to_utf32.h:252-255). -/
theorem gen_utf32_errors_safe (T : Target) (hT : T ∈ targets) (f : Nat) {bs : List Nat}
    (hb : Bytes bs) :
    (∀ s, Store w01 T.k32 false false T.p32 (Scalar.run Scalar.u8to32 f) (fun t => min 3 t) bs s →
      s ≤ L (bs.map w01)) ∧ (∀ e, Load w01 T.k32 false bs e → e ≤ bs.length) :=
  have h := targets_ok T hT
  gen_safe wok_w01 h.2.2.1 h.2.2.2.1 (fun ys hy => Scalar.utf8to32_le f ys hy)
    (fun _ => Nat.min_le_right _ _) (fun t h => by omega) (fun _ _ => w01_le1) hb

/-- **`convert_utf8_to_latin1`** (src/generic/utf8_to_latin1/utf8_to_latin1.h:157-253). -/
theorem gen_latin1_safe (T : Target) (hT : T ∈ targets) (f : Nat) {bs : List Nat}
    (hb : Bytes bs) :
    (∀ s, Store w01 T.kL false true T.pL (Scalar.run Scalar.u8toL f) id bs s →
      s ≤ L (bs.map w01)) ∧ (∀ e, Load w01 T.kL false bs e → e ≤ bs.length) :=
  have h := targets_ok T hT
  gen_safe wok_w01 h.2.2.2.2.1 h.2.2.2.2.2 (fun ys hy => Scalar.utf8toL_le f ys hy)
    (fun _ => Nat.le_refl _) (fun _ h => h) (fun h => by cases h) hb

/-- **`convert_utf8_to_latin1_with_errors`**
(src/generic/utf8_to_latin1/utf8_to_latin1.h:255-359). -/
theorem gen_latin1_errors_safe (T : Target) (hT : T ∈ targets) (f : Nat) {bs : List Nat}
    (hb : Bytes bs) :
    (∀ s, Store w01 T.kL false false T.pL (Scalar.run Scalar.u8toL f) id bs s →
      s ≤ L (bs.map w01)) ∧ (∀ e, Load w01 T.kL false bs e → e ≤ bs.length) :=
  have h := targets_ok T hT
  gen_safe wok_w01 h.2.2.2.2.1 h.2.2.2.2.2 (fun ys hy => Scalar.utf8toL_le f ys hy)
    (fun _ => Nat.le_refl _) (fun _ h => h) (fun _ _ => w01_le1) hb

end SafetyMargins.GenericUtf8
