lut_lo = [0x0 for _ in range(16)]
lut_hi = [0x0 for _ in range(16)]

# uses (c & 0xf) for indexing lut_lo
# uses ((c >> 3) & 0xf) for indexing lut_hi
# bytes with c > 127 will have lut_hi mapped to 0 via vtbl1q_u8

# represent spaces by 0x1
# represent + (_) by 0x2
# represent / (-) by 0x4
# represent numbers by 0x8
# represent the partial row of upper alphabets with 0x10
# represent full valid row of upper alphabets with 0x20
# represent the partial row of lower alphabets with 0x40
# represent full valid row of lower alphabets with 0x80
# since '@' and 'P' both map to same lut_lo[0], use two different bits to separate them
lut_hi[0] = 0x0  # ['\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07']
lut_hi[1] = (
    0x1  # can have spaces ['\x08', '\t', '\n', '\x0b', '\x0c', '\r', '\x0e', '\x0f']
)
for z in range(16):
    if "\t\n\r\f".find(chr(z)) != -1:
        lut_lo[z & 0xF] = 0x1  # allowed

# invalid chars
lut_hi[2] = 0x0  # ['\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17']
lut_hi[3] = 0x0  # ['\x18', '\x19', '\x1a', '\x1b', '\x1c', '\x1d', '\x1e', '\x1f']

lut_hi[4] = 0x1  # can have spaces [' ', '!', '"', '#', '$', '%', '&', "'"]
lut_lo[ord(" ") & 0xF] |= 0x1

lut_hi[5] = 0x6  # for + and / ['(', ')', '*', '+', ',', '-', '.', '/']

lut_lo[ord("+") & 0xF] |= 0x2
lut_lo[ord("/") & 0xF] |= 0x4

# 0x30 numbers
lut_hi[0x6] = 0x8  # ['0', '1', '2', '3', '4', '5', '6', '7']
lut_hi[0x7] = 0x8  # ['8', '9', ':', ';', '<', '=', '>', '?']
for z in range(0, 10):
    lut_lo[z] |= 0x8

# 0x40, 0x60 letters
lut_hi[0x8] = 0x10  # ['@', 'A', 'B', 'C', 'D', 'E', 'F', 'G']
lut_hi[0x9] = 0x20  # ['H', 'I', 'J', 'K', 'L', 'M', 'N', 'O']
lut_hi[10] = 0x20  # ['P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W']
lut_hi[11] = 0x10  # ['X', 'Y', 'Z', '[', '\\', ']', '^', '_']

lut_lo[ord("A") & 0xF] |= 0x10
lut_lo[ord("B") & 0xF] |= 0x10
lut_lo[ord("C") & 0xF] |= 0x10
lut_lo[ord("D") & 0xF] |= 0x10
lut_lo[ord("E") & 0xF] |= 0x10
lut_lo[ord("F") & 0xF] |= 0x10
lut_lo[ord("G") & 0xF] |= 0x10
lut_lo[ord("X") & 0xF] |= 0x10
lut_lo[ord("Y") & 0xF] |= 0x10
lut_lo[ord("Z") & 0xF] |= 0x10

lut_hi[12] = 0x40  # ['`', 'a', 'b', 'c', 'd', 'e', 'f', 'g']
lut_hi[13] = 0x80  # ['h', 'i', 'j', 'k', 'l', 'm', 'n', 'o']
lut_hi[14] = 0x80  # ['p', 'q', 'r', 's', 't', 'u', 'v', 'w']
lut_hi[15] = 0x40  # ['x', 'y', 'z', '{', '|', '}', '~', '\x7f']

lut_lo[ord("a") & 0xF] |= 0x40
lut_lo[ord("b") & 0xF] |= 0x40
lut_lo[ord("c") & 0xF] |= 0x40
lut_lo[ord("d") & 0xF] |= 0x40
lut_lo[ord("e") & 0xF] |= 0x40
lut_lo[ord("f") & 0xF] |= 0x40
lut_lo[ord("g") & 0xF] |= 0x40
lut_lo[ord("x") & 0xF] |= 0x40
lut_lo[ord("y") & 0xF] |= 0x40
lut_lo[ord("z") & 0xF] |= 0x40

for z in range(0, 16):
    lut_lo[z] |= 0x80 | 0x20

print("base64 regular lut_lo")
print(",".join([hex(c) for c in lut_lo]))
print("base64 regular lut_hi")
print(",".join([hex(c) for c in lut_hi]))

lut_lo[ord("-") & 0xF] |= 0x4
lut_lo[ord("+") & 0xF] &= ~0x2
lut_lo[ord("/") & 0xF] &= ~0x4
lut_lo[ord("_") & 0xF] |= 0x2
lut_hi[11] = 0x12  # can have _ as well ['X', 'Y', 'Z', '[', '\\', ']', '^', '_']
lut_hi[5] = 0x4  # for - ['(', ')', '*', '+', ',', '-', '.', '/']
print("base64 url lut_lo")
print(",".join([hex(c) for c in lut_lo]))
print("base64 url lut_hi")
print(",".join([hex(c) for c in lut_hi]))

lut_lo[ord("+") & 0xF] |= 0x2
lut_lo[ord("/") & 0xF] |= 0x4
lut_hi[5] = 0x6  # for +, - and / ['(', ')', '*', '+', ',', '-', '.', '/']
print("base64 hybrid lut_lo")
print(",".join([hex(c) for c in lut_lo]))
print("base64 hybrid lut_hi")
print(",".join([hex(c) for c in lut_hi]))
