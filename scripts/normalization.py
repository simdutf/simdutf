#!/usr/bin/env python3

import sys
from dataclasses import dataclass
from itertools import batched
from typing import Collection

BMP_LIMIT = 0x10000

SHIFT = 6
BLOCK_LENGTH = 1 << SHIFT
MASK = BLOCK_LENGTH - 1

FLAG_UNSEEN = 0
FLAG_SEEN = 1


# Simple trie for 16-bit code points. We use perfect hash tables for code points in the
# supplementary plane. If better compression or faster lookups are possible using a more
# complex trie (like what ICU uses), we should consider that instead for the supplementary
# plane. We could even try compressed bitmap?
class Trie:
    def __init__(self) -> None:
        self.index: list[int] = [0] * (BMP_LIMIT >> SHIFT)
        self.data: list[int] = []
        self._flags: list[int] = [FLAG_UNSEEN] * (BMP_LIMIT >> SHIFT)

    def get(self, c: int) -> int:
        i = c >> SHIFT
        return self.data[self.index[i] + (c & MASK)]

    def set(self, c: int, value: int) -> None:
        assert c < BMP_LIMIT
        i = c >> SHIFT
        block: int
        # Already exists as an index
        if self._flags[i] == FLAG_SEEN:
            block = self.index[i]
        else:
            block = self._alloc_new_block(i)
        self.data[block + (c & MASK)] = value

    def compact(self) -> None:
        compressed: list[int] = []

        # Keeps track of blocks that we've seen before
        blocks: dict[tuple, int] = {}
        for i in range(len(self.index)):
            block_index = self.index[i]
            block_slice = self.data[block_index : block_index + BLOCK_LENGTH]
            block = tuple(block_slice)
            # Check if we can fully deduplicate this block
            if block in blocks:
                self.index[i] = blocks[block]
                continue

            # If we can't deduplicate the block, then see if we can merge it
            # partially with the previous block
            overlap = longest_overlap(compressed, block_slice, BLOCK_LENGTH)
            if overlap > 0:
                index = len(compressed) - overlap
                compressed.extend(block_slice[overlap:])
                self.index[i] = index
                blocks[block] = index
                continue

            # If we can't do either of the above compaction operations, we
            # should allocate the new data
            self.index[i] = len(compressed)
            blocks[block] = len(compressed)
            compressed.extend(block)

        self.data = compressed

    def _alloc_new_block(self, i: int) -> int:
        new_block = len(self.data)
        self.data.extend([0] * BLOCK_LENGTH)
        self.index[i] = new_block
        self._flags[i] = FLAG_SEEN
        return new_block


def longest_overlap(lst1: list, lst2: list, max_overlap: int) -> int:
    overlap = max_overlap - 1
    if overlap > len(lst1):
        return 0
    while overlap > 0 and lst1[-overlap:] != lst2[:overlap]:
        overlap -= 1
    return overlap


@dataclass
class DecompValue:
    decomps: list[int]
    ccc: int


DecompMap = dict[int, DecompValue]
CompMap = dict[tuple[int, int], int]


# Helper to recursively decompose a Unicode character `c`
def expand(c: int, map: DecompMap) -> list[int]:
    expansion = []
    stack = [c]
    while stack:
        x = stack.pop()
        if (
            x not in map
            or not map[x].decomps
            or (len(map[x].decomps) == 1 and x == map[x].decomps[0])
        ):
            expansion.append(x)
        elif map[x]:
            stack.extend(reversed(map[x].decomps))
    return expansion


# Credit: unicode-rs/unicode-normalization
def my_hash(x: int, salt: int, n: int) -> int:
    # This is hash based on the theory that multiplication is efficient
    mask_32 = 0xFFFFFFFF
    y = ((x + salt) * 2654435769) & mask_32
    y ^= (x * 0x31415926) & mask_32
    return (y * n) >> 32


# Credit: unicode-rs/unicode-normalization
def minimal_perfect_hash(d: Collection[int]) -> tuple[list[int], list[int]]:
    n = len(d)
    buckets: dict[int, list[int]] = dict((h, []) for h in range(n))
    for key in d:
        h = my_hash(key, 0, n)
        buckets[h].append(key)
    bsorted = [(len(buckets[h]), h) for h in range(n)]
    bsorted.sort(reverse=True)
    claimed = [False] * n
    salts = [0] * n
    keys = [0] * n
    for bucket_size, h in bsorted:
        # Note: the traditional perfect hashing approach would also special-case
        # bucket_size == 1 here and assign any empty slot, rather than iterating
        # until rehash finds an empty slot. But we're not doing that so we can
        # avoid the branch.
        if bucket_size == 0:
            break
        else:
            for salt in range(1, 32768):
                rehashes = [my_hash(key, salt, n) for key in buckets[h]]
                # Make sure there are no rehash collisions within this bucket.
                if all(not claimed[hash] for hash in rehashes):
                    if len(set(rehashes)) < bucket_size:
                        continue
                    salts[h] = salt
                    for key in buckets[h]:
                        rehash = my_hash(key, salt, n)
                        claimed[rehash] = True
                        keys[rehash] = key
                    break
            if salts[h] == 0:
                print("minimal perfect hashing failed")
                # Note: if this happens (because of unfortunate data), then there are
                # a few things that could be done. First, the hash function could be
                # tweaked. Second, the bucket order could be scrambled (especially the
                # singletons). Right now, the buckets are sorted, which has the advantage
                # of being deterministic.
                #
                # As a more extreme approach, the singleton bucket optimization could be
                # applied (give the direct address for singleton buckets, rather than
                # relying on a rehash). That is definitely the more standard approach in
                # the minimal perfect hashing literature, but in testing the branch was a
                # significant slowdown.
                exit(1)
    return salts, keys


