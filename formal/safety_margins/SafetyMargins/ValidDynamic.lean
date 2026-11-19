import SafetyMargins.ValidExact
import SafetyMargins.ValidLatin1
import SafetyMargins.Scalar
/-
`valid_utf8_to_utf16.h` and `valid_utf8_to_utf32.h` (`convert_valid`): the
dynamic margin, on the input their contract allows: valid UTF-8
(`ValidExact.Valid8`).

The headers run their blocks on a fixed margin. After that loop, except on
arm64 for UTF-16 and on LoongArch for UTF-32, they scan back from the end
until they have seen `K = leading_bytes = 3` leading bytes and run one more
block if `pos + 64 + (size - margin + 1) <= size`
(src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:80-89,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:48-61). This file proves the
blocks such a dynamic test admits, for any `K`; `ValidHybrid` combines it
with the fixed margin into the code of the headers.

* **The loop test.** `scanLoop` transcribes the backward scan;
  `scan_eq_tk` proves that `size - margin` is `Utf8.tk K` of the leading-byte
  weights, and `tk_map` that the weights of either length function give the
  same `tk`.
* **Stores (`dyn_store`).** On valid input the cursor is exact and every call
  ends just before a leading byte (`ValidExact.vx_inv`). That byte is at most
  `pos + 63`, and the loop test leaves `K` leading bytes at or after
  `pos + 65`, so at least `1 + K` units follow the call. A call that stores
  `s ≤ p + d` units for the `p` it produces therefore fits when `d ≤ K + 1`.
  The deficits are `d = 4` on x86, POWER and LoongArch for UTF-16 and on x86,
  POWER and arm64 for UTF-32 (the `0xfff`, `0x924` and `idx < 145` paths), and
  `d = 2` on arm64 for UTF-16 and on LoongArch for UTF-32 (`ValidExact`,
  §"The stores"). So the stores need `K = 3` and `K = 1`.
* **Loads (`dyn_load`).** The 64-byte block load at `pos` and the 16-byte
  load of each call at `t < pos + 52` stay in the input when `K ≥ 2`
  (`Utf8.read_ok`).
* **The dynamic blocks, end to end (`utf16_safe`, `utf32_safe`).** Every
  store of a loop whose blocks pass the dynamic test (`DStore`: the calls, the ASCII blocks and the scalar tail, which writes at
  most `L` of the rest on every byte string, `Scalar.valid8to16_le`,
  `Scalar.valid8to32_le`) ends inside `L`, and every load (`DLoad`) inside the
  input, with the header's `K` for each target.
* **Optimality, on inputs of the contract, with the calls of the kernels
  derived through the loop model `ValidExact.VX`.**
  `utf16_x86_K2`, `utf32_x86_K2`: on `lvTight` (67 bytes) `K = 2` admits a
  block whose call at byte 51 takes the `0xfff` path (westmere, haswell, ppc64
  for UTF-16; westmere, haswell, ppc64, arm64 for UTF-32) and stores 16 units
  ending one past `L`. `utf16_la_K2`: on `la924b` (68 bytes) `K = 2` admits a
  block whose call at 51 takes the lsx/lasx `0x924` path, 8 units ending one
  past `L`. `read_K1`: on `ValidLatin1.laRead` (66 bytes) `K = 1` admits a
  block whose call at 51 loads 16 bytes, one past the input (with the kernels
  of arm64 UTF-16 and lsx/lasx UTF-32; those targets have no dynamic block in
  the headers, so this bounds the dynamic test only).

Read from the C++, as in `ValidExact`: per path, the deficit `s ≤ p + d`; that a
call ends by `pos + 63` (`hd`; the end-of-code-point mask has bit 63 clear and
a call ends at a set bit, including the 16-byte fast paths); that the
kernels take the paths named in the witnesses (the fast-path tests, and the
table rows `TableFacts.lv_rows`, `TableFacts.la_924_rows`).
-/
namespace SafetyMargins.ValidDynamic
open SafetyMargins SafetyMargins.Utf8 SafetyMargins.Validator SafetyMargins.Exact
  SafetyMargins.ValidExact SafetyMargins.ValidLatin1 SafetyMargins.Tables SafetyMargins.TableFacts

/-! ## The loop test -/

/-- `int8_t(b) > -65` (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:85,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:57): `b` read as a signed byte. -/
def leadTest (b : Nat) : Bool :=
  decide ((if b < 128 then (b : Int) else (b : Int) - 256) > -65)

/-- On a byte, the test is "not a continuation byte", the weight `w01`. -/
theorem leadTest_w01 : ∀ b, b < 256 → (if leadTest b then 1 else 0) = w01 b := by
  decide +kernel

/-- The `for` loop of src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:84-86
(src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:56-58): `scanLoop bs K margin
leading_byte` is the final `margin`. -/
def scanLoop (bs : List Nat) (K : Nat) : Nat → Nat → Nat
  | 0, _ => 0
  | m + 1, lb =>
    if lb < K then scanLoop bs K m (lb + if leadTest (bs.getD m 0) then 1 else 0)
    else m + 1

