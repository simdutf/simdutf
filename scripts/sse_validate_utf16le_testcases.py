from itertools import product
from random import randint, seed
from sse_validate_utf16le_proof import bitmask


# This is a copy from sse_validate_utf16le_proof.py with
# adjusted the mask for the 16-bit base
def mask(words):
    L = bitmask(words, 'L')
    H = bitmask(words, 'H')
    V = (~(L | H)) & 0xffff

    a = L & (H >> 1)
    b = a << 1
    c = V | a | b

    return c


class Record:
    def __init__(self):
        self.words = []


    def add(self, word):
        self.words.append(word)


    @property
    def is_valid(self):
        c = mask(self.words)

        if c == 0xffff:
          return True

        if c == 0x7fff:
          # in test we reject cases when 'L' or 'H' ends a chunk
          if self.words[-1] in ('L', 'H'):
            return False
          else:
            return True

        return False


    def __str__(self):
        words = ''.join(self.words)
        if self.is_valid:
            return 'T' + words
        else:
            return 'F' + words


def test_words():
    collection = set()
    for seq in test_words_aux():
        collection.add(tuple(seq))

    return sorted(collection)


def test_words_aux():
    # 1. all valid
    yield ['V'] * 16

    # 2. only low surrogates
    yield ['L'] * 16

    # 3. only high surrogates
    yield ['H'] * 16

    # 4. sole low surrogate
    for i in range(16):
        seq = ['V'] * 16
        seq[i] = 'L'
        yield seq

    # 5. sole high surrogate
    for i in range(16):
        seq = ['V'] * 16
        seq[i] = 'H'
        yield seq

    # 6. scattered three surrogates
    for i in range(16):
        for j in range(16):
            for k in range(16):
                    seq = ['V'] * 16
                    for a, b, c in product('LH', repeat=3):
                        seq[i] = a
                        seq[j] = b
                        seq[k] = c
                        yield seq

    # To cover all 16-byte inputs we would need 3**16 cases (43'046'721)
    # Instead, we cover all possible 6-element combinations (3**6 = 729)
    # and move it within 16-element input. This yields 729 * 10 cases.
    k = 6
    for combination in product('VLH', repeat=k):
        for position in range(16 - k):
            seq = ['V'] * 16
            for i, v in enumerate(combination):
                seq[i + position] = v

            yield seq


TXT = """# generated by scripts/sse_validate_utf16le_testcases.py
"""

def write_file(file):
    file.write(TXT)
    for words in test_words():
        record = Record()
        for word in words:
            record.add(word)

        file.write(str(record))
        file.write('\n')


def main():
    seed(0)
    with open('validate_utf16_testcases.txt', 'w') as f:
        write_file(f)


if __name__ == '__main__':
    main()