ELEMENT_SIZES = {
    "uint8_t": 1,
    "uint16_t": 2,
    "uint32_t": 4,
    "uint64_t": 8,
    "HangulShuf": 25,
}


class Packed:
    def __init__(self, *fields) -> None:
        self._fields = []
        for value, bits in fields:
            assert 0 <= value < (1 << bits)
            self._fields.append((value, bits))

    def bits(self) -> int:
        return sum(bits for _, bits in self._fields)

    def to_int(self, n) -> int:
        assert n >= self.bits()
        result = 0
        shift = 0
        for value, bits in self._fields:
            result |= value << shift
            shift += bits
        assert result < (1 << n)
        return result


class HeaderDef:
    def __init__(self, name: str, type_: str, array_sizes: list[int] | None = None):
        self.name = name
        self.type_ = type_
        if array_sizes is not None:
            self.array_sizes: list[int] = array_sizes
        else:
            self.array_sizes: list[int] = []

    @classmethod
    def array(cls, name: str, type_: str, array_size: int):
        inst = cls(name, type_)
        inst.array_sizes.append(array_size)
        return inst

    @classmethod
    def multi_array(cls, name: str, type_: str, array_sizes: list[int]):
        inst = cls(name, type_)
        inst.array_sizes = array_sizes
        return inst

    def size(self) -> int:
        product = ELEMENT_SIZES[self.type_]
        for size in self.array_sizes:
            product *= size
        return product


def prefix(headers: list[HeaderDef], prefix: str) -> list[HeaderDef]:
    return [
        HeaderDef(f"{prefix}::{header.name}", header.type_, header.array_sizes)
        for header in headers
    ]


def generate_array(writer, name: str, data: list[int], data_width: int) -> HeaderDef:
    writer.write(f"\nconst uint{data_width}_t {name}[{len(data)}] = {{\n")
    for row in batched(data, 10):
        writer.write(" ")
        for x in row:
            assert x < 2**data_width
            if data_width == 32:
                writer.write(f" 0x{x:08X},")
            elif data_width == 16:
                writer.write(f" 0x{x:04X},")
            elif data_width == 8:
                writer.write(f" 0x{x:02X},")
            else:
                raise ValueError(f"Unknown data width: {data_width}")
        writer.write("\n")
    writer.write("};\n")
    return HeaderDef.array(name, f"uint{data_width}_t", len(data))


def generate_decomp_hash_table(
    writer, decomp_map: DecompMap, qc: dict[int, str], name: str
) -> list[HeaderDef]:
    supplementary_map: DecompMap = {k: v for k, v in decomp_map.items() if k > 0xFFFF}
    # Right now, we are putting all qc elements in the map. This is because we want to use this
    # table to check if a certain character is relevant to NF(K)C
    for x in qc:
        if x not in supplementary_map and x > 0xFFFF:
            supplementary_map[x] = DecompValue([x], 0)

    offsets = {}
    lengths = {}
    offset = 0
    all_decomps = []
    for k, decomp in supplementary_map.items():
        offsets[k] = offset
        all_decomps.extend(decomp.decomps)
        lengths[k] = len(decomp.decomps)
        offset += len(decomp.decomps)
    assert offset <= 2**16 - 1

    writer.write(f"const uint32_t {name}_chars[{len(all_decomps)}] = {{\n")
    for row in batched(all_decomps, 13):
        writer.write(" ")
        for b in row:
            writer.write(f" 0x{b:08X},")
        writer.write("\n")
    writer.write("};\n")

    decomp_salt, decomp_keys = minimal_perfect_hash(supplementary_map)
    writer.write(f"\nconst uint16_t {name}_salt[{len(decomp_salt)}] = {{\n")
    for salts in batched(decomp_salt, 14):
        writer.write(" ")
        for s in salts:
            writer.write(f" 0x{s:04X},")
        writer.write("\n")
    writer.write("};\n")

    writer.write(f"\nconst uint64_t {name}_kv[{len(decomp_keys)}] = {{\n")
    for batch in batched(decomp_keys, 8):
        writer.write(" ")
        for k in batch:
            decomp = supplementary_map[k]
            ccc_vals = [
                decomp_map.get(a, DecompValue([], 0)).ccc for a in decomp.decomps
            ]
            last_ccc = ccc_vals[-1]
            if (
                len(decomp.decomps) > 1
                and any(ccc < last_ccc and ccc != 0 for ccc in ccc_vals)
                and ccc_vals[0] != 0
            ):
                print("Detected complex ccc decomposition!")
                sys.exit(1)
            ccc = supplementary_map[k].ccc
            qc_value = 0
            if supplementary_map[k].decomps[0] != k:
                qc_value |= 0b01
            if k in qc:
                qc_value |= 0b10
            assert lengths[k] < 4
            packed = Packed(
                (k, 21),
                (offsets[k], 16),
                (last_ccc, 8),
                (ccc, 8),
                (lengths[k], 2),
                (int(supplementary_map[k].decomps[0] != k), 1),
                (int(k in qc), 1),
            )
            writer.write(f"  {packed.to_int(64)},")
        writer.write("\n")
    writer.write("};\n")

    return [
        HeaderDef.array(f"{name}_chars", "uint32_t", len(all_decomps)),
        HeaderDef.array(f"{name}_salt", "uint16_t", len(decomp_salt)),
        HeaderDef.array(f"{name}_kv", "uint64_t", len(decomp_keys)),
    ]