/-- `safety_margin = size - margin + 1`
(src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:89,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:61), in the test of the extra block. -/
def safetyMargin (K : Nat) (bs : List Nat) : Nat :=
  bs.length - scanLoop bs K bs.length 0 + 1

theorem scanLoop_le (bs : List Nat) (K : Nat) : ∀ m lb, scanLoop bs K m lb ≤ m := by
  intro m
  induction m with
  | zero => intro lb; simp [scanLoop]
  | succ m ih =>
    intro lb
    simp only [scanLoop]
    split
    · exact Nat.le_trans (ih _) (Nat.le_succ m)
    · exact Nat.le_refl _

theorem scanLoop_append (bs ys : List Nat) (K : Nat) :
    ∀ m lb, m ≤ bs.length → scanLoop (bs ++ ys) K m lb = scanLoop bs K m lb := by
  intro m
  induction m with
  | zero => intro lb _; rfl
  | succ m ih =>
    intro lb hm
    simp only [scanLoop]
    have hg : (bs ++ ys).getD m 0 = bs.getD m 0 := by
      simp only [List.getD_eq_getElem?_getD, List.getElem?_append_left (by omega : m < bs.length)]
    rw [hg, ih _ (by omega)]

theorem cnt_append_one (zs : List Nat) (v : Nat) :
    cnt (zs ++ [v]) = cnt zs + (if v = 0 then 0 else 1) := by
  rw [cnt_append]; simp

/-- `tk` grows from the back: the last unit is in every non-empty suffix. -/
theorem tk_snoc (K : Nat) (zs : List Nat) (v : Nat) :
    tk K (zs ++ [v]) = if K = 0 then 0 else 1 + tk (K - (if v = 0 then 0 else 1)) zs := by
  induction zs with
  | nil =>
    simp only [List.nil_append, tk, cnt_nil]
    by_cases hK : K = 0 <;> simp [hK]
  | cons z zs ih =>
    have e1 : tk K ((z :: zs) ++ [v]) =
        if K ≤ cnt (zs ++ [v]) then tk K (zs ++ [v]) else (zs ++ [v]).length + 1 := rfl
    have e2 : tk (K - (if v = 0 then 0 else 1)) (z :: zs) =
        if K - (if v = 0 then 0 else 1) ≤ cnt zs then tk (K - (if v = 0 then 0 else 1)) zs
        else zs.length + 1 := rfl
    rw [e1, e2, cnt_append_one, ih, List.length_append, List.length_singleton]
    by_cases hK : K = 0
    · simp [hK]
    · simp only [hK, ↓reduceIte]
      by_cases hv : v = 0
      · simp only [hv, ↓reduceIte, Nat.add_zero, Nat.sub_zero]
        split <;> omega
      · simp only [hv, ↓reduceIte]
        by_cases h1 : K ≤ cnt zs + 1
        · rw [ite_eq_left h1, ite_eq_left (by omega)]
        · rw [ite_eq_right h1, ite_eq_right (by omega)]; omega

/-- The leading bytes `int8_t(b) > -65` are the non-zero `w01` weights. -/
theorem scanLoop_tk (K : Nat) :
    ∀ (bs : List Nat), (∀ b ∈ bs, b < 256) → ∀ lb,
      bs.length - scanLoop bs K bs.length lb = tk (K - lb) (bs.map w01) := by
  suffices H : ∀ n (bs : List Nat), bs.length = n → (∀ b ∈ bs, b < 256) → ∀ lb,
      bs.length - scanLoop bs K bs.length lb = tk (K - lb) (bs.map w01) from
    fun bs => H _ bs rfl
  intro n
  induction n with
  | zero =>
    intro bs hn _ lb
    rw [List.eq_nil_of_length_eq_zero hn]; simp [scanLoop, tk]
  | succ n ih' =>
    intro bs hn hb lb
    rcases List.eq_nil_or_concat bs with h0 | ⟨ys, y, rfl⟩
    · subst h0; simp at hn
    rw [List.concat_eq_append] at hn hb ⊢
    have ih : (∀ b ∈ ys, b < 256) → ∀ lb,
        ys.length - scanLoop ys K ys.length lb = tk (K - lb) (ys.map w01) :=
      ih' ys (by simp at hn; omega)
    have hy : y < 256 := hb y (by simp)
    have hys : ∀ b ∈ ys, b < 256 := fun b h => hb b (by simp [h])
    rw [List.length_append, List.length_singleton, List.map_append, List.map_singleton, tk_snoc]
    simp only [scanLoop]
    have hg : (ys ++ [y]).getD ys.length 0 = y := by simp
    rw [hg, leadTest_w01 y hy]
    by_cases hlb : lb < K
    · rw [ite_eq_left hlb, scanLoop_append ys [y] K _ _ (Nat.le_refl _), ite_eq_right (by omega)]
      have hih := ih hys (lb + w01 y)
      have hle := scanLoop_le ys K ys.length (lb + w01 y)
      have hk : K - (lb + w01 y) = K - lb - (if w01 y = 0 then 0 else 1) := by
        unfold w01; split <;> simp <;> omega
      rw [hk] at hih
      omega
    · rw [ite_eq_right hlb, ite_eq_left (by omega)]
      omega

