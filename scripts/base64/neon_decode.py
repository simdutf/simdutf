t='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
spaces=' \t\n\r'
lut_lo = [0x3a, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x70, 0x61, 0xe1, 0xb4, 0xf4, 0xe5, 0xf4, 0xb4]
lut_hi = [0x11, 0x20, 0x42, 0x80, 0x8,  0x4,  0x8,  0x4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20]
roll = [0x0, 0x10, 0x13, 0x4, 0xbf, 0xbf, 0xb9, 0xb9, 0x0, 0x0,  0x0,  0x0, 0x0,  0x0,  0x0,  0x0]
def decode(s):
    low = s & 0xf
    high = s >> 4
    m = lut_lo[low] & lut_hi[high]
    if(m > 0x3):
        return (m, None)
    if s == 0x2f:
        off = roll[high - 1]
    else:
        off = roll[high]
    return (m,(s + off)&0xff)

for i in range(256):
    m,d = decode(i)
    if d is None:
        assert t.find(chr(i)) == -1
        assert spaces.find(chr(i)) == -1
        continue
    if m == 0:
        assert d >= 0
        # we must have a base64 element
        v = t.find(chr(i))
        #print(i, chr(i), v, d)
        assert v == d
    else:
        # we must have a space
        v = spaces.find(chr(i))
        assert v >= 0
