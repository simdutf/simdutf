/-
The `utf16fix` kernels (`to_well_formed_utf16le/be`): every load and every
store lies in `[0, n)`.

`n` is the input length; the output also holds `n` units, and in place
(`in == out`) is the same buffer. Positions are counted in `char16_t` units
from the start of the buffer and are integers, so that a lookback such as
`in[-1]` or `out[-1]` of a block at position `p` is the access `p - 1`, and a
negative position is a real out-of-bounds access.

Each kernel is modelled by the list of every access it can make for a given
`n`: the scalar head (`out[0]`, `in[0]`), the blocks at the positions the loop
`for (i = 1; i + W < n; i += W)` takes, the final overlapping block at
`n - W`, and the scalar tail (`out[n - 1]`, read then written). A block at
`p` of width `W` loads `in[p - 1, p - 1 + W)` (lookback) and `in[p, p + W)`,
reads and rewrites `out[p - 1]`, and stores `out[p, p + W)`. The stores that
the C++ does only on some branch (the `out[-1]` rewrite on an ill-sequenced
block, the block store in place) are all listed: the list over-approximates
every run. Under the in-place branch, which skips the clean-block store, the
accesses are a subset of those listed.

What is read from the C++ and not proved: the loop bounds, the widths, the
offsets of the accesses (as cited per kernel), that the scalar fallback
writes `output[i - 1]` only for `1 ≤ i` (its `high_surrogate_prev` is false at
`i = 0`), that the arm64 fix-up index `r` from `trailing_zeroes` of a nonzero
64-bit mask is below 64, and that AVX-512 masked loads and stores do not touch
masked-off lanes (only the active lanes are listed for `utf16fix_short` and
the `out - 1` masked store).

Main results: `westmere_inBounds`, `haswell_inBounds`, `icelake_inBounds`,
`arm64_inBounds`, `generic_inBounds` (lsx `N = 8`, lasx `N = 16`),
`rvv_inBounds` (any `VL ≥ 1`), `scalar_inBounds` (fallback, ppc64), each for
every `n`.
-/

namespace SafetyMargins.Utf16Fix

/-- Which buffer an access touches (both hold `n` units). -/
inductive Buf where
  | inp
  | out
deriving Repr, DecidableEq

/-- An access to `w` consecutive units of `buf`, starting at unit `lo`. -/
structure Acc where
  buf : Buf
  lo : Int
  w : Nat
deriving Repr, DecidableEq

/-- The access lies in a buffer of `n` units. -/
def Acc.InB (n : Nat) (a : Acc) : Prop := 0 ≤ a.lo ∧ a.lo + a.w ≤ n

/-- Every access in the list lies in `[0, n)`. -/
def AllInB (n : Nat) (as : List Acc) : Prop := ∀ a ∈ as, a.InB n

theorem allInB_append {n : Nat} {as bs : List Acc} :
    AllInB n as → AllInB n bs → AllInB n (as ++ bs) := by
  intro ha hb a h
  rcases List.mem_append.mp h with h | h
  · exact ha a h
  · exact hb a h

theorem allInB_flatMap {α : Type} {n : Nat} {xs : List α} {f : α → List Acc} :
    (∀ x ∈ xs, AllInB n (f x)) → AllInB n (xs.flatMap f) := by
  intro h a ha
  obtain ⟨x, hx, hax⟩ := List.mem_flatMap.mp ha
  exact h x hx a hax

/-! ## The loop -/

/-- Block positions of `for (i = i0; i + W < n; i += W)`. The `0 < W` conjunct
only serves termination; every kernel has `W ≥ 8`. -/
def loopStarts (W n i : Nat) : List Nat :=
  if _h : i + W < n ∧ 0 < W then i :: loopStarts W n (i + W) else []
termination_by n - i
decreasing_by omega

/-- The value of `i` when that loop exits. -/
def loopEnd (W n i : Nat) : Nat :=
  if _h : i + W < n ∧ 0 < W then loopEnd W n (i + W) else i
