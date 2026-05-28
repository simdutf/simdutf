#!/usr/bin/env python3
"""Reflow prose paragraphs in README.md onto single lines.

Doxygen 1.9.x emits a separate <p> for every hard newline inside a paragraph,
which makes the API page (api/) show odd breaks mid-paragraph. CommonMark also
treats soft line breaks as spaces, so reflowing is the right canonical form on
GitHub as well.

Only prose paragraphs are touched: fenced code blocks, indented code blocks,
headers, list items, blockquotes, tables, and raw HTML are left alone.
"""

import re
import sys
import pathlib

LIST_RE = re.compile(r"^[ \t]*([-*+]\s+|\d+\.\s+)")
HTML_RE = re.compile(r"^[ \t]*<")
HEADER_RE = re.compile(r"^[ \t]*#")
TABLE_RE = re.compile(r"^[ \t]*\|")
QUOTE_RE = re.compile(r"^[ \t]*>")
INDENT_CODE_RE = re.compile(r"^    ")  # 4-space indent => code block per CommonMark
FENCE_RE = re.compile(r"^[ \t]*```")


def is_breaking(line: str) -> bool:
    """A line that ends the current paragraph/list-item run."""
    if not line.strip():
        return True
    if FENCE_RE.match(line):
        return True
    if HEADER_RE.match(line):
        return True
    if TABLE_RE.match(line):
        return True
    if QUOTE_RE.match(line):
        return True
    if HTML_RE.match(line):
        return True
    if INDENT_CODE_RE.match(line):
        return True
    return False


def reflow(text: str) -> str:
    lines = text.splitlines(keepends=False)
    out = []
    i = 0
    in_fence = False
    while i < len(lines):
        line = lines[i]
        if FENCE_RE.match(line):
            in_fence = not in_fence
            out.append(line)
            i += 1
            continue
        if in_fence:
            out.append(line)
            i += 1
            continue
        if not line.strip() or HEADER_RE.match(line) or TABLE_RE.match(line) \
                or QUOTE_RE.match(line) or HTML_RE.match(line) or INDENT_CODE_RE.match(line):
            out.append(line)
            i += 1
            continue

        list_m = LIST_RE.match(line)
        if list_m:
            # list item: keep marker + reflow this item's body until next break
            # or until we see another list marker at the same column or earlier.
            marker = list_m.group(0)
            buf = [line.rstrip()]
            j = i + 1
            while j < len(lines):
                nxt = lines[j]
                if is_breaking(nxt):
                    break
                # Stop if next line is itself a top-level list item (not a deeper continuation)
                nxt_list = LIST_RE.match(nxt)
                if nxt_list:
                    break
                buf.append(nxt.strip())
                j += 1
            out.append(" ".join(buf))
            i = j
            continue

        # prose paragraph: collect a run of plain prose lines
        buf = [line.rstrip()]
        j = i + 1
        while j < len(lines):
            nxt = lines[j]
            if is_breaking(nxt) or LIST_RE.match(nxt):
                break
            buf.append(nxt.strip())
            j += 1
        out.append(" ".join(buf))
        i = j
    # preserve trailing newline if input had one
    trailing = "\n" if text.endswith("\n") else ""
    return "\n".join(out) + trailing


def main():
    path = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "README.md")
    src = path.read_text()
    new = reflow(src)
    if new != src:
        path.write_text(new)
        print(f"Reflowed {path}")
    else:
        print(f"No changes for {path}")


if __name__ == "__main__":
    main()
