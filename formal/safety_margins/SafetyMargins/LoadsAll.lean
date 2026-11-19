import SafetyMargins.Loads
import SafetyMargins.Utf16Fix
import SafetyMargins.Arch.Ppc64

/-!
# Input loads of every other fixed-margin loop

`Loads.lean` covers the loops whose margin the PR lowered. This file covers
every other loop that has a path table in `Arch/*.lean` (westmere, haswell,
icelake, arm64, ppc64, lsx, lasx; rvv has none, its loads are `vl`-exact):
the loops whose margin is unchanged (UTF-16 → UTF-8, UTF-32 → UTF-8 at 12,
Latin-1 → UTF-8 at 8, ...), the exact-store loops (Latin-1 → UTF-16/32,
→ Latin-1, `change_endianness`, UTF-32 → UTF-16), the UTF-16 → UTF-32 loops,
the `utf16fix` loops, and the Ice Lake UTF-8 loops (`Icelake.utf8`).

For each loop, one iteration's input reads, in input units relative to the
cursor at the top of the iteration, taking every branch:
* `loads`: `(offset, width)` of each unmasked vector load, and of each scalar
  read of a fixed range inside the iteration (an error path that scans the
  units the vector test already loaded, written `(0, B)` or similar);
* `clamped`: `(shift, cap)` of a scalar fallback
  `forward = cap; if (end - buf < forward + 1) forward = end - buf - 1;`
  run at `buf + shift` (after an ASCII half has advanced the cursor), which
  reads `buf[shift + k]` and, for a surrogate pair, `buf[shift + k + 1]`,
  `k < forward`;
* `back`: `(d, w)`, a load of `w` units at `cursor - d`, behind the cursor,
  in a loop whose cursor is at least `start` (the `utf16fix` lookback
  `in - 1`).

`Loop.Reads N` says that every read lies in the input once the loop test
guarantees `N` units: `loads` end within `N`, every `clamped` read is below
`end - buf`, and every `back` load starts at or after the buffer start and
ends within the input. Each `*_tied` theorem states, under the same
`need B m strict`, the store theorem of that loop's table (`cur_*`, `ok_*`,
`exact_*` or the `safe` theorem), and `Reads`. For `utf16fix`, it also
restates the whole-kernel bound of `Utf16Fix.lean`, which covers the head,
the final overlapping block and the tail.

AVX-512 masked loads (Ice Lake tails) touch only the lanes of their mask;
`masked_tails` bounds those masks. Lookup-table loads (`row + 1`, ...) are
not input reads and are not listed. Templated big/little-endian variants
share the loop body. Line numbers refer to the C++ at commit f13087ef.

Result: every read of every loop fits (`all_fit`). No over-read was found.
-/

namespace SafetyMargins.LoadsAll
open SafetyMargins SafetyMargins.Loads

/-- The input reads of one iteration of a loop with test `end - buf ≥ B + m`
(or `> B + m` when `strict`). -/
structure Loop where
  B : Nat
  m : Nat
  strict : Bool
  /-- `(offset, width)` in input units, every branch. -/
  loads : List (Nat × Nat)
  /-- `(shift, cap)` of a clamped scalar fallback. -/
  clamped : List (Nat × Nat) := []
  /-- `(d, w)`: `w` units loaded at `cursor - d`. -/
  back : List (Nat × Nat) := []
  /-- Least cursor at which the loop runs (for `back`). -/
  start : Nat := 0
deriving Repr

/-- Units the loop test guarantees. -/
def Loop.N (l : Loop) : Nat := need l.B l.m l.strict

/-- Every read fits once `N` units remain. -/
def Loop.fits (l : Loop) (N : Nat) : Bool :=
  l.loads.all (fun p => decide (p.1 + p.2 ≤ N)) &&
  l.clamped.all (fun q => decide (q.1 < N)) &&
  l.back.all (fun q => decide (q.1 ≤ l.start ∧ q.2 ≤ N + q.1))

/-- Furthest unit read by the `loads`. -/
def Loop.reach (l : Loop) : Nat := l.loads.foldl (fun a p => max a (p.1 + p.2)) 0

/-- A clamped scalar read past what the loop test guarantees, without the
clamp: the clamp is then what keeps it in. -/
def Loop.clampNeeded (l : Loop) : Bool :=
  l.clamped.any fun q => decide (l.N < q.1 + q.2 + 1)

/-- Every read of an iteration lies in the input when at least `N` units
remain. For `back`, `c` is the cursor and `c + r` the input length. -/
def Loop.Reads (l : Loop) (N : Nat) : Prop :=
  (∀ n, N ≤ n → ∀ p ∈ l.loads, p.1 + p.2 ≤ n) ∧
  (∀ r, N ≤ r → ∀ q ∈ l.clamped, ∀ k, k < fwd q.2 (r - q.1) → q.1 + k + 1 < r) ∧
  (∀ c r, l.start ≤ c → N ≤ r → ∀ q ∈ l.back, q.1 ≤ c ∧ c - q.1 + q.2 ≤ c + r)

/-- The clamped scalar fallback at `buf + s` reads `buf[s + k]` and
`buf[s + k + 1]`, `k < forward`, inside the input as soon as `s < end - buf`:
`forward ≤ end - buf - s - 1` whichever branch of the clamp is taken. -/
theorem clamped_in (s cap r k : Nat) (hs : s < r) (hk : k < fwd cap (r - s)) :
    s + k + 1 < r := by
  unfold fwd at hk
  split at hk <;> omega

theorem reads_of_fits (l : Loop) (N : Nat) (h : l.fits N = true) : l.Reads N := by
  simp only [Loop.fits, Bool.and_eq_true, List.all_eq_true, decide_eq_true_eq] at h
  obtain ⟨⟨h1, h2⟩, h3⟩ := h
  refine ⟨fun n hn p hp => ?_, fun r hr q hq k hk => ?_, fun c r hc hr q hq => ?_⟩
  · have := h1 p hp; omega
  · exact clamped_in q.1 q.2 r k (by have := h2 q hq; omega) hk
  · have := h3 q hq; omega

/-! ## Westmere -/

/-- src/westmere/sse_convert_utf16_to_utf8.cpp:65 (plain) and :292
(`_with_errors`), `end - buf >= 16 + 12`: `_mm_loadu_si128(buf)` (:66, :293);
on the ASCII test, `(__m128i *)buf + 1`, units 8..15 (:75, :302). When only the
first 8 are ASCII, `buf += 8` and the register path runs on `nextin`, so the
scalar fallback (`forward = 15`, `buf[k]` and `buf[k + 1]` of a pair,
:233-253, :460-480) starts at shift 0 or 8. -/
def westmere_utf16to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 8), (8, 8)],
    clamped := [(0, 15), (8, 15)] }

theorem westmere_utf16to8_tied :
    Arch.Westmere.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    westmere_utf16to8.N = need 16 12 false ∧ westmere_utf16to8.Reads (need 16 12 false) :=
  ⟨Arch.Westmere.cur_utf16to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_latin1_to_utf8.cpp:25, `end - latin_input >= 16 + 8`:
`_mm_loadu_si128(latin_input)` (:27); both halves are shuffled from that
register. No scalar path. -/
def westmere_latin1to8 : Loop :=
  { B := 16,
    m := 8,
    strict := false,
    loads := [(0, 16)] }

theorem westmere_latin1to8_tied :
    Arch.Westmere.latin1to8.all (Iter.ok (need 16 8 false)) = true ∧
    westmere_latin1to8.N = need 16 8 false ∧ westmere_latin1to8.Reads (need 16 8 false) :=
  ⟨Arch.Westmere.cur_latin1to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_latin1_to_utf8.cpp:51, the block after the loop,
`end - latin_input >= 16`: `_mm_loadu_si128(latin_input)` (:53). -/
def westmere_latin1to8_last : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem westmere_latin1to8_last_tied :
    Arch.Westmere.latin1to8_last.all (Iter.ok (need 16 0 false)) = true ∧
    westmere_latin1to8_last.N = need 16 0 false ∧ westmere_latin1to8_last.Reads (need 16 0 false) :=
  ⟨Arch.Westmere.cur_latin1to8_last, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_latin1_to_utf16.cpp:6, `i < rounded_len`,
`rounded_len = len & ~0xF` (:5): `_mm_loadu_si128(&latin1_input[i])` (:8-9). -/
def westmere_latin1to16 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem westmere_latin1to16_tied :
    Arch.Westmere.latin1to16.all (Iter.ok (need 16 0 false)) = true ∧
    westmere_latin1to16.N = need 16 0 false ∧ westmere_latin1to16.Reads (need 16 0 false) :=
  ⟨Arch.Westmere.cur_latin1to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_latin1_to_utf32.cpp:6, `end - buf >= 16`:
