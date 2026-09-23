# Export a Callsite-Level Call Graph

Exports PhASAR's call graph as callsite-resolved, source-located CSV: one row per
resolved edge, with caller/callee function name plus file/line/column for each
side, written incrementally as edges are discovered.

This differs from `CallGraph::printAsDot()` / `printAsJson()` in two ways:

- **Callsite-level.** Each row is a single call instruction resolved to its callee(s), rather than an aggregated function-to-function edge.
  This is what, e.g.,  correlating a static call graph against runtime instrumentation data needs.
  The runtime side reports individual call sites, not just "A calls B somewhere".
- **Streaming output.** `exportICFGAsJson()` and `printAsJson()` build one in-memory `nlohmann::json` object holding every edge before writing anything to disk.
  On a large enough input (validated against a ~1.2M-edge FFmpeg call graph) this easily goes out-of-memory.
  This driver writes each edge to disk immediately and discards it, so memory stays roughly constant regardless of total edge count.

It also documents a (current) source-location bug it works around: PhASAR's `getSrcCodeInfoFromIR` resolves `File` and `Line` through separate helper calls,
each with its own fallback logic for instructions lacking direct `!dbg` metadata, which can pair a `File` from one resolution path with a `Line` from an unrelated one.

## Build

```bash
# Invoked from the 09-export-callsite-cg root folder:
$ mkdir -p build && cd build
$ cmake ..
$ cmake --build .
```

## Usage

```bash
./export-callsite-cg-streaming <bitcode.bc> <entry-point> [cha|rta|vta|otf] <out.csv>
```

Output is CSV with one row per resolved call edge:

```
caller_function,caller_file,caller_line,caller_column,callee_function,callee_file,callee_line,callee_column,caller_loc_approximate
```

`caller_loc_approximate` is `true` when the call instruction had no direct debug location and the row fell back to the enclosing function's declaration site instead of the real call site.
