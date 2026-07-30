#!/usr/bin/env python3

import sys
from dataclasses import dataclass
from itertools import batched
from collections import defaultdict
from typing import Collection
from ucptrie import UCPTrie, UCPTrieGroup, build_ucptrie, build_ucptries

# Max Unicode code point
SUPPLEMENTARY_LIMIT = 0x110000
BMP_LIMIT = 0x10000


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


def prefix(s: str, header: HeaderDef) -> HeaderDef:
    return HeaderDef(f"{s}::{header.name}", header.type_, header.array_sizes)


def prefix_all(s: str, headers: list[HeaderDef]) -> list[HeaderDef]:
    return [prefix(s, header) for header in headers]


def generate_array(writer, name: str, data: list[int], data_width: int) -> HeaderDef:
    writer.write(f"\nconst uint{data_width}_t {name}[{len(data)}] = {{\n")
    for row in batched(data, 10):
        writer.write(" ")
        for x in row:
            assert x < 2**data_width
            if data_width == 64:
                writer.write(f" 0x{x:016X},")
            elif data_width == 32:
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


def generate_pack_hangul(writer) -> HeaderDef:
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
    return HeaderDef.array("pack_hangul", "HangulShuf", 16)


def generate_shuf_utf16(writer) -> HeaderDef:
    writer.write(f"\nconst uint8_t shuf_utf16[256][16] = {{\n")
    for x in range(1 << 8):
        tbl = list(range(16))
        s = f"{x:08b}"
        pairs = [int(s[i : i + 2], 2) for i in range(0, len(s), 2)]
        pairs.reverse()
        # In this case, we can't fit the decomposition into 16 bytes
        if sum(x % 3 for x in pairs) > 4:
            tbl = [255] * 16
            writer.write(f"  {{{", ".join(map(str, tbl))}}},\n")
            continue
        displacement = 0
        for i, delta in enumerate(pairs):
            if delta == 0b11:
                continue
            lookup_base = 16 + (i * 8)
            decomp_size = 2 + (delta * 2)
            tbl_pos = (i * 2) + displacement
            tbl[tbl_pos : tbl_pos + decomp_size] = list(
                range(lookup_base, lookup_base + decomp_size)
            )
            tbl[tbl_pos + decomp_size : 16] = [
                j - (delta * 2) for j in tbl[tbl_pos + decomp_size :]
            ]
            displacement += delta * 2
        assert len(tbl) == 16
        writer.write(f"  {{{", ".join(map(str, tbl))}}},\n")
    writer.write("};\n")
    return HeaderDef.multi_array("shuf_utf16", "uint8_t", [256, 16])


def generate_ucptrie(
    writer,
    name: str,
    trie: UCPTrie,
    index_width: int,
    index1_width: int,
    index2_width: int,
    data_width: int,
) -> list[HeaderDef]:
    return [
        generate_array(writer, name + "_index", trie.index, index_width),
        generate_array(writer, name + "_index1", trie.index1, index1_width),
        generate_array(writer, name + "_index2", trie.index2, index2_width),
        generate_array(writer, name + "_data", trie.data, data_width),
    ]


def generate_ucptrie_group(
    writer,
    name: str,
    group: UCPTrieGroup,
    namespaces: list[str],
    data_width: int,
) -> list[HeaderDef]:
    assert len(namespaces) == len(group.tries)
    defs = [generate_array(writer, name + "_data", group.data, data_width)]
    for ns, trie in zip(namespaces, group.tries):
        writer.write(f"\nnamespace {ns} {{\n")
        defs.extend(
            prefix_all(
                ns,
                [
                    generate_array(writer, name + "_index", trie.index, 16),
                    generate_array(writer, name + "_index1", trie.index1, 16),
                    generate_array(writer, name + "_index2", trie.index2, 16),
                ],
            )
        )
        writer.write(f"}} // namespace {ns}\n")
    return defs