/-- **The loop test is `Utf8.tk`.** `size - margin` is the length of the
shortest suffix holding `K` leading bytes (the whole input if there are
fewer), so the loop test is `pos + 64 + (tk K + 1) <= size`. -/
theorem safetyMargin_eq (K : Nat) (bs : List Nat) (hb : ∀ b ∈ bs, b < 256) :
    safetyMargin K bs = tk K (bs.map w01) + 1 := by
  unfold safetyMargin
  rw [scanLoop_tk K bs hb 0, Nat.sub_zero]

/-- `tk` sees only which weights are zero. -/
theorem cnt_map (w w' : Nat → Nat) (bs : List Nat) (h : ∀ b ∈ bs, (w b = 0 ↔ w' b = 0)) :
    cnt (bs.map w) = cnt (bs.map w') := by
  induction bs with
  | nil => rfl
  | cons b bs ih =>
    simp only [List.map_cons, cnt_cons]
    rw [ih (fun c hc => h c (by simp [hc]))]
    have := h b (by simp)
    by_cases h0 : w b = 0 <;> simp_all

theorem tk_map (K : Nat) (w w' : Nat → Nat) (bs : List Nat) (h : ∀ b ∈ bs, (w b = 0 ↔ w' b = 0)) :
    tk K (bs.map w) = tk K (bs.map w') := by
  induction bs with
  | nil => rfl
  | cons b bs ih =>
    have h' : ∀ c ∈ bs, (w c = 0 ↔ w' c = 0) := fun c hc => h c (by simp [hc])
    simp only [List.map_cons, tk, List.length_map]
    rw [ih h', cnt_map w w' bs h']

/-- For the weights of a length function (`WOK`: zero exactly on continuation
bytes), the loop test in terms of those weights. -/
theorem safetyMargin_w {w : Nat → Nat} (hw : WOK w) (K : Nat) {bs : List Nat} (hv : Valid8 bs) :
    safetyMargin K bs = tk K (bs.map w) + 1 := by
  rw [safetyMargin_eq K bs (valid_lt256 hv), tk_map K w01 w bs]
  intro b hb
  have hb' := valid_lt256 hv b hb
  rw [(w01_contZero b hb'), (hw.1 b hb')]


/-! ## Stores and loads in a block -/

theorem cnt_one (xs : List Nat) (i : Nat) (h : xs.getD i 0 ≠ 0) : cnt ((xs.drop i).take 1) = 1 := by
  have hi : i < xs.length := by
    refine Classical.byContradiction fun hge => h ?_
    rw [List.getD_eq_getElem?_getD, List.getElem?_eq_none (by omega)]
    rfl
  rw [List.drop_eq_getElem_cons hi]
  rw [List.getD_eq_getElem?_getD, List.getElem?_eq_getElem hi] at h
  simp only [List.take_succ_cons, List.take_zero, cnt_cons, cnt_nil]
  simp at h
  simp [h]

/-- **A call stores inside `L` when `d ≤ K + 1`.** In a block the loop
admitted with the dynamic margin, on valid input, a call at `t` that produces
`p` units, ends by `pos + 63`, and stores `s ≤ p + d` units, ends its store
inside `L`: the cursor is `L` of the consumed bytes (`vx_after_call`), the
byte after the call is a leading byte, and the `K` leading bytes the loop
test counted lie after `pos + 64` (`Utf8.cover`). -/
theorem dyn_store {w : Nat → Nat} (hw : WOK w) {bs : List Nat} (hv : Valid8 bs)
    {K d pos t o c p s : Nat} (h : VX w bs true pos t o) (ht : t < pos + 52)
    (hx : XStep (bs.map w) t c p) (hd : t + c + 1 ≤ pos + 64)
    (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) (hK : d ≤ K + 1) (hs : s ≤ p + d) :
    o + s ≤ L (bs.map w) := by
  obtain ⟨hL, -⟩ := vx_after_call hw hv h ht hx
  rw [safetyMargin_w hw K hv] at hloop
  have h1 := cnt_one (bs.map w) (t + c) hx.2.1
  have := cover K pos (t + c) 1 (o + p) (s - p) 0 (bs.map w) (by simpa using hloop)
    (by omega) (by omega) (by omega)
  omega

/-- **Loads, on every byte string.** With `K ≥ 2` the 64-byte load of the
block at `pos` (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:90,
src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:62) and the 16-byte load of a
call at `t < pos + 52` (`vld1q_u8`, `__lsx_vld`, `_mm_loadu_si128`,
`vector_u8::load` at `input + pos`) stay in the input. -/
theorem dyn_load {K pos t : Nat} {bs : List Nat} (hK : 2 ≤ K) (hb : ∀ b ∈ bs, b < 256)
    (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) (ht : t < pos + 52) :
    pos + 64 ≤ bs.length ∧ t + 16 ≤ bs.length := by
  rw [safetyMargin_eq K bs hb] at hloop
  have := read_ok K pos (bs.map w01) hK (by simpa using hloop)
  simp at this
  omega

/-! ## The model, end to end -/

/-- The least `K` for a UTF-16 dynamic block on each target (stores and
loads): 2 on arm64, 3 elsewhere. The header uses `K = 3`
(src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:80) and has no dynamic block
on arm64 (`ValidHybrid.kd16`). -/
def k16 (arm64 : Bool) : Nat := if arm64 then 2 else 3

/-- The least `K` for a UTF-32 dynamic block on each target: 2 on lsx and
lasx, 3 elsewhere, which the header uses
(src/generic/utf8_to_utf32/valid_utf8_to_utf32.h:48-52, `ValidHybrid.kd32`). -/
def k32 (la : Bool) : Nat := if la then 2 else 3

/-- The deficit of the UTF-16 calls on valid input: 2 on arm64
(`0xaaa`, `idx < 64`: 8 stored, 6 produced; `0x888` is exact on valid input,
`vx_888`), 4 elsewhere (`0xfff`: 16 for 12; `0x924`, `idx < 145`: 8 for 4). -/
def d16 (arm64 : Bool) : Nat := if arm64 then 2 else 4

/-- The deficit of the UTF-32 calls on valid input: 2 on lsx and lasx
(`0xaaa`, `idx < 64`: 8 stored, 6 produced), 4 elsewhere (`0xfff`: 16 for 12). -/
def d32 (la : Bool) : Nat := if la then 2 else 4

/-- The stores of a `convert_valid` loop whose blocks pass the dynamic test
with `K` (src/generic/utf8_to_utf16/valid_utf8_to_utf16.h:89): a call
(:100-105, the call at :101), storing `s ≤ p + d` units; an ASCII block of 64
units (:91-94); the scalar tail `tail` on the rest (:110-111).
`ValidHybrid.HStore` is the loop of the headers. -/
inductive DStore (w : Nat → Nat) (K d : Nat) (tail : List Nat → Nat) (bs : List Nat) :
    Nat → Prop
  | call (pos t o c p s : Nat) (h : VX w bs true pos t o) (ht : t < pos + 52)
      (hx : XStep (bs.map w) t c p) (hd : t + c + 1 ≤ pos + 64)
      (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) (hs : s ≤ p + d) :
      DStore w K d tail bs (o + s)
  | ascii (t o : Nat) (h : VX w bs false t t o) (hlen : t + 64 ≤ bs.length)
      (hascii : ∀ i, i < 64 → bs.getD (t + i) 0 < 0x80) : DStore w K d tail bs (o + 64)
  | tail (t o : Nat) (h : VX w bs false t t o) : DStore w K d tail bs (o + tail (bs.drop t))

/-- The ends of the input loads: the block load at a `pos` the dynamic test
admitted, and the 16-byte load of a call at `t < pos + 52` in that block. -/
inductive DLoad (K : Nat) (bs : List Nat) : Nat → Prop
  | block (pos : Nat) (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) : DLoad K bs (pos + 64)
  | call (pos t : Nat) (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) (ht : t < pos + 52) :
      DLoad K bs (t + 16)

/-- Every store of the model ends inside `L` when `d ≤ K + 1` and the tail
writes at most `L` of its bytes. -/
theorem dyn_safe {w : Nat → Nat} (hw : WOK w) (tail : List Nat → Nat)
    (htail : ∀ ys, (∀ b ∈ ys, b < 256) → tail ys ≤ L (ys.map w))
    {bs : List Nat} (hv : Valid8 bs) {K d : Nat} (hK : d ≤ K + 1) (s : Nat)
    (h : DStore w K d tail bs s) : s ≤ L (bs.map w) := by
  cases h with
  | call pos t o c p s h ht hx hd hloop hs => exact dyn_store hw hv h ht hx hd hloop hK hs
  | ascii t o h hlen hascii => exact vx_ascii_store hw hv h hlen hascii
  | tail t o h =>
    have hi := (vx_inv hw hv _ _ _ _ h).1
    have ht := htail (bs.drop t) (fun b hb => valid_lt256 hv b (List.mem_of_mem_drop hb))
    rw [List.map_drop] at ht
    have := L_take_add_drop t (bs.map w)
    omega

theorem dyn_loads {K : Nat} (hK : 2 ≤ K) {bs : List Nat} (hb : ∀ b ∈ bs, b < 256) (e : Nat)
    (h : DLoad K bs e) : e ≤ bs.length := by
  cases h with
  | block pos hloop => exact (dyn_load (t := pos) hK hb hloop (by omega)).1
  | call pos t hloop ht => exact (dyn_load hK hb hloop ht).2

/-- **UTF-16, the blocks the dynamic test admits, end to end.** On valid
UTF-8, with the least `K` (`k16`) and the target's deficit (`d16`), every
store of a loop whose blocks pass the dynamic test (the calls, the ASCII
blocks, the scalar `convert_valid` on the rest) ends inside
`utf16_length_from_utf8`, and every load inside the input. The code of the
header (fixed-margin loop, then at most one dynamic block) is
`ValidHybrid.hyb_utf16_safe`. -/
theorem utf16_safe (arm64 : Bool) (f : Nat) {bs : List Nat} (hv : Valid8 bs) :
    (∀ s, DStore w16 (k16 arm64) (d16 arm64)
        (Scalar.run (fun ys => Scalar.u8valid ys 2) f) bs s → s ≤ L (bs.map w16)) ∧
    (∀ e, DLoad (k16 arm64) bs e → e ≤ bs.length) :=
  ⟨dyn_safe wok_w16 _ (fun ys hb => Scalar.valid8to16_le f ys hb) hv
      (by cases arm64 <;> decide),
   dyn_loads (by cases arm64 <;> decide) (valid_lt256 hv)⟩

/-- **UTF-32, the blocks the dynamic test admits, end to end** (`k32`,
`d32`); the header's loop is `ValidHybrid.hyb_utf32_safe`. -/
theorem utf32_safe (la : Bool) (f : Nat) {bs : List Nat} (hv : Valid8 bs) :
    (∀ s, DStore w01 (k32 la) (d32 la)
        (Scalar.run (fun ys => Scalar.u8valid ys 1) f) bs s → s ≤ L (bs.map w01)) ∧
    (∀ e, DLoad (k32 la) bs e → e ≤ bs.length) :=
  ⟨dyn_safe wok_w01 _ (fun ys hb => Scalar.valid8to32_le f ys hb) hv
      (by cases la <;> decide),
   dyn_loads (by cases la <;> decide) (valid_lt256 hv)⟩

/-! ### Table calls

A table call needs no hypothesis but its deficit: on valid input it is exact
(`vx_w01`, `vx_utf16_145`, `vx_utf16_3`) and ends by `pos + 63` (`row_hd`). -/

/-- UTF-32: a table call storing `s ≤ units + d`. -/
theorem dyn_row32 {bs : List Nat} (hv : Valid8 bs) {K d pos t o s : Nat}
    (h : VX w01 bs true pos t o) (ht : t < pos + 52)
    (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) (hK : d ≤ K + 1)
    (hs : s ≤ units bigIdx[(row (bs.map w01) t).m]! + d) : o + s ≤ L (bs.map w01) :=
  dyn_store wok_w01 hv h ht (vx_w01 hv h ht).2 (row_hd ht) hloop hK hs

/-- UTF-16: a row below 145 storing `s ≤ units + d`. -/
theorem dyn_row16_145 {bs : List Nat} (hv : Valid8 bs) {K d pos t o s : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52)
    (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) (hK : d ≤ K + 1)
    (hi : bigIdx[(row (bs.map w16) t).m]! < 145)
    (hs : s ≤ units bigIdx[(row (bs.map w16) t).m]! + d) : o + s ≤ L (bs.map w16) :=
  dyn_store wok_w16 hv h ht (vx_utf16_145 hv h ht hi) (row_hd ht) hloop hK hs

/-- UTF-16: a three-code-point row, whose output (one or two units per lane,
by the pair test) is exact; its store is at most the output plus `d`. -/
theorem dyn_row16_3 (arm : Bool) {bs : List Nat} (hv : Valid8 bs) {K d pos t o s : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52)
    (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) (hK : d ≤ K + 1)
    (hi1 : 145 ≤ bigIdx[(row (bs.map w16) t).m]!) (hi2 : bigIdx[(row (bs.map w16) t).m]! < 209)
    (hs : s ≤ (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 0) then 2 else 1) +
       (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 1) then 2 else 1) +
       (if pairTest arm (perm bs t bigIdx[(row (bs.map w16) t).m]! 2) then 2 else 1) + d) :
    o + s ≤ L (bs.map w16) :=
  dyn_store wok_w16 hv h ht (vx_utf16_3 arm hv h ht hi1 hi2) (row_hd ht) hloop hK hs