termination_by n - i
decreasing_by omega

/-- Every loop position is at least the start and its block ends before `n`
(strictly: the loop test is `i + W < n`). -/
theorem mem_loopStarts {W n i p : Nat} (h : p ∈ loopStarts W n i) :
    i ≤ p ∧ p + W < n := by
  induction i using loopStarts.induct W n with
  | case1 i hc ih =>
    rw [loopStarts, dite_eq_left_of_eq_true (eq_true hc)] at h
    rcases List.mem_cons.mp h with h | h
    · omega
    · have := ih h; omega
  | case2 i hc =>
    rw [loopStarts, dite_eq_right_of_eq_false (eq_false hc)] at h
    simp at h

theorem le_loopEnd (W n i : Nat) : i ≤ loopEnd W n i := by
  induction i using loopEnd.induct W n with
  | case1 i hc ih => rw [loopEnd, dite_eq_left_of_eq_true (eq_true hc)]; omega
  | case2 i hc => rw [loopEnd, dite_eq_right_of_eq_false (eq_false hc)]; omega

/-! ## Shared pieces -/

/-- `out[0] = is_low_surrogate(in[0]) ? replacement : in[0]`. -/
def headAcc : List Acc := [⟨.inp, 0, 1⟩, ⟨.out, 0, 1⟩]

/-- `out[n - 1] = is_high_surrogate(out[n - 1]) ? replacement : out[n - 1]`
(read, then write). -/
def tailAcc (n : Nat) : List Acc := [⟨.out, (n : Int) - 1, 1⟩]

/-- One `utf16fix_block` of width `W` at `out + p`, `in + p`: the lookback
load at `in - 1`, the block load at `in`, the read and rewrite of `out[-1]`,
the block store at `out`. -/
def blockAcc (W : Nat) (p : Int) : List Acc :=
  [⟨.inp, p - 1, W⟩, ⟨.inp, p, W⟩, ⟨.out, p - 1, 1⟩, ⟨.out, p, W⟩]

theorem headAcc_inB {n : Nat} (h : 1 ≤ n) : AllInB n headAcc := by
  intro a ha
  simp [headAcc] at ha
  rcases ha with rfl | rfl <;> simp [Acc.InB] <;> omega

theorem tailAcc_inB {n : Nat} (h : 1 ≤ n) : AllInB n (tailAcc n) := by
  intro a ha
  simp [tailAcc] at ha
  subst ha
  simp [Acc.InB]; omega

/-- A block in `[1, n - W]` keeps all its accesses, `out[-1]` and `in[-1]`
included, in `[0, n)`: `p - 1 ≥ 0` and `p + W ≤ n`. -/
theorem blockAcc_inB {W n : Nat} {p : Int} (h1 : 1 ≤ p) (h2 : p + W ≤ n) :
    AllInB n (blockAcc W p) := by
  intro a ha
  simp [blockAcc] at ha
  rcases ha with rfl | rfl | rfl | rfl <;> simp [Acc.InB] <;> omega

/-- The loop blocks of `for (i = i0; i + W < n; i += W)` with `1 ≤ i0`. -/
theorem loopBlocks_inB {W n i0 : Nat} (h : 1 ≤ i0) :
    AllInB n ((loopStarts W n i0).flatMap fun (p : Nat) => blockAcc W p) := by
  apply allInB_flatMap
  intro p hp
  have := mem_loopStarts hp
  exact blockAcc_inB (by omega) (by omega)

/-- The final overlapping block at `n - W` ends exactly at `n`, and its
`out[-1]`, `in[-1]` is unit `n - W - 1 ≥ 0`, given `W + 1 ≤ n`. -/
theorem finalBlock_inB {W n : Nat} (h : W + 1 ≤ n) :
    AllInB n (blockAcc W ((n : Int) - W)) :=
  blockAcc_inB (by omega) (by omega)

theorem finalBlock_ends_at_n (W n : Nat) :
    ((n : Int) - W) + W = n := by omega