def generate_trie(
    writer, name: str, trie: Trie, index_width: int, data_width: int
) -> list[HeaderDef]:
    return [
        generate_array(writer, name + "_index", trie.index, index_width),
        generate_array(writer, name + "_data", trie.data, data_width),
    ]


# Namespace names for the two tries in each shared-data group, in build order.
DECOMPOSED_FORMS = ["nfd", "nfkd"]
COMPOSED_FORMS = ["nfc", "nfkc"]

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


ENCODING_WIDTHS = {
    "UTF-8": 1,
    "UTF-16LE": 2,
}


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


# It is more efficient to rank CCC values instead of using their raw byte value. We can do this
# successfully because CCC values are only used for comparison
def rank_ccc_values(maps: list[DecompMap]) -> None:
    values = sorted({d.ccc for m in maps for d in m.values()} | {0})
    # This guarantees we can fit ccc values in 6 bits
    assert len(values) <= 64
    ranks = {c: i for i, c in enumerate(values)}
    # We 1 as a special value (see HACK)
    assert ranks[1] == 1
    for map in maps:
        for decomp in map.values():
            decomp.ccc = ranks[decomp.ccc]


def create_decomp_values_utf8(
    decomp_map: DecompMap,
    offsets: dict[int, int],
    decomp_bound: int,
) -> tuple[list[int], list[int]]:
    trie_data = [0] * SUPPLEMENTARY_LIMIT
    decomp_trie_data = [0] * SUPPLEMENTARY_LIMIT
    for x in range(SUPPLEMENTARY_LIMIT):
        try:
            size = len(chr(x).encode("UTF-8"))
        except UnicodeEncodeError:
            continue
        if x not in decomp_map:
            trie_data[x] = size - 1
            decomp_trie_data[x] = 0
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
            (size - 1, 2),
            (last_ccc, 6),
            (0, 2),
            (int(decomp.decomps[0] != x), 1),
            (final_decomp & 0x1F, 5),
        )
        trie_data[x] = packed.to_int(16)
        assert length <= 0x3F
        assert offset <= 0x7FFF
        ccc_delta = 0 if first_ccc == 0 else last_ccc - first_ccc
        packed = Packed(
            (offset, 15), (length, 6), (last_ccc, 6), (ccc_delta, 2), (0, 3)
        )
        decomp_trie_data[x] = packed.to_int(32)
    return trie_data, decomp_trie_data


def create_decomp_values_utf16(
    decomp_map: DecompMap, offsets: dict[int, int], decomp_bound: int
) -> list[int]:
    trie_data = [0] * SUPPLEMENTARY_LIMIT
    for x in range(SUPPLEMENTARY_LIMIT):
        if x not in decomp_map:
            trie_data[x] = 0
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
        packed = Packed(
            (offset, 14),
            (delta, 6),
            (ccc_delta, 2),
            (0, 1),
            (int(decomp.decomps[0] != x), 1),
            (last_ccc, 6),
        )
        trie_data[x] = packed.to_int(32)
    return trie_data


def create_check_values(
    decomp_map: DecompMap,
    qc: dict[int, str],
    non_starters: list[int],
    encoding: str,
) -> list[int]:
    trie_data = [0] * SUPPLEMENTARY_LIMIT
    jamo_size = len(chr(L_BASE).encode(encoding)) // ENCODING_WIDTHS[encoding]
    for x in range(SUPPLEMENTARY_LIMIT):
        has_decomp = False
        if x in decomp_map:
            length = 0
            decomp = decomp_map[x]
            for c in decomp.decomps:
                length += len(chr(c).encode(encoding)) // ENCODING_WIDTHS[encoding]
            has_decomp = decomp.decomps[0] != x
        elif x >= S_BASE and x < S_BASE + S_COUNT:
            # Hangul syllables decompose algorithmically, so they have no entry in
            # `decomp_map`, but they do have a decomposition.
            s_index = x - S_BASE
            length = 2 * jamo_size if s_index % T_COUNT == 0 else 3 * jamo_size
            has_decomp = True
        else:
            try:
                length = len(chr(x).encode(encoding)) // ENCODING_WIDTHS[encoding]
            except UnicodeEncodeError:
                # Python throws an error if we try to encode a surrogate in UTF-8
                continue
        ccc = decomp_map.get(x, DecompValue([], 0)).ccc
        packed = Packed(
            (length, 6),
            (ccc, 6),
            (int(x in qc or x in non_starters), 1),
            (int(x in qc), 1),
            (int(x in decomp_map or has_decomp), 1),
            (int(has_decomp), 1),
        )
        trie_data[x] = packed.to_int(16)
    return trie_data


