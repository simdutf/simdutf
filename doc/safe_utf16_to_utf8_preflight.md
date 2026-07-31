# Safe UTF-16 to UTF-8 preflight: residual handoff

Status: draft evidence only. Do not merge this optimization or present it as a
general performance result until the residual below has a sealed green result.

## What is supported

The candidate commit `38d0fc678d9d38bcc36c582a2b483d0e926372f5` is directly
on top of the comparison/harness commit `af4bf61e7076d98a89e6dcf687588f3d14b1f282`.
For the public `convert_utf16_to_utf8_safe` API on Haswell, ASCII input of 256
UTF-16 code units, and exact output capacity, ten alternating paired runs
measured a median of 139.904 ns/op for the comparison and 42.504 ns/op for the
candidate. The paired-median improvement was 97.488 ns/op, with a 95% interval
of 95.783 to 98.962 ns/op.

The focused benchmark validates the selected public implementation against the
fallback implementation, checks output guards, and consumes the timed result.
The candidate also passed the available CMake, sanitizer, and differential
correctness checks. Those results support this narrow ASCII selector only.

## Residual: a missed no-regression path

The candidate takes its new path when the output capacity equals the UTF-16
length, the input is 32 through 4096 code units, and code units at positions
0, `len / 2`, and `len - 1` are ASCII. It then calls the Haswell UTF-16 ASCII
validator before deciding whether to use the direct converter.

At length 256, input that is ASCII except for U+00E9 at index 127 passes the
three cheap samples: positions 0, 128, and 255 are all ASCII. The candidate
therefore enters the vector ASCII preflight, which discovers the interior
non-ASCII code unit and falls through to the old safe-conversion path. That
adds preflight work to a valid public-API workload that receives none of the
ASCII fast-path benefit.

The benchmark selectors do not exercise this state:

- `ascii` takes the new fast path.
- `one-to-three` and `three-to-four` have non-ASCII data at a sampled position.
- `late-three` puts its non-ASCII code unit at `len - 1`, also a sampled
  position.

They either bypass the preflight immediately or take the fast path; none leaves
all three samples ASCII while making the subsequent validation fail.

## Handoff to the next session

Do not change the optimization in this draft. First add a sealed public-API
paired Haswell selector with these properties:

1. 256 UTF-16 code units and output capacity 256.
2. ASCII input except U+00E9 at index 127.
3. Existing fallback-result and output-guard validation before and after the
   timed loop.
4. Alternating comparison/candidate runs under the same build, implementation,
   and selector.
5. An explicit no-regression gate: the paired comparison-minus-candidate median
   and its lower confidence bound must both be at least zero.

If that selector regresses, the next session should repair the preflight shape
and then rerun the narrow ASCII win and this no-regression selector together.
If it passes, it closes this residual only; broader architecture and workload
coverage still need their own claims and measurements.