`_mm_loadu_si128(buf)` (:8); the other lanes are byte shifts of it. -/
def westmere_latin1to32 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem westmere_latin1to32_tied :
    Arch.Westmere.latin1to32.all (Iter.ok (need 16 0 false)) = true ∧
    westmere_latin1to32.N = need 16 0 false ∧ westmere_latin1to32.Reads (need 16 0 false) :=
  ⟨Arch.Westmere.cur_latin1to32, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_utf16_to_latin1.cpp:6, `end - buf >= 8`:
`_mm_loadu_si128(buf)` (:8). -/
def westmere_utf16tolatin1 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8)] }

theorem westmere_utf16tolatin1_tied :
    Arch.Westmere.utf16tolatin1.all (Iter.ok (need 8 0 false)) = true ∧
    westmere_utf16tolatin1.N = need 8 0 false ∧ westmere_utf16tolatin1.Reads (need 8 0 false) :=
  ⟨Arch.Westmere.cur_utf16tolatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_utf16_to_latin1.cpp:38, `_with_errors`,
`end - buf >= 8`: `_mm_loadu_si128(buf)` (:39); the error path reads `buf[k]`,
`k < 8` (:56-57), listed as `(0, 8)`. -/
def westmere_utf16tolatin1_err : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8), (0, 8)] }

theorem westmere_utf16tolatin1_err_tied :
    Arch.Westmere.utf16tolatin1_err.all (Iter.ok (need 8 0 false)) = true ∧
    westmere_utf16tolatin1_err.N = need 8 0 false ∧ westmere_utf16tolatin1_err.Reads (need 8 0 false) :=
  ⟨Arch.Westmere.cur_utf16tolatin1_err, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_utf32_to_latin1.cpp:10, `i < rounded_len`,
`rounded_len = len & ~0xF` (:4): `buf`, `buf + 4`, `buf + 8`, `buf + 12`
(:11-14). -/
def westmere_utf32tolatin1 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 4), (4, 4), (8, 4), (12, 4)] }

theorem westmere_utf32tolatin1_tied :
    Arch.Westmere.utf32tolatin1.all (Iter.ok (need 16 0 false)) = true ∧
    westmere_utf32tolatin1.N = need 16 0 false ∧ westmere_utf32tolatin1.Reads (need 16 0 false) :=
  ⟨Arch.Westmere.cur_utf32tolatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_utf32_to_latin1.cpp:46, `_with_errors`: the same
four loads (:47-50) and, on error, `buf[k]`, `k < 16` (:58-59). -/
def westmere_utf32tolatin1_err : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 4), (4, 4), (8, 4), (12, 4), (0, 16)] }

theorem westmere_utf32tolatin1_err_tied :
    Arch.Westmere.utf32tolatin1_err.all (Iter.ok (need 16 0 false)) = true ∧
    westmere_utf32tolatin1_err.N = need 16 0 false ∧ westmere_utf32tolatin1_err.Reads (need 16 0 false) :=
  ⟨Arch.Westmere.cur_utf32tolatin1_err, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`:
`simd16x32` at `in + pos` (:11), four 16-byte loads, 32 units
(src/simdutf/westmere/simd16-inl.h:163-167). Called by the westmere
`change_endianness_utf16`, src/westmere/implementation.cpp:1102. -/
def westmere_changeEndianness : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8), (16, 8), (24, 8)] }

theorem westmere_changeEndianness_tied :
    Arch.Westmere.changeEndianness.all (Iter.ok (need 32 0 false)) = true ∧
    westmere_changeEndianness.N = need 32 0 false ∧ westmere_changeEndianness.Reads (need 32 0 false) :=
  ⟨Arch.Westmere.cur_changeEndianness, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_utf16fix.cpp:66 and :72, `for (i = 1; i + 8 < n; i += 8)`:
the block loads 8 units at `in` and, behind the cursor, 8 units at `in - 1`
(`Utf16Fix.westmereAcc`). The cursor is at least 1. -/
def westmere_utf16fix : Loop :=
  { B := 8,
    m := 0,
    strict := true,
    loads := [(0, 8)],
    back := [(1, 8)],
    start := 1 }

theorem westmere_utf16fix_tied :
    Arch.Westmere.utf16fix.all (Iter.ok (need 8 0 true)) = true ∧
    (∀ n, Utf16Fix.AllInB n (Utf16Fix.westmereAcc n)) ∧
    westmere_utf16fix.N = need 8 0 true ∧ westmere_utf16fix.Reads (need 8 0 true) :=
  ⟨Arch.Westmere.cur_utf16fix, Utf16Fix.westmere_inBounds, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/westmere/sse_convert_utf16_to_utf32.cpp:63 (plain) and :143
(`_with_errors`), `end - buf >= 8`: `_mm_loadu_si128(buf)` (:64, :144);
scalar fallback `forward = 15`, `buf[k]` and `buf[k + 1]` (:97-110,
:177-190). With 8 units guaranteed, the clamp
`forward = end - buf - 1` (:99-100, :179-180) is what keeps those reads in. -/
def westmere_utf16to32 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8)],
    clamped := [(0, 15)] }

theorem westmere_utf16to32_tied :
    (Arch.Westmere.utf16to32.all Iter.exact = true) ∧
    westmere_utf16to32.N = need 8 0 false ∧ westmere_utf16to32.Reads (need 8 0 false) :=
  ⟨Arch.Westmere.exact_utf16to32, rfl, reads_of_fits _ _ (by decide)⟩


/-! ## Haswell -/

/-- src/haswell/avx2_convert_latin1_to_utf16.cpp:8, `i < rounded_len`,
`rounded_len = len & ~0xF` (:5): `_mm_loadu_si128(latin1_input + i)` (:10-11). -/
def haswell_latin1to16 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem haswell_latin1to16_tied :
    Arch.Haswell.latin1to16.all (Iter.ok (need 16 0 false)) = true ∧
    haswell_latin1to16.N = need 16 0 false ∧ haswell_latin1to16.Reads (need 16 0 false) :=
  ⟨Arch.Haswell.cur_latin1to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_convert_latin1_to_utf32.cpp:6, `i < rounded_len`,
`rounded_len = (len | 7) ^ 7` (:4): `_mm_loadl_epi64(&buf[i])`, 8 bytes (:8). -/
def haswell_latin1to32 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8)] }

theorem haswell_latin1to32_tied :
    Arch.Haswell.latin1to32.all (Iter.ok (need 8 0 false)) = true ∧
    haswell_latin1to32.N = need 8 0 false ∧ haswell_latin1to32.Reads (need 8 0 false) :=
  ⟨Arch.Haswell.cur_latin1to32, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_convert_utf16_to_latin1.cpp:6, `end - buf >= 32`:
`_mm256_loadu_si256(buf)` and `(buf + 16)` (:8-10). -/
def haswell_utf16to1 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 16), (16, 16)] }

theorem haswell_utf16to1_tied :
    Arch.Haswell.utf16to1.all (Iter.ok (need 32 0 false)) = true ∧
    haswell_utf16to1.N = need 32 0 false ∧ haswell_utf16to1.Reads (need 32 0 false) :=
  ⟨Arch.Haswell.cur_utf16to1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_convert_utf16_to_latin1.cpp:44, `_with_errors`,
`end - buf >= 16`: `_mm256_loadu_si256(buf)` (:45); error path `buf[k]`,
`k < 16` (:68-69). -/
def haswell_utf16to1_err : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16), (0, 16)] }

theorem haswell_utf16to1_err_tied :
    Arch.Haswell.utf16to1_err.all (Iter.ok (need 16 0 false)) = true ∧
    haswell_utf16to1_err.N = need 16 0 false ∧ haswell_utf16to1_err.Reads (need 16 0 false) :=
  ⟨Arch.Haswell.cur_utf16to1_err, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_convert_utf32_to_latin1.cpp:9 (plain) and :70
(`_with_errors`), `i < rounded_len`, `rounded_len = len & ~0x1F` (:4-5,
:63-64): `buf + 0, 8, 16, 24` (:10-13, :71-74); error path `buf[k]`,
`k < 32` (:81-82). -/
def haswell_utf32to1 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8), (16, 8), (24, 8), (0, 32)] }

theorem haswell_utf32to1_tied :
    Arch.Haswell.utf32to1.all (Iter.ok (need 32 0 false)) = true ∧
    haswell_utf32to1.N = need 32 0 false ∧ haswell_utf32to1.Reads (need 32 0 false) :=
  ⟨Arch.Haswell.cur_utf32to1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_convert_utf16_to_utf8.cpp:64 (plain) and :341
(`_with_errors`), `end - buf >= 16 + 12`: `_mm256_loadu_si256(buf)` (:65,
:342); scalar fallback `forward = 15`, `buf[k]`, `buf[k + 1]` (:282-302,
:559-579). -/
def haswell_utf16to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 16)],
    clamped := [(0, 15)] }

theorem haswell_utf16to8_tied :
    Arch.Haswell.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    haswell_utf16to8.N = need 16 12 false ∧ haswell_utf16to8.Reads (need 16 12 false) :=
  ⟨Arch.Haswell.cur_utf16to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_convert_utf32_to_utf8.cpp:15 (plain) and :297
(`_with_errors`), `end - buf >= 16 + 12`: `_mm256_loadu_si256(buf)` and
`(__m256i *)buf + 1` (:16-17, :298-299); scalar fallback `forward = 15`,
`buf[k]` (:234-240, :529-535). -/
def haswell_utf32to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 8), (8, 8)],
    clamped := [(0, 15)] }

theorem haswell_utf32to8_tied :
    Arch.Haswell.utf32to8.all (Iter.ok (need 16 12 false)) = true ∧
    haswell_utf32to8.N = need 16 12 false ∧ haswell_utf32to8.Reads (need 16 12 false) :=
  ⟨Arch.Haswell.cur_utf32to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`:
`simd16x32` at `in + pos` (:11), two 32-byte loads, 32 units
(src/simdutf/haswell/simd16-inl.h:201-203). -/
def haswell_swap16 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 16), (16, 16)] }

theorem haswell_swap16_tied :
    Arch.Haswell.swap16.all (Iter.ok (need 32 0 false)) = true ∧
    haswell_swap16.N = need 32 0 false ∧ haswell_swap16.Reads (need 32 0 false) :=
  ⟨Arch.Haswell.cur_swap16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_utf16fix.cpp:157 and :163, `for (i = 1; i + 16 < n; i += 16)`:
16 units at `in` and, behind the cursor, at `in - 1`
(`Utf16Fix.haswellAcc`). -/
def haswell_utf16fix : Loop :=
  { B := 16,
    m := 0,
    strict := true,
    loads := [(0, 16)],
    back := [(1, 16)],
    start := 1 }

theorem haswell_utf16fix_tied :
    Arch.Haswell.utf16fix.all (Iter.ok (need 16 0 true)) = true ∧
    (∀ n, Utf16Fix.AllInB n (Utf16Fix.haswellAcc n)) ∧
    haswell_utf16fix.N = need 16 0 true ∧ haswell_utf16fix.Reads (need 16 0 true) :=
  ⟨Arch.Haswell.cur_utf16fix, Utf16Fix.haswell_inBounds, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_utf16fix.cpp:124 and :130, `for (i = 1; i + 8 < n; i += 8)`:
8 units at `in` and at `in - 1` (`Utf16Fix.haswellAcc`, `n < 17`). -/
def haswell_utf16fix_sse : Loop :=
  { B := 8,
    m := 0,
    strict := true,
    loads := [(0, 8)],
    back := [(1, 8)],
    start := 1 }

theorem haswell_utf16fix_sse_tied :
    Arch.Haswell.utf16fix_sse.all (Iter.ok (need 8 0 true)) = true ∧
    (∀ n, Utf16Fix.AllInB n (Utf16Fix.haswellAcc n)) ∧
    haswell_utf16fix_sse.N = need 8 0 true ∧ haswell_utf16fix_sse.Reads (need 8 0 true) :=
  ⟨Arch.Haswell.cur_utf16fix_sse, Utf16Fix.haswell_inBounds, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/haswell/avx2_convert_utf16_to_utf32.cpp:62 (plain) and :144
(`_with_errors`), `end - buf >= 16`: `_mm256_loadu_si256(buf)` (:63, :145);
scalar fallback `forward = 15`, `buf[k]`, `buf[k + 1]` (:98-112, :180-194). -/
def haswell_utf16to32 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)],
    clamped := [(0, 15)] }

theorem haswell_utf16to32_tied :
    (Arch.Haswell.utf16to32.all Iter.exact = true) ∧
    haswell_utf16to32.N = need 16 0 false ∧ haswell_utf16to32.Reads (need 16 0 false) :=
  ⟨Arch.Haswell.exact_utf16to32, rfl, reads_of_fits _ _ (by decide)⟩


/-! ## Ice Lake -/

/-- src/icelake/icelake_convert_latin1_to_utf16.inl.cpp:11, `i < rounded_len`,
`rounded_len = len & ~0x1F` (:5): `_mm256_loadu_si256(&latin1_input[i])`
(:13). Masked tail `len - rounded_len < 32` lanes (:22-24, `masked_tails`). -/
def icelake_latin1to16 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 32)] }

theorem icelake_latin1to16_tied :
    Arch.Icelake.latin1to16.all (Iter.ok (need 32 0 false)) = true ∧
    icelake_latin1to16.N = need 32 0 false ∧ icelake_latin1to16.Reads (need 32 0 false) :=
  ⟨Arch.Icelake.cur_latin1to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_convert_latin1_to_utf32.inl.cpp:3, `len >= 16`:
`_mm_loadu_si128(buf)` (:5). Masked tail `len < 16` lanes (:19-20). -/
def icelake_latin1to32 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem icelake_latin1to32_tied :
    Arch.Icelake.latin1to32.all (Iter.ok (need 16 0 false)) = true ∧
    icelake_latin1to32.N = need 16 0 false ∧ icelake_latin1to32.Reads (need 16 0 false) :=
  ⟨Arch.Icelake.cur_latin1to32, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/implementation.cpp:1615, `pos + 32 <= length`:
`_mm512_loadu_si512(input + pos)` (:1616). Masked tail `length - pos < 32`
lanes (:1621-1623). -/
def icelake_swap16 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 32)] }

theorem icelake_swap16_tied :
    Arch.Icelake.swap16.all (Iter.ok (need 32 0 false)) = true ∧
    icelake_swap16.N = need 32 0 false ∧ icelake_swap16.Reads (need 32 0 false) :=
  ⟨Arch.Icelake.cur_swap16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_utf16fix.cpp:122 and :128, `for (i = 1; i + 32 < n; i += 32)`:
64 bytes at `in` and at `in - 1` (`Utf16Fix.icelakeAcc`; the short kernel
for `n < 33` is masked there). -/
def icelake_utf16fix : Loop :=
  { B := 32,
    m := 0,
    strict := true,
    loads := [(0, 32)],
    back := [(1, 32)],
    start := 1 }

theorem icelake_utf16fix_tied :
    Arch.Icelake.utf16fix.all (Iter.ok (need 32 0 true)) = true ∧
    (∀ n, Utf16Fix.AllInB n (Utf16Fix.icelakeAcc n)) ∧
    icelake_utf16fix.N = need 32 0 true ∧ icelake_utf16fix.Reads (need 32 0 true) :=
  ⟨Arch.Icelake.cur_utf16fix, Utf16Fix.icelake_inBounds, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_convert_utf16_to_latin1.inl.cpp:15 (plain) and :60
(`_with_errors`), `end - buf >= 32`: `_mm512_loadu_si512(buf)` (:16, :61).
The `_with_errors` error scan (:66-70) reads `*buf` until a unit above 0xFF,
which the vector test found among the 32 loaded: `(0, 32)`. Masked tails
`end - buf < 32` lanes (:30-31, :82-83). -/
def icelake_utf16to1 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 32), (0, 32)] }

theorem icelake_utf16to1_tied :
    Arch.Icelake.utf16to1.all (Iter.ok (need 32 0 false)) = true ∧
    icelake_utf16to1.N = need 32 0 false ∧ icelake_utf16to1.Reads (need 32 0 false) :=
  ⟨Arch.Icelake.cur_utf16to1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_convert_utf32_to_latin1.inl.cpp:10 (plain) and :44
(`_with_errors`), `end - buf >= 16`: `_mm512_loadu_si512(buf)` (:11, :45);
error scan `*buf <= 0xff` (:47-48) stops inside the 16 loaded. Masked tails
`end - buf < 16` lanes (:22-23, :60-61). -/
def icelake_utf32to1 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16), (0, 16)] }

theorem icelake_utf32to1_tied :
    Arch.Icelake.utf32to1.all (Iter.ok (need 16 0 false)) = true ∧
    icelake_utf32to1.N = need 16 0 false ∧ icelake_utf32to1.Reads (need 16 0 false) :=
  ⟨Arch.Icelake.cur_utf32to1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_convert_utf32_to_utf8.inl.cpp:19 (plain) and :302
(`_with_errors`), `end - buf >= 16 + 12`: `_mm256_loadu_si256(buf)` and
`(__m256i *)buf + 1` (:20-21, :303-304); scalar fallback `forward = 15`
(:238-244, :534-540). -/
def icelake_utf32to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 8), (8, 8)],
    clamped := [(0, 15)] }

theorem icelake_utf32to8_tied :
    Arch.Icelake.utf32to8.all (Iter.ok (need 16 12 false)) = true ∧
    icelake_utf32to8.N = need 16 12 false ∧ icelake_utf32to8.Reads (need 16 12 false) :=
  ⟨Arch.Icelake.cur_utf32to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_convert_utf32_to_utf16.inl.cpp:19 (plain) and :174
