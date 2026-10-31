import SafetyMargins.Tables
/-
Facts about the lookup tables, checked against the data extracted from the C++
sources by `extract_tables.py`. They justify the store positions used in
`Kernels.lean` and `Utf8.lean`: a table-driven path advances the output by
exactly the length-function value of what it converted.
-/
namespace SafetyMargins.TableFacts
open SafetyMargins.Tables

def bit (n i : Nat) : Nat := n / 2 ^ i % 2

/-- Number of set bits among the low `w` bits of `n`. -/
def popcount : Nat → Nat → Nat
  | 0, _ => 0
  | w + 1, n => bit n w + popcount w n

/-- `pack_1_2_utf8_bytes`: the index has one bit per code unit, set when the
unit is ASCII (1 byte), clear when it takes 2 bytes. The byte count stored in
the row is exactly the sum: 8 ASCII units give 8, none give 16. -/
theorem pack12_exact :
    (List.range 256).all (fun i => pack12Count[i]! == 16 - popcount 8 i) = true := by
  native_decide

/-- Byte length of one 2-bit lane of a `pack_1_2_3_utf8_bytes` index:
`11` ASCII, `10` two bytes, `00` three bytes (`01` cannot occur, since a unit
below 0x80 is also below 0x800). -/
def laneLen (l : Nat) : Nat := if l = 3 then 1 else if l = 2 then 2 else 3

def lane (i j : Nat) : Nat := i / 4 ^ j % 4

def reachable123 (i : Nat) : Bool :=
  (List.range 4).all fun j => lane i j != 1

/-- `pack_1_2_3_utf8_bytes`: for every reachable index the row's byte count is
the sum of the four lane lengths, so the 1/2/3-byte path advances by exactly
L(4 units), which is at least 4. -/
theorem pack123_exact :
    (List.range 256).all (fun i =>
      !reachable123 i ||
        pack123Count[i]! == laneLen (lane i 0) + laneLen (lane i 1)
          + laneLen (lane i 2) + laneLen (lane i 3)) = true := by
  native_decide

theorem pack123_ge4 :
    (List.range 256).all (fun i => !reachable123 i || decide (4 ≤ pack123Count[i]!)) = true := by
  native_decide

/-- Code points produced by a `utf8bigindex` entry: 6, 4 or 3 depending on the
shuffle index; entries at 209 and above are errors. -/
def units (idx : Nat) : Nat := if idx < 64 then 6 else if idx < 145 then 4 else 3

/-- `utf8bigindex`: the index is the 12-bit end-of-code-point mask (bit `k` set
when byte `k + 1` starts a new character). For every non-error entry, the
consumed bytes end exactly at a code point boundary (bit `consumed - 1` set),
and the number of code points produced equals the number of code-point ends
inside the consumed bytes. Error entries consume 12 bytes (and the kernels
store nothing for them). Consequently, for a call starting at a leading byte,
the code points produced equal the leading bytes consumed, and the byte right
after the consumed ones is a leading byte. -/
theorem bigindex_exact :
    (List.range 4096).all (fun m =>
      let i := bigIdx[m]!
      let c := bigConsumed[m]!
      if i < 209 then
        decide (1 ≤ c) && bit m (c - 1) == 1 && popcount c m == units i
      else c == 12) = true := by
  native_decide

/-- No row consumes more than the 12 bytes its mask describes. -/
theorem bigconsumed_le :
    (List.range 4096).all (fun m => decide (bigConsumed[m]! ≤ 12)) = true := by
  native_decide

/-- Start of code point `j` of a three-code-point row of `shufutf8`: 0, then one
past the last byte of the previous one (entry `4 (j - 1)`). -/
def start3 (i j : Nat) : Nat := if j = 0 then 0 else shuf[16 * i + 4 * (j - 1)]! + 1

/-- For every mask whose row has three code points (`145 ≤ idx < 209`): the
second and third start where the mask has a code-point end just before, in
increasing order inside the consumed bytes; and the top byte of each 32-bit
lane is either zero (shuffle index at least 16) or the first byte of its code
point. -/
theorem shuf3_starts :
    (List.range 4096).all (fun m =>
      let i := bigIdx[m]!
      let c := bigConsumed[m]!
      !(145 ≤ i && i < 209) ||
        (bit m (start3 i 1 - 1) == 1 && bit m (start3 i 2 - 1) == 1 &&
          decide (0 < start3 i 1) && decide (start3 i 1 < start3 i 2) &&
          decide (start3 i 2 < c) &&
          (List.range 3).all fun j =>
            decide (16 ≤ shuf[16 * i + 4 * j + 3]!) || shuf[16 * i + 4 * j + 3]! == start3 i j)) = true := by
  native_decide

/-- A mask with no run of four zero bits: every code point it describes is at
most 4 bytes long (four consecutive continuation bytes never pass the check). -/
def wellShaped (m : Nat) : Bool :=
  (List.range 9).all fun j => bit m j + bit m (j + 1) + bit m (j + 2) + bit m (j + 3) != 0

/-- No error row on a well-shaped mask. -/
theorem well_noerr :
    (List.range 4096).all (fun m => !wellShaped m || decide (bigIdx[m]! < 209)) = true := by
  native_decide

/-- A row below 145 never consumes a 4-byte character: no code-point start
`j` inside the consumed bytes is followed by three continuation bytes. -/
theorem well_145 :
    (List.range 4096).all (fun m =>
      !wellShaped m || !decide (bigIdx[m]! < 145) ||
        (List.range bigConsumed[m]!).all fun j =>
          !(j == 0 || bit m (j - 1) == 1) || !decide (j + 2 < 12) ||
            bit m j + bit m (j + 1) + bit m (j + 2) != 0) = true := by
  native_decide

/-- End of code point `j` of a three-code-point row (`start3` of the next one,
or the consumed count for the last). -/
def end3 (i c j : Nat) : Nat := if j < 2 then start3 i (j + 1) else c

/-- On a well-shaped three-code-point row, a 4-byte lane gathers its bytes in
reverse order (lead in the top byte), and a shorter lane has a zero top byte. -/
theorem well_3 :
    (List.range 4096).all (fun m =>
      let i := bigIdx[m]!
      let c := bigConsumed[m]!
      !wellShaped m || !(145 ≤ i && i < 209) ||
        (List.range 3).all fun j =>
          let s := start3 i j
          if end3 i c j = s + 4 then
            shuf[16 * i + 4 * j]! == s + 3 && shuf[16 * i + 4 * j + 1]! == s + 2 &&
              shuf[16 * i + 4 * j + 2]! == s + 1 && shuf[16 * i + 4 * j + 3]! == s
          else decide (16 ≤ shuf[16 * i + 4 * j + 3]!)) = true := by
  native_decide

/-- Every `shufutf8` entry is a byte index below 16 or `0xFF`, which pshufb,
`vqtbl1q` and the masked `vshuf_b` all read as zero. -/
theorem shuf_entries :
    (List.range (209 * 16)).all (fun k => decide (shuf[k]! < 16) || shuf[k]! == 255) = true := by
  native_decide

end SafetyMargins.TableFacts