def generate_supplementary_ccc_hash_table(
    writer, decomp_map: DecompMap
) -> list[HeaderDef]:
    supplementary_ccc_map: DecompMap = {
        k: v for k, v in decomp_map.items() if k > 0xFFFF and v.ccc > 0
    }

    salt, keys = minimal_perfect_hash(supplementary_ccc_map)
    writer.write(f"\nconst uint16_t ccc_salt[{len(salt)}] = {{\n")
    for salts in batched(salt, 14):
        writer.write(" ")
        for s in salts:
            writer.write(f" 0x{s:04X},")
        writer.write("\n")
    writer.write("};\n")

    writer.write(f"\nconst uint32_t ccc_kv[{len(keys)}] = {{\n")
    for batch in batched(keys, 13):
        writer.write(" ")
        for k in batch:
            packed = Packed((k, 21), (supplementary_ccc_map[k].ccc, 8))
            writer.write(f" {packed.to_int(32)},")
        writer.write("\n")
    writer.write("};\n")

    return [
        HeaderDef.array("ccc_salt", "uint16_t", len(salt)),
        HeaderDef.array("ccc_kv", "uint32_t", len(keys)),
    ]


def generate_comp_hash_table(writer, comp_map: CompMap) -> list[HeaderDef]:
    comp_table = {}
    for (c1, c2), x in comp_map.items():
        if c1 <= 0xFFFF and c2 <= 0xFFFF:
            comp_table[(c1 << 16) | c2] = x
    comp_salt, comp_keys = minimal_perfect_hash(comp_table)
    writer.write(f"\nconst uint16_t compose_salt[{len(comp_salt)}] = {{\n")
    for salts in batched(comp_salt, 14):
        writer.write(" ")
        for s in salts:
            writer.write(f" 0x{s:04X},")
        writer.write("\n")
    writer.write("};\n")
    writer.write(f"\nconst uint32_t compose_kv[{len(comp_keys)}][2] = {{\n")
    for batch in batched(comp_keys, 8):
        writer.write(" ")
        for k in batch:
            comp = comp_table[k]
            writer.write(f" {{0x{comp:05X}, 0x{k:08X}}},")
        writer.write("\n")
    writer.write("};\n")

    # Composition for code points in the supplementary plane. There are very few of these,
    # so we can just create a branch for all of them.
    writer.write("\nuint32_t compose_supplementary(uint32_t c1, uint32_t c2) {\n")
    for (c1, c2), x in comp_map.items():
        if c1 <= 0xFFFF and c2 <= 0xFFFF:
            continue
        writer.write(f"  if (c1 == 0x{c1:08X} && c2 == 0x{c2:08X}) return 0x{x:08X};\n")
    writer.write("  return 0;\n}\n")

    return [
        HeaderDef.array("compose_salt", "uint16_t", len(comp_salt)),
        HeaderDef.multi_array("compose_kv", "uint32_t", [len(comp_keys), 2]),
    ]


def generate_pack_hangul(writer) -> list[HeaderDef]:
    writer.write(f"\nconst HangulShuf pack_hangul[16] = {{\n")
    for x in range(1 << 4):
        exclude = []
        total_size = 0
        for i, bit in enumerate(reversed(f"{x:04b}")):
            if bit == "1":
                exclude.append(4 + i * 6)
                exclude.append(5 + i * 6)
                total_size += 6
            else:
                total_size += 9
        tbl = [x for x in range(24) if x not in exclude]
        # Pad to be 24 in length
        tbl.extend([255] * (24 - len(tbl)))
        writer.write(f"  {{{total_size}, {{{", ".join(map(str, tbl))}}}}},\n")
    writer.write("};\n")
    return [HeaderDef.array("pack_hangul", "HangulShuf", 16)]


def generate_trie(
    writer, name: str, trie: Trie, index_width: int, data_width: int
) -> list[HeaderDef]:
    return [
        generate_array(writer, name + "_index", trie.index, index_width),
        generate_array(writer, name + "_data", trie.data, data_width),
    ]


S_BASE = 0xAC00
L_BASE = 0x1100
V_BASE = 0x1161
T_BASE = 0x11A7
L_COUNT = 19
V_COUNT = 21
T_COUNT = 28
N_COUNT = V_COUNT * T_COUNT
S_COUNT = L_COUNT * N_COUNT

NORMALIZATION_PREAMBLE = """// This file was generated by scripts/normalization.py
#ifndef SIMDUTF_NORMALIZATION_TABLES_H
#define SIMDUTF_NORMALIZATION_TABLES_H

namespace simdutf {
namespace {
namespace tables {
namespace normalization {

"""

NORMALIZATION_POSTAMBLE = """} // namespace normalization
} // namespace tables
} // unnamed namespace
} // namespace simdutf

#endif // SIMDUTF_NORMALIZATION_TABLES_H
"""