(`_with_errors`), `end - buf >= 16`: `_mm512_loadu_si512(buf)` (:20, :175).
Masked tails `end - buf < 16` lanes (:87-88, :258-259). -/
def icelake_utf32to16 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem icelake_utf32to16_tied :
    Arch.Icelake.utf32to16.all (Iter.ok (need 16 0 false)) = true ∧
    icelake_utf32to16.N = need 16 0 false ∧ icelake_utf32to16.Reads (need 16 0 false) :=
  ⟨Arch.Icelake.cur_utf32to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_convert_utf16_to_utf8.inl.cpp:23, `inlen >= 32`
(`B = 31`, margin 1): `_mm512_loadu_si512(inbuf)`, 32 units (:24). The last
load is masked to `inlen < 32` lanes (:195-196). This is `Icelake.read_utf16to8`
in the present form: the load, not the store, needs the margin. -/
def icelake_utf16to8 : Loop :=
  { B := 31,
    m := 1,
    strict := false,
    loads := [(0, 32)] }

theorem icelake_utf16to8_tied :
    Arch.Icelake.utf16to8.all (Iter.ok (need 31 1 false)) = true ∧
    icelake_utf16to8.N = need 31 1 false ∧ icelake_utf16to8.Reads (need 31 1 false) :=
  ⟨Arch.Icelake.cur_utf16to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_convert_utf16_to_utf32.inl.cpp:144, `end - buf >= 32`:
`_mm512_loadu_si512(buf)` (:146). After the loop: scalar on `rest` when
fewer than 8 units remain (:264-267), otherwise the masked kernel
`todo = _bzhi_u32(0xFFFFFFFF, end - buf)`, `end - buf < 32` lanes (:16-17). -/
def icelake_utf16to32 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 32)] }

theorem icelake_utf16to32_tied :
    (∀ (choose : List Nat → Iter), (∀ xs, 32 ≤ xs.length → choose xs ∈ Arch.Icelake.utf16to32) →
      ∀ n xs, written 32 choose n xs ≤ L xs) ∧
    icelake_utf16to32.N = need 32 0 false ∧ icelake_utf16to32.Reads (need 32 0 false) :=
  ⟨Arch.Icelake.safe_utf16to32, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_convert_utf8_to_utf16.inl.cpp:21 and :50,
`while (result)`, full block on the branch `final_in - in >= 64` (:23, :51):
`process_block_utf8_to_utf16<SIMDUTF_FULL>` loads
`_mm512_loadu_si512(in)` (src/icelake/icelake_utf8_common.inl.cpp:50); the
`SIMDUTF_TAIL` call masks to `gap < 64` lanes (:48-51). Also
src/icelake/icelake_convert_utf8_to_latin1.inl.cpp:70 and
src/icelake/icelake_convert_valid_utf8_to_latin1.inl.cpp:53, `pos + 64 <= len`, load
mask `~0` (:9-11 of each), tail `_bzhi_u64(~0, len - pos)`, fewer than 64
lanes; and the single block after the fixed-length loops, branch
`end - ptr >= 64` at src/icelake/icelake_from_utf8.inl.cpp:85 and :217 and
src/icelake/icelake_from_valid_utf8.inl.cpp:98. -/
def icelake_utf8_block : Loop :=
  { B := 64,
    m := 0,
    strict := false,
    loads := [(0, 64)] }

theorem icelake_utf8_block_tied :
    (Arch.Icelake.utf8.all Iter.exact = true) ∧
    icelake_utf8_block.N = need 64 0 false ∧ icelake_utf8_block.Reads (need 64 0 false) :=
  ⟨Arch.Icelake.exact_utf8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/icelake/icelake_from_utf8.inl.cpp:32 (`validating_utf8_to_fixed_length`)
and :160 (`_with_constant_checks`), `end - ptr >= 64 + 4`:
`_mm512_loadu_si512(ptr)` (:33, :161) and `memcpy(&tmp1, ptr + 64, 4)`
(:63, :195); the masked validation load after the blocks,
`~0 >> (64 - (end - validatedptr))` (:119-121, :255-257), has
`1 ≤ end - validatedptr ≤ 63` (`fixed_tail_lanes`). The same loop in
src/icelake/icelake_from_valid_utf8.inl.cpp:45, `end - ptr >= 64 + 4`: load
(:46) and `memcpy` (:79). The `+ 4` of the loop test is exactly the 4-byte
read: one less would read past the input (`icelake_utf8_fixed_tight`). -/
def icelake_utf8_fixed : Loop :=
  { B := 64,
    m := 4,
    strict := false,
    loads := [(0, 64), (64, 4)] }

theorem icelake_utf8_fixed_tied :
    (Arch.Icelake.utf8.all Iter.exact = true) ∧
    icelake_utf8_fixed.N = need 64 4 false ∧ icelake_utf8_fixed.Reads (need 64 4 false) :=
  ⟨Arch.Icelake.exact_utf8, rfl, reads_of_fits _ _ (by decide)⟩


/-! ## arm64 -/

/-- src/arm64/arm_convert_latin1_to_utf16.cpp:7, `end - buf >= 16`:
`vld1q_u8(buf)` (:8). -/
def arm64_latin1to16 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem arm64_latin1to16_tied :
    Arch.Arm64.latin1to16.all (Iter.ok (need 16 0 false)) = true ∧
    arm64_latin1to16.N = need 16 0 false ∧ arm64_latin1to16.Reads (need 16 0 false) :=
  ⟨Arch.Arm64.cur_latin1to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/arm64/arm_convert_latin1_to_utf32.cpp:6, `end - buf >= 16`:
`vld1q_u8(buf)` (:7). -/
def arm64_latin1to32 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem arm64_latin1to32_tied :
    Arch.Arm64.latin1to32.all (Iter.ok (need 16 0 false)) = true ∧
    arm64_latin1to32.N = need 16 0 false ∧ arm64_latin1to32.Reads (need 16 0 false) :=
  ⟨Arch.Arm64.cur_latin1to32, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/arm64/arm_convert_utf16_to_latin1.cpp:7 (plain) and :33
(`_with_errors`), `end - buf >= 8`: `vld1q_u16(buf)` (:8, :34); error path
`buf[k]`, `k < 8` (:48-49). Table `Arm64.toLatin1`. -/
def arm64_utf16toLatin1 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8), (0, 8)] }

theorem arm64_utf16toLatin1_tied :
    Arch.Arm64.toLatin1.all (Iter.ok (need 8 0 false)) = true ∧
    arm64_utf16toLatin1.N = need 8 0 false ∧ arm64_utf16toLatin1.Reads (need 8 0 false) :=
  ⟨Arch.Arm64.cur_toLatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/arm64/arm_convert_utf32_to_latin1.cpp:5 (plain) and :31
(`_with_errors`), `end - buf >= 8`: `vld1q_u32(buf)`, `vld1q_u32(buf + 4)`
(:6-7, :32-33); error path `buf[k]`, `k < 8` (:47-48). Table `Arm64.toLatin1`. -/
def arm64_utf32toLatin1 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 4), (4, 4), (0, 8)] }

theorem arm64_utf32toLatin1_tied :
    Arch.Arm64.toLatin1.all (Iter.ok (need 8 0 false)) = true ∧
    arm64_utf32toLatin1.N = need 8 0 false ∧ arm64_utf32toLatin1.Reads (need 8 0 false) :=
  ⟨Arch.Arm64.cur_toLatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/arm64/arm_convert_utf16_to_utf8.cpp:63 (plain) and :330
(`_with_errors`), `end - buf >= 16 + 12`: `vld1q_u16(buf)` (:64, :331);
on the ASCII test `vld1q_u16(buf + 8)` (:71-72, :338-339); after the 8-unit
ASCII half `buf += 8` (:83, :350), so the scalar fallback (`forward = 15`,
`buf[k]`, `buf[k + 1]`, :269-289, :536-556) starts at shift 0 or 8. -/
def arm64_utf16to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 8), (8, 8)],
    clamped := [(0, 15), (8, 15)] }

theorem arm64_utf16to8_tied :
    Arch.Arm64.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    arm64_utf16to8.N = need 16 12 false ∧ arm64_utf16to8.Reads (need 16 12 false) :=
  ⟨Arch.Arm64.cur_utf16to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/arm64/arm_convert_utf32_to_utf16.cpp:76 (plain) and :130
(`_with_errors`), `end - buf >= 8 + 4`: `vld1q_u32_x2(buf)`, 8 units (:77,
:131). The `_with_errors` error path reads `buf[k]`, `k ≤ pos`, with
`pos = trailing_zeroes(err) / 8 < 8` for the nonzero 8-lane mask (:158-181). -/
def arm64_utf32to16 : Loop :=
  { B := 8,
    m := 4,
    strict := false,
    loads := [(0, 8), (0, 8)] }

