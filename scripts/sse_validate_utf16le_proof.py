# 'V' - single-word character (always valid)
# 'L' - low surrogate (must be followed by the high surrogate)
# 'H' - high surrogate
def all_sequences():
    index = ['V'] * 8

    def increment():
        nonlocal index
        for i in range(8):
            if index[i] == 'V':
                index[i] = 'L'
                return False

            if index[i] == 'L':
                index[i] = 'H'
                return False

            if index[i] == 'H':
                index[i] = 'V' # wrap around
                pass

        return True

    overflow = False
    while not overflow:
        yield index
        overflow = increment()


def find_error_in_words(words):
    prev = None

    if words[0] == 'H':
        # We assume that our vector algoritm load proper data into vectors.
        # In the case low surrogate was the last item in the previous iteration.
        return 'high surrogate must not start a chunk'

    for i, kind in enumerate(words):
        if kind == 'V':
            if prev == 'L':
                return f'low surrogate {i - 1} must be followed by high surrogate'
        elif kind == 'L':
            if prev == 'L':
                return f'low surrogate {i - 1} must be followed by high surrogate'
        elif kind == 'H':
            if prev != 'L':
                return f'high surrogate {i} must be preceded by low surrogate'

        prev = kind

    return ''


def bitmask(words, state):
    result = 0
    for bit, type in enumerate(words):
        result |= 1 << (2*bit) # fake 1 yiels by the SSE code

        if type == state:
            result |= 1 << (2*bit + 1)

    return result


def mask(words):
    V = bitmask(words, 'V')
    L = bitmask(words, 'L')
    H = bitmask(words, 'H')

    a = L & (H >> 2)
    b = a << 2
    c = V | a | b

    return c


def main():
    for words in all_sequences():

        c = mask(words)

        if False:
            words_image = "[ %s ]" % ' | '.join(words)
            error = find_error_in_words(words)
            if error == '':
                valid_image = 'T'
            else:
                valid_image = ' '
            print(words_image, valid_image, '{:016b} {:04x}'.format(c, c))

        if c == 0xffff:
            # all 8 words are valid (either 'V' or pairs 'L', 'H')
            assert find_error_in_words(words) == ''

        if c == 0x7fff:
            # all 7 words are valid (either 'V' or pairs 'L', 'H')
            # the last words is either 'L' or 'H' (the word will be
            # re-examined in the next iteration
            if words[-1] == 'H':
                assert find_error_in_words(words) == 'high surrogate 7 must be preceded by low surrogate'
            elif words[-1] == 'L':
                assert find_error_in_words(words) == ''
            else:
                assert False

    print("All OK")


if __name__ == '__main__':
    main()
