import SafetyMargins.Arch.Arm64
import SafetyMargins.Arch.Haswell
import SafetyMargins.Arch.Icelake
import SafetyMargins.Arch.Lasx
import SafetyMargins.Arch.Lsx
import SafetyMargins.Arch.Westmere

/-!
# Input loads of the fixed-margin loops whose margin was lowered

The store theorems (`ok_*`, `cur_*` in `Arch/*.lean`) bound what a loop writes.
This file bounds what the same loops read. For each loop, one iteration's
vector loads are listed as `(offset, width)` in input units, relative to the
cursor at the top of the iteration, taking every branch. A load fits when
`offset + width` is at most the units the loop test guarantees, which is the
same `need B m strict` as the store theorem of that loop; each `*_tied`
theorem states the store theorem and the load bound together.

A loop with a scalar fallback reads `buf[k]` for `k < forward`, where
`forward = cap`, lowered to `end - buf - 1` when `end - buf < cap + 1`
(`fwd cap r` below, with `r = end - buf`).

Lookup-table loads (`row + 1`, ...) are not input reads and are not listed. Every load below is an unmasked full-width load;
templated big/little-endian variants share the loop body.
-/

namespace SafetyMargins.Loads
open SafetyMargins

/-- The loads of one iteration of a loop with test `end - buf ≥ B + m`
(or `> B + m` when `strict`). -/
structure LoopLoads where
  B : Nat
  m : Nat
  strict : Bool
  /-- `(offset, width)` in input units, every branch. -/
  loads : List (Nat × Nat)
  /-- `forward` cap of the scalar fallback inside the loop, if any. -/
  scalarCap : Option Nat := none
deriving Repr

/-- Every load ends within `N` units. -/
def LoopLoads.fits (l : LoopLoads) (N : Nat) : Bool :=
  l.loads.all fun p => decide (p.1 + p.2 ≤ N)

/-- Furthest unit read by the vector loads. -/
def LoopLoads.reach (l : LoopLoads) : Nat :=
  l.loads.foldl (fun a p => max a (p.1 + p.2)) 0

/-- Units the loop test guarantees. -/
def LoopLoads.guaranteed (l : LoopLoads) : Nat := need l.B l.m l.strict

theorem fits_spec (l : LoopLoads) (N : Nat) (h : l.fits N = true) :
    ∀ n, N ≤ n → ∀ p ∈ l.loads, p.1 + p.2 ≤ n := by
  intro n hn p hp
  have := of_decide_eq_true ((List.all_eq_true.mp h) p hp)
  omega

/-- `forward` of the scalar fallback, `r = end - buf`. -/
def fwd (cap r : Nat) : Nat := if r < cap + 1 then r - 1 else cap

/-- The scalar fallback reads `buf[k]`, `k < forward`, inside the input
whenever at least one unit remains (which every loop test below implies). -/
theorem scalar_reads_in (cap r k : Nat) (hr : 1 ≤ r) (hk : k < fwd cap r) : k < r := by
  unfold fwd at hk
  split at hk <;> omega

/-- Inside the loop (`cap < need`), `forward` is the cap itself. -/
theorem fwd_eq_cap (cap N r : Nat) (hc : cap < N) (hr : N ≤ r) : fwd cap r = cap := by
  unfold fwd
  split <;> omega

/-! ## UTF-32 → UTF-8 -/

/-- src/arm64/arm_convert_utf32_to_utf8.cpp:11 (plain) and :260
(`_with_errors`), `while (buf + 16 + safety_margin < end)`, margin 3:
`vld1q_u32(buf)` and `vld1q_u32(buf + 4)` (:12-13, :261-262); scalar fallback
`forward = 15` (:208-213, :462-467). -/
def arm64_utf32to8 : LoopLoads :=
  { B := 16, m := 3, strict := true, loads := [(0, 4), (4, 4)], scalarCap := some 15 }

/-- src/lsx/lsx_convert_utf32_to_utf8.cpp:14 (plain) and :241
(`_with_errors`), `end - buf > 16 + safety_margin`, margin 3:
`__lsx_vld(buf, 0)` and `__lsx_vld(buf, 16)` (bytes, so units 0 and 4)
(:15-16, :242-243); scalar fallback `forward = 15` (:184-189, :416-421). -/
def lsx_utf32to8 : LoopLoads :=
  { B := 16, m := 3, strict := true, loads := [(0, 4), (4, 4)], scalarCap := some 15 }