/-- The common shape `n > W`: head, loop from 1, final block at `n - W`,
tail. Used by westmere, haswell (both widths), icelake, lsx and lasx. -/
def fixAcc (W n : Nat) : List Acc :=
  headAcc ++ (loopStarts W n 1).flatMap (fun (p : Nat) => blockAcc W p) ++
    blockAcc W ((n : Int) - W) ++ tailAcc n

theorem fixAcc_inB {W n : Nat} (h : W + 1 ≤ n) : AllInB n (fixAcc W n) := by
  unfold fixAcc
  refine allInB_append (allInB_append (allInB_append ?_ ?_) ?_) ?_
  · exact headAcc_inB (by omega)
  · exact loopBlocks_inB (Nat.le_refl 1)
  · exact finalBlock_inB h
  · exact tailAcc_inB (by omega)

/-! ## Scalar fallback -/

/-- `scalar::utf16::to_well_formed_utf16`,
include/simdutf/scalar/utf16.h:189-214. Unit `i < len` reads `input[i]`,
writes `output[i]`, and, for `1 ≤ i`, may write `output[i - 1]` (line 199;
`high_surrogate_prev` is false at `i = 0`); after the loop, `output[len - 1]`
for `1 ≤ len` (line 212). Used for `n < 9` on westmere and haswell, `n < 17`
on arm64, `n < N + 1` on lsx and lasx, and always on fallback and ppc64
(src/ppc64/implementation.cpp:268-278). -/
def scalarAcc (n : Nat) : List Acc :=
  (List.range n).flatMap (fun i =>
    [⟨.inp, i, 1⟩, ⟨.out, i, 1⟩] ++
      (if 1 ≤ i then [⟨.out, (i : Int) - 1, 1⟩] else [])) ++
  (if 1 ≤ n then [⟨.out, (n : Int) - 1, 1⟩] else [])

theorem scalar_inBounds (n : Nat) : AllInB n (scalarAcc n) := by
  unfold scalarAcc
  refine allInB_append (allInB_flatMap ?_) ?_
  · intro i hi a ha
    have hi := List.mem_range.mp hi
    simp at ha
    rcases ha with rfl | rfl | ⟨h1, rfl⟩ <;> simp [Acc.InB] <;> omega
  · intro a ha
    split at ha
    · simp at ha; subst ha; simp [Acc.InB]; omega
    · simp at ha

/-! ## Westmere -/

/-- `utf16fix_sse`, src/westmere/sse_utf16fix.cpp:52-82: scalar for `n < 9`
(line 56); otherwise `out[0]` (line 61), blocks of 8 at `i = 1, 9, ...` while
`i + 8 < n` (lines 66, 72), the final block at `n - 8` (lines 70, 76), and
`out[n - 1]` (line 79). The block, lines 9-50, loads `in - 1` and `in`
(16 bytes each, lines 20-21), rewrites `out[-1]` (lines 40-41) and stores 8
units at `out` (lines 46, 48). -/
def westmereAcc (n : Nat) : List Acc :=
  if n < 9 then scalarAcc n else fixAcc 8 n

theorem westmere_inBounds (n : Nat) : AllInB n (westmereAcc n) := by
  unfold westmereAcc
  split
  · exact scalar_inBounds n
  · exact fixAcc_inB (by omega)

/-! ## Haswell -/

/-- `utf16fix_avx`, src/haswell/avx2_utf16fix.cpp:142-173: `utf16fix_sse`
for `n < 17` (line 147), which is lines 109-140, the westmere kernel above
with the same 8-unit block (lines 71-107) and scalar for `n < 9` (line 114);
otherwise `out[0]` (line 152), blocks of 16 at `i = 1, 17, ...` while
`i + 16 < n` (lines 157, 163), the final block at `n - 16` (lines 161, 167),
and `out[n - 1]` (line 170). The 16-unit block, lines 9-69, loads 32 bytes at
`in - 1` and `in` (lines 18-19), rewrites `out[-1]` (lines 59-60) and stores
at `out` (lines 65, 67). -/
def haswellAcc (n : Nat) : List Acc :=
  if n < 17 then westmereAcc n else fixAcc 16 n

