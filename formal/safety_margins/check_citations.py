#!/usr/bin/env python3
"""Check the C++ line citations in the Lean comments of SafetyMargins/.

Citations are `src/...cpp:N`, `include/...h:N`, ranges `:N-M`, lists
`:N and :M`, `:N/M`, `lines N, M`, bare `:N` (the most recent file named before it in the
same Lean file, a bare file name such as `implementation.cpp` resolved by the
directory of the architecture the Lean file covers) and `line N`.

Every citation is printed with the text of the cited line (the first and last
line of a range). A citation of a loop test, that is one followed or preceded
closely by the word `loop`, or followed by a quoted condition (`while (...)`,
`for (...)`, or a backquoted comparison such as `end - buf >= 32`), must cite a
line that contains `while` or `for` (for a range, its first line). The script
exits with status 1 if one does not, or if a cited file is missing or a line
is past the end of its file.

Usage: python3 check_citations.py [--quiet] [--skip File.lean ...]\n(run from anywhere; --skip leaves out a Lean file by base name)
"""
import glob
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..'))
LEAN = os.path.join(HERE, 'SafetyMargins')

BY_NAME = {}
for d in ('src', 'include'):
    for p in glob.glob(os.path.join(ROOT, d, '**', '*'), recursive=True):
        if os.path.isfile(p):
            BY_NAME.setdefault(os.path.basename(p), []).append(
                os.path.relpath(p, ROOT))

ARCH = {'Arm64': 'arm64', 'Haswell': 'haswell', 'Icelake': 'icelake',
        'Lasx': 'lasx', 'Lsx': 'lsx', 'Ppc64': 'ppc64', 'Rvv': 'rvv',
        'Westmere': 'westmere'}


def comments(txt):
    """(start, end) of the comment regions of a Lean file."""
    i, n = 0, len(txt)
    while i < n:
        if txt.startswith('/-', i):
            depth, j = 1, i + 2
            while j < n and depth:
                if txt.startswith('/-', j):
                    depth, j = depth + 1, j + 2
                elif txt.startswith('-/', j):
                    depth, j = depth - 1, j + 2
                else:
                    j += 1
            yield i, j
            i = j
        elif txt.startswith('--', i):
            j = txt.find('\n', i)
            j = n if j < 0 else j
            yield i, j
            i = j
        elif txt[i] == '"':
            j = i + 1
            while j < n and txt[j] != '"':
                j += 2 if txt[j] == '\\' else 1
            i = j + 1
        else:
            i += 1


PATH = re.compile(r'((?:src|include)/[\w/.\-]+\.(?:cpp|h))'
                  r'|\b([\w\-]+\.(?:inl\.cpp|inl\.h|cpp|h))\b')
NUM = re.compile(r'(?:(?:(?<=\.cpp)|(?<=\.h)|(?<![\w.]))(?<!0x):'
                 r'|(?<=\d)(?:/|, and |, | and )|\blines? |\bline\s)'
                 r'(\d+)(?:[-–](\d+))?')
# A list continuation counts only after a citation: `:N/M`, `:N and M`,
# `lines N, M` (`:N, 8, 6` is a line and two counts, so `, M` needs `lines`).
CHAIN = re.compile(r'(?::\d+(?:-\d+)?(?:(?:/| and )\d+(?:-\d+)?)*'
                   r'|\blines? \d+(?:-\d+)?'
                   r'(?:(?:/|, and |, | and )\d+(?:-\d+)?)*)$')
LINES = re.compile(r'\blines? \d+(?:-\d+)?(?:(?:/|, and |, | and )\d+(?:-\d+)?)*$')
COND = re.compile(r'^[\s,)]*(?:loop\b|`(?:while|for)\b'
                  r'|`[^`\n]*(?:<=|>=|<|>|!=)[^`\n]*`)')
# `loop (:N)`, `loop at :N`, but not `before the loop (:N)` / `after the loop`.
LOOP_BEFORE = re.compile(r'(?<!before the )(?<!after the )(?<!Before the )'
                         r'(?<!After the )loop[^.;:`]{0,12}$')


def resolve(name, lean):
    cands = BY_NAME.get(name, [])
    if len(cands) == 1:
        return cands[0]
    arch = next((v for k, v in ARCH.items()
                 if os.path.basename(lean) == k + '.lean'), None)
    for c in cands:
        if arch and c.startswith('src/%s/' % arch):
            return c
    return None


def citations(skip=()):
    files = sorted(glob.glob(os.path.join(LEAN, '**', '*.lean'),
                             recursive=True))
    for f in files:
        if os.path.basename(f) in skip:
            continue
        txt = open(f).read()
        cur = None
        for a, b in comments(txt):
            seg = txt[a:b]
            toks = sorted([(m.start(), 'p', m) for m in PATH.finditer(seg)] +
                          [(m.start(), 'n', m) for m in NUM.finditer(seg)],
                          key=lambda t: t[0])
            for pos, kind, m in toks:
                if kind == 'p':
                    cur = m.group(1) or resolve(m.group(2), f)
                    continue
                if cur is None:
                    continue  # a number with no file named before it
                sep = m.group(0)[0]
                if sep in '/, ' and not CHAIN.search(seg[:pos]):
                    continue  # `0/1`: not a list of lines
                if sep == ',' and not LINES.search(seg[:pos]):
                    continue  # `(:72, 8, 6)`: a line, then two counts
                lo = int(m.group(1))
                hi = int(m.group(2)) if m.group(2) else lo
                after = seg[m.end():m.end() + 60]
                before = seg[max(0, pos - 40):pos]
                loop = bool(COND.match(after) or LOOP_BEFORE.search(before)
                            or re.match(r'^[\s(]*`(?:while|for)\b', after))
                yield dict(lean=os.path.relpath(f, HERE),
                           line=txt.count('\n', 0, a + pos) + 1,
                           file=cur, lo=lo, hi=hi, loop=loop)


def main():
    quiet = '--quiet' in sys.argv
    skip = [a for i, a in enumerate(sys.argv)
            if i > 0 and sys.argv[i - 1] == '--skip']
    bad = n = nloop = 0
    cache = {}
    for c in citations(skip):
        n += 1
        path = os.path.join(ROOT, c['file'])
        if not os.path.exists(path):
            print('MISSING FILE %s:%d %s' % (c['lean'], c['line'], c['file']))
            bad += 1
            continue
        lines = cache.setdefault(path, open(path).read().split('\n'))
        rng = '%d' % c['lo'] + ('-%d' % c['hi'] if c['hi'] != c['lo'] else '')
        texts = []
        for k in sorted({c['lo'], c['hi']}):
            if k > len(lines) or k < 1:
                print('PAST END %s:%d %s:%d' % (c['lean'], c['line'],
                                                c['file'], k))
                bad += 1
                texts.append('<past end>')
            else:
                texts.append(lines[k - 1].strip())
        status = ''
        if c['loop']:
            nloop += 1
            if re.search(r'\b(?:while|for)\b', texts[0]):
                status = '[loop ok] '
            else:
                status = '[LOOP FAIL] '
                bad += 1
        if not quiet or status.startswith('[LOOP FAIL'):
            print('%s%s:%d  %s:%s' % (status, c['lean'], c['line'],
                                      c['file'], rng))
            for t in texts:
                print('    | ' + t[:110])
    print('%d citations, %d of loop tests, %d failures' % (n, nloop, bad))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