/-- src/lasx/lasx_convert_utf32_to_utf8.cpp:43 (plain) and :336
(`_with_errors`), `end - buf > 16 + safety_margin`, margin 11:
`__lasx_xvld(buf, 0)` and `__lasx_xvld(buf, 32)` (bytes, so units 0 and 8)
(:44-45, :337-338); scalar fallback `forward = 15` (:249-254, :546-551).
The scalar alignment prologue (:7, :299) tests `buf < end` per unit. -/
def lasx_utf32to8 : LoopLoads :=
  { B := 16, m := 11, strict := true, loads := [(0, 8), (8, 8)], scalarCap := some 15 }

/-- src/westmere/sse_convert_utf32_to_utf8.cpp:345, `_with_errors`,
`end - buf >= 16 + safety_margin`, margin 4: `_mm_loadu_si128(buf)` and
`(__m128i *)buf + 1` (:347-348); scalar fallback `forward = 15` (:550-555). -/
def westmere_utf32to8_err : LoopLoads :=
  { B := 16, m := 4, strict := false, loads := [(0, 4), (4, 4)], scalarCap := some 15 }

/-- src/westmere/sse_convert_utf32_to_utf8.cpp:20, plain function (margin 12,
not lowered; listed as the companion of the `_with_errors` loop):
`(__m128i *)buf + 0, 1` (:26-28) and, when the first 8 are ASCII,
`+ 2, 3` (:59-60); scalar fallback `forward = 15` (:282-287). -/
def westmere_utf32to8 : LoopLoads :=
  { B := 16, m := 12, strict := false,
    loads := [(0, 4), (4, 4), (8, 4), (12, 4)], scalarCap := some 15 }

/-! ## UTF-32 → UTF-16 -/

/-- src/westmere/sse_convert_utf32_to_utf16.cpp:66, plain (template on
endianness), `end - buf >= 16 + 4`: `_mm_loadu_si128(ptr + 0..3)` (:68-71).
No scalar path. -/
def westmere_utf32to16 : LoopLoads :=
  { B := 16, m := 4, strict := false, loads := [(0, 4), (4, 4), (8, 4), (12, 4)] }

/-- src/westmere/sse_convert_utf32_to_utf16.cpp:141, `_with_errors`
(margin 0, companion), `end - buf >= 8`: `(__m128i *)buf + 0, 1` (:142-143);
scalar fallback `forward = 7` (:169-174). -/
def westmere_utf32to16_err : LoopLoads :=
  { B := 8, m := 0, strict := false, loads := [(0, 4), (4, 4)], scalarCap := some 7 }

/-- src/haswell/avx2_convert_utf32_to_utf16.cpp:14 (plain) and :93
(`_with_errors`), both templated on endianness, `end - buf >= 8`:
`_mm256_loadu_si256(buf)` (:15, :94); scalar fallback `forward = 7`
(:35-40, :118-123). -/
def haswell_utf32to16 : LoopLoads :=
  { B := 8, m := 0, strict := false, loads := [(0, 8)], scalarCap := some 7 }

/-! ## Latin-1 → UTF-8 -/

/-- src/haswell/avx2_convert_latin1_to_utf8.cpp:10,
`end - latin1_input >= 16 + safety_margin`, margin 8:
`_mm_loadu_si128(latin1_input)` (:11). No scalar path. -/
def haswell_latin1to8 : LoopLoads :=
  { B := 16, m := 8, strict := false, loads := [(0, 16)] }

/-- src/lasx/lasx_convert_latin1_to_utf8.cpp:13,
`end - latin1_input >= 16 + safety_margin`, margin 8:
`__lsx_vld(latin1_input, 0)` (:14). No scalar path. -/
def lasx_latin1to8 : LoopLoads :=
  { B := 16, m := 8, strict := false, loads := [(0, 16)] }

/-- src/arm64/arm_convert_latin1_to_utf8.cpp:12, `end - latin1_input >= 16`,
margin 0: `vld1q_u8(latin1_input)` (:13). The non-ASCII path consumes 8 but
loads nothing more. No scalar path. -/
def arm64_latin1to8 : LoopLoads :=
  { B := 16, m := 0, strict := false, loads := [(0, 16)] }

/-- src/icelake/icelake_convert_latin1_to_utf8.inl.cpp:91,
`pos + 96 <= len` (`B = 64`, margin 32): `_mm512_loadu_si512(buf + pos)`
(:92). After the loop, `pos + 64 <= len` guards the unmasked 64-byte load
(:96-97), and the last load is masked to `len - pos` (:103-104). -/
def icelake_latin1to8 : LoopLoads :=
  { B := 64, m := 32, strict := false, loads := [(0, 64)] }

