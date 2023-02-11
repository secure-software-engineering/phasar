# Breaking Changes

## development HEAD

- `TaintConfig` has been renamed to `LLVMTaintConfig`. For generic code you may want to use the LLVM-independent `TaintConfigBase` CRTP interface instead.
- `LLVMPointsTo*` has been renamed to `LLVMAlias*`
- The ctor of `LLVMAliasSet` now takes the `LLVMProjectIRDB` as pointer instead of a reference to better document that it may capture the IRDB by reference.
- The `PointsToInfo` interface has been replaced by the CRTP interface `AliasInfoBase`. Introduced two type-erased implementation of that interface: `AliasInfo` and `AliasInfoRef`. In most cases you should replace `PointsToInfo*` and `LLVMPointsToInfo*` by `AliasInfoRef`, bzw. `LLVMAliasInfoRef`.
- Introduced a new interface `PointsToInfoBase` and type-erased implementations `PointstoInfo` and `PointsToInfoRef`. Don't confuse them with the old `PointsToInfo`!!! (However, they have different APIs, so you should encounter compilation errors then)

## v1222

- Removed virtual interfaces `CFG<N,F>` and `ICFG<N,F>` and replaced by CRTP interfaces `CFGBase` and `ICFGBase`. Use the concrete types `LLVMBasedICFG` and `LLVMBasedCFG` instead. In template code you can use the type traits `is_crtp_base_of_v` and `is_icfg_v` to check for conformance to the interfaces.
- The `LLVMBasedICFG` now takes the IRDB as pointer instead of a reference to better document that it may capture the IRDB by reference.
- Renamed `ProjectIRDB` to `LLVMProjectIRDB` and added a generic CRTP interface `ProjectIRDBBase` that does not depend on LLVM
- Changed the API of `LLVMProjectIRDB`: The IRDB does no longer link multiple LLVM modules together, i.e. the ctor that reads a module from a file now takes a single filename instead of a vector. If you still want to link multiple LLVM modules together, use LLVM's Linker functionality instead. `ProjecIRDB::getAllModules()` has been removed and `ProjectIRDB::getWPAModule()` has been renamed to `LLVMProjectIRDB::getModule()`.
- The `LLVMProjectIRDB` ctor that takes a raw-pointer to `llvm::Module` does no longer preprocess the module (i.e. attaching metadata IDs to it). You can still explicitly use the `ValueAnnotationPass` to do so manually.
- The type `WholeProgramAnalysis` has been removed. Use `AnalysisController` instead.
- The IFDS and IDE TabulationProblems no longer take all of `LLVMProjectIRDB*`, `LLVMTypeHierarchy*`, `LLVMPointsToInfo*` and `LLVMBasedICFG*` as an argument. Instead, they only get what they need.
- The `IFDSSolver` and `IDESolver` now take an instance of the `ICFGBase` interface as additional argument to their ctor (because the analysis problems now not necessarily store a reference to it anymore).
- The `IDETabulationProblem` is now a base class of `IFDSTabulationProblem` (and not vice versa as it was previously). In their ctors they only take the bare minimum of arguments: The IRDB, the entrypoints and optionally the special zero-value. If the zero-value is not passed in the ctor (as it was previously), it has to be set from within the client analysis' ctor. You may use te new function `initializeZeroValue(d_t)` for this.