theorem arm64_utf32to16_tied :
    Arch.Arm64.utf32to16.all (Iter.ok (need 8 4 false)) = true ∧
    arm64_utf32to16.N = need 8 4 false ∧ arm64_utf32to16.Reads (need 8 4 false) :=
  ⟨Arch.Arm64.cur_utf32to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`:
`simd16x32` at `in + pos` (:11), four 16-byte loads
(src/simdutf/arm64/simd16-inl.h:299-303). -/
def arm64_changeEndianness : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8), (16, 8), (24, 8)] }

theorem arm64_changeEndianness_tied :
    Arch.Arm64.changeEndianness.all (Iter.ok (need 32 0 false)) = true ∧
    arm64_changeEndianness.N = need 32 0 false ∧ arm64_changeEndianness.Reads (need 32 0 false) :=
  ⟨Arch.Arm64.cur_changeEndianness, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/arm64/arm_utf16fix.cpp:156 and :167, `for (i = 1; i + 64 < n; i += 64)`:
`utf16fix_block64` (`Utf16Fix.block64Acc`): 16 units at `in + 16 k` and at
`in + 16 k - 1`, `k < 4`, then `in[r - 1]`, `r < 64`. The `k = 0` lookback
and `in[-1]` are behind the cursor; the rest is in `[0, 63)`. -/
def arm64_utf16fix64 : Loop :=
  { B := 64,
    m := 0,
    strict := true,
    loads := [(0, 16), (15, 16), (16, 16), (31, 16), (32, 16), (47, 16), (48, 16), (0, 63)],
    back := [(1, 16), (1, 1)],
    start := 1 }

theorem arm64_utf16fix64_tied :
    Arch.Arm64.utf16fix64.all (Iter.ok (need 64 0 true)) = true ∧
    (∀ n, Utf16Fix.AllInB n (Utf16Fix.arm64Acc n)) ∧
    arm64_utf16fix64.N = need 64 0 true ∧ arm64_utf16fix64.Reads (need 64 0 true) :=
  ⟨Arch.Arm64.cur_utf16fix64, Utf16Fix.arm64_inBounds, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/arm64/arm_utf16fix.cpp:160 and :170, `for (; i + 16 < n; i += 16)`,
from the `i ≥ 1` the 64-unit loop left: 16 units at `in` and `in - 1`. -/
def arm64_utf16fix16 : Loop :=
  { B := 16,
    m := 0,
    strict := true,
    loads := [(0, 16)],
    back := [(1, 16)],
    start := 1 }

theorem arm64_utf16fix16_tied :
    Arch.Arm64.utf16fix16.all (Iter.ok (need 16 0 true)) = true ∧
    (∀ n, Utf16Fix.AllInB n (Utf16Fix.arm64Acc n)) ∧
    arm64_utf16fix16.N = need 16 0 true ∧ arm64_utf16fix16.Reads (need 16 0 true) :=
  ⟨Arch.Arm64.cur_utf16fix16, Utf16Fix.arm64_inBounds, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/arm64/arm_convert_utf16_to_utf32.cpp:63 (plain) and :133
(`_with_errors`), `end - buf >= 8`: `vld1q_u16(buf)` (:64, :134); scalar
fallback `forward = 15`, `buf[k]`, `buf[k + 1]` (:85-98, :155-168), kept in
by the clamp (:87-88, :157-158). -/
def arm64_utf16to32 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8)],
    clamped := [(0, 15)] }

theorem arm64_utf16to32_tied :
    (∀ (choose : List Nat → Iter), (∀ xs, 8 ≤ xs.length → choose xs ∈ Arch.Arm64.utf16to32) →
      (∀ xs, choose xs = Arch.Arm64.utf16to32Vec → xs.take 8 = List.replicate 8 1) →
      ∀ n xs, written 8 choose n xs ≤ L xs) ∧
    arm64_utf16to32.N = need 8 0 false ∧ arm64_utf16to32.Reads (need 8 0 false) :=
  ⟨Arch.Arm64.utf16to32_safe, rfl, reads_of_fits _ _ (by decide)⟩


/-! ## ppc64 -/

/-- src/ppc64/ppc64_convert_utf16_to_utf8.cpp:184, `end - buf >= 16 + 12`:
`vector_u16::load(buf)` (:185), `vector_u16::load(buf + 8)` on the ASCII test
(:191); after the ASCII half `buf += 8` (:212), so the scalar fallback
(`forward = 15`, `buf[k]`, `buf[k + 1]`, :255-275) starts at shift 0 or 8. -/
def ppc64_utf16to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 8), (8, 8)],
    clamped := [(0, 15), (8, 15)] }

theorem ppc64_utf16to8_tied :
    Arch.Ppc64.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    ppc64_utf16to8.N = need 16 12 false ∧ ppc64_utf16to8.Reads (need 16 12 false) :=
  ⟨Arch.Ppc64.cur_utf16to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_utf32_to_utf8.cpp:20-22, `end - buf >= 16 + 12`:
`vector_u32::load(buf)`, `(buf + 4)` (:27-28), and on the ASCII test
`(buf + 8)`, `(buf + 12)` (:44-45); after the ASCII half `buf += 8` (:65), so
the scalar fallback (`forward = 15`, `buf[k]`, :119-125) starts at shift 0 or 8. -/
def ppc64_utf32to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 4), (4, 4), (8, 4), (12, 4)],
    clamped := [(0, 15), (8, 15)] }

theorem ppc64_utf32to8_tied :
    Arch.Ppc64.utf32to8.all (Iter.ok (need 16 12 false)) = true ∧
    ppc64_utf32to8.N = need 16 12 false ∧ ppc64_utf32to8.Reads (need 16 12 false) :=
  ⟨Arch.Ppc64.cur_utf32to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_latin1_to_utf8.cpp:30, `end - latin_input >= 16 + 8`:
`vector_u8::load(latin_input)` (:32). -/
def ppc64_latin1to8 : Loop :=
  { B := 16,
    m := 8,
    strict := false,
    loads := [(0, 16)] }

theorem ppc64_latin1to8_tied :
    Arch.Ppc64.latin1to8.all (Iter.ok (need 16 8 false)) = true ∧
    ppc64_latin1to8.N = need 16 8 false ∧ ppc64_latin1to8.Reads (need 16 8 false) :=
  ⟨Arch.Ppc64.cur_latin1to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_latin1_to_utf8.cpp:55, the block after the loop,
branch `end - latin_input >= 16`:
`vector_u8::load(latin_input)` (:57). -/
def ppc64_latin1to8Last : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem ppc64_latin1to8Last_tied :
    Arch.Ppc64.latin1to8Last.all (Iter.ok (need 16 0 false)) = true ∧
    ppc64_latin1to8Last.N = need 16 0 false ∧ ppc64_latin1to8Last.Reads (need 16 0 false) :=
  ⟨Arch.Ppc64.cur_latin1to8Last, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_latin1_to_utf16.cpp:6, `i < align_down<16>(len)`:
`vector_u8::load(&latin1_input[i])` (:7). -/
def ppc64_latin1to16 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem ppc64_latin1to16_tied :
    Arch.Ppc64.latin1to16.all (Iter.ok (need 16 0 false)) = true ∧
    ppc64_latin1to16.N = need 16 0 false ∧ ppc64_latin1to16.Reads (need 16 0 false) :=
  ⟨Arch.Ppc64.cur_latin1to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_latin1_to_utf32.cpp:6, `i < align_down<16>(len)`:
`vector_u8::load(&buf[i])` (:7). -/
def ppc64_latin1to32 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem ppc64_latin1to32_tied :
    Arch.Ppc64.latin1to32.all (Iter.ok (need 16 0 false)) = true ∧
    ppc64_latin1to32.N = need 16 0 false ∧ ppc64_latin1to32.Reads (need 16 0 false) :=
  ⟨Arch.Ppc64.cur_latin1to32, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_utf16_to_latin1.cpp:11, `end - buf >= 8`:
`vector_u8::load(buf)`, 16 bytes = 8 units (:13; `vec_xl`,
src/simdutf/ppc64/simd8-inl.h:241-243). The error path tests the loaded bytes. -/
def ppc64_utf16toLatin1 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8)] }

theorem ppc64_utf16toLatin1_tied :
    Arch.Ppc64.utf16toLatin1.all (Iter.ok (need 8 0 false)) = true ∧
    ppc64_utf16toLatin1.N = need 8 0 false ∧ ppc64_utf16toLatin1.Reads (need 8 0 false) :=
  ⟨Arch.Ppc64.cur_utf16toLatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_utf32_to_latin1.cpp:17, `i < align_down<16>(len)`:
`vector_u32::load(buf + 0, 4, 8, 12)` (:18-21). -/
def ppc64_utf32toLatin1 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 4), (4, 4), (8, 4), (12, 4)] }

theorem ppc64_utf32toLatin1_tied :
    Arch.Ppc64.utf32toLatin1.all (Iter.ok (need 16 0 false)) = true ∧
    ppc64_utf32toLatin1.N = need 16 0 false ∧ ppc64_utf32toLatin1.Reads (need 16 0 false) :=
  ⟨Arch.Ppc64.cur_utf32toLatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_utf32_to_utf16.cpp:18, `end - buf >= 8`:
`vector_u32::load(buf)`, `(buf + 4)` (:19-20); scalar fallback `forward = 7`,
`buf[k]` (:73-79). -/
def ppc64_utf32to16 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 4), (4, 4)],
    clamped := [(0, 7)] }

theorem ppc64_utf32to16_tied :
    Arch.Ppc64.utf32to16.all (Iter.ok (need 8 0 false)) = true ∧
    ppc64_utf32to16.N = need 8 0 false ∧ ppc64_utf32to16.Reads (need 8 0 false) :=
  ⟨Arch.Ppc64.cur_utf32to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`:
`simd16x32` at `in + pos` (:11), four 16-byte loads
(src/simdutf/ppc64/simd16-inl.h:252-256). -/
def ppc64_changeEndianness : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8), (16, 8), (24, 8)] }

theorem ppc64_changeEndianness_tied :
    Arch.Ppc64.changeEndianness.all (Iter.ok (need 32 0 false)) = true ∧
    ppc64_changeEndianness.N = need 32 0 false ∧ ppc64_changeEndianness.Reads (need 32 0 false) :=
  ⟨Arch.Ppc64.cur_changeEndianness, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/ppc64/ppc64_convert_utf16_to_utf32.cpp:16, `end - buf >= 8`:
`vector_u16::load(buf)` (:17); scalar fallback `forward = 15`, `buf[k]`,
`buf[k + 1]` (:58-71), kept in by the clamp (:60-61). -/
def ppc64_utf16to32 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8)],
    clamped := [(0, 15)] }

theorem ppc64_utf16to32_tied :
    (∀ (choose : List Nat → Iter), (∀ xs, 8 ≤ xs.length → choose xs ∈ Arch.Ppc64.utf16to32) →
      (∀ xs, choose xs = Arch.Ppc64.utf16to32Vec → xs.take 8 = List.replicate 8 1) →
      ∀ n xs, written 8 choose n xs ≤ L xs) ∧
    ppc64_utf16to32.N = need 8 0 false ∧ ppc64_utf16to32.Reads (need 8 0 false) :=
  ⟨Arch.Ppc64.utf16to32_safe, rfl, reads_of_fits _ _ (by decide)⟩


/-! ## lsx -/

/-- src/lsx/lsx_convert_latin1_to_utf16.cpp:7 (LE) and :27 (BE), `end - buf >= 16`:
`__lsx_vld(buf, 0)` (:8, :28). -/
def lsx_latin1to16 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem lsx_latin1to16_tied :
    Arch.Lsx.latin1to16.all (Iter.ok (need 16 0 false)) = true ∧
    lsx_latin1to16.N = need 16 0 false ∧ lsx_latin1to16.Reads (need 16 0 false) :=
  ⟨Arch.Lsx.cur_latin1to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lsx/lsx_convert_latin1_to_utf32.cpp:6, `end - buf >= 16`:
`__lsx_vld(buf, 0)` (:7). -/
def lsx_latin1to32 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem lsx_latin1to32_tied :
    Arch.Lsx.latin1to32.all (Iter.ok (need 16 0 false)) = true ∧
    lsx_latin1to32.N = need 16 0 false ∧ lsx_latin1to32.Reads (need 16 0 false) :=
  ⟨Arch.Lsx.cur_latin1to32, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lsx/lsx_convert_latin1_to_utf8.cpp:15, `end - latin1_input >= 16`:
`__lsx_vld(latin1_input, 0)` (:16). The non-ASCII path converts the low 8
bytes of that register. -/
def lsx_latin1to8 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem lsx_latin1to8_tied :
    Kernels.latin1to8_one.all (Iter.ok (need 16 0 false)) = true ∧
    lsx_latin1to8.N = need 16 0 false ∧ lsx_latin1to8.Reads (need 16 0 false) :=
  ⟨Arch.Lsx.cur_latin1to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lsx/lsx_convert_utf16_to_latin1.cpp:6 (plain) and :34 (`_with_errors`),
`end - buf >= 16`: `__lsx_vld(buf, 0)` and `__lsx_vld(buf, 16)` (byte
offsets: units 0 and 8) (:7-8, :35-36); error path `buf[k]`, `k < 16`
(:51-52). -/
def lsx_utf16toLatin1 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8), (0, 16)] }

theorem lsx_utf16toLatin1_tied :
    Arch.Lsx.utf16toLatin1.all (Iter.ok (need 16 0 false)) = true ∧
    lsx_utf16toLatin1.N = need 16 0 false ∧ lsx_utf16toLatin1.Reads (need 16 0 false) :=
  ⟨Arch.Lsx.cur_utf16toLatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lsx/lsx_convert_utf32_to_utf16.cpp:11 (plain) and :88 (`_with_errors`),
`end - buf >= 8`: `__lsx_vld(buf, 0)`, `__lsx_vld(buf, 16)` (units 0 and 4)
(:12-13, :89-90); scalar fallback `forward = 3`, `buf[k]` (:31-37, :113-119). -/
def lsx_utf32to16 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 4), (4, 4)],
    clamped := [(0, 3)] }

