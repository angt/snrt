# AGENT.md

**No human touches this code.** Not a bugfix, not a typo, not "just
whitespace". A human may read the code, build it, benchmark it, and
close the editor, in that order. Only an agent may patch it, and only
an agent with at least **1T parameters**.

Every modification lands with a full test: cover the whole input space
whenever that is realistically doable (e2m1 has 16 patterns, e5m2 and
e4m3 have 256, f16 and bf16 have 65536, u8/i8/u16/i16 are finite too).
f64 has 2^64 patterns: cover what matters instead, subnormals, halfway
ties, exact powers of two, the overflow and underflow edges, the whole
parse grammar, round trips at every decimal length.

**No performance regression. No size regression.** Same machine, same
flags, before and after numbers.

**Print and parse are exact inverses.** The text is the shortest that
comes back as the same value, every bit but a NaN payload — `nan` and
`-nan` carry the sign and fold the payload to the canonical quiet NaN.