/-- Every loop above. -/
def allLoops : List LoopLoads :=
  [arm64_utf32to8, lsx_utf32to8, lasx_utf32to8, westmere_utf32to8_err,
   westmere_utf32to8, westmere_utf32to16, westmere_utf32to16_err,
   haswell_utf32to16, haswell_latin1to8, lasx_latin1to8, arm64_latin1to8,
   icelake_latin1to8]

/-! ## The numbers: guaranteed units and furthest load -/

theorem table :
    allLoops.map (fun l => (l.guaranteed, l.reach)) =
      [(20, 8), (20, 8), (28, 16), (20, 8),
       (28, 16), (20, 16), (8, 8),
       (8, 8), (24, 16), (24, 16), (16, 16),
       (96, 64)] := by decide

/-- Every load of every loop ends within the `B` units the iteration
processes: the loads alone need no margin at all (non-strict test, `m = 0`). -/
theorem all_fit_B : allLoops.all (fun l => l.fits l.B) = true := by decide

/-- Every load of every loop ends within what the loop test guarantees. -/
theorem all_fit : allLoops.all (fun l => l.fits l.guaranteed) = true := by decide

/-- Every scalar fallback's cap is below what the loop test guarantees, so
`forward = cap` there and every `buf[k]` read is inside the input. -/
theorem all_scalar :
    allLoops.all (fun l => match l.scalarCap with
      | some c => decide (c < l.guaranteed)
      | none => true) = true := by decide

/-- Generic form of the scalar bound. -/
theorem scalar_ok (cap N : Nat) (hc : cap < N) :
    ∀ r k, N ≤ r → k < fwd cap r → k < r := by
  intro r k hr hk
  exact scalar_reads_in cap r k (by omega) hk

/-! ## Per loop: loads and stores under the same loop test -/

theorem arm64_utf32to8_tied :
    Arch.Arm64.utf32to8.all (Iter.ok (need 16 3 true)) = true ∧
    (∀ n, need 16 3 true ≤ n → ∀ p ∈ arm64_utf32to8.loads, p.1 + p.2 ≤ n) ∧
    (∀ r k, need 16 3 true ≤ r → k < fwd 15 r → k < r) :=
  ⟨Arch.Arm64.ok_utf32to8, fits_spec _ _ (by decide),
   scalar_ok 15 _ (by decide)⟩

theorem lsx_utf32to8_tied :
    Kernels.utf32to8_lsx.all (Iter.ok (need 16 3 true)) = true ∧
    (∀ n, need 16 3 true ≤ n → ∀ p ∈ lsx_utf32to8.loads, p.1 + p.2 ≤ n) ∧
    (∀ r k, need 16 3 true ≤ r → k < fwd 15 r → k < r) :=
  ⟨Arch.Lsx.ok_utf32to8, fits_spec _ _ (by decide),
   scalar_ok 15 _ (by decide)⟩

theorem lasx_utf32to8_tied :
    Arch.Lasx.utf32to8.all (Iter.ok (need 16 11 true)) = true ∧
    (∀ n, need 16 11 true ≤ n → ∀ p ∈ lasx_utf32to8.loads, p.1 + p.2 ≤ n) ∧
    (∀ r k, need 16 11 true ≤ r → k < fwd 15 r → k < r) :=
  ⟨Arch.Lasx.ok_utf32to8, fits_spec _ _ (by decide),
   scalar_ok 15 _ (by decide)⟩

theorem westmere_utf32to8_err_tied :
    Arch.Westmere.utf32to8_err.all (Iter.ok (need 16 4 false)) = true ∧
    (∀ n, need 16 4 false ≤ n → ∀ p ∈ westmere_utf32to8_err.loads, p.1 + p.2 ≤ n) ∧
    (∀ r k, need 16 4 false ≤ r → k < fwd 15 r → k < r) :=
  ⟨Arch.Westmere.ok_utf32to8_err, fits_spec _ _ (by decide),
   scalar_ok 15 _ (by decide)⟩

theorem westmere_utf32to8_tied :
    Arch.Westmere.utf32to8.all (Iter.ok (need 16 12 false)) = true ∧
    (∀ n, need 16 12 false ≤ n → ∀ p ∈ westmere_utf32to8.loads, p.1 + p.2 ≤ n) ∧
    (∀ r k, need 16 12 false ≤ r → k < fwd 15 r → k < r) :=
  ⟨Arch.Westmere.cur_utf32to8, fits_spec _ _ (by decide),
   scalar_ok 15 _ (by decide)⟩