def create_comp_values(
    qc: dict[int, str],
    comp_map: CompMap,
    decomp_map: DecompMap,
    non_starters: list[int],
    composables: list[int],
) -> tuple[list[int], list[int]]:
    trie_data = [0] * SUPPLEMENTARY_LIMIT
    compositions: list[int] = [0]
    forward_comps: dict[int, list[tuple[int, int]]] = defaultdict(list)
    for (starter, trail), composite in comp_map.items():
        forward_comps[starter].append((trail, composite))
    for v in forward_comps.values():
        # Sort by trailing character
        v.sort(key=lambda x: x[0])
    for x in range(SUPPLEMENTARY_LIMIT):
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
                indicator = 1
            else:
                indicator = 2
        else:
            indicator = 0
        if x in decomp_map:
            if indicator == 1 and decomp_map[x].decomps[-1] in decomp_map:
                # For indicator 1, we should use the decomposition's ccc
                ccc = decomp_map[decomp_map[x].decomps[-1]].ccc
            else:
                ccc = decomp_map[x].ccc
        else:
            ccc = 0
        if x not in forward_comps:
            offset = 0
        else:
            offset = len(compositions)
        for i, (trail, composite) in enumerate(forward_comps[x]):
            packed = Packed(
                (trail, 21),
                (composite, 21),
                (int(composite in forward_comps), 1),
                (int(i == len(forward_comps[x]) - 1), 1),
            )
            compositions.append(packed.to_int(64))
        # Whether the code point actually decomposes
        if S_BASE <= x < S_BASE + S_COUNT:
            has_decomp = 1
        elif x in decomp_map and decomp_map[x].decomps[0] != x:
            has_decomp = 1
        else:
            has_decomp = 0
        # `has_decomp` sits at bit 12 in both layouts so it can be read without
        # first testing the composes-forward flag.
        if offset > 0:
            assert ccc == 0
            # Ten bits is enough because there are far fewer than 1024
            # composition list entries; the bits freed up pay for `has_decomp`.
            assert offset <= 0x3FF
            packed = Packed(
                (indicator, 2), (offset, 10), (has_decomp, 1), (0, 2), (1, 1)
            )
        else:
            packed = Packed((indicator, 2), (ccc, 6), (0, 4), (has_decomp, 1), (0, 3))
        trie_data[x] = packed.to_int(16)
    return trie_data, compositions


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
    lines: list[tuple[str, str]] = []
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
    print(title, file=sys.stderr)
    for line in aligned:
        print(line, file=sys.stderr)


