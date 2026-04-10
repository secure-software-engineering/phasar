# PTABen Benchmark Tool

A sample tool that can be used to benchmark PhASAR's alias analyses against SVF's [Test Suite](https://github.com/SVF-tools/Test-Suite).

To use this tool, we expect that you have a folder with the compiled LLVM bitcode of the relevant PTABen benchmark files that you care about.
Just provide the path to that folder to `tools/ptaben/ptaben`. It will run the alias analyses on each LLVM bitcode file in that folder (and subfolders) and output a .csv file per alias analysis + one for the ground-truth.

## Supported Analyses

Currently, the benchmark tool supports the following analyses:

- LLVM's CFLAndersen analysis, wrapped into `psr::LLVMAliasSet`
- LLVM's CFLSteensgaard analysis, wrapped into `psr::LLVMAliasSet`
- PhASAR's context-sensitive union-find analysis (same as if passing `--alias-analysis=union-find --union-find-aa=ctx-sens` to `phasar-cli`)
- PhASAR's indirection-sensitive union-find analysis (same as if passing `--alias-analysis=union-find --union-find-aa=ind-sens` to `phasar-cli`)
- PhASAR's bottom-up context-sensitive union-find analysis (same as if passing `--alias-analysis=union-find --union-find-aa=botctx-sens` to `phasar-cli`)
- PhASAR's context-sensitive indirection-sensitive union-find analysis (same as if passing `--alias-analysis=union-find --union-find-aa=ctx-ind-sens` to `phasar-cli`)
- PhASAR's bottom-up context-sensitive indirection-sensitive union-find analysis (same as if passing `--alias-analysis=union-find --union-find-aa=botctx-ind-sens` to `phasar-cli`)

Of course, you can extend this tool and provide your own analyses here.

## Preliminary Results

|           | anders | steens | ctx-sens | bot-sens | ind-sens | ctx-ind-sens | bot-ctx-ind-sens |
|-----------|--------|--------|----------|----------|----------|--------------|------------------|
| precision | 0.712  | 0.709  | 0.803    | 0.800    | 0.681    | 0.803        | 0.800            |
| recall    | 0.961  | 0.975  | 0.929    | 0.667    | 0.960    | 0.929        | 0.667            |
| f1score   | 0.818  | 0.821  | 0.862    | 0.728    | 0.796    | 0.862        | 0.728            |