/-- arm64 UTF-16, the `0x888` path: on valid input its 6 units are its output
(`vx_888`), so they fit with any `K`. -/
theorem dyn_888 {bs : List Nat} (hv : Valid8 bs) {K pos t o : Nat}
    (h : VX w16 bs true pos t o) (ht : t < pos + 52)
    (hloop : pos + 64 + safetyMargin K bs ≤ bs.length) (hmask : emask (bs.map w16) t 12 = 0x888) :
    o + 6 ≤ L (bs.map w16) :=
  dyn_store (d := 0) wok_w16 hv h ht (vx_888 hv h ht hmask).2 (by omega) hloop (by omega) (by omega)

/-- The stores alone need `K = d - 1`: 3 for deficit 4, 1 for deficit 2.
The loads need 2 (`dyn_load`). So the header values are `max (d - 1) 2`. -/
theorem k_values :
    (∀ a, k16 a = max (d16 a - 1) 2) ∧ (∀ a, k32 a = max (d32 a - 1) 2) := by
  refine ⟨fun a => ?_, fun a => ?_⟩ <;> cases a <;> decide


/-! ## One less fails, on inputs of the contract

The calls of each kernel are derived through `VX` up to byte 51, as in
`ValidLatin1.lv_tight_K2` and `Utf8.la_924_tight`; the dynamic test is computed
from the bytes with `safetyMargin`. `ValidHybrid` shows which of these the
loop of the headers reaches. -/