def main() -> None:
    nfd_map, nfkd_map = load_decomp_maps()
    rank_ccc_values([nfd_map, nfkd_map])
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

    utf8_nfd_values, utf8_nfd_full_values = create_decomp_values_utf8(
        nfd_map, offsets_nfd_utf8, decomp_bound=16
    )
    utf8_nfkd_values, utf8_nfkd_full_values = create_decomp_values_utf8(
        nfkd_map, offsets_nfkd_utf8, decomp_bound=48
    )
    utf16_nfd_values = create_decomp_values_utf16(
        nfd_map, offsets_nfd_utf16, decomp_bound=16
    )
    utf16_nfkd_values = create_decomp_values_utf16(
        nfkd_map, offsets_nfkd_utf16, decomp_bound=48
    )
    nfc_values, compositions = create_comp_values(
        derived.nfc_qc, comp_map, nfd_map, non_starters, composables
    )
    nfkc_values, nfkc_compositions = create_comp_values(
        derived.nfkc_qc, comp_map, nfkd_map, non_starters, composables
    )
    # Compatibility decompositions never compose, so NFC and NFKC recompose from
    # the identical canonical composition table. Emit it once.
    assert compositions == nfkc_compositions
    ccc_values = [
        nfkd_map[x].ccc if x in nfkd_map else 0 for x in range(SUPPLEMENTARY_LIMIT)
    ]

    ccc_trie = build_ucptrie(ccc_values)
    utf8_decomp_group = build_ucptries([utf8_nfd_values, utf8_nfkd_values])
    utf8_full_group = build_ucptries([utf8_nfd_full_values, utf8_nfkd_full_values])
    utf16_decomp_group = build_ucptries([utf16_nfd_values, utf16_nfkd_values])
    comp_group = build_ucptries([nfc_values, nfkc_values])
    utf8_check_group = build_ucptries(
        [
            create_check_values(nfd_map, derived.nfc_qc, non_starters, "UTF-8"),
            create_check_values(nfkd_map, derived.nfkc_qc, non_starters, "UTF-8"),
        ]
    )
    utf16_check_group = build_ucptries(
        [
            create_check_values(nfd_map, derived.nfc_qc, non_starters, "UTF-16LE"),
            create_check_values(nfkd_map, derived.nfkc_qc, non_starters, "UTF-16LE"),
        ]
    )

    headers: defaultdict[str, list[HeaderDef]] = defaultdict(list[HeaderDef])

    def emit_group(file: str, writer, name: str, group, data_width: int) -> None:
        headers[file].extend(
            generate_ucptrie_group(writer, name, group, DECOMPOSED_FORMS, data_width)
        )

    with open("utf8_to_decomposed_tables.h", "w") as f:
        f.write(UTF8_TO_DECOMPOSED_PREAMBLE)
        file = "utf8_to_decomposed_tables.h"
        headers[file].append(generate_pack_hangul(f))
        headers[file].append(
            generate_array(f, "decompositions", decomp_bytes_utf8, data_width=8)
        )
        emit_group(file, f, "trie", utf8_decomp_group, data_width=16)
        emit_group(file, f, "full_trie", utf8_full_group, data_width=32)
        emit_group(file, f, "check_trie", utf8_check_group, data_width=16)
        f.write(UTF8_TO_DECOMPOSED_POSTAMBLE)
    with open("utf16_to_decomposed_tables.h", "w") as f:
        f.write(UTF16_TO_DECOMPOSED_PREAMBLE)
        file = "utf16_to_decomposed_tables.h"
        headers[file].append(
            generate_array(f, "decompositions", decomp_bytes_utf16, data_width=16)
        )
        headers[file].append(generate_shuf_utf16(f))
        emit_group(file, f, "trie", utf16_decomp_group, data_width=32)
        emit_group(file, f, "check_trie", utf16_check_group, data_width=16)
        f.write(UTF16_TO_DECOMPOSED_POSTAMBLE)
    # Tables agnostic of encoding
    with open("normalization_tables.h", "w") as f:
        f.write(NORMALIZATION_PREAMBLE)
        file = "normalization_tables.h"
        headers[file].extend(
            generate_ucptrie(
                f,
                "ccc_trie",
                ccc_trie,
                index_width=16,
                index1_width=16,
                index2_width=16,
                data_width=8,
            )
        )
        headers[file].append(generate_array(f, "compositions", compositions, 64))
        headers[file].extend(
            generate_ucptrie_group(f, "trie", comp_group, COMPOSED_FORMS, data_width=16)
        )
        f.write(NORMALIZATION_POSTAMBLE)

    for file, h in headers.items():
        print_header_summary(file, h)
        print()


if __name__ == "__main__":
    main()