theorem haswell_inBounds (n : Nat) : AllInB n (haswellAcc n) := by
  unfold haswellAcc
  split
  · exact westmere_inBounds n
  · exact fixAcc_inB (by omega)

/-! ## Ice Lake -/

/-- `utf16fix_short`, src/icelake/icelake_utf16fix.cpp:63-104, for
`1 ≤ n ≤ 32`: `mask` has the low `n` bits (line 68). The lookback load at
`in - 1` uses `mask << 1` (lines 72-73), so its active lanes are
`1 .. min n 31` (bit 32 of the shifted 32-bit mask is dropped when `n = 32`),
within `in[0, n)`, and `in[-1]` is not read; the model takes all of
`in[0, n)`, a safe over-approximation; the block load (line 74) and the store
(lines 94-99) use `mask`: `in[0, n)`, `out[0, n)`. Then `out[n - 1]`
(line 101). -/
def icelakeShortAcc (n : Nat) : List Acc :=
  [⟨.inp, 0, n⟩, ⟨.inp, 0, n⟩, ⟨.out, 0, n⟩] ++ tailAcc n

/-- `utf16fix_avx512`, src/icelake/icelake_utf16fix.cpp:106-138: nothing for
`n = 0` (line 111), `utf16fix_short` for `n < 33` (line 113); otherwise
`out[0]` (line 117), blocks of 32 at `i = 1, 33, ...` while `i + 32 < n`
(lines 122, 128), the final block at `n - 32` (lines 126, 132), and
`out[n - 1]` (line 135). The block, lines 11-57, loads 64 bytes at `in - 1`
and `in` (lines 20-21), writes `out[-1]` by a masked store at `out - 1` whose
mask keeps lane 0 only (lines 41-43), and stores 32 units at `out`, masked
or full (lines 47-55). -/
def icelakeAcc (n : Nat) : List Acc :=
  if n = 0 then [] else if n < 33 then icelakeShortAcc n else fixAcc 32 n

theorem icelake_inBounds (n : Nat) : AllInB n (icelakeAcc n) := by
  unfold icelakeAcc
  split
  · intro a ha; simp at ha
  · split
    · refine allInB_append ?_ (tailAcc_inB (by omega))
      intro a ha
      simp at ha
      rcases ha with rfl | rfl <;> simp [Acc.InB]
    · exact fixAcc_inB (by omega)

/-! ## arm64 -/

/-- `utf16fix_block64` at `p`, src/arm64/arm_utf16fix.cpp:104-141: four
`get_mismatch_copy` (lines 65-79) at `p + 16 k`, `k < 4`, each loading
16 units (`vld2q_u8`, 32 bytes) at `in - 1` and `in` and storing 16 units at
`out`; then for each set bit `r < 64` of the mask (lines 130-137) a read of
`in[r - 1]` and a write of `out[r - is_high]`, `is_high ∈ {0, 1}`. -/
def block64Acc (p : Int) : List Acc :=
  ((List.range 4).flatMap fun k =>
    [⟨.inp, p + 16 * k - 1, 16⟩, ⟨.inp, p + 16 * k, 16⟩,
      ⟨.out, p + 16 * k, 16⟩]) ++
  ((List.range 64).flatMap fun r =>
    [⟨.inp, p + r - 1, 1⟩, ⟨.out, p + r, 1⟩, ⟨.out, p + r - 1, 1⟩])

theorem block64Acc_inB {n : Nat} {p : Int} (h1 : 1 ≤ p) (h2 : p + 64 ≤ n) :
    AllInB n (block64Acc p) := by
  unfold block64Acc
  refine allInB_append (allInB_flatMap ?_) (allInB_flatMap ?_)
  · intro k hk a ha
    have hk := List.mem_range.mp hk
    simp at ha
    rcases ha with rfl | rfl | rfl <;> simp [Acc.InB] <;> omega
  · intro r hr a ha
    have hr := List.mem_range.mp hr
    simp at ha
    rcases ha with rfl | rfl | rfl <;> simp [Acc.InB] <;> omega

