import SafetyMargins.ValidDynamic
/-
`valid_utf8_to_utf16.h` and `valid_utf8_to_utf32.h` (`convert_valid`) as the
headers run them, on the input their contract allows (valid UTF-8,
`ValidExact.Valid8`): the loop on a fixed margin, then at most one more block
on the dynamic margin of `ValidDynamic`, then the scalar tail.

The loop (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:23-72,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:20-40) runs a block while
`pos + 64 + safety_margin <= size`, with `safety_margin` 9 for UTF-16 (3 on
arm64, src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:18-22) and 12 for UTF-32 (4 on lsx and lasx,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:15-19). After it, except on
arm64 for UTF-16 and on LoongArch for UTF-32
(src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:73-109,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:41-77), if
`pos + 64 + leading_bytes + 1 <= size` it scans back for `leading_bytes = 3`
leading bytes (`ValidDynamic.safetyMargin`) and, if
`pos + 64 + (size - margin + 1) <= size`, converts one more block with the
same code. Fewer than 64 bytes then remain, so no further block runs.

* **The loop (`head`, `HX`).** `head` says whether a block runs at `pos`: in
  the loop (`sc = false`) on the fixed test, and else, once, on the dynamic
  test where the target has the extra block (`kd = some 3`); after the extra
  block (`sc = true`) none. `HX` is `ValidExact.VX` with that test at every
  block head. `hx_vx`: every state of `HX` is a state of `VX`, and a block it
  runs passes the fixed or the dynamic test (`Admit`).
* **Stores.** In a block the fixed test admitted, the bytes after a call
  (which ends by `pos + 63`) are at least `1 + m` bytes of complete characters
  (`ValidExact.tail_expand`). A character of at most 4 bytes gives one unit,
  one of at most 3 bytes one UTF-16 unit, so a deficit `d` fits when
  `3 d ≤ m + 3` (UTF-16, `fixed_store16`) or `4 d ≤ m + 4` (UTF-32,
  `fixed_store32`): `m = 9` and 3 for deficits 4 and 2 in UTF-16, `m = 12`
  and 4 in UTF-32. In the block the dynamic test admitted, `d ≤ K + 1`
  (`ValidDynamic.dyn_store`): `K = 3` for deficit 4.
* **Loads.** The fixed margins are at least 3 and `K = 3 ≥ 2`, so the 64-byte
  block load and the 16-byte loads of the calls at `t < pos + 52` stay in the
  input (`hyb_load`).
* **End to end (`hyb_utf16_safe`, `hyb_utf32_safe`).** On valid input, every
  store (`HStore`: the calls, the ASCII blocks, the scalar tail,
  `Scalar.valid8to16_le`, `Scalar.valid8to32_le`) ends inside `L` and every
  load (`HLoad`) inside the input, with each target's constants.