UTF8_TO_DECOMPOSED_PREAMBLE = """// This file was generated by scripts/normalization.py
#ifndef SIMDUTF_UTF8_TO_DECOMPOSED_TABLES_H
#define SIMDUTF_UTF8_TO_DECOMPOSED_TABLES_H

namespace simdutf {
namespace {
namespace tables {
namespace utf8_to_decomposed {

struct HangulShuf {
  uint8_t len;
  uint8_t tbl[24];
};

"""

UTF8_TO_DECOMPOSED_POSTAMBLE = """} // namespace utf8_to_decomposed
} // namespace tables
} // unnamed namespace
} // namespace simdutf

#endif // SIMDUTF_UTF8_TO_DECOMPOSED_TABLES_H
"""

UTF8_TO_COMPOSED_PREAMBLE = """// This file was generated by scripts/normalization.py
#ifndef SIMDUTF_UTF8_TO_COMPOSED_TABLES_H
#define SIMDUTF_UTF8_TO_COMPOSED_TABLES_H

namespace simdutf {
namespace {
namespace tables {
namespace utf8_to_composed {

"""

UTF8_TO_COMPOSED_POSTAMBLE = """} // namespace utf8_to_composed
} // namespace tables
} // unnamed namespace
} // namespace simdutf

#endif // SIMDUTF_UTF8_TO_COMPOSED_TABLES_H
"""

UTF16_TO_DECOMPOSED_PREAMBLE = """// This file was generated by scripts/normalization.py
#ifndef SIMDUTF_UTF16_TO_DECOMPOSED_TABLES_H
#define SIMDUTF_UTF16_TO_DECOMPOSED_TABLES_H

namespace simdutf {
namespace {
namespace tables {
namespace utf16_to_decomposed {

"""

UTF16_TO_DECOMPOSED_POSTAMBLE = """} // namespace utf16_to_decomposed
} // namespace tables
} // unnamed namespace
} // namespace simdutf

#endif // SIMDUTF_UTF16_TO_DECOMPOSED_TABLES_H
"""


UTF16_TO_COMPOSED_PREAMBLE = """// This file was generated by scripts/normalization.py
#ifndef SIMDUTF_UTF16_TO_COMPOSED_TABLES_H
#define SIMDUTF_UTF16_TO_COMPOSED_TABLES_H

namespace simdutf {
namespace {
namespace tables {
namespace utf16_to_composed {

"""

UTF16_TO_COMPOSED_POSTAMBLE = """} // namespace utf16_to_composed
} // namespace tables
} // unnamed namespace
} // namespace simdutf

#endif // SIMDUTF_UTF16_TO_COMPOSED_TABLES_H
"""


# Load NFD and NFKD decomposition maps from `UnicodeData.txt`.
def load_decomp_maps() -> tuple[DecompMap, DecompMap]:
    nfd_map: DecompMap = {}
    nfkd_map: DecompMap = {}

    # Read in UnicodeData.txt and generate a decomposition mapping
    with open("ucd/UnicodeData.txt", "r") as f:
        for line in f:
            info = line.split(";")
            value = int(info[0], 16)
            mappings = info[5].split(" ")
            ccc = int(info[3])

            if ccc > 0:
                # Add all CCC > 0 characters to the decomp map
                nfd_map[value] = DecompValue([value], ccc)
                nfkd_map[value] = DecompValue([value], ccc)

            # Skip decomp if there is nothing or if it is a compatibility decomposition
            if mappings[0] == "":
                continue

            if mappings[0].startswith("<"):
                assert len(mappings) > 1
                nfkd_map[value] = DecompValue([int(x, 16) for x in mappings[1:]], ccc)
                continue

            nfd_map[value] = DecompValue([int(x, 16) for x in mappings], ccc)
            nfkd_map[value] = DecompValue([int(x, 16) for x in mappings], ccc)

    return nfd_map, nfkd_map


def create_decomp_trie_utf8(
    decomp_map: DecompMap,
    offsets: dict[int, int],
    decomp_bound: int,
) -> tuple[Trie, Trie]:
    trie = Trie()
    decomp_trie = Trie()
    for x in range(BMP_LIMIT):
        try:
            size = len(chr(x).encode("UTF-8"))
        except UnicodeEncodeError:
            continue
        if x not in decomp_map:
            trie.set(x, size)
            decomp_trie.set(x, 0)
            continue
        decomp = decomp_map[x]
        offset = offsets[x]
        # We use the lower 16 bits for the offset into the data table
        assert offset <= 0xFFFF
        length = 0
        for c in decomp.decomps:
            length += len(chr(c).encode("UTF-8"))
        assert length <= decomp_bound
        decomp_delta = length - len(chr(x).encode("UTF-8"))
        final_decomp = min(decomp_delta, 15)
        first_ccc = 0
        last_ccc = 0
        if decomp.decomps[-1] in decomp_map:
            last_ccc = decomp_map[decomp.decomps[-1]].ccc
            ccc_vals = [
                decomp_map.get(a, DecompValue([], 0)).ccc for a in decomp.decomps
            ]
            if (
                len(ccc_vals) > 1
                and any(ccc < last_ccc and ccc != 0 for ccc in ccc_vals)
                and ccc_vals[0] != 0
            ):
                first_ccc = ccc_vals[0]
                assert last_ccc - first_ccc in range(0, 8)
                final_decomp = 15
        # Delta decomposition can only be done with relatively small decomp
        # lengths (<= 8). A `final_decomp` value of 15 indicates that the
        # code point definitely cannot be delta decomposed, and thus the
        # `decomp_trie` trie should be used to get length information.
        if length > 8:
            final_decomp = 15
        packed = Packed(
            (size, 2),
            (last_ccc, 8),
            (int(decomp.decomps[0] != x), 1),
            (final_decomp & 0x1F, 5),
        )
        trie.set(x, packed.to_int(16))
        assert length <= 0x3F
        assert offset <= 0x7FFF
        ccc_delta = 0 if first_ccc == 0 else last_ccc - first_ccc
        assert ccc_delta <= 0b111
        packed = Packed((offset, 15), (length, 6), (last_ccc, 8), (ccc_delta, 3))
        decomp_trie.set(x, packed.to_int(32))
    trie.compact()
    decomp_trie.compact()
    return trie, decomp_trie


