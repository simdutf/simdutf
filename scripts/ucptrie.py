from array import array
from dataclasses import dataclass


@dataclass
class TrieConfig:
    key_bits: int = 21
    block_shift: int = 6  # data block size = 2**block_shift entries
    fast_limit: int = 1 << 16  # keys below this use the single-stage fast path
    i1_block_shift: int = 5  # index2 entries grouped per index1 slot = 2**this
    overlap: bool = True  # enable prefix/suffix overlap compaction
    overlap_window: int = 64  # how far back (in data entries) to search overlaps

    @property
    def block_size(self) -> int:
        return 1 << self.block_shift

    @property
    def i1_group(self) -> int:
        return 1 << self.i1_block_shift


class BlockCompactor:
    def __init__(self, overlap: bool = True, overlap_window: int = 64):
        self.overlap = overlap
        self.overlap_window = overlap_window
        self.data: list[int] = []
        self._window_pos: dict[tuple[int, ...], int] = (
            {}
        )  # any window -> earliest offset
        self._window_len: int = 0  # fixed to the first block length seen
        self._indexed_to: int = (
            0  # data[:_indexed_to] has had all its windows registered
        )

    def add(self, block: tuple[int, ...]) -> int:
        """Add one block, returning its start offset into self.data."""
        if self._window_len == 0:
            self._window_len = len(block)

        off = self._window_pos.get(block)
        if off is not None:
            return off

        off = self._place(block)
        self._index_new_windows()
        return off

    def _index_new_windows(self) -> None:
        data = self.data
        n = len(data)
        w = self._window_len
        start = max(self._indexed_to - w + 1, 0)
        for i in range(start, n - w + 1):
            key = tuple(data[i : i + w])
            if key not in self._window_pos:  # keep the earliest offset
                self._window_pos[key] = i
        self._indexed_to = n

    def _place(self, block: tuple[int, ...]) -> int:
        data = self.data
        n = len(data)
        blen = len(block)

        if not self.overlap or n == 0:
            data.extend(block)
            return n

        max_l = min(blen, self.overlap_window, n)
        best_overlap = 0
        for l in range(max_l, 0, -1):  # longest possible match first
            if tuple(data[n - l : n]) == block[:l]:
                best_overlap = l
                break

        pos = n - best_overlap
        if best_overlap < blen:
            data.extend(block[best_overlap:])
        return pos


def _chunk(values: list[int], block_size: int) -> list[tuple[int, ...]]:
    n = len(values)
    pad = (-n) % block_size
    if pad:
        values = values + [values[-1] if values else 0] * pad
    return [
        tuple(values[i : i + block_size]) for i in range(0, len(values), block_size)
    ]


@dataclass
class UCPTrie:
    config: TrieConfig
    index: array  # fast range: block index -> offset into data
    index1: array  # supplementary stage 1: group index -> offset into index2
    index2: array  # supplementary stage 2: block index -> offset into data
    data: array

    def lookup(self, cp: int) -> int:
        cfg = self.config
        if cp < cfg.fast_limit:
            block = self.index[cp >> cfg.block_shift]
            return self.data[block + (cp & (cfg.block_size - 1))]
        shift2 = cfg.block_shift + cfg.i1_block_shift
        scp = cp - cfg.fast_limit
        i1 = self.index1[scp >> shift2]
        i2 = self.index2[i1 + ((scp >> cfg.block_shift) & (cfg.i1_group - 1))]
        return self.data[i2 + (cp & (cfg.block_size - 1))]


def build_ucptrie(values: list[int], config: TrieConfig = TrieConfig()) -> UCPTrie:
    cfg = config
    n_keys = 1 << cfg.key_bits

    fast_values = values[: cfg.fast_limit]
    supp_values = values[cfg.fast_limit :]

    compactor = BlockCompactor(cfg.overlap, cfg.overlap_window)

    # Fast range: single-stage index -> data
    fast_blocks = _chunk(fast_values, cfg.block_size)
    index = [compactor.add(b) for b in fast_blocks]

    # Supplementary range: two-stage index -> index2 -> data
    supp_blocks = _chunk(supp_values, cfg.block_size)
    index2_raw = [compactor.add(b) for b in supp_blocks]

    index2_compactor = BlockCompactor(cfg.overlap, cfg.overlap_window)
    index2_chunks = _chunk(index2_raw, cfg.i1_group)
    index1 = [index2_compactor.add(c) for c in index2_chunks]

    return UCPTrie(
        config=cfg,
        index=array("I", index),
        index1=array("I", index1),
        index2=array("I", index2_compactor.data),
        data=array("I", compactor.data),
    )