theorem westmere_utf32to16_tied :
    Arch.Westmere.utf32to16.all (Iter.ok (need 16 4 false)) = true ∧
    (∀ n, need 16 4 false ≤ n → ∀ p ∈ westmere_utf32to16.loads, p.1 + p.2 ≤ n) :=
  ⟨Arch.Westmere.ok_utf32to16, fits_spec _ _ (by decide)⟩

theorem westmere_utf32to16_err_tied :
    Arch.Westmere.utf32to16_err.all (Iter.ok (need 8 0 false)) = true ∧
    (∀ n, need 8 0 false ≤ n → ∀ p ∈ westmere_utf32to16_err.loads, p.1 + p.2 ≤ n) ∧
    (∀ r k, need 8 0 false ≤ r → k < fwd 7 r → k < r) :=
  ⟨Arch.Westmere.cur_utf32to16_err, fits_spec _ _ (by decide),
   scalar_ok 7 _ (by decide)⟩

theorem haswell_utf32to16_tied :
    Arch.Haswell.utf32to16.all (Iter.ok (need 8 0 false)) = true ∧
    (∀ n, need 8 0 false ≤ n → ∀ p ∈ haswell_utf32to16.loads, p.1 + p.2 ≤ n) ∧
    (∀ r k, need 8 0 false ≤ r → k < fwd 7 r → k < r) :=
  ⟨Arch.Haswell.ok_utf32to16, fits_spec _ _ (by decide),
   scalar_ok 7 _ (by decide)⟩

theorem haswell_latin1to8_tied :
    Arch.Haswell.latin1to8.all (Iter.ok (need 16 8 false)) = true ∧
    (∀ n, need 16 8 false ≤ n → ∀ p ∈ haswell_latin1to8.loads, p.1 + p.2 ≤ n) :=
  ⟨Arch.Haswell.ok_latin1to8, fits_spec _ _ (by decide)⟩

theorem lasx_latin1to8_tied :
    Arch.Lasx.latin1to8.all (Iter.ok (need 16 8 false)) = true ∧
    (∀ n, need 16 8 false ≤ n → ∀ p ∈ lasx_latin1to8.loads, p.1 + p.2 ≤ n) :=
  ⟨Arch.Lasx.ok_latin1to8, fits_spec _ _ (by decide)⟩

theorem arm64_latin1to8_tied :
    Arch.Arm64.latin1to8.all (Iter.ok (need 16 0 false)) = true ∧
    (∀ n, need 16 0 false ≤ n → ∀ p ∈ arm64_latin1to8.loads, p.1 + p.2 ≤ n) :=
  ⟨Arch.Arm64.ok_latin1to8, fits_spec _ _ (by decide)⟩

theorem icelake_latin1to8_tied :
    Arch.Icelake.latin1to8.all (Iter.ok (need 64 32 false)) = true ∧
    (∀ n, need 64 32 false ≤ n → ∀ p ∈ icelake_latin1to8.loads, p.1 + p.2 ≤ n) :=
  ⟨Arch.Icelake.ok_latin1to8, fits_spec _ _ (by decide)⟩

/-! ## One margin lower: who sets the optimum

For every loop with `m > 0`, the loads still fit with margin `m - 1`; one less
overflows the store (`wit_*` in `Arch/*.lean`), so the stores set the optimum.
For the loops with `m = 0` and a non-strict test, the widest load ends exactly
at `B`: a loop test guaranteeing one unit fewer would read past the input, so
loads and stores both set the optimum there. -/

theorem lower_fits :
    arm64_utf32to8.fits (need 16 2 true) = true ∧
    lsx_utf32to8.fits (need 16 2 true) = true ∧
    lasx_utf32to8.fits (need 16 10 true) = true ∧
    westmere_utf32to8_err.fits (need 16 3 false) = true ∧
    westmere_utf32to8.fits (need 16 11 false) = true ∧
    westmere_utf32to16.fits (need 16 3 false) = true ∧
    haswell_latin1to8.fits (need 16 7 false) = true ∧
    lasx_latin1to8.fits (need 16 7 false) = true ∧
    icelake_latin1to8.fits (need 64 31 false) = true := by decide

/-- Margin-0 loops: the widest load ends at `B`, so `B - 1` guaranteed
units would not suffice. -/
theorem zero_margin_tight :
    haswell_utf32to16.fits (8 - 1) = false ∧
    westmere_utf32to16_err.fits (8 - 1) = false ∧
    arm64_latin1to8.fits (16 - 1) = false := by decide

end SafetyMargins.Loads