/-- `VX` sees the weights only through `bs.map w`. -/
theorem vx_congr {w w' : Nat → Nat} {bs : List Nat} (hm : bs.map w = bs.map w') :
    ∀ b pos t o, VX w bs b pos t o → VX w' bs b pos t o := by
  intro b pos t o h
  induction h with
  | start => exact VX.start
  | ascii t o h hlen hascii ih => exact VX.ascii t o ih hlen hascii
  | enter t o h hlen ih => exact VX.enter t o ih hlen
  | call pos t o c p h ht hs ih => exact VX.call pos t o c p ih ht (hm ▸ hs)
  | leave pos t o h ht ih => exact VX.leave pos t o ih ht

/-- The calls of the westmere, haswell, ppc64 (UTF-16 and UTF-32) and arm64
(UTF-32) kernels on `lvTight`, up to byte 51, in the loop model: rows of six
ASCII bytes at 0, 6, 12, 18 (masks `0xFFF`, `0xABF`; these kernels take their
`0xfff` path only when the whole remaining mask is `0xfff`), then the rows of
masks `0xAAA` at 24 (12 bytes; arm64 UTF-32 takes its `0xaaa` path, also 12),
`0xD7E` at 36 (7 bytes) and `0xFFA` at 43 (8 bytes), 6 units each
(`TableFacts.lv_rows`). The weights are the same under both length
functions (no 4-byte character). -/
theorem lv_calls :
    lvTight.map w16 = lvTight.map w01 ∧ VX w01 lvTight true 0 51 42 ∧
      VX w16 lvTight true 0 51 42 := by
  have hm : lvTight.map w16 = lvTight.map w01 := by decide
  have h1 : VX w01 lvTight true 0 0 0 := VX.enter 0 0 VX.start (by decide)
  have h2 : VX w01 lvTight true 0 6 6 := VX.call 0 0 0 6 6 h1 (by decide) (by unfold XStep; decide)
  have h3 : VX w01 lvTight true 0 12 12 := VX.call 0 6 6 6 6 h2 (by decide) (by unfold XStep; decide)
  have h4 : VX w01 lvTight true 0 18 18 := VX.call 0 12 12 6 6 h3 (by decide) (by unfold XStep; decide)
  have h5 : VX w01 lvTight true 0 24 24 := VX.call 0 18 18 6 6 h4 (by decide) (by unfold XStep; decide)
  have h6 : VX w01 lvTight true 0 36 30 := VX.call 0 24 24 12 6 h5 (by decide) (by unfold XStep; decide)
  have h7 : VX w01 lvTight true 0 43 36 := VX.call 0 36 30 7 6 h6 (by decide) (by unfold XStep; decide)
  have h8 : VX w01 lvTight true 0 51 42 := VX.call 0 43 36 8 6 h7 (by decide) (by unfold XStep; decide)
  exact ⟨hm, h8, vx_congr hm.symm _ _ _ _ h8⟩