/-- `utf16fix_neon_64bits`, src/arm64/arm_utf16fix.cpp:143-179: scalar for
`n < 17` (line 147); otherwise `out[0]` (line 150), blocks of 64 at
`i = 1, 65, ...` while `i + 64 < n` (lines 156, 167), then, from that `i`,
blocks of 16 while `i + 16 < n` (lines 160, 170), the final 16-unit block at
`n - 16` (lines 165, 174), and `out[n - 1]` (line 176). The 16-unit block,
lines 17-63, loads at `in - 1` and `in` (lines 27-28), rewrites `out[-1]`
(lines 47-48) and stores 16 units at `out` (lines 59, 61). -/
def arm64Acc (n : Nat) : List Acc :=
  if n < 17 then scalarAcc n else
    headAcc ++ (loopStarts 64 n 1).flatMap (fun (p : Nat) => block64Acc p) ++
      (loopStarts 16 n (loopEnd 64 n 1)).flatMap (fun (p : Nat) => blockAcc 16 p) ++
      blockAcc 16 ((n : Int) - 16) ++ tailAcc n

theorem arm64_inBounds (n : Nat) : AllInB n (arm64Acc n) := by
  unfold arm64Acc
  split
  · exact scalar_inBounds n
  · refine allInB_append (allInB_append (allInB_append (allInB_append ?_ ?_) ?_) ?_) ?_
    · exact headAcc_inB (by omega)
    · apply allInB_flatMap
      intro p hp
      have := mem_loopStarts hp
      exact block64Acc_inB (by omega) (by omega)
    · exact loopBlocks_inB (by have := le_loopEnd 64 n 1; omega)
    · exact finalBlock_inB (by omega)
    · exact tailAcc_inB (by omega)

/-! ## lsx and lasx -/

/-- `utf16::to_well_formed<N>`, src/generic/utf16/to_well_formed.h:53-88,
with `N = vector_u16::ELEMENTS`: 8 on lsx (16-byte `__m128i`,
src/simdutf/lsx/simd16-inl.h:4-6; used at src/lsx/implementation.cpp:385-393)
and 16 on lasx (32-byte `__m256i`, src/simdutf/lasx/simd16-inl.h:15-18; used
at src/lasx/implementation.cpp:385-393). Scalar for `n < N + 1` (line 58);
otherwise `out[0]` (line 65), blocks of `N` at `i = 1, N + 1, ...` while
`i + N < n` (lines 71, 78), the final block at `n - N` (lines 75, 82), and
`out[n - 1]` (line 85). The block, lines 16-51, loads at `in - 1` and `in`
(lines 25-26), rewrites `out[-1]` (line 42) and stores `N` units at `out`
(lines 47, 49). -/
def genericAcc (N n : Nat) : List Acc :=
  if n < N + 1 then scalarAcc n else fixAcc N n

theorem generic_inBounds (N n : Nat) : AllInB n (genericAcc N n) := by
  unfold genericAcc
  split
  · exact scalar_inBounds n
  · exact fixAcc_inB (by omega)

theorem lsx_inBounds (n : Nat) : AllInB n (genericAcc 8 n) := generic_inBounds 8 n
theorem lasx_inBounds (n : Nat) : AllInB n (genericAcc 16 n) := generic_inBounds 16 n

/-! ## RISC-V Vector -/

/-- One `utf16fix_block_rvv` of `vl` units at `o`,
src/rvv/rvv_utf16fix.cpp:1-51: the block load (line 5), the scalar read of
`in[-1]` (line 6; the lookback is a slide, not a load at `in - 1`), the read
and rewrite of `out[-1]` (lines 41-42), the store of `vl` units (lines 47,
49). -/
def rvvBlockAcc (vl : Nat) (o : Int) : List Acc :=
  [⟨.inp, o, vl⟩, ⟨.inp, o - 1, 1⟩, ⟨.out, o - 1, 1⟩, ⟨.out, o, vl⟩]