* **One less fails, on valid inputs, through `HX`** (the calls of the kernels
  as in `ValidDynamic`; `head` at the first block, with the lowered constant,
  runs the block at 0; with the header's constants neither test runs it):
  - the fixed margin minus one: `fix16_x86` (UTF-16 margin 8, westmere,
    haswell, ppc64), `fix16_la` (UTF-16 margin 8, lsx and lasx), `fix32_x86`
    (UTF-32 margin 11, westmere, haswell, ppc64, arm64), `fix32_la` (UTF-32
    margin 3, lsx and lasx), `fix16_arm_read` (arm64 UTF-16 margin 2: a load
    past the input). The failing block necessarily fails the dynamic test
    with `K = 3` (`dyn_store` covers the blocks it admits); each witness
    states it.
  - `K = 2` in the extra block: `k16_x86`, `k16_la`, `k32_x86`. The fixed
    loop runs no block on these inputs.

Read from the C++, as in `ValidExact` and `ValidDynamic`: per path, the
deficit `s ≤ p + d`; that a call ends by `pos + 63`; the paths the kernels take
in the witnesses. The arithmetic minima of the fixed margins are
`fixed16_tight` and `fixed32_tight`.
-/
namespace SafetyMargins.ValidHybrid
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.Validator SafetyMargins.Exact
  SafetyMargins.ValidExact SafetyMargins.ValidLatin1 SafetyMargins.ValidDynamic

/-! ## The loop -/

/-- `safety_margin` (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:18-22):
3 on arm64, 9 elsewhere. -/
def f16 (arm64 : Bool) : Nat := if arm64 then 3 else 9

/-- `safety_margin` (src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:15-19):
4 on lsx and lasx, 12 elsewhere. -/
def f32 (la : Bool) : Nat := if la then 4 else 12

/-- The extra block: `leading_bytes = 3` except on arm64, which has none
(src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:73, :80). -/
def kd16 (arm64 : Bool) : Option Nat := if arm64 then none else some 3

/-- The extra block: `leading_bytes = 3` except on lsx and lasx, which have
none (src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:41, :48). -/
def kd32 (la : Bool) : Option Nat := if la then none else some 3

/-- Whether a block runs at `pos`, and the next phase. In the loop
(`sc = false`) the fixed test (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:23,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:20); when it fails, with an
extra block (`kd = some K`), `pos + 64 + K + 1 <= size` and the dynamic test
(src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:81-89,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:49-57), after which the phase
is `true`; in phase `true` no block runs (the tail follows,
src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:110-111). -/
def head (fm : Nat) (kd : Option Nat) (bs : List Nat) (pos : Nat) (sc : Bool) : Option Bool :=
  if sc then none
  else if pos + 64 + fm ≤ bs.length then some false
  else match kd with
    | none => none
    | some K =>
      if bs.length < pos + 64 + K + 1 then none
      else if bs.length < pos + 64 + safetyMargin K bs then none
      else some true

/-- A block at `pos` passes the fixed test, or the dynamic one where the
target has the extra block. -/
def Admit (fm : Nat) (kd : Option Nat) (bs : List Nat) (pos : Nat) : Prop :=
  pos + 64 + fm ≤ bs.length ∨ ∃ K, kd = some K ∧ pos + 64 + safetyMargin K bs ≤ bs.length

/-- `ValidExact.VX` with the test of the headers at each block head; `sc` is
the phase (`true` once the extra block ran). -/
inductive HX (w : Nat → Nat) (fm : Nat) (kd : Option Nat) (bs : List Nat) :
    Bool → Nat → Nat → Nat → Bool → Prop
  | start : HX w fm kd bs false 0 0 0 false
  /-- `in.is_ascii()` (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:27-30, :91-94). -/
  | ascii (t o : Nat) (sc sc' : Bool) (h : HX w fm kd bs false t t o sc)
      (hh : head fm kd bs t sc = some sc') (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) :
      HX w fm kd bs false (t + 64) (t + 64) (o + 64) sc'
  /-- The slow path (:31-71, :95-106). -/
  | enter (t o : Nat) (sc sc' : Bool) (h : HX w fm kd bs false t t o sc)
      (hh : head fm kd bs t sc = some sc') : HX w fm kd bs true t t o sc'
  /-- `while (pos < max_starting_point)` (:48-66, :100-105), one call. -/
  | call (pos t o c p : Nat) (sc : Bool) (h : HX w fm kd bs true pos t o sc)
      (ht : t < pos + 52) (hs : XStep (bs.map w) t c p) : HX w fm kd bs true pos (t + c) (o + p) sc
  | leave (pos t o : Nat) (sc : Bool) (h : HX w fm kd bs true pos t o sc)
      (ht : pos + 52 ≤ t) : HX w fm kd bs false t t o sc

theorem head_admit {fm : Nat} {kd : Option Nat} {bs : List Nat} {pos : Nat} {sc sc' : Bool}
    (h : head fm kd bs pos sc = some sc') : Admit fm kd bs pos := by
  unfold head at h
  split at h
  · cases h
  · split at h
    · exact Or.inl ‹_›
    · split at h
      · cases h
      · rename_i K
        split at h
        · cases h
        · split at h
          · cases h
          · exact Or.inr ⟨K, rfl, by omega⟩

theorem Admit.len {fm : Nat} {kd : Option Nat} {bs : List Nat} {pos : Nat}
    (h : Admit fm kd bs pos) : pos + 64 ≤ bs.length := by
  rcases h with h | ⟨K, -, h⟩
  · omega
  · unfold safetyMargin at h; omega

/-- **Every state of the headers' loop is a state of `VX`**, and the block it
runs passed the fixed or the dynamic test. -/
theorem hx_vx {w : Nat → Nat} {fm : Nat} {kd : Option Nat} {bs : List Nat} :
    ∀ b pos t o sc, HX w fm kd bs b pos t o sc →
      VX w bs b pos t o ∧ (b = true → Admit fm kd bs pos) := by
  intro b pos t o sc h
  induction h with
  | start => exact ⟨VX.start, fun h => by simp at h⟩
  | ascii t o sc sc' h hh hascii ih =>
    exact ⟨VX.ascii t o ih.1 (head_admit hh).len hascii, fun h => by simp at h⟩
  | enter t o sc sc' h hh ih =>
    exact ⟨VX.enter t o ih.1 (head_admit hh).len, fun _ => head_admit hh⟩
  | call pos t o c p sc h ht hs ih =>
    exact ⟨VX.call pos t o c p ih.1 ht hs, ih.2⟩
  | leave pos t o sc h ht ih =>
    exact ⟨VX.leave pos t o ih.1 ht, fun h => by simp at h⟩

/-! ## Stores and loads in a block -/

/-- **UTF-16, a block the fixed margin `m` admitted**: a call storing
`s ≤ p + d` fits when `3 d ≤ m + 3` (9 for `d = 4`, 3 for `d = 2`). -/
theorem fixed_store16 {bs : List Nat} (hv : Valid8 bs) {m d pos t o c p s : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52) (hx : XStep (bs.map w16) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hloop : pos + 64 + m ≤ bs.length) (hm : 3 * d ≤ m + 3)
    (hs : s ≤ p + d) : o + s ≤ L (bs.map w16) := by
  obtain ⟨hL, hb⟩ := vx_after_call wok_w16 hv h ht hx
  obtain ⟨ks, hks, e16, -, eb⟩ := tail_expand hv hb
  have h3 := bytes_le_3u16 ks hks
  have hsplit := L_take_add_drop (t + c) (bs.map w16)
  rw [e16, L_expand16 ks hks] at hsplit
  omega

/-- **UTF-32, a block the fixed margin `m` admitted**: `4 d ≤ m + 4` (12 for
`d = 4`, 4 for `d = 2`). -/
theorem fixed_store32 {bs : List Nat} (hv : Valid8 bs) {m d pos t o c p s : Nat}
    (h : VX w01 bs true pos t o) (ht : t < pos + 52) (hx : XStep (bs.map w01) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hloop : pos + 64 + m ≤ bs.length) (hm : 4 * d ≤ m + 4)
    (hs : s ≤ p + d) : o + s ≤ L (bs.map w01) := by
  obtain ⟨hL, hb⟩ := vx_after_call wok_w01 hv h ht hx
  obtain ⟨ks, hks, -, e32, eb⟩ := tail_expand hv hb
  have h4 := bytes_le_4u32 ks hks
  have hsplit := L_take_add_drop (t + c) (bs.map w01)
  rw [e32, L_expand32 ks hks] at hsplit
  omega

/-- The bound of `fixed_store16`/`fixed_store32` is the least on the bytes
after the call: `1 + m` bytes of 3-byte characters give `(1 + m) / 3` UTF-16
units (three for `m = 8`, one for `m = 2`), of 4-byte characters `(1 + m) / 4`
code points (three for `m = 11`, one for `m = 3`). Witnesses on inputs the
kernels reach are below. -/
theorem fixed16_tight :
    bytes [3, 3, 3] = 1 + 8 ∧ u16 [3, 3, 3] = 3 ∧ bytes [3] = 1 + 2 ∧ u16 [3] = 1 := by decide

theorem fixed32_tight :
    bytes [4, 4, 4] = 1 + 11 ∧ [4, 4, 4].length = 3 ∧ bytes [4] = 1 + 3 ∧ [4].length = 1 := by
  decide

/-- **UTF-16, a call in a block of the headers' loop.** -/
theorem hyb_store16 {bs : List Nat} (hv : Valid8 bs) {fm : Nat} {kd : Option Nat}
    {d pos t o c p s : Nat} {sc : Bool}
    (h : HX w16 fm kd bs true pos t o sc) (ht : t < pos + 52) (hx : XStep (bs.map w16) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hm : 3 * d ≤ fm + 3) (hK : ∀ K, kd = some K → d ≤ K + 1)
    (hs : s ≤ p + d) : o + s ≤ L (bs.map w16) := by
  obtain ⟨hvx, ha⟩ := hx_vx _ _ _ _ _ h
  rcases ha rfl with hf | ⟨K, hk, hdyn⟩
  · exact fixed_store16 hv hvx ht hx hd hf hm hs
  · exact dyn_store wok_w16 hv hvx ht hx hd hdyn (hK K hk) hs

/-- **UTF-32, a call in a block of the headers' loop.** -/
theorem hyb_store32 {bs : List Nat} (hv : Valid8 bs) {fm : Nat} {kd : Option Nat}
    {d pos t o c p s : Nat} {sc : Bool}
    (h : HX w01 fm kd bs true pos t o sc) (ht : t < pos + 52) (hx : XStep (bs.map w01) t c p)
    (hd : t + c + 1 ≤ pos + 64) (hm : 4 * d ≤ fm + 4) (hK : ∀ K, kd = some K → d ≤ K + 1)
    (hs : s ≤ p + d) : o + s ≤ L (bs.map w01) := by
  obtain ⟨hvx, ha⟩ := hx_vx _ _ _ _ _ h
  rcases ha rfl with hf | ⟨K, hk, hdyn⟩
  · exact fixed_store32 hv hvx ht hx hd hf hm hs
  · exact dyn_store wok_w01 hv hvx ht hx hd hdyn (hK K hk) hs

/-- **Loads, on every byte string.** With `fm ≥ 3` and `K ≥ 2`, a block
either test admits loads inside the input: 64 bytes at `pos`
(src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:26, :90;
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:21, :58) and 16 bytes at each
call `t < pos + 52`. -/
theorem hyb_load {fm : Nat} {kd : Option Nat} {pos t : Nat} {bs : List Nat} (hf : 3 ≤ fm)
    (hK : ∀ K, kd = some K → 2 ≤ K)
    (hb : ∀ b ∈ bs, b < 256) (ha : Admit fm kd bs pos) (ht : t < pos + 52) :
    pos + 64 ≤ bs.length ∧ t + 16 ≤ bs.length := by
  rcases ha with h | ⟨K, hk, h⟩
  · omega
  · exact dyn_load (hK K hk) hb h ht

/-! ## The headers' loop, end to end -/

/-- The stores of the headers' loop: a call (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:62,
:101; src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:34, :69) storing
`s ≤ p + d` units; an ASCII block (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:27-30,
:91-94; src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:22-25, :59-62); the
scalar tail on the rest (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:110-111,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:78-79). -/
inductive HStore (w : Nat → Nat) (fm : Nat) (kd : Option Nat) (d : Nat)
    (tail : List Nat → Nat) (bs : List Nat) : Nat → Prop
  | call (pos t o c p s : Nat) (sc : Bool) (h : HX w fm kd bs true pos t o sc)
      (ht : t < pos + 52) (hx : XStep (bs.map w) t c p) (hd : t + c + 1 ≤ pos + 64)
      (hs : s ≤ p + d) : HStore w fm kd d tail bs (o + s)
  | ascii (t o : Nat) (sc sc' : Bool) (h : HX w fm kd bs false t t o sc)
      (hh : head fm kd bs t sc = some sc') (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) :
      HStore w fm kd d tail bs (o + 64)
  | tail (t o : Nat) (sc : Bool) (h : HX w fm kd bs false t t o sc) :
      HStore w fm kd d tail bs (o + tail (bs.drop t))

/-- The ends of the input loads of the headers' loop: the block load at a
head that runs the block, and the 16-byte load of each call. -/
inductive HLoad (w : Nat → Nat) (fm : Nat) (kd : Option Nat) (bs : List Nat) : Nat → Prop
  | block (t o : Nat) (sc sc' : Bool) (h : HX w fm kd bs false t t o sc)
      (hh : head fm kd bs t sc = some sc') : HLoad w fm kd bs (t + 64)
  | call (pos t o : Nat) (sc : Bool) (h : HX w fm kd bs true pos t o sc)
      (ht : t < pos + 52) : HLoad w fm kd bs (t + 16)

theorem hyb_loads {w : Nat → Nat} {fm : Nat} {kd : Option Nat} (hf : 3 ≤ fm)
    (hK : ∀ K, kd = some K → 2 ≤ K) {bs : List Nat}
    (hb : ∀ b ∈ bs, b < 256) (e : Nat) (h : HLoad w fm kd bs e) : e ≤ bs.length := by
  cases h with
  | block t o sc sc' h hh => exact (head_admit hh).len
  | call pos t o sc h ht => exact (hyb_load hf hK hb ((hx_vx _ _ _ _ _ h).2 rfl) ht).2

theorem hyb_tail {w : Nat → Nat} (hw : WOK w) (tail : List Nat → Nat)
    (htail : ∀ ys, (∀ b ∈ ys, b < 256) → tail ys ≤ L (ys.map w))
    {bs : List Nat} (hv : Valid8 bs) {t o : Nat} (h : VX w bs false t t o) :
    o + tail (bs.drop t) ≤ L (bs.map w) := by
  have hi := (vx_inv hw hv _ _ _ _ h).1
  have ht := htail (bs.drop t) (fun b hb => valid_lt256 hv b (List.mem_of_mem_drop hb))
  rw [List.map_drop] at ht
  have := L_take_add_drop t (bs.map w)
  omega

theorem kd16_ok (a : Bool) : ∀ K, kd16 a = some K → d16 a ≤ K + 1 ∧ 2 ≤ K := by
  intro K h; cases a <;> simp [kd16] at h <;> subst h <;> decide

theorem kd32_ok (a : Bool) : ∀ K, kd32 a = some K → d32 a ≤ K + 1 ∧ 2 ≤ K := by
  intro K h; cases a <;> simp [kd32] at h <;> subst h <;> decide

/-- **`valid_utf8_to_utf16.h` on every target, end to end.** On valid UTF-8,
with the target's fixed margin (`f16`), extra block (`kd16`) and deficit
(`ValidDynamic.d16`), every store of the loop (the calls, the ASCII blocks,
the scalar `convert_valid` on the rest) ends inside `utf16_length_from_utf8`,
and every load inside the input. -/
theorem hyb_utf16_safe (arm64 : Bool) (f : Nat) {bs : List Nat} (hv : Valid8 bs) :
    (∀ s, HStore w16 (f16 arm64) (kd16 arm64) (d16 arm64)
        (Scalar.run (fun ys => Scalar.u8valid ys 2) f) bs s → s ≤ L (bs.map w16)) ∧
    (∀ e, HLoad w16 (f16 arm64) (kd16 arm64) bs e → e ≤ bs.length) := by
  refine ⟨fun s h => ?_, hyb_loads (by cases arm64 <;> decide)
    (fun K hk => (kd16_ok arm64 K hk).2) (valid_lt256 hv)⟩
  cases h with
  | call pos t o c p s sc h ht hx hd hs =>
    exact hyb_store16 hv h ht hx hd (by cases arm64 <;> decide)
      (fun K hk => (kd16_ok arm64 K hk).1) hs
  | ascii t o sc sc' h hh hascii =>
    exact vx_ascii_store wok_w16 hv (hx_vx _ _ _ _ _ h).1 (head_admit hh).len hascii
  | tail t o sc h =>
    exact hyb_tail wok_w16 _ (fun ys hb => Scalar.valid8to16_le f ys hb) hv (hx_vx _ _ _ _ _ h).1

/-- **`valid_utf8_to_utf32.h` on every target, end to end** (`f32`, `kd32`,
`d32`). -/
theorem hyb_utf32_safe (la : Bool) (f : Nat) {bs : List Nat} (hv : Valid8 bs) :
    (∀ s, HStore w01 (f32 la) (kd32 la) (d32 la)
        (Scalar.run (fun ys => Scalar.u8valid ys 1) f) bs s → s ≤ L (bs.map w01)) ∧
    (∀ e, HLoad w01 (f32 la) (kd32 la) bs e → e ≤ bs.length) := by
  refine ⟨fun s h => ?_, hyb_loads (by cases la <;> decide)
    (fun K hk => (kd32_ok la K hk).2) (valid_lt256 hv)⟩
  cases h with
  | call pos t o c p s sc h ht hx hd hs =>
    exact hyb_store32 hv h ht hx hd (by cases la <;> decide) (fun K hk => (kd32_ok la K hk).1) hs
  | ascii t o sc sc' h hh hascii =>
    exact vx_ascii_store wok_w01 hv (hx_vx _ _ _ _ _ h).1 (head_admit hh).len hascii
  | tail t o sc h =>
    exact hyb_tail wok_w01 _ (fun ys hb => Scalar.valid8to32_le f ys hb) hv (hx_vx _ _ _ _ _ h).1

/-- The constants: each fixed margin is the least for its deficit
(`3 d ≤ m + 3`, `4 d ≤ m + 4`) and at least 3 (the loads), and the extra
block's `K = 3` is the least with `d ≤ K + 1`. -/
theorem constants :
    (∀ a, f16 a = max (3 * d16 a - 3) 3) ∧ (∀ a, f32 a = max (4 * d32 a - 4) 3) ∧
    kd16 false = some (d16 false - 1) ∧ kd32 false = some (d32 false - 1) := by
  refine ⟨fun a => ?_, fun a => ?_, by decide, by decide⟩ <;> cases a <;> decide

/-! ## One less fails, through the headers' loop

Each witness is valid UTF-8. With one constant lowered, `head` at the first
block runs the block at 0, whose calls (`runChain`, each an exact call
`XStep` below `pos + 52`) reach byte 51; the call there stores past `L` or
loads past the input. With the header's constants, `head` runs no block at
0 (neither the fixed test nor, where there is one, the extra block), so the
C++ hands the whole input to the scalar tail. -/

/-- A chain of calls `(consumed, produced)` from byte `t` in the block at
`pos`: each below `pos + 52` and exact. -/
def runChain (xs : List Nat) (pos t o : Nat) : List (Nat × Nat) → Option (Nat × Nat)
  | [] => some (t, o)
  | (c, p) :: r =>
    if t < pos + 52 ∧ 1 ≤ c ∧ xs.getD (t + c) 0 ≠ 0 ∧ p = L ((xs.drop t).take c) then
      runChain xs pos (t + c) (o + p) r
    else none

theorem hx_chain {w : Nat → Nat} {fm : Nat} {kd : Option Nat} {bs : List Nat} {pos : Nat}
    {sc : Bool} :
    ∀ (cs : List (Nat × Nat)) (t o t' o' : Nat), HX w fm kd bs true pos t o sc →
      runChain (bs.map w) pos t o cs = some (t', o') → HX w fm kd bs true pos t' o' sc := by
  intro cs
  induction cs with
  | nil => intro t o t' o' h hr; simp [runChain] at hr; obtain ⟨rfl, rfl⟩ := hr; exact h
  | cons cp r ih =>
    intro t o t' o' h hr
    obtain ⟨c, p⟩ := cp
    simp only [runChain] at hr
    split at hr
    · rename_i hc
      exact ih _ _ _ _ (HX.call pos t o c p sc h hc.1 ⟨hc.2.1, hc.2.2.1, hc.2.2.2⟩) hr
    · cases hr

/-- The block at 0, entered from the start. -/
theorem hx_enter {w : Nat → Nat} {fm : Nat} {kd : Option Nat} {bs : List Nat} {sc : Bool}
    (hh : head fm kd bs 0 false = some sc) : HX w fm kd bs true 0 0 0 sc :=
  HX.enter 0 0 false sc HX.start hh

/-- The calls of `lvTight` up to byte 51 (`ValidDynamic.lv_calls`). -/
def lvChain : List (Nat × Nat) := [(6, 6), (6, 6), (6, 6), (6, 6), (12, 6), (7, 6), (8, 6)]
/-- The calls of `la924b` up to byte 51 (`ValidDynamic.utf16_la_K2`). -/
def laChain : List (Nat × Nat) := [(16, 16), (16, 16), (12, 4), (7, 4)]
/-- The calls of `laRead` up to byte 51 (`ValidDynamic.read_K1`). -/
def rdChain : List (Nat × Nat) := [(16, 16), (16, 16), (12, 6), (7, 6)]

/-- `lvTight` with its last 4 bytes replaced by three U+3042 (`E3 81 82`):
72 bytes. -/
def v16fix : List Nat := lvTight.take 63 ++ (List.replicate 3 [0xE3, 0x81, 0x82]).flatten

/-- `lvTight` with its last 4 bytes replaced by three U+1F600
(`F0 9F 98 80`): 75 bytes (the input of the former `ValidTight`). -/
def v32fix : List Nat := lvTight.take 63 ++ (List.replicate 3 [0xF0, 0x9F, 0x98, 0x80]).flatten

/-- `ValidDynamic.la924b` with its last 5 bytes replaced by three U+3042:
72 bytes. -/
def la16fix : List Nat := la924b.take 63 ++ (List.replicate 3 [0xE3, 0x81, 0x82]).flatten

/-- 32 ASCII bytes, seven `é`, 5 ASCII bytes, six `é`, one U+1F600: 67 bytes. -/
def la32fix : List Nat :=
  List.replicate 32 0x41 ++ (List.replicate 7 [0xC3, 0xA9]).flatten ++ List.replicate 5 0x41 ++
    (List.replicate 6 [0xC3, 0xA9]).flatten ++ [0xF0, 0x9F, 0x98, 0x80]

/-- **UTF-16, westmere, haswell, ppc64: fixed margin 8 overflows.** On
`v16fix` (hex: `41`×24, `C3 A9`×7, `41`×5, `C3 A9`×2, `41`×16, `E3 81 82`×3)
the fixed test with 8 runs the block at 0 (`64 + 8 ≤ 72`). With the header's
9 the loop runs no block, and the extra block does not run either: the scan
for 3 leading bytes covers the three 3-byte characters, dynamic margin 10.
The calls reach 51 with 42 units, the whole mask is `0xfff` there, and the
kernel stores 16 units for 12 (src/westmere/sse_convert_utf8_to_utf16.cpp:27-40),
ending at 58, one past `L = 57`: the 9 bytes after the call are three
characters, three units. -/
theorem fix16_x86 (tail : List Nat → Nat) :
    let bs := v16fix
    let xs := bs.map w16
    Valid8 bs ∧ bs.length = 72 ∧ emask xs 51 12 = 0xFFF ∧
      head 8 (some 3) bs 0 false = some false ∧ head 9 (some 3) bs 0 false = none ∧
      ¬ (0 + 64 + safetyMargin 3 bs ≤ bs.length) ∧
      HX w16 8 (some 3) bs true 0 51 42 false ∧ XStep xs 51 12 12 ∧
      HStore w16 8 (some 3) 4 tail bs (L xs + 1) := by
  intro bs xs
  have hh : head 8 (some 3) bs 0 false = some false := by decide
  have h : HX w16 8 (some 3) bs true 0 51 42 false :=
    hx_chain lvChain 0 0 51 42 (hx_enter hh) (by decide)
  have hx : XStep xs 51 12 12 := by unfold XStep; decide
  have hL : 42 + 16 = L xs + 1 := by decide
  refine ⟨by unfold Valid8; decide +kernel, by decide, by decide, hh, by decide, by decide, h, hx, ?_⟩
  rw [← hL]
  exact HStore.call 0 51 42 12 12 16 false h (by decide) hx (by decide) (by decide)

/-- **UTF-16, westmere, haswell, ppc64: `K = 2` in the extra block
overflows**, on `ValidDynamic`'s `lvTight` (67 bytes). The fixed 9 runs no
block (`73 > 67`); with `K = 2` the extra block runs (`64 + 3 ≤ 67`,
dynamic margin 3). With the header's `K = 3` it does not (`68 > 67`). Same
calls and store as `ValidDynamic.utf16_x86_K2`. -/
theorem k16_x86 (tail : List Nat → Nat) :
    let bs := lvTight
    let xs := bs.map w16
    head 9 (some 2) bs 0 false = some true ∧ head 9 (some 3) bs 0 false = none ∧
      HX w16 9 (some 2) bs true 0 51 42 true ∧ HStore w16 9 (some 2) 4 tail bs (L xs + 1) := by
  intro bs xs
  have hh : head 9 (some 2) bs 0 false = some true := by decide
  have h : HX w16 9 (some 2) bs true 0 51 42 true :=
    hx_chain lvChain 0 0 51 42 (hx_enter hh) (by decide)
  have hL : 42 + 16 = L xs + 1 := by decide
  refine ⟨hh, by decide, h, ?_⟩
  rw [← hL]
  exact HStore.call 0 51 42 12 12 16 true h (by decide) (by unfold XStep; decide) (by decide)
    (by decide)

/-- **UTF-32, westmere, haswell, ppc64, arm64: fixed margin 11 overflows.**
On `v32fix` (hex: `41`×24, `C3 A9`×7, `41`×5, `C3 A9`×2, `41`×16,
`F0 9F 98 80`×3; 75 bytes) the fixed test with 11 runs the block at 0; with
the header's 12 no block runs, the extra block included (dynamic margin
13). At 51 the kernel stores 16 units for 12
(src/westmere/sse_convert_utf8_to_utf32.cpp:24-35,
src/arm64/arm_convert_utf8_to_utf32.cpp:23-30), ending at 58, one past
`L = 57`. -/
theorem fix32_x86 (tail : List Nat → Nat) :
    let bs := v32fix
    let xs := bs.map w01
    Valid8 bs ∧ bs.length = 75 ∧ emask xs 51 12 = 0xFFF ∧
      head 11 (some 3) bs 0 false = some false ∧ head 12 (some 3) bs 0 false = none ∧
      ¬ (0 + 64 + safetyMargin 3 bs ≤ bs.length) ∧
      HX w01 11 (some 3) bs true 0 51 42 false ∧ HStore w01 11 (some 3) 4 tail bs (L xs + 1) := by
  intro bs xs
  have hh : head 11 (some 3) bs 0 false = some false := by decide
  have h : HX w01 11 (some 3) bs true 0 51 42 false :=
    hx_chain lvChain 0 0 51 42 (hx_enter hh) (by decide)
  have hL : 42 + 16 = L xs + 1 := by decide
  refine ⟨by unfold Valid8; decide +kernel, by decide, by decide, hh, by decide, by decide, h, ?_⟩
  rw [← hL]
  exact HStore.call 0 51 42 12 12 16 false h (by decide) (by unfold XStep; decide) (by decide)
    (by decide)

/-- **UTF-32, westmere, haswell, ppc64, arm64: `K = 2` in the extra block
overflows**, on `lvTight`: the fixed 12 runs no block (`76 > 67`), the extra
block with `K = 2` runs (dynamic margin 3); with `K = 3` it does not. -/
theorem k32_x86 (tail : List Nat → Nat) :
    let bs := lvTight
    let xs := bs.map w01
    head 12 (some 2) bs 0 false = some true ∧ head 12 (some 3) bs 0 false = none ∧
      HX w01 12 (some 2) bs true 0 51 42 true ∧ HStore w01 12 (some 2) 4 tail bs (L xs + 1) := by
  intro bs xs
  have hh : head 12 (some 2) bs 0 false = some true := by decide
  have h : HX w01 12 (some 2) bs true 0 51 42 true :=
    hx_chain lvChain 0 0 51 42 (hx_enter hh) (by decide)
  have hL : 42 + 16 = L xs + 1 := by decide
  refine ⟨hh, by decide, h, ?_⟩
  rw [← hL]
  exact HStore.call 0 51 42 12 12 16 true h (by decide) (by unfold XStep; decide) (by decide)
    (by decide)

/-- **UTF-16, lsx and lasx: fixed margin 8 overflows.** On `la16fix` (hex:
`41`×32, `E3 81 82`×4, `C3 A9`×2, `41`, `C3 A9`, `E3 81 82`×7; 72 bytes) the
fixed 8 runs the block at 0; with the header's 9 no block runs, the extra
block included (dynamic margin 10). The calls of `ValidDynamic.utf16_la_K2`
reach 51 with 40 units; the `0x924` path there
(src/lsx/lsx_convert_utf8_to_utf16.cpp:36-47) stores 8 units for 4, ending
at 48, one past `L = 47`. -/
theorem fix16_la (tail : List Nat → Nat) :
    let bs := la16fix
    let xs := bs.map w16
    Valid8 bs ∧ bs.length = 72 ∧ emask xs 51 12 = 0x924 ∧
      head 8 (some 3) bs 0 false = some false ∧ head 9 (some 3) bs 0 false = none ∧
      ¬ (0 + 64 + safetyMargin 3 bs ≤ bs.length) ∧
      HX w16 8 (some 3) bs true 0 51 40 false ∧ HStore w16 8 (some 3) 4 tail bs (L xs + 1) := by
  intro bs xs
  have hh : head 8 (some 3) bs 0 false = some false := by decide
  have h : HX w16 8 (some 3) bs true 0 51 40 false :=
    hx_chain laChain 0 0 51 40 (hx_enter hh) (by decide)
  have hL : 40 + 8 = L xs + 1 := by decide
  refine ⟨by unfold Valid8; decide +kernel, by decide, by decide, hh, by decide, by decide, h, ?_⟩
  rw [← hL]
  exact HStore.call 0 51 40 12 4 8 false h (by decide) (by unfold XStep; decide) (by decide)
    (by decide)

/-- **UTF-16, lsx and lasx: `K = 2` in the extra block overflows**, on
`la924b` (68 bytes): the fixed 9 runs no block (`73 > 68`), the extra block
with `K = 2` runs (dynamic margin 3); with `K = 3` the scan gives 6 and it
does not. -/
theorem k16_la (tail : List Nat → Nat) :
    let bs := la924b
    let xs := bs.map w16
    head 9 (some 2) bs 0 false = some true ∧ head 9 (some 3) bs 0 false = none ∧
      HX w16 9 (some 2) bs true 0 51 40 true ∧ HStore w16 9 (some 2) 4 tail bs (L xs + 1) := by
  intro bs xs
  have hh : head 9 (some 2) bs 0 false = some true := by decide
  have h : HX w16 9 (some 2) bs true 0 51 40 true :=
    hx_chain laChain 0 0 51 40 (hx_enter hh) (by decide)
  have hL : 40 + 8 = L xs + 1 := by decide
  refine ⟨hh, by decide, h, ?_⟩
  rw [← hL]
  exact HStore.call 0 51 40 12 4 8 true h (by decide) (by unfold XStep; decide) (by decide)
    (by decide)

/-- The row of mask `0x57E` (one `é`, five ASCII bytes): six characters, 7
bytes. -/
theorem row_57E : Tables.bigIdx[0x57E]! = 32 ∧ Tables.bigConsumed[0x57E]! = 7 := by decide +kernel

/-- **UTF-32, lsx and lasx: fixed margin 3 overflows.** These targets have
no extra block. On `la32fix` (hex: `41`×32, `C3 A9`×7, `41`×5, `C3 A9`×6,
`F0 9F 98 80`; 67 bytes) the fixed 3 runs the block at 0; the header's 4
does not. The calls of src/lsx/lsx_convert_utf8_to_utf32.cpp and
src/lasx/lasx_convert_utf8_to_utf32.cpp: 16 ASCII bytes at 0 and 16, the
`0xaaa` path at 32 (12 bytes, 6 units), the row of mask `0x57E` at 44 (7
bytes, 6 units, `row_57E`), so 44 units at 51. There the mask is `0xaaa`
again: 8 units stored for 6 (src/lsx/lsx_convert_utf8_to_utf32.cpp:44-55),
ending at 52, one past `L = 51`: the 4 bytes after the call are one
character. -/
theorem fix32_la (tail : List Nat → Nat) :
    let bs := la32fix
    let xs := bs.map w01
    Valid8 bs ∧ bs.length = 67 ∧ emask xs 32 12 = 0xAAA ∧ emask xs 44 12 = 0x57E ∧
      emask xs 51 12 = 0xAAA ∧
      head 3 none bs 0 false = some false ∧ head 4 none bs 0 false = none ∧
      HX w01 3 none bs true 0 51 44 false ∧ HStore w01 3 none 2 tail bs (L xs + 1) := by
  intro bs xs
  have hh : head 3 none bs 0 false = some false := by decide
  have h : HX w01 3 none bs true 0 51 44 false := hx_chain rdChain 0 0 51 44 (hx_enter hh) (by decide)
  have hL : 44 + 8 = L xs + 1 := by decide
  refine ⟨by unfold Valid8; decide +kernel, by decide, by decide, by decide, by decide, hh,
    by decide, h, ?_⟩
  rw [← hL]
  exact HStore.call 0 51 44 12 6 8 false h (by decide) (by unfold XStep; decide) (by decide)
    (by decide)

/-- **arm64 UTF-16: fixed margin 2 loads past the input.** arm64 has no
extra block. On `ValidLatin1.laRead` (hex: `41`×32, `C3 A9`×7, `41`×20; 66
bytes) the fixed 2 runs the block at 0 (`64 + 2 ≤ 66`), the calls of
`ValidDynamic.read_K1` reach 51, and the call there loads bytes 51 to 66
(src/arm64/arm_convert_utf8_to_utf16.cpp:13); with the header's 3 no block
runs. -/
theorem fix16_arm_read :
    let bs := laRead
    head 2 none bs 0 false = some false ∧ head 3 none bs 0 false = none ∧
      HLoad w16 2 none bs (bs.length + 1) := by
  intro bs
  have h1 : head 2 none bs 0 false = some false := by decide
  have hl : bs.length + 1 = 51 + 16 := by decide
  refine ⟨h1, by decide, ?_⟩
  rw [hl]
  exact HLoad.call 0 51 44 false (hx_chain rdChain 0 0 51 44 (hx_enter h1) (by decide)) (by decide)

end SafetyMargins.ValidHybrid