/-- **UTF-16 on westmere, haswell and ppc64: `K = 2` overflows on valid
input.** `lvTight` (hex: `41`×24, `C3 A9`×7, `41`×5, `C3 A9`×2, `41`×16,
`C3 A9 41 41`; 67 bytes) is valid UTF-8. The dynamic test admits the block at 0
with `K = 2` and rejects it with `K = 3`. The kernels
(src/westmere/sse_convert_utf8_to_utf16.cpp, src/haswell/avx2_convert_utf8_to_utf16.cpp,
src/ppc64/ppc64_convert_utf8_to_utf16.cpp) reach byte 51 with 42 units out
(`lv_calls`). At 51 the 12 bits of the mask are `0xFFF` and are the last of
the block, so the whole mask is `0xfff`: the kernel stores 16 units for 12
(src/westmere/sse_convert_utf8_to_utf16.cpp:27-40), a call of `DStore` with
deficit 4, and the store ends at 58, one past `L = 57`. -/
theorem utf16_x86_K2 (tail : List Nat → Nat) :
    let bs := lvTight
    let xs := bs.map w16
    Valid8 bs ∧ bs.length = 67 ∧
      emask xs 0 12 = 0xFFF ∧ emask xs 6 12 = 0xFFF ∧ emask xs 12 12 = 0xFFF ∧
      emask xs 18 12 = 0xABF ∧ emask xs 24 12 = 0xAAA ∧ emask xs 36 12 = 0xD7E ∧
      emask xs 43 12 = 0xFFA ∧ emask xs 51 12 = 0xFFF ∧
      VX w16 bs true 0 51 42 ∧ 42 = L (xs.take 51) ∧
      0 + 64 + safetyMargin 2 bs ≤ bs.length ∧ ¬ (0 + 64 + safetyMargin 3 bs ≤ bs.length) ∧
      XStep xs 51 12 12 ∧ DStore w16 2 4 tail bs (L xs + 1) := by
  intro bs xs
  have hx : XStep xs 51 12 12 := by unfold XStep; decide
  have hv := lv_calls.2.2
  have hL : 42 + 16 = L xs + 1 := by decide
  refine ⟨by unfold Valid8; decide +kernel, by decide, by decide, by decide, by decide, by decide,
    by decide, by decide, by decide, by decide, hv, by decide, by decide, by decide, hx, ?_⟩
  rw [← hL]
  exact DStore.call 0 51 42 12 12 16 hv (by decide) hx (by decide) (by decide) (by decide)