/-- The loop of `rvv_to_well_formed_utf16` from offset `o` with `m` units
left: blocks of `VL` while `m > VL` (lines 68, 73), then the last block of
the `m` remaining units (lines 71, 76), then `out[m - 1]` relative to the
advanced `out` (line 79). The `0 < VL` conjunct only serves termination
(`__riscv_vsetvlmax_e16m8() ≥ 1`). -/
def rvvLoopAcc (VL : Nat) (o m : Nat) : List Acc :=
  if h : m > VL ∧ 0 < VL then rvvBlockAcc VL o ++ rvvLoopAcc VL (o + VL) (m - VL)
  else rvvBlockAcc m o ++ [⟨.out, (o : Int) + m - 1, 1⟩]
termination_by m
decreasing_by omega

/-- `rvv_to_well_formed_utf16`, src/rvv/rvv_utf16fix.cpp:53-82: nothing for
`n = 0` (line 57); otherwise `out[0]` (line 60), then `n - 1` units from
offset 1 (lines 62-64). The last block is not overlapping: it has the
`m ≤ VL` remaining units; for `n = 1` it has `vl = 0`, still reads
`in[-1] = in[0]`, and line 79 rewrites `out[0]`. -/
def rvvAcc (VL n : Nat) : List Acc :=
  if n = 0 then [] else headAcc ++ rvvLoopAcc VL 1 (n - 1)

theorem rvvLoopAcc_inB (VL n : Nat) :
    ∀ o m, 1 ≤ o → o + m = n → AllInB n (rvvLoopAcc VL o m) := by
  intro o m
  induction o, m using rvvLoopAcc.induct VL with
  | case1 o m hc ih =>
    intro h1 h2
    rw [rvvLoopAcc, dite_eq_left_of_eq_true (eq_true hc)]
    refine allInB_append ?_ (ih (by omega) (by omega))
    intro a ha
    simp [rvvBlockAcc] at ha
    rcases ha with rfl | rfl | rfl | rfl <;> simp [Acc.InB] <;> omega
  | case2 o m hc =>
    intro h1 h2
    rw [rvvLoopAcc, dite_eq_right_of_eq_false (eq_false hc)]
    intro a ha
    simp [rvvBlockAcc] at ha
    rcases ha with rfl | rfl | rfl | rfl | rfl <;> simp [Acc.InB] <;> omega

theorem rvv_inBounds (VL n : Nat) : AllInB n (rvvAcc VL n) := by
  unfold rvvAcc
  split
  · intro a ha; simp at ha
  · exact allInB_append (headAcc_inB (by omega))
      (rvvLoopAcc_inB VL n 1 (n - 1) (Nat.le_refl 1) (by omega))

/-! ## The final block and the lookback, stated directly -/

/-- For every kernel with a final overlapping block of width `W`
(`W = 8, 16, 32`; lsx 8, lasx 16, arm64 16), the block runs only when
`W + 1 ≤ n`, starts at `n - W ≥ 1`, ends at `n`, and its `out[-1]` and
`in[-1]` are unit `n - W - 1 ≥ 0`. -/
theorem final_block_window {W n : Nat} (h : W + 1 ≤ n) :
    0 ≤ (n : Int) - W - 1 ∧ (n : Int) - W + W = n := by omega

/-- Every loop block at `i` has its `out[-1]`, `in[-1]` at `i - 1 ≥ 0`
and ends at `i + W < n`. -/
theorem loop_block_window {W n p : Nat} (h : p ∈ loopStarts W n 1) :
    0 ≤ (p : Int) - 1 ∧ p + W < n := by
  have := mem_loopStarts h; omega

end SafetyMargins.Utf16Fix