def create_decomp_trie_utf16(
    decomp_map: DecompMap, offsets: dict[int, int], decomp_bound: int
) -> Trie:
    trie = Trie()
    for x in range(0x10000):
        if x not in decomp_map:
            trie.set(x, 0)
            continue
        decomp = decomp_map[x]
        offset = offsets[x]
        # We use the lower 14 bits for the offset into the data table
        assert offset <= 0x3FFF
        length = 0
        for c in decomp.decomps:
            length += len(chr(c).encode("UTF-16LE")) // 2
        assert length <= decomp_bound
        delta = length - 1
        assert delta >= 0
        first_ccc = 0
        last_ccc = 0
        if decomp.decomps[-1] in decomp_map:
            last_ccc = decomp_map[decomp.decomps[-1]].ccc
            ccc_vals = [
                decomp_map.get(a, DecompValue([], 0)).ccc for a in decomp.decomps
            ]
            if (
                len(ccc_vals) > 1
                and any(ccc < last_ccc and ccc != 0 for ccc in ccc_vals)
                and ccc_vals[0] != 0
            ):
                first_ccc = ccc_vals[0]
                assert last_ccc - first_ccc in range(0, 8)
        ccc_delta = 0 if first_ccc == 0 else last_ccc - first_ccc
        assert ccc_delta <= 0b111
        packed = Packed(
            (offset, 14),
            (delta, 6),
            (ccc_delta, 3),
            (int(decomp.decomps[0] != x), 1),
            (last_ccc, 8),
        )
        trie.set(x, packed.to_int(32))
    trie.compact()
    return trie


def create_decomp_check_trie(decomp_map: DecompMap, encoding: str) -> Trie:
    trie = Trie()
    for x in range(BMP_LIMIT):
        if x not in decomp_map:
            has_decomp = False
            if x >= S_BASE and x < S_BASE + S_COUNT:
                # Handle Hangul syllables
                s_index = x - S_BASE
                jamo_size = len(chr(L_BASE).encode(encoding))
                length = 2 * jamo_size if s_index % T_COUNT == 0 else 3 * jamo_size
                has_decomp = True
            else:
                try:
                    length = len(chr(x).encode(encoding))
                except UnicodeEncodeError:
                    # Python throws an error if we try to encode a surrogate
                    # in UTF-8
                    continue
        else:
            length = 0
            decomp = decomp_map[x]
            for c in decomp.decomps:
                length += len(chr(c).encode(encoding))
            has_decomp = decomp.decomps[0] != x
        ccc = decomp_map.get(x, DecompValue([], 0)).ccc
        packed = Packed((length, 6), (ccc, 8), (0, 1), (int(has_decomp), 1))
        trie.set(x, packed.to_int(16))
    trie.compact()
    return trie


# NOTE: if we could compress one more bit, we could merge this with comp_check_trie to save
# one trie of space. This trie is pretty small, though.
def create_comp_trie(
    qc: dict[int, str],
    decomp_map: DecompMap,
    non_starters: list[int],
    composables: list[int],
) -> Trie:
    trie = Trie()
    for x in range(BMP_LIMIT):
        if x in qc or x in non_starters:
            # This identifies a special but common class of characters that:
            # 1. Do not compose with anything
            # 2. Decompose into a single character
            # 3. The decomposed character does not compose with anything
            # Such characters are a subset of NF(K)C_QC that have nothing to
            # do with composition at all (they're only relevance is that they
            # can be decomposed). They get a special value in the trie so that
            # a potential optimization is available: if we have an input x with
            # code points with value 0 or this special 1 value (and no 2 values),
            # we have NF(K)D(x) == NF(K)C(x).
            if (
                x not in composables
                and len(decomp_map[x].decomps) == 1
                and decomp_map[x].decomps[0] not in composables
            ):
                value = 1
            else:
                value = 2
        else:
            value = 0
        trie.set(x, value)
    trie.compact()
    return trie