/-- **UTF-32 on westmere, haswell, ppc64 and arm64: `K = 2` overflows on
valid input**, on the same `lvTight` and the same calls
(src/westmere/sse_convert_utf8_to_utf32.cpp, src/haswell/avx2_convert_utf8_to_utf32.cpp,
src/ppc64/ppc64_convert_utf8_to_utf32.cpp, src/arm64/arm_convert_utf8_to_utf32.cpp).
At 51 the whole mask is `0xfff`, and the kernel stores 16 units for 12
(src/westmere/sse_convert_utf8_to_utf32.cpp:24-35,
src/arm64/arm_convert_utf8_to_utf32.cpp:23-30), ending one past `L = 57`. -/
theorem utf32_x86_K2 (tail : List Nat → Nat) :
    let bs := lvTight
    let xs := bs.map w01
    Valid8 bs ∧ emask xs 51 12 = 0xFFF ∧
      VX w01 bs true 0 51 42 ∧ 42 = L (xs.take 51) ∧
      0 + 64 + safetyMargin 2 bs ≤ bs.length ∧ ¬ (0 + 64 + safetyMargin 3 bs ≤ bs.length) ∧
      XStep xs 51 12 12 ∧ DStore w01 2 4 tail bs (L xs + 1) := by
  intro bs xs
  have hx : XStep xs 51 12 12 := by unfold XStep; decide
  have hv := lv_calls.2.1
  have hL : 42 + 16 = L xs + 1 := by decide
  refine ⟨by unfold Valid8; decide +kernel, by decide, hv, by decide, by decide, by decide, hx, ?_⟩
  rw [← hL]
  exact DStore.call 0 51 42 12 12 16 hv (by decide) hx (by decide) (by decide) (by decide)

/-- 32 ASCII bytes, four U+3042 (`E3 81 82`), two `é`, one ASCII byte, one
`é`, five U+3042, two ASCII bytes: 68 bytes of valid UTF-8 whose
`utf16_length_from_utf8` weights are `Utf8.la924`. -/
def la924b : List Nat :=
  List.replicate 32 0x41 ++ (List.replicate 4 [0xE3, 0x81, 0x82]).flatten ++
    (List.replicate 2 [0xC3, 0xA9]).flatten ++ [0x41] ++ [0xC3, 0xA9] ++
    (List.replicate 5 [0xE3, 0x81, 0x82]).flatten ++ [0x41, 0x41]