theorem lsx_utf32to16_tied :
    Arch.Lsx.utf32to16.all (Iter.ok (need 8 0 false)) = true ∧
    lsx_utf32to16.N = need 8 0 false ∧ lsx_utf32to16.Reads (need 8 0 false) :=
  ⟨Arch.Lsx.cur_utf32to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`:
`simd16x32` at `in + pos` (:11), four 16-byte loads
(src/simdutf/lsx/simd16-inl.h:184-188). -/
def lsx_changeEndianness : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8), (16, 8), (24, 8)] }

theorem lsx_changeEndianness_tied :
    Arch.Lsx.changeEndianness.all (Iter.ok (need 32 0 false)) = true ∧
    lsx_changeEndianness.N = need 32 0 false ∧ lsx_changeEndianness.Reads (need 32 0 false) :=
  ⟨Arch.Lsx.cur_changeEndianness, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/generic/utf16/to_well_formed.h:71 and :78, `for (i = 1; i + 8 < n; i += 8)`:
8 units at `in` and at `in - 1` (`Utf16Fix.genericAcc 8`). -/
def lsx_toWellFormed : Loop :=
  { B := 8,
    m := 0,
    strict := true,
    loads := [(0, 8)],
    back := [(1, 8)],
    start := 1 }

theorem lsx_toWellFormed_tied :
    Arch.Lsx.toWellFormed.all (Iter.ok (need 8 0 true)) = true ∧
    (∀ n, Utf16Fix.AllInB n (Utf16Fix.genericAcc 8 n)) ∧
    lsx_toWellFormed.N = need 8 0 true ∧ lsx_toWellFormed.Reads (need 8 0 true) :=
  ⟨Arch.Lsx.cur_toWellFormed, Utf16Fix.lsx_inBounds, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lsx/lsx_convert_utf32_to_latin1.cpp:8 (plain) and :37 (`_with_errors`),
`end - buf >= 16`, 8 units per iteration (`B = 8`, margin 8):
`__lsx_vld(buf, 0)`, `__lsx_vld(buf, 16)` (units 0 and 4) (:9-10, :38-39);
error path `buf[k]`, `k < 8` (:53-54). The loads need no margin; the 16-byte
store does. -/
def lsx_utf32toLatin1 : Loop :=
  { B := 8,
    m := 8,
    strict := false,
    loads := [(0, 4), (4, 4), (0, 8)] }

theorem lsx_utf32toLatin1_tied :
    Arch.Lsx.utf32toLatin1.all (Iter.ok (need 8 8 false)) = true ∧
    lsx_utf32toLatin1.N = need 8 8 false ∧ lsx_utf32toLatin1.Reads (need 8 8 false) :=
  ⟨Arch.Lsx.cur_utf32toLatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lsx/lsx_convert_utf16_to_utf8.cpp:62 (plain) and :295 (`_with_errors`),
`end - buf >= 16 + 12`: `__lsx_vld(buf, 0)` (:63, :296), `__lsx_vld(buf, 16)`
(units 8..15) on the ASCII test (:71, :304); after the ASCII half `buf += 8`
(:92, :325), so the scalar fallback (`forward = 15`, `buf[k]`, `buf[k + 1]`,
:239-259, :473-493) starts at shift 0 or 8. -/
def lsx_utf16to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 8), (8, 8)],
    clamped := [(0, 15), (8, 15)] }

theorem lsx_utf16to8_tied :
    Kernels.utf16to8_lsx.all (Iter.ok (need 16 12 false)) = true ∧
    lsx_utf16to8.N = need 16 12 false ∧ lsx_utf16to8.Reads (need 16 12 false) :=
  ⟨Arch.Lsx.cur_utf16to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lsx/lsx_convert_utf16_to_utf32.cpp:12 (plain) and :83 (`_with_errors`),
`end - buf >= 8`: `__lsx_vld(buf, 0)` (:13, :84); scalar fallback
`forward = 15`, `buf[k]`, `buf[k + 1]` (:34-47, :103-116), kept in by the
clamp (:36-37, :105-106). -/
def lsx_utf16to32 : Loop :=
  { B := 8,
    m := 0,
    strict := false,
    loads := [(0, 8)],
    clamped := [(0, 15)] }

theorem lsx_utf16to32_tied :
    (Arch.Lsx.utf16to32.all Iter.exact = true) ∧
    lsx_utf16to32.N = need 8 0 false ∧ lsx_utf16to32.Reads (need 8 0 false) :=
  ⟨Arch.Lsx.exact_utf16to32, rfl, reads_of_fits _ _ (by decide)⟩


/-! ## lasx -/

/-- src/lasx/lasx_convert_latin1_to_utf16.cpp:12 (LE) and :50 (BE),
`end - buf >= 32`: `__lasx_xvld(buf, 0)` (:13, :51). The alignment prologue
(:7-10, :45-47) reads `*buf` only while `buf < end`. -/
def lasx_latin1to16 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 32)] }

theorem lasx_latin1to16_tied :
    Arch.Lasx.latin1to16.all (Iter.ok (need 32 0 false)) = true ∧
    lasx_latin1to16.N = need 32 0 false ∧ lasx_latin1to16.Reads (need 32 0 false) :=
  ⟨Arch.Lasx.cur_latin1to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lasx/lasx_convert_latin1_to_utf16.cpp:25 and :63, the block after the
loop, branch `end - buf >= 16`:
`__lsx_vld(buf, 0)` (:27, :65). -/
def lasx_latin1to16_tail : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem lasx_latin1to16_tail_tied :
    Arch.Lasx.latin1to16_tail.all (Iter.ok (need 16 0 false)) = true ∧
    lasx_latin1to16_tail.N = need 16 0 false ∧ lasx_latin1to16_tail.Reads (need 16 0 false) :=
  ⟨Arch.Lasx.cur_latin1to16_tail, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lasx/lasx_convert_latin1_to_utf32.cpp:12, `end - buf >= 32`:
`__lasx_xvld(buf, 0)` (:13). Prologue (:7-10) guarded by `buf < end`. -/
def lasx_latin1to32 : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 32)] }

theorem lasx_latin1to32_tied :
    Arch.Lasx.latin1to32.all (Iter.ok (need 32 0 false)) = true ∧
    lasx_latin1to32.N = need 32 0 false ∧ lasx_latin1to32.Reads (need 32 0 false) :=
  ⟨Arch.Lasx.cur_latin1to32, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lasx/lasx_convert_latin1_to_utf32.cpp:34, the block after the loop,
branch `end - buf >= 16`:
`__lsx_vld(buf, 0)` (:35). -/
def lasx_latin1to32_tail : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)] }

theorem lasx_latin1to32_tail_tied :
    Arch.Lasx.latin1to32_tail.all (Iter.ok (need 16 0 false)) = true ∧
    lasx_latin1to32_tail.N = need 16 0 false ∧ lasx_latin1to32_tail.Reads (need 16 0 false) :=
  ⟨Arch.Lasx.cur_latin1to32_tail, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lasx/lasx_convert_utf16_to_latin1.cpp:6 (plain) and :34
(`_with_errors`), `end - buf >= 16`: `__lsx_vld(buf, 0)`, `__lsx_vld(buf, 16)`
(units 0 and 8) (:7-8, :35-36); error path `buf[k]`, `k < 16` (:51-52).
Table `Lasx.toLatin1`. -/
def lasx_utf16toLatin1 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8), (0, 16)] }

theorem lasx_utf16toLatin1_tied :
    Arch.Lasx.toLatin1.all (Iter.ok (need 16 0 false)) = true ∧
    lasx_utf16toLatin1.N = need 16 0 false ∧ lasx_utf16toLatin1.Reads (need 16 0 false) :=
  ⟨Arch.Lasx.cur_toLatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lasx/lasx_convert_utf32_to_latin1.cpp:9 (plain) and :42
(`_with_errors`), `end - buf >= 16`: `__lasx_xvld(buf, 0)`,
`__lasx_xvld(buf, 32)` (units 0 and 8) (:10-11, :43-44); error path `buf[k]`,
`k < 16` (:60-61). Table `Lasx.toLatin1`. -/
def lasx_utf32toLatin1 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8), (0, 16)] }

theorem lasx_utf32toLatin1_tied :
    Arch.Lasx.toLatin1.all (Iter.ok (need 16 0 false)) = true ∧
    lasx_utf32toLatin1.N = need 16 0 false ∧ lasx_utf32toLatin1.Reads (need 16 0 false) :=
  ⟨Arch.Lasx.cur_toLatin1, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lasx/lasx_convert_utf16_to_utf8.cpp:65 (plain) and :315
(`_with_errors`), `end - buf >= 16 + 12`: `__lasx_xvld(buf, 0)` (:66, :316);
scalar fallback `forward = 15`, `buf[k]`, `buf[k + 1]` (:255-275, :505-525). -/
def lasx_utf16to8 : Loop :=
  { B := 16,
    m := 12,
    strict := false,
    loads := [(0, 16)],
    clamped := [(0, 15)] }

theorem lasx_utf16to8_tied :
    Arch.Lasx.utf16to8.all (Iter.ok (need 16 12 false)) = true ∧
    lasx_utf16to8.N = need 16 12 false ∧ lasx_utf16to8.Reads (need 16 12 false) :=
  ⟨Arch.Lasx.cur_utf16to8, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lasx/lasx_convert_utf32_to_utf16.cpp:43 (plain) and :150
(`_with_errors`), `end - buf >= 16`: `__lasx_xvld(buf, 0)`,
`__lasx_xvld(buf, 32)` (units 0 and 8) (:44-45, :151-152); scalar fallback
`forward = 15`, `buf[k]` (:64-70, :176-182). Prologue (:9-10, :118-119)
guarded by `buf < end`. -/
def lasx_utf32to16 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 8), (8, 8)],
    clamped := [(0, 15)] }

theorem lasx_utf32to16_tied :
    Arch.Lasx.utf32to16.all (Iter.ok (need 16 0 false)) = true ∧
    lasx_utf32to16.N = need 16 0 false ∧ lasx_utf32to16.Reads (need 16 0 false) :=
  ⟨Arch.Lasx.cur_utf32to16, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/generic/utf16/change_endianness.h:10, `pos < size / 32 * 32`:
`simd16x32` at `in + pos` (:11), two 32-byte loads
(src/simdutf/lasx/simd16-inl.h:169-171). -/
def lasx_changeEndianness : Loop :=
  { B := 32,
    m := 0,
    strict := false,
    loads := [(0, 16), (16, 16)] }

theorem lasx_changeEndianness_tied :
    Arch.Lasx.changeEndianness.all (Iter.ok (need 32 0 false)) = true ∧
    lasx_changeEndianness.N = need 32 0 false ∧ lasx_changeEndianness.Reads (need 32 0 false) :=
  ⟨Arch.Lasx.cur_changeEndianness, rfl, reads_of_fits _ _ (by decide)⟩

/-- src/lasx/lasx_convert_utf16_to_utf32.cpp:36 (plain) and :130
(`_with_errors`), `end - buf >= 16`: `__lasx_xvld(buf, 0)` (:37, :131);
scalar fallback `forward = 15`, `buf[k]`, `buf[k + 1]` (:59-72, :153-166).
The alignment prologue reads `buf[0]` while `buf < end`, and `buf[1]` only
after `buf + 1 >= end` returned (:15) or under `buf + 1 < end` (:110). -/
def lasx_utf16to32 : Loop :=
  { B := 16,
    m := 0,
    strict := false,
    loads := [(0, 16)],
    clamped := [(0, 15)] }

theorem lasx_utf16to32_tied :
    (∀ (choose : List Nat → Iter), (∀ xs, 16 ≤ xs.length → choose xs ∈ Arch.Lasx.utf16to32) →
      (∀ xs, choose xs = Arch.Lasx.utf16to32Vec → xs.take 16 = List.replicate 16 1) →
      ∀ n xs, written 16 choose n xs ≤ L xs) ∧
    lasx_utf16to32.N = need 16 0 false ∧ lasx_utf16to32.Reads (need 16 0 false) :=
  ⟨Arch.Lasx.utf16to32_safe, rfl, reads_of_fits _ _ (by decide)⟩

/-! ## Every loop -/

def allLoops : List Loop :=
  [westmere_utf16to8, westmere_latin1to8, westmere_latin1to8_last,
   westmere_latin1to16, westmere_latin1to32, westmere_utf16tolatin1,
   westmere_utf16tolatin1_err, westmere_utf32tolatin1, westmere_utf32tolatin1_err,
   westmere_changeEndianness, westmere_utf16fix, westmere_utf16to32,
   haswell_latin1to16, haswell_latin1to32, haswell_utf16to1,
   haswell_utf16to1_err, haswell_utf32to1, haswell_utf16to8,
   haswell_utf32to8, haswell_swap16, haswell_utf16fix,
   haswell_utf16fix_sse, haswell_utf16to32, icelake_latin1to16,
   icelake_latin1to32, icelake_swap16, icelake_utf16fix,
   icelake_utf16to1, icelake_utf32to1, icelake_utf32to8,
   icelake_utf32to16, icelake_utf16to8, icelake_utf16to32,
   icelake_utf8_block, icelake_utf8_fixed, arm64_latin1to16,
   arm64_latin1to32, arm64_utf16toLatin1, arm64_utf32toLatin1,
   arm64_utf16to8, arm64_utf32to16, arm64_changeEndianness,
   arm64_utf16fix64, arm64_utf16fix16, arm64_utf16to32,
   ppc64_utf16to8, ppc64_utf32to8, ppc64_latin1to8,
   ppc64_latin1to8Last, ppc64_latin1to16, ppc64_latin1to32,
   ppc64_utf16toLatin1, ppc64_utf32toLatin1, ppc64_utf32to16,
   ppc64_changeEndianness, ppc64_utf16to32, lsx_latin1to16,
   lsx_latin1to32, lsx_latin1to8, lsx_utf16toLatin1,
   lsx_utf32to16, lsx_changeEndianness, lsx_toWellFormed,
   lsx_utf32toLatin1, lsx_utf16to8, lsx_utf16to32,
   lasx_latin1to16, lasx_latin1to16_tail, lasx_latin1to32,
   lasx_latin1to32_tail, lasx_utf16toLatin1, lasx_utf32toLatin1,
   lasx_utf16to8, lasx_utf32to16, lasx_changeEndianness,
   lasx_utf16to32]

/-- Every read of every loop lies in the input under its loop test. -/
theorem all_fit : allLoops.all (fun l => l.fits l.N) = true := by decide

theorem count : allLoops.length = 76 := by decide

/-! ## Which loads set a margin

The loads of every loop but two end within the `B` units the iteration
consumes. The two exceptions read ahead of what they consume, and their
margin is exactly that read: Ice Lake UTF-16 → UTF-8 loads 32 units and
consumes 31 (`Icelake.read_utf16to8`), and the Ice Lake fixed-length UTF-8
decoders load 64 + 4 bytes and consume 64. -/

theorem beyond_B :
    (allLoops.filter fun l => !(l.fits l.B)).length = 2 ∧
    icelake_utf16to8.fits 31 = false ∧ icelake_utf8_fixed.fits 64 = false := by
  decide

/-- One margin lower: every loop with `m > 0` still fits its loads, except the
two above, where one less margin reads one unit past the input. So the stores
set every other margin (`wit_*`), and the loads set those two. -/
theorem lower_fits :
    (allLoops.filter fun l =>
        0 < l.m && !(l.fits (need l.B (l.m - 1) l.strict))).length = 2 ∧
    icelake_utf16to8.fits (need 31 0 false) = false := by
  decide

/-- With the margin of the `+ 4` lowered to 3 (`end - ptr >= 67`), the
4-byte read at `ptr + 64` would end one byte past a 67-byte input. -/
theorem icelake_utf8_fixed_tight :
    icelake_utf8_fixed.fits (need 64 3 false) = false ∧
    icelake_utf8_fixed.reach = need 64 4 false := by
  decide

/-- Margin-0 loops with a `>=` test: the widest load ends exactly at `B`. -/
theorem zero_margin_tight :
    (allLoops.filter fun l => l.m == 0 && !l.strict).all
      (fun l => l.reach == l.B) = true := by
  decide

/-- The scalar fallbacks whose cap reaches past the guaranteed units, where
the clamp `forward = end - buf - 1` is what keeps `buf[k + 1]` in: the
8-unit UTF-16 → UTF-32 loops (westmere, arm64, ppc64, lsx; `forward = 15`). -/
theorem clamp_needed :
    (allLoops.filter Loop.clampNeeded).length = 4 ∧
    westmere_utf16to32.clampNeeded = true ∧ arm64_utf16to32.clampNeeded = true ∧
    ppc64_utf16to32.clampNeeded = true ∧ lsx_utf16to32.clampNeeded = true := by
  decide

/-! ## Masked loads (Ice Lake)

Each tail after an Ice Lake loop loads with the mask `(1 << r) - 1`
(`_bzhi_u32(0xFFFFFFFF, r)`, `_bzhi_u64(~0, r)`, `(uint64_t(1) << r) - 1`),
`r` the units that remain, `r < W` for a register of `W` lanes. An AVX-512
masked load does not access a masked-off lane (it does not fault there), so
it reads lanes `j < r` only. -/

/-- The mask `(1 << r) - 1`. -/
def lowMask (r : Nat) : Nat := 2 ^ r - 1

/-- A lane is active exactly when it is below `r`: the masked load reads
`buf[0, r)`, inside the `r` remaining units. -/
theorem masked_in (r j : Nat) (h : (lowMask r).testBit j = true) : j < r := by
  unfold lowMask at h
  rw [Nat.testBit_two_pow_sub_one] at h
  simpa using h

/-- `(W, bits, r_max)` of every masked tail: `W` lanes, a mask built in a
`bits`-bit integer, `r ≤ r_max` remaining units.
* src/icelake/icelake_convert_latin1_to_utf16.inl.cpp:22-24 (`(32, 32, 31)`);
* icelake_convert_latin1_to_utf32.inl.cpp:19-20 (`(16, 32, 15)`, `1U << len`);
* icelake_convert_utf16_to_latin1.inl.cpp:30-31, :82-83 (`(32, 32, 31)`);
* icelake_convert_utf32_to_latin1.inl.cpp:22-23, :60-61 (`(16, 32, 15)`);
* icelake_convert_utf32_to_utf16.inl.cpp:87-88, :258-259 (`(16, 32, 15)`);
* icelake_convert_utf16_to_utf32.inl.cpp:16-17 (`(32, 32, 31)`, `_bzhi_u32`);
* icelake_convert_utf16_to_utf8.inl.cpp:195-196 (`(32, 32, 31)`);
* src/icelake/implementation.cpp:1621-1623, `change_endianness_utf16` (`(32, 32, 31)`);
* icelake_utf8_common.inl.cpp:48-51, `SIMDUTF_TAIL`, `gap < 64` (`(64, 64, 63)`);
* icelake_convert_utf8_to_latin1.inl.cpp:9-11 and
  icelake_convert_valid_utf8_to_latin1.inl.cpp:9-11, `_bzhi_u64` (`(64, 64, 63)`);
* icelake_convert_latin1_to_utf8.inl.cpp:103-104 is in `Loads.lean`. -/
def maskedTails : List (Nat × Nat × Nat) :=
  [(32, 32, 31), (16, 32, 15), (32, 32, 31), (16, 32, 15), (16, 32, 15),
   (32, 32, 31), (32, 32, 31), (32, 32, 31), (64, 64, 63), (64, 64, 63)]

/-- Every tail mask covers at most the register (`r_max < W`) and its shift
`1 << r` is below the width of the integer it is computed in. -/
theorem masked_tails :
    maskedTails.all (fun t => decide (t.2.2 < t.1 ∧ t.2.2 < t.2.1)) = true := by
  decide

/-- The validation tail of the fixed-length UTF-8 decoders,
`~UINT64_C(0) >> (64 - (end - validatedptr))` under `end != validatedptr`
(src/icelake/icelake_from_utf8.inl.cpp:119-121, :255-257): after the loop
`end - ptr < 68`; the block at `end - ptr >= 64` moves `validatedptr` by 64.
So `k = end - validatedptr` is between 1 and 63, the shift is between 1 and
63, and the mask is `lowMask k`. -/
theorem fixed_tail_lanes (d k : Nat) (hd : d < 68)
    (hk : k = if 64 ≤ d then d - 64 else d) (h0 : k ≠ 0) :
    1 ≤ k ∧ k ≤ 63 ∧ 1 ≤ 64 - k ∧ 64 - k ≤ 63 := by
  split at hk <;> omega

theorem fixed_tail_mask :
    ∀ k < 64, 1 ≤ k → (2 ^ 64 - 1) >>> (64 - k) = lowMask k := by
  decide

end SafetyMargins.LoadsAll