def create_comp_check_trie(
    qc: dict[int, str], decomp_map: DecompMap, encoding: str
) -> Trie:
    trie = Trie()
    for x in range(BMP_LIMIT):
        if x >= S_BASE and x < S_BASE + S_COUNT:
            # Handle Hangul syllables
            s_index = x - S_BASE
            jamo_size = len(chr(L_BASE).encode(encoding))
            length = 2 * jamo_size if s_index % T_COUNT == 0 else 3 * jamo_size
        elif x in decomp_map:
            length = sum(len(chr(c).encode(encoding)) for c in decomp_map[x].decomps)
        else:
            try:
                length = len(chr(x).encode(encoding))
            except UnicodeEncodeError:
                continue
        ccc = decomp_map.get(x, DecompValue([], 0)).ccc
        packed = Packed((length, 6), (ccc, 8), (0, 1), (int(x in qc), 1))
        trie.set(x, packed.to_int(16))
    trie.compact()
    return trie


def create_decomp_words_bmp(
    nfd_map: DecompMap, nfkd_map: DecompMap, encoding: str
) -> tuple[list[int], dict[int, int], dict[int, int]]:
    width: int
    if encoding == "UTF-8":
        width = 1
    elif encoding == "UTF-16LE":
        width = 2
    else:
        print(f"unknown encoding: {encoding}")
        exit(1)
    decomp_bytes = [0] * width
    offsets_nfd: dict[int, int] = {}
    offsets_nfkd: dict[int, int] = {}
    for x, value in nfkd_map.items():
        if x >= BMP_LIMIT:
            continue
        offset = len(decomp_bytes)
        for c in value.decomps:
            decomp_bytes.extend(chr(c).encode(encoding))
        offsets_nfkd[x] = offset // width
        if x in nfd_map:
            nfd_value = nfd_map[x]
            if nfd_value.decomps == value.decomps:
                offsets_nfd[x] = offset // width
            else:
                nfd_offset = len(decomp_bytes)
                for c in nfd_value.decomps:
                    decomp_bytes.extend(chr(c).encode(encoding))
                offsets_nfd[x] = nfd_offset // width
    # Add 16 bytes of padding to make it safe to do oversized loads from the tail of the decomposition
    # bytes, which is reasonable to do in vectorized versions of decomposition.
    decomp_bytes.extend([0] * 16)
    words: list[int]
    if encoding == "UTF-8":
        words = decomp_bytes
    elif encoding == "UTF-16LE":
        words = [
            decomp_bytes[i] | (decomp_bytes[i + 1] << 8)
            for i in range(0, len(decomp_bytes), 2)
        ]
    return words, offsets_nfd, offsets_nfkd


@dataclass
class DerivedProps:
    comp_exclusions: list[int]
    nfc_qc: dict[int, str]
    nfkc_qc: dict[int, str]


def load_derived_props() -> DerivedProps:
    exclusions: list[int] = []
    nfc_qc: dict[int, str] = {}
    nfkc_qc: dict[int, str] = {}

    with open("ucd/DerivedNormalizationProps.txt", "r") as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue

            parts = line.split(";")
            raw_code_points = parts[0].strip().split("..")
            assert len(raw_code_points) <= 2
            if len(raw_code_points) == 1:
                c = int(raw_code_points[0], 16)
                code_points = range(c, c + 1)
            else:
                start = int(raw_code_points[0], 16)
                end = int(raw_code_points[1], 16)
                code_points = range(start, end + 1)

            if "Full_Composition_Exclusion" in line:
                exclusions.extend(code_points)
            if "NFC_QC" in line:
                for c in code_points:
                    value = parts[2].strip()[0]
                    assert value == "M" or value == "N"
                    nfc_qc[c] = value
            if "NFKC_QC" in line:
                for c in code_points:
                    value = parts[2].strip()[0]
                    assert value == "M" or value == "N"
                    nfkc_qc[c] = value

    return DerivedProps(comp_exclusions=exclusions, nfc_qc=nfc_qc, nfkc_qc=nfkc_qc)


# Flatten a decomposition map so that any recursive decompositions are resolved.
def flatten_decomp_map(map: DecompMap):
    for x, decomp in map.items():
        final_decomp: list[int] = []
        for c in decomp.decomps:
            # Get the full expansion of each code point that makes up `x`
            expansion = expand(c, map)
            final_decomp.extend(expansion)
        map[x].decomps = final_decomp


def align_key_value_lines(lines: list[tuple[str, str]]) -> list[str]:
    max_key_len = max(len(key.strip()) for key, _ in lines)
    aligned = [f"{key.strip():<{max_key_len}} {value.strip()}" for key, value in lines]
    return aligned


def print_header_summary(title: str, headers: list[HeaderDef]) -> None:
    lines: list[str] = []
    KILOBYTE = 1024
    total = 0
    for header in headers:
        if not header.array_sizes:
            continue
        size = header.size()
        lines.append((header.name, f"{size / KILOBYTE:.1f}KiB"))
        total += size
    lines.append(("TOTAL", f"{total / KILOBYTE:.1f}KiB"))
    aligned = align_key_value_lines(lines)
    print(f"{title}:")
    for line in aligned:
        print(line, file=sys.stderr)