/-- **UTF-16 on lsx and lasx: `K = 2` overflows on valid input.** The dynamic
test admits the block at 0 with `K = 2` and rejects it with `K = 3`. The
calls of src/lsx/lsx_convert_utf8_to_utf16.cpp and
src/lasx/lasx_convert_utf8_to_utf16.cpp: 16 ASCII bytes at 0 and 16
(`(mask & 0xFFFF) == 0xFFFF`), `0x924` at 32 (12 bytes, 4 units), the row
of mask `0x25A` at 44 (7 bytes, 4 units, `TableFacts.la_924_rows`), so 40
units at byte 51. There the 12-bit mask is `0x924`
(src/lsx/lsx_convert_utf8_to_utf16.cpp:36-47): 8 units stored for 4, ending
at 48, one past `L = 47`. -/
theorem utf16_la_K2 (tail : List Nat → Nat) :
    let bs := la924b
    let xs := bs.map w16
    Valid8 bs ∧ bs.length = 68 ∧ xs = la924 ∧
      emask xs 0 16 = 0xFFFF ∧ emask xs 16 16 = 0xFFFF ∧ emask xs 32 12 = 0x924 ∧
      emask xs 44 12 = 0x25A ∧ emask xs 51 12 = 0x924 ∧
      VX w16 bs true 0 51 40 ∧ 40 = L (xs.take 51) ∧
      0 + 64 + safetyMargin 2 bs ≤ bs.length ∧ ¬ (0 + 64 + safetyMargin 3 bs ≤ bs.length) ∧
      XStep xs 51 12 4 ∧ DStore w16 2 4 tail bs (L xs + 1) := by
  intro bs xs
  have hx : XStep xs 51 12 4 := by unfold XStep; decide
  have h1 : VX w16 la924b true 0 0 0 := VX.enter 0 0 VX.start (by decide)
  have h2 : VX w16 la924b true 0 16 16 :=
    VX.call 0 0 0 16 16 h1 (by decide) (by unfold XStep; decide)
  have h3 : VX w16 la924b true 0 32 32 :=
    VX.call 0 16 16 16 16 h2 (by decide) (by unfold XStep; decide)
  have h4 : VX w16 la924b true 0 44 36 :=
    VX.call 0 32 32 12 4 h3 (by decide) (by unfold XStep; decide)
  have hv : VX w16 la924b true 0 51 40 :=
    VX.call 0 44 36 7 4 h4 (by decide) (by unfold XStep; decide)
  have hL : 40 + 8 = L xs + 1 := by decide
  refine ⟨by unfold Valid8; decide +kernel, by decide, by decide, by decide, by decide, by decide,
    by decide, by decide, hv, by decide, by decide, by decide, hx, ?_⟩
  rw [← hL]
  exact DStore.call 0 51 40 12 4 8 hv (by decide) hx (by decide) (by decide) (by decide)

/-- **The kernels of arm64 UTF-16 and LoongArch UTF-32: `K = 1` reads past
the input** (a bound on the dynamic test: these targets have no dynamic block
in the headers, where the fixed margins 3 and 4 decide;
`ValidHybrid.fix16_arm_read`).
`ValidLatin1.laRead` (hex: `41`×32, `C3 A9`×7, `41`×20; 66 bytes) is valid
UTF-8. The dynamic test admits the block at 0 with `K = 1` and rejects it with
`K = 2`. The calls of src/arm64/arm_convert_utf8_to_utf16.cpp and
src/lsx/lsx_convert_utf8_to_utf32.cpp, src/lasx/lasx_convert_utf8_to_utf32.cpp:
16 ASCII bytes at 0 and 16 (`(mask & 0xFFFF) == 0xffff`), the `0xaaa` path
at 32 (12 bytes, 6 units), the row of mask `0xFFE` at 44 (7 bytes, 6 units,
`TableFacts.lv_rows`). At 51, below `pos + 52`, the inner loop calls again,
and the call begins with a 16-byte load of bytes 51 to 66
(src/arm64/arm_convert_utf8_to_utf16.cpp:13, src/lsx/lsx_convert_utf8_to_utf32.cpp:13):
byte 66 is past the input. The stores alone would be safe with `K = 1`
(`dyn_store`, `d = 2`). -/
theorem read_K1 :
    let bs := laRead
    Valid8 bs ∧ bs.length = 66 ∧
      emask (bs.map w16) 0 16 = 0xFFFF ∧ emask (bs.map w16) 16 16 = 0xFFFF ∧
      emask (bs.map w16) 32 12 = 0xAAA ∧ emask (bs.map w16) 44 12 = 0xFFE ∧
      bs.map w16 = bs.map w01 ∧
      VX w16 bs true 0 51 44 ∧ VX w01 bs true 0 51 44 ∧
      0 + 64 + safetyMargin 1 bs ≤ bs.length ∧ ¬ (0 + 64 + safetyMargin 2 bs ≤ bs.length) ∧
      DLoad 1 bs (bs.length + 1) := by
  intro bs
  have hm : laRead.map w16 = laRead.map w01 := by decide
  have h1 : VX w01 laRead true 0 0 0 := VX.enter 0 0 VX.start (by decide)
  have h2 : VX w01 laRead true 0 16 16 :=
    VX.call 0 0 0 16 16 h1 (by decide) (by unfold XStep; decide)
  have h3 : VX w01 laRead true 0 32 32 :=
    VX.call 0 16 16 16 16 h2 (by decide) (by unfold XStep; decide)
  have h4 : VX w01 laRead true 0 44 38 :=
    VX.call 0 32 32 12 6 h3 (by decide) (by unfold XStep; decide)
  have hv : VX w01 laRead true 0 51 44 :=
    VX.call 0 44 38 7 6 h4 (by decide) (by unfold XStep; decide)
  have hl : bs.length + 1 = 51 + 16 := by decide
  refine ⟨by unfold Valid8; decide +kernel, by decide, by decide, by decide, by decide, by decide,
    hm, vx_congr hm.symm _ _ _ _ hv, hv, by decide, by decide, ?_⟩
  rw [hl]
  exact DLoad.call 0 51 (by decide) (by decide)

end SafetyMargins.ValidDynamic