def main() -> None:
    nfd_map, nfkd_map = load_decomp_maps()
    derived = load_derived_props()

    non_starters = [x for x, decomp in nfd_map.items() if decomp.ccc > 0]

    comp_map: CompMap = {}
    # Tracks characters that compose
    composables: list[int] = []
    for x, decomp in nfd_map.items():
        if x in derived.comp_exclusions or decomp.decomps[0] == x:
            continue
        assert len(decomp.decomps) == 2
        composables.extend(decomp.decomps)
        comp_map[(decomp.decomps[0], decomp.decomps[1])] = x
    # Add Hangul V Jamo
    composables.extend(range(V_BASE, V_BASE + V_COUNT))
    # Add Hangul T Jamo
    composables.extend(range(T_BASE + 1, T_BASE + T_COUNT))

    flatten_decomp_map(nfd_map)
    flatten_decomp_map(nfkd_map)

    for x, decomps in nfkd_map.items():
        if (
            decomps.ccc == 0
            and decomps.decomps
            and all(d in nfkd_map and nfkd_map[d].ccc > 0 for d in decomps.decomps)
        ):
            # HACK: this is a very implementation-specific operation, but here's my best
            #       explanation: we want to ensure that any character that decomposes
            #       into combining characters (all ccc values > 0) also has a ccc value > 0.
            #       This is important, because one way we detect for when we need to do a
            #       combining character sort is by looking at the original (precomposed)
            #       character's ccc value. There are a few code points that have a ccc value
            #       of zero, yet decompose solely into code points with ccc values > 0. This
            #       amends those characters so that they can be properly detected as combining
            #       marks. Obviously, patching over the Unicode character database is suboptimal,
            #       but this presently causes no issues with the decomposition process.
            #       See https://corp.unicode.org/pipermail/unicode/2025-July/011511.html for the
            #       relevant discussion on this. It might also be a more convincing argument
            #       for why this operation doesn't mess with the canonical decomposition process
            #       in a harmful way.
            #
            #       From https://www.unicode.org/versions/Unicode17.0.0/core-spec/chapter-3/#G1632
            #       > All characters with non-zero canonical combining class are combining characters,
            #       > but the reverse is not the case: there are combining characters with a zero
            #       > canonical combining class.
            #       This change makes "the reverse" true: x.ccc > 0 iff x is a combining character.
            nfkd_map[x].ccc = 1
            if x in nfd_map:
                nfd_map[x].ccc = 1

    decomp_bytes_utf8, offsets_nfd_utf8, offsets_nfkd_utf8 = create_decomp_words_bmp(
        nfd_map, nfkd_map, "UTF-8"
    )
    decomp_bytes_utf16, offsets_nfd_utf16, offsets_nfkd_utf16 = create_decomp_words_bmp(
        nfd_map, nfkd_map, "UTF-16LE"
    )

    utf8_nfd_trie, utf8_nfd_data_trie = create_decomp_trie_utf8(
        nfd_map, offsets_nfd_utf8, decomp_bound=16
    )
    utf8_nfkd_trie, utf8_nfkd_data_trie = create_decomp_trie_utf8(
        nfkd_map, offsets_nfkd_utf8, decomp_bound=48
    )
    utf16_nfd_trie = create_decomp_trie_utf16(
        nfd_map, offsets_nfd_utf16, decomp_bound=16
    )
    utf16_nfkd_trie = create_decomp_trie_utf16(
        nfkd_map, offsets_nfkd_utf16, decomp_bound=48
    )
    ccc_trie = Trie()
    for x in range(BMP_LIMIT):
        if x in nfd_map:
            ccc_trie.set(x, nfd_map[x].ccc)
        else:
            ccc_trie.set(x, 0)
    ccc_trie.compact()
    nfc_trie = create_comp_trie(derived.nfc_qc, nfd_map, non_starters, composables)
    nfkc_trie = create_comp_trie(derived.nfkc_qc, nfkd_map, non_starters, composables)
    utf8_nfd_check_trie = create_decomp_check_trie(nfd_map, "UTF-8")
    utf8_nfkd_check_trie = create_decomp_check_trie(nfkd_map, "UTF-8")
    utf8_nfc_check_trie = create_comp_check_trie(derived.nfc_qc, nfd_map, "UTF-8")
    utf8_nfkc_check_trie = create_comp_check_trie(derived.nfkc_qc, nfkd_map, "UTF-8")
    utf16_nfd_check_trie = create_decomp_check_trie(nfd_map, "UTF-16LE")
    utf16_nfkd_check_trie = create_decomp_check_trie(nfkd_map, "UTF-16LE")
    utf16_nfc_check_trie = create_comp_check_trie(derived.nfc_qc, nfd_map, "UTF-16LE")
    utf16_nfkc_check_trie = create_comp_check_trie(
        derived.nfkc_qc, nfkd_map, "UTF-16LE"
    )

    other_headers: list[HeaderDef] = []
    utf8_nfd_headers: list[HeaderDef] = []
    utf8_nfkd_headers: list[HeaderDef] = []
    with open("utf8_to_decomposed_tables.h", "w") as f:
        f.write(UTF8_TO_DECOMPOSED_PREAMBLE)
        other_headers.extend(generate_pack_hangul(f))
        other_headers.append(generate_array(f, "decompositions", decomp_bytes_utf8, 8))
        f.write("namespace nfd {\n")
        utf8_nfd_headers.extend(generate_trie(f, "trie", utf8_nfd_trie, 16, 16))
        utf8_nfd_headers.extend(
            generate_trie(f, "full_trie", utf8_nfd_data_trie, 16, 32)
        )
        utf8_nfd_headers.extend(
            generate_trie(f, "check_trie", utf8_nfd_check_trie, 16, 16)
        )
        f.write("} // namespace nfd\n")
        f.write("namespace nfkd {\n")
        utf8_nfkd_headers.extend(generate_trie(f, "trie", utf8_nfkd_trie, 16, 16))
        utf8_nfkd_headers.extend(
            generate_trie(f, "full_trie", utf8_nfkd_data_trie, 16, 32)
        )
        utf8_nfkd_headers.extend(
            generate_trie(f, "check_trie", utf8_nfkd_check_trie, 16, 16)
        )
        f.write("} // namespace nfkd\n")
        f.write(UTF8_TO_DECOMPOSED_POSTAMBLE)
    utf8_nfc_headers: list[HeaderDef] = []
    utf8_nfkc_headers: list[HeaderDef] = []
    with open("utf8_to_composed_tables.h", "w") as f:
        f.write(UTF8_TO_COMPOSED_PREAMBLE)
        f.write("namespace nfc {\n")
        utf8_nfc_headers.extend(
            generate_trie(f, "check_trie", utf8_nfc_check_trie, 16, 16)
        )
        f.write("} // namespace nfc\n")
        f.write("namespace nfkc {\n")
        utf8_nfkc_headers.extend(
            generate_trie(f, "check_trie", utf8_nfkc_check_trie, 16, 16)
        )
        f.write("} // namespace nfkc\n")
        f.write(UTF8_TO_COMPOSED_POSTAMBLE)
    utf16_nfd_headers: list[HeaderDef] = []
    utf16_nfkd_headers: list[HeaderDef] = []
    with open("utf16_to_decomposed_tables.h", "w") as f:
        f.write(UTF16_TO_DECOMPOSED_PREAMBLE)
        other_headers.append(
            generate_array(f, "decompositions", decomp_bytes_utf16, 16)
        )
        f.write("namespace nfd {\n")
        utf16_nfd_headers.extend(generate_trie(f, "trie", utf16_nfd_trie, 16, 32))
        utf16_nfd_headers.extend(
            generate_trie(f, "check_trie", utf16_nfd_check_trie, 16, 16)
        )
        f.write("} // namespace nfd\n")
        f.write("namespace nfkd {\n")
        utf16_nfkd_headers.extend(generate_trie(f, "trie", utf16_nfkd_trie, 16, 32))
        utf16_nfkd_headers.extend(
            generate_trie(f, "check_trie", utf16_nfkd_check_trie, 16, 16)
        )
        f.write("} // namespace nfkd\n")
        f.write(UTF16_TO_DECOMPOSED_POSTAMBLE)
    utf16_nfc_headers: list[HeaderDef] = []
    utf16_nfkc_headers: list[HeaderDef] = []
    with open("utf16_to_composed_tables.h", "w") as f:
        f.write(UTF16_TO_COMPOSED_PREAMBLE)
        f.write("namespace nfc {\n")
        utf16_nfc_headers.extend(
            generate_trie(f, "check_trie", utf16_nfc_check_trie, 16, 16)
        )
        f.write("} // namespace nfc\n")
        f.write("namespace nfkc {\n")
        utf16_nfkc_headers.extend(
            generate_trie(f, "check_trie", utf16_nfkc_check_trie, 16, 16)
        )
        f.write("} // namespace nfkc\n")
        f.write(UTF16_TO_COMPOSED_POSTAMBLE)
    # Tables agnostic of encoding
    with open("normalization_tables.h", "w") as f:
        f.write(NORMALIZATION_PREAMBLE)
        other_headers.extend(generate_trie(f, "ccc_trie", ccc_trie, 16, 8))
        other_headers.extend(generate_supplementary_ccc_hash_table(f, nfd_map))
        other_headers.extend(generate_comp_hash_table(f, comp_map))
        f.write("namespace nfd {\n")
        other_headers.extend(
            prefix(
                generate_decomp_hash_table(f, nfd_map, derived.nfc_qc, "lookup"), "nfd"
            )
        )
        f.write("} // namespace nfd\n")
        f.write("namespace nfkd {\n")
        other_headers.extend(
            prefix(
                generate_decomp_hash_table(f, nfkd_map, derived.nfkc_qc, "lookup"),
                "nfkd",
            )
        )
        f.write("} // namespace nfkd\n")
        f.write("namespace nfc {\n")
        other_headers.extend(prefix(generate_trie(f, "trie", nfc_trie, 16, 8), "nfc"))
        f.write("} // namespace nfc\n")
        f.write("namespace nfkc {\n")
        other_headers.extend(prefix(generate_trie(f, "trie", nfkc_trie, 16, 8), "nfkc"))
        f.write("} // namespace nfkc\n")
        f.write(NORMALIZATION_POSTAMBLE)

    print_header_summary("UTF-8 NFD", utf8_nfd_headers)
    print()
    print_header_summary("UTF-8 NFKD", utf8_nfkd_headers)
    print()
    print_header_summary("UTF-8 NFC", utf8_nfc_headers)
    print()
    print_header_summary("UTF-8 NFKC", utf8_nfkc_headers)
    print()
    print_header_summary("UTF-16 NFD", utf16_nfd_headers)
    print()
    print_header_summary("UTF-16 NFKD", utf16_nfkd_headers)
    print()
    print_header_summary("UTF-16 NFC", utf16_nfc_headers)
    print()
    print_header_summary("UTF-16 NFKC", utf16_nfkc_headers)
    print()
    print_header_summary("OTHER", other_headers)


if __name__ == "__main__":
    main()
